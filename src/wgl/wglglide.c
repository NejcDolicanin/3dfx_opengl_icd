/*
** wglglide.c -- the Glide side of the window layer: pixel-format tables, resolution
** selection, grSstWinOpen/Close, lose/restore on WM_ACTIVATE, LFB-backed drawable
** buffers for the fullscreen window, the display-driver escapes, and the hardware validate /
** pick-buffer procs.  Ancestors (ref/3dfx-opengl):
** GLIDE3X/SST/sst_export.c (open / close), SRC/DRIVERS/SST/SSTPICK.C (wglGlideSetupDrawBuffers).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wglpriv.h"
#include <glide.h>
#include "icdprotos.h"
#include "glsi.h"

/* functions of other files this one calls or installs in the proc tables (their definitions' signatures) */

typedef GLint (__stdcall *GCfn_winopenext)(FxU32, GLint, GLint, GLint, GLint, GLint, GLint, GLint);

__GLcontext *g_glideOwnerGC;        /* 690f66c0  the gc that owns the fullscreen Glide window */
GLint g_screenWidth, g_screenHeight;/* 690f66bc, 690f66b8 */
static GLint s_lfbPitch0, s_lfbPitch1;   /* 690f66b0, 690f66b4 */

/* -------------------------------------------------------- display escapes */
/* 69014c10 -- ExtEscape 0x3df7: 4-byte query from the 3dfx display driver */
GLint wglDisplayQuery(void)
{
    HDC hdc = GetDC(NULL);
    GLint in = 0, out = 0;
    if (ExtEscape(hdc, 0x3df7, 4, (LPCSTR)&in, 4, (LPSTR)&out) != 1) {
        ReleaseDC(NULL, hdc);
        ICD_T("ExtEscape 3df7 (query) failed", 0);
        return 0;
    }
    ICD_T("ExtEscape 3df7 (query) out", out);
    ReleaseDC(NULL, hdc);
    return out;
}
/* 69014c70 / 69014cc0 -- ExtEscape 0x3df5 / 0x3df6: tell the display driver the Glide window opened / closed */
static __inline void wglDisplayNotify(int code)        /* inlined into both callers in the original */
{
    HDC hdc = GetDC(NULL);
    GLint in = 0, out = 0;
    ExtEscape(hdc, code, 4, (LPCSTR)&in, 4, (LPSTR)&out);
    ReleaseDC(NULL, hdc);
    ICD_T("ExtEscape (3df5 open / 3df6 close) code", code); ICD_T("  out", out);
}
/* 69014c70 -- wglDisplayNotifyOpen: tell the display driver the Glide window opened (ExtEscape 0x3df5) */
void wglDisplayNotifyOpen(void)  { wglDisplayNotify(0x3df5); }     /* 69014c70 */
/* 69014cc0 -- wglDisplayNotifyClose: tell the display driver the Glide window closed (ExtEscape 0x3df6) */
void wglDisplayNotifyClose(void) { wglDisplayNotify(0x3df6); }     /* 69014cc0 */

/* ----------------------------------------------------- device callbacks */
/* the device procs get GetCurrentThreadId() in ECX from their callers; none of them reads it */
BOOL __fastcall wglGlideProcessAttach(DWORD threadId)                                         /* 69014d10 */
{
#ifdef ICD_TRACE
    ICD_T("wglGlideProcessAttach: grGlideInit", threadId);
    grGlideInit();
    ICD_T("wglGlideProcessAttach: grEnable(8)", 0);
    grEnable(8 /* GR_OPENGL_MODE_EXT (g3ext.h) */);
    ICD_T("wglGlideProcessAttach: done", 0);
    return TRUE;
#else
    (void)threadId;
    return ((GLboolean (__fastcall *)(void))hwGlideInit)() != 0;
#endif
}
/* 69014d20 -- wglGlideProcessDetach: device processDetach: nothing to do; TRUE */
BOOL __fastcall wglGlideProcessDetach(DWORD threadId) { (void)threadId; return TRUE; }        /* 69014d20 */
/* 69014d30 -- wglGlideThreadAttach: device threadAttach: select Glide board 0 for the thread; TRUE */
BOOL __fastcall wglGlideThreadAttach(DWORD threadId)  { (void)threadId; grSstSelect(0); return TRUE; } /* 69014d30 */
/* 69014d40 -- describePixelFormat callback: fetches a whole 44-byte table row (PFD + multisample count) and
** copies the 40-byte PIXELFORMATDESCRIPTOR out.  The buffer must hold the whole row (the original: sub esp,0x2c):
** a 40-byte one let wglGlideDescribePixelFormat's 44-byte copy overwrite the return address (Quake 2 crashed at
** EIP 9 in the pixel-format path). */
LONG __fastcall wglGlideDescribePixelFormatCB(HDC hdc, void *out, int index)
{
    DWORD row[11];
    LONG n = wglGlideDescribePixelFormat(hdc, out != NULL ? (PIXELFORMATDESCRIPTOR *)row : NULL, index);
    if (out != NULL) memcpy(out, row, sizeof(PIXELFORMATDESCRIPTOR));
    return n;
}

/* 69014db8 -- the 3dfx pixel formats.  Two tables of 44-byte rows (PIXELFORMATDESCRIPTOR
** + multisample count + pad): 16 bpp at 690eecd8 (4 rows, count 690eed88) and 32 bpp at
** 690eed90 (4 rows, count 690eee40).  Returns the number of formats, or 0 when the
** hardware cannot do GL on this desktop.  index -1 = just count.
*/
typedef struct { PIXELFORMATDESCRIPTOR pfd; GLint samples; } __WGLpfdRow;   /* 44 bytes */
/* 690eecd8 -- 16 bpp: single, double, double+2x FSAA, double+4x FSAA */
const __WGLpfdRow g_pfdTable16[4] = {
    { { 40, 1, 0x34, 0, 16, 5, 11, 6, 5, 5, 0, 0, 0, 64, 16, 16, 16, 16, 16, 8, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 40, 1, 0x35, 0, 16, 5, 11, 6, 5, 5, 0, 0, 0, 64, 16, 16, 16, 16, 16, 8, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 40, 1, 0x35, 0, 16, 5, 11, 6, 5, 5, 0, 0, 0, 64, 16, 16, 16, 16, 16, 8, 0, 0, 0, 0, 0, 0 }, 2 },
    { { 40, 1, 0x35, 0, 16, 5, 11, 6, 5, 5, 0, 0, 0, 64, 16, 16, 16, 16, 16, 8, 0, 0, 0, 0, 0, 0 }, 4 },
};
/* 690eed90 -- 32 bpp: same four, ARGB8888 with 24-bit depth */
const __WGLpfdRow g_pfdTable32[4] = {
    { { 40, 1, 0x34, 0, 32, 8, 16, 8, 8, 8, 0, 8, 24, 64, 16, 16, 16, 16, 24, 8, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 40, 1, 0x35, 0, 32, 8, 16, 8, 8, 8, 0, 8, 24, 64, 16, 16, 16, 16, 24, 8, 0, 0, 0, 0, 0, 0 }, 0 },
    { { 40, 1, 0x35, 0, 32, 8, 16, 8, 8, 8, 0, 8, 24, 64, 16, 16, 16, 16, 24, 8, 0, 0, 0, 0, 0, 0 }, 2 },
    { { 40, 1, 0x35, 0, 32, 8, 16, 8, 8, 8, 0, 8, 24, 64, 16, 16, 16, 16, 24, 8, 0, 0, 0, 0, 0, 0 }, 4 },
};
GLint g_pfdCount16 = 4;                 /* 690eed88 */
GLint g_pfdCount32 = 4;                 /* 690eee40 */
/* 69014db8 -- wglGlideDescribePixelFormat: the 16 bpp table, or the 32 bpp one on a 32 bpp desktop when the PIXEXT
** extension is present; Voodoo4 drops the 4x FSAA row, Banshee / Voodoo3 both FSAA rows.  No formats (0) on a
** Banshee / Voodoo3 with under 16 MB and a desktop above 1024x768, or with the SURFACE extension on a desktop under
** 16 bpp.  A valid index copies the whole 44-byte row into out (see the CB above) and returns the count; an
** invalid one returns 0, or the count when out is NULL */
GLint __fastcall wglGlideDescribePixelFormat(HDC hdc, PIXELFORMATDESCRIPTOR *out, int index)
{
    const __WGLpfdRow *table = g_pfdTable16;
    GLint count = g_pfdCount16;
    int bpp;
    const char *hw;
    RECT desk;

    bpp = GetDeviceCaps(hdc, BITSPIXEL);
    hw = (const char *)grGetString(GR_HARDWARE);
    if (strstr(hw, "Voodoo Banshee") != NULL || strstr(hw, "Voodoo3") != NULL) {
        GLuint mem;
        if (grGetInteger(GR_MEMORY_UMA) != 0) mem = grGetInteger(GR_MEMORY_UMA);   /* unified memory, else the frame buffer */
        else                                  mem = grGetInteger(GR_MEMORY_FB);
        if (mem < 0x1000000) {                                  /* < 16 MB: no GL above 1024x768 */
            GetWindowRect(GetDesktopWindow(), &desk);
            if (desk.right > 0x400 && desk.bottom > 0x300) return 0;
        }
    }
    if (hwHasSurfaceExt() && bpp < 16) return 0;                  /* SURFACE ext present but 8-bpp desktop */
    if (bpp == 32 && hwHasPixextExt()) { table = g_pfdTable32; count = g_pfdCount32; }
    if (strstr(hw, "Voodoo4") != NULL) count -= 1;             /* no 4x FSAA on one chip */
    if (strstr(hw, "Voodoo Banshee") != NULL || strstr(hw, "Voodoo3") != NULL) count -= 2;   /* no FSAA */
    if (index >= 0 && index < count) {
        if (out != NULL) memcpy(out, &table[index], sizeof(__WGLpfdRow));
        return count;
    }
    return (out != NULL) ? 0 : count;
}

/*
** The ICD's copy of Glide's resolution table, indexed by GrScreenResolution_t.  The original has the 24 standard
** modes (0x00..0x17); the widescreen modes 0x18..0x2c of the widescreen glide3x follow, in the enum order of
** sst1vid.h (src/sdk/glide3x), so e.g. a 1920x800 window matches GR_RESOLUTION_1920x800 exactly.
*/
typedef struct { GLint code, width, height; } __WGLresEntry;
const __WGLresEntry g_resTable[] = {                         /* 690eee88 in the original (24 rows + sentinel) */
    { 0, 320, 200 }, { 1, 320, 240 }, { 2, 400, 256 }, { 3, 512, 384 }, { 4, 640, 200 }, { 5, 640, 350 },
    { 6, 640, 400 }, { 7, 640, 480 }, { 8, 800, 600 }, { 9, 960, 720 }, { 10, 856, 480 }, { 11, 512, 256 },
    { 12, 1024, 768 }, { 13, 1280, 1024 }, { 14, 1600, 1200 }, { 15, 400, 300 }, { 16, 1152, 864 },
    { 17, 1280, 960 }, { 18, 1600, 1024 }, { 19, 1792, 1344 }, { 20, 1856, 1392 }, { 21, 1920, 1440 },
    { 22, 2048, 1536 }, { 23, 2048, 2048 },
    /* widescreen (not in the original) */
    { 0x18, 1280, 720 }, { 0x19, 1280, 800 }, { 0x1a, 1360, 768 }, { 0x1b, 1440, 900 }, { 0x1c, 1600, 900 },
    { 0x1d, 1680, 720 }, { 0x1e, 1680, 1050 }, { 0x1f, 1792, 768 }, { 0x20, 1920, 800 }, { 0x21, 1920, 1080 },
    { 0x22, 1920, 1200 }, { 0x23, 1960, 840 }, { 0x24, 2096, 900 }, { 0x25, 2304, 960 }, { 0x26, 2560, 1080 },
    { 0x27, 768, 480 }, { 0x28, 960, 600 }, { 0x29, 1064, 600 }, { 0x2a, 1144, 480 }, { 0x2b, 1400, 600 },
    { 0x2c, 3840, 2160 },
    { 0xff, 0, 0 } };
#define RES_TABLE_ROWS  (sizeof(g_resTable) / sizeof(g_resTable[0]) - 1)    /* without the sentinel */

/* 69014f2f -- pick the GrScreenResolution_t for the window's client size: exact match
** first, else the smallest supported mode that contains the window.  0xff = none.
** (Bugdom is forced to 640x480.)  The gc's imports are used for the temp buffer and size.
** Modes Glide reports past the table's last row are skipped (the original indexes the table unchecked).
*/
GLint __fastcall wglGlideChooseResolution(__GLcontext *gc)
{
    typedef void *(__fastcall *ImpCalloc)(__GLcontext *, size_t, size_t);   /* imports.calloc(gc, n, size) */
    typedef void  (__fastcall *ImpFree)(__GLcontext *, void *);             /* imports.free(gc, p) */
    GrResolution query, *list;
    GLint count, i, result;
    GLuint w, h;
    HWND hwnd;
    HMODULE hmod;
    CHAR name[256];

    query.resolution = GR_QUERY_ANY;
    query.refresh = GR_REFRESH_60Hz;
    query.numColorBuffers = 2;
    query.numAuxBuffers = 1;
    count = (GLuint)grQueryResolutions(&query, NULL) >> 4;
    list = (GrResolution *)((ImpCalloc)GC_imports_calloc(gc))(gc, 16, count);
    grQueryResolutions(&query, list);
    hwnd = GC_imports_wscx(gc)->window->hwnd;
    hmod = (HMODULE)GetWindowLongA(hwnd, GWL_HINSTANCE);
    GetModuleFileNameA(hmod, name, 0xff);
    if (strstr(name, "bugdom") == NULL && strstr(name, "BUGDOM") == NULL) {
        GC_imports_getDrawableSize(gc)(gc, (GLint *)&w, (GLint *)&h, &w, &h);   /* (the origin goes to w/h too, then the size) */
    } else {
        w = 640; h = 480;
    }
    ICD_T("wglGlideChooseResolution window width", w); ICD_T("  height", h); ICD_T("  glide resolutions", count);
    result = 0xff;
    for (i = 0; i < count; i++) {
        if ((GLuint)list[i].resolution >= RES_TABLE_ROWS) continue;
        if (w == (GLuint)g_resTable[list[i].resolution].width && h == (GLuint)g_resTable[list[i].resolution].height) {
            result = g_resTable[list[i].resolution].code;
            if (result != 0xff) goto done;
            break;
        }
    }
    for (i = 0; i < count; i++) {
        if ((GLuint)list[i].resolution >= RES_TABLE_ROWS) continue;
        if (w <= (GLuint)g_resTable[list[i].resolution].width && h <= (GLuint)g_resTable[list[i].resolution].height) {
            result = g_resTable[list[i].resolution].code;
            break;
        }
    }
done:
    ((ImpFree)GC_imports_free(gc))(gc, list);
    return result;
}

/* 690150e3 -- open the Glide screen on the window (parent window for child windows) */
GLint __fastcall wglGlideWinOpen(__GLcontext *gc, GLint resolution)
{
    HWND hwnd = GC_imports_wscx(gc)->window->hwnd;
    GLint ctx;
    if (GetWindowLongA(hwnd, GWL_STYLE) & WS_CHILD) hwnd = GetParent(GC_imports_wscx(gc)->window->hwnd);
    if (gc->hwcx.grSstWinOpenExt != NULL) {
        GLint samples = gc->fx.numSamples;
        GLint pixfmt;
        const char *s = getenv("__GL_SAMPLEBUFFERS");
        if (s != NULL) samples = atoi(s);
        if (gc->hwcx.has32bpp == 0) {
            switch (samples) {
            case 2:  pixfmt = 6; break;     /* GR_PIXFMT_AA_2_RGB_565 */
            case 4:  pixfmt = 9; break;     /* GR_PIXFMT_AA_4_RGB_565 */
            default: pixfmt = 3; break;     /* GR_PIXFMT_RGB_565 */
            }
        } else {
            switch (samples) {
            case 2:  pixfmt = 8; break;     /* GR_PIXFMT_AA_2_ARGB_8888 */
            case 4:  pixfmt = 11; break;    /* GR_PIXFMT_AA_4_ARGB_8888 */
            default: pixfmt = 5; break;     /* GR_PIXFMT_ARGB_8888 */
            }
        }
        ctx = ((GCfn_winopenext)gc->hwcx.grSstWinOpenExt)
                ((FxU32)hwnd, resolution, GR_REFRESH_60Hz, 0, 0, pixfmt, 2, 1);
    } else {
        ctx = grSstWinOpen((FxU32)hwnd, resolution, GR_REFRESH_60Hz, GR_COLORFORMAT_ARGB /*0*/, GR_ORIGIN_UPPER_LEFT /*0*/, 2, 1);
    }
    ICD_T("wglGlideWinOpen grSstWinOpen ctx (0 = failed)", ctx);
    if (ctx == 0) return -1;
    wglDisplayNotifyOpen();
    return ctx;
}

/* 690151e0 -- close the Glide screen */
void __fastcall wglGlideWinClose(__GLcontext *gc)
{
    ICD_T("wglGlideWinClose ctx", gc->hwcx.glideContext); ICD_T("  LFB locks held", ICD_LFB_N);
    grSstWinClose(gc->hwcx.glideContext);
    wglDisplayNotifyClose();
}

/* 69015553 -- window->swapBuffers for the fullscreen window: just make sure Glide is selected.
** Returns a GLboolean (mov al,1), as the callers (drv.c, wglstub.c, wglstand.c) read it. */
GLboolean __fastcall wglGlideWindowSwap(__WGLwindow *w)
{
    __GLcontext *gc = g_glideOwnerGC;
    GLint *open = &g_glideOwnerGC->fx.glideOpen;          /* (no NULL check, as in the original) */

    (void)w;
    if (*open != 0) {
        if (grSelectContext(g_glideOwnerGC->hwcx.glideContext) == 0) {
            wglGlideTrace("Lost Glide context");
            ICD_T("wglGlideWindowSwap: Glide context lost", g_glideOwnerGC->hwcx.glideContext);
            wglGlideWinClose(gc);
            *open = 0;
        }
    }
    return GL_TRUE;
}
/* 690155b0 -- wglGlideTrace: the trace printf, compiled out (empty) */
void __cdecl wglGlideTrace(const char *fmt, ...) { (void)fmt; }     /* 690155b0 empty trace */

/* 690155c0 -- free the fullscreen window's buffers (front, back, depth: the set 69015200 installs).  Unreferenced
** in the original. */
void __fastcall wglGlideFreeBuffers(__GLcontext *gc)
{
    __WGLdrawable *dp = &GC_imports_wscx(gc)->window->dp;

    if (dp->frontBuffer.free) {
        (*dp->frontBuffer.free)(&dp->frontBuffer, dp);
    }
    if (dp->backBuffer.free) {
        (*dp->backBuffer.free)(&dp->backBuffer, dp);
    }
    if (dp->depthBuffer.free) {
        (*dp->depthBuffer.free)(&dp->depthBuffer, dp);
    }
}

/* 69015619 -- open the fullscreen Glide window for a gc: choose the mode, open, record
** the screen size (gc->maxViewport = screen), hook the window swap.  -1 on failure.
*/

GLint __fastcall wglGlideOpen(__GLcontext *gc)
{
    __WGLwindow *w = GC_imports_wscx(gc)->window;
    GLint res, ctx;

    if (hwHasSurfaceExt()) {                     /* SURFACE ext: drop any windowed surfaces first */
        winFreeDrawableBuffers(w);
        wglGlideInitLFBWindow(w, (__WGLmodes *)&gc->modes);  /* 69015200 */
    }
    grSstSelect(0);
    res = wglGlideChooseResolution(gc);
    ICD_T("wglGlideOpen resolution (0xff = none)", res);
    if (res == 0xff) {
        if (getenv("HAL_CSIM") == NULL) goto fail;
        res = g_defaultResolution;            /* (690eeedc) under HAL_CSIM */
    }
    wglUnlock();                              /* the caller holds the lock: open the screen without it */
    if (gc->fx.glideOpen == 0) ctx = wglGlideWinOpen(gc, res);
    else ctx = gc->hwcx.glideContext;
    ICD_T("wglGlideOpen context (-1 = failed)", ctx);
    if (ctx == -1) {
        wglLock();
        goto fail;
    }
    g_screenWidth  = gc->constants.maxViewportWidth  = g_resTable[res].width;
    g_screenHeight = gc->constants.maxViewportHeight = g_resTable[res].height;
    ICD_T("wglGlideOpen screen width", g_screenWidth); ICD_T("  screen height", g_screenHeight);
    gc->hwcx.resolution = res;
    g_glideOwnerGC = gc;
    w->swapBuffers = (GLuint (__fastcall *)(__WGLwindow *))wglGlideWindowSwap;   /* returns a GLboolean */
    gc->fx.glideOpen = 1;
    wglLock();
    grSelectContext(ctx);
    return ctx;
fail:
    return -1;
}
GLint g_defaultResolution = 7;          /* 690eeedc  GR_RESOLUTION_640x480: 7 in the original (= g_resTable[7].code,
                                           whose row {7, 640, 480} it is) */

/* 69015710 -- close the fullscreen window (loseCurrent path) */
void __fastcall wglGlideClose(__GLcontext *gc)
{
    __WGLwindow *w = GC_imports_wscx(gc)->window;
    if (gc->fx.glideOpen != 0) {
        wglGlideWinClose(gc);
        gc->fx.glideOpen = 0;
    }
    hwInitDDrawWindow(w, (__WGLmodes *)&gc->modes);
    g_glideOwnerGC = NULL;
}

/* 69015750 -- WM_ACTIVATE(WA_INACTIVE): give the screen back */
void __fastcall wglGlideLose(HWND hwnd)
{
    GLint *open;
    if (g_glideOwnerGC != NULL && g_glideOwnerGC != g_nullGC) {
        open = &g_glideOwnerGC->fx.glideOpen;
        if (*open != 0 && hwnd == GC_imports_wscx(g_glideOwnerGC)->window->hwnd) {
            ICD_T("wglGlideLose (WM_ACTIVATE inactive)", hwnd);
            wglGlideWinClose(g_glideOwnerGC);
            *open = 0;
        }
    }
}

/* 690157a1 -- WM_ACTIVATE(WA_ACTIVE): "Restoring glide" */
void __fastcall wglGlideRestore(HWND hwnd)
{
    __GLcontext *fx = g_glideOwnerGC;
    GLint *open;
    if (g_glideOwnerGC != NULL && g_glideOwnerGC != g_nullGC) {
        open = &g_glideOwnerGC->fx.glideOpen;
        if (*open == 0 && hwnd == GC_imports_wscx(g_glideOwnerGC)->window->hwnd) {
            __GLcontext *gc = g_glideOwnerGC;
            __GLtexMgrICD *texMgr;
            GLint ctx;
            wglGlideTrace("Restoring glide\n");
            ICD_T("wglGlideRestore (WM_ACTIVATE active)", hwnd);
            texMgr = gc->texMgr;
            ctx = fx->hwcx.glideContext = wglGlideWinOpen(gc, fx->hwcx.resolution);   /* the mode it had */
            if (ctx != 0 && ctx != -1) {
                wglReloadGammaRamp();
                (*texMgr->reset)(gc, gc->texMgr);                          /* 6903b7a0: rebuild the texture heaps */
                hwDirtyAllGrState(gc);                                          /* every Glide state record dirty */
                ((void (__fastcall *)(__GLcontext *))hwFlushGrState)(gc);   /* EDX not set */
                *open = 1;
                return;
            }
            *open = 0;
        }
    }
}

/* ------------------------------------------------- LFB-backed drawable buffers */
/* the drawable buffer behind a colour buffer is buf.wglBuf; its readlock / writelock flags (SSTPICK.C) select
** the LFB locks lfbLock / lfbUnlock take */

/* the 0x24-byte private of an LFB buffer (dp->calloc(0x24, 1)); only the depth buffer uses the tail */
typedef struct LfbPrivRec {
    GLuint  unk_00[6];
    void   *savedBase;              /* +0x18  depth: buf->base outside the LFB lock */
    void   *lock;                   /* +0x1c  depth: lfbDepthLock */
    void   *unlock;                 /* +0x20  depth: lfbDepthUnlock */
} LfbPriv;
/* the colour buffer living in the Glide frame buffer (LFB lock on demand): its procs, then wglGlideInitLFBColor */
static GLubyte __fastcall lfbResize(__WGLbuffer *buf, GLint x, GLint y, GLuint w, GLuint h, __WGLdrawable *dp, GLuint mask)
{ (void)buf; (void)x; (void)y; (void)w; (void)h; (void)dp; (void)mask; return 1; }          /* 690152e0 */
/* 690152f0 -- lfbLock: grLfbLock the buffer (front if it is dp->frontBuffer, else back), read-only if readlock and
** write-only if writelock, and point base / byteWidth / outerWidth at the LFB (the last lock's info); a failed
** lock returns with the buffer unchanged */
static void __fastcall lfbLock(__WGLbuffer *buf, __WGLdrawable *dp)                         /* 690152f0 */
{
    GrLfbInfo_t info;
    GrBuffer_t which = (&dp->frontBuffer != buf);          /* front = 0, back = 1 */
    info.size = sizeof(info);
    if (buf->readlock && !grLfbLock(GR_LFB_READ_ONLY, which, GR_LFBWRITEMODE_ANY, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) return;
    ICD_LFB(buf->readlock ? 1 : 0);
    info.size = sizeof(info);
    if (buf->writelock && !grLfbLock(GR_LFB_WRITE_ONLY, which, GR_LFBWRITEMODE_ANY, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) return;
    ICD_LFB(buf->writelock ? 1 : 0);
    buf->base = info.lfbPtr;
    buf->byteWidth = info.strideInBytes;
    buf->outerWidth = info.strideInBytes / buf->elementSize;
}
/* 69015380 -- lfbUnlock: release lfbLock's read / write locks on the front or back buffer and clear base and
** byteWidth */
static void __fastcall lfbUnlock(__WGLbuffer *buf, __WGLdrawable *dp)                       /* 69015380 */
{
    GrBuffer_t which = (&dp->frontBuffer != buf);
    if (buf->readlock) grLfbUnlock(GR_LFB_READ_ONLY, which);
    if (buf->writelock) grLfbUnlock(GR_LFB_WRITE_ONLY, which);
    ICD_LFB(-((buf->readlock ? 1 : 0) + (buf->writelock ? 1 : 0)));
    buf->base = NULL;
    buf->byteWidth = 0;
}
/* 690153c0 -- lfbFree: free the LFB colour buffer's private and clear buf->free */
static void __fastcall lfbFree(__WGLbuffer *buf, __WGLdrawable *dp)                         /* 690153c0 */
{
    dp->free(buf->private_);
    buf->free = NULL;        /* sic: original clears slot 0x3c */
}

/* 69015260 -- wglGlideInitLFBColor: set buf up as a colour buffer of the given depth in the LFB (the lfb* procs) */
void __fastcall wglGlideInitLFBColor(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth, int unused)
{
    (void)unused;
    buf->depth = depth;
    buf->height = 0; buf->width = 0; buf->base = NULL; buf->handle = NULL; buf->size = 0; buf->byteWidth = 0;
    buf->elementSize = ((depth - 1) / 8) + 1;
    buf->elementSizeLog2 = wglLog2(buf->elementSize);
    buf->resize = lfbResize; buf->lock = lfbLock; buf->unlock = lfbUnlock; buf->fill = NULL; buf->free = lfbFree;
    buf->mainInit = NULL; buf->fallbackInit = NULL;
    buf->private_ = dp->calloc(0x24, 1);                  /* the 0x24-byte LFB buffer private */
}
/* the depth buffer in the LFB (aux buffer), with pitch cache: its procs, then wglGlideInitLFBDepth */
static GLubyte __fastcall lfbDepthResize(__WGLbuffer *buf, GLint x, GLint y, GLuint w, GLuint h, __WGLdrawable *dp, GLuint mask)
{ (void)x; (void)y; (void)dp; (void)mask; buf->width = w; buf->height = h; return 1; }       /* 69015480 */
/* 690154a0 -- lfbDepthLock: keep base in the private's savedBase, then grLfbLock the aux (depth) buffer read-only
** and point base / byteWidth / outerWidth at it; on failure base stays as it was */
static void __fastcall lfbDepthLock(__WGLbuffer *buf, __WGLdrawable *dp)                    /* 690154a0 */
{
    GrLfbInfo_t info;
    (void)dp;
    ((LfbPriv *)buf->private_)->savedBase = buf->base;
    info.size = sizeof(info);
    if (grLfbLock(GR_LFB_READ_ONLY, GR_BUFFER_AUXBUFFER, GR_LFBWRITEMODE_ANY, GR_ORIGIN_UPPER_LEFT, FXFALSE, &info)) {
        ICD_LFB(1);
        buf->base = info.lfbPtr;
        buf->byteWidth = info.strideInBytes;
        buf->outerWidth = info.strideInBytes / buf->elementSize;
    }
}
/* 690154f0 -- lfbDepthUnlock: grLfbUnlock the aux (depth) buffer and restore base from the private's savedBase */
static void __fastcall lfbDepthUnlock(__WGLbuffer *buf, __WGLdrawable *dp)                  /* 690154f0 */
{
    LfbPriv *pv = (LfbPriv *)buf->private_;
    (void)dp;
    grLfbUnlock(GR_LFB_READ_ONLY, GR_BUFFER_AUXBUFFER);
    ICD_LFB(-1);
    buf->base = pv->savedBase;
}
/* 69015510 -- lfbDepthFree: free the private and buf->base, and clear buf->free */
static void __fastcall lfbDepthFree(__WGLbuffer *buf, __WGLdrawable *dp)                    /* 69015510 */
{
    dp->free(buf->private_);
    dp->free(buf->base);
    buf->free = NULL;
}

/* 690153e0 -- wglGlideInitLFBDepth: set buf up as the depth buffer in the LFB (the lfbDepth* procs) */
void __fastcall wglGlideInitLFBDepth(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth, int unused)
{
    LfbPriv *pv;
    (void)unused;
    buf->depth = depth;
    buf->width = dp->width; buf->height = dp->height;
    buf->base = NULL; buf->handle = NULL; buf->size = 0; buf->byteWidth = 0;
    buf->elementSize = ((depth - 1) / 8) + 1;
    buf->elementSizeLog2 = wglLog2(buf->elementSize);
    buf->resize = lfbDepthResize; buf->lock = lfbDepthLock; buf->unlock = lfbDepthUnlock; buf->fill = NULL; buf->free = lfbDepthFree;
    buf->mainInit = NULL; buf->fallbackInit = NULL;
    pv = (LfbPriv *)dp->calloc(0x24, 1);                     /* the 0x24-byte depth private (original: ecx = 0x24, edx = 1) */
    buf->private_ = pv;
    buf->base = NULL;
    s_lfbPitch0 = 0; s_lfbPitch1 = 0;
    pv->lock = (void *)lfbDepthLock;
    pv->unlock = (void *)lfbDepthUnlock;
}
/* 69015200 -- fullscreen window: front/back in the LFB, depth in the aux buffer */
void __fastcall wglGlideInitLFBWindow(__WGLwindow *w, const __WGLmodes *m)
{
    __WGLdrawable *dp = &w->dp;
    w->swapBuffers = (GLuint (__fastcall *)(__WGLwindow *))wglGlideWindowSwap;   /* returns a GLboolean */
    /* the original passes modes+0x2c (named indexBits here; the ICD keeps the colour depth there too) */
    wglGlideInitLFBColor(&dp->frontBuffer, dp, m->indexBits, 0);
    if (m->doubleBufferMode) wglGlideInitLFBColor(&dp->backBuffer, dp, m->indexBits, 0);
    if (m->haveDepthBuffer)  wglGlideInitLFBDepth(&dp->depthBuffer, dp, m->depthBits, 0);
}

/* 6904d4c0 -- device->initWindow: windowed surfaces (SURFACE extension) or the LFB window, plus the
** memory accumulation and stencil buffers (dp->windowRect = window+0x270, the 0x14-byte window rectangle) */
void __fastcall wglGlideInitWindow(__WGLwindow *w, const __WGLmodes *m)
{
    __WGLdrawable *dp = &w->dp;
    GLint accumBits = m->accumRedBits + m->accumGreenBits + m->accumBlueBits + m->accumAlphaBits;

    dp->windowRect = (__WGLwindowRect *)(*dp->calloc)(sizeof(__WGLwindowRect), 1);
    dp->yInverted = 1;
    w->dp.updateClipRegion = (void (__fastcall *)(__WGLdrawable *))hwUpdateClipRegion;
    if (hwHasSurfaceExt()) {
        hwInitDDrawWindow(w, m);
    } else {
        wglGlideInitLFBWindow(w, m);
    }
    if (m->haveAccumBuffer) {
        wglMemInit(&dp->accumBuffer, dp, accumBits);
    }
    if (m->haveStencilBuffer) {
        wglMemInit(&dp->stencilBuffer, dp, m->stencilBits);
    }
}
/* 6904d560 -- wglGlideDestroyWindow: device->destroyWindow: under the drawable's lock, each buffer's free proc and
** the window rectangle block */
void __fastcall wglGlideDestroyWindow(__WGLwindow *w)
{
    __WGLdrawable *dp = &w->dp;
    wglCSEnter(dp->lock);
    if (dp->frontBuffer.free)   dp->frontBuffer.free(&dp->frontBuffer, dp);
    if (dp->backBuffer.free)    dp->backBuffer.free(&dp->backBuffer, dp);
    if (dp->accumBuffer.free)   dp->accumBuffer.free(&dp->accumBuffer, dp);
    if (dp->depthBuffer.free)   dp->depthBuffer.free(&dp->depthBuffer, dp);
    if (dp->stencilBuffer.free) dp->stencilBuffer.free(&dp->stencilBuffer, dp);
    if (dp->windowRect != NULL) dp->free(dp->windowRect);
    wglCSLeave(dp->lock);
}

/* ---------------------------------------------- hw-path validate / buffer modes */
/* 69015c40 -- gc->procs validate (hw): */
void __fastcall wglGlideValidate(__GLcontext *gc)
{
    (*gc->procs.pickAllProcsPhase2)(gc);
    if (gc->hwcx.swRendering != 0) { (*gc->procs.validate)(gc); return; }   /* switched to software meanwhile */
    ((void (__fastcall *)(__GLcontext *))hwFlushGrState)(gc);   /* flush the Glide state records; EDX not set */
}
/* 69015c70 -- validate (sw): phase-2 pick, revalidate; procs.validate if back on hw, else self-copy the dispatch */
void __fastcall wglGlideValidateSW(__GLcontext *gc)
{
    (*gc->procs.pickAllProcsPhase2)(gc);
    ((void (__fastcall *)(__GLcontext *))__glSSTRevalidate)(gc);   /* revalidate; EDX not set */
    if (gc->hwcx.swRendering == 0) { (*gc->procs.validate)(gc); return; }   /* back on the hardware meanwhile */
    __glCopyDispatchTable(&__glCurrentGC()->currentDispatchState, &__glCurrentGC()->currentDispatchState);
}
/* 69015cd0 -- the polygon face / cull part of __glGenericPickTriangleProcs (s_pick.c): polygon.cullFace and
** polygon.face[] from glCullFace / glFrontFace.  Unlike the SI, GL_FRONT_AND_BACK does not return early. */
void __fastcall wglGlideSetupBufferModes(__GLcontext *gc)
{
    if (gc->state.enables.general & __GL_CULL_FACE_ENABLE) {
        switch (gc->state.polygon.cull) {
        case GL_FRONT: gc->polygon.cullFace = 0; break;                  /* __GL_CULL_FLAG_FRONT */
        case GL_BACK:  gc->polygon.cullFace = 1; break;                  /* __GL_CULL_FLAG_BACK */
        case GL_FRONT_AND_BACK:
            gc->procs.renderTriangle = (__GLproc)__glDontRenderTriangle;           /* __glDontRenderTriangle */
            gc->procs.fillTriangle = 0;
            break;
        }
    } else {
        gc->polygon.cullFace = 2;                                        /* __GL_CULL_FLAG_DONT */
    }
    /* face[__GL_CW], face[__GL_CCW]: __GL_FRONTFACE 0 / __GL_BACKFACE 1 */
    switch (gc->state.polygon.frontFaceDirection) {
    case GL_CW:
        if (gc->constants.yInverted) { gc->polygon.face[0] = 1; gc->polygon.face[1] = 0; gc->dirtyMask |= __GL_DIRTY_FACE_ICD; return; }
        gc->polygon.face[0] = 0; gc->polygon.face[1] = 1;
        break;
    case GL_CCW:
        if (!gc->constants.yInverted) { gc->polygon.face[0] = 1; gc->polygon.face[1] = 0; gc->dirtyMask |= __GL_DIRTY_FACE_ICD; return; }
        gc->polygon.face[0] = 0; gc->polygon.face[1] = 1;
        break;
    }
    gc->dirtyMask |= __GL_DIRTY_FACE_ICD;
}
/* 69015da0 -- texture object: pack the filter and wrap modes into the object's hardware key
** (params.hwKey, texture +8): minFilter(3 bits) magFilter wrapS wrapT */
void __fastcall wglGlideTexKey(__GLcontext *gc, BYTE *texObj)
{
    /* each field ORs in its code; the 0 codes (GL_NEAREST, GL_REPEAT) are still cases of their own */
    __GLtextureICD *tex = (__GLtextureICD *)texObj;
    GLuint key = 0;
    (void)gc;
    switch (tex->params.minFilter) {
    case GL_NEAREST:                key |= 0; break;
    case GL_LINEAR:                 key = 1; break;
    case GL_NEAREST_MIPMAP_NEAREST: key = 2; break;
    case GL_LINEAR_MIPMAP_NEAREST:  key = 3; break;
    case GL_NEAREST_MIPMAP_LINEAR:  key = 4; break;
    case GL_LINEAR_MIPMAP_LINEAR:   key = 5; break;
    }
    key <<= 1;
    switch (tex->params.magFilter) {
    case GL_NEAREST: key |= 0; break;
    case GL_LINEAR:  key |= 1; break;
    }
    key <<= 1;
    switch (tex->params.sWrapMode) {
    case GL_CLAMP: case GL_CLAMP_TO_EDGE: key |= 1; break;
    case GL_REPEAT: key |= 0; break;
    }
    key <<= 1;
    switch (tex->params.tWrapMode) {
    case GL_CLAMP: case GL_CLAMP_TO_EDGE: key |= 1; break;
    case GL_REPEAT: key |= 0; break;
    }
    tex->params.hwKey = key;
}

/* 69015e40 -- __glSSTPickBufferProcs (SRC/DRIVERS/SST/SSTPICK.C), procs.pickBufferProcs (hw): the draw and
** read buffers, their lock flags and the buffer lock procs.  ICD additions: GL_FRONT_AND_BACK also sets the
** front buffer's write lock, and the draw buffer goes to the hardware (6901fb60) before its write lock is set. */
void __fastcall wglGlideSetupDrawBuffers(__GLcontext *gc)
{
    GLuint renderBufferMask = 0;
    GLuint drawBufferMask, readBufferMask;   /* no initialisers and empty defaults, as in the original */
    __GLcolorBufferICD *cfb;
    __WGLbuffer *wb;
    __GLbufferMachineICD *buffers;
    buffers = &gc->buffers;
    buffers->doubleStore = GL_FALSE;
    gc->buffers.bufferToLock = NULL;
    if (gc->frontBuffer.buf.wglBuf != NULL) { gc->frontBuffer.buf.wglBuf->readlock = 0; gc->frontBuffer.buf.wglBuf->writelock = 0; }
    if (gc->backBuffer.buf.wglBuf != NULL)  { gc->backBuffer.buf.wglBuf->readlock = 0;  gc->backBuffer.buf.wglBuf->writelock = 0; }
    switch (gc->state.raster.drawBuffer) {
    case GL_FRONT:
        cfb = gc->front; drawBufferMask = __GL_FRONT_BUFFER_MASK; gc->drawBuffer = cfb;
        if (cfb->buf.wglBuf->lock && cfb->buf.wglBuf->unlock) { renderBufferMask = __GL_FRONT_BUFFER_MASK; gc->buffers.bufferToLock = &cfb->buf; }
        break;
    case GL_FRONT_AND_BACK:
        if (gc->modes.doubleBufferMode) {
            gc->drawBuffer = gc->back; drawBufferMask = __GL_FRONT_BUFFER_MASK | __GL_BACK_BUFFER_MASK;
            gc->front->buf.wglBuf->writelock = 1;
            buffers->doubleStore = GL_TRUE;
            wb = gc->front->buf.wglBuf;
            if (wb->lock && wb->unlock) renderBufferMask = __GL_FRONT_BUFFER_MASK;
            wb = gc->back->buf.wglBuf;
            if (wb->lock && wb->unlock) renderBufferMask |= __GL_BACK_BUFFER_MASK;
        } else {
            cfb = gc->front; drawBufferMask = __GL_FRONT_BUFFER_MASK; gc->drawBuffer = cfb;
            if (cfb->buf.wglBuf->lock && cfb->buf.wglBuf->unlock) { renderBufferMask = __GL_FRONT_BUFFER_MASK; gc->buffers.bufferToLock = &cfb->buf; }
        }
        break;
    case GL_BACK:
        cfb = gc->back; drawBufferMask = __GL_BACK_BUFFER_MASK; gc->drawBuffer = cfb;
        if (cfb->buf.wglBuf->lock && cfb->buf.wglBuf->unlock) { renderBufferMask = __GL_BACK_BUFFER_MASK; gc->buffers.bufferToLock = &cfb->buf; }
        break;
    default:
        break;
    }
    ((int (__fastcall *)(__GLcontext *))hwValidateDrawBuffer)(gc);      /* the draw buffer to the hardware (EDX not loaded: still 0 here) */
    gc->drawBuffer->buf.wglBuf->writelock = 1;
    switch (gc->state.pixel.readBuffer) {
    case GL_FRONT: gc->front->buf.wglBuf->readlock = 1; readBufferMask = __GL_FRONT_BUFFER_MASK; break;
    case GL_BACK:  gc->back->buf.wglBuf->readlock = 1;  readBufferMask = __GL_BACK_BUFFER_MASK; break;
    default:       break;
    }
    gc->buffers.lockBuffers = (__GLproc)__glLockBuffer;              /* __glLockBuffers */
    gc->buffers.unlockBuffers = (__GLproc)__glUnlockBuffer;            /* __glUnlockBuffers */
    gc->buffers.lockRenderBuffers = (__GLproc)__glLockRenderBuffers;        /* __glLockRenderBuffers */
    gc->buffers.unlockRenderBuffers = (__GLproc)__glUnlockRenderBuffers;      /* __glUnlockRenderBuffers */
    if ((*gc->polygon.shader.getModeFlags)(gc) & 0x4 /* __GL_SHADE_DEPTH_TEST */)    renderBufferMask |= __GL_DEPTH_BUFFER_MASK;
    if ((*gc->polygon.shader.getModeFlags)(gc) & 0x20 /* __GL_SHADE_STENCIL_TEST */) renderBufferMask |= __GL_STENCIL_BUFFER_MASK;
    /* fast paths: one colour buffer (and the depth buffer) */
    if ((drawBufferMask | __GL_DEPTH_BUFFER_MASK) == renderBufferMask && gc->buffers.doubleStore == GL_FALSE) {
        gc->buffers.lockRenderBuffers = (__GLproc)__glLockColorBufferDepthBuffer;    /* __glLockColorBufferDepthBuffer */
        gc->buffers.unlockRenderBuffers = (__GLproc)__glUnlockColorBufferDepthBuffer;  /* __glUnlockColorBufferDepthBuffer */
    }
    if (drawBufferMask == renderBufferMask && gc->buffers.doubleStore == GL_FALSE) {
        gc->buffers.lockRenderBuffers = (__GLproc)__glLockColorBuffer;    /* __glLockColorBuffer */
        gc->buffers.unlockRenderBuffers = (__GLproc)__glUnlockColorBuffer;  /* __glUnlockColorBuffer */
    }
    gc->buffers.drawBufferMask = drawBufferMask;
    gc->buffers.renderBufferMask = renderBufferMask;
    gc->buffers.readBufferMask = readBufferMask;
}

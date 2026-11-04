/*
** wglddraw.c -- DirectDraw-backed drawable buffers (windowed rendering on the primary
** surface).  Uses DirectDraw 1 interfaces.
*/
#include <stdio.h>
#include "wglpriv.h"

HMODULE             g_hDDraw;          /* 690f665c */
LPDIRECTDRAW        g_ddraw;           /* 690f6660 */
LPDIRECTDRAWSURFACE g_ddPrimary;       /* 690f6664 */
void               *g_ddPrimaryPtr;    /* 690f6668  lpSurface of the primary at init */
int                 g_ddRefs;          /* 690f666c */

extern BOOL wglIsScreenSaverProcess(void);   /* 6900f2cd */

/* per-buffer private block (buf->private_) */
typedef struct { void *lockedPtr; LPDIRECTDRAWCLIPPER clipper; } __WGLddPriv;

static const struct { HRESULT hr; const char *name; } s_ddErrors[] = {     /* 690eea00 */
    { (HRESULT)0x88760005, "DDERR_ALREADYINITIALIZED" },
    { (HRESULT)0x8876000a, "DDERR_CANNOTATTACHSURFACE" },
    { (HRESULT)0x88760014, "DDERR_CANNOTDETACHSURFACE" },
    { (HRESULT)0x88760028, "DDERR_CURRENTLYNOTAVAIL" },
    { (HRESULT)0x88760037, "DDERR_EXCEPTION" },
    { (HRESULT)0x80004005, "DDERR_GENERIC" },
    { (HRESULT)0x8876005a, "DDERR_HEIGHTALIGN" },
    { (HRESULT)0x8876005f, "DDERR_INCOMPATIBLEPRIMARY" },
    { (HRESULT)0x88760064, "DDERR_INVALIDCAPS" },
    { (HRESULT)0x8876006e, "DDERR_INVALIDCLIPLIST" },
    { (HRESULT)0x88760078, "DDERR_INVALIDMODE" },
    { (HRESULT)0x88760082, "DDERR_INVALIDOBJECT" },
    { (HRESULT)0x80070057, "DDERR_INVALIDPARAMS" },
    { (HRESULT)0x88760091, "DDERR_INVALIDPIXELFORMAT" },
    { (HRESULT)0x88760096, "DDERR_INVALIDRECT" },
    { (HRESULT)0x887600a0, "DDERR_LOCKEDSURFACES" },
    { (HRESULT)0x887600aa, "DDERR_NO3D" },
    { (HRESULT)0x887600b4, "DDERR_NOALPHAHW" },
    { (HRESULT)0x887600cd, "DDERR_NOCLIPLIST" },
    { (HRESULT)0x887600d2, "DDERR_NOCOLORCONVHW" },
    { (HRESULT)0x887600d4, "DDERR_NOCOOPERATIVELEVELSET" },
    { (HRESULT)0x887600d7, "DDERR_NOCOLORKEY" },
    { (HRESULT)0x887600dc, "DDERR_NOCOLORKEYHW" },
    { (HRESULT)0x887600de, "DDERR_NODIRECTDRAWSUPPORT" },
    { (HRESULT)0x887600e1, "DDERR_NOEXCLUSIVEMODE" },
    { (HRESULT)0x887600e6, "DDERR_NOFLIPHW" },
    { (HRESULT)0x887600f0, "DDERR_NOGDI" },
    { (HRESULT)0x887600fa, "DDERR_NOMIRRORHW" },
    { (HRESULT)0x887600ff, "DDERR_NOTFOUND" },
    { (HRESULT)0x88760104, "DDERR_NOOVERLAYHW" },
    { (HRESULT)0x88760118, "DDERR_NORASTEROPHW" },
    { (HRESULT)0x88760122, "DDERR_NOROTATIONHW" },
    { (HRESULT)0x88760136, "DDERR_NOSTRETCHHW" },
    { (HRESULT)0x8876013c, "DDERR_NOT4BITCOLOR" },
    { (HRESULT)0x8876013d, "DDERR_NOT4BITCOLORINDEX" },
    { (HRESULT)0x88760140, "DDERR_NOT8BITCOLOR" },
    { (HRESULT)0x8876014a, "DDERR_NOTEXTUREHW" },
    { (HRESULT)0x8876014f, "DDERR_NOVSYNCHW" },
    { (HRESULT)0x88760154, "DDERR_NOZBUFFERHW" },
    { (HRESULT)0x8876015e, "DDERR_NOZOVERLAYHW" },
    { (HRESULT)0x88760168, "DDERR_OUTOFCAPS" },
    { (HRESULT)0x8007000e, "DDERR_OUTOFMEMORY" },
    { (HRESULT)0x8876017c, "DDERR_OUTOFVIDEOMEMORY" },
    { (HRESULT)0x8876017e, "DDERR_OVERLAYCANTCLIP" },
    { (HRESULT)0x88760180, "DDERR_OVERLAYCOLORKEYONLYONEACTIVE" },
    { (HRESULT)0x88760183, "DDERR_PALETTEBUSY" },
    { (HRESULT)0x88760190, "DDERR_COLORKEYNOTSET" },
    { (HRESULT)0x8876019a, "DDERR_SURFACEALREADYATTACHED" },
    { (HRESULT)0x887601a4, "DDERR_SURFACEALREADYDEPENDENT" },
    { (HRESULT)0x887601ae, "DDERR_SURFACEBUSY" },
    { (HRESULT)0x887601b8, "DDERR_SURFACEISOBSCURED" },
    { (HRESULT)0x887601c2, "DDERR_SURFACELOST" },
    { (HRESULT)0x887601cc, "DDERR_SURFACENOTATTACHED" },
    { (HRESULT)0x887601d6, "DDERR_TOOBIGHEIGHT" },
    { (HRESULT)0x887601e0, "DDERR_TOOBIGWIDTH" },
    { (HRESULT)0x80004001, "DDERR_UNSUPPORTED" },
    { (HRESULT)0x887601fe, "DDERR_UNSUPPORTEDFORMAT" },
    { (HRESULT)0x88760208, "DDERR_UNSUPPORTEDMASK" },
    { (HRESULT)0x88760219, "DDERR_VERTICALBLANKINPROGRESS" },
    { (HRESULT)0x8876021c, "DDERR_WASSTILLDRAWING" },
    { (HRESULT)0x88760230, "DDERR_XALIGN" },
    { (HRESULT)0x88760231, "DDERR_INVALIDDIRECTDRAWGUID" },
    { (HRESULT)0x88760232, "DDERR_DIRECTDRAWALREADYCREATED" },
    { (HRESULT)0x88760233, "DDERR_NODIRECTDRAWHW" },
    { (HRESULT)0x88760234, "DDERR_PRIMARYSURFACEALREADYEXISTS" },
    { (HRESULT)0x88760235, "DDERR_NOEMULATION" },
    { (HRESULT)0x88760236, "DDERR_REGIONTOOSMALL" },
    { (HRESULT)0x88760237, "DDERR_CLIPPERISUSINGHWND" },
    { (HRESULT)0x88760238, "DDERR_NOCLIPPERATTACHED" },
    { (HRESULT)0x88760239, "DDERR_NOHWND" },
    { (HRESULT)0x8876023a, "DDERR_HWNDSUBCLASSED" },
    { (HRESULT)0x8876023b, "DDERR_HWNDALREADYSET" },
    { (HRESULT)0x8876023c, "DDERR_NOPALETTEATTACHED" },
    { (HRESULT)0x8876023d, "DDERR_NOPALETTEHW" },
    { (HRESULT)0x8876023e, "DDERR_BLTFASTCANTCLIP" },
    { (HRESULT)0x8876023f, "DDERR_NOBLTHW" },
    { (HRESULT)0x88760240, "DDERR_NODDROPSHW" },
    { (HRESULT)0x88760241, "DDERR_OVERLAYNOTVISIBLE" },
    { (HRESULT)0x88760242, "DDERR_NOOVERLAYDEST" },
    { (HRESULT)0x88760243, "DDERR_INVALIDPOSITION" },
    { (HRESULT)0x88760244, "DDERR_NOTAOVERLAYSURFACE" },
    { (HRESULT)0x88760245, "DDERR_EXCLUSIVEMODEALREADYSET" },
    { (HRESULT)0x88760246, "DDERR_NOTFLIPPABLE" },
    { (HRESULT)0x88760247, "DDERR_CANTDUPLICATE" },
    { (HRESULT)0x88760248, "DDERR_NOTLOCKED" },
    { (HRESULT)0x88760249, "DDERR_CANTCREATEDC" },
    { (HRESULT)0x8876024a, "DDERR_NODC" },
    { (HRESULT)0x8876024b, "DDERR_WRONGMODE" },
    { (HRESULT)0x8876024c, "DDERR_IMPLICITLYCREATED" },
    { (HRESULT)0x8876024d, "DDERR_NOTPALETTIZED" },
    { (HRESULT)0x8876024e, "DDERR_UNSUPPORTEDMODE" },
};

/* 69011b7f -- wglDDrawError: log a DirectDraw HRESULT by name (s_ddErrors) */
void __fastcall wglDDrawError(const char *where, HRESULT hr)
{
    const char *name = "Unknown";
    int i;
    CHAR buf[256];
    for (i = 0; i < (int)(sizeof(s_ddErrors) / sizeof(s_ddErrors[0])); i++) {
        if (hr == s_ddErrors[i].hr) { name = s_ddErrors[i].name; break; }
    }
    wsprintfA(buf, "WGL: %s: DDERROR = %s\n", where, name);
    wglMessage(buf);
}

/* 6900f41f -- create the DirectDraw object and primary surface (ref-counted) */
LPDIRECTDRAW wglDDrawAllocate(void)
{
    HRESULT (WINAPI *pDirectDrawCreate)(GUID *, LPDIRECTDRAW *, IUnknown *);
    LPDIRECTDRAW dd;
    DDSURFACEDESC ddsd;
    HRESULT hr;

    if (++g_ddRefs > 1) return g_ddraw;
    g_hDDraw = LoadLibraryA("DDRAW.DLL");
    if (g_hDDraw == NULL) return NULL;
    pDirectDrawCreate = (HRESULT (WINAPI *)(GUID *, LPDIRECTDRAW *, IUnknown *))GetProcAddress(g_hDDraw, "DirectDrawCreate");
    hr = pDirectDrawCreate(NULL, &dd, NULL);
    if (hr != DD_OK) { wglDDrawError("DDraw: Allocate: Create", hr); return NULL; }
    hr = IDirectDraw_SetCooperativeLevel(dd, NULL, DDSCL_NORMAL);
    if (hr != DD_OK) { wglDDrawError("DDraw: Allocate: SetCooperativeLevel", hr); return NULL; }
    /* the ICD does not clear ddsd: only dwSize, dwFlags and ddsCaps are set */
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    hr = IDirectDraw_CreateSurface(dd, &ddsd, &g_ddPrimary, NULL);
    if (hr != DD_OK) {
        if (wglIsScreenSaverProcess()) IDirectDraw_Release(dd);
        wglDDrawError("DDraw: Allocate: CreateSurface", hr);
        return NULL;
    }
    g_ddraw = dd;
    return dd;
}

/* 6900f520 -- wglDDrawFree: drop a reference; the last one releases the DirectDraw object and frees DDRAW.DLL */
void __fastcall wglDDrawFree(LPDIRECTDRAW dd)
{
    if (--g_ddRefs == 0) {
        IDirectDraw_Release(dd);
        FreeLibrary(g_hDDraw);
    }
}

/* 6900f550 -- __wglGetDDrawObject (SGI WGLDDRAW.C).  6900f550 .. 6900f950: unreferenced in the original. */
LPDIRECTDRAW wglDDrawGetObject(void)
{
    return g_ddraw;
}

/* 6900f560 -- an empty function */
void wglDDrawNop(void)
{
}

/* 6900f930 -- __wglDDrawGetPrimaryBase: the primary surface address wglDDrawInitPrimary recorded (g_ddPrimaryPtr) */
void *wglDDrawGetPrimaryBase(void)
{
    return g_ddPrimaryPtr;
}

/* 6900f940 -- __wglDDrawOpen: wglDDrawAllocate (a reference on the DirectDraw object); GL_TRUE if it succeeded */
GLboolean wglDDrawOpen(void)
{
    if (wglDDrawAllocate()) {
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/* 6900f950 -- __wglDDrawClose: wglDDrawFree(g_ddraw), the pointer left as it is; GL_TRUE */
GLboolean wglDDrawClose(void)
{
    wglDDrawFree(g_ddraw);

    return GL_TRUE;
}

/* 6900f2cd -- NT screen-saver detection: when launched as a screen saver on NT the
** DirectDraw primary cannot be created; the ICD kills the saver's window and lowers
** its priority instead.
*/
BOOL wglIsScreenSaverProcess(void)
{
    OSVERSIONINFOA vi;
    STARTUPINFOA si;

    memset(&vi, 0, sizeof(vi));
    vi.dwOSVersionInfoSize = sizeof(vi);
    GetVersionExA(&vi);
    if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        si.cb = sizeof(si);
        GetStartupInfoA(&si);
        if (strstr(si.lpDesktop, "Screen-saver") != NULL) {
            if (strstr(si.lpTitle, ".scr") == NULL && strstr(si.lpTitle, ".SCR") == NULL)
                return FALSE;
            DestroyWindow(GetActiveWindow());
            SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
            return TRUE;
        }
    }
    return FALSE;
}

/* 6900fa73 -- restore a lost surface, recreating it when the display mode changed */
void __fastcall wglDDrawRestoreSurface(__WGLbuffer *buf, __WGLdrawable *dp)
{
    LPDIRECTDRAWSURFACE s;
    DDSURFACEDESC ddsd;
    HRESULT hr;

    wglCSEnter(dp->lock);
    s = (LPDIRECTDRAWSURFACE)buf->handle;
    if (s != NULL) {
        hr = IDirectDrawSurface_IsLost(s);
        if (hr != DD_OK) {
            if (hr != DDERR_SURFACELOST) {
                wglDDrawError("DDraw: RestoreSurface: IsLost", hr);
            } else {
                hr = IDirectDrawSurface_Restore(s);
                if (hr == DDERR_WRONGMODE) {
                    if (buf == &dp->frontBuffer) {
                        hr = IDirectDrawSurface_Release(s);
                        if (hr != DD_OK) {
                            wglDDrawError("DDraw: RestoreSurface: Release (primary)", hr);
                        } else {
                            buf->handle = NULL;
                            ddsd.dwSize = sizeof(ddsd);              /* (only these three fields are set) */
                            ddsd.dwFlags = DDSD_CAPS;
                            ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
                            hr = IDirectDraw_CreateSurface(g_ddraw, &ddsd, &s, NULL);
                            if (hr != DD_OK) {
                                wglDDrawError("DDraw: RestoreSurface: CreateSurface (primary)", hr);
                            } else {
                                buf->handle = s;
                                wglCSLeave(dp->lock);
                                return;
                            }
                        }
                    } else {
                        ddsd.dwSize = sizeof(ddsd);
                        hr = IDirectDrawSurface_GetSurfaceDesc(s, &ddsd);
                        if (hr != DD_OK) wglDDrawError("DDraw: RestoreSurface: GetSurfaceDesc", hr);
                        hr = IDirectDrawSurface_Release(s);
                        if (hr != DD_OK) {
                            wglDDrawError("DDraw: RestoreSurface: Release", hr);
                        } else {
                            buf->handle = NULL;
                            ddsd.dwSize = sizeof(ddsd);
                            if (buf == &dp->depthBuffer) {
                                ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_ZBUFFERBITDEPTH;
                                ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
                                ddsd.ddsCaps.dwCaps = (g_device->size & 0x20) ? (DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY)
                                                                              : (DDSCAPS_ZBUFFER | DDSCAPS_SYSTEMMEMORY);
                            } else {
                                ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
                                ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
                                ddsd.ddsCaps.dwCaps = (g_device->size & 0x4) ? (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY)
                                                                             : (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY);
                            }
                            hr = IDirectDraw_CreateSurface(g_ddraw, &ddsd, &s, NULL);
                            if (hr != DD_OK) wglDDrawError("DDraw: RestoreSurface: CreateSurface", hr);
                            buf->handle = s;
                            wglCSLeave(dp->lock);
                            return;
                        }
                    }
                } else if (hr != DD_OK) {
                    wglDDrawError("DDraw: RestoreSurface: Restore: failed", hr);
                }
            }
        }
    }
    wglCSLeave(dp->lock);
}

/* ---------------------------------------------------------- primary surface */
/* 690109e8 -- resize (primary): make sure a clipper bound to the window exists */
static GLubyte __fastcall ddPrimaryResize(__WGLbuffer *buf, GLint x, GLint y, GLuint w, GLuint h, __WGLdrawable *dp, GLuint mask)
{
    __WGLddPriv *pv = (__WGLddPriv *)buf->private_;
    __WGLwindow *win = dp->window;
    LPDIRECTDRAWCLIPPER clipper;
    HRESULT hr;
    (void)x; (void)y; (void)w; (void)h; (void)mask;

    if (pv->clipper == NULL) {
        hr = IDirectDraw_CreateClipper(g_ddraw, 0, &clipper, NULL);
        if (hr != DD_OK) { wglDDrawError("Update: CreateClipper", hr); return 0; }
        hr = IDirectDrawClipper_SetHWnd(clipper, 0, win->hwnd);
        if (hr != DD_OK) { wglDDrawError("Update: SetHWND", hr); return 0; }
        pv->clipper = clipper;                 /* one write, after both calls succeeded.  As the original:
                                                  a clipper whose SetHWnd fails is not released */
    }
    return 1;
}

/* 69010a89 -- lock (primary): lock the visible part of the window on the primary
   (3dfx's LockPrimary, locking the window rect clamped to the screen) */
static void __fastcall ddPrimaryLock(__WGLbuffer *buf, __WGLdrawable *dp)
{
    LPDIRECTDRAWSURFACE lpSurface = (LPDIRECTDRAWSURFACE)buf->handle;
    __WGLddPriv *surfaceInfo = (__WGLddPriv *)buf->private_;
    __WGLwindow *wglPriv = dp->window;
    GLint xOff, yOff;
    DDSURFACEDESC ddsd;
    RECT rect;
    HRESULT status;

    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);

    /* the window rect, clamped to the screen */
    xOff = 0;
    yOff = 0;
    rect.left   = wglPriv->clientRect.left;
    rect.right  = (wglPriv->clientRect.right  < buf->width)  ? wglPriv->clientRect.right  : buf->width;
    rect.top    = wglPriv->clientRect.top;
    rect.bottom = (wglPriv->clientRect.bottom < buf->height) ? wglPriv->clientRect.bottom : buf->height;
    if (rect.left < 0) { xOff = -rect.left; rect.left = 0; }
    if (rect.top  < 0) { yOff = -rect.top;  rect.top  = 0; }
    if (rect.right  < 0) rect.right  = 0;
    if (rect.bottom < 0) rect.bottom = 0;

    /* lock the surface */
    while (1) {
        status = IDirectDrawSurface_Lock(lpSurface, &rect, &ddsd, DDLOCK_WAIT, NULL);
        if (status == DD_OK) {
            break;
        } else if (status == DDERR_SURFACELOST) {
            wglDDrawRestoreSurface(buf, dp);
        } else if (status == DDERR_SURFACEBUSY) {
            /* try again */
            continue;
        } else {
            wglDDrawError("DDraw: LockPrimary: Lock", status);
        }
    }

    /* get the pointer */
    buf->byteWidth  = ddsd.lPitch;
    buf->base       = ddsd.lpSurface;
    buf->outerWidth = ddsd.lPitch / buf->elementSize;
    surfaceInfo->lockedPtr = ddsd.lpSurface;
    /* (0,0) of the buffer is the window origin, possibly off screen */
    buf->base = (BYTE *)buf->base - (buf->elementSize * xOff + yOff * buf->byteWidth);

    /* update clip list */
    if (surfaceInfo->clipper != NULL) {
        (*dp->updateClipRegion)(dp);
    }
}

/* 69010bd8 -- fill (primary): clipped Blt (3dfx's FillPrimary) */
static void __fastcall ddPrimaryFill(__WGLbuffer *buf, __WGLdrawable *dp, GLuint val, GLint x, GLint y, GLint w, GLint h)
{
    LPDIRECTDRAWSURFACE lpSurface;
    LPDIRECTDRAWCLIPPER lpClipper;
    __WGLddPriv *surfaceInfo;
    __WGLwindow *wglPriv;
    HRESULT status;

    wglCSEnter(dp->lock);
    surfaceInfo = (__WGLddPriv *)buf->private_;

    wglPriv = dp->window;
    lpSurface = (LPDIRECTDRAWSURFACE)buf->handle;
    lpClipper = surfaceInfo->clipper;

    /* install clipper on the destination surface */
    status = IDirectDrawSurface_SetClipper(lpSurface, lpClipper);
    if (status != DD_OK) {
        wglDDrawError("DDraw: Fill: SetClipper", status);
    }

    /* make the coords screen relative */
    x += wglPriv->clientOrigin.x;
    y += wglPriv->clientOrigin.y;

    ddColorFill(buf, dp, val, x, y, w, h);     /* 69010258 */
    wglCSLeave(dp->lock);
}

/* 69010c60 -- free (primary): ddPrimaryFree: if the buffer has a clipper, unlock the surface when still locked and
** release the clipper; then free the __WGLddPriv (the shared g_ddPrimary surface itself is not released) */
static void __fastcall ddPrimaryFree(__WGLbuffer *buf, __WGLdrawable *dp)
{
    __WGLddPriv *pv = (__WGLddPriv *)buf->private_;
    LPDIRECTDRAWSURFACE surf = (LPDIRECTDRAWSURFACE)buf->handle;
    LPDIRECTDRAWCLIPPER clipper = pv->clipper;
    (void)dp;
    if (clipper != NULL) {
        if (pv->lockedPtr != NULL) IDirectDrawSurface_Unlock(surf, pv->lockedPtr);
        IDirectDrawClipper_Release(clipper);
        pv->clipper = NULL;
    }
    wglFree(pv);
    buf->private_ = NULL;
}

/* 69010852 -- wglDDrawInitPrimary ("__wglInitDDrawPrimary"): set up buf as the DirectDraw primary: install the
** primary resize / lock / fill / free procs and the shared unlock, take g_ddPrimary as its handle (restoring it if
** lost), Lock / Unlock it once to record g_ddPrimaryPtr, size it from GetDisplayMode (width, height, depth, pitch,
** element size) and calloc its __WGLddPriv; a failing DirectDraw call logs and returns early */
void __fastcall wglDDrawInitPrimary(__WGLbuffer *buf, __WGLdrawable *dp)
{
    DDSURFACEDESC ddsd, mode;              /* the original has two (Lock, GetDisplayMode) */
    HRESULT hr;

    buf->height = 0; buf->width = 0; buf->base = NULL; buf->handle = NULL; buf->size = 0; buf->byteWidth = 0;
    buf->resize = ddPrimaryResize;
    buf->lock   = ddPrimaryLock;
    buf->unlock = ddUnlock;              /* 690101ee, shared */
    buf->fill   = ddPrimaryFill;
    buf->free   = ddPrimaryFree;
    buf->mainInit = NULL;
    buf->fallbackInit = NULL;
    buf->handle = g_ddPrimary;
    wglDDrawRestoreSurface(buf, dp);
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    hr = IDirectDrawSurface_Lock(g_ddPrimary, NULL, &ddsd, DDLOCK_WAIT, NULL);
    if (hr != DD_OK) { wglDDrawError("__wglInitDDrawPrimary: Lock", hr); return; }
    g_ddPrimaryPtr = ddsd.lpSurface;
    hr = IDirectDrawSurface_Unlock(g_ddPrimary, ddsd.lpSurface);
    if (hr != DD_OK) { wglDDrawError("__wglInitDDrawPrimary: Unlock", hr); return; }
    mode.dwSize = sizeof(mode);
    hr = IDirectDraw_GetDisplayMode(g_ddraw, &mode);
    if (hr != DD_OK) { wglDDrawError("__wglInitDDrawPrimary: GetPixelFormat", hr); return; }
    buf->height = mode.dwHeight;
    buf->depth  = mode.ddpfPixelFormat.dwRGBBitCount;
    buf->width  = mode.dwWidth;
    buf->base   = NULL;
    buf->elementSize     = ((buf->depth - 1) / 8) + 1;
    buf->elementSizeLog2 = wglLog2(buf->elementSize);
    buf->byteWidth  = mode.lPitch;
    buf->outerWidth = mode.lPitch / buf->elementSize;
    buf->private_ = wglCalloc(1, sizeof(__WGLddPriv));
}

/* ------------------------------------------------- offscreen color/depth surfaces */
/* 690101ee -- unlock (shared by all DirectDraw buffers) */
void __fastcall ddUnlock(__WGLbuffer *buf, __WGLdrawable *dp)
{
    __WGLddPriv *pv = (__WGLddPriv *)buf->private_;
    HRESULT hr;
    (void)dp;
    hr = IDirectDrawSurface_Unlock((LPDIRECTDRAWSURFACE)buf->handle, pv->lockedPtr);
    if (hr != DD_OK) wglDDrawError("DDraw: Unlock", hr);
    pv->lockedPtr = NULL;
    buf->base = NULL;
}

/* lock an offscreen surface (3dfx's Lock): the body of ddOffscreenLock (colour, 6901014c) and ddDepthLock (depth,
** 6901065c); the original has the same code twice, one copy per buffer type */
static __inline void ddLockSurface(__WGLbuffer *buf, __WGLdrawable *dp)
{
    __WGLddPriv *surfaceInfo = (__WGLddPriv *)buf->private_;
    LPDIRECTDRAWSURFACE lpSurface = (LPDIRECTDRAWSURFACE)buf->handle;
    DDSURFACEDESC ddsd;
    HRESULT status;

    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);

    /* lock the surface */
    while (1) {
        status = IDirectDrawSurface_Lock(lpSurface, NULL, &ddsd, DDLOCK_WAIT, NULL);
        if (status == DD_OK) {
            break;
        } else if (status == DDERR_SURFACELOST) {
            wglDDrawRestoreSurface(buf, dp);
        } else if (status == DDERR_SURFACEBUSY) {
            /* try again */
            continue;
        } else {
            wglDDrawError("DDraw: Lock", status);
        }
    }

    /* get the pointer */
    buf->byteWidth  = ddsd.lPitch;
    buf->base       = ddsd.lpSurface;
    buf->outerWidth = ddsd.lPitch / buf->elementSize;
    surfaceInfo->lockedPtr = ddsd.lpSurface;
}

/* 6901014c -- lock (offscreen colour surface) */
static void __fastcall ddOffscreenLock(__WGLbuffer *buf, __WGLdrawable *dp)
{
    ddLockSurface(buf, dp);
}

/* 6901065c -- lock (depth surface): the same code as ddOffscreenLock.  Dead in the original: only wglDDrawInitDepth
** installs it, and wglDrawInitDDraw, which calls that, has no caller; ToDo UNUSED CODE: nothing in the driver calls it */
static void __fastcall ddDepthLock(__WGLbuffer *buf, __WGLdrawable *dp)
{
    ddLockSurface(buf, dp);
}

/* 69010258 -- color fill via Blt(DDBLT_COLORFILL) */
void __fastcall ddColorFill(__WGLbuffer *buf, __WGLdrawable *dp, GLuint val, GLint x, GLint y, GLint w, GLint h)
{
    LPDIRECTDRAWSURFACE s;
    RECT r;
    DDBLTFX fx;
    HRESULT hr;

    wglCSEnter(dp->lock);
    r.left = x; r.right = x + w; r.top = y; r.bottom = y + h;
    s = (LPDIRECTDRAWSURFACE)buf->handle;
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = val;
    hr = IDirectDrawSurface_Blt(s, &r, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
    if (hr != DD_OK) {
        if (hr == DDERR_SURFACELOST) {
            wglDDrawRestoreSurface(buf, dp);
            hr = IDirectDrawSurface_Blt(s, &r, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &fx);   /* (the old surface pointer) */
            if (hr != DD_OK) wglDDrawError("DDraw: Fill: Blt", hr);
        } else {
            wglDDrawError("DDraw: Fill: Blt(2)", hr);
        }
    }
    wglCSLeave(dp->lock);
}

/* 69010701 -- depth fill via Blt(DDBLT_DEPTHFILL) (3dfx's FillDepth) */
static void __fastcall ddDepthFill(__WGLbuffer *buf, __WGLdrawable *dp, GLuint val, GLint x, GLint y, GLint w, GLint h)
{
    LPDIRECTDRAWSURFACE lpSurface;
    RECT dstRect;
    DDBLTFX ddbltfx;
    DWORD flags;
    HRESULT status;

    wglCSEnter(dp->lock);
    lpSurface = (LPDIRECTDRAWSURFACE)buf->handle;

    dstRect.left = x;
    dstRect.right = x + w;
    dstRect.top = y;
    dstRect.bottom = y + h;

    memset(&ddbltfx, 0, sizeof(DDBLTFX));
    ddbltfx.dwSize = sizeof(DDBLTFX);
    ddbltfx.dwFillDepth = (DWORD) val;
    flags = DDBLT_WAIT | DDBLT_DEPTHFILL;

    status = IDirectDrawSurface_Blt(lpSurface, &dstRect, NULL, NULL, flags, &ddbltfx);
    if (status != DD_OK) {
        if (status == DDERR_SURFACELOST) {
            wglDDrawRestoreSurface(buf, dp);
            status = IDirectDrawSurface_Blt(lpSurface, &dstRect, NULL, NULL, flags, &ddbltfx);
            if (status != DD_OK) {
                wglDDrawError("DDraw: Fill: Blt", status);
            }
        }
    }
    wglCSLeave(dp->lock);
}

/* 6901034c -- free an offscreen surface */
static void __fastcall ddOffscreenFree(__WGLbuffer *buf, __WGLdrawable *dp)
{
    __WGLddPriv *pv = (__WGLddPriv *)buf->private_;
    LPDIRECTDRAWSURFACE s;
    HRESULT hr;
    (void)dp;
    if (buf != NULL && (s = (LPDIRECTDRAWSURFACE)buf->handle) != NULL) {
        if (pv->lockedPtr != NULL) IDirectDrawSurface_Unlock(s, pv->lockedPtr);
        hr = IDirectDrawSurface_Release(s);
        if (hr != DD_OK) wglDDrawError("DDraw: Free", hr);
        buf->handle = NULL;
        buf->base = NULL;
    }
    wglFree(pv);
    buf->private_ = NULL;
}

/* 690100d0 -- resize() installed after a fallback to system memory: lock the memory
** buffer's view (through the fallback implementation).
*/
static GLubyte __fastcall ddFallbackResize(__WGLbuffer *buf, GLint x, GLint y, GLuint w, GLuint h, __WGLdrawable *dp, GLuint mask)
{
    __WGLwindow *win;
    GLubyte r;
    wglCSEnter(dp->lock);
    win = dp->window;
    buf->free(buf, dp);
    buf->mainInit(buf, dp, buf->depth, buf->fallbackInit);
    win->swapBuffers = win->present;
    r = buf->resize(buf, x, y, w, h, dp, mask);
    wglCSLeave(dp->lock);
    return r;
}

/* 6900ff06 -- ddColorResize: colour buffer resize: keep the offscreen-plain surface when it is at least w x h, else
** (re)create it, in video memory when the device caps word has a bit of mask, else in system memory.  If that fails,
** switch the buffer to its memory fallback (free, fallbackInit, swap by copying, resize it, install
** ddFallbackResize) and report WGLBUF_FALLBACK.  Installed by wglDDrawInitColor; ToDo UNUSED CODE: the whole
** DirectDraw drawable path is unreachable (nothing calls wglDrawInitDDraw 6900ca60) */
static GLubyte __fastcall ddColorResize(__WGLbuffer *buf, GLint x, GLint y, GLuint w, GLuint h, __WGLdrawable *dp, GLuint mask)
{
    LPDIRECTDRAWSURFACE lpSurface = (LPDIRECTDRAWSURFACE)buf->handle;
    LPDIRECTDRAWSURFACE newSurface;
    __WGLwindow *wglPriv;
    DDSURFACEDESC ddsd;
    HRESULT status;

    if (lpSurface != NULL) {
        ddsd.dwSize = sizeof(DDSURFACEDESC);
        status = IDirectDrawSurface_GetSurfaceDesc(lpSurface, &ddsd);
        if (status != DD_OK) {
            wglDDrawError("DDraw: Update: GetSurfaceDesc", status);
            return WGLBUF_FAILED;
        }
        /* too small: drop it */
        if (w > ddsd.dwWidth || h > ddsd.dwHeight) {
            status = IDirectDrawSurface_Release(lpSurface);
            if (status != DD_OK) {
                wglDDrawError("DDraw: Update: Release", status);
                return WGLBUF_FAILED;
            }
            buf->handle = NULL;
        }
    }

    if (buf->handle == NULL) {
        ddsd.dwSize = sizeof(DDSURFACEDESC);
        ddsd.dwWidth = w;
        ddsd.dwHeight = h;
        if (w == 0) ddsd.dwWidth = 1;
        if (h == 0) ddsd.dwHeight = 1;
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        if (g_device->size & mask)
            ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
        else
            ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
        status = IDirectDraw_CreateSurface(g_ddraw, &ddsd, &newSurface, NULL);
        if (status != DD_OK) {
            /* fall back to a memory buffer */
            wglPriv = dp->window;
            buf->free(buf, dp);
            buf->fallbackInit(buf, dp, buf->depth);
            wglPriv->swapBuffers = wglPriv->copyBackToFront;
            if (!buf->resize(buf, x, y, w, h, dp, mask)) {
                wglDDrawError("DDraw: Update: UpdateFallback", status);
                return WGLBUF_FAILED;
            }
            buf->height = h;
            buf->resize = ddFallbackResize;
            buf->width = w;
            return WGLBUF_FALLBACK;
        }
        buf->handle = newSurface;
    }
    buf->width = w;
    buf->height = h;
    return WGLBUF_OK;
}

/* 6901048b -- ddDepthResize: depth buffer resize: as ddColorResize, with a z-buffer surface of buf->depth bits.
** Installed by wglDDrawInitDepth; ToDo UNUSED CODE: unreachable like ddColorResize */
static GLubyte __fastcall ddDepthResize(__WGLbuffer *buf, GLint x, GLint y, GLuint w, GLuint h, __WGLdrawable *dp, GLuint mask)
{
    LPDIRECTDRAWSURFACE lpSurface = (LPDIRECTDRAWSURFACE)buf->handle;
    LPDIRECTDRAWSURFACE newSurface;
    __WGLwindow *wglPriv;
    DDSURFACEDESC ddsd;
    HRESULT status;

    if (lpSurface != NULL) {
        ddsd.dwSize = sizeof(DDSURFACEDESC);
        status = IDirectDrawSurface_GetSurfaceDesc(lpSurface, &ddsd);
        if (status != DD_OK) {
            wglDDrawError("DDraw: Update: GetSurfaceDesc", status);
            return WGLBUF_FAILED;
        }
        /* too small: drop it */
        if (w > ddsd.dwWidth || h > ddsd.dwHeight) {
            status = IDirectDrawSurface_Release(lpSurface);
            if (status != DD_OK) {
                wglDDrawError("DDraw: Update: Release", status);
                return WGLBUF_FAILED;
            }
            buf->handle = NULL;
        }
    }

    if (buf->handle == NULL) {
        ddsd.dwSize = sizeof(DDSURFACEDESC);
        ddsd.dwWidth = w;
        ddsd.dwHeight = h;
        ddsd.dwZBufferBitDepth = buf->depth;
        if (w == 0) ddsd.dwWidth = 1;
        if (h == 0) ddsd.dwHeight = 1;
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_ZBUFFERBITDEPTH;
        ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
        if (g_device->size & mask)
            ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
        else
            ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
        status = IDirectDraw_CreateSurface(g_ddraw, &ddsd, &newSurface, NULL);
        if (status != DD_OK) {
            /* fall back to a memory buffer */
            wglPriv = dp->window;
            buf->free(buf, dp);
            buf->fallbackInit(buf, dp, buf->depth);
            wglPriv->swapBuffers = wglPriv->copyBackToFront;
            if (!buf->resize(buf, x, y, w, h, dp, mask)) {
                wglDDrawError("DDraw: UpdateDepth: UpdateFallback", status);
                return WGLBUF_FAILED;
            }
            buf->height = h;
            buf->resize = ddFallbackResize;
            buf->width = w;
            return WGLBUF_FALLBACK;
        }
        buf->handle = newSurface;
    }
    buf->width = w;
    buf->height = h;
    return WGLBUF_OK;
}

/* 6900fe30 -- init an offscreen color-type surface buffer */
void __fastcall wglDDrawInitColor(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth, void *fallbackInit)
{
    (void)dp;
    buf->depth = depth;
    buf->height = 0; buf->width = 0; buf->base = NULL; buf->handle = NULL; buf->size = 0; buf->byteWidth = 0;
    buf->elementSize     = ((depth - 1) / 8) + 1;
    buf->elementSizeLog2 = wglLog2(buf->elementSize);
    buf->resize = ddColorResize;
    buf->lock   = ddOffscreenLock;
    buf->unlock = ddUnlock;
    buf->fill   = ddColorFill;
    buf->free   = ddOffscreenFree;
    buf->mainInit = wglDDrawInitColor;
    buf->fallbackInit = (void (__fastcall *)(__WGLbuffer *, __WGLdrawable *, GLint))fallbackInit;
    buf->private_ = wglCalloc(1, 4);
}

/* 690103b0 -- init an offscreen depth surface buffer */
void __fastcall wglDDrawInitDepth(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth, void *fallbackInit)
{
    (void)dp;
    buf->depth = depth;
    buf->height = 0; buf->width = 0; buf->base = NULL; buf->handle = NULL; buf->size = 0; buf->byteWidth = 0;
    buf->elementSize     = ((depth - 1) / 8) + 1;
    buf->elementSizeLog2 = wglLog2(buf->elementSize);
    buf->resize = ddDepthResize;
    buf->lock   = ddDepthLock;
    buf->unlock = ddUnlock;
    buf->fill   = ddDepthFill;
    buf->free   = ddOffscreenFree;
    buf->mainInit = wglDDrawInitDepth;
    buf->fallbackInit = (void (__fastcall *)(__WGLbuffer *, __WGLdrawable *, GLint))fallbackInit;
    buf->private_ = wglCalloc(1, 4);
}

/* ------------------------------------------------------------- window ops */
/* 6900f59c -- swap: Blt the back surface onto the primary through the clipper
   (3dfx's __wglDDrawDDrawSwapBuffers) */
GLboolean __fastcall wglDDrawSwapBuffers(__WGLwindow *w)
{
    LPDIRECTDRAWSURFACE lpDst, lpSrc;
    __WGLddPriv *surfaceInfo;
    LPDIRECTDRAWCLIPPER lpClipper;
    __WGLswapRegion *rgnData;
    HRESULT status;

    wglCSEnter(w->dp.lock);

    /* Get source and destination surfaces */
    if (w->dp.frontBuffer.handle == NULL) {
        wglCSLeave(w->dp.lock);
        return GL_FALSE;
    }
    lpDst = (LPDIRECTDRAWSURFACE)w->dp.frontBuffer.handle;
    if (w->dp.backBuffer.handle == NULL) {
        wglCSLeave(w->dp.lock);
        return GL_FALSE;
    }
    lpSrc = (LPDIRECTDRAWSURFACE)w->dp.backBuffer.handle;

    /* Install clipper on the primary */
    surfaceInfo = (__WGLddPriv *)w->dp.frontBuffer.private_;
    lpClipper = surfaceInfo->clipper;
    if (lpClipper) {
        status = IDirectDrawSurface_SetClipper(g_ddPrimary, lpClipper);
        if (status != DD_OK) {
            wglDDrawError("Update: SetClipper", status);
        }
        (*w->dp.updateClipRegion)(&w->dp);
    }

    rgnData = w->swapRegion;

    /* only the bounding rect of the swap hint rects is used */
    if (rgnData->numRects != 0) {
        RECT *lpRect = (RECT *)&rgnData->boundingRect;
        RECT dstRect;

        dstRect.left = w->clientRect.left + lpRect->left;
        dstRect.right = w->clientRect.left + lpRect->right;
        dstRect.top = w->clientRect.top + lpRect->top;
        dstRect.bottom = w->clientRect.top + lpRect->bottom;

        status = IDirectDrawSurface_Blt(lpDst, &dstRect, lpSrc, lpRect, DDBLT_WAIT, 0);
        rgnData->numRects = 0;
    } else {
        RECT srcRect;

        srcRect.left = 0;
        srcRect.right = w->width;
        srcRect.top = 0;
        srcRect.bottom = w->height;

        status = IDirectDrawSurface_Blt(lpDst, &w->clientRect, lpSrc, &srcRect, DDBLT_WAIT, 0);
    }

    if (status != DD_OK) {
        if (status == DDERR_SURFACELOST) {
            wglDDrawRestoreSurface(&w->dp.frontBuffer, &w->dp);
            wglDDrawRestoreSurface(&w->dp.backBuffer, &w->dp);
        } else {
            wglDDrawError("DDraw: SwapBuffers: Blt", status);
        }
        wglCSLeave(w->dp.lock);
        return GL_FALSE;
    }
    wglCSLeave(w->dp.lock);
    return GL_TRUE;
}

/* 6900f720 -- swap with a memory back buffer: copy rows into the locked primary, per clip rect.
   3dfx's __wglDDrawMemSwapBuffers (SRC/WGL/WGLDDRAW.C): dword-aligned rows go through an
   inline-asm loop (16 bytes at a time with EBP as a fourth data register, then the remaining
   bytes); others through memcpy. */
GLboolean __fastcall wglDDrawCopyBackToFront(__WGLwindow *w)
{
    __WGLdrawable *dp = &w->dp;
    __WGLbuffer *front = &dp->frontBuffer;
    __WGLbuffer *back = &dp->backBuffer;
    __WGLregionRect *rect;
    GLubyte *f, *b;
    int i, n, y;
    int x0, y0, x1, y1;
    int padf, padb;

    wglCSEnter(w->dp.lock);
    if (back->width != 0 && back->height != 0) {
	(*front->lock)(front, dp);
	(*back->lock)(back, dp);

	n = dp->numRects;
	for (i = 0, rect = &dp->rects[0]; i < n; i++, rect++) {
	    int cnt;

	    x0 = rect->x0;
	    x1 = rect->x1;
	    y0 = rect->y0;
	    y1 = rect->y1;

	    f = (GLubyte *)front->base
		+ y0 * front->byteWidth + x0 * front->elementSize;
	    b = (GLubyte *)back->base
		+ y0 * back->byteWidth + x0 * back->elementSize;

	    padf = front->byteWidth;
	    padb = back->byteWidth;

	    cnt = (x1 - x0) * front->elementSize;

	    if ((((GLuint)f) & 3) || (((GLuint)b) & 3) || (padf & 3) || (padb & 3)) {
		/* unaligned path */
		for (y = y0; y < y1; y++) {
		    memcpy(f, b, cnt);
		    f += padf;
		    b += padb;
		}
	    } else {
		/* dword aligned path. */
		for (y = y0; y < y1; y++) {
		    int c16 = cnt & ~0xf;
		    int c1 = cnt - c16;

		    __asm {
			mov	ecx, c16;
			mov	esi, b;
			mov	edi, f;
			push	ebp;

			cmp	ecx, 0;
			je	l_16end;
		    l_16loop:
			mov	eax, dword ptr [esi];
			mov	ebx, dword ptr [esi+4];
			mov	edx, dword ptr [esi+8];
			mov	ebp, dword ptr [esi+12];
			mov	dword ptr [edi], eax;
			mov	dword ptr [edi+4], ebx;
			mov	dword ptr [edi+8], edx;
			mov	dword ptr [edi+12], ebp;
			add	esi, 16;
			add	edi, 16;
			sub	ecx, 16;
			jne	l_16loop;

		    l_16end:
			pop	ebp;
			mov	ecx, c1;
			push	ebp;

			cmp	ecx, 0;
			je	l_1end;
		    l_1loop:
			mov	al, byte ptr [esi];
			mov	byte ptr [edi], al;
			inc	esi;
			inc	edi;
			dec	ecx;
			jne	l_1loop;
		    l_1end:
			pop	ebp;
		    }
		    f += padf;
		    b += padb;
		}
	    }
	}

	(*front->unlock)(front, dp);
	(*back->unlock)(back, dp);
	wglCSLeave(w->dp.lock);
	return GL_TRUE;
    }
    wglCSLeave(w->dp.lock);
    return GL_TRUE;
}


/* 6900fceb -- updateClipRegion: fetch the clipper's clip list into the ownership region
   (3dfx's __wglDDrawUpdateClipList, with the IsClipListChanged test moved in) */
void __fastcall wglDDrawUpdateClipList(__WGLdrawable *dp)
{
    __WGLwindow *wglPriv;
    __WGLddPriv *surfaceInfo;
    LPDIRECTDRAWCLIPPER lpClipper;
    BOOL clipListChanged;
    RECT screenClipRect;
    LPRGNDATA rgnData;
    DWORD rgnDataSize;
    HRESULT status;

    wglCSEnter(dp->lock);
    wglPriv = dp->window;
    surfaceInfo = (__WGLddPriv *)wglPriv->dp.frontBuffer.private_;
    if (surfaceInfo->clipper == NULL) {
        wglCSLeave(dp->lock);
        return;
    }
    lpClipper = surfaceInfo->clipper;
    status = IDirectDrawClipper_IsClipListChanged(lpClipper, &clipListChanged);
    if (status != DD_OK) {
        wglDDrawError("DDraw: UpdateClipList: IsClipListChanged", status);
        wglCSLeave(dp->lock);
        return;
    }
    if (!wglPriv->clipChanged && !clipListChanged) {
        wglCSLeave(dp->lock);
        return;
    }

    /* Convert core clip rect to be screen relative */
    screenClipRect.left = wglPriv->clipRect.x0 + wglPriv->clientOrigin.x;
    screenClipRect.top = wglPriv->clipRect.y0 + wglPriv->clientOrigin.y;
    screenClipRect.right = wglPriv->clipRect.x1 + wglPriv->clientOrigin.x;
    screenClipRect.bottom = wglPriv->clipRect.y1 + wglPriv->clientOrigin.y;

    /* Allocate a buffer to receive window region clip list */
    status = IDirectDrawClipper_GetClipList(lpClipper, &screenClipRect, NULL, &rgnDataSize);
    if (status != DD_OK) {
        wglDDrawError("DDraw: UpdateClipList: GetClipList (1)", status);
    }

    if (rgnDataSize > 0) {
        rgnData = (LPRGNDATA)wglMalloc(rgnDataSize);

        /* Get window region clip list */
        status = IDirectDrawClipper_GetClipList(lpClipper, &screenClipRect, rgnData, &rgnDataSize);
        if (status != DD_OK) {
            wglDDrawError("DDraw: UpdateClipList: GetClipList (2)", status);
        }

        wglDrawSetRegion(wglPriv, rgnData);
        wglFree(rgnData);
    } else {
        wglDrawSetRegion(wglPriv, NULL);
    }
    wglPriv->clipChanged = 0;
    wglCSLeave(dp->lock);
}

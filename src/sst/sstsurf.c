/*
** sstsurf.c -- 3dfx back end, original VA range 6901a3b0..6901d000: windowed rendering on DirectDraw / Glide
** surfaces (rect helpers, the windowed buffers' empty lock / unlock procs), the glColor entry-point pick and
** the hardware-path glColorMask, glCullFace and glDepthMask.
** Then the rest of the windowed (DirectDraw surface) path, in address order: restore / present / read back,
** the front and back surfaces, colour / depth buffer init, lock, resize and free, the first windowed context
** and its make-current.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "icdprotos.h"
#include "icdglob.h"
#include "si_names.h"
#include "glshade.h"
#include "og.h"
#include "gldisp.h"

/* 690f66c8 .. 690f66ec -- the windowed-rendering DirectDraw objects (icdglob.h) */
LPDIRECTDRAW2 g_winDDraw = NULL;
LPDIRECTDRAWCLIPPER g_winClipper = NULL;
CRITICAL_SECTION g_winPrimaryLock = { 0 };
GLint g_winUsers = 0;
LPDIRECTDRAWSURFACE2 g_winPrimary = NULL;
/* 693c3ce8 .. 693c3cfc -- the Glide SURFACE extension's procs, fetched below */
void (__stdcall *grSurfaceReleaseContextExt)(GLuint ctx);
int (__stdcall *grSurfaceCalcTextureWHDExt)(void *info, DWORD *w, DWORD *h, DWORD *d);
GLuint (__stdcall *grSurfaceCreateContextExt)(GLuint type);
void (__stdcall *grSurfaceSetAuxSurfaceExt)(void *sfc);
void (__stdcall *grSurfaceSetTextureSurfaceExt)(GLint tmu, void *sfc);
void (__stdcall *grSurfaceSetRenderingSurfaceExt)(void *sfc);

extern void __fastcall winLockDepthBuffer(__WGLbuffer *buf, __WGLdrawable *dp);

/* ---- forward declarations ---- */
void __fastcall winDebugMessage(void);
GLint __fastcall rectArea(const RECT *r);
void __fastcall rectOffset(RECT *dst, const RECT *origin, const RECT *src);
void __fastcall winUnlockColorBuffer(void);
void __fastcall winUnlockDepthBuffer(void);
int __fastcall winLock2DepthBuffer(void);
void __fastcall winUnlock2DepthBuffer(void);
void __fastcall hwPickColorProcs(__GLcontext *gc);
void __stdcall __glsstim_ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void __stdcall __glsstim_CullFace(GLenum mode);
void __stdcall __glsstim_DepthMask(GLboolean flag);

/* 6901ad40 (size 1) -- winDebugMessage: the debug-message stub: an empty printf (the windowed code calls it __cdecl with a
** format string) */
void __fastcall winDebugMessage(void)
{
}

/* 6901b020 -- rectArea: the area of a RECT */
GLint __fastcall rectArea(const RECT *r)
{
    return (r->bottom - r->top) * (r->right - r->left);
}

/* 6901b040 -- rectOffset: dst = src moved by origin's top-left corner */
void __fastcall rectOffset(RECT *dst, const RECT *origin, const RECT *src)
{
    dst->left = origin->left + src->left;
    dst->top = origin->top + src->top;
    dst->right = (src->right - src->left) + dst->left;
    dst->bottom = (src->bottom - src->top) + dst->top;
}

/* 6901bb60 -- winUnlockColorBuffer: unlock of a windowed colour buffer: nothing to do */
void __fastcall winUnlockColorBuffer(void)
{
}

/* 6901bd20 -- winUnlockDepthBuffer: unlock of a windowed depth buffer: nothing to do */
void __fastcall winUnlockDepthBuffer(void)
{
}

/* 6901bd30 -- winLock2DepthBuffer: the depth buffer's second lock entry (lock2), a jump to winLockDepthBuffer */
#ifdef _MSC_VER
/* 6901bd30 -- winLock2DepthBuffer: (VC6 build) the tail jump to winLockDepthBuffer */
__declspec(naked) int __fastcall winLock2DepthBuffer() { __asm jmp winLockDepthBuffer }
#else
/* winLock2DepthBuffer: the stand-in that lets the non-MSVC syntax check compile (returns 0) */
int __fastcall winLock2DepthBuffer() { return 0; }
#endif

/* 6901bd40 -- winUnlock2DepthBuffer: the depth buffer's unlock2: nothing to do */
void __fastcall winUnlock2DepthBuffer(void)
{
}

/* 6901c400 -- hwPickColorProcs: pick the glColor3f / 3fv / 4f / 4fv / 4ubv entry points: the colour-material
** versions while GL_COLOR_MATERIAL is on, else the plain hardware ones; then the current dispatch is re-copied onto
** itself */
void __fastcall hwPickColorProcs(__GLcontext *gc)
{
    if (gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE) {
        gc->dispatchState->fn[__GL_DISP_Color3f] = (__GLdispatchFn)__glim_Color3f;
        gc->dispatchState->fn[__GL_DISP_Color4f] = (__GLdispatchFn)__glim_Color4f;
        gc->dispatchState->fn[__GL_DISP_Color3fv] = (__GLdispatchFn)__glim_Color3fv;
        gc->dispatchState->fn[__GL_DISP_Color4fv] = (__GLdispatchFn)__glim_Color4fv;
        gc->dispatchState->fn[__GL_DISP_Color4ubv] = (__GLdispatchFn)__glim_Color4ubv;
    } else {
        gc->dispatchState->fn[__GL_DISP_Color3f] = (__GLdispatchFn)__glsstim_Color3f;
        gc->dispatchState->fn[__GL_DISP_Color4f] = (__GLdispatchFn)__glsstim_Color4f;
        gc->dispatchState->fn[__GL_DISP_Color3fv] = (__GLdispatchFn)__glsstim_Color3fv;
        gc->dispatchState->fn[__GL_DISP_Color4fv] = (__GLdispatchFn)__glsstim_Color4fv;
        gc->dispatchState->fn[__GL_DISP_Color4ubv] = (__GLdispatchFn)__glsstim_Color4ubv;
    }
    __glCopyDispatchTable(&__glCurrentGC()->currentDispatchState, &__glCurrentGC()->currentDispatchState);
}

/* 6901c900 -- __glsstim_ColorMask: __glim_ColorMask (s_attrib.c), hardware path: the Glide colour mask record
** takes the four masks at 32 bpp (grColorMaskExt); at 16 bpp one rgb mask, all on unless r, g and b agree */
void __stdcall __glsstim_ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->hwcx.has32bpp) {
        GRS_SET(__GL_GR_COLOR_MASK, 1, red);
        GRS_SET(__GL_GR_COLOR_MASK, 2, green);
        GRS_SET(__GL_GR_COLOR_MASK, 3, blue);
        GRS_SET(__GL_GR_COLOR_MASK, 4, alpha);
    } else if (red == green && green == blue) {
        GRS_SET(__GL_GR_COLOR_MASK, 0, red);
        GRS_SET(__GL_GR_COLOR_MASK, 4, alpha);
    } else {
        GRS_SET(__GL_GR_COLOR_MASK, 0, 1);
        GRS_SET(__GL_GR_COLOR_MASK, 4, 1);
    }
    gc->state.raster.gMask = green;
    gc->state.raster.rMask = red;
    gc->state.raster.bMask = blue;
    gc->state.raster.aMask = alpha;
    __GL_DELAY_VALIDATE(gc);
}

/* 6901cab0 -- __glsstim_CullFace: __glim_CullFace (s_attrib.c), hardware path: only on a change; resets the Glide
** buffer modes and sets the ICD's cull dirty bit (no delayed validation) */
void __stdcall __glsstim_CullFace(GLenum mode)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->state.polygon.cull != mode) {
        switch (mode) {
          case GL_FRONT:
          case GL_BACK:
          case GL_FRONT_AND_BACK:
            gc->state.polygon.cull = mode;
            wglGlideSetupBufferModes(gc);
            gc->dirtyMask |= __GL_DIRTY_FACE_ICD;
            return;
          default:
            __glSetError(GL_INVALID_ENUM);
            break;
        }
    }
}

/* 6901cdd0 -- __glsstim_DepthMask: __glim_DepthMask (s_attrib.c), hardware path: also the Glide depth mask record */
void __stdcall __glsstim_DepthMask(GLboolean flag)
{
    __GL_SETUP_NOT_IN_BEGIN();

    gc->state.depth.writeEnable = flag;
    GRS_SET(__GL_GR_DEPTH_MASK, 0, flag);
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
}

#define w22DbgMsg(s)    ((void (__cdecl *)(const char *))winDebugMessage)(s)
/************************************************************************/
/* IID_IDirectDraw2 comes from the DirectX 6.0 SDK's dxguid.lib (toolchain/lib/dx60sdk/DXGUID.LIB), as in the original:
** its one object, all 379 GUIDs in one section, is the GUID block at 69004238 (IID_IDirectDraw2 at 69004268) */
/* the SURFACE extension's entry words (icdglob.h's grSurface*Ext macros) as the GrProc grGetProcAddress returns */
#define SFC_PROC(entry)     (*(GrProc *)&(entry))
/************************************************************************/
extern void __fastcall winDebugMessage(void);
extern GLboolean __fastcall winResizeColorBuffer(__WGLbuffer *buf, GLint x, GLint y, GLint w, GLint h, __WGLdrawable *dp, GLuint mask);
extern GLboolean __fastcall winResizeDepthBuffer(__WGLbuffer *buf, GLint x, GLint y, GLint w, GLint h, __WGLdrawable *dp, GLuint mask);
extern GLint __fastcall rectArea(const RECT *r);
extern void __fastcall rectOffset(RECT *dst, const RECT *origin, const RECT *src);
extern GLboolean __fastcall winSwapSurfaces(__WGLwindow *w);
extern GLuint __fastcall winInitColorBuffer(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth, void *fallbackInit);
extern GLuint __fastcall winInitDepthBuffer(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth, void *fallbackInit);
extern int __fastcall winOpenDDraw(HWND hwnd);
extern void __fastcall winLockColorBuffer(__WGLbuffer *buf, __WGLdrawable *dp);
extern void __fastcall winUnlockColorBuffer(void);
extern void __fastcall winFreeBuffer(__WGLbuffer *buf, __WGLdrawable *dp);
extern void __fastcall winUnlockDepthBuffer(void);
extern int __fastcall winLock2DepthBuffer();
extern void __fastcall winUnlock2DepthBuffer(void);
extern void __fastcall winFreeDepthBuffer(__WGLbuffer *buf, __WGLdrawable *dp);

#pragma optimize("y", off)      /* the original keeps the ebp frame here (no frame-pointer omission) */
/* 6901aa2e (size 778) -- winRestoreSurfaces: restores the DirectDraw surfaces of a windowed drawable after a mode
** switch (the primary, then the front / back / depth surfaces) and reopens the Glide window surface */
GLboolean __fastcall winRestoreSurfaces(__WGLdrawable *dp, __GLcontext *gc)
{
    DDSURFACEDESC ddsd;
    LPDIRECTDRAWSURFACE *surf = (LPDIRECTDRAWSURFACE *)dp->frontBuffer.private_;
    RECT *r = &dp->windowRect->rect;
    HRESULT hr;
    GLint x, y, w, h;
    GLuint ctx;

    if (g_winPrimary != NULL) {
        hr = IDirectDrawSurface2_IsLost(g_winPrimary);
        if (hr == DD_OK || hr != DDERR_SURFACELOST) return (GLboolean)hr;
        hr = IDirectDrawSurface2_Restore(g_winPrimary);
        if (hr == DDERR_WRONGMODE) {
            IDirectDrawSurface2_Release(g_winPrimary);
            ddsd.dwSize = sizeof(ddsd);
            ddsd.dwFlags = DDSD_CAPS;
            ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
            do {
                Sleep(10);
            } while (IDirectDraw2_CreateSurface(g_winDDraw, &ddsd, (LPDIRECTDRAWSURFACE *)&g_winPrimary, NULL) < 0);
            if (IDirectDrawSurface2_SetClipper(g_winPrimary, g_winClipper) < 0) {
                w22DbgMsg("IDirectDrawSurface2_SetClipper( primary, clipper ) Failed");
                return GL_FALSE;
            }
        } else if (hr != DD_OK) {
            return GL_FALSE;
        }
    } else {
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        if (IDirectDraw2_CreateSurface(g_winDDraw, &ddsd, (LPDIRECTDRAWSURFACE *)&g_winPrimary, NULL) < 0) {
            w22DbgMsg("IDirectDraw2_CreateSurface(pdd, &surfdesc, &primary, NULL) Failed");
            return GL_FALSE;
        }
    }
    if (gc->hwcx.glideVerFlags & 0xfffffffe)
        grSurfaceSetRenderingSurfaceExt2(0, 0);
    else
        grSurfaceSetRenderingSurfaceExt1(0);
    grSurfaceSetAuxSurfaceExt(0);

    x = r->left;
    y = r->top;
    w = r->right - x;
    h = r->bottom - y;
    if (*surf != NULL) {
        if (IDirectDrawSurface_Restore(*surf) == DDERR_WRONGMODE) {
            IDirectDrawSurface_Release(*surf);
            *surf = NULL;
            winResizeColorBuffer(&dp->frontBuffer, x, y, w, h, dp, 1);
        }
    } else {
        winResizeColorBuffer(&dp->frontBuffer, x, y, w, h, dp, 1);
    }
    if (gc->modes.doubleBufferMode) {
        LPDIRECTDRAWSURFACE *back = (LPDIRECTDRAWSURFACE *)dp->backBuffer.private_;   /* own variables: the original's frame slots */
        if (*back != NULL) {
            if (IDirectDrawSurface_Restore(*back) == DDERR_WRONGMODE) {
                IDirectDrawSurface_Release(*back);
                *back = NULL;
                winResizeColorBuffer(&dp->backBuffer, x, y, w, h, dp, 4);
            }
        } else {
            winResizeColorBuffer(&dp->backBuffer, x, y, w, h, dp, 4);
        }
    }
    if (gc->modes.haveDepthBuffer) {
        LPDIRECTDRAWSURFACE *depth = (LPDIRECTDRAWSURFACE *)dp->depthBuffer.private_;
        if (*depth != NULL) {
            if (IDirectDrawSurface_Restore(*depth) == DDERR_WRONGMODE) {
                IDirectDrawSurface_Release(*depth);
                *depth = NULL;
                winResizeDepthBuffer(&dp->depthBuffer, x, y, w, h, dp, 0x20);
            }
        } else {
            winResizeDepthBuffer(&dp->depthBuffer, x, y, w, h, dp, 0x20);
        }
    }

    grSurfaceReleaseContextExt(gc->hwcx.glideContext);
    ctx = grSurfaceCreateContextExt(0);
    grSelectContext(ctx);
    gc->hwcx.glideContext = ctx;
    (*gc->drawablePrivate->lockDP)(gc->drawablePrivate, gc);
    (*gc->drawablePrivate->unlockDP)(gc->drawablePrivate);
    ((void (__fastcall *)(__GLcontext *, GLint, GLint, GLint, GLint))gc->exports[__GL_EXP_NOTIFY_RESIZE])(gc,
        gc->transform.clipX0, gc->transform.clipY0, gc->transform.clipX1, gc->transform.clipY1);
    hwDirtyAllGrState(gc);
    ((void (__fastcall *)(__GLcontext *))hwFlushGrState)(gc);
    texMgrReset(gc, (__GLtexMgrFxICD *)gc->texMgr);
    __GL_DELAY_VALIDATE(gc);
    return GL_TRUE;
}
#pragma optimize("", on)

/* 6901adca (size 585) -- winPresent: presents a windowed drawable: blits the back (or single) buffer's DirectDraw
** surface into the window on the primary surface, restoring the surfaces (6901aa2e) when that fails.  Win loaded
** once at the top, a struct copy of the buffer rect (member-wise for the scissor box), and the blit as an HRESULT
** status chain: the status form is what gives the original's gc/win (edi/esi) allocation. */
GLboolean __fastcall winPresent(__GLcontext *gc)
{
    RECT src, dst;
    DDSURFACEDESC ddsd;
    __WGLwindow *win;
    __WGLwinSurface *sb;
    HRESULT status;

    win = gc->drawablePrivate->window;
    if (win->dp.modes->singleBuffered)
        sb = gc->frontBuffer.buf.drawableBuf;
    else
        sb = gc->backBuffer.buf.drawableBuf;
    if ((gc->state.enables.general & __GL_SCISSOR_TEST_ENABLE) && gc->fx.isScreenSaver == 1) {
        src.left = gc->transform.clipX0;
        src.top = gc->transform.clipY0;
        src.right = gc->transform.clipX1;
        src.bottom = gc->transform.clipY1;
    } else {
        src = sb->rect;
    }
    if (!gc->hwcx.swRendering) grFlush();
    if (win->dp.childWindow) wglWindowResized(win);
    wglUpdateWindowRect(win);
    win->dp.fullScreen = wglIsFullScreenWindow(win);
    if (rectArea(&win->clientRect)) {
        if (IDirectDrawClipper_SetHWnd(g_winClipper, 0, win->hwnd) < 0) {
            w22DbgMsg("IDirectDrawClipper_SetHWnd( clipper, 0, wglPriv->hWnd ) Failed");
            return GL_FALSE;
        }
        EnterCriticalSection(&g_winPrimaryLock);
        rectOffset(&dst, &win->clientRect, &src);
        if (g_winPrimary != NULL && sb->surf != NULL) {
            if (IDirectDrawSurface2_SetClipper(g_winPrimary, g_winClipper) < 0) {
                w22DbgMsg("IDirectDrawSurface2_SetClipper( primary, clipper ) Failed");
                return GL_FALSE;
            }
            ddsd.dwSize = sizeof(ddsd);
            status = IDirectDrawSurface2_GetSurfaceDesc(g_winPrimary, &ddsd);
            if (status == DD_OK && ddsd.ddpfPixelFormat.dwRGBBitCount >= 16)
                status = IDirectDrawSurface2_Blt(g_winPrimary, &dst, (LPDIRECTDRAWSURFACE2)sb->surf, &src, DDBLT_WAIT, NULL);
            if (status != DD_OK || !grSelectContext(gc->hwcx.glideContext)) {
                if (winRestoreSurfaces(gc->drawablePrivate, gc)) {
                    ddsd.dwSize = sizeof(ddsd);
                    if (IDirectDrawSurface2_GetSurfaceDesc(g_winPrimary, &ddsd) == DD_OK
                        && ddsd.ddpfPixelFormat.dwRGBBitCount >= 16)
                        IDirectDrawSurface2_Blt(g_winPrimary, &dst, (LPDIRECTDRAWSURFACE2)sb->surf, &src, DDBLT_WAIT, NULL);
                }
            }
            LeaveCriticalSection(&g_winPrimaryLock);
        } else {
            winRestoreSurfaces(gc->drawablePrivate, gc);
            LeaveCriticalSection(&g_winPrimaryLock);
            return GL_FALSE;
        }
    }
    return GL_TRUE;
}

/* 6901b080 -- winReattachFrontBuffer: re-attaches the front buffer to the window rectangle: the windowed resize (6901b419, front
** buffer mask 1) and its makeCurrent */
void __fastcall winReattachFrontBuffer(__GLcontext *gc)
{
    __WGLdrawable *dp;
    RECT *r;
    __WGLbuffer *wb;
    __GLcolorBufferICD *fb;

    fb = &gc->frontBuffer;
    dp = gc->drawablePrivate;
    r = &dp->windowRect->rect;
    wb = &dp->frontBuffer;
    winResizeColorBuffer(wb, r->left, r->top, r->right - r->left, r->bottom - r->top, dp, 1);
    (*(void (__fastcall *)(__GLcolorBufferICD *, __WGLbuffer *, __GLcontext *))fb->buf.makeCurrent)(fb, wb, gc);
}

/* 6901b148 (size 361) -- winReadBack: reads the window back: blits the window's area of the primary surface into the
** front buffer's DirectDraw surface (restoring the surfaces when that fails) */
GLboolean __fastcall winReadBack(__GLcontext *gc)
{
    RECT dst;
    __WGLwinSurface *sb = gc->frontBuffer.buf.drawableBuf;
    __WGLwindow *win = gc->drawablePrivate->window;

    if (!gc->hwcx.swRendering) grFlush();
    if (win->dp.childWindow) wglWindowResized(win);
    wglUpdateWindowRect(win);
    win->dp.fullScreen = wglIsFullScreenWindow(win);
    if (rectArea(&win->clientRect)) {
        if (IDirectDrawClipper_SetHWnd(g_winClipper, 0, win->hwnd) < 0) {
            w22DbgMsg("IDirectDrawClipper_SetHWnd(clipper, 0, wglPriv->hWnd) Failed");
            return GL_FALSE;
        }
        EnterCriticalSection(&g_winPrimaryLock);
        rectOffset(&dst, &win->clientRect, &sb->rect);
        if (g_winPrimary != NULL && sb->surf != NULL) {
            if (IDirectDrawSurface2_SetClipper(g_winPrimary, g_winClipper) < 0) {
                w22DbgMsg("IDirectDrawSurface2_SetClipper(primary, clipper) Failed");
                return GL_FALSE;
            }
            if (IDirectDrawSurface_Blt(sb->surf, &sb->rect, (LPDIRECTDRAWSURFACE)g_winPrimary, &dst, DDBLT_WAIT, NULL) != DD_OK
                && winRestoreSurfaces(gc->drawablePrivate, gc))
                IDirectDrawSurface_Blt(sb->surf, &sb->rect, (LPDIRECTDRAWSURFACE)g_winPrimary, &dst, DDBLT_WAIT, NULL);
            LeaveCriticalSection(&g_winPrimaryLock);
        } else {
            winRestoreSurfaces(gc->drawablePrivate, gc);
            LeaveCriticalSection(&g_winPrimaryLock);
            return GL_FALSE;
        }
    }
    return GL_TRUE;
}

/* 6901b2c0 -- winSetFrontSurface: selects the front buffer's Glide surface (693c3cfc: one more argument on newer
** Glide versions) */
void __fastcall winSetFrontSurface(__GLcontext *gc, int edx_unused)
{
  __WGLwinSurface *surf;

  surf = gc->frontBuffer.buf.drawableBuf;
  if ((gc->hwcx.glideVerFlags & 0xfffffffe) != 0) {
    grSurfaceSetRenderingSurfaceExt2(surf->surf, 0);
  }
  else {
    grSurfaceSetRenderingSurfaceExt1(surf->surf);
  }
  __GL_DELAY_VALIDATE_MASK(gc, 0x100);
}

/* 6901b320 -- winSetBackSurface: selects the back buffer's Glide surface (693c3cfc: one more argument on newer
** Glide versions) */
void __fastcall winSetBackSurface(__GLcontext *gc, int edx_unused)
{
  __WGLwinSurface *surf;

  surf = gc->backBuffer.buf.drawableBuf;
  if ((gc->hwcx.glideVerFlags & 0xfffffffe) != 0) {
    grSurfaceSetRenderingSurfaceExt2(surf->surf, 0);
  }
  else {
    grSurfaceSetRenderingSurfaceExt1(surf->surf);
  }
  __GL_DELAY_VALIDATE_MASK(gc, 0x100);
}

/************************************************************************/
#pragma optimize("w", on)
/* 6901b419 (size 550) -- winResizeColorBuffer: resize of a colour buffer of a windowed drawable: the front buffer
** of a double-buffered drawable only records the window rectangle; otherwise (re)creates the DirectDraw surface
** (video memory, else system memory) and clears it */
GLboolean __fastcall winResizeColorBuffer(__WGLbuffer *buf, GLint x, GLint y, GLint w, GLint h, __WGLdrawable *dp, GLuint mask)
{
    DDSURFACEDESC ddsd;
    DDBLTFX bltfx;
    RECT *win = &dp->windowRect->rect;
    __WGLwinSurface *sb = (__WGLwinSurface *)buf->private_;
    __WGLmodes *modes;
    LPDIRECTDRAWSURFACE surf;
    GLint width, height;

    width = w ? w : 1;
    height = h ? h : 1;
    if (mask == 1) {
        win->left = x;
        win->top = y;
        win->right = x + width;
        win->bottom = y + height;
        if (!dp->modes->singleBuffered) return GL_TRUE;
    }
    surf = sb->surf;
    if (surf != NULL) {
        ddsd.dwSize = sizeof(ddsd);
        if (IDirectDrawSurface_GetSurfaceDesc(surf, &ddsd) < 0) {
            w22DbgMsg("IDirectDrawSurface_GetSurfaceDesc(sstdbuf->ddsurf, &ddsd) Failed");
            return GL_FALSE;
        }
        if (width != (GLint)ddsd.dwWidth || height != (GLint)ddsd.dwHeight || !sb->inVideo) {
            IDirectDrawSurface_Release(sb->surf);
            sb->surf = NULL;
        }
    }
    sb->rect.left = 0;
    sb->rect.top = 0;
    sb->rect.right = width;
    sb->rect.bottom = height;
    if (sb->surf == NULL) {
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwWidth = width;
        ddsd.dwHeight = height;
        modes = dp->modes;
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;
        ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
        ddsd.ddpfPixelFormat.dwRGBBitCount = modes->rgbBits;
        ddsd.ddpfPixelFormat.dwRBitMask = modes->redMask;
        ddsd.ddpfPixelFormat.dwGBitMask = modes->greenMask;
        ddsd.ddpfPixelFormat.dwBBitMask = modes->blueMask;
        if (IDirectDraw2_CreateSurface(g_winDDraw, &ddsd, &sb->surf, NULL) != DD_OK) {
            ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
            if (IDirectDraw2_CreateSurface(g_winDDraw, &ddsd, &sb->surf, NULL) != DD_OK) {
                w22DbgMsg("Can't allocate system memory for color buffer");
                return GL_FALSE;
            }
            w22DbgMsg("Using system memory for color buffer");
            sb->inVideo = 0;
        } else {
            sb->inVideo = 1;
        }
        surf = sb->surf;
        memset(&bltfx, 0, sizeof(bltfx));
        bltfx.dwSize = sizeof(bltfx);
        bltfx.dwFillColor = 0;
        IDirectDrawSurface_Blt(surf, &sb->rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &bltfx);
    }
    buf->width = width;
    buf->byteWidth = buf->elementSize * width;
    buf->height = height;
    buf->outerWidth = buf->byteWidth / buf->elementSize;
    return GL_TRUE;
}
#pragma optimize("", on)

/************************************************************************/

/* buf->private_ of a DirectDraw-backed buffer of a windowed drawable: __WGLwinSurface (wgldraw.h) */

/* 6901b6e2 (size 440) -- winResizeDepthBuffer: resize of the depth buffer of a windowed drawable: (re)creates its
** DirectDraw surface (video memory, else system memory) when the size changed or it fell back to system memory */
GLboolean __fastcall winResizeDepthBuffer(__WGLbuffer *buf, GLint x, GLint y, GLint w, GLint h, __WGLdrawable *dp, GLuint mask)
{
    DDSURFACEDESC ddsd;
    __WGLwinSurface *sb = (__WGLwinSurface *)buf->private_;
    __WGLmodes *modes;
    GLint width, height, bits;
    LPDIRECTDRAWSURFACE surf;

    width = w;
    if (width == 0) width = 1;
    height = h;
    if (height == 0) height = 1;
    surf = sb->surf;
    if (surf != NULL) {
        ddsd.dwSize = sizeof(ddsd);
        if (IDirectDrawSurface_GetSurfaceDesc(surf, &ddsd) < 0) {
            w22DbgMsg("IDirectDrawSurface_GetSurfaceDesc(sstdbuf->ddsurf, &ddsd) Failed");
            return GL_FALSE;
        }
        if (width != (GLint)ddsd.dwWidth || height != (GLint)ddsd.dwHeight || !sb->inVideo) {
            IDirectDrawSurface_Release(sb->surf);
            sb->surf = NULL;
        }
    }
    sb->rect.left = 0;
    sb->rect.top = 0;
    sb->rect.right = width;
    sb->rect.bottom = height;
    if (sb->surf == NULL) {
        modes = dp->modes;
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwWidth = width;
        ddsd.dwHeight = height;
        ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_3DDEVICE;
        ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
        bits = modes->depthBits;
        if (bits == 24) bits = 32;
        ddsd.ddpfPixelFormat.dwRGBBitCount = bits;
        ddsd.ddpfPixelFormat.dwRBitMask = modes->redMask;
        ddsd.ddpfPixelFormat.dwGBitMask = modes->greenMask;
        ddsd.ddpfPixelFormat.dwBBitMask = modes->blueMask;
        if (IDirectDraw2_CreateSurface(g_winDDraw, &ddsd, &sb->surf, NULL) != DD_OK) {
            ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
            if (IDirectDraw2_CreateSurface(g_winDDraw, &ddsd, &sb->surf, NULL) != DD_OK) {
                w22DbgMsg("Can't allocate system memory for depth buffer");
                return GL_FALSE;
            }
            w22DbgMsg("Using system memory for depth");
            sb->inVideo = 0;
        } else {
            sb->inVideo = 1;
        }
    }
    buf->height = height;
    buf->byteWidth = buf->elementSize * width;
    buf->width = width;
    buf->outerWidth = buf->byteWidth / buf->elementSize;
    return GL_TRUE;
}

/* 6901b8dd -- hwInitDDrawWindow of the windowed (Glide surface) path: the DirectDraw primary and clipper
** (6901bfda; no software fallback without them), the single-buffer swap proc, then the front, back and depth
** buffers (6901b9e0 / 6901bba0).  Typed (the drawable / rect pointers give the original's register order) */
void __fastcall hwInitDDrawWindow(__WGLwindow *w, const __WGLmodes *m)
{
    __WGLdrawable *dp = &w->dp;
    __WGLwindowRect *r = dp->windowRect;

    if (winOpenDDraw(w->hwnd) == 0) {
        OutputDebugStringA("No Software Fallback for no primary surface\n");
    }
    w->swapBuffers = (GLuint (__fastcall *)(__WGLwindow *))winSwapSurfaces;
    r->swFallback = 0;
    winInitColorBuffer(&dp->frontBuffer, dp, m->indexBits, 0);
    if (m->doubleBufferMode) {
        winInitColorBuffer(&dp->backBuffer, dp, m->indexBits, 0);
    }
    if (m->haveDepthBuffer) {
        winInitDepthBuffer(&dp->depthBuffer, dp, m->indexBits, 0);
    }
}

/* 6901b960 (size 125) -- winSwapSurfaces: window->swapBuffers of a windowed single-buffered drawable rendered through
** the Glide surface: exchanges the front and back surfaces */
GLboolean __fastcall winSwapSurfaces(__WGLwindow *w)
{
    __GLcontext *gc = wglGetCurrentGC();

    (*gc->drawablePrivate->lockDP)(gc->drawablePrivate, gc);
    if (w->dp.modes->singleBuffered) {
        __WGLwinSurface *f = (__WGLwinSurface *)w->dp.frontBuffer.private_;
        __WGLwinSurface *b = (__WGLwinSurface *)w->dp.backBuffer.private_;
        LPDIRECTDRAWSURFACE surf = f->surf;
        GLint inVideo = f->inVideo;

        f->surf = b->surf;
        f->inVideo = b->inVideo;
        b->surf = surf;
        b->inVideo = inVideo;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_SCISSOR);
    }
    (*gc->drawablePrivate->unlockDP)(gc->drawablePrivate);
    return GL_TRUE;
}

/* 6901b9e0 (size 145) -- winInitColorBuffer: init of a colour buffer of a windowed drawable (the DirectDraw surface
** comes with the first resize, 6901b419); cf. wglDDrawInitColor */
GLuint __fastcall winInitColorBuffer(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth, void *fallbackInit)
{
    __WGLwinSurface *sb;

    buf->depth = depth;
    buf->height = 0; buf->width = 0; buf->base = NULL; buf->handle = NULL; buf->size = 0; buf->byteWidth = 0;
    buf->elementSize     = ((depth - 1) / 8) + 1;
    buf->elementSizeLog2 = wglLog2(buf->elementSize);
    buf->resize = (GLubyte (__fastcall *)(__WGLbuffer *, GLint, GLint, GLuint, GLuint, __WGLdrawable *, GLuint))winResizeColorBuffer;
    buf->lock   = (void (__fastcall *)(__WGLbuffer *, __WGLdrawable *))winLockColorBuffer;
    buf->unlock = (void (__fastcall *)(__WGLbuffer *, __WGLdrawable *))winUnlockColorBuffer;
    buf->fill   = NULL;
    buf->free   = (void (__fastcall *)(__WGLbuffer *, __WGLdrawable *))winFreeBuffer;
    buf->mainInit = NULL;
    buf->fallbackInit = (void (__fastcall *)(__WGLbuffer *, __WGLdrawable *, GLint))fallbackInit;
    sb = (__WGLwinSurface *)(*dp->calloc)(sizeof(__WGLwinSurface), 1);
    buf->private_ = sb;
    sb->surf = NULL;
    sb->rect.left = -1;
    sb->rect.top = -1;
    sb->rect.right = -1;
    sb->rect.bottom = -1;
    return GL_TRUE;
}

/* 6901ba98 -- winLockColorBuffer: as 6901bc58 (the colour buffers' lock; the gc is read after the surface) */
void __fastcall winLockColorBuffer(__WGLbuffer *buf, __WGLdrawable *dp)
{
    __WGLwindowRect *r = dp->windowRect;
    LPDIRECTDRAWSURFACE *surf;
    __GLcontext *gc;
    HRESULT hr;
    DDSURFACEDESC ddsd;

    surf = (LPDIRECTDRAWSURFACE *)buf->private_;
    gc = wglGetCurrentGC();
    if (gc->renderMode != GL_FEEDBACK) {
        if (r->swFallback == 0) {
            grFinish();
        }
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        while (1) {
            hr = IDirectDrawSurface_Lock(*surf, NULL, &ddsd, DDLOCK_WAIT, NULL);
            if (hr == DD_OK) {
                break;
            }
            if (hr == DDERR_SURFACELOST) {
                winRestoreSurfaces(dp, gc);
            }
            else if (hr != DDERR_SURFACEBUSY) {
                ((int (__cdecl *)(const char *, ...))winDebugMessage)("Can't lock surface (%d)", hr);
            }
        }
        buf->byteWidth = ddsd.lPitch;
        buf->base = ddsd.lpSurface;
        buf->outerWidth = (GLuint)ddsd.lPitch / buf->elementSize;
        IDirectDrawSurface_Unlock(*surf, ddsd.lpSurface);
    }
}

/* 6901bb70 -- winFreeBuffer: buf->free of a windowed colour buffer (installed by 6901b9e0; 6901bd50 below is the
** same code for the depth buffer): release the DirectDraw surface (the first word of buf->private_), forget the locked pointer, then free
** the private block through dp->free.  buf->private_ itself is left pointing at the freed block (as in the
** original). */
void __fastcall winFreeBuffer(__WGLbuffer *buf, __WGLdrawable *dp)
{
    LPDIRECTDRAWSURFACE *surf = (LPDIRECTDRAWSURFACE *)buf->private_;

    if (*surf != NULL) {
        IDirectDrawSurface_Release(*surf);
        *surf = NULL;
        buf->base = NULL;
    }
    (*dp->free)(surf);
}

/* 6901bba0 (size 159) -- winInitDepthBuffer: init of the depth buffer of a windowed drawable (6901b6e2 resizes it) */
GLuint __fastcall winInitDepthBuffer(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth, void *fallbackInit)
{
    __WGLwinSurface *sb;

    buf->depth = depth;
    buf->height = 0; buf->width = 0; buf->base = NULL; buf->handle = NULL; buf->size = 0; buf->byteWidth = 0;
    buf->elementSize     = ((depth - 1) / 8) + 1;
    buf->elementSizeLog2 = wglLog2(buf->elementSize);
    buf->resize = (GLubyte (__fastcall *)(__WGLbuffer *, GLint, GLint, GLuint, GLuint, __WGLdrawable *, GLuint))winResizeDepthBuffer;
    buf->lock   = (void (__fastcall *)(__WGLbuffer *, __WGLdrawable *))winLockDepthBuffer;
    buf->unlock = (void (__fastcall *)(__WGLbuffer *, __WGLdrawable *))winUnlockDepthBuffer;
    buf->fill   = NULL;
    buf->free   = (void (__fastcall *)(__WGLbuffer *, __WGLdrawable *))winFreeDepthBuffer;
    buf->mainInit = NULL;
    buf->fallbackInit = (void (__fastcall *)(__WGLbuffer *, __WGLdrawable *, GLint))fallbackInit;
    sb = (__WGLwinSurface *)(*dp->calloc)(sizeof(__WGLwinSurface), 1);
    buf->private_ = sb;
    sb->surf = NULL;
    sb->rect.left = -1;
    sb->rect.top = -1;
    sb->rect.right = -1;
    sb->rect.bottom = -1;
    sb->lock2 = (void *)winLock2DepthBuffer;
    sb->unlock2 = (void *)winUnlock2DepthBuffer;
    return GL_TRUE;
}

/* 6901bc58 -- winLockDepthBuffer: lock of a windowed DirectDraw buffer (the depth buffer's): unless in feedback
** mode, after grFinish (unless windowRect->swFallback is set), locks the surface (DDLOCK_WAIT; DDERR_SURFACELOST
** restores the drawable's surfaces through 6901aa2e, DDERR_SURFACEBUSY retries, anything else is reported and
** retried), takes its pitch and address, and unlocks it again */
void __fastcall winLockDepthBuffer(__WGLbuffer *buf, __WGLdrawable *dp)
{
    __WGLwindowRect *r = dp->windowRect;
    LPDIRECTDRAWSURFACE *surf;
    __GLcontext *gc;
    HRESULT hr;
    DDSURFACEDESC ddsd;

    gc = wglGetCurrentGC();
    surf = (LPDIRECTDRAWSURFACE *)buf->private_;
    if (gc->renderMode != GL_FEEDBACK) {
        if (r->swFallback == 0) {
            grFinish();
        }
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        while (1) {
            hr = IDirectDrawSurface_Lock(*surf, NULL, &ddsd, DDLOCK_WAIT, NULL);
            if (hr == DD_OK) {
                break;
            }
            if (hr == DDERR_SURFACELOST) {
                winRestoreSurfaces(dp, gc);
            }
            else if (hr != DDERR_SURFACEBUSY) {
                ((int (__cdecl *)(const char *, ...))winDebugMessage)("Can't lock surface (%d)", hr);
            }
        }
        buf->byteWidth = ddsd.lPitch;
        buf->base = ddsd.lpSurface;
        buf->outerWidth = (GLuint)ddsd.lPitch / buf->elementSize;
        IDirectDrawSurface_Unlock(*surf, ddsd.lpSurface);
    }
}

/* 6901bd50 -- winFreeDepthBuffer: winFreeBuffer for the depth buffer (installed by 6901bba0 winInitDepthBuffer) */
void __fastcall winFreeDepthBuffer(__WGLbuffer *buf, __WGLdrawable *dp)
{
    LPDIRECTDRAWSURFACE *surf = (LPDIRECTDRAWSURFACE *)buf->private_;

    if (*surf != NULL) {
        IDirectDrawSurface_Release(*surf);
        *surf = NULL;
        buf->base = NULL;
    }
    (*dp->free)(surf);
}

/* 6901bfda (size 519) -- winOpenDDraw: the first windowed context: fetches the Glide surface extension, creates the
** DirectDraw object, the primary surface and the window clipper; counts the users */
int __fastcall winOpenDDraw(HWND hwnd)
{
    DDSURFACEDESC ddsd;
    LPDIRECTDRAW pddx;

    if (g_winUsers == 0) {
        SFC_PROC(grSurfaceCreateContextExt) = grGetProcAddress("grSurfaceCreateContextExt");
        SFC_PROC(grSurfaceReleaseContextExt) = grGetProcAddress("grSurfaceReleaseContextExt");
        SFC_PROC(grSurfaceSetRenderingSurfaceExt1) = grGetProcAddress("grSurfaceSetRenderingSurfaceExt");
        SFC_PROC(grSurfaceSetAuxSurfaceExt) = grGetProcAddress("grSurfaceSetAuxSurfaceExt");
        SFC_PROC(grSurfaceSetTextureSurfaceExt) = grGetProcAddress("grSurfaceSetTextureSurfaceExt");
        SFC_PROC(grSurfaceCalcTextureWHDExt) = grGetProcAddress("grSurfaceCalcTextureWHDExt");
        if (!SFC_PROC(grSurfaceCreateContextExt) && !SFC_PROC(grSurfaceReleaseContextExt)
            && !SFC_PROC(grSurfaceSetRenderingSurfaceExt1) && !SFC_PROC(grSurfaceSetAuxSurfaceExt)
            && !SFC_PROC(grSurfaceSetTextureSurfaceExt) && !SFC_PROC(grSurfaceCalcTextureWHDExt))
            return 0;   /* only when none of them exists (original bug: the others are called unchecked) */
        if (DirectDrawCreate(NULL, &pddx, NULL) < 0) {
            w22DbgMsg("DirectDrawCreate( 0, &pddx, NULL ) Failed");
            return 0;
        }
        if (IDirectDraw_QueryInterface(pddx, &IID_IDirectDraw2, (LPVOID *)&g_winDDraw) < 0) {
            w22DbgMsg("IDirectDraw_QueryInterface( pddx, &IID_IDirectDraw2, (LPVOID*)&pdd ) Failed");
            return 0;
        }
        IDirectDraw_Release(pddx);
        if (IDirectDraw2_SetCooperativeLevel(g_winDDraw, hwnd, DDSCL_NORMAL) < 0) {
            w22DbgMsg("IDirectDraw2_SetCooperativeLevel( pdd, hwnd, DDSCL_NORMAL ) Failed");
            return 0;
        }
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        if (IDirectDraw2_CreateSurface(g_winDDraw, &ddsd, (LPDIRECTDRAWSURFACE *)&g_winPrimary, NULL) < 0) {
            w22DbgMsg("IDirectDraw2_CreateSurface( pdd, &desc, &primary, NULL ) Failed");
            return 0;
        }
        if (IDirectDraw2_CreateClipper(g_winDDraw, 0, &g_winClipper, NULL) < 0) {
            w22DbgMsg("IDirectDraw2_CreateClipper( pdd, 0, &clipper, 0 ) Failed");
            return 0;
        }
        if (IDirectDrawClipper_SetHWnd(g_winClipper, 0, hwnd) < 0) {
            w22DbgMsg("IDirectDrawClipper_SetHWnd( clipper, 0, hwnd ) Failed");
            return 0;
        }
        if (IDirectDrawSurface2_SetClipper(g_winPrimary, g_winClipper) < 0) {
            w22DbgMsg("IDirectDrawSurface2_SetClipper( primary, clipper ) Failed");
            return 0;
        }
        InitializeCriticalSection(&g_winPrimaryLock);
    }
    return ++g_winUsers;
}

/* 6901c2ea (size 214) -- PrimaryMakeCurrent (the name in its debug message): a windowed context made current: the
** window rectangle, the Glide window surface (software rendering when it cannot be opened), the window's swap proc */
GLuint __fastcall PrimaryMakeCurrent(__GLcontext *gc)
{
    __WGLwindow *win = GC_imports_wscx(gc)->window;
    RECT *r = &win->dp.windowRect->rect;
    GLuint ctx;
    GLint i;

    if (g_winPrimary == NULL) return (GLuint)-1;
    ((void (__cdecl *)(const char *, ...))winDebugMessage)("PrimaryMakeCurrent:%d %d\n", win->dp.width, win->dp.height);
    r->left = win->clientOrigin.x;
    r->top = win->clientOrigin.y;
    r->right = win->width + win->clientOrigin.x;
    r->bottom = win->height + win->clientOrigin.y;
    ctx = grSurfaceCreateContextExt(0);
    if (ctx == 0) {
        gc->hwcx.swRequested = 1;
        ((void (__fastcall *)(__GLcontext *))wglDevForceSWRendering)(gc);
    } else {
        grSelectContext(ctx);
    }
    win->swapBuffers = (GLuint (__fastcall *)(__WGLwindow *))winSwapSurfaces;
    for (i = 0; i < gc->fx.numTMUs; i++)
        gc->hwcx.unitTexture[i] = (void *)-1;
    __GL_DELAY_VALIDATE_MASK(gc, 0x800);
    return ctx;
}

/* 6901c3d0 -- winReleaseSurfaceContext: release the context's Glide surface context, then select board 0 again */
void __fastcall winReleaseSurfaceContext(__GLcontext *gc)
{
    if (gc->hwcx.glideContext != 0) {
        grSurfaceReleaseContextExt(gc->hwcx.glideContext);
        gc->hwcx.glideContext = 0;
        grSstSelect(0);
    }
}

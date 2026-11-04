/*
** wglstand.c -- stand-alone wgl and pixel-format implementations used when 3dfxOGL.dll is
** loaded directly as opengl32.dll (no Microsoft opengl32 in between).  The exported
** wgl* entry points jump through g_wglEntryPoints[] (wglstub.c), whose defaults point
** here; wglBindStandAlone() may rebind them.
*/
#include <string.h>
#ifndef _GDI32_
#define _GDI32_            /* we ARE the GL/WGL exporter: no dllimport on wgl* and gl* */
#endif
#include "wglpriv.h"

DWORD g_nextStandAloneRc = 1;        /* 690ee9c4 */

/* 6900d840 -- DescribePixelFormat: the device's describePixelFormat for format iPixelFormat (1-based); copies up to
** nBytes (at most 40) of the descriptor to ppfd and returns the number of formats, 0 for an invalid one.  With
** ppfd NULL it just counts (but still copies, to NULL, when iPixelFormat > 0 and nBytes != 0) */
int WINAPI wglSADescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd)
{
    PIXELFORMATDESCRIPTOR pfd;
    LONG n;
    if (ppfd != NULL) n = g_device->describePixelFormat(hdc, &pfd, iPixelFormat - 1);
    else              n = g_device->describePixelFormat(hdc, NULL, -1);
    if (n != 0 && iPixelFormat > 0) {
        if (nBytes > sizeof(pfd)) nBytes = sizeof(pfd);
        if (nBytes != 0) memcpy(ppfd, &pfd, nBytes);
    }
    return n;
}

/* 6900d8b0 -- ChoosePixelFormat: classic GDI scoring against the device's formats.  3dfx's
** wglChoosePixelFormat (GLIDE3X/WGL/WGLCMDS.C) with the candidates fetched from the device and the
** best one kept as a copy (bestIndex == -1 instead of ppfdBest == NULL). */
int WINAPI wglSAChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd)
{
    PIXELFORMATDESCRIPTOR pfd, best;
    int n, i, bestIndex = -1;

    if (ppfd->dwFlags != (ppfd->dwFlags &
			  (PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI |
			   PFD_SUPPORT_OPENGL | PFD_GENERIC_FORMAT | PFD_NEED_PALETTE |
			   PFD_NEED_SYSTEM_PALETTE | PFD_DOUBLEBUFFER | PFD_STEREO |
			   PFD_DOUBLEBUFFER_DONTCARE | PFD_STEREO_DONTCARE |
			   PFD_SWAP_COPY | PFD_SWAP_EXCHANGE)))
	return 0;                                                   /* bad dwFlags */
    switch (ppfd->iPixelType) {
    case PFD_TYPE_RGBA:
    case PFD_TYPE_COLORINDEX:
	break;
    default:
	return 0;
    }
    switch (ppfd->iLayerType) {
    case PFD_MAIN_PLANE:
    case PFD_OVERLAY_PLANE:
    case PFD_UNDERLAY_PLANE:
	break;
    default:
	return 0;
    }
    n = g_device->describePixelFormat(hdc, NULL, -1);
    for (i = 0; i < n; ++i) {
	g_device->describePixelFormat(hdc, &pfd, i);
	if (ppfd->iPixelType != pfd.iPixelType) continue;
	if (ppfd->iLayerType != pfd.iLayerType) continue;
	if (((ppfd->dwFlags ^ pfd.dwFlags) & ppfd->dwFlags) &
	    (PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL)) continue;
	if (!(ppfd->dwFlags & PFD_DOUBLEBUFFER_DONTCARE)) {
	    if ((ppfd->dwFlags & PFD_DOUBLEBUFFER) != (pfd.dwFlags & PFD_DOUBLEBUFFER)) continue;
	}
	if (!(ppfd->dwFlags & PFD_STEREO_DONTCARE)) {
	    if ((ppfd->dwFlags & PFD_STEREO) != (pfd.dwFlags & PFD_STEREO)) continue;
	}
	if (ppfd->iPixelType == PFD_TYPE_RGBA && ppfd->cAlphaBits && !pfd.cAlphaBits) continue;
	if (ppfd->iPixelType == PFD_TYPE_RGBA && ppfd->cAccumBits && !pfd.cAccumBits) continue;
	if (ppfd->cDepthBits && !pfd.cDepthBits) continue;
	if (ppfd->cStencilBits && !pfd.cStencilBits) continue;
	if (ppfd->cAuxBuffers && !pfd.cAuxBuffers) continue;
	if (bestIndex == -1) {
	    best = pfd;
	    bestIndex = i;
	    continue;
	}
	if ((ppfd->cColorBits > best.cColorBits && pfd.cColorBits > best.cColorBits) ||
	    (ppfd->cColorBits <= pfd.cColorBits && pfd.cColorBits < best.cColorBits)) {
	    best = pfd;
	    bestIndex = i;
	    continue;
	}
	if (ppfd->iPixelType == PFD_TYPE_RGBA && ppfd->cAlphaBits && pfd.cAlphaBits > best.cAlphaBits) {
	    best = pfd;
	    bestIndex = i;
	    continue;
	}
	if (ppfd->iPixelType == PFD_TYPE_RGBA && ppfd->cAccumBits && pfd.cAccumBits > best.cAccumBits) {
	    best = pfd;
	    bestIndex = i;
	    continue;
	}
	if ((ppfd->cDepthBits > best.cDepthBits && pfd.cDepthBits > best.cDepthBits) ||
	    (ppfd->cDepthBits <= pfd.cDepthBits && pfd.cDepthBits < best.cDepthBits)) {
	    best = pfd;
	    bestIndex = i;
	    continue;
	}
	if (ppfd->cStencilBits && pfd.cStencilBits > best.cStencilBits) {
	    best = pfd;
	    bestIndex = i;
	    continue;
	}
	if (ppfd->cAuxBuffers && pfd.cAuxBuffers > best.cAuxBuffers) {
	    best = pfd;
	    bestIndex = i;
	    continue;
	}
    }
    return bestIndex + 1;
}

/* 6900dab0 -- GetPixelFormat: the iPixelFormat of the DC's window record (wglFindWindow), 0 when it has none */
int WINAPI wglSAGetPixelFormat(HDC hdc)
{
    __WGLwindow *w = wglFindWindow(hdc, WindowFromDC(hdc));
    if (w == NULL) return 0;
    return w->iPixelFormat;
}

/* 6900daf2 -- SetPixelFormat: create the window record (wglCreateWindow with the format's modes) if the DC has
** none, then record iPixelFormat on it.  FALSE when the window already has a format, the format is invalid, or
** it lacks PFD_DRAW_TO_BITMAP (memory DC) / PFD_DRAW_TO_WINDOW (window DC); ppfd is ignored */
BOOL WINAPI wglSASetPixelFormat(HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR *ppfd)
{
    HWND hwnd = WindowFromDC(hdc);
    __WGLwindow *w;
    PIXELFORMATDESCRIPTOR pfd;
    __WGLmodes modes;
    DWORD type;
    (void)ppfd;

    w = wglFindWindow(hdc, hwnd);
    if (w == NULL) {
        PIXELFORMATDESCRIPTOR modePfd;             /* its own frame slot, as in the original */
        if (wglSADescribePixelFormat(hdc, iPixelFormat, sizeof(modePfd), &modePfd) == 0) {
            wglSystemError("wglSetPixelFormat", ERROR_INVALID_PIXEL_FORMAT);
            return FALSE;
        }
        wglModesFromPFD(&modes, &modePfd);
        w = wglCreateWindow(hdc, hwnd, &modes);
    }
    if (w->iPixelFormat != 0) return FALSE;
    if (wglSADescribePixelFormat(hdc, iPixelFormat, sizeof(pfd), &pfd) == 0) return FALSE;
    switch (GetObjectType(hdc)) {
    case OBJ_MEMDC: if (!(pfd.dwFlags & PFD_DRAW_TO_BITMAP)) return FALSE; break;
    case OBJ_DC:    if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW)) return FALSE; break;
    default:        return FALSE;
    }
    w->iPixelFormat = iPixelFormat;
    return TRUE;
}

/* 6900d7b0 -- SwapBuffers: for the first rc bound to the DC's window, if double-buffered, notify its gc
** (notifySwapBuffers) and return the window's swapBuffers proc's result; FALSE otherwise */
BOOL WINAPI wglSASwapBuffers(HDC hdc)
{
    HWND hwnd = WindowFromDC(hdc);
    __WGLrc *rc;
    if (hwnd != NULL) {
        rc = wglRcFromHwnd(hwnd, NULL);
        if (rc != NULL && rc->modes->doubleBufferMode) {
            GC_exports_notifySwapBuffers(rc->gc)(rc->gc);
            return ((GLubyte (__fastcall *)(__WGLwindow *))rc->window->swapBuffers)(rc->window);   /* returns a GLboolean */
        }
    }
    return FALSE;
}

/* 6900d12d -- wglCreateContext: allocate a zeroed rc for the DC's pixel format (a copy of its descriptor), give it
** the next stand-alone handle, create its gc (wglRcCreateGC, layer plane 1) and its own DC (CreateCompatibleDC
** for a memory DC, GetDC of the window otherwise), and link it into the rc list; the handle, or NULL with a
** system error (no pixel format, no memory, gc creation failed, other DC types) */
HGLRC WINAPI wglSACreateContext(HDC hdc)
{
    int iPixelFormat;
    __WGLrc *rc;

    iPixelFormat = wglSAGetPixelFormat(hdc);
    if (iPixelFormat == 0) { wglSystemError("wglCreateContex", ERROR_INVALID_PIXEL_FORMAT); return NULL; }
    rc = (__WGLrc *)wglMalloc(sizeof(__WGLrc));
    if (rc == NULL) { wglSystemError("wglCreateContext", ERROR_NOT_ENOUGH_MEMORY); return NULL; }
    memset(rc, 0, sizeof(*rc));
    if (wglSADescribePixelFormat(hdc, iPixelFormat, sizeof(rc->pfd), &rc->pfd) == 0) {
        wglFree(rc);
        wglSystemError("wglCreateContext", ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }
    rc->isCurrent = FALSE;
    rc->threadId = (DWORD)-1;
    rc->iPixelFormat = iPixelFormat;
    rc->everCurrent = FALSE;
    rc->flags = 0;
    *(BYTE *)&rc->unk_20 = 1;                  /* low byte only */
    wglLock();
    rc->handle = g_nextStandAloneRc++;
    if (!wglRcCreateGC(rc, 1)) {
        wglFree(rc);
        wglSystemError("wglCreateContext", 0xC0000000);
        wglUnlock();
        return NULL;
    }
    switch (GetObjectType(hdc)) {
    case OBJ_MEMDC: rc->hdc = CreateCompatibleDC(hdc); break;
    case OBJ_DC:    rc->hdc = GetDC(WindowFromDC(hdc)); break;
    default:
        wglFree(rc);
        wglSystemError("wglCreateContext", 0xC0000000);
        wglUnlock();
        return NULL;
    }
    wglRcListAdd(rc);
    wglUnlock();
    return (HGLRC)rc->handle;
}

/* 6900d2b5 -- wglDeleteContext: an rc current on this thread is first released (the current gc's loseCurrent,
** its window reference dropped, the null gc and null dispatch installed); then its gc is destroyed and the rc
** unlinked and freed.  FALSE for an unknown handle, an rc current on another thread or a failed lose / destroy */
BOOL WINAPI wglSADeleteContext(HGLRC hrc)
{
    __GLcontext *cur = wglGetCurrentGC();
    __WGLrc *rc = wglRcFromHandle((DWORD)hrc);
    __WGLwindow *w;
    LPCRITICAL_SECTION cs;

    if (rc == NULL) { wglSystemError("wglDeleteContext", ERROR_INVALID_HANDLE); return FALSE; }
    if (rc->isCurrent) {
        if (rc->threadId != GetCurrentThreadId()) goto fail;
        w = rc->window;
        if (!GC_exports_loseCurrent(cur)(cur)) return FALSE;  /* loseCurrent */
        cs = w->dp.lock;
        if (wglDestroyWindow(w)) wglDeleteCritSect(cs);
        rc->window = NULL;
        rc->isCurrent = FALSE;
        rc->threadId = (DWORD)-1;
        rc->hwnd = NULL;
        wglSetCurrentGC(g_nullGC);
        __glInstallNullDispatch();
    }
    if (!wglRcDestroyGC(rc)) { wglSystemError("wglDeleteContext", 0xC0000000); goto fail; }
    wglLock();
    wglRcListRemove(rc);
    wglUnlock();
    wglFree(rc);
    return TRUE;
fail:
    return FALSE;
}

/* 6900d3a0 -- wglSAGetCurrentContext: the handle of the current gc's rc (imports wscx, not NULL-checked) */
HGLRC WINAPI wglSAGetCurrentContext(void)
{
    __GLcontext *gc = wglGetCurrentGC();
    return (HGLRC)(GC_imports_wscx(gc))->handle;
}

/* 6900d3b0 -- wglSAGetCurrentDC: the hdc of the current gc's rc (imports wscx, not NULL-checked) */
HDC WINAPI wglSAGetCurrentDC(void)
{
    __GLcontext *gc = wglGetCurrentGC();
    return (GC_imports_wscx(gc))->hdc;
}

/* 6900d40f -- wglMakeCurrent: with hrc, check the DC (window or memory DC with a bitmap), that the rc is not
** current elsewhere and that the window record (wglGetWindow, created if needed) has the rc's pixel format; TRUE
** at once if that rc / DC / window is already current.  Takes a window reference, releases the current gc
** (loseCurrent, its window reference dropped), then binds: window rect / fullscreen refreshed, wglWindowResized,
** the gc's makeCurrent, TEB gc set, rc marked current on this thread (flags |= 1).  hrc NULL just releases and
** installs the null dispatch.  Finally WGL_CHECK_RESIZE on the rc's gc.  All under the wgl lock */
BOOL WINAPI wglSAMakeCurrent(HDC hdc, HGLRC hrc)
{
    __GLcontext *cur = wglGetCurrentGC();
    __WGLrc *rc = wglRcFromHandle((DWORD)hrc);
    __WGLwindow *w;                               /* (only set and used when hrc != NULL) */
    HWND hwnd = NULL;

    wglLock();
    if (hrc != NULL) {
        DWORD type = GetObjectType(hdc);
        HGDIOBJ hbm = NULL;
        if (rc == NULL) { wglSystemError("wglMakeCurrent", ERROR_INVALID_HANDLE); wglUnlock(); return FALSE; }
        switch (type) {
        case OBJ_DC:    hwnd = WindowFromDC(hdc); break;
        case OBJ_MEMDC: hbm = GetCurrentObject(hdc, OBJ_BITMAP); break;
        default:        wglUnlock(); return FALSE;
        }
        if (hwnd == NULL && hbm == NULL) { wglSystemError("wglMakeCurrent", ERROR_INVALID_HANDLE); wglUnlock(); return FALSE; }
        if (rc->isCurrent && rc->threadId != GetCurrentThreadId()) {
            wglSystemError("wglMakeCurrent", ERROR_INVALID_PARAMETER); wglUnlock(); return FALSE;
        }
        w = wglGetWindow(hdc, hwnd, rc->modes);
        if (w == NULL) { wglSystemError("wglMakeCurrent", ERROR_INVALID_HANDLE); wglUnlock(); return FALSE; }
        if (rc->hdc == hdc && rc->gc == cur && w->hwnd == hwnd) { wglUnlock(); return TRUE; }
        if (w->iPixelFormat != rc->iPixelFormat) {
            wglSystemError("wglMakeCurrent", ERROR_INVALID_PIXEL_FORMAT); wglUnlock(); return FALSE;
        }
        w->dp.refCount++;
    }
    if (cur != g_nullGC) {                        /* release whatever is current */
        __WGLrc *currc = GC_imports_wscx(cur);
        __WGLwindow *curw = currc->window;
        LPCRITICAL_SECTION cs;
        rc->flags &= ~1u;                         /* sic: the new rc, not currc (hrc == 0: the sentinel s_rcHead) */
        if (!((GLboolean (__fastcall *)(__GLcontext *))GC_exports_loseCurrent(cur))(cur)) { wglUnlock(); return FALSE; }
        cs = curw->dp.lock;
        if (wglDestroyWindow(curw)) wglDeleteCritSect(cs);
        currc->threadId = (DWORD)-1;
        currc->window = NULL;
        currc->isCurrent = FALSE;
        rc->hwnd = NULL;
        rc->hdc = NULL;
        wglSetCurrentGC(g_nullGC);
    }
    if (hrc != NULL) {
        __GLcontext *gc = rc->gc;
        rc->window = w;
        wglUpdateWindowRect(w);
        w->dp.fullScreen = (GLboolean)wglIsFullScreenWindow(w);
        rc->flags &= ~1u;
        wglWindowResized(w);
        if (!GC_exports_makeCurrent(gc)(gc)) {   /* makeCurrent */
            w->dp.refCount--;
            rc->hwnd = NULL;
            rc->window = NULL;
            rc->hdc = NULL;
            wglSetCurrentGC(NULL);
            wglUnlock();
            return FALSE;
        }
        wglSetCurrentGC(gc);
        rc->isCurrent = TRUE;
        rc->threadId = GetCurrentThreadId();
        rc->hwnd = hwnd;
        rc->everCurrent = TRUE;
        rc->hdc = hdc;
        rc->flags |= 1;
    } else {
        __glInstallNullDispatch();
    }
    wglUnlock();
    WGL_CHECK_RESIZE(rc->gc);
    return TRUE;
}

/* 6900d71e -- wglShareLists: b's gc shareContext(b, a); FALSE with a system error for an unknown handle or a
** failed share */
BOOL WINAPI wglSAShareLists(HGLRC a, HGLRC b)
{
    __WGLrc *ra, *rb;
    ra = wglRcFromHandle((DWORD)a);
    if (ra == NULL) { wglSystemError("wglShareLists", ERROR_INVALID_HANDLE); return FALSE; }
    rb = wglRcFromHandle((DWORD)b);
    if (rb == NULL) { wglSystemError("wglShareLists", ERROR_INVALID_HANDLE); return FALSE; }
    wglLock();
    if (!((GCfn_bool_gc_gc)GC_exports_shareContext(rb->gc))(rb->gc, ra->gc)) {   /* rb's shareContext(rb->gc, ra->gc) */
        wglSystemError("wglShareLists", ERROR_INVALID_PARAMETER);
        return FALSE;                             /* sic: returns with the lock held */
    }
    wglUnlock();
    return TRUE;
}

/* 6900dc20 -- wglSAGetProcAddress: wglGetProcAddressTable(name) */
PROC WINAPI wglSAGetProcAddress(LPCSTR name)
{
    return (PROC)wglGetProcAddressTable(name);
}

/* the "not supported" group: 6900d81d, 6900dc57, 6900dca4, 6900dcf4, 6900dd48, 6900dd95, 6900dde8, 6900de32 */
BOOL  WINAPI wglSACopyContext(HGLRC src, HGLRC dst, UINT mask)                             { (void)src; (void)dst; (void)mask; wglSystemError("wglCopyContext not supported", 0xC0000000); return FALSE; }
/* 6900dc57 -- wglSAGetDefaultProcAddress: not supported: wglSystemError (code 0xC0000000), NULL */
PROC  WINAPI wglSAGetDefaultProcAddress(LPCSTR name)                                       { (void)name; wglSystemError("wglGetDefaultProcAddress not supported", 0xC0000000); return NULL; }
/* 6900dca4 -- wglSACreateLayerContext: not supported: wglSystemError (code 0xC0000000), NULL */
HGLRC WINAPI wglSACreateLayerContext(HDC hdc, int layer)                                   { (void)hdc; (void)layer; wglSystemError("wglCreateLayerContext not supported", 0xC0000000); return NULL; }
/* 6900dcf4 -- wglSADescribeLayerPlane: not supported: wglSystemError (code 0xC0000000), FALSE */
BOOL  WINAPI wglSADescribeLayerPlane(HDC hdc, int fmt, int layer, UINT n, LPLAYERPLANEDESCRIPTOR lpd) { (void)hdc; (void)fmt; (void)layer; (void)n; (void)lpd; wglSystemError("wglDescribeLayerPlane not supported", 0xC0000000); return FALSE; }
/* 6900dd48 -- wglSAGetLayerPaletteEntries: not supported: wglSystemError (code 0xC0000000), 0 */
int   WINAPI wglSAGetLayerPaletteEntries(HDC hdc, int layer, int start, int n, COLORREF *c) { (void)hdc; (void)layer; (void)start; (void)n; (void)c; wglSystemError("wglGetLayerPaletteEntries not supported", 0xC0000000); return 0; }
/* 6900dd95 -- wglSARealizeLayerPalette: not supported: wglSystemError (code 0xC0000000), FALSE */
BOOL  WINAPI wglSARealizeLayerPalette(HDC hdc, int layer, BOOL realize)                    { (void)hdc; (void)layer; (void)realize; wglSystemError("wglRealizeLayerPalette not supported", 0xC0000000); return FALSE; }
/* 6900dde8 -- wglSASetLayerPaletteEntries: not supported ("wglSetlayer...", sic): wglSystemError, 0 */
int   WINAPI wglSASetLayerPaletteEntries(HDC hdc, int layer, int start, int n, CONST COLORREF *c) { (void)hdc; (void)layer; (void)start; (void)n; (void)c; wglSystemError("wglSetlayerPaletteEntries not supported", 0xC0000000); return 0; }
/* 6900de32 -- wglSASwapLayerBuffers: not supported: wglSystemError (code 0xC0000000), FALSE */
BOOL  WINAPI wglSASwapLayerBuffers(HDC hdc, UINT planes)                                   { (void)hdc; (void)planes; wglSystemError("wglSwapLayerBuffers not supported", 0xC0000000); return FALSE; }

/* the wglGetProcAddress table (690ee000: {name, fn} x 0x72, glstubs.c) */
extern struct { const char *name; PROC fn; } g_extProcTable[0x72];   /* 690ee000 .. 690ee390 */
/* 69009882 -- wglGetProcAddressTable: look name up (exact, case-sensitive) in g_extProcTable and return its
** function, NULL when it is not there; with no current context (the null gc) NULL and ERROR_INVALID_HANDLE */
void *__fastcall wglGetProcAddressTable(const char *name)
{
    int i;
    if (wglGetCurrentGC() == g_nullGC) {
        wglSystemError("wglGetProcAddress", ERROR_INVALID_HANDLE);   /* string 69009870, code 6 */
        return NULL;
    }
    for (i = 0; i < 0x72; i++) {
        const char *a = name, *b = g_extProcTable[i].name;
        while (*a == *b && *a != '\0') { a++; b++; }   /* inline strcmp */
        if (*a == *b && *a == '\0') return (void *)g_extProcTable[i].fn;
    }
    return NULL;
}

/* 69009dd5 -- process detach: destroy every remaining rc (always the list head), then the null context */
void wglReleaseAllRcs(void)
{
    __WGLrc *rc;

    wglLock();
    for (rc = s_rcHead.next; s_rcHead.next != NULL; rc = s_rcHead.next) {
        if (!wglRcDestroyGC(s_rcHead.next)) {
            wglSystemError("wglDeleteContext", 0xC0000000);
            wglUnlock();
            return;
        }
        wglRcListRemove(rc);
        wglFree(rc);
    }
    if (s_nullGCCopy != NULL && !GC_exports_destroyContext(s_nullGCCopy)(s_nullGCCopy))
        wglSystemError("wglDeleteContext", 0xC0000000);
    wglUnlock();
}

/* 69009ee0 -- find the rc owning a gc */
__WGLrc *__fastcall wglRcFromGC(__GLcontext *gc)
{
    __WGLrc *rc;
    wglLock();
    for (rc = (__WGLrc *)&s_rcHead; rc != NULL; rc = rc->next)        /* (sic: starts at the sentinel) */
        if (rc->gc == gc) { wglUnlock(); return rc; }
    wglUnlock();
    return NULL;
}

/*
** drv.c -- the ICD entry points (Drv*) exported to opengl32.dll.
** This is the Microsoft ICD-kit layer
** ("wglDrvCliSetContext" etc.), lightly modified by 3dfx.
*/
#include <string.h>
#include "wglpriv.h"
#include "icd.h"

extern GLboolean (__fastcall *gcMakeCurrentProc(__GLcontext *gc))(void);

/* 6900b940 -- DrvValidateVersion: accept ICD interface versions 0 and 1 only */
BOOL WINAPI DrvValidateVersion(ULONG version)
{
    return 1 >= version;
}

/* 6900b870 -- DrvGetProcAddress: the extension entry point of that name (wglGetProcAddressTable) */
PROC WINAPI DrvGetProcAddress(LPCSTR name)
{
    return (PROC)wglGetProcAddressTable(name);
}

/* 6900b9f0 -- fill *ppfd from the device's pixel format table */
LONG WINAPI DrvDescribePixelFormat(HDC hdc, INT iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd)
{
    PIXELFORMATDESCRIPTOR pfd;
    LONG n;

    wglGetCurrentGC();
    if (ppfd != NULL) n = g_device->describePixelFormat(hdc, &pfd, iPixelFormat - 1);
    else              n = g_device->describePixelFormat(hdc, NULL, -1);
    if (n != 0) {
        if (nBytes > sizeof(pfd)) nBytes = sizeof(pfd);
        if (nBytes != 0) memcpy(ppfd, &pfd, nBytes);
    }
    return n;
}

/* 6900b610 -- DrvSetPixelFormat: find (or create, with the modes of iPixelFormat) the __WGLwindow of hdc and record
** iPixelFormat in it; FALSE if the window already has a format, the format cannot be described, or it lacks
** PFD_DRAW_TO_BITMAP for a memory DC / PFD_DRAW_TO_WINDOW for a window DC (other DC types fail) */
BOOL WINAPI DrvSetPixelFormat(HDC hdc, INT iPixelFormat)
{
    HWND hwnd;
    __WGLwindow *w;
    PIXELFORMATDESCRIPTOR pfd;
    __WGLmodes modes;
    DWORD type;

    hwnd = WindowFromDC(hdc);
    w = wglFindWindow(hdc, hwnd);
    if (w == NULL) {
        PIXELFORMATDESCRIPTOR modePfd;             /* its own frame slot, as in the original */
        if (DescribePixelFormat(hdc, iPixelFormat, sizeof(modePfd), &modePfd) == 0) return FALSE;
        wglModesFromPFD(&modes, &modePfd);
        w = wglCreateWindow(hdc, hwnd, &modes);
    }
    if (w->iPixelFormat != 0) return FALSE;    /* may only be set once */
    if (DescribePixelFormat(hdc, iPixelFormat, sizeof(pfd), &pfd) == 0) return FALSE;
    switch (GetObjectType(hdc)) {
    case OBJ_MEMDC: if (!(pfd.dwFlags & PFD_DRAW_TO_BITMAP)) return FALSE; break;
    case OBJ_DC:    if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW)) return FALSE; break;
    default:        return FALSE;
    }
    w->iPixelFormat = iPixelFormat;
    return TRUE;
}

/* 6900c360 -- DrvCreateContext: DrvCreateLayerContext(hdc, 0), the main plane */
HGLRC WINAPI DrvCreateContext(HDC hdc)
{
    return DrvCreateLayerContext(hdc, 0);
}

/* 6900c0f5 -- DrvCreateLayerContext: allocate a zeroed __WGLrc holding the DC's pixel format descriptor (for a
** nonzero iLayerPlane overwritten from DrvDescribeLayerPlane's descriptor, which that stub never fills), create its
** gc through wglRcCreateGC (always layer 0), keep its own DC (CreateCompatibleDC for a memory DC, GetDC of the
** window otherwise), add it to the rc list and return its handle (g_nextRcHandle++); NULL with a system error on
** failure (no pixel format, no memory, gc creation, other DC types; that last path frees the rc but not its gc) */
HGLRC WINAPI DrvCreateLayerContext(HDC hdc, INT iLayerPlane)
{
    INT   iPixelFormat;
    __WGLrc *rc;

    iPixelFormat = GetPixelFormat(hdc);
    wglGetCurrentGC();
    if (iPixelFormat == 0) {
        wglSystemError("wglCreateLayerContext", ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }
    rc = (__WGLrc *)wglMalloc(sizeof(__WGLrc));           /* 0x19 dwords */
    if (rc == NULL) {
        wglSystemError("wglCreateLayerContext", ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    memset(rc, 0, sizeof(*rc));
    if (DescribePixelFormat(hdc, iPixelFormat, sizeof(rc->pfd), &rc->pfd) == 0) {
        wglFree(rc);
        wglSystemError("wglCreateLayerContext", ERROR_INVALID_PIXEL_FORMAT);
        return NULL;
    }
    rc->pfd.iLayerType = 0;                                 /* +0x4a */
    if (iLayerPlane != 0) {
        LAYERPLANEDESCRIPTOR lpd;               /* (block scope: lets VC6 schedule the byte copies) */
        if (!DrvDescribeLayerPlane(hdc, iPixelFormat, iLayerPlane, sizeof(lpd), &lpd)) {
            wglFree(rc);
            wglSystemError("wglCreateLayerContext", ERROR_INVALID_PIXEL_FORMAT);
            return NULL;
        }
        rc->pfd.iPixelType   = lpd.iPixelType;
        rc->pfd.cColorBits   = lpd.cColorBits;
        rc->pfd.cRedBits     = lpd.cRedBits;
        rc->pfd.cRedShift    = lpd.cRedShift;
        rc->pfd.cGreenBits   = lpd.cGreenBits;
        rc->pfd.cGreenShift  = lpd.cGreenShift;
        rc->pfd.cBlueBits    = lpd.cBlueBits;
        rc->pfd.cBlueShift   = lpd.cBlueShift;
        rc->pfd.cAlphaBits   = lpd.cAlphaBits;
        rc->pfd.cAlphaShift  = lpd.cAlphaShift;
        rc->pfd.cAccumBits   = lpd.cAccumBits;
        rc->pfd.cAccumRedBits   = lpd.cAccumRedBits;
        rc->pfd.cAccumGreenBits = lpd.cAccumGreenBits;
        rc->pfd.cAccumBlueBits  = lpd.cAccumBlueBits;
        rc->pfd.cAccumAlphaBits = lpd.cAccumAlphaBits;
        rc->pfd.cDepthBits   = lpd.cDepthBits;
        rc->pfd.nSize    = sizeof(PIXELFORMATDESCRIPTOR);
        rc->pfd.nVersion = 1;
        rc->pfd.dwFlags  = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_NEED_PALETTE;   /* 0xa4 */
        rc->pfd.cStencilBits = lpd.cStencilBits;
        rc->pfd.cAuxBuffers  = lpd.cAuxBuffers;
        rc->pfd.iLayerType  = (BYTE)iLayerPlane;
        rc->pfd.dwVisibleMask = lpd.crTransparent;          /* +0x50 <- lpd+0x1c */
    }
    wglLock();
    rc->isCurrent    = FALSE;
    rc->threadId     = (DWORD)-1;
    rc->iPixelFormat = iPixelFormat;
    rc->everCurrent  = FALSE;
    rc->flags        = 0;
    *(BYTE *)&rc->unk_20 = 1;                  /* low byte only */
    rc->handle = g_nextRcHandle++;
    if (!wglRcCreateGC(rc, 0)) {
        wglFree(rc);
        wglSystemError("wglCreateLayerContext", 0xC0000000);
        wglUnlock();
        return NULL;
    }
    switch (GetObjectType(hdc)) {
    case OBJ_MEMDC: rc->hdc = CreateCompatibleDC(hdc); break;
    case OBJ_DC:    rc->hdc = GetDC(WindowFromDC(hdc)); break;
    default:
        wglFree(rc);
        wglSystemError("wglCreateLayerContext", 0xC0000000);
        wglUnlock();
        return NULL;
    }
    wglRcListAdd(rc);
    wglUnlock();
    return (HGLRC)rc->handle;
}

/* 6900bf45 -- DrvDeleteContext: destroy the rc's gc (wglRcDestroyGC), delete / release the DC it holds, unlink it
** from the rc list and free it; FALSE for a bad handle, an rc current in another thread or a failed gc destroy;
** a no-op (TRUE) after process detach */
BOOL WINAPI DrvDeleteContext(DHGLRC dhrc)
{
    __WGLrc *rc;
    HDC hdc;
    DWORD type;

    if (g_processDetached) return TRUE;
    rc = wglRcFromHandle(dhrc);
    if (rc == NULL) {
        wglSystemError("wglDeleteContext", ERROR_INVALID_HANDLE);
        return FALSE;
    }
    if (rc->isCurrent && rc->threadId != GetCurrentThreadId()) return FALSE;
    wglLock();
    hdc = rc->hdc;
    if (!wglRcDestroyGC(rc)) {
        wglSystemError("wglDeleteContext", 0xc0000000);   /* the original: 0xc0000000, not a Win32 error code */
        wglUnlock();
        return FALSE;
    }
    switch (GetObjectType(hdc)) {
    case OBJ_MEMDC: DeleteDC(hdc); break;
    case OBJ_DC:    ReleaseDC(WindowFromDC(hdc), hdc); break;
    }
    wglRcListRemove(rc);
    wglUnlock();
    wglFree(rc);
    return TRUE;
}

/* 6900bbf4 -- DrvSetContext ("wglDrvCliSetContext"): unless dhrc is already current on this hdc, make it current:
** 3dfx first MoveWindow()s a client area larger than the screen down to the desktop size; then validate the rc
** (not current elsewhere, window pixel format = the rc's, read with GetPixelFormat on NT4), get / create the window,
** reference it, update its rect, full-screen flag, size and clip region, and call the gc's exports.makeCurrent
** (on failure the null gc goes current and NULL is returned); dhrc 0 installs the null dispatch instead.  Stores the
** callback in the rc and returns the current gc's dispatch table as opengl32's proc table */
PGLCLTPROCTABLE WINAPI DrvSetContext(HDC hdc, DHGLRC dhrc, PFN_SETPROCTABLE callback)
{
    __GLcontext *cur;
    __WGLrc *rc;
    __WGLwindow *w;
    HWND hwnd;
    HGDIOBJ hbm;
    DWORD type;
    RECT client, desktop;
    int cx, cy;

    cur  = wglGetCurrentGC();
    rc   = wglRcFromHandle(dhrc);
    type = GetObjectType(hdc);
    hwnd = NULL;
    hbm = NULL;
    if (rc == NULL || rc->hdc != hdc || rc->gc != cur) {
        switch (type) {
        case OBJ_MEMDC: hbm  = GetCurrentObject(hdc, OBJ_BITMAP); break;
        case OBJ_DC:    hwnd = WindowFromDC(hdc); break;
        default:        return NULL;
        }

        wglLock();
        /* 3dfx: clamp an oversized client area to the device / desktop size */
        GetClientRect(hwnd, &client);
        if (client.right - client.left > GetDeviceCaps(hdc, HORZRES) ||
            client.bottom - client.top > GetDeviceCaps(hdc, VERTRES)) {
            GetWindowRect(GetDesktopWindow(), &desktop);
            cy = (client.bottom - client.top < desktop.bottom) ? client.bottom - client.top : desktop.bottom;
            cx = (client.right - client.left < desktop.right) ? client.right - client.left : desktop.right;
            MoveWindow(hwnd, client.left, client.top, cx + client.left, cy + client.top, TRUE);
        }
        if (dhrc != 0) {
            if (rc == NULL) { wglSystemError("wglDrvCliSetContext", ERROR_INVALID_HANDLE); wglUnlock(); return NULL; }
            if (hwnd == NULL && hbm == NULL) { wglSystemError("wglDrvCliSetContext", ERROR_INVALID_HANDLE); wglUnlock(); return NULL; }
            if (rc->isCurrent && rc->threadId != GetCurrentThreadId()) {
                wglSystemError("wglDrvCliSetContext", ERROR_INVALID_PARAMETER); wglUnlock(); return NULL;
            }
            w = wglGetWindow(hdc, hwnd, rc->modes);
            if (w == NULL) { wglSystemError("wglDrvCliSetContext", ERROR_INVALID_HANDLE); wglUnlock(); return NULL; }
            if (g_isNT4 && w->iPixelFormat == 0)
                w->iPixelFormat = GetPixelFormat(hdc);
            if (w->iPixelFormat != rc->iPixelFormat) {
                wglSystemError("wglDrvCliSetContext", ERROR_INVALID_PIXEL_FORMAT); wglUnlock(); return NULL;
            }
        }                                      /* w is left unset when dhrc == 0 (only used below when != 0) */
        rc->callbacks = (void *)callback;      /* NB: original writes through rc even when dhrc==0 */
        if (dhrc != 0) {
            __GLcontext *gc = rc->gc;
            w->dp.refCount++;
            rc->window = w;
            wglUpdateWindowRect(w);
            w->dp.fullScreen = (GLboolean)wglIsFullScreenWindow(w);
            rc->flags |= 0x10;
            wglWindowResized(w);
            w->dp.updateClipRegion(&w->dp);
            if (!GC_exports_makeCurrent(gc)(gc)) {  /* exports.makeCurrent */
                w->dp.refCount--;
                rc->hwnd = NULL;
                rc->window = NULL;
                wglSetCurrentGC(g_nullGC);
                rc->flags |= 1;
                wglUnlock();
                return NULL;
            }
            wglSetCurrentGC(gc);
            rc->isCurrent   = TRUE;
            rc->threadId    = GetCurrentThreadId();
            rc->hwnd        = hwnd;
            rc->everCurrent = TRUE;
            rc->hdc         = hdc;
            rc->flags = (rc->flags & ~0x10) | 1;
        } else {
            __glInstallNullDispatch();                    /* null dispatch into current gc */
        }
        wglUnlock();
        WGL_CHECK_RESIZE(rc->gc);
    }
    return (PGLCLTPROCTABLE)__glGetCurrentDispatch();   /* &gc->dispatch table for opengl32 */
}

/* 6900b960 -- DrvReleaseContext: if a real gc is current, call its exports.loseCurrent (FALSE if that fails), drop
** its window reference (deleting the window's lock when it was freed), mark its rc not current and put the null gc
** current; dhrc is only used for flag 0x10 and hwnd = NULL on its own rc, not the current one (unchecked) */
BOOL WINAPI DrvReleaseContext(DHGLRC dhrc)
{
    __GLcontext *cur;
    __WGLrc *rc;

    wglLock();
    cur = wglGetCurrentGC();
    rc  = wglRcFromHandle(dhrc);
    if (cur != g_nullGC) {
        __WGLrc *currc = GC_imports_wscx(cur);      /* gc->imports.wscx-like back pointer */
        __WGLwindow *curw = currc->window;
        LPCRITICAL_SECTION cs;
        rc->flags |= 0x10;
        if (!GC_exports_loseCurrent(cur)(cur)) {  /* exports.loseCurrent */
            wglUnlock();
            return FALSE;
        }
        cs = curw->dp.lock;
        if (wglDestroyWindow(curw)) wglDeleteCritSect(cs);
        currc->threadId  = (DWORD)-1;
        currc->window    = NULL;
        currc->isCurrent = FALSE;
        rc->hwnd = NULL;
        wglSetCurrentGC(g_nullGC);
    }
    wglUnlock();
    return TRUE;
}

/* 6900ba9f -- DrvCopyContext: wglCopyContext through the destination gc's exports.copyContext(dst, src, mask);
** FALSE with a system error for bad handles, a mask with bits 20..31 set, or a destination gc that is current */
BOOL WINAPI DrvCopyContext(DHGLRC src, DHGLRC dst, UINT mask)
{
    __WGLrc *rs, *rd;
    __GLcontext *gsrc, *gdst;
    GCfn_bool_gc_gc_u copy;
    BOOL r;

    rs = wglRcFromHandle(src);
    if (rs == NULL) { wglSystemError("DrvCopyContext", ERROR_INVALID_HANDLE); return FALSE; }
    gsrc = rs->gc;
    rd = wglRcFromHandle(dst);
    if (rd == NULL) { wglSystemError("DrvCopyContext", ERROR_INVALID_HANDLE); return FALSE; }
    gdst = rd->gc;
    if (gsrc == NULL) { wglSystemError("DrvCopyContext", ERROR_INVALID_HANDLE); return FALSE; }
    if (mask & 0xfff00000) { wglSystemError("DrvCopyContext", ERROR_INVALID_PARAMETER); return FALSE; }
    if (rd->gc == wglGetCurrentGC()) { wglSystemError("DrvCopyContext", ERROR_ACCESS_DENIED); return FALSE; }
    wglLock();
    copy = GC_exports_copyContext(gdst);
    if (copy != NULL) r = copy(gdst, gsrc, mask);
    else              r = FALSE;
    wglUnlock();
    return r;
}

/* 6900b8ae -- DrvShareLists: wglShareLists through a's gc: exports.shareContext(b->gc, a->gc); FALSE with a system
** error for bad handles or when the share fails */
BOOL WINAPI DrvShareLists(DHGLRC a, DHGLRC b)
{
    __WGLrc *ra, *rb;

    ra = wglRcFromHandle(a);
    if (ra == NULL) { wglSystemError("DrvShareLists", ERROR_INVALID_HANDLE); return FALSE; }
    rb = wglRcFromHandle(b);
    if (rb == NULL) { wglSystemError("DrvShareLists", ERROR_INVALID_HANDLE); return FALSE; }
    wglLock();
    if (!GC_exports_shareContext(ra->gc)(rb->gc, ra->gc)) { /* ra's exports.shareContext(rb->gc, ra->gc) */
        wglSystemError("DrvShareLists", ERROR_INVALID_PARAMETER);
        wglUnlock();
        return FALSE;
    }
    wglUnlock();
    return TRUE;
}

/* 6900b590 -- DrvSwapBuffers: for the first rc bound to hdc's window, if double-buffered, call the gc's
** exports.notifySwapBuffers and then the window's swapBuffers proc and return its result; FALSE otherwise */
BOOL WINAPI DrvSwapBuffers(HDC hdc)
{
    HWND hwnd;
    __WGLrc *rc;
    BOOL r;

    hwnd = WindowFromDC(hdc);
    rc = wglRcFromHwnd(hwnd, NULL);
    wglLock();
    if (rc != NULL && *((BYTE *)rc->modes + 2) /* doubleBufferMode */) {
        GC_exports_notifySwapBuffers(rc->gc)(rc->gc);          /* exports: finish/flush before swap */
        r = ((GLubyte (__fastcall *)(__WGLwindow *))rc->window->swapBuffers)(rc->window);   /* returns a GLboolean */
        wglUnlock();
        return r;
    }
    wglUnlock();
    return FALSE;
}

/* 6900b5f0 -- DrvSwapLayerBuffers: DrvSwapBuffers(hdc) whatever the planes (the planes test is always true); TRUE */
BOOL WINAPI DrvSwapLayerBuffers(HDC hdc, UINT planes)
{
    if ((planes | 1) != 0) DrvSwapBuffers(hdc);   /* sic: always true */
    return TRUE;
}

/* 6900c026 -- DrvDescribeLayerPlane: stub: TRUE for any nonzero pixel format and layer plane without filling plpd;
** otherwise ERROR_INVALID_PIXEL_FORMAT and FALSE */
BOOL WINAPI DrvDescribeLayerPlane(HDC hdc, INT iPixelFormat, INT iLayerPlane, UINT nBytes, LPLAYERPLANEDESCRIPTOR plpd)
{
    (void)hdc; (void)nBytes; (void)plpd;
    wglGetCurrentGC();
    if (iPixelFormat != 0 && iLayerPlane != 0) return TRUE;   /* stub: never fills plpd */
    wglSystemError("wglDescribeLayerPlane", ERROR_INVALID_PIXEL_FORMAT);
    return FALSE;
}

/* 6900b737 -- DrvRealizeLayerPalette: stub: ERROR_INVALID_PIXEL_FORMAT and FALSE for the main plane, FALSE for a
** non-window DC, else TRUE without realizing anything */
BOOL WINAPI DrvRealizeLayerPalette(HDC hdc, INT iLayerPlane, BOOL bRealize)
{
    DWORD type = GetObjectType(hdc);
    (void)bRealize;
    wglGetCurrentGC();
    if (iLayerPlane == 0) { wglSystemError("wglRealizeLayerPalette", ERROR_INVALID_PIXEL_FORMAT); return FALSE; }
    if (type != OBJ_DC) return FALSE;
    WindowFromDC(hdc);
    return TRUE;
}

/* 6900b7a6 -- DrvGetLayerPaletteEntries: stub: 0 for the main plane (error text "wglDescribeLayerPlane") or a
** non-window DC, else 1 without writing pcr */
int WINAPI DrvGetLayerPaletteEntries(HDC hdc, INT iLayerPlane, INT iStart, INT cEntries, COLORREF *pcr)
{
    DWORD type = GetObjectType(hdc);
    (void)iStart; (void)cEntries; (void)pcr;
    wglGetCurrentGC();
    if (iLayerPlane == 0) { wglSystemError("wglDescribeLayerPlane", ERROR_INVALID_PIXEL_FORMAT); return 0; }
    if (type != OBJ_DC) return 0;
    WindowFromDC(hdc);
    return 1;
}

/* 6900b816 -- DrvSetLayerPaletteEntries: stub: always 0 (sets nothing); ERROR_INVALID_PIXEL_FORMAT for the main
** plane (error text "wglDescribeLayerPlane") */
int WINAPI DrvSetLayerPaletteEntries(HDC hdc, INT iLayerPlane, INT iStart, INT cEntries, CONST COLORREF *pcr)
{
    DWORD type = GetObjectType(hdc);
    (void)iStart; (void)cEntries; (void)pcr;
    wglGetCurrentGC();
    if (iLayerPlane == 0) { wglSystemError("wglDescribeLayerPlane", ERROR_INVALID_PIXEL_FORMAT); return 0; }
    if (type == OBJ_DC) WindowFromDC(hdc);
    return 0;
}

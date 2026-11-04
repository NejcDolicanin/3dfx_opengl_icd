/*
** wgldib.c -- system-memory, DIB-section and DCI drawable buffers.
*/
#include <stdio.h>
#include <stdlib.h>
#include "wglpriv.h"

/* ------------------------------------------------------------ memory buffer */
/* 69010d47 -- resize: (re)allocate through the drawable's allocator, 32-byte aligned base */
static GLubyte __fastcall memResize(__WGLbuffer *buf, GLint x, GLint y, GLuint w, GLuint h, __WGLdrawable *dp, GLuint mask)
{
    GLuint size = w * h * buf->elementSize;
    void *p;
    (void)x; (void)y; (void)mask;
    if (size != buf->size) {
        if (buf->handle != NULL) {
            p = dp->realloc(buf->handle, size + 0x1f);
            if (p == NULL) { wglOglError("Mem: Update: Realloc failed"); return 0; }
        } else {
            p = dp->malloc(size + 0x1f);
            if (p == NULL) { wglOglError("Mem: Update: Malloc failed"); return 0; }
        }
        buf->handle = p;
        buf->size = size;
        buf->base = (void *)(((GLuint)p + 0x1f) & ~0x1fu);
    }
    buf->width = w;
    buf->byteWidth = buf->elementSize * w;
    buf->height = h;
    buf->outerWidth = buf->byteWidth / buf->elementSize;
    return 1;
}
/* 69010df0 -- memLock: lock (system memory): nothing to do */
static void __fastcall memLock(__WGLbuffer *buf, __WGLdrawable *dp)   { (void)buf; (void)dp; }   /* 69010df0 */
/* 69010e00 -- memUnlock: unlock (system memory): nothing to do */
static void __fastcall memUnlock(__WGLbuffer *buf, __WGLdrawable *dp) { (void)buf; (void)dp; }   /* 69010e00 */
/* 69010e10 -- memFree: free (system memory): release the block through the drawable's free, clear buf->handle */
static void __fastcall memFree(__WGLbuffer *buf, __WGLdrawable *dp)
{
    if (buf->handle != NULL) { dp->free(buf->handle); buf->handle = NULL; }
}

/* 69010cb0 -- wglMemInit: set buf up as an empty system-memory buffer of the given depth (element size and its
** log2 from it), with the mem resize / lock / unlock / free procs and no fill */
void __fastcall wglMemInit(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth)
{
    (void)dp;
    buf->depth = depth;
    buf->height = 0; buf->width = 0; buf->base = NULL; buf->handle = NULL; buf->size = 0; buf->byteWidth = 0;
    buf->elementSize     = ((depth - 1) / 8) + 1;
    buf->elementSizeLog2 = wglLog2(buf->elementSize);
    buf->fill = NULL;
    buf->resize = memResize;
    buf->lock   = memLock;
    buf->unlock = memUnlock;
    buf->free   = memFree;
}

/* ---------------------------------------------------------- DIB section buffer */
typedef struct {
    HBITMAP     hbm;        /* 0x00 */
    HGDIOBJ     oldBitmap;  /* 0x04 */
    BITMAPINFO *bmi;        /* 0x08 */
    void       *bits;       /* 0x0c */
    LONG        pitch;      /* 0x10 */
    HDC         hdc;        /* 0x14 */
} __WGLdibPriv;

/* 69010e30 -- wglMemUpdateDrawablePalette (3dfx WGLMEM.C __wglMemUpdateDrawablePalette, under the drawable lock):
** an 8-bit window copies its DC's logical palette into the back buffer's DIB colour table.  Unreferenced in the
** original. */
void __fastcall wglMemUpdateDrawablePalette(__WGLwindow *w)
{
    if (w->dp.frontBuffer.elementSize == 1) {
        HPALETTE hPal;
        PALETTEENTRY *entries;
        RGBQUAD *colors;
        int i;

        entries = (PALETTEENTRY *) malloc(256 * sizeof(PALETTEENTRY));
        colors = (RGBQUAD *) malloc(256 * sizeof(RGBQUAD));

        wglCSEnter(w->dp.lock);
        hPal = (HPALETTE)GetCurrentObject(w->hdc, OBJ_PAL);
        GetPaletteEntries(hPal, 0, 256, entries);
        for (i = 0; i < 256; i++) {
            colors[i].rgbRed = entries[i].peRed;
            colors[i].rgbGreen = entries[i].peGreen;
            colors[i].rgbBlue = entries[i].peBlue;
        }

        if (w->dp.modes->doubleBufferMode) {
            SetDIBColorTable(((__WGLdibPriv *)w->dp.backBuffer.private_)->hdc, 0, 256, colors);
        }
        wglCSLeave(w->dp.lock);

        free(entries);
        free(colors);
    }
}

/* 69010f00 -- wglDIBGetDC: the DIB buffer's memory DC (the one its DIB section is selected into) */
HDC __fastcall wglDIBGetDC(__WGLbuffer *buf) { return ((__WGLdibPriv *)buf->private_)->hdc; }

/* 69010ff4 -- resize: create a DIB section of the right size/format */
static GLubyte __fastcall dibResize(__WGLbuffer *buf, GLint x, GLint y, GLuint w, GLuint h, __WGLdrawable *dp, GLuint mask)
{
    __WGLwindow *win = dp->window;
    __WGLdibPriv *pv = (__WGLdibPriv *)buf->private_;
    BITMAPINFO *bmi;
    HBITMAP hbm;
    HDC hdc;
    void *bits;
    UINT usage;                 /* left unset for other depths (the original passes whatever was there) */
    GLint depth, pitch, rows;
    size_t bmiSize;
    char msg[256];
    (void)x; (void)y; (void)mask;

    if (pv == NULL) {
        pv = (__WGLdibPriv *)wglCalloc(1, sizeof(__WGLdibPriv));
        buf->private_ = pv;
    }
    if (pv->hbm != NULL && ((GLint)w > pv->bmi->bmiHeader.biWidth || (GLint)h > pv->bmi->bmiHeader.biHeight)) {
        if (!DeleteObject(pv->hbm)) wglOglError("DIB: Update: DeleteObject failed");
        if (!DeleteDC(pv->hdc))     wglOglError("DIB: Update: DeleteDC failed");
        wglFree(pv->bmi);
        pv->hbm = NULL;
    }
    if (pv->hbm == NULL) {
        hdc = win->hdc;
        depth = buf->depth;
        bmiSize = sizeof(BITMAPINFOHEADER) + 4;             /* 0x2c */
        switch (depth) {
        case 1:  bmiSize = 0x30;  break;                    /* 2 palette indices */
        case 8:  bmiSize = 0x22c; break;                    /* 256 palette indices */
        case 16: bmiSize = 0x38;  break;                    /* 3 masks */
        }
        rows = h + 1;                                       /* one spare row */
        pitch = ((depth * (GLint)w + 0x7f) / 0x80) * 0x10;  /* 128-bit aligned rows */
        bmi = (BITMAPINFO *)wglCalloc(1, bmiSize);
        bmi->bmiHeader.biSize   = sizeof(BITMAPINFOHEADER);
        bmi->bmiHeader.biWidth  = w ? (pitch * 8) / depth : 1;
        bmi->bmiHeader.biHeight = rows ? -rows : 1;         /* top-down */
        bmi->bmiHeader.biPlanes = 1;
        bmi->bmiHeader.biBitCount = (WORD)depth;
        bmi->bmiHeader.biXPelsPerMeter = 0;
        bmi->bmiHeader.biYPelsPerMeter = 0;
        bmi->bmiHeader.biClrUsed = 0;
        bmi->bmiHeader.biClrImportant = 0;
        switch (depth) {
        case 1: {
            WORD *pal = (WORD *)bmi->bmiColors;              /* palette indices 0, 1 */
            bmi->bmiHeader.biCompression = BI_RGB;
            bmi->bmiHeader.biSizeImage = 0;
            usage = DIB_PAL_COLORS;
            pal[0] = 0;
            pal[1] = 1;
            break;
        }
        case 8: {
            WORD *pal;                                       /* identity palette indices */
            int i;
            bmi->bmiHeader.biCompression = BI_RGB;
            bmi->bmiHeader.biSizeImage = 0;
            usage = DIB_PAL_COLORS;
            pal = (WORD *)bmi->bmiColors;
            for (i = 0; i < 256; i++) pal[i] = (WORD)i;
            break;
        }
        case 16: {
            DWORD *masks = (DWORD *)bmi->bmiColors;
            bmi->bmiHeader.biCompression = BI_BITFIELDS;
            bmi->bmiHeader.biSizeImage = pitch * rows;       /* includes the spare row */
            usage = DIB_RGB_COLORS;
            masks[0] = dp->modes->redMask;
            masks[1] = dp->modes->greenMask;
            masks[2] = dp->modes->blueMask;
            break;
        }
        case 24:
        case 32:
            bmi->bmiHeader.biCompression = BI_RGB;
            bmi->bmiHeader.biSizeImage = 0;
            usage = DIB_RGB_COLORS;
            break;
        }
        hbm = CreateDIBSection(hdc, bmi, usage, &bits, NULL, 0);
        if (hbm == NULL) {
            sprintf(msg, "DIB: Update: CreateDIBSection failed: 0x%x 0x%x", (unsigned)GetLastError(), (unsigned)(ULONG_PTR)bits);
            wglOglError(msg);
            return 0;
        }
        pv->hdc = CreateCompatibleDC(hdc);
        pv->oldBitmap = SelectObject(pv->hdc, hbm);
        pv->hbm   = hbm;
        pv->bmi   = bmi;
        pv->bits  = bits;
        pv->pitch = pitch;
        buf->base      = bits;
        buf->byteWidth = pv->pitch;
        buf->outerWidth = pv->pitch / buf->elementSize;
    }
    buf->width  = w;
    buf->height = h;
    return 1;
}
/* 690112c0 -- dibLock: lock (DIB section): nothing to do */
static void __fastcall dibLock(__WGLbuffer *buf, __WGLdrawable *dp)   { (void)buf; (void)dp; }   /* 690112c0 */
/* 690112d0 -- dibUnlock: unlock (DIB section): nothing to do */
static void __fastcall dibUnlock(__WGLbuffer *buf, __WGLdrawable *dp) { (void)buf; (void)dp; }   /* 690112d0 */
/* 690112e0 -- fill: FillRect with a black brush (value ignored) */
static void __fastcall dibFill(__WGLbuffer *buf, __WGLdrawable *dp, GLuint val, GLint x, GLint y, GLint w, GLint h)
{
    __WGLdibPriv *pv = (__WGLdibPriv *)buf->private_;
    HBRUSH hbr;
    RECT r;
    (void)val;
    r.left = x; r.right = x + w; r.top = y; r.bottom = y + h;
    wglCSEnter(dp->lock);
    hbr = CreateSolidBrush(0);
    FillRect(pv->hdc, &r, hbr);
    DeleteObject(hbr);
    wglCSLeave(dp->lock);
}
/* 6901138b -- dibFree: free (DIB section): reselect the old bitmap into the private DC, delete the DIB section and
** the DC (logging failures), free the BITMAPINFO and the __WGLdibPriv, and clear buf->private_ */
static void __fastcall dibFree(__WGLbuffer *buf, __WGLdrawable *dp)
{
    __WGLdibPriv *pv = (__WGLdibPriv *)buf->private_;
    (void)dp;
    if (pv != NULL) {
        SelectObject(pv->hdc, pv->oldBitmap);
        if (!DeleteObject(pv->hbm)) wglOglError("DIB: Free: DeleteObject failed");
        if (!DeleteDC(pv->hdc))     wglOglError("DIB: Free: DeleteDC failed");
        wglFree(pv->bmi);
        wglFree(pv);
    }
    buf->private_ = NULL;
}

/* 69010f20 -- wglDIBInit: set buf up as an empty DIB-section buffer of the given depth (element size and its
** log2 from it), with the dib resize / lock / unlock / fill / free procs; the section is created by the resize */
void __fastcall wglDIBInit(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth)
{
    (void)dp;
    buf->depth = depth;
    buf->height = 0; buf->width = 0; buf->base = NULL; buf->handle = NULL; buf->size = 0; buf->byteWidth = 0;
    buf->elementSize     = ((depth - 1) / 8) + 1;
    buf->elementSizeLog2 = wglLog2(buf->elementSize);
    buf->resize = dibResize;
    buf->lock   = dibLock;
    buf->unlock = dibUnlock;
    buf->fill   = dibFill;
    buf->free   = dibFree;
}

/* 69011530 / 69011540 / 69011550 -- the unmanaged DIB (memory DC) never resizes */
static GLubyte __fastcall udibResize(__WGLbuffer *buf, GLint x, GLint y, GLuint w, GLuint h, __WGLdrawable *dp, GLuint mask)
{ (void)buf; (void)x; (void)y; (void)w; (void)h; (void)dp; (void)mask; return 1; }
/* 69011540 -- udibLock: lock (unmanaged DIB): nothing to do */
static void __fastcall udibLock(__WGLbuffer *buf, __WGLdrawable *dp)   { (void)buf; (void)dp; }
/* 69011550 -- udibUnlock: unlock (unmanaged DIB): nothing to do */
static void __fastcall udibUnlock(__WGLbuffer *buf, __WGLdrawable *dp) { (void)buf; (void)dp; }

/* 69011448 -- "__wglInitUnmanagedDIB": front buffer is the DIB section selected into the memory DC */
void __fastcall wglDIBInitUnmanaged(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth)
{
    __WGLwindow *win = dp->window;
    HGDIOBJ hbm;
    DIBSECTION ds;

    hbm = GetCurrentObject(win->hdc, OBJ_BITMAP);
    if (hbm == NULL) wglOglError("__wglInitUnmanagedDIB: GetCurrentObject failed");
    if (GetObjectA(hbm, sizeof(ds), &ds) == 0) wglOglError("__wglInitUnmanagedDIB: GetObject failed");
    buf->depth = depth;
    buf->elementSize     = ((depth - 1) / 8) + 1;
    buf->elementSizeLog2 = wglLog2(buf->elementSize);
    buf->resize = udibResize;
    buf->lock   = udibLock;
    buf->unlock = udibUnlock;
    buf->fill   = NULL;
    buf->free   = NULL;
    buf->width  = ds.dsBm.bmWidth;
    buf->height = ds.dsBm.bmHeight;
    if (ds.dsBmih.biHeight < 0) win->dp.yInverted = 1;
    else                        win->dp.yInverted = 0;
    buf->base   = ds.dsBm.bmBits;
    buf->byteWidth = (ds.dsBm.bmWidthBytes + 3) & ~3;
    win->width  = buf->width;
    win->height = buf->height;
}

/* --------------------------------------------------------------- DCI primary */
typedef struct { LPVOID dciSurface; HANDLE winWatch; } __WGLdciPriv;

HMODULE g_hDciman;          /* 690f66a0 */
int     g_dciRefs;          /* 690f66a8 */
HDC     g_dciProvider;      /* 690f669c */
LPVOID  g_dciPrimary;       /* 690f6698 */
/* 690f6670..0x6694: the ten DCIMAN32 entry points, in name-table order 690ee9d8 */
static struct {
    HDC    (WINAPI *DCIOpenProvider)(void);
    void   (WINAPI *DCICloseProvider)(HDC);
    int    (WINAPI *DCICreatePrimary)(HDC, LPVOID *);
    void   (WINAPI *DCIDestroy)(LPVOID);
    HANDLE (WINAPI *WinWatchOpen)(HWND);
    void   (WINAPI *WinWatchClose)(HANDLE);
    UINT   (WINAPI *WinWatchGetClipList)(HANDLE, LPRECT, UINT, LPRGNDATA);
    BOOL   (WINAPI *WinWatchDidStatusChange)(HANDLE);
    int    (WINAPI *DCIBeginAccess)(LPVOID, int, int, int, int);
    void   (WINAPI *DCIEndAccess)(LPVOID);
} s_dci;
static const char *s_dciNames[10] = {   /* 690ee9d8 */
    "DCIOpenProvider", "DCICloseProvider", "DCICreatePrimary", "DCIDestroy", "WinWatchOpen",
    "WinWatchClose", "WinWatchGetClipList", "WinWatchDidStatusChange", "DCIBeginAccess", "DCIEndAccess" };

/* 690115c8 -- wglDCIOpen ("__wglInitializeDCI", ref-counted): on the first reference load DCIMAN32.DLL and its ten
** entry points, open the DCI provider and create the DCI primary surface; FALSE on failure (a failed provider or
** primary undoes through wglDCIClose; a failed LoadLibrary keeps the reference counted) */
GLboolean wglDCIOpen(void)
{
    int i;
    if (++g_dciRefs <= 1) {
        if (g_hDciman == NULL) {
            g_hDciman = LoadLibraryA("DCIMAN32.DLL");
            if (g_hDciman == NULL) return FALSE;
            for (i = 0; i < 10; i++) ((FARPROC *)&s_dci)[i] = GetProcAddress(g_hDciman, s_dciNames[i]);
        }
        if (g_dciProvider == NULL && (g_dciProvider = s_dci.DCIOpenProvider()) == NULL) {
            wglOglError("__wglInitializeDCI: DCIOpenProvider failed");
            wglDCIClose();
            return FALSE;
        }
        if (g_dciPrimary == NULL) {
            if (s_dci.DCICreatePrimary(g_dciProvider, &g_dciPrimary) != 0) {
                wglOglError("__wglInitializeDCI: DCICreatePrimary failed");
                wglDCIClose();
                return FALSE;
            }
        }
    }
    return TRUE;
}

/* 69011690 -- wglDCIClose: drop a DCI reference; on the last one destroy the DCI primary, close the provider and
** free DCIMAN32.DLL; always TRUE */
GLboolean wglDCIClose(void)
{
    if (--g_dciRefs == 0) {
        if (g_dciPrimary != NULL)  { s_dci.DCIDestroy(g_dciPrimary); g_dciPrimary = NULL; }
        if (g_dciProvider != NULL) { s_dci.DCICloseProvider(g_dciProvider); g_dciProvider = NULL; }
        if (g_hDciman != NULL)     { FreeLibrary(g_hDciman); g_hDciman = NULL; }
    }
    return TRUE;
}

/* 690116f0 -- an empty function, unreferenced in the original (between wglDCIClose and the DCI buffer procs) */
void wglDIBNop(void)
{
}

/* 69011890 -- resize: bind the DCI primary and a WinWatch for the window */
static GLubyte __fastcall dciResize(__WGLbuffer *buf, GLint x, GLint y, GLuint w, GLuint h, __WGLdrawable *dp, GLuint mask)
{
    __WGLwindow *win = dp->window;
    __WGLdciPriv *pv = (__WGLdciPriv *)buf->handle;
    (void)x; (void)y; (void)mask;
    if (pv == NULL) { pv = (__WGLdciPriv *)wglCalloc(1, sizeof(*pv)); buf->handle = pv; }
    if (pv->dciSurface == NULL) {
        pv->dciSurface = g_dciPrimary;
        pv->winWatch = s_dci.WinWatchOpen(win->hwnd);
    }
    buf->width = w;
    buf->height = h;
    return 1;
}

/* DCISURFACEINFO (dciddi.h): the fields dciLock uses */
typedef struct {
    DWORD dwSize;               /* 0x00 */
    DWORD dwDCICaps;            /* 0x04 */
    DWORD dwCompression;        /* 0x08 */
    DWORD dwMask[3];            /* 0x0c */
    DWORD dwWidth;              /* 0x18 */
    DWORD dwHeight;             /* 0x1c */
    LONG  lStride;              /* 0x20 */
    DWORD dwBitCount;           /* 0x24 */
    ULONG_PTR dwOffSurface;     /* 0x28 */
    WORD  wSelSurface;          /* 0x2c */
    WORD  wReserved;
} __WGLdciSurfaceInfo;

/* 6901193d -- lock: DCIBeginAccess on the window rect; resolve a selector:offset surface */
static void __fastcall dciLock(__WGLbuffer *buf, __WGLdrawable *dp)
{
    __WGLwindow *win = dp->window;
    __WGLdciPriv *pv = (__WGLdciPriv *)buf->handle;
    __WGLdciSurfaceInfo *surf = (__WGLdciSurfaceInfo *)pv->dciSurface;
    HANDLE winWatch = pv->winWatch;
    LDT_ENTRY ldt;
    int rc;

    rc = s_dci.DCIBeginAccess(surf, win->clientRect.left, win->clientRect.top,
                              win->clientRect.right - win->clientRect.left,
                              win->clientRect.bottom - win->clientRect.top);
    if (rc != 0 && rc < 0) wglOglError("DCI: Lock: DCIBeginAccess failed");
    if (surf->wSelSurface != 0) {
        /* selector surface: the selector's linear base (dwOffSurface is not added, sic) */
        if (GetThreadSelectorEntry(GetCurrentThread(), surf->wSelSurface, &ldt) != TRUE)
            wglOglError("DCI: Lock: GetThreadSelectorEntry failed");
        buf->base = (BYTE *)(ldt.BaseLow | ((ldt.HighWord.Bytes.BaseHi << 8 | ldt.HighWord.Bytes.BaseMid) << 16))
                  + buf->elementSize * win->clientRect.left + win->clientRect.top * surf->lStride;
    } else {
        buf->base = (BYTE *)(buf->elementSize * win->clientRect.left + win->clientRect.top * surf->lStride + surf->dwOffSurface);
    }
    buf->byteWidth = surf->lStride;
    if (win->clipChanged == 0 && !s_dci.WinWatchDidStatusChange(winWatch)) return;
    wglDCIUpdateClipList(win, winWatch);
    win->clipChanged = 0;
}

/* 69011a40 -- fetch the WinWatch clip list into the ownership region */
void __fastcall wglDCIUpdateClipList(__WGLwindow *win, HANDLE winWatch)
{
    UINT size;
    RGNDATA *rgn;
    wglCSEnter(win->dp.lock);
    size = s_dci.WinWatchGetClipList(winWatch, (LPRECT)&win->clipRect, 0, NULL);
    if (size > 0) {
        rgn = (RGNDATA *)wglMalloc(size);
        s_dci.WinWatchGetClipList(winWatch, (LPRECT)&win->clipRect, size, rgn);
        wglDrawSetRegion(win, rgn);
        wglFree(rgn);
    } else {
        wglDrawSetRegion(win, NULL);
    }
    wglCSLeave(win->dp.lock);
}

/* 69011ac0 -- dciUnlock: unlock: DCIEndAccess on the buffer's DCI surface */
static void __fastcall dciUnlock(__WGLbuffer *buf, __WGLdrawable *dp)
{
    (void)dp;
    s_dci.DCIEndAccess(((__WGLdciPriv *)buf->handle)->dciSurface);
}
/* 69011ad0 -- fill: FillRect on the window DC */
static void __fastcall dciFill(__WGLbuffer *buf, __WGLdrawable *dp, GLuint val, GLint x, GLint y, GLint w, GLint h)
{
    __WGLwindow *win = dp->window;
    HBRUSH hbr = CreateSolidBrush(0);
    RECT r;
    (void)buf; (void)val;
    r.left = x; r.right = x + w; r.top = y; r.bottom = y + h;
    FillRect(win->hdc, &r, hbr);
    DeleteObject(hbr);
}
/* 69011b30 -- dciFree: free: close the WinWatch, free the __WGLdciPriv, clear buf->handle (DCI ref kept) */
static void __fastcall dciFree(__WGLbuffer *buf, __WGLdrawable *dp)
{
    __WGLdciPriv *pv = (__WGLdciPriv *)buf->handle;
    (void)dp;
    if (pv != NULL) { s_dci.WinWatchClose(pv->winWatch); wglFree(pv); }
    buf->handle = NULL;
}

/* 69011830 -- wglDCIInit: set buf up as an empty DCI primary buffer of the given depth with the dci resize / lock /
** unlock / fill / free procs, and take a DCI reference (wglDCIOpen, its result ignored); elementSizeLog2 is not
** set */
void __fastcall wglDCIInit(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth)
{
    (void)dp;
    buf->depth  = depth;
    buf->height = 0; buf->width = 0; buf->base = NULL; buf->handle = NULL; buf->size = 0; buf->byteWidth = 0;
    buf->resize = dciResize;
    buf->lock   = dciLock;
    buf->unlock = dciUnlock;
    buf->elementSize = ((depth - 1) / 8) + 1;
    buf->fill   = dciFill;
    buf->free   = dciFree;
    wglDCIOpen();
}

/* 69011762 -- "__wglDCIDIBSwapBuffers": BitBlt the DIB back buffer to the window */
GLboolean __fastcall wglDCIDIBSwapBuffers(__WGLwindow *w)
{
    __WGLswapRegion *rg = w->swapRegion;
    __WGLbuffer *back = &w->dp.backBuffer;
    BOOL ok;
    if (rg->numRects != 0) {
        GLint x = rg->boundingRect.x0, y = rg->boundingRect.y0;
        GLint cx = rg->boundingRect.x1 - x, cy = rg->boundingRect.y1 - y;
        ok = BitBlt(w->hdc, x, y, cx, cy, wglDIBGetDC(back), x, y, SRCCOPY);
        if (!ok) wglOglError("__wglDCIDIBSwapBuffers: BitBlt failed");
        rg->numRects = 0;
        return (GLboolean)ok;
    }
    ok = BitBlt(w->hdc, 0, 0, w->width, w->height, wglDIBGetDC(back), 0, 0, SRCCOPY);
    if (!ok) wglOglError("__wglDCIDIBSwapBuffers: BitBlt (2) failed");
    return (GLboolean)ok;
}

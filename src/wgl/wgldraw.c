/*
** wgldraw.c -- generic drawable-private code of the WGL layer (MS ICD-kit flavour of the
** SGI __GLdrawablePrivate).
*/
#include <string.h>
#include "wglpriv.h"

void __fastcall dciUpdateClipRegion(void);   /* 69011700 empty updateClipRegion (DCI) */
void __fastcall dibUpdateClipRegion(void);   /* 69010f10 empty updateClipRegion (DIB) */

/* 6900c9b0 -- wglCSEnter: EnterCriticalSection, a tiny wrapper the buffer code calls through */
void __fastcall wglCSEnter(LPCRITICAL_SECTION cs) { EnterCriticalSection(cs); }
/* 6900c9c0 -- wglCSLeave: the matching LeaveCriticalSection wrapper */
void __fastcall wglCSLeave(LPCRITICAL_SECTION cs) { LeaveCriticalSection(cs); }

/* 6907a5d0 -- __glFloorLog2 (s_math.c): floor(log2(n)), 0 for n <= 1 */
GLuint __fastcall wglLog2(GLuint n)
{
    GLint i = 1;

    while ((n >> i) > 0)
        i++;
    return i - 1;
}

/* 69009fc0 -- addSwapRect: append a rectangle (GL coords, y up) to the window's swap region */
void __fastcall wglDrawAddSwapRect(__WGLdrawable *dp, GLint x, GLint y, GLsizei w, GLsizei h)
{
    __WGLwindow *win = dp->window;
    __WGLswapRegion *rg;
    __WGLregionRect *r;

    wglLock();
    if (win->swapRegion->numRects == win->swapRectCapacity) {
        win->swapRectCapacity *= 2;
        win->swapRegion = (__WGLswapRegion *)wglRealloc(win->swapRegion, win->swapRectCapacity * 0x10 + 0x24);
    }
    rg = win->swapRegion;
    r  = (__WGLregionRect *)((BYTE *)rg + (rg->numRects + 2) * 0x10);
    r->x0 = x;
    r->x1 = x + w;
    y = win->height - y;                 /* flip to window coords */
    r->y1 = y;
    r->y0 = y - h;
    if (++rg->numRects == 1) {
        rg->byteSize = 0x10;
        rg->boundingRect = *r;
    } else {
        rg->byteSize += 0x10;
        if (r->x0 < rg->boundingRect.x0) rg->boundingRect.x0 = r->x0;
        if (r->x1 > rg->boundingRect.x1) rg->boundingRect.x1 = r->x1;
        if (r->y0 < rg->boundingRect.y0) rg->boundingRect.y0 = r->y0;
        if (r->y1 > rg->boundingRect.y1) rg->boundingRect.y1 = r->y1;
    }
    wglUnlock();
}

/* 6900a0a0 -- setClipRect: wglDrawSetClipRect: store x, y, w, h as the window's clipRect (x0, y0, x1, y1) and set
** clipChanged, which makes the DCI lock / DirectDraw clip-list update refetch the clip list */
void __fastcall wglDrawSetClipRect(__WGLdrawable *dp, GLint x, GLint y, GLsizei w, GLsizei h)
{
    __WGLwindow *win = dp->window;
    win->clipRect.x0 = x;
    win->clipRect.y0 = y;
    win->clipRect.x1 = x + w;
    win->clipRect.y1 = y + h;
    win->clipChanged = 1;
}

/* 6900c9d0 -- lockDP: take the window lock and service deferred resize/destroy flags */
void __fastcall wglDrawLockDP(__WGLdrawable *dp, __GLcontext *gc)
{
    __WGLrc *rc = GC_imports_wscx(gc);
    DWORD flags;

    EnterCriticalSection(dp->lock);
    flags = rc->flags;
    if (flags & 7) {
        if ((flags & 1) && !(flags & 0x10)) {            /* window moved / resized */
            __WGLwindow *w = rc->window;
            rc->flags = flags & ~1;
            wglDrawResize(dp, w->clientOrigin.x, w->clientOrigin.y, w->width, w->height);
            GC_exports_notifyResize(gc)(gc, w->clientOrigin.x, w->clientOrigin.y, w->width, w->height);
        }
        if (flags & 2) {                                 /* window destroyed */
            rc->flags &= ~2;
            GC_exports_notifyDestroy(gc)(gc);
        }
    }
}

/* 6900ca50 -- unlockDP: leave the drawable's critical section (dp->lock) */
void __fastcall wglDrawUnlockDP(__WGLdrawable *dp)
{
    LeaveCriticalSection(dp->lock);
}

/* 6900cce0 -- resize every buffer the modes ask for.  Each buffer's resize() returns
** 0 (fail), 1 (ok, accelerated) or 0x10 (ok, fell back to a memory buffer); the
** accelerated ones are collected in accelBufferMask.
*/
GLubyte __fastcall wglDrawResize(__WGLdrawable *dp, GLint x, GLint y, GLuint w, GLuint h)
{
    const __WGLmodes *m = dp->modes;
    GLubyte r;
    GLuint mask;

    wglCSEnter(dp->lock);
    r = dp->frontBuffer.resize(&dp->frontBuffer, x, y, w, h, dp, 1);
    if (r == 0) return 0;
    mask = 1;
    if (r == WGLBUF_FALLBACK) mask = 0;
    if (m->doubleBufferMode) {
        r = dp->backBuffer.resize(&dp->backBuffer, x, y, w, h, dp, 4);
        if (r == 0) return 0;
        mask |= 4; if (r == WGLBUF_FALLBACK) mask &= ~4u;
    }
    if (m->haveAccumBuffer) {
        r = dp->accumBuffer.resize(&dp->accumBuffer, x, y, w, h, dp, 0x10);
        if (r == 0) return 0;
        mask |= 0x10; if (r == WGLBUF_FALLBACK) mask &= ~0x10u;
    }
    if (m->haveDepthBuffer) {
        r = dp->depthBuffer.resize(&dp->depthBuffer, x, y, w, h, dp, 0x20);
        if (r == 0) return 0;
        mask |= 0x20; if (r == WGLBUF_FALLBACK) mask &= ~0x20u;
    }
    if (m->haveStencilBuffer) {
        r = dp->stencilBuffer.resize(&dp->stencilBuffer, x, y, w, h, dp, 0x40);
        if (r == 0) return 0;
        mask |= 0x40; if (r == WGLBUF_FALLBACK) mask &= ~0x40u;
    }
    dp->accelBufferMask = mask;
    wglCSLeave(dp->lock);
    return 1;
}

/* 6900ce70 -- wglFreeBuffers (SGI WGLBUF.C __wglFreeBuffers, under the drawable lock): free every buffer of the
** window's drawable.  Unreferenced in the original (windows free their buffers through dp->freeBuffers). */
void __fastcall wglFreeBuffers(__WGLwindow *w)
{
    __WGLdrawable *dp = &w->dp;

    wglCSEnter(dp->lock);
    if (dp->frontBuffer.free) {
        (*dp->frontBuffer.free)(&dp->frontBuffer, dp);
    }
    if (dp->backBuffer.free) {
        (*dp->backBuffer.free)(&dp->backBuffer, dp);
    }
    if (dp->accumBuffer.free) {
        (*dp->accumBuffer.free)(&dp->accumBuffer, dp);
    }
    if (dp->depthBuffer.free) {
        (*dp->depthBuffer.free)(&dp->depthBuffer, dp);
    }
    if (dp->stencilBuffer.free) {
        (*dp->stencilBuffer.free)(&dp->stencilBuffer, dp);
    }
    wglCSLeave(dp->lock);
}

#define R(i) (((__WGLregionRect *)rgn->Buffer)[i])
/* 6900cef0 -- rebuild the ownership region from a GDI/DDraw RGNDATA (screen coords),
** translated to window coords and clipped against the window's clip rect.  NULL means
** "the clip rect is the region"; an empty RGNDATA means "nothing visible".
*/
void __fastcall wglDrawSetRegion(__WGLwindow *win, const RGNDATA *rgn)
{
    GLint i;
    __WGLregionRect clip;           /* a struct (x0 / y0 stay in registers; x1 / y1 get its frame slots) */
    __WGLdrawable *dp = &win->dp;

    if (dp->rects != NULL) wglFree(dp->rects);
    clip.y1 = win->clipRect.y1;
    clip.x1 = win->clipRect.x1;
    clip.y0 = win->clipRect.y0;
    clip.x0 = win->clipRect.x0;
    if (rgn == NULL) {
        dp->numRects = 1;
        dp->rects = (__WGLregionRect *)wglMalloc(sizeof(__WGLregionRect));
        dp->rects[0].x0 = win->clipRect.x0;
        dp->rects[0].y0 = win->clipRect.y0;
        dp->rects[0].x1 = win->clipRect.x1;
        dp->rects[0].y1 = win->clipRect.y1;
        return;
    }
    if (rgn->rdh.nCount == 0) {
        dp->numRects = 1;
        dp->rects = (__WGLregionRect *)wglMalloc(sizeof(__WGLregionRect));
        dp->rects[0].x0 = 0;
        dp->rects[0].x1 = 0;
        dp->rects[0].y0 = 0;
        dp->rects[0].y1 = 0;
        return;
    }
    for (i = 0; i < (GLint)rgn->rdh.nCount; i++) {   /* RECT {l,t,r,b} -> {x0,y0,x1,y1} in window coords */
        __WGLregionRect *r = &R(i);
        r->x0 -= win->clientOrigin.x;
        r->y0 -= win->clientOrigin.y;
        r->x1 -= win->clientOrigin.x;
        r->y1 -= win->clientOrigin.y;
        r->y0 = r->y0 > clip.y0 ? r->y0 : clip.y0;
        r->y0 = r->y0 < clip.y1 ? r->y0 : clip.y1;
        r->y1 = r->y1 > clip.y0 ? r->y1 : clip.y0;
        r->y1 = r->y1 < clip.y1 ? r->y1 : clip.y1;
        r->x0 = r->x0 > clip.x0 ? r->x0 : clip.x0;
        r->x0 = r->x0 < clip.x1 ? r->x0 : clip.x1;
        r->x1 = r->x1 > clip.x0 ? r->x1 : clip.x0;
        r->x1 = r->x1 < clip.x1 ? r->x1 : clip.x1;
    }
    dp->numRects = rgn->rdh.nCount;
    dp->rects = (__WGLregionRect *)wglMalloc(dp->numRects * sizeof(__WGLregionRect));
    for (i = 0; i < dp->numRects; i++) {
        __WGLregionRect *d = &dp->rects[i];
        const __WGLregionRect *s = &R(i);

        d->x0 = s->x0;
        d->y0 = s->y0;
        d->x1 = s->x1;
        d->y1 = s->y1;
    }
}
#undef R

/* 6900ca60 -- window on screen: DirectDraw primary + offscreen surfaces (memory fallback) */
void __fastcall wglDrawInitDDraw(__WGLwindow *w, const __WGLmodes *m)
{
    __WGLdrawable *dp = &w->dp;
    GLint rgbBits = m->rgbBits;
    GLint accumBits = m->accumRedBits + m->accumGreenBits + m->accumBlueBits + m->accumAlphaBits;

    w->present         = (GLuint (__fastcall *)(__WGLwindow *))wglDDrawSwapBuffers;   /* returns a GLboolean */
    w->copyBackToFront = (GLuint (__fastcall *)(__WGLwindow *))wglDDrawCopyBackToFront;
    w->swapBuffers     = (GLuint (__fastcall *)(__WGLwindow *))wglDDrawSwapBuffers;   /* returns a GLboolean */
    w->dp.updateClipRegion = wglDDrawUpdateClipList;
    if (m->doubleBufferMode) {
        if (m->colorIndexMode) {
            wglDDrawInitPrimary(&dp->frontBuffer, dp);
            wglDDrawInitColor(&dp->backBuffer, dp, m->indexBits, wglMemInit);
        } else {
            wglDDrawInitPrimary(&dp->frontBuffer, dp);
            wglDDrawInitColor(&dp->backBuffer, dp, rgbBits, wglMemInit);
        }
    } else {
        wglDDrawInitPrimary(&dp->frontBuffer, dp);
    }
    dp->yInverted = 1;
    if (m->haveAccumBuffer)   wglMemInit(&dp->accumBuffer, dp, accumBits);
    if (m->haveDepthBuffer)   wglDDrawInitDepth(&dp->depthBuffer, dp, m->depthBits, wglMemInit);
    if (m->haveStencilBuffer) wglDDrawInitColor(&dp->stencilBuffer, dp, m->stencilBits, wglMemInit);
}

/* 6900cb50 -- window on screen without DirectDraw: DCI primary + DIB back buffer */
void __fastcall wglDrawInitDCI(__WGLwindow *w, const __WGLmodes *m)
{
    __WGLdrawable *dp = &w->dp;
    GLint rgbBits = m->rgbBits;
    GLint accumBits = m->accumRedBits + m->accumGreenBits + m->accumBlueBits + m->accumAlphaBits;

    w->present         = (GLuint (__fastcall *)(__WGLwindow *))wglDCIDIBSwapBuffers;   /* returns a GLboolean */
    w->copyBackToFront = NULL;
    w->swapBuffers     = (GLuint (__fastcall *)(__WGLwindow *))wglDCIDIBSwapBuffers;   /* returns a GLboolean */
    w->dp.updateClipRegion = (void (__fastcall *)(__WGLdrawable *))dciUpdateClipRegion;   /* empty */
    if (m->doubleBufferMode) {
        if (m->colorIndexMode) {
            wglDCIInit(&dp->frontBuffer, dp, m->indexBits);
            wglDIBInit(&dp->backBuffer, dp, m->indexBits);
        } else {
            wglDCIInit(&dp->frontBuffer, dp, rgbBits);
            wglDIBInit(&dp->backBuffer, dp, rgbBits);
        }
    } else {
        if (m->colorIndexMode) wglDCIInit(&dp->frontBuffer, dp, m->indexBits);
        else                   wglDCIInit(&dp->frontBuffer, dp, rgbBits);
    }
    dp->yInverted = 1;
    if (m->haveAccumBuffer)   wglMemInit(&dp->accumBuffer, dp, accumBits);
    if (m->haveDepthBuffer)   wglMemInit(&dp->depthBuffer, dp, m->depthBits);
    if (m->haveStencilBuffer) wglDIBInit(&dp->stencilBuffer, dp, m->stencilBits);
}

/* 6900cc40 -- memory DC: render straight into the selected DIB section (3dfx __wglDIBInitDrawable,
** SRC/WGL/WGLBUF.C; the colour-index / RGB calls kept separate as there) */
void __fastcall wglDrawInitDIB(__WGLwindow *w, const __WGLmodes *m)
{
    __WGLdrawable *dp = &w->dp;
    GLint rgbBits = m->redBits + m->greenBits + m->blueBits + m->alphaBits;
    GLint accumBits = m->accumRedBits + m->accumGreenBits + m->accumBlueBits + m->accumAlphaBits;

    w->dp.updateClipRegion = (void (__fastcall *)(__WGLdrawable *))dibUpdateClipRegion;   /* empty */
    if (m->colorIndexMode) {
        wglDIBInitUnmanaged(&dp->frontBuffer, dp, m->indexBits);
    } else {
        wglDIBInitUnmanaged(&dp->frontBuffer, dp, rgbBits);
    }
    if (m->haveAccumBuffer)   wglMemInit(&dp->accumBuffer, dp, accumBits);
    if (m->haveDepthBuffer)   wglDIBInit(&dp->depthBuffer, dp, m->depthBits);
    if (m->haveStencilBuffer) wglDIBInit(&dp->stencilBuffer, dp, m->stencilBits);
}


/* 69011700 -- dciUpdateClipRegion: the DCI drawable's updateClipRegion, empty */
void __fastcall dciUpdateClipRegion(void) {}   /* 69011700 empty updateClipRegion (DCI) */
/* 69010f10 -- dibUpdateClipRegion: the DIB drawable's updateClipRegion, empty */
void __fastcall dibUpdateClipRegion(void) {}   /* 69010f10 empty updateClipRegion (DIB) */

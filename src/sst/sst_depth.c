/*
** sst_depth.c -- the depth buffer of the hardware path: init, make / lose current, free, the windowed resize.
*/
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "glshade.h"
#include "si_names.h"
#include "icdglob.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "og.h"
#include "icdprotos.h"
#include "gldisp.h"
#include "glsitypes.h"
typedef void (__fastcall *DepthInitProc)(__GLdepthBufferICD *fb, __GLcontext *gc);
extern void __fastcall sstDepth_FreePrivate(__WGLbuffer *buf, __WGLdrawable *dp);
extern void __fastcall sstDepth_LoseCurrent(__GLdepthBufferICD *fb);
extern GLboolean __fastcall hwDepthResize(__GLdepthBufferICD *fb, GLint edx_unused, GLint p3, GLint w, GLint h, GLuint mask);

/* 6904abd0 -- sstDepth_MakeCurrent: the hardware depth buffer's makeCurrent (fb, buf, gc): the generic makeCurrent
** (6907dae0); on first use the buffer's __WGLdepthPrivate (z-offset clear state by depth bits: <= 16 -> 16 fraction
** bits, mask 0xffff; <= 24 -> 7 bits, 0xffffff; else 0 bits, 0xffffffff; one full clear pending) with 6904acd0 to
** free it; windowed, the drawable buffer takes the surface, which becomes Glide's aux (depth) surface when it is in
** video memory.  Typed (buffer / gc / private): the original's register allocation with `<= 16` */
void __fastcall sstDepth_MakeCurrent(__GLdepthBufferICD *fb, __WGLbuffer *buf, __GLcontext *gc)
{
    __WGLwinSurface *dst = fb->buf.drawableBuf;
    __WGLwinSurface *sb = (__WGLwinSurface *)buf->private_;
    __WGLdepthPrivate *dpriv = (__WGLdepthPrivate *)buf->depthPrivate;
    GLint depthBits = gc->modes.depthBits;

    __glMakeCurrentBuffer(&fb->buf, buf, gc);
    if (dpriv == NULL) {
        dpriv = (__WGLdepthPrivate *)(*gc->drawablePrivate->malloc)(sizeof(__WGLdepthPrivate));
        buf->depthPrivate = dpriv;
        buf->freePrivate = sstDepth_FreePrivate;
        if (depthBits <= 0x10) {
            dpriv->cnt = 0;
            dpriv->numFracBits = 16;
            dpriv->zDepthMask = 0xffff;
            dpriv->zClearInc = 0;
        } else if (depthBits <= 24) {
            dpriv->cnt = 0;
            dpriv->numFracBits = 7;
            dpriv->zDepthMask = 0xffffff;
            dpriv->zClearInc = 0;
        } else {
            dpriv->zDepthMask = 0xffffffff;
            dpriv->cnt = 0;
            dpriv->numFracBits = 0;
            dpriv->zClearInc = 0;
        }
        dpriv->fullZClear = 1;
        dpriv->invertZRange = 0;
    }
    if (gc->hwcx.windowed != 0) {
        LPDIRECTDRAWSURFACE surf = sb->surf;
        dst->surf = surf;
        if (sb->inVideo != 0) {
            grSurfaceSetAuxSurfaceExt(surf);
        }
    }
    __GL_DELAY_VALIDATE_MASK(gc, 0x180);
}

/* 6904acd0 -- sstDepth_FreePrivate: frees the depth buffer's __WGLdepthPrivate (installed in
** __WGLbuffer.freePrivate by 6904abd0) */
void __fastcall sstDepth_FreePrivate(__WGLbuffer *buf, __WGLdrawable *dp)
{
    if (buf->depthPrivate != NULL) {
        (*dp->free)(buf->depthPrivate);
    }
    buf->depthPrivate = NULL;
}

/* 6904acf0 -- sstDepth_LoseCurrent: depth buffer loseCurrent: __glLoseCurrentBuffer (clears buf.gc / buf.wglBuf),
** then on a Glide surface detach the aux (depth) surface and forget the surface handle (buf.drawableBuf->surf) */
void __fastcall sstDepth_LoseCurrent(__GLdepthBufferICD *fb)
{
    __GLcontext *gc = fb->buf.gc;
    __WGLwinSurface *drawable = fb->buf.drawableBuf;

    __glLoseCurrentBuffer(&fb->buf);
    if (gc->hwcx.windowed != 0) {
        grSurfaceSetAuxSurfaceExt(0);
        drawable->surf = NULL;
    }
}

/* ---- the windowed (SURFACE extension) depth buffer: __WGLwinSurface (wgldraw.h) ---- */
/* ---- the windowed (SURFACE extension) depth buffer ---- */

/*
** 6904ad40 -- hwDepthResize: depth buffer resize (windowed surfaces): marks the depth clear state dirty; in a window
** takes over the drawable's surface handle and rectangle and re-selects the surface.  Returns 0 for an
** empty size, 1 when done, 0xf0 when the surface is not in video memory (no Glide aux buffer) or software rendering was asked for.
*/
GLboolean __fastcall hwDepthResize(__GLdepthBufferICD *fb, GLint edx_unused, GLint p3, GLint w, GLint h,
                                  GLuint mask)
{
    __GLcontext *gc = fb->buf.gc;
    __WGLwinSurface *cur = fb->buf.drawableBuf;
    __WGLwinSurface *src = (__WGLwinSurface *)fb->buf.wglBuf->private_;

    if (w == 0 && h == 0) return GL_FALSE;
    (*fb->buf.gc->drawablePrivate->lockDP)(fb->buf.gc->drawablePrivate, fb->buf.gc);
    ((__WGLdepthPrivate *)fb->buf.wglBuf->depthPrivate)->fullZClear = 1;
    (*fb->buf.gc->drawablePrivate->unlockDP)(fb->buf.gc->drawablePrivate);
    if (!gc->hwcx.windowed) return GL_TRUE;
    if (fb->buf.gc) {
        if (cur->surf != src->surf && src->surf) {
            (*fb->buf.gc->drawablePrivate->lockDP)(fb->buf.gc->drawablePrivate, fb->buf.gc);
            cur->surf = src->surf;
            (*fb->buf.gc->drawablePrivate->unlockDP)(fb->buf.gc->drawablePrivate);
        }
        cur->rect = src->rect;
        if (cur->surf && src->inVideo && h && w && gc->hwcx.glideContext) {
            grSurfaceSetAuxSurfaceExt(cur->surf);
        }
    }
    if (src->inVideo && !gc->hwcx.swRequested) return GL_TRUE;
    return 0xf0;
}

/* 6904ab70 -- __glSSTInitDepth: the hardware depth buffer (fb, gc): the SI depth procs by depth bits
** (__glInitDepth16 / 24 / 32), a 0x14-byte drawable block, the hardware makeCurrent / loseCurrent / resize / free */
void __fastcall __glSSTInitDepth(__GLdepthBufferICD *fb, __GLcontext *gc)
{
    if (gc->modes.depthBits <= 16) {
        ((DepthInitProc)__glInitDepth16)(fb, gc);
    }
    else if (gc->modes.depthBits <= 24) {
        ((DepthInitProc)__glInitDepth24)(fb, gc);
    }
    else {
        ((DepthInitProc)__glInitDepth32)(fb, gc);
    }
    fb->buf.drawableBuf = (__WGLwinSurface *)__GL_CALLOC(gc, 0x14, 1);     /* the surf / rect head */
    fb->buf.makeCurrent = (__GLproc)sstDepth_MakeCurrent;
    fb->buf.loseCurrent = (__GLproc)sstDepth_LoseCurrent;
    fb->buf.resize = (__GLproc)hwDepthResize;
    fb->buf.freeBuf = (__GLproc)sstDepth_FreeBuf;
}

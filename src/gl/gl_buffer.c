/*
** gl_buffer.c -- s_buffer.c / s_clear.c: buffer set-up, resize and glClear.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include "glsitypes.h"
#include "glshade.h"
#include "icdglob.h"
#include <string.h>

#define DPFN(dp, off)   (*(__GLproc *)((GLubyte *)(dp) + (off)))
#define BUFOPS(cfb)     (*(GLubyte **)((GLubyte *)(cfb) + 8))
/* ---- 3dfx S_BUFFER.C: the render buffer lock procs (gc->buffers.lock/unlockRenderBuffers) --------
** The ICD locks the drawable around them; a buffer's lock / unlock procs are those of the
** __WGLbuffer behind buf.other. */
#define __GL_WGLBUF(b)      (((__GLbufferICD *)(b))->wglBuf)
/* ---- s_buffer.c: __glResizeBuffers (the ICD takes the drawable size as arguments) ------------
** A failing buffer resize returns GL_FALSE at once; the clip box and scissor are recomputed only
** when the size changed. */
typedef GLboolean (__fastcall *__GLresizeProc)(void *buf, GLint x, GLint y, GLint width,
                                             GLint height, GLuint mask);
typedef GLboolean (__fastcall *__GLgetDrawableSize)(__GLcontext *gc, GLint *x, GLint *y,
						     GLint *width, GLint *height);
typedef GLboolean (__fastcall *__GLresize)(__GLcontext *gc, GLint x, GLint y,
					    GLint width, GLint height);
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/* 6907db00 -- __glLockBuffer (3dfx S_BUFFER.C, the ICD's form with a buffer mask): lock the drawable's buffers
** for the given mask */
void __fastcall __glLockBuffer(__GLcontext *gc, GLuint mask)
{
    GLubyte *dp = (GLubyte *)gc->drawablePrivate;
    GLubyte *ops;

    (*DPFN(dp, 0x1fc))(dp, gc);
    if (!(*gc->buffers.obtainSLock)(gc)) {
        if (((mask & 0x20) || ((mask & 0x40) && gc->modes.depthBits == 24)) && DPFN(dp, 0x148)) {
            (*DPFN(dp, 0x148))(dp + 0x118, dp);
        }
        if (mask & 1) {
            ops = BUFOPS(gc->front);
            if (DPFN(ops, 0x30)) (*DPFN(ops, 0x30))(ops, dp);
        }
        if (mask & 4) {
            ops = BUFOPS(gc->back);
            if (DPFN(ops, 0x30)) (*DPFN(ops, 0x30))(ops, dp);
        }
        if ((mask & 0x40) && DPFN(dp, 0x19c)) {
            (*DPFN(dp, 0x19c))(dp + 0x16c, dp);
        }
        if ((mask & 0x10) && DPFN(dp, 0xf4)) {
            (*DPFN(dp, 0xf4))(dp + 0xc4, dp);
        }
        if (gc->buffers.lockDevice) {
            (*gc->buffers.lockDevice)(gc);
        }
    }
}

/* 6907dbd0 -- __glUnlockBuffer (3dfx S_BUFFER.C, the ICD's form with a buffer mask): unlock what 6907db00 locked */
void __fastcall __glUnlockBuffer(__GLcontext *gc, GLuint mask)
{
    GLubyte *dp = (GLubyte *)gc->drawablePrivate;
    GLubyte *ops;

    if (gc->buffers.sLockCnt == 1 && gc->buffers.unlockDevice) {
        (*gc->buffers.unlockDevice)(gc);
    }
    if (!(GLboolean)(*gc->buffers.releaseSLock)(gc)) {
        if (mask & 1) {
            ops = BUFOPS(gc->front);
            if (DPFN(ops, 0x34)) (*DPFN(ops, 0x34))(ops, dp);
        }
        if (mask & 4) {
            ops = BUFOPS(gc->back);
            if (DPFN(ops, 0x34)) (*DPFN(ops, 0x34))(ops, dp);
        }
        if (((mask & 0x20) || ((mask & 0x40) && gc->modes.depthBits == 24)) && DPFN(dp, 0x14c)) {
            (*DPFN(dp, 0x14c))(dp + 0x118, dp);
        }
        if ((mask & 0x40) && DPFN(dp, 0x1a0)) {
            (*DPFN(dp, 0x1a0))(dp + 0x16c, dp);
        }
        if ((mask & 0x10) && DPFN(dp, 0xf8)) {
            (*DPFN(dp, 0xf8))(dp + 0xc4, dp);
        }
    }
    (*DPFN(dp, 0x200))(dp);
}

/* 6907dcc0 -- __glLockColorBuffer (3dfx S_BUFFER.C): lock the drawable; the first share lock
** holder locks the color buffer and the device */
void __fastcall __glLockColorBuffer(__GLcontext *gc)
{
    __WGLbuffer *buf = __GL_WGLBUF(gc->buffers.bufferToLock);
    __WGLdrawable *dp = (__WGLdrawable *)gc->drawablePrivate;

    (*dp->lockDP)(dp, gc);
    if (!(*gc->buffers.obtainSLock)(gc)) {
	(*buf->lock)(buf, dp);
	if (gc->buffers.lockDevice) {
	    (*gc->buffers.lockDevice)(gc);
	}
    }
}

/* 6907dd10 -- __glUnlockColorBuffer (3dfx S_BUFFER.C): the same without the depth buffer */
void __fastcall __glUnlockColorBuffer(__GLcontext *gc)
{
    __WGLbuffer *buf = __GL_WGLBUF(gc->buffers.bufferToLock);
    __WGLdrawable *dp = (__WGLdrawable *)gc->drawablePrivate;

    if (gc->buffers.sLockCnt == 1 && gc->buffers.unlockDevice) {
	(*gc->buffers.unlockDevice)(gc);
    }
    if (!(GLboolean)(*gc->buffers.releaseSLock)(gc)) {
	(*buf->unlock)(buf, dp);
    }
    (*dp->unlockDP)(dp);
}

/* 6907ddb0 -- __glUnlockColorBufferDepthBuffer (3dfx S_BUFFER.C): the last share lock releases the
** device, the color and the depth buffer; the drawable is always unlocked */
void __fastcall __glUnlockColorBufferDepthBuffer(__GLcontext *gc)
{
    __WGLdrawable *dp = (__WGLdrawable *)gc->drawablePrivate;

    if (gc->buffers.sLockCnt == 1 && gc->buffers.unlockDevice) {
	(*gc->buffers.unlockDevice)(gc);
    }
    if (!(GLboolean)(*gc->buffers.releaseSLock)(gc)) {
	/* unlock color */
	(*__GL_WGLBUF(gc->buffers.bufferToLock)->unlock)(__GL_WGLBUF(gc->buffers.bufferToLock), dp);
	/* unlock depth */
	(*__GL_WGLBUF(&gc->depthBuffer.buf)->unlock)(__GL_WGLBUF(&gc->depthBuffer.buf), dp);
    }
    (*dp->unlockDP)(dp);
}

/* 6907de00 -- __glTestOwnership (S_BUFFER.C): is the window pixel (x, y) inside the drawable's ownership region */
GLboolean __fastcall __glTestOwnership(__GLcontext *gc, GLint x, GLint y)
{
    __GLdrawableRegion *rgn = (__GLdrawableRegion *)&gc->drawablePrivate->numRects;
    __WGLregionRect *r;
    GLint i, n;

    x -= gc->constants.viewportXAdjust;
    y -= gc->constants.viewportYAdjust;
    n = rgn->numRects;
    for (i = 0; i < n; i++) {
	r = &rgn->rects[i];
	if (x >= r->x0 && x < r->x1 && y >= r->y0 && y < r->y1) {
	    return GL_TRUE;
	}
    }
    return GL_FALSE;
}

/* 6907ded0 -- __glResizeBuffers */
GLboolean __fastcall __glResizeBuffers(__GLcontext *gc, GLint x, GLint y, GLint width, GLint height)
{
    __WGLdrawable *dp = gc->drawablePrivate;

    (*dp->lockDP)(dp, gc);
    gc->buffers.accelBufferMask = dp->accelBufferMask;
    (*dp->unlockDP)(dp);

    if (gc->frontBuffer.buf.resize) {
        if (!(*(__GLresizeProc)gc->frontBuffer.buf.resize)(&gc->frontBuffer.buf,
                x, y, width, height, __GL_FRONT_BUFFER_MASK))
            return GL_FALSE;
    }
    if (gc->modes.doubleBufferMode && gc->backBuffer.buf.resize) {
        if (!(*(__GLresizeProc)gc->backBuffer.buf.resize)(&gc->backBuffer.buf,
                x, y, width, height, __GL_BACK_BUFFER_MASK))
            return GL_FALSE;
    }
    if (gc->modes.haveAccumBuffer && gc->accumBuffer.buf.resize) {
        if (!(*(__GLresizeProc)gc->accumBuffer.buf.resize)(&gc->accumBuffer.buf,
                x, y, width, height, __GL_ACCUM_BUFFER_MASK))
            return GL_FALSE;
    }
    if (gc->modes.haveDepthBuffer && gc->depthBuffer.buf.resize) {
        if (!(*(__GLresizeProc)gc->depthBuffer.buf.resize)(&gc->depthBuffer.buf,
                x, y, width, height, __GL_DEPTH_BUFFER_MASK))
            return GL_FALSE;
    }
    if (gc->modes.haveStencilBuffer && gc->stencilBuffer.buf.resize) {
        if (!(*(__GLresizeProc)gc->stencilBuffer.buf.resize)(&gc->stencilBuffer.buf,
                x, y, width, height, __GL_STENCIL_BUFFER_MASK))
            return GL_FALSE;
    }

    if ((width != gc->constants.width) ||
        (height != gc->constants.height))
    {
        if (gc->constants.yInverted && height != gc->constants.height) {
            gc->state.current.rasterPos.window.y += (GLuint)(height - gc->constants.height);
        }

        gc->constants.width = width;
        gc->constants.height = height;
        (*gc->procs.computeClipBox)(gc);
        (*gc->procs.applyScissor)(gc);
    }
    return GL_TRUE;
}

/* 6907e079 -- __glFindWindowSize (S_BUFFER.C): re-read the drawable size through the imports and hand it to
** the exports' resize
** (the whole desktop when full-screen and not under the HAL simulator) */
GLboolean __fastcall __glFindWindowSize(__GLcontext *gc)
{
    GLint x, y, width, height;
    RECT rect;
    GLboolean ok;

    (*gc->drawablePrivate->lockDP)(gc->drawablePrivate, gc);
    ok = (*(__GLgetDrawableSize)gc->imports[7])(gc, &x, &y, &width, &height);
    if (getenv("HAL_CSIM") == NULL && !gc->hwcx.windowed) {
	GetWindowRect(GetDesktopWindow(), &rect);
	x = 0;
	y = 0;
	width = rect.right;
	height = rect.bottom;
    }
    if (ok) {
	ok = (*(__GLresize)gc->exports[__GL_EXP_NOTIFY_RESIZE])(gc, x, y, width, height);
    }
    (*gc->drawablePrivate->unlockDP)(gc->drawablePrivate);
    return ok;
}

/* 690cdad0 -- s_clear.c __glim_Clear (the ICD waits for the hardware before clearing) */
void __stdcall __glim_Clear(GLbitfield mask)
{
    __GL_SETUP();
    GLuint beginMode;

    beginMode = __gl_beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    __gl_beginMode = __GL_NOT_IN_BEGIN;
	    glClear(mask);
	    return;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
    }

    (*gc->procs.waitIdle)(gc);

    if (mask & ~(GL_COLOR_BUFFER_BIT | GL_ACCUM_BUFFER_BIT
		 | GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    if (gc->renderMode == GL_RENDER) {
	if (mask & GL_COLOR_BUFFER_BIT) {
	    switch (gc->state.raster.drawBuffer) {
	      case GL_NONE:
		break;
	      case GL_FRONT:
		(*gc->front->clear)(gc->front);
		break;
	      case GL_BACK:
		if (gc->modes.doubleBufferMode) {
		    (*gc->back->clear)(gc->back);
		}
		break;
	      case GL_FRONT_AND_BACK:
		(*gc->front->clear)(gc->front);
		if (gc->modes.doubleBufferMode) {
		    (*gc->back->clear)(gc->back);
		}
		break;
	    }
	}
	if ((mask & GL_ACCUM_BUFFER_BIT) && gc->modes.haveAccumBuffer) {
	    (*gc->accumBuffer.clear)(&gc->accumBuffer);
	}
	if ((mask & GL_STENCIL_BUFFER_BIT) && gc->modes.haveStencilBuffer) {
	    (*gc->stencilBuffer.clear)(&gc->stencilBuffer);
	}
	if ((mask & GL_DEPTH_BUFFER_BIT) && gc->modes.haveDepthBuffer) {
	    (*gc->depthBuffer.clear)(&gc->depthBuffer);
	}
    }
}

/* 6907e360 -- s_buffer.c __glim_AddSwapHintRectWIN (the ICD's addSwapRect returns nothing) */
void __stdcall __glim_AddSwapHintRectWIN(GLint x, GLint y,
			     GLsizei width, GLsizei height)
{
    __WGLdrawable *dp;
    GLint dpWidth, dpHeight;
    GLint x0, x1, y0, y1;
    __GL_SETUP_NOT_IN_BEGIN();

    dp = gc->drawablePrivate;

    dpWidth = dp->width;
    dpHeight = dp->height;

    x0 = x;
    y0 = y;
    x1 = x + width;
    y1 = y + height;

    x0 = MAX(x0, 0); x0 = MIN(x0, dpWidth);
    x1 = MAX(x1, 0); x1 = MIN(x1, dpWidth);
    y0 = MAX(y0, 0); y0 = MIN(y0, dpHeight);
    y1 = MAX(y1, 0); y1 = MIN(y1, dpHeight);

    /* addswaphintrect only makes sense for dbuf drawables. */
    if (gc->modes.doubleBufferMode) {
	(*gc->drawablePrivate->addSwapRect)(dp, x0, y0, x1-x0, y1-y0);
    }
}

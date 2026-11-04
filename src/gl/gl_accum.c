/*
** gl_accum.c -- s_accum.c: the accumulation buffer (its procs, __glInitAccum64, glAccum).
*/
#include <string.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"                      /* __GLaccumCell */
#include "glshade.h"
#include "si_names.h"
#include "icdprotos.h"
#include <math.h>
#define LOCK_BUFFERS(gc, m)   (((void (__fastcall *)(__GLcontext *, GLuint))(gc)->buffers.lockBuffers)(gc, m))
#define UNLOCK_BUFFERS(gc, m) (((void (__fastcall *)(__GLcontext *, GLuint))(gc)->buffers.unlockBuffers)(gc, m))

/************************************************************************/
/* s_accum.c on the WGL accumulation buffer (16-bit components) */

#define __GL_ACCUM_ADDRESS(gc,dBuf,cast,x,y) \
    (cast (((GLubyte *) (dBuf)->base) \
	+ (((y) - (gc)->constants.viewportYAdjust) * (dBuf)->outerWidth \
	   - (gc)->constants.viewportXAdjust + (x)) * (dBuf)->elementSize))

/* 6909b690 -- accum_Add: glAccum(GL_ADD) (s_accum.c): add value, scaled to accumulation-cell units and rounded
** (+half), to every r/g/b/a cell inside the clip rectangle (gc->transform.clipX0..clipY1) */
void __fastcall accum_Add(__GLaccumBufferICD *afb, GLuint unused, __GLfloat value)
{
    __GLcontext *gc = afb->buf.gc;
    __WGLbuffer *dBuf;
    GLint x0, y0, x1, y1;
    GLint w, w4, w1, skip;
    __GLaccumCellElement rval, gval, bval, aval;
    __GLaccumCell *ac;

    x0 = gc->transform.clipX0;
    y0 = gc->transform.clipY0;
    x1 = gc->transform.clipX1;
    y1 = gc->transform.clipY1;

    rval = (__GLaccumCellElement)
	(value * gc->frontBuffer.redScale * afb->redScale + gc->constants.half);
    gval = (__GLaccumCellElement)
	(value * gc->frontBuffer.greenScale * afb->greenScale + gc->constants.half);
    bval = (__GLaccumCellElement)
	(value * gc->frontBuffer.blueScale * afb->blueScale + gc->constants.half);
    aval = (__GLaccumCellElement)
	(value * gc->frontBuffer.alphaScale * afb->alphaScale + gc->constants.half);

    LOCK_BUFFERS(gc, __GL_ACCUM_BUFFER_MASK);

    dBuf = afb->buf.wglBuf;

    ac = __GL_ACCUM_ADDRESS(gc,dBuf,(__GLaccumCell*),x0,y0);
    w = x1 - x0;
    w4 = w >> 2;
    w1 = w & 3;
    skip = dBuf->outerWidth - w;
    for (; y0 < y1; y0++) {
	w = w4;
	while (--w >= 0) {
	    ac[0].r += rval; ac[0].g += gval; ac[0].b += bval; ac[0].a += aval;
	    ac[1].r += rval; ac[1].g += gval; ac[1].b += bval; ac[1].a += aval;
	    ac[2].r += rval; ac[2].g += gval; ac[2].b += bval; ac[2].a += aval;
	    ac[3].r += rval; ac[3].g += gval; ac[3].b += bval; ac[3].a += aval;
	    ac += 4;
	}
	w = w1;
	while (--w >= 0) {
	    ac[0].r += rval; ac[0].g += gval; ac[0].b += bval; ac[0].a += aval;
	    ac++;
	}
	ac += skip;
    }

    UNLOCK_BUFFERS(gc, __GL_ACCUM_BUFFER_MASK);
}

/* 6909b990 -- accum_Clear: clear the accumulation buffer inside the clip rectangle to gc->state.accum.clear,
** converted to cell units via the front buffer's and the accum buffer's component scales (truncated, no rounding)
** (s_accum.c) */
void __fastcall accum_Clear(__GLaccumBufferICD *afb)
{
    __GLcontext *gc = afb->buf.gc;
    __WGLbuffer *dBuf;
    GLint x0, y0, y1;
    GLint w, w4, w1, skip;
    __GLaccumCell *ac;
    __GLaccumCellElement r, g, b, a;
    __GLcolorBufferICD *cfb = &gc->frontBuffer;
    __GLcolor *val = &gc->state.accum.clear;

    x0 = gc->transform.clipX0;
    y0 = gc->transform.clipY0;
    y1 = gc->transform.clipY1;

    /*
    ** Convert abstract color into specific color value.
    */
    r = (__GLaccumCellElement) (val->r * cfb->redScale * afb->redScale);
    g = (__GLaccumCellElement) (val->g * cfb->greenScale * afb->greenScale);
    b = (__GLaccumCellElement) (val->b * cfb->blueScale * afb->blueScale);
    a = (__GLaccumCellElement) (val->a * cfb->alphaScale * afb->alphaScale);

    LOCK_BUFFERS(gc, __GL_ACCUM_BUFFER_MASK);

    dBuf = afb->buf.wglBuf;

    ac = __GL_ACCUM_ADDRESS(gc,dBuf,(__GLaccumCell*),x0,y0);
    w = gc->transform.clipX1 - x0;
    w4 = w >> 2;
    w1 = w & 3;
    skip = dBuf->outerWidth - w;
    for (; y0 < y1; y0++) {
	w = w4;
	while (--w >= 0) {
	    ac[0].r = r; ac[0].g = g; ac[0].b = b; ac[0].a = a;
	    ac[1].r = r; ac[1].g = g; ac[1].b = b; ac[1].a = a;
	    ac[2].r = r; ac[2].g = g; ac[2].b = b; ac[2].a = a;
	    ac[3].r = r; ac[3].g = g; ac[3].b = b; ac[3].a = a;
	    ac += 4;
	}
	w = w1;
	while (--w >= 0) {
	    ac[0].r = r; ac[0].g = g; ac[0].b = b; ac[0].a = a;
	    ac++;
	}
	ac += skip;
    }

    UNLOCK_BUFFERS(gc, __GL_ACCUM_BUFFER_MASK);
}

/* 6909ade0 -- accum_Load: glAccum(GL_LOAD) (s_accum.c): for each row of the clip rectangle read a span from
** gc->readBuffer (readSpan into a 4096-entry stack buffer) and store color * val * accum scale into the cells */
void __fastcall accum_Load(__GLaccumBufferICD *afb, GLuint unused, __GLfloat val)
{
    __GLcontext *gc = afb->buf.gc;
    __WGLbuffer *dBuf;
    GLint x0, y0, x1, y1;
    GLint w, w4, w1, ow, skip;
    __GLfloat rval, gval, bval, aval;
    __GLaccumCell *ac;
    __GLcolorBufferICD *cfb;
    __GLcolor cbuf[4096];/*XXX*/

    x0 = gc->transform.clipX0;
    y0 = gc->transform.clipY0;
    x1 = gc->transform.clipX1;
    y1 = gc->transform.clipY1;
    w = x1 - x0;

    LOCK_BUFFERS(gc, gc->buffers.readBufferMask | __GL_ACCUM_BUFFER_MASK);

    dBuf = afb->buf.wglBuf;

    ac = __GL_ACCUM_ADDRESS(gc,dBuf,(__GLaccumCell*),x0,y0);
    cfb = gc->readBuffer;
    ow = w;
    w4 = w >> 2;
    w1 = w & 3;
    skip = dBuf->outerWidth - w;

    rval = val * afb->redScale;
    gval = val * afb->greenScale;
    bval = val * afb->blueScale;
    aval = val * afb->alphaScale;

    for (; y0 < y1; y0++) {
	__GLcolor *cp = &cbuf[0];
	((void (__fastcall *)(__GLcolorBufferICD *, GLint, GLint, __GLcolor *, GLint))cfb->readSpan)(cfb, x0, y0, &cbuf[0], ow);

	w = w4;
	while (--w >= 0) {
	    ac[0].r = (__GLaccumCellElement) (cp[0].r * rval);
	    ac[0].g = (__GLaccumCellElement) (cp[0].g * gval);
	    ac[0].b = (__GLaccumCellElement) (cp[0].b * bval);
	    ac[0].a = (__GLaccumCellElement) (cp[0].a * aval);

	    ac[1].r = (__GLaccumCellElement) (cp[1].r * rval);
	    ac[1].g = (__GLaccumCellElement) (cp[1].g * gval);
	    ac[1].b = (__GLaccumCellElement) (cp[1].b * bval);
	    ac[1].a = (__GLaccumCellElement) (cp[1].a * aval);

	    ac[2].r = (__GLaccumCellElement) (cp[2].r * rval);
	    ac[2].g = (__GLaccumCellElement) (cp[2].g * gval);
	    ac[2].b = (__GLaccumCellElement) (cp[2].b * bval);
	    ac[2].a = (__GLaccumCellElement) (cp[2].a * aval);

	    ac[3].r = (__GLaccumCellElement) (cp[3].r * rval);
	    ac[3].g = (__GLaccumCellElement) (cp[3].g * gval);
	    ac[3].b = (__GLaccumCellElement) (cp[3].b * bval);
	    ac[3].a = (__GLaccumCellElement) (cp[3].a * aval);
	    ac += 4;
	    cp += 4;
	}

	w = w1;
	while (--w >= 0) {
	    ac->r = (__GLaccumCellElement) (cp->r * rval);
	    ac->g = (__GLaccumCellElement) (cp->g * gval);
	    ac->b = (__GLaccumCellElement) (cp->b * bval);
	    ac->a = (__GLaccumCellElement) (cp->a * aval);
	    ac++;
	    cp++;
	}
	ac += skip;
    }

    UNLOCK_BUFFERS(gc, gc->buffers.readBufferMask | __GL_ACCUM_BUFFER_MASK);
}

/* 6909b0a0 -- accum_Accumulate: glAccum(GL_ACCUM) (s_accum.c): like accum_Load, but adds the read-buffer
** color * val * accum scale to the existing cells instead of replacing them */
void __fastcall accum_Accumulate(__GLaccumBufferICD *afb, GLuint unused, __GLfloat val)
{
    __GLcontext *gc = afb->buf.gc;
    __WGLbuffer *dBuf;
    GLint x0, y0, x1, y1;
    GLint w, ow, skip, w4, w1;
    __GLfloat rval, gval, bval, aval;
    __GLaccumCell *ac;
    __GLcolorBufferICD *cfb;
    __GLcolor cbuf[4096];/*XXX*/

    x0 = gc->transform.clipX0;
    y0 = gc->transform.clipY0;
    x1 = gc->transform.clipX1;
    y1 = gc->transform.clipY1;
    w = x1 - x0;

    LOCK_BUFFERS(gc, gc->buffers.readBufferMask | __GL_ACCUM_BUFFER_MASK);

    dBuf = afb->buf.wglBuf;

    ac = __GL_ACCUM_ADDRESS(gc,dBuf,(__GLaccumCell*),x0,y0);
    cfb = gc->readBuffer;
    ow = w;
    w4 = w >> 2;
    w1 = w & 3;
    skip = dBuf->outerWidth - w;

    rval = val * afb->redScale;
    gval = val * afb->greenScale;
    bval = val * afb->blueScale;
    aval = val * afb->alphaScale;

    for (; y0 < y1; y0++) {
	__GLcolor *cp = &cbuf[0];
	((void (__fastcall *)(__GLcolorBufferICD *, GLint, GLint, __GLcolor *, GLint))cfb->readSpan)(cfb, x0, y0, &cbuf[0], ow);

	w = w4;
	while (--w >= 0) {
	    ac[0].r += (__GLaccumCellElement) (cp[0].r * rval);
	    ac[0].g += (__GLaccumCellElement) (cp[0].g * gval);
	    ac[0].b += (__GLaccumCellElement) (cp[0].b * bval);
	    ac[0].a += (__GLaccumCellElement) (cp[0].a * aval);

	    ac[1].r += (__GLaccumCellElement) (cp[1].r * rval);
	    ac[1].g += (__GLaccumCellElement) (cp[1].g * gval);
	    ac[1].b += (__GLaccumCellElement) (cp[1].b * bval);
	    ac[1].a += (__GLaccumCellElement) (cp[1].a * aval);

	    ac[2].r += (__GLaccumCellElement) (cp[2].r * rval);
	    ac[2].g += (__GLaccumCellElement) (cp[2].g * gval);
	    ac[2].b += (__GLaccumCellElement) (cp[2].b * bval);
	    ac[2].a += (__GLaccumCellElement) (cp[2].a * aval);

	    ac[3].r += (__GLaccumCellElement) (cp[3].r * rval);
	    ac[3].g += (__GLaccumCellElement) (cp[3].g * gval);
	    ac[3].b += (__GLaccumCellElement) (cp[3].b * bval);
	    ac[3].a += (__GLaccumCellElement) (cp[3].a * aval);
	    ac += 4;
	    cp += 4;
	}

	w = w1;
	while (--w >= 0) {
	    ac->r += (__GLaccumCellElement) (cp->r * rval);
	    ac->g += (__GLaccumCellElement) (cp->g * gval);
	    ac->b += (__GLaccumCellElement) (cp->b * bval);
	    ac->a += (__GLaccumCellElement) (cp->a * aval);
	    ac++;
	    cp++;
	}
	ac += skip;
    }

    UNLOCK_BUFFERS(gc, gc->buffers.readBufferMask | __GL_ACCUM_BUFFER_MASK);
}

/* 6909b360 -- accum_Mult: glAccum(GL_MULT) (s_accum.c): multiply every cell inside the clip rectangle by val;
** val == 0 takes a separate loop that just zeroes the cells */
void __fastcall accum_Mult(__GLaccumBufferICD *afb, GLuint unused, __GLfloat val)
{
    __GLcontext *gc = afb->buf.gc;
    __WGLbuffer *dBuf;
    GLint x0, y0, x1, y1;
    GLint w, w4, w1, skip;
    __GLaccumCell *ac;

    x0 = gc->transform.clipX0;
    y0 = gc->transform.clipY0;
    x1 = gc->transform.clipX1;
    y1 = gc->transform.clipY1;

    LOCK_BUFFERS(gc, __GL_ACCUM_BUFFER_MASK);

    dBuf = afb->buf.wglBuf;

    ac = __GL_ACCUM_ADDRESS(gc,dBuf,(__GLaccumCell*),x0,y0);
    w = x1 - x0;
    skip = dBuf->outerWidth - w;

    if (val == __GL_ZERO_F) {
	/* Zero out the buffers contents */
	for (; y0 < y1; y0++) {
	    GLint ww = w;
	    while (ww > 0) {
		ac[0].r = 0; ac[0].g = 0; ac[0].b = 0; ac[0].a = 0;
		ac++;
		ww--;
	    }
	    ac += skip;
	}
	UNLOCK_BUFFERS(gc, __GL_ACCUM_BUFFER_MASK);
	return;
    }

    w4 = w >> 2;
    w1 = w & 3;
    for (; y0 < y1; y0++) {
	w = w4;
	while (--w >= 0) {
	    ac[0].r = (__GLaccumCellElement) (ac[0].r * val);
	    ac[0].g = (__GLaccumCellElement) (ac[0].g * val);
	    ac[0].b = (__GLaccumCellElement) (ac[0].b * val);
	    ac[0].a = (__GLaccumCellElement) (ac[0].a * val);
	    ac[1].r = (__GLaccumCellElement) (ac[1].r * val);
	    ac[1].g = (__GLaccumCellElement) (ac[1].g * val);
	    ac[1].b = (__GLaccumCellElement) (ac[1].b * val);
	    ac[1].a = (__GLaccumCellElement) (ac[1].a * val);
	    ac[2].r = (__GLaccumCellElement) (ac[2].r * val);
	    ac[2].g = (__GLaccumCellElement) (ac[2].g * val);
	    ac[2].b = (__GLaccumCellElement) (ac[2].b * val);
	    ac[2].a = (__GLaccumCellElement) (ac[2].a * val);
	    ac[3].r = (__GLaccumCellElement) (ac[3].r * val);
	    ac[3].g = (__GLaccumCellElement) (ac[3].g * val);
	    ac[3].b = (__GLaccumCellElement) (ac[3].b * val);
	    ac[3].a = (__GLaccumCellElement) (ac[3].a * val);
	    ac += 4;
	}
	w = w1;
	while (--w >= 0) {
	    ac[0].r = (__GLaccumCellElement) (ac[0].r * val);
	    ac[0].g = (__GLaccumCellElement) (ac[0].g * val);
	    ac[0].b = (__GLaccumCellElement) (ac[0].b * val);
	    ac[0].a = (__GLaccumCellElement) (ac[0].a * val);
	    ac++;
	}
	ac += skip;
    }

    UNLOCK_BUFFERS(gc, __GL_ACCUM_BUFFER_MASK);
}
#ifndef GL_ADD
#define GL_ADD                          0x0104
#endif
typedef void (__fastcall *__GLreturnSpan)(__GLcolorBufferICD *cfb, GLint x, GLint y,
                                              const __GLaccumCell *ac, __GLfloat scale, GLint w);
/* ---- s_accum.c ---------------------------------------------------------------------------- */
typedef void (__fastcall *__GLaccumOp)(__GLaccumBufferICD *afb, __GLfloat value);
extern void __fastcall accum_Load(__GLaccumBufferICD *afb, GLuint unused, __GLfloat val);
extern void __fastcall accum_Accumulate(__GLaccumBufferICD *afb, GLuint unused, __GLfloat val);
extern void __fastcall accum_Mult(__GLaccumBufferICD *afb, GLuint unused, __GLfloat val);
extern void __fastcall accum_Add(__GLaccumBufferICD *afb, GLuint unused, __GLfloat value);
extern void __fastcall accum_Return(__GLaccumBufferICD *afb, GLuint unused, __GLfloat val);
extern void __fastcall accum_Clear(__GLaccumBufferICD *afb);

/* 6909ad20 -- __glInitAccum64 */
void __fastcall __glInitAccum64(__GLaccumBufferICD *afb, __GLcontext *gc)
{
    __glInitBuffer(&afb->buf);

    afb->pick = (__GLproc)accum_Pick;
    afb->clear = (__GLproc)accum_Clear;
    afb->accumulate = (__GLproc)accum_Accumulate;
    afb->load = (__GLproc)accum_Load;
    afb->ret = (__GLproc)accum_Return;
    afb->mult = (__GLproc)accum_Mult;
    afb->add = (__GLproc)accum_Add;

    if (gc->modes.rgbMode) {
    __GLcolorBufferICD *cfb;
    __GLfloat scale;

    scale = (1 << (8 * sizeof(GLshort)))/2 - 1;

    cfb = &gc->frontBuffer;
    afb->redScale = scale / (cfb->redScale);
    afb->blueScale = scale / (cfb->blueScale);
    afb->greenScale = scale / (cfb->greenScale);
    afb->alphaScale = scale / (cfb->alphaScale);

    afb->oneOverRedScale = 1 / afb->redScale;
    afb->oneOverGreenScale = 1 / afb->greenScale;
    afb->oneOverBlueScale = 1 / afb->blueScale;
    afb->oneOverAlphaScale = 1 / afb->alphaScale;
    }
}

/* 6909b860 -- accum_Return: glAccum(GL_RETURN) (s_accum.c): write each row of the clip rectangle back through
** the color buffer's returnSpan with scale val -- to front and back buffer when gc->buffers.doubleStore, else to
** gc->drawBuffer */
void __fastcall accum_Return(__GLaccumBufferICD *afb, GLuint unused, __GLfloat val)
{
    __GLcontext *gc = afb->buf.gc;
    __WGLbuffer *dBuf;
    GLint x0, y0, x1, y1;
    GLint w, next;
    __GLaccumCell *ac;
    __GLcolorBufferICD *cfb;
    __GLcolorBufferICD *cfb2;

    x0 = gc->transform.clipX0;
    y0 = gc->transform.clipY0;
    x1 = gc->transform.clipX1;
    y1 = gc->transform.clipY1;
    LOCK_BUFFERS(gc, gc->buffers.drawBufferMask | __GL_ACCUM_BUFFER_MASK);

    dBuf = afb->buf.wglBuf;

    ac = __GL_ACCUM_ADDRESS(gc,dBuf,(__GLaccumCell*),x0,y0);
    w = x1 - x0;
    next = dBuf->outerWidth;

    if (gc->buffers.doubleStore) {
        /* Store to both buffers */
        cfb = &gc->frontBuffer;
        cfb2 = &gc->backBuffer;
        for (; y0 < y1; y0++) {
            (*(__GLreturnSpan)cfb->returnSpan)(cfb, x0, y0, ac, val, w);
            (*(__GLreturnSpan)cfb2->returnSpan)(cfb2, x0, y0, ac, val, w);
            ac += next;
        }
    } else {
        cfb = gc->drawBuffer;
        for (; y0 < y1; y0++) {
            (*(__GLreturnSpan)cfb->returnSpan)(cfb, x0, y0, ac, val, w);
            ac += next;
        }
    }

    UNLOCK_BUFFERS(gc, gc->buffers.drawBufferMask | __GL_ACCUM_BUFFER_MASK);
}

/* 6909bb60 -- s_accum.c __glim_Accum (revalidate + waitIdle before the op) */
void __stdcall __glim_Accum(GLenum op, GLfloat value)
{
    __GLaccumOp accumOp;
    __GLaccumBufferICD *fb;
    __GL_SETUP();
    GLuint beginMode;

    beginMode = __gl_beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    __gl_beginMode = __GL_NOT_IN_BEGIN;
	    glAccum(op,value);
	    return;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
    }

    fb = &gc->accumBuffer;
    if (!gc->modes.haveAccumBuffer || gc->modes.colorIndexMode) {
	__glSetError(GL_INVALID_OPERATION);
	return;
    }

    switch (op) {
      case GL_ACCUM:
	accumOp = (__GLaccumOp)accum_Accumulate;
	break;
      case GL_LOAD:
	accumOp = (__GLaccumOp)accum_Load;
	break;
      case GL_RETURN:
	accumOp = (__GLaccumOp)accum_Return;
	break;
      case GL_MULT:
	accumOp = (__GLaccumOp)accum_Mult;
	break;
      case GL_ADD:
	accumOp = (__GLaccumOp)accum_Add;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    (*gc->procs.revalidate)(gc);
    (*gc->procs.waitIdle)(gc);
    if (gc->renderMode == GL_RENDER) {
	__GL_API_PIXEL_OP();
	(*accumOp)(fb, value);
    }
}

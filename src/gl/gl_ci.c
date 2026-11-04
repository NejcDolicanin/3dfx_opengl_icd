/*
** gl_ci.c -- s_ci.c: the colour-index buffer procs (SI port: si_s_ci.c).
*/
#include <string.h>
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include "icdglob.h"
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "icdprotos.h"
#include <stdlib.h>
/************************************************************************/
/* s_ci.c: the generic colour-index stores on the WGL buffer */
#define __GL_DITHER_PRECISION       16
#define __GL_DITHER_INDEX(x, y)     (((x) & 3) + (((y) & 3) << 2))
#define MODEFLAGS(gc)   ((*(GLuint (__fastcall *)(__GLcontext *))(gc)->polygon.shader.getModeFlags)(gc))
#define __GL_FB_ADDRESS(gc,fb,drawableBuf,cast,x,y) \
    (cast (((GLubyte *) (drawableBuf)->base) \
        + (((y) - (gc)->constants.viewportYAdjust) * (drawableBuf)->outerWidth \
           + (x) - (gc)->constants.viewportXAdjust) * (drawableBuf)->elementSize))
#define CI_STORE(name, TYPE)                                                            \
void __fastcall name(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)                  \
{                                                                                           \
    __GLcontext *gc = cfb->buf.gc;                                                          \
    __WGLbuffer *dBuf = cfb->buf.wglBuf;                                      \
    GLuint modeFlags = MODEFLAGS(gc);                                                   \
    TYPE *fp;                                                                               \
    __GLfloat inc;                                                                          \
    TYPE fbcolor, result;                                                                   \
    GLint ix;                                                                               \
                                                                                            \
    if ((modeFlags & __GL_SHADE_OWNERSHIP_TEST) &&                                      \
                !__glTestOwnership(gc, frag->x, frag->y)) {                             \
        return;                                                                             \
    }                                                                                       \
                                                                                            \
    fp = __GL_FB_ADDRESS(gc, 0, dBuf, (TYPE *), frag->x, frag->y);                         \
    if (modeFlags & __GL_SHADE_DITHER) {                                                \
        ix = __GL_DITHER_INDEX(frag->x, frag->y);                                       \
        inc = ((__glDitherTable[ix] << 1) + 1) /                                            \
            (__GLfloat) (2 * __GL_DITHER_PRECISION);                                    \
    } else {                                                                                \
        inc = gc->constants.half;                                                           \
    }                                                                                       \
    result = (TYPE)(frag->color.r + inc);                                                   \
    fbcolor = *fp;                                                                          \
    if (modeFlags & __GL_SHADE_LOGICOP) {                                               \
        switch(gc->state.raster.logicOp) {                                                  \
          case GL_CLEAR:         result = 0; break;                                         \
          case GL_AND:           result = result & fbcolor; break;                          \
          case GL_AND_REVERSE:   result = result & (~fbcolor); break;                       \
          case GL_COPY:          result = result; break;                                    \
          case GL_AND_INVERTED:  result = (~result) & fbcolor; break;                       \
          case GL_NOOP:          result = fbcolor; break;                                   \
          case GL_XOR:           result = result ^ fbcolor; break;                          \
          case GL_OR:            result = result | fbcolor; break;                          \
          case GL_NOR:           result = ~(result | fbcolor); break;                       \
          case GL_EQUIV:         result = ~(result ^ fbcolor); break;                       \
          case GL_INVERT:        result = ~fbcolor; break;                                  \
          case GL_OR_REVERSE:    result = result | (~fbcolor); break;                       \
          case GL_COPY_INVERTED: result = ~result; break;                                   \
          case GL_OR_INVERTED:   result = (~result) | fbcolor; break;                       \
          case GL_NAND:          result = ~(result & fbcolor); break;                       \
          case GL_SET:           result = ~0; break;                                        \
        }                                                                                   \
    }                                                                                       \
                                                                                            \
    *fp = (fbcolor & cfb->destMask) | (result & cfb->sourceMask);                           \
}
/* ---- s_ci.c ------------------------------------------------------------------------------- */
#define __GL_CI_ADDRESS(gc,dBuf,cast,x,y) \
    (cast ((GLubyte *)(dBuf)->base \
	+ (((y) - (gc)->constants.viewportYAdjust) * (dBuf)->outerWidth \
	   - (gc)->constants.viewportXAdjust + (x)) * (dBuf)->elementSize))
typedef void (__fastcall *__GLclearRectProc)(__GLcolorBufferICD *cfb, GLint x0, GLint y0, GLint x1, GLint y1);
extern void __fastcall ci_ClearRect_8_Dither(__GLcolorBufferICD *cfb, GLint x0, GLint y0, GLint x1, GLint y1);
typedef void (__fastcall *__GLstore)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
#define __GL_CFB_WGL(cfb)   (((__GLcolorBufferICD *)(cfb))->buf.wglBuf)
extern void __fastcall ci_Store_8(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
extern void __fastcall ci_Store_16(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
extern void __fastcall ci_Store_32(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
extern void __fastcall ci_Clear(__GLcolorBufferICD *cfb, GLuint unused);
extern void __fastcall ci_Pick(__GLcontext *gc, __GLcolorBufferICD *cfb);
extern void __fastcall ci_ClearRegionRects(__GLcolorBufferICD *cfb, __GLclearRectProc clearRect);
extern void __fastcall ci_ClearRect(__GLcolorBufferICD *cfb, GLint x, GLint y, GLint x1, GLint y1);
extern void __fastcall ci_ClearRect_8(__GLcolorBufferICD *cfb, GLint x0, GLint y0, GLint x1, GLint y1);

extern void __fastcall ci_Fetch_8(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result);
extern void __fastcall ci_Fetch_16(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result);
extern void __fastcall ci_Fetch_32(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result);
/* 6909bc80 -- s_ci.c __glInitCI */
void __fastcall __glInitCI(__GLcolorBufferICD *cfb, __GLcontext *gc)
{
    GLint indexBits;

    __glInitBuffer(&cfb->buf);         /* __glInitBuffer (gc stays in EDX) */

    cfb->readSpan = (__GLproc)__glReadSpan;
    cfb->returnSpan = (__GLproc)__glReturnSpan;

    cfb->clear = (__GLproc)ci_Clear;
    cfb->pick = (__GLproc)ci_Pick;
    cfb->fetchSpan = (__GLproc)__glFetchSpan;
    cfb->fetchStippledSpan = (__GLproc)__glFetchSpan;
    cfb->storeSpan = (__GLproc)ci_StoreSpan;
    cfb->storeStippledSpan = (__GLproc)ci_StoreStippledSpan;

    cfb->redScale = 1.0;
    cfb->greenScale = 1.0;
    cfb->blueScale = 1.0;
    cfb->alphaScale = 1.0;

    indexBits = gc->modes.indexBits;
    if (indexBits < 32) {
        cfb->sourceMask = (1 << indexBits) - 1;
    } else {
        cfb->sourceMask = 0xffffffff;
    }
    cfb->redMax = cfb->sourceMask;

    if (indexBits > 16) {
	cfb->store = (__GLproc)ci_Store_32;
        cfb->fetch = cfb->readColor = (__GLproc)ci_Fetch_32;
    } else if (indexBits > 8) {
	cfb->store = (__GLproc)ci_Store_16;
        cfb->fetch = cfb->readColor = (__GLproc)ci_Fetch_16;
    } else {
	cfb->store = (__GLproc)ci_Store_8;
        cfb->fetch = cfb->readColor = (__GLproc)ci_Fetch_8;
    }
}

/* 6909bd70 -- ci_Store_8: Store_8 (s_ci.c) */
CI_STORE(ci_Store_8, GLubyte)

/* 6909bed0 -- ci_Store_16: Store_16 (s_ci.c) */
CI_STORE(ci_Store_16, GLushort)

/* 6909c040 -- ci_Store_32: Store_32 (s_ci.c) */
CI_STORE(ci_Store_32, GLuint)

/* 6909c1a0 -- ci_Fetch_8: s_ci.c Fetch_8 (no ownership test) */
void __fastcall ci_Fetch_8(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *dBuf = cfb->buf.wglBuf;
    GLubyte *fp;

    fp = __GL_CI_ADDRESS(gc, dBuf, (GLubyte *), x, y);
    result->r = *fp;
}

/* 6909c1e0 -- ci_Fetch_16: s_ci.c Fetch_16 */
void __fastcall ci_Fetch_16(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *dBuf = cfb->buf.wglBuf;
    GLushort *fp;

    fp = __GL_CI_ADDRESS(gc, dBuf, (GLushort *), x, y);
    result->r = *fp;
}

/* 6909c230 -- ci_Fetch_32: s_ci.c Fetch_32 */
void __fastcall ci_Fetch_32(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *dBuf = cfb->buf.wglBuf;
    GLuint *fp;

    fp = __GL_CI_ADDRESS(gc, dBuf, (GLuint *), x, y);
    result->r = *fp;
}

/* 6909c280 -- ci_Clear: Clear (s_ci.c, CI colour buffer), drawable, buffer and device locks around the
** clear-rect proc;
** 0x400000 = __GL_SHADE_OWNERSHIP_TEST -> ClearRegionRects (6909c330) */
void __fastcall ci_Clear(__GLcolorBufferICD *cfb, GLuint unused)
{
    __GLcontext *gc = cfb->buf.gc;
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    __WGLbuffer *buf;

    (*gc->drawablePrivate->lockDP)(gc->drawablePrivate, gc);
    buf = cfb->buf.wglBuf;
    if (buf->lock) (*buf->lock)(buf, gc->drawablePrivate);
    if (gc->buffers.lockDevice) (*gc->buffers.lockDevice)(gc);

    if (modeFlags & __GL_SHADE_OWNERSHIP_TEST) {
        ci_ClearRegionRects(cfb, (__GLclearRectProc)cfb->clearRect);
    } else {
        (*(__GLclearRectProc)cfb->clearRect)(cfb,
                  gc->transform.clipX0,
                  gc->transform.clipY0,
                  gc->transform.clipX1,
                  gc->transform.clipY1);
    }

    if (gc->buffers.unlockDevice) (*gc->buffers.unlockDevice)(gc);
    buf = cfb->buf.wglBuf;
    if (buf->unlock) (*buf->unlock)(buf, gc->drawablePrivate);
    (*gc->drawablePrivate->unlockDP)(gc->drawablePrivate);
}

/* 6909c4c0 -- ci_Pick: Pick (s_ci.c, CI colour buffer), the I386 CI8 clear-rect procs when all bits are written */
void __fastcall ci_Pick(__GLcontext *gc, __GLcolorBufferICD *cfb)
{
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc, cfb);

    if (gc->state.raster.drawBuffer == GL_NONE) {
        cfb->sourceMask = 0;
        cfb->destMask = cfb->redMax;
    } else {
        cfb->sourceMask = gc->state.raster.writeMask & cfb->redMax;
        cfb->destMask = cfb->redMax & ~gc->state.raster.writeMask;
    }
    cfb->clear = (__GLproc)ci_Clear;
    cfb->clearRect = (__GLproc)ci_ClearRect;
    if ((cfb->sourceMask == (GLuint) cfb->redMax) &&
        ((cfb->buf.wglBuf)->elementSize == 1)) {
        if (modeFlags & 0x40) {
            cfb->clearRect = (__GLproc)ci_ClearRect_8_Dither;
        } else {
            cfb->clearRect = (__GLproc)ci_ClearRect_8;
        }
    }
}

/* 6909c550 -- ci_ClearRect: ClearRect (s_ci.c): the ICD turns the logic-op enable off around the stores */
void __fastcall ci_ClearRect(__GLcolorBufferICD *cfb, GLint x, GLint y, GLint x1, GLint y1)
{
    __GLcontext *gc = cfb->buf.gc;
    GLuint logicOp;
    GLint i, j;
    __GLfragmentICD frag;

    /* Turn off logic op so store proc doesn't do it.  ORIGINAL BUG: SI clears __GL_INDEX_LOGIC_OP_ENABLE (1 << 2);
    ** the ICD clears and restores bit 21, the RGBA one, so the colour-index clear still runs the index logic op */
    logicOp = gc->state.enables.general & __GL_COLOR_LOGIC_OP_ENABLE;
    gc->state.enables.general &= ~__GL_COLOR_LOGIC_OP_ENABLE;

    frag.color.r = gc->state.raster.clearIndex;

    for (j = y; j < y1; j++) {
        frag.y = j;
        for (i = x; i < x1; i++) {
            frag.x = i;

            (*(__GLstore)cfb->store)(cfb, &frag);
        }
    }

    gc->state.enables.general |= logicOp;
}

/* 6909c330 -- ci_ClearRegionRects: s_ci.c ClearRegionRects */
void __fastcall ci_ClearRegionRects(__GLcolorBufferICD *cfb, __GLclearRectProc clearRect)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLdrawable *dp = gc->drawablePrivate;
    __WGLregionRect *rect;
    int i, n;

    (*dp->lockDP)(dp, gc);

    n = dp->numRects;
    for (i=0, rect = &dp->rects[0]; i<n; ++i, ++rect) {
	(*clearRect)(cfb,
		     rect->x0 + gc->constants.viewportXAdjust,
		     rect->y0 + gc->constants.viewportYAdjust,
		     rect->x1 + gc->constants.viewportXAdjust,
		     rect->y1 + gc->constants.viewportYAdjust);
    }

    (*dp->unlockDP)(dp);
}

/* 690e3490 -- ci_ClearRect_8: s_ci.c ClearRect for an 8-bit colour-index buffer (the ICD fills the rectangle) */
void __fastcall ci_ClearRect_8(__GLcolorBufferICD *cfb, GLint x0, GLint y0, GLint x1, GLint y1)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *drawableBuf = __GL_CFB_WGL(cfb);
    GLint w, h;

    w = x1 - x0;
    h = y1 - y0;
    if (w > 0 && h > 0) {
	FastMemClear1(gc, __GL_FB_ADDRESS(gc, cfb, drawableBuf, (GLubyte *), x0, y0),
		     w, h, __GL_CFB_WGL(cfb)->outerWidth - w, (GLint)gc->state.raster.clearIndex);
    }
}

/* dither matrix (s_global.c __glDitherTable as floats) and the 1.5 * 2^23 float-to-int bias */
typedef union { __GLfloat f; GLint i; } __GLfloatBits;

/* the drawable's row pitch read through a second view of the buffer record (the original's stride
** load is not merged with the address computation's) */

/*
** 690e3500 -- ci_ClearRect_8_Dither: clearRect of an 8-bit colour-index buffer with dithering (cfb->clearRect, installed by
** 6909c4c0 for one byte per pixel with the dither mode bit; 690e3490 otherwise): a dword of four dithered clear indexes per
** row phase (y & 3), the row filled as leading bytes, rep stosd dwords and trailing bytes.
** Kept as the original: the leading count is (address & 3) and both partial runs pick the byte by
** the remaining count (1: bits 16..23, 2: bits 8..15, 3: bits 0..7, else nothing).
*/
void __fastcall ci_ClearRect_8_Dither(__GLcolorBufferICD *cfb, GLint x0, GLint y0, GLint x1, GLint y1)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *db = cfb->buf.wglBuf;
    GLuint pat[4];
    GLint i, height, width, lead, ndw, trail, n, m, stride;
    GLubyte *row, *p;
    GLuint pattern;
    __GLfloatBits f0, f1, f2, f3;

    height = y1 - y0;
    for (i = 0; i < 4; i++) {
        const __GLfloat *d = &__glDitherTableFloat[(i & 3) * 4];
        f0.f = gc->state.raster.clearIndex + d[0] + __glFloatToIntBias;
        f1.f = d[1] + gc->state.raster.clearIndex + __glFloatToIntBias;
        f2.f = d[2] + gc->state.raster.clearIndex + __glFloatToIntBias;
        f3.f = d[3] + gc->state.raster.clearIndex + __glFloatToIntBias;
        pat[i] = ((((f2.i & 0xff) | (f3.i << 8)) << 8 | (f1.i & 0xff)) << 8) | (f0.i & 0xff);
    }
    row = (GLubyte *)db->base + (((y0 - gc->constants.viewportYAdjust) * db->outerWidth
                                  - gc->constants.viewportXAdjust + x0) * db->elementSize);
    stride = cfb->buf.wglBuf->outerWidth;
    width = x1 - x0;

    lead = (GLint)row & 3;
    if (lead > width) {
        lead = width;
        ndw = 0;
        trail = 0;
    } else {
        ndw = (width - lead) >> 2;
        trail = width - ndw * 4 - lead;
    }
    for (; height; height--) {
        n = lead;
        m = trail;
        p = row;
        pattern = pat[y0 & 3];
        if (lead) {
            do {
                switch (n) {
                  case 1: *p++ = (GLubyte)(pattern >> 16); break;
                  case 2: *p++ = (GLubyte)(pattern >> 8); break;
                  case 3: *p++ = (GLubyte)pattern; break;
                }
            } while (--n);
        }
        if (ndw) {
            __asm {
                cld
                mov     ecx, ndw
                mov     edi, p
                mov     eax, pattern
                rep stosd
            }
        }
        p += ndw * 4;
        if (trail) {
            do {
                switch (m) {
                  case 1: *p++ = (GLubyte)(pattern >> 16); break;
                  case 2: *p++ = (GLubyte)(pattern >> 8); break;
                  case 3: *p++ = (GLubyte)pattern; break;
                }
            } while (--m);
        }
        row += stride;
        y0++;
    }
}

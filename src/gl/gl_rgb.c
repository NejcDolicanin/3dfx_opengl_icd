/*
** gl_rgb.c -- s_rgb.c: the RGBA colour-buffer procs (stores, fetches, spans, clears, pick) (SI
** port: si_s_rgb.c).
*/
#include <string.h>
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "glshade.h"
#include "icdglob.h"
#include "icdprotos.h"
#include <stdlib.h>
#include "glsitypes.h"
#include "og.h"
#include <math.h>
/* 3dfx declared the 16-bit fill with a GLushort value here; its definition (gl_memclr.c) takes a GLuint whose
** low half is the pixel, and these callers pass the value unmasked: kept local so both sides compile as the original */
extern void __fastcall __glFillRect16(__GLcontext *gc, GLushort *dst, GLint w, GLint h, GLint skip, GLushort value);

#define __GL_CFB_WGL(cfb)   (((__GLcolorBufferICD *)(cfb))->buf.wglBuf)
#define CFB_NEEDCOLORFRAGMENTOPS(cfb)   ((cfb)->needColorFragmentOps)
/* s_rgb.c helpers */
#define __GL_FB_ADDRESS(gc,fb,drawableBuf,cast,x,y) \
    (cast (((GLubyte *) (drawableBuf)->base) \
	+ (((y) - (gc)->constants.viewportYAdjust) \
            * (drawableBuf)->outerWidth \
	   + (x) - (gc)->constants.viewportXAdjust) \
	* (drawableBuf)->elementSize))
/* s_rgb.c generic stores, ICD form (no clamp in ColorToPixel) */
typedef __GLcolorBufferICD __GLcolorBuffer;
typedef __GLfragmentICD __GLfragment;
#define __GL_DITHER_INDEX(x, y)     (((x) & 3) + (((y) & 3) << 2))
#define __GL_DITHER_PRECISION   16
typedef void (__fastcall *__GLblendProc)(__GLcontext *gc, __GLcolorBuffer *cfb, const __GLfragment *frag, __GLcolor *result);
/* ColorToPixelAW16: inline helper for Store_32A: OR the integer r, g, b, a shifted by the colour buffer's
** shifts into a pixel, no clamp or mask (the caller passes colour + 0.5, so the conversion to GLint rounds) */
static __inline GLuint ColorToPixelAW16(__GLcontext *gc, __GLcolorBufferICD *cfb,
					GLint r, GLint g, GLint b, GLint a)
{
    GLuint pixel;

    pixel = (r << cfb->redShift) | (g << cfb->greenShift) |
	(b << cfb->blueShift) | (a << cfb->alphaShift);
    return pixel;
}
/* ColorToPixelW16: ColorToPixelAW16 without alpha, the inline helper for rgb_Store_32 */
static __inline GLuint ColorToPixelW16(__GLcontext *gc, __GLcolorBufferICD *cfb,
				       GLint r, GLint g, GLint b)
{
    GLuint pixel;

    pixel = (r << cfb->redShift) | (g << cfb->greenShift) |
	(b << cfb->blueShift);
    return pixel;
}
/* ---- s_span.c ---- */
typedef void (__fastcall *__GLfn_store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
/* ---- s_rgb.c ------------------------------------------------------------------------------- */

/* fmacros.h */
#define FLOAT_MANTISSA_BITS     23
#define FLOAT_MANTISSA_MASK     ((1 << FLOAT_MANTISSA_BITS) - 1)
#define Float2Int(_flt_, _int_) \
    (*((float *)(&(_int_)))=(_flt_)+((float)(1<<(FLOAT_MANTISSA_BITS))), \
     (_int_)&=FLOAT_MANTISSA_MASK)
typedef void (__fastcall *RGBfn_blend)(__GLcontext *gc, __GLcolorBufferICD *cfb,
                                       const __GLfragmentICD *frag, __GLcolor *result);
/* the ICD's ColorToPixel does not clamp */
#define ColorToPixel(gc, cfb, r, g, b) \
    (((GLint) (b) << (cfb)->blueShift) | ((GLint) (g) << (cfb)->greenShift) | \
     ((GLint) (r) << (cfb)->redShift))
/* the ICD's float -> int: the bits of f + 2^23, unmasked */
#define FTOI(f, i)      (*(__GLfloat *)&(i) = (f) + (__GLfloat)(1 << 23))
#define COLORTOPIXEL(cfb, r, g, b) \
    (((r) << (cfb)->redShift) | ((g) << (cfb)->greenShift) | ((b) << (cfb)->blueShift))
/* the rounding float -> int of the dithered spans: the bits of f + 1.5 * 2^23 */
#define FTOIR(f, i)     (*(__GLfloat *)&(i) = (f) + (__GLfloat)(3 << 22))
typedef void (__fastcall *__GLclearRectProc)(__GLcolorBufferICD *cfb, GLint x0, GLint y0,
						GLint x1, GLint y1);
#define __GL_UB_MAGIC       ((__GLfloat) 8388608.0)
extern void __fastcall rgb_Clear(__GLcolorBufferICD *cfb, GLuint mask);
extern void __fastcall rgb_Pick(__GLcontext *gc, __GLcolorBufferICD *cfb);
extern void __fastcall rgb_Pick16(__GLcontext *gc, __GLcolorBufferICD *cfb);
extern void __fastcall rgb_Fetch_8(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result);
extern void __fastcall rgb_Fetch_16(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result);
extern void __fastcall Fetch_24(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result);
extern void __fastcall rgb_Fetch_32(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result);
extern void __fastcall rgb_Store_16(__GLcolorBuffer *cfb, const __GLfragment *frag);
extern void __fastcall rgb_ClearRect(__GLcolorBufferICD *cfb, GLint x, GLint y, GLint x1, GLint y1);
extern GLboolean __fastcall StoreSpan_16_General(__GLcontext *gc);
extern GLboolean __fastcall StoreSpan_16(__GLcontext *gc);
extern GLboolean __fastcall StoreSpan_16_Dither(__GLcontext *gc);
extern void __fastcall rgb_Store_8(__GLcolorBuffer *cfb, const __GLfragment *frag);
extern void __fastcall Store_24(__GLcolorBuffer *cfb, const __GLfragment *frag);
extern void __fastcall ClearRect_24(__GLcolorBufferICD *cfb, GLint x0, GLint y, GLint x1, GLint y1);
extern void __fastcall rgb_ClearRegionRects(__GLcolorBufferICD *cfb, __GLclearRectProc clearRect);

/* 690c56a0 -- ClearFill (s_rgb.c): colour buffer clearRect, the fill done here */
void __fastcall ClearFill(__GLcolorBufferICD *cfb, GLint x0, GLint y0, GLint x1, GLint y1)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *drawableBuf = cfb->buf.wglBuf;
    GLint w, h;
    __GLcolor color;
    GLuint clearVal;

    color.r = gc->frontBuffer.redScale * gc->state.raster.clear.r;
    color.g = gc->frontBuffer.greenScale * gc->state.raster.clear.g;
    color.b = gc->frontBuffer.blueScale * gc->state.raster.clear.b;

    if (gc->modes.alphaBits > 0) {
        color.a = gc->frontBuffer.alphaScale * gc->state.raster.clear.a;
        clearVal =
            (((GLuint) (color.r + __glHalf)) << cfb->redShift) |
            (((GLuint) (color.g + __glHalf)) << cfb->greenShift) |
            (((GLuint) (color.b + __glHalf)) << cfb->blueShift) |
            (((GLuint) (color.a + __glHalf)) << cfb->alphaShift);
    } else {
        clearVal =
            (((GLuint) (color.r + __glHalf)) << cfb->redShift) |
            (((GLuint) (color.g + __glHalf)) << cfb->greenShift) |
            (((GLuint) (color.b + __glHalf)) << cfb->blueShift);
    }

    w = x1 - x0;
    h = y1 - y0;
    if (w > 0 && h > 0) {
        switch (__GL_CFB_WGL(cfb)->elementSize) {
        case 1:
            FastMemClear1(gc, (GLubyte *)drawableBuf->base
                         + ((y0 - gc->constants.viewportYAdjust) * drawableBuf->outerWidth
                            - gc->constants.viewportXAdjust + x0) * drawableBuf->elementSize,
                         w, h, __GL_CFB_WGL(cfb)->outerWidth - w, clearVal);
            break;
        case 2:
            __glFillRect16(gc, (GLushort *)((GLubyte *)drawableBuf->base
                         + ((y0 - gc->constants.viewportYAdjust) * drawableBuf->outerWidth
                            - gc->constants.viewportXAdjust + x0) * drawableBuf->elementSize),
                         w, h, (__GL_CFB_WGL(cfb)->outerWidth - w) * 2, (GLushort)clearVal);
            break;
        case 4:
            fillRectDwords(gc, (GLuint *)((GLubyte *)drawableBuf->base
                         + ((y0 - gc->constants.viewportYAdjust) * drawableBuf->outerWidth
                            - gc->constants.viewportXAdjust + x0) * drawableBuf->elementSize),
                         w, h, (__GL_CFB_WGL(cfb)->outerWidth - w) * 4, clearVal);
            break;
        }
    }
}
/* 690cbd40 -- __glInitRGB (s_rgb.c).  The fetch procs are chosen by the mode word at 0x8e0
** (glctx.h's indexBits).
*/
void __fastcall __glInitRGB(__GLcolorBufferICD *cfb, __GLcontext *gc)
{
    GLint i;
    GLuint mask;
    GLuint sourceMask;

    __glInitBuffer(&cfb->buf);

    CFB_NEEDCOLORFRAGMENTOPS(cfb) = GL_FALSE;

    cfb->readSpan = (__GLproc) __glReadSpan;
    cfb->returnSpan = (__GLproc) __glReturnSpan;

    cfb->clear = (__GLproc) rgb_Clear;
    cfb->pick = (__GLproc) rgb_Pick;
    cfb->fetchSpan = (__GLproc) __glFetchSpan;
    cfb->fetchStippledSpan = (__GLproc) __glFetchSpan;
    cfb->storeSpan = (__GLproc) rgb_StoreSpan;
    cfb->storeStippledSpan = (__GLproc) rgb_StoreStippledSpan;

    mask = gc->modes.redMask;
    sourceMask = mask;
    for (i=0; mask; i++, mask>>=1) if (mask & 1) break;
    cfb->redShift = i;
    cfb->redScale = cfb->iRedScale = cfb->redMax = mask;

    mask = gc->modes.greenMask;
    sourceMask |= mask;
    for (i=0; mask; i++, mask>>=1) if (mask & 1) break;
    cfb->greenShift = i;
    cfb->greenScale = cfb->iGreenScale = cfb->greenMax = mask;

    mask = gc->modes.blueMask;
    sourceMask |= mask;
    for (i=0; mask; i++, mask>>=1) if (mask & 1) break;
    cfb->blueShift = i;
    cfb->blueScale = cfb->iBlueScale = cfb->blueMax = mask;

    mask = gc->modes.alphaMask;
    sourceMask |= mask;
    for (i=0; mask; i++, mask>>=1) if (mask & 1) break;
    cfb->alphaShift = i;
    if (mask) {
        cfb->alphaScale = cfb->iAlphaScale = mask;
    } else {
        /* Set to number large enough for table lookups */
        cfb->alphaScale = cfb->iAlphaScale = 255;
    }

    cfb->sourceMask = sourceMask;

    switch (gc->modes.indexBits) {
    case 8:
        cfb->fetch = cfb->readColor = (__GLproc) rgb_Fetch_8;
        break;
    case 16:
        cfb->fetch = cfb->readColor = (__GLproc) rgb_Fetch_16;

        /* Enable FB-specific picker */
        cfb->pick = (__GLproc) rgb_Pick16;
        break;
    case 24:
        cfb->fetch = cfb->readColor = (__GLproc) Fetch_24;
        break;
    case 32:
        if (gc->modes.alphaMask) {
            cfb->fetch = cfb->readColor = (__GLproc) Fetch_32A;
        } else {
            cfb->fetch = cfb->readColor = (__GLproc) rgb_Fetch_32;
        }
        break;
    }
}

/* 690cbf30 -- rgb_Fetch_8: s_rgb.c Fetch_8 (no ownership test in the ICD) */
void __fastcall rgb_Fetch_8(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *drawableBuf = cfb->buf.wglBuf;
    GLubyte *fp;
    GLubyte value;

    fp = __GL_FB_ADDRESS(gc, cfb, drawableBuf, (GLubyte *), x, y);
    value = *fp;
    result->r = (value & gc->modes.redMask) >> cfb->redShift;
    result->g = (value & gc->modes.greenMask) >> cfb->greenShift;
    result->b = (value & gc->modes.blueMask) >> cfb->blueShift;
    result->a = cfb->alphaScale;
}

/* 690cbff0 -- rgb_Fetch_16: s_rgb.c Fetch_16 */
void __fastcall rgb_Fetch_16(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *drawableBuf = cfb->buf.wglBuf;
    GLushort *fp;
    GLushort value;

    fp = __GL_FB_ADDRESS(gc, cfb, drawableBuf, (GLushort *), x, y);
    value = *fp;
    result->r = (value & gc->modes.redMask) >> cfb->redShift;
    result->g = (value & gc->modes.greenMask) >> cfb->greenShift;
    result->b = (value & gc->modes.blueMask) >> cfb->blueShift;
    result->a = cfb->alphaScale;
}

/* 690cc0a0 -- s_rgb.c Fetch_24: read the 24-bit DIB pixel at (x, y) (B, G, R bytes, rows padded to 4 bytes) as
** integer component values, alpha = cfb->alphaScale */
void __fastcall Fetch_24(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *drawableBuf = cfb->buf.wglBuf;
    GLubyte *fp;
    GLuint stride;

    stride = ((drawableBuf->outerWidth * 3) + 3) & ~3;
    fp = ((GLubyte *) drawableBuf->base) +
	(y - gc->constants.viewportYAdjust) * stride +
	(x - gc->constants.viewportXAdjust) * 3;

    result->r = fp[2];
    result->g = fp[1];
    result->b = fp[0];
    result->a = cfb->alphaScale;
}

/* 690cc1e0 -- rgb_Fetch_32: s_rgb.c Fetch_32 */
void __fastcall rgb_Fetch_32(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *drawableBuf = cfb->buf.wglBuf;
    GLuint *fp;
    GLuint value;

    fp = __GL_FB_ADDRESS(gc, cfb, drawableBuf, (GLuint *), x, y);
    value = *fp;
    result->r = (value & gc->modes.redMask) >> cfb->redShift;
    result->g = (value & gc->modes.greenMask) >> cfb->greenShift;
    result->b = (value & gc->modes.blueMask) >> cfb->blueShift;
    result->a = cfb->alphaScale;
}

/* 690cc6b0 -- rgb_Store_8: s_rgb.c Store_8 */
void __fastcall rgb_Store_8(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLdrawableBuffer *drawableBuf = cfb->buf.wglBuf;
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    GLubyte *fp;
    __GLcolor blendColor;
    const __GLcolor *color;
    __GLfloat inc;
    GLubyte fbcolor, result;
    GLint ix;

    if ((modeFlags & __GL_SHADE_OWNERSHIP_TEST) &&
		!__glTestOwnership(gc, frag->x, frag->y)) {
	return;
    }

    fp = __GL_FB_ADDRESS(gc, cfb, drawableBuf, (GLubyte *), frag->x, frag->y);
    if (modeFlags & __GL_SHADE_DITHER) {
	ix = __GL_DITHER_INDEX(frag->x, frag->y);
	inc = ((__glDitherTable[ix] << 1) + 1) / 
	    (__GLfloat) (2 * __GL_DITHER_PRECISION);
    } else {
	inc = __glHalf;
    }
    if (modeFlags & __GL_SHADE_BLEND) {
	color = &blendColor;
	(*(__GLblendProc)gc->procs.blend)(gc, cfb, frag, &blendColor);
    } else {
	color = &(frag->color);
    }
    result = (GLubyte) ColorToPixel(gc, cfb,
				     color->r + inc,
				     color->g + inc,
				     color->b + inc);

    fbcolor = *fp;
    if (modeFlags & __GL_SHADE_LOGICOP) {
	switch(gc->state.raster.logicOp) {
	  case GL_CLEAR:         result = 0; break;
	  case GL_AND:           result = result & fbcolor; break;
	  case GL_AND_REVERSE:   result = result & (~fbcolor); break;
	  case GL_COPY:          result = result; break;
	  case GL_AND_INVERTED:  result = (~result) & fbcolor; break;
	  case GL_NOOP:          result = fbcolor; break;
	  case GL_XOR:           result = result ^ fbcolor; break;
	  case GL_OR:            result = result | fbcolor; break;
	  case GL_NOR:           result = ~(result | fbcolor); break;
	  case GL_EQUIV:         result = ~(result ^ fbcolor); break;
	  case GL_INVERT:        result = ~fbcolor; break;
	  case GL_OR_REVERSE:    result = result | (~fbcolor); break;
	  case GL_COPY_INVERTED: result = ~result; break;
	  case GL_OR_INVERTED:   result = (~result) | fbcolor; break;
	  case GL_NAND:          result = ~(result & fbcolor); break;
	  case GL_SET:           result = ~0; break;
	}
    }

    *fp = (fbcolor & cfb->destMask) | (result & cfb->sourceMask);
}

/* 690cc870 -- rgb_Store_16: s_rgb.c rgb_Store_16 */
void __fastcall rgb_Store_16(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLdrawableBuffer *drawableBuf = cfb->buf.wglBuf;
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    GLushort *fp;
    __GLcolor blendColor;
    const __GLcolor *color;
    __GLfloat inc;
    GLushort fbcolor, result;
    GLint ix;

    if ((modeFlags & __GL_SHADE_OWNERSHIP_TEST) &&
		!__glTestOwnership(gc, frag->x, frag->y)) {
	return;
    }

    fp = __GL_FB_ADDRESS(gc, cfb, drawableBuf, (GLushort *), frag->x, frag->y);
    if (modeFlags & __GL_SHADE_DITHER) {
	ix = __GL_DITHER_INDEX(frag->x, frag->y);
	inc = ((__glDitherTable[ix] << 1) + 1) / 
	    (__GLfloat) (2 * __GL_DITHER_PRECISION);
    } else {
	inc = __glHalf;
    }
    if (modeFlags & __GL_SHADE_BLEND) {
	color = &blendColor;
	(*(__GLblendProc)gc->procs.blend)(gc, cfb, frag, &blendColor);
    } else {
	color = &(frag->color);
    }
    result = (GLushort) ColorToPixel(gc, cfb,
				     color->r + inc,
				     color->g + inc,
				     color->b + inc);

    fbcolor = *fp;
    if (modeFlags & __GL_SHADE_LOGICOP) {
	switch(gc->state.raster.logicOp) {
	  case GL_CLEAR:         result = 0; break;
	  case GL_AND:           result = result & fbcolor; break;
	  case GL_AND_REVERSE:   result = result & (~fbcolor); break;
	  case GL_COPY:          result = result; break;
	  case GL_AND_INVERTED:  result = (~result) & fbcolor; break;
	  case GL_NOOP:          result = fbcolor; break;
	  case GL_XOR:           result = result ^ fbcolor; break;
	  case GL_OR:            result = result | fbcolor; break;
	  case GL_NOR:           result = ~(result | fbcolor); break;
	  case GL_EQUIV:         result = ~(result ^ fbcolor); break;
	  case GL_INVERT:        result = ~fbcolor; break;
	  case GL_OR_REVERSE:    result = result | (~fbcolor); break;
	  case GL_COPY_INVERTED: result = ~result; break;
	  case GL_OR_INVERTED:   result = (~result) | fbcolor; break;
	  case GL_NAND:          result = ~(result & fbcolor); break;
	  case GL_SET:           result = ~0; break;
	}
    }

    *fp = (fbcolor & cfb->destMask) | (result & cfb->sourceMask);
}

/* 690cca40 -- s_rgb.c Store_24: store one fragment into the 24-bit DIB (B, G, R bytes, rows padded to 4 bytes):
** ownership test, dither (or +0.5) rounding, blend, logic op on the pixel assembled with the colour shifts, then the
** source / dest masks, written back byte by byte */
void __fastcall Store_24(__GLcolorBuffer *cfb, const __GLfragment *frag)
{
    __GLcontext *gc = cfb->buf.gc;
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    GLubyte *fp;
    __GLcolor blendColor;
    const __GLcolor *color;
    __GLfloat inc;
    GLuint fbcolor, result;
    GLint ix;
    GLuint stride;

    if ((modeFlags & __GL_SHADE_OWNERSHIP_TEST) &&
		!__glTestOwnership(gc, frag->x, frag->y)) {
	return;
    }

    stride = ((__GL_CFB_WGL(cfb)->outerWidth * 3) + 3) & ~3;
    fp = ((GLubyte *) __GL_CFB_WGL(cfb)->base) +
	(frag->y - gc->constants.viewportYAdjust) * stride +
	(frag->x - gc->constants.viewportXAdjust) * 3;

    if (modeFlags & __GL_SHADE_DITHER) {
	ix = __GL_DITHER_INDEX(frag->x, frag->y);
	inc = ((__glDitherTable[ix] << 1) + 1) / 
	    (__GLfloat) (2 * __GL_DITHER_PRECISION);
    } else {
	inc = __glHalf;
    }
    if (modeFlags & __GL_SHADE_BLEND) {
	color = &blendColor;
	(*(__GLblendProc)gc->procs.blend)(gc, cfb, frag, &blendColor);
    } else {
	color = &(frag->color);
    }
    result = ColorToPixel(gc, cfb,
			  color->r + inc,
			  color->g + inc,
			  color->b + inc);

    fbcolor = (fp[1] << cfb->greenShift) | (fp[0] << cfb->blueShift) | (fp[2] << cfb->redShift);
    if (modeFlags & __GL_SHADE_LOGICOP) {
	switch(gc->state.raster.logicOp) {
	  case GL_CLEAR:         result = 0; break;
	  case GL_AND:           result = result & fbcolor; break;
	  case GL_AND_REVERSE:   result = result & (~fbcolor); break;
	  case GL_COPY:          result = result; break;
	  case GL_AND_INVERTED:  result = (~result) & fbcolor; break;
	  case GL_NOOP:          result = fbcolor; break;
	  case GL_XOR:           result = result ^ fbcolor; break;
	  case GL_OR:            result = result | fbcolor; break;
	  case GL_NOR:           result = ~(result | fbcolor); break;
	  case GL_EQUIV:         result = ~(result ^ fbcolor); break;
	  case GL_INVERT:        result = ~fbcolor; break;
	  case GL_OR_REVERSE:    result = result | (~fbcolor); break;
	  case GL_COPY_INVERTED: result = ~result; break;
	  case GL_OR_INVERTED:   result = (~result) | fbcolor; break;
	  case GL_NAND:          result = ~(result & fbcolor); break;
	  case GL_SET:           result = ~0; break;
	}
    }

    result = (fbcolor & cfb->destMask) | (result & cfb->sourceMask);
    fp[2] = (GLubyte) (result >> cfb->redShift);
    fp[1] = (GLubyte) (result >> cfb->greenShift);
    fp[0] = (GLubyte) (result >> cfb->blueShift);
}

/* 690ccc60 -- s_rgb.c Store_32A (the ICD: alpha without the clamp, through the pixel helper) */
void __fastcall Store_32A(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *drawableBuf = __GL_CFB_WGL(cfb);
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    GLuint *fp;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLuint fbcolor, result;

    if ((modeFlags & __GL_SHADE_OWNERSHIP_TEST) &&
		!__glTestOwnership(gc, frag->x, frag->y)) {
	return;
    }

    fp = __GL_FB_ADDRESS(gc, cfb, drawableBuf, (GLuint *), frag->x, frag->y);
    if (modeFlags & __GL_SHADE_BLEND) {
	color = &blendColor;
	(*(__GLblendProc)gc->procs.blend)(gc, cfb, frag, &blendColor);
    } else {
	color = &(frag->color);
    }

    result = ColorToPixelAW16(gc, cfb,
			      color->r + __glHalf,
			      color->g + __glHalf,
			      color->b + __glHalf,
			      color->a + __glHalf);

    fbcolor = *fp;
    if (modeFlags & __GL_SHADE_LOGICOP) {
	switch(gc->state.raster.logicOp) {
	  case GL_CLEAR:         result = 0; break;
	  case GL_AND:           result = result & fbcolor; break;
	  case GL_AND_REVERSE:   result = result & (~fbcolor); break;
	  case GL_COPY:          result = result; break;
	  case GL_AND_INVERTED:  result = (~result) & fbcolor; break;
	  case GL_NOOP:          result = fbcolor; break;
	  case GL_XOR:           result = result ^ fbcolor; break;
	  case GL_OR:            result = result | fbcolor; break;
	  case GL_NOR:           result = ~(result | fbcolor); break;
	  case GL_EQUIV:         result = ~(result ^ fbcolor); break;
	  case GL_INVERT:        result = ~fbcolor; break;
	  case GL_OR_REVERSE:    result = result | (~fbcolor); break;
	  case GL_COPY_INVERTED: result = ~result; break;
	  case GL_OR_INVERTED:   result = (~result) | fbcolor; break;
	  case GL_NAND:          result = ~(result & fbcolor); break;
	  case GL_SET:           result = ~0; break;
	}
    }

    *fp = (fbcolor & cfb->destMask) | (result & cfb->sourceMask);
}

/* 690cce20 -- rgb_Store_32: s_rgb.c Store_32 */
void __fastcall rgb_Store_32(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *drawableBuf = __GL_CFB_WGL(cfb);
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    GLuint *fp;
    __GLcolor blendColor;
    const __GLcolor *color;
    GLuint fbcolor, result;

    if ((modeFlags & __GL_SHADE_OWNERSHIP_TEST) &&
		!__glTestOwnership(gc, frag->x, frag->y)) {
	return;
    }

    fp = __GL_FB_ADDRESS(gc, cfb, drawableBuf, (GLuint *), frag->x, frag->y);
    if (modeFlags & __GL_SHADE_BLEND) {
	color = &blendColor;
	(*(__GLblendProc)gc->procs.blend)(gc, cfb, frag, &blendColor);
    } else {
	color = &(frag->color);
    }

    result = ColorToPixelW16(gc, cfb,
			     color->r + __glHalf,
			     color->g + __glHalf,
			     color->b + __glHalf);

    fbcolor = *fp;
    if (modeFlags & __GL_SHADE_LOGICOP) {
	switch(gc->state.raster.logicOp) {
	  case GL_CLEAR:         result = 0; break;
	  case GL_AND:           result = result & fbcolor; break;
	  case GL_AND_REVERSE:   result = result & (~fbcolor); break;
	  case GL_COPY:          result = result; break;
	  case GL_AND_INVERTED:  result = (~result) & fbcolor; break;
	  case GL_NOOP:          result = fbcolor; break;
	  case GL_XOR:           result = result ^ fbcolor; break;
	  case GL_OR:            result = result | fbcolor; break;
	  case GL_NOR:           result = ~(result | fbcolor); break;
	  case GL_EQUIV:         result = ~(result ^ fbcolor); break;
	  case GL_INVERT:        result = ~fbcolor; break;
	  case GL_OR_REVERSE:    result = result | (~fbcolor); break;
	  case GL_COPY_INVERTED: result = ~result; break;
	  case GL_OR_INVERTED:   result = (~result) | fbcolor; break;
	  case GL_NAND:          result = ~(result & fbcolor); break;
	  case GL_SET:           result = ~0; break;
	}
    }

    *fp = (fbcolor & cfb->destMask) | (result & cfb->sourceMask);
}

/* 690ccfd0 -- rgb_ClearRect: ClearRect (s_rgb.c): the generic clear through cfb->store.  The ICD turns blending
** and logic op off in the enables (not in the shader's mode flags, which are only recomputed).
*/
void __fastcall rgb_ClearRect(__GLcolorBufferICD *cfb, GLint x, GLint y, GLint x1, GLint y1)
{
    __GLcontext *gc = cfb->buf.gc;
    GLuint enables;
    GLint i, j;
    __GLfragmentICD frag;

    __GL_SHADE_MODEFLAGS(gc);

    /* Turn off blending and logicop so store proc doesn't do it */
    enables = gc->state.enables.general & (__GL_BLEND_ENABLE | __GL_COLOR_LOGIC_OP_ENABLE);
    gc->state.enables.general &= ~(__GL_BLEND_ENABLE | __GL_COLOR_LOGIC_OP_ENABLE);

    frag.color.r = gc->state.raster.clear.r * gc->frontBuffer.redScale;
    frag.color.g = gc->state.raster.clear.g * gc->frontBuffer.greenScale;
    frag.color.b = gc->state.raster.clear.b * gc->frontBuffer.blueScale;
    frag.color.a = gc->state.raster.clear.a * gc->frontBuffer.alphaScale;

    for (j = y; j < y1; j++) {
        frag.y = j;

        for (i = x; i < x1; i++) {
            frag.x = i;

            (*(__GLfn_store) cfb->store)(cfb, &frag);
        }
    }

    gc->state.enables.general |= enables;
}

/* 690cd0c0 -- ClearRect_24 (s_rgb.c): the 24-bit DIB clear, the buffer is the cfb's WGL buffer */
void __fastcall ClearRect_24(__GLcolorBufferICD *cfb,
                             GLint x0, GLint y, GLint x1, GLint y1)
{
    __GLcontext *gc = cfb->buf.gc;
    GLubyte *fp, *rfp;
    __GLfloat fr, fg, fb;
    unsigned int r, g, b;
    unsigned int color;
    GLuint pack0, pack1, pack2;
    int      stride;
    int      width, height;
    int      dword_width, pre_step_pixels, fixup_pixels;
    __WGLbuffer *drawableBuf;

    fr = gc->state.raster.clear.r * gc->frontBuffer.redScale;
    fg = gc->state.raster.clear.g * gc->frontBuffer.greenScale;
    fb = gc->state.raster.clear.b * gc->frontBuffer.blueScale;

    Float2Int(fr, r);
    Float2Int(fg, g);
    Float2Int(fb, b);

    color = (r << cfb->redShift) |
            (g << cfb->greenShift) |
            (b << cfb->blueShift);

    width  = x1 - x0;
    height = y1 - y;

    pack0 = (color << 24) | color;
    pack1 = (color << 16) | (color >> 8);
    pack2 = (color << 8) | (color >> 16);

    drawableBuf = cfb->buf.wglBuf;
    stride = ((drawableBuf->outerWidth * 3) + 3) & ~3;
    fp = ((GLubyte *) drawableBuf->base) +
        (y - gc->constants.viewportYAdjust) * stride +
        (x0 - gc->constants.viewportXAdjust) * 3;

    while (height) {
        rfp = fp;

        pre_step_pixels = (((unsigned) rfp) & 3);
        if (pre_step_pixels > width) {
            pre_step_pixels = width;
            dword_width = 0;
            fixup_pixels = 0;
        } else {
            dword_width     = ( width - pre_step_pixels ) >> 2;
            fixup_pixels    = width - (dword_width*4 + pre_step_pixels);
        }

        while (pre_step_pixels) {
            rfp[0] = b;
            rfp[1] = g;
            rfp[2] = r;
            rfp += 3;
            pre_step_pixels--;
        }

        while (dword_width) {
            ((GLuint *)rfp)[0] = pack0;
            ((GLuint *)rfp)[1] = pack1;
            ((GLuint *)rfp)[2] = pack2;

            rfp += 12;
            dword_width--;
        }

        while (fixup_pixels) {
            rfp[0] = b;
            rfp[1] = g;
            rfp[2] = r;

            rfp += 3;
            fixup_pixels--;
        }

        fp += stride;
        height--;
    }
}

/* 690cd370 -- rgb_Pick16: Pick16 (s_rgb.c; the ICD: modeFlags through getModeFlags, the masks set in an if / else,
** one rgb_Clear for every case that picks a clearRect proc instead) */
void __fastcall rgb_Pick16(__GLcontext *gc, __GLcolorBufferICD *cfb)
{
    GLuint totalMask, sourceMask;
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc, cfb);

    totalMask = gc->modes.redMask | gc->modes.greenMask | gc->modes.blueMask |
	    gc->modes.alphaMask;
    if (gc->state.raster.drawBuffer == GL_NONE) {
	cfb->sourceMask = 0;
	cfb->destMask = totalMask;
    } else {
	sourceMask = 0;
	if (gc->state.raster.rMask) {
	    sourceMask = gc->modes.redMask;
	}
	if (gc->state.raster.gMask) {
	    sourceMask |= gc->modes.greenMask;
	}
	if (gc->state.raster.bMask) {
	    sourceMask |= gc->modes.blueMask;
	}
	if (gc->state.raster.aMask) {
	    sourceMask |= gc->modes.alphaMask;
	}
	cfb->sourceMask = sourceMask;
	cfb->destMask = totalMask & ~sourceMask;
    }

    cfb->store = (__GLproc)rgb_Store_16;
    cfb->storeSpan = (__GLproc)StoreSpan_16_General;
    cfb->clear = (__GLproc)rgb_Clear;
    cfb->clearRect = (__GLproc)rgb_ClearRect;

    if (cfb->sourceMask == totalMask) {
	if (0 == (modeFlags & 0x180)) {        /* __GL_SHADE_LOGICOP | __GL_SHADE_BLEND */
	    if (modeFlags & 0x40) {            /* __GL_SHADE_DITHER */
		cfb->storeSpan = (__GLproc)StoreSpan_16_Dither;
	    } else {
		cfb->storeSpan = (__GLproc)StoreSpan_16;
	    }
	}
	if (modeFlags & 0x40) {
	    cfb->clearRect = (__GLproc)ClearRect_16_Dither;
	} else {
	    cfb->clearRect = (__GLproc)ClearFill;
	}
    }
}

/* 690cd480 -- StoreSpan_16_General (s_rgb.c): storeSpan for a 16-bit buffer, the full per-pixel path over the
** shader's colour span: ownership test, dither (or +0.5) rounding, blend, logic op, then the source / dest masks
** -- rgb_Store_16 inlined; returns GL_FALSE */
GLboolean __fastcall StoreSpan_16_General(__GLcontext *gc)
{
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    int x, x1;
    int i;
    __GLfragmentICD frag;
    __GLcolor *cp;
    __GLcolorBufferICD *cfb;
    __WGLbuffer *drawableBuf;
    GLint w;
    GLushort *fp;
    __GLcolor blendColor;
    const __GLcolor *color;
    __GLfloat inc;
    GLushort fbcolor, result;

    w = gc->polygon.shader.length;

    frag.y = gc->polygon.shader.frag.y;
    x = gc->polygon.shader.frag.x;
    x1 = gc->polygon.shader.frag.x + w;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;
    drawableBuf = cfb->buf.wglBuf;

    fp = __GL_FB_ADDRESS(gc, cfb, drawableBuf, (GLushort *), x, frag.y);

    for (i = x; i < x1; i++, fp++, cp++) {

        frag.x = i;
        frag.color = *cp;

        if ((modeFlags & __GL_SHADE_OWNERSHIP_TEST) &&
                    !__glTestOwnership(gc, frag.x, frag.y)) {
            continue;
        }

        if (modeFlags & __GL_SHADE_DITHER) {
            GLint ix = __GL_DITHER_INDEX(frag.x, frag.y);
            inc = ((__glDitherTable[ix] << 1) + 1) /
                (__GLfloat) (2 * __GL_DITHER_PRECISION);
        } else {
            inc = __glHalf;
        }
        if (modeFlags & __GL_SHADE_BLEND) {
            color = &blendColor;
            (*(RGBfn_blend) gc->procs.blend)(gc, cfb, &frag, &blendColor);
        } else {
            color = &frag.color;
        }

        result = (GLushort) ColorToPixel( gc, cfb, color->r + inc,
                                          color->g + inc, color->b + inc );

        fbcolor = *fp;
        if (modeFlags & __GL_SHADE_LOGICOP) {
            switch(gc->state.raster.logicOp) {
            case GL_CLEAR:         result = 0; break;
            case GL_AND:           result = result & fbcolor; break;
            case GL_AND_REVERSE:   result = result & (~fbcolor); break;
            case GL_COPY:          result = result; break;
            case GL_AND_INVERTED:  result = (~result) & fbcolor; break;
            case GL_NOOP:          result = fbcolor; break;
            case GL_XOR:           result = result ^ fbcolor; break;
            case GL_OR:            result = result | fbcolor; break;
            case GL_NOR:           result = ~(result | fbcolor); break;
            case GL_EQUIV:         result = ~(result ^ fbcolor); break;
            case GL_INVERT:        result = ~fbcolor; break;
            case GL_OR_REVERSE:    result = result | (~fbcolor); break;
            case GL_COPY_INVERTED: result = ~result; break;
            case GL_OR_INVERTED:   result = (~result) | fbcolor; break;
            case GL_NAND:          result = ~(result & fbcolor); break;
            case GL_SET:           result = ~0; break;
            }
        }

        *fp = (fbcolor & cfb->destMask) | (result & cfb->sourceMask);
    }

    return GL_FALSE;
}

/* 690cd6f0 -- StoreSpan_16 (s_rgb.c): storeSpan for a 16-bit buffer with all components written and no blend /
** logic op / dither: each colour of the shader's span converted by FTOI (truncated, no +0.5) and shifted into place,
** ownership test only; returns GL_FALSE */
GLboolean __fastcall StoreSpan_16(__GLcontext *gc)
{
    int x, x1, y;
    __GLcolor *cp;
    __GLcolorBufferICD *cfb;
    __WGLbuffer *drawableBuf;
    GLushort *fp;
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);

    y = gc->polygon.shader.frag.y;
    x = gc->polygon.shader.frag.x;
    x1 = x + gc->polygon.shader.length;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;
    drawableBuf = cfb->buf.wglBuf;

    fp = __GL_FB_ADDRESS(gc, cfb, drawableBuf, (GLushort *), x, y);

    for (;x < x1; x++, cp++, fp++) {
        unsigned int r, g, b;

        if ((modeFlags & __GL_SHADE_OWNERSHIP_TEST) &&
                    !__glTestOwnership(gc, x, y)) {
            continue;
        }
        FTOI(cp->r, r);
        FTOI(cp->g, g);
        FTOI(cp->b, b);
        *fp = (GLushort) COLORTOPIXEL(cfb, r, g, b);
    }

    return GL_FALSE;
}

/* 690cd800 -- StoreSpan_16_Dither (s_rgb.c): StoreSpan_16 with dithering: each component plus the
** __glDitherTableFloat entry for (x & 3, y & 3), rounded to nearest by FTOIR; returns GL_FALSE */
GLboolean __fastcall StoreSpan_16_Dither(__GLcontext *gc)
{
    int x, x1, y;
    __GLcolor *cp;
    __GLcolorBufferICD *cfb;
    __WGLbuffer *drawableBuf;
    GLushort *fp;
    __GLfloat *dither;
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);

    y = gc->polygon.shader.frag.y;
    x = gc->polygon.shader.frag.x;
    x1 = x + gc->polygon.shader.length;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;
    drawableBuf = cfb->buf.wglBuf;

    fp = __GL_FB_ADDRESS(gc, cfb, drawableBuf, (GLushort *), x, y);

    dither = __glDitherTableFloat + ((y<<2) & 12);

    for (;x < x1; x++, cp++, fp++) {
        unsigned int r, g, b;
        __GLfloat inc;

        if ((modeFlags & __GL_SHADE_OWNERSHIP_TEST) &&
                    !__glTestOwnership(gc, x, y)) {
            continue;
        }

        inc = dither[x & 3];

        FTOIR(cp->r + inc, r);
        FTOIR(cp->g + inc, g);
        FTOIR(cp->b + inc, b);
        *fp = (GLushort) COLORTOPIXEL(cfb, r, g, b);
    }

    return GL_FALSE;
}

/* 690cc340 -- rgb_ClearRegionRects: s_rgb.c ClearRegionRects */
void __fastcall rgb_ClearRegionRects(__GLcolorBufferICD *cfb, __GLclearRectProc clearRect)
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

/* 690cd280 -- s_rgb.c StoreSpan_24 (no dither in the ICD: the components are rounded by adding
** 2^23 and storing the low byte of the float) */
GLboolean __fastcall StoreSpan_24(__GLcontext *gc)
{
    int x, x1, y;
    __GLcolor *cp;
    __GLcolorBufferICD *cfb;
    __WGLbuffer *drawableBuf;
    GLubyte *fp;
    GLuint stride;
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);

    y = gc->polygon.shader.frag.y;
    x = gc->polygon.shader.frag.x;
    x1 = x + gc->polygon.shader.length;
    cp = gc->polygon.shader.colors;
    cfb = (__GLcolorBufferICD *)gc->polygon.shader.cfb;
    drawableBuf = __GL_CFB_WGL(cfb);

    stride = ((drawableBuf->outerWidth * 3) + 3) & ~3;
    fp = ((GLubyte *) drawableBuf->base) +
	(y - gc->constants.viewportYAdjust) * stride +
	(x - gc->constants.viewportXAdjust) * 3;

    for (;x < x1; x++, cp++, fp+=3) {
	union { __GLfloat f; GLubyte b; } r, g, b;  /* each its own stack slot */

	if ((modeFlags & __GL_SHADE_OWNERSHIP_TEST) &&
		    !__glTestOwnership(gc, x, y)) {
	    continue;
	}

	r.f = cp->r + __GL_UB_MAGIC;
	g.f = cp->g + __GL_UB_MAGIC;
	b.f = cp->b + __GL_UB_MAGIC;

	fp[2] = r.b;
	fp[1] = g.b;
	fp[0] = b.b;
    }

    return GL_FALSE;
}

/* 690cc500 -- rgb_Pick: Pick (s_rgb.c; the ICD: no fill() clear; the i386 clearRect choice) */
void __fastcall rgb_Pick(__GLcontext *gc, __GLcolorBufferICD *cfb)
{
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    GLuint totalMask;

    totalMask = gc->modes.redMask |
		gc->modes.greenMask |
		gc->modes.blueMask |
		gc->modes.alphaMask;

    if (gc->state.raster.drawBuffer == GL_NONE) {
	cfb->sourceMask = 0;
	cfb->destMask = totalMask;
    } else {
	GLuint sourceMask = 0;

	if (gc->state.raster.rMask) {
	    sourceMask |= gc->modes.redMask;
	}
	if (gc->state.raster.gMask) {
	    sourceMask |= gc->modes.greenMask;
	}
	if (gc->state.raster.bMask) {
	    sourceMask |= gc->modes.blueMask;
	}
	if (gc->state.raster.aMask) {
	    sourceMask |= gc->modes.alphaMask;
	}
	cfb->sourceMask = sourceMask;
	cfb->destMask = totalMask & ~sourceMask;
    }

    switch (__GL_CFB_WGL(cfb)->elementSize) {
    case 1:
	cfb->store = (__GLproc)rgb_Store_8;
	break;

    case 2:
	cfb->store = (__GLproc)rgb_Store_16;
	break;

    case 3:
	cfb->store = (__GLproc)Store_24;

	if (__GL_SHADE_MODEFLAGS(gc) & (__GL_SHADE_LOGICOP |
					__GL_SHADE_BLEND)) {
	    cfb->storeSpan = (__GLproc)rgb_StoreSpan;
	} else {
	    cfb->storeSpan = (__GLproc)StoreSpan_24;
	}
	break;

    case 4:
	if (gc->modes.alphaMask) {
	    cfb->store = (__GLproc)Store_32A;
	} else {
	    cfb->store = (__GLproc)rgb_Store_32;
	}
	break;
    }

    cfb->clear = (__GLproc)rgb_Clear;
    cfb->clearRect = (__GLproc)rgb_ClearRect;

    if (cfb->sourceMask == totalMask) {
	if (modeFlags & __GL_SHADE_DITHER) {
	    switch (__GL_CFB_WGL(cfb)->elementSize) {
	      case 1:
		  cfb->clearRect = (__GLproc)ClearRect_8_Dither;
		  break;
	      case 2:
		  cfb->clearRect = (__GLproc)ClearRect_16_Dither;
		  break;
	      case 3:
	      case 4:
		  cfb->clearRect = (__GLproc)ClearFill;
		  break;
	    }
	} else {
	    if (__GL_CFB_WGL(cfb)->elementSize == 3) {
		cfb->clearRect = (__GLproc)ClearRect_24;
	    } else {
		cfb->clearRect = (__GLproc)ClearFill;
	    }
	}
    }
}

/* 690cc290 -- rgb_Clear: rgb_Clear (s_rgb.c; the ICD: drawable and buffer locks around the clear) */
void __fastcall rgb_Clear(__GLcolorBufferICD *cfb, GLuint mask)
{
    __GLcontext *gc = cfb->buf.gc;
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    __WGLbuffer *buf;

    (*gc->drawablePrivate->lockDP)(gc->drawablePrivate, gc);
    buf = cfb->buf.wglBuf;
    if (buf->lock) {
	(*buf->lock)(buf, gc->drawablePrivate);
    }
    if (gc->buffers.lockDevice) {
	((void (__fastcall *)(__GLcontext *)) gc->buffers.lockDevice)(gc);
    }
    if (modeFlags & __GL_SHADE_OWNERSHIP_TEST) {
	rgb_ClearRegionRects(cfb, (__GLclearRectProc) cfb->clearRect);
    } else {
	(*(__GLclearRectProc) cfb->clearRect)(cfb, gc->transform.clipX0, gc->transform.clipY0,
						 gc->transform.clipX1, gc->transform.clipY1);
    }
    if (gc->buffers.unlockDevice) {
	((void (__fastcall *)(__GLcontext *)) gc->buffers.unlockDevice)(gc);
    }
    buf = cfb->buf.wglBuf;
    if (buf->unlock) {
	(*buf->unlock)(buf, gc->drawablePrivate);
    }
    (*gc->drawablePrivate->unlockDP)(gc->drawablePrivate);
}

/*
** sst_rgb.c -- the colour buffer of the hardware path (sstRGB_*): s_rgb.c procs copied for the hardware module
** (fetch, store, spans, clear, pick).
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
#include "glsitypes.h"

#define __GL_FB_ADDRESS(gc,fb,drawableBuf,cast,x,y) \
    (cast (((GLubyte *) (drawableBuf)->base) \
	+ (((y) - (gc)->constants.viewportYAdjust) \
            * (drawableBuf)->outerWidth \
	   + (x) - (gc)->constants.viewportXAdjust) \
	* (drawableBuf)->elementSize))
#define __GL_DITHER_INDEX(x, y) (((x) & 3) + (((y) & 3) << 2))
#define __GL_DITHER_PRECISION   16
typedef void (__fastcall *__GLblendProc)(__GLcontext *gc, __GLcolorBufferICD *cfb,
					    const __GLfragmentICD *frag, __GLcolor *result);
/* ColorToPixelW27: s_rgb.c ColorToPixel without the clamp: pack the integer r, g, b at the buffer's red / green /
** blue shifts (sstRGB_Store_8's copy; inlined) */
static __inline GLuint ColorToPixelW27(__GLcontext *gc, __GLcolorBufferICD *cfb,
				       GLint r, GLint g, GLint b)
{
    GLuint pixel;

    pixel = (r << cfb->redShift) | (g << cfb->greenShift) |
	(b << cfb->blueShift);
    return pixel;
}
typedef __GLfragmentICD __GLfragment;
typedef void (__fastcall *__GLblendProcICD)(__GLcontext *gc, __GLcolorBufferICD *cfb, const __GLfragment *frag, __GLcolor *result);
/* ColorToPixelICD: s_rgb.c ColorToPixel without the clamp: pack the integer r, g, b at the buffer's red / green /
** blue shifts (sstRGB_Store_16's and sstRGB_Store_24's copy; inlined) */
static __inline GLuint ColorToPixelICD(__GLcontext *gc, __GLcolorBufferICD *cfb,
			   GLint r, GLint g, GLint b)
{
    GLuint pixel;

    pixel = (r << cfb->redShift) | (g << cfb->greenShift) |
	(b << cfb->blueShift);
    return pixel;
}
#define __GL_CFB_WGL(cfb)   (((__GLcolorBufferICD *)(cfb))->buf.wglBuf)
/* ColorToPixelAW27: the alpha ColorToPixel without the clamp: pack the integer r, g, b, a at the buffer's red /
** green / blue / alpha shifts (sstRGB_Store_32A's copy; inlined) */
static __inline GLuint ColorToPixelAW27(__GLcontext *gc, __GLcolorBufferICD *cfb,
					GLint r, GLint g, GLint b, GLint a)
{
    GLuint pixel;

    pixel = (r << cfb->redShift) | (g << cfb->greenShift) |
	(b << cfb->blueShift) | (a << cfb->alphaShift);
    return pixel;
}
typedef void (__fastcall *__GLblendProcW16)(__GLcontext *gc, __GLcolorBufferICD *cfb,
					    const __GLfragmentICD *frag, __GLcolor *result);
/* ColorToPixelW16: s_rgb.c ColorToPixel without the clamp: pack the integer r, g, b at the buffer's red / green /
** blue shifts (sstRGB_Store_32's copy; inlined) */
static __inline GLuint ColorToPixelW16(__GLcontext *gc, __GLcolorBufferICD *cfb,
				       GLint r, GLint g, GLint b)
{
    GLuint pixel;

    pixel = (r << cfb->redShift) | (g << cfb->greenShift) |
	(b << cfb->blueShift);
    return pixel;
}
/* ---- s_rgb.c (the hardware module's 16 / 24-bit colour buffer procs) ---- */

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
typedef void (__fastcall *__GLfn_store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
#define __GL_UB_MAGIC       ((__GLfloat) 8388608.0)
/* ---- s_rgb.c __glInitRGB ---- */
#define CFB_NEEDCOLORFRAGMENTOPS(cfb)   ((cfb)->needColorFragmentOps)
typedef void (__fastcall *__GLclearRectProc_cr)(__GLcolorBufferICD *, GLint, GLint, GLint, GLint);
typedef void (__fastcall *__GLclearRectProc)(__GLcolorBufferICD *cfb, GLint x0, GLint y0,
						GLint x1, GLint y1);
extern GLboolean __fastcall sstRGB_StoreSpan(__GLcontext *gc);
extern void __fastcall sstRGB_Pick16(__GLcontext *gc, __GLcolorBufferICD *cfb);
extern void __fastcall sstRGB_Store_16(__GLcolorBufferICD *cfb, const __GLfragment *frag);
extern void __fastcall sstRGB_ClearRect(__GLcolorBufferICD *cfb, GLint x, GLint y, GLint x1, GLint y1);
extern GLboolean __fastcall sstRGB_StoreSpan_16_General(__GLcontext *gc);
extern GLboolean __fastcall sstRGB_StoreSpan_16(__GLcontext *gc);
extern GLboolean __fastcall sstRGB_StoreSpan_16_Dither(__GLcontext *gc);
extern void __fastcall sstRGB_Store_32A(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
extern void __fastcall sstRGB_Store_8(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
extern void __fastcall sstRGB_Store_24(__GLcolorBufferICD *cfb, const __GLfragment *frag);
extern void __fastcall sstRGB_ClearRect_24(__GLcolorBufferICD *cfb, GLint x0, GLint y, GLint x1, GLint y1);
extern void __fastcall sstRGB_Pick(__GLcontext *gc, __GLcolorBufferICD *cfb);
extern void __fastcall sstRGB_Fetch_8(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result);
extern void __fastcall sstRGB_Fetch_16(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result);
extern void __fastcall sstRGB_Fetch_24(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result);
extern void __fastcall sstRGB_Fetch_32(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result);
extern void __fastcall sstRGB_ClearRegionRects(__GLcolorBufferICD *cfb, __GLclearRectProc_cr clearRect);

/* 6904b030 -- s_rgb.c sstRGB_Fetch_8 (no ownership test in the ICD; twin of gl_rgb.c 690cbf30) */
void __fastcall sstRGB_Fetch_8(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result)
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

/* 6904b0f0 -- s_rgb.c sstRGB_Fetch_16 (twin of sst_rgb.c 690cbff0) */
void __fastcall sstRGB_Fetch_16(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result)
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

/* 6904b1a0 -- s_rgb.c sstRGB_Fetch_24 (twin of sst_rgb.c 690cc0a0) */
void __fastcall sstRGB_Fetch_24(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result)
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

/* 6904b2e0 -- s_rgb.c sstRGB_Fetch_32 (twin of sst_rgb.c 690cc1e0) */
void __fastcall sstRGB_Fetch_32(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result)
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

/* ---- the glcore twin in src/gl/si_s_rgb.c, copied (the hardware module has its own copy of the code) ---- */
/* 6904b4c0 -- sstRGB_StoreSpan: s_rgb.c StoreSpan (copy of 690cc3c0) */
GLboolean __fastcall sstRGB_StoreSpan(__GLcontext *gc)
{
    int x, x1;
    int i;
    __GLfragmentICD frag;
    __GLcolor *cp;
    __GLcolorBufferICD *cfb;
    GLint w;

    w = gc->polygon.shader.length;

    frag.y = gc->polygon.shader.frag.y;
    x = gc->polygon.shader.frag.x;
    x1 = gc->polygon.shader.frag.x + w;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    for (i = x; i < x1; i++) {
        frag.x = i;
        frag.color = *cp++;

        (*cfb->store)(cfb, &frag);
    }

    return GL_FALSE;
}

/* 6904b530 -- sstRGB_StoreStippledSpan: s_rgb.c StoreStippledSpan (copy of 690cc430) */
GLboolean __fastcall sstRGB_StoreStippledSpan(__GLcontext *gc)
{
    int x;
    __GLfragmentICD frag;
    __GLcolor *cp;
    __GLcolorBufferICD *cfb;
    __GLstippleWord inMask, bit, *sp;
    GLint count;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    frag.y = gc->polygon.shader.frag.y;
    x = gc->polygon.shader.frag.x;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp++;
        bit = __GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (inMask & bit) {
                frag.x = x;
                frag.color = *cp;

                (*cfb->store)(cfb, &frag);
            }
            x++;
            cp++;
#if defined(__GL_STIPPLE_MSB)
            bit >>= 1;
#else /* __GL_STIPPLE_MSB */
            bit <<= 1;
#endif /* __GL_STIPPLE_MSB */
        }
    }

    return GL_FALSE;
}

/* 6904b7b0 -- s_rgb.c sstRGB_Store_8 (twin of sst_rgb.c 690cc6b0) */
void __fastcall sstRGB_Store_8(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *drawableBuf = cfb->buf.wglBuf;
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
    result = (GLubyte) ColorToPixelW27(gc, cfb,
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

/* 6904b970 -- sstRGB_Store_16: s_rgb.c Store_16 (copy of 690cc870) */
void __fastcall sstRGB_Store_16(__GLcolorBufferICD *cfb, const __GLfragment *frag)
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
	(*(__GLblendProcICD)gc->procs.blend)(gc, cfb, frag, &blendColor);
    } else {
	color = &(frag->color);
    }
    result = (GLushort) ColorToPixelICD(gc, cfb,
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

/* 6904bb40 -- sstRGB_Store_24: s_rgb.c Store_24 (copy of 690cca40) */
void __fastcall sstRGB_Store_24(__GLcolorBufferICD *cfb, const __GLfragment *frag)
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
	(*(__GLblendProcICD)gc->procs.blend)(gc, cfb, frag, &blendColor);
    } else {
	color = &(frag->color);
    }
    result = ColorToPixelICD(gc, cfb,
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

/* 6904bd60 -- s_rgb.c sstRGB_Store_32A (the ICD: alpha without the clamp; twin of gl_rgb.c 690ccc60) */
void __fastcall sstRGB_Store_32A(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *drawableBuf = cfb->buf.wglBuf;
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

    result = ColorToPixelAW27(gc, cfb,
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

/* 6904bf20 -- sstRGB_Store_32: s_rgb.c Store_32 (copy of 690cce20) */
void __fastcall sstRGB_Store_32(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
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
	(*(__GLblendProcW16)gc->procs.blend)(gc, cfb, frag, &blendColor);
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

/* 6904c1c0 -- sstRGB_ClearRect_24: ClearRect_24 (s_rgb.c), the hardware module's copy (same code as the glcore's
** 690cd0c0) */
void __fastcall sstRGB_ClearRect_24(__GLcolorBufferICD *cfb,
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

extern void __fastcall sstRGB_Clear(__GLcolorBufferICD *cfb, GLuint mask);
/* 6904c470 -- sstRGB_Pick16 (SST_RGB.C Pick16): the 16-bit pick of the sst colour buffer: the masks, Store_16
** (6904b970) / StoreSpan (6904c580), Clear (6904b390) / ClearRect (6904c0d0); with every component written the 16-bit
** span stores (dithered 6904c900, plain 6904c7f0) unless blending or logic op, and the RGB clear rect (dithered
** 690c6200, plain 690c56a0) */
void __fastcall sstRGB_Pick16(__GLcontext *gc, __GLcolorBufferICD *cfb)
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

    cfb->store = (__GLproc)sstRGB_Store_16;
    cfb->storeSpan = (__GLproc)sstRGB_StoreSpan_16_General;

    cfb->clear = (__GLproc)sstRGB_Clear;
    cfb->clearRect = (__GLproc)sstRGB_ClearRect;

    if (cfb->sourceMask == totalMask) {
	if (0 == (modeFlags & (__GL_SHADE_LOGICOP | __GL_SHADE_BLEND))) {
	    if (modeFlags & __GL_SHADE_DITHER) {
		cfb->storeSpan = (__GLproc)sstRGB_StoreSpan_16_Dither;
	    } else {
		cfb->storeSpan = (__GLproc)sstRGB_StoreSpan_16;
	    }
	}
	if (modeFlags & __GL_SHADE_DITHER) {
	    cfb->clearRect = (__GLproc)ClearRect_16_Dither;
	} else {
	    cfb->clearRect = (__GLproc)ClearFill;
	}
    }
}

/* 6904c580 -- sstRGB_StoreSpan_16_General: StoreSpan_16_General (s_rgb.c), the hardware module's copy (same code
** as 690cd480) */
GLboolean __fastcall sstRGB_StoreSpan_16_General(__GLcontext *gc)
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

/* 6904c7f0 -- sstRGB_StoreSpan_16: StoreSpan_16 (s_rgb.c), the hardware module's copy of 690cd6f0 */
GLboolean __fastcall sstRGB_StoreSpan_16(__GLcontext *gc)
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

/* 6904c900 -- sstRGB_StoreSpan_16_Dither: StoreSpan_16_Dither (s_rgb.c), the hardware module's copy of 690cd800 */
GLboolean __fastcall sstRGB_StoreSpan_16_Dither(__GLcontext *gc)
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

/*
** 6904c0d0 -- sstRGB_ClearRect: ClearRect (s_rgb.c), the hardware module's copy of 690ccfd0: the generic clear
** through cfb->store.  The ICD turns blending and logic op off in the enables (not in the shader's mode flags,
** which are only recomputed).
*/
void __fastcall sstRGB_ClearRect(__GLcolorBufferICD *cfb, GLint x, GLint y, GLint x1, GLint y1)
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

/* 6904c380 -- sstRGB_StoreSpan_24: s_rgb.c StoreSpan_24, the hardware module's copy of 690cd280 (no dither in the
** ICD: the components are rounded by adding 2^23 and storing the low byte of the float) */
GLboolean __fastcall sstRGB_StoreSpan_24(__GLcontext *gc)
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
    drawableBuf = (((__GLcolorBufferICD *)(cfb))->buf.wglBuf);

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

/* 6904b600 -- sstRGB_Pick (s_rgb.c Pick): copy of 690cc500 with the module's procs; no fill() clear, the i386
** clearRect choice */
void __fastcall sstRGB_Pick(__GLcontext *gc, __GLcolorBufferICD *cfb)
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
	cfb->store = (__GLproc)sstRGB_Store_8;
	break;

    case 2:
	cfb->store = (__GLproc)sstRGB_Store_16;
	break;

    case 3:
	cfb->store = (__GLproc)sstRGB_Store_24;

	if (__GL_SHADE_MODEFLAGS(gc) & (__GL_SHADE_LOGICOP |
					__GL_SHADE_BLEND)) {
	    cfb->storeSpan = (__GLproc)sstRGB_StoreSpan;
	} else {
	    cfb->storeSpan = (__GLproc)sstRGB_StoreSpan_24;
	}
	break;

    case 4:
	if (gc->modes.alphaMask) {
	    cfb->store = (__GLproc)sstRGB_Store_32A;
	} else {
	    cfb->store = (__GLproc)sstRGB_Store_32;
	}
	break;
    }

    cfb->clear = (__GLproc)sstRGB_Clear;
    cfb->clearRect = (__GLproc)sstRGB_ClearRect;

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
		cfb->clearRect = (__GLproc)sstRGB_ClearRect_24;
	    } else {
		cfb->clearRect = (__GLproc)ClearFill;
	    }
	}
    }
}

/*
** 6904ae40 -- sstRGB_Init (s_rgb.c __glInitRGB, the hardware twin of gl_rgb.c 690cbd40).  The fetch procs are
** chosen by the mode word at 0x8e0 (glctx.h's indexBits).
*/
void __fastcall sstRGB_Init(__GLcolorBufferICD *cfb, __GLcontext *gc)
{
    GLint i;
    GLuint mask;
    GLuint sourceMask;

    hwInitBufferHead(&cfb->buf, gc);

    CFB_NEEDCOLORFRAGMENTOPS(cfb) = GL_FALSE;

    cfb->readSpan = (__GLproc) __glReadSpan;
    cfb->returnSpan = (__GLproc) __glReturnSpan;

    cfb->clear = (__GLproc) sstRGB_Clear;
    cfb->pick = (__GLproc) sstRGB_Pick;
    cfb->fetchSpan = (__GLproc) __glFetchSpan;
    cfb->fetchStippledSpan = (__GLproc) __glFetchSpan;
    cfb->storeSpan = (__GLproc) sstRGB_StoreSpan;
    cfb->storeStippledSpan = (__GLproc) sstRGB_StoreStippledSpan;

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
        cfb->fetch = cfb->readColor = (__GLproc) sstRGB_Fetch_8;
        break;
    case 16:
        cfb->fetch = cfb->readColor = (__GLproc) sstRGB_Fetch_16;

        /* Enable FB-specific picker */
        cfb->pick = (__GLproc) sstRGB_Pick16;
        break;
    case 24:
        cfb->fetch = cfb->readColor = (__GLproc) sstRGB_Fetch_24;
        break;
    case 32:
        if (gc->modes.alphaMask) {
            cfb->fetch = cfb->readColor = (__GLproc) sstRGB_Fetch_32A;
        } else {
            cfb->fetch = cfb->readColor = (__GLproc) sstRGB_Fetch_32;
        }
        break;
    }
}

/* 6904b440 -- sstRGB_ClearRegionRects: s_ci.c ClearRegionRects (copy of 6909c330) */
void __fastcall sstRGB_ClearRegionRects(__GLcolorBufferICD *cfb, __GLclearRectProc_cr clearRect)
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

/* 6904b390 -- sstRGB_Clear (s_rgb.c Clear): the sst colour buffer's clear (twin of gl_rgb.c 690cc290: drawable and
** buffer locks around the clear; ClearRegionRects 6904b440 under the ownership test) */
void __fastcall sstRGB_Clear(__GLcolorBufferICD *cfb, GLuint mask)
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
	sstRGB_ClearRegionRects(cfb, (__GLclearRectProc) cfb->clearRect);
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

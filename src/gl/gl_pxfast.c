/*
** gl_pxfast.c -- the ICD's direct pixel paths, no SI counterpart.
**
** glDrawPixels (picked by 6908cc90 when nothing modifies the pixels): unzoomed rows are packed
** straight into the WGL colour buffer, clipped to each rectangle of the drawable's ownership
** region.  glBitmap (picked by 6908cfc0 when no fragment op is on): the raster position is
** advanced, the bitmap clipped to the window clip box and its set bits written with the raster
** colour.
**
** 3dfx's ancestor is GLIDE3X/GLCORE/FASTDRAW.C (same functions in the same order; the ICD adds the
** per-rectangle clip and the RGBA-without-alpha path).  Its `for (i = 0; i < height; i++)` row loop
** is kept (VC6 then re-tests the row count before the loop, as the original).
**
** Codegen notes (VC6 RTM): the DrawPixels paths read their spanInfo
** fields the ancestor's way, inside the branch that uses them: startRow in each zoomy branch and
** startCol / endCol in each branch of `if (spanInfo->endCol > spanInfo->startCol)`.  Reading them
** once before the test gives the same values but other live ranges, and VC6 then keeps gc in
** another register (ecx/ebp instead of edx/ebx/esi) through the whole function.  The prologue
** statement order only steers the prologue schedule.  The DrawPixels paths also begin with two
** dead initialised locals (the ancestor's destRow / height): VC6 removes them, but they shift its
** register tie-breaks at the rectangle top -- without them it loads rect->x0 straight into x0
** where the original loads it into a spare register and copies xadj (b500/bd00/b8e0), or the
** other way round (c0e0); with them, and the x0, y0, x1, y1 statement order, all four match.
** The shift locals are read red, green, blue (the load order is visible in the prologue).  6908bd00 skips the alpha byte as
** `*sp >> s; sp += 2`.  The glBitmap paths read the context's ySign once more in the GL_SELECT
** branch -- these only steer register allocation and cross-jumping, the values are the same.
** Then fastdraw.c, in address order.
*/
#include <stdlib.h>
#include "wglpriv.h"
#include "glsi.h"
#include "siprotos.h"
#include "glsitypes.h"                      /* __GLbitmap */
#include "glshade.h"
#include "si_names.h"
#include <string.h>
#include "icdglob.h"
#include "icdprotos.h"

typedef __GLpixelSpanInfoICD __GLpixelSpanInfo;

/* the 4x4 ordered-dither matrix (Bayer order, 0x00..0xff in steps of 0x11) at 690f4490 */
/* 690f4490 -- the 4x4 dither table scaled to 0..0xff */
static GLuint __glDitherTable16[16] = {
    0x00, 0x88, 0x22, 0xaa,
    0xcc, 0x44, 0xee, 0x66,
    0x33, 0xbb, 0x11, 0x99,
    0xff, 0x77, 0xdd, 0x55,
};

typedef void (__fastcall *__GLtextureRasterProc)(__GLcontext *gc, __GLcolor *color,
						 __GLfloat s, __GLfloat t, __GLfloat rhow);
typedef void (__fastcall *__GLfogPointProc)(__GLcontext *gc, __GLfragmentICD *frag, __GLfloat eyeZ);

/* 6908b500 -- __glDrawPixels_FastRGB16 (3dfx FASTDRAW.C): RGB unsigned bytes into a 16-bit colour buffer */
void __fastcall __glDrawPixels_FastRGB16(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint destRow = spanInfo->startRow;     /* dead (the ancestor's row and height); see the top */
    GLint height = spanInfo->height;
    __GLcolorBufferICD *cfb = gc->drawBuffer;
    __WGLbuffer *buf = cfb->buf.wglBuf;
    __WGLdrawable *dp = gc->drawablePrivate;
    GLint rShift = 8 - gc->modes.redBits;
    GLint gShift = 8 - gc->modes.greenBits;
    GLint bShift = 8 - gc->modes.blueBits;
    GLint redShift = cfb->redShift;
    GLint greenShift = cfb->greenShift;
    GLint blueShift = cfb->blueShift;
    GLint outerWidth = (spanInfo->zoomy < 0 ? -1 : 1) * buf->outerWidth;
    GLint numRects = dp->numRects;
    __WGLregionRect *rect = dp->rects;
    GLint x0, x1, y0, y1, y, ey, rows, x, ex, width, i, k;
    const GLubyte *src, *sp;
    GLushort *dst, *dp16;

    for (; numRects > 0; numRects--, rect++) {
	x0 = gc->constants.viewportXAdjust + rect->x0;
	y0 = rect->y0 + gc->constants.viewportYAdjust;
	x1 = rect->x1 + gc->constants.viewportXAdjust;
	y1 = rect->y1 + gc->constants.viewportYAdjust;
	if (x1 - x0 == 0 || y0 - y1 == 0) continue;
	src = (const GLubyte *)spanInfo->srcCurrent;
	if (spanInfo->zoomy > 0) {
	    y = spanInfo->startRow;
	    ey = spanInfo->height + y;
	    if (y0 > y) {
		src += (y0 - y) * spanInfo->srcRowIncrement;
		y = y0;
	    }
	    if (y1 < ey) ey = y1;
	    rows = ey - y;
	} else {
	    y = spanInfo->startRow;
	    ey = y - spanInfo->height;
	    if (y1 < y) {
		src += (y - y1) * spanInfo->srcRowIncrement;
		y = y1;
	    }
	    if (y0 > ey) ey = y0;
	    rows = y - ey;
	}
	if (rows <= 0) continue;
	if (spanInfo->endCol > spanInfo->startCol) {
	    x = spanInfo->startCol;
	    ex = spanInfo->endCol;
	    if (x0 > x) {
		src += x0 - x;
		x = x0;
	    }
	    if (x1 < ex) ex = x1;
	    width = ex - x;
	    dst = (GLushort *)(((y - gc->constants.viewportYAdjust) * buf->outerWidth
				- gc->constants.viewportXAdjust + x) * buf->elementSize
			       + (GLubyte *)buf->base);
	    for (k = 0; k < rows; k++) {
		sp = src;
		dp16 = dst;
		for (i = width; i > 0; i--) {
		    GLuint r = *sp++ >> (GLubyte)rShift;
		    GLuint g = *sp++ >> (GLubyte)gShift;
		    GLuint b = *sp++ >> (GLubyte)bShift;
		    *dp16++ = (GLushort)((b << blueShift) | (g << greenShift) | (r << redShift));
		}
		src += spanInfo->srcRowIncrement;
		dst += outerWidth;
	    }
	} else {
	    x = spanInfo->startCol;
	    ex = spanInfo->endCol;
	    if (x1 < x) {
		src += x - x1;
		x = x1;
	    }
	    if (x0 > ex) ex = x0;
	    width = x - ex;
	    dst = (GLushort *)(((y - gc->constants.viewportYAdjust) * buf->outerWidth
				- gc->constants.viewportXAdjust + x) * buf->elementSize
			       + (GLubyte *)buf->base);
	    for (k = 0; k < rows; k++) {
		sp = src;
		dp16 = dst;
		for (i = width; i > 0; i--) {
		    GLuint r = *sp++ >> (GLubyte)rShift;
		    GLuint g = *sp++ >> (GLubyte)gShift;
		    GLuint b = *sp++ >> (GLubyte)bShift;
		    *dp16-- = (GLushort)((b << blueShift) | (g << greenShift) | (r << redShift));
		}
		src += spanInfo->srcRowIncrement;
		dst += outerWidth;
	    }
	}
    }
}

/* 6908b8e0 -- __glDrawPixels_FastRGB16_Dither (3dfx FASTDRAW.C): RGB unsigned bytes into a 16-bit colour
** buffer, dithered */
void __fastcall __glDrawPixels_FastRGB16_Dither(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint destRow = spanInfo->startRow;     /* dead (the ancestor's row and height); see the top */
    GLint height = spanInfo->height;
    __GLcolorBufferICD *cfb = gc->drawBuffer;
    __WGLdrawable *dp = gc->drawablePrivate;
    __WGLbuffer *buf = cfb->buf.wglBuf;
    GLint redShift = cfb->redShift;
    GLint greenShift = cfb->greenShift;
    GLint blueShift = cfb->blueShift;
    GLint redBits = gc->modes.redBits;
    GLint greenBits = gc->modes.greenBits;
    GLint blueBits = gc->modes.blueBits;
    GLint outerWidth = (spanInfo->zoomy < 0 ? -1 : 1) * buf->outerWidth;
    GLint numRects = dp->numRects;
    __WGLregionRect *rect = dp->rects;
    GLint x0, x1, y0, y1, y, ey, rows, x, ex, width, i, n, d, k;
    GLint r, g, b;
    const GLubyte *src, *sp;
    GLushort *dst, *dp16;

    for (n = 0; n < numRects; n++, rect++) {
	x0 = gc->constants.viewportXAdjust + rect->x0;
	y0 = rect->y0 + gc->constants.viewportYAdjust;
	x1 = rect->x1 + gc->constants.viewportXAdjust;
	y1 = rect->y1 + gc->constants.viewportYAdjust;
	if (x1 - x0 == 0 || y0 - y1 == 0) continue;
	src = (const GLubyte *)spanInfo->srcCurrent;
	if (spanInfo->zoomy > 0) {
	    y = spanInfo->startRow;
	    ey = y + spanInfo->height;
	    if (y0 > y) {
		src += (y0 - y) * spanInfo->srcRowIncrement;
		y = y0;
	    }
	    if (y1 < ey) ey = y1;
	    rows = ey - y;
	} else {
	    y = spanInfo->startRow;
	    ey = y - spanInfo->height;
	    if (y1 < y) {
		src += (y - y1) * spanInfo->srcRowIncrement;
		y = y1;
	    }
	    if (y0 > ey) ey = y0;
	    rows = y - ey;
	}
	if (rows <= 0) continue;
	if (spanInfo->endCol > spanInfo->startCol) {
	    x = spanInfo->startCol;
	    ex = spanInfo->endCol;
	    if (x0 > x) {
		src += x0 - x;
		x = x0;
	    }
	    if (x1 < ex) ex = x1;
	    width = ex - x;
	    dst = (GLushort *)(((y - gc->constants.viewportYAdjust) * buf->outerWidth
				- gc->constants.viewportXAdjust + x) * buf->elementSize
			       + (GLubyte *)buf->base);
	    for (k = 0; k < rows; k++) {
		sp = src;
		dp16 = dst;
		for (i = width; i > 0; i--) {
		    d = __glDitherTable16[(x & 3) + (y & 3) * 4];
		    r = *sp++;
		    g = *sp++;
		    b = *sp++;
		    *dp16++ = (GLushort)((((b << blueBits) - b + d) >> 8 << blueShift)
				       | (((g << greenBits) - g + d) >> 8 << greenShift)
				       | (((r << redBits) - r + d) >> 8 << redShift));
		}
		src += spanInfo->srcRowIncrement;
		dst += outerWidth;
	    }
	} else {
	    x = spanInfo->startCol;
	    ex = spanInfo->endCol;
	    if (x1 < x) {
		src += x - x1;
		x = x1;
	    }
	    if (x0 > ex) ex = x0;
	    width = x - ex;
	    dst = (GLushort *)(((y - gc->constants.viewportYAdjust) * buf->outerWidth
				- gc->constants.viewportXAdjust + x) * buf->elementSize
			       + (GLubyte *)buf->base);
	    for (k = 0; k < rows; k++) {
		sp = src;
		dp16 = dst;
		for (i = width; i > 0; i--) {
		    d = __glDitherTable16[(x & 3) + (y & 3) * 4];
		    r = *sp++;
		    g = *sp++;
		    b = *sp++;
		    *dp16-- = (GLushort)((((b << blueBits) - b + d) >> 8 << blueShift)
				       | (((g << greenBits) - g + d) >> 8 << greenShift)
				       | (((r << redBits) - r + d) >> 8 << redShift));
		}
		src += spanInfo->srcRowIncrement;
		dst += outerWidth;
	    }
	}
    }
}

/* 6908bd00 -- __glDrawPixels_FastRGBA16 (ICD-only): RGBA unsigned bytes (alpha ignored) into a 16-bit colour
** buffer */
void __fastcall __glDrawPixels_FastRGBA16(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint destRow = spanInfo->startRow;     /* dead (the ancestor's row and height); see the top */
    GLint height = spanInfo->height;
    __GLcolorBufferICD *cfb = gc->drawBuffer;
    __WGLbuffer *buf = cfb->buf.wglBuf;
    __WGLdrawable *dp = gc->drawablePrivate;
    GLint rShift = 8 - gc->modes.redBits;
    GLint gShift = 8 - gc->modes.greenBits;
    GLint bShift = 8 - gc->modes.blueBits;
    GLint redShift = cfb->redShift;
    GLint greenShift = cfb->greenShift;
    GLint blueShift = cfb->blueShift;
    GLint outerWidth = (spanInfo->zoomy < 0 ? -1 : 1) * buf->outerWidth;
    GLint numRects = dp->numRects;
    __WGLregionRect *rect = dp->rects;
    GLint x0, x1, y0, y1, y, ey, rows, x, ex, width, i, k;
    const GLubyte *src, *sp;
    GLushort *dst, *dp16;

    for (; numRects > 0; numRects--, rect++) {
	x0 = gc->constants.viewportXAdjust + rect->x0;
	y0 = rect->y0 + gc->constants.viewportYAdjust;
	x1 = rect->x1 + gc->constants.viewportXAdjust;
	y1 = rect->y1 + gc->constants.viewportYAdjust;
	if (x1 - x0 == 0 || y0 - y1 == 0) continue;
	src = (const GLubyte *)spanInfo->srcCurrent;
	if (spanInfo->zoomy > 0) {
	    y = spanInfo->startRow;
	    ey = spanInfo->height + y;
	    if (y0 > y) {
		src += (y0 - y) * spanInfo->srcRowIncrement;
		y = y0;
	    }
	    if (y1 < ey) ey = y1;
	    rows = ey - y;
	} else {
	    y = spanInfo->startRow;
	    ey = y - spanInfo->height;
	    if (y1 < y) {
		src += (y - y1) * spanInfo->srcRowIncrement;
		y = y1;
	    }
	    if (y0 > ey) ey = y0;
	    rows = y - ey;
	}
	if (rows <= 0) continue;
	if (spanInfo->endCol > spanInfo->startCol) {
	    x = spanInfo->startCol;
	    ex = spanInfo->endCol;
	    if (x0 > x) {
		src += x0 - x;
		x = x0;
	    }
	    if (x1 < ex) ex = x1;
	    width = ex - x;
	    dst = (GLushort *)(((y - gc->constants.viewportYAdjust) * buf->outerWidth
				- gc->constants.viewportXAdjust + x) * buf->elementSize
			       + (GLubyte *)buf->base);
	    for (k = 0; k < rows; k++) {
		sp = src;
		dp16 = dst;
		for (i = width; i > 0; i--) {
		    GLuint r = *sp++ >> (GLubyte)rShift;
		    GLuint g = *sp++ >> (GLubyte)gShift;
		    GLuint b = *sp >> (GLubyte)bShift;
		    sp += 2;
		    *dp16++ = (GLushort)((b << blueShift) | (g << greenShift) | (r << redShift));
		}
		src += spanInfo->srcRowIncrement;
		dst += outerWidth;
	    }
	} else {
	    x = spanInfo->startCol;
	    ex = spanInfo->endCol;
	    if (x1 < x) {
		src += x - x1;
		x = x1;
	    }
	    if (x0 > ex) ex = x0;
	    width = x - ex;
	    dst = (GLushort *)(((y - gc->constants.viewportYAdjust) * buf->outerWidth
				- gc->constants.viewportXAdjust + x) * buf->elementSize
			       + (GLubyte *)buf->base);
	    for (k = 0; k < rows; k++) {
		sp = src;
		dp16 = dst;
		for (i = width; i > 0; i--) {
		    GLuint r = *sp++ >> (GLubyte)rShift;
		    GLuint g = *sp++ >> (GLubyte)gShift;
		    GLuint b = *sp >> (GLubyte)bShift;
		    sp += 2;
		    *dp16-- = (GLushort)((b << blueShift) | (g << greenShift) | (r << redShift));
		}
		src += spanInfo->srcRowIncrement;
		dst += outerWidth;
	    }
	}
    }
}

/* 6908c0e0 -- __glDrawPixels_FastRGBA16_Atest (3dfx FASTDRAW.C): RGBA / ABGR / BGRA unsigned bytes into a
** 16-bit colour buffer, alpha tested
** against the front buffer's alpha-function table */
void __fastcall __glDrawPixels_FastRGBA16_Atest(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint destRow = spanInfo->startRow;     /* dead (the ancestor's row and height); see the top */
    GLint height = spanInfo->height;
    __WGLdrawable *dp = gc->drawablePrivate;
    __GLcolorBufferICD *cfb = gc->drawBuffer;
    __WGLbuffer *buf = cfb->buf.wglBuf;
    GLint rShift = 8 - gc->modes.redBits;
    GLint gShift = 8 - gc->modes.greenBits;
    GLint bShift = 8 - gc->modes.blueBits;
    GLint redShift = cfb->redShift;
    GLint greenShift = cfb->greenShift;
    GLint blueShift = cfb->blueShift;
    /* the front buffer word at +0x2c, used as a byte table indexed by alpha (the alpha test) */
    GLubyte *alphaTab = gc->frontBuffer.alphaTestFuncTable;
    GLint rIdx, gIdx, bIdx, aIdx;
    GLint outerWidth, numRects, n;
    __WGLregionRect *rect;
    GLint k, x0, x1, y0, y1, y, ey, rows, x, ex, width, i;
    const GLubyte *src, *sp;
    GLushort *dst, *dp16;

    if (spanInfo->srcFormat == GL_RGBA) {
	rIdx = 0; gIdx = 1; bIdx = 2; aIdx = 3;
    } else if (spanInfo->srcFormat == GL_ABGR_EXT) {
	rIdx = 3; gIdx = 2; bIdx = 1; aIdx = 0;
    } else if (spanInfo->srcFormat == GL_BGRA_EXT) {
	rIdx = 2; gIdx = 1; bIdx = 0; aIdx = 3;
    } else {
	abort();
    }
    outerWidth = (spanInfo->zoomy < 0 ? -1 : 1) * buf->outerWidth;
    numRects = dp->numRects;
    rect = dp->rects;

    for (n = 0; n < numRects; n++, rect++) {
	x0 = rect->x0 + gc->constants.viewportXAdjust;
	y0 = rect->y0 + gc->constants.viewportYAdjust;
	x1 = rect->x1 + gc->constants.viewportXAdjust;
	y1 = rect->y1 + gc->constants.viewportYAdjust;
	if (x1 - x0 == 0 || y0 - y1 == 0) continue;
	src = (const GLubyte *)spanInfo->srcCurrent;
	if (spanInfo->zoomy > 0) {
	    y = spanInfo->startRow;
	    ey = spanInfo->height + y;
	    if (y0 > y) {
		src += (y0 - y) * spanInfo->srcRowIncrement;
		y = y0;
	    }
	    if (y1 < ey) ey = y1;
	    rows = ey - y;
	} else {
	    y = spanInfo->startRow;
	    ey = y - spanInfo->height;
	    if (y1 < y) {
		src += (y - y1) * spanInfo->srcRowIncrement;
		y = y1;
	    }
	    if (y0 > ey) ey = y0;
	    rows = y - ey;
	}
	if (rows <= 0) continue;
	if (spanInfo->endCol > spanInfo->startCol) {
	    x = spanInfo->startCol;
	    ex = spanInfo->endCol;
	    if (x0 > x) {
		src += x0 - x;
		x = x0;
	    }
	    if (x1 < ex) ex = x1;
	    width = ex - x;
	    dst = (GLushort *)(((y - gc->constants.viewportYAdjust) * buf->outerWidth
				- gc->constants.viewportXAdjust + x) * buf->elementSize
			       + (GLubyte *)buf->base);
	    for (k = 0; k < rows; k++) {
		sp = src;
		dp16 = dst;
		for (i = width; i > 0; i--) {
		    if (alphaTab[sp[aIdx]]) {
			*dp16 = (GLushort)((GLubyte)(sp[rIdx] >> (GLubyte)rShift) << redShift)
			      | (GLushort)((GLubyte)(sp[gIdx] >> (GLubyte)gShift) << greenShift)
			      | (GLushort)((GLubyte)(sp[bIdx] >> (GLubyte)bShift) << blueShift);
		    }
		    dp16++;
		    sp += 4;
		}
		src += spanInfo->srcRowIncrement;
		dst += outerWidth;
			    }
	} else {
	    x = spanInfo->startCol;
	    ex = spanInfo->endCol;
	    if (x1 < x) {
		src += x - x1;
		x = x1;
	    }
	    if (x0 > ex) ex = x0;
	    width = x - ex;
	    dst = (GLushort *)(((y - gc->constants.viewportYAdjust) * buf->outerWidth
				- gc->constants.viewportXAdjust + x) * buf->elementSize
			       + (GLubyte *)buf->base);
	    for (k = 0; k < rows; k++) {
		sp = src;
		dp16 = dst;
		for (i = width; i > 0; i--) {
		    if (alphaTab[sp[aIdx]]) {
			*dp16 = (GLushort)((GLubyte)(sp[rIdx] >> (GLubyte)rShift) << redShift)
			      | (GLushort)((GLubyte)(sp[gIdx] >> (GLubyte)gShift) << greenShift)
			      | (GLushort)((GLubyte)(sp[bIdx] >> (GLubyte)bShift) << blueShift);
		    }
		    dp16--;
		    sp += 4;
		}
		src += spanInfo->srcRowIncrement;
		dst += outerWidth;
			    }
	}
    }
}

/* 6908c580 -- __glRenderBitmap_FastRGB16 (3dfx FASTDRAW.C): glBitmap straight into a 16-bit colour buffer */
void __fastcall __glRenderBitmap_FastRGB16(__GLcontext *gc, const __GLbitmap *bitmap, const GLubyte *data)
{
    __GLcolorBufferICD *cfb = gc->drawBuffer;
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    GLint ySign = gc->constants.ySign;
    __GLvertexICD *rp = &gc->state.current.rasterPos;
    __GLfragmentICD frag;
    GLushort color;
    GLint width, height, w, d, bit, skip, i;
    GLint mask0;
    GLubyte mask;
    __WGLbuffer *buf;
    GLushort *dst;

    if (!gc->state.current.validRasterPos) {
	return;
    }
    if (gc->renderMode == GL_SELECT) {
	rp->window.x += bitmap->xmove;
	rp->window.y += ySign * bitmap->ymove;
	return;
    }
    if (gc->renderMode == GL_FEEDBACK) {
	__glFeedbackBitmap(gc, rp);
	rp->window.x += bitmap->xmove;
	rp->window.y += ySign * bitmap->ymove;
	return;
    }

    frag.color.r = gc->constants.redRescale * rp->color->r;
    frag.color.g = rp->color->g * gc->constants.greenRescale;
    frag.color.b = rp->color->b * gc->constants.blueRescale;
    frag.color.a = rp->color->a * gc->constants.alphaRescale;
    if (modeFlags & __GL_SHADE_TEXTURE) {
	__GLfloat rq = 1.0f / rp->texture[0].w;
	(*(__GLtextureRasterProc)gc->procs.textureRaster[0])(gc, &frag.color,
		rq * rp->texture[0].x, rq * rp->texture[0].y, 1.0f);
    }
    if (gc->state.enables.general & __GL_FOG_ENABLE) {
	if (gc->state.fog.coordSource == GL_FRAGMENT_DEPTH_EXT) {
	    (*(__GLfogPointProc)gc->procs.fogPoint)(gc, &frag, rp->eye.z);
	} else {
	    (*(__GLfogPointProc)gc->procs.fogPoint)(gc, &frag, rp->fogCoord);
	}
    }

    frag.x = (GLint)(rp->window.x - bitmap->xorig);
    frag.y = (GLint)(rp->window.y - ySign * bitmap->yorig);
    frag.z = (GLuint)(gc->constants.depthRescale * rp->window.z);
    color = (GLushort)((GLint)(frag.color.b + 0.5f) << cfb->blueShift)
	  | (GLushort)((GLint)(frag.color.g + 0.5f) << cfb->greenShift)
	  | (GLushort)((GLint)(frag.color.r + 0.5f) << cfb->redShift);
    rp->window.x += bitmap->xmove;
    rp->window.y += ySign * bitmap->ymove;

    width = bitmap->width;
    height = bitmap->height;
    w = bitmap->width;
    bit = 0;
    if (ySign > 0) {
	if (frag.y < gc->transform.clipY0) {
	    d = gc->transform.clipY0 - frag.y;
	    height -= d;
	    if (height <= 0) return;
	    frag.y += d;
	    data += ((width + 7) >> 3) * d;
	}
	if (gc->transform.clipY1 <= height + frag.y) {
	    height = gc->transform.clipY1 - frag.y;
	    if (height <= 0) return;
	}
    } else {
	if (frag.y - height < gc->transform.clipY0) {
	    height = frag.y - gc->transform.clipY0 + 1;
	    if (height <= 0) return;
	}
	if (gc->transform.clipY1 <= frag.y) {
	    d = frag.y - gc->transform.clipY1 + 1;
	    height -= d;
	    if (height <= 0) return;
	    frag.y -= d;
	    data += ((width + 7) >> 3) * d;
	}
    }
    if (frag.x < gc->transform.clipX0) {
	d = gc->transform.clipX0 - frag.x;
	w -= d;
	if (w <= 0) return;
	frag.x += d;
	data += d >> 3;
	bit = d & 7;
    }
    if (gc->transform.clipX1 <= w + frag.x) {
	w = gc->transform.clipX1 - frag.x;
	if (w <= 0) return;
    }

    mask0 = 0x80 >> bit;
    skip = ((width + 7) >> 3) - ((bit + w + 7) >> 3) + 1;
    (*gc->buffers.lockBuffers)(gc, gc->buffers.renderBufferMask);
    buf = cfb->buf.wglBuf;
    mask = mask0;
    for (; height > 0; height--) {
	dst = (GLushort *)(((frag.y - gc->constants.viewportYAdjust) * buf->outerWidth
			    - gc->constants.viewportXAdjust + frag.x) * buf->elementSize
			   + (GLubyte *)buf->base);
	for (i = w; i > 0; i--) {
	    if (*data & mask) {
		*dst = color;
	    }
	    dst++;
	    mask >>= 1;
	    if (!mask) {
		mask = 0x80;
		data++;
	    }
	}
	frag.y += ySign;
	if (mask == 0x80) {
	    mask = mask0;
	    data += skip - 1;
	} else {
	    mask = mask0;
	    data += skip;
	}
    }
    (*gc->buffers.unlockBuffers)(gc, gc->buffers.renderBufferMask);
}

/* 6908c940 -- __glRenderBitmap_FastCI8 (3dfx FASTDRAW.C): glBitmap straight into an 8-bit colour-index buffer */
void __fastcall __glRenderBitmap_FastCI8(__GLcontext *gc, const __GLbitmap *bitmap, const GLubyte *data)
{
    __GLcolorBufferICD *cfb = gc->drawBuffer;
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    GLint ySign = gc->constants.ySign;
    __GLvertexICD *rp = &gc->state.current.rasterPos;
    __GLfragmentICD frag;
    GLubyte index;
    GLint width, height, w, d, bit, skip, i;
    GLint mask0;
    GLubyte mask;
    __WGLbuffer *buf;
    GLubyte *dst;

    if (!gc->state.current.validRasterPos) {
	return;
    }
    if (gc->renderMode == GL_SELECT) {
	rp->window.x += bitmap->xmove;
	rp->window.y += ySign * bitmap->ymove;
	return;
    }
    if (gc->renderMode == GL_FEEDBACK) {
	__glFeedbackBitmap(gc, rp);
	rp->window.x += bitmap->xmove;
	rp->window.y += ySign * bitmap->ymove;
	return;
    }

    frag.color = *rp->color;
    if (modeFlags & __GL_SHADE_TEXTURE) {
	__GLfloat rq = 1.0f / rp->texture[0].w;
	(*(__GLtextureRasterProc)gc->procs.textureRaster[0])(gc, &frag.color,
		rq * rp->texture[0].x, rq * rp->texture[0].y, 1.0f);
    }
    if (gc->state.enables.general & __GL_FOG_ENABLE) {
	if (gc->state.fog.coordSource == GL_FRAGMENT_DEPTH_EXT) {
	    (*(__GLfogPointProc)gc->procs.fogPoint)(gc, &frag, rp->eye.z);
	} else {
	    (*(__GLfogPointProc)gc->procs.fogPoint)(gc, &frag, rp->fogCoord);
	}
    }

    frag.x = (GLint)(rp->window.x - bitmap->xorig);
    frag.y = (GLint)(rp->window.y - ySign * bitmap->yorig);
    frag.z = (GLuint)(gc->constants.depthRescale * rp->window.z);
    index = (GLubyte)(GLint)frag.color.r;
    rp->window.x += bitmap->xmove;
    rp->window.y += ySign * bitmap->ymove;

    width = bitmap->width;
    height = bitmap->height;
    w = bitmap->width;
    bit = 0;
    if (ySign > 0) {
	if (frag.y < gc->transform.clipY0) {
	    d = gc->transform.clipY0 - frag.y;
	    height -= d;
	    if (height <= 0) return;
	    frag.y += d;
	    data += ((width + 7) >> 3) * d;
	}
	if (gc->transform.clipY1 <= height + frag.y) {
	    height = gc->transform.clipY1 - frag.y;
	    if (height <= 0) return;
	}
    } else {
	if (frag.y - height < gc->transform.clipY0) {
	    height = frag.y - gc->transform.clipY0 + 1;
	    if (height <= 0) return;
	}
	if (gc->transform.clipY1 <= frag.y) {
	    d = frag.y - gc->transform.clipY1 + 1;
	    height -= d;
	    if (height <= 0) return;
	    frag.y -= d;
	    data += ((width + 7) >> 3) * d;
	}
    }
    if (frag.x < gc->transform.clipX0) {
	d = gc->transform.clipX0 - frag.x;
	w -= d;
	if (w <= 0) return;
	frag.x += d;
	data += d >> 3;
	bit = d & 7;
    }
    if (gc->transform.clipX1 <= w + frag.x) {
	w = gc->transform.clipX1 - frag.x;
	if (w <= 0) return;
    }

    mask0 = 0x80 >> bit;
    skip = ((width + 7) >> 3) - ((bit + w + 7) >> 3) + 1;
    (*gc->buffers.lockBuffers)(gc, gc->buffers.renderBufferMask);
    buf = cfb->buf.wglBuf;
    mask = mask0;
    for (; height > 0; height--) {
	dst = (GLubyte *)(((frag.y - gc->constants.viewportYAdjust) * buf->outerWidth
			    - gc->constants.viewportXAdjust + frag.x) * buf->elementSize
			   + (GLubyte *)buf->base);
	for (i = w; i > 0; i--) {
	    if (*data & mask) {
		*dst = index;
	    }
	    dst++;
	    mask >>= 1;
	    if (!mask) {
		mask = 0x80;
		data++;
	    }
	}
	frag.y += ySign;
	if (mask == 0x80) {
	    mask = mask0;
	    data += skip - 1;
	} else {
	    mask = mask0;
	    data += skip;
	}
    }
    (*gc->buffers.unlockBuffers)(gc, gc->buffers.renderBufferMask);
}

#define __GL_CFB_WGL(cfb)   (((__GLcolorBufferICD *)(cfb))->buf.wglBuf)
typedef void (__fastcall *__GLlockBuffersProc)(__GLcontext *gc, GLuint mask);
/* px_api / px_rend DrawPixels with the ICD's 8 / 16 bit fast paths */
#define __GL_FLOAT_BITS(f)      (*(GLint *)&(f))
#define __GL_PIXEL_LOCK_MASK(gc, format, mask) \
    mask = (gc)->buffers.renderBufferMask; \
    if ((format) == GL_DEPTH_COMPONENT) { \
	mask |= 0x20; \
    } else if ((format) == GL_STENCIL_INDEX) { \
	mask = 0x40; \
    }
#define __GL_PIXEL_UNLOCK(gc, format, mask) \
    __GL_PIXEL_LOCK_MASK(gc, format, mask) \
    (*(__GLlockBuffersProc)(gc)->buffers.unlockBuffers)(gc, mask)
#define SHADE_BITMAP_SLOW   0x00600bb4  /* any fragment op */
#define SHADE_BITMAP_0x40   0x00000040
extern void __fastcall __glDrawPixels_FastRGB16(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
extern void __fastcall __glDrawPixels_FastRGB16_Dither(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
extern void __fastcall __glDrawPixels_FastRGBA16(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
extern void __fastcall __glDrawPixels_FastRGBA16_Atest(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
extern void __fastcall __glRenderBitmap_FastRGB16(__GLcontext *gc, const __GLbitmap *bitmap, const GLubyte *data);
extern void __fastcall __glRenderBitmap_FastCI8(__GLcontext *gc, const __GLbitmap *bitmap, const GLubyte *data);

/* 6908b3a0 -- __glDrawPixels_FastCI8 (3dfx FASTDRAW.C): rows of 1-byte pixels copied straight into the draw
** buffer (memcpy when drawing left to right, reversed byte copy for a negative x zoom). */
void __fastcall __glDrawPixels_FastCI8(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    __WGLbuffer *buf;
    GLint y, yinc, startCol, endCol, rows, width, i, j;
    const GLubyte *src;
    GLubyte *dst;

    y = spanInfo->startRow;
    buf = __GL_CFB_WGL(gc->drawBuffer);
    yinc = -1;
    if (spanInfo->zoomy >= __GL_ZERO_F) {
        yinc = 1;
    }
    rows = spanInfo->height;
    endCol = spanInfo->endCol;
    startCol = spanInfo->startCol;
    if (spanInfo->endCol > startCol) {     /* re-read here (the original compares the field) */
        width = endCol - startCol;
        src = (const GLubyte *)spanInfo->srcCurrent;
        for (j = 0; j < rows; j++) {
            dst = (GLubyte *)buf->base
                + ((y - gc->constants.viewportYAdjust) * buf->outerWidth
                   - gc->constants.viewportXAdjust + startCol) * buf->elementSize;
            memcpy(dst, src, width);
            src += spanInfo->srcRowIncrement;
            y += yinc;
        }
    } else {
        width = startCol - endCol;
        src = (const GLubyte *)spanInfo->srcCurrent;
        for (j = 0; j < rows; j++) {
            dst = (GLubyte *)buf->base
                + ((y - gc->constants.viewportYAdjust) * buf->outerWidth
                   - gc->constants.viewportXAdjust + startCol) * buf->elementSize;
            for (i = 0; i < width; i++) {
                *dst-- = src[i];
            }
            src += spanInfo->srcRowIncrement;
            y += yinc;
        }
    }
}

/* 6908cc90 -- __glOptPickDrawPixels (3dfx FASTDRAW.C; procs.drawPixels): a fast path when nothing modifies the
** pixels, else __glSlowPickDrawPixels */
void __fastcall __glOptPickDrawPixels(__GLcontext *gc, GLsizei width, GLsizei height, GLenum format,
			     GLenum type, const GLvoid *pixels, GLboolean dlist)
{
    __GLpixelSpanInfo spanInfo;
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    GLuint mask;

    __GL_PIXEL_LOCK_MASK(gc, format, mask)
    (*(__GLlockBuffersProc)gc->buffers.lockBuffers)(gc, mask);

    if (type == GL_UNSIGNED_BYTE && !(modeFlags & 0x2009bc) &&
	(gc->state.pixel.transferMode.zoomX == __GL_ONE_F ||
	 gc->state.pixel.transferMode.zoomX == -__GL_ONE_F) &&
	(gc->state.pixel.transferMode.zoomY == __GL_ONE_F ||
	 gc->state.pixel.transferMode.zoomY == -__GL_ONE_F) &&
	!gc->state.pixel.transferMode.mapColor &&
	gc->state.pixel.transferMode.bias.r == __GL_ZERO_F &&
	gc->state.pixel.transferMode.bias.g == __GL_ZERO_F &&
	gc->state.pixel.transferMode.bias.b == __GL_ZERO_F &&
	gc->state.pixel.transferMode.bias.a == __GL_ZERO_F &&
	__GL_FLOAT_BITS(gc->state.pixel.transferMode.scale.r) == 0x3f800000 &&
	__GL_FLOAT_BITS(gc->state.pixel.transferMode.scale.g) == 0x3f800000 &&
	__GL_FLOAT_BITS(gc->state.pixel.transferMode.scale.b) == 0x3f800000 &&
	__GL_FLOAT_BITS(gc->state.pixel.transferMode.scale.a) == 0x3f800000 &&
	gc->state.pixel.transferMode.indexOffset == __GL_ZERO_F &&
	gc->state.pixel.transferMode.indexShift == __GL_ZERO_F) {
	__glInitDrawPixelsInfo(gc, &spanInfo, width, height, format, GL_UNSIGNED_BYTE, pixels);
	__glLoadUnpackModes(gc, &spanInfo, dlist);
	if (!__glClipDrawPixels(gc, &spanInfo)) {
	    return;
	}
	__glInitUnpacker(gc, &spanInfo);
	if (gc->modes.rgbMode && __GL_CFB_WGL(gc->drawBuffer)->elementSize == 2) {
	    if (modeFlags & __GL_SHADE_DITHER) {
		if (modeFlags & __GL_SHADE_ALPHA_TEST) {
		    goto slow;
		}
		switch (format) {
		  case GL_RGB:
		    __glDrawPixels_FastRGB16_Dither(gc, &spanInfo);
		    __GL_PIXEL_UNLOCK(gc, format, mask);
		    return;
		  case GL_RGBA:
		    __glDrawPixels_FastRGBA16(gc, &spanInfo);
		    __GL_PIXEL_UNLOCK(gc, format, mask);
		    return;
		  default:
		    goto slow;
		}
	    } else if (modeFlags & __GL_SHADE_ALPHA_TEST) {
		if (format == GL_RGBA || format == GL_BGRA_EXT || format == GL_ABGR_EXT) {
		    __glDrawPixels_FastRGBA16_Atest(gc, &spanInfo);
		    __GL_PIXEL_UNLOCK(gc, format, mask);
		    return;
		}
		goto slow;
	    } else {
		switch (format) {
		  case GL_RGB:
		    __glDrawPixels_FastRGB16(gc, &spanInfo);
		    __GL_PIXEL_UNLOCK(gc, format, mask);
		    return;
		  case GL_RGBA:
		    __glDrawPixels_FastRGBA16(gc, &spanInfo);
		    __GL_PIXEL_UNLOCK(gc, format, mask);
		    return;
		  default:
		    goto slow;
		}
	    }
	} else if (!gc->modes.rgbMode && __GL_CFB_WGL(gc->drawBuffer)->elementSize == 1 &&
		   format == GL_COLOR_INDEX && !(modeFlags & __GL_SHADE_OWNERSHIP_TEST)) {
	    if (modeFlags & __GL_SHADE_DITHER) {
		__glDrawPixels_FastCI8(gc, &spanInfo);
	    } else {
		__glDrawPixels_FastCI8(gc, &spanInfo);
	    }
	    return;
	}
    }
slow:
    __glSlowPickDrawPixels(gc, width, height, format, type, pixels, dlist);
    __GL_PIXEL_UNLOCK(gc, format, mask);
}

/* 6908cfc0 -- __glOptPickRenderBitmapProcs (3dfx FASTDRAW.C): pick glBitmap's renderer, the direct 16-bit RGB
** / 8-bit index paths (gl_pxfast.c)
** when no fragment op is on, else the generic pick */
void __fastcall __glOptPickRenderBitmapProcs(__GLcontext *gc)
{
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);

    gc->procs.renderBitmap = NULL;
    if (!(modeFlags & SHADE_BITMAP_SLOW)) {
	if (gc->modes.rgbMode &&
	    (gc->drawBuffer->buf.wglBuf)->elementSize == 2) {
	    if (!(modeFlags & SHADE_BITMAP_0x40)) {
		gc->procs.renderBitmap = (__GLproc)__glRenderBitmap_FastRGB16;
		return;
	    }
	} else if (!gc->modes.rgbMode &&
		   (gc->drawBuffer->buf.wglBuf)->elementSize == 1) {
	    if (!(modeFlags & SHADE_BITMAP_0x40)) {
		gc->procs.renderBitmap = (__GLproc)__glRenderBitmap_FastCI8;
		return;
	    }
	}
    }
    __glGenericPickRenderBitmapProcs(gc);
}

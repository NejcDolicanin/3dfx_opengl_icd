/*
** gl_lnfast.c -- the s_lnfast.c fast line stores (8-bit colour index and 16-bit RGB, flat, smooth
** and dithered, plain and stippled), written from the SGI Sample Implementation source with the
** ICD's line machine (options inline in gc->line) and WGL colour buffer (buf.other).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "icdglob.h"
#include "si_names.h"
#include "glshade.h"

typedef __GLcolorBufferICD __GLcolorBuffer;
typedef __GLlineMachineICD __GLlineOptions;

/* fmacros.h */
#define FLOAT_MANTISSA_BITS     23
#define FLOAT_MANTISSA_MASK     ((1 << FLOAT_MANTISSA_BITS) - 1)
#define SFloat2Int(_flt_, _int_) \
    (*((float *)(&(_int_)))=(_flt_)+((float)(3<<(FLOAT_MANTISSA_BITS-1))), \
     (_int_)=((_int_)&FLOAT_MANTISSA_MASK)-(1<<(FLOAT_MANTISSA_BITS-1)))

/* buffers.h on the WGL buffer */
#define __GL_FB_ADDRESS(gc,fb,drawableBuf,cast,x,y) \
    (cast (((GLubyte *) (drawableBuf)->base) \
	+ (((y) - (gc)->constants.viewportYAdjust) \
            * (drawableBuf)->outerWidth \
	   + (x) - (gc)->constants.viewportXAdjust) \
	* (drawableBuf)->elementSize))

/* s_global.c __glDitherTableFloat: sixteen floats at 690f5398 */

/* 690d61f0 -- __glStoreLine_CI_8_Flat */
GLboolean __fastcall __glStoreLine_CI_8_Flat(__GLcontext *gc)
{
    GLint xLittle, xBig, yLittle, yBig, dLittle, dBig;
    GLint fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLdrawableBuffer *dBuf;
    GLint len, ii;
    GLubyte *fp;
    GLuint result;

    len = gc->polygon.shader.length;
    cfb = gc->polygon.shader.cfb;
    dBuf = cfb->buf.wglBuf;
    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    dBig = xBig + yBig * dBuf->outerWidth;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    dLittle = xLittle + yLittle * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    cp = gc->polygon.shader.colors;

    fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLubyte *),
			 gc->line.xStart, gc->line.yStart);

    SFloat2Int(cp->r, result);

    for (ii = 0; ii < len; ii++) {

	*fp = result;

	fraction += dfraction;
	if (fraction < 0) {
	    fraction &= ~0x80000000;
	    fp += dBig;
	} else {
	    fp += dLittle;
	}
    }

    return GL_FALSE;
}

/* 690d62c0 -- __glStoreLine_CI_8_Smooth */
GLboolean __fastcall __glStoreLine_CI_8_Smooth(__GLcontext *gc)
{
    GLint dLittle, dBig;
    GLint fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLdrawableBuffer *dBuf;
    GLint len, ii;
    GLubyte *fp;
    GLuint result;

    len = gc->polygon.shader.length;
    cfb = gc->polygon.shader.cfb;
    dBuf = cfb->buf.wglBuf;
    dBig = gc->line.xBig +
	gc->line.yBig * dBuf->outerWidth;
    dLittle = gc->line.xLittle +
	gc->line.yLittle * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    cp = gc->polygon.shader.colors;

    fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLubyte *),
			 gc->line.xStart, gc->line.yStart);

    for (ii = 0; ii < len; ii++, cp++) {

	SFloat2Int(cp->r, result);
	*fp = result;

	fraction += dfraction;
	if (fraction < 0) {
	    fraction &= ~0x80000000;
	    fp += dBig;
	} else {
	    fp += dLittle;
	}
    }

    return GL_FALSE;
}

/* 690d63a0 -- __glStoreLine_CI_8_Flat_Dither */
GLboolean __fastcall __glStoreLine_CI_8_Flat_Dither(__GLcontext *gc)
{
    GLint dLittle, dBig, fraction, dfraction, len;
    GLubyte *fp;
    GLint x, y, xLittle, yLittle, xBig, yBig;
    __GLfloat fresult;

    /* Initialization */
    {
	__GLcolorBuffer *cfb;
	__GLdrawableBuffer *dBuf;
	__GLlineOptions *options;
	GLint outerWidth;

        {
	    __GLshade *shader;
	    shader = &gc->polygon.shader;
	    cfb = shader->cfb;
	    dBuf = cfb->buf.wglBuf;
	    len = shader->length;
	    fresult = shader->colors->r;
	}

	outerWidth = dBuf->outerWidth;
	options = &gc->line;
	xBig = options->xBig;
	yBig = options->yBig;
	dBig = xBig + yBig * outerWidth;
	yBig <<= 2;
	xLittle = options->xLittle;
	yLittle = options->yLittle;
	dLittle = xLittle + yLittle * outerWidth;
	yLittle <<= 2;
	x = options->xStart;
	y = options->yStart << 2;
	fraction = options->fraction;
	dfraction = options->dfraction;
	fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLubyte *), 
			     options->xStart, options->yStart);
    }

    while (--len >= 0) {

	GLuint ix = ((x & 3) | (y & 12));
	__GLfloat ftmp;
	GLuint result;

	ftmp = fresult + __glDitherTableFloat[ix];
	SFloat2Int(ftmp, result);
	*fp = result;

	fraction += dfraction;
	if (fraction < 0) {
	    fraction &= ~0x80000000;
	    x += xBig;
	    y += yBig;
	    fp += dBig;
	} else {
	    x += xLittle;
	    y += yLittle;
	    fp += dLittle;
	}
    }

    return GL_FALSE;
}

/* 690d64d0 -- __glStoreLine_CI_8_Smooth_Dither */
GLboolean __fastcall __glStoreLine_CI_8_Smooth_Dither(__GLcontext *gc)
{
    GLint dLittle, dBig, fraction, dfraction, len;
    GLubyte *fp;
    __GLcolor *cp;
    GLint x, y, xLittle, yLittle, xBig, yBig;

    /* Initialization */
    {
	__GLcolorBuffer *cfb;
	__GLdrawableBuffer *dBuf;
	__GLlineOptions *options;
	GLint outerWidth;

        {
	    __GLshade *shader;
	    shader = &gc->polygon.shader;
	    cfb = shader->cfb;
	    dBuf = cfb->buf.wglBuf;
	    len = shader->length;
	    cp = shader->colors;
	}

	outerWidth = dBuf->outerWidth;
	options = &gc->line;
	xBig = options->xBig;
	yBig = options->yBig;
	dBig = xBig + yBig * outerWidth;
	yBig <<= 2;
	xLittle = options->xLittle;
	yLittle = options->yLittle;
	dLittle = xLittle + yLittle * outerWidth;
	yLittle <<= 2;
	x = options->xStart;
	y = options->yStart << 2;
	fraction = options->fraction;
	dfraction = options->dfraction;
	fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLubyte *), 
			     options->xStart, options->yStart);
    }

    while (--len >= 0) {

	GLuint ix = ((x & 3) | (y & 12));
	__GLfloat fresult;
	GLuint result;

	fresult = cp->r + __glDitherTableFloat[ix];
	SFloat2Int(fresult, result);
	*fp = result;
	cp++;

	fraction += dfraction;
	if (fraction < 0) {
	    fraction &= ~0x80000000;
	    x += xBig;
	    y += yBig;
	    fp += dBig;
	} else {
	    x += xLittle;
	    y += yLittle;
	    fp += dLittle;
	}
    }

    return GL_FALSE;
}

/* 690d6610 -- __glStoreLine_RGB_16_Flat */
GLboolean __fastcall __glStoreLine_RGB_16_Flat(__GLcontext *gc)
{
    GLint xLittle, xBig, yLittle, yBig, dLittle, dBig;
    GLint fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLdrawableBuffer *dBuf;
    GLint len, ii;
    GLushort *fp;
    GLushort result;
    GLuint r, g, b;

    len = gc->polygon.shader.length;
    cfb = gc->polygon.shader.cfb;
    dBuf = cfb->buf.wglBuf;
    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    dBig = xBig + yBig * dBuf->outerWidth;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    dLittle = xLittle + yLittle * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    cp = gc->polygon.shader.colors;

    fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLushort *),
			 gc->line.xStart, gc->line.yStart);

    SFloat2Int(cp->r, r);
    SFloat2Int(cp->g, g);
    SFloat2Int(cp->b, b);
    result =
	(r << cfb->redShift) |
	(g << cfb->greenShift) |
	(b << cfb->blueShift);

    for (ii = 0; ii < len; ii++) {
	*fp = result;
	fraction += dfraction;
	if (fraction < 0) {
	    fraction &= ~0x80000000;
	    fp += dBig;
	} else {
	    fp += dLittle;
	}
    }

    return GL_FALSE;
}

/* 690d6710 -- __glStoreLine_RGB_16_Dither */
GLboolean __fastcall __glStoreLine_RGB_16_Dither(__GLcontext *gc)
{
    GLint x, y, xLittle, xBig, yLittle, yBig, dLittle, dBig;
    GLint fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLdrawableBuffer *dBuf;
    GLint len, ii;
    GLushort *fp;
    GLuint r, g, b;

    len = gc->polygon.shader.length;
    cfb = gc->polygon.shader.cfb;
    dBuf = cfb->buf.wglBuf;
    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    dBig = xBig + yBig * dBuf->outerWidth;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    dLittle = xLittle + yLittle * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    cp = gc->polygon.shader.colors;

    x = gc->line.xStart;
    y = gc->line.yStart;
    fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLushort *), x, y);

    /* These are used only for dithering */
    y <<= 2;
    yBig <<= 2;
    yLittle <<= 2;

    for (ii = 0; ii < len; ii++, cp++) {
	
	GLuint ix = ((x & 3) | (y & 12));
	__GLfloat ftmp, inc;

	inc = __glDitherTableFloat[ix];
	ftmp = cp->r + inc;
	SFloat2Int(ftmp, r);
	ftmp = cp->g + inc;
	SFloat2Int(ftmp, g);
	ftmp = cp->b + inc;
	SFloat2Int(ftmp, b);

	*fp =
	    (r << cfb->redShift) |
	    (g << cfb->greenShift) |
	    (b << cfb->blueShift);

	fraction += dfraction;
	if (fraction < 0) {
	    fraction &= ~0x80000000;
	    x += xBig;
	    y += yBig;
	    fp += dBig;
	} else {
	    x += xLittle;
	    y += yLittle;
	    fp += dLittle;
	}
    }

    return GL_FALSE;
}

/* 690d68b0 -- __glStoreLine_RGB_16_Smooth */
GLboolean __fastcall __glStoreLine_RGB_16_Smooth(__GLcontext *gc)
{
    GLint xLittle, xBig, yLittle, yBig, dLittle, dBig;
    GLint fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLdrawableBuffer *dBuf;
    GLint len, ii;
    GLushort *fp;
    GLint rs, gs, bs;

    len = gc->polygon.shader.length;
    cfb = gc->polygon.shader.cfb;
    dBuf = cfb->buf.wglBuf;
    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    dBig = xBig + yBig * dBuf->outerWidth;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    dLittle = xLittle + yLittle * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    cp = gc->polygon.shader.colors;

    fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLushort *),
			 gc->line.xStart, gc->line.yStart);

    rs = cfb->redShift;
    gs = cfb->greenShift;
    bs = cfb->blueShift;

    for (ii = 0; ii < len; ii++, cp++) {
	unsigned int r, g, b;

	SFloat2Int(cp->r, r);
	SFloat2Int(cp->g, g);
	SFloat2Int(cp->b, b);

	*fp = (r << rs) | (g << gs) | (b << bs);

	fraction += dfraction;
	if (fraction < 0) {
	    fraction &= ~0x80000000;
	    fp += dBig;
	} else {
	    fp += dLittle;
	}
    }

    return GL_FALSE;
}

/* 690d69f0 -- __glStoreStippledLine_CI_8_Flat */
GLboolean __fastcall __glStoreStippledLine_CI_8_Flat(__GLcontext *gc)
{
    GLint xLittle, xBig, yLittle, yBig, dLittle, dBig;
    GLint fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLdrawableBuffer *dBuf;
    __GLstippleWord inMask, bit, *sp;
    GLint len, count;
    GLubyte *fp;
    GLuint result;

    len = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;
    cfb = gc->polygon.shader.cfb;
    dBuf = cfb->buf.wglBuf;
    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    dBig = xBig + yBig * dBuf->outerWidth;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    dLittle = xLittle + yLittle * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    cp = gc->polygon.shader.colors;

    fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLubyte *),
			 gc->line.xStart, gc->line.yStart);

    SFloat2Int(cp->r, result);

    do {
	count = len;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	len -= count;

	inMask = *sp++;
	bit = __GL_STIPPLE_SHIFT(0);

	do {
	    if (inMask & bit) {
		*fp = result;
	    }

#if defined(__GL_STIPPLE_MSB)
	    bit >>= 1;
#else /* __GL_STIPPLE_MSB */
	    bit <<= 1;
#endif /* __GL_STIPPLE_MSB */

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		fp += dBig;
	    } else {
		fp += dLittle;
	    }

	} while (--count);
    } while (len);

    return GL_FALSE;
}

/* 690d6b10 -- __glStoreStippledLine_CI_8_Smooth */
GLboolean __fastcall __glStoreStippledLine_CI_8_Smooth(__GLcontext *gc)
{
    GLint dLittle, dBig;
    GLint fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLdrawableBuffer *dBuf;
    __GLstippleWord inMask, bit, *sp;
    GLint len, count;
    GLubyte *fp;
    GLuint result;

    len = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;
    cfb = gc->polygon.shader.cfb;
    dBuf = cfb->buf.wglBuf;
    dBig = gc->line.xBig +
	gc->line.yBig * dBuf->outerWidth;
    dLittle = gc->line.xLittle +
	gc->line.yLittle * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    cp = gc->polygon.shader.colors;

    fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLubyte *),
			 gc->line.xStart, gc->line.yStart);

    do {
	count = len;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	len -= count;

	inMask = *sp++;
	bit = __GL_STIPPLE_SHIFT(0);

	do {
	    if (inMask & bit) {
		SFloat2Int(cp->r, result);
		*fp = result;
	    }

#if defined(__GL_STIPPLE_MSB)
	    bit >>= 1;
#else /* __GL_STIPPLE_MSB */
	    bit <<= 1;
#endif /* __GL_STIPPLE_MSB */
	    cp++;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		fp += dBig;
	    } else {
		fp += dLittle;
	    }
	} while (--count);
    } while (len);

    return GL_FALSE;
}

/* 690d6c20 -- __glStoreStippledLine_CI_8_Flat_Dither */
GLboolean __fastcall __glStoreStippledLine_CI_8_Flat_Dither(__GLcontext *gc)
{
    GLint dLittle, dBig, fraction, dfraction, len;
    GLubyte *fp;
    GLint x, y, xLittle, yLittle, xBig, yBig, count;
    __GLstippleWord inMask, bit, *sp;
    __GLfloat fresult;

    /* Initialization */
    {
	__GLcolorBuffer *cfb;
	__GLdrawableBuffer *dBuf;
	__GLlineOptions *options;
	GLint outerWidth;

        {
	    __GLshade *shader;
	    shader = &gc->polygon.shader;
	    sp = shader->stipplePat;
	    cfb = shader->cfb;
	    dBuf = cfb->buf.wglBuf;
	    len = shader->length;
	    fresult = shader->colors->r;
	}

	outerWidth = dBuf->outerWidth;
	options = &gc->line;
	xBig = options->xBig;
	yBig = options->yBig;
	dBig = xBig + yBig * outerWidth;
	yBig <<= 2;
	xLittle = options->xLittle;
	yLittle = options->yLittle;
	dLittle = xLittle + yLittle * outerWidth;
	yLittle <<= 2;
	x = options->xStart;
	y = options->yStart << 2;
	fraction = options->fraction;
	dfraction = options->dfraction;
	fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLubyte *), 
			     options->xStart, options->yStart);
    }

    do {
	count = len;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	len -= count;

	inMask = *sp++;
	bit = __GL_STIPPLE_SHIFT(0);
	do {
	    if (inMask & bit) {
		GLuint ix = ((x & 3) | (y & 12));
		__GLfloat ftmp;
		GLuint result;

		ftmp = fresult + __glDitherTableFloat[ix];
		SFloat2Int(ftmp, result);
		*fp = result;
	    }

#if defined(__GL_STIPPLE_MSB)
	    bit >>= 1;
#else /* __GL_STIPPLE_MSB */
	    bit <<= 1;
#endif /* __GL_STIPPLE_MSB */

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		x += xBig;
		y += yBig;
		fp += dBig;
	    } else {
		x += xLittle;
		y += yLittle;
		fp += dLittle;
	    }
	} while (--count);
    } while (len);

    return GL_FALSE;
}

/* 690d6db0 -- __glStoreStippledLine_CI_8_Smooth_Dither */
GLboolean __fastcall __glStoreStippledLine_CI_8_Smooth_Dither(__GLcontext *gc)
{
    GLint dLittle, dBig, fraction, dfraction, len;
    GLubyte *fp;
    __GLcolor *cp;
    GLint x, y, xLittle, yLittle, xBig, yBig, count;
    __GLstippleWord inMask, bit, *sp;

    /* Initialization */
    {
	__GLcolorBuffer *cfb;
	__GLdrawableBuffer *dBuf;
	__GLlineOptions *options;
	GLint outerWidth;

        {
	    __GLshade *shader;
	    shader = &gc->polygon.shader;
	    sp = shader->stipplePat;
	    cfb = shader->cfb;
	    dBuf = cfb->buf.wglBuf;
	    len = shader->length;
	    cp = shader->colors;
	}

	outerWidth = dBuf->outerWidth;
	options = &gc->line;
	xBig = options->xBig;
	yBig = options->yBig;
	dBig = xBig + yBig * outerWidth;
	yBig <<= 2;
	xLittle = options->xLittle;
	yLittle = options->yLittle;
	dLittle = xLittle + yLittle * outerWidth;
	yLittle <<= 2;
	x = options->xStart;
	y = options->yStart << 2;
	fraction = options->fraction;
	dfraction = options->dfraction;
	fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLubyte *), 
			     options->xStart, options->yStart);
    }

    do {
	count = len;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	len -= count;

	inMask = *sp++;
	bit = __GL_STIPPLE_SHIFT(0);
	do {
	    if (inMask & bit) {
		GLuint ix = ((x & 3) | (y & 12));
		__GLfloat fresult;
		GLuint result;

		fresult = cp->r + __glDitherTableFloat[ix];
		SFloat2Int(fresult, result);
		*fp = result;
	    }
#if defined(__GL_STIPPLE_MSB)
	    bit >>= 1;
#else /* __GL_STIPPLE_MSB */
	    bit <<= 1;
#endif /* __GL_STIPPLE_MSB */
	    cp++;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		x += xBig;
		y += yBig;
		fp += dBig;
	    } else {
		x += xLittle;
		y += yLittle;
		fp += dLittle;
	    }
	} while (--count);
    } while (len);

    return GL_FALSE;
}

/* 690d6f40 -- __glStoreStippledLine_RGB_16_Flat */
GLboolean __fastcall __glStoreStippledLine_RGB_16_Flat(__GLcontext *gc)
{
    GLint xLittle, xBig, yLittle, yBig, dLittle, dBig;
    GLint fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLdrawableBuffer *dBuf;
    __GLstippleWord inMask, bit, *sp;
    GLint len, count;
    GLushort *fp;
    GLushort result;
    GLuint r, g, b;

    len = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;
    cfb = gc->polygon.shader.cfb;
    dBuf = cfb->buf.wglBuf;
    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    dBig = xBig + yBig * dBuf->outerWidth;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    dLittle = xLittle + yLittle * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    cp = gc->polygon.shader.colors;

    fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLushort *),
			 gc->line.xStart, gc->line.yStart);

    SFloat2Int(cp->r, r);
    SFloat2Int(cp->g, g);
    SFloat2Int(cp->b, b);
    result =
	(r << cfb->redShift) |
	(g << cfb->greenShift) |
	(b << cfb->blueShift);

    do {
	count = len;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	len -= count;

	inMask = *sp++;
	bit = __GL_STIPPLE_SHIFT(0);
	do {
	    if (inMask & bit) {
		*fp = result;
	    }
#if defined(__GL_STIPPLE_MSB)
	    bit >>= 1;
#else /* __GL_STIPPLE_MSB */
	    bit <<= 1;
#endif /* __GL_STIPPLE_MSB */
	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		fp += dBig;
	    } else {
		fp += dLittle;
	    }
	} while (--count);
    } while (len);

    return GL_FALSE;
}

/* 690d7090 -- __glStoreStippledLine_RGB_16_Dither */
GLboolean __fastcall __glStoreStippledLine_RGB_16_Dither(__GLcontext *gc)
{
    GLint x, y, xLittle, xBig, yLittle, yBig, dLittle, dBig;
    GLint fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLdrawableBuffer *dBuf;
    __GLstippleWord inMask, bit, *sp;
    GLint len, count;
    GLushort *fp;
    GLuint r, g, b;

    len = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;
    cfb = gc->polygon.shader.cfb;
    dBuf = cfb->buf.wglBuf;
    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    dBig = xBig + yBig * dBuf->outerWidth;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    dLittle = xLittle + yLittle * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    cp = gc->polygon.shader.colors;

    x = gc->line.xStart;
    y = gc->line.yStart;
    fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLushort *), x, y);

    /* These are used only for dithering */
    y <<= 2;
    yBig <<= 2;
    yLittle <<= 2;

    do {
	count = len;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	len -= count;

	inMask = *sp++;
	bit = __GL_STIPPLE_SHIFT(0);
	do {
	    if (inMask & bit) {
		GLuint ix = ((x & 3) | (y & 12));
		__GLfloat ftmp, inc;

		inc = __glDitherTableFloat[ix];
		ftmp = cp->r + inc;
		SFloat2Int(ftmp, r);
		ftmp = cp->g + inc;
		SFloat2Int(ftmp, g);
		ftmp = cp->b + inc;
		SFloat2Int(ftmp, b);

		*fp =
		     (r << cfb->redShift) |
		     (g << cfb->greenShift) |
		     (b << cfb->blueShift);
	    }
#if defined(__GL_STIPPLE_MSB)
	    bit >>= 1;
#else /* __GL_STIPPLE_MSB */
	    bit <<= 1;
#endif /* __GL_STIPPLE_MSB */
	    cp++;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		x += xBig;
		y += yBig;
		fp += dBig;
	    } else {
		x += xLittle;
		y += yLittle;
		fp += dLittle;
	    }
	} while (--count);
    } while (len);

    return GL_FALSE;
}

/* 690d7280 -- __glStoreStippledLine_RGB_16_Smooth */
GLboolean __fastcall __glStoreStippledLine_RGB_16_Smooth(__GLcontext *gc)
{
    GLint xLittle, xBig, yLittle, yBig, dLittle, dBig;
    GLint fraction, dfraction;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLdrawableBuffer *dBuf;
    __GLstippleWord inMask, bit, *sp;
    GLint len, count;
    GLushort *fp;
    GLint rs, gs, bs;

    len = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;
    cfb = gc->polygon.shader.cfb;
    dBuf = cfb->buf.wglBuf;
    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    dBig = xBig + yBig * dBuf->outerWidth;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    dLittle = xLittle + yLittle * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    cp = gc->polygon.shader.colors;

    fp = __GL_FB_ADDRESS(gc, cfb, dBuf, (GLushort *),
			 gc->line.xStart, gc->line.yStart);

    rs = cfb->redShift;
    gs = cfb->greenShift;
    bs = cfb->blueShift;

    do {
	count = len;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	len -= count;

	inMask = *sp++;
	bit = __GL_STIPPLE_SHIFT(0);
	do {
	    if (inMask & bit) {
		unsigned int r, g, b;

		SFloat2Int(cp->r, r);
		SFloat2Int(cp->g, g);
		SFloat2Int(cp->b, b);

		*fp = (r << rs) | (g << gs) | (b << bs);
	    }
#if defined(__GL_STIPPLE_MSB)
	    bit >>= 1;
#else /* __GL_STIPPLE_MSB */
	    bit <<= 1;
#endif /* __GL_STIPPLE_MSB */
	    cp++;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		fp += dBig;
	    } else {
		fp += dLittle;
	    }
	} while (--count);
    } while (len);

    return GL_FALSE;
}

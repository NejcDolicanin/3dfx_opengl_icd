/*
** gl_lnspan.c -- s_lnspan.c line span procs the SI port could not take as-is (they reach the depth
** buffer through the WGL drawable buffer), written from the SGI Sample Implementation source.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "icdglob.h"
#include "si_names.h"
#include "icdprotos.h"

/* the line procs return GLboolean (the original tests al) */
typedef GLboolean (__fastcall *__GLlineProc)(__GLcontext *gc);

/* gc->depthBuffer.buf.other is the WGL depth buffer; its depthPrivate holds the z offset (wgldraw.h) */
#define __GL_DEPTH_BUF(db)      ((db)->buf.wglBuf)
#define __GL_DEPTH_DP(db)       ((__WGLdepthPrivate *)__GL_DEPTH_BUF(db)->depthPrivate)
#define __GL_DEPTH_ELEMSIZE(gc) (__GL_DEPTH_BUF(&(gc)->depthBuffer)->elementSize)
/* the stencil span walks by the WGL stencil buffer's element size too */
#define __GL_STENCIL_ELEMSIZE(gc) (((gc)->stencilBuffer.buf.wglBuf)->elementSize)

typedef GLubyte __GLstencilCell;

/* render.h / consts.h: the line machine keeps the SI options inline (gc->line.axis ...) */
#define __GL_SHADE_LINE_STIPPLE 0x8000

/* buffers.h with the ICD's buffers: buf.other is the WGL buffer; a stencil cell is the top byte
** of its (depth/stencil) element */
typedef __GLstencilBufferICD __GLstencilBuffer;
typedef __GLdepthBufferICD __GLdepthBuffer;
typedef __WGLbuffer __GLdrawableBuffer;
typedef __GLfragmentICD __GLfragment;      /* one colour: SI color[__GL_PRIMARY_COLOR] is .color */
typedef __GLcolorBufferICD __GLcolorBuffer;
typedef void (__fastcall *__GLstoreProc)(__GLcolorBuffer *cfb, const __GLfragment *frag);
#define __GL_FB_ADDRESS(gc,fb,drawableBuf,cast,x,y) \
    (cast (((GLubyte *) (drawableBuf)->base) \
	+ (((y) - (gc)->constants.viewportYAdjust) \
            * (drawableBuf)->outerWidth \
	   + (x) - (gc)->constants.viewportXAdjust) \
	* (drawableBuf)->elementSize))
#define __GL_DEPTH_ADDR(a,b,c,d,e,f) __GL_FB_ADDRESS(a,b,c,d,e,f)
#define __GL_STENCIL_ADDR(gc,fb,drawableBuf,cast,x,y) \
    (cast (((GLubyte *) (drawableBuf)->base) \
	+ (((y) - (gc)->constants.viewportYAdjust) \
            * (drawableBuf)->outerWidth \
	   + (x) - (gc)->constants.viewportXAdjust + 1) \
	* (drawableBuf)->elementSize - 1))

/* 690d3ef0 -- __glProcessLine */
GLboolean __fastcall __glProcessLine(__GLcontext *gc)
{
    GLboolean stippling;
    GLint i,n;

    /* the ICD allocates the scanline arrays (__GL_ALLOCATE_SCANLINE_ARRAYS) */
    gc->polygon.shader.cfb = gc->drawBuffer;

    stippling = GL_FALSE;
    n = gc->procs.line.n;

    gc->polygon.shader.done = GL_FALSE;

    /* Step 1:  Perform early line stipple, coloring procs */
    for (i = 0; i < n; i++) {
	if (stippling) {
	    if ((*(__GLlineProc) gc->procs.line.stippledLineFuncs[i])(gc)) {
		/* Line stippled away! */
		return GL_TRUE;
	    }
	} else {
	    if ((*(__GLlineProc) gc->procs.line.lineFuncs[i])(gc)) {
		if (gc->polygon.shader.done) return GL_TRUE;
		stippling = GL_TRUE;
	    }
	}
    }

    if (stippling) {
	return (*(__GLlineProc) gc->procs.line.wideStippledLineRep)(gc);
    } else {
	return (*(__GLlineProc) gc->procs.line.wideLineRep)(gc);
    }
}

/* 690d3f80 -- __glProcessLine3NW */
GLboolean __fastcall __glProcessLine3NW(__GLcontext *gc)
{
    gc->polygon.shader.cfb = gc->drawBuffer;

    gc->polygon.shader.done = GL_FALSE;

    /* Call non-stippled procs... */
    if ((*(__GLlineProc) gc->procs.line.lineFuncs[0])(gc)) {
	if (gc->polygon.shader.done) return GL_TRUE;
	goto stippled1;
    }
    if ((*(__GLlineProc) gc->procs.line.lineFuncs[1])(gc)) {
	if (gc->polygon.shader.done) return GL_TRUE;
	goto stippled2;
    }
    return (*(__GLlineProc) gc->procs.line.lineFuncs[2])(gc);

stippled1:
    if ((*(__GLlineProc) gc->procs.line.stippledLineFuncs[1])(gc)) {
	return GL_TRUE;
    }
stippled2:
    return (*(__GLlineProc) gc->procs.line.stippledLineFuncs[2])(gc);
}

/* 690d3ff0 -- __glWideLineRep */
GLboolean __fastcall __glWideLineRep(__GLcontext *gc)
{
    GLint i, m, n, width;
    GLboolean stippling;

    n = gc->procs.line.n;
    m = gc->procs.line.m;
    
    width = gc->line.width;

    /* Step 2:  Replicate wide line */
    while (--width >= 0) {
	stippling = GL_FALSE;
	for (i = n; i < m; i++) {
	    if (stippling) {
		if ((*(__GLlineProc) gc->procs.line.stippledLineFuncs[i])(gc)) {
		    /* Line stippled away! */
		    goto nextLine;
		}
	    } else {
		if ((*(__GLlineProc) gc->procs.line.lineFuncs[i])(gc)) {
		    if (gc->polygon.shader.done) {
			gc->polygon.shader.done = GL_FALSE;
			goto nextLine;
		    }
		    stippling = GL_TRUE;
		}
	    }
	}
	if (stippling) {
	    (*gc->procs.line.drawStippledLine)(gc);
	} else {
	    (*gc->procs.line.drawLine)(gc);
	}
nextLine:
	if (gc->line.axis == __GL_X_MAJOR) {
	    gc->line.yStart++;
	} else {
	    gc->line.xStart++;
	}
    }

    return GL_FALSE;
}

/* 690d40c0 -- __glWideStippleLineRep */
GLboolean __fastcall __glWideStippleLineRep(__GLcontext *gc)
{
    GLint i, m, n, width;
    GLint stipLen;
    GLint w;
    __GLlineState *ls = &gc->state.line;
    __GLstippleWord stipplePat[__GL_MAX_STIPPLE_WORDS], *fsp, *tsp;

    n = gc->procs.line.n;
    m = gc->procs.line.m;
    w = gc->polygon.shader.length;
    
    width = ls->aliasedWidth;

    /*
    ** XXX - Saving the stipple like this is only really necessary if 
    ** depth or stencil testing.
    */
    stipLen = (w + __GL_STIPPLE_BITS - 1) >> __GL_STIPPLE_COUNT_BITS;

    fsp = gc->polygon.shader.stipplePat;
    tsp = stipplePat;
    for (i = 0; i < stipLen; i++) {
	*tsp++ = *fsp++;
    }

    /* Step 2:  Replicate wide line */
    while (--width >= 0) {
	for (i = n; i < m; i++) {
	    if ((*(__GLlineProc) gc->procs.line.stippledLineFuncs[i])(gc)) {
		/* Line stippled away! */
		goto nextLine;
	    }
	}
	(*gc->procs.line.drawStippledLine)(gc);
nextLine:
	if (width) {
	    tsp = gc->polygon.shader.stipplePat;
	    fsp = stipplePat;
	    for (i = 0; i < stipLen; i++) {
		*tsp++ = *fsp++;
	    }

	    if (gc->line.axis == __GL_X_MAJOR) {
		gc->line.yStart++;
	    } else {
		gc->line.xStart++;
	    }
	}
    }

    return GL_FALSE;
}

/* 690d4190 -- __glDrawBothLine */
GLboolean __fastcall __glDrawBothLine(__GLcontext *gc)
{
    GLint i, j, m, l;
    GLboolean stippling;
    __GLcolor colors[__GL_MAX_MAX_VIEWPORT], *fcp, *tcp;
    GLint w;

    w = gc->polygon.shader.length;
    m = gc->procs.line.m;
    l = gc->procs.line.l;
    
    /*
    ** XXX - Saving colors like this is only really necessary if blending,
    ** logicOping, or masking.  
    */
    fcp = gc->polygon.shader.colors;
    tcp = colors;
    if (gc->modes.rgbMode) {
	for (i = 0; i < w; i++) {
	    *tcp++ = *fcp++;
	}
    } else {
	for (i = 0; i < w; i++) {
	    tcp->r = fcp->r;
	    fcp++;
	    tcp++;
	}
    }

    /* Step 3:  Draw to FRONT_AND_BACK */
    for (j = 0; j < 2; j++) {
	if (j == 0) {
	    gc->polygon.shader.cfb = &gc->frontBuffer;
	} else {
	    gc->polygon.shader.cfb = &gc->backBuffer;
	}
	stippling = GL_FALSE;
	for (i = m; i < l; i++) {
	    if (stippling) {
		if ((*(__GLlineProc) gc->procs.line.stippledLineFuncs[i])(gc)) {
		    /* Line stippled away! */
		    break;
		}
	    } else {
		if ((*(__GLlineProc) gc->procs.line.lineFuncs[i])(gc)) {
		    if (gc->polygon.shader.done) {
			gc->polygon.shader.done = GL_FALSE;
			break;
		    }
		    stippling = GL_TRUE;
		}
	    }
	}

	if (j == 0) {
	    tcp = gc->polygon.shader.colors;
	    fcp = colors;
	    if (gc->modes.rgbMode) {
		for (i = 0; i < w; i++) {
		    *tcp++ = *fcp++;
		}
	    } else {
		for (i = 0; i < w; i++) {
		    tcp->r = fcp->r;
		    fcp++;
		    tcp++;
		}
	    }
	}
    }

    return GL_FALSE;
}

/* 690d42e0 -- __glDrawBothStippledLine */
GLboolean __fastcall __glDrawBothStippledLine(__GLcontext *gc)
{
    GLint i, m, l, j;
    GLint stipLen;
    GLint w;
    __GLstippleWord stipplePat[__GL_MAX_STIPPLE_WORDS], *fsp, *tsp;
    __GLcolor colors[__GL_MAX_MAX_VIEWPORT], *fcp, *tcp;

    l = gc->procs.line.l;
    m = gc->procs.line.m;
    
    w = gc->polygon.shader.length;

    /*
    ** XXX - Saving colors like this is only really necessary if blending,
    ** logicOping, or masking, and not drawing to FRONT_AND_BACK (because
    ** if we are, then that proc will save colors too)
    */
    fcp = gc->polygon.shader.colors;
    tcp = colors;
    if (gc->modes.rgbMode) {
	for (i = 0; i < w; i++) {
	    *tcp++ = *fcp++;
	}
    } else {
	for (i = 0; i < w; i++) {
	    tcp->r = fcp->r;
	    fcp++;
	    tcp++;
	}
    }

    /*
    ** XXX - Saving the stipple like this is only really necessary if 
    ** depth or stencil testing.
    */
    stipLen = (w + __GL_STIPPLE_BITS - 1) >> __GL_STIPPLE_COUNT_BITS;

    fsp = gc->polygon.shader.stipplePat;
    tsp = stipplePat;
    for (i = 0; i < stipLen; i++) {
	*tsp++ = *fsp++;
    }

    /* Step 2:  Replicate wide line */
    for (j = 0; j < 2; j++) {
	if (j == 0) {
	    gc->polygon.shader.cfb = &gc->frontBuffer;
	} else {
	    gc->polygon.shader.cfb = &gc->backBuffer;
	}
	for (i = m; i < l; i++) {
	    if ((*(__GLlineProc) gc->procs.line.stippledLineFuncs[i])(gc)) {
		/* Line stippled away! */
		break;
	    }
	}
	if (j == 0) {
	    tcp = gc->polygon.shader.colors;
	    fcp = colors;
	    if (gc->modes.rgbMode) {
		for (i = 0; i < w; i++) {
		    *tcp++ = *fcp++;
		}
	    } else {
		for (i = 0; i < w; i++) {
		    tcp->r = fcp->r;
		    fcp++;
		    tcp++;
		}
	    }

	    tsp = gc->polygon.shader.stipplePat;
	    fsp = stipplePat;
	    for (i = 0; i < stipLen; i++) {
		*tsp++ = *fsp++;
	    }
	}
    }

    return GL_FALSE;
}

/* 690d4440 -- __glScissorLine */
GLboolean __fastcall __glScissorLine(__GLcontext *gc)
{
    GLint clipX0, clipX1;
    GLint clipY0, clipY1;
    GLint xStart, yStart, xEnd, yEnd;
    GLint xLittle, yLittle;
    GLint xBig, yBig;
    GLint fraction, dfraction;
    GLint highWord, lowWord;
    GLint bigs, littles;
    GLint failed, count;
    GLint w;
    __GLstippleWord bit, outMask, *osp;

    w = gc->polygon.shader.length;
    
    clipX0 = gc->transform.clipX0;
    clipX1 = gc->transform.clipX1;
    clipY0 = gc->transform.clipY0;
    clipY1 = gc->transform.clipY1;

    xBig = gc->line.xBig;
    yBig = gc->line.yBig;

    xStart = gc->line.xStart;
    yStart = gc->line.yStart;

    /* If the start point is in the scissor region, we attempt to trivially
    ** accept the line.
    */
    if (xStart >= clipX0 && xStart < clipX1 &&
	    yStart >= clipY0 && yStart < clipY1) {

	w--;	/* Makes our math simpler */
	/* Trivial accept attempt */
	xEnd = xStart + xBig * w;
	yEnd = yStart + yBig * w;
	if (xEnd >= clipX0 && xEnd < clipX1 && 
		yEnd >= clipY0 && yEnd < clipY1) {
	    return GL_FALSE;
	}

	xLittle = gc->line.xLittle;
	yLittle = gc->line.yLittle;
	fraction = gc->line.fraction;
	dfraction = gc->line.dfraction;

	/* Invert negative minor slopes so we can assume dfraction > 0 */
	if (dfraction < 0) {
	    dfraction = -dfraction;
	    fraction = 0x7fffffff - fraction;
	}

	/* Now we compute number of littles and bigs in this line */

	/* We perform a 16 by 32 bit multiply.  Ugh. */
	highWord = (((GLuint) dfraction) >> 16) * w + 
		(((GLuint) fraction) >> 16);
	lowWord = (dfraction & 0xffff) * w + (fraction & 0xffff);
	highWord += (((GLuint) lowWord) >> 16);
	bigs = ((GLuint) highWord) >> 15;
	littles = w - bigs;

	/* Second trivial accept attempt */
	xEnd = xStart + xBig*bigs + xLittle*littles;
	yEnd = yStart + yBig*bigs + yLittle*littles;
	if (xEnd >= clipX0 && xEnd < clipX1 && 
		yEnd >= clipY0 && yEnd < clipY1) {
	    return GL_FALSE;
	}
	w++;	/* Restore w */
    } else {
	xLittle = gc->line.xLittle;
	yLittle = gc->line.yLittle;
	fraction = gc->line.fraction;
	dfraction = gc->line.dfraction;
    }

    /*
    ** Note that we don't bother to try trivially rejecting this line.  After
    ** all, it has already been clipped, and the only way that it might
    ** possibly be trivially rejected is if it is a piece of a wide line that
    ** runs right along the edge of the window.
    */

    /*
    ** This sucks.  The line needs to be scissored.
    ** Well, it should only happen rarely, so we can afford
    ** to make it slow.  We achieve this by tediously stippling the line.
    ** (rather than clipping it, of course, which would be faster but harder).
    */
    failed = 0;
    osp = gc->polygon.shader.stipplePat;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (xStart < clipX0 || xStart >= clipX1 ||
		    yStart < clipY0 || yStart >= clipY1) {
		outMask &= ~bit;
		failed++;
	    }

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		xStart += xBig;
		yStart += yBig;
	    } else {
		xStart += xLittle;
		yStart += yLittle;
	    }

	    bit >>= 1;
	}

	*osp++ = outMask;
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_TRUE;
    }

    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

/* 690d46c0 -- __glScissorStippledLine */
GLboolean __fastcall __glScissorStippledLine(__GLcontext *gc)
{
    GLint clipX0, clipX1;
    GLint clipY0, clipY1;
    GLint xStart, yStart, xEnd, yEnd;
    GLint xLittle, yLittle;
    GLint xBig, yBig;
    GLint fraction, dfraction;
    GLint highWord, lowWord;
    GLint bigs, littles;
    GLint failed, count;
    GLint w;
    __GLstippleWord *sp;
    __GLstippleWord bit, outMask, inMask;

    w = gc->polygon.shader.length;

    clipX0 = gc->transform.clipX0;
    clipX1 = gc->transform.clipX1;
    clipY0 = gc->transform.clipY0;
    clipY1 = gc->transform.clipY1;

    xBig = gc->line.xBig;
    yBig = gc->line.yBig;

    xStart = gc->line.xStart;
    yStart = gc->line.yStart;

    /* If the start point is in the scissor region, we attempt to trivially
    ** accept the line.
    */
    if (xStart >= clipX0 && xStart < clipX1 &&
	    yStart >= clipY0 && yStart < clipY1) {

	w--;	/* Makes our math simpler */
	/* Trivial accept attempt */
	xEnd = xStart + xBig * w;
	yEnd = yStart + yBig * w;
	if (xEnd >= clipX0 && xEnd < clipX1 && 
		yEnd >= clipY0 && yEnd < clipY1) {
	    return GL_FALSE;
	}

	xLittle = gc->line.xLittle;
	yLittle = gc->line.yLittle;
	fraction = gc->line.fraction;
	dfraction = gc->line.dfraction;

	/* Invert negative minor slopes so we can assume dfraction > 0 */
	if (dfraction < 0) {
	    dfraction = -dfraction;
	    fraction = 0x7fffffff - fraction;
	}

	/* Now we compute number of littles and bigs in this line */

	/* We perform a 16 by 32 bit multiply.  Ugh. */
	highWord = (((GLuint) dfraction) >> 16) * w + 
		(((GLuint) fraction) >> 16);
	lowWord = (dfraction & 0xffff) * w + (fraction & 0xffff);
	highWord += (((GLuint) lowWord) >> 16);
	bigs = ((GLuint) highWord) >> 15;
	littles = w - bigs;

	/* Second trivial accept attempt */
	xEnd = xStart + xBig*bigs + xLittle*littles;
	yEnd = yStart + yBig*bigs + yLittle*littles;
	if (xEnd >= clipX0 && xEnd < clipX1 && 
		yEnd >= clipY0 && yEnd < clipY1) {
	    return GL_FALSE;
	}
	w++;	/* Restore w */
    } else {
	xLittle = gc->line.xLittle;
	yLittle = gc->line.yLittle;
	fraction = gc->line.fraction;
	dfraction = gc->line.dfraction;
    }

    /*
    ** Note that we don't bother to try trivially rejecting this line.  After
    ** all, it has already been clipped, and the only way that it might
    ** possibly be trivially rejected is if it is a piece of a wide line that
    ** runs right along the edge of the window.
    */

    /*
    ** This sucks.  The line needs to be scissored.
    ** Well, it should only happen rarely, so we can afford
    ** to make it slow.  We achieve this by tediously stippling the line.
    ** (rather than clipping it, of course, which would be faster but harder).
    */
    sp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	inMask = *sp;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		if (xStart < clipX0 || xStart >= clipX1 ||
			yStart < clipY0 || yStart >= clipY1) {
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		xStart += xBig;
		yStart += yBig;
	    } else {
		xStart += xLittle;
		yStart += yLittle;
	    }

	    bit >>= 1;
	}

	*sp++ = outMask & inMask;
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_FALSE;
    }

    return GL_TRUE;
}

/* 690d4a50 -- __glStencilTestLine */
GLboolean __fastcall __glStencilTestLine(__GLcontext *gc)
{
    __GLstencilBuffer *sb = &gc->stencilBuffer;
    __GLdrawableBuffer *dBuf = sb->buf.wglBuf;
    __GLstencilCell *tft, *sfb, *fail, cell;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dspLittle, dspBig;
    GLint count, failed;
    __GLstippleWord bit, outMask, *osp;
    GLuint smask;
    GLint w;

    w = gc->polygon.shader.length;
    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    sfb = __GL_STENCIL_ADDR(gc, sb, dBuf, (__GLstencilCell *),
			    gc->line.xStart, gc->line.yStart);
    dspLittle = xLittle + yLittle * dBuf->outerWidth;
    dspBig = xBig + yBig * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    tft = gc->stencilBuffer.testFuncTable;
    fail = gc->stencilBuffer.failOpTable;
    smask = gc->state.stencil.mask;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    cell = sfb[0];
	    if (!tft[cell & smask]) {
		/* Test failed */
		outMask &= ~bit;
		sfb[0] = fail[cell];
		failed++;
	    }

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		sfb += dspBig;
	    } else {
		sfb += dspLittle;
	    }
	    bit >>= 1;
	}
	*osp++ = outMask;
    }

    if (failed == 0) {
	return GL_FALSE;
    } else {
	if (failed != gc->polygon.shader.length) {
	    /* Call next proc */
	    return GL_TRUE;
	}
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

/* 690d4c00 -- __glStencilTestStippledLine */
GLboolean __fastcall __glStencilTestStippledLine(__GLcontext *gc)
{
    __GLstencilBuffer *sb = &gc->stencilBuffer;
    __GLdrawableBuffer *dBuf = sb->buf.wglBuf;
    __GLstencilCell *tft, *sfb, *fail, cell;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint count, failed;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLint dspLittle, dspBig;
    GLuint smask;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;
    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    sfb = __GL_STENCIL_ADDR(gc, sb, dBuf, (__GLstencilCell *),
			    gc->line.xStart, gc->line.yStart);
    dspLittle = xLittle + yLittle * dBuf->outerWidth;
    dspBig = xBig + yBig * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    tft = gc->stencilBuffer.testFuncTable;
    fail = gc->stencilBuffer.failOpTable;
    smask = gc->state.stencil.mask;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	inMask = *sp;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		cell = sfb[0];
		if (!tft[cell & smask]) {
		    /* Test failed */
		    outMask &= ~bit;
		    sfb[0] = fail[cell];
		    failed++;
		}
	    } else failed++;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		sfb += dspBig;
	    } else {
		sfb += dspLittle;
	    }
	    bit >>= 1;
	}
	*sp++ = outMask & inMask;
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_FALSE;
    }
    return GL_TRUE;
}

/* 690d4da0 -- __glDepthTestLine */
GLboolean __fastcall __glDepthTestLine(__GLcontext *gc)
{
    __GLdepthBuffer *db = &gc->depthBuffer;
    __GLdrawableBuffer *dBuf = db->buf.wglBuf;
    __GLzValue zFrac, z, *zfb;
    GLint dzdx, dzdxBig;
    GLuint cnt;
    GLint numFracBits;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dzpLittle, dzpBig;
    GLint failed, count;
    __GLdepthTestPixel testFunc;
    __GLstippleWord bit, outMask, *osp;
    GLint w;

    w = gc->polygon.shader.length;

    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    zfb = __GL_DEPTH_ADDR(gc, db, dBuf, (__GLzValue *),
			  gc->line.xStart, gc->line.yStart);
    dzpLittle = (xLittle + yLittle * dBuf->outerWidth) * dBuf->elementSize;
    dzpBig = (xBig + yBig * dBuf->outerWidth) * dBuf->elementSize;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;

    numFracBits = __GL_DEPTH_DP(&gc->depthBuffer)->numFracBits;
    cnt = __GL_DEPTH_DP(&gc->depthBuffer)->cnt;
    testFunc = (__GLdepthTestPixel)gc->procs.DTPixel;
    zFrac = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx >> numFracBits;
    dzdxBig = gc->polygon.shader.dzdxBig;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	z = (zFrac >> numFracBits) + cnt;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if( (*testFunc)(z,zfb) == GL_FALSE ) {
		outMask &= ~bit;
		failed++;
	    }
	    z += dzdx;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		zfb = (__GLzValue *) ((GLubyte *) zfb + dzpBig);
	    } else {
		zfb = (__GLzValue *) ((GLubyte *) zfb + dzpLittle);
	    }
	    bit >>= 1;
	}
	*osp++ = outMask;
	zFrac += dzdxBig;
    }

    if (failed == 0) {
	/* Call next span proc */
	return GL_FALSE;
    } else {
	if (failed != gc->polygon.shader.length) {
	    /* Call next stippled span proc */
	    return GL_TRUE;
	}
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

/* 690d4fa0 -- __glDepthTestStippledLine */
GLboolean __fastcall __glDepthTestStippledLine(__GLcontext *gc)
{
    __GLdepthBuffer *db = &gc->depthBuffer;
    __GLdrawableBuffer *dBuf = db->buf.wglBuf;
    __GLzValue zFrac, z, *zfb;
    GLint dzdx, dzdxBig;
    GLuint cnt;
    GLint numFracBits;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dzpLittle, dzpBig;
    GLint failed, count;
    __GLdepthTestPixel testFunc;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;
    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    zfb = __GL_DEPTH_ADDR(gc, db, dBuf, (__GLzValue *),
			  gc->line.xStart, gc->line.yStart);
    dzpLittle = (xLittle + yLittle * dBuf->outerWidth) * dBuf->elementSize;
    dzpBig = (xBig + yBig * dBuf->outerWidth) * dBuf->elementSize;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;

    numFracBits = __GL_DEPTH_DP(db)->numFracBits;
    cnt = __GL_DEPTH_DP(db)->cnt;
    testFunc = (__GLdepthTestPixel)gc->procs.DTPixel;
    zFrac = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx >> numFracBits;
    dzdxBig = gc->polygon.shader.dzdxBig;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	z = (zFrac >> numFracBits) + cnt;
	inMask = *sp;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		if( (*testFunc)(z,zfb) == GL_FALSE ) {
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;
	    z += dzdx;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		zfb = (__GLzValue *) ((GLubyte *) zfb + dzpBig);
	    } else {
		zfb = (__GLzValue *) ((GLubyte *) zfb + dzpLittle);
	    }
	    bit >>= 1;
	}
	*sp++ = outMask & inMask;
	zFrac += dzdxBig;
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_FALSE;
    }
    return GL_TRUE;
}

/* 690d5190 -- __glDepthTestStencilLine */
GLboolean __fastcall __glDepthTestStencilLine(__GLcontext *gc)
{
    __GLstencilBuffer *sb = &gc->stencilBuffer;
    __GLdrawableBuffer *sBuf = sb->buf.wglBuf;
    __GLdepthBuffer *db = &gc->depthBuffer;
    __GLdrawableBuffer *dBuf = db->buf.wglBuf;
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dzpLittle, dzpBig;
    GLint dspLittle, dspBig;
    __GLzValue zFrac, z, *zfb;
    GLint dzdx, dzdxBig;
    GLuint cnt;
    GLint numFracBits;
    GLint failed, count;
    __GLdepthTestPixel testFunc;
    __GLstippleWord bit, outMask, *osp;
    GLint w;

    w = gc->polygon.shader.length;

    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    zfb = __GL_DEPTH_ADDR(gc, db, dBuf, (__GLzValue *),
			  gc->line.xStart, gc->line.yStart);
    dzpLittle = (xLittle + yLittle * dBuf->outerWidth) * dBuf->elementSize;
    dzpBig = (xBig + yBig * dBuf->outerWidth) * dBuf->elementSize;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;

    sfb = __GL_STENCIL_ADDR(gc, sb, sBuf, (__GLstencilCell *),
			    gc->line.xStart, gc->line.yStart);
    dspLittle = xLittle + yLittle * sBuf->outerWidth;
    dspBig = xBig + yBig * sBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;

    zFailOp = gc->stencilBuffer.depthFailOpTable;
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    numFracBits = __GL_DEPTH_DP(&gc->depthBuffer)->numFracBits;
    cnt = __GL_DEPTH_DP(&gc->depthBuffer)->cnt;
    testFunc = (__GLdepthTestPixel)gc->procs.DTPixel;
    zFrac = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx >> numFracBits;
    dzdxBig = gc->polygon.shader.dzdxBig;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	z = (zFrac >> numFracBits) + cnt;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if( (*testFunc)(z,zfb) ) {
		sfb[0] = zPassOp[sfb[0]];
	    } else {
		sfb[0] = zFailOp[sfb[0]];
		outMask &= ~bit;
		failed++;
	    }
	    z += dzdx;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		zfb = (__GLzValue *) ((GLubyte *) zfb + dzpBig);
		sfb += dspBig;
	    } else {
		zfb = (__GLzValue *) ((GLubyte *) zfb + dzpLittle);
		sfb += dspLittle;
	    }
	    bit >>= 1;
	}
	*osp++ = outMask;
	zFrac += dzdxBig;
    }

    if (failed == 0) {
	/* Call next span proc */
	return GL_FALSE;
    } else {
	if (failed != gc->polygon.shader.length) {
	    /* Call next stippled span proc */
	    return GL_TRUE;
	}
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

/* 690d5440 -- __glDepthTestStencilStippledLine */
GLboolean __fastcall __glDepthTestStencilStippledLine(__GLcontext *gc)
{
    __GLstencilBuffer *sb = &gc->stencilBuffer;
    __GLdrawableBuffer *sBuf = sb->buf.wglBuf;
    __GLdepthBuffer *db = &gc->depthBuffer;
    __GLdrawableBuffer *dBuf = db->buf.wglBuf;
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dzpLittle, dzpBig;
    GLint dspLittle, dspBig;
    __GLzValue zFrac, z, *zfb;
    GLint dzdx, dzdxBig;
    GLuint cnt;
    GLint numFracBits;
    GLint failed, count;
    GLboolean (*testFunc)( __GLzValue, __GLzValue * ) ;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    zfb = __GL_DEPTH_ADDR(gc, db, dBuf, (__GLzValue *),
			  gc->line.xStart, gc->line.yStart);
    dzpLittle = (xLittle + yLittle * dBuf->outerWidth) * dBuf->elementSize;
    dzpBig = (xBig + yBig * dBuf->outerWidth) * dBuf->elementSize;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;

    sfb = __GL_STENCIL_ADDR(gc, sb, sBuf, (__GLstencilCell *),
			    gc->line.xStart, gc->line.yStart);
    dspLittle = xLittle + yLittle * sBuf->outerWidth;
    dspBig = xBig + yBig * sBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;

    numFracBits = __GL_DEPTH_DP(db)->numFracBits;
    cnt = __GL_DEPTH_DP(db)->cnt;
    testFunc = (__GLdepthTestPixel)gc->procs.DTPixel;
    zFailOp = gc->stencilBuffer.depthFailOpTable;
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    zFrac = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx >> numFracBits;
    dzdxBig = gc->polygon.shader.dzdxBig;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	z = (zFrac >> numFracBits) + cnt;
	inMask = *sp;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		if( (*testFunc)(z,zfb) ) {
		    sfb[0] = zPassOp[sfb[0]];
		} else {
		    sfb[0] = zFailOp[sfb[0]];
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;
	    z += dzdx;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		zfb = (__GLzValue *) ((GLubyte *) zfb + dzpBig);
		sfb += dspBig;
	    } else {
		zfb = (__GLzValue *) ((GLubyte *) zfb + dzpLittle);
		sfb += dspLittle;
	    }
	    bit >>= 1;
	}
	*sp++ = outMask & inMask;
	zFrac += dzdxBig;
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_FALSE;
    }

    return GL_TRUE;
}

/* 690d56e0 -- __glDepthPassLine */
GLboolean __fastcall __glDepthPassLine(__GLcontext *gc)
{
    __GLstencilBuffer *sb = &gc->stencilBuffer;
    __GLdrawableBuffer *dBuf = sb->buf.wglBuf;
    __GLstencilCell *sfb, *zPassOp;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dspLittle, dspBig;
    GLint w;

    w = gc->polygon.shader.length;

    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    sfb = __GL_STENCIL_ADDR(gc, sb, dBuf, (__GLstencilCell *),
			    gc->line.xStart, gc->line.yStart);
    dspLittle = xLittle + yLittle * dBuf->outerWidth;
    dspBig = xBig + yBig * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;

    zPassOp = gc->stencilBuffer.depthPassOpTable;
    while (--w >= 0) {
	sfb[0] = zPassOp[sfb[0]];
	fraction += dfraction;
	if (fraction < 0) {
	    fraction &= ~0x80000000;
	    sfb += dspBig;
	} else {
	    sfb += dspLittle;
	}
    }

    return GL_FALSE;
}

/* 690d5790 -- __glDepthPassStippledLine */
GLboolean __fastcall __glDepthPassStippledLine(__GLcontext *gc)
{
    __GLstencilBuffer *db = &gc->stencilBuffer;
    __GLdrawableBuffer *dBuf = db->buf.wglBuf;
    __GLstencilCell *sfb, *zPassOp;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dspLittle, dspBig;
    __GLstippleWord bit, inMask, *sp;
    GLint count;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    sfb = __GL_STENCIL_ADDR(gc, db, dBuf, (__GLstencilCell *),
			    gc->line.xStart, gc->line.yStart);
    dspLittle = xLittle + yLittle * dBuf->outerWidth;
    dspBig = xBig + yBig * dBuf->outerWidth;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    zPassOp = gc->stencilBuffer.depthPassOpTable;
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
		sfb[0] = zPassOp[sfb[0]];
	    }
	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		sfb += dspBig;
	    } else {
		sfb += dspLittle;
	    }
	    bit >>= 1;
	}
    }

    return GL_FALSE;
}

/* 690d58a0 -- __glStoreLine */
GLboolean __fastcall __glStoreLine(__GLcontext *gc)
{
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    __GLfragment frag;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLstoreProc store;
    GLint len;

    len = gc->polygon.shader.length;

    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;
    store = (__GLstoreProc)cfb->store;
    frag.x = gc->line.xStart;
    frag.y = gc->line.yStart;

    while (--len >= 0) {
	frag.color = *cp++;
	(*store)(cfb, &frag);

	fraction += dfraction;
	if (fraction < 0) {
	    fraction &= ~0x80000000;
	    frag.x += xBig;
	    frag.y += yBig;
	} else {
	    frag.x += xLittle;
	    frag.y += yLittle;
	}
    }

    return GL_FALSE;
}

/* 690d59a0 -- __glStoreStippledLine */
GLboolean __fastcall __glStoreStippledLine(__GLcontext *gc)
{
    GLint x, y, xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    __GLfragment frag;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLstippleWord inMask, bit, *sp;
    GLint count;
    __GLstoreProc store;
    GLint len;

    len = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;
    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;
    store = (__GLstoreProc)cfb->store;
    x = gc->line.xStart;
    y = gc->line.yStart;

    while (len) {
	count = len;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	len -= count;

	inMask = *sp++;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		frag.x = x;
		frag.y = y;
		frag.color = *cp;
		(*store)(cfb, &frag);
	    }

	    cp++;
	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		x += xBig;
		y += yBig;
	    } else {
		x += xLittle;
		y += yLittle;
	    }
	    bit >>= 1;
	}
    }

    return GL_FALSE;
}

/* 690d5af0 -- __glAntiAliasLine */
GLboolean __fastcall __glAntiAliasLine(__GLcontext *gc)
{
    __GLfloat length;	/* Dist along length */
    __GLfloat width;	/* Dist along width */
    GLint fraction, dfraction;
    __GLfloat dlLittle, dlBig;
    __GLfloat ddLittle, ddBig;
    __GLcolor *cp;
    __GLfloat coverage;
    __GLfloat lineWidth;
    __GLfloat lineLength;
    GLint failed, count;
    __GLstippleWord bit, outMask, *osp;
    GLint w;
    GLuint modeFlags = ((GLuint (__fastcall *)(__GLcontext *)) gc->polygon.shader.getModeFlags)(gc);

    w = gc->polygon.shader.length;

    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;

    cp = gc->polygon.shader.colors;

    dlLittle = gc->line.dlLittle;
    dlBig = gc->line.dlBig;
    ddLittle = gc->line.ddLittle;
    ddBig = gc->line.ddBig;

    length = gc->line.plength;
    width = gc->line.pwidth;
    lineLength = gc->line.realLength - __glHalf;
    lineWidth = __glHalf * gc->state.line.smoothWidth - __glHalf;

    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);

	while (--count >= 0) {
	    /* Coverage for sides */
	    if (width > lineWidth) {
		coverage = lineWidth - width + __GL_ONE_F;
		if (coverage < __GL_ZERO_F) {
		    coverage = __GL_ZERO_F;
		    goto coverageZero;
		}
	    } else if (width < -lineWidth) {
		coverage = width + lineWidth + __GL_ONE_F;
		if (coverage < __GL_ZERO_F) {
		    coverage = __GL_ZERO_F;
		    goto coverageZero;
		}
	    } else {
		coverage = __GL_ONE_F;
	    }

	    /* Coverage for start, end */
	    if (length < __glHalf) {
		coverage *= length + __glHalf;
		if (coverage < __GL_ZERO_F) {
		    coverage = __GL_ZERO_F;
		    goto coverageZero;
		}
	    } else if (length > lineLength) {
		coverage *= lineLength - length + __GL_ONE_F;
		if (coverage < __GL_ZERO_F) {
		    coverage = __GL_ZERO_F;
		    goto coverageZero;
		}
	    } 

	    /* Coverage for internal stipples */
	    if ( modeFlags & __GL_SHADE_LINE_STIPPLE ) {
		__GLfloat stippleOffset;
		GLint lowStip, highStip;
		GLint lowBit, highBit;
		GLint lowVal, highVal;
		__GLfloat percent;

		/* Minor correction */
		if (length > __glHalf) {
		    stippleOffset = gc->line.stippleOffset + length;
		} else {
		    stippleOffset = gc->line.stippleOffset + __glHalf;
		}
		lowStip = (GLint)__GL_FLOORF(stippleOffset);
		highStip = lowStip + 1;

		/* percent is the percent of highStip that will be used */
		percent = stippleOffset - lowStip;

		lowBit = (GLint) (lowStip * 
			gc->line.oneOverStippleRepeat) & 0xf;
		highBit = (GLint) (highStip * 
			gc->line.oneOverStippleRepeat) & 0xf;

		if (gc->state.line.stipple & (1<<lowBit)) {
		    lowVal = 1;
		} else {
		    lowVal = 0;
		}

		if (gc->state.line.stipple & (1<<highBit)) {
		    highVal = 1;
		} else {
		    highVal = 0;
		}

		coverage *= lowVal * (__GL_ONE_F - percent) +
			highVal * percent;
	    }

	    if (coverage == __GL_ZERO_F) {
coverageZero:;
		outMask &= ~bit;
		failed++;
	    } else {
		if (gc->modes.colorIndexMode) {
		    cp->r = __glBuildAntiAliasIndex(cp->r, coverage);
		} else {
		    cp->a *= coverage;
		}
	    }
	    cp++;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		length += dlBig;
		width += ddBig;
	    } else {
		length += dlLittle;
		width += ddLittle;
	    }

	    bit >>= 1;
	}
	*osp++ = outMask;
    }

    if (failed == 0) {
	/* Call next span proc */
	return GL_FALSE;
    } else {
	if (failed != gc->polygon.shader.length) {
	    /* Call next stippled span proc */
	    return GL_TRUE;
	}
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

/* 690d5e70 -- __glAntiAliasStippledLine */
GLboolean __fastcall __glAntiAliasStippledLine(__GLcontext *gc)
{
    __GLfloat length;	/* Dist along length */
    __GLfloat width;	/* Dist along width */
    GLint fraction, dfraction;
    __GLfloat dlLittle, dlBig;
    __GLfloat ddLittle, ddBig;
    __GLcolor *cp;
    __GLfloat coverage;
    __GLfloat lineWidth;
    __GLfloat lineLength;
    GLint failed, count;
    __GLstippleWord bit, outMask, inMask, *sp;
    GLint w;
    GLuint modeFlags = ((GLuint (__fastcall *)(__GLcontext *)) gc->polygon.shader.getModeFlags)(gc);

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;

    cp = gc->polygon.shader.colors;

    dlLittle = gc->line.dlLittle;
    dlBig = gc->line.dlBig;
    ddLittle = gc->line.ddLittle;
    ddBig = gc->line.ddBig;

    length = gc->line.plength;
    width = gc->line.pwidth;
    lineLength = gc->line.realLength - __glHalf;
    lineWidth = __glHalf * gc->state.line.smoothWidth - __glHalf;

    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	inMask = *sp;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		/* Coverage for sides */
		if (width > lineWidth) {
		    coverage = lineWidth - width + __GL_ONE_F;
		    if (coverage < __GL_ZERO_F) {
			coverage = __GL_ZERO_F;
			goto coverageZero;
		    }
		} else if (width < -lineWidth) {
		    coverage = width + lineWidth + __GL_ONE_F;
		    if (coverage < __GL_ZERO_F) {
			coverage = __GL_ZERO_F;
			goto coverageZero;
		    }
		} else {
		    coverage = __GL_ONE_F;
		}

		/* Coverage for start, end */
		if (length < __glHalf) {
		    coverage *= length + __glHalf;
		    if (coverage < __GL_ZERO_F) {
			coverage = __GL_ZERO_F;
			goto coverageZero;
		    }
		} else if (length > lineLength) {
		    coverage *= lineLength - length + __GL_ONE_F;
		    if (coverage < __GL_ZERO_F) {
			coverage = __GL_ZERO_F;
			goto coverageZero;
		    }
		} 

		/* Coverage for internal stipples */
		if (modeFlags & __GL_SHADE_LINE_STIPPLE) {
		    __GLfloat stippleOffset;
		    GLint lowStip, highStip;
		    GLint lowBit, highBit;
		    GLint lowVal, highVal;
		    __GLfloat percent;

		    /* Minor correction */
		    if (length > __glHalf) {
			stippleOffset = gc->line.stippleOffset + length;
		    } else {
			stippleOffset = gc->line.stippleOffset + __glHalf;
		    }
		    lowStip = (GLint)__GL_FLOORF(stippleOffset);
		    highStip = lowStip + 1;

		    /* percent is the percent of highStip that will be used */
		    percent = stippleOffset - lowStip;

		    lowBit = (GLint) (lowStip * 
			    gc->line.oneOverStippleRepeat) & 0xf;
		    highBit = (GLint) (highStip * 
			    gc->line.oneOverStippleRepeat) & 0xf;

		    if (gc->state.line.stipple & (1<<lowBit)) {
			lowVal = 1;
		    } else {
			lowVal = 0;
		    }

		    if (gc->state.line.stipple & (1<<highBit)) {
			highVal = 1;
		    } else {
			highVal = 0;
		    }

		    coverage *= lowVal * (__GL_ONE_F - percent) +
			    highVal * percent;
		}

		if (coverage == __GL_ZERO_F) {
coverageZero:;
		    outMask &= ~bit;
		    failed++;
		} else {
		    if (gc->modes.colorIndexMode) {
			cp->r = __glBuildAntiAliasIndex(cp->r, coverage);
		    } else {
			cp->a *= coverage;
		    }
		}
	    } else failed++;
	    cp++;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		length += dlBig;
		width += ddBig;
	    } else {
		length += dlLittle;
		width += ddLittle;
	    }

	    bit >>= 1;
	}
	*sp++ = outMask & inMask;
    }

    if (failed == gc->polygon.shader.length) {
	return GL_TRUE;
    }
    return GL_FALSE;
}

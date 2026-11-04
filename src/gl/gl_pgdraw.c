/*
** gl_pgdraw.c -- the s_pgdraw.c triangle filler (edge snapping, polygon offset), written from
** the SGI Sample Implementation source with the ICD's shader: modeFlags is read through
** polygon.shader.getModeFlags at every use and the buffers are the WGL ones (buf.other).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "glshade.h"


/* 690d7400 -- __glSnapXLeft */
void __fastcall __glSnapXLeftProc(__GLcontext *gc, GLuint edx_unused, __GLfloat xLeft, __GLfloat dxdyLeft)
{
    __GLfloat little, dx;
    GLint ixLeft, ixLeftFrac, frac, lineBytes, elementSize, ilittle, ibig;

    ixLeft = (GLint) xLeft;
    dx = xLeft - ixLeft;
    __FRACTION(ixLeftFrac,dx);

    gc->polygon.shader.ixLeft = ixLeft + (((GLuint) ixLeftFrac) >> 31);
    gc->polygon.shader.ixLeftFrac = ixLeftFrac & ~0x80000000;

    /* Compute big and little steps */
    ilittle = (GLint) dxdyLeft;
    little = (__GLfloat) ilittle;
    if (dxdyLeft < 0) {
	ibig = ilittle - 1;
	dx = little - dxdyLeft;
	__FRACTION(frac,dx);
	gc->polygon.shader.dxLeftFrac = -frac;
    } else {
	ibig = ilittle + 1;
	dx = dxdyLeft - little;
	__FRACTION(frac,dx);
	gc->polygon.shader.dxLeftFrac = frac;
    }
    if (__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_STENCIL_TEST) {
	/*
	** Compute the big and little stencil buffer steps.  We walk the
	** memory pointers for the stencil buffer along the edge of the
	** triangle as we walk the edge.  This way we don't have to
	** recompute the buffer address as we go.
	*/
	elementSize = __GL_BUF_WGL(gc->stencilBuffer)->elementSize;
	lineBytes = elementSize * __GL_BUF_WGL(gc->stencilBuffer)->outerWidth;
	gc->polygon.shader.sbufLittle = lineBytes + ilittle * elementSize;
	gc->polygon.shader.sbufBig = lineBytes + ibig * elementSize;
    }
    if (__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_DEPTH_TEST) {
	/*
	** Compute the big and little depth buffer steps.  We walk the
	** memory pointers for the depth buffer along the edge of the
	** triangle as we walk the edge.  This way we don't have to
	** recompute the buffer address as we go.
	*/
	elementSize = __GL_BUF_WGL(gc->depthBuffer)->elementSize;
	lineBytes = elementSize * __GL_BUF_WGL(gc->depthBuffer)->outerWidth;
	gc->polygon.shader.zbufLittle = lineBytes + ilittle * elementSize;
	gc->polygon.shader.zbufBig = lineBytes + ibig * elementSize;
    }
    gc->polygon.shader.dxLeftLittle = ilittle;
    gc->polygon.shader.dxLeftBig = ibig;
}

/* 690d7510 -- __glSnapXRight */
void __fastcall __glSnapXRightProc(__GLshade *sh, GLuint edx_unused, __GLfloat xRight, __GLfloat dxdyRight)
{
    __GLfloat little, big, dx;
    GLint ixRight, ixRightFrac, frac;

    ixRight = (GLint) xRight;
    dx = xRight - ixRight;
    __FRACTION(ixRightFrac,dx);

    sh->ixRight = ixRight + (((GLuint) ixRightFrac) >> 31);
    sh->ixRightFrac = ixRightFrac & ~0x80000000;

    /* Compute big and little steps */
    little = (__GLfloat) ((GLint) dxdyRight);
    if (dxdyRight < 0) {
	big = little - 1;
	dx = little - dxdyRight;
	__FRACTION(frac,dx);
	sh->dxRightFrac = -frac;
    } else {
	big = little + 1;
	dx = dxdyRight - little;
	__FRACTION(frac,dx);
	sh->dxRightFrac = frac;
    }
    sh->dxRightLittle = (GLint) little;
    sh->dxRightBig = (GLint) big;
}

/* 690d75c0 -- __glPolygonOffsetZ */
__GLzValue __fastcall __glPolygonOffsetZ(__GLcontext *gc, const __GLvertexICD *a,
			      __GLfloat dx, __GLfloat dy)
{

    __GLshade *sh = &gc->polygon.shader;
    __GLfloat factor;
    __GLfloat maxdZ;
    __GLfloat bias;
    __GLfloat newZ;

    /*
    ** Calculate factor and bias
    */
    factor = gc->state.polygon.factor;
    bias = gc->state.polygon.units * gc->depthBuffer.minResolution;

    /*
    ** find the maximum Z slope with respect to X and Y
    */
    /* the ICD compares and keeps the slopes as floats */
    if((__GLfloat)__GL_ABSF(sh->dzdxf) > (__GLfloat)__GL_ABSF(sh->dzdyf))
        maxdZ = (__GLfloat)__GL_ABSF(sh->dzdxf);
    else
        maxdZ = (__GLfloat)__GL_ABSF(sh->dzdyf);

    newZ = (a->window.z + dx*sh->dzdxf + dy*sh->dzdyf) +
           factor * maxdZ + 
	   bias;

    /*
    ** Order the limits that might be crossed
    */
    if((GLint)sh->dzdx < 0) {
      sh->startZLimit = gc->depthBuffer.scale;
      sh->endZLimit = 0;
    } else {
      sh->startZLimit = 0;
      sh->endZLimit = gc->depthBuffer.scale;
    }

    sh->offsetZ = newZ;

    if(sh->dzdxf > 0.0) { /* bound is max z value */
      sh->outBoundCount = (gc->depthBuffer.scale - newZ)/sh->dzdxf;
      sh->rangeCount = (GLdouble)gc->depthBuffer.scale/sh->dzdxf;
    } else if(sh->dzdxf < 0.0) { /* bound is min z value */
      sh->outBoundCount = -newZ/sh->dzdxf;
      sh->rangeCount = gc->depthBuffer.scale/-sh->dzdxf;
    } else { /* dzdxf == 0; use a near-zero number, (no dz for entire span) */
      sh->outBoundCount = (gc->depthBuffer.scale - newZ)/
	                  __GL_PGON_OFFSET_NEAR_ZERO;
      sh->rangeCount = gc->depthBuffer.scale/__GL_PGON_OFFSET_NEAR_ZERO;
    }

    return((__GLzValue)newZ);
}

/* 690d85a0 -- FillSubTriangle: one colour and one texture coordinate set (texIter holds the
** little steps in [0..3] and the big steps in [4..7]); the scanline arrays are not on the stack */
void __fastcall FillSubTriangle(__GLcontext *gc, GLint iyBottom, GLint iyTop)
{
    GLint ixLeft, ixRight;
    GLint ixLeftFrac, ixRightFrac;
    GLint dxLeftFrac, dxRightFrac;
    GLint dxLeftLittle, dxRightLittle;
    GLint dxLeftBig, dxRightBig;
    GLint spanWidth, clipY0, clipY1;
    GLuint modeFlags;

    ixLeft = gc->polygon.shader.ixLeft;
    ixLeftFrac = gc->polygon.shader.ixLeftFrac;
    ixRight = gc->polygon.shader.ixRight;
    ixRightFrac = gc->polygon.shader.ixRightFrac;
    clipY0 = gc->transform.clipY0;
    clipY1 = gc->transform.clipY1;
    dxLeftFrac = gc->polygon.shader.dxLeftFrac;
    dxLeftBig = gc->polygon.shader.dxLeftBig;
    dxLeftLittle = gc->polygon.shader.dxLeftLittle;
    dxRightFrac = gc->polygon.shader.dxRightFrac;
    dxRightBig = gc->polygon.shader.dxRightBig;
    dxRightLittle = gc->polygon.shader.dxRightLittle;
    modeFlags = __GL_SHADE_MODEFLAGS(gc);

    if (modeFlags & __GL_SHADE_STENCIL_TEST) {
	__GLdrawableBuffer *dBuf = __GL_BUF_WGL(gc->stencilBuffer);
	gc->polygon.shader.sbuf =
	    __GL_STENCIL_ADDR(gc, &gc->stencilBuffer, dBuf, 
			      (__GLstencilCell*), ixLeft, iyBottom);
    }
    if (modeFlags & __GL_SHADE_DEPTH_TEST) {
	__GLdrawableBuffer *dBuf = __GL_BUF_WGL(gc->depthBuffer);
	gc->polygon.shader.zbuf =
	    __GL_DEPTH_ADDR(gc, &gc->depthBuffer, dBuf, 
			    (__GLzValue*), ixLeft, iyBottom);
    }
    gc->polygon.shader.cfb = gc->drawBuffer;
    while (iyBottom < iyTop) {
	spanWidth = ixRight - ixLeft;
	/*
	** Only render spans that have non-zero width and which are
	** not scissored out vertically.
	*/
	if ((spanWidth > 0) && (iyBottom >= clipY0) && (iyBottom < clipY1)) {
	    gc->polygon.shader.frag.x = ixLeft;
	    gc->polygon.shader.frag.y = iyBottom;
	    gc->polygon.shader.length = spanWidth;
	    (*gc->procs.span.processSpan)(gc);
	}

	/* Advance right edge fixed point, adjusting for carry */
	ixRightFrac += dxRightFrac;
	if (ixRightFrac < 0) {
	    /* Carry/Borrow'd. Use large step */
	    ixRight += dxRightBig;
	    ixRightFrac &= ~0x80000000;
	} else {
	    ixRight += dxRightLittle;
	}

	iyBottom++;
	ixLeftFrac += dxLeftFrac;
	if (ixLeftFrac < 0) {
	    /* Carry/Borrow'd.  Use large step */
	    ixLeft += dxLeftBig;
	    ixLeftFrac &= ~0x80000000;

	    if (modeFlags & __GL_SHADE_RGB) {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    gc->polygon.shader.frag.color.r += gc->polygon.shader.colorIter.rBig;
		    gc->polygon.shader.frag.color.g += gc->polygon.shader.colorIter.gBig;
		    gc->polygon.shader.frag.color.b += gc->polygon.shader.colorIter.bBig;
		    gc->polygon.shader.frag.color.a += gc->polygon.shader.colorIter.aBig;
		}
	    } else {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    gc->polygon.shader.frag.color.r += gc->polygon.shader.colorIter.rBig;
		}
	    }
	    if (modeFlags & __GL_SHADE_TEXTURE) {
		gc->polygon.shader.frag.texture[0] += gc->polygon.shader.texIter[4];
		gc->polygon.shader.frag.texture[1] += gc->polygon.shader.texIter[5];
		gc->polygon.shader.frag.texture[2] += gc->polygon.shader.texIter[6];
		gc->polygon.shader.frag.texture[3] += gc->polygon.shader.texIter[7];
	    }
	    if (modeFlags & __GL_SHADE_STENCIL_TEST) {
		/* The implicit multiply is taken out of the loop */
		gc->polygon.shader.sbuf = (__GLstencilCell*)
		    ((GLubyte*) gc->polygon.shader.sbuf
		     + gc->polygon.shader.sbufBig);
	    }
	    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
		gc->polygon.shader.frag.z += gc->polygon.shader.zBig;
	    }

	    if (modeFlags & __GL_SHADE_POLYGON_OFFSET_FILL) {
	      gc->polygon.shader.offsetZ += gc->polygon.shader.zBig;
		gc->polygon.shader.outBoundCount += 
		  gc->polygon.shader.outCountBig;
	    }

	    if (modeFlags & __GL_SHADE_DEPTH_TEST) {
		/* The implicit multiply is taken out of the loop */
		gc->polygon.shader.zbuf = (__GLzValue*)
		    ((GLubyte*) gc->polygon.shader.zbuf
		     + gc->polygon.shader.zbufBig);
	    }
	    if (modeFlags & __GL_SHADE_SLOW_FOG) {
		gc->polygon.shader.frag.f += gc->polygon.shader.fBig;
	    }
	} else {
	    /* Use small step */
	    ixLeft += dxLeftLittle;
	    if (modeFlags & __GL_SHADE_RGB) {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    gc->polygon.shader.frag.color.r += gc->polygon.shader.colorIter.rLittle;
		    gc->polygon.shader.frag.color.g += gc->polygon.shader.colorIter.gLittle;
		    gc->polygon.shader.frag.color.b += gc->polygon.shader.colorIter.bLittle;
		    gc->polygon.shader.frag.color.a += gc->polygon.shader.colorIter.aLittle;
		}
	    } else {
		if (modeFlags & __GL_SHADE_SMOOTH) {
		    gc->polygon.shader.frag.color.r += gc->polygon.shader.colorIter.rLittle;
		}
	    }
	    if (modeFlags & __GL_SHADE_TEXTURE) {
		gc->polygon.shader.frag.texture[0] += gc->polygon.shader.texIter[0];
		gc->polygon.shader.frag.texture[1] += gc->polygon.shader.texIter[1];
		gc->polygon.shader.frag.texture[2] += gc->polygon.shader.texIter[2];
		gc->polygon.shader.frag.texture[3] += gc->polygon.shader.texIter[3];
	    }
	    if (modeFlags & __GL_SHADE_STENCIL_TEST) {
		/* The implicit multiply is taken out of the loop */
		gc->polygon.shader.sbuf = (__GLstencilCell*)
		    ((GLubyte*) gc->polygon.shader.sbuf
		     + gc->polygon.shader.sbufLittle);
	    }
	    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
		gc->polygon.shader.frag.z += gc->polygon.shader.zLittle;
	    }

	    if (modeFlags & __GL_SHADE_POLYGON_OFFSET_FILL) {
	        gc->polygon.shader.offsetZ += gc->polygon.shader.zLittle;
		gc->polygon.shader.outBoundCount += 
		  gc->polygon.shader.outCountLittle;
	    }
	    if (modeFlags & __GL_SHADE_DEPTH_TEST) {
		/* The implicit multiply is taken out of the loop */
		gc->polygon.shader.zbuf = (__GLzValue*)
		    ((GLubyte*) gc->polygon.shader.zbuf
		     + gc->polygon.shader.zbufLittle);
	    }
	    if (modeFlags & __GL_SHADE_SLOW_FOG) {
		gc->polygon.shader.frag.f += gc->polygon.shader.fLittle;
	    }
	}
    }
    gc->polygon.shader.ixLeft = ixLeft;
    gc->polygon.shader.ixLeftFrac = ixLeftFrac;
    gc->polygon.shader.ixRight = ixRight;
    gc->polygon.shader.ixRightFrac = ixRightFrac;
}

/* 690d89d0 -- SetInitialParameters: the ICD passes the vertex colour in, keeps one colour iterator
** and one texture set {s, t, qw, rhow} (texIter: little 0..3, big 4..7, d/dx 8..11, d/dy 12..15) */
void __fastcall SetInitialParameters(__GLcontext *gc, const __GLvertexICD *a,
			     const __GLcolor *vertexColor,
			     __GLfloat aFog, __GLfloat aRhow,
			     __GLfloat dx, __GLfloat dy)
{
    __GLshade *sh = &gc->polygon.shader;
    __GLfloat little = sh->dxLeftLittle;
    __GLfloat big = sh->dxLeftBig;
    __GLcolor *fragColor;
    __GLcolorIteratorICD *iter;
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);

    if (big > little) {
	if (modeFlags & __GL_SHADE_RGB) {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		fragColor = &sh->frag.color;
		iter = &sh->colorIter;

		fragColor->r = vertexColor->r + dx*iter->drdx +
		    dy*iter->drdy;
		iter->rLittle = iter->drdy + little * iter->drdx;
		iter->rBig = iter->rLittle + iter->drdx;

		fragColor->g = vertexColor->g + dx*iter->dgdx +
		    dy*iter->dgdy;
		iter->gLittle = iter->dgdy + little * iter->dgdx;
		iter->gBig = iter->gLittle + iter->dgdx;

		fragColor->b = vertexColor->b + dx*iter->dbdx +
		    dy*iter->dbdy;
		iter->bLittle = iter->dbdy + little * iter->dbdx;
		iter->bBig = iter->bLittle + iter->dbdx;

		fragColor->a = vertexColor->a + dx*iter->dadx +
		    dy*iter->dady;
		iter->aLittle = iter->dady + little * iter->dadx;
		iter->aBig = iter->aLittle + iter->dadx;
	    }
	} else {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		fragColor = &sh->frag.color;
		iter = &sh->colorIter;

		fragColor->r = vertexColor->r + dx*iter->drdx +
		    dy*iter->drdy;
		iter->rLittle = iter->drdy + little * iter->drdx;
		iter->rBig = iter->rLittle + iter->drdx;
	    }
	}
	if (modeFlags & __GL_SHADE_TEXTURE) {
	    __GLfloat s = a->texture[0].x;
	    __GLfloat t = a->texture[0].y;
	    __GLfloat q = a->texture[0].w;

	    if ((modeFlags & __GL_SHADE_PERSP_TEXTURE) &&
		!(modeFlags & __GL_SHADE_PROJSCALED_TEXTURE))
	    {
		__GLfloat w = a->window.w;

		s *= w;
		t *= w;
		q *= w;
	    }

	    /* s */
	    sh->frag.texture[0] =
		    s + dx*sh->texIter[8] + dy*sh->texIter[12];
	    sh->texIter[0] =
		    sh->texIter[12] + little * sh->texIter[8];
	    sh->texIter[4] =
		    sh->texIter[0] + sh->texIter[8];

	    /* t */
	    sh->frag.texture[1] =
		    t + dx*sh->texIter[9] + dy*sh->texIter[13];
	    sh->texIter[1] =
		    sh->texIter[13] + little * sh->texIter[9];
	    sh->texIter[5] =
		    sh->texIter[1] + sh->texIter[9];

	    /* qw */
	    sh->frag.texture[2] =
		    q + dx*sh->texIter[10] + dy*sh->texIter[14];
	    sh->texIter[2] =
		    sh->texIter[14] + little * sh->texIter[10];
	    sh->texIter[6] =
		    sh->texIter[2] + sh->texIter[10];

	    /* rhow */
	    sh->frag.texture[3] =
		    aRhow + dx*sh->texIter[11] + dy*sh->texIter[15];
	    sh->texIter[3] =
		    sh->texIter[15] + little * sh->texIter[11];
	    sh->texIter[7] =
		    sh->texIter[3] + sh->texIter[11];
	}
	if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	    __GLfloat zLittle;

	    zLittle = sh->dzdyf + little * sh->dzdxf;
	    sh->zLittle = (GLint)zLittle;
	    sh->zBig = (GLint)(zLittle + sh->dzdxf);

	    if(gc->state.enables.general & __GL_POLYGON_OFFSET_FILL_ENABLE) {
	        sh->frag.z = __glPolygonOffsetZ(gc, a, dx, dy);
		if(sh->dzdxf == 0.) { /* approximate dzdxf == 0. */
		    sh->outCountBig = -sh->zBig/__GL_PGON_OFFSET_NEAR_ZERO;
		    sh->outCountLittle = -sh->zLittle/__GL_PGON_OFFSET_NEAR_ZERO;
		} else {
		    sh->outCountBig = -sh->zBig/sh->dzdxf;
		    sh->outCountLittle = -sh->zLittle/sh->dzdxf;
		}
	    } else {
	        sh->frag.z = (__GLzValue)
		    (a->window.z + dx*sh->dzdxf + dy*sh->dzdyf);
	    }
	}
	if (modeFlags & __GL_SHADE_SLOW_FOG) {
	    sh->frag.f = aFog + dx*sh->dfdx + dy*sh->dfdy;
	    sh->fLittle = sh->dfdy + little * sh->dfdx;
	    sh->fBig = sh->fLittle + sh->dfdx;
	}
    } else {	
	if (modeFlags & __GL_SHADE_RGB) {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		fragColor = &sh->frag.color;
		iter = &sh->colorIter;

		fragColor->r = vertexColor->r + dx*iter->drdx +
		    dy*iter->drdy;
		iter->rLittle = iter->drdy + little * iter->drdx;
		iter->rBig = iter->rLittle - iter->drdx;

		fragColor->g = vertexColor->g + dx*iter->dgdx +
		    dy*iter->dgdy;
		iter->gLittle = iter->dgdy + little * iter->dgdx;
		iter->gBig = iter->gLittle - iter->dgdx;

		fragColor->b = vertexColor->b + dx*iter->dbdx +
		    dy*iter->dbdy;
		iter->bLittle = iter->dbdy + little * iter->dbdx;
		iter->bBig = iter->bLittle - iter->dbdx;

		fragColor->a = vertexColor->a + dx*iter->dadx +
		    dy*iter->dady;
		iter->aLittle = iter->dady + little * iter->dadx;
		iter->aBig = iter->aLittle - iter->dadx;
	    }
	} else {
	    if (modeFlags & __GL_SHADE_SMOOTH) {
		fragColor = &sh->frag.color;
		iter = &sh->colorIter;

		fragColor->r = vertexColor->r + dx*iter->drdx +
		    dy*iter->drdy;
		iter->rLittle = iter->drdy + little * iter->drdx;
		iter->rBig = iter->rLittle - iter->drdx;
	    }
	}
	if (modeFlags & __GL_SHADE_TEXTURE) {
	    __GLfloat s = a->texture[0].x;
	    __GLfloat t = a->texture[0].y;
	    __GLfloat q = a->texture[0].w;

	    if ((modeFlags & __GL_SHADE_PERSP_TEXTURE) &&
		!(modeFlags & __GL_SHADE_PROJSCALED_TEXTURE))
	    {
		__GLfloat w = a->window.w;

		s *= w;
		t *= w;
		q *= w;
	    }

	    /* s */
	    sh->frag.texture[0] =
		    s + dx*sh->texIter[8] + dy*sh->texIter[12];
	    sh->texIter[0] =
		    sh->texIter[12] + little * sh->texIter[8];
	    sh->texIter[4] =
		    sh->texIter[0] - sh->texIter[8];

	    /* t */
	    sh->frag.texture[1] =
		    t + dx*sh->texIter[9] + dy*sh->texIter[13];
	    sh->texIter[1] =
		    sh->texIter[13] + little * sh->texIter[9];
	    sh->texIter[5] =
		    sh->texIter[1] - sh->texIter[9];

	    /* qw */
	    sh->frag.texture[2] =
		    q + dx*sh->texIter[10] + dy*sh->texIter[14];
	    sh->texIter[2] =
		    sh->texIter[14] + little * sh->texIter[10];
	    sh->texIter[6] =
		    sh->texIter[2] - sh->texIter[10];

	    /* rhow */
	    sh->frag.texture[3] =
		    aRhow + dx*sh->texIter[11] + dy*sh->texIter[15];
	    sh->texIter[3] =
		    sh->texIter[15] + little * sh->texIter[11];
	    sh->texIter[7] =
		    sh->texIter[3] - sh->texIter[11];
	}
	if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	    __GLfloat zLittle;

	    zLittle = sh->dzdyf + little * sh->dzdxf;
	    sh->zLittle = (GLint)zLittle;
	    sh->zBig = (GLint)(zLittle - sh->dzdxf);

	    if(gc->state.enables.general & __GL_POLYGON_OFFSET_FILL_ENABLE) {
	        sh->frag.z = __glPolygonOffsetZ(gc, a, dx, dy);
		if(sh->dzdxf == 0.) { /* approximate dzdxf == 0. */
		    sh->outCountBig = -sh->zBig/__GL_PGON_OFFSET_NEAR_ZERO;
		    sh->outCountLittle = -sh->zLittle/__GL_PGON_OFFSET_NEAR_ZERO;
		} else {
		    sh->outCountBig = -sh->zBig/sh->dzdxf;
		    sh->outCountLittle = -sh->zLittle/sh->dzdxf;
		}
	    } else {
	        sh->frag.z = (__GLzValue)
		    (a->window.z + dx*sh->dzdxf + dy*sh->dzdyf);
	    }
	}
	if (modeFlags & __GL_SHADE_SLOW_FOG) {
	    sh->frag.f = aFog + dx*sh->dfdx + dy*sh->dfdy;
	    sh->fLittle = sh->dfdy + little * sh->dfdx;
	    sh->fBig = sh->fLittle - sh->dfdx;
	}
    }
}

/* 690d7750 -- __glFillTriangle */
void __fastcall __glFillTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b,
			     __GLvertexICD *c)
{
    __GLvertexICD *oa = a, *ob = b, *oc = c;
    __GLfloat ccw, area, oneOverArea, t1, t2, t3, t4;
    __GLfloat dxAC, dxBC, dyAC, dyBC;
    __GLfloat aFog, bFog;
    __GLfloat aRhow, bRhow;
    __GLfloat dxAB, dyAB;
    __GLfloat dx, dy, dxdyLeft, dxdyRight;
    __GLcolor *ac, *bc;
    GLint aIY, bIY, cIY;
    GLuint modeFlags;
    __GL_TRIANGLE_VERT_SAVES;

    __GL_ADJUST_TRIANGLE_VERTS(gc, oa, ob, oc, gc->vertex.provoking);

    /*
    ** Sort vertices in y.
    */
    if (*(int *)&a->window.y < *(int *)&b->window.y) {
        if (*(int *)&b->window.y < *(int *)&c->window.y) {
            /* Already sorted */
        } else {
            if (*(int *)&a->window.y < *(int *)&c->window.y) {
                __GLvertexICD *temp=b; b=c; c=temp;
            } else {
                __GLvertexICD *temp=a; a=c; c=b; b=temp;
            }
        }
    } else {
        if (*(int *)&b->window.y < *(int *)&c->window.y) {
            if (*(int *)&a->window.y < *(int *)&c->window.y) {
                __GLvertexICD *temp=a; a=b; b=temp;
            } else {
                __GLvertexICD *temp=a; a=b; b=c; c=temp;
            }
        } else {
            __GLvertexICD *temp=a; a=c; c=temp;
        }
    }

    /* Compute signed area of the triangle */
    dxAC = a->window.x - c->window.x;
    dxBC = b->window.x - c->window.x;
    dyAC = a->window.y - c->window.y;
    dyBC = b->window.y - c->window.y;
    area = dxAC * dyBC - dxBC * dyAC;
    ccw = !(*(int *)&area >> 31);

    /* Pre-compute one over polygon area */
    oneOverArea = __GL_ONE_F / area;

    /* Fetch some stuff we are going to reuse */
    modeFlags = __GL_SHADE_MODEFLAGS(gc);
    ac = a->color;
    bc = b->color;

    /*
    ** Compute delta values for unit changes in x or y for each
    ** parameter.
    */
    t1 = dyAC * oneOverArea;
    t2 = dyBC * oneOverArea;
    t3 = dxAC * oneOverArea;
    t4 = dxBC * oneOverArea;
    if (modeFlags & __GL_SHADE_RGB) {
	if (modeFlags & __GL_SHADE_SMOOTH) {
	    __GLfloat drAC, dgAC, dbAC, daAC;
	    __GLfloat drBC, dgBC, dbBC, daBC;
	    __GLcolor *cc;

	    cc = c->color;
	    drAC = ac->r - cc->r;
	    drBC = bc->r - cc->r;
	    gc->polygon.shader.colorIter.drdx = drAC * t2 - drBC * t1;
	    gc->polygon.shader.colorIter.drdy = drBC * t3 - drAC * t4;
	    dgAC = ac->g - cc->g;
	    dgBC = bc->g - cc->g;
	    gc->polygon.shader.colorIter.dgdx = dgAC * t2 - dgBC * t1;
	    gc->polygon.shader.colorIter.dgdy = dgBC * t3 - dgAC * t4;
	    dbAC = ac->b - cc->b;
	    dbBC = bc->b - cc->b;
	    gc->polygon.shader.colorIter.dbdx = dbAC * t2 - dbBC * t1;
	    gc->polygon.shader.colorIter.dbdy = dbBC * t3 - dbAC * t4;
	    daAC = ac->a - cc->a;
	    daBC = bc->a - cc->a;
	    gc->polygon.shader.colorIter.dadx = daAC * t2 - daBC * t1;
	    gc->polygon.shader.colorIter.dady = daBC * t3 - daAC * t4;
	} else {
	    __GLcolor *flatColor = gc->vertex.provoking->color;
	    gc->polygon.shader.frag.color.r = flatColor->r;
	    gc->polygon.shader.frag.color.g = flatColor->g;
	    gc->polygon.shader.frag.color.b = flatColor->b;
	    gc->polygon.shader.frag.color.a = flatColor->a;
	}
    } else {
	if (modeFlags & __GL_SHADE_SMOOTH) {
	    __GLfloat drAC;
	    __GLfloat drBC;
	    __GLcolor *cc;

	    cc = c->color;
	    drAC = ac->r - cc->r;
	    drBC = bc->r - cc->r;
	    gc->polygon.shader.colorIter.drdx = drAC * t2 - drBC * t1;
	    gc->polygon.shader.colorIter.drdy = drBC * t3 - drAC * t4;
	} else {
	    __GLcolor *flatColor = gc->vertex.provoking->color;
	    gc->polygon.shader.frag.color.r = flatColor->r;
	}
    }
    if (modeFlags & __GL_SHADE_TEXTURE) {
	__GLfloat ax, ay, aw, bx, by, bw, cx, cy, cw;
	__GLfloat dsAC, dsBC, dtAC, dtBC, dqwAC, dqwBC;
	__GLfloat drhowAC, drhowBC, cRhow;

	ax = a->texture[0].x;
	ay = a->texture[0].y;
	aw = a->texture[0].w;

	bx = b->texture[0].x;
	by = b->texture[0].y;
	bw = b->texture[0].w;

	cx = c->texture[0].x;
	cy = c->texture[0].y;
	cw = c->texture[0].w;

	if ((modeFlags & __GL_SHADE_PERSP_TEXTURE) &&
	    !(modeFlags & __GL_SHADE_PROJSCALED_TEXTURE))
	{
	    ax *= a->window.w;
	    ay *= a->window.w;
	    aw *= a->window.w;

	    bx *= b->window.w;
	    by *= b->window.w;
	    bw *= b->window.w;

	    cx *= c->window.w;
	    cy *= c->window.w;
	    cw *= c->window.w;
	}

	dsAC = ax - cx;
	dsBC = bx - cx;
	gc->polygon.shader.texIter[8] = dsAC * t2 - dsBC * t1;
	gc->polygon.shader.texIter[12] = dsBC * t3 - dsAC * t4;
	dtAC = ay - cy;
	dtBC = by - cy;
	gc->polygon.shader.texIter[9] = dtAC * t2 - dtBC * t1;
	gc->polygon.shader.texIter[13] = dtBC * t3 - dtAC * t4;
	dqwAC = aw - cw;
	dqwBC = bw - cw;
	gc->polygon.shader.texIter[10] = dqwAC * t2 - dqwBC * t1;
	gc->polygon.shader.texIter[14] = dqwBC * t3 - dqwAC * t4;

	/* calculate rho/w at the vertices */
	aRhow = (*(__GLcalcRhoProc)gc->procs.calcPolygonRho[0])(gc, &gc->polygon.shader,
						ax, ay, aw) * aw;
	bRhow = (*(__GLcalcRhoProc)gc->procs.calcPolygonRho[0])(gc, &gc->polygon.shader,
						bx, by, bw) * bw;
	cRhow = (*(__GLcalcRhoProc)gc->procs.calcPolygonRho[0])(gc, &gc->polygon.shader,
						cx, cy, cw) * cw;

	drhowAC = aRhow - cRhow;
	drhowBC = bRhow - cRhow;
	gc->polygon.shader.texIter[11] = drhowAC * t2 - drhowBC * t1;
	gc->polygon.shader.texIter[15] = drhowBC * t3 - drhowAC * t4;
    }
    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	__GLfloat dzAC, dzBC;

	dzAC = a->window.z - c->window.z;
	dzBC = b->window.z - c->window.z;
	gc->polygon.shader.dzdxf = dzAC * t2 - dzBC * t1;
	gc->polygon.shader.dzdyf = dzBC * t3 - dzAC * t4;
	gc->polygon.shader.dzdx = (GLint) gc->polygon.shader.dzdxf;
	gc->polygon.shader.dzdxBig = 
	    gc->polygon.shader.dzdx << __GL_STIPPLE_COUNT_BITS;
    }
    if (modeFlags & __GL_SHADE_SLOW_FOG) {
	__GLfloat dfAC, dfBC, cFog;

	if (gc->state.fog.coordSource == GL_FRAGMENT_DEPTH_EXT) {
	    if (gc->state.hints.fog == GL_NICEST) {
		/* Use eyeZ for interpolation value */
		aFog = a->eye.z;
		bFog = b->eye.z;
		cFog = c->eye.z;
	    } else {
		/* Use fog(eyeZ) for interpolation value */
		aFog = a->fog;
		bFog = b->fog;
		cFog = c->fog;
	    }
	} else {
	    /* EXT_fog_coord: the per-vertex coordinate */
	    aFog = a->fogCoord;
	    bFog = b->fogCoord;
	    cFog = c->fogCoord;
	}
	dfAC = aFog - cFog;
	dfBC = bFog - cFog;
	gc->polygon.shader.dfdx = dfAC * t2 - dfBC * t1;
	gc->polygon.shader.dfdy = dfBC * t3 - dfAC * t4;
    }

    /* Snap each y coordinate to its pixel center */
    aIY = (GLint) (a->window.y);
    bIY = (GLint) (b->window.y);
    cIY = (GLint) (c->window.y);

    /*
    ** This algorithim always fills from bottom to top, left to right.
    ** Because of this, ccw triangles are inherently faster because
    ** the parameter values need not be recomputed.
    */
    dxAB = a->window.x - b->window.x;
    dyAB = a->window.y - b->window.y;

    if (ccw) {
	dxdyLeft = dxAC / dyAC;
	dy = (aIY + __GL_ONE_F) - a->window.y;
	__glSnapXLeft(gc, a->window.x + dy*dxdyLeft, dxdyLeft);
	dx = (gc->polygon.shader.ixLeft + __GL_ONE_F) - a->window.x;
	SetInitialParameters(gc, a, ac, aFog, aRhow, dx, dy);
	if (aIY != bIY) {
	    dxdyRight = dxAB / dyAB;
	    __glSnapXRight(&gc->polygon.shader, a->window.x + dy*dxdyRight,
		       dxdyRight);
	    FillSubTriangle(gc, aIY, bIY);
	}

	if (bIY != cIY) {
	    dxdyRight = dxBC / dyBC;
	    dy = (bIY + __GL_ONE_F) - b->window.y;
	    __glSnapXRight(&gc->polygon.shader, b->window.x + dy*dxdyRight,
		       dxdyRight);
	    FillSubTriangle(gc, bIY, cIY);
	}
    } else {
	dxdyRight = dxAC / dyAC;
	dy = (aIY + __GL_ONE_F) - a->window.y;
	__glSnapXRight(&gc->polygon.shader, a->window.x + dy*dxdyRight, dxdyRight);
	if (aIY != bIY) {
	    dxdyLeft = dxAB / dyAB;
	    __glSnapXLeft(gc, a->window.x + dy*dxdyLeft, dxdyLeft);
	    dx = (gc->polygon.shader.ixLeft + __GL_ONE_F) - a->window.x;
	    SetInitialParameters(gc, a, ac, aFog, aRhow, dx, dy);
	    FillSubTriangle(gc, aIY, bIY);
	}

	if (bIY != cIY) {
	    dxdyLeft = dxBC / dyBC;
	    dy = (bIY + __GL_ONE_F) - b->window.y;
	    __glSnapXLeft(gc, b->window.x + dy*dxdyLeft, dxdyLeft);
	    dx = (gc->polygon.shader.ixLeft + __GL_ONE_F) - b->window.x;
	    SetInitialParameters(gc, b, bc, bFog, bRhow, dx, dy);
	    FillSubTriangle(gc, bIY, cIY);
	}
    }

    __GL_RESTORE_TRIANGLE_VERTS(gc, oa, ob, oc, gc->vertex.provoking);
}

/* 690d9230 -- __glFillFlatFogTriangle: the ICD also fogs the provoking vertex's colour when it
** is not one of the three */
void __fastcall __glFillFlatFogTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b,
			     __GLvertexICD *c)
{
    __GLcolor acol, bcol, ccol, pcol;
    __GLcolor *aocp, *bocp, *cocp, *pocp;
    __GLvertexICD *pv;

    pv = gc->vertex.provoking;
    (*(__GLfogColorProc)gc->procs.fogColor)(gc, &acol, pv->color, a->fog);
    (*(__GLfogColorProc)gc->procs.fogColor)(gc, &bcol, pv->color, b->fog);
    (*(__GLfogColorProc)gc->procs.fogColor)(gc, &ccol, pv->color, c->fog);
    if (pv != a && pv != b && pv != c) {
	(*(__GLfogColorProc)gc->procs.fogColor)(gc, &pcol, pv->color, pv->fog);
    }
    aocp = a->color;
    bocp = b->color;
    cocp = c->color;
    a->color = &acol;
    b->color = &bcol;
    c->color = &ccol;
    if (pv != a && pv != b && pv != c) {
	pocp = pv->color;
	pv->color = &pcol;
    }

    (*(__GLfillTriangleProc)gc->procs.fillTriangle2)(gc, a, b, c);

    a->color = aocp;
    b->color = bocp;
    c->color = cocp;
    if (pv != a && pv != b && pv != c) {
	pv->color = pocp;
    }
}

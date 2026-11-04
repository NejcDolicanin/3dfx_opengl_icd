/*
** gl_pgspan.c -- s_pgspan.c span procs the SI port could not take as-is (they reach the depth
** buffer through the WGL drawable buffer), written from the SGI Sample Implementation source.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "icdglob.h"
#include "si_names.h"

/* gc->depthBuffer.buf.other is the WGL depth buffer; its depthPrivate holds the z offset (wgldraw.h) */
#define __GL_DEPTH_BUF(db)      ((db)->buf.wglBuf)
#define __GL_DEPTH_DP(db)       ((__WGLdepthPrivate *)__GL_DEPTH_BUF(db)->depthPrivate)
#define __GL_DEPTH_ELEMSIZE(gc) (__GL_DEPTH_BUF(&(gc)->depthBuffer)->elementSize)
/* the stencil span walks by the WGL stencil buffer's element size too */
#define __GL_STENCIL_ELEMSIZE(gc) (((gc)->stencilBuffer.buf.wglBuf)->elementSize)

typedef GLubyte __GLstencilCell;

/* 690ce540 -- __glStencilTestSpan */
GLboolean __fastcall __glStencilTestSpan(__GLcontext *gc)
{
    __GLstencilCell *tft, *sfb, *fail, cell;
    GLint count, failed;
    __GLstippleWord bit, outMask, *osp;
    GLint w;

    w = gc->polygon.shader.length;

    sfb = gc->polygon.shader.sbuf;
    tft = gc->stencilBuffer.testFuncTable;
    fail = gc->stencilBuffer.failOpTable;
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
	    /* test func table already anded cell values with mask */
	    if (!tft[cell]) {
		/* Test failed */
		outMask &= ~bit;
		sfb[0] = fail[cell];
		failed++;
	    }
	    sfb += __GL_STENCIL_ELEMSIZE(gc);
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

/* 690ce630 -- __glStencilTestStippledSpan */
GLboolean __fastcall __glStencilTestStippledSpan(__GLcontext *gc)
{
    __GLstencilCell *tft, *sfb, *fail, cell;
    GLint failed, count;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLuint smask;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    sfb = gc->polygon.shader.sbuf;
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
	    sfb += __GL_STENCIL_ELEMSIZE(gc);
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

/* 690ce730 -- __glDepthTestOffsetSpan */
GLboolean __fastcall __glDepthTestOffsetSpan(__GLcontext *gc)
{
    __GLzValue *zfb, z;
    GLint numFracBits;
    GLuint cnt;
    GLint failed, count;
    __GLdepthTestPixel testFunc[2];
    __GLstippleWord bit, outMask, *osp;

    GLuint i; /* index */
    GLint w[3]; /* segment count (last entry is always 0) */
    __GLzValue zFrac[2];
    __GLzValue dzdx[2];
    __GLzValue dzdxBig[2];

    __GLfloat offsetZ; /* actual z value */
    __GLfloat outCount;
    __GLfloat range;

    GLint boundary;
    __GLzValue boundarydzdxBig;

    w[0] = gc->polygon.shader.length;
    w[1] = 0;
    w[2] = 0; /* termination condition */

    zfb = gc->polygon.shader.zbuf;
    testFunc[0] = (__GLdepthTestPixel)gc->procs.DTPixel;
    numFracBits = __GL_DEPTH_DP(&gc->depthBuffer)->numFracBits;
    cnt = __GL_DEPTH_DP(&gc->depthBuffer)->cnt;

    osp = gc->polygon.shader.stipplePat;

    failed = 0;
    boundary = 0;

    offsetZ = gc->polygon.shader.offsetZ; 
    outCount = gc->polygon.shader.outBoundCount;
    range = gc->polygon.shader.rangeCount;

    if(outCount > 0) { /* heading towards it */
      if(outCount < range) { /* starts inside */
	/* offsetZ should be within z buffer range */
	zFrac[0] = offsetZ;
	dzdx[0] = gc->polygon.shader.dzdx >> numFracBits;
	dzdxBig[0] = gc->polygon.shader.dzdxBig;
	if(w[0] > outCount) { /* going out */
	  w[1] = w[0] - outCount;
	  w[0] -= w[1];
	  if((gc->depthBuffer.testFunc == GL_LESS &&
	      gc->polygon.shader.endZLimit != 0) ||
	     (gc->depthBuffer.testFunc == GL_GREATER &&
	     gc->polygon.shader.endZLimit == 0))
	      testFunc[1] = __glCDTPixel[0]; /* pick GL_NEVER */
	    else
	      testFunc[1] = (__GLdepthTestPixel)gc->procs.DTPixel;
	  zFrac[1] = gc->polygon.shader.endZLimit;
	  boundary = w[0] % __GL_STIPPLE_BITS;
	  dzdx[1] = 0;
	  dzdxBig[1] = 0;
	}
      } else { /* starts outside */
	zFrac[0] = gc->polygon.shader.startZLimit;
	dzdx[0] = 0;
	dzdxBig[0] = 0;
	if((gc->depthBuffer.testFunc == GL_LESS &&
	    gc->polygon.shader.startZLimit != 0) ||
	   (gc->depthBuffer.testFunc == GL_GREATER &&
	    gc->polygon.shader.startZLimit == 0))
	  testFunc[0] = __glCDTPixel[0]; /* pick GL_NEVER */
	if(w[0] > (GLint)(outCount - range + 1.)) { /* goes in */
	  w[1] = w[0] - (GLint)(outCount - range + 1.);
	  w[0] = (GLint)(outCount - range + 1.);
	  boundary = w[0] % __GL_STIPPLE_BITS;
	  testFunc[1] = (__GLdepthTestPixel)gc->procs.DTPixel;
	  zFrac[1] = w[0] / __GL_STIPPLE_BITS * gc->polygon.shader.dzdxBig + 
	             boundary * gc->polygon.shader.dzdx +
		     (GLdouble)offsetZ;
	  /* starting Z for 2nd segment should be inside z-buffer range */
	  dzdx[1] = gc->polygon.shader.dzdx >> numFracBits;
	  dzdxBig[1] = gc->polygon.shader.dzdxBig;
	}
      }
    } else { /* already past */
      if((gc->depthBuffer.testFunc == GL_LESS &&
	  gc->polygon.shader.endZLimit != 0) ||
	 (gc->depthBuffer.testFunc == GL_GREATER &&
	  gc->polygon.shader.endZLimit == 0))
	testFunc[0] = __glCDTPixel[0]; /* pick GL_NEVER */
      zFrac[0] = gc->polygon.shader.endZLimit;
      dzdx[0] = 0;
      dzdxBig[0] = 0;
    }

    /* this algorithm shouldn't change the number of pixels tested */

    i = 0;
    while(w[i]) {
      while (w[i]) {
      
        count = w[i];

	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w[i] -= count;

	z = (zFrac[i] >> numFracBits) + cnt;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if( (*testFunc[i])(z,zfb) == GL_FALSE ) {
		outMask &= ~bit;
		failed++;
	    }
	    z += dzdx[i];
	    zfb = (__GLzValue *) ((GLubyte *) zfb + 
				  __GL_DEPTH_ELEMSIZE(gc));
	    bit >>= 1;
	}
	*osp++ = outMask;
	zFrac[i] += dzdxBig[i];
      }
      i++;
      if(w[i] && boundary) { /* stitch mask together */
	count = __GL_STIPPLE_BITS - boundary;
	boundarydzdxBig = (GLint)((GLint)dzdxBig[i] * 
	                  ((GLdouble)count / (GLdouble)__GL_STIPPLE_BITS));
	if(count > w[i])
	  count = w[i];
	w[i] -= count;
	osp--;
	z = (zFrac[i] >> numFracBits) + cnt;
	while (--count >= 0) {
	  if( (*testFunc[i])(z,zfb) == GL_FALSE ) {
	    outMask &= ~bit;
	    failed++;
	  }
	  z += dzdx[i];
	  zfb = (__GLzValue *) ((GLubyte *) zfb + 
				__GL_DEPTH_ELEMSIZE(gc));
	  bit >>= 1;
	}
	*osp++ = outMask;
	zFrac[i] += boundarydzdxBig;
      }
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

/* 690ceb90 -- __glDepthTestStippledOffsetSpan */
GLboolean __fastcall __glDepthTestStippledOffsetSpan(__GLcontext *gc)
{
    __GLzValue z, *zfb;
    GLint numFracBits;
    GLuint cnt;
    GLint failed, count;
    __GLdepthTestPixel testFunc[2];
    __GLstippleWord bit, inMask, outMask, *sp;

    GLuint i; /* index */
    GLint w[3]; /* segment count (last entry is always 0) */
    __GLzValue zFrac[2];
    __GLzValue dzdx[2];
    __GLzValue dzdxBig[2];

    __GLfloat offsetZ; /* actual z value */
    __GLfloat outCount;
    __GLfloat range;

    GLint boundary;
    __GLzValue boundarydzdxBig;

    w[0] = gc->polygon.shader.length;
    w[1] = 0;
    w[2] = 0; /* termination condition */

    zfb = gc->polygon.shader.zbuf;
    testFunc[0] = (__GLdepthTestPixel)gc->procs.DTPixel;
    numFracBits = __GL_DEPTH_DP(&gc->depthBuffer)->numFracBits;
    cnt = __GL_DEPTH_DP(&gc->depthBuffer)->cnt;

    sp = gc->polygon.shader.stipplePat;

    failed = 0;
    boundary = 0;

    offsetZ = gc->polygon.shader.offsetZ; 
    outCount = gc->polygon.shader.outBoundCount;
    range = gc->polygon.shader.rangeCount;

    if(outCount > 0) { /* heading towards it */
      if(outCount < range) { /* starts inside */
	/* offsetZ should be within z buffer range */
	zFrac[0] = offsetZ;
	dzdx[0] = gc->polygon.shader.dzdx >> numFracBits;
	dzdxBig[0] = gc->polygon.shader.dzdxBig;
	if(w[0] > outCount) { /* going out */
	  w[1] = w[0] - outCount;
	  w[0] -= w[1];
	  if((gc->depthBuffer.testFunc == GL_LESS &&
	      gc->polygon.shader.endZLimit != 0) ||
	     (gc->depthBuffer.testFunc == GL_GREATER &&
	     gc->polygon.shader.endZLimit == 0))
	      testFunc[1] = __glCDTPixel[0]; /* pick GL_NEVER */
	    else
	      testFunc[1] = (__GLdepthTestPixel)gc->procs.DTPixel;
	  zFrac[1] = gc->polygon.shader.endZLimit;
	  boundary = w[0] % __GL_STIPPLE_BITS;
	  dzdx[1] = 0;
	  dzdxBig[1] = 0;
	}
      } else { /* starts outside */
	zFrac[0] = gc->polygon.shader.startZLimit;
	dzdx[0] = 0;
	dzdxBig[0] = 0;
	if((gc->depthBuffer.testFunc == GL_LESS &&
	    gc->polygon.shader.startZLimit != 0) ||
	   (gc->depthBuffer.testFunc == GL_GREATER &&
	    gc->polygon.shader.startZLimit == 0))
	  testFunc[0] = __glCDTPixel[0]; /* pick GL_NEVER */
	if(w[0] > (GLint)(outCount - range + 1.)) { /* goes in */
	  w[1] = w[0] - (GLint)(outCount - range + 1.);
	  w[0] = (GLint)(outCount - range + 1.);
	  boundary = w[0] % __GL_STIPPLE_BITS;
	  testFunc[1] = (__GLdepthTestPixel)gc->procs.DTPixel;
	  zFrac[1] = w[0] / __GL_STIPPLE_BITS * gc->polygon.shader.dzdxBig + 
	             boundary * gc->polygon.shader.dzdx +
		     (GLdouble)offsetZ;
	  /* starting Z for 2nd segment should be inside z-buffer range */
	  dzdx[1] = gc->polygon.shader.dzdx >> numFracBits;
	  dzdxBig[1] = gc->polygon.shader.dzdxBig;
	}
      }
    } else { /* already past */
      if((gc->depthBuffer.testFunc == GL_LESS &&
	  gc->polygon.shader.endZLimit != 0) ||
	 (gc->depthBuffer.testFunc == GL_GREATER &&
	  gc->polygon.shader.endZLimit == 0))
	testFunc[0] = __glCDTPixel[0]; /* pick GL_NEVER */
      zFrac[0] = gc->polygon.shader.endZLimit;
      dzdx[0] = 0;
      dzdxBig[0] = 0;
    }
	
    /* this algorithm shouldn't change the number of pixels tested */

    i = 0;
    while(w[i]) {
      while (w[i]) {
	count = w[i];
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w[i] -= count;

	z = (zFrac[i] >> numFracBits) + cnt;
	inMask = *sp;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		if( (*testFunc[i])(z,zfb) == GL_FALSE ) {
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;
	    z += dzdx[i];
	    zfb = (__GLzValue *) ((GLubyte *) zfb + 
				  __GL_DEPTH_ELEMSIZE(gc));
	    bit >>= 1;
	}
	*sp++ = outMask & inMask;
	zFrac[i] += dzdxBig[i];
      }
      i++;
      if(w[i] && boundary) { /* stitch mask together */
	count = __GL_STIPPLE_BITS - boundary;
	boundarydzdxBig = (GLint)((GLint)dzdxBig[i] * 
	                  ((GLdouble)count / (GLdouble)__GL_STIPPLE_BITS));
	if(count > w[i])
	  count = w[i];
	w[i] -= count;
	sp--;
	z = (zFrac[i] >> numFracBits) + cnt;

	while (--count >= 0) {
	    if (inMask & bit) {
		if( (*testFunc[i])(z,zfb) == GL_FALSE ) {
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;
	    z += dzdx[i];
	    zfb = (__GLzValue *) ((GLubyte *) zfb + 
				  __GL_DEPTH_ELEMSIZE(gc));
	  bit >>= 1;
	}
	*sp++ = outMask & inMask;
	zFrac[i] += boundarydzdxBig;
      }
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_FALSE;
    }
    return GL_TRUE;
}

/* 690cf000 -- __glDepthTestSpan */
GLboolean __fastcall __glDepthTestSpan(__GLcontext *gc)
{
    __GLzValue zFrac, dzdxBig, *zfb;
    __GLfloat z, dzdx, scale;
    GLint failed, count;
    __GLdepthTestPixel testFunc;
    __GLstippleWord bit, outMask, *osp;
    GLint w;

    w = gc->polygon.shader.length;

    zfb = gc->polygon.shader.zbuf;
    testFunc = (__GLdepthTestPixel)gc->procs.DTPixel;
    zFrac = gc->polygon.shader.frag.z;
    dzdxBig = gc->polygon.shader.dzdxBig;
    scale = pow(2.0, __GL_DEPTH_DP(&gc->depthBuffer)->numFracBits);
    dzdx = gc->polygon.shader.dzdxf / scale;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	z = zFrac / scale;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if( (*testFunc)((__GLzValue)z, zfb) == GL_FALSE ) {
		outMask &= ~bit;
		failed++;
	    }
	    z += dzdx;
	    zfb = (__GLzValue *) ((GLubyte *) zfb + 
				  __GL_DEPTH_ELEMSIZE(gc));
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

/* 690cf160 -- __glDepthTestStippledSpan */
GLboolean __fastcall __glDepthTestStippledSpan(__GLcontext *gc)
{
    __GLzValue zFrac, dzdxBig, *zfb;
    __GLfloat z, dzdx, scale;
    GLint failed, count;
    __GLdepthTestPixel testFunc;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLint w;

    sp = gc->polygon.shader.stipplePat;
    w = gc->polygon.shader.length;

    zfb = gc->polygon.shader.zbuf;
    testFunc = (__GLdepthTestPixel)gc->procs.DTPixel;
    zFrac = gc->polygon.shader.frag.z;
    dzdxBig = gc->polygon.shader.dzdxBig;
    scale = pow(2.0, __GL_DEPTH_DP(&gc->depthBuffer)->numFracBits);
    dzdx = gc->polygon.shader.dzdxf / scale;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	z = zFrac / scale;
	inMask = *sp;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		if( (*testFunc)((__GLzValue)z, zfb) == GL_FALSE ) {
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;
	    z += dzdx;
	    zfb = (__GLzValue *) ((GLubyte *) zfb + 
				  __GL_DEPTH_ELEMSIZE(gc));
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

/* 690cf2c0 -- __glDepthTestStencilSpan */
GLboolean __fastcall __glDepthTestStencilSpan(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    __GLzValue zFrac, dzdxBig, *zfb;
    __GLfloat z, dzdx, scale;
    GLint failed, count;
    __GLdepthTestPixel testFunc;
    __GLstippleWord bit, outMask, *osp;
    GLint w;

    w = gc->polygon.shader.length;

    zfb = gc->polygon.shader.zbuf;
    sfb = gc->polygon.shader.sbuf;
    zFailOp = gc->stencilBuffer.depthFailOpTable;
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    testFunc = (__GLdepthTestPixel)gc->procs.DTPixel;
    zFrac = gc->polygon.shader.frag.z;
    dzdxBig = gc->polygon.shader.dzdxBig;
    scale = pow(2.0, __GL_DEPTH_DP(&gc->depthBuffer)->numFracBits);
    dzdx = gc->polygon.shader.dzdxf / scale;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	z = zFrac / scale;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if( (*testFunc)((__GLzValue)z, zfb) ) {
		sfb[0] = zPassOp[sfb[0]];
	    } else {
		sfb[0] = zFailOp[sfb[0]];
		outMask &= ~bit;
		failed++;
	    }
	    z += dzdx;
	    zfb = (__GLzValue *) ((GLubyte *) zfb + 
				  __GL_DEPTH_ELEMSIZE(gc));
	    sfb += __GL_STENCIL_ELEMSIZE(gc);
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

/* 690cf470 -- __glDepthTestStencilStippledSpan */
GLboolean __fastcall __glDepthTestStencilStippledSpan(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    __GLzValue zFrac, dzdxBig, *zfb;
    __GLfloat z, dzdx, scale;
    GLint failed, count;
    __GLdepthTestPixel testFunc;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    zfb = gc->polygon.shader.zbuf;
    sfb = gc->polygon.shader.sbuf;
    zFailOp = gc->stencilBuffer.depthFailOpTable;
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    testFunc = (__GLdepthTestPixel)gc->procs.DTPixel;
    zFrac = gc->polygon.shader.frag.z;
    dzdxBig = gc->polygon.shader.dzdxBig;
    scale = pow(2.0, __GL_DEPTH_DP(&gc->depthBuffer)->numFracBits);
    dzdx = gc->polygon.shader.dzdxf / scale;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	z = zFrac / scale;
	inMask = *sp;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		if( (*testFunc)((__GLzValue)z, zfb) ) {
		    sfb[0] = zPassOp[sfb[0]];
		} else {
		    sfb[0] = zFailOp[sfb[0]];
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;
	    z += dzdx;
	    zfb = (__GLzValue *) ((GLubyte *) zfb + 
				  __GL_DEPTH_ELEMSIZE(gc));
	    sfb += __GL_STENCIL_ELEMSIZE(gc);
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

/* 690cf620 -- __glDepthTestStencilOffsetSpan */
GLboolean __fastcall __glDepthTestStencilOffsetSpan(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    __GLzValue z, *zfb;
    GLint numFracBits;
    GLuint cnt;
    GLint failed, count;
    __GLdepthTestPixel testFunc[2];
    __GLstippleWord bit, outMask, *osp;

    GLuint i; /* index */
    GLint w[3]; /* segment count (last entry is always 0) */
    __GLzValue zFrac[2];
    __GLzValue dzdx[2];
    __GLzValue dzdxBig[2];

    __GLfloat offsetZ; /* actual z value */
    __GLfloat outCount;
    __GLfloat range;

    GLint boundary;
    __GLzValue boundarydzdxBig;

    w[0] = gc->polygon.shader.length;
    w[1] = 0;
    w[2] = 0; /* termination condition */

    zfb = gc->polygon.shader.zbuf;
    sfb = gc->polygon.shader.sbuf;
    zFailOp = gc->stencilBuffer.depthFailOpTable;
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    testFunc[0] = (__GLdepthTestPixel)gc->procs.DTPixel;
    numFracBits = __GL_DEPTH_DP(&gc->depthBuffer)->numFracBits;
    cnt = __GL_DEPTH_DP(&gc->depthBuffer)->cnt;

    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    boundary = 0;

    offsetZ = gc->polygon.shader.offsetZ; 
    outCount = gc->polygon.shader.outBoundCount;
    range = gc->polygon.shader.rangeCount;

    if(outCount > 0) { /* heading towards it */
      if(outCount < range) { /* starts inside */
	/* offsetZ should be within z buffer range */
	zFrac[0] = offsetZ;
	dzdx[0] = gc->polygon.shader.dzdx >> numFracBits;
	dzdxBig[0] = gc->polygon.shader.dzdxBig;
	if(w[0] > outCount) { /* going out */
	  w[1] = w[0] - outCount;
	  w[0] -= w[1];
	  if((gc->depthBuffer.testFunc == GL_LESS &&
	      gc->polygon.shader.endZLimit != 0) ||
	     (gc->depthBuffer.testFunc == GL_GREATER &&
	     gc->polygon.shader.endZLimit == 0))
	      testFunc[1] = __glCDTPixel[0]; /* pick GL_NEVER */
	    else
	      testFunc[1] = (__GLdepthTestPixel)gc->procs.DTPixel;
	  zFrac[1] = gc->polygon.shader.endZLimit;
	  boundary = w[0] % __GL_STIPPLE_BITS;
	  dzdx[1] = 0;
	  dzdxBig[1] = 0;
	}
      } else { /* starts outside */
	zFrac[0] = gc->polygon.shader.startZLimit;
	dzdx[0] = 0;
	dzdxBig[0] = 0;
	if((gc->depthBuffer.testFunc == GL_LESS &&
	    gc->polygon.shader.startZLimit != 0) ||
	   (gc->depthBuffer.testFunc == GL_GREATER &&
	    gc->polygon.shader.startZLimit == 0))
	  testFunc[0] = __glCDTPixel[0]; /* pick GL_NEVER */
	if(w[0] > (GLint)(outCount - range + 1.)) { /* goes in */
	  w[1] = w[0] - (GLint)(outCount - range + 1.);
	  w[0] = (GLint)(outCount - range + 1.);
	  boundary = w[0] % __GL_STIPPLE_BITS;
	  testFunc[1] = (__GLdepthTestPixel)gc->procs.DTPixel;
	  zFrac[1] = w[0] / __GL_STIPPLE_BITS * gc->polygon.shader.dzdxBig + 
	             boundary * gc->polygon.shader.dzdx +
		     (GLdouble)offsetZ;
	  /* starting Z for 2nd segment should be inside z-buffer range */
	  dzdx[1] = gc->polygon.shader.dzdx >> numFracBits;
	  dzdxBig[1] = gc->polygon.shader.dzdxBig;
	}
      }
    } else { /* already past */
      if((gc->depthBuffer.testFunc == GL_LESS &&
	  gc->polygon.shader.endZLimit != 0) ||
	 (gc->depthBuffer.testFunc == GL_GREATER &&
	  gc->polygon.shader.endZLimit == 0))
	testFunc[0] = __glCDTPixel[0]; /* pick GL_NEVER */
      zFrac[0] = gc->polygon.shader.endZLimit;
      dzdx[0] = 0;
      dzdxBig[0] = 0;
    }
	
    /* this algorithm shouldn't change the number of pixels tested */

    i = 0;
    while(w[i]) {
      while (w[i]) {
	count = w[i];
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w[i] -= count;

	z = (zFrac[i] >> numFracBits) + cnt;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if( (*testFunc[i])(z,zfb) ) {
		sfb[0] = zPassOp[sfb[0]];
	    } else {
		sfb[0] = zFailOp[sfb[0]];
		outMask &= ~bit;
		failed++;
	    }
	    z += dzdx[i];
	    zfb = (__GLzValue *) ((GLubyte *) zfb + 
				  __GL_DEPTH_ELEMSIZE(gc));
	    sfb += __GL_STENCIL_ELEMSIZE(gc);
	    bit >>= 1;
	}
	*osp++ = outMask;
	zFrac[i] += dzdxBig[i];
      }
      i++;
      if(w[i] && boundary) { /* stitch mask together */
	count = __GL_STIPPLE_BITS - boundary;
	boundarydzdxBig = (GLint)((GLint)dzdxBig[i] * 
	                  ((GLdouble)count / (GLdouble)__GL_STIPPLE_BITS));
	if(count > w[i])
	  count = w[i];
	w[i] -= count;
	osp--;
	z = (zFrac[i] >> numFracBits) + cnt;

	while (--count >= 0) {
	    if( (*testFunc[i])(z,zfb) ) {
		sfb[0] = zPassOp[sfb[0]];
	    } else {
		sfb[0] = zFailOp[sfb[0]];
		outMask &= ~bit;
		failed++;
	    }
	    z += dzdx[i];
	    zfb = (__GLzValue *) ((GLubyte *) zfb + 
				  __GL_DEPTH_ELEMSIZE(gc));
	    sfb += __GL_STENCIL_ELEMSIZE(gc);
	  bit >>= 1;
	}
	*osp++ = outMask;
	zFrac[i] += boundarydzdxBig;
      }
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

/* 690cfb30 -- __glDepthTestStencilStippledOffsetSpan */
GLboolean __fastcall __glDepthTestStencilStippledOffsetSpan(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    __GLzValue z, *zfb;
    GLint numFracBits;
    GLuint cnt;
    GLint failed, count;
    __GLdepthTestPixel testFunc[2];
    __GLstippleWord bit, inMask, outMask, *sp;

    GLuint i; /* index */
    GLint w[3]; /* segment count (last entry is always 0) */
    __GLzValue zFrac[2];
    __GLzValue dzdx[2];
    __GLzValue dzdxBig[2];

    __GLfloat offsetZ; /* actual z value */
    __GLfloat outCount;
    __GLfloat range;

    GLint boundary;
    __GLzValue boundarydzdxBig;

    w[0] = gc->polygon.shader.length;
    w[1] = 0;
    w[2] = 0; /* termination condition */

    zfb = gc->polygon.shader.zbuf;
    sfb = gc->polygon.shader.sbuf;
    zFailOp = gc->stencilBuffer.depthFailOpTable;
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    testFunc[0] = (__GLdepthTestPixel)gc->procs.DTPixel;
    numFracBits = __GL_DEPTH_DP(&gc->depthBuffer)->numFracBits;
    cnt = __GL_DEPTH_DP(&gc->depthBuffer)->cnt;

    sp = gc->polygon.shader.stipplePat;

    failed = 0;
    boundary = 0;

    offsetZ = gc->polygon.shader.offsetZ; 
    outCount = gc->polygon.shader.outBoundCount;
    range = gc->polygon.shader.rangeCount;

    if(outCount > 0) { /* heading towards it */
      if(outCount < range) { /* starts inside */
	/* offsetZ should be within z buffer range */
	zFrac[0] = offsetZ;
	dzdx[0] = gc->polygon.shader.dzdx >> numFracBits;
	dzdxBig[0] = gc->polygon.shader.dzdxBig;
	if(w[0] > outCount) { /* going out */
	  w[1] = w[0] - outCount;
	  w[0] -= w[1];
	  if((gc->depthBuffer.testFunc == GL_LESS &&
	      gc->polygon.shader.endZLimit != 0) ||
	     (gc->depthBuffer.testFunc == GL_GREATER &&
	     gc->polygon.shader.endZLimit == 0))
	      testFunc[1] = __glCDTPixel[0]; /* pick GL_NEVER */
	    else
	      testFunc[1] = (__GLdepthTestPixel)gc->procs.DTPixel;
	  zFrac[1] = gc->polygon.shader.endZLimit;
	  boundary = w[0] % __GL_STIPPLE_BITS;
	  dzdx[1] = 0;
	  dzdxBig[1] = 0;
	}
      } else { /* starts outside */
	zFrac[0] = gc->polygon.shader.startZLimit;
	dzdx[0] = 0;
	dzdxBig[0] = 0;
	if((gc->depthBuffer.testFunc == GL_LESS &&
	    gc->polygon.shader.startZLimit != 0) ||
	   (gc->depthBuffer.testFunc == GL_GREATER &&
	    gc->polygon.shader.startZLimit == 0))
	  testFunc[0] = __glCDTPixel[0]; /* pick GL_NEVER */
	if(w[0] > (GLint)(outCount - range + 1.)) { /* goes in */
	  w[1] = w[0] - (GLint)(outCount - range + 1.);
	  w[0] = (GLint)(outCount - range + 1.);
	  boundary = w[0] % __GL_STIPPLE_BITS;
	  testFunc[1] = (__GLdepthTestPixel)gc->procs.DTPixel;
	  zFrac[1] = w[0] / __GL_STIPPLE_BITS * gc->polygon.shader.dzdxBig + 
	             boundary * gc->polygon.shader.dzdx +
		     (GLdouble)offsetZ;
	  /* starting Z for 2nd segment should be inside z-buffer range */
	  dzdx[1] = gc->polygon.shader.dzdx >> numFracBits;
	  dzdxBig[1] = gc->polygon.shader.dzdxBig;
	}
      }
    } else { /* already past */
      if((gc->depthBuffer.testFunc == GL_LESS &&
	  gc->polygon.shader.endZLimit != 0) ||
	 (gc->depthBuffer.testFunc == GL_GREATER &&
	  gc->polygon.shader.endZLimit == 0))
	testFunc[0] = __glCDTPixel[0]; /* pick GL_NEVER */
      zFrac[0] = gc->polygon.shader.endZLimit;
      dzdx[0] = 0;
      dzdxBig[0] = 0;
    }
	
    /* this algorithm shouldn't change the number of pixels tested */

    i = 0;
    while(w[i]) {
      while (w[i]) {
	count = w[i];
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w[i] -= count;

	z = (zFrac[i] >> numFracBits) + cnt;
	inMask = *sp;
	outMask = ~0;
	bit = __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		if( (*testFunc[i])(z,zfb) ) {
		    sfb[0] = zPassOp[sfb[0]];
		} else {
		    sfb[0] = zFailOp[sfb[0]];
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;
	    z += dzdx[i];
	    zfb = (__GLzValue *) ((GLubyte *) zfb + 
				  __GL_DEPTH_ELEMSIZE(gc));
	    sfb += __GL_STENCIL_ELEMSIZE(gc);
	    bit >>= 1;
	}
	*sp++ = outMask & inMask;
	zFrac[i] += dzdxBig[i];
      }
      i++;
      if(w[i] && boundary) { /* stitch mask together */
	count = __GL_STIPPLE_BITS - boundary;
	boundarydzdxBig = (GLint)((GLint)dzdxBig[i] * 
	                  ((GLdouble)count / (GLdouble)__GL_STIPPLE_BITS));
	if(count > w[i])
	  count = w[i];
	w[i] -= count;
	sp--;
	z = (zFrac[i] >> numFracBits) + cnt;
	while (--count >= 0) {
	    if (inMask & bit) {
		if( (*testFunc[i])(z,zfb) ) {
		    sfb[0] = zPassOp[sfb[0]];
		} else {
		    sfb[0] = zFailOp[sfb[0]];
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;
	    z += dzdx[i];
	    zfb = (__GLzValue *) ((GLubyte *) zfb + 
				  __GL_DEPTH_ELEMSIZE(gc));
	    sfb += __GL_STENCIL_ELEMSIZE(gc);
	    bit >>= 1;
	  }
	*sp++ = outMask & inMask;
	zFrac[i] += boundarydzdxBig;
      }
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_FALSE;
    }

    return GL_TRUE;
}

/* 690d0030 -- __glDepthPassSpan */
GLboolean __fastcall __glDepthPassSpan(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp;
    GLint count;
    GLint w;

    w = gc->polygon.shader.length;

    sfb = gc->polygon.shader.sbuf;
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    count = w;
    while (--count >= 0) {
	sfb[0] = zPassOp[sfb[0]];
	sfb += __GL_STENCIL_ELEMSIZE(gc);
    }

    return GL_FALSE;
}

/* 690d0070 -- __glDepthPassStippledSpan */
GLboolean __fastcall __glDepthPassStippledSpan(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp;
    GLint count;
    __GLstippleWord bit, inMask, *sp;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    sfb = gc->polygon.shader.sbuf;
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
	    sfb += __GL_STENCIL_ELEMSIZE(gc);
	    bit >>= 1;
	}
    }

    /* Call next proc */
    return GL_FALSE;
}

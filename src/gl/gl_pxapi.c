/*
** gl_pxapi.c -- px_api.c / px_read.c / px_rend.c: glPixelStore / Transfer / Map / Zoom, the span readers and
** renderers of the pixel paths.
*/
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "icdprotos.h"
#include "icdglob.h"
#include <string.h>
#include "og.h"

/* ---- the colour buffer store of a raster fragment: texture and fog from the raster position ---- */
typedef void (__fastcall *__GLtextureRaster)(__GLcontext *gc, __GLcolor *color, __GLfloat s, __GLfloat t, __GLfloat r);
typedef void (__fastcall *__GLfogPoint)(__GLcontext *gc, __GLfragmentICD *frag, __GLfloat z);
typedef void (__fastcall *__GLstore)(__GLcolorBufferICD *cfb, __GLfragmentICD *frag);
/* the depth buffer's WGL buffer private block (wgldraw.h __WGLdepthPrivate) */
#define DEPTH_PRIVATE(gc)    ((__WGLdepthPrivate *)(gc)->depthBuffer.buf.wglBuf->depthPrivate)
/* ---- px_rend.c: __glSpanRenderDepthUint2 -------------------------------------------------------
** The shift comes from the depth buffer's numFracBits (__WGLdepthPrivate) and modes.depthBits. */
typedef void (__fastcall *__GLpixelStoreProc)(struct __GLcolorBufferRec *cfb, const __GLfragmentICD *frag);
/* ---- px_read.c: depth span readers -------------------------------------------------------------
** db = the depth buffer's WGL buffer: depth bits at +8, its depthPrivate block holds numFracBits. */
typedef GLuint (__fastcall *__GLdepthFetchProc)(__GLdepthBufferICD *fb, GLint x, GLint y);
#define DEPTH_FRACBITS(gc)   (DEPTH_PRIVATE(gc)->numFracBits)
#define DEPTH_WGL(gc)        ((gc)->depthBuffer.buf.wglBuf)
/* the packed pixel types this kit knows */
#define GL_UNSIGNED_BYTE_3_3_2         0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4      0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1      0x8034
#define GL_UNSIGNED_INT_8_8_8_8        0x8035
#define GL_UNSIGNED_INT_10_10_10_2     0x8036
extern void __stdcall __glim_PixelStorei(GLenum mode, GLint value);
typedef __GLpixelSpanInfoICD __GLpixelSpanInfo;
typedef void (__fastcall *__GLstoreProc)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
#define __GL_DEPTH_DP(gc)    ((__WGLdepthPrivate *)((gc)->depthBuffer.buf.wglBuf)->depthPrivate)
extern void __stdcall __glim_PixelTransferf(GLenum mode, GLfloat value);

/* 69083630 -- __glSlowDrawPixelsStore (px_rend.c): one texture unit with q divided out, and the
** fog distance is eye.z or the fog coordinate by GL_FOG_COORDINATE_SOURCE_EXT */
void __fastcall __glSlowDrawPixelsStore(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GLcontext *gc = cfb->buf.gc;
    __GLfragmentICD f;
    __GLfloat oneOverQ;

    f = *frag;
    if (gc->texture.textureEnabled[0]) {
	oneOverQ = __GL_ONE_F / gc->state.current.rasterPos.texture[0].w;
	(*(__GLtextureRaster)gc->procs.textureRaster[0])(gc, &f.color,
		gc->state.current.rasterPos.texture[0].x * oneOverQ, gc->state.current.rasterPos.texture[0].y * oneOverQ, __GL_ONE_F);
    }
    if (gc->state.enables.general & __GL_FOG_ENABLE) {
	if (gc->state.fog.coordSource == GL_FRAGMENT_DEPTH_EXT) {
	    (*(__GLfogPoint)gc->procs.fogPoint)(gc, &f, gc->state.current.rasterPos.eye.z);
	} else {
	    (*(__GLfogPoint)gc->procs.fogPoint)(gc, &f, gc->state.current.rasterPos.fogCoord);
	}
    }
    (*(__GLstore)gc->procs.store)(cfb, &f);
}

/* 69083e70 -- __glSpanRenderDepthUint2 */
void __fastcall __glSpanRenderDepthUint2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    const GLuint *spanData;
    __GLfragmentICD frag;
    __GLpixelStoreProc store;
    GLint endCol, startCol;
    GLint rows;
    GLint shiftAmt = 32 - DEPTH_PRIVATE(gc)->numFracBits - gc->modes.depthBits;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);

    frag.color.r = gc->constants.redRescale *
        gc->state.current.rasterPos.colors[__GL_FRONTFACE].r;
    frag.color.g = gc->constants.greenRescale *
        gc->state.current.rasterPos.colors[__GL_FRONTFACE].g;
    frag.color.b = gc->constants.blueRescale *
        gc->state.current.rasterPos.colors[__GL_FRONTFACE].b;
    frag.color.a = gc->constants.alphaRescale *
        gc->state.current.rasterPos.colors[__GL_FRONTFACE].a;
    store = (__GLpixelStoreProc)gc->procs.pxStore;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        spanData = (const GLuint*) span;
        frag.y = row;
        i = startCol;
        do {
            frag.z = (*spanData++) >> shiftAmt;
            frag.x = i;

            (*store)(gc->drawBuffer, &frag);
            i += coladd;
        } while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69085930 -- __glSpanReadDepth */
void __fastcall __glSpanReadDepth(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;
    GLshort *pixelArray;
    GLint skipCount;
    __GLfloat oneOverScale;

    width = spanInfo->width;
    spanData = (GLfloat*) span;
    pixelArray = spanInfo->pixelArray;

    readY = (GLint) spanInfo->readY;
    readX = (GLint) spanInfo->readX;
    oneOverScale = __GL_ONE_F / (GLuint)(gc->depthBuffer.scale >> DEPTH_FRACBITS(gc));

    for (i=0; i<width; i++) {
        *spanData++ =
            (*(__GLdepthFetchProc)gc->depthBuffer.fetch)(&(gc->depthBuffer), readX, readY) *
            oneOverScale;
        skipCount = *pixelArray++;
        readX += skipCount;
    }
}

/* 690859f0 -- __glSpanReadDepth2 */
void __fastcall __glSpanReadDepth2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;
    __GLfloat oneOverScale;

    width = spanInfo->width;
    spanData = (GLfloat*) span;

    readY = (GLint) spanInfo->readY;
    readX = (GLint) spanInfo->readX;
    oneOverScale = __GL_ONE_F / (GLuint)(gc->depthBuffer.scale >> DEPTH_FRACBITS(gc));

    for (i=0; i<width; i++) {
        *spanData++ =
            (*(__GLdepthFetchProc)gc->depthBuffer.fetch)(&(gc->depthBuffer), readX, readY) *
            oneOverScale;
        readX++;
    }
}

/* 69085aa0 -- __glSpanReadDepthUint */
void __fastcall __glSpanReadDepthUint(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLvoid *span)
{
    GLint i;
    GLint width;
    GLuint *spanData;
    GLint readY, readX;
    GLshort *pixelArray;
    GLint skipCount;
    GLint shiftAmt = 32 - DEPTH_FRACBITS(gc) - DEPTH_WGL(gc)->depth;

    width = spanInfo->width;
    spanData = (GLuint*) span;
    pixelArray = spanInfo->pixelArray;

    readY = (GLint) spanInfo->readY;
    readX = (GLint) spanInfo->readX;

    for (i=0; i<width; i++) {
        *spanData++ =
            (*(__GLdepthFetchProc)gc->depthBuffer.fetch)(&(gc->depthBuffer), readX, readY) <<
            shiftAmt;
        skipCount = *pixelArray++;
        readX += skipCount;
    }
}

/* 69085b50 -- __glSpanReadDepthUint2 */
void __fastcall __glSpanReadDepthUint2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLvoid *span)
{
    GLint i;
    GLint width;
    GLuint *spanData;
    GLint readY, readX;
    GLint shiftAmt = 32 - DEPTH_FRACBITS(gc) - DEPTH_WGL(gc)->depth;

    width = spanInfo->width;
    spanData = (GLuint*) span;

    readY = (GLint) spanInfo->readY;
    readX = (GLint) spanInfo->readX;

    for (i=0; i<width; i++) {
        *spanData++ =
            (*(__GLdepthFetchProc)gc->depthBuffer.fetch)(&(gc->depthBuffer), readX, readY) <<
            shiftAmt;
        readX++;
    }
}

/************************************************************************/
/* px_api.c */

/* 690bf820 -- __glInitPixelState (the ICD does not store the zero biases; no color table /
** convolution / histogram / minmax state) */
void __fastcall __glInitPixelState(__GLcontext *gc)
{
    __GLpixelState *ps = &gc->state.pixel;
    GLenum m;

    /*
    ** Initialize transfer mode.
    */
    ps->transferMode.scale.r = 1.0F;
    ps->transferMode.scale.g = 1.0F;
    ps->transferMode.scale.b = 1.0F;
    ps->transferMode.scale.a = 1.0F;
    ps->transferMode.depthScale = 1.0F;
    ps->transferMode.zoomX = 1.0F;
    ps->transferMode.zoomY = 1.0F;

    /*
    ** Initialize pixel maps with default sizes and values.
    */
    for (m = GL_PIXEL_MAP_I_TO_I; m <= GL_PIXEL_MAP_A_TO_A; m++) {
	__glInitDefaultPixelMap(gc, m);
    }

    /*
    ** Initialize store mode.
    */
    ps->packModes.alignment = 4;
    ps->unpackModes.alignment = 4;

    /* Setup to use the correct read buffer */
    if (gc->modes.doubleBufferMode) {
	ps->readBuffer = GL_BACK;
    } else {
	ps->readBuffer = GL_FRONT;
    }
    ps->readBufferReturn = ps->readBuffer;

    __glPixelSetColorScales(gc);
}


/* 690c05c0 -- __glCheckDrawPixelArgs */
GLboolean __fastcall __glCheckDrawPixelArgs(__GLcontext *gc, GLsizei width, GLsizei height,
				 GLenum format, GLenum type)
{
    GLboolean index;

    if ((width < 0) || (height < 0)) {
	__glSetError(GL_INVALID_VALUE);
	return GL_FALSE;
    }
    switch (format) {
      case GL_STENCIL_INDEX:
	if (!gc->modes.haveStencilBuffer) {
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_ABGR_EXT:
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	if (gc->modes.colorIndexMode) {
	    /* Can't convert RGB to color index */
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
      case GL_DEPTH_COMPONENT:
	index = GL_FALSE;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return GL_FALSE;
    }

    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __glSetError(GL_INVALID_ENUM);
	    return GL_FALSE;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      case GL_UNSIGNED_BYTE_3_3_2:
	switch (format) {
	  case GL_RGB:
	  case GL_BGR_EXT:
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
	break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_10_10_10_2:
	switch (format) {
	  case GL_RGBA:
	  case GL_ABGR_EXT:
	  case GL_BGRA_EXT:
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return GL_FALSE;
    }
    return GL_TRUE;
}

/* ---- px_api.c ---- */

/* 690bf970 -- __glim_PixelStorei (px_api.c, 1.1: no image height / skip images; the modes live in
   gc->state.pixel) */
void __stdcall __glim_PixelStorei(GLenum mode, GLint value)
{
    __GLpixelState *ps;
    __GL_SETUP_NOT_IN_BEGIN();

    ps = &gc->state.pixel;

    switch (mode) {
      case GL_PACK_ROW_LENGTH:
        if (value < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        ps->packModes.lineLength = value;
        break;
      case GL_PACK_SKIP_ROWS:
        if (value < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        ps->packModes.skipLines = value;
        break;
      case GL_PACK_SKIP_PIXELS:
        if (value < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        ps->packModes.skipPixels = value;
        break;
      case GL_PACK_ALIGNMENT:
        switch (value) {
          case 1: case 2: case 4: case 8:
            ps->packModes.alignment = value;
            break;
          default:
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        break;
      case GL_PACK_SWAP_BYTES:
        ps->packModes.swapEndian = (value != 0);
        break;
      case GL_PACK_LSB_FIRST:
        ps->packModes.lsbFirst = (value != 0);
        break;

      case GL_UNPACK_ROW_LENGTH:
        if (value < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        ps->unpackModes.lineLength = value;
        break;
      case GL_UNPACK_SKIP_ROWS:
        if (value < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        ps->unpackModes.skipLines = value;
        break;
      case GL_UNPACK_SKIP_PIXELS:
        if (value < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        ps->unpackModes.skipPixels = value;
        break;
      case GL_UNPACK_ALIGNMENT:
        switch (value) {
          case 1: case 2: case 4: case 8:
            ps->unpackModes.alignment = value;
            break;
          default:
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        break;
      case GL_UNPACK_SWAP_BYTES:
        ps->unpackModes.swapEndian = (value != 0);
        break;
      case GL_UNPACK_LSB_FIRST:
        ps->unpackModes.lsbFirst = (value != 0);
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_PIXEL);
}

/* 690bfbc0 -- __glim_PixelZoom (px_api.c; the rounded factors are integers) */
void __stdcall __glim_PixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    __GLpixelState *ps;
    GLint xtemp, ytemp;
    __GL_SETUP_NOT_IN_BEGIN();

    ps = &gc->state.pixel;

    /* Round xfactor and yfactor to fixed point accuracy. */
    if (xfactor > 0) {
        xtemp = (xfactor / gc->constants.viewportEpsilon) + __glHalf;
    } else {
        xtemp = (xfactor / gc->constants.viewportEpsilon) - __glHalf;
    }
    if (yfactor > 0) {
        ytemp = (yfactor / gc->constants.viewportEpsilon) + __glHalf;
    } else {
        ytemp = (yfactor / gc->constants.viewportEpsilon) - __glHalf;
    }
    xfactor = xtemp * gc->constants.viewportEpsilon;
    yfactor = ytemp * gc->constants.viewportEpsilon;

    ps->transferMode.zoomX = xfactor;
    ps->transferMode.zoomY = yfactor;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_PIXEL);
}

/* 690bfcb0 -- __glim_PixelTransferf (px_api.c, 1.1: scale, depth scale, bias, depth bias) */
void __stdcall __glim_PixelTransferf(GLenum mode, GLfloat value)
{
    __GLpixelState *ps;
    __GL_SETUP_NOT_IN_BEGIN();

    ps = &gc->state.pixel;

    switch (mode) {
      case GL_RED_SCALE:
        ps->transferMode.scale.r = value;
        break;
      case GL_GREEN_SCALE:
        ps->transferMode.scale.g = value;
        break;
      case GL_BLUE_SCALE:
        ps->transferMode.scale.b = value;
        break;
      case GL_ALPHA_SCALE:
        ps->transferMode.scale.a = value;
        break;
      case GL_DEPTH_SCALE:
        ps->transferMode.depthScale = value;
        break;
      case GL_RED_BIAS:
        ps->transferMode.bias.r = value;
        break;
      case GL_GREEN_BIAS:
        ps->transferMode.bias.g = value;
        break;
      case GL_BLUE_BIAS:
        ps->transferMode.bias.b = value;
        break;
      case GL_ALPHA_BIAS:
        ps->transferMode.bias.a = value;
        break;
      case GL_DEPTH_BIAS:
        ps->transferMode.depthBias = value;
        break;
      case GL_INDEX_SHIFT:
        /* Round */
        if (value > 0) {
            ps->transferMode.indexShift = (GLint) (value + __glHalf);
        } else {
            ps->transferMode.indexShift = (GLint) (value - __glHalf);
        }
        break;
      case GL_INDEX_OFFSET:
        /* Round */
        if (value > 0) {
            ps->transferMode.indexOffset = (GLint) (value + __glHalf);
        } else {
            ps->transferMode.indexOffset = (GLint) (value - __glHalf);
        }
        break;
      case GL_MAP_COLOR:
        ps->transferMode.mapColor = (value != __GL_ZERO_F);
        break;
      case GL_MAP_STENCIL:
        ps->transferMode.mapStencil = (value != __GL_ZERO_F);
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_PIXEL);
}

/* 690bfec0 -- __glim_PixelTransferi (px_api.c): the dispatch tables' glPixelTransferi slot (249; the header used
** to call it glPixelStorei, which is 690bf970) */
void __stdcall __glim_PixelTransferi(GLenum pname, GLint param)
{
    __glim_PixelTransferf(pname, (GLfloat) param);
}

/* 690c0700 -- __glCheckReadPixelArgs */
GLboolean __fastcall __glCheckReadPixelArgs(__GLcontext *gc, GLsizei width, GLsizei height,
				 GLenum format, GLenum type)
{
    if ((width < 0) || (height < 0)) {
	__glSetError(GL_INVALID_VALUE);
	return GL_FALSE;
    }
    switch (format) {
      case GL_STENCIL_INDEX:
	if (!gc->modes.haveStencilBuffer) {
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
	break;
      case GL_COLOR_INDEX:
	if (gc->modes.rgbMode) {
	    /* Can't convert RGB to color index */
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
	break;
      case GL_DEPTH_COMPONENT:
	if (!gc->modes.haveDepthBuffer) {
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_ABGR_EXT:
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return GL_FALSE;
    }
    switch (type) {
      case GL_BITMAP:
	if (format != GL_STENCIL_INDEX && format != GL_COLOR_INDEX) {
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      case GL_UNSIGNED_BYTE_3_3_2:
	switch (format) {
	  case GL_RGB:
	  case GL_BGR_EXT:
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
	break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_10_10_10_2:
	switch (format) {
	  case GL_RGBA:
	  case GL_ABGR_EXT:
	  case GL_BGRA_EXT:
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return GL_FALSE;
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return GL_FALSE;
    }
    return GL_TRUE;
}

/************************************************************************/
/* px_api.c */

/* 690c0a30 -- __glim_CopyPixels (the ICD revalidates and waits for the hardware before the copy) */
void __stdcall __glim_CopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    GLenum format;
    GLuint beginMode;
    __GL_SETUP();

    beginMode = __gl_beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    __gl_beginMode = __GL_NOT_IN_BEGIN;
	    glCopyPixels(x,y,width,height,type);
	    return;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
    }

    if ((width < 0) || (height < 0)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    switch (type) {
      case GL_STENCIL:
	if (!gc->modes.haveStencilBuffer) {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	format = GL_STENCIL_INDEX;
	break;
      case GL_COLOR:
	if (gc->modes.rgbMode) {
	    format = GL_RGBA;
	} else {
	    format = GL_COLOR_INDEX;
	}
	break;
      case GL_DEPTH:
	if (!gc->modes.haveDepthBuffer) {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	format = GL_DEPTH_COMPONENT;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    if (!gc->state.current.validRasterPos) {
	return;
    }

    if (gc->renderMode == GL_FEEDBACK) {
	__glFeedbackCopyPixels(gc, &gc->state.current.rasterPos);
	return;
    }

    if (gc->renderMode != GL_RENDER) return;

    (*gc->procs.revalidate)(gc);
    (*gc->procs.waitIdle)(gc);
    (*gc->procs.copyPixels)(gc, x, y, width, height, format);
}

/* 690bf8b0 -- __glim_PixelStoref (px_api.c, 1.1).  As the original: the boolean modes have no
** break, so __glim_PixelStorei is called again with (GLint) value.  __glim_PixelStorei is called
** as a stack-only entry (its ECX / EDX are dummies).
*/
void __stdcall __glim_PixelStoref(GLenum mode, GLfloat value)
{
    switch (mode) {
      case GL_PACK_ROW_LENGTH:
      case GL_PACK_SKIP_ROWS:
      case GL_PACK_SKIP_PIXELS:
      case GL_PACK_ALIGNMENT:
      case GL_UNPACK_ROW_LENGTH:
      case GL_UNPACK_SKIP_ROWS:
      case GL_UNPACK_SKIP_PIXELS:
      case GL_UNPACK_ALIGNMENT:
        /* Round */
        if (value < 0) {
            __glim_PixelStorei(mode, (GLint) (value - (__GLfloat) 0.5));
        } else {
            __glim_PixelStorei(mode, (GLint) (value + (__GLfloat) 0.5));
        }
        break;
      case GL_PACK_SWAP_BYTES:
      case GL_PACK_LSB_FIRST:
      case GL_UNPACK_SWAP_BYTES:
      case GL_UNPACK_LSB_FIRST:
        if (value == __GL_ZERO_F) {
            __glim_PixelStorei(mode, GL_FALSE);
        } else {
            __glim_PixelStorei(mode, GL_TRUE);
        }
        /* no break (original) */
      default:
        __glim_PixelStorei(mode, (GLint) value);
        break;
    }
}

/* 69083cd0 -- px_rend.c __glSpanRenderDepthUint (the depth resolution is gc->modes.depthBits) */
void __fastcall __glSpanRenderDepthUint(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLuint *spanData;
    __GLfragmentICD frag;
    __GLstoreProc store;
    GLint rows;
    GLint startCol;
    GLshort *pixelArray;
    __WGLdepthPrivate *db = __GL_DEPTH_DP(gc);
    GLint shiftAmt = 32 - gc->modes.depthBits - db->numFracBits;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;

    frag.color.r = gc->state.current.rasterPos.colors[0].r * gc->constants.redRescale;
    frag.color.g = gc->state.current.rasterPos.colors[0].g * gc->constants.greenRescale;
    frag.color.b = gc->state.current.rasterPos.colors[0].b * gc->constants.blueRescale;
    frag.color.a = gc->state.current.rasterPos.colors[0].a * gc->constants.alphaRescale;
    store = (__GLstoreProc)gc->procs.pxStore;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
	if (rows == 0) break;
	rows--;
	column = startCol;
	pixelArray = spanInfo->pixelArray;
	spanData = (const GLuint *) span;
	frag.y = row;

	for (i=0; i<width; i++) {
	    iright = column + *pixelArray++;
	    frag.z = (*spanData++) >> shiftAmt;
	    do {
		frag.x = column;

		/* This procedure will do the rest */
		(*store)(gc->drawBuffer, &frag);
		column += coladd;
	    } while (column != iright);
	}
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/*
** gl_pxpaths.c -- px_paths.c: the pixel-path helpers (DrawPixels / ReadPixels / CopyPixels, texture image store
** / fetch) (SI port: si_px_paths.c; the path picker is gl_pxpick.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "siprotos.h"
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "icdglob.h"
#include "icdprotos.h"
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <string.h>

/* ---- px_paths.c: __glDrawPixelsN (the span-skip loop returns once the image is used up) -------- */
typedef void (__fastcall *__GLspanModProc)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                         const GLvoid *inspan, GLvoid *outspan);
typedef void (__fastcall *__GLspanRenderProc)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                            const GLvoid *inspan);
/* ---- px_paths.c: the zoomed DrawPixels loops (the ICD leaves the loop as soon as the skipped rows
** reach height: no extra span is drawn where the SI only asserts) ------------------------------- */
typedef void (__fastcall *__GLspanMod)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                           const GLvoid *inspan, GLvoid *outspan);
typedef void (__fastcall *__GLspanRender)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                              const GLvoid *inspan);
/* 69086aa0 (__glGenericPickDrawPixels) is in gl_pxpick.c, in front of PickSpanModifiers as in px_paths.c */
/* ---- px_paths.c: __glGenericPickReadPixels (SI 1.1: the span-mod picker also returns the reader) */
typedef void (__fastcall *__GLpixelPathProc)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo);
typedef __GLpixelSpanInfoICD __GLpixelSpanInfo;
typedef void (__fastcall *__GLdrawPixelsProc)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo);
/* fetchTex / storeTex / readImage get the texture object: its texFormat's storeSpan / fetchSpan are the span
** modifiers of the store / fetch side */
typedef void (__fastcall *__GLspanModProcICD)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
					const GLvoid *inspan, GLvoid *outspan);
#define SPANMOD(p)          ((__GLspanModProcICD)(p))
/* ---- px_paths.c: memory sources / destinations of the texture paths ------------------------------ */
#define __GL_RED_ALPHA      1
extern void __fastcall __glDrawPixels0(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo);

/* ---- px_paths.c: __glInitDrawPixelsInfo (no half-pixel offset) --------------------------------------- */
/* 69086390 -- __glInitDrawPixelsInfo */
void __fastcall __glInitDrawPixelsInfo(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             GLint width, GLint height, GLenum format,
                             GLenum type, const GLvoid *pixels)
{
    __GLfloat x,y;
    __GLfloat zoomx, zoomy;

    x = gc->state.current.rasterPos.window.x;
    y = gc->state.current.rasterPos.window.y;

    spanInfo->x = x;
    spanInfo->y = y;
    spanInfo->fragz = (__GLzValue) (gc->constants.depthRescale *
                                    gc->state.current.rasterPos.window.z);
    zoomx = gc->state.pixel.transferMode.zoomX;
    if (zoomx > __GL_ZERO_F) {
        if (zoomx < __GL_ONE_F) {
            spanInfo->rendZoomx = __GL_ONE_F;
        } else {
            spanInfo->rendZoomx = zoomx;
        }
        spanInfo->coladd = 1;
    } else {
        if (zoomx > (GLfloat) -1.0) {
            spanInfo->rendZoomx = (GLfloat) -1.0;
        } else {
            spanInfo->rendZoomx = zoomx;
        }
        spanInfo->coladd = -1;
    }
    spanInfo->zoomx = zoomx;
    zoomy = gc->state.pixel.transferMode.zoomY;
    if (gc->constants.yInverted) {
        zoomy = -zoomy;
    } else {
        spanInfo->y += gc->constants.viewportEpsilon;
    }
    if (zoomy > __GL_ZERO_F) {
        spanInfo->rowadd = 1;
    } else {
        spanInfo->rowadd = -1;
    }
    spanInfo->zoomy = zoomy;
    spanInfo->width = width;
    spanInfo->height = height;
    if (format == GL_COLOR_INDEX && gc->modes.rgbMode) {
        spanInfo->dstFormat = GL_RGBA;
    } else {
        spanInfo->dstFormat = format;
    }
    spanInfo->srcFormat = format;
    spanInfo->srcType = type;
    spanInfo->srcImage = pixels;

    spanInfo->dstType = GL_FLOAT;
    spanInfo->dstElementSize = 4;
    spanInfo->dstSwapBytes = GL_FALSE;
    spanInfo->dstImage = NULL;
    spanInfo->dim = 2;
}

/* 690864f0 -- __glDrawPixelsN */
void __fastcall __glDrawPixelsN(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    int i;
    __GLfloat zoomy, newy;
    GLint inty, height;
    GLint numSpanMods = spanInfo->numSpanMods;
    GLint spanModNum;
    __GLspanModProc *spanModifier;
    __GLspanRenderProc render;
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
    GLubyte *tmpSpanData, *spanDataIn, *spanDataOut;

    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    spanModifier = (__GLspanModProc *)spanInfo->spanModifier;
    render = (__GLspanRenderProc)spanInfo->spanRender;

    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;

    for (i=0; i<height; i++) {
        spanInfo->y = newy;
        newy += zoomy;
        while ((GLint) newy == inty && i<height) {
            spanInfo->y = newy;
            spanInfo->srcCurrent = (const GLubyte *) spanInfo->srcCurrent +
                    spanInfo->srcRowIncrement;
            newy += zoomy;
            i++;
            if (i >= height) return;
        }
        inty = (GLint) newy;

        (*spanModifier[0])(gc, spanInfo, spanInfo->srcCurrent, spanData1);
        spanInfo->srcCurrent = (const GLubyte *) spanInfo->srcCurrent +
                spanInfo->srcRowIncrement;
        spanDataIn = spanData1; spanDataOut = spanData2;
        for (spanModNum = 1; spanModNum < numSpanMods; spanModNum++) {
            (*spanModifier[spanModNum])(gc, spanInfo, spanDataIn, spanDataOut);
            tmpSpanData = spanDataIn;
            spanDataIn = spanDataOut;
            spanDataOut = tmpSpanData;
        }
        (*render)(gc, spanInfo, spanDataIn);
    }
}

/* 69086670 -- __glDrawPixels2 */
void __fastcall __glDrawPixels2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    int i;
    __GLfloat zoomy, newy;
    GLint inty, height;
    __GLspanMod span1, span2;
    __GLspanRender render;
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    render = spanInfo->spanRender;

    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;

    for (i = 0; i < height; i++) {
	spanInfo->y = newy;
	newy += zoomy;
	while ((GLint) newy == inty && i < height) {
	    spanInfo->y = newy;
	    spanInfo->srcCurrent = (const GLubyte *) spanInfo->srcCurrent +
		    spanInfo->srcRowIncrement;
	    newy += zoomy;
	    i++;
	    if (i >= height) return;
	}
	inty = (GLint) newy;
	(*span1)(gc, spanInfo, spanInfo->srcCurrent, spanData1);
	spanInfo->srcCurrent = (const GLubyte *) spanInfo->srcCurrent +
		spanInfo->srcRowIncrement;
	(*span2)(gc, spanInfo, spanData1, spanData2);
	(*render)(gc, spanInfo, spanData2);
    }
}

/* 690867b0 -- __glDrawPixels1 */
void __fastcall __glDrawPixels1(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    int i;
    __GLfloat zoomy, newy;
    GLint inty, height;
    __GLspanMod span1;
    __GLspanRender render;
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    span1 = spanInfo->spanModifier[0];
    render = spanInfo->spanRender;

    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;

    for (i = 0; i < height; i++) {
	spanInfo->y = newy;
	newy += zoomy;
	while ((GLint) newy == inty && i < height) {
	    spanInfo->y = newy;
	    spanInfo->srcCurrent = (const GLubyte *) spanInfo->srcCurrent +
		    spanInfo->srcRowIncrement;
	    newy += zoomy;
	    i++;
	    if (i >= height) return;
	}
	inty = (GLint) newy;
	(*span1)(gc, spanInfo, spanInfo->srcCurrent, spanData1);
	spanInfo->srcCurrent = (const GLubyte *) spanInfo->srcCurrent +
		spanInfo->srcRowIncrement;
	(*render)(gc, spanInfo, spanData1);
    }
}

/* 690868d0 -- __glDrawPixels0 (px_paths.c); the SI's assert(i != height) is a return here */
void __fastcall __glDrawPixels0(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    int i;
    __GLfloat zoomy, newy;
    GLint inty, height;
    void (__fastcall *render)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			      const GLvoid *inspan);
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    render = spanInfo->spanRender;

    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;

    for (i=0; i<height; i++) {
	spanInfo->y = newy;
	newy += zoomy;
	/*
	** Skip over spans when zoomy is in the range (-1,1)
	*/
	while ((GLint) newy == inty && i<height) {
	    spanInfo->y = newy;
	    spanInfo->srcCurrent = (const GLubyte *) spanInfo->srcCurrent + 
		    spanInfo->srcRowIncrement;
	    newy += zoomy;
	    i++;
	    if (i >= height) return;
	}
	inty = (GLint) newy;
	(*render)(gc, spanInfo, spanInfo->srcCurrent);
	spanInfo->srcCurrent = (const GLubyte *) spanInfo->srcCurrent + 
		spanInfo->srcRowIncrement;
    }
}

/* 690888b0 -- __glLoadPackModes (no 3D image fields) */
void __fastcall __glLoadPackModes(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    GLint lineLength = gc->state.pixel.packModes.lineLength;

    spanInfo->dstAlignment = gc->state.pixel.packModes.alignment;
    spanInfo->dstSkipPixels = gc->state.pixel.packModes.skipPixels;
    spanInfo->dstSkipLines = gc->state.pixel.packModes.skipLines;
    spanInfo->dstLsbFirst = gc->state.pixel.packModes.lsbFirst;
    spanInfo->dstSwapBytes = gc->state.pixel.packModes.swapEndian;
    if (lineLength <= 0) lineLength = spanInfo->width;
    spanInfo->dstLineLength = lineLength;
}

/* 69088900 -- __glInitReadPixelsInfo (px_paths.c): no line-length fix-up, the unpacked flag
** cleared, dim (+0x16c) = 2 */
void __fastcall __glInitReadPixelsInfo(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     GLint x, GLint y, GLint width, GLint height,
			     GLenum format, GLenum type, GLvoid *pixels)
{
    spanInfo->readX = (__GLfloat) (x + gc->constants.viewportXAdjust);
    if (gc->constants.yInverted) {
	spanInfo->readY = (__GLfloat) (GLuint) ((gc->constants.height - y - 1) + 
		gc->constants.viewportYAdjust);
    } else {
	spanInfo->readY = (__GLfloat) (y + gc->constants.viewportYAdjust);
    }
    spanInfo->width = width;
    spanInfo->height = height;
    spanInfo->dstFormat = format;
    spanInfo->dstType = type;
    spanInfo->dstImage = pixels;
    spanInfo->zoomx = __GL_ONE_F;
    spanInfo->x = __GL_ZERO_F;
    __glLoadPackModes(gc, spanInfo);
    /*
    ** These aren't needed for ReadPixels, but they are read by the
    ** generic pixel path setup code, so something needs to be set
    ** in them.
    */
    spanInfo->srcType = GL_FLOAT;
    spanInfo->srcElementSize = 4;
    spanInfo->srcSwapBytes = GL_FALSE;
    spanInfo->srcImage = NULL;
    spanInfo->srcPackedData = GL_FALSE;
    spanInfo->srcFormat = gc->modes.rgbMode ? GL_RGBA : GL_COLOR_INDEX;
    spanInfo->dim = 2;
}

/* 69088d60 -- __glGenericPickReadPixels */
void __fastcall __glGenericPickReadPixels(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    __GLpixelPathProc rpfn;
    __GLpixelSpanModInfo spanModInfo;

    spanModInfo.srcData = __GL_PIXDATA_FB;
    spanModInfo.dstData = __GL_PIXDATA_MEM;
    spanModInfo.pixelPath = __GL_PIXPATH_READPIX;
    spanModInfo.applyPixelTransfer = GL_TRUE;

    spanInfo->numSpanMods = 0;
    PickSpanModifiers(gc, spanInfo, &spanModInfo);
    spanInfo->spanReader = (void (__fastcall *)(struct __GLcontextRec *, struct __GLpixelSpanInfoICDRec *, GLvoid *))
        spanModInfo.reader;

    switch(spanInfo->numSpanMods) {
    case 0:
        rpfn = __glReadPixels0;
        break;
    case 1:
        rpfn = __glReadPixels1;
        break;
    case 2:
        rpfn = __glReadPixels2;
        break;
    default:
        rpfn = __glReadPixelsN;
        break;
    }

    (*rpfn)(gc, spanInfo);
}

/* 69089390 -- px_paths.c __glInitCopyPixelsInfo */
void __fastcall __glInitCopyPixelsInfo(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     GLint x, GLint y, GLint width, GLint height, GLenum format)
{
    __GLfloat rpx, rpy;
    __GLfloat zoomx, zoomy;

    rpx = gc->state.current.rasterPos.window.x;
    rpy = gc->state.current.rasterPos.window.y;

    spanInfo->x = rpx;
    spanInfo->y = rpy;
    spanInfo->fragz = (__GLzValue) (gc->state.current.rasterPos.window.z *
						gc->constants.depthRescale);
    zoomx = gc->state.pixel.transferMode.zoomX;
    if (zoomx > __GL_ZERO_F) {
	if (zoomx < __GL_ONE_F) {
	    spanInfo->rendZoomx = __GL_ONE_F;
	} else {
	    spanInfo->rendZoomx = zoomx;
	}
	spanInfo->coladd = 1;
    } else {
	if (zoomx > (GLfloat) -1.0) {
	    spanInfo->rendZoomx = (GLfloat) -1.0;
	} else {
	    spanInfo->rendZoomx = zoomx;
	}
	spanInfo->coladd = -1;
    }
    spanInfo->zoomx = zoomx;
    zoomy = gc->state.pixel.transferMode.zoomY;
    if (gc->constants.yInverted) {
	zoomy = -zoomy;
    } else {
	spanInfo->y += gc->constants.viewportEpsilon;
    }
    if (zoomy > __GL_ZERO_F) {
	spanInfo->rowadd = 1;
    } else {
	spanInfo->rowadd = -1;
    }
    spanInfo->zoomy = zoomy;
    spanInfo->readX = (__GLfloat) (x + gc->constants.viewportXAdjust);
    if (gc->constants.yInverted) {
	spanInfo->readY = (__GLfloat) (((GLuint)gc->constants.height - y - 1) +
		gc->constants.viewportYAdjust);
    } else {
	spanInfo->readY = (__GLfloat) (y + gc->constants.viewportYAdjust);
    }
    spanInfo->dstFormat = spanInfo->srcFormat = format;
    spanInfo->width = width;
    spanInfo->height = height;
    spanInfo->srcType = GL_FLOAT;
    spanInfo->srcElementSize = 4;
    spanInfo->srcSwapBytes = GL_FALSE;
    spanInfo->srcImage = NULL;
    spanInfo->srcPackedData = GL_FALSE;
    spanInfo->srcFormat = gc->modes.rgbMode ? GL_RGBA : GL_COLOR_INDEX;
    spanInfo->dstType = GL_FLOAT;
    spanInfo->dstElementSize = 4;
    spanInfo->dstSwapBytes = GL_FALSE;
    spanInfo->dstImage = NULL;
    spanInfo->applyFlags[0] = GL_FALSE;
    spanInfo->applyFlags[1] = GL_TRUE;
    spanInfo->applyFlags[2] = GL_TRUE;
    spanInfo->applyFlags[3] = GL_FALSE;
    spanInfo->dim = 2;
}

/* 6908a300 -- __glGenericPickCopyPixels (px_paths.c): reader and render taken from the pick */
void __fastcall __glGenericPickCopyPixels(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    __GLpixelSpanModInfo spanModInfo;
    __GLdrawPixelsProc cpfn;

    spanModInfo.srcData = __GL_PIXDATA_FB;
    spanModInfo.dstData = __GL_PIXDATA_FB;
    spanModInfo.applyPixelTransfer = GL_TRUE;
    spanModInfo.pixelPath = __GL_PIXPATH_COPYPIX;

    spanInfo->numSpanMods = 0;
    PickSpanModifiers(gc, spanInfo, &spanModInfo);
    spanInfo->spanReader = (void (__fastcall *)(__GLcontext *, __GLpixelSpanInfoICD *,
						GLvoid *))spanModInfo.reader;
    spanInfo->spanRender = (void (__fastcall *)(__GLcontext *, __GLpixelSpanInfoICD *,
						const GLvoid *))spanModInfo.render;

    switch(spanInfo->numSpanMods) {
    case 0:
	cpfn = __glCopyPixels0;
	break;
    case 1:
	cpfn = __glCopyPixels1;
	break;
    case 2:
	cpfn = __glCopyPixels2;
	break;
    default:
	cpfn = __glCopyPixelsN;
	break;
    }
    (*cpfn)(gc, spanInfo);
}

/* 6908a550 -- __glGenericPickCopyImage (px_paths.c), procs.copyImage: a span copy is the one
** modifier when nothing else is needed.  The ICD's "add a modifier" stores numSpanMods back
** unchanged (a numSpanMods = numSpanMods++ that VC6 folds to a re-store); kept. */
void __fastcall __glGenericPickCopyImage(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     GLboolean applyPixelTransfer)
{
    __GLpixelSpanModInfo spanModInfo;
    __GLdrawPixelsProc cpfn;

    spanModInfo.srcData = __GL_PIXDATA_MEM;
    spanModInfo.dstData = __GL_PIXDATA_MEM;
    spanModInfo.pixelPath = __GL_PIXPATH_COPYIMAGE;
    spanModInfo.applyPixelTransfer = applyPixelTransfer;

    spanInfo->numSpanMods = 0;
    PickSpanModifiers(gc, spanInfo, &spanModInfo);

    switch(spanInfo->numSpanMods) {
    case 0:
	spanInfo->spanModifier[spanInfo->numSpanMods] = __glSpanCopy;
	spanInfo->numSpanMods = spanInfo->numSpanMods++;
	cpfn = __glCopyImage1;
	break;
    case 1:
	cpfn = __glCopyImage1;
	break;
    case 2:
	cpfn = __glCopyImage2;
	break;
    default:
	cpfn = __glCopyImageN;
	break;
    }
    (*cpfn)(gc, spanInfo);
}

/* 6908a5f0 -- __glGenericStoreTexImage: procs.storeTex, the copy-image pick with the image's store converter
** appended */
void __fastcall __glGenericStoreTexImage(__GLcontext *gc, __GLtextureICD *image,
			     __GLpixelSpanInfoICD *spanInfo, GLboolean applyPixelTransfer)
{
    __GLpixelSpanModInfo spanModInfo;
    __GLdrawPixelsProc cpfn;
    __GLproc store;

    spanModInfo.srcData = __GL_PIXDATA_MEM;
    spanModInfo.dstData = __GL_PIXDATA_MEM;
    spanModInfo.pixelPath = __GL_PIXPATH_COPYIMAGE;
    spanModInfo.applyPixelTransfer = applyPixelTransfer;

    spanInfo->numSpanMods = 0;
    PickSpanModifiers(gc, spanInfo, &spanModInfo);
    store = image->texFormat->storeSpan;
    if (store) {
	spanInfo->spanModifier[spanInfo->numSpanMods] = SPANMOD(store);
	spanInfo->numSpanMods = spanInfo->numSpanMods++;
    }

    switch(spanInfo->numSpanMods) {
    case 0:
	spanInfo->spanModifier[spanInfo->numSpanMods] = __glSpanCopy;
	spanInfo->numSpanMods = spanInfo->numSpanMods++;
	cpfn = __glCopyImage1;
	break;
    case 1:
	cpfn = __glCopyImage1;
	break;
    case 2:
	cpfn = __glCopyImage2;
	break;
    default:
	cpfn = __glCopyImageN;
	break;
    }
    (*cpfn)(gc, spanInfo);
}

/* 6908a6c0 -- __glGenericFetchTexImage: procs.fetchTex, the image's fetch converter first, then the copy-image
** pick */
void __fastcall __glGenericFetchTexImage(__GLcontext *gc, __GLtextureICD *image,
			     __GLpixelSpanInfoICD *spanInfo, GLboolean applyPixelTransfer)
{
    __GLpixelSpanModInfo spanModInfo;
    __GLdrawPixelsProc cpfn;
    __GLproc fetch;

    spanModInfo.srcData = __GL_PIXDATA_MEM;
    spanModInfo.dstData = __GL_PIXDATA_MEM;
    spanModInfo.pixelPath = __GL_PIXPATH_COPYIMAGE;
    spanModInfo.applyPixelTransfer = applyPixelTransfer;

    spanInfo->numSpanMods = 0;
    fetch = image->texFormat->fetchSpan;
    if (fetch) {
	spanInfo->spanModifier[spanInfo->numSpanMods++] = SPANMOD(fetch);
    }
    PickSpanModifiers(gc, spanInfo, &spanModInfo);

    switch(spanInfo->numSpanMods) {
    case 0:
	spanInfo->spanModifier[spanInfo->numSpanMods++] = __glSpanCopy;
	cpfn = __glCopyImage1;
	break;
    case 1:
	cpfn = __glCopyImage1;
	break;
    case 2:
	cpfn = __glCopyImage2;
	break;
    default:
	cpfn = __glCopyImageN;
	break;
    }
    (*cpfn)(gc, spanInfo);
}

/* 6908a790 -- __glInitReadImageSrcInfo (px_paths.c), the SI 1.1 form without the 3D-image fields */
void __fastcall __glInitReadImageSrcInfo(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             GLint x, GLint y, GLsizei width, GLsizei height)
{
    spanInfo->readX = (__GLfloat) (x + gc->constants.viewportXAdjust);
    if (gc->constants.yInverted) {
        spanInfo->readY = (__GLfloat) (GLuint) ((gc->constants.height - y - 1) +
                gc->constants.viewportYAdjust);
    } else {
        spanInfo->readY = (__GLfloat) (y + gc->constants.viewportYAdjust);
    }
    spanInfo->srcFormat = gc->modes.rgbMode ? GL_RGBA : GL_COLOR_INDEX;
    spanInfo->srcType = GL_FLOAT;
    spanInfo->width = width;
    spanInfo->height = height;
    spanInfo->zoomx = __GL_ONE_F;
    spanInfo->x = __GL_ZERO_F;

    spanInfo->srcAlignment = 1;
    spanInfo->srcSkipPixels = 0;
    spanInfo->srcSkipLines = 0;
    spanInfo->srcLsbFirst = GL_FALSE;
    spanInfo->srcSwapBytes = GL_FALSE;
    spanInfo->srcLineLength = spanInfo->width;
    spanInfo->srcType = GL_FLOAT;
    spanInfo->srcElementSize = 4;
    spanInfo->srcSwapBytes = GL_FALSE;
    spanInfo->srcImage = NULL;
    spanInfo->srcPackedData = GL_FALSE;
}

/* 6908a870 -- __glGenericReadTexImage: procs.readImage, the read-pixels pick into memory with the image's
** store converter
** appended, the reader taken from the pick */
void __fastcall __glGenericReadTexImage(__GLcontext *gc, __GLtextureICD *image,
			     __GLpixelSpanInfoICD *spanInfo)
{
    __GLpixelSpanModInfo spanModInfo;
    __GLdrawPixelsProc rpfn;
    __GLproc store;

    spanModInfo.srcData = __GL_PIXDATA_FB;
    spanModInfo.applyPixelTransfer = GL_TRUE;
    spanModInfo.dstData = __GL_PIXDATA_MEM;
    spanModInfo.pixelPath = __GL_PIXPATH_READIMAGE;

    spanInfo->numSpanMods = 0;
    PickSpanModifiers(gc, spanInfo, &spanModInfo);
    store = image->texFormat->storeSpan;
    if (store) {
	spanInfo->spanModifier[spanInfo->numSpanMods++] = SPANMOD(store);
    }
    spanInfo->spanReader = (void (__fastcall *)(__GLcontext *, __GLpixelSpanInfoICD *,
						GLvoid *))spanModInfo.reader;

    switch(spanInfo->numSpanMods) {
    case 0:
	rpfn = __glReadPixels0;
	break;
    case 1:
	rpfn = __glReadPixels1;
	break;
    case 2:
	rpfn = __glReadPixels2;
	break;
    default:
	rpfn = __glReadPixelsN;
	break;
    }
    (*rpfn)(gc, spanInfo);
}

/* 6908a950 -- __glInitMemLoad */
void __fastcall __glInitMemLoad(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLenum internalFormat, GLvoid *buf)
{
    spanInfo->scaleBias[0] = 1.0f;         /* rscale .. ascale */
    spanInfo->scaleBias[1] = 1.0f;
    spanInfo->scaleBias[2] = 1.0f;
    spanInfo->scaleBias[3] = 1.0f;
    spanInfo->scaleBias[4] = 0.0f;         /* rbias .. abias */
    spanInfo->scaleBias[5] = 0.0f;
    spanInfo->scaleBias[6] = 0.0f;
    spanInfo->scaleBias[7] = 0.0f;

    spanInfo->dstImage = buf;
    spanInfo->dstSkipPixels = 0;
    spanInfo->dstSkipLines = 0;
    spanInfo->dstSwapBytes = GL_FALSE;
    spanInfo->dstLsbFirst = GL_TRUE;
    spanInfo->dstLineLength = spanInfo->width;
    spanInfo->dim = 0;                  /* 3dfx PX_PATHS.C: dim = 0 (don't convolve); SI 1.2 stores dstSkipImages here */

    switch(internalFormat) {
      case GL_LUMINANCE:
	spanInfo->dstFormat = GL_RED;
	spanInfo->dstType = GL_FLOAT;
	spanInfo->dstAlignment = 4;
	break;
      case GL_LUMINANCE_ALPHA:
	spanInfo->dstFormat = __GL_RED_ALPHA;
	spanInfo->dstType = GL_FLOAT;
	spanInfo->dstAlignment = 4;
	break;
      case GL_RGB:
	spanInfo->dstFormat = GL_RGB;
	spanInfo->dstType = GL_FLOAT;
	spanInfo->dstAlignment = 4;
	break;
      case GL_RGBA:
	spanInfo->dstFormat = GL_RGBA;
	spanInfo->dstType = GL_FLOAT;
	spanInfo->dstAlignment = 4;
	break;
      case GL_ALPHA:
	spanInfo->dstFormat = GL_ALPHA;
	spanInfo->dstType = GL_FLOAT;
	spanInfo->dstAlignment = 4;
	break;
      case GL_INTENSITY:
	spanInfo->dstFormat = GL_RED;
	spanInfo->dstType = GL_FLOAT;
	spanInfo->dstAlignment = 4;
	break;
    }
}

/* 6908aa70 -- __glInitMemGet */
void __fastcall __glInitMemGet(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     GLsizei width, GLsizei height, GLenum internalFormat, const GLvoid *buf)
{
    spanInfo->srcImage = buf;
    spanInfo->srcSkipPixels = 0;
    spanInfo->srcSkipLines = 0;
    spanInfo->srcSwapBytes = GL_FALSE;
    spanInfo->dim = 0;                  /* SI 1.2 stores srcSkipImages here */
    spanInfo->srcLsbFirst = GL_TRUE;
    spanInfo->srcLineLength = width;

    switch(internalFormat) {
      case GL_LUMINANCE:
	spanInfo->srcFormat = GL_RED;
	spanInfo->srcType = GL_FLOAT;
	spanInfo->srcAlignment = 4;
	break;
      case GL_LUMINANCE_ALPHA:
	spanInfo->srcFormat = __GL_RED_ALPHA;
	spanInfo->srcType = GL_FLOAT;
	spanInfo->srcAlignment = 4;
	break;
      case GL_RGB:
	spanInfo->srcFormat = GL_RGB;
	spanInfo->srcType = GL_FLOAT;
	spanInfo->srcAlignment = 4;
	break;
      case GL_RGBA:
	spanInfo->srcFormat = GL_RGBA;
	spanInfo->srcType = GL_FLOAT;
	spanInfo->srcAlignment = 4;
	break;
      case GL_ALPHA:
	spanInfo->srcFormat = GL_ALPHA;
	spanInfo->srcType = GL_FLOAT;
	spanInfo->srcAlignment = 4;
	break;
      case GL_INTENSITY:
	spanInfo->srcFormat = GL_RED;
	spanInfo->srcType = GL_FLOAT;
	spanInfo->srcAlignment = 4;
	break;
    }
}

/************************************************************************/
/* px_paths.c-style span converters (siblings in gl_pxpack.c) */

/* 690c89f0 -- __glSpanPackRGBA8888ToARGB8888: 32-bit span rotated right by eight */
void __fastcall __glSpanPackRGBA8888ToARGB8888(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;

    for (i=0; i<width; i++) {
	*outData++ = (*inData << 24) | (*inData >> 8);
	inData++;
    }
}

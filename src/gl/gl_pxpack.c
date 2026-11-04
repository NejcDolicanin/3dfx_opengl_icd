/*
** gl_pxpack.c -- px_pack.c / px_upack.c: span packers and unpackers of the pixel paths (SI
** ports: si_px_pack.c, si_px_upack.c).
*/
#include <stdlib.h>
#include <string.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "og.h"
#include "icdprotos.h"
#include <math.h>
#include "icdglob.h"

/* ---- px_pack.c / px_upack.c: the 1.1 packer / unpacker setup (no 3D image parameters) ---- */

/* 690c6bb0 -- __glInitPacker (px_pack.c) */
void __fastcall __glInitPacker(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    GLint alignment;
    GLint components;
    GLint element_size;
    GLint rowsize;
    GLint padding;
    GLint groupsize;
    GLint groups_per_line;
    GLint skip_pixels, skip_lines;
    GLint swap_bytes;
    GLenum format, type;
    GLvoid *pixels;

    format = spanInfo->dstFormat;
    type = spanInfo->dstType;
    pixels = spanInfo->dstImage;
    skip_pixels = spanInfo->dstSkipPixels;
    skip_lines = spanInfo->dstSkipLines;
    alignment = spanInfo->dstAlignment;
    swap_bytes = spanInfo->dstSwapBytes;

    components = __glElementsPerGroup(format, type);
    groups_per_line = spanInfo->dstLineLength;

    element_size = __glBytesPerElement(type);
    if (element_size == 1) swap_bytes = 0;
    groupsize = element_size * components;

    rowsize = groups_per_line * groupsize;
    if (type == GL_BITMAP) {
        rowsize = (groups_per_line + 7)/8;
    }
    padding = (rowsize % alignment);
    if (padding) {
        rowsize += alignment - padding;
    }
    if (((skip_pixels & 0x7) && type == GL_BITMAP) ||
            (swap_bytes && element_size > 1)) {
        spanInfo->dstPackedData = GL_FALSE;
    } else {
        spanInfo->dstPackedData = GL_TRUE;
    }

    if (type == GL_BITMAP) {
        spanInfo->dstCurrent = (GLvoid *) (((GLubyte*) pixels) +
                skip_lines * rowsize + skip_pixels / 8);
        spanInfo->dstStartBit = skip_pixels % 8;
    } else {
        spanInfo->dstCurrent = (GLvoid *) (((GLubyte*) pixels) +
                skip_lines * rowsize + skip_pixels * groupsize);
    }
    spanInfo->dstRowIncrement = rowsize;
    spanInfo->dstGroupIncrement = groupsize;
    spanInfo->dstComponents = components;
    spanInfo->dstElementSize = element_size;
    spanInfo->applyFlags[0] = GL_FALSE;
    spanInfo->applyFlags[1] = GL_TRUE;
    spanInfo->applyFlags[2] = GL_TRUE;
    spanInfo->applyFlags[3] = GL_FALSE;
}

/* 690c7390 -- __glSpanPackUbyteRound: px_pack.c __glSpanPackUbyte with the correct rounding (+ half) */
void __fastcall __glSpanPackUbyteRound(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
	*outData++ = (GLubyte)(*inData++ * gc->constants.val255 + gc->constants.half);
    }
}

/* ---- pixel span modifiers ---- */

/* 690c7900 -- __glSpanPackXRGB8888Uint: float RGBA span -> 0xffRRGGBB words (px_pack.c __glSpanPack8888Uint
** style; alpha
   skipped and forced opaque) */
void __fastcall __glSpanPackXRGB8888Uint(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLuint *outData = (GLuint *) outspan;
    GLint totalSize = width;
    GLfloat invalue;

    for (i=0; i<totalSize; i++) {
        invalue = *inData++;
        *outData = ((GLuint)(invalue * 255) << 16) & 0x00FF0000;
        invalue = *inData++;
        *outData |= ((GLuint)(invalue * 255) << 8) & 0x0000FF00;
        invalue = *inData++;
        *outData |= ((GLuint)(invalue * 255)) & 0x000000FF;
        inData++;
        *outData |= 0xFF000000;
        outData++;
    }
}

/* 690c8850 -- __glSpanPackRGB8ToXRGB8888: RGB ubyte span to opaque 0xffRRGGBB words */
void __fastcall __glSpanPackRGB8ToXRGB8888(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLuint *outData = (GLuint *) outspan;
    const GLubyte *inData = (const GLubyte *) inspan;

    for (i=0; i<width; i++) {
	*outData = (((inData[0] | 0xffffff00) << 8) | inData[1]) << 8 | inData[2];
	inData += 3;
	outData++;
    }
}

/* 690c8940 -- __glSpanPackRGBA5551ToARGB1555: 16-bit span rotated right by one (5551 <-> 1555) */
void __fastcall __glSpanPackRGBA5551ToARGB1555(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLushort *inData = (const GLushort *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLuint v;

    for (i=0; i<width; i++) {
	v = *inData;
	*outData++ = (GLushort)((v >> 1) | (v << 15));
	inData++;
    }
}

/* 690c8980 -- __glSpanPackRGBA5551ToXRGB1555: 16-bit span shifted right by one */
void __fastcall __glSpanPackRGBA5551ToXRGB1555(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLushort *inData = (const GLushort *) inspan;
    GLushort *outData = (GLushort *) outspan;

    for (i=0; i<width; i++) {
	*outData++ = *inData++ >> 1;
    }
}

/* 690c89b0 -- __glSpanPackRGBA4444ToARGB4444: 16-bit span rotated right by four (4444 RGBA <-> ARGB) */
void __fastcall __glSpanPackRGBA4444ToARGB4444(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLushort *inData = (const GLushort *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLuint v;

    for (i=0; i<width; i++) {
	v = *inData;
	*outData++ = (GLushort)((v >> 4) | (v << 12));
	inData++;
    }
}

/* 690c8a30 -- __glSpanPackRGBA8888ToXRGB8888: 32-bit span shifted right by eight */
void __fastcall __glSpanPackRGBA8888ToXRGB8888(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;

    for (i=0; i<width; i++) {
	*outData++ = *inData++ >> 8;
    }
}

/* 690c8ad0 -- px_upack.c __glBytesPerElement (ICD type set: the internal types 0..5) */
GLint __fastcall __glBytesPerElement(GLenum type)
{
    switch(type) {
      case GL_UNSIGNED_SHORT:
      case GL_SHORT:
      case 0x8034:      /* GL_UNSIGNED_SHORT_5_5_5_1 */
      case 0x8033:      /* GL_UNSIGNED_SHORT_4_4_4_4 */
      case 0:
      case 1:
      case 2:
      case 3:
	return 2;
      case GL_UNSIGNED_BYTE:
      case GL_BYTE:
      case 0x8032:      /* GL_UNSIGNED_BYTE_3_3_2 */
	return 1;
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
      case 0x8036:      /* GL_UNSIGNED_INT_10_10_10_2 */
      case 0x8035:      /* GL_UNSIGNED_INT_8_8_8_8 */
      case 4:
      case 5:
	return 4;
      case GL_BITMAP:
	return 1;
      default:
	return 0;
    }
}

/* 690c8b70 -- __glInitUnpacker (px_upack.c) */
void __fastcall __glInitUnpacker(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    GLint alignment;
    GLint components;
    GLint element_size;
    GLint rowsize;
    GLint padding;
    GLint groupsize;
    GLint groups_per_line;
    GLint skip_pixels, skip_lines;
    GLint swap_bytes;
    GLenum format, type;
    const GLvoid *pixels;

    format = spanInfo->srcFormat;
    type = spanInfo->srcType;
    pixels = spanInfo->srcImage;
    skip_pixels = spanInfo->srcSkipPixels;
    skip_lines = spanInfo->srcSkipLines;
    alignment = spanInfo->srcAlignment;
    swap_bytes = spanInfo->srcSwapBytes;

    components = __glElementsPerGroup(format, type);
    groups_per_line = spanInfo->srcLineLength;

    element_size = __glBytesPerElement(type);
    if (element_size == 1) swap_bytes = 0;
    groupsize = element_size * components;

    rowsize = groups_per_line * groupsize;
    if (type == GL_BITMAP) {
        rowsize = (groups_per_line + 7)/8;
    }
    padding = (rowsize % alignment);
    if (padding) {
        rowsize += alignment - padding;
    }
    if (((skip_pixels & 0x7) && type == GL_BITMAP) ||
            (swap_bytes && element_size > 1)) {
        spanInfo->srcPackedData = GL_FALSE;
    } else {
        spanInfo->srcPackedData = GL_TRUE;
    }

    if (type == GL_BITMAP) {
        spanInfo->srcCurrent = (const GLvoid *) (((const GLubyte *) pixels) +
                skip_lines * rowsize + skip_pixels / 8);
        spanInfo->srcStartBit = skip_pixels % 8;
    } else {
        spanInfo->srcCurrent = (const GLvoid *) (((const GLubyte *) pixels) +
                skip_lines * rowsize + skip_pixels * groupsize);
    }
    spanInfo->srcRowIncrement = rowsize;
    spanInfo->srcGroupIncrement = groupsize;
    spanInfo->srcComponents = components;
    spanInfo->srcElementSize = element_size;
    spanInfo->applyFlags[0] = GL_FALSE;
    spanInfo->applyFlags[1] = GL_TRUE;
    spanInfo->applyFlags[2] = GL_TRUE;
    spanInfo->applyFlags[3] = GL_FALSE;
}

/* 690cb5a0 -- __glSpanZeroFillAlpha (3dfx PX_UPACK.C): RGBA word span to RGB0 */
void __fastcall __glSpanZeroFillAlpha(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLuint *outData = (GLuint *) outspan;
    const GLuint *inData = (const GLuint *) inspan;

    for (i=0; i<width; i++) {
	*outData++ = *inData++;
	*outData++ = *inData++;
	*outData++ = *inData++;
	inData++;
	*outData++ = 0;
    }
}

/* 690cbab0 -- __glSpanGetMemRGBA (3dfx PX_UPACK.C): RGBA word span to RGBA; ToDo UNUSED CODE: nothing in the driver calls it */
void __fastcall __glSpanGetMemRGBA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;

    for (i=0; i<width; i++) {
	*outData++ = *inData++;
	*outData++ = *inData++;
	*outData++ = *inData++;
	*outData++ = *inData++;
    }
}

/* 690cbb00 -- __glSpanGetMemRGB (3dfx PX_UPACK.C): RGB word span to RGB0 */
void __fastcall __glSpanGetMemRGB(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;

    for (i=0; i<width; i++) {
	*outData++ = *inData++;
	*outData++ = *inData++;
	*outData++ = *inData++;
	*outData++ = 0;
    }
}

/* 690cbb50 -- __glSpanGetMemL (3dfx PX_UPACK.C): one-word span to X000 */
void __fastcall __glSpanGetMemL(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;

    for (i=0; i<width; i++) {
	*outData++ = *inData++;
	*outData++ = 0;
	*outData++ = 0;
	*outData++ = 0;
    }
}

/* 690cbb90 -- __glSpanGetMemLA (3dfx PX_UPACK.C): LA word span to L00A; ToDo UNUSED CODE: nothing in the driver calls it */
void __fastcall __glSpanGetMemLA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;

    for (i=0; i<width; i++) {
	*outData++ = *inData++;
	*outData++ = 0;
	*outData++ = 0;
	*outData++ = *inData++;
    }
}

/* 690cbbe0 -- __glSpanGetMemI (3dfx PX_UPACK.C): the same as GetMemL */
void __fastcall __glSpanGetMemI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;

    for (i=0; i<width; i++) {
	*outData++ = *inData++;
	*outData++ = 0;
	*outData++ = 0;
	*outData++ = 0;
    }
}

/* 690cbc20 -- __glSpanMemPutRGBA (3dfx PX_UPACK.C): RGBA word span to RGBA; ToDo UNUSED CODE: nothing in the driver calls it */
void __fastcall __glSpanMemPutRGBA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;

    for (i=0; i<width; i++) {
	*outData++ = *inData++;
	*outData++ = *inData++;
	*outData++ = *inData++;
	*outData++ = *inData++;
    }
}

/* 690cbc70 -- __glSpanMemPutRGB (3dfx PX_UPACK.C): RGBA word span to RGB */
void __fastcall __glSpanMemPutRGB(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;

    for (i=0; i<width; i++) {
	*outData++ = *inData++;
	*outData++ = *inData++;
	*outData++ = *inData++;
	inData++;
    }
}

/************************************************************************/
/* px_paths.c-style span extractors (the red/alpha one, 690cbce0, is above) */

/* 690cbcb0 -- __glSpanMemPutL (3dfx PX_UPACK.C): RGBA word span to red */
void __fastcall __glSpanMemPutL(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;

    for (i=0; i<width; i++) {
	*outData++ = *inData;
	inData += 4;
    }
}

/* 690cbce0 -- __glSpanMemPutLA (3dfx PX_UPACK.C): RGBA word span to red, alpha */
void __fastcall __glSpanMemPutLA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;

    for (i=0; i<width; i++) {
	*outData++ = *inData;
	inData += 3;
	*outData++ = *inData++;
    }
}

/* 690cbd10 -- __glSpanMemPutI (3dfx PX_UPACK.C): RGBA word span to one component */
void __fastcall __glSpanMemPutI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;

    for (i=0; i<width; i++) {
	*outData++ = *inData;
	inData += 4;
    }
}

/* 690c88a0 -- __glSpanPackRGBA8ToARGB8888: RGBA8 span (0xAABBGGRR) to 0xAARRGGBB words */
void __fastcall __glSpanPackRGBA8ToARGB8888(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;
    GLuint p;

    for (i=0; i<width; i++) {
	p = *inData;
	*outData = (p >> 16 & 0xff) | (p & 0xff) << 16 | (p & 0xff00ff00);
	inData++;
	outData++;
    }
}

/* 690c88f0 -- __glSpanPackRGBA8ToXRGB8888: RGBA8 span with the alpha byte forced to 0xff */
void __fastcall __glSpanPackRGBA8ToXRGB8888(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;
    GLuint p;

    for (i=0; i<width; i++) {
	p = *inData;
	*outData = (p >> 16 & 0xff) | (p | 0xffffff00) << 16 | (p & 0xff00);
	inData++;
	outData++;
    }
}

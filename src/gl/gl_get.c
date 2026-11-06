/*
** gl_get.c -- original VA range 690bc000..690c0000: s_get.c glGetMaterialfv and the glGet*v
** wrappers, glGetPointervEXT, dl_table.c dlist array new / free and glDeleteLists, and px_api.c
** __glFreePixelState.
** Then the rest of s_get.c, in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "siprotos.h"
#include "si_names.h"
#include "icdprotos.h"
#include "glsitypes.h"
#include "icdglob.h"
#include "glshade.h"
#include "og.h"
extern void __stdcall __glim_GetPointerv(GLenum pname, void **params);

/* s_get.c DoGet result types */
#define __GL_FLOAT32    1
#define __GL_FLOAT64    2
#define __GL_INT32      3
#define __GL_BOOLEAN    4

/* EXT_index_material / index_func material parameters */
#define GL_INDEX_SHIFT_MATERIAL     0x0d12      /* GL_INDEX_SHIFT */
#define GL_INDEX_OFFSET_MATERIAL    0x0d13      /* GL_INDEX_OFFSET */

typedef void *(__fastcall *__GLdlMallocProcBC0)(__GLcontext *gc, size_t size);
typedef void (__fastcall *__GLdlFreeProcBC0)(__GLcontext *gc, void *p);
typedef void (__fastcall *__GLdlArrayProcBC0)(__GLcontext *gc, __GLdlistArray *array);

/* ---- forward declarations ---- */
void __fastcall __glNop(void);

/* 690bc430 -- __glim_GetMaterialfv (s_get.c), with the ICD's GL_INDEX_OFFSET / GL_INDEX_SHIFT
** material values.
*/
void __stdcall __glim_GetMaterialfv(GLenum face, GLenum pname, GLfloat result[])
{
    __GLmaterialState *mat;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (face) {
      case GL_FRONT:
        mat = &gc->state.light.front;
        break;
      case GL_BACK:
        mat = &gc->state.light.back;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    switch (pname) {
      case GL_COLOR_INDEXES:
        result[0] = mat->cmapa;
        result[1] = mat->cmapd;
        result[2] = mat->cmaps;
        break;
      case GL_SHININESS:
        result[0] = mat->specularExponent;
        break;
      case GL_EMISSION:
        __glUnScaleColorf(gc, result, &mat->emissive);
        break;
      case GL_AMBIENT:
        result[0] = mat->ambient.r;
        result[1] = mat->ambient.g;
        result[2] = mat->ambient.b;
        result[3] = mat->ambient.a;
        break;
      case GL_DIFFUSE:
        result[0] = mat->diffuse.r;
        result[1] = mat->diffuse.g;
        result[2] = mat->diffuse.b;
        result[3] = mat->diffuse.a;
        break;
      case GL_SPECULAR:
        result[0] = mat->specular.r;
        result[1] = mat->specular.g;
        result[2] = mat->specular.b;
        result[3] = mat->specular.a;
        break;
      case GL_INDEX_OFFSET_MATERIAL:
        result[0] = mat->indexOffset;
        break;
      case GL_INDEX_SHIFT_MATERIAL:
        result[0] = mat->indexShift;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690bf0bd -- __glim_GetDoublev (s_get.c); the name strings sit in .text in front of each wrapper */
void __stdcall __glim_GetDoublev(GLenum sq, GLdouble result[])
{
    __glDoGet(sq, result, __GL_FLOAT64, "glGetDoublev");
}

/* 690bf0ec -- __glim_GetFloatv (s_get.c) */
void __stdcall __glim_GetFloatv(GLenum sq, GLfloat result[])
{
    __glDoGet(sq, result, __GL_FLOAT32, "glGetFloatv");
}

/* 690bf11e -- __glim_GetIntegerv (s_get.c) */
void __stdcall __glim_GetIntegerv(GLenum sq, GLint result[])
{
    __glDoGet(sq, result, __GL_INT32, "glGetIntegerv");
}

/* 690bf14e -- __glim_GetBooleanv (s_get.c) */
void __stdcall __glim_GetBooleanv(GLenum sq, GLboolean result[])
{
    __glDoGet(sq, result, __GL_BOOLEAN, "glGetBooleanv");
}

/* 690bf380 -- __glim_GetPointervEXT (EXT_vertex_array): glGetPointerv */
void __stdcall __glim_GetPointervEXT(GLenum pname, void **params)
{
    __glim_GetPointerv(pname, params);
}

/* 690bf3b0 -- __glNop: an empty proc (the SI's __glNop) */
void __fastcall __glNop()
{
}

/* 690bf3c0 -- __glNop_2: an empty proc (the SI's __glNop, a second copy) */
void __fastcall __glNop_2()
{
}

/* 690bf3d0 -- __glDlistNewArray (dl_table.c) */
__GLdlistArray *__fastcall __glDlistNewArray(__GLcontext *gc)
{
    __GLdlistArray *array;

    array = (__GLdlistArray *)(*(__GLdlMallocProcBC0)gc->dlist.malloc)(gc, sizeof(__GLdlistArray));
    if (array == NULL) {
        __glSetError(GL_OUT_OF_MEMORY);
        return NULL;
    }
    array->refcount = 1;

    /* call machine-dependent function */
    if (gc->dlist.newArray)
        (*(__GLdlArrayProcBC0)gc->dlist.newArray)(gc, array);

    return array;
}

/* 690bf410 -- __glDlistFreeArray (dl_table.c) */
void __fastcall __glDlistFreeArray(__GLcontext *gc, __GLdlistArray *array)
{
    /* call machine-dependent function */
    if (gc->dlist.freeArray)
        (*(__GLdlArrayProcBC0)gc->dlist.freeArray)(gc, array);

    (*(__GLdlFreeProcBC0)gc->dlist.free)(gc, array);
}

/* 690bf540 -- __glim_DeleteLists (dl_table.c) */
void __stdcall __glim_DeleteLists(GLuint list, GLsizei range)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (range < 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    if (range == 0) return;

    __glNamesDeleteRange(gc, (__GLnamesArray *)gc->dlist.namesArray, list, range);
}

/* 690bf740 -- __glFreePixelState (px_api.c): the ten glPixelMap tables and the pixel machine's
** lookup maps.  No colour table / convolution / histogram / minmax state in this kit.
*/
void __fastcall __glFreePixelState(__GLcontext *gc)
{
    __GLpixelMapHead *pMap = (__GLpixelMapHead *)gc->state.pixel.map;
    GLint i;

    /* free memory allocated to pixel maps */
    for (i = 0; i < 10; i++) {
        if (pMap[i].base.mapI) {
            __GL_FREE(gc, pMap[i].base.mapI);
            pMap[i].base.mapI = 0;
        }
    }
    __GL_FREE(gc, gc->pixel.redMap);
    __GL_FREE(gc, gc->pixel.greenMap);
    __GL_FREE(gc, gc->pixel.blueMap);
    __GL_FREE(gc, gc->pixel.alphaMap);
    __GL_FREE(gc, gc->pixel.iMap);
    if (gc->pixel.redModMap) {
        __GL_FREE(gc, gc->pixel.redModMap);
        __GL_FREE(gc, gc->pixel.greenModMap);
        __GL_FREE(gc, gc->pixel.blueModMap);
        __GL_FREE(gc, gc->pixel.alphaModMap);
    }
    if (gc->pixel.iToRMap) {
        __GL_FREE(gc, gc->pixel.iToRMap);
        __GL_FREE(gc, gc->pixel.iToGMap);
        __GL_FREE(gc, gc->pixel.iToBMap);
        __GL_FREE(gc, gc->pixel.iToAMap);
    }
    if (gc->pixel.iToIMap) {
        __GL_FREE(gc, gc->pixel.iToIMap);
    }
}

/* texture lookups (s_tex.c, si_names.h): __glLookUpTextureParams -> the object's parameter block,
** __glLookUpTextureTexobjs -> &tex->name ({name, priority}), __glLookUpTexture -> the object */
#define LookupParams(gc, target)    __glLookUpTextureParams(gc, target, (gc)->fx.activeTexture)
#define LookupNamePrio(gc, target)  __glLookUpTextureTexobjs(gc, target, (gc)->fx.activeTexture)
#define LookupTexture(gc, target)   __glLookUpTexture(gc, target, (gc)->fx.activeTexture)
#ifndef GL_TEXTURE_PRIORITY
#define GL_TEXTURE_PRIORITY     0x8066
#define GL_TEXTURE_RESIDENT     0x8067
#endif
#ifndef GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB    0x86A0
#define GL_TEXTURE_COMPRESSED_ARB               0x86A1
#endif
/* dlist plumbing: 690b44d0 __glDlistAllocOp2 (opcode word at +0x0c, data at +0x10), 690b4520
** __glDlistAppendOp; 69096060 __glFillImage (__gllc_InvalidValue / InvalidEnum: glsi.h) */
#ifndef GL_UNSIGNED_BYTE_3_3_2
#define GL_UNSIGNED_BYTE_3_3_2          0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4       0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1       0x8034
#define GL_UNSIGNED_INT_8_8_8_8         0x8035
#define GL_UNSIGNED_INT_10_10_10_2      0x8036
#endif
#ifndef GL_PROXY_TEXTURE_2D
#define GL_PROXY_TEXTURE_2D     0x8064
#endif
/* ---- s_get.c __glim_GetTexImage.  Mipmap level words used: +0x00 buffer, +0x08 the compressed
** data, +0x0c width, +0x10 height, +0x14 depth, +0x44 border, +0x48 requested format, +0x54 the
** level's texel format (bits per texel at +0x20). */
extern void __cdecl _txImgDequantizeFXT1(GLuint *out, GLuint *in, int w, int h);   /* texus2 dequant.c */
#define GL_COMPRESSED_RGB_FXT1_3DFX     0x86B0
#define GL_COMPRESSED_RGBA_FXT1_3DFX    0x86B1
#define __GL_FLOAT          0
/* ---- s_get.c ---- */
#ifndef GL_FOG_COORDINATE_ARRAY_POINTER_EXT
#define GL_FOG_COORDINATE_ARRAY_POINTER_EXT 0x8456
#endif

/* 690bb480 -- __glim_GetTexGeniv */
void APIENTRY __glim_GetTexGeniv(GLenum coord, GLenum pname, GLint v[])
{
    __GLtextureGenState* tcs;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (coord) {
      case GL_S:
        tcs = &(&gc->state.texture.unit[gc->fx.activeTexture])->s;
        break;
      case GL_T:
        tcs = &(&gc->state.texture.unit[gc->fx.activeTexture])->t;
        break;
      case GL_R:
        tcs = &(&gc->state.texture.unit[gc->fx.activeTexture])->r;
        break;
      case GL_Q:
        tcs = &(&gc->state.texture.unit[gc->fx.activeTexture])->q;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    switch (pname) {
      case GL_TEXTURE_GEN_MODE:
        v[0] = tcs->mode;
        break;
      case GL_OBJECT_PLANE:
        __glConvertResult(gc, 0 /* __GL_FLOAT */, &tcs->objectPlaneEquation.x, 3 /* __GL_INT32 */, v, 4);
        break;
      case GL_EYE_PLANE:
        __glConvertResult(gc, 0, &tcs->eyePlaneEquation.x, 3, v, 4);
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690bb5a0 -- __glim_GetTexParameterfv: the enums are unsigned in the ICD (fild qword) */
void __stdcall __glim_GetTexParameterfv(GLenum target, GLenum pname, GLfloat v[])
{
    __GLtexObjParamICD *pts;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    pts = LookupParams(gc, target);

    if (!pts) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    switch (pname) {
      case GL_TEXTURE_WRAP_S:
	v[0] = (GLfloat)pts->sWrapMode;
	break;
      case GL_TEXTURE_WRAP_T:
	v[0] = (GLfloat)pts->tWrapMode;
	break;
      case GL_TEXTURE_MIN_FILTER:
	v[0] = (GLfloat)pts->minFilter;
	break;
      case GL_TEXTURE_MAG_FILTER:
	v[0] = (GLfloat)pts->magFilter;
	break;
      case GL_TEXTURE_BORDER_COLOR:
	v[0] = pts->borderColor.r;
	v[1] = pts->borderColor.g;
	v[2] = pts->borderColor.b;
	v[3] = pts->borderColor.a;
	break;
      case GL_TEXTURE_PRIORITY:
	v[0] = LookupNamePrio(gc, target)->priority;
	break;
      case GL_TEXTURE_RESIDENT:
	{
	    __GLtextureICD *tex = LookupTexture(gc, target);
	    /* fix: the active unit's TMU slot (one shared slot with TEXUMA), as GL_TRUE / GL_FALSE */
	    v[0] = tex->hwHandle[__GL_TMU_SLOT(gc->fx.activeTexture)] ? 1.0f : 0.0f;
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 690bb740 -- __glim_GetTexParameteriv */
void __stdcall __glim_GetTexParameteriv(GLenum target, GLenum pname, GLint v[])
{
    __GLtexObjParamICD *pts;
    __GL_SETUP_NOT_IN_BEGIN();

    pts = LookupParams(gc, target);

    if (!pts) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    switch (pname) {
      case GL_TEXTURE_WRAP_S:
	v[0] = pts->sWrapMode;
	break;
      case GL_TEXTURE_WRAP_T:
	v[0] = pts->tWrapMode;
	break;
      case GL_TEXTURE_MIN_FILTER:
	v[0] = pts->minFilter;
	break;
      case GL_TEXTURE_MAG_FILTER:
	v[0] = pts->magFilter;
	break;
      case GL_TEXTURE_BORDER_COLOR:
	v[0] = __GL_FLOAT_TO_I(pts->borderColor.r);
	v[1] = __GL_FLOAT_TO_I(pts->borderColor.g);
	v[2] = __GL_FLOAT_TO_I(pts->borderColor.b);
	v[3] = __GL_FLOAT_TO_I(pts->borderColor.a);
	break;
      case GL_TEXTURE_PRIORITY:
	v[0] = __GL_FLOAT_TO_I(LookupNamePrio(gc, target)->priority);
	break;
      case GL_TEXTURE_RESIDENT:
	/* fix: the flag's value for the active unit's TMU slot (the original returned &hwHandle[0], never 0) */
	v[0] = LookupTexture(gc, target)->hwHandle[__GL_TMU_SLOT(gc->fx.activeTexture)] ? GL_TRUE : GL_FALSE;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 690bb920 -- __glim_GetTexLevelParameterfv (the internal format converts as unsigned) */
void __stdcall __glim_GetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat v[])
{
    __GLtextureICD *tex;
    __GLmipMapLevelICD *lp;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    tex = LookupTexture(gc, target);

    if (!tex) {
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    if ((level < 0) || (level >= gc->constants.maxMipMapLevel)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    lp = tex->level[level];

    switch (pname) {
      case GL_TEXTURE_WIDTH:
	v[0] = lp->width;
	break;
      case GL_TEXTURE_HEIGHT:
	if (tex->dim < 2) {
	    v[0] = lp->height - lp->border*2;
	} else {
	    v[0] = lp->height;
	}
	break;
      case GL_TEXTURE_INTERNAL_FORMAT:
	v[0] = (GLuint)lp->requestedFormat;
	break;
      case GL_TEXTURE_BORDER:
	v[0] = lp->border;
	break;
      case GL_TEXTURE_RED_SIZE:
	v[0] = lp->redSize;
	break;
      case GL_TEXTURE_GREEN_SIZE:
	v[0] = lp->greenSize;
	break;
      case GL_TEXTURE_BLUE_SIZE:
	v[0] = lp->blueSize;
	break;
      case GL_TEXTURE_ALPHA_SIZE:
	v[0] = lp->alphaSize;
	break;
      case GL_TEXTURE_LUMINANCE_SIZE:
	v[0] = lp->luminanceSize;
	break;
      case GL_TEXTURE_INTENSITY_SIZE:
	v[0] = lp->intensitySize;
	break;
      case GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB:
	v[0] = lp->compressedSize;
	break;
      case GL_TEXTURE_COMPRESSED_ARB:
	break;
      default:
	goto bad_enum;
    }
}

/* 690bbaf0 -- __glim_GetTexLevelParameteriv: no GL_TEXTURE_DEPTH; ARB_texture_compression's image
** size added, and GL_TEXTURE_COMPRESSED_ARB accepted but nothing stored (as the original) */
void __stdcall __glim_GetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint v[])
{
    __GLtextureICD *tex;
    __GLmipMapLevelICD *lp;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    tex = LookupTexture(gc, target);

    if (!tex) {
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    if ((level < 0) || (level >= gc->constants.maxMipMapLevel)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    lp = tex->level[level];

    switch (pname) {
      case GL_TEXTURE_WIDTH:
	v[0] = lp->width;
	break;
      case GL_TEXTURE_HEIGHT:
	if (tex->dim < 2) {
	    v[0] = lp->height - lp->border*2;
	} else {
	    v[0] = lp->height;
	}
	break;
      case GL_TEXTURE_INTERNAL_FORMAT:
	v[0] = lp->requestedFormat;
	break;
      case GL_TEXTURE_BORDER:
	v[0] = lp->border;
	break;
      case GL_TEXTURE_RED_SIZE:
	v[0] = lp->redSize;
	break;
      case GL_TEXTURE_GREEN_SIZE:
	v[0] = lp->greenSize;
	break;
      case GL_TEXTURE_BLUE_SIZE:
	v[0] = lp->blueSize;
	break;
      case GL_TEXTURE_ALPHA_SIZE:
	v[0] = lp->alphaSize;
	break;
      case GL_TEXTURE_LUMINANCE_SIZE:
	v[0] = lp->luminanceSize;
	break;
      case GL_TEXTURE_INTENSITY_SIZE:
	v[0] = lp->intensitySize;
	break;
      case GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB:
	v[0] = lp->compressedSize;
	break;
      case GL_TEXTURE_COMPRESSED_ARB:
	break;
      default:
	goto bad_enum;
    }
}

/************************************************************************/

/* 690bbd30 -- __glInitImagePack (SI 1.1: depth / dim, no skip-images / image-height) */
void __fastcall __glInitImagePack(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
			     GLint width, GLint height, GLenum format, GLenum type,
			     GLvoid *buf)
{
    spanInfo->x = __GL_ZERO_F;
    spanInfo->zoomx = 1.0F;
    spanInfo->width = width;
    spanInfo->height = height;
    spanInfo->depth = 1;
    spanInfo->dim = 2;

    spanInfo->srcAlignment = 4;
    spanInfo->srcSkipPixels = 0;
    spanInfo->srcSkipLines = 0;
    spanInfo->srcSwapBytes = GL_FALSE;
    spanInfo->srcLsbFirst = GL_FALSE;
    spanInfo->srcLineLength = width;

    spanInfo->dstFormat = format;
    spanInfo->dstType = type;
    spanInfo->dstImage = buf;
    __glLoadPackModes(gc, spanInfo);
}

/* 690bbdb0 -- __glim_GetTexImage: no proxy / 3D handling, depth taken without the dim test; FXT1
** levels are decompressed into a fresh imports.malloc buffer (never freed, as the original) that
** stands in for the level buffer during the fetch */
void __stdcall __glim_GetTexImage(GLenum target, GLint level,
			     GLenum format, GLenum type, GLvoid *texels)
{
    GLint width, height;
    __GLtextureICD *tex;
    __GLmipMapLevelICD *lp;
    __GLpixelSpanInfoICD spanInfo;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    tex = LookupTexture(gc, target);

    if (!tex || (target == GL_PROXY_TEXTURE_1D) ||
		(target == GL_PROXY_TEXTURE_2D))
    {
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    if ((level < 0) || (level >= gc->constants.maxMipMapLevel)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    lp = (__GLmipMapLevelICD *) tex->level[level];
    switch (format) {
      case GL_RGBA:
      case GL_RGB:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_ABGR_EXT:
      case GL_BGR:
      case GL_BGRA:
	break;
      default:
	goto bad_enum;
    }
    switch (type) {
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
	  case GL_BGR:
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_10_10_10_2:
	switch (format) {
	  case GL_RGBA:
	  case GL_ABGR_EXT:
	  case GL_BGRA:
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	break;
      default:
	goto bad_enum;
    }

    width = lp->width;
    if (tex->dim < 2) {
	height = lp->height - lp->border*2;
    } else {
	height = lp->height;
    }

    /* If no texture has been loaded */
    if (!width || !height) return;

    if (lp->requestedFormat != GL_COMPRESSED_RGB_FXT1_3DFX &&
	lp->requestedFormat != GL_COMPRESSED_RGBA_FXT1_3DFX &&
	(lp->requestedFormat < 0x83a0 || lp->requestedFormat > 0x83a3)) {     /* the S3TC formats are stored as FXT1 */
	__glInitTexDestPack(gc, &spanInfo, width, height, lp->depth - lp->border*2,
			    format, type, texels);
	__glInitTexImageGet(gc, &spanInfo, tex, level);
	__glInitPacker(gc, &spanInfo);
	__glInitUnpacker(gc, &spanInfo);
	(*gc->procs.fetchTex)(gc, tex, &spanInfo, GL_TRUE);
    } else {
	void *buffer = lp->buffer;
	GLint lineLength;

	lp->buffer = __GL_MALLOC(gc,
	    ((lp->texFormat->bitsPerTexel + 7) >> 3) * ((width + 7) & ~7) * ((height + 3) & ~3) + 1);
	_txImgDequantizeFXT1(lp->buffer, lp->compressedImage, (width + 7) & ~7, (height + 3) & ~3);
	lineLength = (width + 7) & ~7;
	__glInitTexDestPack(gc, &spanInfo, width, height, lp->depth - lp->border*2,
			    format, type, texels);
	__glInitTexImageGet(gc, &spanInfo, tex, level);
	spanInfo.srcLineLength = lineLength;
	__glInitPacker(gc, &spanInfo);
	__glInitUnpacker(gc, &spanInfo);
	(*gc->procs.fetchTex)(gc, tex, &spanInfo, GL_TRUE);
	lp->buffer = buffer;
    }
}

/* 690bc160 -- __glim_GetLightfv: glGetLightfv */
void __stdcall __glim_GetLightfv(GLenum light, GLenum pname,
			     GLfloat result[])
{
    GLint index;
    __GLlightSourceStateICD *src;
    __GL_SETUP_NOT_IN_BEGIN();

    index = light - GL_LIGHT0;
    if ((index < 0) || (index >= gc->constants.numberOfLights)) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    src = &((__GLlightSourceStateICD *) gc->state.light.source)[index];
    switch (pname) {
      case GL_AMBIENT:
	__glUnScaleColorf(gc, result, &src->ambient);
	break;
      case GL_DIFFUSE:
	__glUnScaleColorf(gc, result, &src->diffuse);
	break;
      case GL_SPECULAR:
	__glUnScaleColorf(gc, result, &src->specular);
	break;
      case GL_POSITION:
	result[0] = src->positionEye.x;
	result[1] = src->positionEye.y;
	result[2] = src->positionEye.z;
	result[3] = src->positionEye.w;
	break;
      case GL_SPOT_DIRECTION:
	result[0] = src->direction.x;
	result[1] = src->direction.y;
	result[2] = src->direction.z;
	break;
      case GL_SPOT_EXPONENT:
	result[0] = src->spotLightExponent;
	break;
      case GL_SPOT_CUTOFF:
	result[0] = src->spotLightCutOffAngle;
	break;
      case GL_CONSTANT_ATTENUATION:
	result[0] = src->constantAttenuation;
	break;
      case GL_LINEAR_ATTENUATION:
	result[0] = src->linearAttenuation;
	break;
      case GL_QUADRATIC_ATTENUATION:
	result[0] = src->quadraticAttenuation;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 690bc2b0 -- __glim_GetLightiv: glGetLightiv */
void __stdcall __glim_GetLightiv(GLenum light, GLenum pname,
			     GLint result[])
{
    GLint index;
    __GLlightSourceStateICD *src;
    __GL_SETUP_NOT_IN_BEGIN();

    index = light - GL_LIGHT0;
    if ((index < 0) || (index >= gc->constants.numberOfLights)) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    src = &((__GLlightSourceStateICD *) gc->state.light.source)[index];
    switch (pname) {
      case GL_AMBIENT:
	__glUnScaleColori(gc, result, &src->ambient);
	break;
      case GL_DIFFUSE:
	__glUnScaleColori(gc, result, &src->diffuse);
	break;
      case GL_SPECULAR:
	__glUnScaleColori(gc, result, &src->specular);
	break;
      case GL_POSITION:
	__glConvertResult(gc, __GL_FLOAT, &src->positionEye.x,
			  __GL_INT32, result, 4);
	break;
      case GL_SPOT_DIRECTION:
	__glConvertResult(gc, __GL_FLOAT, &src->direction.x,
			  __GL_INT32, result, 3);
	break;
      case GL_SPOT_EXPONENT:
	__glConvertResult(gc, __GL_FLOAT, &src->spotLightExponent,
			  __GL_INT32, result, 1);
	break;
      case GL_SPOT_CUTOFF:
	__glConvertResult(gc, __GL_FLOAT, &src->spotLightCutOffAngle,
			  __GL_INT32, result, 1);
	break;
      case GL_CONSTANT_ATTENUATION:
	__glConvertResult(gc, __GL_FLOAT, &src->constantAttenuation,
			  __GL_INT32, result, 1);
	break;
      case GL_LINEAR_ATTENUATION:
	__glConvertResult(gc, __GL_FLOAT, &src->linearAttenuation,
			  __GL_INT32, result, 1);
	break;
      case GL_QUADRATIC_ATTENUATION:
	__glConvertResult(gc, __GL_FLOAT, &src->quadraticAttenuation,
			  __GL_INT32, result, 1);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 690bc590 -- __glim_GetMaterialiv: glGetMaterialiv (the ICD adds GL_INDEX_SHIFT / GL_INDEX_OFFSET) */
void __stdcall __glim_GetMaterialiv(GLenum face, GLenum pname,
			     GLint result[])
{
    __GLmaterialState *mat;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (face) {
      case GL_FRONT:
	mat = &gc->state.light.front;
	break;
      case GL_BACK:
	mat = &gc->state.light.back;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    switch (pname) {
      case GL_COLOR_INDEXES:
	__glConvertResult(gc, __GL_FLOAT, &mat->cmapa,
			  __GL_INT32, result, 3);
	break;
      case GL_SHININESS:
	__glConvertResult(gc, __GL_FLOAT, &mat->specularExponent,
			  __GL_INT32, result, 1);
	break;
      case GL_EMISSION:
	__glUnScaleColori(gc, result, &mat->emissive);
	break;
      case GL_AMBIENT:
	result[0] = __GL_FLOAT_TO_I(mat->ambient.r);
	result[1] = __GL_FLOAT_TO_I(mat->ambient.g);
	result[2] = __GL_FLOAT_TO_I(mat->ambient.b);
	result[3] = __GL_FLOAT_TO_I(mat->ambient.a);
	break;
      case GL_DIFFUSE:
	result[0] = __GL_FLOAT_TO_I(mat->diffuse.r);
	result[1] = __GL_FLOAT_TO_I(mat->diffuse.g);
	result[2] = __GL_FLOAT_TO_I(mat->diffuse.b);
	result[3] = __GL_FLOAT_TO_I(mat->diffuse.a);
	break;
      case GL_SPECULAR:
	result[0] = __GL_FLOAT_TO_I(mat->specular.r);
	result[1] = __GL_FLOAT_TO_I(mat->specular.g);
	result[2] = __GL_FLOAT_TO_I(mat->specular.b);
	result[3] = __GL_FLOAT_TO_I(mat->specular.a);
	break;
      case GL_INDEX_SHIFT:
	result[0] = __GL_FLOAT_TO_I(mat->indexShift);
	break;
      case GL_INDEX_OFFSET:
	result[0] = __GL_FLOAT_TO_I(mat->indexOffset);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 690bf170 -- __glim_GetError: glGetError */
GLenum __fastcall __glim_GetError(void)
{
    __GLcontext *gc;
    GLenum error;

    gc = __glCurrentGC();
    if (*(int *)&__glCurrentGC()->beginMode == 1) {
	__glSetError(GL_INVALID_OPERATION);
	return 0;
    }
    error = gc->error;
    gc->error = 0;
    return error;
}

/* 690bf260 -- __glim_GetPointerv (s_get.c, + the EXT_fog_coord array) */
void __stdcall __glim_GetPointerv(GLenum pname, void **params)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch(pname) {
      case GL_VERTEX_ARRAY_POINTER:
          *params = (GLvoid *)gc->vertexArray.vertex.pointer;
          return;
      case GL_NORMAL_ARRAY_POINTER:
          *params = (GLvoid *)gc->vertexArray.normal.pointer;
          return;
      case GL_COLOR_ARRAY_POINTER:
          *params = (GLvoid *)gc->vertexArray.color.pointer;
          return;
      case GL_INDEX_ARRAY_POINTER:
          *params = (GLvoid *)gc->vertexArray.index.pointer;
          return;
      case GL_TEXTURE_COORD_ARRAY_POINTER:
          *params = (GLvoid *)gc->vertexArray.texture.pointer[gc->fx.clientActiveTexture];
          return;
      case GL_EDGE_FLAG_ARRAY_POINTER:
          *params = (GLvoid *)gc->vertexArray.edgeFlag.pointer;
          return;
      case GL_FOG_COORDINATE_ARRAY_POINTER_EXT:
          *params = (GLvoid *)gc->vertexArray.fogCoord.pointer;
          return;
      case GL_SELECTION_BUFFER_POINTER:
          *params = *(void **)&gc->select.resultBase;
          return;
      case GL_FEEDBACK_BUFFER_POINTER:
          *params = *(void **)&gc->feedback.resultBase;
          return;
      default:
        __glSetError(GL_INVALID_OPERATION);
        return;
    }
}

/* the ICD's texture environment (0x54 bytes per unit): the SI's mode / color, then the
** GL_EXT_texture_env_combine state */
typedef struct __GLtextureEnvCombineICDRec {
    GLenum      mode;                       /* +0x00 */
    __GLcolor   color;                      /* +0x04 */
    GLenum      combineRGB;                 /* +0x14  GL_COMBINE_RGB_EXT */
    GLenum      combineAlpha;               /* +0x18  GL_COMBINE_ALPHA_EXT */
    GLenum      sourceRGB[3];               /* +0x1c  GL_SOURCE0..2_RGB_EXT */
    GLenum      sourceAlpha[3];             /* +0x28  GL_SOURCE0..2_ALPHA_EXT */
    GLenum      operandRGB[3];              /* +0x34  GL_OPERAND0..2_RGB_EXT */
    GLenum      operandAlpha[3];            /* +0x40  GL_OPERAND0..2_ALPHA_EXT */
} __GLtextureEnvCombineICD;
#define __GL_ACTIVE_ENV(gc) \
    ((__GLtextureEnvCombineICD *)(gc)->state.texture.unit[(gc)->fx.activeTexture].env)
#define ENV(gc)     ((__GLtextureEnvState *)(gc)->state.texture.unit[(gc)->fx.activeTexture].env)

/* 690bade0: __glim_GetTexEnviv (s_get.c, with the combine queries; stdcall, ecx / edx unused) */
void __stdcall __glim_GetTexEnviv(GLenum target, GLenum pname, GLint v[])
{
    __GLcolor color;
    __GL_SETUP_NOT_IN_BEGIN();

    if (target != GL_TEXTURE_ENV) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    switch (pname) {
      case GL_TEXTURE_ENV_MODE:
        v[0] = __GL_ACTIVE_ENV(gc)->mode;
        return;
      case GL_TEXTURE_ENV_COLOR:
        color.r = __GL_ACTIVE_ENV(gc)->color.r *
            gc->frontBuffer.oneOverRedScale * gc->constants.redScale;
        color.g = __GL_ACTIVE_ENV(gc)->color.g *
            gc->frontBuffer.oneOverGreenScale * gc->constants.greenScale;
        color.b = __GL_ACTIVE_ENV(gc)->color.b *
            gc->frontBuffer.oneOverBlueScale * gc->constants.blueScale;
        color.a = __GL_ACTIVE_ENV(gc)->color.a *
            gc->frontBuffer.oneOverAlphaScale * gc->constants.alphaScale;
        __glUnScaleColori(gc, v, &color);
        return;
      case 0x8571:  /* GL_COMBINE_RGB_EXT */
        v[0] = __GL_ACTIVE_ENV(gc)->combineRGB;
        return;
      case 0x8572:  /* GL_COMBINE_ALPHA_EXT */
        v[0] = __GL_ACTIVE_ENV(gc)->combineAlpha;
        return;
      case 0x8580:  /* GL_SOURCE0_RGB_EXT */
        v[0] = __GL_ACTIVE_ENV(gc)->sourceRGB[0];
        return;
      case 0x8581:
        v[0] = __GL_ACTIVE_ENV(gc)->sourceRGB[1];
        return;
      case 0x8582:
        v[0] = __GL_ACTIVE_ENV(gc)->sourceRGB[2];
        return;
      case 0x8588:  /* GL_SOURCE0_ALPHA_EXT */
        v[0] = __GL_ACTIVE_ENV(gc)->sourceAlpha[0];
        return;
      case 0x8589:
        v[0] = __GL_ACTIVE_ENV(gc)->sourceAlpha[1];
        return;
      case 0x858a:
        v[0] = __GL_ACTIVE_ENV(gc)->sourceAlpha[2];
        return;
      case 0x8590:  /* GL_OPERAND0_RGB_EXT */
        v[0] = __GL_ACTIVE_ENV(gc)->operandRGB[0];
        return;
      case 0x8591:
        v[0] = __GL_ACTIVE_ENV(gc)->operandRGB[1];
        return;
      case 0x8592:
        v[0] = __GL_ACTIVE_ENV(gc)->operandRGB[2];
        return;
      case 0x8598:  /* GL_OPERAND0_ALPHA_EXT */
        v[0] = __GL_ACTIVE_ENV(gc)->operandAlpha[0];
        return;
      case 0x8599:
        v[0] = __GL_ACTIVE_ENV(gc)->operandAlpha[1];
        return;
      case 0x859a:
        v[0] = __GL_ACTIVE_ENV(gc)->operandAlpha[2];
        return;
    }
    __glSetError(GL_INVALID_ENUM);
}

/* 690bc0a0 -- __glim_GetPolygonStipple */
void __stdcall __glim_GetPolygonStipple(GLubyte *outImage)
{
    __GLpixelSpanInfoICD spanInfo;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    spanInfo.srcImage = &(gc->state.polygonStipple.stipple[0]);
    spanInfo.srcType = GL_BITMAP;
    spanInfo.srcFormat = GL_COLOR_INDEX;
    __glInitImagePack(gc, &spanInfo, 32, 32, GL_COLOR_INDEX, GL_BITMAP,
            outImage);
    __glInitPacker(gc, &spanInfo);
    __glInitUnpacker(gc, &spanInfo);
    (*gc->procs.copyImage)(gc, &spanInfo, GL_TRUE);    /* ICD: a third argument (690bc13b) */
}

/* 690bac20 -- s_get.c __glim_GetTexEnvfv: the ICD's per-unit env record (0x54 bytes) also answers
** GL_RGB_SCALE_EXT (+0x4c) and GL_ALPHA_SCALE (+0x50); the colour is rescaled from the frame buffer's
** scale before __glUnScaleColorf */
void __stdcall __glim_GetTexEnvfv(GLenum target, GLenum pname, GLfloat v[])
{
    __GLcolor c;
    __GL_SETUP_NOT_IN_BEGIN();

    if (target != GL_TEXTURE_ENV) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    switch (pname) {
      case 0x8573:                          /* GL_RGB_SCALE_EXT */
        v[0] = ENV(gc)->rgbScale;
        return;
      case GL_TEXTURE_ENV_COLOR:
        c.r = ENV(gc)->color.r * gc->frontBuffer.oneOverRedScale * gc->constants.redScale;
        c.g = ENV(gc)->color.g * gc->frontBuffer.oneOverGreenScale * gc->constants.greenScale;
        c.b = ENV(gc)->color.b * gc->frontBuffer.oneOverBlueScale * gc->constants.blueScale;
        c.a = ENV(gc)->color.a * gc->frontBuffer.oneOverAlphaScale * gc->constants.alphaScale;
        __glUnScaleColorf(gc, v, &c);
        return;
      case GL_ALPHA_SCALE:
        v[0] = ENV(gc)->alphaScale;
        return;
      case GL_TEXTURE_ENV_MODE:
        v[0] = ENV(gc)->mode;
        return;
    }
    __glSetError(GL_INVALID_ENUM);
}

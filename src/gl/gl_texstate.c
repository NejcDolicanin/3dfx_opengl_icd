/*
** gl_texstate.c -- original VA range 6909c000..690a0000: texture state helpers of the 3dfx s_tex.c
** (ref/3dfx-opengl GLIDE3X/GLCORE/S_TEX.C): lookups, span-info setup, the TexImage / TexSubImage
** argument checks, the texture-coordinate calc procs and glGenTextures.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"                      /* __GLtexNamePriICD */
#include "si_names.h"
#include "icdprotos.h"
#include <glide.h>

/* texture.h (3dfx): the per-unit target indices */
#define __GL_TEXTURE_INDEX_1D           0
#define __GL_TEXTURE_INDEX_2D           1

#define GL_COMPRESSED_RGB_FXT1_3DFX     0x86b0
#define GL_COMPRESSED_RGBA_FXT1_3DFX    0x86b1

/* vertex.h: the clip-code half of flagBits (__GL_CLIP_SHIFT 16; the ICD tests every bit) */

#define TEX_NAMES(gc)   ((__GLnamesArray *)(gc)->texture.namesArray)

GLint __fastcall __glTexParameteriv_size(GLenum e);

/* 6909c610 -- __glLookUpTextureParams: the unit's parameter state of a target, 0 if none */
__GLtexObjParamICD *__fastcall __glLookUpTextureParams(__GLcontext *gc, GLenum target, GLint texUnit)
{
    switch (target) {
      case GL_TEXTURE_1D:
	return &gc->state.texture.unit[texUnit].param[__GL_TEXTURE_INDEX_1D].params;
      case GL_TEXTURE_2D:
	return &gc->state.texture.unit[texUnit].param[__GL_TEXTURE_INDEX_2D].params;
      default:
	return 0;
    }
}

/* 6909c660 -- __glLookUpTextureTexobjs: the unit's {name, priority} of a target, 0 if none */
__GLtexNamePriICD *__fastcall __glLookUpTextureTexobjs(__GLcontext *gc, GLenum target, GLint texUnit)
{
    switch (target) {
      case GL_TEXTURE_1D:
	return (__GLtexNamePriICD *)&gc->state.texture.unit[texUnit].param[__GL_TEXTURE_INDEX_1D].name;
      case GL_TEXTURE_2D:
	return (__GLtexNamePriICD *)&gc->state.texture.unit[texUnit].param[__GL_TEXTURE_INDEX_2D].name;
      default:
	return 0;
    }
}

/* 6909c760 -- __glCurrentTexture: the texture object bound to the unit's enabled target (the 3dfx
** gc->texture.currentTexture[unit]); 0 when the unit has no enabled target */
__GLtextureICD *__fastcall __glCurrentTexture(__GLcontext *gc, GLint unit)
{
    GLint index = gc->texture.currentIndex[unit];

    if (index >= 0 && index < 4) {
	return gc->texture.boundTexture[unit][index];
    }
    return 0;
}

/* 6909c780 -- __glShareTextureObjects: drop our texture names, share shareMe's */
void __fastcall __glShareTextureObjects(__GLcontext *gc, __GLcontext *shareMe)
{
    TEX_NAMES(gc)->refcount--;
    if (TEX_NAMES(gc)->refcount == 0) {
	__glNamesFreeArray(gc, TEX_NAMES(gc));
    }
    gc->texture.namesArray = NULL;

    gc->texture.namesArray = shareMe->texture.namesArray;
    TEX_NAMES(gc)->refcount++;
}

/* 6909d670 -- __glTexParameteriv_size: a 5-byte jump to __glTexParameterfv_size (6909d640), the same counts */
#ifdef _MSC_VER
__declspec(naked) GLint __fastcall __glTexParameteriv_size(GLenum e) { __asm jmp __glTexParameterfv_size }
#else
GLint __fastcall __glTexParameteriv_size(GLenum e) { return __glTexParameterfv_size(e); }
#endif

/* 6909e410 -- __glInitTexImageStore: the destination half of spanInfo for a texture level.  The
** ICD takes format, type and alignment from the level's device format (the ancestor switches on
** the internal format).
*/
void __fastcall __glInitTexImageStore(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, __GLtextureICD *tex, GLint lod)
{
    __GLmipMapLevelICD *lp = tex->level[lod];

    spanInfo->dstImage = lp->buffer;
    spanInfo->dstSkipPixels = 0;
    spanInfo->dstSkipLines = 0;
    spanInfo->dstSwapBytes = GL_FALSE;
    spanInfo->dstLsbFirst = GL_TRUE;
    spanInfo->dstLineLength = lp->width;
    spanInfo->dim = tex->dim;
    if (tex->dim == 1) {
	spanInfo->dstSkipLines = lp->border;
    }
    spanInfo->dstFormat = lp->texFormat->pixelFormat;
    spanInfo->dstType = lp->texFormat->pixelType;
    spanInfo->dstAlignment = lp->texFormat->alignment;
}

/* 6909e480 -- __glInitTexImageGet: the source half of spanInfo for a texture level */
void __fastcall __glInitTexImageGet(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, __GLtextureICD *tex, GLint lod)
{
    __GLmipMapLevelICD *lp = tex->level[lod];

    spanInfo->srcImage = lp->buffer;
    spanInfo->srcSkipPixels = 0;
    spanInfo->srcSkipLines = 0;
    spanInfo->srcSwapBytes = GL_FALSE;
    spanInfo->srcLsbFirst = GL_TRUE;
    spanInfo->srcLineLength = lp->width;
    spanInfo->dim = tex->dim;
    if (tex->dim == 1) {
	spanInfo->srcSkipLines = lp->border;
    }
    spanInfo->srcFormat = lp->texFormat->pixelFormat;
    spanInfo->srcType = lp->texFormat->pixelType;
    spanInfo->srcAlignment = lp->texFormat->alignment;
}

/* 6909e4f0 -- __glInitTexSourceUnpack: an application (or list) image as the span source */
void __fastcall __glInitTexSourceUnpack(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLsizei width, GLsizei height,
			     GLsizei depth, GLenum format, GLenum type, const GLvoid *buf, GLboolean packed)
{
    spanInfo->x = 0;
    spanInfo->width = width;
    spanInfo->height = height;
    spanInfo->depth = depth;
    spanInfo->srcFormat = format;
    spanInfo->srcType = type;
    spanInfo->srcImage = buf;
    spanInfo->zoomx = __GL_ONE_F;
    __glLoadUnpackModes(gc, spanInfo, packed);
}

/* 6909e550 -- __glInitTexDestPack: an application image as the span destination */
void __fastcall __glInitTexDestPack(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLsizei width, GLsizei height,
			     GLsizei depth, GLenum format, GLenum type, GLvoid *buf)
{
    spanInfo->x = 0;
    spanInfo->width = width;
    spanInfo->height = height;
    spanInfo->depth = depth;
    spanInfo->dstFormat = format;
    spanInfo->dstType = type;
    spanInfo->zoomx = __GL_ONE_F;
    spanInfo->dstImage = buf;
    __glLoadPackModes(gc, spanInfo);
}

/* 6909e5a0 -- __glCheckTexImage1DArgs */
__GLtextureICD *__fastcall __glCheckTexImage1DArgs(__GLcontext *gc, GLenum target, GLint lod, GLint components,
					 GLsizei length, GLint border, GLenum format, GLenum type)
{
    __GLtextureICD *tex;

    tex = CheckTexImageArgs(gc, target, lod, components, border, format, type, 1);
    if (!tex) {
	return 0;
    }
    return IsLegalRange(gc, length, border) ? tex : 0;
}

/* 6909e940 -- __glCheckTexImage2DArgs */
__GLtextureICD *__fastcall __glCheckTexImage2DArgs(__GLcontext *gc, GLenum target, GLint lod, GLint components,
					 GLsizei w, GLsizei h, GLint border, GLenum format, GLenum type)
{
    __GLtextureICD *tex;

    tex = CheckTexImageArgs(gc, target, lod, components, border, format, type, 2);
    if (!tex) {
	return 0;
    }
    if (!IsLegalRange(gc, w, border)) {
	return 0;
    }
    return IsLegalRange(gc, h, border) ? tex : 0;
}

/* 6909e9b0 -- __glCheckCompressedTexImage2DArgs: the glCompressedTexImage2DARB argument check: the active
** unit's texture of the
** target, an FXT1 internal format and legal sizes */
__GLtextureICD *__fastcall __glCheckCompressedTexImage2DArgs(__GLcontext *gc, GLenum target, GLint lod, GLint internalformat,
					 GLsizei w, GLsizei h, GLint border)
{
    __GLtextureICD *tex;

    tex = __glLookUpTexture(gc, target, gc->fx.activeTexture);
    if (!tex) {
	__glSetError(GL_INVALID_ENUM);
	return 0;
    }
    if (internalformat < GL_COMPRESSED_RGB_FXT1_3DFX || internalformat > GL_COMPRESSED_RGBA_FXT1_3DFX) {
	__glSetError(GL_INVALID_VALUE);
    } else if (IsLegalRange(gc, w, border)) {
	return IsLegalRange(gc, h, border) ? tex : 0;
    }
    return 0;
}

/* 6909ec50 -- __gllei_TexImage1D: the ICD replays a list's image through the current dispatch
** table's glTexImage1D (gc is not used: the entry reads the context again) */
void __fastcall __gllei_TexImage1D(__GLcontext *gc, GLenum target, GLint lod, GLint components, GLsizei length,
			     GLint border, GLenum format, GLenum type, const GLubyte *image)
{
    (*(void (APIENTRY *)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *))
	__glCurrentGC()->currentDispatchState.fn[183])
	(target, lod, components, length, border, format, type, image);
}

/* 6909ee00 -- __gllei_TexImage2D: through the current glTexImage2D */
void __fastcall __gllei_TexImage2D(__GLcontext *gc, GLenum target, GLint lod, GLint components, GLsizei w,
			     GLsizei h, GLint border, GLenum format, GLenum type, const GLubyte *image)
{
    (*(void (APIENTRY *)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *))
	__glCurrentGC()->currentDispatchState.fn[184])
	(target, lod, components, w, h, border, format, type, image);
}

/* 6909f3a0 -- __glCalcTexturePersp: both units' s, t and q times 1/w (when not clipped) */
void __fastcall __glCalcTexturePersp(__GLcontext *gc, __GLvertexICD *vx)
{
    (*gc->procs.calcTextureUnit[0])(gc, vx);
    if (!(vx->flagBits & __GL_CLIP_MASK)) {
	vx->texture[0].x = vx->window.w * vx->texture[0].x;
	vx->texture[0].y = vx->texture[0].y * vx->window.w;
	vx->texture[0].w = vx->texture[0].w * vx->window.w;
	vx->texture[1].x = vx->texture[1].x * vx->window.w;
	vx->texture[1].y = vx->texture[1].y * vx->window.w;
	vx->texture[1].w = vx->texture[1].w * vx->window.w;
    }
}

/* 6909f460 -- __glCalcTextureUVScale: s and t times the base level's size (unit 0 is assumed
** to have a texture) */
void __fastcall __glCalcTextureUVScale(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLmipMapLevelICD *lp = __glCurrentTexture(gc, 0)->level[0];

    (*gc->procs.calcTextureUnit[0])(gc, vx);
    vx->texture[0].x = lp->width2f * vx->texture[0].x;
    vx->texture[0].y = lp->height2f * vx->texture[0].y;
    if (__glCurrentTexture(gc, 1)) {
	lp = __glCurrentTexture(gc, 1)->level[0];
	vx->texture[1].x = lp->width2f * vx->texture[1].x;
	vx->texture[1].y = lp->height2f * vx->texture[1].y;
    }
}

/* 6909f4d0 -- __glFastCalcTextureUVScale: the texture matrices only when not identity */
void __fastcall __glFastCalcTextureUVScale(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLmipMapLevelICD *lp = __glCurrentTexture(gc, 0)->level[0];

    if (gc->transform.texture[0]->matrix.matrixType != __GL_MT_IDENTITY ||
	gc->transform.texture[1]->matrix.matrixType != __GL_MT_IDENTITY) {
	(*gc->procs.calcTextureUnit[0])(gc, vx);
    }
    vx->texture[0].x = lp->width2f * vx->texture[0].x;
    vx->texture[0].y = lp->height2f * vx->texture[0].y;
    if (__glCurrentTexture(gc, 1)) {
	lp = __glCurrentTexture(gc, 1)->level[0];
	vx->texture[1].x = lp->width2f * vx->texture[1].x;
	vx->texture[1].y = lp->height2f * vx->texture[1].y;
    }
}

/* 6909f560 -- __glCalcTexturePerspUVScale: UV scale, and 1/w when not clipped */
void __fastcall __glCalcTexturePerspUVScale(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat s0;
    __GLmipMapLevelICD *lp = __glCurrentTexture(gc, 0)->level[0];

    (*gc->procs.calcTextureUnit[0])(gc, vx);
    s0 = lp->width2f * vx->texture[0].x;
    if (!(vx->flagBits & __GL_CLIP_MASK)) {
	vx->texture[0].x = s0 * vx->window.w;
	vx->texture[0].y = lp->height2f * vx->texture[0].y * vx->window.w;
	vx->texture[0].w = vx->texture[0].w * vx->window.w;
	if (__glCurrentTexture(gc, 1)) {
	    lp = __glCurrentTexture(gc, 1)->level[0];
	    vx->texture[1].x = lp->width2f * vx->texture[1].x * vx->window.w;
	    vx->texture[1].y = lp->height2f * vx->texture[1].y * vx->window.w;
	    vx->texture[1].w = vx->texture[1].w * vx->window.w;
	}
    } else {
	vx->texture[0].x = s0;
	vx->texture[0].y = lp->height2f * vx->texture[0].y;
	if (__glCurrentTexture(gc, 1)) {
	    lp = __glCurrentTexture(gc, 1)->level[0];
	    vx->texture[1].x = lp->width2f * vx->texture[1].x;
	    vx->texture[1].y = lp->height2f * vx->texture[1].y;
	}
    }
}

/* 6909f640 -- __glFastCalcTexturePerspUVScale */
void __fastcall __glFastCalcTexturePerspUVScale(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLmipMapLevelICD *lp = __glCurrentTexture(gc, 0)->level[0];

    if (gc->transform.texture[0]->matrix.matrixType != __GL_MT_IDENTITY ||
	gc->transform.texture[1]->matrix.matrixType != __GL_MT_IDENTITY) {
	(*gc->procs.calcTextureUnit[0])(gc, vx);
    }
    if (!(vx->flagBits & __GL_CLIP_MASK)) {
	vx->texture[0].x = lp->width2f * vx->window.w * vx->texture[0].x;
	vx->texture[0].y = lp->height2f * vx->texture[0].y * vx->window.w;
	vx->texture[0].w = vx->texture[0].w * vx->window.w;
	if (__glCurrentTexture(gc, 1)) {
	    lp = __glCurrentTexture(gc, 1)->level[0];
	    vx->texture[1].x = lp->width2f * vx->window.w * vx->texture[1].x;
	    vx->texture[1].y = lp->height2f * vx->window.w * vx->texture[1].y;
	    vx->texture[1].w = vx->texture[1].w * vx->window.w;
	}
    } else {
	vx->texture[0].x = lp->width2f * vx->texture[0].x;
	vx->texture[0].y = lp->height2f * vx->texture[0].y;
	if (__glCurrentTexture(gc, 1)) {
	    lp = __glCurrentTexture(gc, 1)->level[0];
	    vx->texture[1].x = lp->width2f * vx->texture[1].x;
	    vx->texture[1].y = lp->height2f * vx->texture[1].y;
	}
    }
}

/* 6909f740 -- __glInitTexSubImageStore: the image store at an offset into the level */
void __fastcall __glInitTexSubImageStore(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, __GLtextureICD *tex, GLint lod,
			     GLint xoffset, GLint yoffset, GLint zoffset)
{
    __GLmipMapLevelICD *lp = tex->level[lod];

    __glInitTexImageStore(gc, spanInfo, tex, lod);
    spanInfo->dstSkipPixels = lp->border + xoffset;
    spanInfo->dstSkipLines = lp->border + yoffset;
    spanInfo->dstLineLength = lp->width;
}

/* 6909f780 -- __glCheckTexSubImage1DArgs */
__GLtextureICD *__fastcall __glCheckTexSubImage1DArgs(__GLcontext *gc, GLenum target, GLint lod, GLint xoffset,
					 GLint length, GLenum format, GLenum type)
{
    __GLtextureICD *tex;
    __GLmipMapLevelICD *lp;

    tex = CheckTexSubImageArgs(gc, target, lod, format, type, 1);
    if (!tex) {
	return 0;
    }
    lp = tex->level[lod];
    if (lp->buffer == NULL) {
	__glSetError(GL_INVALID_OPERATION);
	return 0;
    }
    return CheckTexSubImageRange(gc, lp, xoffset, length, lp->width) ? tex : 0;
}

/* 6909f9a0 -- __glCheckTexSubImage2DArgs */
__GLtextureICD *__fastcall __glCheckTexSubImage2DArgs(__GLcontext *gc, GLenum target, GLint lod, GLint xoffset,
					 GLint yoffset, GLsizei w, GLsizei h, GLenum format, GLenum type)
{
    __GLtextureICD *tex;
    __GLmipMapLevelICD *lp;

    tex = CheckTexSubImageArgs(gc, target, lod, format, type, 2);
    if (!tex) {
	return 0;
    }
    lp = tex->level[lod];
    if (lp->buffer == NULL) {
	__glSetError(GL_INVALID_OPERATION);
	return 0;
    }
    if (!CheckTexSubImageRange(gc, lp, xoffset, w, lp->width)) {
	return 0;
    }
    return CheckTexSubImageRange(gc, lp, yoffset, h, lp->height) ? tex : 0;
}

/* 6909fa30 -- __glCheckCompressedTexSubImage2DArgs: the glCompressedTexSubImage2DARB argument check: the level
** must exist and have
** the given (compressed) format */
__GLtextureICD *__fastcall __glCheckCompressedTexSubImage2DArgs(__GLcontext *gc, GLenum target, GLint lod, GLint xoffset,
					 GLint yoffset, GLsizei w, GLsizei h, GLenum format)
{
    __GLtextureICD *tex;
    __GLmipMapLevelICD *lp;

    tex = __glLookUpTexture(gc, target, gc->fx.activeTexture);
    if (!tex) {
	__glSetError(GL_INVALID_ENUM);
	return 0;
    }
    lp = tex->level[lod];
    if (lp->buffer == NULL || format != (GLenum)lp->requestedFormat) {
	__glSetError(GL_INVALID_OPERATION);
    } else if (CheckTexSubImageRange(gc, lp, xoffset, w, lp->width)) {
	return CheckTexSubImageRange(gc, lp, yoffset, h, lp->height) ? tex : 0;
    }
    return 0;
}

/* 6909fad0 -- __glCheckCopyTexSubImageArgs */
__GLtextureICD *__fastcall __glCheckCopyTexSubImageArgs(__GLcontext *gc, GLenum target, GLint lod, GLint xoffset,
					 GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei w, GLsizei h,
					 GLint dim)
{
    __GLtextureICD *tex;
    __GLmipMapLevelICD *lp;

    tex = CheckTexSubImageArgs(gc, target, lod, GL_RGBA, GL_FLOAT, dim);
    if (!tex) {
	return 0;
    }
    lp = tex->level[lod];
    if (lp->buffer == NULL) {
	__glSetError(GL_INVALID_OPERATION);
	return 0;
    }
    if (!CheckTexSubImageRange(gc, lp, xoffset, w, lp->width)) {
	return 0;
    }
    if (!CheckTexSubImageRange(gc, lp, yoffset, h, lp->height)) {
	return 0;
    }
    return CheckTexSubImageRange(gc, lp, zoffset, 1, lp->depth) ? tex : 0;
}

/* 6909fcc0 -- __gllei_TexSubImage1D: through the current glTexSubImage1D */
void __fastcall __gllei_TexSubImage1D(__GLcontext *gc, GLenum target, GLint lod, GLint xoffset, GLsizei length,
			     GLenum format, GLenum type, const GLubyte *image)
{
    (*(void (APIENTRY *)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *))
	__glCurrentGC()->currentDispatchState.fn[333])
	(target, lod, xoffset, length, format, type, image);
}

/* 6909fe70 -- __gllei_TexSubImage2D: through the current glTexSubImage2D */
void __fastcall __gllei_TexSubImage2D(__GLcontext *gc, GLenum target, GLint lod, GLint xoffset, GLint yoffset,
			     GLsizei w, GLsizei h, GLenum format, GLenum type, const GLubyte *image)
{
    (*(void (APIENTRY *)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *))
	__glCurrentGC()->currentDispatchState.fn[334])
	(target, lod, xoffset, yoffset, w, h, format, type, image);
}

/* 6909fec0 -- __glim_GenTextures.  A negative n sets GL_INVALID_VALUE but does not return
** (original behaviour, kept): the names are then generated with that n. */
void __stdcall __glim_GenTextures(GLsizei n, GLuint *textures)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (n < 0) {
	__glSetError(GL_INVALID_VALUE);
    }
    if (n != 0 && textures != NULL) {
	__glNamesGenNames(gc, TEX_NAMES(gc), n, textures);
    }
}

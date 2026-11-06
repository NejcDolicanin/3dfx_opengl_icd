/*
** sst_tex.c -- texture entries of the hardware path (__glsstim_TexParameter* / TexEnv* / TexImage* /
** CompressedTex* / BindTexture / GetTexLevelParameter*), the texel formats and the texture coordinate
** calculations.
*/
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "glshade.h"
#include "si_names.h"
#include "icdglob.h"
#include "icdprotos.h"
#include <glide.h>
#include "og.h"
#include <string.h>
#include <stdlib.h>

#pragma warning(disable: 4273)
/* ---- s_texextr.c -------------------------------------------------------------------------- */
#define WIDTHLOG2(level)    (level)->widthLog2
#define __glLookupActiveTexture(gc, target) \
    __glLookUpTexture(gc, target, (gc)->fx.activeTexture)
/* the state's per-target parameter copy (6909c610) has the object parameters' layout;
** 6909c660 returns the bound object's {name, priority} */
#define __glLookUpActiveTextureParams(gc, target) \
    __glLookUpTextureParams(gc, target, (gc)->fx.activeTexture)
#define __glLookUpTextureName(gc, target) \
    __glLookUpTextureTexobjs(gc, target, (gc)->fx.activeTexture)
#define Clampf(v, lo, hi) \
    (((__GLfloat (__fastcall *)(__GLfloat, __GLfloat, __GLfloat))sstTex_Clampf)(v, lo, hi))
#ifndef GL_COMBINE_EXT
#define GL_COMBINE_EXT                  0x8570
#define GL_COMBINE_RGB_EXT              0x8571
#define GL_COMBINE_ALPHA_EXT            0x8572
#define GL_RGB_SCALE_EXT                0x8573
#define GL_ADD_SIGNED_EXT               0x8574
#define GL_INTERPOLATE_EXT              0x8575
#define GL_CONSTANT_EXT                 0x8576
#define GL_PRIMARY_COLOR_EXT            0x8577
#define GL_PREVIOUS_EXT                 0x8578
#define GL_SOURCE0_RGB_EXT              0x8580
#define GL_SOURCE1_RGB_EXT              0x8581
#define GL_SOURCE2_RGB_EXT              0x8582
#define GL_SOURCE0_ALPHA_EXT            0x8588
#define GL_SOURCE1_ALPHA_EXT            0x8589
#define GL_SOURCE2_ALPHA_EXT            0x858A
#define GL_OPERAND0_RGB_EXT             0x8590
#define GL_OPERAND1_RGB_EXT             0x8591
#define GL_OPERAND2_RGB_EXT             0x8592
#define GL_OPERAND0_ALPHA_EXT           0x8598
#define GL_OPERAND1_ALPHA_EXT           0x8599
#define GL_OPERAND2_ALPHA_EXT           0x859A
#endif
#ifndef GL_ADD
#define GL_ADD                          0x0104
#endif
#define ACTIVE_ENV(gc) \
    ((__GLtextureEnvState *)(gc)->state.texture.unit[(gc)->fx.activeTexture].env)
/* ---- texture images, texture coordinates, points and sprites ---- */
typedef __GLmipMapLevelICD *(__fastcall *__GLcreateLevelProc)(__GLcontext *gc, __GLtextureICD *tex, GLint lod,
                                    GLint components, GLsizei w, GLsizei h, GLsizei d, GLint border, GLint dim);
typedef void (__fastcall *__GLcopyTexImageProc)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                    __GLtextureICD *tex, GLint lod);
typedef void (__fastcall *__GLunmakeResidentProc)(__GLcontext *gc, __GLtextureICD *tex, GLint unit);
typedef GLvoid *(__fastcall *SSTCREATELEVEL)(__GLcontext *gc, __GLtextureFxICD *tex, GLint level, GLint components,
                                             GLsizei w, GLsizei h, GLsizei d, GLint border, GLint dim);
/* ---- sst texture objects (createTexture 6903b930: 0x574 bytes): glctx.h __GLtextureFxICD ------------- */
typedef void (__fastcall *XF4PROC)(__GLcoord *res, __GLcoord *v, __GLmatrixICD *m);
/* ---- texture coordinate generation (SI s_tex.c __glCalcMixedTexture, generated in place) -------- */
#define __glLookUpActiveTexture(gc, target) \
    __glLookUpTexture(gc, target, (gc)->fx.activeTexture)
/* 32-bit texels: the board has them and 16-bit textures are not forced (FX_GL_FORCE_16BPP_TEXTURES 1); the unsized
** GL_RGB / GL_RGBA also need 32-bit rendering, unless 32-bit textures are forced (2) */
#define TEX32(gc)           ((gc)->hwcx.hasTexFmt32 && (gc)->fx.force16bppTextures != 1)
#define TEX32_UNSIZED(gc)   (TEX32(gc) && ((gc)->hwcx.has32bpp || (gc)->fx.force16bppTextures == 2))
/* ---- ARB_texture_compression (compressed images are copied as they come) ------------------------- */
extern float __fastcall sstTex_Clampf(GLuint ecx_unused, GLuint edx_unused, float x, float lo, float hi);
extern void __stdcall __glsstim_TexParameterfv(GLenum target, GLenum pname, const GLfloat pv[]);
extern void __stdcall __glsstim_TexParameteriv(GLenum target, GLenum pname, const GLint pv[]);
extern void __stdcall __glsstim_TexEnvfv(GLenum target, GLenum pname, const GLfloat pv[]);

/* 6904d660 -- __glSSTExtractTexelRGBA8: s_texextr.c __glExtractTexelRGBA8 for the hardware texture images: the
** texels are stored B, G, R, A */
void __fastcall __glSSTExtractTexelRGBA8(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;

    if ((row < 0) || (col < 0) ||
	(row >= level->height2) || (col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = (GLubyte) (255.0F * tex->params.borderColor.r);
	result->g = (GLubyte) (255.0F * tex->params.borderColor.g);
	result->b = (GLubyte) (255.0F * tex->params.borderColor.b);
	result->a = (GLubyte) (255.0F * tex->params.borderColor.a);
    } else {
	image = (GLubyte *)level->buffer + ((row << WIDTHLOG2(level)) + col) * 4;
	result->a = image[3];
	result->r = image[2];
	result->g = image[1];
	result->b = image[0];
    }
}

/* 6905cf50 -- __glsstim_TexParameterfv: __glim_TexParameterfv of the hardware path (6909d160 plus): a new min
** filter clears the object's per-TMU resident flags, the priority is clamped by the module's own Clampf
** (6905d1c0), and the unit's cached hardware texture is dropped and the object's texture key rebuilt
** (wglGlideTexKey) */
void __stdcall __glsstim_TexParameterfv(GLenum target, GLenum pname, const GLfloat pv[])
{
    GLenum e;
    GLint i;
    __GLtexObjParamICD *params;
    __GLtextureICD *tex;
    __GLtexNamePriICD *texName;
    __GL_SETUP_NOT_IN_BEGIN();

    params = __glLookUpActiveTextureParams(gc, target);
    if (!params) goto bad_enum;
    tex = __glLookupActiveTexture(gc, target);
    if (tex->refCount > 2) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }

    switch (pname) {
      case GL_TEXTURE_WRAP_S:
        switch (e = (GLenum) pv[0]) {
          case GL_REPEAT:
          case GL_CLAMP:
          case GL_CLAMP_TO_EDGE:
            params->sWrapMode = e;
            tex->params.sWrapMode = e;
            break;
          default:
            goto bad_enum;
        }
        break;
      case GL_TEXTURE_WRAP_T:
        switch (e = (GLenum) pv[0]) {
          case GL_REPEAT:
          case GL_CLAMP:
          case GL_CLAMP_TO_EDGE:
            params->tWrapMode = e;
            tex->params.tWrapMode = e;
            break;
          default:
            goto bad_enum;
        }
        break;
      case GL_TEXTURE_MIN_FILTER:
        switch (e = (GLenum) pv[0]) {
          case GL_NEAREST:
          case GL_LINEAR:
          case GL_NEAREST_MIPMAP_NEAREST:
          case GL_LINEAR_MIPMAP_NEAREST:
          case GL_NEAREST_MIPMAP_LINEAR:
          case GL_LINEAR_MIPMAP_LINEAR:
            params->minFilter = e;
            tex->params.minFilter = e;
            for (i = 0; i < gc->fx.numTMUs; i++) {
                tex->hwHandle[__GL_TMU_SLOT(i)] = 0;
            }
            break;
          default:
            goto bad_enum;
        }
        break;
      case GL_TEXTURE_MAG_FILTER:
        switch (e = (GLenum) pv[0]) {
          case GL_NEAREST:
          case GL_LINEAR:
            params->magFilter = e;
            tex->params.magFilter = e;
            break;
          default:
            goto bad_enum;
        }
        break;
      case GL_TEXTURE_BORDER_COLOR:
        __glClampColorf(gc, &params->borderColor, pv);
        tex->params.borderColor = params->borderColor;
        break;
      case GL_TEXTURE_PRIORITY:
        texName = __glLookUpTextureName(gc, target);
        texName->priority = Clampf(pv[0], __GL_ZERO_F, __GL_ONE_F);
        break;
      default:
      bad_enum:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->hwcx.unitTexture[gc->fx.activeTexture] = (void *)-1;
    __GL_DELAY_VALIDATE(gc);
    wglGlideTexKey(gc, (BYTE *)tex);
}

/* 6905d1c0 -- sstTex_Clampf (SST_TEX.C Clampf): x clamped to [lo, hi] */
float __fastcall sstTex_Clampf(GLuint ecx_unused, GLuint edx_unused, float x, float lo, float hi)
{
  if (x < lo) {
    return lo;
  }
  if (x > hi) {
    return hi;
  }
  return x;
}

/* 6905d200 -- __glsstim_TexParameterf: glTexParameterf (hw) */
void __stdcall __glsstim_TexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_PRIORITY:
        __glsstim_TexParameterfv(target, pname, &param);
        return;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 6905d4b0 -- __glsstim_TexParameteri: glTexParameteri (hw) */
void __stdcall __glsstim_TexParameteri(GLenum target, GLenum pname, GLint param)
{
    switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_PRIORITY:
        __glsstim_TexParameteriv(target, pname, &param);
        return;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 6905d4f0 -- __glsstim_TexEnvfv: __glim_TexEnvfv (s_tex.c), the hardware module's copy of 6909d680 that also
** drops the unit's cached hardware texture (hwcx.unitTexture[unit] = -1); GL_COMBINE_EXT is accepted here only */
void __stdcall __glsstim_TexEnvfv(GLenum target, GLenum pname, const GLfloat pv[])
{
    __GLtextureEnvState *tes;
    GLenum e;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    if(target < GL_TEXTURE_ENV) {
      __glSetError(GL_INVALID_ENUM);
      return;
    }
    target -= GL_TEXTURE_ENV;
    if (target >= (GLuint) gc->constants.numberOfTextureEnvs) {
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    tes = &ACTIVE_ENV(gc)[target];

    switch (pname) {
      case GL_TEXTURE_ENV_MODE:
	switch(e = (GLenum) pv[0]) {
	  case GL_MODULATE:
	  case GL_DECAL:
	  case GL_BLEND:
	  case GL_REPLACE:
	  case GL_ADD:
	  case GL_COMBINE_EXT:
	    tes->mode = e;
	    break;
	  default:
	    goto bad_enum;
	}
	break;
      case GL_ALPHA_SCALE:
	if (pv[0] != 1.0 && pv[0] != 2.0 && pv[0] != 4.0) {
	    __glSetError(GL_INVALID_VALUE);
	    return;
	}
	tes->alphaScale = pv[0];
	break;
      case GL_TEXTURE_ENV_COLOR:
	__glClampAndScaleColorf(gc, &tes->color, pv);
	tes->color.r *= gc->constants.redRescale;
	tes->color.g *= gc->constants.greenRescale;
	tes->color.b *= gc->constants.blueRescale;
	tes->color.a *= gc->constants.alphaRescale;
	break;
      case GL_COMBINE_RGB_EXT:
	switch(e = (GLenum) pv[0]) {
	  case GL_REPLACE:
	  case GL_MODULATE:
	  case GL_ADD:
	  case GL_ADD_SIGNED_EXT:
	  case GL_INTERPOLATE_EXT:
	    tes->combineRGB = e;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_COMBINE_ALPHA_EXT:
	switch(e = (GLenum) pv[0]) {
	  case GL_REPLACE:
	  case GL_MODULATE:
	  case GL_ADD:
	  case GL_ADD_SIGNED_EXT:
	  case GL_INTERPOLATE_EXT:
	    tes->combineAlpha = e;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_SOURCE0_RGB_EXT:
	e = (GLenum) pv[0];
	if (e == GL_TEXTURE || (e > GL_INTERPOLATE_EXT && e <= GL_PREVIOUS_EXT)) {
	    tes->sourceRGB[0] = e;
	    break;
	}
	goto bad_enum;
      case GL_SOURCE1_RGB_EXT:
	e = (GLenum) pv[0];
	if (e == GL_TEXTURE || (e > GL_INTERPOLATE_EXT && e <= GL_PREVIOUS_EXT)) {
	    tes->sourceRGB[1] = e;
	    break;
	}
	goto bad_enum;
      case GL_SOURCE2_RGB_EXT:
	e = (GLenum) pv[0];
	if (e == GL_TEXTURE || (e > GL_INTERPOLATE_EXT && e <= GL_PREVIOUS_EXT)) {
	    tes->sourceRGB[2] = e;
	    break;
	}
	goto bad_enum;
      case GL_RGB_SCALE_EXT:
	if (pv[0] != 1.0 && pv[0] != 2.0 && pv[0] != 4.0) {
	    __glSetError(GL_INVALID_VALUE);
	    return;
	}
	tes->rgbScale = pv[0];
	break;
      case GL_SOURCE0_ALPHA_EXT:
	e = (GLenum) pv[0];
	if (e == GL_TEXTURE || (e > GL_INTERPOLATE_EXT && e <= GL_PREVIOUS_EXT)) {
	    tes->sourceAlpha[0] = e;
	    break;
	}
	goto bad_enum;
      case GL_SOURCE1_ALPHA_EXT:
	e = (GLenum) pv[0];
	if (e == GL_TEXTURE || (e > GL_INTERPOLATE_EXT && e <= GL_PREVIOUS_EXT)) {
	    tes->sourceAlpha[1] = e;
	    break;
	}
	goto bad_enum;
      case GL_SOURCE2_ALPHA_EXT:
	e = (GLenum) pv[0];
	if (e == GL_TEXTURE || (e > GL_INTERPOLATE_EXT && e <= GL_PREVIOUS_EXT)) {
	    tes->sourceAlpha[2] = e;
	    break;
	}
	goto bad_enum;
      case GL_OPERAND0_RGB_EXT:
	e = (GLenum) pv[0];
	if (e >= GL_SRC_COLOR && e <= GL_ONE_MINUS_SRC_ALPHA) {
	    tes->operandRGB[0] = e;
	    break;
	}
	goto bad_enum;
      case GL_OPERAND1_RGB_EXT:
	e = (GLenum) pv[0];
	if (e >= GL_SRC_COLOR && e <= GL_ONE_MINUS_SRC_ALPHA) {
	    tes->operandRGB[1] = e;
	    break;
	}
	goto bad_enum;
      case GL_OPERAND2_RGB_EXT:
	e = (GLenum) pv[0];
	if (e == GL_SRC_ALPHA) {
	    tes->operandRGB[2] = e;
	    break;
	}
	goto bad_enum;
      case GL_OPERAND0_ALPHA_EXT:
	e = (GLenum) pv[0];
	if (e >= GL_SRC_ALPHA && e <= GL_ONE_MINUS_SRC_ALPHA) {
	    tes->operandAlpha[0] = e;
	    break;
	}
	goto bad_enum;
      case GL_OPERAND1_ALPHA_EXT:
	e = (GLenum) pv[0];
	if (e >= GL_SRC_ALPHA && e <= GL_ONE_MINUS_SRC_ALPHA) {
	    tes->operandAlpha[1] = e;
	    break;
	}
	goto bad_enum;
      case GL_OPERAND2_ALPHA_EXT:
	e = (GLenum) pv[0];
	if (e == GL_SRC_ALPHA) {
	    tes->operandAlpha[2] = e;
	    break;
	}
	goto bad_enum;
      default:
	goto bad_enum;
    }
    gc->hwcx.unitTexture[gc->fx.activeTexture] = (void *)-1;    /* the hardware path: re-send the unit's texture */
    __GL_DELAY_VALIDATE(gc);
}

/* 6905da40 (size 119) -- __glsstim_TexEnvf: glTexEnvf (scalar parameters only) */
void __stdcall __glsstim_TexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    switch (pname) {
      case GL_TEXTURE_ENV_MODE:
      case GL_ALPHA_SCALE:
      case 0x8571: case 0x8572: case 0x8573:
      case 0x8580: case 0x8581: case 0x8582:
      case 0x8588: case 0x8589: case 0x858a:
      case 0x8590: case 0x8591: case 0x8592:
      case 0x8598: case 0x8599: case 0x859a:
        ((void (__stdcall *)(GLenum, GLenum, const GLfloat *))__glsstim_TexEnvfv)(target, pname, &param);
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 6905dae0 (size 1201) -- __glsstim_TexEnviv: glTexEnviv */
void __stdcall __glsstim_TexEnviv(GLenum target, GLenum pname, const GLint *pv)
{
    __GLtextureEnvState *tes;
    GLint v;

    __GL_SETUP_NOT_IN_BEGIN();

    if (target < GL_TEXTURE_ENV ||
        (target -= GL_TEXTURE_ENV, target >= (GLenum)gc->constants.numberOfTextureEnvs)) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    tes = &((__GLtextureEnvState *)gc->state.texture.unit[gc->fx.activeTexture].env)[target];
    switch (pname) {
      case GL_TEXTURE_ENV_MODE:
        switch ((GLenum)pv[0]) {
          case GL_MODULATE:
          case GL_DECAL:
          case GL_BLEND:
          case GL_REPLACE:
          case GL_ADD:
          case 0x8570:
            tes->mode = (GLenum)pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case GL_ALPHA_SCALE:
        v = pv[0];
        if (v != 1.0 && v != 2.0 && v != 4.0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        tes->alphaScale = (__GLfloat)v;
        break;
      case GL_TEXTURE_ENV_COLOR:
        __glClampAndScaleColori(gc, &tes->color, pv);
        tes->color.r = gc->constants.redRescale * tes->color.r;
        tes->color.g = gc->constants.greenRescale * tes->color.g;
        tes->color.b = gc->constants.blueRescale * tes->color.b;
        tes->color.a = gc->constants.alphaRescale * tes->color.a;
        break;
      case 0x8571:     /* GL_COMBINE_RGB_EXT */
        switch ((GLenum)pv[0]) {
          case GL_REPLACE:
          case GL_MODULATE:
          case GL_ADD:
          case 0x8574:
          case 0x8575:
            tes->combineRGB = (GLenum)pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case 0x8572:     /* GL_COMBINE_ALPHA_EXT */
        switch ((GLenum)pv[0]) {
          case GL_REPLACE:
          case GL_MODULATE:
          case GL_ADD:
          case 0x8574:
          case 0x8575:
            tes->combineAlpha = (GLenum)pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case 0x8580:     /* GL_SOURCE0_RGB_EXT */
        switch ((GLenum)pv[0]) {
          case GL_TEXTURE: case 0x8576: case 0x8577: case 0x8578:
            tes->sourceRGB[0] = (GLenum)pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case 0x8581:
        switch ((GLenum)pv[0]) {
          case GL_TEXTURE: case 0x8576: case 0x8577: case 0x8578:
            tes->sourceRGB[1] = (GLenum)pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case 0x8582:
        switch ((GLenum)pv[0]) {
          case GL_TEXTURE: case 0x8576: case 0x8577: case 0x8578:
            tes->sourceRGB[2] = (GLenum)pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case 0x8573:     /* GL_RGB_SCALE_EXT */
        v = pv[0];
        if (v != 1.0 && v != 2.0 && v != 4.0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        tes->rgbScale = (__GLfloat)v;
        break;
      case 0x8588:     /* GL_SOURCE0_ALPHA_EXT */
        switch ((GLenum)pv[0]) {
          case GL_TEXTURE: case 0x8576: case 0x8577: case 0x8578:
            tes->sourceAlpha[0] = (GLenum)pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case 0x8589:
        switch ((GLenum)pv[0]) {
          case GL_TEXTURE: case 0x8576: case 0x8577: case 0x8578:
            tes->sourceAlpha[1] = (GLenum)pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case 0x858a:
        switch ((GLenum)pv[0]) {
          case GL_TEXTURE: case 0x8576: case 0x8577: case 0x8578:
            tes->sourceAlpha[2] = (GLenum)pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case 0x8590:     /* GL_OPERAND0_RGB_EXT */
        switch ((GLenum)pv[0]) {
          case GL_SRC_COLOR: case GL_ONE_MINUS_SRC_COLOR: case GL_SRC_ALPHA: case GL_ONE_MINUS_SRC_ALPHA:
            tes->operandRGB[0] = (GLenum)pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case 0x8591:
        switch ((GLenum)pv[0]) {
          case GL_SRC_COLOR: case GL_ONE_MINUS_SRC_COLOR: case GL_SRC_ALPHA: case GL_ONE_MINUS_SRC_ALPHA:
            tes->operandRGB[1] = (GLenum)pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case 0x8592:
        v = pv[0];
        if ((GLenum)v == GL_SRC_ALPHA) {
            tes->operandRGB[2] = v;
            break;
        }
        __glSetError(GL_INVALID_ENUM);
        return;
      case 0x8598:     /* GL_OPERAND0_ALPHA_EXT */
        switch ((GLenum)pv[0]) {
          case GL_SRC_ALPHA: case GL_ONE_MINUS_SRC_ALPHA:
            tes->operandAlpha[0] = (GLenum)pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case 0x8599:
        switch ((GLenum)pv[0]) {
          case GL_SRC_ALPHA: case GL_ONE_MINUS_SRC_ALPHA:
            tes->operandAlpha[1] = (GLenum)pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case 0x859a:
        v = pv[0];
        if ((GLenum)v == GL_SRC_ALPHA) {
            tes->operandAlpha[2] = v;
            break;
        }
        __glSetError(GL_INVALID_ENUM);
        return;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->hwcx.unitTexture[gc->fx.activeTexture] = (void *)-1;
    __GL_DELAY_VALIDATE(gc);
}

/* 6905dff0 (size 119) -- __glsstim_TexEnvi: glTexEnvi (scalar parameters only) */
void __stdcall __glsstim_TexEnvi(GLenum target, GLenum pname, GLint param)
{
    switch (pname) {
      case GL_TEXTURE_ENV_MODE:
      case GL_ALPHA_SCALE:
      case 0x8571: case 0x8572: case 0x8573:
      case 0x8580: case 0x8581: case 0x8582:
      case 0x8588: case 0x8589: case 0x858a:
      case 0x8590: case 0x8591: case 0x8592:
      case 0x8598: case 0x8599: case 0x859a:
        ((void (__stdcall *)(GLenum, GLenum, const GLint *))__glsstim_TexEnviv)(target, pname, &param);
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 6905e090 -- s_tex.c __glsstim_TexImage1D of the sst path: no validation, no pixel-path width adjustment;
** the texture is taken off every TMU */
void __stdcall __glsstim_TexImage1D(GLenum target, GLint lod, GLint components, GLsizei length, GLint border,
                            GLenum format, GLenum type, const GLvoid *buf)
{
    __GLtextureICD *tex;
    __GLmipMapLevelICD *dest;
    GLint i;
    __GLpixelSpanInfoICD spanInfo;
    __GL_SETUP_NOT_IN_BEGIN();

    tex = __glCheckTexImage1DArgs(gc, target, lod, components, length, border, format, type);
    if (tex) {
        if (tex->refCount > 2) {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
        dest = (*(__GLcreateLevelProc)tex->createLevel)(gc, tex, lod, components,
                                            length, 1+border*2, 1+border*2, border, 1);
        if (buf && dest) {
            __glInitTexSourceUnpack(gc, &spanInfo, length, 1, 1, format, type, buf, GL_FALSE);
            __glInitTexImageStore(gc, &spanInfo, tex, lod);
            __glInitUnpacker(gc, &spanInfo);
            __glInitPacker(gc, &spanInfo);
            (*(__GLcopyTexImageProc)tex->copyTexImage)(gc, &spanInfo, tex, lod);
        }
        for (i = 0; i < gc->fx.numTMUs; i++) {
            (*(__GLunmakeResidentProc)tex->unmakeResident)(gc, tex, i);
        }
        __GL_DELAY_VALIDATE(gc);
    }
}

/* 6905e1f0 (size 604) -- __glsstim_TexImage2D: glTexImage2D, hardware path (1x1 .. 2x2 RGBA levels are replicated
** to 4x4) */
void __stdcall __glsstim_TexImage2D(GLenum target, GLint level, GLint components,
                             GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type,
                             const GLvoid *pixels)
{
    __GLpixelSpanInfoICD spanInfo;
    GLuint buf[16];
    __GLtextureFxICD *tex;
    GLuint *p;
    GLint x, y, i, j;
    GLvoid *dest;

    __GL_SETUP_NOT_IN_BEGIN();

    if (components == 0x84ed) {
        components = 0x86b0;
    } else if (components == 0x84ee) {
        components = 0x86b1;
    }
    tex = (__GLtextureFxICD *)__glCheckTexImage2DArgs(gc, target, level, components, width, height, border, format, type);
    if (!tex) return;
    if (tex->t.refCount > 2) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }
    if (format == GL_RGBA && type == GL_UNSIGNED_BYTE && width <= 2 && height <= 2 && level <= 4 &&
        gc->fx.fullscreen == 1) {
        p = buf;
        for (y = 0; y < height; y++) {
            for (j = 0; j < 4 / height; j++) {
                for (x = 0; x < width; x++) {
                    for (i = 0; i < 4 / width; i++) {
                        *p++ = ((const GLuint *)pixels)[y * width + x];
                    }
                }
            }
        }
        width = 4;
        height = 4;
        pixels = buf;
    }
    dest = (*(SSTCREATELEVEL)tex->t.createLevel)(gc, tex, level, components, width, height, 2 * border + 1,
                                                 border, 2);
    if (pixels && dest) {
        __glInitTexSourceUnpack(gc, &spanInfo, width, height, 1, format, type, pixels, 0);
        __glInitTexImageStore(gc, &spanInfo, &tex->t, level);
        __glInitUnpacker(gc, &spanInfo);
        __glInitPacker(gc, &spanInfo);
        ((void (__fastcall *)(__GLcontext *, void *, __GLtextureFxICD *, GLint))tex->t.copyTexImage)(gc, &spanInfo, tex, level);
    }
    for (i = 0; i < gc->fx.numTMUs; i++) {
        (*tex->t.unmakeResident)(gc, tex, i);
    }
    __GL_DELAY_VALIDATE(gc);
}

/* 6905e450 -- s_tex.c __glsstim_TexSubImage1D of the sst path (as gl_tex.c 6909fb80): no validation, no
** pixel-path width adjustment; the texture is taken off every TMU */
void __stdcall __glsstim_TexSubImage1D(GLenum target, GLint lod, GLint xoffset, GLint length, GLenum format,
                            GLenum type, const GLvoid *buf)
{
    __GLtextureICD *tex;
    GLint i;
    __GLpixelSpanInfoICD spanInfo;
    __GL_SETUP_NOT_IN_BEGIN();

    tex = __glCheckTexSubImage1DArgs(gc, target, lod, xoffset, length, format, type);
    if (tex) {
        __glInitTexSourceUnpack(gc, &spanInfo, length, 1, 1, format, type, buf, GL_FALSE);
        __glInitTexSubImageStore(gc, &spanInfo, tex, lod, xoffset, 0, 0);
        __glInitUnpacker(gc, &spanInfo);
        __glInitPacker(gc, &spanInfo);
        (*(__GLcopyTexImageProc)tex->copyTexImage)(gc, &spanInfo, tex, lod);
        for (i = 0; i < gc->fx.numTMUs; i++) {
            (*(__GLunmakeResidentProc)tex->unmakeResident)(gc, tex, i);
        }
        __GL_DELAY_VALIDATE(gc);
    }
}

/* 6905e570 (size 996) -- __glsstim_TexSubImage2D: glTexSubImage2D, hardware path */
void __stdcall __glsstim_TexSubImage2D(GLenum target, GLint level, GLint xoffset,
                             GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type,
                             const GLvoid *pixels)
{
    __GLpixelSpanInfoICD spanInfo;
    __GLtextureFxICD *tex;
    __GLmipMapLevelICD *lp;
    const __GLtextureFormatICD *li;
    GLubyte *buf;
    GLint i, t0, t1;

    __GL_SETUP_NOT_IN_BEGIN();

    tex = (__GLtextureFxICD *)__glCheckTexSubImage2DArgs(gc, target, level, xoffset, yoffset, width, height, format, type);
    if (!tex) return;

    if (format == GL_RGBA && type == GL_UNSIGNED_BYTE &&
        (lp = tex->t.level[level], (lp)->texFormat->pixelType == 0)) {
        const GLubyte *src = (const GLubyte *)pixels;
        GLint stride = lp->width2;
        GLushort *dst = (GLushort *)lp->buffer + (stride * yoffset + xoffset);
        GLint skip = stride - width;
        GLint x, y;

        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                *dst++ = (GLushort)(((GLushort)(src[0] & 0xf8) << 5 | (GLushort)src[1] & 0xfffc) << 3 |
                                    (GLushort)(src[2] >> 3));
                src += 4;
            }
            dst += skip;
        }
    } else {
        __glInitTexSourceUnpack(gc, &spanInfo, width, height, 1, format, type, pixels, 0);
        __glInitTexSubImageStore(gc, &spanInfo, &tex->t, level, xoffset, yoffset, 0);
        __glInitUnpacker(gc, &spanInfo);
        __glInitPacker(gc, &spanInfo);
        ((void (__fastcall *)(__GLcontext *, void *, __GLtextureFxICD *, GLint))tex->t.copyTexImage)(gc, &spanInfo, tex, level);
    }

    if (tex->wideAspect) {
        for (i = 0; i < gc->fx.numTMUs; i++) {
            if (tex->onTmu[__GL_TMU_SLOT(i)]) {
                (*tex->t.unmakeResident)(gc, tex, i);
            }
        }
        __GL_DELAY_VALIDATE(gc);
        return;
    }
    lp = tex->t.level[level];
    li = lp->texFormat;
    if (lp->compressedImage != NULL) {
        /* fix: a compressed (FXT1) level was re-encoded whole by copyTexImage: upload all its 4-row blocks from the
        ** compressed image, as the full download does (the original sent rows of the uncompressed copy as FXT1) */
        buf = (GLubyte *)lp->compressedImage;
        t0 = 0;
        t1 = ((lp->height + 3) & ~3) - 1;
    } else {
        buf = (GLubyte *)lp->buffer + __glBytesPerElement(li->pixelType) * lp->width2 * yoffset;
        t0 = yoffset;
        t1 = yoffset + height - 1;
    }
    if (g_tmuSharedMemory) {
        if (tex->onTmu[0]) {
            if (gc->hwcx.windowed) {
                grSurfaceSetTextureSurfaceExt(gc->fx.tmuTexUnit[gc->fx.activeTexture], tex->mem[0]->hwSurface);
            }
            hwTexDownloadLevel(gc, tex, gc->fx.activeTexture, level, li->grFormat, buf, t0, t1);
        }
    } else {
        for (i = 0; i < gc->fx.numTMUs; i++) {
            if (tex->onTmu[i]) {
                if (gc->hwcx.windowed) {
                    grSurfaceSetTextureSurfaceExt(gc->fx.tmuTexUnit[i], tex->mem[__GL_TMU_SLOT(i)]->hwSurface);
                }
                hwTexDownloadRows(gc->fx.tmuTexUnit[i], tex->mem[__GL_TMU_SLOT(i)]->start, level,
                             tex->t.maxLevel, tex->aspectRatioLog2, li->grFormat, buf, t0, t1);
            }
        }
    }
}

/* 6905ec20 -- __glsstim_BindTexture of the sst path: GL_TEXTURE_1D / 2D to the target index 0 / 1 for
** __glBindTexture, then the texture state is revalidated */
void __stdcall __glsstim_BindTexture(GLenum target, GLuint texture)
{
    GLuint targetIndex;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (target) {
    case GL_TEXTURE_1D:
        targetIndex = 0;
        break;
    case GL_TEXTURE_2D:
        targetIndex = 1;
        break;
    default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    __glBindTexture(gc, targetIndex, texture);
    __GL_DELAY_VALIDATE_MASK(gc, 0x800);
}

extern void __fastcall sstTex_SphereGen(__GLcontext *gc, __GLvertexICD *vx, __GLcoord *result);
/* 6905ed30 (size 656) -- __glSSTCalcMixedTexture: __glCalcMixedTexture over every TMU (SI s_tex.c form, gen copy) */
void __fastcall __glSSTCalcMixedTexture(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLcoord sphereCoord, gen, *c;
    GLboolean didSphereGen = GL_FALSE;
    __GLmatrixICD *m;
    GLuint enables;
    GLint unit;

    for (unit = 0; unit < gc->fx.numTMUs; unit++) {
        enables = gc->state.enables.texture[unit];
        /* Generate/copy s coordinate */
        if (enables & 4) {
            switch (gc->state.texture.unit[unit].s.mode) {
              case GL_EYE_LINEAR:
                c = &gc->state.texture.unit[unit].s.eyePlaneEquation;
                gen.x = c->x * vx->eye.x + c->y * vx->eye.y + c->z * vx->eye.z + c->w * vx->eye.w;
                break;
              case GL_OBJECT_LINEAR:
                c = &gc->state.texture.unit[unit].s.objectPlaneEquation;
                gen.x = c->x * vx->obj.x + c->y * vx->obj.y + c->z * vx->obj.z + c->w * vx->obj.w;
                break;
              case GL_SPHERE_MAP:
                sstTex_SphereGen(gc, vx, &sphereCoord);
                gen.x = sphereCoord.x;
                didSphereGen = GL_TRUE;
                break;
            }
        } else {
            gen.x = vx->texture[unit].x;
        }
        /* Generate/copy t coordinate */
        if (enables & 8) {
            switch (gc->state.texture.unit[unit].t.mode) {
              case GL_EYE_LINEAR:
                c = &gc->state.texture.unit[unit].t.eyePlaneEquation;
                gen.y = c->x * vx->eye.x + c->y * vx->eye.y + c->z * vx->eye.z + c->w * vx->eye.w;
                break;
              case GL_OBJECT_LINEAR:
                c = &gc->state.texture.unit[unit].t.objectPlaneEquation;
                gen.y = c->x * vx->obj.x + c->y * vx->obj.y + c->z * vx->obj.z + c->w * vx->obj.w;
                break;
              case GL_SPHERE_MAP:
                if (!didSphereGen) {
                    sstTex_SphereGen(gc, vx, &sphereCoord);
                }
                gen.y = sphereCoord.y;
                break;
            }
        } else {
            gen.y = vx->texture[unit].y;
        }
        /* Generate/copy r coordinate */
        if (enables & __GL_TEXTURE_GEN_R_ENABLE) {
            switch (gc->state.texture.unit[unit].r.mode) {
              case GL_EYE_LINEAR:
                c = &gc->state.texture.unit[unit].r.eyePlaneEquation;
                gen.z = c->x * vx->eye.x + c->y * vx->eye.y + c->z * vx->eye.z + c->w * vx->eye.w;
                break;
              case GL_OBJECT_LINEAR:
                c = &gc->state.texture.unit[unit].r.objectPlaneEquation;
                gen.z = c->x * vx->obj.x + c->y * vx->obj.y + c->z * vx->obj.z + c->w * vx->obj.w;
                break;
            }
        } else {
            gen.z = vx->texture[unit].z;
        }
        /* Generate/copy q coordinate */
        if (enables & __GL_TEXTURE_GEN_Q_ENABLE) {
            switch (gc->state.texture.unit[unit].q.mode) {
              case GL_EYE_LINEAR:
                c = &gc->state.texture.unit[unit].q.eyePlaneEquation;
                gen.w = c->x * vx->eye.x + c->y * vx->eye.y + c->z * vx->eye.z + c->w * vx->eye.w;
                break;
              case GL_OBJECT_LINEAR:
                c = &gc->state.texture.unit[unit].q.objectPlaneEquation;
                gen.w = c->x * vx->obj.x + c->y * vx->obj.y + c->z * vx->obj.z + c->w * vx->obj.w;
                break;
            }
        } else {
            gen.w = vx->texture[unit].w;
        }

        /* Finally, apply texture matrix */
        m = (__GLmatrixICD *)gc->transform.texture[unit];
        (*(XF4PROC)m->xf4)(&vx->texture[unit], &gen, m);
    }
}

/* 6905efc0 (size 439) -- sstTex_SphereGen: SphereGen (normalises the vertex normal first when GL_NORMALIZE is on) */
void __fastcall sstTex_SphereGen(__GLcontext *gc, __GLvertexICD *vx, __GLcoord *result)
{
    __GLcoord u, r;
    __GLfloat m, ndotu;

    /* Get unit vector from origin to the vertex in eye coordinates into u */
    (*gc->procs.normalize)(&u.x, &vx->eye.x);

    if (gc->state.enables.general & __GL_NORMALIZE_ENABLE) {
        __GLfloat x, y, z, len2;

        z = vx->normal.z;                       /* loaded z, y, x: the original's summation order */
        y = vx->normal.y;
        x = vx->normal.x;
        len2 = x * x + y * y + z * z;
        if (len2 <= 0.0f) {
            vx->normal.x = 0.0f;
            vx->normal.y = 0.0f;
            vx->normal.z = 0.0f;
        } else if (len2 != 1.0f) {
            __GLfloat g, t, a, s;

            g = len2;
            *(GLuint *)&g = 0x5f375a00 - (*(GLuint *)&g >> 1);
            t = g * g * len2;
            a = 3.0f - t;
            s = (12.0f - a * a * t) * a * g * 0.0625f;
            vx->normal.x = s * vx->normal.x;
            vx->normal.y = s * vx->normal.y;
            vx->normal.z = s * vx->normal.z;
        }
    }

    /* Dot the normal with the unit position u */
    ndotu = (GLfloat)dotProduct3d(&vx->normal, &u);

    /* Compute r */
    r.x = u.x - 2 * (ndotu * vx->normal.x);
    r.y = u.y - 2 * (ndotu * vx->normal.y);
    r.z = u.z - 2 * (ndotu * vx->normal.z);

    /* Compute m */
    m = 2 * __GL_SQRTF(r.x * r.x + r.y * r.y + (r.z + 1) * (r.z + 1));

    if (m) {
        result->x = r.x / m + gc->constants.half;
        result->y = r.y / m + gc->constants.half;
    } else {
        result->x = (__GLfloat)(gc->constants.half);
        result->y = (__GLfloat)(gc->constants.half);
    }
}

/* 6905f1a0 (size 711) -- __glSSTCalcMixedTextureUnit: __glCalcMixedTexture (gc, vx, unit) */
void __fastcall __glSSTCalcMixedTextureUnit(__GLcontext *gc, __GLvertexICD *vx, GLint unit)
{
    __GLcoord sphereCoord, gen;
    __GLcoord *c, *tc;
    GLboolean didSphereGen;
    __GLmatrixICD *m;
    GLuint enables;

    didSphereGen = GL_FALSE;
    tc = &vx->texture[unit];
    enables = gc->state.enables.texture[unit];
    if (enables & 4) {
        switch (gc->state.texture.unit[unit].s.mode) {
          case GL_EYE_LINEAR:
            c = &gc->state.texture.unit[unit].s.eyePlaneEquation;
            tc->x = c->x * vx->eye.x + c->y * vx->eye.y + c->z * vx->eye.z + c->w * vx->eye.w;
            break;
          case GL_OBJECT_LINEAR:
            c = &gc->state.texture.unit[unit].s.objectPlaneEquation;
            tc->x = vx->obj.x * c->x + vx->obj.y * c->y + vx->obj.z * c->z + vx->obj.w * c->w;
            break;
          case GL_SPHERE_MAP:
            sstTex_SphereGen(gc, vx, &sphereCoord);
            didSphereGen = GL_TRUE;
            tc->x = sphereCoord.x;
            break;
        }
    }
    if (enables & 8) {
        switch (gc->state.texture.unit[unit].t.mode) {
          case GL_EYE_LINEAR:
            c = &gc->state.texture.unit[unit].t.eyePlaneEquation;
            tc->y = c->x * vx->eye.x + c->y * vx->eye.y + c->z * vx->eye.z + c->w * vx->eye.w;
            break;
          case GL_OBJECT_LINEAR:
            c = &gc->state.texture.unit[unit].t.objectPlaneEquation;
            tc->y = vx->obj.x * c->x + vx->obj.y * c->y + vx->obj.z * c->z + vx->obj.w * c->w;
            break;
          case GL_SPHERE_MAP:
            if (!didSphereGen) {
                sstTex_SphereGen(gc, vx, &sphereCoord);
            }
            tc->y = sphereCoord.y;
            break;
        }
    }
    if (enables & __GL_TEXTURE_GEN_R_ENABLE) {
        switch (gc->state.texture.unit[unit].r.mode) {
          case GL_EYE_LINEAR:
            c = &gc->state.texture.unit[unit].r.eyePlaneEquation;
            tc->z = c->x * vx->eye.x + c->y * vx->eye.y + c->z * vx->eye.z + c->w * vx->eye.w;
            break;
          case GL_OBJECT_LINEAR:
            c = &gc->state.texture.unit[unit].r.objectPlaneEquation;
            tc->z = vx->obj.x * c->x + vx->obj.y * c->y + vx->obj.z * c->z + vx->obj.w * c->w;
            break;
        }
    }
    if (enables & __GL_TEXTURE_GEN_Q_ENABLE) {
        switch (gc->state.texture.unit[unit].q.mode) {
          case GL_EYE_LINEAR:
            c = &gc->state.texture.unit[unit].q.eyePlaneEquation;
            tc->w = c->x * vx->eye.x + c->y * vx->eye.y + c->z * vx->eye.z + c->w * vx->eye.w;
            break;
          case GL_OBJECT_LINEAR:
            c = &gc->state.texture.unit[unit].q.objectPlaneEquation;
            tc->w = vx->obj.x * c->x + vx->obj.y * c->y + vx->obj.z * c->z + vx->obj.w * c->w;
            break;
        }
    }

    /* Finally, apply texture matrix */
    m = (__GLmatrixICD *)gc->transform.texture[unit];
    (*(XF4PROC)m->xf4)(&gen, &vx->texture[unit], m);
    *tc = gen;
}

/* 69067b90 (size 138) -- __glsstim_GetTexLevelParameteriv: glGetTexLevelParameteriv + GL_TEXTURE_COMPRESSED_ARB */
void __stdcall __glsstim_GetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
    __GLtextureFxICD *tex;
    __GL_SETUP();

    ((void (__stdcall *)(GLenum, GLint, GLenum, GLint *))__glim_GetTexLevelParameteriv)(target, level, pname, params);
    tex = (__GLtextureFxICD *)__glLookUpTexture(gc, target, gc->fx.activeTexture);
    if (tex && level >= 0 && level < gc->constants.maxMipMapLevel && pname == 0x86a1) {
        GLint v;

        if (tex->t.level[level]->compressedImage && !tex->wideAspect) {
            v = 1;
        } else {
            v = 0;
        }
        *params = v;
    }
}

/* 69067c20 (size 136) -- __glsstim_GetTexLevelParameterfv: glGetTexLevelParameterfv + GL_TEXTURE_COMPRESSED_ARB */
void __stdcall __glsstim_GetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
    __GLtextureFxICD *tex;
    GLint v;
    __GL_SETUP();

    ((void (__stdcall *)(GLenum, GLint, GLenum, GLfloat *))__glim_GetTexLevelParameterfv)(target, level, pname, params);
    tex = (__GLtextureFxICD *)__glLookUpTexture(gc, target, gc->fx.activeTexture);
    if (tex && level >= 0 && level < gc->constants.maxMipMapLevel && pname == 0x86a1) {
        if (tex->t.level[level]->compressedImage && !tex->wideAspect) {
            v = 1;
        } else {
            v = 0;
        }
        *params = (GLfloat)v;
    }
}

#pragma optimize("w", on)
/* 6905d240 -- __glsstim_TexParameteriv: s_tex.c __glim_TexParameteriv of the hardware path (6909d3f0 plus, as
** 6905cf50); compiled assuming no aliasing in the original */
void __stdcall __glsstim_TexParameteriv(GLenum target, GLenum pname, const GLint pv[])
{
    GLenum e;
    GLint i;
    __GLtexObjParamICD *params;
    __GLtextureICD *tex;
    __GLtexNamePriICD *texName;
    __GLfloat priority;
    __GL_SETUP_NOT_IN_BEGIN();

    params = __glLookUpActiveTextureParams(gc, target);
    if (!params) goto bad_enum;
    tex = __glLookUpActiveTexture(gc, target);
    if (tex->refCount > 2) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }

    switch (pname) {
      case GL_TEXTURE_WRAP_S:
        switch (e = (GLenum) pv[0]) {
          case GL_REPEAT:
          case GL_CLAMP:
          case GL_CLAMP_TO_EDGE:
            params->sWrapMode = e;
            tex->params.sWrapMode = e;
            break;
          default:
            goto bad_enum;
        }
        break;
      case GL_TEXTURE_WRAP_T:
        switch (e = (GLenum) pv[0]) {
          case GL_REPEAT:
          case GL_CLAMP:
          case GL_CLAMP_TO_EDGE:
            params->tWrapMode = e;
            tex->params.tWrapMode = e;
            break;
          default:
            goto bad_enum;
        }
        break;
      case GL_TEXTURE_MIN_FILTER:
        switch (e = (GLenum) pv[0]) {
          case GL_NEAREST:
          case GL_LINEAR:
          case GL_NEAREST_MIPMAP_NEAREST:
          case GL_LINEAR_MIPMAP_NEAREST:
          case GL_NEAREST_MIPMAP_LINEAR:
          case GL_LINEAR_MIPMAP_LINEAR:
            params->minFilter = e;
            tex->params.minFilter = e;
            for (i = 0; i < gc->fx.numTMUs; i++) {
                tex->hwHandle[__GL_TMU_SLOT(i)] = 0;
            }
            break;
          default:
            goto bad_enum;
        }
        break;
      case GL_TEXTURE_MAG_FILTER:
        switch (e = (GLenum) pv[0]) {
          case GL_NEAREST:
          case GL_LINEAR:
            params->magFilter = e;
            tex->params.magFilter = e;
            break;
          default:
            goto bad_enum;
        }
        break;
      case GL_TEXTURE_BORDER_COLOR:
        __glClampColori(gc, &params->borderColor, pv);
        tex->params.borderColor = params->borderColor;
        break;
      case GL_TEXTURE_PRIORITY:
        texName = __glLookUpTextureName(gc, target);
        priority = __GL_I_TO_FLOAT(pv[0]);
        texName->priority = Clampf(priority, __GL_ZERO_F, __GL_ONE_F);
        break;
      default:
      bad_enum:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->hwcx.unitTexture[gc->fx.activeTexture] = (void *)-1;
    __GL_DELAY_VALIDATE(gc);
    wglGlideTexKey(gc, (BYTE *)tex);
}
#pragma optimize("", on)

/*
** 6904d740 -- hwTexelFormat: the texel format of an internal format: the descriptor, and the base format through
** *baseFormat.  RGB8.. / RGBA8.. get the 32-bit formats when the board has them and 16-bit textures are not forced
** (TEX32); the unsized RGB / RGBA also need 32-bit rendering or forced 32-bit textures (TEX32_UNSIZED); else
** 565 / 4444; 0 for an unknown format.  No labels: the sized groups fall through into the small
** formats, and VC6's cross-jumping merges the duplicated 565 / 4444 / RGBA8 blocks into the original's
** layout (the RGB8 group must come before GL_RGB).
*/
const __GLtextureFormatICD *__fastcall hwTexelFormat(__GLcontext *gc, GLenum internalFormat, GLenum *baseFormat)
{
    switch (internalFormat) {
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
        if (TEX32(gc)) {
            *baseFormat = GL_RGBA;
            return &g_texFmtARGB8888;
        }
        /* fall through: 565 */
      case GL_RGB4:
      case GL_RGB5:
        *baseFormat = GL_RGB;
        return &g_texFmtRGB565;
      case 3:
      case GL_RGB:
        if (TEX32_UNSIZED(gc)) {
            *baseFormat = GL_RGB;
            return &g_texFmtXRGB8888;
        }
        *baseFormat = GL_RGB;
        return &g_texFmtRGB565;
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
        if (TEX32(gc)) {
            *baseFormat = GL_RGBA;
            return &g_texFmtARGB8888;
        }
        /* fall through: 4444 */
      case GL_RGBA2:
      case GL_RGBA4:
        *baseFormat = GL_RGBA;
        return &g_texFmtARGB4444;
      case 4:
      case GL_RGBA:
        if (TEX32_UNSIZED(gc)) {
            *baseFormat = GL_RGBA;
            return &g_texFmtARGB8888;
        }
        *baseFormat = GL_RGBA;
        return &g_texFmtARGB4444;
      case GL_R3_G3_B2:
        *baseFormat = GL_RGB;
        return &g_texFmtRGB332;
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
        *baseFormat = GL_ALPHA;
        return &g_texFmtA8;
      case GL_RGB5_A1:
        *baseFormat = GL_RGBA;
        return &g_texFmtARGB1555;
      case GL_COLOR_INDEX1_EXT:
      case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT:
      case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX12_EXT:
      case GL_COLOR_INDEX16_EXT:
        *baseFormat = GL_COLOR_INDEX;
        return &g_texFmtP8;
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
      case 0x84eb:                          /* GL_COMPRESSED_LUMINANCE_ALPHA_ARB */
        *baseFormat = GL_LUMINANCE_ALPHA;
        return &g_texFmtLA88;
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
      case 0x84ea:                          /* GL_COMPRESSED_LUMINANCE_ARB */
        *baseFormat = GL_LUMINANCE;
        return &g_texFmtL8;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
      case 0x84ec:                          /* GL_COMPRESSED_INTENSITY_ARB */
        *baseFormat = GL_INTENSITY;
        return &g_texFmtI8;
      case 0x86b1:                          /* GL_COMPRESSED_RGBA_FXT1_3DFX */
      case 0x83a2:                          /* GL_RGBA_S3TC, GL_RGBA4_S3TC: stored as FXT1 RGBA */
      case 0x83a3:
        *baseFormat = GL_RGBA;
        return &g_texFmtFXT1RGBA;
      case 0x83a0:                          /* GL_RGB_S3TC, GL_RGB4_S3TC: stored as FXT1 RGB */
      case 0x83a1:
      case 0x86b0:                          /* GL_COMPRESSED_RGB_FXT1_3DFX */
        *baseFormat = GL_RGB;
        return &g_texFmtFXT1RGB;
    }
    return 0;
}

#pragma optimize("w", on)
/* 6905e960 (size 272) -- __glsstim_CompressedTexImage2DARB: glCompressedTexImage2DARB -- compiled assuming no
** aliasing in the original */
void __stdcall __glsstim_CompressedTexImage2DARB(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data)
{
    __GLtextureFxICD *tex;
    GLint i;

    __GL_SETUP_NOT_IN_BEGIN();

    tex = (__GLtextureFxICD *)__glCheckCompressedTexImage2DArgs(gc, target, level, internalFormat, width, height, border);
    if (!tex) return;
    if (tex->t.refCount > 2) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }
    (*(SSTCREATELEVEL)tex->t.createLevel)(gc, tex, level, internalFormat, width, height, 2 * border + 1,
                                          border, 2);
    memcpy((void *)tex->t.level[level]->compressedImage, data, imageSize);
    for (i = 0; i < gc->fx.numTMUs; i++) {
        (*tex->t.unmakeResident)(gc, tex, i);
    }
    __GL_DELAY_VALIDATE(gc);
}
#pragma optimize("", on)

#pragma optimize("w", on)
/* 6905ea70 (size 233) -- __glsstim_CompressedTexSubImage2DARB: glCompressedTexSubImage2DARB (the whole level is
** replaced) -- no aliasing */
void __stdcall __glsstim_CompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data)
{
    __GLtextureFxICD *tex;
    GLint i;

    __GL_SETUP_NOT_IN_BEGIN();

    tex = (__GLtextureFxICD *)__glCheckCompressedTexSubImage2DArgs(gc, target, level, xoffset, yoffset, width, height, format);
    if (!tex) return;
    if (tex->t.refCount > 2) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }
    memcpy((void *)tex->t.level[level]->compressedImage, data, imageSize);
    for (i = 0; i < gc->fx.numTMUs; i++) {
        (*tex->t.unmakeResident)(gc, tex, i);
    }
    __GL_DELAY_VALIDATE(gc);
}
#pragma optimize("", on)

#pragma optimize("w", on)
/* 6905eb60 -- __glsstim_GetCompressedTexImageARB: copies the level's compressed image (compressedSize bytes);
** proxy targets GL_INVALID_ENUM, a level outside [0, maxMipMapLevel) GL_INVALID_VALUE.  Compiled assuming
** no aliasing in the original */
void __stdcall __glsstim_GetCompressedTexImageARB(GLenum target, GLint lod, GLvoid *img)
{
    __GLtextureICD *tex;
    __GLmipMapLevelICD *lp;
    __GL_SETUP_NOT_IN_BEGIN();

    tex = __glLookUpTexture(gc, target, gc->fx.activeTexture);
    if (tex != NULL && target != GL_PROXY_TEXTURE_1D && target != GL_PROXY_TEXTURE_2D) {
        if (lod >= 0 && lod < gc->constants.maxMipMapLevel) {
            lp = tex->level[lod];
            memcpy(img, lp->compressedImage, lp->compressedSize);
            return;
        }
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    __glSetError(GL_INVALID_ENUM);
}
#pragma optimize("", on)

/* ---- the hardware texture formats hwTexelFormat (6904d740) picks from (icdglob.h): the SI record + the Glide
** format, each 8-byte aligned in the original (69003bc8 .. 69003fd8, its .rdata) */
#ifndef __GL_RED_ALPHA
#define __GL_RED_ALPHA 1
#endif
extern void __fastcall __glExtractTexelA8(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelA8_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelARGB1555(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelARGB1555_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelARGB4(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelARGB4_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelCI8(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelCI8_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelI8(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelI8_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelL8(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelL8_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelLA8(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelLA8_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelRGB332(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelRGB332_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelRGB565(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelRGB565_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glSSTExtractTexelRGBA8_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
/* 69003bc8 -- GR_TEXFMT_INTENSITY_8 */
const __GLtextureFormatICD g_texFmtL8 = {
    0, 0, 0, 0, 0, 0, 8, 0,  8,  GL_RED, 0x1401, 1,
    (__GLproc)__glExtractTexelL8, (__GLproc)__glExtractTexelL8_B, NULL, NULL, { 0, 0 }, 3
};
/* 69003c18 -- GR_TEXFMT_ALPHA_INTENSITY_88 */
const __GLtextureFormatICD g_texFmtLA88 = {
    1, 0, 0, 0, 0, 8, 8, 0,  16,  __GL_RED_ALPHA, 0x1401, 1,
    (__GLproc)__glExtractTexelLA8, (__GLproc)__glExtractTexelLA8_B, NULL, NULL, { 0, 0 }, 13
};
/* 69003c68 -- GR_TEXFMT_RGB_332 */
const __GLtextureFormatICD g_texFmtRGB332 = {
    3, 0, 3, 3, 2, 0, 0, 0,  8,  GL_RGB, 0x8032, 1,
    (__GLproc)__glExtractTexelRGB332, (__GLproc)__glExtractTexelRGB332_B, NULL, NULL, { 0, 0 }, 0
};
/* 69003cb8 -- GR_TEXFMT_RGB_565 */
const __GLtextureFormatICD g_texFmtRGB565 = {
    5, 0, 5, 6, 5, 0, 0, 0,  16,  GL_RGB, 0, 2,
    (__GLproc)__glExtractTexelRGB565, (__GLproc)__glExtractTexelRGB565_B, NULL, NULL, { 0, 0 }, 10
};
/* 69003d08 -- GR_TEXFMT_ARGB_4444 */
const __GLtextureFormatICD g_texFmtARGB4444 = {
    8, 0, 4, 4, 4, 4, 0, 0,  16,  GL_RGBA, 3, 2,
    (__GLproc)__glExtractTexelARGB4, (__GLproc)__glExtractTexelARGB4_B, NULL, NULL, { 0, 0 }, 12
};
/* 69003d58 -- GR_TEXFMT_ARGB_1555 */
const __GLtextureFormatICD g_texFmtARGB1555 = {
    10, 0, 5, 5, 5, 1, 0, 0,  16,  GL_RGBA, 2, 2,
    (__GLproc)__glExtractTexelARGB1555, (__GLproc)__glExtractTexelARGB1555_B, NULL, NULL, { 0, 0 }, 11
};
/* 69003da8 -- GR_TEXFMT_ARGB_8888, GL_RGBA pixels */
const __GLtextureFormatICD g_texFmtARGB8888 = {
    15, 0, 8, 8, 8, 8, 0, 0,  32,  GL_RGBA, 4, 4,
    (__GLproc)__glSSTExtractTexelRGBA8, (__GLproc)__glSSTExtractTexelRGBA8_B, NULL, NULL, { 0, 0 }, 18
};
/* 69003df8 -- GR_TEXFMT_ARGB_8888, GL_RGB pixels */
const __GLtextureFormatICD g_texFmtXRGB8888 = {
    15, 0, 8, 8, 8, 8, 0, 0,  32,  GL_RGBA, 5, 4,
    (__GLproc)__glSSTExtractTexelRGBA8, (__GLproc)__glSSTExtractTexelRGBA8_B, NULL, NULL, { 0, 0 }, 18
};
/* 69003e48 -- GR_TEXFMT_ALPHA_8 */
const __GLtextureFormatICD g_texFmtA8 = {
    11, 0, 0, 0, 0, 8, 0, 0,  8,  GL_ALPHA, 0x1401, 1,
    (__GLproc)__glExtractTexelA8, (__GLproc)__glExtractTexelA8_B, NULL, NULL, { 0, 0 }, 2
};
/* 69003e98 -- GR_TEXFMT_ALPHA_8 (intensity) */
const __GLtextureFormatICD g_texFmtI8 = {
    12, 0, 0, 0, 0, 0, 0, 8,  8,  GL_RED, 0x1401, 1,
    (__GLproc)__glExtractTexelI8, (__GLproc)__glExtractTexelI8_B, NULL, NULL, { 0, 0 }, 2
};
/* 69003ee8 -- GR_TEXFMT_P_8 */
const __GLtextureFormatICD g_texFmtP8 = {
    13, 0, 0, 0, 0, 0, 0, 0,  8,  GL_COLOR_INDEX, 0x1401, 1,
    (__GLproc)__glExtractTexelCI8, (__GLproc)__glExtractTexelCI8_B, NULL, NULL, { 0, 0 }, 5
};
/* 69003f38 -- GR_TEXFMT_ARGB_CMP_FXT1 */
const __GLtextureFormatICD g_texFmtFXT1RGBA = {
    16, 0, 8, 8, 8, 8, 0, 0,  32,  GL_RGBA, 4, 4,
    (__GLproc)__glSSTExtractTexelRGBA8, (__GLproc)__glSSTExtractTexelRGBA8_B, NULL, NULL, { 0, 0 }, 17
};
/* 69003f88 -- GR_TEXFMT_ARGB_CMP_FXT1 */
const __GLtextureFormatICD g_texFmtFXT1RGB = {
    16, 0, 8, 8, 8, 8, 0, 0,  32,  GL_RGBA, 5, 4,
    (__GLproc)__glSSTExtractTexelRGBA8, (__GLproc)__glSSTExtractTexelRGBA8_B, NULL, NULL, { 0, 0 }, 17
};

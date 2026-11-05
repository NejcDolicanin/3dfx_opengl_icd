/*
** gl_tex.c -- s_tex.c / s_texture.c / s_texmgr.c: texture objects, images, levels and the texture API entries.
** SI-ported parts: si_s_tex.c, si_s_texel.c.
*/
#include <string.h>
#include "wglpriv.h"
#include "glsi.h"
#include "siprotos.h"
#include "glsitypes.h"
#include "si_names.h"
#include "icdprotos.h"
#include <math.h>
#include "glshade.h"
#include "icdglob.h"
#include <stdlib.h>
#include "og.h"

/* 690f5258 -- the software texture manager template (690c3640 copies it) */
extern GLboolean __fastcall __glInvalidateAllTextures(void);    /* gl_texcoord.c */
__GLtexMgrICD g_swTexMgrTemplate = { 0, (__GLproc)__glFreeTextureManager, (__GLproc)__glInvalidateAllTextures };

/* ---- s_tex.c ------------------------------------------------------------------------------ */
typedef void *(__fastcall *__GLcallocProc)(__GLcontext *gc, size_t n, size_t size);
typedef GLint *(__fastcall *__GLcreateTextureProc)(__GLcontext *gc, GLuint name, GLint targetIndex);
#define __GL_IMPORTS_CALLOC(gc) (*(__GLcallocProc *)&(gc)->imports[1])
#define GL_CONSTANT_EXT                 0x8576
#define GL_PREVIOUS_EXT                 0x8578
#define TEX ((__GLtextureICD *)*bt)
#define UNIT (&gc->state.texture.unit[i])
/* 690a0130: __glBindTexture of the active unit (texture 0: the default object) */
typedef void (__fastcall *__GLtexFreeProc)(__GLcontext *gc, __GLtextureICD *tex);
/************************************************************************/
/* the immediate-mode Vertex*fv entries inside Begin/End: an inline vertex routine (its parameters are
** evaluated before any store, which is what makes the original load all coordinates first) stores the
** object coordinates into gc->vertex.v0, transforms them by the modelview's mvp, clip-checks and
** hands the vertex to procs.v */
typedef void (__fastcall *__GLxformProc)(__GLcoord *res, const __GLfloat *v, const __GLmatrixICD *m);
/* glTexGen{f,d,i}v: the SI with the active unit's state (gc->fx.activeTexture) and the modelview's
** updateInverse flag at +0x124 */
#define TR_UPDATE_INVERSE(tr)   ((tr)->updateInverse)
#define TEXGEN_BODY()                                                                \
    __GLtextureGenState *tcs;                                                           \
    __GLfloat v[4];                                                                     \
    __GLtransformICD *tr;                                                               \
    __GL_SETUP_NOT_IN_BEGIN();                                                          \
                                                                                        \
    switch (coord) {                                                                    \
      case GL_S: tcs = &gc->state.texture.unit[gc->fx.activeTexture].s; break;          \
      case GL_T: tcs = &gc->state.texture.unit[gc->fx.activeTexture].t; break;          \
      case GL_R: tcs = &gc->state.texture.unit[gc->fx.activeTexture].r; break;          \
      case GL_Q: tcs = &gc->state.texture.unit[gc->fx.activeTexture].q; break;          \
      default:                                                                          \
        __glSetError(GL_INVALID_ENUM);                                                  \
        return;                                                                         \
    }                                                                                   \
    switch (pname) {                                                                    \
      case GL_TEXTURE_GEN_MODE:                                                         \
        switch ((GLenum) pv[0]) {                                                       \
          case GL_EYE_LINEAR:                                                           \
          case GL_OBJECT_LINEAR:                                                        \
            tcs->mode = (GLenum) pv[0];                                                 \
            break;                                                                      \
          case GL_SPHERE_MAP:                                                           \
            if ((coord == GL_R) || (coord == GL_Q)) {                                   \
                __glSetError(GL_INVALID_ENUM);                                          \
                return;                                                                 \
            }                                                                           \
            tcs->mode = (GLenum) pv[0];                                                 \
            break;                                                                      \
          default:                                                                      \
            __glSetError(GL_INVALID_ENUM);                                              \
            return;                                                                     \
        }                                                                               \
        break;                                                                          \
      case GL_OBJECT_PLANE:                                                             \
        tcs->objectPlaneEquation.x = pv[0];                                             \
        tcs->objectPlaneEquation.y = pv[1];                                             \
        tcs->objectPlaneEquation.z = pv[2];                                             \
        tcs->objectPlaneEquation.w = pv[3];                                             \
        break;                                                                          \
      case GL_EYE_PLANE:                                                                \
        v[0] = pv[0]; v[1] = pv[1]; v[2] = pv[2]; v[3] = pv[3];                         \
        tr = gc->transform.modelView;                                                   \
        if (TR_UPDATE_INVERSE(tr)) {                                                \
            (*gc->procs.computeInverseTranspose)(gc, tr);                               \
        }                                                                               \
        (*(__GLxformProc)tr->inverseTranspose.xf4)(&tcs->eyePlaneEquation, v,     \
                                    &tr->inverseTranspose);                             \
        break;                                                                          \
      default:                                                                          \
        __glSetError(GL_INVALID_ENUM);                                                  \
        return;                                                                         \
    }                                                                                   \
    __GL_DELAY_VALIDATE(gc);
/************************************************************************/
/* s_tex.c */
#define __glLookupActiveTexture(gc, target) \
    __glLookUpTexture(gc, target, (gc)->fx.activeTexture)
#define Clampf(v, lo, hi) \
    (((__GLfloat (__fastcall *)(__GLfloat, __GLfloat, __GLfloat))tex_Clamp)(v, lo, hi))
/* the state's per-target parameter copy (6909c610) has the object parameters' layout;
** 6909c660 returns the bound object's {name, priority} */
#define __glLookUpActiveTextureParams(gc, target) \
    __glLookUpTextureParams(gc, target, (gc)->fx.activeTexture)
#define __glLookUpTextureName(gc, target) \
    __glLookUpTextureTexobjs(gc, target, (gc)->fx.activeTexture)
/* glTexEnv single-value entries: the ICD accepts the EXT_texture_env_combine names as well */
#define GL_COMBINE_RGB_EXT          0x8571
#define GL_COMBINE_ALPHA_EXT        0x8572
#define GL_RGB_SCALE_EXT            0x8573
#define GL_SOURCE0_RGB_EXT          0x8580
#define GL_SOURCE1_RGB_EXT          0x8581
#define GL_SOURCE2_RGB_EXT          0x8582
#define GL_SOURCE0_ALPHA_EXT        0x8588
#define GL_SOURCE1_ALPHA_EXT        0x8589
#define GL_SOURCE2_ALPHA_EXT        0x858A
#define GL_OPERAND0_RGB_EXT         0x8590
#define GL_OPERAND1_RGB_EXT         0x8591
#define GL_OPERAND2_RGB_EXT         0x8592
#define GL_OPERAND0_ALPHA_EXT       0x8598
#define GL_OPERAND1_ALPHA_EXT       0x8599
#define GL_OPERAND2_ALPHA_EXT       0x859A
extern void __stdcall __glim_TexEnvfv(GLenum target, GLenum pname, const GLfloat pv[]);
extern void __stdcall __glim_TexEnviv(GLenum target, GLenum pname, const GLint pv[]);
#define GL_UNSIGNED_BYTE_3_3_2          0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4       0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1       0x8034
#define GL_UNSIGNED_INT_8_8_8_8         0x8035
#define GL_UNSIGNED_INT_10_10_10_2      0x8036
#define GL_RGB_S3TC               0x83a0      /* GL_S3_s3tc: all four are stored as FXT1 (RGB / RGBA) */
#define GL_RGB4_S3TC              0x83a1
#define GL_RGBA_S3TC              0x83a2
#define GL_RGBA4_S3TC             0x83a3
#define GL_COMPRESSED_ALPHA_ARB         0x84e9
#define GL_COMPRESSED_LUMINANCE_ARB     0x84ea
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB 0x84eb
#define GL_COMPRESSED_INTENSITY_ARB     0x84ec
#define GL_COMPRESSED_RGB_FXT1_3DFX     0x86b0
#define GL_COMPRESSED_RGBA_FXT1_3DFX    0x86b1
/* s_tex.c sub-image helpers */
typedef __GLpixelSpanInfoICD __GLpixelSpanInfo;
typedef void (__fastcall *__GLcopyTexImageProc)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
                                                __GLtextureICD *tex, GLint lod);
typedef void (__fastcall *__GLunmakeResidentProc)(__GLcontext *gc, __GLtextureICD *tex, GLint unit);
/************************************************************************/
/* s_tex.c CopyTexImage1D / 2D / CopyTexSubImage1D (__stdcall entries).
** The ICD tests the texture's own reference count (no __glLookUpTextureObject) and creates the level
** before reading. */
typedef __GLmipMapLevelICD *(__fastcall *__GLcreateLevelProc)(__GLcontext *gc, __GLtextureICD *tex,
    GLint lod, GLenum components, GLsizei w, GLsizei h, GLsizei d, GLint border, GLint dim);
/* TexImage1D / 2D (__stdcall entries).  The ICD tests the texture's own reference count and
** unmakes it resident on the active unit (no __glLookUpTextureObject). */
/* ---- texture coordinate generation, both units (6909f1c0 / 6909f250 / 6909f330) ----
** tc = (s plane . eye or obj, t plane . eye or obj, z, w), then the texture matrix's xf4. */
typedef void (__fastcall *__GLxf4Proc)(__GLcoord *res, const __GLcoord *v, void *m);
#define XF4(m)   (*(__GLxf4Proc *)&(m)->matrix.xf4)
/* ---- shared-object release: drop a reference, free on the last one ---- */
typedef void (__fastcall *PFNRELEASE)(__GLcontext *gc, GLint *obj);
#define BOUND_TEXTURE(gc, tex) \
    ((__GLtextureICD *)(gc)->texture.boundTexture[(gc)->fx.activeTexture][(tex)->targetIndex])
#define __glIsActiveTextureConsistent(gc, tex) __glIsTextureConsistent(gc, tex, (gc)->fx.activeTexture)
#define LP_PIXELBUFFER(lp)  ((lp)->pixelBuffer)
#define LP_INTERNALFORMAT(lp) ((lp)->internalFormat)
/* ---- texture level -> frame buffer pixels ------------------------------------------------- */

/* the ICD's internal texel formats (lp->internalFormat) this copy understands */
#define __GL_TEXFMT_L8          0
#define __GL_TEXFMT_LA88        1
#define __GL_TEXFMT_RGB888      2
#define __GL_TEXFMT_RGBA8888    6
#define __GL_TEXFMT_I8          0xc
typedef void *(__fastcall *__GLreallocProc)(__GLcontext *gc, void *p, size_t size);
/************************************************************************/
/* s_texextr.c / s_texel.c */
#define LP_WIDTHLOG2(lp)   ((lp)->widthLog2)
typedef void (__fastcall *__GLextractProc)(__GLmipMapLevelICD *lp, __GLtextureICD *tex, GLint img,
                                              GLint row, GLint col, __GLtexel *result);
/* ---- s_texel.c ----------------------------------------------------------------------------- */
#define __GL_FRAC(f)    ((f) - __GL_FLOORF(f))
#define __glFloor(f)    __GL_FLOORF(f)
#ifndef GL_INTENSITY
#define GL_INTENSITY    0x8049
#endif
typedef void (__fastcall *TEXfn_extract)(__GLmipMapLevelICD *lp, __GLtextureICD *tex,
                                         GLint img, GLint row, GLint col, __GLtexel *result);
typedef void (__fastcall *__GLtexFilterProc)(__GLtextureICD *tex, __GLmipMapLevelICD *lp,
                                                __GLfloat s, __GLfloat t, __GLtexel *result);
/* ---- s_texture.c mipmap filters: the level from lod, then the per-level filter proc ------ */
typedef void (__fastcall *__GLtexLevelFilter)(__GLtextureICD *tex, __GLmipMapLevelICD *lp,
                                                 __GLfloat s, __GLfloat t, __GLtexel *result);
#define MIPMAP_NEAREST(name, slot)                                                      \
void __fastcall name(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod, __GLfloat s, __GLfloat t) \
{                                                                                           \
    __GLmipMapLevelICD *lp;                                                                 \
    GLint p, d;                                                                             \
                                                                                            \
    if (!(tex->gc->constants.half < lod)) {                                                 \
	d = 0;                                                                              \
    } else {                                                                                \
	p = tex->maxLevel;                                                                  \
	d = (GLint) (lod + ((__GLfloat) 0.49995)); /* NOTE: .5 minus epsilon */             \
	if (d > p) {                                                                        \
	    d = p;                                                                          \
	}                                                                                   \
    }                                                                                       \
    lp = tex->level[d];                                                                     \
    (*(__GLtexLevelFilter)tex->slot)(tex, lp, s * lp->width2f, t * lp->height2f, result); \
}
#define MODEFLAGS(gc)   ((*(GLuint (__fastcall *)(__GLcontext *))(gc)->polygon.shader.getModeFlags)(gc))
#define __GL_SHADE_UVSCALED_TEXTURE 0x1000000
/* the shade record's texture iterator as __glComputePolygonRho reads it (sh = &gc->polygon.shader
** from dxLeftLittle, 0x3bec): d(s, t, qw)/dx at +0xec, d(s, t, qw)/dy at +0xfc */
typedef struct __GLshadeRhoRec {
    GLubyte     pad00[0xec];
    __GLfloat   dsdx, dtdx, dqwdx, pad_f8;  /* +0xec */
    __GLfloat   dsdy, dtdy, dqwdy;          /* +0xfc */
} __GLshadeRho;
/************************************************************************/
/* s_texel.c: the UV-scaled filters (level 0's size scales s and t) and the fragment procs.
** Texture procs (glctx.h __GLtextureICD): +0x58 the switch-over lambda c, +0xa8 env (gc, color, texel),
** +0xac magnify / +0xb0 minify, +0xb4 linear / +0xb8 nearest (tex, level, s, t, texel).  690a5030
** declares magnify as (tex, texel, lambda, s, t): the same registers and stack as the ancestor's
** (tex, lod, s, t, texel), kept for its argument evaluation order. */
typedef void (__fastcall *__GLtexEnvProc)(__GLcontext *gc, __GLcolor *color, __GLtexel *texel);
typedef void (__fastcall *__GLtexMagProc)(__GLtextureICD *tex, __GLtexel *texel, __GLfloat lambda, __GLfloat s, __GLfloat t);
/* __glMakeResidentTexture */
/* ---- s_texmgr.c ---------------------------------------------------------------------------- */

/* the device's texture formats (g_device->chooseTexFormat): glctx.h __GLtextureFormatICD */
typedef __GLtextureFormatICD __GLtextureFormat;
/* the mip level's SI fields (glctx.h __GLmipMapLevelICD) */
#define LP_DEPTH(lp)        ((lp)->depth)
#define LP_IMAGESIZE(lp)    ((lp)->imageSize)
#define LP_DEPTH2(lp)       ((lp)->depth2)
#define LP_DEPTH2F(lp)      ((lp)->depth2f)
#define LP_HEIGHTLOG2(lp)   ((lp)->heightLog2)
#define LP_DEPTHLOG2(lp)    ((lp)->depthLog2)
#define LP_TEXFORMAT(lp)    ((lp)->texFormat)
#define TEX_NUMLEVELS(t)    ((t)->numLevels)
#define TEX_SSCALE(t)       ((t)->sScale)
#define TEX_TSCALE(t)       ((t)->tScale)
#define TEX_TEXFORMAT(t)    ((t)->texFormat)
#define __glFloorLog2(n)    ((GLint)wglLog2(n))
#define BOUNDTEX(gc, targetIndex) \
    ((__GLtextureICD **)(gc)->texture.boundTexture)[(gc)->fx.activeTexture * 4 + (targetIndex)]
#define PARAM(gc, targetIndex) \
    (&(gc)->state.texture.unit[(gc)->fx.activeTexture].param[targetIndex])
typedef GLint *(__fastcall *__GLcreateTextureProc2)(__GLcontext *gc, GLuint name, GLint targetIndex);
extern void __fastcall __glConvertTexLevelPixels(__GLcontext *gc, __GLmipMapLevelICD *lp, GLint x, GLint y, GLint w, GLint h);
extern __GLtextureICD * __fastcall CheckCopyTexImageArgs(__GLcontext *gc, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border, GLint dim);
extern __GLtextureICD *__fastcall __glCreateTexture(__GLcontext *gc, GLuint name, GLint targetIndex);
extern void *__fastcall __glTextureCreateLevel(__GLcontext *gc, __GLtextureICD *tex, GLint lod, GLint components, GLsizei w, GLsizei h, GLsizei d, GLint border, GLint dim);

/************************************************************************/
/* s_tex.c */

/* 6909c6b0 -- __glLookUpTexture (s_tex.c): the texture object bound to (target, unit) */
__GLtextureICD *__fastcall __glLookUpTexture(__GLcontext *gc, GLenum target, GLint unit)
{
    switch (target) {
      case GL_TEXTURE_1D:
        return gc->texture.boundTexture[unit][0];
      case GL_TEXTURE_2D:
        return gc->texture.boundTexture[unit][1];
      case GL_PROXY_TEXTURE_1D:
        return gc->texture.boundTexture[unit][2];
      case GL_PROXY_TEXTURE_2D:
        return gc->texture.boundTexture[unit][3];
      default:
        return NULL;
    }
}

/* 6909c720 -- __glTexTargetIndex: texture target -> target index (-1: not a texture target) */
GLuint __fastcall __glTexTargetIndex(__GLcontext *gc, GLenum target, GLint unit)
{
    switch (target) {
      case GL_TEXTURE_1D:
        return 0;
      case GL_TEXTURE_2D:
        return 1;
      case GL_PROXY_TEXTURE_1D:
        return 2;
      case GL_PROXY_TEXTURE_2D:
        return 3;
      default:
        return (GLuint)-1;
    }
}

/* 6909c7d0 -- s_tex.c __glEarlyInitTextureState: the device's texture manager init, the env
** blocks per unit, and the default objects bound to both units */
void __fastcall __glEarlyInitTextureState(__GLcontext *gc)
{
    GLint i, j, n;
    GLint *tex;
    void **dp;
    __GLtextureUnitStateICD *tus;

    (*(void (__fastcall *)(__GLcontext *))g_device->initTextureManager)(gc);

    n = gc->constants.numberOfTextureEnvs;
    gc->constants.numberOfTextures = 4;
    gc->constants.maxTextureSize = 1 << (gc->constants.maxMipMapLevel - 1);
    tus = &gc->state.texture.unit[0];
    for (i = 2; i != 0; i--) {
	tus->env = (__GLtextureEnvState *) __GL_IMPORTS_CALLOC(gc)(gc, n, 0x54);
	tus++;
    }

    if (NULL == gc->texture.namesArray) {
	gc->texture.namesArray = __glNamesNewArray(gc, __GL_NAMES_TEXOBJ);
    }

    dp = (void **)&gc->texture.defaultTexture[0];
    for (i = 0; i < 4; i++, dp++) {
	tex = *dp = (*(__GLcreateTextureProc)gc->createTexture)(gc, 0, i);
	tex[0] = 1;
	for (j = 0; j < 2; j++) {
	    gc->texture.boundTexture[j][i] = tex;
	}
    }
}

/* 6909c880 -- s_tex.c __glInitTextureState (ICD: per TMU; env combine defaults; the default
** objects get the gc, their parameters and dimension, and every level requestedFormat 1) */
void __fastcall __glInitTextureState(__GLcontext *gc)
{
    GLint i, j, k, numTextures, numEnvs, maxLevels;
    __GLtextureEnvState *tes;
    __GLtexParamICD *param;
    void **bt;

    numTextures = gc->constants.numberOfTextures;
    numEnvs = gc->constants.numberOfTextureEnvs;
    maxLevels = gc->constants.maxMipMapLevel;
    gc->state.current.activeTexture = &gc->state.current.texture[0];
    gc->state.current.texture[0].w = 1.0f;
    gc->state.current.texture[1].w = 1.0f;

    for (i = 0; i < gc->fx.numTMUs; i++) {

	tes = UNIT->env;
	for (j = 0; j < numEnvs; j++, tes++) {
	    tes->mode = GL_MODULATE;
	    tes->combineRGB = GL_MODULATE;
	    tes->combineAlpha = GL_MODULATE;
	    tes->sourceRGB[0] = GL_TEXTURE;
	    tes->sourceRGB[1] = GL_PREVIOUS_EXT;
	    tes->sourceRGB[2] = GL_CONSTANT_EXT;
	    tes->sourceAlpha[0] = GL_TEXTURE;
	    tes->sourceAlpha[1] = GL_PREVIOUS_EXT;
	    tes->sourceAlpha[2] = GL_CONSTANT_EXT;
	    tes->operandRGB[0] = GL_SRC_COLOR;
	    tes->operandRGB[1] = GL_SRC_COLOR;
	    tes->operandRGB[2] = GL_SRC_COLOR;
	    tes->operandAlpha[0] = GL_SRC_ALPHA;
	    tes->operandAlpha[1] = GL_SRC_ALPHA;
	    tes->operandAlpha[2] = GL_SRC_ALPHA;
	    tes->rgbScale = 1.0f;
	    tes->alphaScale = 1.0f;
	}

	param = &UNIT->param[0];
	bt = &gc->texture.boundTexture[i][0];
	for (j = 0; j < numTextures; j++, param++, bt++) {
	    param->params.minFilter = GL_NEAREST_MIPMAP_LINEAR;
	    param->params.sWrapMode = GL_REPEAT;
	    param->params.tWrapMode = GL_REPEAT;
	    param->params.magFilter = GL_LINEAR;
	    param->name = 0;
	    param->priority = 1.0f;

	    TEX->gc = gc;
	    TEX->params = param->params;
	    switch (j) {
	      case 0: case 2:
		TEX->dim = 1;
		break;
	      case 1: case 3:
		TEX->dim = 2;
		break;
	    }
	    for (k = 0; k < maxLevels; k++) {
		TEX->level[k]->requestedFormat = 1;
	    }
	}

	UNIT->s.mode = GL_EYE_LINEAR;
	UNIT->s.eyePlaneEquation.x = 1.0f;
	UNIT->s.objectPlaneEquation.x = 1.0f;
	UNIT->t.mode = GL_EYE_LINEAR;
	UNIT->t.eyePlaneEquation.y = 1.0f;
	UNIT->t.objectPlaneEquation.y = 1.0f;
	UNIT->r.mode = GL_EYE_LINEAR;
	UNIT->q.mode = GL_EYE_LINEAR;
	UNIT->scaleBias[0][0] = 1.0f;
	UNIT->scaleBias[0][1] = 1.0f;
	UNIT->scaleBias[0][2] = 1.0f;
	UNIT->scaleBias[0][3] = 1.0f;
    }
}

/* 6909ca80 -- s_tex.c __glFreeTextureState */
void __fastcall __glFreeTextureState(__GLcontext *gc)
{
    GLint i, j, numTextures;
    void **bt;

    numTextures = gc->constants.numberOfTextures;

    /* Unbind all non-default texture objects */
    for (i = 0; i < 2; i++) {
	gc->fx.activeTexture = i;
	for (j = 0; j < numTextures; j++) {
	    __glBindTexture(gc, j, 0);
	}
	gc->fx.activeTexture = 0;
    }

    /* Free default textures */
    bt = &gc->texture.boundTexture[0][0];
    for (i = 0; i < numTextures; i++, bt++) {
	((__GLtextureICD *)*bt)->refCount--;
	(*(__GLtexFreeProc)((__GLtextureICD *)*bt)->free)(gc, *bt);
	gc->texture.defaultTexture[i] = NULL;
    }

    /* Get rid of texture object names array, unless it is shared */
    gc->texture.namesArray->refcount--;
    if (gc->texture.namesArray->refcount == 0) {
	__glNamesFreeArray(gc, gc->texture.namesArray);
    }
    gc->texture.namesArray = NULL;

    (*(void (__fastcall *)(__GLcontext *))gc->texMgr->free)(gc);

    for (i = 0; i < 2; i++) {
	(*(void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, gc->state.texture.unit[i].env);
    }
}

/* 6909cb40 -- __glim_TexGenfv: glTexGenfv */
void __stdcall __glim_TexGenfv(GLenum coord, GLenum pname, const GLfloat pv[])
{
    TEXGEN_BODY()
}

/* 6909cd30 -- __glim_TexGendv: glTexGendv */
void __stdcall __glim_TexGendv(GLenum coord, GLenum pname, const GLdouble pv[])
{
    TEXGEN_BODY()
}

/* 6909cf20 -- __glim_TexGeniv: glTexGeniv */
void __stdcall __glim_TexGeniv(GLenum coord, GLenum pname, const GLint pv[])
{
    TEXGEN_BODY()
}

/* 6909d160 -- __glim_TexParameterfv: each parameter goes into the state's per-target copy and the
** bound object */
void __stdcall __glim_TexParameterfv(GLenum target, GLenum pname, const GLfloat pv[])
{
    GLenum e;
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
    __GL_DELAY_VALIDATE(gc);
}

/* 6909d370 -- tex_Clamp: Clamp (s_tex.c), val clamped to [min, max] */
double __fastcall tex_Clamp(GLuint unused1, GLuint unused2, __GLfloat val, __GLfloat min, __GLfloat max)
{
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

/* 6909d3f0 -- s_tex.c __glim_TexParameteriv: each parameter goes into the state's per-target copy
** and the bound object */
void __stdcall __glim_TexParameteriv(GLenum target, GLenum pname, const GLint pv[])
{
    GLenum e;
    __GLtexObjParamICD *params;
    __GLtextureICD *tex;
    __GLtexNamePriICD *texName;
    __GLfloat priority;
    __GL_SETUP_NOT_IN_BEGIN();

    params = __glLookUpActiveTextureParams(gc, target);
    tex = __glLookupActiveTexture(gc, target);
    if (!params) goto bad_enum;
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
    __GL_DELAY_VALIDATE(gc);
}

/* 6909d640 -- __glTexParameterfv_size (no 1.2 entries) */
GLuint __fastcall __glTexParameterfv_size(GLenum e)
{
    switch (e) {
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_PRIORITY:
        return 1;
      case GL_TEXTURE_BORDER_COLOR:
        return 4;
      default:
        return (GLuint)-1;
    }
}

/* 6909dbc0 -- s_tex.c __glim_TexEnvf */
void __stdcall __glim_TexEnvf(GLenum target, GLenum pname, GLfloat f)
{
    /* Accept only enumerants that correspond to single values */
    switch (pname) {
      case GL_TEXTURE_ENV_MODE:
      case GL_ALPHA_SCALE:
      case GL_COMBINE_RGB_EXT:
      case GL_COMBINE_ALPHA_EXT:
      case GL_RGB_SCALE_EXT:
      case GL_SOURCE0_RGB_EXT:
      case GL_SOURCE1_RGB_EXT:
      case GL_SOURCE2_RGB_EXT:
      case GL_SOURCE0_ALPHA_EXT:
      case GL_SOURCE1_ALPHA_EXT:
      case GL_SOURCE2_ALPHA_EXT:
      case GL_OPERAND0_RGB_EXT:
      case GL_OPERAND1_RGB_EXT:
      case GL_OPERAND2_RGB_EXT:
      case GL_OPERAND0_ALPHA_EXT:
      case GL_OPERAND1_ALPHA_EXT:
      case GL_OPERAND2_ALPHA_EXT:
	__glim_TexEnvfv(target, pname, &f);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 6909e160 -- s_tex.c __glim_TexEnvi */
void __stdcall __glim_TexEnvi(GLenum target, GLenum pname, GLint i)
{
    /* Accept only enumerants that correspond to single values */
    switch (pname) {
      case GL_TEXTURE_ENV_MODE:
      case GL_ALPHA_SCALE:
      case GL_COMBINE_RGB_EXT:
      case GL_COMBINE_ALPHA_EXT:
      case GL_RGB_SCALE_EXT:
      case GL_SOURCE0_RGB_EXT:
      case GL_SOURCE1_RGB_EXT:
      case GL_SOURCE2_RGB_EXT:
      case GL_SOURCE0_ALPHA_EXT:
      case GL_SOURCE1_ALPHA_EXT:
      case GL_SOURCE2_ALPHA_EXT:
      case GL_OPERAND0_RGB_EXT:
      case GL_OPERAND1_RGB_EXT:
      case GL_OPERAND2_RGB_EXT:
      case GL_OPERAND0_ALPHA_EXT:
      case GL_OPERAND1_ALPHA_EXT:
      case GL_OPERAND2_ALPHA_EXT:
	__glim_TexEnviv(target, pname, &i);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 6909e5f0 -- CheckTexImageArgs: the 1.2 packed types without the _REV ones, no dim test for
** bitmaps, the colour-index internal formats, and the ARB / 3dfx compressed formats */
__GLtextureICD * __fastcall CheckTexImageArgs(__GLcontext *gc, GLenum target, GLint lod,
                                         GLint components, GLint border,
                                         GLenum format, GLenum type, GLint dim)
{
    __GLtextureICD *tex = __glLookupActiveTexture(gc, target);

    if (!tex || (tex->dim != dim)) {
      bad_enum:
        __glSetError(GL_INVALID_ENUM);
        return 0;
    }

    switch (type) {
      case GL_BITMAP:
        if (format != GL_COLOR_INDEX) goto bad_enum;
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
            return 0;
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
            return 0;
        }
        break;

      default:
        goto bad_enum;
    }

    switch (format) {
      case GL_COLOR_INDEX:      case GL_RED:
      case GL_GREEN:            case GL_BLUE:
      case GL_ALPHA:            case GL_RGB:
      case GL_RGBA:             case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:  case GL_ABGR_EXT:
      case GL_BGRA:         case GL_BGR:
        break;
      default:
        goto bad_enum;
    }

    if ((lod < 0) || (lod >= gc->constants.maxMipMapLevel)) {
      bad_value:
        __glSetError(GL_INVALID_VALUE);
        return 0;
    }

    switch (components) {
      case 1: case 2: case 3: case 4:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:       case GL_LUMINANCE8:
      case GL_LUMINANCE12:      case GL_LUMINANCE16:
      case GL_COMPRESSED_LUMINANCE_ARB:
        break;
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:        case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:        case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:      case GL_LUMINANCE16_ALPHA16:
      case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
        break;
      case GL_RGB:
      case GL_R3_G3_B2:         case GL_RGB4:
      case GL_RGB5:             case GL_RGB8:
      case GL_RGB10:            case GL_RGB12:
      case GL_RGB16:
      case GL_RGB_S3TC:         case GL_RGB4_S3TC:
      case GL_COMPRESSED_RGB_FXT1_3DFX:
        break;
      case GL_RGBA:
      case GL_RGBA2:            case GL_RGBA4:
      case GL_RGBA8:            case GL_RGBA12:
      case GL_RGBA16:           case GL_RGB5_A1:
      case GL_RGB10_A2:
      case GL_RGBA_S3TC:        case GL_RGBA4_S3TC:
      case GL_COMPRESSED_RGBA_FXT1_3DFX:
        break;
      case GL_ALPHA:
      case GL_ALPHA4:           case GL_ALPHA8:
      case GL_ALPHA12:          case GL_ALPHA16:
      case GL_COMPRESSED_ALPHA_ARB:
        break;
      case GL_INTENSITY:
      case GL_INTENSITY4:       case GL_INTENSITY8:
      case GL_INTENSITY12:      case GL_INTENSITY16:
      case GL_COMPRESSED_INTENSITY_ARB:
        break;
      case GL_COLOR_INDEX1_EXT: case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT: case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX12_EXT: case GL_COLOR_INDEX16_EXT:
        if (format != GL_COLOR_INDEX)
            goto bad_value;
        switch (type) {
        case GL_BYTE:   case GL_UNSIGNED_BYTE:
        case GL_SHORT:  case GL_UNSIGNED_SHORT:
        case GL_INT:    case GL_UNSIGNED_INT:
            break;
        default:
            goto bad_value;
        }
        break;
      default:
        goto bad_enum;
    }

    if ((border < 0) || (border > 1)) {
        goto bad_value;
    }

    return tex;
}

/* 6909ea30 -- CheckCopyTexImageArgs (no 3D proxy target) */
__GLtextureICD * __fastcall CheckCopyTexImageArgs(__GLcontext *gc, GLenum target,
                      GLint level,
                      GLenum internalformat,
                      GLint x, GLint y,
                      GLsizei width, GLsizei height,
                      GLint border, GLint dim)
{
    __GLtextureICD *tex;

    tex = CheckTexImageArgs(gc, target, level, internalformat, border,
                GL_RGBA, GL_FLOAT, dim);

    switch(target) {
      case GL_PROXY_TEXTURE_1D:
      case GL_PROXY_TEXTURE_2D:
    __glSetError(GL_INVALID_ENUM);
    return NULL;
    }
    switch(internalformat) {
    case 1: case 2: case 3: case 4:
        __glSetError(GL_INVALID_ENUM);
        return NULL;
    default:
        break;
    }
    if (!tex) {
    return 0;
    }
    if (!IsLegalRange(gc, width, border)) {
    return 0;
    }
    if ((dim > 1) &&
    !IsLegalRange(gc, height, border)) {
    return 0;
    }
    return tex;
}

/* 6909eaf0 -- s_tex.c __glim_TexImage1D */
void __stdcall __glim_TexImage1D(GLenum target, GLint lod, GLint components, GLsizei length,
                            GLint border, GLenum format, GLenum type, const GLvoid *buf)
{
    __GLtextureICD *tex;
    __GLmipMapLevelICD *dest;
    __GLpixelSpanInfo spanInfo;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_STATE();

    /* Check arguments and get the right texture being changed */
    tex = __glCheckTexImage1DArgs(gc, target, lod, components, length, border, format, type);
    if (!tex) {
        return;
    }
    if (tex->refCount > 2) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }

    /* Allocate memory for the level data */
    dest = (*(__GLcreateLevelProc)tex->createLevel)(gc, tex, lod, components,
                               length, 1+border*2, 1+border*2, border, 1);
    /* Copy image data */
    if (buf && dest) {
        __glInitTexSourceUnpack(gc, &spanInfo, length, 1, 1, format, type, buf, GL_FALSE);
        __glInitTexImageStore(gc, &spanInfo, tex, lod);
        __glInitUnpacker(gc, &spanInfo);
        __glInitPacker(gc, &spanInfo);
        (*(__GLcopyTexImageProc)tex->copyTexImage)(gc, &spanInfo, tex, lod);
    }

    /* remove texture from active list.  If needed, validation will reload */
    (*(__GLunmakeResidentProc)tex->unmakeResident)(gc, tex, gc->fx.activeTexture);

    /* Might have just disabled texturing... */
    __GL_DELAY_VALIDATE(gc);
}

/* 6909ec90 -- s_tex.c __glim_TexImage2D */
void __stdcall __glim_TexImage2D(GLenum target, GLint lod, GLint components, GLsizei w, GLsizei h,
                            GLint border, GLenum format, GLenum type, const GLvoid *buf)
{
    __GLtextureICD *tex;
    __GLmipMapLevelICD *dest;
    __GLpixelSpanInfo spanInfo;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_STATE();

    /* Check arguments and get the right texture being changed */
    tex = __glCheckTexImage2DArgs(gc, target, lod, components, w, h, border, format, type);
    if (!tex) {
        return;
    }
    if (tex->refCount > 2) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }

    /* Allocate memory for the level data */
    dest = (*(__GLcreateLevelProc)tex->createLevel)(gc, tex, lod, components,
                               w, h, 1+border*2, border, 2);
    /* Copy image data */
    if (buf && dest) {
        __glInitTexSourceUnpack(gc, &spanInfo, w, h, 1, format, type, buf, GL_FALSE);
        __glInitTexImageStore(gc, &spanInfo, tex, lod);
        __glInitUnpacker(gc, &spanInfo);
        __glInitPacker(gc, &spanInfo);
        (*(__GLcopyTexImageProc)tex->copyTexImage)(gc, &spanInfo, tex, lod);
    }

    /* remove texture from active list.  If needed, validation will reload */
    (*(__GLunmakeResidentProc)tex->unmakeResident)(gc, tex, gc->fx.activeTexture);

    /* Might have just disabled texturing... */
    __GL_DELAY_VALIDATE(gc);
}

/* 6909f1c0 -- texGenEyeLinearST: GL_EYE_LINEAR s and t on unit 0 */
void __fastcall texGenEyeLinearST(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLcoord tc;
    tc.z = vx->texture[0].z;
    tc.w = vx->texture[0].w;
    tc.x = gc->state.texture.unit[0].s.eyePlaneEquation.x * vx->eye.x +
           gc->state.texture.unit[0].s.eyePlaneEquation.y * vx->eye.y +
           gc->state.texture.unit[0].s.eyePlaneEquation.z * vx->eye.z +
           gc->state.texture.unit[0].s.eyePlaneEquation.w * vx->eye.w;
    tc.y = gc->state.texture.unit[0].t.eyePlaneEquation.x * vx->eye.x +
           gc->state.texture.unit[0].t.eyePlaneEquation.y * vx->eye.y +
           gc->state.texture.unit[0].t.eyePlaneEquation.z * vx->eye.z +
           gc->state.texture.unit[0].t.eyePlaneEquation.w * vx->eye.w;
    (*XF4(gc->transform.texture[0]))(&vx->texture[0], &tc, gc->transform.texture[0]);
}

/* 6909f250 -- texGenObjectLinearST: GL_OBJECT_LINEAR s and t on unit 0 */
void __fastcall texGenObjectLinearST(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLcoord tc;
    tc.z = vx->texture[0].z;
    tc.w = vx->texture[0].w;
    tc.x = gc->state.texture.unit[0].s.objectPlaneEquation.x * vx->obj.x +
           gc->state.texture.unit[0].s.objectPlaneEquation.y * vx->obj.y +
           gc->state.texture.unit[0].s.objectPlaneEquation.z * vx->obj.z +
           gc->state.texture.unit[0].s.objectPlaneEquation.w * vx->obj.w;
    tc.y = gc->state.texture.unit[0].t.objectPlaneEquation.x * vx->obj.x +
           gc->state.texture.unit[0].t.objectPlaneEquation.y * vx->obj.y +
           gc->state.texture.unit[0].t.objectPlaneEquation.z * vx->obj.z +
           gc->state.texture.unit[0].t.objectPlaneEquation.w * vx->obj.w;
    (*XF4(gc->transform.texture[0]))(&vx->texture[0], &tc, gc->transform.texture[0]);
}

/* 6909f2f0 -- __glCalcSphereMap for unit 0 */
void __fastcall __glCalcSphereMap(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLcoord sphereCoord;
    __GLmatrixICD *m;

    SphereGen(gc, vx, &sphereCoord);
    sphereCoord.z = vx->texture[0].z;
    sphereCoord.w = vx->texture[0].w;
    m = &gc->transform.texture[0]->matrix;
    (*(__GLxformProc)m->xf4)(&vx->texture[0], &sphereCoord.x, m);
}

/* 6909f330 -- __glCalcTexture (3dfx S_TEX.C): both units through their texture matrices */
void __fastcall __glCalcTexture(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLcoord tc;
    tc.x = vx->texture[0].x; tc.y = vx->texture[0].y; tc.z = vx->texture[0].z; tc.w = vx->texture[0].w;
    (*XF4(gc->transform.texture[0]))(&vx->texture[0], &tc, gc->transform.texture[0]);
    tc.x = vx->texture[1].x; tc.y = vx->texture[1].y; tc.z = vx->texture[1].z; tc.w = vx->texture[1].w;
    (*XF4(gc->transform.texture[1]))(&vx->texture[1], &tc, gc->transform.texture[1]);
}

/* 6909f3f0 -- __glFastCalcTexturePersp (3dfx S_TEX.C): the texture matrices unless both are identity
** (matrixType 4), then s, t and q of both units scaled by window.w when the vertex is not clipped */
void __fastcall __glFastCalcTexturePersp(__GLcontext *gc, __GLvertexICD *vx)
{
    if (gc->transform.texture[0]->matrix.matrixType != 4 ||
        gc->transform.texture[1]->matrix.matrixType != 4) {
        (*gc->procs.calcTextureUnit[0])(gc, vx);
    }
    if (!(vx->flagBits & 0xffff0000)) {
        vx->texture[0].x = vx->window.w * vx->texture[0].x;
        vx->texture[0].y = vx->window.w * vx->texture[0].y;
        vx->texture[0].w = vx->window.w * vx->texture[0].w;
    }
    if (!(vx->flagBits & 0xffff0000)) {
        vx->texture[1].x = vx->window.w * vx->texture[1].x;
        vx->texture[1].y = vx->window.w * vx->texture[1].y;
        vx->texture[1].w = vx->window.w * vx->texture[1].w;
    }
}

/* 6909f7f0 -- CheckTexSubImageArgs (types as CheckTexImageArgs, no 3D proxy) */
__GLtextureICD * __fastcall CheckTexSubImageArgs(__GLcontext *gc, GLenum target,
                                         GLint lod, GLenum format,
                                         GLenum type, GLint dim)
{
    __GLtextureICD *tex = __glLookupActiveTexture(gc, target);

    if (!tex || (target == GL_PROXY_TEXTURE_1D) ||
                (target == GL_PROXY_TEXTURE_2D))
    {
      bad_enum:
        __glSetError(GL_INVALID_ENUM);
        return 0;
    }

    if (tex->dim != dim) {
        goto bad_enum;
    }

    switch (type) {
      case GL_BITMAP:
        if (format != GL_COLOR_INDEX) goto bad_enum;
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
            return 0;
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
            return 0;
        }
        break;
      default:
        goto bad_enum;
    }

    switch (format) {
      case GL_COLOR_INDEX:      case GL_RED:
      case GL_GREEN:            case GL_BLUE:
      case GL_ALPHA:            case GL_RGB:
      case GL_RGBA:             case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:  case GL_ABGR_EXT:
      case GL_BGRA:         case GL_BGR:
        break;
      default:
        goto bad_enum;
    }

    if ((lod < 0) || (lod >= gc->constants.maxMipMapLevel)) {
        __glSetError(GL_INVALID_VALUE);
        return 0;
    }

    return tex;
}

/* 6909fb80 -- s_tex.c __glim_TexSubImage1D (no pixel-path width adjustment) */
void __stdcall __glim_TexSubImage1D(GLenum target, GLint lod,
                             GLint xoffset, GLint length, GLenum format, GLenum type, const GLvoid *buf)
{
    __GLtextureICD *tex;
    __GLpixelSpanInfo spanInfo;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_STATE();

    /* Check arguments and get the right texture level being changed */
    tex = __glCheckTexSubImage1DArgs(gc, target, lod, xoffset, length, format, type);
    if (!tex) {
	return;
    }

    /* Copy sub-image data */
    __glInitTexSourceUnpack(gc, &spanInfo, length, 1, 1, format, type, buf, GL_FALSE);
    __glInitTexSubImageStore(gc, &spanInfo, tex, lod, xoffset, 0, 0);
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);
    (*(__GLcopyTexImageProc)tex->copyTexImage)(gc, &spanInfo, tex, lod);

    /* remove texture from active list.  If needed, validation will reload */
    (*(__GLunmakeResidentProc)tex->unmakeResident)(gc, tex, gc->fx.activeTexture);

    __GL_DELAY_VALIDATE(gc);
}

/* 6909fd00 -- s_tex.c __glim_TexSubImage2D; a level with a second image (+0x04) gets the
** rectangle through 690a0b90 too */
void __stdcall __glim_TexSubImage2D(GLenum target, GLint lod,
                             GLint xoffset, GLint yoffset, GLsizei w, GLsizei h,
                             GLenum format, GLenum type, const GLvoid *buf)
{
    __GLtextureICD *tex;
    __GLmipMapLevelICD *lp;
    __GLpixelSpanInfo spanInfo;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_STATE();

    tex = __glCheckTexSubImage2DArgs(gc, target, lod, xoffset, yoffset, w, h, format, type);
    if (!tex) {
	return;
    }

    __glInitTexSourceUnpack(gc, &spanInfo, w, h, 1, format, type, buf, GL_FALSE);
    __glInitTexSubImageStore(gc, &spanInfo, tex, lod, xoffset, yoffset, 0);
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);
    (*(__GLcopyTexImageProc)tex->copyTexImage)(gc, &spanInfo, tex, lod);

    lp = tex->level[lod];
    if (lp->pixelBuffer) {
	__glConvertTexLevelPixels(gc, lp, xoffset, yoffset, w, h);
    }

    (*(__GLunmakeResidentProc)tex->unmakeResident)(gc, tex, gc->fx.activeTexture);

    __GL_DELAY_VALIDATE(gc);
}

/* 6909feb0 -- __glDisposeTexObj (s_names.c namesTypeInfo[TEXOBJ].free): drop a reference; the texture's free
** proc (+0x3c) on the last one */
void __fastcall __glDisposeTexObj(__GLcontext *gc, GLint *obj)
{
    if (--obj[0] == 0) (*(PFNRELEASE)obj[15])(gc, obj);
}

/* 690a0250 -- __glim_BindTexture: glBindTexture (__glim_BindTexture: 1D / 2D only) */
void __stdcall __glim_BindTexture(GLenum target, GLuint texture)
{
    GLuint targetIndex;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

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

    __GL_DELAY_VALIDATE(gc);
}

/* 690a0310 -- __glim_PrioritizeTextures: a negative n only sets the error; the bound object of the
** active unit and its per-target parameter copy get the new priority too */
void __stdcall __glim_PrioritizeTextures(GLsizei n,
                             const GLuint* textures, const GLclampf* priorities)
{
    int i;
    __GLtextureICD *tex;
    __GLfloat priority;

    __GL_SETUP_NOT_IN_BEGIN();
    if (n < 0) {
        __glSetError(GL_INVALID_VALUE);
    }
    if (n == 0) return;

    for (i=0; i < n; i++) {
        /* silently ignore default texture */
        if (0 == textures[i]) continue;

        tex = (__GLtextureICD *)
            __glNamesLockData(gc, gc->texture.namesArray, textures[i]);

        /* silently ignore non-texture */
        if (NULL == tex) continue;

        priority = Clampf(priorities[i], __GL_ZERO_F, __GL_ONE_F);
        tex->priority = priority;
        if (tex->name == BOUND_TEXTURE(gc, tex)->name) {
            BOUND_TEXTURE(gc, tex)->priority = priority;
            *(GLuint *)&gc->state.texture.unit[gc->fx.activeTexture].param[tex->targetIndex].priority =
                *(GLuint *)&BOUND_TEXTURE(gc, tex)->priority;
        }

        __glNamesUnlockData(gc, tex);
    }
}

/* 690a0430 -- __glim_AreTexturesResident (a negative n only sets the error; consistency per active unit) */
GLboolean __stdcall __glim_AreTexturesResident(GLsizei n,
                                  const GLuint* textures, GLboolean* residences)
{
    int i;
    __GLtextureICD *tex;
    GLboolean allResident = GL_TRUE;

    __GL_SETUP_NOT_IN_BEGIN2();
    if (n < 0) {
        __glSetError(GL_INVALID_VALUE);
    }
    if (n == 0) return GL_FALSE;

    for (i=0; i < n; i++) {
        /* Can't query a default texture. */
        if (0 == textures[i]) {
            __glSetError(GL_INVALID_VALUE);
            return GL_FALSE;
        }
        tex = (__GLtextureICD *)
            __glNamesLockData(gc, gc->texture.namesArray, textures[i]);
        if (NULL == tex) {
            __glSetError(GL_INVALID_VALUE);
            return GL_FALSE;
        }
        if (!__glIsActiveTextureConsistent(gc, tex) || !IsTextureResident(gc, tex)) {
            allResident = GL_FALSE;
            residences[i] = GL_FALSE;
        } else {
            residences[i] = GL_TRUE;
        }
        __glNamesUnlockData(gc, tex);
    }

    return allResident;
}

/* 690a0600 -- s_tex.c __glim_CopyTexImage1D */
void __stdcall __glim_CopyTexImage1D(GLenum target, GLint level, GLenum internalformat,
                            GLint x, GLint y, GLsizei width, GLint border)
{
    __GLtextureICD *tex;
    __GLmipMapLevelICD *dest;
    __GLpixelSpanInfo spanInfo;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_PIXEL_OP();

    (*gc->procs.revalidate)(gc);
    (*gc->procs.waitIdle)(gc);

    /* Check arguments and get the right texture being changed */
    tex = CheckCopyTexImageArgs(gc, target, level, internalformat, x, y, width, 1, border, 1);
    if (!tex) {
        return;
    }
    if (tex->refCount > 2) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }

    /* Allocate memory for the level data */
    dest = (*(__GLcreateLevelProc)tex->createLevel)(gc, tex, level, internalformat,
                               width, 1+border*2, 1+border*2, border, 1);

    /* Copy image data */
    if (dest) {
        __glInitReadImageSrcInfo(gc, &spanInfo, x, y, width, 1);
        __glInitTexImageStore(gc, &spanInfo, tex, level);
        __glInitUnpacker(gc, &spanInfo);
        __glInitPacker(gc, &spanInfo);

        if (!__glClipReadPixels(gc, &spanInfo)) return;
        (*(__GLcopyTexImageProc)tex->readTexImage)(gc, &spanInfo, tex, level);
    }

    /* Might have just disabled texturing... */
    __GL_DELAY_VALIDATE(gc);
}

/* 690a0770 -- s_tex.c __glim_CopyTexImage2D */
void __stdcall __glim_CopyTexImage2D(GLenum target, GLint level, GLenum internalformat,
                            GLint x, GLint y, GLsizei w, GLsizei h, GLint border)
{
    __GLtextureICD *tex;
    __GLmipMapLevelICD *dest;
    __GLpixelSpanInfo spanInfo;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_PIXEL_OP();

    (*gc->procs.revalidate)(gc);
    (*gc->procs.waitIdle)(gc);

    /* Check arguments and get the right texture being changed */
    tex = CheckCopyTexImageArgs(gc, target, level, internalformat, x, y, w, h, border, 2);
    if (!tex) {
        return;
    }
    if (tex->refCount > 2) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }

    /* Allocate memory for the level data */
    dest = (*(__GLcreateLevelProc)tex->createLevel)(gc, tex, level, internalformat,
                               w, h, 1+border*2, border, 2);

    /* Copy image data */
    if (dest) {
        __glInitReadImageSrcInfo(gc, &spanInfo, x, y, w, h);
        __glInitTexImageStore(gc, &spanInfo, tex, level);
        __glInitUnpacker(gc, &spanInfo);
        __glInitPacker(gc, &spanInfo);

        if (!__glClipReadPixels(gc, &spanInfo)) return;
        (*(__GLcopyTexImageProc)tex->readTexImage)(gc, &spanInfo, tex, level);
    }

    /* Might have just disabled texturing... */
    __GL_DELAY_VALIDATE(gc);
}

/* 690a08e0 -- s_tex.c __glim_CopyTexSubImage1D */
void __stdcall __glim_CopyTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                            GLint x, GLint y, GLsizei width)
{
    __GLtextureICD *tex;
    __GLpixelSpanInfo spanInfo;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_PIXEL_OP();

    (*gc->procs.revalidate)(gc);
    (*gc->procs.waitIdle)(gc);

    /* Check arguments and get the right texture level being changed */
    tex = __glCheckCopyTexSubImageArgs(gc, target, level, xoffset, 0, 0, x, y, width, 1, 1);
    if (!tex) {
        return;
    }
    if (tex->refCount > 2) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }

    /* Copy sub-image data */
    __glInitReadImageSrcInfo(gc, &spanInfo, x, y, width, 1);
    __glInitTexSubImageStore(gc, &spanInfo, tex, level, xoffset, 0, 0);
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);

    if (__glClipReadPixels(gc, &spanInfo)) {
        (*(__GLcopyTexImageProc)tex->readTexImage)(gc, &spanInfo, tex, level);
        __GL_DELAY_VALIDATE_MASK(gc, 0x20);   /* __GL_DIRTY_TEXTURE */
    }
}

/* 690a0b90 -- __glConvertTexLevelPixels: convert a rectangle of a texture level's texels into the draw
** buffer's pixel format
** (lp->pixelBuffer, allocated lazily); the source offset uses w as the row length, as the original */
void __fastcall __glConvertTexLevelPixels(__GLcontext *gc, __GLmipMapLevelICD *lp,
                             GLint x, GLint y, GLint w, GLint h)
{
    __GLcolorBufferICD *cfb = gc->drawBuffer;
    GLint rShift = 8 - gc->modes.redBits;
    GLint gShift = 8 - gc->modes.greenBits;
    GLint bShift = 8 - gc->modes.blueBits;
    GLint aShift = 8 - gc->modes.alphaBits;
    GLint elementSize = (cfb->buf.wglBuf)->elementSize;
    GLubyte *src = (GLubyte *) lp->buffer + y * w + x;
    GLubyte *dst = (GLubyte *) LP_PIXELBUFFER(lp) + (x + y * w) * elementSize;
    GLint srcSkip = lp->width - w;
    GLint dstSkip = srcSkip * elementSize;
    GLubyte r, g, b, a;
    GLuint pixel;
    GLint i, j;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            switch (LP_INTERNALFORMAT(lp)) {
              case __GL_TEXFMT_L8:
                r = *src++;
                g = r;
                b = r;
                a = 0;
                break;
              case __GL_TEXFMT_LA88:
                r = src[0];
                g = r;
                b = r;
                a = src[1];
                src += 2;
                break;
              case __GL_TEXFMT_RGB888:
                r = src[0];
                g = src[1];
                b = src[2];
                a = 0;
                src += 3;
                break;
              case __GL_TEXFMT_RGBA8888:
                r = src[0];
                g = src[1];
                b = src[2];
                a = src[3];
                src += 4;
                break;
              case __GL_TEXFMT_I8:
                r = *src++;
                g = r;
                b = r;
                a = r;
                break;
            }
            r >>= rShift;
            g >>= gShift;
            b >>= bShift;
            a >>= aShift;
            pixel = (a << cfb->alphaShift) | (b << cfb->blueShift) |
                    (g << cfb->greenShift) | (r << cfb->redShift);
            switch (elementSize) {
              case 1:
                *dst = (GLubyte) pixel;
                dst++;
                break;
              case 2:
                *(GLushort *) dst = (GLushort) pixel;
                dst += 2;
                break;
              case 4:
                *(GLuint *) dst = pixel;
                dst += 4;
                break;
            }
        }
        src += srcSkip;
        dst += dstSkip;
    }
}

/* 690a0dd0 -- __glMakeTexPixelBuffer: unit 0 texture, level 0: allocate the second buffer (+0x04: width * height
** draw-buffer elements) on first use and fill it through 690a0b90
*/
void __fastcall __glMakeTexPixelBuffer(__GLcontext *gc)
{
    __GLmipMapLevelICD *lp = __glCurrentTexture(gc, 0)->level[0];
    GLint size;

    if (lp->pixelBuffer == 0) {
        size = lp->height * lp->width;
        size *= (gc->drawBuffer->buf.wglBuf)->elementSize;
        lp->pixelBuffer = (void *)(*(__GLreallocProc)gc->imports[2])(gc, NULL, size);
        __glConvertTexLevelPixels(gc, lp, 0, 0, lp->width, lp->height);
    }
}

/* 690a2580 -- __glExtractTexelRGB565 */
void __fastcall __glExtractTexelRGB565(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                   GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLushort *image, texel;

    if ((row < 0) || (col < 0) ||
        (row >= level->height2) || (col >= level->width2)) {
        result->r = (GLubyte) (tex->params.borderColor.r * 255.0F);
        result->g = (GLubyte) (tex->params.borderColor.g * 255.0F);
        result->b = (GLubyte) (tex->params.borderColor.b * 255.0F);
    } else {
        image = (GLushort *)level->buffer + (row << LP_WIDTHLOG2(level)) + col;

        texel = image[0];
        result->r = (GLubyte) (((GLubyte) ((texel >> 11) & 0x1F)) * (255.0F/31.0F));
        result->g = (GLubyte) (((GLubyte) ((texel >>  5) & 0x3F)) * (255.0F/63.0F));
        result->b = (GLubyte) (((GLubyte) ((texel      ) & 0x1F)) * (255.0F/31.0F));
    }
}

/* 690a35b0 -- __glNearestFilter1 */
void __fastcall __glNearestFilter1(__GLtextureICD *tex, __GLmipMapLevelICD *lp,
                             __GLfloat s, __GLfloat t, __GLtexel *result)
{
    GLint col = (GLint) s;
    GLint w2 = lp->width2;

    if (tex->params.sWrapMode == GL_REPEAT) {
        col &= w2-1;
    } else {
        if (col < 0) col = 0;
        else if (col >= w2) col = w2 - 1;
    }

    (*(__GLextractProc)lp->extract)(lp, tex, 0, 0, col, result);
}

/* 690a3600 -- __glNearestFilter2 (no r coordinate) */
void __fastcall __glNearestFilter2(__GLtextureICD *tex, __GLmipMapLevelICD *lp,
                             __GLfloat s, __GLfloat t, __GLtexel *result)
{
    GLint row = (GLint) t;
    GLint col = (GLint) s;
    GLint w2 = lp->width2;
    GLint h2 = lp->height2;

    if (tex->params.sWrapMode == GL_REPEAT) {
        col &= w2-1;
    } else {
        if (col < 0) col = 0;
        else if (col >= w2) col = w2 - 1;
    }

    if (tex->params.tWrapMode == GL_REPEAT) {
        row &= h2-1;
    } else {
        if (row < 0) row = 0;
        else if (row >= h2) row = h2 - 1;
    }

    (*(__GLextractProc)lp->extract)(lp, tex, 0, row, col, result);
}

/* 690a3680 -- __glLinearFilter1 (s_texel.c; the ICD's filters take no r and have no
** GL_CLAMP_TO_EDGE) */
void __fastcall __glLinearFilter1(__GLtextureICD *tex, __GLmipMapLevelICD *lp,
                             __GLfloat s, __GLfloat t, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    __GLfloat u, alpha, omalpha;
    GLint col0, col1;
    __GLtexel t0, t1;

    /* Find col0 and col1 */
    u = s;
    if (tex->params.sWrapMode == GL_REPEAT) {
        u -= __glHalf;
        col0 = ((GLint) __glFloor(u)) & (lp->width2 - 1);
        col1 = (col0 + 1) & (lp->width2 - 1);
    } else {
        if (u < __GL_ZERO_F) u = __GL_ZERO_F;
        else if (u > lp->width2) u = lp->width2;
        u -= __glHalf;
        col0 = (GLint) __glFloor(u);
        col1 = col0 + 1;
    }

    /* Compute alpha and beta */
    alpha = __GL_FRAC(u);

    /* Calculate the final texel value as a combination of the two texels */
    (*(TEXfn_extract) lp->extract)(lp, tex, 0, 0, col0, &t0);
    (*(TEXfn_extract) lp->extract)(lp, tex, 0, 0, col1, &t1);

    omalpha = __GL_ONE_F - alpha;

    switch (tex->baseFormat) {
      case GL_LUMINANCE_ALPHA:
        result->a = omalpha * t0.a + alpha * t1.a;
        /* FALLTHROUGH */
      case GL_LUMINANCE:
        result->r = omalpha * t0.r + alpha * t1.r;
        break;
      case GL_RGBA:
        result->a = omalpha * t0.a + alpha * t1.a;
        /* FALLTHROUGH */
      case GL_RGB:
        result->r = omalpha * t0.r + alpha * t1.r;
        result->g = omalpha * t0.g + alpha * t1.g;
        result->b = omalpha * t0.b + alpha * t1.b;
        break;
      case GL_ALPHA:
        result->a = omalpha * t0.a + alpha * t1.a;
        break;
      case GL_INTENSITY:
        result->r = omalpha * t0.r + alpha * t1.r;
        break;
    }
}

/* 690a3fd0 -- __glLinearFilter (s_texel.c): s and t scaled by the base level's width2f / height2f */
void __fastcall __glLinearFilter(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod,
                             __GLfloat s, __GLfloat t)
{
    __GLmipMapLevelICD *lp = tex->level[0];

    (*(__GLtexFilterProc)tex->linear)(tex, lp, s * lp->width2f, t * lp->height2f, result);
}

/* 690a4000 -- __glNearestFilter (s_texel.c): s and t scaled by the base level's width2f / height2f */
void __fastcall __glNearestFilter(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod,
                             __GLfloat s, __GLfloat t)
{
    __GLmipMapLevelICD *lp = tex->level[0];

    (*(__GLtexFilterProc)tex->nearest)(tex, lp, s * lp->width2f, t * lp->height2f, result);
}

/* 690a4030 -- s_texture.c __glNMNFilter (nearest mipmap; the level filter tex->nearest) */
MIPMAP_NEAREST(__glNMNFilter, nearest)

/* 690a40a0 -- s_texture.c __glLMNFilter (nearest mipmap; the level filter tex->linear) */
MIPMAP_NEAREST(__glLMNFilter, linear)

/* 690a4da0 -- __glComputePolygonRho: unit 0, two coordinates, no zero tests */
__GLfloat __fastcall __glComputePolygonRho(__GLcontext *gc, __GLshadeRho *sh,
                                  __GLfloat s, __GLfloat t, __GLfloat qw)
{
    GLuint modeFlags = MODEFLAGS(gc);
    __GLtextureICD *tex = __glCurrentTexture(gc, 0);
    __GLfloat qw0, qw1, p0, p1;
    __GLfloat pupx, pupy, pvpx, pvpy;
    __GLfloat px, py;

    /* Compute partial of u with respect to x */
    qw0 = __GL_ONE_F / (qw - sh->dqwdx);
    qw1 = __GL_ONE_F / (qw + sh->dqwdx);
    p0 = (s - sh->dsdx) * qw0;
    p1 = (s + sh->dsdx) * qw1;
    pupx = p1 - p0;
    if (!(modeFlags & __GL_SHADE_UVSCALED_TEXTURE)) {
        pupx *= tex->level[0]->width2f;
    }

    /* Compute partial of v with respect to x */
    p0 = (t - sh->dtdx) * qw0;
    p1 = (t + sh->dtdx) * qw1;
    pvpx = p1 - p0;
    if (!(modeFlags & __GL_SHADE_UVSCALED_TEXTURE)) {
        pvpx *= tex->level[0]->height2f;
    }

    /* Compute partial of u with respect to y */
    qw0 = __GL_ONE_F / (qw - sh->dqwdy);
    qw1 = __GL_ONE_F / (qw + sh->dqwdy);
    p0 = (s - sh->dsdy) * qw0;
    p1 = (s + sh->dsdy) * qw1;
    pupy = p1 - p0;
    if (!(modeFlags & __GL_SHADE_UVSCALED_TEXTURE)) {
        pupy *= tex->level[0]->width2f;
    }

    /* Compute partial of v with respect to y */
    p0 = (t - sh->dtdy) * qw0;
    p1 = (t + sh->dtdy) * qw1;
    pvpy = p1 - p0;
    if (!(modeFlags & __GL_SHADE_UVSCALED_TEXTURE)) {
        pvpy *= tex->level[0]->height2f;
    }

    /* Finally, figure sum of squares */
    px = pupx * pupx + pvpx * pvpx;
    py = pupy * pupy + pvpy * pvpy;

    /* Return largest value as the level of detail */
    if (px > py) {
        return px * ((__GLfloat) 0.25);
    } else {
        return py * ((__GLfloat) 0.25);
    }
}

/************************************************************************/
/* s_texel.c */

/* 690a4f20 -- __glComputeLineRho: unit 0's texture, no zero tests, no third coordinate */
__GLfloat __fastcall __glComputeLineRho(__GLcontext *gc, GLint unit,
                                  __GLfloat s, __GLfloat t, __GLfloat qw)
{
    __GLfloat pspx, pspy, ptpx, ptpy;
    __GLfloat pupx, pupy, pvpx, pvpy;
    __GLfloat temp, pu, pv, p2;
    __GLfloat invMag, invMag2;
    __GLfloat dx, dy, invqw;
    const __GLtextureICD *tex = __glCurrentTexture(gc, 0);
    const __GLvertexICD *v0 = gc->line.v0;
    const __GLvertexICD *v1 = gc->line.v1;

    /* Compute the length of the line (its magnitude) */
    dx = v1->window.x - v0->window.x;
    dy = v1->window.y - v0->window.y;
    invMag = __GL_ONE_F / (__GLfloat)sqrt(dx*dx + dy*dy);
    invMag2 = invMag * invMag;

    invqw = __GL_ONE_F / qw;

    /* Compute s partials */
    temp = ((v1->texture[0].x - v0->texture[0].x) - s) * invqw;
    pspx = temp * dx * invMag2;
    pspy = temp * dy * invMag2;

    /* Compute t partials */
    temp = ((v1->texture[0].y - v0->texture[0].y) - t) * invqw;
    ptpx = temp * dx * invMag2;
    ptpy = temp * dy * invMag2;

    pupx = pspx * tex->level[0]->width2f;
    pupy = pspy * tex->level[0]->width2f;
    pvpx = ptpx * tex->level[0]->height2f;
    pvpy = ptpy * tex->level[0]->height2f;

    /* Now compute rho */
    pu = pupx * dx + pupy * dy;
    pv = pvpx * dx + pvpy * dy;
    p2 = pu * pu + pv * pv;

    return (p2 * invMag2);
}

/* 690a4ff0 -- __glTextureFragmentUVScale (3dfx S_TEX.C): unit 0's texture proc with s and t scaled by level
** 0's width (both by width2f, as the ancestor) */
void __fastcall __glTextureFragmentUVScale(__GLcontext *gc, __GLcolor *color, __GLfloat s, __GLfloat t, __GLfloat rho)
{
    __GLmipMapLevelICD *lp = __glCurrentTexture(gc, 0)->level[0];

    (*(void (__fastcall *)(__GLcontext *, __GLcolor *, __GLfloat, __GLfloat, __GLfloat))
        gc->procs.texture[0])(gc, color, s * lp->width2f, t * lp->width2f, rho);
}

/* 690a5030 -- __glFastTextureFragment */
void __fastcall __glFastTextureFragment(__GLcontext *gc, __GLcolor *color, __GLfloat s, __GLfloat t, __GLfloat rho)
{
    __GLtextureICD *tex = __glCurrentTexture(gc, 0);
    __GLtexel texel;

    (*(__GLtexMagProc)tex->magnify)(tex, &texel, __GL_ZERO_F, s, t);
    (*(__GLtexEnvProc)tex->env)(gc, color, &texel);
}

/* 690c3640 -- __glInitTextureManager (s_texmgr.c): a copy of the template, referenced once */
void __fastcall __glInitTextureManager(__GLcontext *gc)
{
    __GLtexMgrICD *tm;

    tm = (__GLtexMgrICD *)__GL_MALLOC(gc, sizeof(__GLtexMgrICD));
    *tm = g_swTexMgrTemplate;
    tm->refs++;
    gc->texMgr = tm;
    gc->createTexture = (__GLproc) __glCreateTexture;
}

/* 690c3680 -- __glFreeTextureManager (s_texmgr.c): release the shared texture memory manager */
void __fastcall __glFreeTextureManager(__GLcontext *gc)
{
    __GLtexMgrICD *tm = gc->texMgr;
    if (--tm->refs == 0) __GL_FREE(gc, tm);
    gc->texMgr = NULL;
}
/* 690c36d0 -- __glCreateTexture (s_texmgr.c; gc->createTexture): the target comes as an index
** (0 1D, 1 2D, 2 proxy 1D, 3 proxy 2D).
*/
__GLtextureICD *__fastcall __glCreateTexture(__GLcontext *gc, GLuint name, GLint targetIndex)
{
    __GLtextureICD *tex;
    GLint level, maxMipMapLevel;
    __GLmipMapLevelICD *lp;

    tex = (__GLtextureICD *) __GL_CALLOC(gc, 1, sizeof(__GLtextureICD));

    tex->gc = gc;
    tex->refCount = 1;
    tex->targetIndex = targetIndex;
    {   /* a loop over both TMU slots: the zero is then held in a register (VC6 constant weight) */
        GLint i;
        for (i = 0; i < 2; i++)
            tex->hwHandle[i] = 0;
    }
    tex->free = (__GLproc) __glFreeTexture;
    tex->makeResident = (__GLproc) __glMakeResidentTexture;
    tex->unmakeResident = (__GLproc) __glUnmakeResidentTexture;
    tex->copyTexImage = (__GLproc) __glCopyTexImage;
    tex->readTexImage = (__GLproc) __glReadTexImage;
    tex->name = name;
    tex->priority = 1.0F;
    tex->CT.internalFormat = GL_RGBA;

    /* Set default parameter state */
    tex->params.sWrapMode = GL_REPEAT;
    tex->params.tWrapMode = GL_REPEAT;
    tex->params.minFilter = GL_NEAREST_MIPMAP_LINEAR;
    tex->params.magFilter = GL_LINEAR;

    switch (targetIndex) {
      case 0:
        tex->dim = 1;
        tex->createLevel = (__GLproc) __glTextureCreateLevel;
        tex->CT.target = GL_TEXTURE_1D;
        break;
      case 1:
        tex->dim = 2;
        tex->createLevel = (__GLproc) __glTextureCreateLevel;
        tex->CT.target = GL_TEXTURE_2D;
        break;
      case 2:
        tex->dim = 1;
        tex->createLevel = (__GLproc) __glTextureCreateProxyLevel;
        break;
      case 3:
        tex->dim = 2;
        tex->createLevel = (__GLproc) __glTextureCreateProxyLevel;
        break;
      default:
        break;
    }
    tex->releaseLevel = (__GLproc) __glTextureDeleteLevel;

    maxMipMapLevel = gc->constants.maxMipMapLevel;
    tex->level = (__GLmipMapLevelICD **)
        __GL_CALLOC(gc, (size_t) maxMipMapLevel,
                                         sizeof(__GLmipMapLevelICD *));

    tex->level[0] = (__GLmipMapLevelICD *)
        __GL_CALLOC(gc, (size_t) maxMipMapLevel,
                                         sizeof(__GLmipMapLevelICD));

    /* Init each texture level */
    for (level = 0, lp = tex->level[0];
         level < maxMipMapLevel; level++, lp++) {
        tex->level[level] = lp;
        tex->level[level]->requestedFormat = 1;
    }

    return tex;
}

/* 690c3900 -- __glTextureCreateLevel (s_texmgr.c, 2D: the depth is not part of the size); the ICD
** also records the byte size of uncompressed images and the level-0 aspect scales */
void *__fastcall __glTextureCreateLevel(__GLcontext *gc, __GLtextureICD *tex,
                              GLint lod, GLint components,
                              GLsizei w, GLsizei h, GLsizei d,
                              GLint border, GLint dim)
{
    __GLmipMapLevelICD *lp = tex->level[lod];
    const __GLtextureFormat *texFormat;
    GLenum baseFormat;
    GLint baseWidth  = (w-border*2) * (1 << lod);
    GLint baseHeight = (h-border*2) * (1 << lod);
    GLint bufferSize;
    GLint maxDim;
    GLint compressed;

    if (components == GL_COMPRESSED_RGB_FXT1_3DFX ||
        components == GL_COMPRESSED_RGBA_FXT1_3DFX ||
        (components >= GL_RGB_S3TC && components <= GL_RGBA4_S3TC)) {
        compressed = 1;
    } else {
        compressed = 0;
    }

    if (baseWidth > baseHeight) {
        maxDim = baseWidth;
    } else {
        maxDim = baseHeight;
    }

    TEX_NUMLEVELS(tex) = __glFloorLog2(maxDim)+1;

    texFormat = (const __GLtextureFormat *)
        (*g_device->chooseTexFormat)(gc, components, &baseFormat);

    if (compressed) {
        bufferSize = w * h * ((texFormat->bitsPerTexel+7) >> 3) + 1;
    } else {
        bufferSize = w * h * ((texFormat->bitsPerTexel+7) >> 3) + 1;
        lp->compressedSize = bufferSize - 1;
    }

    if (baseWidth > gc->constants.maxTextureSize ||
        baseHeight > gc->constants.maxTextureSize) {
        /* Texture allocation failed */
        __glSetError(GL_INVALID_VALUE);
        return 0;
    } else if (bufferSize) {
        /* Texture allocation succeeded, fill in new level info */
        lp->buffer = __GL_REALLOC(gc, lp->buffer, (size_t) bufferSize);

        if (lp->buffer == NULL) {
            __glSetError(GL_OUT_OF_MEMORY);
            return 0;
        }

        /* This is allocated lazily */
        if (LP_PIXELBUFFER(lp) != NULL) {
            __GL_FREE(gc, LP_PIXELBUFFER(lp));
            LP_PIXELBUFFER(lp) = NULL;
        }

        lp->width = w;
        lp->height = h;
        LP_DEPTH(lp) = d;
        LP_IMAGESIZE(lp) = w * h;
        lp->width2 = w - border*2;
        LP_WIDTHLOG2(lp) = __glFloorLog2(lp->width2);
        lp->height2 = h - border*2;
        LP_HEIGHTLOG2(lp) =__glFloorLog2(lp->height2);
        LP_DEPTH2(lp) = d - border*2;
        LP_DEPTHLOG2(lp) =__glFloorLog2(LP_DEPTH2(lp));
        lp->width2f = lp->width2;
        lp->height2f = lp->height2;
        LP_DEPTH2F(lp) = LP_DEPTH2(lp);
        lp->border = border;
        LP_TEXFORMAT(lp) = texFormat;
        lp->requestedFormat = (GLenum) components;
        lp->baseFormat = baseFormat;
        LP_INTERNALFORMAT(lp) = texFormat->internalFormat;
        lp->redSize = texFormat->redSize;
        lp->greenSize = texFormat->greenSize;
        lp->blueSize = texFormat->blueSize;
        lp->alphaSize = texFormat->alphaSize;
        lp->luminanceSize = texFormat->luminanceSize;
        lp->intensitySize = texFormat->intensitySize;
        if (border) {
            lp->extract = texFormat->extractTexelBorder;
        } else {
            lp->extract = texFormat->extractTexel;
        }
        TEX_TEXFORMAT(tex) = texFormat;
    } else {
        /* The texture level is being freed */
        if (lp->buffer != NULL) {
            __GL_FREE(gc, lp->buffer);
            lp->buffer = NULL;
        }
        if (LP_PIXELBUFFER(lp) != NULL) {
            __GL_FREE(gc, LP_PIXELBUFFER(lp));
            LP_PIXELBUFFER(lp) = NULL;
        }

        lp->width = 0;
        lp->height = 0;
        LP_DEPTH(lp) = 0;
        LP_IMAGESIZE(lp) = 0;
        lp->width2 = 0;
        lp->height2 = 0;
        LP_DEPTH2(lp) = 0;
        LP_WIDTHLOG2(lp) = 0;
        LP_HEIGHTLOG2(lp) = 0;
        LP_DEPTHLOG2(lp) = 0;
        lp->border = 0;
        LP_TEXFORMAT(lp) = 0;
        lp->requestedFormat = 1;
        lp->baseFormat = 0;
        LP_INTERNALFORMAT(lp) = 0;
        lp->redSize = 0;
        lp->greenSize = 0;
        lp->blueSize = 0;
        lp->alphaSize = 0;
        lp->luminanceSize = 0;
        lp->intensitySize = 0;
        lp->extract = (__GLproc) __glNop6;
    }

    if (lod == 0) {
        __GLmipMapLevelICD *lp0;
        GLint diff;

        tex->maxLevel = LP_HEIGHTLOG2(lp);
        if (LP_WIDTHLOG2(lp) > tex->maxLevel) tex->maxLevel = LP_WIDTHLOG2(lp);
        if (LP_DEPTHLOG2(lp) > tex->maxLevel) tex->maxLevel = LP_DEPTHLOG2(lp);

        lp0 = tex->level[0];
        diff = LP_WIDTHLOG2(lp0) - LP_HEIGHTLOG2(lp0);
        if (diff > 3) {
            diff = 3;
        } else if (diff < -3) {
            diff = -3;
        }
        if (diff >= 0) {
            TEX_SSCALE(tex) = 1.0f / 256.0f;
            TEX_TSCALE(tex) = (__GLfloat)(1 << diff) * (1.0f / 256.0f);
        } else {
            TEX_TSCALE(tex) = 1.0f / 256.0f;
            TEX_SSCALE(tex) = (__GLfloat)(1 << -diff) * (1.0f / 256.0f);
        }
    }

    return lp->buffer;
}

/* 690c3c40 -- __glTextureCreateProxyLevel (s_texmgr.c, 2D: the depth is not checked) */
GLint __fastcall __glTextureCreateProxyLevel(__GLcontext *gc, __GLtextureICD *tex,
                              GLint lod, GLint components,
                              GLsizei w, GLsizei h, GLsizei d,
                              GLint border, GLint dim)
{
    __GLmipMapLevelICD *lp = tex->level[lod];
    const __GLtextureFormatICD *texFormat;
    GLenum baseFormat;
    GLint baseWidth, baseHeight;

    texFormat = (const __GLtextureFormatICD *)
        (*g_device->chooseTexFormat)(gc, components, &baseFormat);
    baseWidth = (w-border*2) * (1 << lod);
    baseHeight = (h-border*2) * (1 << lod);

    if (baseWidth > gc->constants.maxTextureSize ||
        baseHeight > gc->constants.maxTextureSize) {
        /* Proxy allocation failed */
        lp->width = 0;
        lp->height = 0;
        LP_DEPTH(lp) = 0;
        lp->border = 0;
        LP_TEXFORMAT(lp) = 0;
        lp->requestedFormat = 0;
        lp->baseFormat = 0;
        LP_INTERNALFORMAT(lp) = 0;
        lp->redSize = 0;
        lp->greenSize = 0;
        lp->blueSize = 0;
        lp->alphaSize = 0;
        lp->luminanceSize = 0;
        lp->intensitySize = 0;
    } else {
        /* Proxy allocation succeeded */
        lp->width = w;
        lp->height = h;
        LP_DEPTH(lp) = d;
        lp->border = border;
        LP_TEXFORMAT(lp) = texFormat;
        lp->requestedFormat = (GLenum) components;
        lp->baseFormat = baseFormat;
        LP_INTERNALFORMAT(lp) = texFormat->internalFormat;
        lp->redSize = texFormat->redSize;
        lp->greenSize = texFormat->greenSize;
        lp->blueSize = texFormat->blueSize;
        lp->alphaSize = texFormat->alphaSize;
        lp->luminanceSize = texFormat->luminanceSize;
        lp->intensitySize = texFormat->intensitySize;
    }
    lp->extract = (__GLproc) __glNop6;
    return 0;
}

/* 690a0130 -- s_tex.c __glBindTexture (ICD: the active unit; the state's per-target parameter copy
** is saved into the old object and loaded from the new one; the s/t scale of the old binding goes
** into gc->constants) */
void __fastcall __glBindTexture(__GLcontext *gc, GLint targetIndex, GLuint texture)
{
    __GLtextureICD *tex;
    __GLtexParamICD *param;
    __GLtextureICD *boundTexture;

    /*
    ** Retrieve the texture object from the namesArray structure.
    */
    if (0 == texture) {
	tex = (__GLtextureICD *)gc->texture.defaultTexture[targetIndex];
    } else {
	tex = (__GLtextureICD *)
		__glNamesLockData(gc, gc->texture.namesArray, texture);
    }

    /*
    ** Is this the first time this name has been bound?
    ** If so, create a new texture object and initialize it.
    */
    if (NULL == tex) {
	tex = (__GLtextureICD *)(*(__GLcreateTextureProc2)gc->createTexture)(gc, texture, targetIndex);
	__glNamesNewData(gc, gc->texture.namesArray, texture, tex);
	/*
	** Shortcut way to lock without doing another lookup.
	*/
	tex->refCount++;
    } else {
	if (tex->targetIndex != targetIndex) {
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
    }

    boundTexture = BOUNDTEX(gc, targetIndex);
    gc->constants.texSScale = boundTexture->sScale;
    gc->constants.texTScale = boundTexture->tScale;

    /*
    ** Release texture that is being unbound.
    */
    param = PARAM(gc, targetIndex);
    boundTexture = BOUNDTEX(gc, targetIndex);
    boundTexture->params = param->params;
    boundTexture->name = param->name;
    boundTexture->priority = param->priority;
    if (boundTexture->name != 0) {
	__glNamesUnlockData(gc, boundTexture);
    }

    /*
    ** Install the new texture into the correct target in this context.
    */
    BOUNDTEX(gc, targetIndex) = tex;
    param->params = tex->params;
    param->name = tex->name;
    param->priority = tex->priority;
}

/************************************************************************/
/* glTexEnv with GL_EXT_texture_env_combine: the ICD's 0x54-byte env record per unit and target */
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
/* s_texel.c mipmap filters: the ICD's texture keeps the per-level procs at +0xac
** (linear = linear, [3] = nearest: (tex, level, s, t, result)) and picks the two levels
** around lod from (GLint) lod + 1 down */
typedef void (__fastcall *__GLtexFilterProcICD)(__GLtextureICD *tex, __GLmipMapLevelICD *lp,
                                                __GLfloat s, __GLfloat t, __GLtexel *result);
typedef void (__fastcall *__GLtexMagMinProc)(__GLtextureICD *tex, __GLfloat lod, __GLfloat s, __GLfloat t, __GLtexel *texel);
/* s_texfmt.c texture formats (gl_texextract.c) */
extern const __GLtextureFormatSI __glTexFormatLuminance8, __glTexFormatLuminanceAlpha8, __glTexFormatRGB8,
                                 __glTexFormatRGBA8, __glTexFormatAlpha8, __glTexFormatIntensity8,
                                 __glTexFormatColorIndex8, __glTexFormatColorIndex16;
extern void __stdcall __glim_TexGenfv(GLenum coord, GLenum pname, const GLfloat pv[]);
extern void __stdcall __glim_TexGendv(GLenum coord, GLenum pname, const GLdouble pv[]);
extern void __stdcall __glim_TexGeniv(GLenum coord, GLenum pname, const GLint pv[]);
extern void __stdcall __glim_TexParameterfv(GLenum target, GLenum pname, const GLfloat pv[]);
extern void __stdcall __glim_TexParameteriv(GLenum target, GLenum pname, const GLint pv[]);

/* 6909cd00: __glim_TexGenf */
void __stdcall __glim_TexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    if (pname != GL_TEXTURE_GEN_MODE) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    __glim_TexGenfv(coord, GL_TEXTURE_GEN_MODE, &param);
}

/* 6909cef0: __glim_TexGend */
void __stdcall __glim_TexGend(GLenum coord, GLenum pname, GLdouble param)
{
    if (pname != GL_TEXTURE_GEN_MODE) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    __glim_TexGendv(coord, GL_TEXTURE_GEN_MODE, &param);
}

/* 6909d0e0: __glim_TexGeni */
void __stdcall __glim_TexGeni(GLenum coord, GLenum pname, GLint param)
{
    if (pname != GL_TEXTURE_GEN_MODE) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    __glim_TexGeniv(coord, GL_TEXTURE_GEN_MODE, &param);
}

/* 6909d3b0: __glim_TexParameterf */
void __stdcall __glim_TexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_PRIORITY:
        __glim_TexParameterfv(target, pname, &param);
        return;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 6909d600: __glim_TexParameteri */
void __stdcall __glim_TexParameteri(GLenum target, GLenum pname, GLint param)
{
    switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
      case GL_TEXTURE_MIN_FILTER:
      case GL_TEXTURE_WRAP_S:
      case GL_TEXTURE_WRAP_T:
      case GL_TEXTURE_PRIORITY:
        __glim_TexParameteriv(target, pname, &param);
        return;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 6909d680: __glim_TexEnvfv (s_tex.c); GL_COMBINE_EXT is accepted here only, not by TexEnviv */
void __stdcall __glim_TexEnvfv(GLenum target, GLenum pname, const GLfloat pv[])
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
    __GL_DELAY_VALIDATE(gc);
}

/* 6909dc60: __glim_TexEnviv (s_tex.c) */
void __stdcall __glim_TexEnviv(GLenum target, GLenum pname, const GLint pv[])
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
	tes->alphaScale = (__GLfloat) pv[0];
	break;
      case GL_TEXTURE_ENV_COLOR:
	__glClampAndScaleColori(gc, &tes->color, pv);
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
	tes->rgbScale = (__GLfloat) pv[0];
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
    __GL_DELAY_VALIDATE(gc);
}

/* 6909e230: __glIsTextureConsistent (s_tex.c), per texture unit; the ICD tests width and height only
** for the end of the mipmap chain and accepts colour-index textures in colour-index mode */
GLboolean __fastcall __glIsTextureConsistent(__GLcontext *gc, __GLtextureICD *tex, GLint unit)
{
    GLint i, width, height, depth;
    GLint maxLevel;
    GLint border;
    GLenum baseFormat;
    GLint requestedFormat;
    GLboolean rgb;

    if ((tex->level[0]->width == 0) ||
        (tex->level[0]->height == 0) ||
        (tex->level[0]->depth == 0)) {
        return GL_FALSE;
    }

    border   = tex->level[0]->border;
    width    = tex->level[0]->width - border*2;
    height   = tex->level[0]->height - border*2;
    depth    = tex->level[0]->depth - border*2;
    maxLevel = gc->constants.maxMipMapLevel;
    rgb = gc->modes.rgbMode;

    baseFormat = tex->level[0]->baseFormat;
    if (rgb) {
        if (baseFormat == GL_COLOR_INDEX) {
            baseFormat = tex->CT.baseFormat;
            if (baseFormat == 0 || baseFormat == GL_COLOR_INDEX) {
                return GL_FALSE;
            }
        }
    } else if (baseFormat != GL_COLOR_INDEX) {
        return GL_FALSE;
    }

    requestedFormat = tex->level[0]->requestedFormat;

    switch (gc->state.texture.unit[unit].env->mode) {
      case GL_DECAL:
        if (!(rgb && (baseFormat == GL_RGB || baseFormat == GL_RGBA))) {
            return GL_FALSE;
        }
        break;
      case GL_BLEND:
      case GL_MODULATE:
        if (!rgb) {
            return GL_FALSE;
        }
        break;
      case GL_ADD:
        if (rgb) {
            return GL_FALSE;
        }
      default:
        break;
    }

    switch (tex->params.minFilter) {
      case GL_NEAREST:
      case GL_LINEAR:
        return GL_TRUE;
      default:
        break;
    }

    i = 0;
    while (++i < maxLevel) {
        if (width == 1 && height == 1) break;
        width >>= 1;
        if (width == 0) width = 1;
        height >>= 1;
        if (height == 0) height = 1;
        depth >>= 1;
        if (depth == 0) depth = 1;

        if (tex->level[i]->border != border ||
            tex->level[i]->requestedFormat != requestedFormat ||
            tex->level[i]->width != width + border*2 ||
            tex->level[i]->height != height + border*2 ||
            tex->level[i]->depth != depth + border*2) {
            return GL_FALSE;
        }
    }

    return GL_TRUE;
}

/************************************************************************/

/* 6909ee40: __glCalcMixedTexture (s_tex.c) for every TMU unit in one call; didSphereGen is set once
** for the whole call (as in the original) */
void __fastcall __glCalcMixedTexture(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLcoord sphereCoord, gen, *c;
    GLboolean didSphereGen = GL_FALSE;
    __GLmatrixICD *m;
    GLint unit;
    GLuint enables;

    for (unit = 0; unit < gc->fx.numTMUs; unit++) {
    enables = gc->state.enables.texture[unit];
    /* Generate/copy s coordinate */
    if (enables & __GL_TEXTURE_GEN_S_ENABLE) {
	switch (gc->state.texture.unit[unit].s.mode) {
	  case GL_EYE_LINEAR:
	    c = &gc->state.texture.unit[unit].s.eyePlaneEquation;
	    gen.x = c->x * vx->eye.x + c->y * vx->eye.y
		+ c->z * vx->eye.z + c->w * vx->eye.w;
	    break;
	  case GL_OBJECT_LINEAR:
	    c = &gc->state.texture.unit[unit].s.objectPlaneEquation;
	    gen.x = c->x * vx->obj.x + c->y * vx->obj.y
		+ c->z * vx->obj.z + c->w * vx->obj.w;
	    break;
	  case GL_SPHERE_MAP:
	    SphereGen(gc, vx, &sphereCoord);
	    gen.x = sphereCoord.x;
	    didSphereGen = GL_TRUE;
	    break;
	}
    } else {
	gen.x = vx->texture[unit].x;
    }

    /* Generate/copy t coordinate */
    if (enables & __GL_TEXTURE_GEN_T_ENABLE) {
	switch (gc->state.texture.unit[unit].t.mode) {
	  case GL_EYE_LINEAR:
	    c = &gc->state.texture.unit[unit].t.eyePlaneEquation;
	    gen.y = c->x * vx->eye.x + c->y * vx->eye.y
		+ c->z * vx->eye.z + c->w * vx->eye.w;
	    break;
	  case GL_OBJECT_LINEAR:
	    c = &gc->state.texture.unit[unit].t.objectPlaneEquation;
	    gen.y = c->x * vx->obj.x + c->y * vx->obj.y
		+ c->z * vx->obj.z + c->w * vx->obj.w;
	    break;
	  case GL_SPHERE_MAP:
	    if (!didSphereGen) {
		SphereGen(gc, vx, &sphereCoord);
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
	    gen.z = c->x * vx->eye.x + c->y * vx->eye.y
		+ c->z * vx->eye.z + c->w * vx->eye.w;
	    break;
	  case GL_OBJECT_LINEAR:
	    c = &gc->state.texture.unit[unit].r.objectPlaneEquation;
	    gen.z = c->x * vx->obj.x + c->y * vx->obj.y
		+ c->z * vx->obj.z + c->w * vx->obj.w;
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
	    gen.w = c->x * vx->eye.x + c->y * vx->eye.y
		+ c->z * vx->eye.z + c->w * vx->eye.w;
	    break;
	  case GL_OBJECT_LINEAR:
	    c = &gc->state.texture.unit[unit].q.objectPlaneEquation;
	    gen.w = c->x * vx->obj.x + c->y * vx->obj.y
		+ c->z * vx->obj.z + c->w * vx->obj.w;
	    break;
	}
    } else {
	gen.w = vx->texture[unit].w;
    }

    /* Finally, apply texture matrix */
    m = &gc->transform.texture[unit]->matrix;
    ((void (__fastcall *)(__GLcoord *, const __GLfloat *, __GLmatrixICD *))m->xf4)
	(&vx->texture[unit], &gen.x, m);
    }
}

/************************************************************************/

/* 6909ff30: __glim_DeleteTextures (s_tex.c): bound textures of every TMU unit, and the unit's
** per-target parameter copy is reloaded from the default texture */
void __stdcall __glim_DeleteTextures(GLsizei n, const GLuint *textures)
{
    GLuint start, rangeVal;
    __GLnamesArray *array;
    int i, j, k;
    GLint numTargets;
    __GLtextureICD **ptex;

    __GL_SETUP_NOT_IN_BEGIN();
    if (n < 0) {
	__glSetError(GL_INVALID_VALUE);
    }
    if (n == 0) {
	return;
    }
    __GL_API_STATE();

    array = (__GLnamesArray *)gc->texture.namesArray;
    numTargets = gc->constants.numberOfTextures;

    start = rangeVal = textures[0];
    for (i=0; i < n; i++, rangeVal++) {
	if (0 == textures[i]) {		/* skip default textures */
	    /* delete up to this one */
	    __glNamesDeleteRange(gc, array, start, rangeVal-start);

	    /* skip over this one by setting start to the next one */
	    start = textures[i+1];
	    rangeVal = start-1;		/* because it gets incremented later */
	    continue;
	}
	for (j=0; j < gc->fx.numTMUs; j++) {
	    ptex = (__GLtextureICD **)gc->texture.boundTexture[j];
	    for (k=0; k < numTargets; k++, ptex++) {
		__GLtextureICD *tex;

		/* Is the texture currently bound? */
		if ((*ptex)->name == textures[i]) {
		    __GLtexParamICD *tp = &gc->state.texture.unit[j].param[k];

		    /* if we don't unlock it, it won't get deleted */
		    __glNamesUnlockData(gc, *ptex);

		    /* bind the default texture to this target */
		    tex = (__GLtextureICD *)(gc->texture.boundTexture[j][k] =
				gc->texture.defaultTexture[k]);
		    *ptex = tex;
		    tp->name = tex->name;
		    *(GLuint *)&tp->priority = *(GLuint *)&tex->priority;
		    *(__GLtexObjParamICD *)tp = tex->params;

		    /* Need to reset the current texture and such. */
		    __GL_DELAY_VALIDATE(gc);
		    break;
		}
	    }
	}
	if (textures[i] != rangeVal) {
	    /* delete up to this one */
	    __glNamesDeleteRange(gc, array, start, rangeVal-start);
	    start = rangeVal = textures[i];
	}
    }
    __glNamesDeleteRange(gc, array, start, rangeVal-start);
}

/* 690a4110: __glNMLFilter */
void __fastcall __glNMLFilter(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod,
                             __GLfloat s, __GLfloat t)
{
    __GLmipMapLevelICD *lp;
    GLint level, maxLevel;
    __GLtexel td, td1;
    __GLfloat f, omf;

    maxLevel = tex->maxLevel;
    level = (GLint)lod + 1;
    if (level > maxLevel || level < 0) {
        lp = tex->level[maxLevel];
        (*(__GLtexFilterProcICD)tex->nearest)(tex, lp, s * lp->width2f, t * lp->height2f, result);
    } else {
        lp = tex->level[level];
        (*(__GLtexFilterProcICD)tex->nearest)(tex, lp, s * lp->width2f, t * lp->height2f, &td);
        lp = tex->level[level-1];
        (*(__GLtexFilterProcICD)tex->nearest)(tex, lp, s * lp->width2f, t * lp->height2f, &td1);

        f = lod - __GL_FLOORF(lod);
        omf = __GL_ONE_F - f;
        switch (tex->baseFormat) {
          case GL_LUMINANCE_ALPHA:
            result->a = omf * td1.a + f * td.a;
            /* FALLTHROUGH */
          case GL_LUMINANCE:
            result->r = omf * td1.r + f * td.r;
            break;
          case GL_RGBA:
            result->a = omf * td1.a + f * td.a;
            /* FALLTHROUGH */
          case GL_RGB:
            result->r = omf * td1.r + f * td.r;
            result->g = omf * td1.g + f * td.g;
            result->b = omf * td1.b + f * td.b;
            break;
          case GL_ALPHA:
            result->a = omf * td1.a + f * td.a;
            break;
          case GL_INTENSITY:
            result->r = omf * td1.r + f * td.r;
            break;
        }
    }
}

/* 690a4400: __glLMLFilter */
void __fastcall __glLMLFilter(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod,
                             __GLfloat s, __GLfloat t)
{
    __GLmipMapLevelICD *lp;
    GLint level, maxLevel;
    __GLtexel td, td1;
    __GLfloat f, omf;

    maxLevel = tex->maxLevel;
    level = (GLint)lod + 1;
    if (level > maxLevel || level < 0) {
        lp = tex->level[maxLevel];
        (*(__GLtexFilterProcICD)tex->linear)(tex, lp, s * lp->width2f, t * lp->height2f, result);
    } else {
        lp = tex->level[level];
        (*(__GLtexFilterProcICD)tex->linear)(tex, lp, s * lp->width2f, t * lp->height2f, &td);
        lp = tex->level[level-1];
        (*(__GLtexFilterProcICD)tex->linear)(tex, lp, s * lp->width2f, t * lp->height2f, &td1);

        f = lod - __GL_FLOORF(lod);
        omf = __GL_ONE_F - f;
        switch (tex->baseFormat) {
          case GL_LUMINANCE_ALPHA:
            result->a = omf * td1.a + f * td.a;
            /* FALLTHROUGH */
          case GL_LUMINANCE:
            result->r = omf * td1.r + f * td.r;
            break;
          case GL_RGBA:
            result->a = omf * td1.a + f * td.a;
            /* FALLTHROUGH */
          case GL_RGB:
            result->r = omf * td1.r + f * td.r;
            result->g = omf * td1.g + f * td.g;
            result->b = omf * td1.b + f * td.b;
            break;
          case GL_ALPHA:
            result->a = omf * td1.a + f * td.a;
            break;
          case GL_INTENSITY:
            result->r = omf * td1.r + f * td.r;
            break;
        }
    }
}

/* 690a5070 -- __glTextureFragment: the ICD passes lod 0 to minify too */
void __fastcall __glTextureFragment(__GLcontext *gc, __GLcolor *color, __GLfloat s, __GLfloat t, __GLfloat rho)
{
    __GLtextureICD *tex = __glCurrentTexture(gc, 0);
    __GLtexel texel;

    if (rho <= tex->c) {
	(*(__GLtexMagMinProc)tex->magnify)(tex, __GL_ZERO_F, s, t, &texel);
    } else {
	(*(__GLtexMagMinProc)tex->minify)(tex, __GL_ZERO_F, s, t, &texel);
    }

    /* Now apply texture environment to get final color */
    (*(__GLtexEnvProc)tex->env)(gc, color, &texel);
}

/* 690a50d0 -- __glMipMapFragment (the 3dfx ancestor's log2 approximation, verbatim) */
void __fastcall __glMipMapFragment(__GLcontext *gc, __GLcolor *color, __GLfloat s, __GLfloat t, __GLfloat rho)
{
    __GLtextureICD *tex = __glCurrentTexture(gc, 0);
    __GLtexel texel;

    /* In the spec c is given in terms of lambda.
    ** Here c is compared to rho (really rho^2) and adjusted accordingly.
    */
    if (rho <= tex->c) {
	/* NOTE: rho is ignored by magnify proc */
	(*(__GLtexMagMinProc)tex->magnify)(tex, rho, s, t, &texel);
    } else {
	if (rho) {
	    __GLfloat twotolev;
	    GLuint irho, lev;

	    /* this is an approximation of log base 2 */
	    irho = rho;
	    lev = 0;
	    while( irho >>= 1 ) lev++;
	    twotolev = 1<<lev;
	    rho = (lev + ( (rho-twotolev) / twotolev ) ) * 0.5;
	} else {
	    rho = __GL_ZERO_F;
	}
	(*(__GLtexMagMinProc)tex->minify)(tex, rho, s, t, &texel);
    }

    /* Now apply texture environment to get final color */
    (*(__GLtexEnvProc)tex->env)(gc, color, &texel);
}

/* 690a51b0: s_texfmt.c __glLookupTextureFormat with the ICD's compressed formats (ecx unused) */
const void * __fastcall __glLookupTextureFormat(GLuint ecx_, const GLenum components, GLenum *baseFormat)
{
    switch (components) {
    case 1:
    case GL_LUMINANCE:
    case GL_LUMINANCE4:
    case GL_LUMINANCE8:
    case GL_LUMINANCE12:
    case GL_LUMINANCE16:
    case 0x84ea:    /* GL_COMPRESSED_LUMINANCE_ARB */
        *baseFormat = GL_LUMINANCE;
        return &__glTexFormatLuminance8;
    case 2:
    case GL_LUMINANCE_ALPHA:
    case GL_LUMINANCE4_ALPHA4:
    case GL_LUMINANCE6_ALPHA2:
    case GL_LUMINANCE8_ALPHA8:
    case GL_LUMINANCE12_ALPHA4:
    case GL_LUMINANCE12_ALPHA12:
    case GL_LUMINANCE16_ALPHA16:
    case 0x84eb:    /* GL_COMPRESSED_LUMINANCE_ALPHA_ARB */
        *baseFormat = GL_LUMINANCE_ALPHA;
        return &__glTexFormatLuminanceAlpha8;
    case 3:
    case GL_RGB:
    case GL_R3_G3_B2:
    case GL_RGB4:
    case GL_RGB5:
    case GL_RGB8:
    case GL_RGB10:
    case GL_RGB12:
    case GL_RGB16:
    case 0x86b0:    /* GL_COMPRESSED_RGB_FXT1_3DFX */
    case GL_RGB_S3TC:
    case GL_RGB4_S3TC:
        *baseFormat = GL_RGB;
        return &__glTexFormatRGB8;
    case 4:
    case GL_RGBA:
    case GL_RGBA2:
    case GL_RGBA4:
    case GL_RGBA8:
    case GL_RGBA12:
    case GL_RGBA16:
    case GL_RGB5_A1:
    case GL_RGB10_A2:
    case GL_RGBA_S3TC:
    case GL_RGBA4_S3TC:
    case 0x86b1:    /* GL_COMPRESSED_RGBA_FXT1_3DFX */
        *baseFormat = GL_RGBA;
        return &__glTexFormatRGBA8;
    case GL_ALPHA:
    case GL_ALPHA4:
    case GL_ALPHA8:
    case GL_ALPHA12:
    case GL_ALPHA16:
    case 0x84e9:    /* GL_COMPRESSED_ALPHA_ARB */
        *baseFormat = GL_ALPHA;
        return &__glTexFormatAlpha8;
    case GL_INTENSITY:
    case GL_INTENSITY4:
    case GL_INTENSITY8:
    case GL_INTENSITY12:
    case GL_INTENSITY16:
    case 0x84ec:    /* GL_COMPRESSED_INTENSITY_ARB */
        *baseFormat = GL_INTENSITY;
        return &__glTexFormatIntensity8;
    case GL_COLOR_INDEX1_EXT:
    case GL_COLOR_INDEX2_EXT:
    case GL_COLOR_INDEX4_EXT:
    case GL_COLOR_INDEX8_EXT:
        *baseFormat = GL_COLOR_INDEX;
        return &__glTexFormatColorIndex8;
    case GL_COLOR_INDEX12_EXT:
    case GL_COLOR_INDEX16_EXT:
        *baseFormat = GL_COLOR_INDEX;
        return &__glTexFormatColorIndex16;
    default:
        return 0;
    }
}

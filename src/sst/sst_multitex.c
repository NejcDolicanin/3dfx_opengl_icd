/*
** sst_multitex.c -- 3dfx back end, original VA range 69064000..69068400: the og call helper, the mode-flag / needs
** getters, glActiveTextureARB / glClientActiveTextureARB and the glMultiTexCoord*ARB entries of the hardware
** path, the colour-table argument check and span setup, and the Glide write-mask clear (then the Glide
** import thunks).
** Then the remaining glMultiTexCoord*ARB entries, in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "og.h"
#include <glide.h>
#include "glsi.h"
#include "icdglob.h"
#include "icdprotos.h"
#include "glshade.h"
#include "si_names.h"

/* 690fc4c8 .. 690fc4d8 -- the shader state the last validation saw (the hardware state-change test) */
GLuint g_savedModeFlags, g_savedNeeds, g_savedFaceNeeds0, g_savedFaceNeeds1, g_savedMaterialNeeds;

#define GL_SHARED_TEXTURE_PALETTE_EXT_W     0x81fb
#define GL_ABGR_EXT_W                       0x8000
#define GL_BGR_EXT_W                        0x80e0
#define GL_BGRA_EXT_W                       0x80e1

/* the current texture coordinates of unit target & 1 (GL_TEXTURE0_ARB / GL_TEXTURE1_ARB) */
#define TC(target)      (gc->state.current.texture[(target) & 1])

/* ---- forward declarations ---- */
void __fastcall hwSaveShadingState(__GLcontext *gc);
void __stdcall __glsstim_ActiveTextureARB(GLenum texture);
void __stdcall __glsstim_ClientActiveTextureARB(GLenum texture);
void __stdcall __glsstim_MultiTexCoord1dvARB(GLenum target, const GLdouble *v);
void __stdcall __glsstim_MultiTexCoord1fvARB(GLenum target, const GLfloat *v);
void __stdcall __glsstim_MultiTexCoord1iARB(GLenum target, GLint s);
void __stdcall __glsstim_MultiTexCoord1ivARB(GLenum target, const GLint *v);
void __stdcall __glsstim_MultiTexCoord1sARB(GLenum target, GLshort s);
void __stdcall __glsstim_MultiTexCoord1svARB(GLenum target, const GLshort *v);
void __stdcall __glsstim_MultiTexCoord2fvARB(GLenum target, const GLfloat *v);
void __stdcall __glsstim_MultiTexCoord2iARB(GLenum target, GLint s, GLint t);
void __stdcall __glsstim_MultiTexCoord2ivARB(GLenum target, const GLint *v);
void __stdcall __glsstim_MultiTexCoord3fvARB(GLenum target, const GLfloat *v);
void __stdcall __glsstim_MultiTexCoord4fvARB(GLenum target, const GLfloat *v);
void __stdcall __glsstim_MultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q);
void __stdcall __glsstim_MultiTexCoord4ivARB(GLenum target, const GLint *v);
void __stdcall __glsstim_MultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
void __stdcall __glsstim_MultiTexCoord4svARB(GLenum target, const GLshort *v);
GLenum __fastcall checkColorTableEXTArgs(__GLcontext *gc, GLenum target, GLenum internalFormat, GLsizei width,
                               GLenum format, GLenum type);
void __fastcall __glSSTInitColorTableStore(__GLcolorTableICD *ct, __GLpixelSpanInfoICD *spanInfo);

/* 690652f0 -- og_callRel32Tri: og_callRel32: emit `call routine` (rel32) at pc: a label already placed at the
** routine */
GLubyte *__fastcall og_callRel32Tri(__GLcontext *gc, GLubyte *pc, GLuint routine)
{
    OgLabel *target;

    target = og_newFixup(gc);
    target->addr = (GLubyte *)routine;
    return og_encode(gc, pc, og_call(gc, target));
}

/* 690657c0 -- hwGetModeFlags: polygon.shader.getModeFlags: recompute the mode flags, return them */
GLuint __fastcall hwGetModeFlags(__GLcontext *gc)
{
    computeModeFlags(gc);
    return gc->polygon.shader.modeFlags;
}

/* 690657d0 -- hwGetNeeds: vertex.getNeeds: recompute the mode flags and the needs, return needs */
GLuint __fastcall hwGetNeeds(__GLcontext *gc)
{
    computeModeFlags(gc);
    computeVertexNeeds(gc);
    return gc->vertex.needs;
}

/* 690657f0 -- hwGetFaceNeeds0: vertex.getFaceNeeds0: as 690657d0, returns faceNeeds[0] */
GLuint __fastcall hwGetFaceNeeds0(__GLcontext *gc)
{
    computeModeFlags(gc);
    computeVertexNeeds(gc);
    return gc->vertex.faceNeeds[0];
}

/* 69065810 -- hwGetFaceNeeds1: vertex.getFaceNeeds1: returns faceNeeds[1] */
GLuint __fastcall hwGetFaceNeeds1(__GLcontext *gc)
{
    computeModeFlags(gc);
    computeVertexNeeds(gc);
    return gc->vertex.faceNeeds[1];
}

/* 69065830 -- hwGetMaterialNeeds: vertex.getMaterialNeeds: returns materialNeeds */
GLuint __fastcall hwGetMaterialNeeds(__GLcontext *gc)
{
    computeModeFlags(gc);
    computeVertexNeeds(gc);
    return gc->vertex.materialNeeds;
}

/* the shading state hwSaveShadingState saves and hwShadingStateUnchanged (sst_shadestate.c) compares */

/* 69065850 (size 58) -- hwSaveShadingState: save the mode flags, needs, face needs and material needs for 69065890 */
void __fastcall hwSaveShadingState(__GLcontext *gc)
{
    g_savedModeFlags = gc->polygon.shader.modeFlags;
    g_savedNeeds = gc->vertex.needs;
    g_savedFaceNeeds0 = gc->vertex.faceNeeds[0];
    g_savedFaceNeeds1 = gc->vertex.faceNeeds[1];
    g_savedMaterialNeeds = gc->vertex.materialNeeds;
}

/* 69066660 -- __glsstim_ActiveTextureARB: glActiveTextureARB, hardware path: the unit is texture & 1 (no enum
** check); glTexCoord* writes through state.current.activeTexture */
void __stdcall __glsstim_ActiveTextureARB(GLenum texture)
{
    __GL_SETUP_NOT_IN_BEGIN();

    gc->fx.activeTexture = texture & 1;
    gc->state.current.activeTexture = &gc->state.current.texture[texture & 1];
}

/* 690666c0 -- __glsstim_ClientActiveTextureARB: glClientActiveTextureARB, hardware path */
void __stdcall __glsstim_ClientActiveTextureARB(GLenum texture)
{
    __GL_SETUP_NOT_IN_BEGIN();

    gc->fx.clientActiveTexture = texture & 1;
}

/* 69066760 -- __glsstim_MultiTexCoord1dvARB: glMultiTexCoord1dvARB */
void __stdcall __glsstim_MultiTexCoord1dvARB(GLenum target, const GLdouble *v)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)v[0];
    TC(target).y = (__GLfloat)0.0f;
    TC(target).z = (__GLfloat)0.0f;
    TC(target).w = (__GLfloat)1.0f;
}

/* 69066800 -- __glsstim_MultiTexCoord1fvARB: glMultiTexCoord1fvARB */
void __stdcall __glsstim_MultiTexCoord1fvARB(GLenum target, const GLfloat *v)
{
    __GL_SETUP();

    TC(target).x = v[0];
    TC(target).y = (__GLfloat)0.0f;
    TC(target).z = (__GLfloat)0.0f;
    TC(target).w = (__GLfloat)1.0f;
}

/* 69066850 -- __glsstim_MultiTexCoord1iARB: glMultiTexCoord1iARB */
void __stdcall __glsstim_MultiTexCoord1iARB(GLenum target, GLint s)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)s;
    TC(target).y = (__GLfloat)0.0f;
    TC(target).z = (__GLfloat)0.0f;
    TC(target).w = (__GLfloat)1.0f;
}

/* 690668a0 -- __glsstim_MultiTexCoord1ivARB: glMultiTexCoord1ivARB */
void __stdcall __glsstim_MultiTexCoord1ivARB(GLenum target, const GLint *v)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)v[0];
    TC(target).y = (__GLfloat)0.0f;
    TC(target).z = (__GLfloat)0.0f;
    TC(target).w = (__GLfloat)1.0f;
}

/* 690668f0 -- __glsstim_MultiTexCoord1sARB: glMultiTexCoord1sARB */
void __stdcall __glsstim_MultiTexCoord1sARB(GLenum target, GLshort s)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)s;
    TC(target).y = (__GLfloat)0.0f;
    TC(target).z = (__GLfloat)0.0f;
    TC(target).w = (__GLfloat)1.0f;
}

/* 69066940 -- __glsstim_MultiTexCoord1svARB: glMultiTexCoord1svARB */
void __stdcall __glsstim_MultiTexCoord1svARB(GLenum target, const GLshort *v)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)v[0];
    TC(target).y = (__GLfloat)0.0f;
    TC(target).z = (__GLfloat)0.0f;
    TC(target).w = (__GLfloat)1.0f;
}

/* 69066a80 -- __glsstim_MultiTexCoord2fvARB: glMultiTexCoord2fvARB */
void __stdcall __glsstim_MultiTexCoord2fvARB(GLenum target, const GLfloat *v)
{
    __GL_SETUP();

    TC(target).x = v[0];
    TC(target).y = v[1];
    TC(target).z = (__GLfloat)0.0f;
    TC(target).w = (__GLfloat)1.0f;
}

/* 69066ad0 -- __glsstim_MultiTexCoord2iARB: glMultiTexCoord2iARB */
void __stdcall __glsstim_MultiTexCoord2iARB(GLenum target, GLint s, GLint t)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)s;
    TC(target).y = (__GLfloat)t;
    TC(target).z = (__GLfloat)0.0f;
    TC(target).w = (__GLfloat)1.0f;
}

/* 69066b20 -- __glsstim_MultiTexCoord2ivARB: glMultiTexCoord2ivARB */
void __stdcall __glsstim_MultiTexCoord2ivARB(GLenum target, const GLint *v)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)v[0];
    TC(target).y = (__GLfloat)v[1];
    TC(target).z = (__GLfloat)0.0f;
    TC(target).w = (__GLfloat)1.0f;
}

/* 69066d20 -- __glsstim_MultiTexCoord3fvARB: glMultiTexCoord3fvARB */
void __stdcall __glsstim_MultiTexCoord3fvARB(GLenum target, const GLfloat *v)
{
    __GL_SETUP();

    TC(target).x = v[0];
    TC(target).y = v[1];
    TC(target).z = v[2];
    TC(target).w = (__GLfloat)1.0f;
}

/* 69066fd0 -- __glsstim_MultiTexCoord4fvARB: glMultiTexCoord4fvARB */
void __stdcall __glsstim_MultiTexCoord4fvARB(GLenum target, const GLfloat *v)
{
    __GL_SETUP();

    TC(target).x = v[0];
    TC(target).y = v[1];
    TC(target).z = v[2];
    TC(target).w = v[3];
}

/* 69067020 -- __glsstim_MultiTexCoord4iARB: glMultiTexCoord4iARB */
void __stdcall __glsstim_MultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)s;
    TC(target).y = (__GLfloat)t;
    TC(target).z = (__GLfloat)r;
    TC(target).w = (__GLfloat)q;
}

/* 69067070 -- __glsstim_MultiTexCoord4ivARB: glMultiTexCoord4ivARB */
void __stdcall __glsstim_MultiTexCoord4ivARB(GLenum target, const GLint *v)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)v[0];
    TC(target).y = (__GLfloat)v[1];
    TC(target).z = (__GLfloat)v[2];
    TC(target).w = (__GLfloat)v[3];
}

/* 690670c0 -- __glsstim_MultiTexCoord4sARB: glMultiTexCoord4sARB */
void __stdcall __glsstim_MultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)s;
    TC(target).y = (__GLfloat)t;
    TC(target).z = (__GLfloat)r;
    TC(target).w = (__GLfloat)q;
}

/* 69067130 -- __glsstim_MultiTexCoord4svARB: glMultiTexCoord4svARB */
void __stdcall __glsstim_MultiTexCoord4svARB(GLenum target, const GLshort *v)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)v[0];
    TC(target).y = (__GLfloat)v[1];
    TC(target).z = (__GLfloat)v[2];
    TC(target).w = (__GLfloat)v[3];
}

/* 690671a0 -- checkColorTableEXTArgs: the argument check of glColorTableEXT (cf. s_ctable.c __glim_ColorTable): the
** GL error, 0 when the arguments are valid (targets GL_TEXTURE_1D / 2D and the shared palette) */
GLenum __fastcall checkColorTableEXTArgs(__GLcontext *gc, GLenum target, GLenum internalFormat, GLsizei width,
                               GLenum format, GLenum type)
{
    switch (target) {
      case GL_TEXTURE_1D:
      case GL_TEXTURE_2D:
      case GL_SHARED_TEXTURE_PALETTE_EXT_W:
        if ((GLint)internalFormat < 0 || width < 0) {
            return GL_INVALID_VALUE;
        }
        switch (format) {
          case GL_RED:
          case GL_GREEN:
          case GL_BLUE:
          case GL_ALPHA:
          case GL_RGB:
          case GL_RGBA:
          case GL_LUMINANCE:
          case GL_LUMINANCE_ALPHA:
          case GL_ABGR_EXT_W:
          case GL_BGR_EXT_W:
          case GL_BGRA_EXT_W:
            break;
          default:
            return GL_INVALID_ENUM;
        }
        switch (type) {
          case GL_BYTE:
          case GL_UNSIGNED_BYTE:
          case GL_SHORT:
          case GL_UNSIGNED_SHORT:
          case GL_INT:
          case GL_UNSIGNED_INT:
          case GL_FLOAT:
            return 0;
        }
        break;
    }
    return GL_INVALID_ENUM;
}

/* 69067700 -- __glSSTInitColorTableStore: __glInitColorTableStore (s_ctable.c): the span info stores into the
** colour table (no dstSkipImages / dstImageHeight in the ICD's span info) */
void __fastcall __glSSTInitColorTableStore(__GLcolorTableICD *ct, __GLpixelSpanInfoICD *spanInfo)
{
    spanInfo->dstImage = ct->table;
    spanInfo->dstSkipPixels = 0;
    spanInfo->dstSkipLines = 0;
    spanInfo->dstSwapBytes = GL_FALSE;
    spanInfo->dstLsbFirst = GL_TRUE;
    spanInfo->dstLineLength = ct->width;
    spanInfo->dstFormat = ct->baseFormat;
    spanInfo->dstType = ct->type;
    spanInfo->dstAlignment = __glBytesPerElement(ct->type);
}

/* 690682d0 -- hwMaskBufferWrites: switch off the Glide writes of the buffers not in mask (1 colour, 2 depth, 4
** stencil at 32 bpp) and mark their state records dirty, so the next flush restores them */
void __fastcall hwMaskBufferWrites(__GLcontext *gc, GLuint mask)
{
    if (!(mask & 1)) {
        if (gc->hwcx.has32bpp) {
            ((void (__stdcall *)(FxBool, FxBool, FxBool, FxBool))gc->hwcx.grColorMaskExt)(0, 0, 0, 0);
        } else {
            grColorMask(0, 0);
        }
        gc->hwcx.grDirty |= 1 << __GL_GR_COLOR_MASK;
    }
    if (!(mask & 2)) {
        grDepthMask(0);
        gc->hwcx.grDirty |= 1 << __GL_GR_DEPTH_MASK;
    }
    if (gc->hwcx.has32bpp && !(mask & 4)) {
        ((void (__stdcall *)(FxU8))gc->hwcx.grStencilMaskExt)(0);
        gc->hwcx.grDirty |= 1 << __GL_GR_STENCIL_MASK;
    }
}

#pragma warning(disable: 4273)
#define CURRENT_TC(target)  (gc->state.current.texture[(target) & 1])

/* 69066710 -- __glsstim_MultiTexCoord1dARB: glMultiTexCoord1dARB (unit = target & 1, no enum check) */
void __stdcall __glsstim_MultiTexCoord1dARB(GLenum target, GLdouble s)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)s;
    TC(target).y = (__GLfloat)0.0f;
    TC(target).z = (__GLfloat)0.0f;
    TC(target).w = (__GLfloat)1.0f;
}

/* 690667b0 -- __glsstim_MultiTexCoord1fARB */
void __stdcall __glsstim_MultiTexCoord1fARB(GLenum target, GLfloat s)
{
  __GL_SETUP();

  CURRENT_TC(target).x = s;
  CURRENT_TC(target).y = (__GLfloat)0.0f;
  CURRENT_TC(target).z = (__GLfloat)0.0f;
  CURRENT_TC(target).w = (__GLfloat)1.0f;
}

/* 69066990 -- __glsstim_MultiTexCoord2dARB: glMultiTexCoord2dARB */
void __stdcall __glsstim_MultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)s;
    TC(target).y = (__GLfloat)t;
    TC(target).z = (__GLfloat)0.0f;
    TC(target).w = (__GLfloat)1.0f;
}

/* ---- glMultiTexCoord*dvARB (optimised; the EBP frame comes from the inline-asm TEB read) ----------- */
/* 690669e0 (size 77) -- __glsstim_MultiTexCoord2dvARB: glMultiTexCoord2dvARB */
void __stdcall __glsstim_MultiTexCoord2dvARB(GLenum target, const GLdouble *v)
{
    __GLcontext *gc = __glCurrentGC();
    GLuint unit = target & 1;

    gc->state.current.texture[unit].x = (__GLfloat)v[0];
    gc->state.current.texture[unit].y = (__GLfloat)v[1];
    gc->state.current.texture[unit].z = 0.0f;
    gc->state.current.texture[unit].w = 1.0f;
}

/* 69066a30 -- __glsstim_MultiTexCoord2fARB */
void __stdcall __glsstim_MultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t)
{
  __GL_SETUP();

  CURRENT_TC(target).x = s;
  CURRENT_TC(target).y = t;
  CURRENT_TC(target).z = (__GLfloat)0.0f;
  CURRENT_TC(target).w = (__GLfloat)1.0f;
}

/* 69066b70 -- __glsstim_MultiTexCoord2sARB */
void __stdcall __glsstim_MultiTexCoord2sARB(GLenum target, GLshort s, GLshort t)
{
  __GL_SETUP();

  CURRENT_TC(target).x = (__GLfloat)s;
  CURRENT_TC(target).y = (__GLfloat)t;
  CURRENT_TC(target).z = (__GLfloat)0.0f;
  CURRENT_TC(target).w = (__GLfloat)1.0f;
}

/* 69066bd0 -- __glsstim_MultiTexCoord2svARB */
void __stdcall __glsstim_MultiTexCoord2svARB(GLenum target, const GLshort *v)
{
  __GL_SETUP();

  CURRENT_TC(target).x = (__GLfloat)v[0];
  CURRENT_TC(target).y = (__GLfloat)v[1];
  CURRENT_TC(target).z = (__GLfloat)0.0f;
  CURRENT_TC(target).w = (__GLfloat)1.0f;
}

/* 69066c30 -- __glsstim_MultiTexCoord3dARB */
void __stdcall __glsstim_MultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
  __GL_SETUP();

  CURRENT_TC(target).x = (__GLfloat)s;
  CURRENT_TC(target).y = (__GLfloat)t;
  CURRENT_TC(target).z = (__GLfloat)r;
  CURRENT_TC(target).w = (__GLfloat)1.0f;
}

/* 69066c80 (size 76) -- __glsstim_MultiTexCoord3dvARB: glMultiTexCoord3dvARB */
void __stdcall __glsstim_MultiTexCoord3dvARB(GLenum target, const GLdouble *v)
{
    __GL_SETUP();

    gc->state.current.texture[target & 1].x = (__GLfloat)v[0];
    gc->state.current.texture[target & 1].y = (__GLfloat)v[1];
    gc->state.current.texture[target & 1].z = (__GLfloat)v[2];
    gc->state.current.texture[target & 1].w = (__GLfloat)1.0f;
}

/* 69066cd0 -- __glsstim_MultiTexCoord3fARB */
void __stdcall __glsstim_MultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
  __GL_SETUP();

  CURRENT_TC(target).x = s;
  CURRENT_TC(target).y = t;
  CURRENT_TC(target).z = r;
  CURRENT_TC(target).w = (__GLfloat)1.0f;
}

/* 69066d70 -- __glsstim_MultiTexCoord3iARB */
void __stdcall __glsstim_MultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r)
{
  __GL_SETUP();

  CURRENT_TC(target).x = (__GLfloat)s;
  CURRENT_TC(target).y = (__GLfloat)t;
  CURRENT_TC(target).z = (__GLfloat)r;
  CURRENT_TC(target).w = (__GLfloat)1.0f;
}

/* 69066dc0 -- __glsstim_MultiTexCoord3ivARB */
void __stdcall __glsstim_MultiTexCoord3ivARB(GLenum target, const GLint *v)
{
  __GL_SETUP();

  CURRENT_TC(target).x = (__GLfloat)v[0];
  CURRENT_TC(target).y = (__GLfloat)v[1];
  CURRENT_TC(target).z = (__GLfloat)v[2];
  CURRENT_TC(target).w = (__GLfloat)1.0f;
}

/* 69066e10 -- __glsstim_MultiTexCoord3sARB */
void __stdcall __glsstim_MultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r)
{
  __GL_SETUP();

  CURRENT_TC(target).x = (__GLfloat)s;
  CURRENT_TC(target).y = (__GLfloat)t;
  CURRENT_TC(target).z = (__GLfloat)r;
  CURRENT_TC(target).w = (__GLfloat)1.0f;
}

/* 69066e70 -- __glsstim_MultiTexCoord3svARB */
void __stdcall __glsstim_MultiTexCoord3svARB(GLenum target, const GLshort *v)
{
  __GL_SETUP();

  CURRENT_TC(target).x = (__GLfloat)v[0];
  CURRENT_TC(target).y = (__GLfloat)v[1];
  CURRENT_TC(target).z = (__GLfloat)v[2];
  CURRENT_TC(target).w = (__GLfloat)1.0f;
}

/* 69066ee0 -- __glsstim_MultiTexCoord4dARB: glMultiTexCoord4dARB */
void __stdcall __glsstim_MultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    __GL_SETUP();

    TC(target).x = (__GLfloat)s;
    TC(target).y = (__GLfloat)t;
    TC(target).z = (__GLfloat)r;
    TC(target).w = (__GLfloat)q;
}

/* 69066f30 (size 75) -- __glsstim_MultiTexCoord4dvARB: glMultiTexCoord4dvARB */
void __stdcall __glsstim_MultiTexCoord4dvARB(GLenum target, const GLdouble *v)
{
    __GLcontext *gc = __glCurrentGC();
    GLuint unit = target & 1;

    gc->state.current.texture[unit].x = (__GLfloat)v[0];
    gc->state.current.texture[unit].y = (__GLfloat)v[1];
    gc->state.current.texture[unit].z = (__GLfloat)v[2];
    gc->state.current.texture[unit].w = (__GLfloat)v[3];
}

/* 69066f80 -- __glsstim_MultiTexCoord4fARB */
void __stdcall __glsstim_MultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
  __GL_SETUP();

  CURRENT_TC(target).x = s;
  CURRENT_TC(target).y = t;
  CURRENT_TC(target).z = r;
  CURRENT_TC(target).w = q;
}

/*
** sst_combine.c -- the texture environments -> Glide combine set-up of the hardware path (plain and extension
** combine, filters / wrap, the TMU source). The record loaders it calls are sststate.c's; they stay in another
** translation unit: compiled after their definitions, VC6 drops stores of __glSSTLoadCombineFunctionExt the
** original keeps.
*/
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "icdglob.h"
#include "icdprotos.h"
#include "si_names.h"
/* the colour / alpha combine of one texture environment (mode) and texture base format (fmt) */
#define SIMPLE_ENV(mode, fmt, env)                                                                            \
    switch (mode) {                                                                                           \
      case GL_BLEND:                                                                                          \
        switch (fmt) {                                                                                        \
          case GL_RGB: case GL_LUMINANCE: hwLoadColorAlphaCombine(gc, 7, 5, 0, 2, 1, 0, 0, 2); break;         \
          case GL_RGBA: case GL_LUMINANCE_ALPHA: hwLoadColorAlphaCombine(gc, 7, 5, 0, 2, 3, 1, 0, 1); break;  \
          case GL_ALPHA: hwLoadColorAlphaCombine(gc, 1, 0, 0, 2, 3, 1, 0, 1); break;                          \
          case GL_INTENSITY: hwLoadColorAlphaCombine(gc, 7, 5, 0, 2, 7, 4, 0, 2); break;                      \
        }                                                                                                     \
        grConstantColorValue(PACK_ARGB2((env)->color));                                                       \
        return;                                                                                               \
      case GL_ADD:                                                                                            \
        switch (fmt) {                                                                                        \
          case GL_RGB: case GL_LUMINANCE: hwLoadColorAlphaCombine(gc, 4, 8, 0, 1, 1, 0, 0, 2); return;        \
          case GL_RGBA: case GL_LUMINANCE_ALPHA: hwLoadColorAlphaCombine(gc, 4, 8, 0, 1, 3, 1, 0, 1); return; \
          case GL_ALPHA: hwLoadColorAlphaCombine(gc, 1, 0, 0, 2, 3, 1, 0, 1); return;                         \
          case GL_INTENSITY: hwLoadColorAlphaCombine(gc, 4, 8, 0, 1, 4, 8, 0, 1); return;                     \
        }                                                                                                     \
        return;                                                                                               \
      case GL_REPLACE:                                                                                        \
        switch (fmt) {                                                                                        \
          case GL_RGB: case GL_LUMINANCE: hwLoadColorAlphaCombine(gc, 3, 8, 1, 1, 1, 0, 0, 2); return;        \
          case GL_RGBA: case GL_LUMINANCE_ALPHA: case GL_INTENSITY:                                           \
            hwLoadColorAlphaCombine(gc, 3, 8, 1, 1, 3, 8, 1, 1); return;                                      \
          case GL_ALPHA: hwLoadColorAlphaCombine(gc, 1, 0, 0, 2, 3, 8, 1, 1); return;                         \
        }                                                                                                     \
        return;                                                                                               \
      case GL_DECAL:                                                                                          \
        switch (fmt) {                                                                                        \
          case GL_RGB: case GL_LUMINANCE: hwLoadColorAlphaCombine(gc, 3, 8, 1, 1, 1, 0, 0, 2); return;        \
          case GL_RGBA: case GL_LUMINANCE_ALPHA: case GL_INTENSITY:                                           \
            hwLoadColorAlphaCombine(gc, 7, 4, 0, 1, 1, 0, 0, 2); return;                                      \
          case GL_ALPHA: hwLoadColorAlphaCombine(gc, 1, 0, 0, 2, 3, 1, 1, 1); return;                         \
        }                                                                                                     \
        return;                                                                                               \
      case GL_MODULATE:                                                                                       \
        switch (fmt) {                                                                                        \
          case GL_RGB: case GL_LUMINANCE: hwLoadColorAlphaCombine(gc, 3, 1, 0, 1, 1, 0, 0, 2); return;        \
          case GL_RGBA: case GL_LUMINANCE_ALPHA: case GL_INTENSITY:                                           \
            hwLoadColorAlphaCombine(gc, 3, 1, 0, 1, 3, 1, 0, 1); return;                                      \
          case GL_ALPHA: hwLoadColorAlphaCombine(gc, 1, 0, 0, 2, 3, 1, 0, 1); return;                         \
        }                                                                                                     \
        return;                                                                                               \
    }
#define PACK_ARGB2(c)                                                       \
    ((((GLint)((c).r / gc->constants.redRescale) & 0xff |                  \
       (GLint)((c).a / gc->constants.alphaRescale) << 8) << 8 |            \
      (GLint)((c).g / gc->constants.greenRescale) & 0xff) << 8 |           \
     (GLint)((c).b / gc->constants.blueRescale) & 0xff)
/* EXT_texture_env_combine / Glide combine-extension values (g3ext.h) */
#define GL_COMBINE_EXT          0x8570
#define GL_ADD_SIGNED_EXT       0x8574
#define GL_INTERPOLATE_EXT      0x8575
#define GL_CONSTANT_EXT         0x8576
#define GL_PRIMARY_COLOR_EXT    0x8577
#define GL_PREVIOUS_EXT         0x8578
#define CMBX_ZERO                   0x00
#define CMBX_B                      0x04
#define CMBX_ITALPHA                0x08
#define CMBX_ITRGB                  0x09
#define CMBX_LOCAL_TEXTURE_ALPHA    0x0a
#define CMBX_LOCAL_TEXTURE_RGB      0x0b
#define CMBX_OTHER_TEXTURE_ALPHA    0x0d
#define CMBX_OTHER_TEXTURE_RGB      0x0e
#define CMBX_TMU_CALPHA             0x10
#define CMBX_TMU_CCOLOR             0x11
#define FMODE_X                     1
#define FMODE_ONE_MINUS_X           2
#define FMODE_NEGATIVE_X            3
#define FMODE_X_MINUS_HALF          4
/* combine argument from an RGB source/operand pair */
#define SRC_RGB(src, op, x)                                                 \
    if ((op) != GL_SRC_COLOR && (op) != GL_ONE_MINUS_SRC_COLOR) {           \
        switch (src) {                                                      \
          case GL_CONSTANT_EXT_W: x = CMBX_TMU_CALPHA; break;               \
          case GL_TEXTURE: x = CMBX_LOCAL_TEXTURE_ALPHA; break;             \
          case GL_PRIMARY_COLOR_EXT: x = CMBX_ITALPHA; break;           \
          case GL_PREVIOUS_EXT:                                         \
            if (i != 0 && enabled[i - 1]) x = CMBX_OTHER_TEXTURE_ALPHA;     \
            else x = CMBX_ITALPHA;                                          \
            break;                                                          \
        }                                                                   \
    } else {                                                                \
        switch (src) {                                                      \
          case GL_CONSTANT_EXT_W: x = CMBX_TMU_CCOLOR; break;               \
          case GL_TEXTURE: x = CMBX_LOCAL_TEXTURE_RGB; break;               \
          case GL_PRIMARY_COLOR_EXT: x = CMBX_ITRGB; break;             \
          case GL_PREVIOUS_EXT:                                         \
            if (i != 0 && enabled[i - 1]) x = CMBX_OTHER_TEXTURE_RGB;       \
            else x = CMBX_ITRGB;                                            \
            break;                                                          \
        }                                                                   \
    }
#define GL_CONSTANT_EXT_W GL_CONSTANT_EXT
#define SRC_ALPHA(src, x)                                                   \
    switch (src) {                                                          \
      case GL_CONSTANT_EXT_W: x = CMBX_TMU_CALPHA; break;                   \
      case GL_TEXTURE: x = CMBX_LOCAL_TEXTURE_ALPHA; break;                 \
      case GL_PREVIOUS_EXT:                                             \
        if (i != 0 && enabled[i - 1]) x = CMBX_OTHER_TEXTURE_ALPHA;         \
        else x = CMBX_ITALPHA;                                              \
        break;                                                              \
      case GL_PRIMARY_COLOR_EXT: x = CMBX_ITALPHA; break;               \
    }
/* function mode of an RGB operand */
#define MODE_RGB(op, m)                                                     \
    switch (op) {                                                           \
      case GL_SRC_COLOR: case GL_SRC_ALPHA: m = FMODE_X; break;             \
      case GL_ONE_MINUS_SRC_COLOR: case GL_ONE_MINUS_SRC_ALPHA: m = FMODE_ONE_MINUS_X; break; \
    }
#define MODE_ALPHA(op, m)                                                   \
    switch (op) {                                                           \
      case GL_SRC_ALPHA: m = FMODE_X; break;                                \
      case GL_ONE_MINUS_SRC_ALPHA: m = FMODE_ONE_MINUS_X; break;            \
    }
#define PACK_ARGB(c)                                                        \
    ((((GLint)((c).r / gc->constants.redRescale) & 0xff |                  \
       (GLint)((c).a / gc->constants.alphaRescale) << 8) << 8 |            \
      (GLint)((c).g / gc->constants.greenRescale) & 0xff) << 8 |           \
     (GLint)((c).b / gc->constants.blueRescale) & 0xff)
/************************************************************************/

/* set field i of Glide state record n, marking it dirty */
#define GRSET(n, i, v, bit)                                     \
    if (gc->hwcx.grState[n][i] != (v)) {                            \
        gc->hwcx.grState[n][i] = (v);                               \
        gc->hwcx.grDirty |= (bit);                                  \
    }
/* the texture parameters of one TMU: grState[f] = filter {mag, min}, [f+1] = clamp {s, t},
** [f+2] = mipmap {mode, lodBlend} (Glide: 0 point / none / wrap, 1 bilinear / nearest / clamp; mode 2 = nearest
** with LOD dithering, used for the *_MIPMAP_LINEAR filters when FX_GL_TRILINEAR_ENABLED is 1: the mip level is
** dithered per pixel instead of blended.  lodBlend is set by hwLoadTexFilterWrap for split trilinear) */
#define TEXPARAMS(f, fbit, cbit, mbit)                                      \
    switch (params->minFilter) {                                                \
      case GL_LINEAR:                                                           \
        GRSET(f, 1, 1, fbit);                                               \
        GRSET(f + 2, 0, 0, mbit);                                           \
        break;                                                                  \
      case GL_NEAREST:                                                          \
        GRSET(f, 1, 0, fbit);                                               \
        GRSET(f + 2, 0, 0, mbit);                                           \
        break;                                                                  \
      case GL_LINEAR_MIPMAP_NEAREST:                                            \
        GRSET(f, 1, 1, fbit);                                               \
        GRSET(f + 2, 0, 1, mbit);                                           \
        break;                                                                  \
      case GL_LINEAR_MIPMAP_LINEAR:                                             \
        GRSET(f, 1, 1, fbit);                                               \
        GRSET(f + 2, 0, gc->fx.trilinearMode == 1 ? 2 : 1, mbit);               \
        break;                                                                  \
      case GL_NEAREST_MIPMAP_NEAREST:                                           \
        GRSET(f, 1, 0, fbit);                                               \
        GRSET(f + 2, 0, 1, mbit);                                           \
        break;                                                                  \
      case GL_NEAREST_MIPMAP_LINEAR:                                            \
        GRSET(f, 1, 0, fbit);                                               \
        GRSET(f + 2, 0, gc->fx.trilinearMode == 1 ? 2 : 1, mbit);               \
        break;                                                                  \
    }                                                                           \
    switch (params->magFilter) {                                                \
      case GL_LINEAR: GRSET(f, 0, 1, fbit); break;                          \
      case GL_NEAREST: GRSET(f, 0, 0, fbit); break;                         \
    }                                                                           \
    switch (params->sWrapMode) {                                                \
      case GL_REPEAT: GRSET(f + 1, 0, 0, cbit); break;                      \
      case GL_CLAMP: case GL_CLAMP_TO_EDGE: GRSET(f + 1, 0, 1, cbit); break; \
    }                                                                           \
    switch (params->tWrapMode) {                                                \
      case GL_REPEAT: GRSET(f + 1, 1, 0, cbit); break;                      \
      case GL_CLAMP: case GL_CLAMP_TO_EDGE: GRSET(f + 1, 1, 1, cbit); break; \
    }
/************************************************************************/
#define TEX_FX(x)               ((__GLtextureFxICD *)(x))
#define TEX_MEM(x, s)       (TEX_FX(x)->mem[s])                /* its heap block */

/* __glSSTLoadCombineFunctionExt first, out of address order: compiled after __glSSTLoadCombineFunction, VC6
** (state carried from one function to the next) drops stores of its locals that the original keeps. */
/* 69017690 (size 5090) -- __glSSTLoadCombineFunctionExt: loads the texture environments of both units into the
** Glide combine records.  Every RGB case writes its own cc / cci / cd / cdi (the original's frame
** slots); VC6 RTM then copies two short RGB tails into the arms that jump to them, where the original kept one copy
** each */
void __fastcall __glSSTLoadCombineFunctionExt(__GLcontext *gc, int unused, int unused2)
{
    GLuint ca, cb, aa, cc, ab, ac, cam, aam, cbm, abm, cci, aci, cdi, cd, adi, ad;
    GLint ashift;
    GLint i;
    __GLtextureEnvState *envs[2];
    __GLtextureICD *tex[2];
    GLuint prev[2][2];                  /* [0] alpha, [1] colour: the previous stage's output, by p */
    GLuint enabled[2];
    __GLtextureEnvState *env;
    GLint p;
    GLenum fmt;

    prev[1][0] = CMBX_ITRGB;
    prev[1][1] = CMBX_OTHER_TEXTURE_RGB;
    prev[0][0] = CMBX_ITALPHA;
    prev[0][1] = CMBX_OTHER_TEXTURE_ALPHA;
    for (i = 0; i < 2; i++) {
        enabled[i] = gc->texture.textureEnabled[i];
        envs[i] = (__GLtextureEnvState *)gc->state.texture.unit[i].env;
        tex[i] = __glCurrentTexture(gc, i);
    }
    hwLoadColorAlphaCombineExt(gc, 0xf, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0);
    for (i = 0; i < 2; i++) {
        if (!enabled[i]) {
            if (i != 0) {
                hwLoadTexColorCombineExt(gc, i, CMBX_OTHER_TEXTURE_RGB, FMODE_X, CMBX_ZERO, FMODE_X, CMBX_ZERO, 1,
                                         CMBX_ZERO, 0, 0);
                hwLoadTexAlphaCombineExt(gc, i, CMBX_OTHER_TEXTURE_ALPHA, FMODE_X, CMBX_ZERO, FMODE_X, CMBX_ZERO, 1,
                                         CMBX_ZERO, 0, 0);
            }
        } else {
            p = (i != 0 && enabled[i - 1]) ? 1 : 0;
            env = envs[i];
            fmt = tex[i]->baseFormat;
            switch (env->mode) {
              case GL_REPLACE:
                if (fmt == GL_ALPHA)
                    hwLoadTexColorCombineExt(gc, i, prev[1][p], FMODE_X, CMBX_ZERO, FMODE_X, CMBX_ZERO, 1,
                                             CMBX_ZERO, 0, 0);
                else
                    hwLoadTexColorCombineExt(gc, i, CMBX_LOCAL_TEXTURE_RGB, FMODE_X, CMBX_ZERO, FMODE_X, CMBX_ZERO,
                                             1, CMBX_ZERO, 0, 0);
                if (fmt == GL_LUMINANCE || fmt == GL_RGB)
                    hwLoadTexAlphaCombineExt(gc, i, prev[0][p], FMODE_X, CMBX_ZERO, FMODE_X, CMBX_ZERO, 1,
                                             CMBX_ZERO, 0, 0);
                else
                    hwLoadTexAlphaCombineExt(gc, i, CMBX_LOCAL_TEXTURE_ALPHA, FMODE_X, CMBX_ZERO, FMODE_X,
                                             CMBX_ZERO, 1, CMBX_ZERO, 0, 0);
                break;
              case GL_BLEND:
                hwLoadTmuConstantColor(gc, i, PACK_ARGB(env->color));
                if (fmt == GL_ALPHA)
                    hwLoadTexColorCombineExt(gc, i, prev[1][p], FMODE_X, CMBX_ZERO, FMODE_X, CMBX_ZERO, 1,
                                             CMBX_ZERO, 0, 0);
                else
                    hwLoadTexColorCombineExt(gc, i, CMBX_TMU_CCOLOR, FMODE_X, prev[1][p], FMODE_NEGATIVE_X,
                                             CMBX_LOCAL_TEXTURE_RGB, 0, CMBX_B, 0, 0);
                if (fmt == GL_LUMINANCE || fmt == GL_RGB)
                    hwLoadTexAlphaCombineExt(gc, i, prev[0][p], FMODE_X, CMBX_ZERO, FMODE_X, CMBX_ZERO, 1,
                                             CMBX_ZERO, 0, 0);
                else if (fmt == GL_INTENSITY)
                    hwLoadTexAlphaCombineExt(gc, i, CMBX_TMU_CALPHA, FMODE_X, prev[0][p], FMODE_NEGATIVE_X,
                                             CMBX_LOCAL_TEXTURE_ALPHA, 0, CMBX_B, 0, 0);
                else
                    hwLoadTexAlphaCombineExt(gc, i, prev[0][p], FMODE_X, CMBX_ZERO, FMODE_X,
                                             CMBX_LOCAL_TEXTURE_ALPHA, 0, CMBX_ZERO, 0, 0);
                break;
              case GL_ADD:
                if (fmt == GL_ALPHA)
                    hwLoadTexColorCombineExt(gc, i, prev[1][p], FMODE_X, CMBX_ZERO, FMODE_X, CMBX_ZERO, 1,
                                             CMBX_ZERO, 0, 0);
                else
                    hwLoadTexColorCombineExt(gc, i, prev[1][p], FMODE_X, CMBX_LOCAL_TEXTURE_RGB, FMODE_X, CMBX_ZERO,
                                             1, CMBX_ZERO, 0, 0);
                if (fmt == GL_LUMINANCE || fmt == GL_RGB)
                    hwLoadTexAlphaCombineExt(gc, i, prev[0][p], FMODE_X, CMBX_ZERO, FMODE_X, CMBX_ZERO, 1,
                                             CMBX_ZERO, 0, 0);
                else if (fmt == GL_INTENSITY)
                    hwLoadTexAlphaCombineExt(gc, i, prev[0][p], FMODE_X, CMBX_LOCAL_TEXTURE_ALPHA, FMODE_X,
                                             CMBX_ZERO, 1, CMBX_ZERO, 0, 0);
                else
                    hwLoadTexAlphaCombineExt(gc, i, prev[0][p], FMODE_X, CMBX_ZERO, FMODE_X,
                                             CMBX_LOCAL_TEXTURE_ALPHA, 0, CMBX_ZERO, 0, 0);
                break;
              case GL_MODULATE:
                if (fmt == GL_ALPHA)
                    hwLoadTexColorCombineExt(gc, i, prev[1][p], FMODE_X, CMBX_ZERO, FMODE_X, CMBX_ZERO, 1,
                                             CMBX_ZERO, 0, 0);
                else
                    hwLoadTexColorCombineExt(gc, i, prev[1][p], FMODE_X, CMBX_ZERO, FMODE_X, CMBX_LOCAL_TEXTURE_RGB,
                                             0, CMBX_ZERO, 0, 0);
                if (fmt == GL_LUMINANCE || fmt == GL_RGB)
                    hwLoadTexAlphaCombineExt(gc, i, prev[0][p], FMODE_X, CMBX_ZERO, FMODE_X, CMBX_ZERO, 1,
                                             CMBX_ZERO, 0, 0);
                else
                    hwLoadTexAlphaCombineExt(gc, i, prev[0][p], FMODE_X, CMBX_ZERO, FMODE_X,
                                             CMBX_LOCAL_TEXTURE_ALPHA, 0, CMBX_ZERO, 0, 0);
                break;
              case GL_COMBINE_EXT:
                hwLoadTmuConstantColor(gc, i, PACK_ARGB(env->color));
                switch (env->combineRGB) {
                  case GL_REPLACE:
                    SRC_RGB(env->sourceRGB[0], env->operandRGB[0], ca);
                    MODE_RGB(env->operandRGB[0], cam);
                    cb = CMBX_ZERO; cbm = FMODE_X;
                    cc = CMBX_ZERO; cci = 1;
                    cd = CMBX_ZERO; cdi = 0;
                    break;
                  case GL_ADD:
                    SRC_RGB(env->sourceRGB[0], env->operandRGB[0], ca);
                    MODE_RGB(env->operandRGB[0], cam);
                    SRC_RGB(env->sourceRGB[1], env->operandRGB[1], cb);
                    MODE_RGB(env->operandRGB[1], cbm);
                    cc = CMBX_ZERO; cci = 1;
                    cd = CMBX_ZERO; cdi = 0;
                    break;
                  case GL_MODULATE:
                    SRC_RGB(env->sourceRGB[0], env->operandRGB[0], ca);
                    MODE_RGB(env->operandRGB[0], cam);
                    SRC_RGB(env->sourceRGB[1], env->operandRGB[1], cc);
                    switch (env->operandRGB[1]) {
                      case GL_SRC_COLOR: case GL_SRC_ALPHA: cci = 0; break;
                      case GL_ONE_MINUS_SRC_COLOR: case GL_ONE_MINUS_SRC_ALPHA: cci = 1; break;
                    }
                    cb = CMBX_ZERO; cbm = FMODE_X;
                    cd = CMBX_ZERO; cdi = 0;
                    break;
                  case GL_ADD_SIGNED_EXT:
                    SRC_RGB(env->sourceRGB[0], env->operandRGB[0], ca);
                    MODE_RGB(env->operandRGB[0], cam);
                    SRC_RGB(env->sourceRGB[1], env->operandRGB[1], cb);
                    MODE_RGB(env->operandRGB[1], cbm);
                    if (env->operandRGB[1] == GL_SRC_COLOR || env->operandRGB[1] == GL_SRC_ALPHA)
                        cbm = FMODE_X_MINUS_HALF;
                    else if (env->operandRGB[0] == GL_SRC_COLOR || env->operandRGB[0] == GL_SRC_ALPHA)
                        cam = FMODE_X_MINUS_HALF;
                    cc = CMBX_ZERO; cci = 1;
                    cd = CMBX_ZERO; cdi = 0;
                    break;
                  case GL_INTERPOLATE_EXT:
                    SRC_RGB(env->sourceRGB[0], env->operandRGB[0], ca);
                    MODE_RGB(env->operandRGB[0], cam);
                    SRC_RGB(env->sourceRGB[1], env->operandRGB[1], cb);
                    /* the operand switch collapses late (every arm stores the same mode): the original keeps
                    ** operandRGB[1] / operandAlpha[1] loaded (ecx / a dead `mov eax,[ebx+0x44]`) */
                    switch (env->operandRGB[1]) {
                      case GL_SRC_COLOR: case GL_SRC_ALPHA: case GL_ONE_MINUS_SRC_COLOR: case GL_ONE_MINUS_SRC_ALPHA:
                        cbm = FMODE_NEGATIVE_X; break;
                      default: cbm = FMODE_NEGATIVE_X; break;
                    }
                    SRC_ALPHA(env->sourceRGB[2], cc);
                    cci = 0;
                    cd = CMBX_B; cdi = 0;
                    break;
                }
                switch (env->combineAlpha) {
                  case GL_REPLACE:
                    SRC_ALPHA(env->sourceAlpha[0], aa);
                    MODE_ALPHA(env->operandAlpha[0], aam);
                    ab = CMBX_ZERO; abm = FMODE_X;
                    ac = CMBX_ZERO; aci = 1;
                    ad = CMBX_ZERO; adi = 0;
                    break;
                  case GL_ADD:
                    SRC_ALPHA(env->sourceAlpha[0], aa);
                    MODE_ALPHA(env->operandAlpha[0], aam);
                    SRC_ALPHA(env->sourceAlpha[1], ab);
                    MODE_ALPHA(env->operandAlpha[1], abm);
                    ac = CMBX_ZERO; aci = 1;
                    ad = CMBX_ZERO; adi = 0;
                    break;
                  case GL_MODULATE:
                    SRC_ALPHA(env->sourceAlpha[0], aa);
                    MODE_ALPHA(env->operandAlpha[0], aam);
                    SRC_ALPHA(env->sourceAlpha[1], ac);
                    switch (env->operandAlpha[1]) {
                      case GL_SRC_ALPHA: aci = 0; break;
                      case GL_ONE_MINUS_SRC_ALPHA: aci = 1; break;
                    }
                    ab = CMBX_ZERO; abm = FMODE_X;
                    ad = CMBX_ZERO; adi = 0;
                    break;
                  case GL_ADD_SIGNED_EXT:
                    SRC_ALPHA(env->sourceAlpha[0], aa);
                    MODE_ALPHA(env->operandAlpha[0], aam);
                    SRC_ALPHA(env->sourceAlpha[1], ab);
                    MODE_ALPHA(env->operandAlpha[1], abm);
                    if (env->operandAlpha[1] == GL_SRC_COLOR || env->operandAlpha[1] == GL_SRC_ALPHA)
                        abm = FMODE_X_MINUS_HALF;
                    else if (env->operandAlpha[0] == GL_SRC_COLOR || env->operandAlpha[0] == GL_SRC_ALPHA)
                        aam = FMODE_X_MINUS_HALF;
                    ac = CMBX_ZERO; aci = 1;
                    ad = CMBX_ZERO; adi = 0;
                    break;
                  case GL_INTERPOLATE_EXT:
                    SRC_ALPHA(env->sourceAlpha[0], aa);
                    MODE_ALPHA(env->operandAlpha[0], aam);
                    SRC_ALPHA(env->sourceAlpha[1], ab);
                    switch (env->operandAlpha[1]) {
                      case GL_SRC_ALPHA: case GL_ONE_MINUS_SRC_ALPHA: abm = FMODE_NEGATIVE_X; break;
                      default: abm = FMODE_NEGATIVE_X; break;
                    }
                    SRC_ALPHA(env->sourceAlpha[2], ac);
                    aci = 0;
                    ad = CMBX_B; adi = 0;
                    break;
                }
                ashift = (GLint)env->alphaScale >> 1;
                hwLoadTexColorCombineExt(gc, i, ca, cam, cb, cbm, cc, cci, cd, cdi, (GLint)env->rgbScale >> 1);
                hwLoadTexAlphaCombineExt(gc, i, aa, aam, ab, abm, ac, aci, ad, adi, ashift);
                break;
              case GL_DECAL:
                if (fmt == GL_RGBA)
                    hwLoadTexColorCombineExt(gc, i, CMBX_LOCAL_TEXTURE_RGB, FMODE_X, prev[1][p], FMODE_NEGATIVE_X,
                                             CMBX_LOCAL_TEXTURE_ALPHA, 0, CMBX_B, 0, 0);
                else
                    hwLoadTexColorCombineExt(gc, i, CMBX_LOCAL_TEXTURE_RGB, FMODE_X, CMBX_ZERO, FMODE_X, CMBX_ZERO,
                                             1, CMBX_ZERO, 0, 0);
                hwLoadTexAlphaCombineExt(gc, i, prev[0][p], FMODE_X, CMBX_ZERO, FMODE_X, CMBX_ZERO, 1, CMBX_ZERO, 0,
                                         0);
                break;
            }
        }
    }
}

/* 69016730 (size 1675) -- __glSSTLoadCombineFunction: texture environment -> Glide texture / colour / alpha combine
** (no combine extension), called per unit by 690190c0 (with the unit's texture object, unused here); unit 0 is
** skipped while unit 1 is enabled (unit 1's call sets both TMUs).  The unit number doubles as the base format of a
** unit without a texture object (original oddity, kept) */
void __fastcall __glSSTLoadCombineFunction(__GLcontext *gc, GLuint unused, GLenum unit)
{
    GLuint en1, en0;
    __GLtextureICD *tex0, *tex1;
    __GLtextureEnvState *env0, *env1;
    GLenum fmt0, fmt;
    __GLtextureEnvState *env;
    GLint mode;

    en0 = gc->texture.textureEnabled[0];
    en1 = gc->texture.textureEnabled[1];
    tex0 = __glCurrentTexture(gc, 0);
    tex1 = __glCurrentTexture(gc, 1);
    env0 = (__GLtextureEnvState *)gc->state.texture.unit[0].env;
    env1 = (__GLtextureEnvState *)gc->state.texture.unit[1].env;

    if (!unit && en1) return;
    if (tex0) fmt0 = tex0->baseFormat; else fmt0 = unit;
    if (tex1) unit = tex1->baseFormat;

    if (gc->fx.numTMUs == 1) {
        if (gc->fx.trilinearMode == 2) {
            /* split trilinear: TMU0 blends its odd mip level with TMU1's even one by the LOD fraction (TMU1 passes
            ** its texel on); any other texture: TMU0 alone */
            if (tex0 && hwTexIsSplit(gc, tex0)) {
                GRS_SET(__GL_GR_TMU0_COMBINE, 0, GR_COMBINE_FUNCTION_BLEND);
                GRS_SET(__GL_GR_TMU0_COMBINE, 1, GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION);
                GRS_SET(__GL_GR_TMU1_COMBINE, 0, GR_COMBINE_FUNCTION_LOCAL);
                GRS_SET(__GL_GR_TMU1_COMBINE, 1, GR_COMBINE_FACTOR_NONE);
            } else {
                GRS_SET(__GL_GR_TMU0_COMBINE, 0, GR_COMBINE_FUNCTION_LOCAL);
                GRS_SET(__GL_GR_TMU0_COMBINE, 1, GR_COMBINE_FACTOR_NONE);
            }
        }
        SIMPLE_ENV(env0->mode, fmt0, env0);
        return;
    }
    if (en0) {
        if (en1) {
            switch (env0->mode) {
              case GL_MODULATE:
                hwLoadTexCombine(gc, 0, 1, 0);
                switch (env1->mode) {
                  case GL_ADD: hwLoadTexCombine(gc, 1, 4, 8); return;
                  case GL_REPLACE: hwLoadTexCombine(gc, 1, 1, 0); return;
                  case GL_MODULATE:
                    hwLoadTexCombine(gc, 1, 3, 1);
                    hwLoadColorAlphaCombine(gc, 3, 1, 0, 1, 3, 1, 0, 1);
                    return;
                }
                return;
              case GL_REPLACE:
                hwLoadTexCombine(gc, 0, 1, 0);
                switch (env1->mode) {
                  case GL_BLEND: hwLoadTexCombine(gc, 1, 3, 9); break;
                  case GL_ADD: hwLoadTexCombine(gc, 1, 4, 8); break;
                  case GL_REPLACE: hwLoadTexCombine(gc, 1, 1, 0); break;
                  case GL_MODULATE: hwLoadTexCombine(gc, 1, 3, 1); break;
                  case GL_DECAL: hwLoadTexCombine(gc, 1, 7, 0xb); break;
                }
                hwLoadColorAlphaCombine(gc, 3, 8, 1, 1, 3, 8, 1, 1);
                return;
              case GL_ADD:
                hwLoadTexCombine(gc, 0, 1, 0);
                switch (env1->mode) {
                  case GL_ADD:
                    hwLoadTexCombine(gc, 1, 4, 8);
                    hwLoadColorAlphaCombine(gc, 4, 8, 0, 1, 3, 1, 0, 1);
                    return;
                  case GL_REPLACE: hwLoadTexCombine(gc, 1, 1, 0); return;
                }
                return;
            }
            return;
        }
        hwLoadTexCombine(gc, 0, 1, 0);
        hwLoadTexCombine(gc, 1, 3, 8);
        mode = env0->mode;
        fmt = fmt0;
        env = env0;
    } else {
        hwLoadTexCombine(gc, 0, 0, 0);
        hwLoadTexCombine(gc, 1, 1, 0);
        mode = env1->mode;
        fmt = unit;
        env = env1;
    }
    if (mode == -1) return;
    SIMPLE_ENV(mode, fmt, env);
}

/* 69018b00 (size 848) -- hwLoadTexFilterWrap: a texture object's filters and wrap modes into the Glide records of
** the TMU fed by texture unit `unit`; a split trilinear texture sets both TMUs, with LOD blending */
void __fastcall hwLoadTexFilterWrap(__GLcontext *gc, __GLtextureICD *tex, GLint unit)
{
    __GLtexObjParamICD *params = &tex->params;
    GLint split;

    if (gc->fx.trilinearMode == 2) {
        split = hwTexIsSplit(gc, tex);
        TEXPARAMS(4, 0x10, 0x20, 0x40);
        GRSET(__GL_GR_TMU0_MIPMAP, 1, split, 0x40);
        if (split) {
            TEXPARAMS(8, 0x100, 0x200, 0x400);
        }
        GRSET(__GL_GR_TMU1_MIPMAP, 1, split, 0x400);
        return;
    }
    if (gc->fx.tmuTexUnit[unit] == 0) {
        TEXPARAMS(4, 0x10, 0x20, 0x40);
    } else {
        TEXPARAMS(8, 0x100, 0x200, 0x400);
    }
}

/* 69018e50 (size 112) -- hwSetTmuSource: points the TMU fed by unit `unit` at a resident texture (and, windowed, at
** its texture surface); a split trilinear texture: TMU0 at its odd levels, TMU1 at the even ones after them */
void __fastcall hwSetTmuSource(__GLcontext *gc, __GLtextureICD *tex, GLuint unit)
{
    if (TEX_FX(tex)->onTmu[__GL_TMU_SLOT(unit)] == 3) {
        GrTexInfo *info = (GrTexInfo *)&TEX_FX(tex)->smallLodLog2;
        GLuint start = TEX_MEM(tex, __GL_TMU_SLOT(unit))->start;

        grTexSource(GR_TMU0, start, GR_MIPMAPLEVELMASK_ODD, info);
        grTexSource(GR_TMU1, start + grTexTextureMemRequired(GR_MIPMAPLEVELMASK_ODD, info),
                    GR_MIPMAPLEVELMASK_EVEN, info);
        return;
    }
    if (gc->hwcx.windowed)
        grSurfaceSetTextureSurfaceExt(gc->fx.tmuTexUnit[unit],
            TEX_MEM(tex, __GL_TMU_SLOT(unit))->hwSurface);
    grTexSource(gc->fx.tmuTexUnit[unit], TEX_MEM(tex, __GL_TMU_SLOT(unit))->start, GR_MIPMAPLEVELMASK_BOTH,
                (GrTexInfo *)&TEX_FX(tex)->smallLodLog2);
}

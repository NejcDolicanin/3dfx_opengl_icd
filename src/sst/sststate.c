/*
** sststate.c -- hardware-path state procs of the 3dfx back end (69016060 .. 6901a3b0): the pixel-proc pick,
** the Glide combine records (plain and combine extension) and their defaults, the sphere-map inverse
** transpose and the check whether the hardware fast path can draw the current state.
*/
#include <string.h>
#include "wglpriv.h"
#include <glide.h>
#include <g3ext.h>
#include "glsi.h"
#include "icdglob.h"
#include "icdprotos.h"

/* ---- forward declarations ---- */
GLint __fastcall hwCanUseFastPath(__GLcontext *gc, GLuint edx);

/* 690163d0 -- __glSSTPickPixelProcs: procs.pickPixelProcs: the generic pick, with the hardware glDrawPixels /
** glReadPixels */
void __fastcall __glSSTPickPixelProcs(__GLcontext *gc)
{
    __glGenericPickPixelProcs(gc);
    gc->procs.drawPixels = (__GLproc)__glSlowPickDrawPixels;
    gc->procs.readPixels = (__GLproc)hwReadPixels;
}

/* 69016590 -- hwLoadColorAlphaCombine: load the Glide colour and alpha combine records {function, factor, local,
** other} */
void __fastcall hwLoadColorAlphaCombine(__GLcontext *gc, GLint cFunc, GLint cFactor, GLint cLocal, GLint cOther,
                             GLint aFunc, GLint aFactor, GLint aLocal, GLint aOther)
{
    GRS_SET(__GL_GR_COLOR_COMBINE, 0, cFunc);
    GRS_SET(__GL_GR_COLOR_COMBINE, 1, cFactor);
    GRS_SET(__GL_GR_COLOR_COMBINE, 2, cLocal);
    GRS_SET(__GL_GR_COLOR_COMBINE, 3, cOther);
    GRS_SET(__GL_GR_ALPHA_COMBINE, 0, aFunc);
    GRS_SET(__GL_GR_ALPHA_COMBINE, 1, aFactor);
    GRS_SET(__GL_GR_ALPHA_COMBINE, 2, aLocal);
    GRS_SET(__GL_GR_ALPHA_COMBINE, 3, aOther);
}

/* 690166a0 -- hwLoadTexCombine: load the texture combine record (rgb function, factor) of the TMU GL unit `unit`
** feeds */
void __fastcall hwLoadTexCombine(__GLcontext *gc, GLint unit, GLint rgbFunc, GLint rgbFactor)
{
    if (gc->fx.tmuTexUnit[unit] == 0) {
        GRS_SET(__GL_GR_TMU0_COMBINE, 0, rgbFunc);
        GRS_SET(__GL_GR_TMU0_COMBINE, 1, rgbFactor);
    } else {
        GRS_SET(__GL_GR_TMU1_COMBINE, 0, rgbFunc);
        GRS_SET(__GL_GR_TMU1_COMBINE, 1, rgbFactor);
    }
}

/* 69016e90 -- hwLoadColorAlphaCombineExt: load the colour and alpha combine-extension records (a, a_mode, b,
** b_mode, c, c_invert, d, d_invert; shift and invert stay).  A change of either reloads both, and the texture combine
** records too. */
void __fastcall hwLoadColorAlphaCombineExt(__GLcontext *gc, GLint ca, GLint caMode, GLint cb, GLint cbMode, GLint cc,
                             GLint ccInvert, GLint cd, GLint cdInvert, GLint aa, GLint aaMode, GLint ab,
                             GLint abMode, GLint ac, GLint acInvert, GLint ad, GLint adInvert)
{
    GRX_SET(__GL_GRX_COLOR_COMBINE, 0, ca);
    GRX_SET(__GL_GRX_COLOR_COMBINE, 1, caMode);
    GRX_SET(__GL_GRX_COLOR_COMBINE, 2, cb);
    GRX_SET(__GL_GRX_COLOR_COMBINE, 3, cbMode);
    GRX_SET(__GL_GRX_COLOR_COMBINE, 4, cc);
    GRX_SET(__GL_GRX_COLOR_COMBINE, 5, ccInvert);
    GRX_SET(__GL_GRX_COLOR_COMBINE, 6, cd);
    GRX_SET(__GL_GRX_COLOR_COMBINE, 7, cdInvert);
    if (gc->hwcx.grDirtyExt & (1 << __GL_GRX_COLOR_COMBINE)) {
        gc->hwcx.grDirtyExt |= 1 << __GL_GRX_ALPHA_COMBINE;
    }
    GRX_SET(__GL_GRX_ALPHA_COMBINE, 0, aa);
    GRX_SET(__GL_GRX_ALPHA_COMBINE, 1, aaMode);
    GRX_SET(__GL_GRX_ALPHA_COMBINE, 2, ab);
    GRX_SET(__GL_GRX_ALPHA_COMBINE, 3, abMode);
    GRX_SET(__GL_GRX_ALPHA_COMBINE, 4, ac);
    GRX_SET(__GL_GRX_ALPHA_COMBINE, 5, acInvert);
    GRX_SET(__GL_GRX_ALPHA_COMBINE, 6, ad);
    GRX_SET(__GL_GRX_ALPHA_COMBINE, 7, adInvert);
    if (gc->hwcx.grDirtyExt & (1 << __GL_GRX_ALPHA_COMBINE)) {
        gc->hwcx.grDirtyExt |= 1 << __GL_GRX_COLOR_COMBINE;
    }
    if (gc->hwcx.grDirtyExt & (1 << __GL_GRX_COLOR_COMBINE)) {
        gc->hwcx.grDirtyExt |= 1 << __GL_GRX_TMU0_ALPHA_COMBINE;
    }
    if (gc->hwcx.grDirtyExt & (1 << __GL_GRX_COLOR_COMBINE)) {
        gc->hwcx.grDirtyExt |= 1 << __GL_GRX_TMU0_COLOR_COMBINE;
    }
    if (gc->hwcx.grDirtyExt & (1 << __GL_GRX_COLOR_COMBINE)) {
        gc->hwcx.grDirtyExt |= 1 << __GL_GRX_TMU1_ALPHA_COMBINE;
    }
    if (gc->hwcx.grDirtyExt & (1 << __GL_GRX_COLOR_COMBINE)) {
        gc->hwcx.grDirtyExt |= 1 << __GL_GRX_TMU1_COLOR_COMBINE;
    }
}

/* 69017110 -- hwLoadTexColorCombineExt: load the texture colour combine-extension record (a .. shift; the record's
** word 0 is the TMU, invert stays) of the TMU GL unit `unit` feeds; a change reloads that TMU's alpha combine too */
void __fastcall hwLoadTexColorCombineExt(__GLcontext *gc, GLint unit, GLint a, GLint aMode, GLint b, GLint bMode, GLint c,
                             GLint cInvert, GLint d, GLint dInvert, GLint shift)
{
    if (gc->fx.tmuTexUnit[unit] == 0) {
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 1, a);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 2, aMode);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 3, b);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 4, bMode);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 5, c);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 6, cInvert);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 7, d);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 8, dInvert);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 9, shift);
        if (gc->hwcx.grDirtyExt & (1 << __GL_GRX_TMU0_COLOR_COMBINE)) {
            gc->hwcx.grDirtyExt |= 1 << __GL_GRX_TMU0_ALPHA_COMBINE;
        }
    } else {
        GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 1, a);
        GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 2, aMode);
        GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 3, b);
        GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 4, bMode);
        GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 5, c);
        GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 6, cInvert);
        GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 7, d);
        GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 8, dInvert);
        GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 9, shift);
        if (gc->hwcx.grDirtyExt & (1 << __GL_GRX_TMU1_COLOR_COMBINE)) {
            gc->hwcx.grDirtyExt |= 1 << __GL_GRX_TMU1_ALPHA_COMBINE;
        }
    }
}

/* 690173a0 -- hwLoadTexAlphaCombineExt: the same for the texture alpha combine-extension record */
void __fastcall hwLoadTexAlphaCombineExt(__GLcontext *gc, GLint unit, GLint a, GLint aMode, GLint b, GLint bMode, GLint c,
                             GLint cInvert, GLint d, GLint dInvert, GLint shift)
{
    if (gc->fx.tmuTexUnit[unit] == 0) {
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 1, a);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 2, aMode);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 3, b);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 4, bMode);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 5, c);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 6, cInvert);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 7, d);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 8, dInvert);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 9, shift);
        if (gc->hwcx.grDirtyExt & (1 << __GL_GRX_TMU0_ALPHA_COMBINE)) {
            gc->hwcx.grDirtyExt |= 1 << __GL_GRX_TMU0_COLOR_COMBINE;
        }
    } else {
        GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 1, a);
        GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 2, aMode);
        GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 3, b);
        GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 4, bMode);
        GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 5, c);
        GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 6, cInvert);
        GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 7, d);
        GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 8, dInvert);
        GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 9, shift);
        if (gc->hwcx.grDirtyExt & (1 << __GL_GRX_TMU1_ALPHA_COMBINE)) {
            gc->hwcx.grDirtyExt |= 1 << __GL_GRX_TMU1_COLOR_COMBINE;
        }
    }
}

/* 69017630 -- hwLoadTmuConstantColor: load the TMU constant colour (grConstantColorValueExt) of the TMU GL unit
** `unit` feeds */
void __fastcall hwLoadTmuConstantColor(__GLcontext *gc, GLint unit, GLuint color)
{
    if (gc->fx.tmuTexUnit[unit] == 0) {
        GRX_SET(__GL_GRX_TMU0_CONSTANT, 1, (GLint)color);
    } else {
        GRX_SET(__GL_GRX_TMU1_CONSTANT, 1, (GLint)color);
    }
}

/* 69018ec0 -- hwDefaultCombine: the plain combine defaults: no texturing -> colour and alpha come from the iterated
** values; unit 0 alone -> TMU0 passes its texture through (scale other by one) */
void __fastcall hwDefaultCombine(__GLcontext *gc)
{
    if (!gc->texture.textureEnabled[0] && !gc->texture.textureEnabled[1]) {
        GRS_SET(__GL_GR_COLOR_COMBINE, 0, GR_COMBINE_FUNCTION_LOCAL);
        GRS_SET(__GL_GR_COLOR_COMBINE, 1, GR_COMBINE_FACTOR_ZERO);
        GRS_SET(__GL_GR_COLOR_COMBINE, 2, GR_COMBINE_LOCAL_ITERATED);
        GRS_SET(__GL_GR_COLOR_COMBINE, 3, GR_COMBINE_OTHER_CONSTANT);
        GRS_SET(__GL_GR_ALPHA_COMBINE, 0, GR_COMBINE_FUNCTION_LOCAL);
        GRS_SET(__GL_GR_ALPHA_COMBINE, 1, GR_COMBINE_FACTOR_ZERO);
        GRS_SET(__GL_GR_ALPHA_COMBINE, 2, GR_COMBINE_LOCAL_ITERATED);
        GRS_SET(__GL_GR_ALPHA_COMBINE, 3, GR_COMBINE_OTHER_CONSTANT);
    }
    if (gc->texture.textureEnabled[0] && !gc->texture.textureEnabled[1]) {
        GRS_SET(__GL_GR_TMU0_COMBINE, 0, GR_COMBINE_FUNCTION_SCALE_OTHER);
        GRS_SET(__GL_GR_TMU0_COMBINE, 1, GR_COMBINE_FACTOR_ONE);
    }
}

/* 69019030 -- hwDefaultCombineExt: the combine-extension defaults of the disabled units: TMU0 passes the iterated
** colour / alpha, TMU1 the upstream TMU's output */
void __fastcall hwDefaultCombineExt(__GLcontext *gc)
{
    if (!gc->texture.textureEnabled[0]) {
        hwLoadTexColorCombineExt(gc, 0, GR_CMBX_ITRGB, GR_FUNC_MODE_X, GR_CMBX_ZERO, GR_FUNC_MODE_X, GR_CMBX_ZERO, 1,
                     GR_CMBX_ZERO, 0, 0);
        hwLoadTexAlphaCombineExt(gc, 0, GR_CMBX_ITALPHA, GR_FUNC_MODE_X, GR_CMBX_ZERO, GR_FUNC_MODE_X, GR_CMBX_ZERO, 1,
                     GR_CMBX_ZERO, 0, 0);
    }
    if (!gc->texture.textureEnabled[1]) {
        hwLoadTexColorCombineExt(gc, 1, GR_CMBX_OTHER_TEXTURE_RGB, GR_FUNC_MODE_X, GR_CMBX_ZERO, GR_FUNC_MODE_X,
                     GR_CMBX_ZERO, 1, GR_CMBX_ZERO, 0, 0);
        hwLoadTexAlphaCombineExt(gc, 1, GR_CMBX_OTHER_TEXTURE_ALPHA, GR_FUNC_MODE_X, GR_CMBX_ZERO, GR_FUNC_MODE_X,
                     GR_CMBX_ZERO, 1, GR_CMBX_ZERO, 0, 0);
    }
}

/* 69019fe0 -- hwValidateSphereMapInverse: sphere-map texgen on unit 0 needs the modelview inverse transpose: update
** a stale one */
void __fastcall hwValidateSphereMapInverse(__GLcontext *gc)
{
    if ((gc->state.enables.texture[0] & (__GL_TEXTURE_GEN_S_ENABLE | __GL_TEXTURE_GEN_T_ENABLE)) &&                 /* GL_TEXTURE_GEN_S / T */
        (gc->state.texture.unit[0].s.mode == GL_SPHERE_MAP || gc->state.texture.unit[0].t.mode == GL_SPHERE_MAP) &&
        gc->transform.modelView->updateInverse) {
        (*gc->procs.computeInverseTranspose)(gc, gc->transform.modelView);
    }
}

/* 6901a020 -- hwCanUseFastPath: can the hardware fast path draw with the current state?  Not with lighting, cheap
** fog, fog-coordinate fog, clip planes, software rendering, feedback / select, GL_FRONT_AND_BACK drawing, stipples or
** logic op (stencil only at 32 bpp), separate r / g / b masks at 16 bpp, a texture that is not resident or has a
** border, a line width other than 1, polygon modes other than GL_FILL, polygon smooth, or polygon offset with points
** / lines (primitives below GL_TRIANGLES) */
GLint __fastcall hwCanUseFastPath(__GLcontext *gc, GLuint edx)
{
    GLuint flags;
    GLint i;
    __GLtextureICD *tex;

    flags = (*gc->polygon.shader.getModeFlags)(gc, edx);
    if (!(gc->state.enables.general & __GL_LIGHTING_ENABLE) &&
        !(flags & 0x2000) &&                                    /* render.h __GL_SHADE_CHEAP_FOG */
        (!(gc->state.enables.general & __GL_FOG_ENABLE) || gc->state.fog.coordSource != GL_FOG_COORDINATE_EXT) &&
        gc->state.enables.clipPlanes == 0 &&
        gc->hwcx.swRendering != 1 &&
        gc->renderMode == GL_RENDER &&
        gc->state.raster.drawBuffer != GL_FRONT_AND_BACK &&
        !(flags & ((gc->hwcx.has32bpp ? 0 : 0x20) | 0x8090)) && /* stencil, line stipple, logic op, stipple */
        (gc->hwcx.has32bpp ||
         (gc->state.raster.rMask == gc->state.raster.gMask && gc->state.raster.rMask == gc->state.raster.bMask))) {
        if (flags & 0x8) {                                      /* __GL_SHADE_TEXTURE */
            for (i = 0; i < gc->fx.numTMUs; i++) {
                tex = __glCurrentTexture(gc, i);
                if (tex) {
                    if (tex->hwHandle[__GL_TMU_SLOT(i)] == 0) {
                        return GL_FALSE;
                    }
                    if (tex->level[0]->border > 0) {
                        return GL_FALSE;
                    }
                }
            }
        }
        if (gc->state.line.requestedWidth == 1.0f &&
            gc->state.polygon.frontMode == gc->state.polygon.backMode &&
            gc->state.polygon.frontMode == GL_FILL &&
            !(gc->state.enables.general & __GL_POLYGON_SMOOTH_ENABLE) &&
            (!(gc->state.enables.general & __GL_POLYGON_OFFSET_ANY_ENABLE) || gc->fx.primMode >= GL_TRIANGLES)) {
            return gc->renderMode != GL_FEEDBACK;
        }
    }
    return GL_FALSE;
}

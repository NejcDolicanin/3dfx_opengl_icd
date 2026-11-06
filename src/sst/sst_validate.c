/*
** sst_validate.c -- procs.validate of the hardware path and its parts: the GL state (alpha test, blend, colour
** mask, depth test, fog, textures, draw buffer) into the Glide state records, the windowed variant and the
** revalidation.
*/
#include "wglpriv.h"
#include <glide.h>
#include <g3ext.h>
#include "glsi.h"
#include "icdglob.h"
#include "icdprotos.h"
#include "si_names.h"
#include "glshade.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "og.h"
#include "gldisp.h"

/************************************************************************/
typedef int (__fastcall *__GLGcFn)(__GLcontext *gc);
#define CALL1(f, gc)    ((__GLGcFn)(f))(gc)
/* the immediate-mode entries that go to the dispatch table of a layer context, else to hwcx.immProcs */
#define SET_IMM(gc, slot, i, proc)                                                  \
    if ((gc)->modes.layerPlane) (gc)->dispatchState->fn[(slot) / 4] = (__GLdispatchFn)(proc); \
    else (gc)->hwcx.immProcs[i] = (__GLproc)(proc)
/************************************************************************/
#define TEX_FX(x)               ((__GLtextureFxICD *)(x))
#define TEX_PALFMT(x)       (TEX_FX(x)->t.CT.baseFormat)       /* the format of its own palette */
#define TEX_HW(x, s)        (TEX_FX(x)->t.hwHandle[s])         /* texture memory handle per TMU slot */
#define TEX_MEM(x, s)       (TEX_FX(x)->mem[s])                /* its heap block */
#define TEX_PALLOADED(x)    (TEX_FX(x)->paletteLoaded)
#define TEX_PALETTE(x)      (TEX_FX(x)->palette)
/************************************************************************/
/************************************************************************/

/* set field i of Glide state record n, marking it dirty */
#define GRSET(n, i, v, bit)                                     \
    if (gc->hwcx.grState[n][i] != (v)) {                            \
        gc->hwcx.grState[n][i] = (v);                               \
        gc->hwcx.grDirty |= (bit);                                  \
    }
#define GR_ALPHATEST    16
#define GR_ALPHAREF     17
/* the Glide fog records: grState[__GL_GR_FOG_COLOR] = the fog colour (packed ARGB), grState[__GL_GR_FOG] = {mode, density, start, end} */
#define GR_FOGCOLOR     24
#define GR_FOG          25
extern void __fastcall __glSSTValidateDepthTest(__GLcontext *gc);
extern void __fastcall __glSSTValidateAlphaTest(__GLcontext *gc);
extern void __fastcall __glSSTValidateBlend(__GLcontext *gc);
extern void __fastcall hwLoadColorMask(__GLcontext *gc);
extern void __fastcall hwValidateFog(__GLcontext *gc);
extern void __fastcall hwValidateFinish(__GLcontext *gc);

/* 69015850 (size 1000) -- __glSSTValidate: procs.validate of the hardware path: the Glide fast path when the state
** allows it (filled polygons both sides, no clip planes, GL_RENDER into the back buffer, no stipple / logic op ...),
** else the generic pickers */
void __fastcall __glSSTValidate(__GLcontext *gc)
{
    GLuint enables = gc->state.enables.general;
    GLint texDone = 0;

    if (gc->hwcx.windowed
        || gc->state.polygon.frontMode != gc->state.polygon.backMode
        || gc->state.polygon.frontMode != GL_FILL
        || gc->state.enables.clipPlanes
        || gc->renderMode != GL_RENDER
        || gc->state.raster.drawBuffer != GL_BACK
        || (enables & (__GL_POLYGON_STIPPLE_ENABLE | __GL_SCISSOR_TEST_ENABLE | __GL_STENCIL_TEST_ENABLE))) {
        (*gc->procs.pickAllProcsPhase2)(gc);
        if (gc->hwcx.swRendering) {
            (*gc->procs.validate)(gc);
            return;
        }
        CALL1(hwFlushGrState, gc);
        return;
    }

    gc->dirtyMaskSaved |= gc->dirtyMask;
    if (gc->dirtyMask & __GL_DIRTY_HWTEXTURE_ICD) {
        __glSSTGenericPickTextureEnables(gc);
        texDone = 1;
        hwValidateTextures(gc);
        gc->procs.calcTexture = (__GLproc)hwTexCoords;
        gc->procs.calcTextureUnit[0] = (__GLproc)hwXformTexCoords;
        gc->dirtyMask &= ~__GL_DIRTY_HWTEXTURE_ICD;
    }
    if (CALL1(hwCanUseFastPath, gc)) {

        og_genOffsetFifo(gc);
        gc->fx.flags &= ~4;
        __glValidateModelViewForLighting(gc);
        __glValidateCullVertexFromModelView(gc);
        hwValidateSphereMapInverse(gc);
        og_initLazyProcs(gc);
        gc->dirtyMask &= ~__GL_DIRTY_MODELVIEW;
        if (gc->hwcx.mvpMatrixType != gc->transform.modelView->mvp.matrixType) {
            gc->hwcx.mvpMatrixType = gc->transform.modelView->mvp.matrixType;
            gc->dirtyMask |= __GL_DIRTY_MVPTYPE_ICD;
        }
        if (gc->fx.primMode != gc->fx.lastPrimMode) {
            gc->fx.flags |= 2;
            gc->fx.lastPrimMode = gc->fx.primMode;
        }
        if (!texDone && (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LIGHTING))) {
            __glSSTGenericPickTextureEnables(gc);
            hwValidateTextures(gc);
            gc->procs.calcTexture = (__GLproc)hwTexCoords;
            gc->procs.calcTextureUnit[0] = (__GLproc)hwXformTexCoords;
        }
        if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LIGHTING)) {
            if (enables & __GL_LIGHTING_ENABLE) {
                __glValidateLighting(gc);
                (*gc->procs.pickColorMaterialProcs)(gc);
                hwInitClipInterp(gc);
            } else {
                gc->procs.applyColor = (__GLproc)gc->hwcx.genApplyColor;
                gc->procs.calcRasterColor = (__GLproc)NopSpanRender;
                if ((enables & __GL_FOG_ENABLE)
                    && ((*gc->polygon.shader.getModeFlags)(gc) & __GL_SHADE_CHEAP_FOG)
                    && ((*gc->polygon.shader.getModeFlags)(gc) & __GL_SHADE_SMOOTH_LIGHT)
                    && gc->renderMode == GL_RENDER) {
                    gc->procs.calcColor = (__GLproc)FogRGBColor;
                } else {
                    gc->procs.calcColor = (__GLproc)NopSpanRender;
                }
            }
            (*gc->procs.pickBlendProcs)(gc);
            if (gc->validateMask & 1) __glValidateAlphaTest(gc);
            if (gc->validateMask & 8) __glValidateIndexTest(gc);
        }
        /* SHADEMODEL | HWDEPTH | NONUNITYQ | COLORARRAY | TEXENABLE */
        if (gc->dirtyMask & 0xec000) hwBuildFifoTriProcs(gc);
        /* SHADEMODEL | NONUNITYQ | TEXENABLE | 0x400000 (never set) */
        if (gc->dirtyMask & 0x4a4000) og_buildClipInterp(gc);
        /* SHADEMODEL | FACE | NONUNITYQ | TEXENABLE | MVPTYPE | TEXMATRIX | TEXGEN_S..Q */
        if ((gc->dirtyMask & 0x1f8b4000) || (gc->fx.flags & 2)) {
            SET_IMM(gc, 0x208, 0, hwLazyVertex2fv);
            SET_IMM(gc, 0x228, 1, hwLazyVertex3fv);
            SET_IMM(gc, 0x248, 2, hwLazyVertex4fv);
            SET_IMM(gc, 0x20, 3, gc->hwcx.genCode[7]);
            SET_IMM(gc, 0xb0, 4, gc->hwcx.genCode[8]);
        }
        CALL1(hwPickRenderProcs, gc);
    } else {
        if (!texDone) gc->dirtyMask |= __GL_DIRTY_HWTEXTURE_ICD;
        (*gc->procs.pickAllProcsPhase2)(gc);
        if (gc->hwcx.swRendering) {
            (*gc->procs.validate)(gc);
            return;
        }
    }
    if (gc->hwcx.swTriangles | gc->hwcx.swPoints | gc->hwcx.swLines)
        CALL1(__glSSTRevalidate, gc);
    gc->validateMask = 0;
    gc->dirtyMask = 0;
    gc->fx.flags = 0;
    CALL1(hwFlushGrState, gc);
}
/* 690190c0 (size 742) -- hwValidateTextures: validates the bound textures of the hardware path: makes each one
** resident, downloads its palette, and loads the TMU combine / filter / LOD records.  The second pass over the units
** only repeats the work for a texture whose TMU memory changed in the first. */
void __fastcall hwValidateTextures(__GLcontext *gc)
{
    GLint i, unit;
    __GLtextureICD *tex;
    GLuint last[2];
    GLenum fmt;

    (*gc->polygon.shader.getModeFlags)(gc);
    for (i = 0; i < gc->fx.numTMUs * 2; i++) {
        unit = i % gc->fx.numTMUs;
        tex = __glCurrentTexture(gc, unit);
        if (tex) {
            if (i < gc->fx.numTMUs || TEX_HW(tex, __GL_TMU_SLOT(unit)) != last[unit]) {
                fmt = tex->level[0]->baseFormat;
                if (fmt == GL_COLOR_INDEX) {
                    if (gc->state.enables.general & __GL_SHARED_TEXTURE_PALETTE_EXT_ENABLE)
                        fmt = gc->hwcx.sharedPalette.baseFormat;
                    else
                        fmt = TEX_PALFMT(tex);
                }
                tex->baseFormat = fmt;
                if (TEX_HW(tex, __GL_TMU_SLOT(unit)) == 0)
                    ((void (__fastcall *)(__GLcontext *, __GLtextureICD *, GLuint, GLint))tex->makeResident)(gc,
                        tex, tex->makeResidentArg, unit);
                if (gc->hwcx.windowed && TEX_HW(tex, __GL_TMU_SLOT(unit)))
                    grSurfaceSetTextureSurfaceExt(gc->fx.tmuTexUnit[unit],
                        TEX_MEM(tex, __GL_TMU_SLOT(unit))->hwSurface);
                last[unit] = TEX_HW(tex, __GL_TMU_SLOT(unit));
                *(GLuint *)&gc->constants.texSScale = *(GLuint *)&TEX_FX(tex)->texScaleInv[0];
                *(GLuint *)&gc->constants.texTScale = *(GLuint *)&TEX_FX(tex)->texScaleInv[1];
                if (TEX_HW(tex, __GL_TMU_SLOT(unit)) == 0) return;
                TEX_MEM(tex, __GL_TMU_SLOT(unit))->lastUse = gc->fx.swapCount;
                if (tex->level[0]->baseFormat == GL_COLOR_INDEX) {
                    if (gc->state.enables.general & __GL_SHARED_TEXTURE_PALETTE_EXT_ENABLE) {
                        if (!gc->hwcx.sharedPaletteLoaded) {
                            grTexDownloadTable((gc->hwcx.sharedPalette.baseFormat != GL_RGB) + GR_TEXTABLE_PALETTE,
                                               gc->hwcx.palette);
                            gc->hwcx.sharedPaletteLoaded = 1;
                            if (gc->hwcx.paletteTexture) {
                                TEX_PALLOADED(gc->hwcx.paletteTexture) = 0;
                                gc->hwcx.paletteTexture = NULL;
                            }
                        }
                    } else if (!TEX_PALLOADED(tex)) {
                        grTexDownloadTable((TEX_PALFMT(tex) != GL_RGB) + GR_TEXTABLE_PALETTE,
                                           TEX_PALETTE(tex));
                        TEX_PALLOADED(tex) = 1;
                        gc->hwcx.sharedPaletteLoaded = 0;
                        if (gc->hwcx.paletteTexture)
                            TEX_PALLOADED(gc->hwcx.paletteTexture) = 0;
                        gc->hwcx.paletteTexture = tex;
                    }
                }
                gc->hwcx.texScaleExp[unit][0] = TEX_FX(tex)->texScaleExp[0];
                gc->hwcx.texScaleExp[unit][1] = TEX_FX(tex)->texScaleExp[1];
                *(GLuint *)&gc->hwcx.texScale[unit][0] = *(GLuint *)&TEX_FX(tex)->texScale[0];
                *(GLuint *)&gc->hwcx.texScale[unit][1] = *(GLuint *)&TEX_FX(tex)->texScale[1];
                if (gc->hwcx.hasCombineExt)
                    __glSSTLoadCombineFunctionExt(gc, (int)tex, unit);
                else
                    __glSSTLoadCombineFunction(gc, (GLuint)tex, unit);
                hwLoadTexFilterWrap(gc, tex, unit);
                hwSetTmuSource(gc, tex, unit);
                gc->hwcx.unitTexture[unit] = (void *)__glCurrentTexture(gc, unit);
            }
        } else if (i < gc->fx.numTMUs) {
            if (gc->hwcx.unitTexture[unit]) {
                if (gc->hwcx.hasCombineExt)
                    ((void (__fastcall *)(__GLcontext *, GLint))hwDefaultCombineExt)(gc, unit);
                else
                    ((void (__fastcall *)(__GLcontext *, GLint))hwDefaultCombine)(gc, unit);
            }
            gc->hwcx.unitTexture[unit] = (void *)__glCurrentTexture(gc, unit);
        }
    }
    gc->dirtyMask &= ~__GL_DIRTY_HWTEXTURE_ICD;
}

/* 69019dd0 -- hwIsTextureDrawable: the unit's texture can be drawn by the hardware: level 0 not empty and, under
** GL_DECAL, an RGB or RGBA base format (a colour-index texture by its palette: the shared one with
** GL_SHARED_TEXTURE_PALETTE_EXT, else its own) */
GLboolean __fastcall hwIsTextureDrawable(__GLcontext *gc, __GLtextureICD *tex, GLint unit)
{
    __GLmipMapLevelICD *lp = tex->level[0];
    GLenum format;

    if (lp->width != 0 && lp->height != 0 && lp->depth != 0) {
        if (gc->state.texture.unit[unit].env->mode != GL_DECAL) {
            return GL_TRUE;
        }
        format = lp->baseFormat;
        if (format == GL_COLOR_INDEX) {
            if (gc->state.enables.general & __GL_SHARED_TEXTURE_PALETTE_EXT_ENABLE) {
                format = gc->hwcx.sharedPalette.baseFormat;
            }
            else {
                format = tex->CT.baseFormat;
            }
            if (format == 0 || format == GL_COLOR_INDEX) {
                return GL_FALSE;
            }
        }
        if (format == GL_RGB || format == GL_RGBA) {
            return GL_TRUE;
        }
    }
    return GL_FALSE;
}

/* 6901a3b0 (size 1415) -- __glSSTValidateWindowed: procs.validate of the windowed hardware path (rendering to a
** Glide surface): the generic pickers plus the Glide fast path when hwCanUseFastPath allows it */
void __fastcall __glSSTValidateWindowed(__GLcontext *gc)
{
    GLuint enables = gc->state.enables.general;
    GLint texDone = 0;

    gc->dirtyMaskSaved |= gc->dirtyMask;
    if (gc->dirtyMask & __GL_DIRTY_HWTEXTURE_ICD) {
        __glSSTGenericPickTextureEnables(gc);
        texDone = 1;
        computeModeFlags(gc);
        computeVertexNeeds(gc);
        (*gc->procs.pickTextureProcs)(gc);
        (*gc->procs.pickCalcTextureProcs)(gc);
        gc->dirtyMask &= ~__GL_DIRTY_HWTEXTURE_ICD;
    }
    og_genOffsetFifo(gc);
    gc->fx.flags &= ~4;
    __glValidateModelViewForLighting(gc);
    __glValidateCullVertexFromModelView(gc);
    hwValidateSphereMapInverse(gc);
    og_initLazyProcs(gc);
    gc->dirtyMask &= ~__GL_DIRTY_MODELVIEW;
    if (gc->hwcx.mvpMatrixType != gc->transform.modelView->mvp.matrixType) {
        gc->hwcx.mvpMatrixType = gc->transform.modelView->mvp.matrixType;
        gc->dirtyMask |= __GL_DIRTY_MVPTYPE_ICD;
    }
    if (gc->fx.primMode != gc->fx.lastPrimMode) {
        gc->fx.flags |= 2;
        gc->fx.lastPrimMode = gc->fx.primMode;
    }
    if (gc->dirtyMask & 0x220f) {   /* GENERIC | LINE | POLYGON | POINT | VERTARRAY | CLIPPLANE */
        (*gc->procs.pickVertexArrayProcs)(gc);
        gc->dirtyMask &= ~__GL_DIRTY_VERTARRAY;
    }
    if (!texDone && (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LIGHTING))) {
        __glSSTGenericPickTextureEnables(gc);
        (*gc->procs.pickTextureProcs)(gc);
        (*gc->procs.pickCalcTextureProcs)(gc);
    }
    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LIGHTING)) {
        __WGLrc *rc;

        if (gc->modes.haveStencilBuffer)
            (*gc->stencilBuffer.pick)(gc, &gc->stencilBuffer);
        (*gc->procs.pickBufferProcs)(gc);
        rc = GC_imports_wscx(gc);
        if ((rc->flags & 1) && ((!rc->flags) & 0x10)) {
            rc->flags &= ~1u;
            if (((GLubyte (__fastcall *)(__GLcontext *, GLint, GLint, GLint, GLint))
                     gc->exports[__GL_EXP_NOTIFY_RESIZE])(gc, gc->transform.clipX0, gc->transform.clipY0,
                                                          gc->transform.clipX1, gc->transform.clipY1) == 0xf1)
                return;
        }
        (*gc->procs.pickTransformProcs)(gc);
        if (enables & __GL_LIGHTING_ENABLE) {
            __glValidateLighting(gc);
            (*gc->procs.pickColorMaterialProcs)(gc);
            hwInitClipInterp(gc);
        } else {
            gc->procs.applyColor = (__GLproc)gc->hwcx.genApplyColor;
            gc->procs.calcRasterColor = (__GLproc)NopSpanRender;
            if (((*gc->polygon.shader.getModeFlags)(gc) & __GL_SHADE_CHEAP_FOG)
                && ((*gc->polygon.shader.getModeFlags)(gc) & __GL_SHADE_SMOOTH_LIGHT)
                && gc->renderMode == GL_RENDER) {
                gc->procs.calcColor = (__GLproc)FogRGBColor;
            } else {
                gc->procs.calcColor = (__GLproc)NopSpanRender;
            }
        }
        (*gc->procs.pickBlendProcs)(gc);
        (*gc->procs.pickFogProcs)(gc);
        if ((gc->dirtyMask & (__GL_DIRTY_SHADEMODEL_ICD | __GL_DIRTY_TEXENABLE_ICD | __GL_DIRTY_HWFOG_ICD | __GL_DIRTY_HWLIGHT_ICD)) || (gc->fx.flags & 1))
            (*gc->procs.pickParameterClipProcs)(gc);
        if (gc->dirtyMask & __GL_DIRTY_SHADEMODEL_ICD)
            (*gc->procs.pickClipProcs)(gc);
        if (gc->validateMask & 1) __glValidateAlphaTest(gc);
        if (gc->validateMask & 8) __glValidateIndexTest(gc);
        (*gc->procs.computeClipBox)(gc);
    }
    if (gc->dirtyMask & __GL_DIRTY_POLYGON_STIPPLE)
        (*gc->procs.convertPolygonStipple)(gc);
    if (gc->dirtyMask & __GL_DIRTY_DEPTH)
        (*gc->procs.pickAllProcsPhase1)(gc);
    if (gc->dirtyMask & __GL_DIRTY_SCISSOR) {
        (*gc->procs.computeClipBox)(gc);
        (*gc->procs.applyScissor)(gc);
    }
    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_POLYGON | __GL_DIRTY_CLIPPLANE_ICD)) {
        if (gc->state.polygon.frontMode == gc->state.polygon.backMode
            && gc->state.polygon.frontMode == GL_FILL
            && !(gc->state.enables.general & (__GL_LIGHTING_ENABLE | __GL_POLYGON_OFFSET_ANY_ENABLE))
            && gc->renderMode == GL_RENDER
            && !(gc->state.enables.general & __GL_FOG_ENABLE)) {
            gc->procs.drawIndexedTriangles = (__GLproc)CALL1(hwBuildIndexedTriangles, gc);
        } else {
            gc->procs.drawIndexedTriangles = (__GLproc)hwDrawIndexedTriangles;
        }
    }
    /* SHADEMODEL | HWDEPTH | NONUNITYQ | COLORARRAY | TEXENABLE */
    if (gc->dirtyMask & 0xec000) hwBuildFifoTriProcs(gc);
    /* SHADEMODEL | NONUNITYQ | TEXENABLE | HWFOG | HWLIGHT | 0x400000 (never set) */
    if (gc->dirtyMask & 0x7a4000) og_buildClipInterp(gc);
    if (CALL1(hwCanUseFastPath, gc)) {
        /* SHADEMODEL | FACE | NONUNITYQ | TEXENABLE | HWFOG | HWLIGHT | MVPTYPE | TEXMATRIX | TEXGEN_S..Q */
        if ((gc->dirtyMask & 0x1fbb4000) || (gc->fx.flags & 2)) {
            SET_IMM(gc, 0x208, 0, hwLazyVertex2fv);
            SET_IMM(gc, 0x228, 1, hwLazyVertex3fv);
            SET_IMM(gc, 0x248, 2, hwLazyVertex4fv);
            SET_IMM(gc, 0x20, 3, gc->hwcx.genCode[7]);
            SET_IMM(gc, 0xb0, 4, gc->hwcx.genCode[8]);
        }
        CALL1(hwPickRenderProcs, gc);
    } else {
        SET_IMM(gc, 0x20, 3, __glsstim_Begin);
        SET_IMM(gc, 0xb0, 4, __glsstim_End);
        SET_IMM(gc, 0x208, 0, __glim_Vertex2fv);
        SET_IMM(gc, 0x228, 1, __glim_Vertex3fv);
        SET_IMM(gc, 0x248, 2, __glim_Vertex4fv);
        (*gc->procs.pickVertexProcs)(gc);
        if (gc->dirtyMask & 0xaf) {   /* GENERIC | LINE | POLYGON | POINT | LIGHTING | DEPTH */
            (*gc->procs.pickTriangleProcs)(gc);
            (*gc->procs.pickPointProcs)(gc);
            (*gc->procs.pickLineProcs)(gc);
        }
    }
    if (gc->dirtyMask & 0x1621)   /* GENERIC | LIGHTING | VERTARRAY | MODELVIEW | TEXTURE */
        (*gc->procs.pickVertexArrayProcs)(gc);
    if (gc->hwcx.swTriangles | gc->hwcx.swPoints | gc->hwcx.swLines)
        CALL1(__glSSTRevalidate, gc);
    gc->dirtyMask = 0;
    gc->validateMask = 0;
    gc->fx.flags = 0;
}

/* 6901f660 (size 138) -- __glSSTValidateAlphaTest: the alpha test state into the Glide records
** grState[__GL_GR_ALPHA_TEST_FUNC] / [17] */
void __fastcall __glSSTValidateAlphaTest(__GLcontext *gc)
{
    if ((__GLfloat) *(GLuint *) gc->hwcx.grState[GR_ALPHAREF] != gc->state.raster.alphaReference) {
        *(GLuint *) gc->hwcx.grState[GR_ALPHAREF] = (GLuint) gc->state.raster.alphaReference;
        gc->hwcx.grDirty |= 1 << GR_ALPHAREF;
    }
    if (gc->state.enables.general & __GL_ALPHA_TEST_ENABLE) {
        GRSET(GR_ALPHATEST, 0, (GLint)(gc->state.raster.alphaFunction - GL_NEVER), 1 << GR_ALPHATEST);
    } else {
        GRSET(GR_ALPHATEST, 0, GR_CMP_ALWAYS, 1 << GR_ALPHATEST);
    }
}

/************************************************************************/

/* 6901f6f0 (size 569) -- __glSSTValidateBlend: the blend state (factors, equation, enable) into the Glide blend
** record grState[__GL_GR_ALPHA_BLEND] {src, dst, equation}; blending off loads ONE, ZERO */
/* GL blend factors -> Glide (no destination alpha: DST_ALPHA -> ONE, ONE_MINUS_DST_ALPHA / SRC_ALPHA_SATURATE -> ZERO).
** Also the NV_blend_square / GL 1.4 factors, source SRC_COLOR and destination DST_COLOR (and ONE_MINUS_), through
** Napalm's SAME_COLOR blend factors (as MesaFX; Heretic 2 water).  FALSE for an invalid factor. */
GLboolean __fastcall hwBlendFactors(__GLcontext *gc, GLenum sfactor, GLenum dfactor, GLuint *src, GLuint *dst)
{
    GLboolean same = gc->hwcx.hasAlphaBlendExt != 0;

    switch (sfactor) {
      case GL_ONE: *src = GR_BLEND_ONE; break;
      case GL_ZERO: *src = GR_BLEND_ZERO; break;
      case GL_SRC_ALPHA: *src = GR_BLEND_SRC_ALPHA; break;
      case GL_ONE_MINUS_SRC_ALPHA: *src = GR_BLEND_ONE_MINUS_SRC_ALPHA; break;
      case GL_DST_ALPHA: *src = gc->modes.alphaBits ? GR_BLEND_DST_ALPHA : GR_BLEND_ONE; break;
      case GL_DST_COLOR: *src = GR_BLEND_DST_COLOR; break;
      case GL_ONE_MINUS_DST_COLOR: *src = GR_BLEND_ONE_MINUS_DST_COLOR; break;
      case GL_ONE_MINUS_DST_ALPHA: *src = gc->modes.alphaBits ? GR_BLEND_ONE_MINUS_DST_ALPHA : GR_BLEND_ZERO; break;
      case GL_SRC_ALPHA_SATURATE: *src = gc->modes.alphaBits ? GR_BLEND_ALPHA_SATURATE : GR_BLEND_ZERO; break;
      case GL_SRC_COLOR: *src = same ? GR_BLEND_SAME_COLOR_EXT : GR_BLEND_ONE; break;
      case GL_ONE_MINUS_SRC_COLOR: *src = same ? GR_BLEND_ONE_MINUS_SAME_COLOR_EXT : GR_BLEND_ONE; break;
      default: return GL_FALSE;
    }
    switch (dfactor) {
      case GL_ONE: *dst = GR_BLEND_ONE; break;
      case GL_ZERO: *dst = GR_BLEND_ZERO; break;
      case GL_SRC_COLOR: *dst = GR_BLEND_SRC_COLOR; break;
      case GL_ONE_MINUS_SRC_COLOR: *dst = GR_BLEND_ONE_MINUS_SRC_COLOR; break;
      case GL_SRC_ALPHA: *dst = GR_BLEND_SRC_ALPHA; break;
      case GL_ONE_MINUS_SRC_ALPHA: *dst = GR_BLEND_ONE_MINUS_SRC_ALPHA; break;
      case GL_DST_ALPHA: *dst = gc->modes.alphaBits ? GR_BLEND_DST_ALPHA : GR_BLEND_ONE; break;
      case GL_ONE_MINUS_DST_ALPHA: *dst = gc->modes.alphaBits ? GR_BLEND_ONE_MINUS_DST_ALPHA : GR_BLEND_ZERO; break;
      case GL_DST_COLOR: *dst = same ? GR_BLEND_SAME_COLOR_EXT : GR_BLEND_ZERO; break;
      case GL_ONE_MINUS_DST_COLOR: *dst = same ? GR_BLEND_ONE_MINUS_SAME_COLOR_EXT : GR_BLEND_ZERO; break;
      default: return GL_FALSE;
    }
    return GL_TRUE;
}

void __fastcall __glSSTValidateBlend(__GLcontext *gc)
{
    GLuint src, dst;

    if (!hwBlendFactors(gc, gc->state.raster.blendSrc, gc->state.raster.blendDst, &src, &dst)) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->hwcx.blendSrcHw = src;
    gc->hwcx.blendDstHw = dst;
    switch (gc->state.raster.blendEquation) {
      case 0x800b: gc->hwcx.blendEquationHw = 2; break;   /* GL_FUNC_REVERSE_SUBTRACT */
      case 0x800a: gc->hwcx.blendEquationHw = 1; break;   /* GL_FUNC_SUBTRACT */
      case 0x8006: gc->hwcx.blendEquationHw = 0; break;   /* GL_FUNC_ADD */
    }
    if (gc->state.enables.general & __GL_BLEND_ENABLE) {
        GRSET(0, 0, src, 1);
        GRSET(0, 1, dst, 1);
        GRSET(0, 2, gc->hwcx.blendEquationHw, 1);
    } else {
        GRSET(0, 0, GR_BLEND_ONE, 1);
        GRSET(0, 1, GR_BLEND_ZERO, 1);
        GRSET(0, 2, 0, 1);
    }
}

/************************************************************************/

/* 6901f940 (size 385) -- hwLoadColorMask: glColorMask into the Glide colour-mask record
** grState[__GL_GR_COLOR_MASK] {rgb, r, g, b, a} (the per-channel mask only with 32-bit colour; else
** all-or-nothing) */
void __fastcall hwLoadColorMask(__GLcontext *gc)
{
    GLboolean r = gc->state.raster.rMask;
    GLboolean g = gc->state.raster.gMask;
    GLboolean b = gc->state.raster.bMask;
    GLboolean a = gc->state.raster.aMask;

    if (gc->hwcx.has32bpp) {
        GRSET(19, 1, r, 0x80000);
        GRSET(19, 2, g, 0x80000);
        GRSET(19, 3, b, 0x80000);
        GRSET(19, 4, a, 0x80000);
    } else if (r == g && g == b) {
        GRSET(19, 0, r, 0x80000);
        GRSET(19, 4, a, 0x80000);
    } else {
        GRSET(19, 0, 1, 0x80000);
        GRSET(19, 4, 1, 0x80000);
    }
}

/* 6901fad0 -- __glSSTValidateDepthTest: the depth test / function / write mask into the Glide state
** records 2, 1, 3 */
void __fastcall __glSSTValidateDepthTest(__GLcontext *gc)
{
  if (gc->state.enables.general & __GL_DEPTH_TEST_ENABLE) {
    if (*gc->hwcx.grState[__GL_GR_DEPTH_MODE] != GR_DEPTHBUFFER_ZBUFFER) {
      *gc->hwcx.grState[__GL_GR_DEPTH_MODE] = GR_DEPTHBUFFER_ZBUFFER;
      gc->hwcx.grDirty |= 1 << 2;
    }
  }
  else {
    if (*gc->hwcx.grState[__GL_GR_DEPTH_MODE] != GR_DEPTHBUFFER_DISABLE) {
      *gc->hwcx.grState[__GL_GR_DEPTH_MODE] = GR_DEPTHBUFFER_DISABLE;
      gc->hwcx.grDirty |= 1 << 2;
    }
  }
  if (*gc->hwcx.grState[__GL_GR_DEPTH_FUNC] != (GLint)(gc->state.depth.testFunc - GL_NEVER)) {
    *gc->hwcx.grState[__GL_GR_DEPTH_FUNC] = gc->state.depth.testFunc - GL_NEVER;
    gc->hwcx.grDirty |= 1 << 1;
  }
  if (*gc->hwcx.grState[__GL_GR_DEPTH_MASK] != gc->state.depth.writeEnable) {
    *gc->hwcx.grState[__GL_GR_DEPTH_MASK] = gc->state.depth.writeEnable;
    gc->hwcx.grDirty |= 1 << 3;
  }
}
/* 6901fb60 -- hwValidateDrawBuffer: the draw buffer to the hardware: windowed -> select the surface, else Glide's
** render buffer state (grState[__GL_GR_RENDER_BUFFER]: 0 front, 1 back) */
void __fastcall hwValidateDrawBuffer(__GLcontext *gc, GLuint edx_unused)
{
  switch (gc->state.raster.drawBuffer) {
  case GL_FRONT_LEFT:
  case GL_FRONT:
    if (gc->hwcx.windowed != 0) {
      ((void (__fastcall *)(__GLcontext *))winSetFrontSurface)(gc);
      return;
    }
    if (*gc->hwcx.grState[__GL_GR_RENDER_BUFFER] != 0) {
      *gc->hwcx.grState[__GL_GR_RENDER_BUFFER] = 0;
      gc->hwcx.grDirty |= 1 << 14;
    }
    break;
  case GL_BACK_LEFT:
  case GL_BACK:
    if (gc->hwcx.windowed != 0) {
      ((void (__fastcall *)(__GLcontext *))winSetBackSurface)(gc);
      return;
    }
    if (*gc->hwcx.grState[__GL_GR_RENDER_BUFFER] != 1) {
      *gc->hwcx.grState[__GL_GR_RENDER_BUFFER] = 1;
      gc->hwcx.grDirty |= 1 << 14;
    }
    break;
  }
}

/************************************************************************/

/* 6901fbf0 (size 314) -- hwValidateFog: the fog state into the Glide fog records grState[__GL_GR_FOG_COLOR]
** (colour) and [25] {mode, density, start, end} */
void __fastcall hwValidateFog(__GLcontext *gc)
{
    union { GLuint u; GLubyte b[4]; } fc;

    fc.b[3] = (GLubyte) gc->state.fog.color.a;
    fc.b[2] = (GLubyte) gc->state.fog.color.r;
    fc.b[1] = (GLubyte) gc->state.fog.color.g;
    fc.b[0] = (GLubyte) gc->state.fog.color.b;
    if (*(GLuint *) gc->hwcx.grState[GR_FOGCOLOR] != fc.u) {
        *(GLuint *) gc->hwcx.grState[GR_FOGCOLOR] = fc.u;
        gc->hwcx.grDirty |= 1 << GR_FOGCOLOR;
    }
    if (*(GLenum *) gc->hwcx.grState[GR_FOG] != gc->state.fog.mode) {
        *(GLenum *) gc->hwcx.grState[GR_FOG] = gc->state.fog.mode;
        gc->hwcx.grDirty |= 1 << GR_FOG;
    }
    switch (gc->state.fog.mode) {
      case GL_EXP:
      case GL_EXP2:
        if (((GLfloat *) gc->hwcx.grState[GR_FOG])[1] != gc->state.fog.density) {
            ((GLfloat *) gc->hwcx.grState[GR_FOG])[1] = gc->state.fog.density;
            gc->hwcx.grDirty |= 1 << GR_FOG;
        }
        break;
      case GL_LINEAR:
        if (((GLfloat *) gc->hwcx.grState[GR_FOG])[2] != gc->state.fog.start) {
            ((GLfloat *) gc->hwcx.grState[GR_FOG])[2] = gc->state.fog.start;
            gc->hwcx.grDirty |= 1 << GR_FOG;
        }
        if (((GLfloat *) gc->hwcx.grState[GR_FOG])[3] != gc->state.fog.end) {
            ((GLfloat *) gc->hwcx.grState[GR_FOG])[3] = gc->state.fog.end;
            gc->hwcx.grDirty |= 1 << GR_FOG;
        }
        break;
    }
}

/* 69020010 -- hwLoadAllState: loads the whole GL state into the Glide state records (make-current): both units'
** texture caches invalidated, the colour entry points (6901c400), alpha test, blend, colour mask, depth (6901fad0),
** draw buffer (6901fb60), fog and stencil, the dither mode (GR_DITHER_4x4 / GR_DITHER_DISABLE), then Glide's line /
** point smoothing enables (0x10002 / 0x10001, as glEnable) */
void __fastcall hwLoadAllState(__GLcontext *gc)
{
    gc->hwcx.unitTexture[0] = (void *)-1;
    gc->hwcx.unitTexture[1] = (void *)-1;
    hwPickColorProcs(gc);
    __glSSTValidateAlphaTest(gc);
    __glSSTValidateBlend(gc);
    hwLoadColorMask(gc);
    __glSSTValidateDepthTest(gc);
    ((void (__fastcall *)(__GLcontext *))hwValidateDrawBuffer)(gc);
    hwValidateFog(gc);
    hwValidateStencil(gc);
    if (gc->state.enables.general & __GL_DITHER_ENABLE) {
        if (*gc->hwcx.grState[__GL_GR_DITHER] != GR_DITHER_4x4) {
            *gc->hwcx.grState[__GL_GR_DITHER] = GR_DITHER_4x4;
            gc->hwcx.grDirty |= 1 << __GL_GR_DITHER;
        }
    }
    else {
        if (*gc->hwcx.grState[__GL_GR_DITHER] != GR_DITHER_DISABLE) {
            *gc->hwcx.grState[__GL_GR_DITHER] = GR_DITHER_DISABLE;
            gc->hwcx.grDirty |= 1 << __GL_GR_DITHER;
        }
    }
    if (gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE) {
        grEnable(0x10002);
    }
    else {
        grDisable(0x10002);
    }
    if (gc->state.enables.general & __GL_POINT_SMOOTH_ENABLE) {
        grEnable(0x10001);
    }
    else {
        grDisable(0x10001);
    }
}

/* 69026f60 -- hwDirtyTexCombine: marks the texture / combine Glide state dirty */
void __fastcall hwDirtyTexCombine(__GLcontext *gc)
{
  gc->hwcx.grDirty |= 0x30071;
  /* the original loads hasCombineExt only after the grDirty store (not hoisted): a volatile read */
  if (*(volatile GLint *)&gc->hwcx.hasCombineExt == 0) {
    gc->hwcx.grDirty |= ((1 << __GL_GR_TMU0_COMBINE) | (1 << __GL_GR_COLOR_COMBINE) | (1 << __GL_GR_ALPHA_COMBINE));
  }
  else {
    gc->hwcx.grDirtyExt |= 0x4f;
  }
  __gl_beginMode = __GL_NEED_VALIDATE;
  gc->hwcx.unitTexture[1] = NULL;
  gc->dirtyMask |= __GL_DIRTY_HWTEXTURE_ICD;
  gc->hwcx.flag_355be = 0;
}

/* 6904d420 -- hwValidateFinish: the end of the hardware path's validation, by the dirty bits: the texture enables
** and procs (0x801), the generated vertex code (0xec000: 6902eaa0, 0x4a4000: 6905b650), 6902ad50 always, the vertex
** arrays grown to the locked range (fx.flags 8), the triangle code (690312c0; dirty 0x200d8004 or fx.flags 0x30);
** then dirtyMask and fx.flags are cleared and the Glide state records flushed (69029070) */
void __fastcall hwValidateFinish(__GLcontext *gc)
{
    if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_HWTEXTURE_ICD)) {
        __glSSTGenericPickTextureEnables(gc);
        (*gc->procs.pickTextureProcs)(gc);
    }
    if (gc->dirtyMask & 0xec000) {   /* SHADEMODEL | HWDEPTH | NONUNITYQ | COLORARRAY | TEXENABLE */
        hwBuildFifoTriProcs(gc);
    }
    if (gc->dirtyMask & 0x4a4000) {   /* SHADEMODEL | NONUNITYQ | TEXENABLE | 0x400000 (never set) */
        og_buildClipInterp(gc);
    }
    og_genOffsetFifo(gc);
    if ((GLubyte)gc->fx.flags & 8) {
        hwGrowVertexArrays(gc, gc->vertexArray.lockCount + gc->vertexArray.lockFirst);
    }
    /* POLYGON | HWDEPTH | FACE | COLORARRAY | TEXENABLE | HWVERTEXBUF */
    if ((gc->dirtyMask & 0x200d8004) || ((GLubyte)gc->fx.flags & 0x30)) {
        ((void (__fastcall *)(__GLcontext *, int))hwGenLockedTriangles)(gc, 4);
    }
    gc->dirtyMask = 0;
    gc->fx.flags = 0;
    ((void (__fastcall *)(__GLcontext *))hwFlushGrState)(gc);
}

/* 6901a940 -- __glSSTRevalidate: procs.revalidate (hardware and software): the hardware state validation
** (690193b0), then re-pick the buffer / store / bitmap / span / pixel procs from the dirty bits saved before the
** flush (the SI __glGenericPickAllProcs tests on dirtyMaskSaved); EDX passes through unchanged */
void __fastcall __glSSTRevalidate(__GLcontext *gc, int arg)
{
    GLuint dirty = gc->dirtyMaskSaved;

    __glSSTGenericPickTextureProcs(gc, arg);
    if (dirty & (__GL_DIRTY_GENERIC | __GL_DIRTY_LIGHTING)) {
        (*gc->front->pick)(gc, gc->front);
        if (gc->modes.doubleBufferMode) {
            (*gc->back->pick)(gc, gc->back);
        }
        (*gc->procs.pickStoreProcs)(gc);
        (*gc->procs.pickRenderBitmapProcs)(gc);
    }
    if (dirty & (__GL_DIRTY_GENERIC | __GL_DIRTY_POLYGON | __GL_DIRTY_LIGHTING | __GL_DIRTY_DEPTH)) {
        (*gc->procs.pickSpanProcs)(gc);
    }
    if (dirty & (__GL_DIRTY_GENERIC | __GL_DIRTY_PIXEL | __GL_DIRTY_LIGHTING)) {
        (*gc->procs.pickPixelProcs)(gc);
    }
    gc->dirtyMaskSaved = 0;
}

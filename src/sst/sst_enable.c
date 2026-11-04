/*
** sst_enable.c -- glEnable / glDisable of the hardware (Glide) path, in the shape of
** the SGI SI's __glim_Enable / __glim_Disable (s_attrib.c): one switch over
** the capability, the enable bit in gc->state.enables, the Glide state record of the capability
** (hwcx.grState[i] / grDirty bit i) where the hardware has one, and the procs to re-pick.
** Case order = the original's body order (VC6 lays the cases of each dispatch range out in source order).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "icdprotos.h"
#include <glide.h>

/* 6901dce0 (size 2105) -- __glsstim_Enable: glEnable */
void __stdcall __glsstim_Enable(GLenum cap)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (cap) {
      case GL_COLOR_MATERIAL:
        gc->state.enables.general |= __GL_COLOR_MATERIAL_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
        (*gc->procs.pickColorMaterialProcs)(gc);
        (*gc->procs.applyColor)(gc);
        hwPickColorProcs(gc);
        return;
      case GL_CULL_FACE:
        if (gc->state.enables.general & __GL_CULL_FACE_ENABLE) return;
        gc->state.enables.general |= __GL_CULL_FACE_ENABLE;
        wglGlideSetupBufferModes(gc);
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
        return;
      case GL_DEPTH_TEST:
        gc->state.enables.general |= __GL_DEPTH_TEST_ENABLE;
        if (*gc->hwcx.grState[__GL_GR_DEPTH_MODE] != GR_DEPTHBUFFER_ZBUFFER) {
            *gc->hwcx.grState[__GL_GR_DEPTH_MODE] = GR_DEPTHBUFFER_ZBUFFER;
            gc->hwcx.grDirty |= 1 << 2;
        }
        gc->dirtyMask |= __GL_DIRTY_HWDEPTH_ICD;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
        (*gc->procs.pickAllProcsPhase1)(gc);
        (*gc->procs.pickBufferProcs)(gc);
        break;
      case GL_FOG:
        gc->state.enables.general |= __GL_FOG_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_HWFOG_ICD;
        (*gc->procs.pickFogProcs)(gc);
        break;
      case GL_LIGHTING:
        gc->state.enables.general |= __GL_LIGHTING_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_HWLIGHT_ICD;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
        (*gc->procs.pickColorMaterialProcs)(gc);
        (*gc->procs.applyColor)(gc);
        return;
      case GL_LINE_SMOOTH:
        gc->state.enables.general |= __GL_LINE_SMOOTH_ENABLE;
        grEnable(0x10002);
        break;
      case GL_LINE_STIPPLE:
        gc->state.enables.general |= __GL_LINE_STIPPLE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
        return;
      case GL_POINT_SMOOTH:
        gc->state.enables.general |= __GL_POINT_SMOOTH_ENABLE;
        grEnable(0x10001);
        break;
      case GL_POLYGON_SMOOTH:
        gc->state.enables.general |= __GL_POLYGON_SMOOTH_ENABLE;
        break;
      case GL_POLYGON_STIPPLE:
        gc->state.enables.general |= __GL_POLYGON_STIPPLE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
        return;
      case GL_STENCIL_TEST:
        gc->state.enables.general |= __GL_STENCIL_TEST_ENABLE;
        if (gc->hwcx.has32bpp) grEnable(7 /* GR_STENCIL_MODE_EXT (g3ext.h) */);
        (*gc->procs.pickBufferProcs)(gc);
        break;
      case GL_ALPHA_TEST:
        gc->state.enables.general |= __GL_ALPHA_TEST_ENABLE;
        if (*gc->hwcx.grState[__GL_GR_ALPHA_TEST_FUNC] != (GLint) (gc->state.raster.alphaFunction - GL_NEVER)) {
            *gc->hwcx.grState[__GL_GR_ALPHA_TEST_FUNC] = gc->state.raster.alphaFunction - GL_NEVER;   /* GL_NEVER.. = GR_CMP_NEVER.. */
            gc->hwcx.grDirty |= 1 << 16;
        }
        break;
      case GL_BLEND:
        gc->state.enables.general |= __GL_BLEND_ENABLE;
        __glSSTValidateBlend(gc);
        break;
      case GL_DITHER:
        gc->state.enables.general |= __GL_DITHER_ENABLE;
        if (*gc->hwcx.grState[__GL_GR_DITHER] != GR_DITHER_4x4) {
            *gc->hwcx.grState[__GL_GR_DITHER] = GR_DITHER_4x4;
            gc->hwcx.grDirty |= 1 << __GL_GR_DITHER;
        }
        break;
      case GL_INDEX_LOGIC_OP:
        gc->state.enables.general |= __GL_INDEX_LOGIC_OP_ENABLE;
        break;
      case GL_COLOR_LOGIC_OP:
        gc->state.enables.general |= __GL_COLOR_LOGIC_OP_ENABLE;
        break;
      case GL_NORMALIZE:
        gc->state.enables.general |= __GL_NORMALIZE_ENABLE;
        break;
      case GL_SCISSOR_TEST:
        if (gc->state.enables.general & __GL_SCISSOR_TEST_ENABLE) return;
        gc->state.enables.general |= __GL_SCISSOR_TEST_ENABLE;
        (*gc->procs.computeClipBox)(gc);
        (*gc->procs.applyScissor)(gc);
        break;
      case GL_TEXTURE_GEN_S:
        gc->state.enables.texture[gc->fx.activeTexture] |= __GL_TEXTURE_GEN_S_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_TEXGEN_S_ICD;
        break;
      case GL_TEXTURE_GEN_T:
        gc->state.enables.texture[gc->fx.activeTexture] |= __GL_TEXTURE_GEN_T_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_TEXGEN_S_ICD << 1;
        break;
      case GL_TEXTURE_GEN_R:
        gc->state.enables.texture[gc->fx.activeTexture] |= __GL_TEXTURE_GEN_R_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_TEXGEN_S_ICD << 2;
        break;
      case GL_TEXTURE_GEN_Q:
        gc->state.enables.texture[gc->fx.activeTexture] |= __GL_TEXTURE_GEN_Q_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_TEXGEN_S_ICD << 3;
        break;
      case GL_TEXTURE_1D:
        gc->state.enables.texture[gc->fx.activeTexture] |= __GL_TEXTURE_1D_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_TEXTURE_ICD);
        break;
      case GL_TEXTURE_2D:
        gc->state.enables.texture[gc->fx.activeTexture] |= __GL_TEXTURE_2D_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_TEXTURE_ICD);
        break;
      case GL_AUTO_NORMAL:
        gc->state.enables.general |= __GL_AUTO_NORMAL_ENABLE;
        break;
      case GL_MAP1_COLOR_4: case GL_MAP1_INDEX: case GL_MAP1_NORMAL:
      case GL_MAP1_TEXTURE_COORD_1: case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3: case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3: case GL_MAP1_VERTEX_4:
        gc->state.enables.eval1 |= (GLushort) (1 << (cap - GL_MAP1_COLOR_4));
        break;
      case GL_MAP2_COLOR_4: case GL_MAP2_INDEX: case GL_MAP2_NORMAL:
      case GL_MAP2_TEXTURE_COORD_1: case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3: case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3: case GL_MAP2_VERTEX_4:
        gc->state.enables.eval2 |= (GLushort) (1 << (cap - GL_MAP2_COLOR_4));
        break;
      case GL_POLYGON_OFFSET_POINT:
        gc->state.enables.general |= __GL_POLYGON_OFFSET_POINT_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POINT);
        break;
      case GL_CLIP_PLANE0: case GL_CLIP_PLANE1: case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3: case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
        gc->state.enables.clipPlanes |= 1 << (cap - GL_CLIP_PLANE0);
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_CLIPPLANE_ICD);
        return;
      case GL_POLYGON_OFFSET_LINE:
        gc->state.enables.general |= __GL_POLYGON_OFFSET_LINE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
        break;
      case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
      case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
        gc->state.enables.lights |= 1 << (cap - GL_LIGHT0);
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
        return;
      case GL_POLYGON_OFFSET_FILL:
        gc->state.enables.general |= __GL_POLYGON_OFFSET_FILL_ENABLE;
        gc->fx.flags |= 4;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
        break;
      case GL_VERTEX_ARRAY:
        glEnableClientState(GL_VERTEX_ARRAY);
        return;
      case GL_NORMAL_ARRAY:
        glEnableClientState(GL_NORMAL_ARRAY);
        return;
      case GL_COLOR_ARRAY:
        glEnableClientState(GL_COLOR_ARRAY);
        return;
      case GL_INDEX_ARRAY:
        glEnableClientState(GL_INDEX_ARRAY);
        return;
      case GL_TEXTURE_COORD_ARRAY:
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        return;
      case GL_EDGE_FLAG_ARRAY:
        glEnableClientState(GL_EDGE_FLAG_ARRAY);
        return;
      case 0x81aa:                                  /* GL_CULL_VERTEX_EXT */
        gc->state.enables.general |= __GL_CULL_VERTEX_ENABLE;
        break;
      case 0x81b8:                                  /* GL_INDEX_MATERIAL_EXT */
        gc->state.enables.general |= __GL_INDEX_MATERIAL_EXT_ENABLE;
        break;
      case 0x81b5:                                  /* GL_INDEX_TEST_EXT */
        gc->state.enables.general |= __GL_INDEX_TEST_EXT_ENABLE;
        break;
      case 0x81fb:                                  /* GL_SHARED_TEXTURE_PALETTE_EXT */
        gc->state.enables.general |= __GL_SHARED_TEXTURE_PALETTE_EXT_ENABLE;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
      case 0x86b2:                                  /* multisample (3dfx T-buffer) */
        if (gc->hwcx.hasTBuffer && !gc->hwcx.windowed && gc->fx.numSamples > 0)
            grEnable(0x80000002);
        break;
    }
    __GL_DELAY_VALIDATE(gc);
}

/* 6901e730 (size 2214) -- __glsstim_Disable: glDisable */
void __stdcall __glsstim_Disable(GLenum cap)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (cap) {
      case GL_COLOR_MATERIAL:
        gc->state.enables.general &= ~__GL_COLOR_MATERIAL_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
        (*gc->procs.pickColorMaterialProcs)(gc);
        hwPickColorProcs(gc);
        return;
      case GL_CULL_FACE:
        if (!(gc->state.enables.general & __GL_CULL_FACE_ENABLE)) return;
        gc->state.enables.general &= ~__GL_CULL_FACE_ENABLE;
        wglGlideSetupBufferModes(gc);
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
        return;
      case GL_DEPTH_TEST:
        gc->state.enables.general &= ~__GL_DEPTH_TEST_ENABLE;
        if (*gc->hwcx.grState[__GL_GR_DEPTH_MODE] != GR_DEPTHBUFFER_DISABLE) {
            *gc->hwcx.grState[__GL_GR_DEPTH_MODE] = GR_DEPTHBUFFER_DISABLE;
            gc->hwcx.grDirty |= 1 << 2;
        }
        gc->dirtyMask |= __GL_DIRTY_HWDEPTH_ICD;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
        (*gc->procs.pickAllProcsPhase1)(gc);
        (*gc->procs.pickBufferProcs)(gc);
        break;
      case GL_FOG:
        gc->state.enables.general &= ~__GL_FOG_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_HWFOG_ICD;
        (*gc->procs.pickFogProcs)(gc);
        break;
      case GL_LIGHTING:
        gc->state.enables.general &= ~__GL_LIGHTING_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_HWLIGHT_ICD;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
        (*gc->procs.pickColorMaterialProcs)(gc);
        (*gc->procs.applyColor)(gc);
        return;
      case GL_LINE_SMOOTH:
        gc->state.enables.general &= ~__GL_LINE_SMOOTH_ENABLE;
        grDisable(0x10002);
        break;
      case GL_LINE_STIPPLE:
        gc->state.enables.general &= ~__GL_LINE_STIPPLE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
        return;
      case GL_POINT_SMOOTH:
        gc->state.enables.general &= ~__GL_POINT_SMOOTH_ENABLE;
        grDisable(0x10001);
        break;
      case GL_POLYGON_SMOOTH:
        gc->state.enables.general &= ~__GL_POLYGON_SMOOTH_ENABLE;
        break;
      case GL_POLYGON_STIPPLE:
        gc->state.enables.general &= ~__GL_POLYGON_STIPPLE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
        return;
      case GL_STENCIL_TEST:
        gc->state.enables.general &= ~__GL_STENCIL_TEST_ENABLE;
        if (gc->hwcx.has32bpp) grDisable(7 /* GR_STENCIL_MODE_EXT (g3ext.h) */);
        (*gc->procs.pickBufferProcs)(gc);
        break;
      case GL_ALPHA_TEST:
        gc->state.enables.general &= ~__GL_ALPHA_TEST_ENABLE;
        if (*gc->hwcx.grState[__GL_GR_ALPHA_TEST_FUNC] != GR_CMP_ALWAYS) {
            *gc->hwcx.grState[__GL_GR_ALPHA_TEST_FUNC] = GR_CMP_ALWAYS;
            gc->hwcx.grDirty |= 1 << 16;
        }
        break;
      case GL_BLEND:
        gc->state.enables.general &= ~__GL_BLEND_ENABLE;
        if (gc->hwcx.grState[__GL_GR_ALPHA_BLEND][0] != GR_BLEND_ONE) {
            gc->hwcx.grState[__GL_GR_ALPHA_BLEND][0] = GR_BLEND_ONE;
            gc->hwcx.grDirty |= 1 << 0;
        }
        if (gc->hwcx.grState[__GL_GR_ALPHA_BLEND][1] != GR_BLEND_ZERO) {
            gc->hwcx.grState[__GL_GR_ALPHA_BLEND][1] = GR_BLEND_ZERO;
            gc->hwcx.grDirty |= 1 << 0;
        }
        if (gc->hwcx.grState[__GL_GR_ALPHA_BLEND][2] != GR_BLEND_ZERO) {
            gc->hwcx.grState[__GL_GR_ALPHA_BLEND][2] = GR_BLEND_ZERO;
            gc->hwcx.grDirty |= 1 << 0;
        }
        break;
      case GL_DITHER:
        gc->state.enables.general &= ~__GL_DITHER_ENABLE;
        if (*gc->hwcx.grState[__GL_GR_DITHER] != GR_DITHER_DISABLE) {
            *gc->hwcx.grState[__GL_GR_DITHER] = GR_DITHER_DISABLE;
            gc->hwcx.grDirty |= 1 << __GL_GR_DITHER;
        }
        break;
      case GL_INDEX_LOGIC_OP:
        gc->state.enables.general &= ~__GL_INDEX_LOGIC_OP_ENABLE;
        break;
      case GL_COLOR_LOGIC_OP:
        gc->state.enables.general &= ~__GL_COLOR_LOGIC_OP_ENABLE;
        break;
      case GL_NORMALIZE:
        gc->state.enables.general &= ~__GL_NORMALIZE_ENABLE;
        break;
      case GL_SCISSOR_TEST:
        gc->state.enables.general &= ~__GL_SCISSOR_TEST_ENABLE;
        (*gc->procs.computeClipBox)(gc);
        (*gc->procs.applyScissor)(gc);
        break;
      case GL_TEXTURE_GEN_S:
        gc->state.enables.texture[gc->fx.activeTexture] &= ~__GL_TEXTURE_GEN_S_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_TEXGEN_S_ICD;
        break;
      case GL_TEXTURE_GEN_T:
        gc->state.enables.texture[gc->fx.activeTexture] &= ~__GL_TEXTURE_GEN_T_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_TEXGEN_S_ICD << 1;
        break;
      case GL_TEXTURE_GEN_R:
        gc->state.enables.texture[gc->fx.activeTexture] &= ~__GL_TEXTURE_GEN_R_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_TEXGEN_S_ICD << 2;
        break;
      case GL_TEXTURE_GEN_Q:
        gc->state.enables.texture[gc->fx.activeTexture] &= ~__GL_TEXTURE_GEN_Q_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_TEXGEN_S_ICD << 3;
        break;
      case GL_TEXTURE_1D:
        gc->state.enables.texture[gc->fx.activeTexture] &= ~__GL_TEXTURE_1D_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_TEXTURE_ICD);
        break;
      case GL_TEXTURE_2D:
        gc->state.enables.texture[gc->fx.activeTexture] &= ~__GL_TEXTURE_2D_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_TEXTURE_ICD);
        break;
      case GL_AUTO_NORMAL:
        gc->state.enables.general &= ~__GL_AUTO_NORMAL_ENABLE;
        break;
      case GL_MAP1_COLOR_4: case GL_MAP1_INDEX: case GL_MAP1_NORMAL:
      case GL_MAP1_TEXTURE_COORD_1: case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3: case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3: case GL_MAP1_VERTEX_4:
        gc->state.enables.eval1 &= (GLushort) ~(1 << (cap - GL_MAP1_COLOR_4));
        break;
      case GL_MAP2_COLOR_4: case GL_MAP2_INDEX: case GL_MAP2_NORMAL:
      case GL_MAP2_TEXTURE_COORD_1: case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3: case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3: case GL_MAP2_VERTEX_4:
        gc->state.enables.eval2 &= (GLushort) ~(1 << (cap - GL_MAP2_COLOR_4));
        break;
      case GL_POLYGON_OFFSET_POINT:
        gc->state.enables.general &= ~__GL_POLYGON_OFFSET_POINT_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POINT);
        break;
      case GL_CLIP_PLANE0: case GL_CLIP_PLANE1: case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3: case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
        gc->state.enables.clipPlanes &= ~(1 << (cap - GL_CLIP_PLANE0));
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_CLIPPLANE_ICD);
        return;
      case GL_POLYGON_OFFSET_LINE:
        gc->state.enables.general &= ~__GL_POLYGON_OFFSET_LINE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
        break;
      case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
      case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
        gc->state.enables.lights &= ~(1 << (cap - GL_LIGHT0));
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
        return;
      case GL_POLYGON_OFFSET_FILL:
        gc->state.enables.general &= ~__GL_POLYGON_OFFSET_FILL_ENABLE;
        gc->fx.flags |= 4;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
        break;
      case GL_VERTEX_ARRAY:
        glDisableClientState(GL_VERTEX_ARRAY);
        return;
      case GL_NORMAL_ARRAY:
        glDisableClientState(GL_NORMAL_ARRAY);
        return;
      case GL_COLOR_ARRAY:
        glDisableClientState(GL_COLOR_ARRAY);
        return;
      case GL_INDEX_ARRAY:
        glDisableClientState(GL_INDEX_ARRAY);
        return;
      case GL_TEXTURE_COORD_ARRAY:
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        return;
      case GL_EDGE_FLAG_ARRAY:
        glDisableClientState(GL_EDGE_FLAG_ARRAY);
        return;
      case 0x81aa:                                  /* GL_CULL_VERTEX_EXT */
        gc->state.enables.general &= ~__GL_CULL_VERTEX_ENABLE;
        break;
      case 0x81b8:                                  /* GL_INDEX_MATERIAL_EXT */
        gc->state.enables.general &= ~__GL_INDEX_MATERIAL_EXT_ENABLE;
        break;
      case 0x81b5:                                  /* GL_INDEX_TEST_EXT */
        gc->state.enables.general &= ~__GL_INDEX_TEST_EXT_ENABLE;
        break;
      case 0x81fb:                                  /* GL_SHARED_TEXTURE_PALETTE_EXT */
        gc->state.enables.general &= ~__GL_SHARED_TEXTURE_PALETTE_EXT_ENABLE;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
      case 0x86b2:                                  /* multisample (3dfx T-buffer) */
        if (gc->hwcx.hasTBuffer && !gc->hwcx.windowed && gc->fx.numSamples > 0)
            grDisable(0x80000002);
        break;
    }
    __GL_DELAY_VALIDATE(gc);
}

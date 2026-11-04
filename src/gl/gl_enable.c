/*
** gl_enable.c -- glEnable / glDisable of the software (generic) path, in the shape of
** the SGI SI's __glim_Enable / __glim_Disable (s_attrib.c): the enable bit,
** the validation mask, and the procs to re-pick.  The hardware path's versions are in src/sst/sst_enable.c.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "icdprotos.h"

/* 690b88e0 (size 1753) -- __glim_Enable: glEnable */
void __stdcall __glim_Enable(GLenum cap)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (cap) {
      case GL_COLOR_MATERIAL:
        gc->state.enables.general |= __GL_COLOR_MATERIAL_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
        (*gc->procs.pickColorMaterialProcs)(gc);
        (*gc->procs.applyColor)(gc);
        return;
      case GL_CULL_FACE:
        if (gc->state.enables.general & __GL_CULL_FACE_ENABLE) return;
        gc->state.enables.general |= __GL_CULL_FACE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
        return;
      case GL_DEPTH_TEST:
        gc->state.enables.general |= __GL_DEPTH_TEST_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_HWDEPTH_ICD;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
        break;
      case GL_FOG:
        gc->state.enables.general |= __GL_FOG_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_HWFOG_ICD;
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
        break;
      case GL_LINE_STIPPLE:
        gc->state.enables.general |= __GL_LINE_STIPPLE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
        return;
      case GL_POINT_SMOOTH:
        gc->state.enables.general |= __GL_POINT_SMOOTH_ENABLE;
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
        break;
      case GL_ALPHA_TEST:
        gc->state.enables.general |= __GL_ALPHA_TEST_ENABLE;
        break;
      case GL_BLEND:
        gc->state.enables.general |= __GL_BLEND_ENABLE;
        break;
      case GL_DITHER:
        gc->state.enables.general |= __GL_DITHER_ENABLE;
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
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_SCISSOR);
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
        break;
      case GL_TEXTURE_2D:
        gc->state.enables.texture[gc->fx.activeTexture] |= __GL_TEXTURE_2D_ENABLE;
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
        break;
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
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
      case 0x81b8:                                  /* GL_INDEX_MATERIAL_EXT */
        gc->state.enables.general |= __GL_INDEX_MATERIAL_EXT_ENABLE;
        break;
      case 0x81b5:                                  /* GL_INDEX_TEST_EXT */
        gc->state.enables.general |= __GL_INDEX_TEST_EXT_ENABLE;
        break;
    }
    __GL_DELAY_VALIDATE(gc);
}

/* 690b9150 (size 1783) -- __glim_Disable: glDisable */
void __stdcall __glim_Disable(GLenum cap)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (cap) {
      case GL_COLOR_MATERIAL:
        gc->state.enables.general &= ~__GL_COLOR_MATERIAL_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
        (*gc->procs.pickColorMaterialProcs)(gc);
        return;
      case GL_CULL_FACE:
        if (!(gc->state.enables.general & __GL_CULL_FACE_ENABLE)) return;
        gc->state.enables.general &= ~__GL_CULL_FACE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
        return;
      case GL_DEPTH_TEST:
        gc->state.enables.general &= ~__GL_DEPTH_TEST_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_HWDEPTH_ICD;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
        break;
      case GL_FOG:
        gc->state.enables.general &= ~__GL_FOG_ENABLE;
        gc->dirtyMask |= __GL_DIRTY_HWFOG_ICD;
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
        break;
      case GL_LINE_STIPPLE:
        gc->state.enables.general &= ~__GL_LINE_STIPPLE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
        return;
      case GL_POINT_SMOOTH:
        gc->state.enables.general &= ~__GL_POINT_SMOOTH_ENABLE;
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
        break;
      case GL_ALPHA_TEST:
        gc->state.enables.general &= ~__GL_ALPHA_TEST_ENABLE;
        break;
      case GL_BLEND:
        gc->state.enables.general &= ~__GL_BLEND_ENABLE;
        break;
      case GL_DITHER:
        gc->state.enables.general &= ~__GL_DITHER_ENABLE;
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
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_SCISSOR);
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
        break;
      case GL_TEXTURE_2D:
        gc->state.enables.texture[gc->fx.activeTexture] &= ~__GL_TEXTURE_2D_ENABLE;
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
        break;
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
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
      case 0x81b8:                                  /* GL_INDEX_MATERIAL_EXT */
        gc->state.enables.general &= ~__GL_INDEX_MATERIAL_EXT_ENABLE;
        break;
      case 0x81b5:                                  /* GL_INDEX_TEST_EXT */
        gc->state.enables.general &= ~__GL_INDEX_TEST_EXT_ENABLE;
        break;
    }
    __GL_DELAY_VALIDATE(gc);
}

/* 690b99f0 (size 1475) -- __glim_IsEnabled: glIsEnabled */
GLboolean __stdcall __glim_IsEnabled(GLenum cap)
{
    GLuint bit;
    __GL_SETUP_NOT_IN_BEGIN2();

    switch (cap) {
      case GL_COLOR_MATERIAL:
        bit = gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE;
        break;
      case GL_CULL_FACE:
        bit = gc->state.enables.general & __GL_CULL_FACE_ENABLE;
        break;
      case GL_DEPTH_TEST:
        bit = gc->state.enables.general & __GL_DEPTH_TEST_ENABLE;
        break;
      case GL_FOG:
        bit = gc->state.enables.general & __GL_FOG_ENABLE;
        break;
      case GL_LIGHTING:
        bit = gc->state.enables.general & __GL_LIGHTING_ENABLE;
        break;
      case GL_LINE_SMOOTH:
        bit = gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE;
        break;
      case GL_LINE_STIPPLE:
        bit = gc->state.enables.general & __GL_LINE_STIPPLE_ENABLE;
        break;
      case GL_POINT_SMOOTH:
        bit = gc->state.enables.general & __GL_POINT_SMOOTH_ENABLE;
        break;
      case GL_POLYGON_SMOOTH:
        bit = gc->state.enables.general & __GL_POLYGON_SMOOTH_ENABLE;
        break;
      case GL_POLYGON_STIPPLE:
        bit = gc->state.enables.general & __GL_POLYGON_STIPPLE_ENABLE;
        break;
      case GL_STENCIL_TEST:
        bit = gc->state.enables.general & __GL_STENCIL_TEST_ENABLE;
        break;
      case GL_ALPHA_TEST:
        bit = gc->state.enables.general & __GL_ALPHA_TEST_ENABLE;
        break;
      case GL_BLEND:
        bit = gc->state.enables.general & __GL_BLEND_ENABLE;
        break;
      case GL_DITHER:
        bit = gc->state.enables.general & __GL_DITHER_ENABLE;
        break;
      case GL_INDEX_LOGIC_OP:
        bit = gc->state.enables.general & __GL_INDEX_LOGIC_OP_ENABLE;
        break;
      case GL_COLOR_LOGIC_OP:
        bit = gc->state.enables.general & __GL_COLOR_LOGIC_OP_ENABLE;
        break;
      case GL_NORMALIZE:
        bit = gc->state.enables.general & __GL_NORMALIZE_ENABLE;
        break;
      case GL_SCISSOR_TEST:
        bit = gc->state.enables.general & __GL_SCISSOR_TEST_ENABLE;
        break;
      case GL_TEXTURE_GEN_S:
        bit = gc->state.enables.texture[gc->fx.activeTexture] & __GL_TEXTURE_GEN_S_ENABLE;
        break;
      case GL_TEXTURE_GEN_T:
        bit = gc->state.enables.texture[gc->fx.activeTexture] & __GL_TEXTURE_GEN_T_ENABLE;
        break;
      case GL_TEXTURE_GEN_R:
        bit = gc->state.enables.texture[gc->fx.activeTexture] & __GL_TEXTURE_GEN_R_ENABLE;
        break;
      case GL_TEXTURE_GEN_Q:
        bit = gc->state.enables.texture[gc->fx.activeTexture] & __GL_TEXTURE_GEN_Q_ENABLE;
        break;
      case GL_AUTO_NORMAL:
        bit = gc->state.enables.general & __GL_AUTO_NORMAL_ENABLE;
        break;
      case GL_TEXTURE_1D:
        bit = gc->state.enables.texture[gc->fx.activeTexture] & __GL_TEXTURE_1D_ENABLE;
        break;
      case GL_TEXTURE_2D:
        bit = gc->state.enables.texture[gc->fx.activeTexture] & __GL_TEXTURE_2D_ENABLE;
        break;
      case GL_MAP1_COLOR_4: case GL_MAP1_INDEX: case GL_MAP1_NORMAL:
      case GL_MAP1_TEXTURE_COORD_1: case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3: case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3: case GL_MAP1_VERTEX_4:
        bit = ((1 << (cap - GL_MAP1_COLOR_4)) & gc->state.enables.eval1);
        break;
      case GL_MAP2_COLOR_4: case GL_MAP2_INDEX: case GL_MAP2_NORMAL:
      case GL_MAP2_TEXTURE_COORD_1: case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3: case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3: case GL_MAP2_VERTEX_4:
        bit = ((1 << (cap - GL_MAP2_COLOR_4)) & gc->state.enables.eval2);
        break;
      case GL_POLYGON_OFFSET_POINT:
        bit = gc->state.enables.general & __GL_POLYGON_OFFSET_POINT_ENABLE;
        break;
      case GL_CLIP_PLANE0: case GL_CLIP_PLANE1: case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3: case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
        bit = ((1 << (cap - GL_CLIP_PLANE0)) & gc->state.enables.clipPlanes);
        break;
      case GL_POLYGON_OFFSET_LINE:
        bit = gc->state.enables.general & __GL_POLYGON_OFFSET_LINE_ENABLE;
        break;
      case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
      case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
        bit = ((1 << (cap - GL_LIGHT0)) & gc->state.enables.lights);
        break;
      case GL_POLYGON_OFFSET_FILL:
        bit = gc->state.enables.general & __GL_POLYGON_OFFSET_FILL_ENABLE;
        break;
      /* the client arrays: component masks of vertexArray.enables (glEnableClientState) */
      case GL_VERTEX_ARRAY:
        bit = gc->vertexArray.enables & 0xff;
        break;
      case GL_NORMAL_ARRAY:
        bit = gc->vertexArray.enables & 0xf00;
        break;
      case GL_COLOR_ARRAY:
        bit = gc->vertexArray.enables & 0xff000;
        break;
      case GL_INDEX_ARRAY:
        bit = gc->vertexArray.enables & 0xf0000000;
        break;
      case GL_TEXTURE_COORD_ARRAY:
        bit = (0x10000 << gc->fx.clientActiveTexture) & gc->vertexArray.enableFlags;
        break;
      case GL_EDGE_FLAG_ARRAY:
        bit = gc->vertexArray.enables & 0x8000000;
        break;
      case 0x81aa:   /* GL_CULL_VERTEX_EXT */
        bit = gc->state.enables.general & __GL_CULL_VERTEX_ENABLE;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return GL_FALSE;
      case GL_FOG_COORDINATE_ARRAY_EXT:
        bit = gc->vertexArray.enables & 0x1f00000;
        break;
      case 0x81b8:                                  /* GL_INDEX_MATERIAL_EXT */
        bit = gc->state.enables.general & __GL_INDEX_MATERIAL_EXT_ENABLE;
        break;
      case 0x81b5:                                  /* GL_INDEX_TEST_EXT */
        bit = gc->state.enables.general & __GL_INDEX_TEST_EXT_ENABLE;
        break;
    }
    return bit != 0;
}

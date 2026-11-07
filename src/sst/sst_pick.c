/*
** sst_pick.c -- the proc pickers of the hardware path (fog, lines, points, triangles, textures, vertex arrays,
** immediate procs, render and draw routines) and the shading mode flags / vertex needs they work from.
*/
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "siprotos.h"
#include "glshade.h"
#include "si_names.h"
#include "icdglob.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "og.h"
#include "icdprotos.h"
#include <stdio.h>
#include "gldisp.h"

/* 690eff3c -- procs.v by idx: 8 fog coordinate, 4 texturing, 2 or 1 colour */
extern void __fastcall __glSaveT(__GLcontext *gc, __GLvertexICD *vx);
extern void __fastcall __glSaveCT(__GLcontext *gc, __GLvertexICD *vx);
extern void __fastcall __glSaveNT(__GLcontext *gc, __GLvertexICD *vx);
extern void __fastcall __glSaveF(__GLcontext *gc, __GLvertexICD *vx);
extern void __fastcall __glSaveNFog(__GLcontext *gc, __GLvertexICD *vx);
extern void __fastcall __glSaveCFog(__GLcontext *gc, __GLvertexICD *vx);
extern void __fastcall __glSaveNCF(__GLcontext *gc, __GLvertexICD *vx);
extern void __fastcall __glSaveTF(__GLcontext *gc, __GLvertexICD *vx);
extern void __fastcall __glSaveNTF(__GLcontext *gc, __GLvertexICD *vx);
extern void __fastcall __glSaveCTF(__GLcontext *gc, __GLvertexICD *vx);
extern void __fastcall __glSaveCAll(__GLcontext *gc, __GLvertexICD *vx);
static __GLproc g_hwVertexProcs[16] = {
    NULL,
    (__GLproc)__glSaveN,
    (__GLproc)__glSaveC,
    NULL,
    (__GLproc)__glSaveT,
    (__GLproc)__glSaveNT,
    (__GLproc)__glSaveCT,
    (__GLproc)__glSaveCAll,
    (__GLproc)__glSaveF,
    (__GLproc)__glSaveNFog,
    (__GLproc)__glSaveCFog,
    (__GLproc)__glSaveNCF,
    (__GLproc)__glSaveTF,
    (__GLproc)__glSaveNTF,
    (__GLproc)__glSaveCTF,
    (__GLproc)__glSaveCAllFog,
};

#pragma warning(disable: 4273)
/* the fog procs (fogPoint, fogVertex variants) and the clear rects (ClearRectRGB, dithered 16-bit) */
typedef int (__fastcall *__GLGcFn)(__GLcontext *gc);
#define CALL1(f, gc)    ((__GLGcFn)(f))(gc)
/************************************************************************/
#define TEX_FX(x)               ((__GLtextureFxICD *)(x))
#define TEX_HW(x, s)        (TEX_FX(x)->t.hwHandle[s])         /* texture memory handle per TMU slot */
#define MODEFLAGS(gc)   (((GLuint (__fastcall *)(__GLcontext *))(gc)->polygon.shader.getModeFlags)(gc))
/************************************************************************/

/************************************************************************/

/* renderPoint variants (sst_linepoint.c, sst_render.c) */
#define TEX_PALFMT(x)       (TEX_FX(x)->t.CT.baseFormat)       /* the format of its own palette */
/************************************************************************/

/* s_pick.c __glGenericPickTextureProcs callees without SI names here */
/* rho: (gc, unit or shade, s, t, qw) */
/* level filters: (tex, level, s, t, result) */
/* texture filters: (tex, result, lod, s, t) */
extern void __fastcall __glNMNFilter(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod, __GLfloat s, __GLfloat t);
extern void __fastcall __glLMNFilter(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod, __GLfloat s, __GLfloat t);
#define __glNop3                        NopSpanRender
#define SHADE_UVSCALED_TEXTURE      0x1000000
/* the texture object as the pick code sees it (glctx.h __GLtextureICD with the proc slots named) */
#define PICK_TEX(gc, i)    (__glCurrentTexture(gc, i))
/************************************************************************/
#define MATRIX(t, off)  ((__GLmatrixICD *)((GLubyte *)(t) + (off)))
#define SET_IMM(gc, i, slot, proc) \
    if ((gc)->modes.layerPlane) (gc)->dispatchState->fn[slot] = (__GLdispatchFn)(proc); \
    else (gc)->hwcx.immProcs[i] = (__GLproc)(proc)

/* 69016060 -- hwPickFogProcs: pickFogProcs of the hardware path: Glide table fog (on the fog coordinate with
** GL_FOG_COORDINATE_EXT, else on Q) while GL_FOG is on, unless the mode flags say the fog is applied to the
** vertex colours (RGB, smooth, smooth light without 0x2000000 / 0x10000 / cheap fog with dither): then
** GR_FOG_DISABLE; the vertex layout's fog coordinate (GR_PARAM_FOG_EXT at +0xa0) and the per-vertex fog
** procs (by the coordinate source and GL_LINEAR: 690a1f10 / 690a1e40 / 690a2030 / 690a1f60) */
void __fastcall hwPickFogProcs(__GLcontext *gc)
{
    if ((gc->state.enables.general & __GL_FOG_ENABLE) &&
        !((__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_RGB) && (__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_SMOOTH) &&
          (__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_SMOOTH_LIGHT) && !(__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_PERSP_TEXTURE) &&
          !(__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_CULL_FACE) &&
          !((__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_CHEAP_FOG) && (__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_DITHER)))) {
        if (gc->state.fog.coordSource == GL_FOG_COORDINATE_EXT) {
            if (*gc->hwcx.grState[__GL_GR_FOG_MODE] != GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT) {
                *gc->hwcx.grState[__GL_GR_FOG_MODE] = GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT;
                gc->hwcx.grDirty |= 1 << __GL_GR_FOG_MODE;
            }
        }
        else {
            if (*gc->hwcx.grState[__GL_GR_FOG_MODE] != GR_FOG_WITH_TABLE_ON_Q) {
                *gc->hwcx.grState[__GL_GR_FOG_MODE] = GR_FOG_WITH_TABLE_ON_Q;
                gc->hwcx.grDirty |= 1 << __GL_GR_FOG_MODE;
            }
        }
    }
    else {
        if (*gc->hwcx.grState[__GL_GR_FOG_MODE] != GR_FOG_DISABLE) {
            *gc->hwcx.grState[__GL_GR_FOG_MODE] = GR_FOG_DISABLE;
            gc->hwcx.grDirty |= 1 << __GL_GR_FOG_MODE;
        }
    }
    if ((gc->state.enables.general & __GL_FOG_ENABLE) && gc->state.fog.coordSource == GL_FOG_COORDINATE_EXT) {
        grVertexLayout(GR_PARAM_FOG_EXT, 0xa0, GR_PARAM_ENABLE);
    }
    else {
        grVertexLayout(GR_PARAM_FOG_EXT, 0xa0, GR_PARAM_DISABLE);
    }
    if (gc->state.enables.general & __GL_FOG_ENABLE) {
        if (gc->state.fog.coordSource == GL_FRAGMENT_DEPTH_EXT) {
            if (gc->state.fog.mode == GL_LINEAR) {
                gc->procs.fogVertex = __glFogVertexLinear;     /* __glFogVertexLinear (eye z) */
            }
            else {
                gc->procs.fogVertex = __glFogVertex;
            }
        }
        else if (gc->state.fog.mode == GL_LINEAR) {
            gc->procs.fogVertex = __glFogVertexLinearFogCoord;         /* __glFogVertexLinear (fog coordinate) */
        }
        else {
            gc->procs.fogVertex = __glFogVertexFogCoord;
        }
        gc->procs.fogPoint = (__GLproc)__glFogFragmentSlow;
        gc->procs.fogColor = (__GLproc)__glFogColorSlow;
        return;
    }
    gc->procs.fogVertex = NULL;
    gc->procs.fogPoint = NULL;
    gc->procs.fogColor = NULL;
}

/* 690161d0 (size 497): hwPickLineProcs: pickLineProcs of the hardware path: Glide lines unless the state needs the
** software picker (as for points, plus line stipple) */
void __fastcall hwPickLineProcs(__GLcontext *gc)
{
    GLuint modeFlags = MODEFLAGS(gc);
    GLint i;
    __GLtextureICD *tex;

    if (gc->renderMode == GL_RENDER && gc->state.raster.drawBuffer != GL_FRONT_AND_BACK
        && !(modeFlags & ((gc->hwcx.has32bpp ? 0 : 0x20) | 0x8080))
        && (gc->hwcx.has32bpp
            || (gc->state.raster.rMask == gc->state.raster.gMask
                && gc->state.raster.rMask == gc->state.raster.bMask))) {
        if (modeFlags & 8) {
            for (i = 0; i < gc->fx.numTMUs; i++) {
                tex = __glCurrentTexture(gc, i);
                if (tex && (!TEX_HW(tex, __GL_TMU_SLOT(i)) || tex->level[0]->border > 0))
                    goto software;
            }
        }
        gc->procs.renderTrianglePolygonModeLine = (__GLproc)hwRenderTrianglePolygonModeLine;
        if (!(*gc->vertex.getFaceNeeds0)(gc))
            gc->procs.vertexLStrip = (__GLproc)__glOtherLStripVertexFast;
        else
            gc->procs.vertexLStrip = (__GLproc)__glOtherLStripVertex;
        gc->procs.vertex2ndLines = (__GLproc)__gl2ndLinesVertex;
        gc->procs.vertex2ndLLoop = (__GLproc)SecondLLoopVertex;
        gc->procs.endLLoop = (__GLproc)__glEndLLoop;
        if (gc->state.line.requestedWidth == __GL_ONE_F) {
            if (modeFlags & 0x20000)
                gc->procs.renderLine = (__GLproc)__glSSTRenderLine;
            else
                gc->procs.renderLine = (__GLproc)__glSSTRenderFlatLine;
        } else if (gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE) {
            if (modeFlags & 0x20000)
                gc->procs.renderLine = (__GLproc)__glSSTRenderAntiAliasedLine;
            else
                gc->procs.renderLine = (__GLproc)__glSSTRenderAntiAliasedFlatLine;
        } else {
            if (modeFlags & 0x20000)
                gc->procs.renderLine = (__GLproc)__glSSTRenderWideLine;
            else
                gc->procs.renderLine = (__GLproc)__glSSTRenderWideFlatLine;
        }
        gc->hwcx.swLines = 0;
        return;
    }
software:
    CALL1(__glGenericPickLineProcs, gc);
    gc->procs.renderTrianglePolygonModeLine = (__GLproc)__glRenderTriangleLineLock;
    gc->hwcx.swLines = 1;
}

/* 690163f0 (size 405): hwPickPointProcs: pickPointProcs of the hardware path: Glide points unless the state needs the
** software picker (selection / feedback, both buffers, logic op or a colour mask the hardware cannot do,
** a texture not resident or with a border) */
void __fastcall hwPickPointProcs(__GLcontext *gc)
{
    GLuint modeFlags = MODEFLAGS(gc);
    GLuint textured;
    GLint i;
    __GLtextureICD *tex;

    if (gc->renderMode == GL_RENDER && gc->state.raster.drawBuffer != GL_FRONT_AND_BACK
        && !(modeFlags & ((gc->hwcx.has32bpp ? 0 : 0x20) | 0x80))
        && (gc->hwcx.has32bpp
            || (gc->state.raster.rMask == gc->state.raster.gMask
                && gc->state.raster.rMask == gc->state.raster.bMask))) {
        textured = modeFlags & 8;
        if (modeFlags & 8) {
            for (i = 0; i < gc->fx.numTMUs; i++) {
                tex = __glCurrentTexture(gc, i);
                if (tex && (!TEX_HW(tex, __GL_TMU_SLOT(i)) || tex->level[0]->border > 0))
                    goto software;
            }
        }
        if (!(*gc->vertex.getFaceNeeds0)(gc))
            gc->procs.vertexPoints = (__GLproc)__glPointFast;
        else
            gc->procs.vertexPoints = (__GLproc)__glPointVertex;
        gc->procs.renderTrianglePolygonModePoint = (__GLproc)hwRenderTrianglePolygonModePoint;
        if (gc->state.point.requestedSize == __GL_ONE_F) {
            gc->procs.renderPoint = (__GLproc)__glSSTRenderPoint;
        } else if (gc->state.enables.general & __GL_POINT_SMOOTH_ENABLE) {
            if (!textured && !(gc->state.enables.general & __GL_ALPHA_TEST_ENABLE) && !gc->hwcx.windowed)
                gc->procs.renderPoint = (__GLproc)__glSSTRenderBigPoint;
            else
                gc->procs.renderPoint = (__GLproc)__glSSTRenderWideAAPoint;
        } else {
            gc->procs.renderPoint = (__GLproc)__glSSTRenderWidePoint;
        }
        gc->hwcx.swPoints = 0;
        return;
    }
software:
    CALL1(__glGenericPickPointProcs, gc);
    gc->procs.renderTrianglePolygonModePoint = (__GLproc)__glRenderPointModeTriangle;
    gc->hwcx.swPoints = 1;
}

/* 690193b0 (size 1245) -- __glSSTGenericPickTextureProcs: __glGenericPickTextureProcs of the hardware path (the
** software one is 6907c9d0, gl_pick.c): colour-index textures take the shared or their own palette's format, no
** texture scale / dirty bit / TMU load */
void __fastcall __glSSTGenericPickTextureProcs(__GLcontext *gc, int unused)
{
    GLuint modeFlags = MODEFLAGS(gc);
    GLint i;

    for (i = 0; i < gc->fx.numTMUs; ++i) {
	__GLtextureICD *current = PICK_TEX(gc, i);

	/* Pick texturing function for the current texture */
	if (current) {
	    GLenum baseFormat;

	    gc->procs.calcLineRho[i] = (__GLproc)__glComputeLineRho;
	    gc->procs.calcPolygonRho[i] = (__GLproc)__glComputePolygonRho;
	    if ((current->params.minFilter != GL_LINEAR) &&
		(current->params.minFilter != GL_NEAREST)) {
		/* mipmapping */
		current->textureFunc = (__GLproc)__glMipMapFragment;

		if ((current->params.magFilter == GL_LINEAR) &&
		    ((current->params.minFilter == GL_NEAREST_MIPMAP_NEAREST) ||
		     (current->params.minFilter == GL_LINEAR_MIPMAP_NEAREST))) {
		    current->c = (__GLfloat)2.0;
		} else {
		    current->c = __GL_ONE_F;
		}
	    } else {
		/* No mipmapping needed */
		if (current->params.minFilter == current->params.magFilter) {
		    /* No rho needed as min/mag application is identical */
		    current->textureFunc = (__GLproc)__glFastTextureFragment;
		    gc->procs.calcLineRho[i] = (__GLproc)__glNopLineRho;
		    gc->procs.calcPolygonRho[i] = (__GLproc)__glNopPolygonRho;
		} else {
		    current->textureFunc = (__GLproc)__glTextureFragment;

		    if ((current->params.magFilter == GL_LINEAR) &&
			((current->params.minFilter == GL_NEAREST_MIPMAP_NEAREST) ||
			 (current->params.minFilter == GL_LINEAR_MIPMAP_NEAREST))) {
			current->c = (__GLfloat)2.0;
		    } else {
			current->c = __GL_ONE_F;
		    }
		}
	    }

	    /* Pick environment function according to base format and mode */
	    baseFormat = current->level[0]->baseFormat;
	    if (baseFormat == GL_COLOR_INDEX) {
		if (gc->state.enables.general & __GL_SHARED_TEXTURE_PALETTE_EXT_ENABLE)
		    baseFormat = gc->hwcx.sharedPalette.baseFormat;
		else
		    baseFormat = TEX_PALFMT(current);
	    }
	    current->baseFormat = baseFormat;
	    switch (gc->state.texture.unit[i].env->mode) {
	      case GL_MODULATE:
		switch (baseFormat) {
		  case GL_LUMINANCE:
		    current->env = (__GLproc)__glTextureModulateL;
		    break;
		  case GL_LUMINANCE_ALPHA:
		    current->env = (__GLproc)__glTextureModulateLA;
		    break;
		  case GL_RGB:
		    current->env = (__GLproc)__glTextureModulateRGB;
		    break;
		  case GL_RGBA:
		    current->env = (__GLproc)__glTextureModulateRGBA;
		    break;
		  case GL_ALPHA:
		    current->env = (__GLproc)__glTextureModulateA;
		    break;
		  case GL_INTENSITY:
		    current->env = (__GLproc)__glTextureModulateI;
		    break;
		  case GL_COLOR_INDEX:
		    current->env = (__GLproc)__glTextureModulateCI;
		    break;
		}
		break;
	      case GL_DECAL:
		switch (baseFormat) {
		  case GL_LUMINANCE:
		    current->env = (__GLproc)__glNop3;
		    break;
		  case GL_LUMINANCE_ALPHA:
		    current->env = (__GLproc)__glNop3;
		    break;
		  case GL_RGB:
		    current->env = (__GLproc)__glTextureDecalRGB;
		    break;
		  case GL_RGBA:
		    current->env = (__GLproc)__glTextureDecalRGBA;
		    break;
		  case GL_ALPHA:
		    current->env = (__GLproc)__glNop3;
		    break;
		  case GL_INTENSITY:
		    current->env = (__GLproc)__glNop3;
		    break;
		}
		break;
	      case GL_BLEND:
		switch (baseFormat) {
		  case GL_LUMINANCE:
		    current->env = (__GLproc)__glTextureBlendL;
		    break;
		  case GL_LUMINANCE_ALPHA:
		    current->env = (__GLproc)__glTextureBlendLA;
		    break;
		  case GL_RGB:
		    current->env = (__GLproc)__glTextureBlendRGB;
		    break;
		  case GL_RGBA:
		    current->env = (__GLproc)__glTextureBlendRGBA;
		    break;
		  case GL_ALPHA:
		    current->env = (__GLproc)__glTextureBlendA;
		    break;
		  case GL_INTENSITY:
		    current->env = (__GLproc)__glTextureBlendI;
		    break;
		}
		break;
	      case GL_REPLACE:
		switch (baseFormat) {
		  case GL_LUMINANCE:
		    current->env = (__GLproc)__glTextureReplaceL;
		    break;
		  case GL_LUMINANCE_ALPHA:
		    current->env = (__GLproc)__glTextureReplaceLA;
		    break;
		  case GL_RGB:
		    current->env = (__GLproc)__glTextureReplaceRGB;
		    break;
		  case GL_RGBA:
		    current->env = (__GLproc)__glTextureReplaceRGBA;
		    break;
		  case GL_ALPHA:
		    current->env = (__GLproc)__glTextureReplaceA;
		    break;
		  case GL_INTENSITY:
		    current->env = (__GLproc)__glTextureReplaceI;
		    break;
		  case GL_COLOR_INDEX:
		    current->env = (__GLproc)__glTextureReplaceCI;
		    break;
		}
		break;
	      case GL_ADD:
		switch (baseFormat) {
		  case GL_COLOR_INDEX:
		    current->env = (__GLproc)__glTextureAddCI;
		    break;
		}
		break;
	    }

	    /* Pick mag/min functions */
	    switch (current->dim) {
	      case 1:
		current->nearest = (__GLproc)__glNearestFilter1;
		current->linear = (__GLproc)__glLinearFilter1;
		break;
	      case 2:
		current->nearest = (__GLproc)__glNearestFilter2;
		current->linear = (__GLproc)__glLinearFilter2;
		break;
	    }

	    /* set min filter function */
	    switch (current->params.minFilter) {
	      case GL_LINEAR:
		if (modeFlags & SHADE_UVSCALED_TEXTURE) {
		    current->minify = (__GLproc)__glLinearFilterUVScaled;
		} else {
		    current->minify = (__GLproc)__glLinearFilter;
		}
		break;
	      case GL_NEAREST:
		if (modeFlags & SHADE_UVSCALED_TEXTURE) {
		    current->minify = (__GLproc)__glNearestFilterUVScaled;
		} else {
		    current->minify = (__GLproc)__glNearestFilter;
		}
		break;
	      case GL_NEAREST_MIPMAP_NEAREST:
		current->minify = (__GLproc)__glNMNFilter;
		break;
	      case GL_LINEAR_MIPMAP_NEAREST:
		current->minify = (__GLproc)__glLMNFilter;
		break;
	      case GL_NEAREST_MIPMAP_LINEAR:
		current->minify = (__GLproc)__glNMLFilter;
		break;
	      case GL_LINEAR_MIPMAP_LINEAR:
		current->minify = (__GLproc)__glLMLFilter;
		break;
	    }

	    /* set mag filter function */
	    switch (current->params.magFilter) {
	      case GL_LINEAR:
		if (modeFlags & SHADE_UVSCALED_TEXTURE) {
		    current->magnify = (__GLproc)__glLinearFilterUVScaled;
		} else {
		    current->magnify = (__GLproc)__glLinearFilter;
		}
		break;
	      case GL_NEAREST:
		if (modeFlags & SHADE_UVSCALED_TEXTURE) {
		    current->magnify = (__GLproc)__glNearestFilterUVScaled;
		} else {
		    current->magnify = (__GLproc)__glNearestFilter;
		}
		break;
	    }

	    gc->procs.texture[i] = current->textureFunc;
	    if (modeFlags & SHADE_UVSCALED_TEXTURE) {
		gc->procs.textureRaster[i] = (__GLproc)__glTextureFragmentUVScale;
	    } else {
		gc->procs.textureRaster[i] = current->textureFunc;
	    }
	} else {
	    gc->procs.texture[i] = 0;
	    gc->procs.textureRaster[i] = 0;
	}

    }
}

/* 690198c0 (size 129): hwPickCalcTextureProcs: pickCalcTextureProcs of the hardware path: the texgen variants when a unit
** generates coordinates */
void __fastcall hwPickCalcTextureProcs(__GLcontext *gc)
{
    GLint i, texgen;

    MODEFLAGS(gc);
    texgen = 0;
    gc->procs.calcTexture = (__GLproc)hwTexCoords;
    gc->procs.calcTextureUnit[0] = (__GLproc)hwXformTexCoords;
    for (i = 0; i < gc->fx.numTMUs; i++) {
        if (gc->state.enables.texture[i] & __GL_TEXTURE_GEN_ANY_ENABLE) texgen = 1;
    }
    if (texgen) {
        gc->procs.calcTextureUnit[0] = (__GLproc)__glSSTCalcMixedTexture;
        gc->procs.calcTexture = (__GLproc)hwTexCoordsXform0;
    }
    gc->procs.calcTextureUnit[1] = gc->procs.calcTextureUnit[0];
}

/* 69019950 -- hwPickTriangleProcs: pickTriangleProcs of the hardware path: Glide triangles (the vertex procs 690943d0..) for
** GL_RENDER into one buffer (not GL_FRONT_AND_BACK) without polygon stipple / logic op (stencil too unless
** 32 bpp) and, below 32 bpp, equal r / g / b write masks, while every enabled texture is resident
** (hwHandle) and borderless; filled polygons (both faces GL_FILL, no polygon offset enable) take the
** smooth / flat Glide fillers, the rest 69061180.  Otherwise the software picker (6907c540) */
void __fastcall hwPickTriangleProcs(__GLcontext *gc)
{
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    __GLtextureICD *tex;
    GLint i;

    if (gc->renderMode == GL_RENDER && gc->state.raster.drawBuffer != GL_FRONT_AND_BACK &&
        (modeFlags & ((gc->hwcx.has32bpp ? 0 : __GL_SHADE_STENCIL_TEST) | __GL_SHADE_LOGICOP | __GL_SHADE_STIPPLE)) == 0 &&
        (gc->hwcx.has32bpp ||
         (gc->state.raster.rMask == gc->state.raster.gMask && gc->state.raster.rMask == gc->state.raster.bMask))) {
        wglGlideSetupBufferModes(gc);
        gc->polygon.mode[0] = (GLubyte)gc->state.polygon.frontMode & 0xf;
        gc->polygon.mode[1] = (GLubyte)gc->state.polygon.backMode & 0xf;
        gc->procs.vertexTFan = (__GLproc)__glOtherTFanVertex;
        gc->procs.vertexEvenTStrip = (__GLproc)__glEvenTStripVertex;
        gc->procs.vertexOddTStrip = (__GLproc)__glOddTStripVertex;
        gc->procs.vertex3rdTris = (__GLproc)__gl3rdTrianglesVertex;
        gc->procs.vertex4thQStrip = (__GLproc)__gl4thQStripVertex;
        gc->procs.vertex4thQuads = (__GLproc)__gl4thQuadsVertex;
        gc->procs.vertexPoly = (__GLproc)__glPolygonVertex;
        gc->procs.endPoly = (__GLproc)__glEndPolygon;
        if (modeFlags & __GL_SHADE_TEXTURE) {
            for (i = 0; i < gc->fx.numTMUs; i++) {
                tex = __glCurrentTexture(gc, i);
                if (tex != NULL && (tex->hwHandle[__GL_TMU_SLOT(i)] == 0 || tex->level[0]->border > 0))
                    goto software;
            }
        }
        if (gc->state.polygon.frontMode == gc->state.polygon.backMode &&
            gc->state.polygon.frontMode == GL_FILL &&
            (gc->state.enables.general & (__GL_POLYGON_OFFSET_POINT_ENABLE | __GL_POLYGON_OFFSET_LINE_ENABLE |
                                          __GL_POLYGON_OFFSET_FILL_ENABLE)) == 0) {
            if (modeFlags & __GL_SHADE_SMOOTH_LIGHT) {
                gc->procs.renderTriangle = (__GLproc)__glSSTRenderSmoothTriangle;
                gc->hwcx.swTriangles = 0;
                return;
            }
            gc->procs.renderTriangle = (__GLproc)__glSSTRenderFlatTriangle;
            gc->hwcx.swTriangles = 0;
            return;
        }
        gc->procs.renderTriangle = (__GLproc)__glSSTRenderTriangle;
        gc->hwcx.swTriangles = 0;
        return;
    }
software:
    ((void (__fastcall *)(__GLcontext *))__glGenericPickTriangleProcs)(gc);
    gc->hwcx.swTriangles = 1;
}

/* 69019c30 (size 274) -- hwPickVertexArrayProcs: the vertex-array procs of the hardware path (vertexArray.flags,
** the cache preparation, validateVertex, the primitives that need the render-buffer lock) */
void __fastcall hwPickVertexArrayProcs(__GLcontext *gc)
{
    if (!(gc->vertexArray.enableFlags & (__GL_VA_COLOR_ENABLE | __GL_VA_VERTEX_ENABLE | __GL_VA_TEXCOORD_ENABLE(0) | __GL_VA_TEXCOORD_ENABLE(1)))) return;
    if ((gc->state.enables.general & __GL_LIGHTING_ENABLE) && (gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE))
        gc->vertexArray.flags |= 0x20;
    else
        gc->vertexArray.flags &= ~0x20;
    gc->vertexArray.flags |= 0x20002;
    if (*(GLubyte *)&gc->vertexArray.enableFlags & __GL_VA_VERTEX_ENABLE)
        gc->vertexArray.flags &= ~0x40;
    else
        gc->vertexArray.flags |= 0x40;
    if (gc->hwcx.swRendering)
        gc->vertexArray.flags |= 0x20;
    if (gc->vertexArray.flags & 4) {
        if (sstVarray_AllocateVbuf(gc, gc->vertexArray.lockFirst, gc->vertexArray.lockCount))
            gc->vertexArray.flags |= 1;
        else
            gc->vertexArray.flags &= ~1;
    } else
        gc->vertexArray.flags &= ~1;
    hwGenVertexArrayFill(gc);
    {   /* stored through a pointer: the original does not hoist the swLines load above this store */
        __GLproc *pp = &gc->vertexArray.prepareCache;
        *pp = (__GLproc)hwPrepareVertexCache;
    }
    gc->procs.validateVertex[0] = (__GLproc)NopSpanRender;
    gc->procs.validateVertex[1] = (__GLproc)__glValidateVertex2;
    gc->procs.validateVertex[2] = (__GLproc)__glValidateVertex3;
    gc->procs.validateVertex[3] = (__GLproc)__glValidateVertex4;
    if (!gc->hwcx.swLines && !gc->hwcx.swPoints && !gc->hwcx.swTriangles)
        gc->vertexArray.lockPrimMask = 0;
    else
        gc->vertexArray.lockPrimMask = 0xffffffff;
}

#pragma optimize("w", on)
/* 69019e60 (size 376) -- __glSSTGenericPickTextureEnables: __glGenericPickTextureEnables of the hardware path (the
** software one is 6907c890, gl_pick.c): the current target index per unit, its parameters copied into the current
** texture, the texture key; dirty bits for a changed texture matrix and a changed enable */
void __fastcall __glSSTGenericPickTextureEnables(__GLcontext *gc)
{
    GLint i, idx;
    GLboolean was;
    __GLtexObjParamICD *params = NULL;

    for (i = 0; i < gc->fx.numTMUs; i++) {
        was = gc->texture.textureEnabled[i];
        gc->texture.currentIndex[i] = -1;
        if (gc->state.enables.texture[i] & __GL_TEXTURE_2D_ENABLE) {
            idx = __glTexTargetIndex(gc, GL_TEXTURE_2D, i);
            if (idx >= 0 && idx < 4) {
                if (hwIsTextureDrawable(gc, gc->texture.boundTexture[i][idx], i)) {
                    gc->texture.currentIndex[i] = idx;
                    params = __glLookUpTextureParams(gc, GL_TEXTURE_2D, i);
                }
            }
        } else if (gc->state.enables.texture[i] & __GL_TEXTURE_1D_ENABLE) {
            idx = __glTexTargetIndex(gc, GL_TEXTURE_1D, i);
            if (idx >= 0 && idx < 4) {
                if (hwIsTextureDrawable(gc, gc->texture.boundTexture[i][idx], i)) {
                    gc->texture.currentIndex[i] = idx;
                    params = __glLookUpTextureParams(gc, GL_TEXTURE_1D, i);
                }
            }
        }
        if (gc->texture.currentIndex[i] != -1) {
            gc->texture.textureEnabled[i] = GL_TRUE;
            __glCurrentTexture(gc, i)->params = *params;
            wglGlideTexKey(gc, (BYTE *)__glCurrentTexture(gc, i));
        } else {
            gc->texture.textureEnabled[i] = GL_FALSE;
        }
        if (gc->texture.textureEnabled[i] && (gc->state.enables.texture[i] & __GL_TEXTURE_GEN_ANY_ENABLE)
            && gc->hwcx.texMatrixType[i] != gc->transform.texture[i]->matrix.matrixType)
            gc->dirtyMask |= __GL_DIRTY_TEXMATRIX_ICD;
        if (gc->texture.textureEnabled[i] != was)
            gc->dirtyMask |= __GL_DIRTY_TEXENABLE_ICD;
    }
}
#pragma optimize("", on)
/* 6901a180 -- hwPickRenderProcs: the render procs of the Glide fast path: a texture unit with a non-identity
** texture matrix or GL_TEXTURE_GEN_Q needs the non-unity-q path (wglDevNonUnityQ); no software fallbacks; Glide lines
** (smooth 6905f590, flat 6905f5a0) and triangles (69061180); points by size (1.0: 6905ff30), GL_POINT_SMOOTH
** (690603ed without texturing / alpha test when fullscreen, else 690600c0) or 6905ff40; both faces filled */
GLuint __fastcall hwPickRenderProcs(__GLcontext *gc, GLuint edx_unused)
{
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    GLint i;

    if (gc->hwcx.nonUnityQ == 0) {
        for (i = 0; i < gc->fx.numTMUs; i++) {
            if (gc->texture.textureEnabled[i] &&
                (gc->transform.texture[i]->matrix.matrixType != __GL_MT_IDENTITY ||
                 (gc->state.enables.texture[i] & __GL_TEXTURE_GEN_Q_ENABLE))) {
                ((void (__fastcall *)(__GLcontext *))wglDevNonUnityQ)(gc);
            }
        }
    }
    gc->hwcx.swTriangles = 0;
    gc->hwcx.swLines = 0;
    gc->hwcx.swPoints = 0;
    if (modeFlags & __GL_SHADE_SMOOTH_LIGHT) {
        gc->procs.renderLine = (__GLproc)__glSSTRenderLine;
    }
    else {
        gc->procs.renderLine = (__GLproc)__glSSTRenderFlatLine;
    }
    gc->procs.renderTriangle = (__GLproc)__glSSTRenderTriangle;
    if (*(GLint *)&gc->state.point.requestedSize == 0x3f800000) {      /* 1.0f */
        gc->procs.renderPoint = (__GLproc)__glSSTRenderPoint;
    }
    else if (gc->state.enables.general & __GL_POINT_SMOOTH_ENABLE) {
        if ((modeFlags & __GL_SHADE_TEXTURE) == 0 && (gc->state.enables.general & __GL_ALPHA_TEST_ENABLE) == 0 &&
            gc->hwcx.windowed == 0) {
            gc->procs.renderPoint = (__GLproc)__glSSTRenderBigPoint;
        }
        else {
            gc->procs.renderPoint = (__GLproc)__glSSTRenderWideAAPoint;
        }
    }
    else {
        gc->procs.renderPoint = (__GLproc)__glSSTRenderWidePoint;
    }
    gc->procs.endPrim = (__GLproc)__glNop1;
    gc->polygon.mode[0] = 2;
    gc->polygon.mode[1] = 2;
    return 1;
}

/* 690270d0 (size 472) -- hwPickVertexProcs: pick the clip-check, validate-vertex and vertex procs of the hardware
** path */
void __fastcall hwPickVertexProcs(__GLcontext *gc)
{
    GLuint general = gc->state.enables.general;
    GLuint *texEnables = &gc->state.enables.texture[0];
    GLuint tex = gc->state.enables.texture[1] | *texEnables;
    __GLmatrixICD *mvp = MATRIX(gc->transform.modelView, 0xc0);
    GLuint type = mvp->matrixType;
    GLuint idx;
    GLint i;

    if (gc->state.enables.clipPlanes) {
        gc->procs.clipCheck2 = (__GLproc)__glClipCheckAll2;
        gc->procs.clipCheck3 = (__GLproc)__glClipCheckAll3;
        gc->procs.clipCheck4 = (__GLproc)__glClipCheckAll4;
    } else {
        gc->procs.clipCheck3 = (__GLproc)__glClipCheckFast;
        gc->procs.clipCheck4 = (__GLproc)__glClipCheckFast;
        if (type < 2 || mvp->matrix[3][2] < -1.0f || mvp->matrix[3][2] > 1.0f)
            gc->procs.clipCheck2 = (__GLproc)__glClipCheckFast;
        else
            gc->procs.clipCheck2 = (__GLproc)__glClipCheckFrustum2D;
    }
    if ((*gc->vertex.getFaceNeeds0)(gc) == 0) {
        gc->procs.validateVertex[0] = (__GLproc)NopSpanRender;
        gc->procs.validateVertex[1] = (__GLproc)NopSpanRender;
        gc->procs.validateVertex[2] = (__GLproc)NopSpanRender;
        gc->procs.validateVertex[3] = (__GLproc)NopSpanRender;
    } else {
        gc->procs.validateVertex[0] = (__GLproc)NopSpanRender;
        gc->procs.validateVertex[1] = (__GLproc)__glValidateVertex2;
        gc->procs.validateVertex[2] = (__GLproc)__glValidateVertex3;
        gc->procs.validateVertex[3] = (__GLproc)__glValidateVertex4;
    }
    if (!gc->hwcx.nonUnityQ) {
        for (i = 0; i < gc->fx.numTMUs; i++) {
            if (gc->texture.textureEnabled[i] &&
                (MATRIX(gc->transform.texture[i], 0)->matrixType != 4 || (gc->state.enables.texture[i] & __GL_TEXTURE_GEN_Q_ENABLE)))
                ((void (__fastcall *)(__GLcontext *))wglDevNonUnityQ)(gc);
        }
    }
    if (gc->renderMode == GL_FEEDBACK) {
        gc->procs.v = (__GLproc)__glSaveCAllFog;
        return;
    }
    idx = (general & __GL_LIGHTING_ENABLE) ? 1 : 2;
    if (gc->texture.textureEnabled[0] || gc->texture.textureEnabled[1]) {
        idx |= 4;
        if ((tex & 4) && (gc->state.texture.unit[0].s.mode == GL_SPHERE_MAP ||
                          gc->state.texture.unit[1].s.mode == GL_SPHERE_MAP))
            idx |= 1;
        if ((tex & 8) && (gc->state.texture.unit[0].t.mode == GL_SPHERE_MAP ||
                          gc->state.texture.unit[1].t.mode == GL_SPHERE_MAP))
            idx |= 1;
    }
    if ((general & __GL_FOG_ENABLE) && gc->state.fog.coordSource == GL_FOG_COORDINATE_EXT)
        idx |= 8;
    gc->procs.v = g_hwVertexProcs[idx];
}

/* 6902aafb (size 234) -- hwPickDrawRoutines: use the direct command-FIFO routines when Glide has COMMAND_TRANSPORT
** and the hardware is one of the known names (or __GL_FORCE_HW says so), else the grDrawTriangle wrappers */
void __fastcall hwPickDrawRoutines(__GLcontext *gc)
{
    char **name;
    const char *hw;
    char *s;
    GLint known;

    gc->hwcx.grCommandTransportInfoExt2 = (__GLproc)grGetProcAddress("grCommandTransportInfoExt2");
    known = 0;
    hw = grGetString(GR_HARDWARE);
    {
        /* 690eff7c -- the hardware that gets the direct-FIFO path (its strings follow this function's first one) */
        static char *fifoHardwareNames[5] = { "Voodoo2", "Voodoo Banshee", "Voodoo3", "Voodoo4", "Voodoo5" };
        /* the pointers compared signed (jl), as in the original */
        for (name = fifoHardwareNames; (GLint)name < (GLint)(fifoHardwareNames + 5); name++) {
            if (strncmp(hw, *name, strlen(*name)) == 0)
                known = 1;
        }
    }
    s = getenv("__GL_FORCE_HW");
    if (s != NULL)
        known = atol(s);
    ICD_T("hwPickDrawRoutines: known hw, InfoExt2", (GLuint)gc->hwcx.grCommandTransportInfoExt2 | known);
    if (gc->hwcx.grCommandTransportInfoExt2 != 0 && known != 0) {
        gc->hwcx.cmdTransportInfo = (*(GrCmdTransportInfo *(__stdcall *)(void))gc->hwcx.grCommandTransportInfoExt2)();
        ICD_T("hwPickDrawRoutines: cmdTransportInfo", gc->hwcx.cmdTransportInfo);
        gc->hwcx.grCommandTransportMakeRoomExt2 = (__GLproc)grGetProcAddress("grCommandTransportMakeRoomExt2");
        ICD_T("hwPickDrawRoutines: makeRoom proc", gc->hwcx.grCommandTransportMakeRoomExt2);
        hwBuildFifoTriProcs(gc);
        ICD_T("hwPickDrawRoutines: FIFO tri procs built", 0);
        og_genOffsetFifo(gc);
        ICD_T("hwPickDrawRoutines: offset FIFO built", 0);
        return;
    }
    ICD_T("hwPickDrawRoutines: no direct FIFO", 0);
    gc->hwcx.cmdTransportInfo = NULL;
    gc->hwcx.drawTri = (__GLproc)hwDrawTri;
    gc->hwcx.drawQuad = (__GLproc)hwDrawQuad;
    gc->hwcx.drawTriPackColor = (__GLproc)hwDrawTriPackColor;
}

/* ---- s_pick.c: the shading mode flags (gc->polygon.shader.modeFlags) ------------------------- */

/* 690653a0 (size 481) -- computeModeFlags: compute modeFlags: build gc->polygon.shader.modeFlags (the
** __GL_SHADE_* bits) from the enables and modes: RGB with logic op / blend / alpha test / alpha bitplanes (blend
** and alpha test also set NEEDS_ALPHA), or CI logic op / writemask / index test; a write mask sets MASK; texturing
** sets TEXTURE, the persp / projection-scaled bits and MIPMAP_TEXTURE when unit 0's min filter is a mipmap one;
** smooth shading, depth test with depth buffer (+ polygon offset fill with a nonzero factor or units), cull face,
** dither on a front buffer of under 3 bytes per element, stipples, stencil, two-sided lighting, and slow (textured)
** or cheap fog */
void __fastcall computeModeFlags(__GLcontext *gc)
{
    GLuint modeFlags, enables;
    __WGLbuffer *wb;

    modeFlags = 0;
    enables = gc->state.enables.general;
    if (gc->modes.rgbMode) {
        modeFlags = __GL_SHADE_RGB;
        if (enables & __GL_COLOR_LOGIC_OP_ENABLE) {
            modeFlags |= __GL_SHADE_LOGICOP;
        } else if (enables & 2) {
            modeFlags |= __GL_SHADE_BLEND | __GL_SHADE_NEEDS_ALPHA;
        }
        if (enables & 1) {
            modeFlags |= __GL_SHADE_ALPHA_TEST | __GL_SHADE_NEEDS_ALPHA;
        }
        if (gc->modes.alphaBits) {
            modeFlags |= __GL_SHADE_NEEDS_ALPHA;
        }
        if (!gc->state.raster.rMask || !gc->state.raster.gMask || !gc->state.raster.bMask ||
            (!gc->state.raster.aMask && gc->modes.alphaBits)) {
            modeFlags |= __GL_SHADE_MASK;
        }
    } else {
        if (enables & 4) {
            modeFlags |= __GL_SHADE_LOGICOP;
        }
        if (gc->state.raster.writeMask != gc->frontBuffer.redMax) {
            modeFlags |= __GL_SHADE_MASK;
        }
        if (enables & __GL_INDEX_TEST_EXT_ENABLE) {
            modeFlags |= __GL_SHADE_INDEX_TEST;
        }
    }
    if (gc->texture.textureEnabled[0] || gc->texture.textureEnabled[1]) {
        modeFlags |= __GL_SHADE_TEXTURE;
        if (__glCurrentTexture(gc, 0) &&
            __glCurrentTexture(gc, 0)->params.minFilter != GL_NEAREST &&
            __glCurrentTexture(gc, 0)->params.minFilter != GL_LINEAR) {
            modeFlags |= __GL_SHADE_MIPMAP_TEXTURE;
        }
        modeFlags |= __GL_SHADE_PROJSCALED_TEXTURE | __GL_SHADE_PERSP_TEXTURE;
    }
    if (gc->state.light.shadingModel == GL_SMOOTH) {
        modeFlags |= __GL_SHADE_SMOOTH | __GL_SHADE_SMOOTH_LIGHT;
    }
    if ((enables & __GL_DEPTH_TEST_ENABLE) && gc->modes.haveDepthBuffer) {
        modeFlags |= __GL_SHADE_DEPTH_TEST | __GL_SHADE_DEPTH_ITER;
        if ((enables & __GL_POLYGON_OFFSET_FILL_ENABLE) &&
            (gc->state.polygon.factor != 0.0 || gc->state.polygon.units != 0.0)) {
            modeFlags |= __GL_SHADE_POLYGON_OFFSET_FILL;
        }
    }
    if (enables & __GL_CULL_FACE_ENABLE) {
        modeFlags |= __GL_SHADE_CULL_FACE;
    }
    if ((enables & 8) && (wb = gc->front->buf.wglBuf) != 0 && wb->elementSize < 3) {
        modeFlags |= __GL_SHADE_DITHER;
    }
    if (enables & __GL_POLYGON_STIPPLE_ENABLE) {
        modeFlags |= __GL_SHADE_STIPPLE;
    }
    if (enables & __GL_LINE_STIPPLE_ENABLE) {
        modeFlags |= __GL_SHADE_LINE_STIPPLE;
    }
    if ((enables & __GL_STENCIL_TEST_ENABLE) && gc->modes.haveStencilBuffer) {
        modeFlags |= __GL_SHADE_STENCIL_TEST;
    }
    if ((enables & __GL_LIGHTING_ENABLE) && gc->state.light.model.twoSided) {
        modeFlags |= __GL_SHADE_TWOSIDED;
    }
    if (enables & __GL_FOG_ENABLE) {
        if (modeFlags & __GL_SHADE_TEXTURE) {
            modeFlags |= __GL_SHADE_SLOW_FOG;
        } else {
            modeFlags |= __GL_SHADE_CHEAP_FOG;
        }
    }
    gc->polygon.shader.modeFlags = modeFlags;
}

/* 69065590 (size 554) -- computeVertexNeeds: vertex needs (SI __glGenericPickAllProcsPhase2's needs mask) */
void __fastcall computeVertexNeeds(__GLcontext *gc)
{
    GLuint needs, faceNeeds, enables, texEnables;

    texEnables = gc->state.enables.texture[1] | gc->state.enables.texture[0];
    enables = gc->state.enables.general;
    needs = 0;
    faceNeeds = 0x100;
    if (gc->texture.textureEnabled[0] != 0 || gc->texture.textureEnabled[1] != 0) {
        needs = 4;
        if (texEnables & 0xc) {
            switch (gc->state.texture.unit[0].s.mode) {
              case GL_EYE_LINEAR:
                needs |= 0x10;
                break;
              case GL_SPHERE_MAP:
                needs |= 0x18;
                break;
            }
            switch (gc->state.texture.unit[1].s.mode) {
              case GL_EYE_LINEAR:
                needs |= 0x10;
                break;
              case GL_SPHERE_MAP:
                needs |= 0x18;
                break;
            }
            switch (gc->state.texture.unit[0].t.mode) {
              case GL_EYE_LINEAR:
                needs |= 0x10;
                break;
              case GL_SPHERE_MAP:
                needs |= 0x18;
                break;
            }
            switch (gc->state.texture.unit[1].t.mode) {
              case GL_EYE_LINEAR:
                needs |= 0x10;
                break;
              case GL_SPHERE_MAP:
                needs |= 0x18;
                break;
            }
        }
    }
    if (enables & __GL_LIGHTING_ENABLE) {
        faceNeeds |= 8;
        if (gc->state.light.model.localViewer) {
            faceNeeds |= 0x10;
        } else {
            GLint i;
            __GLlightSourceStateICD *lss = (__GLlightSourceStateICD *)gc->state.light.source;

            for (i = 0; i < gc->constants.numberOfLights; i++, lss++) {
                if ((gc->state.enables.lights & (1 << i)) && lss->positionEye.w != 0.0f) {
                    faceNeeds |= 0x10;
                    break;
                }
            }
        }
    }
    if (gc->state.enables.general & __GL_FOG_ENABLE) {
        needs |= 0x10;
        if (gc->state.hints.fog != GL_NICEST || (gc->polygon.shader.modeFlags & __GL_SHADE_CHEAP_FOG)) {
            needs |= 0x40;
        }
    }
    if (gc->state.enables.clipPlanes) {
        needs |= 0x10;
    }
    if (gc->state.point.ext[0] != 0.0 || gc->state.point.ext[3] != 1.0 || gc->state.point.ext[4] != 0.0 ||
        gc->state.point.ext[5] != 0.0 || gc->state.point.requestedSize != 1.0f) {
        needs |= 0x10;
    }
    gc->vertex.needs = needs;
    gc->vertex.faceNeeds[0] = faceNeeds | needs;
    gc->vertex.faceNeeds[1] = faceNeeds | needs;
    if ((enables & __GL_LIGHTING_ENABLE) || (gc->polygon.shader.modeFlags & (__GL_SHADE_CHEAP_FOG | __GL_SHADE_SMOOTH_LIGHT))) {
        gc->vertex.faceNeeds[0] |= 1;
        if (gc->state.light.model.twoSided) {
            gc->vertex.faceNeeds[1] |= 2;
        } else {
            gc->vertex.faceNeeds[1] = gc->vertex.faceNeeds[0];
        }
    }
    if (gc->state.light.shadingModel == GL_SMOOTH) {
        /* the volatile read keeps the original's load order (faceNeeds[0] first) */
        gc->vertex.materialNeeds = (*(volatile GLuint *)&gc->vertex.faceNeeds[0] | gc->vertex.faceNeeds[1]) & ~4;
    } else {
        gc->vertex.materialNeeds = 0;
    }
}

/* 69065890 (size 87) -- hwShadingStateUnchanged: GL_TRUE when the shading state is still the one hwSaveShadingState
** saved.  The face needs are copied into a local pair first (the original loads both before the first compare). */
GLboolean __fastcall hwShadingStateUnchanged(__GLcontext *gc)
{
  GLuint faceNeeds[2];

  faceNeeds[0] = gc->vertex.faceNeeds[0];
  faceNeeds[1] = gc->vertex.faceNeeds[1];
  if (g_savedModeFlags == gc->polygon.shader.modeFlags && g_savedNeeds == gc->vertex.needs &&
      g_savedFaceNeeds0 == faceNeeds[0] && g_savedFaceNeeds1 == faceNeeds[1] &&
      g_savedMaterialNeeds == gc->vertex.materialNeeds)
    return GL_TRUE;
  return GL_FALSE;
}
/* one "old vs new" line of the log; as in the original each value is printed as !(flags & bit), i.e. 1 when the
** flag is clear */
#define LOG_FLAG(f, fmt, oldFlags, newFlags, bit) \
    fprintf(f, fmt, !((oldFlags) & (bit)), !((newFlags) & (bit)))

/* 69065f6e (size 1773) -- hwLogShadingStateChange: debug log of a shading-state change: append the saved (old) and
** current (new) mode flags, needs, face needs and material needs, bit by bit, to modeflags_needs.log (the state
** hwShadingStateUnchanged compares); ToDo UNUSED CODE: nothing in the driver calls it */
void __fastcall hwLogShadingStateChange(__GLcontext *gc)
{
    GLuint modeFlags, needs, faceNeeds[2], materialNeeds;   /* faceNeeds a local pair, as in hwShadingStateUnchanged
                                                               (the original's frame has both slots) */
    FILE *f;

    modeFlags = gc->polygon.shader.modeFlags;
    needs = gc->vertex.needs;
    faceNeeds[0] = gc->vertex.faceNeeds[0];
    faceNeeds[1] = gc->vertex.faceNeeds[1];
    materialNeeds = gc->vertex.materialNeeds;
    f = fopen("modeflags_needs.log", "at");
    if (f == NULL)
        return;
    fprintf(f, "MODE FLAGS AND NEEDS OUT OF DATE: old versus new:\n");
    LOG_FLAG(f, "shade_rgb                 %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_RGB);
    LOG_FLAG(f, "shade_logicop             %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_LOGICOP);
    LOG_FLAG(f, "shade_blend               %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_BLEND);
    LOG_FLAG(f, "shade_alpha_test          %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_ALPHA_TEST);
    LOG_FLAG(f, "shade_src_alpha           %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_NEEDS_ALPHA);
    LOG_FLAG(f, "shade_mask                %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_MASK);
    LOG_FLAG(f, "shade_texture             %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_TEXTURE);
    LOG_FLAG(f, "shade_texture_mipmap      %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_MIPMAP_TEXTURE);
    LOG_FLAG(f, "shade_texture_uvscaled    %d vs %d\n", g_savedModeFlags, modeFlags, SHADE_UVSCALED_TEXTURE);
    LOG_FLAG(f, "shade_texture_persp       %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_PERSP_TEXTURE);
    LOG_FLAG(f, "shade_texture_projscaled  %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_PROJSCALED_TEXTURE);
    LOG_FLAG(f, "shade_smooth              %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_SMOOTH);
    LOG_FLAG(f, "shade_smooth_light        %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_SMOOTH_LIGHT);
    LOG_FLAG(f, "shade_depth_test          %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_DEPTH_TEST);
    LOG_FLAG(f, "shade_depth_iter          %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_DEPTH_ITER);
    LOG_FLAG(f, "shade_polygon_offset_fill %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_POLYGON_OFFSET_FILL);
    LOG_FLAG(f, "shade_stencil_test        %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_STENCIL_TEST);
    LOG_FLAG(f, "shade_twosided            %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_TWOSIDED);
    LOG_FLAG(f, "shade_slow_fog            %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_SLOW_FOG);
    LOG_FLAG(f, "shade_cheap_fog           %d vs %d\n", g_savedModeFlags, modeFlags, __GL_SHADE_CHEAP_FOG);

    LOG_FLAG(f, "has_pargb       %d vs %d\n", g_savedNeeds, needs, __GL_HAS_PACKED_COLOR);
    LOG_FLAG(f, "has_texture     %d vs %d\n", g_savedNeeds, needs, __GL_HAS_TEXTURE);
    LOG_FLAG(f, "has_normal      %d vs %d\n", g_savedNeeds, needs, __GL_HAS_NORMAL);
    LOG_FLAG(f, "has_fog         %d vs %d\n", g_savedNeeds, needs, __GL_HAS_FOG);
    LOG_FLAG(f, "has_eye         %d vs %d\n", g_savedNeeds, needs, __GL_HAS_EYE);
    LOG_FLAG(f, "has_front_color %d vs %d\n", g_savedNeeds, needs, __GL_HAS_FRONT_COLOR);
    LOG_FLAG(f, "has_back_color  %d vs %d\n", g_savedNeeds, needs, __GL_HAS_BACK_COLOR);

    LOG_FLAG(f, "has_pargb_f0       %d vs %d\n", g_savedFaceNeeds0, faceNeeds[0], __GL_HAS_PACKED_COLOR);
    LOG_FLAG(f, "has_texture_f0     %d vs %d\n", g_savedFaceNeeds0, faceNeeds[0], __GL_HAS_TEXTURE);
    LOG_FLAG(f, "has_normal_f0      %d vs %d\n", g_savedFaceNeeds0, faceNeeds[0], __GL_HAS_NORMAL);
    LOG_FLAG(f, "has_fog_f0         %d vs %d\n", g_savedFaceNeeds0, faceNeeds[0], __GL_HAS_FOG);
    LOG_FLAG(f, "has_eye_f0         %d vs %d\n", g_savedFaceNeeds0, faceNeeds[0], __GL_HAS_EYE);
    LOG_FLAG(f, "has_front_color_f0 %d vs %d\n", g_savedFaceNeeds0, faceNeeds[0], __GL_HAS_FRONT_COLOR);
    LOG_FLAG(f, "has_back_color_f0  %d vs %d\n", g_savedFaceNeeds0, faceNeeds[0], __GL_HAS_BACK_COLOR);

    LOG_FLAG(f, "has_pargb_f1       %d vs %d\n", g_savedFaceNeeds1, faceNeeds[1], __GL_HAS_PACKED_COLOR);
    LOG_FLAG(f, "has_texture_f1     %d vs %d\n", g_savedFaceNeeds1, faceNeeds[1], __GL_HAS_TEXTURE);
    LOG_FLAG(f, "has_normal_f1      %d vs %d\n", g_savedFaceNeeds1, faceNeeds[1], __GL_HAS_NORMAL);
    LOG_FLAG(f, "has_fog_f1         %d vs %d\n", g_savedFaceNeeds1, faceNeeds[1], __GL_HAS_FOG);
    LOG_FLAG(f, "has_eye_f1         %d vs %d\n", g_savedFaceNeeds1, faceNeeds[1], __GL_HAS_EYE);
    LOG_FLAG(f, "has_front_color_f1 %d vs %d\n", g_savedFaceNeeds1, faceNeeds[1], __GL_HAS_FRONT_COLOR);
    LOG_FLAG(f, "has_back_color_f1  %d vs %d\n", g_savedFaceNeeds1, faceNeeds[1], __GL_HAS_BACK_COLOR);

    LOG_FLAG(f, "has_pargb_mat       %d vs %d\n", g_savedMaterialNeeds, materialNeeds, __GL_HAS_PACKED_COLOR);
    LOG_FLAG(f, "has_texture_mat     %d vs %d\n", g_savedMaterialNeeds, materialNeeds, __GL_HAS_TEXTURE);
    LOG_FLAG(f, "has_normal_mat      %d vs %d\n", g_savedMaterialNeeds, materialNeeds, __GL_HAS_NORMAL);
    LOG_FLAG(f, "has_fog_mat         %d vs %d\n", g_savedMaterialNeeds, materialNeeds, __GL_HAS_FOG);
    LOG_FLAG(f, "has_eye_mat         %d vs %d\n", g_savedMaterialNeeds, materialNeeds, __GL_HAS_EYE);
    LOG_FLAG(f, "has_front_color_mat %d vs %d\n", g_savedMaterialNeeds, materialNeeds, __GL_HAS_FRONT_COLOR);
    LOG_FLAG(f, "has_back_color_mat  %d vs %d\n", g_savedMaterialNeeds, materialNeeds, __GL_HAS_BACK_COLOR);
    fclose(f);
}

/* 6901a2a0 -- hwPickImmedProcs: pick the immediate-mode procs: the hardware fast path (690262e0..) when 6901a020 allows
** it, else the software ones (690a5410..); a layer-plane context patches its dispatch table instead */
void __fastcall hwPickImmedProcs(__GLcontext *gc)
{
    hwGenVertexArrayFill(gc);
    gc->procs.drawIndexedTriangles = (__GLproc)((int (__fastcall *)(__GLcontext *))hwBuildIndexedTriangles)(gc);
    og_buildClipInterp(gc);
    hwBuildFifoTriProcs(gc);
    if (((int (__fastcall *)(__GLcontext *))hwCanUseFastPath)(gc)) {
        SET_IMM(gc, 0, __GL_DISP_Vertex2fv, hwLazyVertex2fv);
        SET_IMM(gc, 1, __GL_DISP_Vertex3fv, hwLazyVertex3fv);
        SET_IMM(gc, 2, __GL_DISP_Vertex4fv, hwLazyVertex4fv);
    } else {
        SET_IMM(gc, 0, __GL_DISP_Vertex2fv, __glim_Vertex2fv);
        SET_IMM(gc, 1, __GL_DISP_Vertex3fv, __glim_Vertex3fv);
        SET_IMM(gc, 2, __GL_DISP_Vertex4fv, __glim_Vertex4fv);
    }
}

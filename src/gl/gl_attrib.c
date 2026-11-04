/*
** gl_attrib.c -- s_attrib.c: glPushAttrib / glPopAttrib and __glCopyContext (gc->exports.copyContext),
** in the shape of the SGI SI's __glim_PushAttrib /
** __glim_PopAttrib / __glCopyContext.  The ICD is an older SI: one texture-binding copy per target
** (no binding-state allocations), no color tables / convolution filters, and its own dirty bits.
**
** The pushed attribute block is the ICD's real __GLattribute (glctx.h gc->state): gc 0x58 .. 0x8a8
** (0x850 bytes, the size glPushAttrib callocs), mask at gc 0x58 and the current state from gc 0x60
** (rasterPos) on.
** The other s_attrib.c entries follow in address order.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"
#include <string.h>
#include "glsitypes.h"
#include "icdglob.h"
#include <stdlib.h>

/* the attribute block glPushAttrib stacks is glctx.h's __GLattribute (gc->state, gc 0x58 .. 0x8a8,
** 0x850 bytes) with its __GLcurrentState (gc 0x60 .. 0x1a8) */

/* the gc->state field at the same place as `p` in the attribute block `sp` */
#define __GL_STATE_FIELD(gc, sp, p) \
    ((void *) (((GLubyte *) (p) - (GLubyte *) (sp)) + (GLubyte *) &(gc)->state))

#define __GL_LIGHT_SOURCE_SIZE  0x74            /* sizeof(__GLlightSourceState) in the ICD */
#define __GL_TEXENV_SIZE        0x54            /* sizeof the per-unit texture env block */

/* attrib.h enable groups (the ICD's bits) */
#define __GL_COLOR_BUFFER_ENABLES_ICD   0x00a0000f
#define __GL_LIGHTING_ENABLES_ICD       0x004000c0
#define __GL_LINE_ENABLES_ICD           0x00000300
#define __GL_POLYGON_ENABLES_ICD        0x001c3800
#define __GL_AUTO_NORMAL_ENABLE_ICD     0x00020000
#define __GL_NORMALIZE_ENABLE_ICD       0x00010000

/************************************************************************/

/* 690b7180 (size 2151): __glCopyContext (gc->exports.copyContext, wglCopyContext) */
GLboolean __fastcall __glCopyContext(__GLcontext *dst, const __GLcontext *src, GLuint mask)
{
    const __GLattribute *sp;
    GLboolean rv = GL_TRUE;

    sp = &src->state;

    if (mask & GL_ACCUM_BUFFER_BIT) {
	dst->state.accum = sp->accum;
    }

    if (mask & GL_COLOR_BUFFER_BIT) {
	dst->state.raster = sp->raster;
	dst->state.enables.general &= ~__GL_COLOR_BUFFER_ENABLES_ICD;
	dst->state.enables.general |=
	    sp->enables.general & __GL_COLOR_BUFFER_ENABLES_ICD;
	dst->validateMask |= 0x1; /*XXX*/
    }

    if (mask & GL_CURRENT_BIT) {
	dst->state.current = sp->current;
    }

    if (mask & GL_DEPTH_BUFFER_BIT) {
	dst->state.depth = sp->depth;
	dst->state.enables.general &= ~__GL_DEPTH_TEST_ENABLE;
	dst->dirtyMask |= __GL_DIRTY_HWDEPTH_ICD;
	dst->state.enables.general |=
	    sp->enables.general & __GL_DEPTH_TEST_ENABLE;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_DEPTH);
    }

    if (mask & GL_ENABLE_BIT) {
	dst->state.enables = sp->enables;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_LINE | __GL_DIRTY_POLYGON |
		__GL_DIRTY_POINT | __GL_DIRTY_LIGHTING | __GL_DIRTY_DEPTH |
		__GL_DIRTY_SCISSOR);
	(*dst->procs.pickColorMaterialProcs)(dst);
	(*dst->procs.applyColor)(dst);
    }

    if (mask & GL_EVAL_BIT) {
	dst->state.evaluator = sp->evaluator;
	dst->state.enables.general &= ~__GL_AUTO_NORMAL_ENABLE_ICD;
	dst->state.enables.general |=
	    sp->enables.general & __GL_AUTO_NORMAL_ENABLE_ICD;
	dst->state.enables.eval1 = sp->enables.eval1;
	dst->state.enables.eval2 = sp->enables.eval2;
    }

    if (mask & GL_FOG_BIT) {
	dst->state.fog = sp->fog;
	dst->state.enables.general &= ~__GL_FOG_ENABLE;
	dst->dirtyMask |= __GL_DIRTY_HWFOG_ICD;
	dst->state.enables.general |=
	    sp->enables.general & __GL_FOG_ENABLE;
    }

    if (mask & GL_HINT_BIT) {
	dst->state.hints = sp->hints;
    }

    if (mask & GL_LIGHTING_BIT) {
	dst->state.light.colorMaterialFace = sp->light.colorMaterialFace;
	dst->state.light.colorMaterialParam = sp->light.colorMaterialParam;
	if (dst->state.light.shadingModel != sp->light.shadingModel) {
	    dst->state.light.shadingModel = sp->light.shadingModel;
	    dst->dirtyMask |= __GL_DIRTY_SHADEMODEL_ICD;
	}
	dst->state.light.model = sp->light.model;
	dst->state.light.front = sp->light.front;
	dst->state.light.back = sp->light.back;
	__GL_MEMCOPY(dst->state.light.source, sp->light.source,
		     dst->constants.numberOfLights
		     * __GL_LIGHT_SOURCE_SIZE);
	dst->state.enables.general &= ~__GL_LIGHTING_ENABLES_ICD;
	dst->state.enables.general |=
	    sp->enables.general & __GL_LIGHTING_ENABLES_ICD;
	dst->state.enables.lights = sp->enables.lights;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_LIGHTING);
    }

    if (mask & GL_LINE_BIT) {
	dst->state.line = sp->line;
	dst->state.enables.general &= ~__GL_LINE_ENABLES_ICD;
	dst->state.enables.general |=
	    sp->enables.general & __GL_LINE_ENABLES_ICD;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_LINE);
    }

    if (mask & GL_LIST_BIT) {
	dst->state.list = sp->list;
    }

    if (mask & GL_PIXEL_MODE_BIT) {
	dst->state.pixel.readBuffer = sp->pixel.readBuffer;
	dst->state.pixel.readBufferReturn = sp->pixel.readBufferReturn;
	dst->state.pixel.transferMode = sp->pixel.transferMode;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_PIXEL);
    }

    if (mask & GL_POINT_BIT) {
	dst->state.point = sp->point;
	dst->state.enables.general &= ~__GL_POINT_SMOOTH_ENABLE;
	dst->state.enables.general |=
	    sp->enables.general & __GL_POINT_SMOOTH_ENABLE;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_POINT);
    }

    if (mask & GL_POLYGON_BIT) {
	dst->state.polygon = sp->polygon;
	dst->state.enables.general &= ~__GL_POLYGON_ENABLES_ICD;
	dst->state.enables.general |=
	    sp->enables.general & __GL_POLYGON_ENABLES_ICD;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_POLYGON);
    }

    if (mask & GL_POLYGON_STIPPLE_BIT) {
	dst->state.polygonStipple = sp->polygonStipple;
	dst->state.enables.general &= ~__GL_POLYGON_STIPPLE_ENABLE;
	dst->state.enables.general |=
	    sp->enables.general & __GL_POLYGON_STIPPLE_ENABLE;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_POLYGON |
		__GL_DIRTY_POLYGON_STIPPLE);
    }

    if (mask & GL_SCISSOR_BIT) {
	dst->state.scissor = sp->scissor;
	dst->state.enables.general &= ~__GL_SCISSOR_TEST_ENABLE;
	dst->state.enables.general |=
	    sp->enables.general & __GL_SCISSOR_TEST_ENABLE;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_SCISSOR);
    }

    if (mask & GL_STENCIL_BUFFER_BIT) {
	dst->state.stencil = sp->stencil;
	dst->state.enables.general &= ~__GL_STENCIL_TEST_ENABLE;
	dst->state.enables.general |=
	    sp->enables.general & __GL_STENCIL_TEST_ENABLE;
	dst->validateMask |= 0x6; /*XXX*/
    }

    if (mask & GL_TEXTURE_BIT) {
	GLint i;
	GLuint j, targets = dst->constants.numberOfTextures;
	const __GLtexParamICD *tbsSrc;

	/* texture unit state */
	dst->fx.numTMUs = src->fx.numTMUs;
	for (i = 0; i < src->fx.numTMUs; i++) {
	    /* TexGen state */
	    dst->state.texture.unit[i].s = sp->texture.unit[i].s;
	    dst->state.texture.unit[i].t = sp->texture.unit[i].t;
	    dst->state.texture.unit[i].r = sp->texture.unit[i].r;
	    dst->state.texture.unit[i].q = sp->texture.unit[i].q;
	    dst->state.texture.unit[i].scaleBias[0][0] = sp->texture.unit[i].scaleBias[0][0];
	    dst->state.texture.unit[i].scaleBias[0][1] = sp->texture.unit[i].scaleBias[0][1];
	    dst->state.texture.unit[i].scaleBias[0][2] = sp->texture.unit[i].scaleBias[0][2];
	    dst->state.texture.unit[i].scaleBias[0][3] = sp->texture.unit[i].scaleBias[0][3];
	    dst->state.texture.unit[i].scaleBias[1][0] = sp->texture.unit[i].scaleBias[1][0];
	    dst->state.texture.unit[i].scaleBias[1][1] = sp->texture.unit[i].scaleBias[1][1];
	    dst->state.texture.unit[i].scaleBias[1][2] = sp->texture.unit[i].scaleBias[1][2];
	    dst->state.texture.unit[i].scaleBias[1][3] = sp->texture.unit[i].scaleBias[1][3];
	}

	/*
	** Texture binding state.  If the texture name is different, a new
	** binding is called for.
	*/
	tbsSrc = sp->texture.unit[0].param;
	{
	    const __GLtexParamICD *pts, *spPts;

	    pts = (const __GLtexParamICD *) __GL_STATE_FIELD(dst, sp, sp->texture.unit[0].param);
	    spPts = sp->texture.unit[0].param;
	    for (j = 0; j < targets; j++, pts++, spPts++) {
		if (pts->name != spPts->name) {
		    __glBindTexture(dst, j, spPts->name);
		}
	    }
	}

	__GL_MEMCOPY(dst->state.texture.unit[0].param, tbsSrc,
		     targets * sizeof(__GLtexParamICD));

	/* TexEnv state */
	__GL_MEMCOPY(dst->state.texture.unit[0].env, sp->texture.unit[0].env,
		     dst->constants.numberOfTextureEnvs * __GL_TEXENV_SIZE);

	/* texture enable state */
	dst->state.enables.texture[0] &= ~(__GL_TEXTURE_1D_ENABLE | __GL_TEXTURE_2D_ENABLE | __GL_TEXTURE_GEN_ANY_ENABLE);
	dst->state.enables.texture[0] |= sp->enables.texture[0] & (__GL_TEXTURE_1D_ENABLE | __GL_TEXTURE_2D_ENABLE | __GL_TEXTURE_GEN_ANY_ENABLE);
    }

    if (mask & GL_TRANSFORM_BIT) {
	dst->state.transform.matrixMode = sp->transform.matrixMode;
	__GL_MEMCOPY(dst->state.transform.eyeClipPlanes,
		     sp->transform.eyeClipPlanes,
		     dst->constants.numberOfClipPlanes * sizeof(__GLcoord));
	dst->state.enables.general &= ~__GL_NORMALIZE_ENABLE_ICD;
	dst->state.enables.general |=
	  sp->enables.general & __GL_NORMALIZE_ENABLE_ICD;
    }

    if (mask & GL_VIEWPORT_BIT) {
	dst->state.viewport = sp->viewport;
	(*dst->procs.applyViewport)(dst);
    }

    __GL_DELAY_VALIDATE(dst);

    return rv;
}

/************************************************************************/

/* 690b79f0 (size 1267) -- __glim_PushAttrib: glPushAttrib */
void __stdcall __glim_PushAttrib(GLuint mask)
{
    __GLattribute **spp;
    __GLattribute *sp;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    spp = gc->attributes.stackPointer;
    if (spp < &gc->attributes.stack[gc->constants.maxAttribStackDepth]) {
	if (!(sp = *spp)) {
	    sp = (__GLattribute *) __GL_CALLOC(gc, 1, sizeof(__GLattribute));
	    *spp = sp;
	}
	sp->mask = mask;
	sp->enables = gc->state.enables;	/* Always save enables */
	gc->attributes.stackPointer = spp + 1;

	if (mask & GL_ACCUM_BUFFER_BIT) {
	    sp->accum = gc->state.accum;
	}
	if (mask & GL_COLOR_BUFFER_BIT) {
	    sp->raster = gc->state.raster;
	}
	if (mask & GL_CURRENT_BIT) {
	    sp->current = gc->state.current;
	}
	if (mask & GL_DEPTH_BUFFER_BIT) {
	    sp->depth = gc->state.depth;
	}
	if (mask & GL_EVAL_BIT) {
	    sp->evaluator = gc->state.evaluator;
	}
	if (mask & GL_FOG_BIT) {
	    sp->fog = gc->state.fog;
	}
	if (mask & GL_HINT_BIT) {
	    sp->hints = gc->state.hints;
	}
	if (mask & GL_LIGHTING_BIT) {
	    size_t bytes = (size_t)
		(gc->constants.numberOfLights * __GL_LIGHT_SOURCE_SIZE);
	    sp->light.colorMaterialFace = gc->state.light.colorMaterialFace;
	    sp->light.colorMaterialParam = gc->state.light.colorMaterialParam;
	    sp->light.indexMaterialFace = gc->state.light.indexMaterialFace;
	    sp->light.indexMaterialParam = gc->state.light.indexMaterialParam;
	    if (sp->light.shadingModel != gc->state.light.shadingModel) {
		sp->light.shadingModel = gc->state.light.shadingModel;
		gc->dirtyMask |= __GL_DIRTY_SHADEMODEL_ICD;
	    }
	    sp->light.model = gc->state.light.model;
	    sp->light.front = gc->state.light.front;
	    sp->light.back = gc->state.light.back;
	    sp->light.source = (struct __GLlightSourceStateRec *) __GL_MALLOC(gc, bytes);
	    __GL_MEMCOPY(sp->light.source, gc->state.light.source, bytes);
	}
	if (mask & GL_LINE_BIT) {
	    sp->line = gc->state.line;
	}
	if (mask & GL_LIST_BIT) {
	    sp->list = gc->state.list;
	}
	if (mask & GL_PIXEL_MODE_BIT) {
	    sp->pixel.readBuffer = gc->state.pixel.readBuffer;
	    sp->pixel.readBufferReturn = gc->state.pixel.readBufferReturn;
	    sp->pixel.transferMode = gc->state.pixel.transferMode;
	}
	if (mask & GL_POINT_BIT) {
	    sp->point = gc->state.point;
	}
	if (mask & GL_POLYGON_BIT) {
	    sp->polygon = gc->state.polygon;
	}
	if (mask & GL_POLYGON_STIPPLE_BIT) {
	    sp->polygonStipple = gc->state.polygonStipple;
	}
	if (mask & GL_SCISSOR_BIT) {
	    sp->scissor = gc->state.scissor;
	}
	if (mask & GL_STENCIL_BUFFER_BIT) {
	    sp->stencil = gc->state.stencil;
	}
	if (mask & GL_TEXTURE_BIT) {
	    GLint i;
	    size_t paramBytes = (size_t)
		(gc->constants.numberOfTextures * sizeof(__GLtexParamICD));
	    size_t envBytes = (size_t)
		(gc->constants.numberOfTextureEnvs * __GL_TEXENV_SIZE);

	    /* texture unit state */
	    for (i = 0; i < gc->fx.numTMUs; i++) {
		/* TexGen state */
		sp->texture.unit[i].s = gc->state.texture.unit[i].s;
		sp->texture.unit[i].t = gc->state.texture.unit[i].t;
		sp->texture.unit[i].r = gc->state.texture.unit[i].r;
		sp->texture.unit[i].q = gc->state.texture.unit[i].q;
		sp->texture.unit[i].scaleBias[0][0] = gc->state.texture.unit[i].scaleBias[0][0];
		sp->texture.unit[i].scaleBias[0][1] = gc->state.texture.unit[i].scaleBias[0][1];
		sp->texture.unit[i].scaleBias[0][2] = gc->state.texture.unit[i].scaleBias[0][2];
		sp->texture.unit[i].scaleBias[0][3] = gc->state.texture.unit[i].scaleBias[0][3];
		sp->texture.unit[i].scaleBias[1][0] = gc->state.texture.unit[i].scaleBias[1][0];
		sp->texture.unit[i].scaleBias[1][1] = gc->state.texture.unit[i].scaleBias[1][1];
		sp->texture.unit[i].scaleBias[1][2] = gc->state.texture.unit[i].scaleBias[1][2];
		sp->texture.unit[i].scaleBias[1][3] = gc->state.texture.unit[i].scaleBias[1][3];
	    }

	    /* texture binding state (unit 0's targets) */
	    __GL_MEMCOPY(sp->texture.unit[0].param, gc->state.texture.unit[0].param, paramBytes);

	    /* TexEnv state */
	    sp->texture.unit[0].env = (__GLtextureEnvState *) __GL_MALLOC(gc, envBytes);
	    __GL_MEMCOPY(sp->texture.unit[0].env, gc->state.texture.unit[0].env, envBytes);
	}
	if (mask & GL_TRANSFORM_BIT) {
	    size_t bytes = (size_t)
		(gc->constants.numberOfClipPlanes * sizeof(__GLcoord));
	    sp->transform.matrixMode = gc->state.transform.matrixMode;
	    sp->transform.eyeClipPlanes = (__GLcoord *) __GL_MALLOC(gc, bytes);
	    __GL_MEMCOPY(sp->transform.eyeClipPlanes,
			 gc->state.transform.eyeClipPlanes, bytes);
	}
	if (mask & GL_VIEWPORT_BIT) {
	    sp->viewport = gc->state.viewport;
	}
    } else {
	__glSetError(GL_STACK_OVERFLOW);
	return;
    }
}

/************************************************************************/

/* 690b7ef0 (size 2418) -- __glim_PopAttrib: glPopAttrib */
void __fastcall __glim_PopAttrib(void)
{
    __GLattribute **spp;
    __GLattribute *sp;
    GLuint mask;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    spp = gc->attributes.stackPointer;
    if (spp > gc->attributes.stack) {
	--spp;
	sp = *spp;
	assert(sp != 0);
	mask = sp->mask;
	gc->attributes.stackPointer = spp;
	if (mask & GL_ACCUM_BUFFER_BIT) {
	    gc->state.accum = sp->accum;
	}
	if (mask & GL_COLOR_BUFFER_BIT) {
	    gc->state.raster = sp->raster;
	    gc->state.enables.general &= ~__GL_COLOR_BUFFER_ENABLES_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_COLOR_BUFFER_ENABLES_ICD;
	    gc->validateMask |= 0x9;
	}
	if (mask & GL_CURRENT_BIT) {
	    gc->state.current = sp->current;
	}
	if (mask & GL_DEPTH_BUFFER_BIT) {
	    gc->state.depth = sp->depth;
	    gc->state.enables.general &= ~__GL_DEPTH_TEST_ENABLE;
	    gc->dirtyMask |= __GL_DIRTY_HWDEPTH_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_DEPTH_TEST_ENABLE;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
	}
	if (mask & GL_ENABLE_BIT) {
	    gc->state.enables = sp->enables;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE | __GL_DIRTY_POLYGON |
		    __GL_DIRTY_POINT | __GL_DIRTY_LIGHTING | __GL_DIRTY_DEPTH |
		    __GL_DIRTY_PIXEL | __GL_DIRTY_SCISSOR);
	    (*gc->procs.pickColorMaterialProcs)(gc);
	    (*gc->procs.applyColor)(gc);
	    (*gc->procs.computeClipBox)(gc);
	    (*gc->procs.applyScissor)(gc);
	}
	if (mask & GL_EVAL_BIT) {
	    gc->state.evaluator = sp->evaluator;
	    gc->state.enables.general &= ~__GL_AUTO_NORMAL_ENABLE_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_AUTO_NORMAL_ENABLE_ICD;
	    gc->state.enables.eval1 = sp->enables.eval1;
	    gc->state.enables.eval2 = sp->enables.eval2;
	}
	if (mask & GL_FOG_BIT) {
	    gc->state.fog = sp->fog;
	    gc->state.enables.general &= ~__GL_FOG_ENABLE;
	    gc->dirtyMask |= __GL_DIRTY_HWFOG_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_FOG_ENABLE;
	}
	if (mask & GL_HINT_BIT) {
	    gc->state.hints = sp->hints;
	}
	if (mask & GL_LIGHTING_BIT) {
	    gc->state.light.colorMaterialFace = sp->light.colorMaterialFace;
	    gc->state.light.colorMaterialParam = sp->light.colorMaterialParam;
	    gc->state.light.indexMaterialFace = sp->light.indexMaterialFace;
	    gc->state.light.indexMaterialParam = sp->light.indexMaterialParam;
	    if (gc->state.light.shadingModel != sp->light.shadingModel) {
		gc->state.light.shadingModel = sp->light.shadingModel;
		gc->dirtyMask |= __GL_DIRTY_SHADEMODEL_ICD;
	    }
	    gc->state.light.model = sp->light.model;
	    gc->state.light.front = sp->light.front;
	    gc->state.light.back = sp->light.back;
	    __GL_MEMCOPY(gc->state.light.source, sp->light.source,
			 gc->constants.numberOfLights
			     * __GL_LIGHT_SOURCE_SIZE);
	    __GL_FREE(gc, sp->light.source);
	    sp->light.source = NULL;
	    gc->state.enables.general &= ~__GL_LIGHTING_ENABLES_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_LIGHTING_ENABLES_ICD;
	    gc->state.enables.lights = sp->enables.lights;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
	}
	if (mask & GL_LINE_BIT) {
	    gc->state.line = sp->line;
	    gc->state.enables.general &= ~__GL_LINE_ENABLES_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_LINE_ENABLES_ICD;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
	}
	if (mask & GL_LIST_BIT) {
	    gc->state.list = sp->list;
	}
	if (mask & GL_PIXEL_MODE_BIT) {
	    gc->state.pixel.transferMode = sp->pixel.transferMode;
	    gc->state.pixel.readBufferReturn = sp->pixel.readBufferReturn;
	    gc->state.pixel.readBuffer = sp->pixel.readBuffer;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_PIXEL);
	}
	if (mask & GL_POINT_BIT) {
	    gc->state.point = sp->point;
	    gc->state.enables.general &= ~__GL_POINT_SMOOTH_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_POINT_SMOOTH_ENABLE;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POINT);
	}
	if (mask & GL_POLYGON_BIT) {
	    gc->state.polygon = sp->polygon;
	    gc->state.enables.general &= ~__GL_POLYGON_ENABLES_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_POLYGON_ENABLES_ICD;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
	}
	if (mask & GL_POLYGON_STIPPLE_BIT) {
	    gc->state.polygonStipple = sp->polygonStipple;
	    gc->state.enables.general &= ~__GL_POLYGON_STIPPLE_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_POLYGON_STIPPLE_ENABLE;
	    (*gc->procs.convertPolygonStipple)(gc);
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
	}
	if (mask & GL_SCISSOR_BIT) {
	    gc->state.scissor = sp->scissor;
	    gc->state.enables.general &= ~__GL_SCISSOR_TEST_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_SCISSOR_TEST_ENABLE;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_SCISSOR);
	    (*gc->procs.computeClipBox)(gc);
	    (*gc->procs.applyScissor)(gc);
	}
	if (mask & GL_STENCIL_BUFFER_BIT) {
	    gc->state.stencil = sp->stencil;
	    gc->state.enables.general &= ~__GL_STENCIL_TEST_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_STENCIL_TEST_ENABLE;
	    gc->validateMask |= 0x6;
	}
	if (mask & GL_TEXTURE_BIT) {
	    GLint i;
	    GLuint j, targets = gc->constants.numberOfTextures;
	    const __GLtexParamICD *tbsSrc;

	    /* texture unit state */
	    for (i = 0; i < gc->fx.numTMUs; i++) {
		/* TexGen state */
		gc->state.texture.unit[i].s = sp->texture.unit[i].s;
		gc->state.texture.unit[i].t = sp->texture.unit[i].t;
		gc->state.texture.unit[i].r = sp->texture.unit[i].r;
		gc->state.texture.unit[i].q = sp->texture.unit[i].q;
		gc->state.texture.unit[i].scaleBias[0][0] = sp->texture.unit[i].scaleBias[0][0];
		gc->state.texture.unit[i].scaleBias[0][1] = sp->texture.unit[i].scaleBias[0][1];
		gc->state.texture.unit[i].scaleBias[0][2] = sp->texture.unit[i].scaleBias[0][2];
		gc->state.texture.unit[i].scaleBias[0][3] = sp->texture.unit[i].scaleBias[0][3];
		gc->state.texture.unit[i].scaleBias[1][0] = sp->texture.unit[i].scaleBias[1][0];
		gc->state.texture.unit[i].scaleBias[1][1] = sp->texture.unit[i].scaleBias[1][1];
		gc->state.texture.unit[i].scaleBias[1][2] = sp->texture.unit[i].scaleBias[1][2];
		gc->state.texture.unit[i].scaleBias[1][3] = sp->texture.unit[i].scaleBias[1][3];
	    }

	    /*
	    ** Texture binding state.  If the texture name is different, a new
	    ** binding is called for.
	    */
	    tbsSrc = sp->texture.unit[0].param;
	    {
		const __GLtexParamICD *pts, *spPts;

		pts = (const __GLtexParamICD *) __GL_STATE_FIELD(gc, sp, sp->texture.unit[0].param);
		spPts = sp->texture.unit[0].param;
		for (j = 0; j < targets; j++, pts++, spPts++) {
		    if (pts->name != spPts->name) {
			__glBindTexture(gc, j, spPts->name);
		    }
		}
	    }

	    __GL_MEMCOPY(gc->state.texture.unit[0].param, tbsSrc,
			 targets * sizeof(__GLtexParamICD));

	    /* TexEnv state */
	    __GL_MEMCOPY(gc->state.texture.unit[0].env, sp->texture.unit[0].env,
			 gc->constants.numberOfTextureEnvs * __GL_TEXENV_SIZE);
	    __GL_FREE(gc, sp->texture.unit[0].env);
	    sp->texture.unit[0].env = NULL;

	    /* texture enable state */
	    gc->state.enables.texture[0] &= ~(__GL_TEXTURE_1D_ENABLE | __GL_TEXTURE_2D_ENABLE | __GL_TEXTURE_GEN_ANY_ENABLE);
	    gc->state.enables.texture[0] |= sp->enables.texture[0] & (__GL_TEXTURE_1D_ENABLE | __GL_TEXTURE_2D_ENABLE | __GL_TEXTURE_GEN_ANY_ENABLE);
	}
	if (mask & GL_TRANSFORM_BIT) {
	    gc->state.transform.matrixMode = sp->transform.matrixMode;
	    __GL_MEMCOPY(gc->state.transform.eyeClipPlanes,
			 sp->transform.eyeClipPlanes,
			 gc->constants.numberOfClipPlanes * sizeof(__GLcoord));
	    __GL_FREE(gc, sp->transform.eyeClipPlanes);
	    sp->transform.eyeClipPlanes = NULL;
	    gc->state.enables.general &= ~__GL_NORMALIZE_ENABLE_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_NORMALIZE_ENABLE_ICD;
	    gc->state.enables.clipPlanes = sp->enables.clipPlanes;
	}
	if (mask & GL_VIEWPORT_BIT) {
	    gc->state.viewport = sp->viewport;
	    (*gc->procs.applyViewport)(gc);
	}

	/*
	** Clear out mask so that any memory frees done above won't get
	** re-done when the context is destroyed
	*/
	sp->mask = 0;

	__GL_DELAY_VALIDATE(gc);
    } else {
	__glSetError(GL_STACK_UNDERFLOW);
	return;
    }
}

/************************************************************************/
/*
** Vertex arrays (the ICD's s_varray.c, MS-kit style): the cached-vertex renderers for
** GL_LINE_LOOP / GL_POLYGON (gc->vertexArray.drawArrays[]), glDrawElements, and the list-compile
** glDrawArrays / glDrawElements that replay the arrays through glBegin / the gl*v entry points /
** glEnd.
*/

/* the vertex cache: (gc, 0, first, count, elements) sizes and fills it */
#define __GL_VARRAY_CACHE_DIRTY 0x2

/* the gl*v entry points the array fetch procs are (APIENTRY: one pointer, or unit + pointer) */
typedef void (APIENTRY *__GLvaFetchProc)(const GLvoid *);
typedef void (APIENTRY *__GLvaTexFetchProc)(GLint, const GLvoid *);

#define __GL_VA_FETCH(a, i) \
    (*(__GLvaFetchProc) (a).fetch)((const GLvoid *) ((a).pointer + (i) * (a).stride))

/* the validate procs: (gc, v, faceNeeds) */
#define DO_VALIDATE(gc, v) \
    (*(gc)->procs.validateVertex[((v)->flagBits >> 14) & 3])(gc, v, (*(gc)->vertex.getFaceNeeds0)(gc))

/* 690affd0: ClipLine(gc, v0, v1); 690af810: __glVarrayClipTriangle(gc, v0, v1, v2) */
#define __glVarrayClipTriangle(gc, v0, v1, v2) \
    ((void (__fastcall *)(__GLcontext *, __GLvertexICD *, __GLvertexICD *, __GLvertexICD *)) __glDoClipTriangle)(gc, v0, v1, v2)

/* 690b0010 (size 374): DrawVerticesLLoop (drawArrays[GL_LINE_LOOP]): refill the vertex cache if dirty, then
** render consecutive cached vertices (first + i, or elements[i]) as lines -- renderLine when both are unclipped,
** ClipLine when not trivially rejected -- and close the loop last -> first; resets the line stipple, makes the
** second vertex of each segment provoking; only vertices of unclipped segments (and the first) are validated */
void __fastcall DrawVerticesLLoop(__GLcontext *gc, GLint first, GLsizei count, GLuint *elements)
{
    __GLvertexICD *vBuf;
    __GLvertexICD *v0, *v1, *vFirst;
    int i;

    if (gc->vertexArray.flags & __GL_VARRAY_CACHE_DIRTY) {
	(*gc->vertexArray.prepareCache)(gc, 0, first, count, elements);
    }
    vBuf = gc->vertexArray.vertexCache;

    if (elements) {
	v1 = vBuf + elements[0];
    } else {
	v1 = vBuf + first;
    }
    DO_VALIDATE(gc, v1);

    gc->line.notResetStipple = GL_FALSE;

    vFirst = v1;

    for (i = 1; i < count;) {
	v0 = v1;
	if (elements) {
	    v1 = vBuf + elements[i++];
	} else {
	    v1 = vBuf + first + i++;
	}

	gc->vertex.provoking = v1;

	if (((v0->flagBits | v1->flagBits) & __GL_CLIP_ALL_MASK) == 0) {
	    DO_VALIDATE(gc, v1);
	    (*gc->procs.renderLine)(gc, v0, v1);
	} else if ((v0->flagBits & v1->flagBits & __GL_CLIP_ALL_MASK) == 0) {
	    ClipLine(gc, v0, v1);
	}
    }

    /* Draw line from last to first. */
    if (((v1->flagBits | vFirst->flagBits) & __GL_CLIP_ALL_MASK) == 0) {
	(*gc->procs.renderLine)(gc, v1, vFirst);
    } else if ((v1->flagBits & vFirst->flagBits & __GL_CLIP_ALL_MASK) == 0) {
	ClipLine(gc, v1, vFirst);
    }
}

/* 690b0470 (size 403): DrawVerticesPolygon (drawArrays[GL_POLYGON]): refill the vertex cache if dirty, then
** draw the polygon (count >= 3) as a fan of renderTriangle / clip-triangle calls around the first vertex (also the
** provoking one), clearing the edge flags of the interior fan edges for each triangle and restoring them after;
** a trivially rejected triangle `continue`s past the restore, leaving those edge flags cleared */
void __fastcall DrawVerticesPolygon(__GLcontext *gc, GLint first, GLsizei count, GLuint *elements)
{
    __GLvertexICD *vBuf;
    __GLvertexICD *v0, *v1, *v2;
    int i;

    if (count < 3) return;

    if (gc->vertexArray.flags & __GL_VARRAY_CACHE_DIRTY) {
	(*gc->vertexArray.prepareCache)(gc, 0, first, count, elements);
    }
    vBuf = gc->vertexArray.vertexCache;

    if (elements) {
	v0 = vBuf + elements[0];
	v2 = vBuf + elements[1];
    } else {
	v0 = vBuf + first;
	v2 = vBuf + first + 1;
    }

    gc->line.notResetStipple = GL_FALSE;
    gc->vertex.provoking = v0;

    for (i = 2; i < count; ++i) {
	GLuint saveEdgeFlags0, saveEdgeFlags2;

	v1 = v2;
	if (elements) {
	    v2 = vBuf + elements[i];
	} else {
	    v2 = vBuf + first + i;
	}

	saveEdgeFlags0 = v0->flagBits & __GL_VERTEX_EDGE_FLAG;
	if (i != 2) {
	    v0->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
	}
	saveEdgeFlags2 = v2->flagBits & __GL_VERTEX_EDGE_FLAG;
	if (i != count - 1) {
	    v2->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
	}

	if (((v0->flagBits | v1->flagBits | v2->flagBits) &
	     __GL_CLIP_ALL_MASK) == 0) {
	    (*gc->procs.renderTriangle)(gc, v0, v1, v2);
	} else if (v0->flagBits & v1->flagBits & v2->flagBits &
		   __GL_CLIP_ALL_MASK) {
	    continue; /* cull -or- trivial reject */
	} else {
	    __glVarrayClipTriangle(gc, v0, v1, v2);
	}

	v0->flagBits |= saveEdgeFlags0;
	v2->flagBits |= saveEdgeFlags2;
    }
}

/* 690b0dc0 (size 511) -- __glim_DrawElements: glDrawElements */
void __stdcall __glim_DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    GLuint *elements;
    GLuint lockPrim;
    GLint i;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    if (count < 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    if (mode > GL_POLYGON) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    switch (type) {
      case GL_UNSIGNED_BYTE:
	elements = (GLuint *) __GL_MALLOC(gc, count * sizeof(GLuint));
	for (i = 0; i < count; i++) {
	    elements[i] = ((const GLubyte *) indices)[i];
	}
	break;
      case GL_UNSIGNED_SHORT:
	elements = (GLuint *) __GL_MALLOC(gc, count * sizeof(GLuint));
	for (i = 0; i < count; i++) {
	    elements[i] = ((const GLushort *) indices)[i];
	}
	break;
      case GL_UNSIGNED_INT:
	elements = (GLuint *) indices;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    if (count != 0) {
	lockPrim = gc->vertexArray.lockPrimMask & (1 << mode);
	if (lockPrim && ++gc->buffers.lockCnt == 1 && gc->buffers.lockRenderBuffers) {
	    (*gc->buffers.lockRenderBuffers)(gc);
	}
	if (gc->vertexArray.flags & 0x60) {
	    glBegin(mode);
	    for (i = 0; i < count; i++) {
		glArrayElement(elements[i]);
	    }
	    glEnd();
	} else {
	    if ((gc->vertexArray.flags & 2) && (gc->vertexArray.flags & 1)) {
		(*gc->vertexArray.fillLockedRange)(gc, gc->vertexArray.lockFirst,
						   gc->vertexArray.lockFirst,
						   gc->vertexArray.lockCount);
		gc->vertexArray.flags &= ~2;
	    }
	    (*gc->vertexArray.drawArrays[mode])(gc, 0, count, elements);
	}
	if (lockPrim && --gc->buffers.lockCnt == 0 && gc->buffers.unlockRenderBuffers) {
	    (*gc->buffers.unlockRenderBuffers)(gc);
	}
    }

    if (elements != (GLuint *) indices) {
	__GL_FREE(gc, elements);
    }
}

/* the list-compile replay of one array element: the enabled arrays through their gl*v entry points */
#define __GL_VA_REPLAY(gc, enables, i) \
	if (enables & 0x01) { \
	    __GL_VA_FETCH(gc->vertexArray.normal, i); \
	} \
	if (enables & 0x02) { \
	    __GL_VA_FETCH(gc->vertexArray.color, i); \
	} \
	for (unit = 0; unit < gc->fx.numTMUs; unit++) { \
	    if (enables & (0x10000 << unit)) { \
		(*(__GLvaTexFetchProc) gc->vertexArray.texture.fetch[unit])(unit, \
		    (const GLvoid *) (gc->vertexArray.texture.pointer[unit] + \
				      (i) * gc->vertexArray.texture.stride[unit])); \
	    } \
	} \
	if (enables & 0x08) { \
	    __GL_VA_FETCH(gc->vertexArray.index, i); \
	} \
	if (enables & 0x10) { \
	    __GL_VA_FETCH(gc->vertexArray.edgeFlag, i); \
	} \
	if (enables & 0x40) { \
	    __GL_VA_FETCH(gc->vertexArray.fogCoord, i); \
	} \
	if (enables & 0x20) { \
	    __GL_VA_FETCH(gc->vertexArray.vertex, i); \
	}

/* 690b1800 (size 501) -- __gllc_DrawArrays: glDrawArrays while compiling a display list */
void __stdcall __gllc_DrawArrays(GLenum mode, GLint first, GLsizei count)
{
    GLuint enables;
    GLint i, unit;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    enables = gc->vertexArray.enableFlags;
    if ((first < 0) || (count < 0)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    if (mode > GL_POLYGON) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    glBegin(mode);
    count += first;
    for (i = first; i < count; i++) {
	__GL_VA_REPLAY(gc, enables, i)
    }
    glEnd();
}

/* 690b1a20 (size 651) -- __gllc_DrawElements: glDrawElements while compiling a display list */
void __stdcall __gllc_DrawElements(GLenum mode, GLsizei count,
			     GLenum type, const GLvoid *indices)
{
    GLuint enables;
    GLuint *elements;
    GLint i, unit;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    enables = gc->vertexArray.enableFlags;
    if (count < 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    if (mode > GL_POLYGON) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    switch (type) {
      case GL_UNSIGNED_BYTE:
	elements = (GLuint *) __GL_MALLOC(gc, count * sizeof(GLuint));
	for (i = 0; i < count; i++) {
	    elements[i] = ((const GLubyte *) indices)[i];
	}
	break;
      case GL_UNSIGNED_SHORT:
	elements = (GLuint *) __GL_MALLOC(gc, count * sizeof(GLuint));
	for (i = 0; i < count; i++) {
	    elements[i] = ((const GLushort *) indices)[i];
	}
	break;
      case GL_UNSIGNED_INT:
	elements = (GLuint *) indices;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    glBegin(mode);
    for (i = 0; i < count; i++) {
	GLuint e = elements[i];
	__GL_VA_REPLAY(gc, enables, e)
    }
    glEnd();

    if (elements != (GLuint *) indices) {
	__GL_FREE(gc, elements);
    }
}

/* ---- s_attrib.c __glTransformLightDirection: the light's state (gc->state.light.source) and machine
** (gc->light.source) direction, through the model-view's inverse transpose */
typedef void (__fastcall *__GLxf4Proc)(__GLcoord *res, const __GLfloat *v, const void *m);
/************************************************************************/
/* s_attrib.c: glLineWidth / glPointSize (RoundWidth / RoundSize read only the float) */
#define __GL_ROUND(proc, w)     ((GLint (__fastcall *)(__GLfloat)) proc)(w)
#define __GL_MAX_STENCIL_VALUE      ((1 << gc->modes.stencilBits) - 1)
/************************************************************************/
/* s_stencil.c */
#define __GL_STENCIL_OP_CASES \
      case GL_KEEP: case GL_ZERO: case GL_REPLACE: case GL_INCR: case GL_DECR: case GL_INVERT: \
      case 0x8507: case 0x8508:     /* GL_INCR_WRAP_EXT, GL_DECR_WRAP_EXT */
/************************************************************************/
/* s_attrib.c */
#define __GL_SP_FIELD(sp, off)  (*(void **)((GLubyte *)(sp) + (off)))
/************************************************************************/
/* EXT_cull_vertex */
extern void __fastcall __glTransformLightDirection(__GLcontext *gc, __GLlightSourceStateICD *lss);

/************************************************************************/

/* 690b46b0 -- __glim_AlphaFunc */
void __stdcall __glim_AlphaFunc(GLenum af, GLfloat ref)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((af < GL_NEVER) || (af > GL_ALWAYS)) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    if (ref < __GL_ZERO_F) ref = __GL_ZERO_F;
    else if (ref > __GL_ONE_F) ref = __GL_ONE_F;

    gc->state.raster.alphaReference = ref;
    gc->state.raster.alphaFunction = af;
    __GL_DELAY_VALIDATE(gc);
    gc->validateMask |= __GL_VALIDATE_ALPHA_FUNC;
}

/************************************************************************/

/* 690b4aa0 -- __glim_ClearDepth: glClearDepth */
void __stdcall __glim_ClearDepth(GLclampd z)
{
    GLdouble scale;
    __GL_SETUP_NOT_IN_BEGIN();

    if (z < (GLclampd) 0.0) {
	z = (GLclampd) 0.0;
    } else if (z > (GLclampd) 1.0) {
	z = (GLclampd) 1.0;
    }
    gc->state.depth.clear = z;
    if ((z != (GLclampd) 1.0) &&
	(gc->fx.zPrecision32 || gc->fx.zPrecision16) && gc->fx.fullscreen) {
	gc->fx.zPrecision32 = 0;
	gc->fx.zPrecision16 = 0;
	if (gc->fx.lastDepthFunc != gc->fx.depthDirection) {
	    /* glDepthFunc (6901cca0, __stdcall: the depthDirection load in ECX is only the compare) */
	    __glsstim_DepthFunc(gc->fx.lastDepthFunc);
	    ((void (__fastcall *)(__GLcontext *)) hwFlushGrState)(gc);
	    scale = (GLdouble)((__GLfloat)(gc->constants.half) * gc->constants.depthScale);
	    /* the inner cast keeps half first; the outer (GLdouble) one places the polygon stores after the first fmul */
	    gc->state.viewport.scale.z = (gc->state.viewport.zFar - gc->state.viewport.zNear) * scale;
	    gc->state.viewport.center.z = ((GLdouble)(gc->state.viewport.zFar) + gc->state.viewport.zNear) * scale;
	    gc->state.polygon.factor = gc->fx.savedPolygonFactor;
	    gc->state.polygon.units = gc->fx.savedPolygonUnits;
	}
    }
    __GL_DELAY_VALIDATE(gc);
}

/* winReattachFrontBuffer (sstsurf.c): called the first time the front buffer becomes a draw target in a window; the
** drawable's modes byte +0x55 (wglpriv.h singleBuffered, = !doubleBufferMode at creation) records it */

/* 690b4fa0 -- __glim_DrawBuffer */
void __stdcall __glim_DrawBuffer(GLenum mode)
{
    __WGLmodes *modes;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    modes = gc->drawablePrivate->modes;
    switch (mode) {
      default:
	if (!(mode > GL_AUX3 && mode < GL_AUX0+247)) {
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	/* fall through */
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
	{
	    GLint i = mode - GL_AUX0;
	    if (i >= gc->modes.numAuxBuffers) {
		goto not_supported_in_this_implementation;
	    }
	    gc->state.raster.drawBuffer = mode;
	}
	break;
      case GL_FRONT_RIGHT:
      case GL_BACK_RIGHT:
      case GL_RIGHT:
      not_supported_in_this_implementation:
	__glSetError(GL_INVALID_OPERATION);
	return;
      case GL_NONE:
	gc->state.raster.drawBuffer = GL_NONE;
	break;
      case GL_FRONT:
      case GL_FRONT_LEFT:
	gc->state.raster.drawBuffer = GL_FRONT;
	if (!gc->fx.fullscreen && !modes->singleBuffered) {
	    modes->singleBuffered = 1;          /* the front buffer is attached now */
	    winReattachFrontBuffer(gc);
	}
	break;
      case GL_FRONT_AND_BACK:
      case GL_LEFT:
	if (!gc->modes.doubleBufferMode) {
	    gc->state.raster.drawBuffer = GL_FRONT;
	} else {
	    gc->state.raster.drawBuffer = GL_FRONT_AND_BACK;
	}
	if (!gc->fx.fullscreen && !modes->singleBuffered) {
	    modes->singleBuffered = 1;          /* the front buffer is attached now */
	    winReattachFrontBuffer(gc);
	}
	break;
      case GL_BACK:
      case GL_BACK_LEFT:
	if (!gc->modes.doubleBufferMode) {
	    goto not_supported_in_this_implementation;
	}
	gc->state.raster.drawBuffer = GL_BACK;
	break;
    }
    gc->state.raster.drawBufferReturn = mode;
    __GL_DELAY_VALIDATE(gc);
}

/* ---- glHint / glGetTexGeniv ---- */

/* 690b5710 -- __glim_Hint: glHint */
void APIENTRY __glim_Hint(GLenum target, GLenum mode)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (mode) {
      case GL_DONT_CARE:
      case GL_FASTEST:
      case GL_NICEST:
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    switch (target) {
      case GL_PERSPECTIVE_CORRECTION_HINT:
        gc->state.hints.perspectiveCorrection = mode;
        break;
      case GL_POINT_SMOOTH_HINT:
        gc->state.hints.pointSmooth = mode;
        __gl_beginMode = 2; gc->dirtyMask |= __GL_DIRTY_POINT;
        return;
      case GL_LINE_SMOOTH_HINT:
        gc->state.hints.lineSmooth = mode;
        __gl_beginMode = 2; gc->dirtyMask |= __GL_DIRTY_LINE;
        return;
      case GL_POLYGON_SMOOTH_HINT:
        gc->state.hints.polygonSmooth = mode;
        __gl_beginMode = 2; gc->dirtyMask |= __GL_DIRTY_POLYGON;
        return;
      case GL_FOG_HINT:
        gc->state.hints.fog = mode;
        break;
      case 0x80f0:
        gc->state.hints.ext590 = mode;
        break;
      case 0x84ef:
        gc->state.hints.ext594 = mode;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    __gl_beginMode = 2; gc->dirtyMask |= __GL_DIRTY_GENERIC;
}

/* 690b5950 -- __glim_IndexMask: glIndexMask (the ICD: masked to the index buffer's bits, 0 in RGBA mode) */
void __stdcall __glim_IndexMask(GLuint i)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->modes.rgbMode) {
	i = 0;
    } else {
	i &= *(GLuint *)&gc->frontBuffer.redMax;
    }
    gc->state.raster.writeMask = i;
    __GL_DELAY_VALIDATE(gc);
}

/* 690b5a60 -- __glTransformLightDirection */
void __fastcall __glTransformLightDirection(__GLcontext *gc, __GLlightSourceStateICD *lss)
{
    __GLcoord dir;
    __GLfloat q;
    GLint target = lss - (__GLlightSourceStateICD *) gc->state.light.source;
    __GLtransformICD *tr;

    dir.x = lss->direction.x;
    dir.y = lss->direction.y;
    dir.z = lss->direction.z;
    if (lss->position.w != __GL_ZERO_F) {
	q = -(dir.x * lss->position.x + dir.y * lss->position.y +
	      dir.z * lss->position.z) / lss->position.w;
    } else {
	q = __GL_ZERO_F;
    }
    dir.w = q;

    tr = (__GLtransformICD *) gc->transform.modelView;
    if (tr->updateInverse) {
	(*gc->procs.computeInverseTranspose)(gc, tr);
    }
    (*(__GLxf4Proc)tr->inverseTranspose.xf4)(&lss->direction, &dir.x, &tr->inverseTranspose);
    (*gc->procs.normalize)(&((__GLlightSourceMachineICD *) gc->light.source)[target].direction.x,
			   &lss->direction.x);
}

/* 690b5b30 -- __glim_Lightfv: glLightfv */
void __stdcall __glim_Lightfv(GLenum light, GLenum p, const GLfloat pv[])
{
    __GLlightSourceStateICD *lss;
    __GLmatrixICD *m;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    light -= GL_LIGHT0;
    if (light >= (GLuint) gc->constants.numberOfLights){ /* assumes GLenum is unsigned */
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    lss = &((__GLlightSourceStateICD *)gc->state.light.source)[light];
    switch (p) {
      case GL_AMBIENT:
	__glScaleColorf(gc, &lss->ambient, pv);
	break;
      case GL_DIFFUSE:
	__glScaleColorf(gc, &lss->diffuse, pv);
	break;
      case GL_SPECULAR:
	__glScaleColorf(gc, &lss->specular, pv);
	break;
      case GL_POSITION:
	lss->position.x = pv[0];
	lss->position.y = pv[1];
	lss->position.z = pv[2];
	lss->position.w = pv[3];

	/*
	** Transform light position into eye space
	*/
	m = &gc->transform.modelView->matrix;
	(*(__GLxf4Proc)m->xf4)(&lss->positionEye, &lss->position.x, m);
	break;
      case GL_SPOT_DIRECTION:
	lss->direction.x = pv[0];
	lss->direction.y = pv[1];
	lss->direction.z = pv[2];
	lss->direction.w = 1;
	__glTransformLightDirection(gc, lss);
	break;
      case GL_SPOT_EXPONENT:
	if ((pv[0] < 0) || (pv[0] > 128)) {
	  bad_value:
	    __glSetError(GL_INVALID_VALUE);
	    return;
	}
	lss->spotLightExponent = pv[0];
	break;
      case GL_SPOT_CUTOFF:
	if ((pv[0] != 180) && ((pv[0] < 0) || (pv[0] > 90))) {
	    goto bad_value;
	}
	lss->spotLightCutOffAngle = pv[0];
	break;
      case GL_CONSTANT_ATTENUATION:
	if (pv[0] < __GL_ZERO_F) {
	    goto bad_value;
	}
	lss->constantAttenuation = pv[0];
	break;
      case GL_LINEAR_ATTENUATION:
	if (pv[0] < __GL_ZERO_F) {
	    goto bad_value;
	}
	lss->linearAttenuation = pv[0];
	break;
      case GL_QUADRATIC_ATTENUATION:
	if (pv[0] < __GL_ZERO_F) {
	    goto bad_value;
	}
	lss->quadraticAttenuation = pv[0];
	break;
      default:
	goto bad_enum;
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
}

/* 690b5da0 -- __glim_Lightiv: glLightiv */
void __stdcall __glim_Lightiv(GLenum light, GLenum p, const GLint pv[])
{
    __GLlightSourceStateICD *lss;
    __GLmatrixICD *m;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    light -= GL_LIGHT0;
    if (light >= (GLuint) gc->constants.numberOfLights){ /* assumes GLenum is unsigned */
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    lss = &((__GLlightSourceStateICD *)gc->state.light.source)[light];
    switch (p) {
      case GL_AMBIENT:
	__glScaleColori(gc, &lss->ambient, pv);
	break;
      case GL_DIFFUSE:
	__glScaleColori(gc, &lss->diffuse, pv);
	break;
      case GL_SPECULAR:
	__glScaleColori(gc, &lss->specular, pv);
	break;
      case GL_POSITION:
	lss->position.x = pv[0];
	lss->position.y = pv[1];
	lss->position.z = pv[2];
	lss->position.w = pv[3];
	/*
	** Transform light position into eye space
	*/
	m = &gc->transform.modelView->matrix;
	(*(__GLxf4Proc)m->xf4)(&lss->positionEye, &lss->position.x, m);
	break;
      case GL_SPOT_DIRECTION:
	lss->direction.x = pv[0];
	lss->direction.y = pv[1];
	lss->direction.z = pv[2];
	lss->direction.w = 1;
	__glTransformLightDirection(gc, lss);
	break;
      case GL_SPOT_EXPONENT:
	if ((pv[0] < 0) || (pv[0] > 128)) {
	  bad_value:
	    __glSetError(GL_INVALID_VALUE);
	    return;
	}
	lss->spotLightExponent = pv[0];
	break;
      case GL_SPOT_CUTOFF:
	if ((pv[0] != 180) && ((pv[0] < 0) || (pv[0] > 90))) {
	    goto bad_value;
	}
	lss->spotLightCutOffAngle = pv[0];
	break;
      case GL_CONSTANT_ATTENUATION:
	if (pv[0] < 0) {
	    goto bad_value;
	}
	lss->constantAttenuation = pv[0];
	break;
      case GL_LINEAR_ATTENUATION:
	if (pv[0] < 0) {
	    goto bad_value;
	}
	lss->linearAttenuation = pv[0];
	break;
      case GL_QUADRATIC_ATTENUATION:
	if (pv[0] < 0) {
	    goto bad_value;
	}
	lss->quadraticAttenuation = pv[0];
	break;
      default:
	goto bad_enum;
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
}

/************************************************************************/
/* s_light.c */

/* 690b5fe0 -- __glim_LightModelfv: glLightModelfv (two-side also flags the Glide path) */
void __stdcall __glim_LightModelfv(GLenum pname, const GLfloat *params)
{
    __GLlightModelState *model;
    __GL_SETUP_NOT_IN_BEGIN();

    model = &gc->state.light.model;
    switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:
	__glScaleColorf(gc, &model->ambient, params);
	break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	model->localViewer = params[0] != __GL_ZERO_F;
	break;
      case GL_LIGHT_MODEL_TWO_SIDE:
	model->twoSided = params[0] != __GL_ZERO_F;
	gc->fx.flags |= 1;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
}

/************************************************************************/
/* s_attrib.c */

/* 690b6100 -- __glim_LightModeliv (no GL_LIGHT_MODEL_COLOR_CONTROL; two-sided lighting also
** flags the hardware state) */
void __stdcall __glim_LightModeliv(GLenum p, const GLint pv[])
{
    __GLlightModelState *model;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    model = &gc->state.light.model;
    switch (p) {
      case GL_LIGHT_MODEL_AMBIENT:
	__glScaleColori(gc, &model->ambient, pv);
	break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	model->localViewer = pv[0] != 0;
	break;
      case GL_LIGHT_MODEL_TWO_SIDE:
	model->twoSided = pv[0] != 0;
	gc->fx.flags |= 1;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
}

/* 690b6290 -- __glim_LineWidth: glLineWidth */
void __stdcall __glim_LineWidth(GLfloat width)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (width <= 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    gc->state.line.requestedWidth = width;
    gc->state.line.aliasedWidth = __GL_ROUND(RoundWidth, width);
    gc->state.line.smoothWidth = ClampWidth(gc, width);
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
}

/* 690b6340 -- RoundWidth (s_attrib.c): glLineWidth's aliased width: 1 below 1.0, else rounded to the nearest */
GLint __fastcall RoundWidth(__GLcontext *gc, __GLfloat size)
{
    if (size < (__GLfloat) 1.0) return 1;
    return (GLint) (size + (__GLfloat) 0.5);
}

/* ---- s_attrib.c material parameters.  Change bits: ambient 1, diffuse 2, specular 4, emissive 8,
** shininess 0x10, color indexes 0x20.  The ICD adds 0x0d12 / 0x0d13 (GL_INDEX_SHIFT / OFFSET as
** pnames) storing into the two words after cmapd, reported as a color-index change. */
/* 690b6600 -- ApplyParameterF: store one glMaterialfv parameter (pname p, values pv) into the material state
** ms -- emission scaled via __glScaleColorf, the other colors stored raw -- and return its change bit(s) */
GLint __fastcall ApplyParameterF(__GLcontext *gc, __GLmaterialState *ms,
			     GLenum p, const GLfloat pv[])
{
    switch (p) {
      case GL_INDEX_SHIFT:
	ms->indexShift = pv[0];
	return __GL_MATERIAL_COLORINDEXES;
      case GL_INDEX_OFFSET:
	ms->indexOffset = pv[0];
	return __GL_MATERIAL_COLORINDEXES;
      case GL_COLOR_INDEXES:
	ms->cmapa = pv[0];
	ms->cmapd = pv[1];
	ms->cmaps = pv[2];
	return __GL_MATERIAL_COLORINDEXES;
      case GL_EMISSION:
	__glScaleColorf(gc, &ms->emissive, pv);
	return __GL_MATERIAL_EMISSIVE;
      case GL_SPECULAR:
	ms->specular.r = pv[0];
	ms->specular.g = pv[1];
	ms->specular.b = pv[2];
	ms->specular.a = pv[3];
	return __GL_MATERIAL_SPECULAR;
      case GL_SHININESS:
	ms->specularExponent = pv[0];
	return __GL_MATERIAL_SHININESS;
      case GL_AMBIENT:
	ms->ambient.r = pv[0];
	ms->ambient.g = pv[1];
	ms->ambient.b = pv[2];
	ms->ambient.a = pv[3];
	return __GL_MATERIAL_AMBIENT;
      case GL_DIFFUSE:
	ms->diffuse.r = pv[0];
	ms->diffuse.g = pv[1];
	ms->diffuse.b = pv[2];
	ms->diffuse.a = pv[3];
	return __GL_MATERIAL_DIFFUSE;
      case GL_AMBIENT_AND_DIFFUSE:
	ms->ambient.r = pv[0];
	ms->ambient.g = pv[1];
	ms->ambient.b = pv[2];
	ms->ambient.a = pv[3];
	ms->diffuse = ms->ambient;
	return __GL_MATERIAL_DIFFUSE | __GL_MATERIAL_AMBIENT;
      default:
	assert(0);
	return 0;
    }
}

/* 690b68a0 -- ApplyParameterI: ApplyParameterF for glMaterialiv: colors converted with __GL_I_TO_FLOAT (emission
** via __glScaleColori), shininess / indexes / shift / offset as plain int-to-float; returns the change bit(s) */
GLint __fastcall ApplyParameterI(__GLcontext *gc, __GLmaterialState *ms,
			     GLenum p, const GLint pv[])
{
    switch (p) {
      case GL_INDEX_SHIFT:
	ms->indexShift = (__GLfloat) pv[0];
	return __GL_MATERIAL_COLORINDEXES;
      case GL_INDEX_OFFSET:
	ms->indexOffset = (__GLfloat) pv[0];
	return __GL_MATERIAL_COLORINDEXES;
      case GL_COLOR_INDEXES:
	ms->cmapa = pv[0];
	ms->cmapd = pv[1];
	ms->cmaps = pv[2];
	return __GL_MATERIAL_COLORINDEXES;
      case GL_EMISSION:
	__glScaleColori(gc, &ms->emissive, pv);
	return __GL_MATERIAL_EMISSIVE;
      case GL_SPECULAR:
	ms->specular.r = __GL_I_TO_FLOAT(pv[0]);
	ms->specular.g = __GL_I_TO_FLOAT(pv[1]);
	ms->specular.b = __GL_I_TO_FLOAT(pv[2]);
	ms->specular.a = __GL_I_TO_FLOAT(pv[3]);
	return __GL_MATERIAL_SPECULAR;
      case GL_SHININESS:
	ms->specularExponent = pv[0];
	return __GL_MATERIAL_SHININESS;
      case GL_AMBIENT:
	ms->ambient.r = __GL_I_TO_FLOAT(pv[0]);
	ms->ambient.g = __GL_I_TO_FLOAT(pv[1]);
	ms->ambient.b = __GL_I_TO_FLOAT(pv[2]);
	ms->ambient.a = __GL_I_TO_FLOAT(pv[3]);
	return __GL_MATERIAL_AMBIENT;
      case GL_DIFFUSE:
	ms->diffuse.r = __GL_I_TO_FLOAT(pv[0]);
	ms->diffuse.g = __GL_I_TO_FLOAT(pv[1]);
	ms->diffuse.b = __GL_I_TO_FLOAT(pv[2]);
	ms->diffuse.a = __GL_I_TO_FLOAT(pv[3]);
	return __GL_MATERIAL_DIFFUSE;
      case GL_AMBIENT_AND_DIFFUSE:
	ms->ambient.r = __GL_I_TO_FLOAT(pv[0]);
	ms->ambient.g = __GL_I_TO_FLOAT(pv[1]);
	ms->ambient.b = __GL_I_TO_FLOAT(pv[2]);
	ms->ambient.a = __GL_I_TO_FLOAT(pv[3]);
	ms->diffuse = ms->ambient;
	return __GL_MATERIAL_DIFFUSE | __GL_MATERIAL_AMBIENT;
      default:
	assert(0);
	return 0;
    }
}

/* 690b6b30 -- __glim_PointSize: glPointSize */
void __stdcall __glim_PointSize(GLfloat size)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (size <= 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    gc->state.point.requestedSize = size;
    gc->state.point.aliasedSize = __GL_ROUND(RoundSize, size);
    gc->state.point.smoothSize = ClampSize(gc, size);
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POINT);
}

/* 690b6be0 -- RoundSize (s_attrib.c): glPointSize's aliased size: 1 below 1.0, else rounded to the nearest */
GLint __fastcall RoundSize(__GLcontext *gc, __GLfloat size)
{
    if (size < (__GLfloat) 1.0) return 1;
    return (GLint) (size + (__GLfloat) 0.5);
}

/* 690b6f50 -- __glim_StencilFunc: glStencilFunc */
void __stdcall __glim_StencilFunc(GLenum func, GLint ref, GLuint mask)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((func < GL_NEVER) || (func > GL_ALWAYS)) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    __GL_API_STATE();

    if (ref < 0) ref = 0;
    if (ref > __GL_MAX_STENCIL_VALUE) ref = __GL_MAX_STENCIL_VALUE;
    gc->state.stencil.reference = (GLshort) ref;
    gc->state.stencil.testFunc = func;
    gc->state.stencil.mask = (GLshort) (mask & __GL_MAX_STENCIL_VALUE);
    __GL_DELAY_VALIDATE(gc);
    gc->validateMask |= __GL_VALIDATE_STENCIL_FUNC;
}

/* 690b7030 -- __glim_StencilOp: glStencilOp */
void __stdcall __glim_StencilOp(GLenum fail, GLenum depthFail, GLenum depthPass)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (fail) {
      __GL_STENCIL_OP_CASES
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    switch (depthFail) {
      __GL_STENCIL_OP_CASES
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    switch (depthPass) {
      __GL_STENCIL_OP_CASES
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    gc->state.stencil.fail = fail;
    gc->state.stencil.depthFail = depthFail;
    gc->state.stencil.depthPass = depthPass;
    __GL_DELAY_VALIDATE(gc);
    gc->validateMask |= 4;
}

/* 690b8870 -- __glFreeAttributeState: the pushed blocks' light sources, texture envs and
** clip planes (sp +0x3dc, +0x6b4, +0x49c), then the blocks */
void __fastcall __glFreeAttributeState(__GLcontext *gc)
{
    void *sp, **spp;

    for (spp = (void **) gc->attributes.stack;
	 spp < &((void **) gc->attributes.stack)[gc->constants.maxAttribStackDepth];
	 spp++) {
	sp = *spp;
	if (sp) {
	    if (__GL_SP_FIELD(sp, 0x3dc)) {
		__GL_FREE(gc, __GL_SP_FIELD(sp, 0x3dc));
	    }
	    if (__GL_SP_FIELD(sp, 0x6b4)) {
		__GL_FREE(gc, __GL_SP_FIELD(sp, 0x6b4));
	    }
	    if (__GL_SP_FIELD(sp, 0x49c)) {
		__GL_FREE(gc, __GL_SP_FIELD(sp, 0x49c));
	    }
	    __GL_FREE(gc, sp);
	} else
	    break;
    }
}

/************************************************************************/
/* client vertex-array state (ICD: component masks in vertexArray.enables, one bit per array in
** vertexArray.enableFlags) */

/* 690ba3e0 -- __glim_PopClientAttrib: glPopClientAttrib */
void __fastcall __glim_PopClientAttrib(void)
{
    __GLclientAttribute **spp;
    __GLclientAttribute *sp;
    GLuint mask;
    __GL_SETUP_NOT_IN_BEGIN();

    spp = gc->attributes.clientStackPointer;
    if (spp > gc->attributes.clientStack) {
	--spp;
	sp = *spp;
	mask = sp->mask;
	gc->attributes.clientStackPointer = spp;
	if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
	    gc->state.pixel.packModes = sp->packModes;
	    gc->state.pixel.unpackModes = sp->unpackModes;
	}
	if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
	    GLint i;

	    gc->vertexArray.vertex.pointer = sp->vertexArray.vertex.pointer;
	    gc->vertexArray.vertex.size = sp->vertexArray.vertex.size;
	    gc->vertexArray.vertex.type = sp->vertexArray.vertex.type;
	    gc->vertexArray.vertex.stride = sp->vertexArray.vertex.stride;
	    gc->vertexArray.vertex.userStride = sp->vertexArray.vertex.userStride;
	    gc->vertexArray.vertex.count = sp->vertexArray.vertex.count;
	    gc->vertexArray.vertex.fetch = sp->vertexArray.vertex.fetch;

	    gc->vertexArray.normal.pointer = sp->vertexArray.normal.pointer;
	    gc->vertexArray.normal.type = sp->vertexArray.normal.type;
	    gc->vertexArray.normal.stride = sp->vertexArray.normal.stride;
	    gc->vertexArray.normal.userStride = sp->vertexArray.normal.userStride;
	    gc->vertexArray.normal.count = sp->vertexArray.normal.count;
	    gc->vertexArray.normal.fetch = sp->vertexArray.normal.fetch;

	    gc->vertexArray.color.pointer = sp->vertexArray.color.pointer;
	    gc->vertexArray.color.size = sp->vertexArray.color.size;
	    gc->vertexArray.color.type = sp->vertexArray.color.type;
	    gc->vertexArray.color.stride = sp->vertexArray.color.stride;
	    gc->vertexArray.color.userStride = sp->vertexArray.color.userStride;
	    gc->vertexArray.color.count = sp->vertexArray.color.count;
	    gc->vertexArray.color.fetch = sp->vertexArray.color.fetch;

	    gc->vertexArray.index.pointer = sp->vertexArray.index.pointer;
	    gc->vertexArray.index.type = sp->vertexArray.index.type;
	    gc->vertexArray.index.stride = sp->vertexArray.index.stride;
	    gc->vertexArray.index.userStride = sp->vertexArray.index.userStride;
	    gc->vertexArray.index.count = sp->vertexArray.index.count;
	    gc->vertexArray.index.fetch = sp->vertexArray.index.fetch;

	    for (i = 0; i < gc->fx.numTMUs; ++i) {
		gc->vertexArray.texture.pointer[i] = sp->vertexArray.texture.pointer[i];
		gc->vertexArray.texture.size[i] = sp->vertexArray.texture.size[i];
		gc->vertexArray.texture.type[i] = sp->vertexArray.texture.type[i];
		gc->vertexArray.texture.stride[i] = sp->vertexArray.texture.stride[i];
		gc->vertexArray.texture.userStride[i] = sp->vertexArray.texture.userStride[i];
		gc->vertexArray.texture.count[i] = sp->vertexArray.texture.count[i];
		gc->vertexArray.texture.fetch[i] = sp->vertexArray.texture.fetch[i];
	    }

	    gc->vertexArray.edgeFlag.pointer = sp->vertexArray.edgeFlag.pointer;
	    gc->vertexArray.edgeFlag.stride = sp->vertexArray.edgeFlag.stride;
	    gc->vertexArray.edgeFlag.userStride = sp->vertexArray.edgeFlag.userStride;
	    gc->vertexArray.edgeFlag.count = sp->vertexArray.edgeFlag.count;
	    gc->vertexArray.edgeFlag.fetch = sp->vertexArray.edgeFlag.fetch;

	    gc->vertexArray.formats = sp->vertexArray.formats;
	    gc->vertexArray.enables = sp->vertexArray.enables;
	    gc->vertexArray.enableFlags = sp->vertexArray.enableFlags;
	    gc->vertexArray.interleavedPointer = sp->vertexArray.interleavedPointer;
	    gc->vertexArray.interleavedFormat = sp->vertexArray.interleavedFormat;
	    gc->vertexArray.flags = sp->vertexArray.flags;
	}

	/*
	** Clear out mask so that any memory frees done above won't get
	** re-done when the context is destroyed
	*/
	sp->mask = 0;

	__GL_DELAY_VALIDATE(gc);
    } else {
	__glSetError(GL_STACK_UNDERFLOW);
	return;
    }
}

/* 690ba6c0 -- __glim_EnableClientState: glEnableClientState */
void __stdcall __glim_EnableClientState(GLenum array)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (array) {
      case GL_FOG_COORDINATE_ARRAY_EXT:
	gc->vertexArray.enables |= 0x1f00000;
	gc->vertexArray.enableFlags |= __GL_VA_FOGCOORD_ENABLE;
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	return;
      case GL_VERTEX_ARRAY:
	gc->vertexArray.enables |= 0xff;
	gc->vertexArray.enableFlags |= __GL_VA_VERTEX_ENABLE;
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	return;
      case GL_NORMAL_ARRAY:
	gc->vertexArray.enables |= 0xf00;
	gc->vertexArray.enableFlags |= __GL_VA_NORMAL_ENABLE;
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	return;
      case GL_COLOR_ARRAY:
	gc->vertexArray.enables |= 0xff000;
	gc->vertexArray.enableFlags |= __GL_VA_COLOR_ENABLE;
	gc->dirtyMask |= __GL_DIRTY_COLORARRAY_ICD;
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	return;
      case GL_INDEX_ARRAY:
	gc->vertexArray.enables |= 0xf0000000;
	gc->vertexArray.enableFlags |= __GL_VA_INDEX_ENABLE;
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	return;
      case GL_TEXTURE_COORD_ARRAY:
	gc->vertexArray.enableFlags |= __GL_VA_TEXCOORD_ENABLE(gc->fx.clientActiveTexture);
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	return;
      case GL_EDGE_FLAG_ARRAY:
	gc->vertexArray.enables |= 0x8000000;
	gc->vertexArray.enableFlags |= __GL_VA_EDGEFLAG_ENABLE;
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	return;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 690ba8f0 -- __glim_DisableClientState: vertexArray.enables holds component masks per array,
** enableFlags one bit per array (texture coordinates: 0x10000 << client active unit) */
void __stdcall __glim_DisableClientState(GLenum array)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (array) {
      case GL_VERTEX_ARRAY:
	gc->vertexArray.enables &= 0xffffff00;
	gc->vertexArray.enableFlags &= ~__GL_VA_VERTEX_ENABLE;
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	break;
      case GL_NORMAL_ARRAY:
	gc->vertexArray.enables &= 0xfffff0ff;
	gc->vertexArray.enableFlags &= ~__GL_VA_NORMAL_ENABLE;
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	break;
      case GL_COLOR_ARRAY:
	gc->vertexArray.enables &= 0xfff00fff;
	gc->vertexArray.enableFlags &= ~__GL_VA_COLOR_ENABLE;
	gc->dirtyMask |= __GL_DIRTY_COLORARRAY_ICD;
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	break;
      case GL_INDEX_ARRAY:
	gc->vertexArray.enables &= 0x0fffffff;
	gc->vertexArray.enableFlags &= ~__GL_VA_INDEX_ENABLE;
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	break;
      case GL_TEXTURE_COORD_ARRAY:
	gc->vertexArray.enableFlags &= ~__GL_VA_TEXCOORD_ENABLE(gc->fx.clientActiveTexture);
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	break;
      case GL_EDGE_FLAG_ARRAY:
	gc->vertexArray.enables &= 0xf7ffffff;
	gc->vertexArray.enableFlags &= ~__GL_VA_EDGEFLAG_ENABLE;
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	break;
      case GL_FOG_COORDINATE_ARRAY_EXT:
	gc->vertexArray.enables &= 0xfe0fffff;
	gc->vertexArray.enableFlags &= ~__GL_VA_FOGCOORD_ENABLE;
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 690babc0 -- glCullParameterdvEXT */
void __stdcall glCullParameterdvEXT(GLenum pname, const GLdouble *params)
{
    GLfloat v[4];
    GLint i;

    if (pname >= 0x81ab && pname <= 0x81ac) {   /* GL_CULL_VERTEX_EYE_POSITION_EXT, _OBJECT_ */
	for (i = 0; i < 4; i++) {
	    v[i] = (GLfloat) params[i];
	}
	((void (__stdcall *)(GLenum, const GLfloat *)) __glim_CullParameterfvEXT)(pname, v);
	return;
    }
    __glSetError(GL_INVALID_ENUM);
}

extern void __stdcall __glim_Lightfv(GLenum light, GLenum p, const GLfloat pv[]);
extern void __stdcall __glim_Lightiv(GLenum light, GLenum p, const GLint pv[]);
extern void __stdcall __glim_LightModelfv(GLenum pname, const GLfloat *params);
extern void __stdcall __glim_LightModeliv(GLenum p, const GLint pv[]);

/* 690b5140 -- __glim_Fogfv */
void __stdcall __glim_Fogfv(GLenum p, const GLfloat pv[])
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    switch (p) {
      case GL_FOG_COLOR:
        __glClampAndScaleColorf(gc, &gc->state.fog.color, pv);

        gc->state.fog.bufColor.r =
            gc->state.fog.color.r * gc->constants.redRescale;
        gc->state.fog.bufColor.g =
            gc->state.fog.color.g * gc->constants.greenRescale;
        gc->state.fog.bufColor.b =
            gc->state.fog.color.b * gc->constants.blueRescale;

        gc->state.fog.r =
            gc->state.fog.color.r * 255.0f * gc->constants.oneOverRedScale;
        gc->state.fog.g =
            gc->state.fog.color.g * 255.0f * gc->constants.oneOverGreenScale;
        gc->state.fog.b =
            gc->state.fog.color.b * 255.0f * gc->constants.oneOverBlueScale;
        break;
      case GL_FOG_DENSITY:
        if (pv[0] < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        gc->state.fog.density = pv[0];
        break;
      case GL_FOG_END:
        gc->state.fog.end = pv[0];
        break;
      case GL_FOG_START:
        gc->state.fog.start = pv[0];
        break;
      case GL_FOG_INDEX:
        {
            GLint fogIndex, indexMask;

            indexMask = (1 << gc->modes.indexBits) - 1;
            fogIndex = (GLint)pv[0];
            gc->state.fog.index = (__GLfloat)(fogIndex & indexMask);
            gc->state.fog.i = fogIndex & indexMask;
        }
        break;
      case GL_FOG_MODE:
        switch ((GLenum) pv[0]) {
          case GL_EXP:
          case GL_EXP2:
          case GL_LINEAR:
            gc->state.fog.mode = (GLenum) pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case GL_FOG_COORDINATE_SOURCE_EXT:      /* ICD (690b52ef) */
        switch ((GLenum) pv[0]) {
          case GL_FOG_COORDINATE_EXT:
          case GL_FRAGMENT_DEPTH_EXT:
            gc->state.fog.coordSource = (GLenum) pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    /*
    ** Recompute cached 1/(end - start) value for linear fogging.
    */
    if (gc->state.fog.mode == GL_LINEAR) {
        if (gc->state.fog.start != gc->state.fog.end) {
            gc->state.fog.oneOverEMinusS =
                __GL_ONE_F / (gc->state.fog.end - gc->state.fog.start);
        } else {
            /*
            ** Use zero as the undefined value.
            */
            gc->state.fog.oneOverEMinusS = __GL_ZERO_F;
        }
    }

    __GL_DELAY_VALIDATE(gc);
}

/* 690b53b0: __glim_Fogf */
void __stdcall __glim_Fogf(GLenum p, GLfloat f)
{
    switch (p) {
      case GL_FOG_DENSITY:
      case GL_FOG_END:
      case GL_FOG_START:
      case GL_FOG_INDEX:
      case GL_FOG_MODE:
      case GL_FOG_COORDINATE_SOURCE_EXT:
        __glim_Fogfv(p, &f);
        return;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690b53f0 -- __glim_Fogiv */
void __stdcall __glim_Fogiv(GLenum p, const GLint pv[])
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    switch (p) {
      case GL_FOG_COLOR:
        __glClampAndScaleColori(gc, &gc->state.fog.color, pv);

        gc->state.fog.bufColor.r =
            gc->state.fog.color.r * gc->constants.redRescale;
        gc->state.fog.bufColor.g =
            gc->state.fog.color.g * gc->constants.greenRescale;
        gc->state.fog.bufColor.b =
            gc->state.fog.color.b * gc->constants.blueRescale;

        gc->state.fog.r =
            gc->state.fog.color.r * 255.0f * gc->constants.oneOverRedScale;
        gc->state.fog.g =
            gc->state.fog.color.g * 255.0f * gc->constants.oneOverGreenScale;
        gc->state.fog.b =
            gc->state.fog.color.b * 255.0f * gc->constants.oneOverBlueScale;
        break;
      case GL_FOG_DENSITY:
        if (pv[0] < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        gc->state.fog.density = pv[0];
        break;
      case GL_FOG_END:
        gc->state.fog.end = pv[0];
        break;
      case GL_FOG_START:
        gc->state.fog.start = pv[0];
        break;
      case GL_FOG_INDEX:
        {
            GLint fogIndex, indexMask;

            indexMask = (1 << gc->modes.indexBits) - 1;
            fogIndex = (GLint)pv[0];
            gc->state.fog.index = (__GLfloat)(fogIndex & indexMask);
            gc->state.fog.i = fogIndex & indexMask;
        }
        break;
      case GL_FOG_MODE:
        switch ((GLenum) pv[0]) {
          case GL_EXP:
          case GL_EXP2:
          case GL_LINEAR:
            gc->state.fog.mode = (GLenum) pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case GL_FOG_COORDINATE_SOURCE_EXT:      /* ICD (690b52ef) */
        switch ((GLenum) pv[0]) {
          case GL_FOG_COORDINATE_EXT:
          case GL_FRAGMENT_DEPTH_EXT:
            gc->state.fog.coordSource = (GLenum) pv[0];
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    /*
    ** Recompute cached 1/(end - start) value for linear fogging.
    */
    if (gc->state.fog.mode == GL_LINEAR) {
        if (gc->state.fog.start != gc->state.fog.end) {
            gc->state.fog.oneOverEMinusS =
                __GL_ONE_F / (gc->state.fog.end - gc->state.fog.start);
        } else {
            /*
            ** Use zero as the undefined value.
            */
            gc->state.fog.oneOverEMinusS = __GL_ZERO_F;
        }
    }

    __GL_DELAY_VALIDATE(gc);
}

/* 690b5640: __glim_Fogi */
void __stdcall __glim_Fogi(GLenum p, GLint i)
{
    switch (p) {
      case GL_FOG_DENSITY:
      case GL_FOG_END:
      case GL_FOG_START:
      case GL_FOG_INDEX:
      case GL_FOG_MODE:
      case GL_FOG_COORDINATE_SOURCE_EXT:
        __glim_Fogiv(p, &i);
        return;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690b5d60: __glim_Lightf */
void __stdcall __glim_Lightf(GLenum light, GLenum p, GLfloat f)
{
    switch (p) {
      case GL_SPOT_EXPONENT:
      case GL_SPOT_CUTOFF:
      case GL_CONSTANT_ATTENUATION:
      case GL_LINEAR_ATTENUATION:
      case GL_QUADRATIC_ATTENUATION:
        __glim_Lightfv(light, p, &f);
        return;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690b5fa0: __glim_Lighti */
void __stdcall __glim_Lighti(GLenum light, GLenum p, GLint i)
{
    switch (p) {
      case GL_SPOT_EXPONENT:
      case GL_SPOT_CUTOFF:
      case GL_CONSTANT_ATTENUATION:
      case GL_LINEAR_ATTENUATION:
      case GL_QUADRATIC_ATTENUATION:
        __glim_Lightiv(light, p, &i);
        return;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690b60d0: __glim_LightModelf */
void __stdcall __glim_LightModelf(GLenum p, GLfloat f)
{
    if (p >= GL_LIGHT_MODEL_LOCAL_VIEWER && p <= GL_LIGHT_MODEL_TWO_SIDE) {
        __glim_LightModelfv(p, &f);
        return;
    }
    __glSetError(GL_INVALID_ENUM);
}

/* 690b61d0: __glim_LightModeli */
void __stdcall __glim_LightModeli(GLenum p, GLint i)
{
    if (p >= GL_LIGHT_MODEL_LOCAL_VIEWER && p <= GL_LIGHT_MODEL_TWO_SIDE) {
        __glim_LightModeliv(p, &i);
        return;
    }
    __glSetError(GL_INVALID_ENUM);
}

/* 690b6500 -- __glim_Materialfv */
void __stdcall __glim_Materialfv(GLenum face, GLenum p, const GLfloat pv[])
{
    GLenum error;
    GLint frontChange, backChange;
    __GL_SETUP();

    error = __glErrorCheckMaterial(face, p, pv[0]);
    if (error != GL_NO_ERROR) {
        __glSetError(error);
        return;
    }

    /*
    ** If we are in the middle of contructing a primitive, we possibly
    ** need to validate the front and back colors of the vertices which
    ** we have queued.
    */
    if (__GL_IN_BEGIN()) {
        if ((*gc->vertex.getMaterialNeeds)(gc)) (*gc->procs.matValidate)(gc);
    }

    switch (face) {
      case GL_FRONT:
        frontChange = ApplyParameterF(gc, &gc->state.light.front, p, pv);
        backChange = 0;
        break;
      case GL_BACK:
        backChange = ApplyParameterF(gc, &gc->state.light.back, p, pv);
        frontChange = 0;
        break;
      case GL_FRONT_AND_BACK:
        backChange = ApplyParameterF(gc, &gc->state.light.back, p, pv);
        frontChange = ApplyParameterF(gc, &gc->state.light.front, p, pv);
        break;
    }

    __glValidateMaterial(gc, frontChange, backChange);

    if (gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE) {
        (*gc->procs.applyColor)(gc);
    }
}

/* 690b6770 -- s_attrib.c __glim_Materialf */
void __stdcall __glim_Materialf(GLenum face, GLenum p, GLfloat f)
{
    /* Accept only enumerants that correspond to single values */
    switch (p) {
      case GL_SHININESS:
        __glim_Materialfv(face, p, &f);
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690b67a0 -- __glim_Materialiv */
void __stdcall __glim_Materialiv(GLenum face, GLenum p,
                      const GLint pv[])
{
    GLenum error;
    GLint frontChange, backChange;
    __GL_SETUP();

    error = __glErrorCheckMaterial(face, p, pv[0]);
    if (error != GL_NO_ERROR) {
        __glSetError(error);
        return;
    }

    /*
    ** If we are in the middle of contructing a primitive, we possibly
    ** need to validate the front and back colors of the vertices which
    ** we have queued.
    */
    if (__GL_IN_BEGIN()) {
        if ((*gc->vertex.getMaterialNeeds)(gc)) (*gc->procs.matValidate)(gc);
    }

    switch (face) {
      case GL_FRONT:
        frontChange = ApplyParameterI(gc, &gc->state.light.front, p, pv);
        backChange = 0;
        break;
      case GL_BACK:
        backChange = ApplyParameterI(gc, &gc->state.light.back, p, pv);
        frontChange = 0;
        break;
      case GL_FRONT_AND_BACK:
        backChange = ApplyParameterI(gc, &gc->state.light.back, p, pv);
        frontChange = ApplyParameterI(gc, &gc->state.light.front, p, pv);
        break;
    }

    __glValidateMaterial(gc, frontChange, backChange);

    if (gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE) {
        (*gc->procs.applyColor)(gc);
    }
}

/* 690b6af0 -- s_attrib.c __glim_Materiali */
void __stdcall __glim_Materiali(GLenum face, GLenum p, GLint i)
{
    /* Accept only enumerants that correspond to single values */
    switch (p) {
      case GL_SHININESS:
      case GL_INDEX_OFFSET:
      case GL_INDEX_SHIFT:
        __glim_Materialiv(face, p, &i);
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

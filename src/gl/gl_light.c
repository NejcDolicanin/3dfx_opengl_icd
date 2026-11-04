/*
** gl_light.c -- original VA range 69090000..69094000: s_light.c colour-material changes,
** __glValidateMaterial / __glValidateLighting and the modelview part of the light validation.
** Then the rest of s_light.c and s_lcache.c, in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "glsitypes.h"
#include "glshade.h"
#include "si_names.h"
#include "icdprotos.h"
#include "icdglob.h"
#include <stddef.h>
#include "og.h"
extern void __fastcall __glClampAndScaleColor(__GLcontext *gc);
extern void __fastcall ComputeMaterialState(__GLcontext *gc, __GLmaterialState *ms, __GLmaterialMachineICD *msm,
                                    GLint changeBits);                  /* ComputeMaterialState */
extern void __fastcall ComputeLightMaterialState(__GLcontext *gc, GLint frontChange, GLint backChange);
extern void __fastcall ComputeIndexMaterialState(__GLcontext *gc, GLuint edx_unused, GLuint unused);   /* colour-index material */
extern void __fastcall ComputeLightState(__GLcontext *gc);
extern void __fastcall ComputeLightProcs(__GLcontext *gc);

void __fastcall __glValidateMaterial(__GLcontext *gc, GLint frontChange, GLint backChange);

/* 69091cc0 -- __glChangeOneMaterialColor (s_light.c, procs.applyColor): the ICD asks
** vertex.getMaterialNeeds instead of reading vertex.materialNeeds.
*/
void __fastcall __glChangeOneMaterialColor(__GLcontext *gc)
{
    /* inside Begin the queued vertices may need their colours validated first */
    if (__GL_IN_BEGIN() && (*gc->vertex.getMaterialNeeds)(gc)) {
        (*gc->procs.matValidate)(gc);
    }

    (*gc->procs.changeMaterial)(gc, gc->light.cm, gc->light.cmm);

    if (!(gc->state.enables.general & __GL_LIGHTING_ENABLE)) {
        /* colour material without lighting: clamp current.color after the material update */
        __glClampAndScaleColor(gc);
    }
}

/* 69091d20 -- __glChangeBothMaterialColors (s_light.c, procs.applyColor) */
void __fastcall __glChangeBothMaterialColors(__GLcontext *gc)
{
    if (__GL_IN_BEGIN() && (*gc->vertex.getMaterialNeeds)(gc)) {
        (*gc->procs.matValidate)(gc);
    }

    (*gc->procs.changeMaterial)(gc, &gc->state.light.front, &gc->light.front);
    (*gc->procs.changeMaterial)(gc, &gc->state.light.back, &gc->light.back);

    if (!(gc->state.enables.general & __GL_LIGHTING_ENABLE)) {
        __glClampAndScaleColor(gc);
    }
}

/* 69091df0 -- __glValidateMaterial (s_light.c): recompute the light state for a material change.
** ICD: also the faces' colour-index material (69092150).
*/
void __fastcall __glValidateMaterial(__GLcontext *gc, GLint frontChange, GLint backChange)
{
    ComputeMaterialState(gc, &gc->state.light.front, &gc->light.front, frontChange);
    ComputeMaterialState(gc, &gc->state.light.back, &gc->light.back, backChange);
    ComputeLightMaterialState(gc, frontChange, backChange);
    ComputeIndexMaterialState(gc, frontChange, backChange);
}

/* 690921d0 -- __glValidateLighting (s_light.c) */
void __fastcall __glValidateLighting(__GLcontext *gc)
{
    if (gc->dirtyMask & __GL_DIRTY_LIGHTING) {
        ComputeLightState(gc);
        ComputeLightProcs(gc);
        __glValidateModelViewForLighting(gc);
        __glValidateMaterial(gc, __GL_MATERIAL_ALL, __GL_MATERIAL_ALL);
    } else {
        ComputeLightProcs(gc);
    }
}

/* 69092c80 -- __glValidateModelViewForLighting (s_light.c): the eye-space light vectors back into
** model space.
*/
void __fastcall __glValidateModelViewForLighting(__GLcontext *gc)
{
    if (gc->state.enables.general & __GL_LIGHTING_ENABLE) {
        __GLtransformICD *tr = gc->transform.modelView;
        __GLlightSourceMachineICD *lsm = gc->light.sources;

        if (tr->updateInverse) {
            (*gc->procs.computeInverseTranspose)(gc, tr);
        }

        for (; lsm; lsm = lsm->next) {
            __glXForm4_Pre(&lsm->unitVPpliModel, &lsm->unitVPpli.x, &tr->inverseTranspose);
            __glXForm4_Pre(&lsm->hHatModel, &lsm->hHat.x, &tr->inverseTranspose);
        }
    }
}

#define MSM_F(m, i)     *(__GLfloat *)&(&(m)->cmapd_minus_cmapa)[i]     /* +0x58 cmapd - cmapa, +0x5c cmaps - cmapa */
#define __GL_MSM_CMAPD_MINUS_CMAPA(msm) ((msm)->cmapd_minus_cmapa)
#define __GL_MSM_CMAPS_MINUS_CMAPA(msm) (msm)->cmaps_minus_cmapa
#define __GL_MS_INDEX_SHIFT(ms)         ((ms)->indexShift)
typedef GLuint (__fastcall *__GLpackColor)(__GLcontext *gc, const __GLcolor *color);
#define CMAPD_MINUS_CMAPA(msm)  ((msm)->cmapd_minus_cmapa)
#define CMAPS_MINUS_CMAPA(msm)  ((msm)->cmaps_minus_cmapa)
/* ---- EXT_index_material: where CI lighting takes each face's index from ------------------------ */
#define __GL_INDEX_MATERIAL_ENABLE  0x400000
#define __glDegreesToRadians    0.017453292f
typedef void (__fastcall *__GLnormalize)(__GLfloat *dst, const __GLfloat *src);
#define MS_INDEX_OFFSET(ms)     ((ms)->indexOffset)   /* ms +0x50 */
#define MS_INDEX_SHIFT(ms)      ((ms)->indexShift)   /* ms +0x54 */
#define INDEX_MATERIAL_ENABLE   (1 << 22)
#define SHADE_CHEAP_FOG         0x00002000
#define SHADE_SMOOTH_LIGHT      0x00020000
extern void __fastcall __glCalcRGBColor(__GLcontext *gc, GLint face, __GLvertexICD *vx);
#define TOO_MANY_LUT_ENTRIES    32
/* the SI order (cache, exp, location): fastcall passes the float on the stack, so the location is
** still in EDX; the arguments are evaluated right to left in this order */
extern __GLspecLUTEntry *__fastcall findEntry(__GLspecLUTCache *lutCache, __GLfloat exp, GLint *location);
extern void __fastcall __glFastCalcRGBColor(__GLcontext *gc, GLint face, __GLvertexICD *vx);
extern void __fastcall __glFastCalcCIColor(__GLcontext *gc, GLint face, __GLvertexICD *vx);
extern void __fastcall __glFastCalcCIColorOffset(__GLcontext *gc, GLint face, __GLvertexICD *vx);
extern void __fastcall __glChangeOneMaterialColor(__GLcontext *gc);
extern void __fastcall __glChangeBothMaterialColors(__GLcontext *gc);
extern void __fastcall ChangeMaterialEmission(__GLcontext *gc, __GLmaterialState *ms, __GLmaterialMachineICD *msm);
extern void __fastcall ChangeMaterialSpecular(__GLcontext *gc, __GLmaterialState *ms, __GLmaterialMachineICD *msm);
extern void __fastcall ChangeMaterialAmbient(__GLcontext *gc, __GLmaterialState *ms, __GLmaterialMachineICD *msm);
extern void __fastcall ChangeMaterialDiffuse(__GLcontext *gc, __GLmaterialState *ms, __GLmaterialMachineICD *msm);
extern void __fastcall ChangeMaterialAmbientAndDiffuse(__GLcontext *gc, __GLmaterialState *ms, __GLmaterialMachineICD *msm);

/************************************************************************/
/* s_light.c */

/* 69090ca0 -- __glFastCalcRGBColor */
void __fastcall __glFastCalcRGBColor(__GLcontext *gc, GLint face, __GLvertexICD *vx)
{
    __GLlightSourceMachineICD *lsm;
    __GLmaterialMachineICD *msm;
    __GLmaterialState *ms;
    __GLcolor *c;
    __GLfloat r, g, b;
    __GLfloat nx, ny, nz;
    __GLfloat zero;

    zero = __GL_ZERO_F;

    /* Pick material to use */
    if (face == 0) {
        c = &vx->colors[0];
        msm = &gc->light.front;
        ms  = &gc->state.light.front;
        nx = vx->normal.x;
        ny = vx->normal.y;
        nz = vx->normal.z;
    } else {
        c = &vx->colors[1];
        msm = &gc->light.back;
        ms  = &gc->state.light.back;
        nx = -vx->normal.x;
        ny = -vx->normal.y;
        nz = -vx->normal.z;
    }

    /* Compute the color */
    r = msm->sceneColor.r;
    g = msm->sceneColor.g;
    b = msm->sceneColor.b;
    lsm = (__GLlightSourceMachineICD *)gc->light.sources;

    while (lsm) {
        __GLfloat n1, n2;
        __GLlightSourceStateICD *lss;

        lss = lsm->state;

        r += lss->ambient.r * ms->ambient.r;
        g += lss->ambient.g * ms->ambient.g;
        b += lss->ambient.b * ms->ambient.b;

        /* Add in specular and diffuse effect of light, if any */
        n1 = nx * lsm->unitVPpli.x +
             ny * lsm->unitVPpli.y +
             nz * lsm->unitVPpli.z;

        if (n1 > zero) {
            n2 = nx * lsm->hHat.x +
                 ny * lsm->hHat.y +
                 nz * lsm->hHat.z;
            n2 -= msm->threshold;
            if (n2 >= zero) {
                GLuint ix = (GLuint)(n2 * msm->scale + gc->constants.half);
                if (ix < __GL_SPEC_LOOKUP_TABLE_SIZE) {
                    n2 = msm->specTable[ix];
                }
                else {
                    n2 = __GL_ONE_F;
                }
                r += lss->specular.r * ms->specular.r * n2;
                g += lss->specular.g * ms->specular.g * n2;
                b += lss->specular.b * ms->specular.b * n2;
            }
            r += lss->diffuse.r * ms->diffuse.r * n1;
            g += lss->diffuse.g * ms->diffuse.g * n1;
            b += lss->diffuse.b * ms->diffuse.b * n1;
        }
        lsm = lsm->next;
    }

    /* Clamp the computed color */
    {
        __GLfloat rl, gl, bl;
        rl = gc->constants.redScale;
        gl = gc->constants.greenScale;
        bl = gc->constants.blueScale;
        if (r <= zero) {
            r = zero;
        } else {
            if (r >= rl) {
                r = rl;
            }
        }
        if (g <= zero) {
            g = zero;
        } else {
            if (g >= gl) {
                g = gl;
            }
        }
        if (b <= zero) {
            b = zero;
        } else {
            if (b >= bl) {
                b = bl;
            }
        }
        c->r = r;
        c->g = g;
        c->b = b;
        c->a = msm->alpha;
    }
}

/* 690916e0 -- __glFastCalcCIColor */
void __fastcall __glFastCalcCIColor(__GLcontext *gc, GLint face, __GLvertexICD *vx)
{
    __GLmaterialMachineICD *msm;
    __GLmaterialState *ms;
    __GLlightSourceMachineICD *lsm;
    __GLcolor *c;
    __GLfloat s, d, zero, nx, ny, nz;

    zero = __GL_ZERO_F;

    /* Pick material to use */
    if (face == 0) {
        c = &vx->colors[0];
        ms = &gc->state.light.front;
        msm = &gc->light.front;
        nx = vx->normal.x;
        ny = vx->normal.y;
        nz = vx->normal.z;
    } else {
        c = &vx->colors[1];
        ms = &gc->state.light.back;
        msm = &gc->light.back;
        nx = -vx->normal.x;
        ny = -vx->normal.y;
        nz = -vx->normal.z;
    }

    s = zero;
    d = zero;
    lsm = (__GLlightSourceMachineICD *)gc->light.sources;
    while (lsm) {
        __GLfloat n1;

        /* Compute specular contribution */
        n1 = nx * lsm->unitVPpli.x + ny * lsm->unitVPpli.y +
            nz * lsm->unitVPpli.z;
        if (n1 > zero) {
            d += n1 * lsm->dli;
            {
                __GLfloat n2;
                n2 = nx * lsm->hHat.x + ny * lsm->hHat.y + nz * lsm->hHat.z;
                n2 -= msm->threshold;
                if (n2 >= zero) {
                    GLuint ix = (GLuint)(n2 * msm->scale + gc->constants.half);
                    if (ix < __GL_SPEC_LOOKUP_TABLE_SIZE) {
                        n2 = msm->specTable[ix];
                        s += n2 * lsm->sli;
                    } else {
                        s += lsm->sli;
                    }
                }
            }
        }
        lsm = lsm->next;
    }

    /* Compute final color */
    if (s >= __GL_ONE_F) {
        c->r = ms->cmaps;
    } else {
        __GLfloat ci;
        ci = ms->cmapa + (__GL_ONE_F - s) * d * MSM_F(msm, 0)
            + s * MSM_F(msm, 1);
        if (ci > ms->cmaps) {
            ci = ms->cmaps;
        }
        c->r = ci;
    }
}

/* 69091890 -- __glFastCalcCIColorOffset: s_light.c __glFastCalcCIColor, plus the ICD's index fix-up (offset by the
** face's index source, shifted by the material's index shift) */
void __fastcall __glFastCalcCIColorOffset(__GLcontext *gc, GLint face, __GLvertexICD *vx)
{
    __GLmaterialMachineICD *msm;
    __GLmaterialState *ms;
    __GLlightSourceMachineICD *lsm;
    __GLcolor *c;
    __GLfloat s, d, zero, nx, ny, nz;
    GLint ix, shift, base;

    zero = __GL_ZERO_F;

    /* Pick material to use */
    if (face == 0) {
	c = &vx->colors[0];
	ms = &gc->state.light.front;
	msm = &gc->light.front;
	nx = vx->normal.x;
	ny = vx->normal.y;
	nz = vx->normal.z;
    } else {
	c = &vx->colors[1];
	ms = &gc->state.light.back;
	msm = &gc->light.back;
	nx = -vx->normal.x;
	ny = -vx->normal.y;
	nz = -vx->normal.z;
    }

    s = zero;
    d = zero;
    lsm = (__GLlightSourceMachineICD *)gc->light.sources;
    while (lsm) {
	__GLfloat n1;

	/* Compute specular contribution */
	n1 = nx * lsm->unitVPpli.x + ny * lsm->unitVPpli.y +
	    nz * lsm->unitVPpli.z;
	if (n1 > zero) {
	    d += n1 * lsm->dli;
	    {
		__GLfloat n2;
		n2 = nx * lsm->hHat.x + ny * lsm->hHat.y + nz * lsm->hHat.z;
		n2 -= msm->threshold;
		if (n2 >= zero) {
		    GLuint ix = (GLuint)(n2 * msm->scale + gc->constants.half);
		    if (ix < 256) {
			n2 = msm->specTable[ix];
			s += n2 * lsm->sli;
		    } else {
			s += lsm->sli;
		    }
		}
	    }
	}
	lsm = lsm->next;
    }

    /* Compute final color */
    if (s >= __GL_ONE_F) {
	c->r = ms->cmaps;
    } else {
	__GLfloat ci;
	ci = ms->cmapa + (__GL_ONE_F - s) * __GL_MSM_CMAPD_MINUS_CMAPA(msm) * d
	    + s * __GL_MSM_CMAPS_MINUS_CMAPA(msm);
	if (ci > ms->cmaps) {
	    ci = ms->cmaps;
	}
	c->r = ci;
    }

    /* ICD: rebase the index on the face's index source */
    ix = (GLint)(c->r * 256.0f + 0.5f);
    shift = (GLint)__GL_MS_INDEX_SHIFT(ms);
    base = (GLint)*msm->indexOffsetPtr;
    if (shift > 0) {
	ix <<= shift;
    } else {
	ix >>= -shift;
    }
    c->r = (base * 256 + ix) * (1.0f / 256.0f);
}

/* 69091b30 -- __glClampAndScaleColor (s_light.c), then the ICD packs the colour */
void __fastcall __glClampAndScaleColor(__GLcontext *gc)
{
    __GLfloat r, g, b, a;

    r = gc->state.current.userColor.r;
    g = gc->state.current.userColor.g;
    b = gc->state.current.userColor.b;
    a = gc->state.current.userColor.a;

    if (r <= __GL_ZERO_F) {
	gc->state.current.color.r = __GL_ZERO_F;
    } else
    if (r >= __GL_ONE_F) {
	gc->state.current.color.r = gc->constants.redScale;
    } else {
	gc->state.current.color.r = r * gc->constants.redScale;
    }

    if (g <= __GL_ZERO_F) {
	gc->state.current.color.g = __GL_ZERO_F;
    } else
    if (g >= __GL_ONE_F) {
	gc->state.current.color.g = gc->constants.greenScale;
    } else {
	gc->state.current.color.g = g * gc->constants.greenScale;
    }

    if (b <= __GL_ZERO_F) {
	gc->state.current.color.b = __GL_ZERO_F;
    } else
    if (b >= __GL_ONE_F) {
	gc->state.current.color.b = gc->constants.blueScale;
    } else {
	gc->state.current.color.b = b * gc->constants.blueScale;
    }

    if (a <= __GL_ZERO_F) {
	gc->state.current.color.a = __GL_ZERO_F;
    } else
    if (a >= __GL_ONE_F) {
	gc->state.current.color.a = gc->constants.alphaScale;
    } else {
	gc->state.current.color.a = a * gc->constants.alphaScale;
    }
    gc->state.current.packedColor =
	(*(__GLpackColor)gc->procs.packColor)(gc, &gc->state.current.color);
}

/* 69091e40 -- ComputeMaterialState (s_light.c): refresh one face's material machine msm from its state ms for
** changeBits (nothing unless emissive/ambient/diffuse/shininess/indexes changed): rebuild the specular LUT when
** none is cached or the exponent differs, sceneColor = emissive + ambient * model ambient, alpha = diffuse alpha
** clamped to [0, alphaScale], and the CI cmapd - cmapa / cmaps - cmapa differences */
void __fastcall ComputeMaterialState(__GLcontext *gc, __GLmaterialState *ms,
			     __GLmaterialMachineICD *msm, GLint changeBits)
{
    GLdouble  exponent;
    __GLspecLUTEntry *lut;

    if ((changeBits & (__GL_MATERIAL_EMISSIVE | __GL_MATERIAL_AMBIENT | 
	    __GL_MATERIAL_DIFFUSE | __GL_MATERIAL_SHININESS |
            __GL_MATERIAL_COLORINDEXES)) == 0) {
	return;
    }
    /* Only compute specular lookup table when it changes */
    if (!msm->cache || (ms->specularExponent != msm->specularExponent)) {
	exponent = msm->specularExponent = ms->specularExponent;

	__glFreeSpecLUT(gc, msm->cache);
	lut = msm->cache = __glCreateSpecLUT(gc, exponent);
	msm->threshold = lut->threshold;
	msm->scale = lut->scale;
	msm->specTable = lut->table;
    }

    /* Compute scene color */
    if (changeBits & (__GL_MATERIAL_EMISSIVE | __GL_MATERIAL_AMBIENT)) {
	msm->sceneColor.r = ms->emissive.r
	    + ms->ambient.r * gc->state.light.model.ambient.r;
	msm->sceneColor.g = ms->emissive.g
	    + ms->ambient.g * gc->state.light.model.ambient.g;
	msm->sceneColor.b = ms->emissive.b
	    + ms->ambient.b * gc->state.light.model.ambient.b;
    }

    /* Clamp material alpha */
    if (changeBits & __GL_MATERIAL_DIFFUSE) {
	msm->alpha = ms->diffuse.a * gc->constants.alphaScale;
	if (msm->alpha < __GL_ZERO_F) {
	    msm->alpha = __GL_ZERO_F;
	} else if (msm->alpha > gc->constants.alphaScale) {
	    msm->alpha = gc->constants.alphaScale;
	}
    }

    /* Precompute some CI lighting expressions */
    if (changeBits & __GL_MATERIAL_COLORINDEXES) {
        CMAPD_MINUS_CMAPA(msm) = ms->cmapd - ms->cmapa;
        CMAPS_MINUS_CMAPA(msm) = ms->cmaps - ms->cmapa;
    }
}

/* light source types */

/* 69091f40 -- ComputeLightMaterialState (s_light.c): for every enabled light source recompute the per-face
** products of light and material ambient / diffuse / specular colours that frontChange / backChange name, and,
** after a front ambient or emissive change, set light.baseColor = front sceneColor + the per-light front ambient
** terms, clamped and scaled; on an emissive-only change the loop adds no ambient terms, so baseColor is the bare
** sceneColor (ChangeMaterialEmission does add them) */
void __fastcall ComputeLightMaterialState(__GLcontext *gc, GLint frontChange, GLint backChange)
{
    __GLmaterialState *front, *back;
    __GLlightSourceMachineICD *lsm;
    __GLlightSourceStateICD *lss;
    __GLfloat r, g, b;
    GLint allChange;
    __GLcolor basecolor;

    allChange = frontChange | backChange;
    if ((allChange & (__GL_MATERIAL_AMBIENT | __GL_MATERIAL_DIFFUSE | 
	    __GL_MATERIAL_SPECULAR | __GL_MATERIAL_EMISSIVE)) == 0) {
	return;
    }

    basecolor = gc->light.front.sceneColor;

    front = &gc->state.light.front;
    back = &gc->state.light.back;
    for (lsm = (__GLlightSourceMachineICD *)gc->light.sources; lsm; lsm = lsm->next) {
	lss = lsm->state;
	/*
	** Pre-multiply and the front & back ambient, diffuse and
	** specular colors
	*/
	if (allChange & __GL_MATERIAL_AMBIENT) {
	    r = lss->ambient.r;
	    g = lss->ambient.g;
	    b = lss->ambient.b;
	    if (frontChange & __GL_MATERIAL_AMBIENT) {
		lsm->front.ambient.r = front->ambient.r * r;
		lsm->front.ambient.g = front->ambient.g * g;
		lsm->front.ambient.b = front->ambient.b * b;

		basecolor.r += lsm->front.ambient.r;
		basecolor.g += lsm->front.ambient.g;
		basecolor.b += lsm->front.ambient.b;
	    }
	    if (backChange & __GL_MATERIAL_AMBIENT) {
		lsm->back.ambient.r = back->ambient.r * r;
		lsm->back.ambient.g = back->ambient.g * g;
		lsm->back.ambient.b = back->ambient.b * b;
	    }
	}

	if (allChange & __GL_MATERIAL_DIFFUSE) {
	    r = lss->diffuse.r;
	    g = lss->diffuse.g;
	    b = lss->diffuse.b;
	    if (frontChange & __GL_MATERIAL_DIFFUSE) {
		lsm->front.diffuse.r = front->diffuse.r * r;
		lsm->front.diffuse.g = front->diffuse.g * g;
		lsm->front.diffuse.b = front->diffuse.b * b;
	    }
	    if (backChange & __GL_MATERIAL_DIFFUSE) {
		lsm->back.diffuse.r = back->diffuse.r * r;
		lsm->back.diffuse.g = back->diffuse.g * g;
		lsm->back.diffuse.b = back->diffuse.b * b;
	    }
	}

	if (allChange & __GL_MATERIAL_SPECULAR) {
	    r = lss->specular.r;
	    g = lss->specular.g;
	    b = lss->specular.b;
	    if (frontChange & __GL_MATERIAL_SPECULAR) {
		lsm->front.specular.r = front->specular.r * r;
		lsm->front.specular.g = front->specular.g * g;
		lsm->front.specular.b = front->specular.b * b;
	    }
	    if (backChange & __GL_MATERIAL_SPECULAR) {
		lsm->back.specular.r = back->specular.r * r;
		lsm->back.specular.g = back->specular.g * g;
		lsm->back.specular.b = back->specular.b * b;
	    }
	}
    }

    if (frontChange & (__GL_MATERIAL_AMBIENT | __GL_MATERIAL_EMISSIVE)) {
	ClampScaledColorf(gc, &gc->light.baseColor, &basecolor.r);
    }
}

/* 69092150 -- ComputeIndexMaterialState (3dfx S_LIGHT.C): the face's material index (state.light.front/back
** +0x50), or the current index for
** the faces glIndexMaterialEXT names while GL_INDEX_MATERIAL_EXT is on */
void __fastcall ComputeIndexMaterialState(__GLcontext *gc, GLuint edx_unused, GLuint unused)
{
    GLenum face;

    if (gc->state.enables.general & __GL_INDEX_MATERIAL_ENABLE) {
	face = gc->state.light.indexMaterialFace;
	if (face == GL_FRONT || face == GL_FRONT_AND_BACK) {
	    gc->light.front.indexOffsetPtr = &gc->state.current.userColorIndex;
	} else {
	    gc->light.front.indexOffsetPtr = &gc->state.light.front.indexOffset;
	}
	if (face == GL_BACK || face == GL_FRONT_AND_BACK) {
	    gc->light.back.indexOffsetPtr = &gc->state.current.userColorIndex;
	} else {
	    gc->light.back.indexOffsetPtr = &gc->state.light.back.indexOffset;
	}
    } else {
	gc->light.front.indexOffsetPtr = &gc->state.light.front.indexOffset;
	gc->light.back.indexOffsetPtr = &gc->state.light.back.indexOffset;
    }
}

/* 69092210 -- s_light.c ComputeLightState (no colorControl test in this SI) */
void __fastcall ComputeLightState(__GLcontext *gc)
{
    __GLlightSourceStateICD *lss;
    __GLlightSourceMachineICD *lsm, **lsmp;
    GLuint enables;
    GLint i;
    __GLspecLUTEntry *lut;

    lss = (__GLlightSourceStateICD *)gc->state.light.source;
    lsm = (__GLlightSourceMachineICD *)gc->light.source;
    lsmp = (__GLlightSourceMachineICD **)&gc->light.sources;
    enables = gc->state.enables.lights;
    for (i = 0; i < gc->constants.numberOfLights;
            i++, lss++, lsm++, enables >>= 1) {
        if (!(enables & 1)) continue;

        /* Link this enabled light on to the list */
        *lsmp = lsm;
        lsm->state = lss;
        lsmp = &lsm->next;

        lsm->position = lss->positionEye;
        lsm->isSpot = lss->spotLightCutOffAngle != 180;
        if (lsm->isSpot) {
            lsm->cosCutOffAngle =
                (__GLfloat)cos(lss->spotLightCutOffAngle * __glDegreesToRadians);
        }

        if (lsm->isSpot && (!lsm->cache ||
                (lsm->spotLightExponent != lss->spotLightExponent))) {
            GLdouble exponent;

            exponent = lsm->spotLightExponent = lss->spotLightExponent;

            if (lsm->cache) {
                __glFreeSpecLUT(gc, lsm->cache);
            }
            lut = lsm->cache = __glCreateSpecLUT(gc, (__GLfloat)exponent);
            lsm->threshold = lut->threshold;
            lsm->scale = lut->scale;
            lsm->spotTable = lut->table;
        }

        lsm->constantAttenuation = lss->constantAttenuation;
        if (lsm->constantAttenuation) {
            lsm->attenuation = __GL_ONE_F / lss->constantAttenuation;
        }
        lsm->linearAttenuation = lss->linearAttenuation;
        lsm->quadraticAttenuation = lss->quadraticAttenuation;

        if (gc->modes.colorIndexMode) {
            lsm->sli = ((__GLfloat) 0.30) * lss->specular.r
                + ((__GLfloat) 0.59) * lss->specular.g
                + ((__GLfloat) 0.11) * lss->specular.b;
            lsm->dli = ((__GLfloat) 0.30) * lss->diffuse.r
                + ((__GLfloat) 0.59) * lss->diffuse.g
                + ((__GLfloat) 0.11) * lss->diffuse.b;
        }
        if (!gc->state.light.model.localViewer && !lsm->isSpot &&
                (lsm->position.w == __GL_ZERO_F)) {
            __GLfloat hv[3];

            /* Compute unit h[i] (normalized) */
            (*(__GLnormalize)gc->procs.normalize)(hv, &lsm->position.x);
            lsm->unitVPpli.x = hv[0];
            lsm->unitVPpli.y = hv[1];
            lsm->unitVPpli.z = hv[2];
            hv[2] += __GL_ONE_F;
            (*(__GLnormalize)gc->procs.normalize)(&lsm->hHat.x, hv);
            lsm->slowPath = GL_FALSE;
        } else {
            lsm->slowPath = GL_TRUE;
        }
    }
    *lsmp = 0;
}

/* 69092470 -- ComputeLightProcs (s_light.c): the fast CI path only without an index material
** offset / shift on either face; mode flags through getModeFlags */
void __fastcall ComputeLightProcs(__GLcontext *gc)
{
    GLboolean anySlow = GL_FALSE;
    __GLlightSourceMachineICD *lsm;

    for (lsm = (__GLlightSourceMachineICD *)gc->light.sources; lsm; lsm = lsm->next) {
	if (lsm->slowPath) {
	    anySlow = GL_TRUE;
	    break;
	}
    }

    if (gc->state.enables.general & __GL_LIGHTING_ENABLE) {
	if (gc->modes.colorIndexMode) {
	    if (!anySlow) {
		if (!(gc->state.enables.general & INDEX_MATERIAL_ENABLE) &&
		    MS_INDEX_OFFSET(&gc->state.light.front) == __GL_ZERO_F &&
		    MS_INDEX_SHIFT(&gc->state.light.front) == __GL_ZERO_F &&
		    MS_INDEX_OFFSET(&gc->state.light.back) == __GL_ZERO_F &&
		    MS_INDEX_SHIFT(&gc->state.light.back) == __GL_ZERO_F) {
		    gc->procs.calcColor = (__GLproc)__glFastCalcCIColor;
		} else {
		    gc->procs.calcColor = (__GLproc)__glFastCalcCIColorOffset;
		}
	    } else {
		gc->procs.calcColor = (__GLproc)__glCalcCIColor;
	    }
	} else {
	    if (!anySlow) {
		gc->procs.calcColor = (__GLproc)__glFastCalcRGBColor;
	    } else {
		gc->procs.calcColor = (__GLproc)__glCalcRGBColor;
	    }
	}
	gc->procs.calcRasterColor = gc->procs.calcColor;
	if (((*gc->polygon.shader.getModeFlags)(gc) & SHADE_CHEAP_FOG) &&
		((*gc->polygon.shader.getModeFlags)(gc) & SHADE_SMOOTH_LIGHT) &&
		gc->renderMode == GL_RENDER) {
	    gc->procs.calcColor2 = gc->procs.calcColor;
	    if (gc->modes.colorIndexMode) {
		gc->procs.calcColor = (__GLproc)FogLitCIColor;
	    } else {
		gc->procs.calcColor = (__GLproc)FogLitRGBColor;
	    }
	}
    } else {
	gc->procs.calcRasterColor = (__GLproc)NopSpanRender;
	if (((*gc->polygon.shader.getModeFlags)(gc) & SHADE_CHEAP_FOG) &&
		((*gc->polygon.shader.getModeFlags)(gc) & SHADE_SMOOTH_LIGHT) &&
		gc->renderMode == GL_RENDER) {
	    if (gc->modes.colorIndexMode) {
		gc->procs.calcColor = (__GLproc)FogCIColor;
	    } else {
		gc->procs.calcColor = (__GLproc)FogRGBColor;
	    }
	} else {
	    gc->procs.calcColor = (__GLproc)NopSpanRender;
	}
    }
}

/* 69092620 -- __glGenericPickColorMaterialProcs */
void __fastcall __glGenericPickColorMaterialProcs(__GLcontext *gc)
{
    if (gc->modes.rgbMode) {
	if (gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE) {
	    switch (gc->state.light.colorMaterialFace) {
	      case GL_FRONT_AND_BACK:
		gc->procs.applyColor = (__GLproc) __glChangeBothMaterialColors;
		gc->light.cm = 0;
		gc->light.cmm = 0;
		break;
	      case GL_FRONT:
		gc->procs.applyColor = (__GLproc) __glChangeOneMaterialColor;
		gc->light.cm = &gc->state.light.front;
		gc->light.cmm = &gc->light.front;
		break;
	      case GL_BACK:
		gc->procs.applyColor = (__GLproc) __glChangeOneMaterialColor;
		gc->light.cm = &gc->state.light.back;
		gc->light.cmm = &gc->light.back;
		break;
	    }
	    switch (gc->state.light.colorMaterialParam) {
	      case GL_EMISSION:
		gc->procs.changeMaterial = (__GLproc) ChangeMaterialEmission;
		break;
	      case GL_SPECULAR:
		gc->procs.changeMaterial = (__GLproc) ChangeMaterialSpecular;
		break;
	      case GL_AMBIENT:
		gc->procs.changeMaterial = (__GLproc) ChangeMaterialAmbient;
		break;
	      case GL_DIFFUSE:
		gc->procs.changeMaterial = (__GLproc) ChangeMaterialDiffuse;
		break;
	      case GL_AMBIENT_AND_DIFFUSE:
		gc->procs.changeMaterial = (__GLproc) ChangeMaterialAmbientAndDiffuse;
		break;
	    }
	} else {
	    if (gc->state.enables.general & __GL_LIGHTING_ENABLE) {
		gc->procs.applyColor = (__GLproc) __glNop1;
	    } else {
		gc->procs.applyColor = (__GLproc) __glClampAndScaleColor;
	    }
	}
    } else {
	gc->procs.applyColor = (__GLproc) __glNop1;
    }
}

/* 69092720 -- ChangeMaterialEmission: colour-material changeMaterial for GL_EMISSION: the material's emission =
** the current user colour times the colour scales, sceneColor recomputed; for the front face light.baseColor =
** sceneColor + every light's cached ambient term, clamped and scaled */
void __fastcall ChangeMaterialEmission(__GLcontext *gc, __GLmaterialState *ms,
                             __GLmaterialMachineICD *msm)
{
    __GLlightSourcePerMaterialMachineICD *lspmm;
    __GLlightSourceMachineICD *lsm;
    GLboolean isBack;
    __GLfloat r, g, b;
    __GLcolor basecolor;

    r = gc->state.current.userColor.r * gc->constants.redScale;
    g = gc->state.current.userColor.g * gc->constants.greenScale;
    b = gc->state.current.userColor.b * gc->constants.blueScale;

    ms->emissive.r = r;
    ms->emissive.g = g;
    ms->emissive.b = b;
    ms->emissive.a = gc->state.current.userColor.a * gc->constants.alphaScale;

    msm->sceneColor.r = r + ms->ambient.r * gc->state.light.model.ambient.r;
    msm->sceneColor.g = g + ms->ambient.g * gc->state.light.model.ambient.g;
    msm->sceneColor.b = b + ms->ambient.b * gc->state.light.model.ambient.b;

    isBack = msm == &gc->light.back;
    basecolor = msm->sceneColor;
    for (lsm = (__GLlightSourceMachineICD *)gc->light.sources; lsm; lsm = lsm->next) {
        lspmm = &lsm->front + isBack;

        basecolor.r += lspmm->ambient.r;
        basecolor.g += lspmm->ambient.g;
        basecolor.b += lspmm->ambient.b;
    }
    if (!isBack) {
        ClampScaledColorf(gc, &gc->light.baseColor, &basecolor.r);
    }
}

/* 69092830 -- ChangeMaterialSpecular (s_light.c): colour-material changeMaterial for GL_SPECULAR: the material's
** specular = the current user colour (unscaled), and each light's cached specular product for this face */
void __fastcall ChangeMaterialSpecular(__GLcontext *gc, __GLmaterialState *ms, __GLmaterialMachineICD *msm)
{
    __GLlightSourcePerMaterialMachineICD *lspmm;
    __GLlightSourceMachineICD *lsm;
    __GLlightSourceStateICD *lss;
    GLboolean isBack;
    __GLfloat r, g, b;

    r = gc->state.current.userColor.r;
    g = gc->state.current.userColor.g;
    b = gc->state.current.userColor.b;

    ms->specular.r = r;
    ms->specular.g = g;
    ms->specular.b = b;
    ms->specular.a = gc->state.current.userColor.a;

    /*
    ** Update per-light-source state that depends on material specular
    ** state
    */
    isBack = msm == &gc->light.back;
    for (lsm = (__GLlightSourceMachineICD *)gc->light.sources; lsm; lsm = lsm->next) {
	lspmm = &lsm->front + isBack;
	lss = lsm->state;

	/* Recompute per-light per-material cached specular */
	lspmm->specular.r = r * lss->specular.r;
	lspmm->specular.g = g * lss->specular.g;
	lspmm->specular.b = b * lss->specular.b;
    }
}

/* 690928d0 -- ChangeMaterialAmbient (s_light.c): colour-material changeMaterial for GL_AMBIENT: the material's
** ambient = the current user colour, sceneColor and each light's cached ambient product recomputed, and for the
** front face light.baseColor = sceneColor + those ambient terms, clamped and scaled */
void __fastcall ChangeMaterialAmbient(__GLcontext *gc, __GLmaterialState *ms, __GLmaterialMachineICD *msm)
{
    __GLlightSourcePerMaterialMachineICD *lspmm;
    __GLlightSourceMachineICD *lsm;
    __GLlightSourceStateICD *lss;
    GLboolean isBack;
    __GLfloat r, g, b;
    __GLcolor basecolor;

    r = gc->state.current.userColor.r;
    g = gc->state.current.userColor.g;
    b = gc->state.current.userColor.b;

    ms->ambient.r = r;
    ms->ambient.g = g;
    ms->ambient.b = b;
    ms->ambient.a = gc->state.current.userColor.a;

    msm->sceneColor.r = ms->emissive.r + r * gc->state.light.model.ambient.r;
    msm->sceneColor.g = ms->emissive.g + g * gc->state.light.model.ambient.g;
    msm->sceneColor.b = ms->emissive.b + b * gc->state.light.model.ambient.b;

    /*
    ** Update per-light-source state that depends on material ambient
    ** state.
    */
    isBack = msm == &gc->light.back;
    basecolor = msm->sceneColor;
    for (lsm = (__GLlightSourceMachineICD *)gc->light.sources; lsm; lsm = lsm->next) {
	lspmm = &lsm->front + isBack;
	lss = lsm->state;

	/* Recompute per-light per-material cached ambient */
	lspmm->ambient.r = r * lss->ambient.r;
	lspmm->ambient.g = g * lss->ambient.g;
	lspmm->ambient.b = b * lss->ambient.b;

	basecolor.r += lspmm->ambient.r;
	basecolor.g += lspmm->ambient.g;
	basecolor.b += lspmm->ambient.b;
    }
    if (!isBack) {
	ClampScaledColorf(gc, &gc->light.baseColor, &basecolor.r);
    }
}

/* 69092a00 -- ChangeMaterialDiffuse: colour-material changeMaterial for GL_DIFFUSE: the material's diffuse = the
** current user colour, msm->alpha = its alpha clamped to [0, 1] times alphaScale, and each light's cached diffuse
** product for this face */
void __fastcall ChangeMaterialDiffuse(__GLcontext *gc, __GLmaterialState *ms,
                             __GLmaterialMachineICD *msm)
{
    __GLlightSourcePerMaterialMachineICD *lspmm;
    __GLlightSourceMachineICD *lsm;
    __GLlightSourceStateICD *lss;
    GLboolean isBack;
    __GLfloat r, g, b, a;

    r = gc->state.current.userColor.r;
    g = gc->state.current.userColor.g;
    b = gc->state.current.userColor.b;
    a = gc->state.current.userColor.a;

    ms->diffuse.r = r;
    ms->diffuse.g = g;
    ms->diffuse.b = b;
    ms->diffuse.a = a;

    if (a < __GL_ZERO_F) {
        a = __GL_ZERO_F;
    } else if (a > __GL_ONE_F) {
        a = __GL_ONE_F;
    }
    msm->alpha = a * gc->constants.alphaScale;

    isBack = msm == &gc->light.back;
    for (lsm = (__GLlightSourceMachineICD *)gc->light.sources; lsm; lsm = lsm->next) {
        lspmm = &lsm->front + isBack;
        lss = lsm->state;

        lspmm->diffuse.r = r * lss->diffuse.r;
        lspmm->diffuse.g = g * lss->diffuse.g;
        lspmm->diffuse.b = b * lss->diffuse.b;
    }
}

/* 69092ae0 -- ChangeMaterialAmbientAndDiffuse (s_light.c): colour-material changeMaterial for
** GL_AMBIENT_AND_DIFFUSE: ChangeMaterialAmbient and ChangeMaterialDiffuse in one pass (material ambient and
** diffuse = the user colour, sceneColor, alpha, per-light ambient / diffuse products, front light.baseColor) */
void __fastcall ChangeMaterialAmbientAndDiffuse(__GLcontext *gc, __GLmaterialState *ms, __GLmaterialMachineICD *msm)
{
    __GLlightSourcePerMaterialMachineICD *lspmm;
    __GLlightSourceMachineICD *lsm;
    __GLlightSourceStateICD *lss;
    GLboolean isBack;
    __GLfloat r, g, b, a;
    __GLcolor basecolor;

    r = gc->state.current.userColor.r;
    g = gc->state.current.userColor.g;
    b = gc->state.current.userColor.b;
    a = gc->state.current.userColor.a;

    ms->ambient.r = r;
    ms->ambient.g = g;
    ms->ambient.b = b;
    ms->ambient.a = a;

    ms->diffuse.r = r;
    ms->diffuse.g = g;
    ms->diffuse.b = b;
    ms->diffuse.a = a;

    msm->sceneColor.r = ms->emissive.r + r * gc->state.light.model.ambient.r;
    msm->sceneColor.g = ms->emissive.g + g * gc->state.light.model.ambient.g;
    msm->sceneColor.b = ms->emissive.b + b * gc->state.light.model.ambient.b;

    if (a < __GL_ZERO_F) {
	a = __GL_ZERO_F;
    } else if (a > __GL_ONE_F) {
	a = __GL_ONE_F;
    }
    msm->alpha = a * gc->constants.alphaScale;

    /*
    ** Update per-light-source state that depends on per-material state.
    */
    isBack = msm == &gc->light.back;
    basecolor = msm->sceneColor;
    for (lsm = (__GLlightSourceMachineICD *)gc->light.sources; lsm; lsm = lsm->next) {
	lspmm = &lsm->front + isBack;
	lss = lsm->state;

	/* Recompute per-light per-material cached ambient */
	lspmm->ambient.r = r * lss->ambient.r;
	lspmm->ambient.g = g * lss->ambient.g;
	lspmm->ambient.b = b * lss->ambient.b;

	/* Recompute per-light per-material cached diffuse */
	lspmm->diffuse.r = r * lss->diffuse.r;
	lspmm->diffuse.g = g * lss->diffuse.g;
	lspmm->diffuse.b = b * lss->diffuse.b;

	basecolor.r += lspmm->ambient.r;
	basecolor.g += lspmm->ambient.g;
	basecolor.b += lspmm->ambient.b;
    }
    if (!isBack) {
	ClampScaledColorf(gc, &gc->light.baseColor, &basecolor.r);
    }
}

/* ---- s_lcache.c: the specular lookup-table cache (glsi.h __GLspecLUTCache) ---- */

/* 690dd3a0 -- __glFreeLUTCache */
void __fastcall __glFreeLUTCache(__GLcontext *gc)
{
    int i;
    GLint nentries;
    __GLspecLUTEntryPtr *entry;
    __GLspecLUTCache *lutCache;

    lutCache = gc->light.lutCache;
    nentries = lutCache->nentries;
    for (i = 0; i < nentries; i++) {
	entry = &(lutCache->entries[i]);
	entry->table->refcount--; /* cache no longer refers to table */
	if (entry->table->refcount == 0) { /* other gc's (through dlists) may */
	    __GL_FREE(gc, entry->table);     /* reference this table */
	}
    }
    __GL_FREE(gc, lutCache);
}

/* 690dd3e0 -- __glCreateSpecLUT (s_lcache.c) */
__GLspecLUTEntry *__fastcall __glCreateSpecLUT(__GLcontext *gc, __GLfloat exp)
{
    GLint nentries, allocatedSize;
    GLint location;
    __GLspecLUTCache *lutCache;
    __GLspecLUTEntryPtr *entry;
    __GLspecLUTEntry *table;
    __GLfloat *tableEntry;
    GLdouble threshold, scale, x, dx;
    GLint i;

    lutCache = gc->light.lutCache;

    table = findEntry(lutCache, exp, &location);
    if (table) {
        table->refcount++;
        return table;
    }

    nentries = lutCache->nentries;
    if (nentries >= TOO_MANY_LUT_ENTRIES) {
        for(i = 0; i < nentries; i++) {
            table = (lutCache->entries[i]).table;
            if (table->refcount == 1) {
                table->refcount--;
                lutCache->nentries = nentries = lutCache->nentries - 1;
                memmove(&(lutCache->entries[i]),
                         &(lutCache->entries[i+1]),
                         (nentries-i) * sizeof(__GLspecLUTEntryPtr));
                __GL_FREE(gc, table);
                break;
            }
        }
        table = findEntry(lutCache, exp, &location);
    }

    lutCache->nentries = nentries = 1 + lutCache->nentries;
    allocatedSize = lutCache->allocatedSize;

    if (nentries > allocatedSize) {
        lutCache->allocatedSize = allocatedSize = allocatedSize + 6;
        gc->light.lutCache = lutCache = (__GLspecLUTCache *)
                __GL_REALLOC(gc, lutCache, sizeof(__GLspecLUTCache) +
                allocatedSize * sizeof(__GLspecLUTEntryPtr));
    }

    if (nentries-location-1) {
        memmove(&(lutCache->entries[location+1]),
                &(lutCache->entries[location]),
                (nentries-location-1) * sizeof(__GLspecLUTEntryPtr));
    }
    entry = &(lutCache->entries[location]);
    entry->exp = exp;
    table = entry->table = (__GLspecLUTEntry *)
            __GL_MALLOC(gc, sizeof(__GLspecLUTEntry));

    if (exp == 0.0) {
        threshold = 0.0;
    } else {
        threshold = pow(0.0007, 1.0 / exp);
    }

    scale = (__GL_SPEC_LOOKUP_TABLE_SIZE - 1) / (1.0 - threshold);
    dx = 1.0 / scale;
    x = threshold;
    tableEntry = table->table;
    for (i = __GL_SPEC_LOOKUP_TABLE_SIZE; --i >= 0; ) {
        *tableEntry++ = pow(x, exp);
        x += dx;
    }
    table->threshold = threshold;
    table->scale = scale;
    table->refcount = 2;
    table->exp = exp;
    return table;
}

/* 690dd590 -- findEntry (s_lcache.c static): EDX = location, the exponent on the stack */
__GLspecLUTEntry *__fastcall findEntry(__GLspecLUTCache *lutCache, __GLfloat exp, GLint *location)
{
    GLint nentries;
    GLint bottom, half, top;
    __GLspecLUTEntry *table;
    __GLspecLUTEntryPtr *entry;

    nentries = lutCache->nentries;
    bottom = 0;
    top = nentries;
    while (top > bottom) {
        half = (bottom+top)/2;
        entry = &(lutCache->entries[half]);
        if (entry->exp == exp) {
            table = entry->table;
            *location = half;
            return table;
        }
        if (exp < entry->exp) {
            top = half;
        } else {
            bottom = half+1;
        }
    }
    *location = bottom;
    return NULL;
}

/*
** 690e2ce0 -- __glCalcRGBColor (s_light.c; gc->procs.calcColor): the general RGB lighting of one
** vertex, an older revision of the SI routine with its inner loop in inline assembly.  The
** function is unoptimised in the original (the C between the __asm blocks is /Od code).
** Light source (lsm) words: +0x60 per-material machine (ambient +0, diffuse +0x10, specular +0x20),
** +0x64/+0x68/+0x6c attenuation (constant, linear, quadratic), +0x74 position (x, y, z; +0x80 is position.w, tested
** as a raw word: non-zero skips the attenuation, the opposite of SGI s_light.c and of __glCalcRGBColorSI, which
** attenuate when w != 0; kept as the original),
** +0x84 spot direction, +0x94 cos cutoff, +0x98 constant 1/attenuation, +0x9c isSpot,
** +0xe8 next, +0xec spot table, +0xf0 / +0xf4 its threshold and scale.
** Kept as the original: the attenuation test ORs only the linear and quadratic words (the constant
** one is loaded into EAX and dropped), and "esi = gc" in the final clamp is never used.
*/
#pragma optimize("", off)
void __fastcall __glCalcRGBColor(__GLcontext *gc, GLint face, __GLvertexICD *vx)
{
    /* /Od: VC6 gives the frame slots by the names' hash buckets, so the suffixed names and this (reversed) order
    ** reproduce the original's layout (the comments give the original offsets) */
    __GLfloat blueScale1;           /* -0xd0 */
    __GLfloat greenScale0;          /* -0xcc */
    __GLfloat redScale0;            /* -0xc8 */
    GLint ix22;                     /* -0xc4 */
    __GLfloat nDotHV9;              /* -0xc0 */
    __GLfloat nDotVPn;              /* -0xbc */
    __GLfloat sum0[4];              /* -0xb8 */
    __GLfloat negXx;                /* -0xa8 */
    __GLfloat dot5;                 /* -0xa4 */
    GLint ix5;                      /* -0xa0 */
    __GLfloat negZ;                 /* -0x9c */
    __GLfloat negY;                 /* -0x98 */
    __GLfloat vPen[4];              /* -0x94 */
    __GLfloat vPliHat[4];           /* -0x84 */
    __GLfloat hv[4];                /* -0x74 */
    __GLfloat *lspmm;               /* -0x64 */
    __GLfloat attenuationv;         /* -0x60 */
    __GLfloat attenl;               /* -0x5c */
    __GLfloat vPlit[4];             /* -0x58 */
    __GLfloat hHat1[4];             /* -0x48 */
    __GLmaterialState *mst;         /* -0x38 */
    GLboolean localViewer;          /* -0x34 */
    __GLfloat zero;                 /* -0x30 */
    __GLfloat nz;                   /* -0x2c */
    __GLfloat one;                  /* -0x28 */
    __GLfloat g;                    /* -0x24 */
    __GLfloat ny;                   /* -0x20 */
    __GLfloat nx;                   /* -0x1c */
    __GLlightSourceMachineICD *lsm; /* -0x18 */
    __GLmaterialMachineICD *msm;    /* -0x14 */
    __GLcolor *c;                   /* -0x10 */
    __GLfloat r;                    /* -0x0c */
    __GLfloat b;                    /* -0x08 */
    GLboolean eyeWIsZero;           /* -0x04 (computed, unused) */

    zero = 0.0f;
    one = 1.0f;
    if (face == __GL_FRONTFACE) {
        c = &vx->colors[__GL_FRONTFACE];
        msm = &gc->light.front;
        mst = &gc->state.light.front;
        nx = vx->normal.x;
        ny = vx->normal.y;
        nz = vx->normal.z;
    } else {
        c = &vx->colors[__GL_BACKFACE];
        msm = &gc->light.back;
        mst = &gc->state.light.back;
        nx = -vx->normal.x;
        ny = -vx->normal.y;
        nz = -vx->normal.z;
    }
    zero = 0.0f;
    one = 1.0f;
    r = msm->sceneColor.r;
    g = msm->sceneColor.g;
    b = msm->sceneColor.b;
    eyeWIsZero = vx->eye.w == zero;
    localViewer = gc->state.light.model.localViewer;
    lsm = gc->light.sources;
    while (lsm) {
        lspmm = (__GLfloat *)lsm->state;
        __glI386Vvec(vPlit, &vx->eye.x, &lsm->position.x);
        __glI386NormalizeCoord(vPliHat, vPlit);
        if (localViewer) {
            __asm {
                mov     esi, vx
                mov     eax, [esi+0x50]
                mov     ebx, [esi+0x54]
                mov     ecx, [esi+0x58]
                xor     eax, 0x80000000
                xor     ebx, 0x80000000
                xor     ecx, 0x80000000
                mov     vPen, eax
                mov     vPen[4], ebx
                mov     vPen[8], ecx
            }
            __glI386NormalizeCoord(vPen, vPen);
            __asm {
                fld     vPliHat
                fadd    vPen
                fld     vPliHat[4]
                fadd    vPen[4]
                fld     vPliHat[8]
                fadd    vPen[8]
                fxch    st(2)
                fstp    hv
                fstp    hv[4]
                fstp    hv[8]
            }
        } else {
            __asm {
                fld     vPliHat[8]
                fadd    one
                mov     eax, vPliHat
                mov     ebx, vPliHat[4]
                mov     hv, eax
                mov     hv[4], ebx
                fstp    hv[8]
            }
        }
        __glI386NormalizeCoord(hHat1, hv);
        __asm {
            mov     edi, lsm
            cmp     dword ptr [edi+0x80], 0
            jne     noAttenuation
            mov     eax, [edi+0x64]
            mov     ebx, [edi+0x68]
            mov     ecx, [edi+0x6c]
            xor     edx, edx
            or      edx, ebx
            or      edx, ecx
            jne     computeAttenuation
            mov     edx, [edi+0x98]
            mov     attenuationv, edx
            jmp     attenuationDone
        computeAttenuation:
            fld     vPlit
            fmul    vPlit
            fld     vPlit[4]
            fmul    vPlit[4]
            fld     vPlit[8]
            fmul    vPlit[8]
            fxch    st(2)
            faddp   st(1), st
            faddp   st(1), st
            fsqrt
            fld     one
            fld     dword ptr [edi+0x6c]
            fmul    st, st(2)
            fadd    dword ptr [edi+0x68]
            fxch    st(2)
            fmulp   st(2), st
            fadd    dword ptr [edi+0x64]
            fxch    st(1)
            fdivp   st(1), st
            fstp    attenuationv
        attenuationDone:
            jmp     haveAttenuation
        noAttenuation:
            mov     attenuationv, 0x3f800000
        haveAttenuation:
            mov     eax, attenuationv
            mov     attenl, eax
            mov     edi, lsm
            cmp     byte ptr [edi+0x9c], 0
            je      notSpot
            mov     eax, vPliHat
            mov     ebx, vPliHat[4]
            mov     ecx, vPliHat[8]
            xor     eax, 0x80000000
            xor     ebx, 0x80000000
            xor     ecx, 0x80000000
            mov     negXx, eax
            mov     negY, ebx
            mov     negZ, ecx
            fld     negXx
            fmul    dword ptr [edi+0x84]
            fld     negY
            fmul    dword ptr [edi+0x88]
            fld     negZ
            fmul    dword ptr [edi+0x8c]
            fxch    st(2)
            faddp   st(1), st
            faddp   st(1), st
            fst     dot5
            mov     eax, dot5
            and     eax, eax
            js      outsideCone
            je      outsideCone
            cmp     eax, [edi+0xf0]
            jl      outsideCone
            cmp     eax, [edi+0x94]
            jl      outsideCone
            fsub    dword ptr [edi+0xf0]
            fmul    dword ptr [edi+0xf4]
            fistp   ix5
            mov     eax, ix5
            cmp     eax, 0x100
            jge     notSpot
            mov     esi, [edi+0xec]
            fld     attenuationv
            fmul    dword ptr [esi+eax*4]
            fstp    attenl
            jmp     notSpot
        outsideCone:
            fstp    st(0)
            jmp     nextLight
        notSpot:
            cmp     attenl, 0
            je      nextLight
            mov     esi, lspmm
            mov     edi, mst
            mov     edx, msm
            fld     dword ptr [esi]
            fmul    dword ptr [edi]
            fld     dword ptr [esi+4]
            fmul    dword ptr [edi+4]
            fld     dword ptr [esi+8]
            fmul    dword ptr [edi+8]
            fxch    st(2)
            fstp    sum0
            fstp    sum0[4]
            fstp    sum0[8]
            fld     nx
            fmul    vPliHat
            fld     ny
            fmul    vPliHat[4]
            fld     nz
            fmul    vPliHat[8]
            fxch    st(2)
            faddp   st(1), st
            faddp   st(1), st
            fstp    nDotVPn
            mov     eax, nDotVPn
            and     eax, eax
            je      addSum
            js      addSum
            fld     nx
            fmul    hHat1
            fld     ny
            fmul    hHat1[4]
            fld     nz
            fmul    hHat1[8]
            fxch    st(2)
            faddp   st(1), st
            faddp   st(1), st
            fsub    dword ptr [edx+0x48]
            fstp    nDotHV9
            mov     eax, nDotHV9
            and     eax, eax
            js      addDiffuse
            fld     nDotHV9
            fmul    dword ptr [edx+0x4c]
            fistp   ix22
            mov     eax, ix22
            cmp     eax, 0x100
            jge     fullSpecular
            mov     ebx, [edx+0x44]
            mov     ebx, [ebx+eax*4]
            mov     nDotHV9, ebx
            fld     nDotHV9
            fmul    dword ptr [esi+0x20]
            fld     nDotHV9
            fmul    dword ptr [esi+0x24]
            fld     nDotHV9
            fmul    dword ptr [esi+0x28]
            fxch    st(2)
            fmul    dword ptr [edi+0x20]
            fxch    st(1)
            fmul    dword ptr [edi+0x24]
            fxch    st(2)
            fmul    dword ptr [edi+0x28]
            fxch    st(1)
            fadd    sum0
            fxch    st(2)
            fadd    sum0[4]
            fxch    st(1)
            fadd    sum0[8]
            fxch    st(2)
            fstp    sum0
            fstp    sum0[4]
            fstp    sum0[8]
            jmp     addDiffuse
        fullSpecular:
            fld     dword ptr [edi+0x20]
            fmul    dword ptr [esi+0x20]
            fld     dword ptr [edi+0x24]
            fmul    dword ptr [esi+0x24]
            fld     dword ptr [edi+0x28]
            fmul    dword ptr [esi+0x28]
            fxch    st(2)
            fadd    sum0
            fxch    st(1)
            fadd    sum0[4]
            fxch    st(2)
            fadd    sum0[8]
            fxch    st(1)
            fstp    sum0
            fstp    sum0[8]
            fstp    sum0[4]
        addDiffuse:
            fld     nDotVPn
            fmul    dword ptr [esi+0x10]
            fld     nDotVPn
            fmul    dword ptr [esi+0x14]
            fld     nDotVPn
            fmul    dword ptr [esi+0x18]
            fxch    st(2)
            fmul    dword ptr [edi+0x10]
            fxch    st(1)
            fmul    dword ptr [edi+0x14]
            fxch    st(2)
            fmul    dword ptr [edi+0x18]
            fxch    st(1)
            fadd    sum0
            fxch    st(2)
            fadd    sum0[4]
            fxch    st(1)
            fadd    sum0[8]
            fxch    st(2)
            fstp    sum0
            fstp    sum0[4]
            fstp    sum0[8]
        addSum:
            fld     sum0
            fmul    attenl
            fld     sum0[4]
            fmul    attenl
            fld     sum0[8]
            fmul    attenl
            fxch    st(2)
            fadd    r
            fxch    st(1)
            fadd    g
            fxch    st(2)
            fadd    b
            fxch    st(1)
            fstp    r
            fstp    b
            fstp    g
        nextLight:
            nop
        }
        lsm = (__GLlightSourceMachineICD *)lsm->next;
    }
    redScale0 = gc->constants.redScale;
    greenScale0 = gc->constants.greenScale;
    blueScale1 = gc->constants.blueScale;
    __asm {
        mov     esi, gc
        mov     edi, c
        mov     eax, r
        mov     ebx, g
        mov     ecx, b
        and     eax, eax
        jle     redZero
        cmp     eax, redScale0
        jb      redDone
        mov     eax, redScale0
        jmp     redDone
    redZero:
        mov     eax, 0
    redDone:
        and     ebx, ebx
        jle     greenZero
        cmp     ebx, greenScale0
        jb      greenDone
        mov     ebx, greenScale0
        jmp     greenDone
    greenZero:
        mov     ebx, 0
    greenDone:
        and     ecx, ecx
        jle     blueZero
        cmp     ecx, blueScale1
        jb      blueDone
        mov     ecx, blueScale1
        jmp     blueDone
    blueZero:
        mov     ecx, 0
    blueDone:
        mov     [edi], eax
        mov     [edi+4], ebx
        mov     [edi+8], ecx
        mov     edx, msm
        mov     edx, [edx+0x54]
        mov     [edi+0xc], edx
    }
}
#pragma optimize("", on)

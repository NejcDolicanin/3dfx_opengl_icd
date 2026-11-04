/*
** gl_point.c -- s_ptfin.c / s_ptdraw.c / s_ptaa.c: point begin / vertex procs and point renderers.
*/
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "icdprotos.h"
#include <string.h>
#include "icdglob.h"

/* ---- s_ptfin.c: point vertex with the render buffers locked ------------------------------------ */
#define __GL_LOCK_RENDER_BUFFERS(gc) \
    if ((++(gc)->buffers.lockCnt == 1) && (gc)->buffers.lockRenderBuffers) \
	((void (__fastcall *)(__GLcontext *))(gc)->buffers.lockRenderBuffers)(gc)
#define __GL_UNLOCK_RENDER_BUFFERS(gc) \
    if ((--(gc)->buffers.lockCnt == 0) && (gc)->buffers.unlockRenderBuffers) \
	((void (__fastcall *)(__GLcontext *))(gc)->buffers.unlockRenderBuffers)(gc)
typedef void (__fastcall *__GLrenderPoint)(__GLcontext *gc, __GLvertexICD *v);
/*
** s_ptaa.c, ICD form.  __GL_ADJUST_POINT_VERTEX biases the window position (see
** __GL_ADJUST_TRIANGLE_VERTS in glshade.h), rescales z, the texture s, t and the colour;
** __GL_RESTORE_POINT_VERTEX puts them back.
*/
/* the saved vertex values in one struct, as SGI's __GLadjustPointData: the members' order is the original's
** frame order (three separate locals are laid out by use weight instead) */
#define __GL_POINT_VERT_SAVES \
    struct { __GLcoord window; __GLcolor color; __GLcoord texture; } adjustData
#define __GL_ADJUST_POINT_VERTEX(gc, v) \
    gc->constants.viewportXAdjust += __GL_VERTEX_BIAS; \
    gc->constants.viewportYAdjust += __GL_VERTEX_BIAS; \
    gc->transform.clipX0 += __GL_VERTEX_BIAS; \
    gc->transform.clipX1 += __GL_VERTEX_BIAS; \
    gc->transform.clipY0 += __GL_VERTEX_BIAS; \
    gc->transform.clipY1 += __GL_VERTEX_BIAS; \
    adjustData.window.x = v->window.x; adjustData.window.y = v->window.y; adjustData.window.z = v->window.z; \
    v->window.x += __GL_VERTEX_BIAS_F; \
    v->window.y += __GL_VERTEX_BIAS_F; \
    v->window.z *= gc->constants.depthRescale; \
    adjustData.texture = v->texture[0]; \
    v->texture[0].x *= gc->constants.texSScale; \
    v->texture[0].y *= gc->constants.texTScale; \
    adjustData.color = *v->color; \
    v->color->r *= gc->constants.redRescale; \
    v->color->g *= gc->constants.greenRescale; \
    v->color->b *= gc->constants.blueRescale; \
    v->color->a *= gc->constants.alphaRescale
#define __GL_RESTORE_POINT_VERTEX(gc, v) \
    gc->constants.viewportXAdjust -= __GL_VERTEX_BIAS; \
    gc->constants.viewportYAdjust -= __GL_VERTEX_BIAS; \
    gc->transform.clipX0 -= __GL_VERTEX_BIAS; \
    gc->transform.clipX1 -= __GL_VERTEX_BIAS; \
    gc->transform.clipY0 -= __GL_VERTEX_BIAS; \
    gc->transform.clipY1 -= __GL_VERTEX_BIAS; \
    v->window.x = adjustData.window.x; \
    v->window.y = adjustData.window.y; \
    v->window.z = adjustData.window.z; \
    v->texture[0].x = adjustData.texture.x; \
    v->texture[0].y = adjustData.texture.y; \
    v->color->r = adjustData.color.r; \
    v->color->g = adjustData.color.g; \
    v->color->b = adjustData.color.b; \
    v->color->a = adjustData.color.a
typedef void (__fastcall *__GLstoreProc)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
/* s_ptdraw.c (SI 1.1 form: the fragment is built here, the texture is applied with s/q, t/q) */
typedef void (__fastcall *__GLtextureProc)(__GLcontext *gc, __GLcolor *color, __GLfloat s, __GLfloat t, __GLfloat rho);
typedef __GLvertexICD __GLvertex;
/* ---- s_ptaa.c ------------------------------------------------------------------------------ */
typedef void (__fastcall *PTfn_store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
typedef void (__fastcall *PTfn_texture)(__GLcontext *gc, __GLcolor *color, __GLfloat s, __GLfloat t, __GLfloat rho);
typedef void (__fastcall *PTfn_fogPoint)(__GLcontext *gc, __GLfragmentICD *frag, __GLfloat eyeZ);
#define Coverage(x, y, r) ((__GLfloat (__fastcall *)(__GLfloat, __GLfloat, __GLfloat))ptaa_Coverage)(x, y, r)
typedef void (__fastcall *__GLfogPointProc)(__GLcontext *gc, __GLfragmentICD *frag, __GLfloat eyeZ);
#define __GL_FOG_POINT(gc, frag, vx) \
    if ((gc)->state.fog.coordSource == GL_FRAGMENT_DEPTH_EXT) { \
	(*(__GLfogPointProc)(gc)->procs.fogPoint)(gc, frag, (vx)->eye.z); \
    } else { \
	(*(__GLfogPointProc)(gc)->procs.fogPoint)(gc, frag, (vx)->fogCoord); \
    }
#define __GL_SAMPLES		4
#define __GL_SAMPLE_HIT		((__GLfloat) 0.0625)	/* 1 / (4*4) */
#define __GL_SAMPLE_DELTA	((__GLfloat) 0.25)	/* 1 / 4 */
#define __GL_COORD_ADJUST	((__GLfloat) 0.375)
static const __GLfloat __glZeroW16 = 0.0f;
extern double __fastcall ptaa_Coverage(GLuint ecx_unused, GLuint edx_unused, __GLfloat xStart, __GLfloat yStart, __GLfloat radiusSquared);

/* 69095540 -- __glPointVertexLock */
void __fastcall __glPointVertexLock(__GLcontext *gc, __GLvertexICD *v)
{
    if (!(v->flagBits & __GL_CLIP_ALL_MASK)) {
	(*gc->procs.validateVertex[(v->flagBits >> 14) & 3])(gc, v, (*gc->vertex.getFaceNeeds0)(gc) | 1);
	__GL_LOCK_RENDER_BUFFERS(gc);
	(*(__GLrenderPoint)gc->procs.renderPoint)(gc, v);
	__GL_UNLOCK_RENDER_BUFFERS(gc);
    }
}

/* 690955c0 -- __glPointVertex: validate and render an unclipped point */
void __fastcall __glPointVertex(__GLcontext *gc, __GLvertexICD *v)
{
    if (!(v->flagBits & __GL_CLIP_ALL_MASK)) {
	(*gc->procs.validateVertex[(v->flagBits >> 14) & 3])(gc, v, (*gc->vertex.getFaceNeeds0)(gc) | 1);
	(*(__GLrenderPoint)gc->procs.renderPoint)(gc, v);
    }
}

/* 69095600 -- __glPointFastLock (s_ptfin.c __glPointFast): point vertex, no validation, with the render
** buffers locked */
void __fastcall __glPointFastLock(__GLcontext *gc, __GLvertexICD *v)
{
    if (!(v->flagBits & __GL_CLIP_ALL_MASK)) {
	__GL_LOCK_RENDER_BUFFERS(gc);
	(*(__GLrenderPoint)gc->procs.renderPoint)(gc, v);
	__GL_UNLOCK_RENDER_BUFFERS(gc);
    }
}

/* 690d1230 -- __glRenderAliasedPoint1_NoTex (s_ptdraw.c): __glRenderAliasedPoint1 without texture */
void __fastcall __glRenderAliasedPoint1_NoTex(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfragmentICD frag;
    __GL_POINT_VERT_SAVES;

    __GL_ADJUST_POINT_VERTEX(gc, vx);

    frag.x = (GLint) (vx->window.x - __glHalf);
    frag.y = (GLint) (vx->window.y - __glHalf);
    frag.z = (__GLzValue)vx->window.z;
    frag.color = *vx->color;

    /* Render the single point */
    (*(__GLstoreProc)gc->procs.store)(gc->drawBuffer, &frag);

    __GL_RESTORE_POINT_VERTEX(gc, vx);
}

/* 690d1460 -- __glRenderAliasedPoint1 */
void __fastcall __glRenderAliasedPoint1(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfragmentICD frag;
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    __GL_POINT_VERT_SAVES;

    __GL_ADJUST_POINT_VERTEX(gc, vx);

    frag.x = (GLint) (vx->window.x - __glHalf);
    frag.y = (GLint) (vx->window.y - __glHalf);
    frag.z = (__GLzValue)vx->window.z;
    frag.color = *vx->color;
    if (modeFlags & __GL_SHADE_TEXTURE) {
	__GLfloat qInv = __GL_ONE_F / vx->texture[0].w;
	(*(__GLtextureProc)gc->procs.texture[0])(gc, &frag.color, vx->texture[0].x * qInv,
			   vx->texture[0].y * qInv, __GL_ONE_F);
    }

    /* Render the single point */
    (*(__GLstoreProc)gc->procs.store)(gc->drawBuffer, &frag);

    __GL_RESTORE_POINT_VERTEX(gc, vx);
}

/* 690d1710 -- __glRenderAliasedPointN */
void __fastcall __glRenderAliasedPointN(__GLcontext *gc, __GLvertexICD *vx)
{
    GLint pointSize, pointSizeHalf, ix, iy, xLeft, xRight, yBottom, yTop;
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    __GLstoreProc store = (__GLstoreProc)gc->procs.store;
    __GLfragmentICD frag;
    __GL_POINT_VERT_SAVES;

    __GL_ADJUST_POINT_VERTEX(gc, vx);

    if (gc->procs.store == gc->procs.cfbStore) {
	/* We need ownership testing */
	store = __glDoStore;
    }

    pointSize = gc->state.point.aliasedSize;
    pointSizeHalf = pointSize >> 1;
    if (pointSize & 1) {
	/* odd point size */
	xLeft = (((GLint)(vx->window.x - __glHalf)) - pointSizeHalf);
	yBottom = (((GLint)(vx->window.y - __glHalf)) - pointSizeHalf);
    } else {
	/* even point size */
	xLeft = (((GLint)(vx->window.x)) - pointSizeHalf);
	yBottom = (((GLint)(vx->window.y)) - pointSizeHalf);
    }
    xRight = xLeft + pointSize;
    yTop = yBottom + pointSize;

    frag.color = *vx->color;
    if (modeFlags & __GL_SHADE_TEXTURE) {
	__GLfloat qInv = __GL_ONE_F / vx->texture[0].w;
	(*(__GLtextureProc)gc->procs.texture[0])(gc, &frag.color, vx->texture[0].x * qInv,
			   vx->texture[0].y * qInv, __GL_ONE_F);
    }

    /*
    ** Now render the square centered on xCenter,yCenter.
    */
    frag.z = (__GLzValue)vx->window.z;
    for (iy = yBottom; iy < yTop; iy++) {
	for (ix = xLeft; ix < xRight; ix++) {
	    frag.x = ix;
	    frag.y = iy;
	    frag.z = (__GLzValue)vx->window.z;
	    (*store)(gc->drawBuffer, &frag);
	}
    }

    __GL_RESTORE_POINT_VERTEX(gc, vx);
}

/* 690d1a50 -- __glRenderAntiAliasedRGBPoint (s_ptaa.c), ICD vertex adjust (glshade.h) and the
** SI 1.1 texture call (s/q, t/q, rho 1) */
void __fastcall __glRenderAntiAliasedRGBPoint(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat xCenter, yCenter, radius, radiusSquared, coverage, x, y;
    __GLfloat oldAlpha, xStart;
    __GLfragmentICD frag;
    GLint w, width, height, ixLeft, iyBottom;
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    PTfn_store store = (PTfn_store) gc->procs.store;
    __GL_POINT_VERT_SAVES;                 /* adjustData: window, color, texture */

    gc->constants.viewportXAdjust += __GL_VERTEX_BIAS;
    gc->constants.viewportYAdjust += __GL_VERTEX_BIAS;
    gc->transform.clipX0 += __GL_VERTEX_BIAS;
    gc->transform.clipX1 += __GL_VERTEX_BIAS;
    gc->transform.clipY0 += __GL_VERTEX_BIAS;
    gc->transform.clipY1 += __GL_VERTEX_BIAS;
    __GL_ADJUST_VERT(gc, vx, adjustData.window.x, adjustData.window.y, adjustData.window.z, adjustData.texture);
    adjustData.color = *vx->color;
    vx->color->r *= gc->constants.redRescale;
    vx->color->g *= gc->constants.greenRescale;
    vx->color->b *= gc->constants.blueRescale;
    vx->color->a *= gc->constants.alphaRescale;

    if (gc->procs.store == gc->procs.cfbStore) {
        /* We need ownership testing */
        store = (PTfn_store) __glDoStore;
    }

    radius = gc->state.point.smoothSize * __glHalf;
    radiusSquared = radius * radius;
    xCenter = vx->window.x - __glHalf;
    yCenter = vx->window.y - __glHalf;

    /* Truncate down to get starting coordinate */
    ixLeft = (GLint) (xCenter - radius);
    iyBottom = (GLint) (yCenter - radius);

    width = ((GLint) (xCenter + radius)) + 1 - ixLeft;
    height = ((GLint) (yCenter + radius)) + 1 - iyBottom;

    frag.z = (__GLzValue)vx->window.z;
    frag.color = *vx->color;
    if (modeFlags & __GL_SHADE_TEXTURE) {
        __GLfloat qInv = __GL_ONE_F / vx->texture[0].w;

        (*(PTfn_texture) gc->procs.texture[0])(gc, &frag.color,
            vx->texture[0].x * qInv, vx->texture[0].y * qInv, __GL_ONE_F);
    }
    if (modeFlags & __GL_SHADE_SLOW_FOG) {
        if (gc->state.fog.coordSource == GL_FRAGMENT_DEPTH_EXT) {
            (*(PTfn_fogPoint) gc->procs.fogPoint)(gc, &frag, vx->eye.z);
        } else {
            (*(PTfn_fogPoint) gc->procs.fogPoint)(gc, &frag, vx->fogCoord);
        }
    }

    oldAlpha = frag.color.a;
    xStart = ixLeft + __GL_ONE_F - xCenter;
    y = iyBottom + __GL_ONE_F - yCenter;
    frag.y = iyBottom;
    while (--height >= 0) {
        x = xStart;
        frag.x = ixLeft;
        for (w = width; --w >= 0; ) {
            coverage = Coverage(x, y, radiusSquared);
            if (coverage > __GL_ZERO_F) {
                frag.color.a = oldAlpha * coverage;
                (*store)(gc->drawBuffer, &frag);
            }
            x += __GL_ONE_F;
            frag.x++;
        }
        y += __GL_ONE_F;
        frag.y++;
    }

    gc->constants.viewportXAdjust -= __GL_VERTEX_BIAS;
    gc->constants.viewportYAdjust -= __GL_VERTEX_BIAS;
    gc->transform.clipX0 -= __GL_VERTEX_BIAS;
    gc->transform.clipX1 -= __GL_VERTEX_BIAS;
    gc->transform.clipY0 -= __GL_VERTEX_BIAS;
    gc->transform.clipY1 -= __GL_VERTEX_BIAS;
    __GL_RESTORE_VERT(vx, adjustData.window.x, adjustData.window.y, adjustData.window.z, adjustData.texture);
    vx->color->r = adjustData.color.r;
    vx->color->g = adjustData.color.g;
    vx->color->b = adjustData.color.b;
    vx->color->a = adjustData.color.a;
}

/* 690d1f00 -- s_ptaa.c __glRenderAntiAliasedCIPoint */
void __fastcall __glRenderAntiAliasedCIPoint(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat xCenter, yCenter, radius, radiusSquared, coverage, x, y;
    __GLfloat zero, one, oldIndex, xStart;
    __GLfragmentICD frag;
    GLint w, width, height, ixLeft, iyBottom;
    __GLstoreProc store = (__GLstoreProc)gc->procs.store;
    __GL_POINT_VERT_SAVES;

    __GL_ADJUST_POINT_VERTEX(gc, vx);

    if (gc->procs.store == gc->procs.cfbStore) {
	/* We need ownership testing */
	store = __glDoStore;
    }

    radius = gc->state.point.smoothSize * __glHalf;
    radiusSquared = radius * radius;
    xCenter = vx->window.x - __glHalf;
    yCenter = vx->window.y - __glHalf;

    /* Truncate down to get starting coordinate */
    ixLeft = (GLint) (xCenter - radius);
    iyBottom = (GLint) (yCenter - radius);

    width = ((GLint) (xCenter + radius)) + 1 - ixLeft;
    height = ((GLint) (yCenter + radius)) + 1 - iyBottom;

    frag.z = (__GLzValue)vx->window.z;
    frag.color.r = vx->color->r;
    if (__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_SLOW_FOG) {
	__GL_FOG_POINT(gc, &frag, vx)
    }

    zero = __GL_ZERO_F;
    one = __GL_ONE_F;
    oldIndex = frag.color.r;
    xStart = ixLeft + __GL_ONE_F - xCenter;
    y = iyBottom + __GL_ONE_F - yCenter;
    frag.y = iyBottom;
    while (--height >= 0) {
	x = xStart;
	frag.x = ixLeft;
	for (w = width; --w >= 0; ) {
	    coverage = Coverage(x, y, radiusSquared);
	    if (coverage > zero) {
		frag.color.r = __glBuildAntiAliasIndex(oldIndex, coverage);
		(*store)(gc->drawBuffer, &frag);
	    }
	    x += one;
	    frag.x++;
	}
	y += one;
	frag.y++;
    }

    __GL_RESTORE_POINT_VERTEX(gc, vx);
}

/* 690d1e30 -- ptaa_Coverage: Coverage (s_ptaa.c; the ICD subtracts the adjust; float args on the stack, called
** through the Coverage cast macro, hence the two unused register slots) */
double __fastcall ptaa_Coverage(GLuint ecx_unused, GLuint edx_unused, __GLfloat xStart,
			       __GLfloat yStart, __GLfloat radiusSquared)
{
    GLint i;
    __GLfloat delta, yBottom, sampleX, sampleY;
    __GLfloat hits, hitsInc;

    sampleX = xStart - __GL_COORD_ADJUST;
    yBottom = yStart - __GL_COORD_ADJUST;

    delta = __GL_SAMPLE_DELTA;
    hits = __glZeroW16;
    hitsInc = __GL_SAMPLE_HIT;
    for (i = __GL_SAMPLES; --i >= 0; ) {
	__GLfloat check = radiusSquared - sampleX * sampleX;

	/* Unrolled inner loop - change this code if __GL_SAMPLES changes */
	sampleY = yBottom;
	if (sampleY * sampleY <= check) {
	    hits += hitsInc;
	}
	sampleY += delta;
	if (sampleY * sampleY <= check) {
	    hits += hitsInc;
	}
	sampleY += delta;
	if (sampleY * sampleY <= check) {
	    hits += hitsInc;
	}
	sampleY += delta;
	if (sampleY * sampleY <= check) {
	    hits += hitsInc;
	}

	sampleX += delta;
    }
    return hits;
}

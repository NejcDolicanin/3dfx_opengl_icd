/*
** gl_lnclip.c -- line clipping (s_lnclip.c), written from the SGI Sample Implementation source
** with the ICD's vertex: clip codes in flagBits >> 16, validateVertex[] indexed by
** (flagBits >> 14) & 3, no zero test on clip.w, and the flat colour also moves the packed colour.
*/
#include "wglpriv.h"
#include "icdprotos.h"
#include "glsi.h"
#include "icdglob.h"
#include "glsitypes.h"                      /* __GLclipVertex */
#include "si_names.h"
#include "glshade.h"

/* vertex.h, ICD values */
#define __GL_VERTEX_0D_SIZE     0
#define __GL_FAST_CLIP_COLOR    (__GL_HAS_FRONT_COLOR | __GL_HAS_PACKED_COLOR)  /* the fast clippers' colour needs */
#define __GL_HAS_LIGHTING_COLOR (__GL_HAS_FRONT_COLOR | __GL_HAS_LIGHTING)     /* what __glClipLine strips for flat lines */

typedef void (__fastcall *__GLvalidateProc)(__GLcontext *gc, __GLvertexICD *vx, GLuint needs);
#define DO_VALIDATE(gc, vx, needs) \
    ((*(__GLvalidateProc)(gc)->procs.validateVertex[((vx)->flagBits >> 14) & 3])(gc, vx, needs))
#define __GL_NEEDS(gc)          (((GLuint (__fastcall *)(__GLcontext *))(gc)->vertex.getNeeds)(gc))
#define __GL_FACE_NEEDS0(gc)    (((GLuint (__fastcall *)(__GLcontext *))(gc)->vertex.getFaceNeeds0)(gc))

typedef void (__fastcall *__GLlineClipProc)(__GLvertexICD *dst, const __GLvertexICD *a,
					    const __GLvertexICD *b, __GLfloat t);

/* __gl_frustumClipPlanes: six planes from 690f5328 (gdata splits the table in two) */

/* 690dc7c0 -- __glClipLine */
void __fastcall __glClipLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b)
{
    __GLvertexICD *provoking = b;
    __GLvertexICD np1, np2;
    __GLcoord *plane;
    GLuint needs, allClipCodes, clipCodes;
    __GLlineClipProc clip;
    __GLfloat zero;
    __GLfloat vpXCenter, vpYCenter, vpZCenter;
    __GLfloat vpXScale, vpYScale, vpZScale;
    __GLviewport *vp;
    __GLfloat wInv;
    GLuint packedColor;

    /* Check for trivial pass of the line */
    needs = __GL_FACE_NEEDS0(gc) | __GL_HAS_CLIP;
    allClipCodes = (a->flagBits | b->flagBits) & __GL_CLIP_MASK;
    allClipCodes >>= __GL_CLIP_SHIFT;

    /*
    ** Have to clip.  Validate a&b before starting.  This might be
    ** wasted, but only in "rare?" cases.
    */
    if (~(a->flagBits) & needs) {
	DO_VALIDATE(gc, a, needs);
    }
    if (~(b->flagBits) & needs) {
	DO_VALIDATE(gc, b, needs);
    }

    /*
    ** For each clippling plane that something is out on, clip
    ** check the verticies.  Note that no bits will be set in
    ** allClipCodes for clip planes that are not enabled.
    */
    zero = __GL_ZERO_F;
    clip = (__GLlineClipProc)gc->procs.lineClipParam;

    /* 
    ** Do user clip planes first, because we will maintain eye coordinates
    ** only while doing user clip planes.  They are ignored for the
    ** frustum clipping planes.
    */
    clipCodes = allClipCodes >> 6;
    if (clipCodes) {
	plane = gc->state.transform.eyeClipPlanes;
	do {
	    /*
	    ** See if this clip plane has anything out of it.  If not,
	    ** press onward to check the next plane.  Note that we
	    ** shift this mask to the right at the bottom of the loop.
	    */
	    if (clipCodes & (__GL_CLIP_LEFT >> __GL_CLIP_SHIFT)) {
		__GLfloat t, d1, d2;

		d1 = (plane->x * a->eye.x) + (plane->y * a->eye.y) +
		     (plane->z * a->eye.z) + (plane->w * a->eye.w);
		d2 = (plane->x * b->eye.x) + (plane->y * b->eye.y) +
		     (plane->z * b->eye.z) + (plane->w * b->eye.w);
		if (d1 < zero) {
		    /* a is out */
		    if (d2 < zero) {
			/* a & b are out */
			return;
		    }

		    /*
		    ** A is out and B is in.  Compute new A coordinate
		    ** clipped to the plane.
		    */
		    t = d2 / (d2 - d1);
		    (*clip)(&np1, a, b, t);
		    np1.eye.x = t*(a->eye.x - b->eye.x) + b->eye.x;
		    np1.eye.y = t*(a->eye.y - b->eye.y) + b->eye.y;
		    np1.eye.z = t*(a->eye.z - b->eye.z) + b->eye.z;
		    np1.eye.w = t*(a->eye.w - b->eye.w) + b->eye.w;
		    a = &np1;
		    a->flagBits = 
		      (b->flagBits & ~__GL_HAS_PACKED_COLOR) | 
			__GL_VERTEX_0D_SIZE;
		    a->color = &a->colors[__GL_FRONTFACE];
		} else {
		    /* a is in */
		    if (d2 < zero) {
			/*
			** A is in and B is out.  Compute new B
			** coordinate clipped to the plane.
			**
			** NOTE: To avoid cracking in polygons with
			** shared clipped edges we always compute "t"
			** from the out vertex to the in vertex.  The
			** above clipping code gets this for free (b is
			** in and a is out).  In this code b is out and a
			** is in, so we reverse the t computation and the
			** argument order to (*clip).
			*/
			t = d1 / (d1 - d2);
			(*clip)(&np2, b, a, t);
			np2.eye.x = t*(b->eye.x - a->eye.x) + a->eye.x;
			np2.eye.y = t*(b->eye.y - a->eye.y) + a->eye.y;
			np2.eye.z = t*(b->eye.z - a->eye.z) + a->eye.z;
			np2.eye.w = t*(b->eye.w - a->eye.w) + a->eye.w;
			b = &np2;
			b->flagBits = 
			  (a->flagBits & ~__GL_HAS_PACKED_COLOR) | 
			    __GL_VERTEX_0D_SIZE;
			b->color = &b->colors[__GL_FRONTFACE];
		    } else {
			/* A and B are in */
		    }
		}
	    }
	    plane++;
	    clipCodes >>= 1;
	} while (clipCodes);
    }

    allClipCodes &= (__GL_CLIP_FRUSTUM_MASK >> __GL_CLIP_SHIFT);
    if (allClipCodes) {
	plane = &__gl_frustumClipPlanes[0];
	do {
	    /*
	    ** See if this clip plane has anything out of it.  If not,
	    ** press onward to check the next plane.  Note that we
	    ** shift this mask to the right at the bottom of the loop.
	    */
	    if (allClipCodes & (__GL_CLIP_LEFT >> __GL_CLIP_SHIFT)) {
		__GLfloat t, d1, d2;

		d1 = (plane->x * a->clip.x) + (plane->y * a->clip.y) +
		     (plane->z * a->clip.z) + (plane->w * a->clip.w);
		d2 = (plane->x * b->clip.x) + (plane->y * b->clip.y) +
		     (plane->z * b->clip.z) + (plane->w * b->clip.w);
		if (d1 < zero) {
		    /* a is out */
		    if (d2 < zero) {
			/* a & b are out */
			return;
		    }

		    /*
		    ** A is out and B is in.  Compute new A coordinate
		    ** clipped to the plane.
		    */
		    t = d2 / (d2 - d1);
		    (*clip)(&np1, a, b, t);
		    a = &np1;
		    a->flagBits = 
		      (b->flagBits & ~__GL_HAS_PACKED_COLOR) | 
			__GL_VERTEX_0D_SIZE;
		    a->color = &a->colors[__GL_FRONTFACE];
		} else {
		    /* a is in */
		    if (d2 < zero) {
			/*
			** A is in and B is out.  Compute new B
			** coordinate clipped to the plane.
			*/
			t = d1 / (d1 - d2);
			(*clip)(&np2, b, a, t);
			b = &np2;
			b->flagBits =
			 (a->flagBits & ~__GL_HAS_PACKED_COLOR) | 
			  __GL_VERTEX_0D_SIZE;
			b->color = &b->colors[__GL_FRONTFACE];
		    } else {
			/* A and B are in */
		    }
		}
	    }
	    plane++;
	    allClipCodes >>= 1;
	} while (allClipCodes);
    }

    vp = &gc->state.viewport;
    vpXCenter = vp->center.x;
    vpYCenter = vp->center.y;
    vpZCenter = vp->center.z;
    vpXScale = vp->scale.x;
    vpYScale = vp->scale.y;
    vpZScale = vp->scale.z;

    /* Compute window coordinates for both vertices. */
    wInv = __GL_ONE_F / a->clip.w;
    a->window.z = (__GLfloat)(a->clip.z) * wInv * vpZScale + vpZCenter;
    a->window.w = wInv;
    a->window.x = (__GLfloat)(a->clip.x) * wInv * vpXScale + vpXCenter;
    a->window.y = (__GLfloat)(a->clip.y) * wInv * vpYScale + vpYCenter;
    if ((a->flagBits & __GL_CLIP_MASK) &&
	__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_PROJSCALED_TEXTURE) {
	a->texture[0].x *= a->window.w;
	a->texture[0].y *= a->window.w;
	a->texture[0].w *= a->window.w;
    }

    wInv = __GL_ONE_F / b->clip.w;
    b->window.z = (__GLfloat)(b->clip.z) * wInv * vpZScale + vpZCenter;
    b->window.w = wInv;
    /* b->window.w, not wInv (the same value, kept in the register): read here, it makes the texture
    ** products below load texture[0].x first, as the original */
    b->window.x = (__GLfloat)(b->clip.x) * b->window.w * vpXScale + vpXCenter;
    b->window.y = (__GLfloat)(b->clip.y) * b->window.w * vpYScale + vpYCenter;
    if ((b->flagBits & __GL_CLIP_MASK) &&
	__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_PROJSCALED_TEXTURE) {
	b->texture[0].x *= b->window.w;
	b->texture[0].y *= b->window.w;
	b->texture[0].w *= b->window.w;
    }

    /* Validate line state */
    if (gc->state.light.shadingModel == GL_FLAT) {
	GLuint pn = needs & __GL_HAS_LIGHTING_COLOR;

	/* Validate provoking vertex color */
	if (~provoking->flagBits & pn) {
	    DO_VALIDATE(gc, provoking, pn);
	}
	packedColor = b->packedColor;
	b->packedColor = provoking->packedColor;
	b->color = &provoking->colors[__GL_FRONTFACE];

	/* Validate a&b verticies.  Don't need color so strip it out */
	needs &= ~__GL_HAS_LIGHTING_COLOR;
	if (~a->flagBits & needs) DO_VALIDATE(gc, a, needs);
	if (~b->flagBits & needs) DO_VALIDATE(gc, b, needs);

	/* Draw the line then restore the b color pointer */
	(*(__GLrenderLineProc)gc->procs.renderLine)(gc, a, b);
	b->packedColor = packedColor;
	b->color = &b->colors[__GL_FRONTFACE];
    } else {
	if (~a->flagBits & needs) DO_VALIDATE(gc, a, needs);
	if (~b->flagBits & needs) DO_VALIDATE(gc, b, needs);

	(*(__GLrenderLineProc)gc->procs.renderLine)(gc, a, b);
    }
}

/* 690dcd60 -- __glFastClipSmoothLine */
void __fastcall __glFastClipSmoothLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b)
{
    GLuint needs;

    if ((a->flagBits | b->flagBits) & __GL_CLIP_MASK) {
	/*
	** The line must be clipped more carefully.  Cannot trivially
	** accept the lines.
	*/
	if ((a->flagBits & b->flagBits & __GL_CLIP_MASK) != 0) {
	    /*
	    ** Trivially reject the line.  If anding the codes is non-zero then
	    ** every vertex in the line is outside of the same set of
	    ** clipping planes (at least one).
	    */
	    return;
	}
	__glClipLine(gc, a, b);
	return;
    }
    needs = __GL_NEEDS(gc) | __GL_FAST_CLIP_COLOR;
    DO_VALIDATE(gc, a, needs);
    DO_VALIDATE(gc, b, needs);
    (*(__GLrenderLineProc)gc->procs.renderLine)(gc, a, b);
}

/* 690dcdf0 -- __glFastClipFlatLine */
void __fastcall __glFastClipFlatLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b)
{
    GLuint needs;

    if ((a->flagBits | b->flagBits) & __GL_CLIP_MASK) {
	/*
	** The line must be clipped more carefully.  Cannot trivially
	** accept the lines.
	*/
	if ((a->flagBits & b->flagBits & __GL_CLIP_MASK) != 0) {
	    /*
	    ** Trivially reject the line.  If anding the codes is non-zero then
	    ** every vertex in the line is outside of the same set of
	    ** clipping planes (at least one).
	    */
	    return;
	}
	__glClipLine(gc, a, b);
	return;
    }
    needs = __GL_NEEDS(gc);

    /* 
    ** Validate a vertex.  Don't need color so strip it out.
    ** Note can't strip out __GL_HAS_LIGHTING because it will strip out
    ** __GL_HAS_EYE and eye.z is not exclusively used for lighting.
    */
    DO_VALIDATE(gc, a, needs & ~__GL_FAST_CLIP_COLOR);

    /* Validate provoking vertex color */
    DO_VALIDATE(gc, b, needs | __GL_FAST_CLIP_COLOR);

    /* Draw the line */
    (*(__GLrenderLineProc)gc->procs.renderLine)(gc, a, b);
}
#define __GL_FACE_NEEDS1(gc)    (((GLuint (__fastcall *)(__GLcontext *))(gc)->vertex.getFaceNeeds1)(gc))


/* 690dce80 -- __glClipAndRenderLine: no drawable lock in the ICD */
void __fastcall __glClipAndRenderLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b)
{
    __WGLdrawable *dp = gc->drawablePrivate;
    __GLdrawableRegion *glRegion;
    __GLregionRect *rect;
    __GLrenderLineProc renderLine2;

    /* the ICD does not lock the drawable here */

    glRegion = (__GLdrawableRegion *)&dp->numRects;
    rect = &glRegion->rects[0];
    renderLine2 = (__GLrenderLineProc)gc->procs.renderLine2;

    /* in the common case, no clipping is necessery (SI shape: rect->x0 ..; renderLine2 is fetched
    ** after rect, which gives the original's register assignment) */
    if (1 == glRegion->numRects &&
	gc->transform.reasonableViewport &&
	rect->x0 == gc->transform.clipX0 - gc->constants.viewportXAdjust &&
	rect->x1 == gc->transform.clipX1 - gc->constants.viewportXAdjust &&
	rect->y0 == gc->transform.clipY0 - gc->constants.viewportYAdjust &&
	rect->y1 == gc->transform.clipY1 - gc->constants.viewportYAdjust
	) {
	(*renderLine2)(gc, a, b);
	return;
    }

    /* 
    ** We must have validated vertices to work with, so we have to do
    ** the same work as clip line.
    ** Note that we take a pessimistic view of what we'll need.  We could
    ** probably do better in the double sided case here...
    */
    {
	GLuint needs = __GL_FACE_NEEDS0(gc) | __GL_FACE_NEEDS1(gc);

	if (~a->flagBits & needs) DO_VALIDATE(gc, a, needs);
	if (~b->flagBits & needs) DO_VALIDATE(gc, b, needs);
    }

    /* Now Clip to each region in the list in turn */
    {
	__GLclipVertex cv[2];
	GLint i;

	for (i=0; i < glRegion->numRects; i++) {
	    rect = &glRegion->rects[i];

	    cv[0].v = *a;
	    cv[1].v = *b;

	    /* generate outcode for vertices */
	    __glCalc2DOutcode(gc, &cv[0], rect);
	    __glCalc2DOutcode(gc, &cv[1], rect);

	    if ((cv[0].outcode | cv[1].outcode) == 0) {
		/* Fully visible. Just draw it. */
		(*renderLine2)(gc, a, b);
	    } else if ((cv[0].outcode & cv[1].outcode) == 0) {
		/* Partially visible.  Have to do a 2D clip. */

		static const GLuint edge_flags[] = {   /* 69006378 */
		    __GL_CLIP_LEFT, 
		    __GL_CLIP_RIGHT, 
		    __GL_CLIP_TOP, 
		    __GL_CLIP_BOTTOM
		};
		int tour;
		__GLclipVertex *a;
		__GLclipVertex *b;

		for (tour = 0; tour < 4; tour++) {
		    __GLclipVertex *newVertex;

		    /* trivial ordering of vertices */
		    a = &cv[0];
		    b = &cv[1];

		    if ((a->outcode ^ b->outcode) & edge_flags[tour]) {
			__GLfloat edge, t;

			if (tour < 2) {
			    /* swap vertices if necessary */
			    if ((a->v.window.x < b->v.window.x) ||
				((a->v.window.x == b->v.window.x) &&
				 (a->v.window.y < b->v.window.y))) {
				newVertex = a; a = b; b = newVertex;
			    }
			    if (tour == 0) {
				/* tour == 0 */
				edge = (__GLfloat)rect->x0;
				edge += gc->constants.fviewportXAdjust;
				newVertex = b;
			    } else {
				/* tour == 1 */
				edge = (__GLfloat)rect->x1;
				edge += gc->constants.fviewportXAdjust;
				newVertex = a;
			    }
			} else {
			    /* tour >= 2 */

			    /* swap vertices if necessary */
			    if ((a->v.window.y < b->v.window.y) ||
				((a->v.window.y == b->v.window.y) &&
				 (a->v.window.x < b->v.window.x))) {
				newVertex = a; a = b; b = newVertex;
			    }
			    if (tour == 2) {
				/* tour == 2 */
				edge = (__GLfloat)rect->y0;
				edge += gc->constants.fviewportYAdjust;
				newVertex = b;
			    } else {
				/* tour == 3 */
				edge = (__GLfloat)rect->y1;
				edge += gc->constants.fviewportYAdjust;
				newVertex = a;
			    }
			}

			if (tour < 2) {
			    t = (b->v.window.x - edge) /
				(b->v.window.x - a->v.window.x);
			    if ((t < __GL_ZERO_F) || (t > __GL_ONE_F)) continue;
			    newVertex->v.window.x = edge;
			    newVertex->v.window.y = b->v.window.y +
				t * (a->v.window.y - b->v.window.y);
			} else {
			    t = (b->v.window.y - edge) /
				(b->v.window.y - a->v.window.y);
			    if ((t < __GL_ZERO_F) || (t > __GL_ONE_F)) continue;
			    newVertex->v.window.x = b->v.window.x +
				t * (a->v.window.x - b->v.window.x);
			    newVertex->v.window.y = edge;
			}
			newVertex->v.window.z = b->v.window.z +
			    t * (a->v.window.z - b->v.window.z);
			newVertex->outcode &= ~edge_flags[tour];
		    }
		}

		(*renderLine2)(gc, &a->v, &b->v);
	    }
	}
    }

}

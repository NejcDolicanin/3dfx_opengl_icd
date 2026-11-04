/*
** gl_pgclip.c -- polygon clipping (s_pgclip.c), written from the SGI Sample Implementation
** source with the ICD's vertex (see gl_lnclip.c).  The ICD's __glClipPolygon fans the polygon
** into triangles for __glClipTriangle instead of clipping it whole.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "icdglob.h"
#include "glsitypes.h"                      /* __GLclipVertex */
#include "si_names.h"
#include "glshade.h"

/* vertex.h, ICD values */
/* the codes __glClipPolygon passes on: the clip codes plus 0x2000 */
#define __GL_CLIP_PASS_MASK     (__GL_CLIP_MASK | 0x2000)

#define __GL_MAX_CLIP_PLANES    16
#define __GL_NVBUF              16
#define	__GL_MAX_CLIP_VERTEX    (__GL_MAX_CLIP_PLANES + __GL_NVBUF)

/* the ICD also keeps the vertex size bit out of a clipped vertex */
#define __GL_COPY_FLAGBITS(dst, src) \
  (dst->flagBits = (dst->flagBits & __GL_CLIP_MASK) | \
   (src->flagBits & ~(__GL_CLIP_MASK | __GL_HAS_PACKED_COLOR)))

typedef void (__fastcall *__GLvalidateProc)(__GLcontext *gc, __GLvertexICD *vx, GLuint needs);
#define DO_VALIDATE(gc, vx, needs) \
    ((*(__GLvalidateProc)(gc)->procs.validateVertex[((vx)->flagBits >> 14) & 3])(gc, vx, needs))
#define __GL_FACE_NEEDS0(gc)    (((GLuint (__fastcall *)(__GLcontext *))(gc)->vertex.getFaceNeeds0)(gc))
#define __GL_FACE_NEEDS1(gc)    (((GLuint (__fastcall *)(__GLcontext *))(gc)->vertex.getFaceNeeds1)(gc))

typedef void (__fastcall *__GLclipParamProc)(__GLvertexICD *dst, const __GLvertexICD *a,
					     const __GLvertexICD *b, __GLfloat t);
/* the polygon clip parameter proc the clipper uses: procs.polyClipParam (0x25a4) */
#define __GL_POLY_CLIP_PARAM(gc) ((__GLclipParamProc)(gc)->procs.polyClipParam)

/* __gl_frustumClipPlanes: six planes from 690f5328 (gdata splits the table in two) */

extern GLint __fastcall clipToPlane(__GLcontext *gc, __GLvertexICD **iv, GLint niv,
				     __GLvertexICD **ov, __GLcoord *plane);
extern GLint __fastcall clipToPlaneEye(__GLcontext *gc, __GLvertexICD **iv, GLint niv,
				     __GLvertexICD **ov, __GLcoord *plane);
extern void __fastcall __glClipTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b,
				    __GLvertexICD *c, GLuint orCodes);

/* 69096090 -- __glDoPolygonClip */
void __fastcall __glDoPolygonClip(__GLcontext *gc, __GLvertexICD **iv, GLint nout,
			     GLuint allClipCodes)
{
    __GLvertexICD *ov[__GL_MAX_CLIP_PLANES][__GL_MAX_CLIP_VERTEX];
    __GLvertexICD **ivp;
    __GLvertexICD **ovp;
    __GLvertexICD *p0, *p1, *p2;
    __GLcoord *plane;
    GLint i;
    __GLviewport *vp;
    __GLfloat vpXScale, vpYScale, vpZScale;
    __GLfloat vpXCenter, vpYCenter, vpZCenter;
    __GLfloat llx, lly, urx, ury;
    __GLfloat winx, winy;
    GLuint clipCodes;
    __GLfillTriangleProc renderTriangle;
    GLuint needs = __GL_FACE_NEEDS0(gc) | __GL_FACE_NEEDS1(gc) | __GL_HAS_CLIP;
    
    allClipCodes &= __GL_CLIP_MASK;
    allClipCodes >>= __GL_CLIP_SHIFT;

    /*
    ** Reset nextClipTemp pointer for any new verticies that are generated
    ** during the clipping.
    */
    gc->transform.nextClipTemp = gc->transform.clipTemp;

    ivp = &iv[0];

    /*
    ** Check each of the clipping planes by examining the allClipCodes
    ** mask. Note that no bits will be set in allClipCodes for clip
    ** planes that are not enabled.
    */
    if (allClipCodes) {
	/* Validate vertex data before clipping */
	ovp = ivp;
	for (i = nout; --i >= 0; ) {
	    __GLvertexICD *vx = *ovp++;
	    if (~vx->flagBits & needs) DO_VALIDATE(gc, vx, needs);
	}

	/* Now clip against the clipping planes */
	ovp = &ov[0][0];

	/* 
	** Do user clip planes first, because we will maintain eye coordinates
	** only while doing user clip planes.  They are ignored for the 
	** frustum clipping planes.
	*/
	clipCodes = allClipCodes >> 6;
	if (clipCodes) {
	    plane = gc->state.transform.eyeClipPlanes;
	    do {
		if (clipCodes & (__GL_CLIP_LEFT >> __GL_CLIP_SHIFT)) {
		    nout = clipToPlaneEye(gc, ivp, nout, ovp, plane);
		    if (nout < 3) {
			return;
		    }
		    ivp = ovp;
		    ovp += __GL_MAX_CLIP_VERTEX;
		}
		clipCodes >>= 1;
		plane++;
	    } while (clipCodes);
	}

	allClipCodes &= (__GL_CLIP_FRUSTUM_MASK >> __GL_CLIP_SHIFT);
	if (allClipCodes) {
	    plane = &__gl_frustumClipPlanes[0];
	    do {
		if (allClipCodes & (__GL_CLIP_LEFT >> __GL_CLIP_SHIFT)) {
		    nout = clipToPlane(gc, ivp, nout, ovp, plane);
		    if (nout < 3) {
			return;
		    }
		    ivp = ovp;
		    ovp += __GL_MAX_CLIP_VERTEX;
		}
		allClipCodes >>= 1;
		plane++;
	    } while (allClipCodes);
	}

	/*
	** Calculate final screen coordinates.  Next phase of polygon
	** processing assumes that window coordinates are already computed.
	*/
	vp = &gc->state.viewport;
	vpXCenter = vp->center.x;
	vpYCenter = vp->center.y;
	vpZCenter = vp->center.z;
	vpXScale = vp->scale.x;
	vpYScale = vp->scale.y;
	vpZScale = vp->scale.z;
	ovp = ivp;

	llx = vpXCenter - vpXScale;
	urx = vpXCenter + vpXScale;
	if (vpYScale > 0) {
	    lly = vpYCenter - vpYScale;
	    ury = vpYCenter + vpYScale;
	} else {
	    lly = vpYCenter + vpYScale;
	    ury = vpYCenter - vpYScale;
	}

	for (i = nout; --i >= 0; ) {
	    __GLfloat wInv;

	    p0 = *ovp++;

	    wInv = __GL_ONE_F / p0->clip.w;
	    winx = (__GLfloat)(p0->clip.x) * wInv * vpXScale + vpXCenter;
	    winy = (__GLfloat)(p0->clip.y) * wInv * vpYScale + vpYCenter;
	    p0->window.z = (__GLfloat)(p0->clip.z) * wInv * vpZScale + vpZCenter;
	    p0->window.w = wInv;	/* stored after z: the texture products below then load window.w first */
	    /* 
	    ** Check if these window coordinates are legal.  At this 
	    ** point, it is quite possible that they are not.  Trivially
	    ** pull them into the legal viewport region if necessary.
	    */
	    if (winx < llx) winx = llx;
	    else if (winx > urx) winx = urx;
	    if (winy < lly) winy = lly;
	    else if (winy > ury) winy = ury;
	    p0->window.x = winx;
	    p0->window.y = winy;
	    if ((p0->flagBits & 0x100000) &&
		__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_PROJSCALED_TEXTURE) {
		p0->texture[0].x *= p0->window.w;
		p0->texture[0].y *= p0->window.w;
		p0->texture[0].w *= p0->window.w;
		p0->texture[1].x *= p0->window.w;
		p0->texture[1].y *= p0->window.w;
		p0->texture[1].w *= p0->window.w;
	    }
	}
    }

    /*
    ** Subdivide the clipped polygon into triangles.  Only convex polys
    ** are supported so this is okay to do.  Non-convex polys will do
    ** something odd here, but thats the clients fault.
    */
    p0 = *ivp++;
    p1 = *ivp++;
    renderTriangle = (__GLfillTriangleProc)gc->procs.renderTriangle;
    p2 = *ivp++;
    if (nout == 3) {
	(*renderTriangle)(gc, p0, p1, p2);
    } else {
	for (i = 0; i < nout - 2; i++) {
	    GLuint t1, t2;
	    if (i == 0) {
		/*
		** Third edge of first sub-triangle is always non-boundary
		*/
		t1 = p2->flagBits & __GL_VERTEX_EDGE_FLAG;
		p2->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
		(*renderTriangle)(gc, p0, p1, p2);
		p2->flagBits |= t1;
	    } else
	    if (i == nout - 3) {
		/*
		** First edge of last sub-triangle is always non-boundary
		*/
		t1 = p0->flagBits & __GL_VERTEX_EDGE_FLAG;
		p0->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
		(*renderTriangle)(gc, p0, p1, p2);
		p0->flagBits |= t1;
	    } else {
		/*
		** Interior sub-triangles have the first and last edge
		** marked non-boundary
		*/
		t1 = p0->flagBits & __GL_VERTEX_EDGE_FLAG;
		t2 = p2->flagBits & __GL_VERTEX_EDGE_FLAG;
		p0->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
		p2->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
		(*renderTriangle)(gc, p0, p1, p2);
		p0->flagBits |= t1;
		p2->flagBits |= t2;
	    }
	    p1 = p2;
	    p2 = (__GLvertexICD *) *ivp++;
	}
    }
}

/* 690964b0 -- clipToPlane (s_pgclip.c): Sutherland-Hodgman clip of the niv-vertex polygon iv against one frustum
** plane in clip coordinates into ov, returning the output count; new vertices come from gc->transform.nextClipTemp
** (advanced on success), are interpolated out -> in by procs.polyClipParam and take the flag bits
** of the edge's start vertex s (its own clip codes kept); a third generated vertex means a non-convex polygon and
** returns 0 */
GLint __fastcall clipToPlane(__GLcontext *gc, __GLvertexICD **iv, GLint niv,
			 __GLvertexICD **ov, __GLcoord *plane)
{
    GLint i, nout, generated;
    __GLvertexICD *s, *p, *newVertex, *temp;
    __GLfloat pDist, sDist, t;
    __GLfloat zero = __GL_ZERO_F;
    __GLclipParamProc clip;

    nout = 0;
    generated = 0;
    temp = gc->transform.nextClipTemp;
    clip = __GL_POLY_CLIP_PARAM(gc);

    s = iv[niv-1];
    sDist = (s->clip.x * plane->x) + (s->clip.y * plane->y) +
	    (s->clip.z * plane->z) + (s->clip.w * plane->w);
    for (i = 0; i < niv; i++) {
	p = iv[i];
	pDist = (p->clip.x * plane->x) + (p->clip.y * plane->y) +
		(p->clip.z * plane->z) + (p->clip.w * plane->w);
	if (pDist >= zero) {
	    /* p is inside the clipping plane half space */
	    if (sDist >= zero) {
		/* s is inside the clipping plane half space */
		*ov++ = p;
		nout++;
	    } else {
		/* s is outside the clipping plane half space */
		t = pDist / (pDist - sDist);
		newVertex = temp++;
		(*clip)(newVertex, s, p, t);
		__GL_COPY_FLAGBITS(newVertex, s);
		*ov++ = newVertex;
		*ov++ = p;
		nout += 2;

		if (++generated >= 3) {
		    /* Toss the non-convex polygon */
		    return 0;
		}
	    }
	} else {
	    /* p is outside the clipping plane half space */
	    if (sDist >= zero) {
		/*
		** s is inside the clipping plane half space
		**
		** NOTE: To avoid cracking in polygons with shared
		** clipped edges we always compute "t" from the out
		** vertex to the in vertex.  The above clipping code gets
		** this for free (p is in and s is out).  In this code p
		** is out and s is in, so we reverse the t computation
		** and the argument order to __glDoClip.
		*/
		t = sDist / (sDist - pDist);
		newVertex = temp++;
		(*clip)(newVertex, p, s, t);
		__GL_COPY_FLAGBITS(newVertex, s);
		*ov++ = newVertex;
		nout++;

		if (++generated >= 3) {
		    /* Toss the non-convex polygon */
		    return 0;
		}
	    } else {
		/* both points are outside */
	    }
	}
	s = p;
	sDist = pDist;
    }
    gc->transform.nextClipTemp = temp;
    return nout;
}

/* 690966a0 -- clipToPlaneEye (s_pgclip.c): clipToPlane against a user clip plane, with the distances taken from
** the eye coordinates and the eye position also interpolated into each new vertex */
GLint __fastcall clipToPlaneEye(__GLcontext *gc, __GLvertexICD **iv, GLint niv,
			    __GLvertexICD **ov, __GLcoord *plane)
{
    GLint i, nout, generated;
    __GLvertexICD *s, *p, *newVertex, *temp;
    __GLfloat pDist, sDist, t;
    __GLfloat zero = __GL_ZERO_F;
    __GLclipParamProc clip;

    nout = 0;
    generated = 0;
    temp = gc->transform.nextClipTemp;
    clip = __GL_POLY_CLIP_PARAM(gc);

    s = iv[niv-1];
    sDist = (s->eye.x * plane->x) + (s->eye.y * plane->y) +
	    (s->eye.z * plane->z) + (s->eye.w * plane->w);
    for (i = 0; i < niv; i++) {
	p = iv[i];
	pDist = (p->eye.x * plane->x) + (p->eye.y * plane->y) +
		(p->eye.z * plane->z) + (p->eye.w * plane->w);
	if (pDist >= zero) {
	    /* p is inside the clipping plane half space */
	    if (sDist >= zero) {
		/* s is inside the clipping plane half space */
		*ov++ = p;
		nout++;
	    } else {
		/* s is outside the clipping plane half space */
		t = pDist / (pDist - sDist);
		newVertex = temp++;
		(*clip)(newVertex, s, p, t);
		newVertex->eye.x = t*(s->eye.x - p->eye.x) + p->eye.x;
		newVertex->eye.y = t*(s->eye.y - p->eye.y) + p->eye.y;
		newVertex->eye.z = t*(s->eye.z - p->eye.z) + p->eye.z;
		newVertex->eye.w = t*(s->eye.w - p->eye.w) + p->eye.w;
		__GL_COPY_FLAGBITS(newVertex, s);
		*ov++ = newVertex;
		*ov++ = p;
		nout += 2;

		if (++generated >= 3) {
		    /* Toss the non-convex polygon */
		    return 0;
		}
	    }
	} else {
	    /* p is outside the clipping plane half space */
	    if (sDist >= zero) {
		/*
		** s is inside the clipping plane half space
		**
		** NOTE: To avoid cracking in polygons with shared
		** clipped edges we always compute "t" from the out
		** vertex to the in vertex.  The above clipping code gets
		** this for free (p is in and s is out).  In this code p
		** is out and s is in, so we reverse the t computation
		** and the argument order to __glDoClip.
		*/
		t = sDist / (sDist - pDist);
		newVertex = temp++;
		(*clip)(newVertex, p, s, t);
		newVertex->eye.x = t*(p->eye.x - s->eye.x) + s->eye.x;
		newVertex->eye.y = t*(p->eye.y - s->eye.y) + s->eye.y;
		newVertex->eye.z = t*(p->eye.z - s->eye.z) + s->eye.z;
		newVertex->eye.w = t*(p->eye.w - s->eye.w) + s->eye.w;
		__GL_COPY_FLAGBITS(newVertex, s);
		*ov++ = newVertex;
		nout++;

		if (++generated >= 3) {
		    /* Toss the non-convex polygon */
		    return 0;
		}
	    } else {
		/* both points are outside */
	    }
	}
	s = p;
	sDist = pDist;
    }
    gc->transform.nextClipTemp = temp;
    return nout;
}

/* 69096910 -- __glClipPolygon: fanned into triangles for __glClipTriangle (edge flags as in
** __glDoPolygonClip's subdivision; p0 / p1 / p2 as there, indexed from v0) */
void __fastcall __glClipPolygon(__GLcontext *gc, __GLvertexICD *v0, GLint nv)
{
    GLint i;
    __GLvertexICD *p0, *p1, *p2;

    gc->vertex.provoking = v0;
    p0 = v0;
    if (nv == 3) {
	p1 = &v0[1];
	p2 = &v0[2];
	__glClipTriangle(gc, p0, p1, p2,
		     (p2->flagBits | p1->flagBits | p0->flagBits) & __GL_CLIP_PASS_MASK);
	return;
    }
    for (i = 0; i < nv - 2; i++) {
	GLuint t1, t2, orCodes;

	p1 = &v0[i + 1];
	p2 = &v0[i + 2];
	orCodes = (p1->flagBits | p0->flagBits | p2->flagBits) & __GL_CLIP_PASS_MASK;
	if (i == 0) {
	    /*
	    ** Third edge of first sub-triangle is always non-boundary
	    */
	    t1 = p2->flagBits & __GL_VERTEX_EDGE_FLAG;
	    p2->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
	    __glClipTriangle(gc, p0, p1, p2, orCodes);
	    p2->flagBits |= t1;
	} else
	if (i == nv - 3) {
	    /*
	    ** First edge of last sub-triangle is always non-boundary
	    */
	    t1 = p0->flagBits & __GL_VERTEX_EDGE_FLAG;
	    p0->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
	    __glClipTriangle(gc, p0, p1, p2, orCodes);
	    p0->flagBits |= t1;
	} else {
	    /*
	    ** Interior sub-triangles have the first and last edge
	    ** marked non-boundary
	    */
	    t1 = p0->flagBits & __GL_VERTEX_EDGE_FLAG;
	    t2 = p2->flagBits & __GL_VERTEX_EDGE_FLAG;
	    p0->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
	    p2->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
	    __glClipTriangle(gc, p0, p1, p2, orCodes);
	    p0->flagBits |= t1;
	    p2->flagBits |= t2;
	}
    }
}

/* 69096a60 -- __glClipTriangle */
void __fastcall __glClipTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b,
			     __GLvertexICD *c, GLuint orCodes)
{
    __GLvertexICD *iv[3];

    iv[0] = a;
    iv[1] = b;
    iv[2] = c;
    __glDoPolygonClip(gc, &iv[0], 3, orCodes);
}

/* 69096a90 -- __glCalc2DOutcode */
void __fastcall __glCalc2DOutcode(__GLcontext *gc, __GLclipVertex *cv,
		       __GLregionRect *rect)
{
    __GLfloat x = cv->v.window.x;
    __GLfloat y = cv->v.window.y;
    GLuint code = 0;

    if (x < (float)rect->x0 + gc->constants.fviewportXAdjust)
	code |= __GL_CLIP_LEFT;
    if (x > (float)rect->x1 + gc->constants.fviewportXAdjust)
	code |= __GL_CLIP_RIGHT;
    if (y < (float)rect->y0 + gc->constants.fviewportYAdjust)
	code |= __GL_CLIP_TOP;
    if (y > (float)rect->y1 + gc->constants.fviewportYAdjust)
	code |= __GL_CLIP_BOTTOM;

    cv->outcode = code;
}


/* 69096b20 -- __glClipAndRenderTriangle: no drawable lock in the ICD */
void __fastcall __glClipAndRenderTriangle(__GLcontext *gc,
			       __GLvertexICD *v0,
			       __GLvertexICD *v1,
			       __GLvertexICD *v2)
{
    __WGLdrawable *dp = gc->drawablePrivate;
    __GLdrawableRegion *glRegion;
    __GLregionRect *rect;
    __GLfillTriangleProc renderTriangle2;

    /* the ICD does not lock the drawable here */

    glRegion = (__GLdrawableRegion *)&dp->numRects;
    rect = &glRegion->rects[0];
    renderTriangle2 = (__GLfillTriangleProc)gc->procs.renderTriangle2;

    /* In the common case, no clipping is necessary. */
    if (1 == glRegion->numRects &&
	gc->transform.reasonableViewport &&
	rect->x0 == gc->transform.clipX0 - gc->constants.viewportXAdjust &&
	rect->x1 == gc->transform.clipX1 - gc->constants.viewportXAdjust &&
	rect->y0 == gc->transform.clipY0 - gc->constants.viewportYAdjust &&
	rect->y1 == gc->transform.clipY1 - gc->constants.viewportYAdjust
	) {
	(*renderTriangle2)(gc, v0, v1, v2);
	return;
    }

    /* 
    ** We must have validated vertices to work with, so we have to do
    ** the same work as triangle setup.
    ** Note that we take a pessimistic view of what we'll need.  We could
    ** probably do better in the double sided case here...
    */
    {
	GLuint needs = __GL_FACE_NEEDS0(gc) | __GL_FACE_NEEDS1(gc);

	if (~v0->flagBits & needs) DO_VALIDATE(gc, v0, needs);
	if (~v1->flagBits & needs) DO_VALIDATE(gc, v1, needs);
	if (~v2->flagBits & needs) DO_VALIDATE(gc, v2, needs);
    }

    /* Now clip to each region in the list in turn. */
    {
	__GLclipVertex cv[3+8];		/* Can add 2 vertices per clip edge */
	int i;
	
	cv[0].v = *v0;
	cv[1].v = *v1;
	cv[2].v = *v2;

	for (i = 0; i < glRegion->numRects; i++) {
	    int j;

	    rect = &glRegion->rects[i];

	    /* Generate outcodes for vertices */
	    for (j = 0; j < 3; j++)
		__glCalc2DOutcode(gc, &cv[j], rect);

	    if ((cv[0].outcode | cv[1].outcode | cv[2].outcode) == 0) {
		/* Fully visible. Just draw it. */

		(*renderTriangle2)(gc, v0, v1, v2);

	    } else if ((cv[0].outcode & cv[1].outcode & cv[2].outcode) == 0) {
		/* Partially visible.  Have to do a 2D clip. */

		__GLclipVertex *polygon[2][7];
		int polygon_vertices, next_polygon_vertices;
		int current, next;	/* double-buffer 'polygon', above */
		int nvi;		/* new vertex index into 'cv' */
		__GLclipParamProc clip;
		static const GLuint edge_flags[] = { __GL_CLIP_LEFT,   /* 69005a0c */
						     __GL_CLIP_RIGHT,
						     __GL_CLIP_TOP,
						     __GL_CLIP_BOTTOM
		};
		int tour;

		clip = __GL_POLY_CLIP_PARAM(gc);

		polygon_vertices = 3;
		polygon[0][0] = &cv[0];
		polygon[0][1] = &cv[1];
		polygon[0][2] = &cv[2];
		
		current = 0;
		next = 1;
		nvi = 3;	/* after first three vertices */

		for (tour = 0; tour < 4; tour++) {
		    next_polygon_vertices = 0;
		    for (j = 0; j < polygon_vertices; j++) {
			__GLclipVertex *a, *b;
			GLboolean reversed = GL_FALSE;

			if (!(polygon[current][j]->outcode & edge_flags[tour])) {
			    polygon[next][next_polygon_vertices++] = 
				polygon[current][j];
			}

			a = polygon[current][j];
			b = polygon[current][(j + 1) % polygon_vertices];

			/* To guarantee consistency across shared edges, we always
			 * clip in the same direction.  To do this, we impose a strict
			 * ordering on a and b.
			 */
			if ((a->v.window.x < b->v.window.x) ||
			    ((a->v.window.x == b->v.window.x) && (a->v.window.y < b->v.window.y))) {
			    __GLclipVertex *temp;

			    temp = a;
			    a = b;
			    b = temp;
			    reversed = GL_TRUE;
			}

			if ((a->outcode ^ b->outcode) & edge_flags[tour]) {
			    __GLclipVertex *newVertex;
			    __GLfloat edge, t;

			    newVertex = &cv[nvi++];
			    newVertex->v.color = &newVertex->v.colors[__GL_FRONTFACE];

			    switch (tour) {
			    case 0:
				edge = (__GLfloat)rect->x0;
				edge += gc->constants.fviewportXAdjust;
				break;
			    case 1:
				edge = (__GLfloat)rect->x1;
				edge += gc->constants.fviewportXAdjust;
				break;
			    case 2:
				edge = (__GLfloat)rect->y0;
				edge += gc->constants.fviewportYAdjust;
				break;
			    case 3:
				edge = (__GLfloat)rect->y1;
				edge += gc->constants.fviewportYAdjust;
				break;
			    }

			    if (tour < 2) {
				__GLfloat dx = b->v.window.x - a->v.window.x;
				t = (b->v.window.x - edge) / dx;
				newVertex->v.window.x = edge;
				newVertex->v.window.y = 
				    b->v.window.y + t * (a->v.window.y - b->v.window.y);
			    } else {
				__GLfloat dy = b->v.window.y - a->v.window.y;
				t = (b->v.window.y - edge) / dy;
				newVertex->v.window.x = 
				    b->v.window.x + t * (a->v.window.x - b->v.window.x);
				newVertex->v.window.y = edge;
			    }
			    newVertex->v.window.z = 
				b->v.window.z + t * (a->v.window.z - b->v.window.z);

			    (*clip)(&newVertex->v, &a->v, &b->v, t);

			    /* XXX redo texture since it might have been messed up by clip() */
			    newVertex->v.texture[0].x = b->v.texture[0].x + t * (a->v.texture[0].x - b->v.texture[0].x);
			    newVertex->v.texture[0].y = b->v.texture[0].y + t * (a->v.texture[0].y - b->v.texture[0].y);
			    newVertex->v.texture[0].w = b->v.texture[0].w + t * (a->v.texture[0].w - b->v.texture[0].w);

			    /* copy the boundary flag if we are entering, set it if we are leaving */
			    if (!reversed && (a->outcode & edge_flags[tour])) {
				/* copy a flags & a edge flag */
				newVertex->v.flagBits = a->v.flagBits;
			    } else if (reversed && (b->outcode & edge_flags[tour])) {
				/* copy a flags & b edge flag */
				newVertex->v.flagBits = (a->v.flagBits & ~__GL_VERTEX_EDGE_FLAG) | (b->v.flagBits & __GL_VERTEX_EDGE_FLAG);
			    } else {
				/* copy a flags & clear edge flag */
				newVertex->v.flagBits = a->v.flagBits & ~__GL_VERTEX_EDGE_FLAG;
			    }

			    __glCalc2DOutcode(gc, newVertex, rect);
			    newVertex->outcode &= ~edge_flags[tour];

			    polygon[next][next_polygon_vertices++] = newVertex;
			}
		    }
		    /* There's a new polygon; swap the buffers */
		    polygon_vertices = next_polygon_vertices;
		    current ^= 1;
		    next ^= 1;
		}

		/* Draw the resulting polygon */
		if (polygon_vertices == 3) {
		    (*renderTriangle2)(gc, 
						 &polygon[current][0]->v, 
						 &polygon[current][1]->v, 
						 &polygon[current][2]->v);
		} else {
		    __GLclipVertex **ivp;
		    __GLvertexICD *p0, *p1, *p2;

		    /* This code lifted from s_pgclip.c - JCB */
		    ivp = &polygon[current][0];
		    p0 = &(*ivp++)->v;
		    p1 = &(*ivp++)->v;
		    p2 = &(*ivp++)->v;

		    for (j = 0; j < (polygon_vertices - 2); j++) {
			GLuint t1, t2;
			if (j == 0) {
			    /*
			    ** Third edge of first sub-triangle is always non-boundary
			    */
			    t1 = p2->flagBits & __GL_VERTEX_EDGE_FLAG;
			    p2->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
			    (*renderTriangle2)(gc, p0, p1, p2);
			    p2->flagBits |= t1;
			} else
			    if (j == (polygon_vertices - 3)) {
				/*
				** First edge of last sub-triangle is always non-boundary
				*/
				t1 = p0->flagBits & __GL_VERTEX_EDGE_FLAG;
				p0->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
				(*renderTriangle2)(gc, p0, p1, p2);
				p0->flagBits |= t1;
			    } else {
				/*
				** Interior sub-triangles have the first and last edge
				** marked non-boundary
				*/
				t1 = p0->flagBits & __GL_VERTEX_EDGE_FLAG;
				t2 = p2->flagBits & __GL_VERTEX_EDGE_FLAG;
				p0->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
				p2->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
				(*renderTriangle2)(gc, p0, p1, p2);
				p0->flagBits |= t1;
				p2->flagBits |= t2;
			    }
			p1 = p2;
			p2 = &(*ivp++)->v;
		    }
		}
	    }
	}
    }

}

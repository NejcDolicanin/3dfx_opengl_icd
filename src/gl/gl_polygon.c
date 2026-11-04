/*
** gl_polygon.c -- s_pgfin.c / s_pgspan.c / s_pgmode.c: the polygon begin / vertex state machines, span shaders
** and polygon-mode renderers.
*/
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include "icdglob.h"
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "icdprotos.h"
#include <string.h>
#include "og.h"

/************************************************************************/
/* s_pgfin.c / s_lnfin.c: the primitive assembly vertex procs */
typedef void (__fastcall *__GLrenderTriangle)(__GLcontext *gc, __GLvertexICD *a,
						  __GLvertexICD *b, __GLvertexICD *c);
typedef void (__fastcall *__GLclipTriangle)(__GLcontext *gc, __GLvertexICD *a,
						 __GLvertexICD *b, __GLvertexICD *c, GLuint orCodes);
#define RENDER_TRIANGLE(gc, a, b, c) (((__GLrenderTriangle)(gc)->procs.renderTriangle)(gc, a, b, c))
#define CLIP_TRIANGLE(gc, a, b, c, o) (((__GLclipTriangle)(gc)->procs.clipTriangle)(gc, a, b, c, o))
#define LOCK_RENDER_BUFFERS(gc) \
    if ((++(gc)->buffers.lockCnt == 1) && (gc)->buffers.lockRenderBuffers) \
	((void (__fastcall *)(__GLcontext *))(gc)->buffers.lockRenderBuffers)(gc)
#define UNLOCK_RENDER_BUFFERS(gc) \
    if ((--(gc)->buffers.lockCnt == 0) && (gc)->buffers.unlockRenderBuffers) \
	((void (__fastcall *)(__GLcontext *))(gc)->buffers.unlockRenderBuffers)(gc)
/************************************************************************/
/* s_pgfin.c */
#define __GL_EDGE_FLAG      0x1000
/* ---- s_pgfin.c: the second vertex of strips and fans, and their material revalidation ------- */
#define __GL_NVBUF  16
typedef void (__fastcall *__GLclipPolygon)(__GLcontext *gc, __GLvertexICD *v0, GLint nv);
#define CLIP_POLYGON(gc, v, n)  (((__GLclipPolygon)(gc)->procs.clipPolygon)(gc, v, n))
/* ---- s_pgfin.c: end of a GL_POLYGON ------------------------------------------------------------ */
typedef void (__fastcall *__GLpolygonProc)(__GLcontext *gc, __GLvertexICD *v, GLint nv);
typedef __GLvertexICD __GLvertex;
#define __GL_POLYGON_MODE_POINT         0
#define __GL_POLYGON_MODE_LINE          1
#define __GL_POLYGON_MODE_FILL          2
#define DO_VALIDATE(gc, v, needs) \
    (*gc->procs.validateVertex[((v)->flagBits >> 14) & 3])(gc, v, needs)
typedef void (__fastcall *__GLrenderLineProc)(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b);
typedef void (__fastcall *__GLrenderPointProc)(__GLcontext *gc, __GLvertexICD *v);
/* ---- s_pgspan.c texture spans (one unit, the 1.1 form: s, t, qw, rhow divided by qw) ---- */
typedef void (__fastcall *__GLfn_texture)(__GLcontext *gc, __GLcolor *color, __GLfloat s,
                                         __GLfloat t, __GLfloat rho);
/* (no outer parentheses: `x += (gc->f)` makes VC6 load the field first, the original loads x) */
#define DSDX(gc)    (gc)->polygon.shader.texIter[8]
#define DTDX(gc)    (gc)->polygon.shader.texIter[9]
#define DQWDX(gc)   (gc)->polygon.shader.texIter[10]
#define DRHOWDX(gc) (gc)->polygon.shader.texIter[11]
/* ---- s_pgspan.c dither spans ---- */
#define __GL_DITHER_BITS            4
#define __GL_DITHER_PRECISION       (1 << __GL_DITHER_BITS)
#define __GL_DITHER_INDEX(x, y)     (((x) & 3) + (((y) & 3) << 2))
#define __GL_MASK_INDEXI(gc, i)     ((i) & (gc)->frontBuffer.redMax)

/* 690940f0 -- __glMatValidateFan: matValidate for triangle fans: revalidate the fan's first vertex and v1 */
void __fastcall __glMatValidateFan(__GLcontext *gc)
{
    GLuint needs = (*gc->vertex.getMaterialNeeds)(gc);
    __GLvertexICD *v;

    v = &gc->vertex.vbuf[0];
    if (needs & ~v->flagBits) {
	(*gc->procs.validateVertex[(v->flagBits >> 14) & 3])(gc, v, needs);
    }
    v = gc->vertex.v1;
    if (needs & ~v->flagBits) {
	(*gc->procs.validateVertex[(v->flagBits >> 14) & 3])(gc, v, needs);
    }
}

/* 690942c0 -- __glOtherTFanVertexLock (s_pgfin.c) */
void __fastcall __glOtherTFanVertexLock(__GLcontext *gc, __GLvertexICD *v0)
{
    __GLvertexICD *v1, *v2;
    GLuint orCodes, andCodes;

    v1 = gc->vertex.v1;
    v2 = &gc->vertex.vbuf[0];

    /* Setup to render this triangle */
    gc->line.notResetStipple = GL_FALSE;
    v0->flagBits |= __GL_VERTEX_EDGE_FLAG;
    gc->vertex.provoking = v0;

    /* Setup for next triangle */
    gc->vertex.v1 = v0;
    gc->vertex.v0 = v1;

    /* Clip check */
    orCodes = (v0->flagBits | v1->flagBits | v2->flagBits) & __GL_CLIP_ALL_MASK;
    if (orCodes) {
	/* Some kind of clipping is needed */
	andCodes = v0->flagBits & v1->flagBits & v2->flagBits & __GL_CLIP_ALL_MASK;
	if (andCodes) {
	    /* Trivially reject the triangle */
	} else {
	    /* Clip the triangle */
	    LOCK_RENDER_BUFFERS(gc);
	    CLIP_TRIANGLE(gc, v0, v2, v1, orCodes);
	    UNLOCK_RENDER_BUFFERS(gc);
	}
    } else {
	/* Render the triangle */
	LOCK_RENDER_BUFFERS(gc);
	RENDER_TRIANGLE(gc, v0, v2, v1);
	UNLOCK_RENDER_BUFFERS(gc);
    }
}

/* 690943d0 -- __glOtherTFanVertex (s_pgfin.c) */
void __fastcall __glOtherTFanVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    __GLvertexICD *v1, *v2;
    GLuint orCodes, andCodes;

    v1 = gc->vertex.v1;
    v2 = &gc->vertex.vbuf[0];

    /* Setup to render this triangle */
    gc->line.notResetStipple = GL_FALSE;
    v0->flagBits |= __GL_VERTEX_EDGE_FLAG;
    gc->vertex.provoking = v0;

    /* Setup for next triangle */
    gc->vertex.v1 = v0;
    gc->vertex.v0 = v1;

    /* Clip check */
    orCodes = (v0->flagBits | v1->flagBits | v2->flagBits) & __GL_CLIP_ALL_MASK;
    if (orCodes) {
	/* Some kind of clipping is needed */
	andCodes = v0->flagBits & v1->flagBits & v2->flagBits & __GL_CLIP_ALL_MASK;
	if (andCodes) {
	    /* Trivially reject the triangle */
	} else {
	    /* Clip the triangle */
	    CLIP_TRIANGLE(gc, v0, v2, v1, orCodes);
	}
    } else {
	/* Render the triangle */
	RENDER_TRIANGLE(gc, v0, v2, v1);
    }
}

/* 690944b0 -- __glSecondTFanVertex */
void __fastcall __glSecondTFanVertex(__GLcontext *gc, __GLvertexICD *v)
{
    v->flagBits |= __GL_VERTEX_EDGE_FLAG;
    gc->vertex.v0 = v + 1;
    gc->vertex.v1 = v;
    gc->procs.vertex = gc->procs.vertexTFan;
    gc->procs.matValidate = (__GLproc)__glMatValidateFan;
}

/* 690944f0 -- __glEvenTStripVertexLock (s_pgfin.c): the even triangle-strip vertex with the render buffers
** locked (SI 1.0: v1 / v2 rotate) */
void __fastcall __glEvenTStripVertexLock(__GLcontext *gc, __GLvertexICD *v0)
{
    __GLvertexICD *v1, *v2;
    GLuint orCodes, andCodes;

    v1 = gc->vertex.v1;
    v2 = gc->vertex.v2;
    gc->line.notResetStipple = GL_FALSE;
    v0->flagBits |= __GL_EDGE_FLAG;
    gc->vertex.provoking = v0;
    orCodes = (v1->flagBits | v2->flagBits | v0->flagBits) & __GL_CLIP_ALL_MASK;
    gc->vertex.v0 = v2;
    gc->vertex.v1 = v0;
    gc->vertex.v2 = v1;
    gc->procs.vertex = gc->procs.vertexOddTStrip;
    if (orCodes) {
        andCodes = v1->flagBits & v2->flagBits & v0->flagBits & __GL_CLIP_ALL_MASK;
        if (andCodes == 0) {
            LOCK_RENDER_BUFFERS(gc);
            CLIP_TRIANGLE(gc, v0, v1, v2, orCodes);
            UNLOCK_RENDER_BUFFERS(gc);
        }
    } else {
        LOCK_RENDER_BUFFERS(gc);
        RENDER_TRIANGLE(gc, v0, v1, v2);
        UNLOCK_RENDER_BUFFERS(gc);
    }
}

/* 69094620 -- __glEvenTStripVertex */
void __fastcall __glEvenTStripVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    __GLvertexICD *v1, *v2;
    GLuint orCodes, andCodes;

    v1 = gc->vertex.v1;
    v2 = gc->vertex.v2;

    /* setup for rendering this triangle */
    gc->line.notResetStipple = GL_FALSE;
    v0->flagBits |= __GL_VERTEX_EDGE_FLAG;
    gc->vertex.provoking = v0;

    /* Clip check & setup for next triangle */
    orCodes = (v0->flagBits | v1->flagBits | v2->flagBits) & __GL_CLIP_ALL_MASK;
    gc->vertex.v0 = v2;
    gc->vertex.v1 = v0;
    gc->vertex.v2 = v1;
    gc->procs.vertex = gc->procs.vertexOddTStrip;

    if (orCodes) {
	/* Some kind of clipping is needed */
	andCodes = v0->flagBits & v1->flagBits & v2->flagBits & __GL_CLIP_ALL_MASK;
	if (andCodes) {
	    /* Trivially reject the triangle */
	} else {
	    /* Clip the triangle (NOTE: v0, v1, v2) */
	    (*(__GLclipTriangle)gc->procs.clipTriangle)(gc, v0, v1, v2, orCodes);
	}
    } else {
	/* Render the triangle (NOTE: v0, v1, v2) */
	(*(__GLrenderTriangle)gc->procs.renderTriangle)(gc, v0, v1, v2);
    }
}

/* 690946b0 -- __glOddTStripVertexLock (s_pgfin.c): the odd triangle-strip vertex with the render buffers locked */
void __fastcall __glOddTStripVertexLock(__GLcontext *gc, __GLvertexICD *v0)
{
    __GLvertexICD *v1, *v2;
    GLuint orCodes, andCodes;

    v1 = gc->vertex.v1;
    v2 = gc->vertex.v2;
    gc->line.notResetStipple = GL_FALSE;
    v0->flagBits |= __GL_EDGE_FLAG;
    gc->vertex.provoking = v0;
    orCodes = (v1->flagBits | v2->flagBits | v0->flagBits) & __GL_CLIP_ALL_MASK;
    gc->vertex.v0 = v2;
    gc->vertex.v1 = v0;
    gc->vertex.v2 = v1;
    gc->procs.vertex = gc->procs.vertexEvenTStrip;
    if (orCodes) {
        andCodes = v1->flagBits & v2->flagBits & v0->flagBits & __GL_CLIP_ALL_MASK;
        if (andCodes == 0) {
            LOCK_RENDER_BUFFERS(gc);
            CLIP_TRIANGLE(gc, v0, v2, v1, orCodes);
            UNLOCK_RENDER_BUFFERS(gc);
        }
    } else {
        LOCK_RENDER_BUFFERS(gc);
        RENDER_TRIANGLE(gc, v0, v2, v1);
        UNLOCK_RENDER_BUFFERS(gc);
    }
}

/* 690947e0 -- __glOddTStripVertex */
void __fastcall __glOddTStripVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    __GLvertexICD *v1, *v2;
    GLuint orCodes, andCodes;

    v1 = gc->vertex.v1;
    v2 = gc->vertex.v2;

    /* setup for rendering this triangle */
    gc->line.notResetStipple = GL_FALSE;
    v0->flagBits |= __GL_VERTEX_EDGE_FLAG;
    gc->vertex.provoking = v0;

    /* Clip check & setup for next triangle */
    orCodes = (v0->flagBits | v1->flagBits | v2->flagBits) & __GL_CLIP_ALL_MASK;
    gc->vertex.v0 = v2;
    gc->vertex.v1 = v0;
    gc->vertex.v2 = v1;
    gc->procs.vertex = gc->procs.vertexEvenTStrip;

    if (orCodes) {
	/* Some kind of clipping is needed */
	andCodes = v0->flagBits & v1->flagBits & v2->flagBits & __GL_CLIP_ALL_MASK;
	if (andCodes) {
	    /* Trivially reject the triangle */
	} else {
	    /* Clip the triangle (NOTE: v0, v2, v1) */
	    (*(__GLclipTriangle)gc->procs.clipTriangle)(gc, v0, v2, v1, orCodes);
	}
    } else {
	/* Render the triangle (NOTE: v0, v2, v1) */
	(*(__GLrenderTriangle)gc->procs.renderTriangle)(gc, v0, v2, v1);
    }
}

/* 690948f0 -- __glSecondTStripVertex */
void __fastcall __glSecondTStripVertex(__GLcontext *gc, __GLvertexICD *v)
{
    v->flagBits |= __GL_VERTEX_EDGE_FLAG;
    gc->vertex.v0 = v + 1;
    gc->vertex.v1 = v;
    gc->procs.vertex = gc->procs.vertexOddTStrip;
    gc->procs.matValidate = (__GLproc)__glMatValidateV1V2;
}

/* 69094930 -- __gl3rdTrianglesVertexLock (s_pgfin.c) */
void __fastcall __gl3rdTrianglesVertexLock(__GLcontext *gc, __GLvertexICD *v0)
{
    __GLvertexICD *vbuf0 = &gc->vertex.vbuf[0];
    __GLvertexICD *vbuf1 = &gc->vertex.vbuf[1];
    GLuint orCodes, andCodes;

    /* Setup for this triangle */
    gc->line.notResetStipple = GL_FALSE;
    v0->flagBits |= gc->state.current.edgeTag;
    gc->vertex.provoking = v0;

    /* Setup for the next triangle */
    gc->vertex.v0 = vbuf0;
    gc->procs.vertex = (__GLproc)FirstTrianglesVertex;

    /* Clip check */
    orCodes = (v0->flagBits | vbuf0->flagBits | vbuf1->flagBits) & __GL_CLIP_ALL_MASK;
    if (orCodes) {
	/* Some kind of clipping is needed */
	andCodes = (v0->flagBits & vbuf0->flagBits & vbuf1->flagBits) & 
	    __GL_CLIP_ALL_MASK;
	if (andCodes) {
	    /* Trivially reject the triangle */
	} else {
	    /* Clip the triangle */
	    LOCK_RENDER_BUFFERS(gc);
	    CLIP_TRIANGLE(gc, v0, vbuf0, vbuf1, orCodes);
	    UNLOCK_RENDER_BUFFERS(gc);
	}
    } else {
	/* Render the triangle */
	LOCK_RENDER_BUFFERS(gc);
	RENDER_TRIANGLE(gc, v0, vbuf0, vbuf1);
	UNLOCK_RENDER_BUFFERS(gc);
    }
}

/* 69094a40 -- __gl3rdTrianglesVertex (the ICD clips through procs.clipTriangle) */
void __fastcall __gl3rdTrianglesVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    __GLvertexICD *vbuf0 = &gc->vertex.vbuf[0];
    __GLvertexICD *vbuf1 = &gc->vertex.vbuf[1];
    GLuint orCodes, andCodes;

    /* Setup for this triangle */
    gc->line.notResetStipple = GL_FALSE;
    v0->flagBits |= gc->state.current.edgeTag;
    gc->vertex.provoking = v0;

    /* Setup for the next triangle */
    gc->vertex.v0 = vbuf0;
    gc->procs.vertex = (__GLproc)FirstTrianglesVertex;

    /* Clip check */
    orCodes = (v0->flagBits | vbuf0->flagBits | vbuf1->flagBits) & __GL_CLIP_ALL_MASK;
    if (orCodes) {
        /* Some kind of clipping is needed */
        andCodes = (v0->flagBits & vbuf0->flagBits & vbuf1->flagBits) &
            __GL_CLIP_ALL_MASK;
        if (andCodes) {
            /* Trivially reject the triangle */
        } else {
            /* Clip the triangle */
            CLIP_TRIANGLE(gc, v0, vbuf0, vbuf1, orCodes);
        }
    } else {
        /* Render the triangle */
        RENDER_TRIANGLE(gc, v0, vbuf0, vbuf1);
    }
}

/* 69094e10 -- __glThirdQStripVertex */
void __fastcall __glThirdQStripVertex(__GLcontext *gc, __GLvertexICD *v)
{
    v->flagBits |= __GL_VERTEX_EDGE_FLAG;
    gc->vertex.v0 = v + 1;
    gc->vertex.v1 = v;
    gc->procs.vertex = gc->procs.vertex4thQStrip;
    gc->procs.matValidate = (__GLproc)__glMatValidateV1V2V3;
}

/* 69094f00 -- __gl4thQuadsVertexLock */
void __fastcall __gl4thQuadsVertexLock(__GLcontext *gc, __GLvertexICD *v0)
{
    __GLvertexICD *va, *vb, *vc;
    GLuint orCodes, andCodes;
    GLuint saveFlag;
    __GLvertexICD *iv[4];

    va = &gc->vertex.vbuf[0];
    vb = &gc->vertex.vbuf[1];
    vc = &gc->vertex.vbuf[2];

    /* Setup for this triangle */
    v0->flagBits |= gc->state.current.edgeTag;
    gc->line.notResetStipple = GL_FALSE;
    gc->vertex.provoking = v0;

    /* Setup for next quad */
    gc->vertex.v0 = va;
    gc->procs.vertex = (__GLproc)FirstQuadsVertex;

    /* Clip Check */
    orCodes = (v0->flagBits | va->flagBits | vb->flagBits | vc->flagBits) &
        __GL_CLIP_ALL_MASK;
    if (orCodes) {
        /* Some kind of clipping is needed */
        andCodes = v0->flagBits & va->flagBits & vb->flagBits & vc->flagBits &
            __GL_CLIP_ALL_MASK;
        if (andCodes) {
            /* Trivially reject the quad */
        } else {
            /* Clip the quad as a polygon */
            iv[0] = va;
            iv[1] = vb;
            iv[2] = vc;
            iv[3] = v0;
            LOCK_RENDER_BUFFERS(gc);
            __glDoPolygonClip(gc, &iv[0], 4, orCodes);
            UNLOCK_RENDER_BUFFERS(gc);
        }
    } else {
        /* Render the quad as two triangles */
        LOCK_RENDER_BUFFERS(gc);
        saveFlag = vb->flagBits & __GL_EDGE_FLAG;
        vb->flagBits &= ~__GL_EDGE_FLAG;
        RENDER_TRIANGLE(gc, v0, va, vb);
        vb->flagBits |= saveFlag;
        v0->flagBits &= ~__GL_EDGE_FLAG;
        RENDER_TRIANGLE(gc, v0, vb, vc);
        UNLOCK_RENDER_BUFFERS(gc);
    }
}

/* 69095250 -- __glPolygonVertexLock (procs +0x25a0 is the polygon clipper) */
void __fastcall __glPolygonVertexLock(__GLcontext *gc, __GLvertexICD *v0)
{
    v0->flagBits |= gc->state.current.edgeTag;
    if (v0 == &gc->vertex.vbuf[__GL_NVBUF - 1]) {
        __GLvertexICD *vFirst = &gc->vertex.vbuf[0];
        __GLvertexICD *vPrev = v0 - 1;
        GLuint vPrevFlag = vPrev->flagBits & __GL_EDGE_FLAG;

        vPrev->flagBits &= ~__GL_EDGE_FLAG;
        LOCK_RENDER_BUFFERS(gc);
        CLIP_POLYGON(gc, vFirst, __GL_NVBUF - 1);
        UNLOCK_RENDER_BUFFERS(gc);
        vPrev->flagBits = vPrevFlag;

        gc->vertex.vbuf[1] = *vPrev;
        gc->vertex.vbuf[2] = *v0;
        gc->vertex.v0 = &gc->vertex.vbuf[3];

        vFirst->flagBits &= ~__GL_EDGE_FLAG;
    } else {
        gc->vertex.v0 = v0 + 1;
    }
}

/* 69095330 -- __glPolygonVertex */
void __fastcall __glPolygonVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    v0->flagBits |= gc->state.current.edgeTag;
    if (v0 == &gc->vertex.vbuf[__GL_NVBUF - 1]) {
        __GLvertexICD *vFirst = &gc->vertex.vbuf[0];
        __GLvertexICD *vPrev = v0 - 1;
        GLuint vPrevFlag = vPrev->flagBits & __GL_EDGE_FLAG;

        vPrev->flagBits &= ~__GL_EDGE_FLAG;
        CLIP_POLYGON(gc, vFirst, __GL_NVBUF - 1);
        vPrev->flagBits = vPrevFlag;

        gc->vertex.vbuf[1] = *vPrev;
        gc->vertex.vbuf[2] = *v0;
        gc->vertex.v0 = &gc->vertex.vbuf[3];

        vFirst->flagBits &= ~__GL_EDGE_FLAG;
    } else {
        gc->vertex.v0 = v0 + 1;
    }
}

/* 690953d0 -- __glEndPolygonLock (s_pgfin.c); the ICD table declares it with two arguments */
void __fastcall __glEndPolygonLock(__GLcontext *gc, GLuint edx_unused)
{
    __GLvertexICD *v0 = gc->vertex.v0;
    __GLvertexICD *vFirst = &gc->vertex.vbuf[0];
    GLint nv = (GLint)(v0 - vFirst);

    if (nv >= 3) {
	/*
	** Decompose polygon remaining in the buffer.  The first vertex's
	** edge tag will have been set properly if this is the tail part
	** of a large polygon that overflowed the internal vertex buffer.
	*/
	LOCK_RENDER_BUFFERS(gc);
	CLIP_POLYGON(gc, vFirst, nv);
	UNLOCK_RENDER_BUFFERS(gc);
    }
    gc->procs.vertex = (__GLproc)__glNop2;
    gc->procs.endPrim = (__GLproc)__glEndPrim;
}

/* 69095460 -- __glEndPolygon: hand the gathered vertices (three or more) to procs +0x25a0
** (sststate.c takes its address with two parameters: the second is unused) */
void __fastcall __glEndPolygon(__GLcontext *gc, GLuint edx_unused)
{
    __GLvertexICD *vbuf = &gc->vertex.vbuf[0];
    GLint nv = gc->vertex.v0 - vbuf;

    if (nv >= 3) {
	(*(__GLpolygonProc)gc->procs.clipPolygon)(gc, vbuf, nv);
    }
    gc->procs.vertex = (__GLproc)__glNop2;
    gc->procs.endPrim = (__GLproc)__glEndPrim;
}

/* 690a1100 -- __glRenderTriangle (s_pgmode.c) with the ICD's mode-flag / needs procs */
void __fastcall __glRenderTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b,
                             __GLvertexICD *c)
{
    GLuint needs, modeFlags, faceNeeds;
    GLint ccw, colorFace, face;
    __GLfloat dxAC, dxBC, dyAC, dyBC, area;
    __GLvertex *pv;

    /* Compute signed area of the triangle */
    dxAC = a->window.x - c->window.x;
    dxBC = b->window.x - c->window.x;
    dyAC = a->window.y - c->window.y;
    dyBC = b->window.y - c->window.y;
    area = dxAC * dyBC - dxBC * dyAC;
    ccw = !(*(int *)&area >> 31);

    /* Figure out if face is culled or not */
    face = gc->polygon.face[ccw];
    if (face == gc->polygon.cullFace) {
        /* Culled */
        return;
    }

    /*
    ** Pick face to use for coloring
    */
    modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    if (modeFlags & __GL_SHADE_TWOSIDED) {
        colorFace = face;
        faceNeeds = (*(&gc->vertex.getFaceNeeds0)[face])(gc);
    } else {
        colorFace = __GL_FRONTFACE;
        faceNeeds = (*gc->vertex.getFaceNeeds0)(gc);
    }

    /*
    ** Choose colors for the vertices.
    */
    needs = (*gc->vertex.getNeeds)(gc);
    pv = gc->vertex.provoking;
    if (modeFlags & __GL_SHADE_SMOOTH_LIGHT) {
        /* Smooth shading */
        a->color = &a->colors[colorFace];
        b->color = &b->colors[colorFace];
        c->color = &c->colors[colorFace];
        needs |= faceNeeds;
    } else {
        GLuint pvneeds;

        pv->color = &pv->colors[colorFace];
        a->color = pv->color;
        b->color = pv->color;
        c->color = pv->color;
        pvneeds = faceNeeds & (__GL_HAS_BOTH | __GL_HAS_LIGHTING);
        if (~pv->flagBits & pvneeds) DO_VALIDATE(gc, pv, pvneeds);
    }

    /* Validate vertices */
    if (~a->flagBits & needs) DO_VALIDATE(gc, a, needs);
    if (~b->flagBits & needs) DO_VALIDATE(gc, b, needs);
    if (~c->flagBits & needs) DO_VALIDATE(gc, c, needs);

    /* Render triangle using the faces polygon mode */
    switch (gc->polygon.mode[face]) {
      case __GL_POLYGON_MODE_FILL:
          if (*(int *)&area << 1) {
              (*gc->procs.fillTriangle)(gc, a, b, c);
          }
          break;
      case __GL_POLYGON_MODE_POINT:
          (*gc->procs.renderTrianglePolygonModePoint)(gc, a, b, c);
          break;
      case __GL_POLYGON_MODE_LINE:
          (*gc->procs.renderTrianglePolygonModeLine)(gc, a, b, c);
          break;
    }

    /* Restore color pointers */
    a->color = &a->colors[__GL_FRONTFACE];
    b->color = &b->colors[__GL_FRONTFACE];
    c->color = &c->colors[__GL_FRONTFACE];
    pv->color = &pv->colors[__GL_FRONTFACE];
}

/* 690a1320 -- s_pgmode.c __glRenderTriangleLineLock */
void __fastcall __glRenderTriangleLineLock(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    LOCK_RENDER_BUFFERS(gc);

    if (a->flagBits & __GL_VERTEX_EDGE_FLAG) (*(__GLrenderLineProc)gc->procs.renderLine)(gc, a, b);
    if (b->flagBits & __GL_VERTEX_EDGE_FLAG) (*(__GLrenderLineProc)gc->procs.renderLine)(gc, b, c);
    if (c->flagBits & __GL_VERTEX_EDGE_FLAG) (*(__GLrenderLineProc)gc->procs.renderLine)(gc, c, a);

    UNLOCK_RENDER_BUFFERS(gc);
}

/* 690a1400 -- __glRenderPointModeTriangle: a triangle in GL_POINT polygon mode: its edge-flagged vertices as
** points, under the
** render-buffer lock */
void __fastcall __glRenderPointModeTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    LOCK_RENDER_BUFFERS(gc);
    if (a->flagBits & 0x1000) (*(__GLrenderPointProc)gc->procs.renderPoint)(gc, a);
    if (b->flagBits & 0x1000) (*(__GLrenderPointProc)gc->procs.renderPoint)(gc, b);
    if (c->flagBits & 0x1000) (*(__GLrenderPointProc)gc->procs.renderPoint)(gc, c);
    UNLOCK_RENDER_BUFFERS(gc);
}

/* 690a14d0 -- s_pgmode.c __glRenderFlatTriangle (mode flags and needs through procs) */
void __fastcall __glRenderFlatTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    GLuint needs, pvneeds, modeFlags, faceNeeds;
    GLint ccw, colorFace, face;
    __GLfloat dxAC, dxBC, dyAC, dyBC, area;
    __GLvertexICD *pv;

    /* Compute signed area of the triangle */
    dxAC = a->window.x - c->window.x;
    dxBC = b->window.x - c->window.x;
    dyAC = a->window.y - c->window.y;
    dyBC = b->window.y - c->window.y;
    area = dxAC * dyBC - dxBC * dyAC;
    if (!(*(int *)&area << 1)) {
	return;
    }
    ccw = !(*(int *)&area & (int)0x80000000);

    /* Figure out if face is culled or not */
    face = gc->polygon.face[ccw];
    if (face == gc->polygon.cullFace) {
	/* Culled */
	return;
    }

    /*
    ** Pick face to use for coloring
    */
    modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    if (modeFlags & __GL_SHADE_TWOSIDED) {
	colorFace = face;
	faceNeeds = (*(&gc->vertex.getFaceNeeds0)[face])(gc);
    } else {
	colorFace = __GL_FRONTFACE;
	faceNeeds = (*gc->vertex.getFaceNeeds0)(gc);
    }

    /*
    ** Choose colors for the vertices.
    */
    needs = (*gc->vertex.getNeeds)(gc);
    pv = gc->vertex.provoking;

    /*
    ** Validate the lighting (and color) information in the provoking
    ** vertex only.  Fill routines always use gc->vertex.provoking->color
    ** to find the color.
    */
    pv->color = &pv->colors[colorFace];
    pvneeds = faceNeeds & (__GL_HAS_BOTH | __GL_HAS_LIGHTING | __GL_HAS_FOG);
    if (~pv->flagBits & pvneeds) DO_VALIDATE(gc, pv, pvneeds);

    /* Validate vertices */
    if (~a->flagBits & needs) DO_VALIDATE(gc, a, needs);
    if (~b->flagBits & needs) DO_VALIDATE(gc, b, needs);
    if (~c->flagBits & needs) DO_VALIDATE(gc, c, needs);

    /* Fill triangle */
    (*gc->procs.fillTriangle)(gc, a, b, c);

    /* Restore color pointers */
    pv->color = &pv->colors[__GL_FRONTFACE];
}

/* ---- s_pgspan.c: __glShadeCISpan (690d0100), __glShadeRGBASpan (690d0140) ---- */
/* 690d0100 -- __glShadeCISpan (s_pgspan.c) */
GLboolean __fastcall __glShadeCISpan(__GLcontext *gc)
{
    __GLcolor *cp;
    __GLfloat r, drdx;
    GLint w;

    w = gc->polygon.shader.length;

    r = gc->polygon.shader.frag.color.r;
    drdx = gc->polygon.shader.colorIter.drdx;
    cp = gc->polygon.shader.colors;
    while (--w >= 0) {
	cp->r = r;
	r += drdx;
	cp++;
    }

    return GL_FALSE;
}

/* 690d0140 -- __glShadeRGBASpan (s_pgspan.c) */
GLboolean __fastcall __glShadeRGBASpan(__GLcontext *gc)
{
    __GLcolor *cp;
    __GLfloat r, g, b, a;
    __GLfloat drdx, dgdx, dbdx, dadx;
    GLint w;

    w = gc->polygon.shader.length;

    r = gc->polygon.shader.frag.color.r;
    g = gc->polygon.shader.frag.color.g;
    b = gc->polygon.shader.frag.color.b;
    a = gc->polygon.shader.frag.color.a;
    drdx = gc->polygon.shader.colorIter.drdx;
    dgdx = gc->polygon.shader.colorIter.dgdx;
    dbdx = gc->polygon.shader.colorIter.dbdx;
    dadx = gc->polygon.shader.colorIter.dadx;
    cp = gc->polygon.shader.colors;
    while (--w >= 0) {
	cp->r = r;
	cp->g = g;
	cp->b = b;
	cp->a = a;
	r += drdx;
	g += dgdx;
	b += dbdx;
	a += dadx;
	cp++;
    }

    return GL_FALSE;
}

/* 690d0270 -- __glTextureSpan (s_pgspan.c) */
GLboolean __fastcall __glTextureSpan(__GLcontext *gc)
{
    __GLcolor *cp;
    __GLcoord strq;
    __GLfloat qInv;
    GLint w;

    strq.x = gc->polygon.shader.frag.texture[0];
    strq.y = gc->polygon.shader.frag.texture[1];
    strq.z = gc->polygon.shader.frag.texture[2];
    strq.w = gc->polygon.shader.frag.texture[3];

    w = gc->polygon.shader.length;
    cp = gc->polygon.shader.colors;

    while (--w >= 0) {
        qInv = __GL_ONE_F / strq.z;
        (*(__GLfn_texture) gc->procs.texture[0])(gc, cp, strq.x * qInv, strq.y * qInv, strq.w * qInv);
        cp++;
        strq.x += DSDX(gc);
        strq.y += DTDX(gc);
        strq.z += DQWDX(gc);
        strq.w += DRHOWDX(gc);
    }

    return GL_FALSE;
}

/* 690d0340 -- __glTextureStippledSpan (s_pgspan.c) */
GLboolean __fastcall __glTextureStippledSpan(__GLcontext *gc)
{
    __GLstippleWord inMask, bit, *sp;
    GLint count;
    __GLcolor *cp;
    __GLfloat s, t, qw, rhow;     /* separate floats, not a __GLcoord: the struct takes other frame slots */
    __GLfloat qInv;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;
    cp = gc->polygon.shader.colors;
    s = gc->polygon.shader.frag.texture[0];
    t = gc->polygon.shader.frag.texture[1];
    qw = gc->polygon.shader.frag.texture[2];
    rhow = gc->polygon.shader.frag.texture[3];

    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp++;
        bit = __GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (inMask & bit) {
                qInv = __GL_ONE_F / qw;
                (*(__GLfn_texture) gc->procs.texture[0])(gc, cp, s * qInv, t * qInv,
                                                        rhow * qInv);
            }
            s += DSDX(gc);
            t += DTDX(gc);
            qw += DQWDX(gc);
            rhow += DRHOWDX(gc);
            cp++;
            bit >>= 1;
        }
    }

    return GL_FALSE;
}

/* 690d0460 -- __glDitherCISpan (s_pgspan.c) */
GLboolean __fastcall __glDitherCISpan(__GLcontext *gc)
{
    __GLcolor *cp;
    GLint ix, x, y, r, maxR, frac;
    GLint w;

    w = gc->polygon.shader.length;

    x = gc->polygon.shader.frag.x;
    y = gc->polygon.shader.frag.y;
    maxR = (1 << gc->modes.indexBits) - 1;
    cp = gc->polygon.shader.colors;
    while (--w >= 0) {
        ix = __GL_DITHER_INDEX(x, y);
        frac = __glDitherTable[ix];
        r = ((GLint) (cp->r * __GL_DITHER_PRECISION + __glHalf) + frac)
            >> __GL_DITHER_BITS;
        if (r > maxR) r = maxR;
        cp->r = r;
        cp++;
        x++;
    }

    return GL_FALSE;
}

/* 690d0510 -- __glDitherCIStippledSpan (s_pgspan.c) */
GLboolean __fastcall __glDitherCIStippledSpan(__GLcontext *gc)
{
    __GLstippleWord bit, inMask, *sp;
    __GLcolor *cp;
    GLint count, ix, x, y, r, maxR, frac;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    x = gc->polygon.shader.frag.x;
    y = gc->polygon.shader.frag.y;
    maxR = (1 << gc->modes.indexBits) - 1;
    cp = gc->polygon.shader.colors;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp++;
        bit = __GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (inMask & bit) {
                ix = __GL_DITHER_INDEX(x, y);
                frac = __glDitherTable[ix];
                r = ((GLint) (cp->r * __GL_DITHER_PRECISION + __glHalf) + frac)
                    >> __GL_DITHER_BITS;
                if (r > maxR) r = maxR;
                cp->r = r;
            }
            cp++;
            x++;
            bit >>= 1;
        }
    }

    return GL_FALSE;
}

/* 690d0610 -- __glDitherRGBASpan (s_pgspan.c) */
GLboolean __fastcall __glDitherRGBASpan(__GLcontext *gc)
{
    __GLcolor *cp;
    GLint ix, x, y, r, g, b, a, frac;
    GLint maxR, maxG, maxB, maxA;
    GLint w;

    w = gc->polygon.shader.length;

    x = gc->polygon.shader.frag.x;
    y = gc->polygon.shader.frag.y;
    maxR = gc->frontBuffer.iRedScale;
    maxG = gc->frontBuffer.iGreenScale;
    maxB = gc->frontBuffer.iBlueScale;
    maxA = gc->frontBuffer.iAlphaScale;
    cp = gc->polygon.shader.colors;
    while (--w >= 0) {
        ix = __GL_DITHER_INDEX(x, y);
        /*
        ** Convert the color into integers, keeping 4 bits of fractional
        ** precision (and rounding up).
        */
        frac = __glDitherTable[ix];
        r = ((GLint) (cp->r * __GL_DITHER_PRECISION + __glHalf) + frac)
            >> __GL_DITHER_BITS;
        if (r > maxR) r = maxR;
        cp->r = r;

        g = ((GLint) (cp->g * __GL_DITHER_PRECISION + __glHalf) + frac)
            >> __GL_DITHER_BITS;
        if (g > maxG) g = maxG;
        cp->g = g;

        b = ((GLint) (cp->b * __GL_DITHER_PRECISION + __glHalf) + frac)
            >> __GL_DITHER_BITS;
        if (b > maxB) b = maxB;
        cp->b = b;

        a = ((GLint) (cp->a * __GL_DITHER_PRECISION + __glHalf) + frac)
            >> __GL_DITHER_BITS;
        if (a > maxA) a = maxA;
        cp->a = a;

        cp++;
        x++;
    }

    return GL_FALSE;
}

/* 690d0770 -- __glDitherRGBAStippledSpan (s_pgspan.c) */
GLboolean __fastcall __glDitherRGBAStippledSpan(__GLcontext *gc)
{
    __GLstippleWord bit, inMask, *sp;
    __GLcolor *cp;
    GLint count, ix, x, y, r, g, b, a, frac;
    GLint maxR, maxG, maxB, maxA;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    x = gc->polygon.shader.frag.x;
    y = gc->polygon.shader.frag.y;
    maxR = gc->frontBuffer.iRedScale;
    maxG = gc->frontBuffer.iGreenScale;
    maxB = gc->frontBuffer.iBlueScale;
    maxA = gc->frontBuffer.iAlphaScale;
    cp = gc->polygon.shader.colors;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp++;
        bit = __GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (inMask & bit) {
                ix = __GL_DITHER_INDEX(x, y);
                /*
                ** Convert the color into integers, keeping 4 bits of
                ** fractional precision (and rounding up).
                */
                frac = __glDitherTable[ix];
                r = ((GLint) (cp->r * __GL_DITHER_PRECISION + __glHalf) + frac)
                    >> __GL_DITHER_BITS;
                if (r > maxR) r = maxR;
                cp->r = r;

                g = ((GLint) (cp->g * __GL_DITHER_PRECISION + __glHalf) + frac)
                    >> __GL_DITHER_BITS;
                if (g > maxG) g = maxG;
                cp->g = g;

                b = ((GLint) (cp->b * __GL_DITHER_PRECISION + __glHalf) + frac)
                    >> __GL_DITHER_BITS;
                if (b > maxB) b = maxB;
                cp->b = b;

                a = ((GLint) (cp->a * __GL_DITHER_PRECISION + __glHalf) + frac)
                    >> __GL_DITHER_BITS;
                if (a > maxA) a = maxA;
                cp->a = a;
            }
            cp++;
            x++;
            bit >>= 1;
        }
    }

    return GL_FALSE;
}

/* 690d0f60 -- __glMaskRGBASpan (s_pgspan.c): the RGBA color-mask span proc: masked-off channels take the frame
** buffer's value.
** As the original: the frame buffer pointer is not stepped (every pixel reads fbcolors[0]).
*/
GLboolean __fastcall __glMaskRGBASpan(__GLcontext *gc)
{
    GLboolean rMask = gc->state.raster.rMask;
    GLboolean gMask = gc->state.raster.gMask;
    GLboolean bMask = gc->state.raster.bMask;
    GLboolean aMask = gc->state.raster.aMask;
    __GLcolor *fbcp = gc->polygon.shader.fbcolors;
    __GLcolor *cp = gc->polygon.shader.colors;
    GLint w = gc->polygon.shader.length;

    while (--w >= 0) {
        cp->r = rMask ? cp->r : fbcp->r;
        cp->g = gMask ? cp->g : fbcp->g;
        cp->b = bMask ? cp->b : fbcp->b;
        cp->a = aMask ? cp->a : fbcp->a;
        cp++;
    }
    return GL_FALSE;
}

/* 690d1000 -- __glMaskCISpan (s_pgspan.c).  As the original: the frame buffer pointer is not
** stepped (every pixel reads fbcolors[0]).
*/
GLboolean __fastcall __glMaskCISpan(__GLcontext *gc)
{
    __GLcolor *cp;
    __GLcolor *fcp;
    GLuint keepMask, changeMask, r, fr;
    GLint w;

    w = gc->polygon.shader.length;
    changeMask = gc->state.raster.writeMask;
    keepMask = __GL_MASK_INDEXI(gc, ~changeMask);
    cp = gc->polygon.shader.colors;
    fcp = gc->polygon.shader.fbcolors;
    while (--w >= 0) {
        r = (GLuint) cp->r;
        fr = (GLuint) fcp->r;
        cp->r = (r & changeMask) | (fr & keepMask);
        cp++;
    }

    return GL_FALSE;
}

/* 690d1090 -- __glIndexTestSpan (3dfx S_PGSPAN.C): the color-index alpha test span (s_pgspan.c
** __glAlphaTestSpan, CI flavour): the
** table index is the integer part of the fragment's index (colors[].r), no conversion or clamp.
*/
GLboolean __fastcall __glIndexTestSpan(__GLcontext *gc)
{
    GLubyte *atft;
    GLint failed, count;
    __GLstippleWord bit, outMask, *osp;
    __GLcolor *cp;
    GLint w;

    atft = gc->frontBuffer.indexTestFuncTable;
    osp = gc->polygon.shader.stipplePat;
    cp = gc->polygon.shader.colors;
    w = gc->polygon.shader.length;
    failed = 0;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        outMask = ~0;
        bit = __GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (!atft[(GLint) cp->r]) {
                outMask &= ~bit;
                failed++;
            }
            cp++;
            bit >>= 1;
        }
        *osp++ = outMask;
    }

    if (failed == 0) {
        return GL_FALSE;
    } else {
        if (failed != gc->polygon.shader.length) {
            return GL_TRUE;
        }
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

/* 690d1160 -- __glIndexTestStippledSpan (3dfx S_PGSPAN.C): the color-index alpha test stippled span
** (s_pgspan.c __glAlphaTestStippledSpan, CI
** flavour: the table index is the integer part of colors[].r).
*/
GLboolean __fastcall __glIndexTestStippledSpan(__GLcontext *gc)
{
    GLubyte *atft;
    GLint count, failed;
    __GLstippleWord bit, inMask, outMask, *isp;
    __GLcolor *cp;
    GLint w;

    atft = gc->frontBuffer.indexTestFuncTable;
    isp = gc->polygon.shader.stipplePat;
    w = gc->polygon.shader.length;
    cp = gc->polygon.shader.colors;
    failed = 0;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *isp;
        outMask = ~0;
        bit = __GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (inMask & bit) {
                if (!atft[(GLint) cp->r]) {
                    outMask &= ~bit;
                    failed++;
                }
            } else failed++;
            cp++;
            bit >>= 1;
        }
        *isp++ = outMask & inMask;
    }

    if (failed != gc->polygon.shader.length) {
        return GL_FALSE;
    }
    return GL_TRUE;
}

/* 69094b50 -- __gl4thQStripVertexLock (s_pgfin.c FourthQStripVertex): 69094d00 with the render buffers locked
** around the drawing */
void __fastcall __gl4thQStripVertexLock(__GLcontext *gc, __GLvertexICD *v3)
{
    __GLvertexICD *v0, *v1, *v2;
    GLuint orCodes, andCodes;
    __GLvertexICD *iv[4];

    v0 = gc->vertex.v1;
    v1 = gc->vertex.v2;
    v2 = gc->vertex.v3;
    gc->line.notResetStipple = GL_FALSE;
    v3->flagBits |= __GL_EDGE_FLAG;
    gc->vertex.provoking = v3;
    gc->vertex.v0 = v2;
    gc->vertex.v2 = v3;
    gc->vertex.v3 = v0;
    gc->procs.vertex = (__GLproc)__glThirdQStripVertex;
    gc->procs.matValidate = (__GLproc)__glMatValidateV2V3;

    orCodes = (v0->flagBits | v1->flagBits | v2->flagBits | v3->flagBits) & __GL_CLIP_ALL_MASK;
    if (orCodes) {
	andCodes = (v0->flagBits & v1->flagBits & v2->flagBits & v3->flagBits) & __GL_CLIP_ALL_MASK;
	if (andCodes == 0) {
	    iv[0] = v2;
	    iv[1] = v1;
	    iv[2] = v3;
	    iv[3] = v0;
	    LOCK_RENDER_BUFFERS(gc);
	    __glDoPolygonClip(gc, iv, 4, orCodes);
	    UNLOCK_RENDER_BUFFERS(gc);
	}
    } else {
	LOCK_RENDER_BUFFERS(gc);
	v1->flagBits &= ~__GL_EDGE_FLAG;
	RENDER_TRIANGLE(gc, v0, v2, v1);
	v0->flagBits &= ~__GL_EDGE_FLAG;
	v1->flagBits |= __GL_EDGE_FLAG;
	RENDER_TRIANGLE(gc, v0, v1, v3);
	v0->flagBits |= __GL_EDGE_FLAG;
	UNLOCK_RENDER_BUFFERS(gc);
    }
}

/* 69094d00 -- __gl4thQStripVertex (s_pgfin.c FourthQStripVertex): the fourth vertex of a quad strip pair; back
** to ThirdQStripVertex */
void __fastcall __gl4thQStripVertex(__GLcontext *gc, __GLvertexICD *v3)
{
    __GLvertexICD *v0, *v1, *v2;
    GLuint orCodes, andCodes;
    __GLvertexICD *iv[4];

    v0 = gc->vertex.v1;
    v1 = gc->vertex.v2;
    v2 = gc->vertex.v3;
    gc->line.notResetStipple = GL_FALSE;
    v3->flagBits |= __GL_EDGE_FLAG;
    gc->vertex.provoking = v3;
    gc->vertex.v0 = v2;
    gc->vertex.v2 = v3;
    gc->vertex.v3 = v0;
    gc->procs.vertex = (__GLproc)__glThirdQStripVertex;
    gc->procs.matValidate = (__GLproc)__glMatValidateV2V3;

    orCodes = (v0->flagBits | v1->flagBits | v2->flagBits | v3->flagBits) & __GL_CLIP_ALL_MASK;
    if (orCodes) {
	andCodes = (v0->flagBits & v1->flagBits & v2->flagBits & v3->flagBits) & __GL_CLIP_ALL_MASK;
	if (andCodes == 0) {
	    iv[0] = v2;
	    iv[1] = v1;
	    iv[2] = v3;
	    iv[3] = v0;
	    __glDoPolygonClip(gc, iv, 4, orCodes);
	}
    } else {
	v1->flagBits &= ~__GL_EDGE_FLAG;
	RENDER_TRIANGLE(gc, v0, v2, v1);
	v0->flagBits &= ~__GL_EDGE_FLAG;
	v1->flagBits |= __GL_EDGE_FLAG;
	RENDER_TRIANGLE(gc, v0, v1, v3);
	v0->flagBits |= __GL_EDGE_FLAG;
    }
}

/* 69095090 -- __gl4thQuadsVertex (s_pgfin.c FourthQuadsVertex): the fourth vertex of a GL_QUADS quad
** (vbuf[0..3]); back to FirstQuadsVertex */
void __fastcall __gl4thQuadsVertex(__GLcontext *gc, __GLvertexICD *v3)
{
    __GLvertexICD *v0, *v1, *v2;
    GLuint orCodes, andCodes, edge;
    __GLvertexICD *iv[4];

    v0 = &gc->vertex.vbuf[0];
    v1 = &gc->vertex.vbuf[1];
    v2 = &gc->vertex.vbuf[2];
    v3->flagBits |= gc->state.current.edgeTag;
    gc->line.notResetStipple = GL_FALSE;
    gc->vertex.provoking = v3;
    gc->vertex.v0 = v0;
    gc->procs.vertex = (__GLproc)FirstQuadsVertex;

    orCodes = (v0->flagBits | v1->flagBits | v2->flagBits | v3->flagBits) & __GL_CLIP_ALL_MASK;
    if (orCodes) {
	andCodes = (v0->flagBits & v1->flagBits & v2->flagBits & v3->flagBits) & __GL_CLIP_ALL_MASK;
	if (andCodes == 0) {
	    iv[0] = v0;
	    iv[1] = v1;
	    iv[2] = v2;
	    iv[3] = v3;
	    __glDoPolygonClip(gc, iv, 4, orCodes);
	}
    } else {
	edge = v1->flagBits & __GL_EDGE_FLAG;
	v1->flagBits &= ~__GL_EDGE_FLAG;
	RENDER_TRIANGLE(gc, v3, v0, v1);
	v1->flagBits |= edge;
	v3->flagBits &= ~__GL_EDGE_FLAG;
	RENDER_TRIANGLE(gc, v3, v1, v2);
    }
}

/* 690a1630 -- __glRenderSmoothTriangle (s_pgdraw.c): render a triangle with culling and two-sided color
** selection; the ICD's lazy-validation form: needs are fetched from the procs and each vertex is
** validated before the fill */
void __fastcall __glRenderSmoothTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    __GLfloat area;
    GLuint face, colorFace, modeFlags, needs;

    area = (a->window.x - c->window.x) * (b->window.y - c->window.y) -
           (b->window.x - c->window.x) * (a->window.y - c->window.y);
    if ((*(GLuint *)&area & 0x7fffffff) == 0) return;
    face = gc->polygon.face[!(*(GLint *)&area & (GLint)0x80000000)];
    if (face == gc->polygon.cullFace) return;
    modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    needs = (*gc->vertex.getNeeds)(gc);
    if (modeFlags & __GL_SHADE_TWOSIDED) {
        colorFace = face;
        needs |= (*(&gc->vertex.getFaceNeeds0)[colorFace])(gc);
    } else {
        colorFace = 0;
        needs |= (*gc->vertex.getFaceNeeds0)(gc);
    }
    a->color = &a->colors[colorFace];
    b->color = &b->colors[colorFace];
    c->color = &c->colors[colorFace];
    if (needs & ~a->flagBits) {
        (*gc->procs.validateVertex[(a->flagBits >> 14) & 3])(gc, a, needs);
    }
    if (needs & ~b->flagBits) {
        (*gc->procs.validateVertex[(b->flagBits >> 14) & 3])(gc, b, needs);
    }
    if (needs & ~c->flagBits) {
        (*gc->procs.validateVertex[(c->flagBits >> 14) & 3])(gc, c, needs);
    }
    (*gc->procs.fillTriangle)(gc, a, b, c);
    a->color = &a->colors[0];
    b->color = &b->colors[0];
    c->color = &c->colors[0];
}

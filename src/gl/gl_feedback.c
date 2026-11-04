/*
** gl_feedback.c -- s_feed.c / s_select.c / s_names.c: feedback and selection.
*/
#include <stdlib.h>
#include <string.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "og.h"
#include "icdprotos.h"
#include <math.h>
#include "icdglob.h"

/* s_feed.c Feedback */
#define __GL_SHADE_UVSCALED_TEXTURE 0x1000000
typedef GLuint (__fastcall *__GLgetNeedsProc)(__GLcontext *gc);
#define DO_VALIDATE(gc, v, needs) \
    (*gc->procs.validateVertex[((v)->flagBits >> 14) & 3])(gc, v, needs)
typedef __GLvertexICD __GLvertex;
#define __GL_POLYGON_MODE_POINT         0
#define __GL_POLYGON_MODE_LINE          1
#define __GL_POLYGON_MODE_FILL          2
/* ---- s_names.c ---- */
/* 690f5a50 -- the empty data structure for display lists; 690f5a68 -- the data types' handling, indexed by
** __GLnamesArrayType (read only) */
extern void __fastcall __glNop(void);
extern void __fastcall __glNop_2(void);
extern void __fastcall __glDisposeDlist(__GLcontext *gc, GLint *obj);
extern void __fastcall __glDisposeTexObj(__GLcontext *gc, GLint *obj);
static __GLdlistICD emptyDlist = {
    1,                                  /* refcount */
    (void (__fastcall *)(__GLcontext *, __GLdlistICD *))__glNop,      /* free */
    (void (__fastcall *)(__GLcontext *, __GLdlistICD *))__glNop_2,    /* execute */
    NULL,                               /* segment */
    {0,0,0,0},                          /* local[0] - local[3] */
};
static __GLnamesArrayTypeInfo namesTypeInfo[] = {
    {
        __GL_NAMES_DLIST,               /* type */
        &emptyDlist,                    /* ptr to empty data struct */
        sizeof(__GLdlistICD),           /* dataSize */
        (void (*)(__GLcontext *, void *))__glDisposeDlist,     /* free callback */
        NULL,                           /* alloc callback */
    },
    {
        __GL_NAMES_TEXOBJ,              /* type */
        NULL,                           /* ptr to empty data struct */
        0xd4,                           /* dataSize: sizeof(__GLtexture) of the software path */
        (void (*)(__GLcontext *, void *))__glDisposeTexObj,    /* free callback */
        NULL,                           /* alloc callback */
    },
};

/* 690c0e60 -- __glSelectHit (s_select.c).  The ICD truncates z with __ftol when the depth buffer is
** 32 bits deep (no __GL_CONVERT_Z_TO_UINT) and divides by the unsigned depth scale otherwise.
** EDX is a dummy (the callers declare it so: @12).
*/
void __fastcall __glSelectHit(__GLcontext *gc, __GLfloat z)
{
    GLuint *src;
    GLuint *dest = gc->select.result;
    GLuint *end = gc->select.resultBase + gc->select.resultLength;
    GLuint iz;

    if (gc->select.overFlowed) {
        return;
    }
    if (gc->depthBuffer.buf.wglBuf->depth == 32) {
        iz = (GLuint)z;
    } else {
        iz = (GLuint)(z * gc->constants.val4294965000 / gc->depthBuffer.scale);
    }

    if (!gc->select.hit) {
        gc->select.hit = GL_TRUE;
        if (dest == end) {
          overflow:
            gc->select.overFlowed = GL_TRUE;
            gc->select.result = end;
            return;
        }
        *dest++ = (GLuint)(gc->select.sp - gc->select.stack);
        gc->select.hits++;
        if (dest == end) goto overflow;
        gc->select.z = dest;
        *dest++ = iz;
        if (dest == end) goto overflow;
        *dest++ = iz;
        for (src = gc->select.stack; src < gc->select.sp; src++) {
            if (dest == end) {
                goto overflow;
            }
            *dest++ = *src;
        }
        gc->select.result = dest;
    } else {
        if (iz < gc->select.z[0]) {
            gc->select.z[0] = iz;
        }
        if (iz > gc->select.z[1]) {
            gc->select.z[1] = iz;
        }
    }
}

/************************************************************************/
/* s_select.c / s_feed.c */

/* 690c1050 -- __glSelectLine */
void __fastcall __glSelectLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b)
{
    __glSelectHit(gc, a->window.z);
    __glSelectHit(gc, b->window.z);
}

/* 690c1070 -- __glSelectPoint */
void __fastcall __glSelectPoint(__GLcontext *gc, __GLvertexICD *v)
{
    __glSelectHit(gc, v->window.z);
}

/* 690c1220 -- Feedback (s_feed.c): write one vertex to the feedback buffer per gc->feedback.type: window x / y
** (+ z * oneOverDepthScale for 3D types, + clip.w for GL_4D_COLOR_TEXTURE), the colour (RGBA divided by the
** colour scales, or the index) and unit 0's texture coordinates with the projection (times clip.w) and
** UV scaling (by level 0's width2f / height2f) undone; tx/ty/tw are read before a vertex lacking
** __GL_HAS_TEXTURE is validated for it */
void __fastcall Feedback(__GLcontext *gc, __GLvertexICD *v)
{
    GLenum type = gc->feedback.type;

    switch (type) {
      case GL_2D:
	__glFeedbackTag(gc, __glReturnWindowX(gc, v->window.x));
	__glFeedbackTag(gc, (GLfloat)__glReturnWindowY(gc, v->window.y));
	break;
      case GL_3D:
      case GL_3D_COLOR:
      case GL_3D_COLOR_TEXTURE:
	__glFeedbackTag(gc, __glReturnWindowX(gc, v->window.x));
	__glFeedbackTag(gc, (GLfloat)__glReturnWindowY(gc, v->window.y));
	__glFeedbackTag(gc, v->window.z * gc->constants.oneOverDepthScale);
	break;
      case GL_4D_COLOR_TEXTURE:
	__glFeedbackTag(gc, __glReturnWindowX(gc, v->window.x));
	__glFeedbackTag(gc, (GLfloat)__glReturnWindowY(gc, v->window.y));
	__glFeedbackTag(gc, v->window.z * gc->constants.oneOverDepthScale);
	__glFeedbackTag(gc, v->clip.w);
	break;
    }
    switch (type) {
      case GL_3D_COLOR:
      case GL_3D_COLOR_TEXTURE:
      case GL_4D_COLOR_TEXTURE:
	{
	    __GLcolor *c = v->color;
	    if (gc->modes.rgbMode) {
		__glFeedbackTag(gc, c->r * gc->constants.oneOverRedScale);
		__glFeedbackTag(gc, c->g * gc->constants.oneOverGreenScale);
		__glFeedbackTag(gc, c->b * gc->constants.oneOverBlueScale);
		__glFeedbackTag(gc, c->a * gc->constants.oneOverAlphaScale);
	    } else {
		__glFeedbackTag(gc, c->r);
	    }
	}
	break;
      case GL_2D:
      case GL_3D:
        break;
    }
    switch (type) {
      case GL_3D_COLOR_TEXTURE:
      case GL_4D_COLOR_TEXTURE:
	{
	    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
	    __GLfloat tx = v->texture[0].x;
	    __GLfloat ty = v->texture[0].y;
	    __GLfloat tw = v->texture[0].w;

	    if ((v->flagBits & __GL_HAS_TEXTURE) == 0) {
		DO_VALIDATE(gc, v, (*(__GLgetNeedsProc)gc->vertex.getNeeds)(gc) | __GL_HAS_TEXTURE);
	    }

	    /* Undo projection */
	    if (modeFlags & __GL_SHADE_PROJSCALED_TEXTURE) {
		tx *= v->clip.w;
		ty *= v->clip.w;
		tw *= v->clip.w;
	    }

	    /* Undo scaling */
	    if (modeFlags & __GL_SHADE_UVSCALED_TEXTURE) {
		__GLmipMapLevelICD *lp = __glCurrentTexture(gc, 0)->level[0];

		tx /= lp->width2f;
		ty /= lp->height2f;
	    }

	    __glFeedbackTag(gc, tx);
	    __glFeedbackTag(gc, ty);
	    __glFeedbackTag(gc, v->texture[0].z);
	    __glFeedbackTag(gc, tw);
	}
	break;
      case GL_2D:
      case GL_3D:
      case GL_3D_COLOR:
	break;
    }
}

/* 690c1460 -- __glFeedbackDrawPixels */
void __fastcall __glFeedbackDrawPixels(__GLcontext *gc, __GLvertexICD *vx)
{
    __glFeedbackTag(gc, GL_DRAW_PIXEL_TOKEN);
    Feedback(gc, vx);
}

/* 690c14c0 -- __glFeedbackLine */
void __fastcall __glFeedbackLine(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1)
{
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    __GLcolor *oldColor = v0->color;

    if (!(modeFlags & __GL_SHADE_SMOOTH_LIGHT)) {
	v0->color = v1->color;
    }

    if (gc->line.notResetStipple) {
	__glFeedbackTag(gc, GL_LINE_TOKEN);
    } else {
	gc->line.notResetStipple = GL_TRUE;
	__glFeedbackTag(gc, GL_LINE_RESET_TOKEN);
    }
    Feedback(gc, v0);
    Feedback(gc, v1);
    v0->color = oldColor;
}

/* 690c1530 -- __glFeedbackTriangle (s_feed.c); the ICD asks the shader / vertex machine for the
** mode flags and needs through procs */
void __fastcall __glFeedbackTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    __GLfloat dxAC, dxBC, dyAC, dyBC, area;
    GLboolean ccw;
    GLint face;
    GLuint needs, modeFlags;

    /* Compute signed area of the triangle */
    dxAC = a->window.x - c->window.x;
    dxBC = b->window.x - c->window.x;
    dyAC = a->window.y - c->window.y;
    dyBC = b->window.y - c->window.y;
    area = dxAC * dyBC - dxBC * dyAC;
    ccw = area >= __GL_ZERO_F;

    face = gc->polygon.face[ccw];
    if (face == gc->polygon.cullFace) {
        /* Culled */
        return;
    }

    /*
    ** Pick face to use for coloring
    */
    modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    needs = (*gc->vertex.getNeeds)(gc);
    if (gc->state.light.shadingModel == GL_FLAT) {
        __GLvertex *pv = gc->vertex.provoking;
        GLuint pvneeds;
        GLuint faceNeeds;
        GLint colorFace;

        if (modeFlags & __GL_SHADE_TWOSIDED) {
            colorFace = face;
            faceNeeds = (*(&gc->vertex.getFaceNeeds0)[face])(gc);
        } else {
            colorFace = __GL_FRONTFACE;
            faceNeeds = (*gc->vertex.getFaceNeeds0)(gc);
        }

        pv->color = &pv->colors[colorFace];
        a->color = pv->color;
        b->color = pv->color;
        c->color = pv->color;
        pvneeds = faceNeeds & (__GL_HAS_BOTH | __GL_HAS_LIGHTING);
        if (~pv->flagBits & pvneeds) {
            DO_VALIDATE(gc, pv, pvneeds);
        }
    } else {
        GLint colorFace;

        if (modeFlags & __GL_SHADE_TWOSIDED) {
            colorFace = face;
            needs |= (*(&gc->vertex.getFaceNeeds0)[face])(gc);
        } else {
            colorFace = __GL_FRONTFACE;
            needs |= (*gc->vertex.getFaceNeeds0)(gc);
        }

        a->color = &a->colors[colorFace];
        b->color = &b->colors[colorFace];
        c->color = &c->colors[colorFace];
    }
    if (~a->flagBits & needs) DO_VALIDATE(gc, a, needs);
    if (~b->flagBits & needs) DO_VALIDATE(gc, b, needs);
    if (~c->flagBits & needs) DO_VALIDATE(gc, c, needs);

    /* Deal with polygon face modes */
    switch (gc->polygon.mode[face]) {
      case __GL_POLYGON_MODE_POINT:
        if (a->flagBits & __GL_VERTEX_EDGE_FLAG) {
            __glFeedbackTag(gc, GL_POINT_TOKEN);
            Feedback(gc, a);
        }
        if (b->flagBits & __GL_VERTEX_EDGE_FLAG) {
            __glFeedbackTag(gc, GL_POINT_TOKEN);
            Feedback(gc, b);
        }
        if (c->flagBits & __GL_VERTEX_EDGE_FLAG) {
            __glFeedbackTag(gc, GL_POINT_TOKEN);
            Feedback(gc, c);
        }
        break;
      case __GL_POLYGON_MODE_LINE:
        if (a->flagBits & __GL_VERTEX_EDGE_FLAG) {
            if (!gc->line.notResetStipple) {
                gc->line.notResetStipple = GL_TRUE;
                __glFeedbackTag(gc, GL_LINE_RESET_TOKEN);
            } else {
                __glFeedbackTag(gc, GL_LINE_TOKEN);
            }
            Feedback(gc, a);
            Feedback(gc, b);
        }
        if (b->flagBits & __GL_VERTEX_EDGE_FLAG) {
            if (!gc->line.notResetStipple) {
                gc->line.notResetStipple = GL_TRUE;
                __glFeedbackTag(gc, GL_LINE_RESET_TOKEN);
            } else {
                __glFeedbackTag(gc, GL_LINE_TOKEN);
            }
            Feedback(gc, b);
            Feedback(gc, c);
        }
        if (c->flagBits & __GL_VERTEX_EDGE_FLAG) {
            if (!gc->line.notResetStipple) {
                gc->line.notResetStipple = GL_TRUE;
                __glFeedbackTag(gc, GL_LINE_RESET_TOKEN);
            } else {
                __glFeedbackTag(gc, GL_LINE_TOKEN);
            }
            Feedback(gc, c);
            Feedback(gc, a);
        }
        break;
      case __GL_POLYGON_MODE_FILL:
        __glFeedbackTag(gc, GL_POLYGON_TOKEN);
        __glFeedbackTag(gc, 3);
        Feedback(gc, a);
        Feedback(gc, b);
        Feedback(gc, c);
        break;
    }

    /* Restore color pointers */
    a->color = &a->colors[__GL_FRONTFACE];
    b->color = &b->colors[__GL_FRONTFACE];
    c->color = &c->colors[__GL_FRONTFACE];
    if (gc->state.light.shadingModel == GL_FLAT) {
        __GLvertex *pv = gc->vertex.provoking;

        pv->color = &pv->colors[__GL_FRONTFACE];
    }
}

/* 690e36e0 -- __glNamesNewArray (s_names.c) */
__GLnamesArray *__fastcall __glNamesNewArray(__GLcontext *gc, __GLnamesArrayType type)
{
    __GLnamesArray *array;
    int i;

    array = (__GLnamesArray *)
            __GL_MALLOC(gc, sizeof(__GLnamesArray));
    if (array == NULL) {
        __glSetError(GL_OUT_OF_MEMORY);
        return NULL;
    }
    array->refcount = 1;
    array->tree = NULL;
    array->depth = 0;
    array->dataInfo = &(namesTypeInfo[type]);
    /*
    ** Pre-allocate a few leaves and branches for paranoid OUT_OF_MEMORY
    ** reasons.
    */
    array->nbranches = __GL_DL_EXTRA_BRANCHES;
    array->nleaves = __GL_DL_EXTRA_LEAVES;
    for (i = 0; i < __GL_DL_EXTRA_BRANCHES; i++) {
        array->branches[i] = (__GLnamesBranch*)
                __GL_MALLOC(gc, sizeof(__GLnamesBranch));
        if (array->branches[i] == NULL) {
            array->nbranches = i;
            break;
        }
    }
    for (i = 0; i < __GL_DL_EXTRA_LEAVES; i++) {
        array->leaves[i] = (__GLnamesLeaf*)
                __GL_MALLOC(gc, sizeof(__GLnamesLeaf));
        if (array->leaves[i] == NULL) {
            array->nleaves = i;
            break;
        }
    }

    return array;
}

/* 690e4dc0 -- s_names.c __glNamesDeleteRange */
void __fastcall __glNamesDeleteRange(__GLcontext *gc, __GLnamesArray *array, 
			  GLuint name, GLsizei range)
{
    __GLnamesLeaf *leaf;
    /*LINTED nextleaf ok; lint doesn't understand for loops*/
    __GLnamesLeaf *nextleaf;
    __GLnamesLeaf *newleaf;
    void *empty;
    GLuint start, end, i;
    GLuint firstdel, lastdel;
    GLuint memoryProblem;

    if (range == 0) return;

    /*
    ** First we check for possible memory problems, since it will be 
    ** difficult to back out once we start.  We note a possible problem, 
    ** and check for it before fragmenting a leaf.
    */
    memoryProblem = 0;
    if (array->nbranches != __GL_DL_EXTRA_BRANCHES ||
	    array->nleaves != __GL_DL_EXTRA_LEAVES) {
	memoryProblem = 1;
    }

    firstdel = name;
    lastdel = name+range-1;

    /*LINTED nextleaf ok; lint bug*/
    for (leaf = findLeaf(array, name, GL_FALSE); leaf != NULL; 
	    leaf = nextleaf) {
	nextleaf = nextLeaf(leaf);
	start = leaf->start;
	end = leaf->end;
	if (lastdel < start) break;
	if (firstdel > end) continue;

	if (firstdel > start) start = firstdel;
	if (lastdel < end) end = lastdel;

	/*
	** Need to delete the range of lists from start to end.
	*/
	if (leaf->dataList) {
	    empty = array->dataInfo->empty;
	    for (i=start; i<=end; i++) {
		if (leaf->dataList[i - leaf->start] != empty) {
		    (*leaf->dataInfo->free)(gc, 
			leaf->dataList[i - leaf->start]);
		    leaf->dataList[i - leaf->start] = empty;
		}
	    }
	}

	if (start == leaf->start) {
	    if (end == leaf->end) {
		/* Bye bye leaf! */
		deleteLeaf(gc, array, leaf);
		freeLeaf(gc, leaf);
	    } else {
		/* Shrink leaf */
		resizeLeaf(gc, leaf, end+1, leaf->end);
	    }
	} else if (end == leaf->end) {
	    /* Shrink leaf */
	    resizeLeaf(gc, leaf, leaf->start, start-1);
	} else {
	    if (memoryProblem) {
		if (!fixMemoryProblem(gc, array)) {
		    __glSetError(GL_OUT_OF_MEMORY);
		    return;
		}
	    }
	    /* Crud.  The middle of the leaf was deleted.  This is tough. */
	    newleaf = allocLeaf(gc, array);

	    newleaf->start = end+1;
	    newleaf->end = leaf->end;
	    if (leaf->dataList) {
		if (!allocLeafData(gc, newleaf)) {
		    /* 
		    ** Damn!  We are screwed.  This is a bad spot for an
		    ** out of memory error.  It is also bloody unlikely,
		    ** because we just freed up some memory.
		    */
		    freeLeaf(gc, newleaf);
		    __glSetError(GL_OUT_OF_MEMORY);
		    return;
		}
		copyLeafInfo(leaf, newleaf);
	    }
	    resizeLeaf(gc, leaf, leaf->start, start-1);
	    insertLeaf(gc, array, newleaf);
	    break;
	}
    }
}

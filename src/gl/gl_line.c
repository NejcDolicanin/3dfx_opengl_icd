/*
** gl_line.c -- s_lnfin.c: the line begin / vertex state machines.
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

/* ---- s_lnfin.c: line strip vertex proc (DO_VALIDATE asks the vertex machine for the face needs) --- */
typedef void (__fastcall *__GLrenderLine)(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b);
/* ---- s_ptfin.c: point vertex with the render buffers locked ------------------------------------ */
#define __GL_LOCK_RENDER_BUFFERS(gc) \
    if ((++(gc)->buffers.lockCnt == 1) && (gc)->buffers.lockRenderBuffers) \
	((void (__fastcall *)(__GLcontext *))(gc)->buffers.lockRenderBuffers)(gc)
#define __GL_UNLOCK_RENDER_BUFFERS(gc) \
    if ((--(gc)->buffers.lockCnt == 0) && (gc)->buffers.unlockRenderBuffers) \
	((void (__fastcall *)(__GLcontext *))(gc)->buffers.unlockRenderBuffers)(gc)
/* ---- s_lnfin.c: the second vertex of a line strip ------------------------------------------- */
typedef void (__fastcall *__GLvalidateVertex)(__GLcontext *gc, __GLvertexICD *v, GLuint needs);
typedef GLuint (__fastcall *__GLgetNeeds)(__GLcontext *gc);
#define RENDER_LINE(gc, a, b)   (((__GLrenderLine)(gc)->procs.renderLine)(gc, a, b))
#define FACE_NEEDS0(gc)         (((__GLgetNeeds)(gc)->vertex.getFaceNeeds0)(gc))
#define DO_VALIDATE(gc, v, needs) \
    (((__GLvalidateVertex)(gc)->procs.validateVertex[((v)->flagBits >> 14) & 3])(gc, v, needs))
#define LOCK_RENDER_BUFFERS(gc) \
    if ((++(gc)->buffers.lockCnt == 1) && (gc)->buffers.lockRenderBuffers) \
	((void (__fastcall *)(__GLcontext *))(gc)->buffers.lockRenderBuffers)(gc)
#define UNLOCK_RENDER_BUFFERS(gc) \
    if ((--(gc)->buffers.lockCnt == 0) && (gc)->buffers.unlockRenderBuffers) \
	((void (__fastcall *)(__GLcontext *))(gc)->buffers.unlockRenderBuffers)(gc)
#define CLIP_LINE(gc, a, b)     (((__GLrenderLine)(gc)->procs.clipLine)(gc, a, b))

/* 69095690 -- __glSecondLStripVertex: draws the first segment with the render buffers locked */
void __fastcall __glSecondLStripVertex(__GLcontext *gc, __GLvertexICD *v1)
{
    __GLvertexICD *v0 = v1 - 1;

    gc->line.notResetStipple = GL_FALSE;
    gc->vertex.v0 = v0;
    gc->procs.vertex = (__GLproc)__gl1stLinesVertex;
    __GL_LOCK_RENDER_BUFFERS(gc);
    (*(__GLrenderLine)gc->procs.clipLine)(gc, v0, v1);
    __GL_UNLOCK_RENDER_BUFFERS(gc);
}

/* 69095770 -- __glOtherLStripVertexLock (s_lnfin.c) */
void __fastcall __glOtherLStripVertexLock(__GLcontext *gc, __GLvertexICD *v0)
{
    __GLvertexICD *v1 = gc->vertex.v1;

    gc->vertex.v1 = v0;
    gc->vertex.v0 = v1;

    if ((v0->flagBits | v1->flagBits) & __GL_CLIP_ALL_MASK) {
	/*
	** The line must be clipped more carefully.  Cannot trivially
	** accept the lines.
	*/
	if (((v0->flagBits & v1->flagBits) & __GL_CLIP_ALL_MASK) != 0) {
	    /*
	    ** Trivially reject the line.  If anding the codes is non-zero then
	    ** every vertex in the line is outside of the same set of
	    ** clipping planes (at least one).
	    */
	    return;
	}
	LOCK_RENDER_BUFFERS(gc);
	__glClipLine(gc, v1, v0);
	UNLOCK_RENDER_BUFFERS(gc);
	return;
    }

    /* Validate provoking vertex color */
    DO_VALIDATE(gc, v0, FACE_NEEDS0(gc));

    /* Draw the line */
    LOCK_RENDER_BUFFERS(gc);
    RENDER_LINE(gc, v1, v0);
    UNLOCK_RENDER_BUFFERS(gc);
}

/* 69095870 -- __glOtherLStripVertex */
void __fastcall __glOtherLStripVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    __GLvertexICD *v1 = gc->vertex.v1;

    gc->vertex.v1 = v0;
    gc->vertex.v0 = v1;

    if ((v0->flagBits | v1->flagBits) & __GL_CLIP_ALL_MASK) {
	if (((v0->flagBits & v1->flagBits) & __GL_CLIP_ALL_MASK) != 0) {
	    return;
	}
	__glClipLine(gc, v1, v0);
	return;
    }

    /* Validate provoking vertex color */
    (*gc->procs.validateVertex[(v0->flagBits >> 14) & 3])(gc, v0, (*gc->vertex.getFaceNeeds0)(gc));

    /* Draw the line */
    (*(__GLrenderLine)gc->procs.renderLine)(gc, v1, v0);
}

/* 690958e0 -- __glOtherLStripVertexFastLock (s_lnfin.c) */
void __fastcall __glOtherLStripVertexFastLock(__GLcontext *gc, __GLvertexICD *v0)
{
    __GLvertexICD *v1 = gc->vertex.v1;

    gc->vertex.v0 = v1;
    gc->vertex.v1 = v0;

    LOCK_RENDER_BUFFERS(gc);
    if ((v1->flagBits | v0->flagBits) & __GL_CLIP_ALL_MASK) {
	CLIP_LINE(gc, v1, v0);
    } else {
	RENDER_LINE(gc, v1, v0);
    }
    UNLOCK_RENDER_BUFFERS(gc);
}

/* 69095960 -- __glOtherLStripVertexFast (s_lnfin.c): line strip vertex, no validation (the color is already final) */
void __fastcall __glOtherLStripVertexFast(__GLcontext *gc, __GLvertexICD *v1)
{
    __GLvertexICD *v0 = gc->vertex.v1;

    gc->vertex.v0 = v0;
    gc->vertex.v1 = v1;
    if ((v0->flagBits | v1->flagBits) & __GL_CLIP_ALL_MASK) {
	(*(__GLrenderLine)gc->procs.clipLine)(gc, v0, v1);
	return;
    }
    (*(__GLrenderLine)gc->procs.renderLine)(gc, v0, v1);
}

/* 690959a0 -- __gl1stLStripVertex (s_lnfin.c) */
void __fastcall __gl1stLStripVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    gc->vertex.v0 = v0 + 1;
    gc->vertex.v1 = v0;
    gc->procs.vertex = gc->procs.vertexLStrip;
    gc->procs.matValidate = (__GLproc)__glMatValidateV1;

    /* Validate first vertex, if needed */
    if (FACE_NEEDS0(gc))
	DO_VALIDATE(gc, v0, FACE_NEEDS0(gc));
}

/* 69095a30 -- __gl2ndLLoopVertexLock (s_lnfin.c) */
void __fastcall __gl2ndLLoopVertexLock(__GLcontext *gc, __GLvertexICD *v0)
{
    gc->vertex.v0 = v0 + 1;
    gc->vertex.v1 = v0;
    gc->procs.vertex = gc->procs.vertexLStrip;
    gc->procs.matValidate = (__GLproc)__glMatValidateFan;

    LOCK_RENDER_BUFFERS(gc);
    CLIP_LINE(gc, v0 - 1, v0);
    UNLOCK_RENDER_BUFFERS(gc);
}

/* 69095af0 -- __glEndLLoopLock: __glEndLLoop: close the loop (v1 back to the first vertex) unless fewer than two
** vertices came */
void __fastcall __glEndLLoopLock(__GLcontext *gc)
{
    if (gc->procs.vertex != (__GLproc)FirstLLoopVertex &&
	gc->procs.vertex != gc->procs.vertex2ndLLoop) {
	__GL_LOCK_RENDER_BUFFERS(gc);
	(*(__GLrenderLine)gc->procs.clipLine)(gc, gc->vertex.v1, &gc->vertex.vbuf[0]);
	__GL_UNLOCK_RENDER_BUFFERS(gc);
    }
    gc->procs.vertex = (__GLproc)__glNop2;
    gc->procs.endPrim = (__GLproc)__glEndPrim;
}

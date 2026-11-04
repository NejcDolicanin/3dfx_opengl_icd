/*
** gl_begin.c -- original VA range 69094000..69098000: the begin / vertex state machines of the
** primitives (3dfx S_PGFIN.C, S_LNFIN.C, S_PTFIN.C: matValidate procs, Begin*, the first vertices,
** End*), s_image.c __glImageSize3D / __glFillImage and s_eval.c __glFreeEvaluatorState.
** ICD: a vertex's boundary edge is the __GL_VERTEX_EDGE_FLAG bit of flagBits, the material needs
** come from vertex.getMaterialNeeds, and some next-vertex procs sit in gc->procs slots.
** Then s_prim.c, in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "icdprotos.h"
#include <stddef.h>
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"

typedef void (__fastcall *__GLvalidateVertexProc940)(__GLcontext *gc, __GLvertexICD *v, GLuint needs);
typedef void (__fastcall *__GLfreeProc940)(__GLcontext *gc, void *p);
#define DO_VALIDATE(gc, v, needs) \
    (*(__GLvalidateVertexProc940)(gc)->procs.validateVertex[((v)->flagBits >> 14) & 3])(gc, v, needs)

/* ---- forward declarations ---- */
void __fastcall __glMatValidateVbuf0N(__GLcontext *gc);
void __fastcall FirstTFanVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall FirstTStripVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall FirstQStripVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall SecondQStripVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __glEndPoints(__GLcontext *gc);

/* 69094050 -- __glMatValidateCacheVbuf0N (S_PGFIN.C): vbuf[0] up to v0 (exclusive) */
void __fastcall __glMatValidateCacheVbuf0N(__GLcontext *gc)
{
    __GLvertexICD *v;
    GLuint needs;

    needs = (*gc->vertex.getMaterialNeeds)(gc);

    for (v = &gc->vertex.vbuf[0]; v < gc->vertex.v0; v++) {
        if (~v->flagBits & needs) DO_VALIDATE(gc, v, needs);
    }
}

/* 690940a0 -- __glMatValidateVbuf0N (S_PGFIN.C): vbuf[0] up to v0 (exclusive) */
void __fastcall __glMatValidateVbuf0N(__GLcontext *gc)
{
    __GLvertexICD *v;
    GLuint needs;

    needs = (*gc->vertex.getMaterialNeeds)(gc);

    for (v = &gc->vertex.vbuf[0]; v < gc->vertex.v0; v++) {
        if (~v->flagBits & needs) DO_VALIDATE(gc, v, needs);
    }
}

/* 69094150 -- __glMatValidateV1 (S_PGFIN.C): v1 only */
void __fastcall __glMatValidateV1(__GLcontext *gc)
{
    __GLvertexICD *v;
    GLuint needs;

    needs = (*gc->vertex.getMaterialNeeds)(gc);

    v = gc->vertex.v1;
    if (~v->flagBits & needs) DO_VALIDATE(gc, v, needs);
}

/* 69094180 -- __glMatValidateV1V2 (S_PGFIN.C): v1 and v2 */
void __fastcall __glMatValidateV1V2(__GLcontext *gc)
{
    __GLvertexICD *v;
    GLuint needs;

    needs = (*gc->vertex.getMaterialNeeds)(gc);

    v = gc->vertex.v1;
    if (~v->flagBits & needs) DO_VALIDATE(gc, v, needs);

    v = gc->vertex.v2;
    if (~v->flagBits & needs) DO_VALIDATE(gc, v, needs);
}

/* 690941e0 -- __glMatValidateV1V2V3 (S_PGFIN.C): v1, v2 and v3 */
void __fastcall __glMatValidateV1V2V3(__GLcontext *gc)
{
    __GLvertexICD *v;
    GLuint needs;

    needs = (*gc->vertex.getMaterialNeeds)(gc);

    v = gc->vertex.v1;
    if (~v->flagBits & needs) DO_VALIDATE(gc, v, needs);

    v = gc->vertex.v2;
    if (~v->flagBits & needs) DO_VALIDATE(gc, v, needs);

    v = gc->vertex.v3;
    if (~v->flagBits & needs) DO_VALIDATE(gc, v, needs);
}

/* 69094260 -- __glMatValidateV2V3 (S_PGFIN.C): v2 and v3 */
void __fastcall __glMatValidateV2V3(__GLcontext *gc)
{
    __GLvertexICD *v;
    GLuint needs;

    needs = (*gc->vertex.getMaterialNeeds)(gc);

    v = gc->vertex.v2;
    if (~v->flagBits & needs) DO_VALIDATE(gc, v, needs);

    v = gc->vertex.v3;
    if (~v->flagBits & needs) DO_VALIDATE(gc, v, needs);
}

/* 69094450 -- __glBeginTFan (S_PGFIN.C) */
void __fastcall __glBeginTFan(__GLcontext *gc)
{
    gc->procs.vertex = (__GLproc)FirstTFanVertex;
    gc->vertex.v0 = &gc->vertex.vbuf[0];
    gc->procs.matValidate = (__GLproc)__glMatValidateVbuf0N;
}

/* 69094480 -- FirstTFanVertex (S_PGFIN.C): the fan centre: edge flag set, advance; next is __glSecondTFanVertex */
void __fastcall FirstTFanVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    v0->flagBits |= __GL_VERTEX_EDGE_FLAG;
    gc->vertex.v0 = v0 + 1;
    gc->procs.vertex = (__GLproc)__glSecondTFanVertex;      /* SecondTFanVertex */
}

/* 69094870 -- __glEndTStrip (S_PGFIN.C) */
void __fastcall __glEndTStrip(__GLcontext *gc)
{
    gc->procs.endPrim = (__GLproc)__glEndPrim;     /* __glEndPrim */
    gc->procs.vertex = (__GLproc)__glNop2;      /* __glNop */
}

/* 69094890 -- __glBeginTStrip (S_PGFIN.C) */
void __fastcall __glBeginTStrip(__GLcontext *gc)
{
    gc->procs.vertex = (__GLproc)FirstTStripVertex;
    gc->vertex.v0 = &gc->vertex.vbuf[0];
    gc->procs.matValidate = (__GLproc)__glMatValidateVbuf0N;
}

/* 690948c0 -- FirstTStripVertex (S_PGFIN.C): edge flag set, kept as v2, advance; next is __glSecondTStripVertex */
void __fastcall FirstTStripVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    v0->flagBits |= __GL_VERTEX_EDGE_FLAG;
    gc->vertex.v0 = v0 + 1;
    gc->vertex.v2 = v0;
    gc->procs.vertex = (__GLproc)__glSecondTStripVertex;      /* SecondTStripVertex */
}

/* 69094ac0 -- FirstTrianglesVertex (S_PGFIN.C): the edge flag from the current edge tag */
void __fastcall FirstTrianglesVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    v0->flagBits |= gc->state.current.edgeTag;
    gc->vertex.v0 = v0 + 1;
    gc->procs.vertex = (__GLproc)SecondTrianglesVertex;      /* SecondTrianglesVertex */
}

/* 69094b20 -- __glBeginTriangles (S_PGFIN.C) */
void __fastcall __glBeginTriangles(__GLcontext *gc)
{
    gc->procs.vertex = (__GLproc)FirstTrianglesVertex;
    gc->vertex.v0 = &gc->vertex.vbuf[0];
    gc->procs.matValidate = (__GLproc)__glMatValidateVbuf0N;
}

/* 69094e50 -- __glEndQStrip (S_PGFIN.C) */
void __fastcall __glEndQStrip(__GLcontext *gc)
{
    gc->procs.endPrim = (__GLproc)__glEndPrim;     /* __glEndPrim */
    gc->procs.vertex = (__GLproc)__glNop2;      /* __glNop */
}

/* 69094e70 -- __glBeginQStrip (S_PGFIN.C) */
void __fastcall __glBeginQStrip(__GLcontext *gc)
{
    gc->procs.vertex = (__GLproc)FirstQStripVertex;
    gc->vertex.v0 = &gc->vertex.vbuf[0];
    gc->procs.matValidate = (__GLproc)__glMatValidateVbuf0N;
}

/* 69094ea0 -- FirstQStripVertex (S_PGFIN.C): edge flag set, kept as v3, advance; next is SecondQStripVertex */
void __fastcall FirstQStripVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    v0->flagBits |= __GL_VERTEX_EDGE_FLAG;
    gc->vertex.v0 = v0 + 1;
    gc->vertex.v3 = v0;
    gc->procs.vertex = (__GLproc)SecondQStripVertex;      /* SecondQStripVertex */
}

/* 69094ed0 -- SecondQStripVertex (S_PGFIN.C): edge flag set, kept as v2, advance; next is __glThirdQStripVertex */
void __fastcall SecondQStripVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    v0->flagBits |= __GL_VERTEX_EDGE_FLAG;
    gc->vertex.v0 = v0 + 1;
    gc->vertex.v2 = v0;
    gc->procs.vertex = (__GLproc)__glThirdQStripVertex;      /* ThirdQStripVertex */
}

/* 69095190 -- FirstQuadsVertex (S_PGFIN.C): edge flag from the current edge tag, advance; next SecondQuadsVertex */
void __fastcall FirstQuadsVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    v0->flagBits |= gc->state.current.edgeTag;
    gc->vertex.v0 = v0 + 1;
    gc->procs.vertex = (__GLproc)SecondQuadsVertex;      /* SecondQuadsVertex */
}

/* 69095220 -- __glBeginQuads (S_PGFIN.C) */
void __fastcall __glBeginQuads(__GLcontext *gc)
{
    gc->procs.vertex = (__GLproc)FirstQuadsVertex;
    gc->vertex.v0 = &gc->vertex.vbuf[0];
    gc->procs.matValidate = (__GLproc)__glMatValidateVbuf0N;
}

/* 690954b0 -- __glBeginPolygon (S_PGFIN.C): the vertex / end procs from gc->procs */
void __fastcall __glBeginPolygon(__GLcontext *gc)
{
    gc->vertex.v0 = &gc->vertex.vbuf[0];
    gc->line.notResetStipple = GL_FALSE;
    gc->procs.vertex = gc->procs.vertexPoly;
    gc->procs.endPrim = gc->procs.endPoly;
    gc->procs.matValidate = (__GLproc)__glMatValidateVbuf0N;
}

/* 690954f0 -- __glBeginPoints (S_PTFIN.C) */
void __fastcall __glBeginPoints(__GLcontext *gc)
{
    gc->procs.vertex = gc->procs.vertexPoints;
    gc->vertex.v0 = &gc->vertex.vbuf[0];
    gc->procs.endPrim = (__GLproc)__glEndPoints;
    gc->procs.matValidate = (__GLproc)__glNop1;
}

/* 69095520 -- __glEndPoints (S_PTFIN.C) */
void __fastcall __glEndPoints(__GLcontext *gc)
{
    gc->procs.vertex = (__GLproc)__glNop2;      /* __glNop */
    gc->procs.endPrim = (__GLproc)__glEndPrim;     /* __glEndPrim */
}

/* 69095660 -- __glPointFast (S_PTFIN.C): render an unclipped point (a tail jump) */
void __fastcall __glPointFast(__GLcontext *gc, __GLvertexICD *vx)
{
    if ((vx->flagBits & __GL_CLIP_ALL_MASK) == 0) {
        (*gc->procs.renderPoint)(gc, vx);
    }
}

/* 69095740 -- __glBeginLines (S_LNFIN.C) */
void __fastcall __glBeginLines(__GLcontext *gc)
{
    gc->procs.vertex = (__GLproc)__gl1stLinesVertex;      /* __glFirstLinesVertex */
    gc->vertex.v0 = &gc->vertex.vbuf[0];
    gc->procs.matValidate = (__GLproc)__glMatValidateVbuf0N;
}

/* 69095a00 -- __glBeginLStrip (S_LNFIN.C) */
void __fastcall __glBeginLStrip(__GLcontext *gc)
{
    gc->line.notResetStipple = GL_FALSE;

    gc->vertex.v0 = &gc->vertex.vbuf[0];
    gc->procs.vertex = (__GLproc)__gl1stLStripVertex;      /* __glFirstLStripVertex */
    gc->procs.matValidate = (__GLproc)__glMatValidateVbuf0N;
}

/* 69095ab0 -- SecondLLoopVertex (S_LNFIN.C): keep it as v1, hand on to procs.vertexLStrip, clip the first line */
void __fastcall SecondLLoopVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    gc->vertex.v1 = v0;
    gc->vertex.v0 = v0 + 1;
    gc->procs.vertex = gc->procs.vertexLStrip;
    gc->procs.matValidate = (__GLproc)__glMatValidateFan;  /* __glMatValidateVbuf0V1 */
    (*gc->procs.clipLine)(gc, v0 - 1, v0);
}

/* 69095ba0 -- __glEndLLoop (S_LNFIN.C): with two vertices or more, close the loop back to vbuf[0]
** (the ICD's second-vertex proc is procs.vertex2ndLLoop).
*/
void __fastcall __glEndLLoop(__GLcontext *gc)
{
    if (gc->procs.vertex != (__GLproc)FirstLLoopVertex &&
            gc->procs.vertex != gc->procs.vertex2ndLLoop) {
        (*gc->procs.clipLine)(gc, gc->vertex.v1, &gc->vertex.vbuf[0]);
    }
    gc->procs.vertex = (__GLproc)__glNop2;      /* __glNop */
    gc->procs.endPrim = (__GLproc)__glEndPrim;     /* __glEndPrim */
}

/* 69095bf0 -- __glBeginLLoop (S_LNFIN.C) */
void __fastcall __glBeginLLoop(__GLcontext *gc)
{
    gc->line.notResetStipple = GL_FALSE;

    gc->vertex.v0 = &gc->vertex.vbuf[0];
    gc->procs.vertex = (__GLproc)FirstLLoopVertex;      /* FirstLLoopVertex */
    gc->procs.endPrim = gc->procs.endLLoop;
    gc->procs.matValidate = (__GLproc)__glMatValidateVbuf0N;
}

/* 69095ce0 -- __glImageSize3D (s_image.c): one image times depth */
GLint __fastcall __glImageSize3D(GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type)
{
    return __glImageSize(width, height, format, type) * depth;
}

/* 69096060 -- __glFillImage (s_image.c): __glFillImage3D with depth 1 */
void __fastcall __glFillImage(__GLcontext *gc, GLsizei width, GLsizei height, GLenum format, GLenum type,
                             const GLvoid *userdata, GLubyte *newimage)
{
    __glFillImage3D(gc, width, height, 1, format, type, userdata, newimage);
}

/* 69097be0 -- __glFreeEvaluatorState (s_eval.c): the control points of the nine 1-D and 2-D maps */
void __fastcall __glFreeEvaluatorState(__GLcontext *gc)
{
    int i;
    __GLevaluatorMachineICD *evals = &gc->eval;

    for (i = 0; i < __GL_MAP_RANGE_COUNT; i++) {
        if (evals->eval1Data[i]) {
            (*(__GLfreeProc940)gc->imports[3])(gc, evals->eval1Data[i]);
            evals->eval1Data[i] = 0;
        }
        if (evals->eval2Data[i]) {
            (*(__GLfreeProc940)gc->imports[3])(gc, evals->eval2Data[i]);
            evals->eval2Data[i] = 0;
        }
    }
}

/* ---- s_prim.c: glEnd -------------------------------------------------------------------------- */
/* 6907d9a0 -- __glim_End */
void __fastcall __glim_End(void)
{
    __GL_SETUP();
    GLint beginMode = __gl_beginMode;

    if (beginMode == __GL_NOT_IN_BEGIN || beginMode == __GL_NEED_VALIDATE) {
	__glSetError(GL_INVALID_OPERATION);
	return;
    }
    (*gc->procs.endPrim)(gc);
    __gl_beginMode = __GL_NOT_IN_BEGIN;
}

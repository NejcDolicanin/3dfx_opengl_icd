/*
** gl_eval.c -- s_eval.c: evaluators (SI port: si_s_eval.c).
*/
#include <string.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "si_names.h"
#include "icdprotos.h"
#include <math.h>
#include "glshade.h"

/************************************************************************/
/* 69005a20 -- s_eval.c defaultMaps (a constant in the ICD) */
static const struct defaultMap {
    GLint	index;
    GLint	k;
    __GLfloat	values[4];
} defaultMaps[__GL_MAP_RANGE_COUNT] = {
    {__GL_C4, 4, 1.0, 1.0, 1.0, 1.0},
    {__GL_I , 1, 1.0, 0.0, 0.0, 0.0},
    {__GL_N3, 3, 0.0, 0.0, 1.0, 0.0},
    {__GL_T1, 1, 0.0, 0.0, 0.0, 0.0},
    {__GL_T2, 2, 0.0, 0.0, 0.0, 0.0},
    {__GL_T3, 3, 0.0, 0.0, 0.0, 0.0},
    {__GL_T4, 4, 0.0, 0.0, 0.0, 1.0},
    {__GL_V3, 3, 0.0, 0.0, 0.0, 0.0},
    {__GL_V4, 4, 0.0, 0.0, 0.0, 1.0},
};
typedef void *(__fastcall *__GLmallocProc)(__GLcontext *gc, size_t size);
#define IMPORTS_MALLOC(gc)  ((__GLmallocProc)(gc)->imports[0])
/* ---- s_eval.c ----------------------------------------------------------------------------- */
typedef void (__fastcall *__GLec1Proc)(__GLcontext *gc, __GLfloat u);
/************************************************************************/
/* s_eval.c */
typedef void (__fastcall *__GLec2)(__GLcontext *gc, __GLfloat u, __GLfloat v);

/* 69097ab0 -- __glInitEvaluatorState */
void __fastcall __glInitEvaluatorState(__GLcontext *gc)
{
    int i,j;
    const struct defaultMap *defMap;
    __GLevaluator1ICD *eval1;
    __GLevaluator2ICD *eval2;
    __GLfloat **eval1Data;
    __GLfloat **eval2Data;

    for (i = 0; i < 9; i++) {
    defMap = &(defaultMaps[i]);
    eval1 = &(gc->eval.eval1[i]);
    eval2 = &(gc->eval.eval2[i]);
    eval1Data = &(gc->eval.eval1Data[i]);
    eval2Data = &(gc->eval.eval2Data[i]);

    eval1->order = 1;
    eval1->u1 = __GL_ZERO_F;
    eval1->u2 = __GL_ONE_F;
    eval1->k = defMap->k;
    eval2->majorOrder = 1;
    eval2->minorOrder = 1;
    eval2->u1 = __GL_ZERO_F;
    eval2->u2 = __GL_ONE_F;
    eval2->v1 = __GL_ZERO_F;
    eval2->v2 = __GL_ONE_F;
    eval2->k = defMap->k;
    *eval1Data = (__GLfloat *)
        (*IMPORTS_MALLOC(gc))(gc, (size_t) (sizeof(__GLfloat) * defMap->k));
    *eval2Data = (__GLfloat *)
        (*IMPORTS_MALLOC(gc))(gc, (size_t) (sizeof(__GLfloat) * defMap->k));
    for (j = 0; j < defMap->k; j++) {
        (*eval1Data)[j] = defMap->values[j];
        (*eval2Data)[j] = defMap->values[j];
    }
    }

    gc->eval.uorder = 0;
    gc->eval.vorder = 0;

    gc->state.evaluator.u1.start = __GL_ZERO_F;
    gc->state.evaluator.u2.start = __GL_ZERO_F;
    gc->state.evaluator.v2.start = __GL_ZERO_F;
    gc->state.evaluator.u1.finish = __GL_ONE_F;
    gc->state.evaluator.u2.finish = __GL_ONE_F;
    gc->state.evaluator.v2.finish = __GL_ONE_F;
    gc->state.evaluator.u1.n = 1;
    gc->state.evaluator.u2.n = 1;
    gc->state.evaluator.v2.n = 1;
}

/* 69098310 -- s_eval.c __glim_EvalPoint1 */
void __stdcall __glim_EvalPoint1(GLint i)
{
    __GLevaluatorGrid *u;
    __GLfloat du;
    __GL_SETUP();
    __GL_API_EVAL();

    u = &gc->state.evaluator.u1;
    du = (u->finish - u->start)/(__GLfloat)u->n;/*XXX cache? */
    (*(__GLec1Proc)gc->procs.ec1)(gc, (i == u->n) ? u->finish : (u->start + i * du) );
}

/* 69098450 -- __glim_EvalPoint2 */
void __stdcall __glim_EvalPoint2(GLint i, GLint j)
{
    __GLevaluatorGrid *u;
    __GLevaluatorGrid *v;
    __GLfloat du;
    __GLfloat dv;
    __GL_SETUP();
    __GL_API_EVAL();

    u = &gc->state.evaluator.u2;
    v = &gc->state.evaluator.v2;
    du = (u->finish - u->start)/(__GLfloat)u->n;
    dv = (v->finish - v->start)/(__GLfloat)v->n;
    (*(__GLec2)gc->procs.ec2)(gc, (i == u->n) ? u->finish : (u->start + i * du),
                                 (j == v->n) ? v->finish : (v->start + j * dv));
}

/* ---- APIENTRY entries with double arguments (SI s_eval.c, s_capi.c, s_tapi.c; the ICD forms) ---- */

/* 69097c20 -- __glim_Map1d (s_eval.c) */
void __stdcall __glim_Map1d(GLenum type, GLdouble u1, GLdouble u2,
                            GLint stride, GLint order, const GLdouble *points)
{
    __GLevaluator1ICD *ev;
    __GLfloat *data;
    __GL_SETUP_NOT_IN_BEGIN();

    ev = __glSetUpMap1(gc, type, order, u1, u2);
    if (ev == 0) {
        return;
    }
    if (stride < ev->k) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }

    __GL_API_EVAL();

    data = gc->eval.eval1Data[__GL_EVAL1D_INDEX(type)];
    __glFillMap1d(ev->k, order, stride, points, data);
}

/* 69097cc0 -- __glim_Map1f (s_eval.c) */
void __stdcall __glim_Map1f(GLenum type, GLfloat u1, GLfloat u2,
                            GLint stride, GLint order, const GLfloat *points)
{
    __GLevaluator1ICD *ev;
    __GLfloat *data;
    __GL_SETUP_NOT_IN_BEGIN();

    ev = __glSetUpMap1(gc, type, order, u1, u2);
    if (ev == 0) {
        return;
    }
    if (stride < ev->k) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }

    __GL_API_EVAL();

    data = gc->eval.eval1Data[__GL_EVAL1D_INDEX(type)];
    __glFillMap1f(ev->k, order, stride, points, data);
}

/* 69097d50 -- __glim_Map2d (s_eval.c) */
void __stdcall __glim_Map2d(GLenum type,
                            GLdouble u1, GLdouble u2,
                            GLint uStride, GLint uOrder,
                            GLdouble v1, GLdouble v2,
                            GLint vStride, GLint vOrder,
                            const GLdouble *points)
{
    __GLevaluator2ICD *ev;
    __GLfloat *data;
    __GL_SETUP_NOT_IN_BEGIN();

    ev = __glSetUpMap2(gc, type, uOrder, vOrder, u1, u2, v1, v2);
    if (ev == 0) {
        return;
    }
    if (uStride < ev->k) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    if (vStride < ev->k) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }

    __GL_API_EVAL();

    data = gc->eval.eval2Data[__GL_EVAL2D_INDEX(type)];
    __glFillMap2d(ev->k, uOrder, vOrder, uStride, vStride,
        points, data);
}

/* 69097e20 -- __glim_Map2f (s_eval.c) */
void __stdcall __glim_Map2f(GLenum type,
                            GLfloat u1, GLfloat u2,
                            GLint uStride, GLint uOrder,
                            GLfloat v1, GLfloat v2,
                            GLint vStride, GLint vOrder,
                            const GLfloat *points)
{
    __GLevaluator2ICD *ev;
    __GLfloat *data;
    __GL_SETUP_NOT_IN_BEGIN();

    ev = __glSetUpMap2(gc, type, uOrder, vOrder, u1, u2, v1, v2);
    if (ev == 0) {
        return;
    }
    if (uStride < ev->k) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    if (vStride < ev->k) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }

    __GL_API_EVAL();

    data = gc->eval.eval2Data[__GL_EVAL2D_INDEX(type)];
    __glFillMap2f(ev->k, uOrder, vOrder, uStride, vStride,
        points, data);
}

/* 69098060 -- __glim_MapGrid1d (s_eval.c) */
void __stdcall __glim_MapGrid1d(GLint nu, GLdouble u0, GLdouble u1)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (nu <= 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }

    __GL_API_EVAL();

    gc->state.evaluator.u1.n = nu;
    gc->state.evaluator.u1.start = (__GLfloat)u0;
    gc->state.evaluator.u1.finish = (__GLfloat)u1;
}

/* 69098140 -- __glim_MapGrid2d (s_eval.c) */
void __stdcall __glim_MapGrid2d(GLint nu, GLdouble u0, GLdouble u1,
                            GLint nv, GLdouble v0, GLdouble v1)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((nu <= 0) || (nv <= 0)) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }

    __GL_API_EVAL();

    gc->state.evaluator.u2.n = nu;
    gc->state.evaluator.v2.n = nv;
    gc->state.evaluator.u2.start = (__GLfloat)u0;
    gc->state.evaluator.u2.finish = (__GLfloat)u1;
    gc->state.evaluator.v2.start = (__GLfloat)v0;
    gc->state.evaluator.v2.finish = (__GLfloat)v1;
}

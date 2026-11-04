/*
** si_s_eval.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_eval.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 690985c0 -- __glMap1_size */
GLint __fastcall __glMap1_size(GLint k, GLint order)
{
    return k * order;
}

/* 690986f0 -- __glMap2_size */
GLint __fastcall __glMap2_size(GLint k, GLint majorOrder, GLint minorOrder)
{
    return k * majorOrder * minorOrder;
}

/* 69098c70 -- PreEvaluate */
void __fastcall PreEvaluate(GLint order, __GLfloat vprime, __GLfloat *coeff)
{
    GLint i, j;
    __GLfloat oldval, temp;
    __GLfloat oneMinusvprime;

    /*
    ** Minor optimization
    ** Compute orders 1 and 2 outright, and set coeff[0], coeff[1] to
    ** their i==1 loop values to avoid the initialization and the i==1 loop.
    */
    if (order == 1) {
        coeff[0] = ((__GLfloat) 1.0);
        return;
    }

    oneMinusvprime = 1-vprime;
    coeff[0] = oneMinusvprime;
    coeff[1] = vprime;
    if (order == 2) return;

    for (i = 2; i < order; i++) {
        oldval = coeff[0] * vprime;
        coeff[0] = oneMinusvprime * coeff[0];
        for (j = 1; j < i; j++) {
            temp = oldval;
            oldval = coeff[j] * vprime;
            coeff[j] = temp + oneMinusvprime * coeff[j];
        }
        coeff[j] = oldval;
    }
}

/* 69099350 -- ComputeFirstPartials */
void __fastcall ComputeFirstPartials(__GLfloat *p, __GLfloat *pu, __GLfloat *pv)
{
    pu[0] = pu[0]*p[3] - pu[3]*p[0];
    pu[1] = pu[1]*p[3] - pu[3]*p[1];
    pu[2] = pu[2]*p[3] - pu[3]*p[2];

    pv[0] = pv[0]*p[3] - pv[3]*p[0];
    pv[1] = pv[1]*p[3] - pv[3]*p[1];
    pv[2] = pv[2]*p[3] - pv[3]*p[2];
}

/* 690993c0 -- ComputeNormal2 */
void __fastcall ComputeNormal2(__GLcontext *gc, __GLfloat *n, __GLfloat *pu, 
                           __GLfloat *pv)
{
    n[0] = pu[1]*pv[2] - pu[2]*pv[1];
    n[1] = pu[2]*pv[0] - pu[0]*pv[2];
    n[2] = pu[0]*pv[1] - pu[1]*pv[0];
    (*gc->procs.normalize)(n, n);
}

/* 69097ee0 -- __glim_EvalCoord1d */
void __stdcall __glim_EvalCoord1d(GLdouble u)
{
    __GL_SETUP();
    __GL_API_EVAL();
    (*(void (__fastcall *)(__GLcontext *, __GLfloat))gc->procs.ec1)(gc, (__GLfloat)u);
}

/* 69097f10 -- __glim_EvalCoord1dv */
void __stdcall __glim_EvalCoord1dv(const GLdouble u[1])
{
    __GL_SETUP();
    __GL_API_EVAL();
    (*(void (__fastcall *)(__GLcontext *, __GLfloat))gc->procs.ec1)(gc, (__GLfloat)u[0]);
}

/* 69097f40 -- __glim_EvalCoord1f */
void __stdcall __glim_EvalCoord1f(GLfloat u)
{
    __GL_SETUP();
    __GL_API_EVAL();
    (*(void (__fastcall *)(__GLcontext *, __GLfloat))gc->procs.ec1)(gc, u);
}

/* 69097f70 -- __glim_EvalCoord1fv */
void __stdcall __glim_EvalCoord1fv(const GLfloat u[1])
{
    __GL_SETUP();
    __GL_API_EVAL();
    (*(void (__fastcall *)(__GLcontext *, __GLfloat))gc->procs.ec1)(gc, u[0]);
}

/* 69097fa0 -- __glim_EvalCoord2d */
void __stdcall __glim_EvalCoord2d(GLdouble u, GLdouble v)
{
    __GL_SETUP();
    __GL_API_EVAL();
    (*(void (__fastcall *)(__GLcontext *, __GLfloat, __GLfloat))gc->procs.ec2)(gc, (__GLfloat)u, (__GLfloat)v);
}

/* 69097fd0 -- __glim_EvalCoord2dv */
void __stdcall __glim_EvalCoord2dv(const GLdouble u[2])
{
    __GL_SETUP();
    __GL_API_EVAL();
    (*(void (__fastcall *)(__GLcontext *, __GLfloat, __GLfloat))gc->procs.ec2)(gc, (__GLfloat)u[0], (__GLfloat)u[1]);
}

/* 69098030 -- __glim_EvalCoord2fv */
void __stdcall __glim_EvalCoord2fv(const GLfloat u[2])
{
    __GL_SETUP();
    __GL_API_EVAL();
    (*(void (__fastcall *)(__GLcontext *, __GLfloat, __GLfloat))gc->procs.ec2)(gc, u[0], u[1]);
}

/* 690980d0 -- __glim_MapGrid1f */
void __stdcall __glim_MapGrid1f(GLint nu, GLfloat u0, GLfloat u1)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (nu <= 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }

    __GL_API_EVAL();

    gc->state.evaluator.u1.start = (__GLfloat)u0;
    gc->state.evaluator.u1.finish = (__GLfloat)u1;
    gc->state.evaluator.u1.n = nu;
}

/* 690981d0 -- __glim_MapGrid2f */
void __stdcall __glim_MapGrid2f(GLint nu, GLfloat u0, GLfloat u1,
                            GLint nv, GLfloat v0, GLfloat v1)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((nu <= 0) || (nv <= 0)) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }

    __GL_API_EVAL();

    gc->state.evaluator.u2.start = (__GLfloat)u0;
    gc->state.evaluator.u2.finish = (__GLfloat)u1;
    gc->state.evaluator.u2.n = nu;
    gc->state.evaluator.v2.start = (__GLfloat)v0;
    gc->state.evaluator.v2.finish = (__GLfloat)v1;
    gc->state.evaluator.v2.n = nv;
}

/* 69098570 -- __glFillMap1d */
void __fastcall __glFillMap1d(GLint k, GLint order, GLint stride, 
                   const GLdouble *points, __GLfloat *data)
{
    int i,j;

    for (i=0; i<order; i++) {
        for (j=0; j<k; j++) {
            data[j] = points[j];
        }
        points += stride;
        data += k;
    }
}

/* 69098680 -- __glFillMap2d */
void __fastcall __glFillMap2d(GLint k, GLint majorOrder, GLint minorOrder, 
                   GLint majorStride, GLint minorStride,
                   const GLdouble *points, __GLfloat *data)
{
    int i,j,x;

    for (i=0; i<majorOrder; i++) {
        for (j=0; j<minorOrder; j++) {
            for (x=0; x<k; x++) {
                data[x] = points[x];
            }
            points += minorStride;
            data += k;
        }
        points += majorStride - minorStride * minorOrder;
    }
}

/* 69098260 -- __glim_EvalMesh1 */
void __stdcall __glim_EvalMesh1(GLenum mode, GLint low, GLint high)
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_EVAL();

    switch(mode) {
      case GL_LINE:
        __glEvalMesh1Line(gc, low, high);
        break;
      case GL_POINT:
        __glEvalMesh1Point(gc, low, high);
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 69098380 -- __glim_EvalMesh2 */
void __stdcall __glim_EvalMesh2(GLenum mode, 
                            GLint lowU, GLint highU, 
                            GLint lowV, GLint highV)
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_EVAL();

    switch(mode) {
      case GL_FILL:
        __glEvalMesh2Fill(gc, lowU, lowV, highU, highV);
        break;
      case GL_LINE:
        __glEvalMesh2Line(gc, lowU, lowV, highU, highV);
        break;
      case GL_POINT:
        __glEvalMesh2Point(gc, lowU, lowV, highU, highV);
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690985d0 -- __glFillMap2f */
void __fastcall __glFillMap2f(GLint k, GLint majorOrder, GLint minorOrder, 
                   GLint majorStride, GLint minorStride,
                   const GLfloat *points, __GLfloat *data)
{
    int i,j,x;

#ifndef __GL_DOUBLE
    /* Optimization always hit during display list execution */
    if (k == minorStride && majorStride == k * minorOrder) {
        __GL_MEMCOPY(data, points, 
                __glMap2_size(k, majorOrder, minorOrder) * sizeof(__GLfloat));
        return;
    }
#endif
    for (i=0; i<majorOrder; i++) {
        for (j=0; j<minorOrder; j++) {
            for (x=0; x<k; x++) {
                data[x] = points[x];
            }
            points += minorStride;
            data += k;
        }
        points += majorStride - minorStride * minorOrder;
    }
}

/* 69098d10 -- __glDoEvalCoord2 */
void __fastcall __glDoEvalCoord2(__GLcontext *gc, __GLfloat u, __GLfloat v)
{
    __GLcolor currentColor;
    __GLcoord currentNormal, currentTexture;

    currentColor = gc->state.current.color;
    currentNormal = gc->state.current.normal;
    currentTexture = gc->state.current.texture[0];

    DoEvalCoord2(gc, u, v, NULL);

    gc->state.current.color = currentColor;
    gc->state.current.normal = currentNormal;
    gc->state.current.texture[0] = currentTexture;
}

/* 690984f0 -- __glFillMap1f */
void __fastcall __glFillMap1f(GLint k, GLint order, GLint stride, 
                   const GLfloat *points, __GLfloat *data)
{
    int i,j;

#ifndef __GL_DOUBLE
    /* Optimization always hit during display list execution */
    if (k == stride) {
        __GL_MEMCOPY(data, points, 
                __glMap1_size(k, order) * sizeof(__GLfloat));
        return;
    }
#endif
    for (i=0; i<order; i++) {
        for (j=0; j<k; j++) {
            data[j] = points[j];
        }
        points += stride;
        data += k;
    }
}

/* 69098700 -- __glSetUpMap1 */
__GLevaluator1ICD * __fastcall __glSetUpMap1(__GLcontext *gc, GLenum type,
                              GLint order, __GLfloat u1, __GLfloat u2)
{
    __GLevaluator1ICD *ev;
    __GLfloat **evData;

    switch (type) {
      case GL_MAP1_COLOR_4:
      case GL_MAP1_INDEX:
      case GL_MAP1_NORMAL:
      case GL_MAP1_TEXTURE_COORD_1:
      case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3:
      case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3:
      case GL_MAP1_VERTEX_4:
        ev = &gc->eval.eval1[__GL_EVAL1D_INDEX(type)];
        evData = &gc->eval.eval1Data[__GL_EVAL1D_INDEX(type)];
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return 0;
    }
    if (u1 == u2 || order < 1 || order > gc->constants.maxEvalOrder) {
        __glSetError(GL_INVALID_VALUE);
        return 0;
    }
    ev->order = order;
    ev->u1 = u1;
    ev->u2 = u2;
    *evData = (__GLfloat *)
        ((void *(__fastcall *)(__GLcontext *, void *, size_t))gc->imports[2])(gc, *evData,
            (size_t) (__glMap1_size(ev->k, order) * sizeof(__GLfloat)));

    return ev;
}

/* 690987a0 -- __glSetUpMap2 */
__GLevaluator2ICD * __fastcall __glSetUpMap2(__GLcontext *gc, GLenum type,
                              GLint majorOrder, GLint minorOrder,
                              __GLfloat u1, __GLfloat u2,
                              __GLfloat v1, __GLfloat v2)
{
    __GLevaluator2ICD *ev;
    __GLfloat **evData;

    switch (type) {
      case GL_MAP2_COLOR_4:
      case GL_MAP2_INDEX:
      case GL_MAP2_NORMAL:
      case GL_MAP2_TEXTURE_COORD_1:
      case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3:
      case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3:
      case GL_MAP2_VERTEX_4:
        ev = &gc->eval.eval2[__GL_EVAL2D_INDEX(type)];
        evData = &gc->eval.eval2Data[__GL_EVAL2D_INDEX(type)];
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return 0;
    }
    if (minorOrder < 1 || minorOrder > gc->constants.maxEvalOrder ||
            majorOrder < 1 || majorOrder > gc->constants.maxEvalOrder ||
            u1 == u2 || v1 == v2) {
        __glSetError(GL_INVALID_VALUE);
        return 0;
    }
    ev->majorOrder = majorOrder;
    ev->minorOrder = minorOrder;
    ev->u1 = u1;
    ev->u2 = u2;
    ev->v1 = v1;
    ev->v2 = v2;
    *evData = (__GLfloat *)
        ((void *(__fastcall *)(__GLcontext *, void *, size_t))gc->imports[2])(gc, *evData,
            (size_t) (__glMap2_size(ev->k, majorOrder, minorOrder)
                      * sizeof(__GLfloat)));

    return ev;
}

/* 69098b80 -- DoDomain1 */
void __fastcall DoDomain1(__GLevaluatorMachineICD *em, __GLfloat u, __GLevaluator1ICD *e, 
        __GLfloat *v, __GLfloat *baseData)
{
    GLint j, row;
    __GLfloat uprime;
    __GLfloat *data;
    GLint k;

    if(e->u2 == e->u1)
        return;
    uprime = (u - e->u1) / (e->u2 - e->u1);

    /* Use already cached values if possible */
    if (em->uvalue != uprime || em->uorder != e->order) {
        /* Compute coefficients for values */
        PreEvaluate(e->order, uprime, em->ucoeff);
        em->utype = TYPE_COEFF;
        em->uorder = e->order;
        em->uvalue = uprime;
    }

    k=e->k;
    for (j = 0; j < k; j++) {
        data=baseData+j;
        v[j] = 0;
        for (row = 0; row < e->order; row++) {
            v[j] += em->ucoeff[row] * (*data);
            data += k;
        }
    }
}

/* 69099410 -- DoDomain2 */
void __fastcall DoDomain2(__GLevaluatorMachineICD *em, __GLfloat u, __GLfloat v, 
        __GLevaluator2ICD *e, __GLfloat *r, __GLfloat *baseData)
{
    GLint j, row, col;
    __GLfloat uprime;
    __GLfloat vprime;
    __GLfloat p;
    __GLfloat *data;
    GLint k;
    
    if((e->u2 == e->u1) || (e->v2 == e->v1))
        return;
    uprime = (u - e->u1) / (e->u2 - e->u1);
    vprime = (v - e->v1) / (e->v2 - e->v1);

    /* Compute coefficients for values */

    /* Use already cached values if possible */
    if (em->uvalue != uprime || em->uorder != e->majorOrder) {
        PreEvaluate(e->majorOrder, uprime, em->ucoeff);
        em->utype = TYPE_COEFF;
        em->uorder = e->majorOrder;
        em->uvalue = uprime;
    }
    if (em->vvalue != vprime || em->vorder != e->minorOrder) {
        PreEvaluate(e->minorOrder, vprime, em->vcoeff);
        em->vtype = TYPE_COEFF;
        em->vorder = e->minorOrder;
        em->vvalue = vprime;
    }
    
    k=e->k;
    for (j = 0; j < k; j++) { 
        data=baseData+j;
        r[j] = 0;
        for (row = 0; row < e->majorOrder; row++)  {
            /* 
            ** Minor optimization.
            ** The col == 0 part of the loop is extracted so we don't
            ** have to initialize p to 0.
            */
            p=em->vcoeff[0] * (*data);
            data += k;
            for (col = 1; col < e->minorOrder; col++) {
                p += em->vcoeff[col] * (*data);
                data += k;
            }
            r[j] += em->ucoeff[row] * p;
        }
    }
}

/* 69099920 -- __glEvalMesh1Line */
void __fastcall __glEvalMesh1Line(__GLcontext *gc, GLint low, GLint high)
{
    __GLevaluatorGrid *u = &gc->state.evaluator.u1;
    __GLfloat du;
    GLint i;
    __GLcolor currentColor;
    __GLcoord currentNormal, currentTexture;

    if ( u->n == 0)
        return;
    du = (u->finish - u->start)/(__GLfloat)u->n;

    currentColor = gc->state.current.color;
    currentNormal = gc->state.current.normal;
    currentTexture = gc->state.current.texture[0];

    /*
    ** multiplication instead of iterative adding done to prevent
    ** accumulation of error.
    */
    (*gc->dispatchState->fn[__GL_DISP_Begin])(GL_LINE_STRIP);
    for (i = low; i <= high ; i++) {
        DoEvalCoord1(gc, (i == u->n) ? u->finish : (u->start + i * du));
    }
    (*gc->dispatchState->fn[__GL_DISP_End])();

    gc->state.current.color = currentColor;
    gc->state.current.normal = currentNormal;
    gc->state.current.texture[0] = currentTexture;
}

/* 69099a90 -- __glEvalMesh1Point */
void __fastcall __glEvalMesh1Point(__GLcontext *gc, GLint low, GLint high)
{
    __GLevaluatorGrid *u = &gc->state.evaluator.u1;
    __GLfloat du;
    GLint i;
    __GLcolor currentColor;
    __GLcoord currentNormal, currentTexture;

    if ( u->n == 0)
        return;
    du = (u->finish - u->start)/(__GLfloat)u->n;

    currentColor = gc->state.current.color;
    currentNormal = gc->state.current.normal;
    currentTexture = gc->state.current.texture[0];

    /*
    ** multiplication instead of iterative adding done to prevent
    ** accumulation of error.
    */
    (*gc->dispatchState->fn[__GL_DISP_Begin])(GL_POINTS);
    for (i = low; i <= high ; i++) {
        DoEvalCoord1(gc, (i == u->n) ? u->finish : (u->start + i * du));
    }
    (*gc->dispatchState->fn[__GL_DISP_End])();

    gc->state.current.color = currentColor;
    gc->state.current.normal = currentNormal;
    gc->state.current.texture[0] = currentTexture;
}

/* 69099eb0 -- sendChange */
void __fastcall sendChange(__GLcontext *gc, StateChange *change)
{
    __GLcolor userColor;

    if (change->changed & CHANGE_COLOR) {
        userColor = gc->state.current.userColor;
        gc->state.current.userColor = change->color;
        (*gc->procs.applyColor)(gc);
    }
    if (change->changed & CHANGE_TEXTURE) {
        gc->state.current.texture[0] = change->texture;
    }
    if (change->changed & CHANGE_NORMAL) {
        gc->state.current.normal = change->normal;
    }
    if (change->changed & CHANGE_VERTEX3) {
        (*gc->dispatchState->fn[__GL_DISP_Vertex3fv])(&change->vertex.x);
    } else if (change->changed & CHANGE_VERTEX4) {
        (*gc->dispatchState->fn[__GL_DISP_Vertex4fv])(&change->vertex.x);
    }
    if (change->changed & CHANGE_COLOR) {
        gc->state.current.userColor = userColor;
        (*gc->procs.applyColor)(gc);
    }
}

/* 69099fb0 -- __glEvalMesh2Point */
void __fastcall __glEvalMesh2Point(__GLcontext *gc, GLint lowU, GLint lowV,
                        GLint highU, GLint highV)
{
    __GLevaluatorGrid *u = &gc->state.evaluator.u2;
    __GLevaluatorGrid *v = &gc->state.evaluator.v2;
    __GLfloat du, dv;
    GLint i, j;
    __GLcolor currentColor;
    __GLcoord currentNormal, currentTexture;

    if ((u->n == 0)|| (v->n == 0))
        return;
    du = (u->finish - u->start)/(__GLfloat)u->n;
    dv = (v->finish - v->start)/(__GLfloat)v->n;

    currentColor = gc->state.current.color;
    currentNormal = gc->state.current.normal;
    currentTexture = gc->state.current.texture[0];

    (*gc->dispatchState->fn[__GL_DISP_Begin])(GL_POINTS);
    for (i = lowU; i <= highU; i++) {
        __GLfloat u1 = (i == u->n) ? u->finish : (u->start + i * du);
        for (j = lowV; j <= highV; j++) {
            __GLfloat v1 = (j == v->n) ? v->finish : (v->start + j * dv);

            DoEvalCoord2(gc, u1, v1, NULL);
        }
    }
    (*gc->dispatchState->fn[__GL_DISP_End])();

    gc->state.current.color = currentColor;
    gc->state.current.normal = currentNormal;
    gc->state.current.texture[0] = currentTexture;
}

/* 6909a1a0 -- __glEvalMesh2Line */
void __fastcall __glEvalMesh2Line(__GLcontext *gc, GLint lowU, GLint lowV,
                       GLint highU, GLint highV)
{
    __GLevaluatorGrid *u = &gc->state.evaluator.u2;
    __GLevaluatorGrid *v = &gc->state.evaluator.v2;
    __GLfloat du, dv;
    GLint i, j;
    __GLcolor currentColor;
    __GLcoord currentNormal, currentTexture;
    __GLfloat u1;
    StateChange changes[MAX_WIDTH];
    int row;

    if ((u->n == 0) || (v->n == 0))
        return;
    du = (u->finish - u->start)/(__GLfloat)u->n;
    dv = (v->finish - v->start)/(__GLfloat)v->n;

    currentColor = gc->state.current.color;
    currentNormal = gc->state.current.normal;
    currentTexture = gc->state.current.texture[0];

    for (i = lowU; i < highU; i++) {
        __GLfloat u2 = ((i+1) == u->n) ? u->finish : (u->start + (i+1) * du);
        u1 = (i == u->n) ? u->finish : (u->start + i * du);

        row=0;
        for (j = lowV; j <= highV; j++) {
            __GLfloat v1 = (j == v->n) ? v->finish : (v->start + j * dv);
            __GLfloat v2 = ((j+1) == v->n) ? v->finish : (v->start + (j+1)*dv);

            (*gc->dispatchState->fn[__GL_DISP_Begin])(GL_LINE_STRIP);
            if (j != highV) {
                if (row < MAX_WIDTH-1) {
                    if (i != lowU) {
                        sendChange(gc, changes+row+1);
                    } else {
                        DoEvalCoord2(gc, u1, v2, changes+row+1);
                    }
                } else {
                    DoEvalCoord2(gc, u1, v2, NULL);
                }
            }
            if (row < MAX_WIDTH) {
                if (i != lowU && j != lowV) {
                    sendChange(gc, changes+row);
                } else {
                    DoEvalCoord2(gc, u1, v1, NULL);
                }
                DoEvalCoord2(gc, u2, v1, changes+row);
            } else {
                DoEvalCoord2(gc, u1, v1, NULL);
                DoEvalCoord2(gc, u2, v1, NULL);
            }
            (*gc->dispatchState->fn[__GL_DISP_End])();
            row++;
        }
    }
    /* Now we need to do the i==highU iteration.  We will do it backwards
    ** (from highV to lowV) so the outline is drawn in the same direction 
    ** as the other mesh lines.
    */
    row--;
    u1 = (i == u->n) ? u->finish : (u->start + i * du);
    (*gc->dispatchState->fn[__GL_DISP_Begin])(GL_LINE_STRIP);
    for (j = highV; j >= lowV; j--) {
        __GLfloat v1 = (j == v->n) ? v->finish : (v->start + j * dv);

        if (row >= 0 && row < MAX_WIDTH) {
            sendChange(gc, changes+row);
        } else {
            DoEvalCoord2(gc, u1, v1, NULL);
        }
        
        row--;
    }
    (*gc->dispatchState->fn[__GL_DISP_End])();

    gc->state.current.color = currentColor;
    gc->state.current.normal = currentNormal;
    gc->state.current.texture[0] = currentTexture;
}

/* 690997f0 -- PreEvaluateWithDeriv */
void __fastcall PreEvaluateWithDeriv(GLint order, __GLfloat vprime, 
    __GLfloat *coeff, __GLfloat *coeffDeriv)
{
    GLint i, j;
    __GLfloat oldval, temp;
    __GLfloat oneMinusvprime;

    oneMinusvprime = 1-vprime;
    /*
    ** Minor optimization
    ** Compute orders 1 and 2 outright, and set coeff[0], coeff[1] to 
    ** their i==1 loop values to avoid the initialization and the i==1 loop.
    */
    if (order == 1) {
        coeff[0] = ((__GLfloat) 1.0);
        coeffDeriv[0] = __GL_ZERO_F;
        return;
    } else if (order == 2) {
        coeffDeriv[0] = __glMinusOne;
        coeffDeriv[1] = ((__GLfloat) 1.0);
        coeff[0] = oneMinusvprime;
        coeff[1] = vprime;
        return;
    }
    coeff[0] = oneMinusvprime;
    coeff[1] = vprime;
    for (i = 2; i < order - 1; i++) {
        oldval = coeff[0] * vprime;
        coeff[0] = oneMinusvprime * coeff[0];
        for (j = 1; j < i; j++) {
            temp = oldval;
            oldval = coeff[j] * vprime;
            coeff[j] = temp + oneMinusvprime * coeff[j];
        }
        coeff[j] = oldval;
    }
    coeffDeriv[0] = -coeff[0];
    /*
    ** Minor optimization:
    ** Would make this a "for (j=1; j<order-1; j++)" loop, but it is always
    ** executed at least once, so this is more efficient.
    */
    j=1;
    do {
        coeffDeriv[j] = coeff[j-1] - coeff[j];
        j++;
    } while (j < order - 1);
    coeffDeriv[j] = coeff[j-1];

    oldval = coeff[0] * vprime;
    coeff[0] = oneMinusvprime * coeff[0];
    for (j = 1; j < i; j++) {
        temp = oldval;
        oldval = coeff[j] * vprime;
        coeff[j] = temp + oneMinusvprime * coeff[j];
    }
    coeff[j] = oldval;
}

/* 690995b0 -- DoDomain2WithDerivs */
void __fastcall DoDomain2WithDerivs(__GLevaluatorMachineICD *em, __GLfloat u, 
                __GLfloat v, __GLevaluator2ICD *e, __GLfloat *r,
                __GLfloat *du, __GLfloat *dv, __GLfloat *baseData)
{
    GLint j, row, col;
    __GLfloat uprime;
    __GLfloat vprime;
    __GLfloat p;
    __GLfloat pdv;
    __GLfloat *data;
    GLint k;

    if((e->u2 == e->u1) || (e->v2 == e->v1))
        return;
    uprime = (u - e->u1) / (e->u2 - e->u1);
    vprime = (v - e->v1) / (e->v2 - e->v1);
    
    /* Compute coefficients for values and derivs */

    /* Use already cached values if possible */
    if (em->uvalue != uprime || em->utype != TYPE_COEFF_AND_DERIV || 
          em->uorder != e->majorOrder) {
        PreEvaluateWithDeriv(e->majorOrder, uprime, em->ucoeff, em->ucoeffDeriv);
        em->utype = TYPE_COEFF_AND_DERIV;
        em->uorder = e->majorOrder;
        em->uvalue = uprime;
    }
    if (em->vvalue != vprime || em->vtype != TYPE_COEFF_AND_DERIV ||
          em->vorder != e->minorOrder) {
        PreEvaluateWithDeriv(e->minorOrder, vprime, em->vcoeff, em->vcoeffDeriv);
        em->vtype = TYPE_COEFF_AND_DERIV;
        em->vorder = e->minorOrder;
        em->vvalue = vprime;
    }

    k=e->k;
    for (j = 0; j < k; j++) {
        data=baseData+j;
        r[j] = du[j] = dv[j] = __GL_ZERO_F;
        for (row = 0; row < e->majorOrder; row++)  {
            /* 
            ** Minor optimization.
            ** The col == 0 part of the loop is extracted so we don't
            ** have to initialize p and pdv to 0.
            */
            p = em->vcoeff[0] * (*data);
            pdv = em->vcoeffDeriv[0] * (*data);
            data += k;
            for (col = 1; col < e->minorOrder; col++) {
                /* Incrementally build up p, pdv value */
                p += em->vcoeff[col] * (*data);
                pdv += em->vcoeffDeriv[col] * (*data);
                data += k;
            }
            /* Use p, pdv value to incrementally add up r, du, dv */
            r[j] += em->ucoeff[row] * p;
            du[j] += em->ucoeffDeriv[row] * p;
            dv[j] += em->ucoeff[row] * pdv;
        }
    }
}

/* 69098880 -- __glDoEvalCoord1 */
void __fastcall __glDoEvalCoord1(__GLcontext *gc, __GLfloat u)
{
    __GLcolor currentColor;
    __GLcoord currentNormal, currentTexture;

    currentColor = gc->state.current.color;
    currentNormal = gc->state.current.normal;
    currentTexture = gc->state.current.texture[0];

    DoEvalCoord1(gc, u);

    gc->state.current.color = currentColor;
    gc->state.current.normal = currentNormal;
    gc->state.current.texture[0] = currentTexture;
}

/* 69099c00 -- __glEvalMesh2Fill */
void __fastcall __glEvalMesh2Fill(__GLcontext *gc, GLint lowU, GLint lowV,
                       GLint highU, GLint highV)
{
    __GLevaluatorGrid *u = &gc->state.evaluator.u2;
    __GLevaluatorGrid *v = &gc->state.evaluator.v2;
    __GLfloat du, dv;
    GLint i, j;
    __GLcolor currentColor;
    __GLcoord currentNormal, currentTexture;
    StateChange changes[MAX_WIDTH];
    int row;

    if ((u->n == 0)|| (v->n == 0))
        return;
    du = (u->finish - u->start)/(__GLfloat)u->n;
    dv = (v->finish - v->start)/(__GLfloat)v->n;

    currentColor = gc->state.current.color;
    currentNormal = gc->state.current.normal;
    currentTexture = gc->state.current.texture[0];

    for (i = lowU; i < highU; i++) {
        __GLfloat u1 = (i == u->n) ? u->finish : (u->start + i * du);
        __GLfloat u2 = ((i+1) == u->n) ? u->finish : (u->start + (i+1) * du);
        (*gc->dispatchState->fn[__GL_DISP_Begin])(GL_QUAD_STRIP);
        row=0;
        for (j = highV; j >= lowV; j--) {
            __GLfloat v1 = (j == v->n) ? v->finish : (v->start + j * dv);

            if (row < MAX_WIDTH) {
                /* use cached info if possible */
                if (i != lowU) {
                    sendChange(gc, changes+row);
                } else {
                    DoEvalCoord2(gc, u1, v1, NULL);
                }
                /* cache the answer for next iteration of i */
                DoEvalCoord2(gc, u2, v1, changes+row);
            } else {
                /* row larger than we expected likely.  No data is cached */
                DoEvalCoord2(gc, u1, v1, NULL);
                DoEvalCoord2(gc, u2, v1, NULL);
            }
            row++;

        }
        (*gc->dispatchState->fn[__GL_DISP_End])();
    }

    gc->state.current.color = currentColor;
    gc->state.current.normal = currentNormal;
    gc->state.current.texture[0] = currentTexture;
}

/* 69098950 -- DoEvalCoord1 */
void __fastcall DoEvalCoord1(__GLcontext *gc, __GLfloat u)
{
    __GLevaluator1ICD *eval = gc->eval.eval1;
    __GLfloat **evalData = gc->eval.eval1Data;
    GLboolean restoreUserColor = GL_FALSE;
    __GLcolor userColor;
    __GLevaluatorMachineICD em = gc->eval;

    if (gc->modes.colorIndexMode) {
        if (gc->state.enables.eval1 & __GL_MAP1_INDEX_ENABLE) {
            DoDomain1(&em, u, &eval[__GL_I], &gc->state.current.color.r,
                evalData[__GL_I]);
            gc->colorSet = GL_TRUE;
        }
    } else {
        if (gc->state.enables.eval1 & __GL_MAP1_COLOR_4_ENABLE) {
            __GLfloat one = __GL_ONE_F;
            __GLfloat zero = __GL_ZERO_F;

            restoreUserColor = GL_TRUE;
            userColor = gc->state.current.userColor;

            DoDomain1(&em, u, &eval[__GL_C4], &gc->state.current.userColor.r,
                evalData[__GL_C4]);
            gc->colorSet = GL_TRUE;
            (*gc->procs.applyColor)(gc);
        }
    }

    if (gc->state.enables.eval1 & __GL_MAP1_TEXTURE_COORD_4_ENABLE) {
        DoDomain1(&em, u, &eval[__GL_T4], &gc->state.current.texture[0].x,
            evalData[__GL_T4]);
    } else if (gc->state.enables.eval1 & __GL_MAP1_TEXTURE_COORD_3_ENABLE) {
        DoDomain1(&em, u, &eval[__GL_T3], &gc->state.current.texture[0].x,
            evalData[__GL_T3]);
        gc->state.current.texture[0].w = __GL_ONE_F;
    } else if (gc->state.enables.eval1 & __GL_MAP1_TEXTURE_COORD_2_ENABLE) {
        DoDomain1(&em, u, &eval[__GL_T2], &gc->state.current.texture[0].x,
            evalData[__GL_T2]);
        gc->state.current.texture[0].z = __GL_ZERO_F;
        gc->state.current.texture[0].w = __GL_ONE_F;
    } else if (gc->state.enables.eval1 & __GL_MAP1_TEXTURE_COORD_1_ENABLE) {
        DoDomain1(&em, u, &eval[__GL_T1], &gc->state.current.texture[0].x,
            evalData[__GL_T1]);
        gc->state.current.texture[0].y = __GL_ZERO_F;
        gc->state.current.texture[0].z = __GL_ZERO_F;
        gc->state.current.texture[0].w = __GL_ONE_F;
    }

    if (gc->state.enables.eval1 & __GL_MAP1_NORMAL_ENABLE) {
        DoDomain1(&em, u, &eval[__GL_N3], &gc->state.current.normal.x,
            evalData[__GL_N3]);
    }

    if (gc->state.enables.eval1 & __GL_MAP1_VERTEX_4_ENABLE) {
        __GLcoord vvec;

        DoDomain1(&em, u, &eval[__GL_V4], &vvec.x, evalData[__GL_V4]);
        (*gc->dispatchState->fn[__GL_DISP_Vertex4fv])(&vvec.x);
    } else if (gc->state.enables.eval1 & __GL_MAP1_VERTEX_3_ENABLE) {
        __GLcoord vvec;

        DoDomain1(&em, u, &eval[__GL_V3], &vvec.x, evalData[__GL_V3]);
        (*gc->dispatchState->fn[__GL_DISP_Vertex3fv])(&vvec.x);
    }
    if (restoreUserColor) {
        gc->state.current.userColor = userColor;
        (*gc->procs.applyColor)(gc);
    }
}

/* 69098df0 -- DoEvalCoord2 */
void __fastcall DoEvalCoord2(__GLcontext *gc, __GLfloat u, __GLfloat v, 
                StateChange *changes)
{
    __GLevaluator2ICD *eval = gc->eval.eval2;
    __GLfloat **evalData = gc->eval.eval2Data;
    GLint vertexType;
    GLboolean restoreUserColor = GL_FALSE;
    __GLcolor userColor;
    __GLcoord vvec;
    __GLevaluatorMachineICD em = gc->eval;

    if (changes) {
        changes->changed = 0;
    }

    vertexType = -1;
    if (gc->state.enables.general & __GL_AUTO_NORMAL_ENABLE) {
        if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_4_ENABLE) {
            __GLfloat du[4];
            __GLfloat dv[4];
            DoDomain2WithDerivs(&em, u, v, &eval[__GL_V4], &vvec.x, du, dv,
                evalData[__GL_V4]);
            ComputeFirstPartials(&vvec.x, du, dv);
            ComputeNormal2(gc, &gc->state.current.normal.x, du, dv);
            if (changes) {
                changes->changed |= CHANGE_NORMAL | CHANGE_VERTEX4;
                changes->normal = gc->state.current.normal;
                changes->vertex = vvec;
            }
            vertexType = 4;
        } else if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_3_ENABLE) {
            __GLfloat du[3];
            __GLfloat dv[3];
            DoDomain2WithDerivs(&em, u, v, &eval[__GL_V3], &vvec.x, du, dv,
                evalData[__GL_V3]);
            ComputeNormal2(gc, &gc->state.current.normal.x, du, dv);
            if (changes) {
                changes->changed |= CHANGE_NORMAL | CHANGE_VERTEX3;
                changes->normal = gc->state.current.normal;
                changes->vertex = vvec;
            }
            vertexType = 3;
        } 
    } else {
        if (gc->state.enables.eval2 & __GL_MAP2_NORMAL_ENABLE) {
            DoDomain2(&em, u, v, &eval[__GL_N3], &gc->state.current.normal.x,
                evalData[__GL_N3]);
            if (changes) {
                changes->changed |= CHANGE_NORMAL;
                changes->normal = gc->state.current.normal;
            }
        }
        if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_4_ENABLE) {
            DoDomain2(&em, u, v, &eval[__GL_V4], &vvec.x,
                evalData[__GL_V4]);
            if (changes) {
                changes->changed |= CHANGE_VERTEX4;
                changes->vertex = vvec;
            }
            vertexType = 4;
        } else if (gc->state.enables.eval2 & __GL_MAP2_VERTEX_3_ENABLE) {
            DoDomain2(&em, u, v, &eval[__GL_V3], &vvec.x,
                evalData[__GL_V3]);
            if (changes) {
                changes->changed |= CHANGE_VERTEX3;
                changes->vertex = vvec;
            }
            vertexType = 3;
        }
    }
    if (gc->modes.colorIndexMode) {
        if (gc->state.enables.eval2 & __GL_MAP2_INDEX_ENABLE) {
            DoDomain2(&em, u, v, &eval[__GL_I], &gc->state.current.color.r,
                evalData[__GL_I]);
            gc->colorSet = GL_TRUE;
            if (changes) {
                changes->changed |= CHANGE_COLOR;
                changes->color = gc->state.current.color;
            }
        }
    } else {
        if (gc->state.enables.eval2 & __GL_MAP2_COLOR_4_ENABLE) {
            __GLfloat one = __GL_ONE_F;
            __GLfloat zero = __GL_ZERO_F;

            restoreUserColor = GL_TRUE;
            userColor = gc->state.current.userColor;

            DoDomain2(&em, u, v, &eval[__GL_C4], &gc->state.current.userColor.r,
                evalData[__GL_C4]);
            gc->colorSet = GL_TRUE;
            (*gc->procs.applyColor)(gc);
            if (changes) {
                changes->changed |= CHANGE_COLOR;
                changes->color = gc->state.current.userColor;
            }
        }
    }

    if (gc->state.enables.eval2 & __GL_MAP2_TEXTURE_COORD_4_ENABLE) {
        DoDomain2(&em, u, v, &eval[__GL_T4], &gc->state.current.texture[0].x,
            evalData[__GL_T4]);
        if (changes) {
            changes->changed |= CHANGE_TEXTURE;
            changes->texture = gc->state.current.texture[0];
        }
    } else if (gc->state.enables.eval2 & __GL_MAP2_TEXTURE_COORD_3_ENABLE) {
        DoDomain2(&em, u, v, &eval[__GL_T3], &gc->state.current.texture[0].x,
            evalData[__GL_T3]);
        gc->state.current.texture[0].w = __GL_ONE_F;
        if (changes) {
            changes->changed |= CHANGE_TEXTURE;
            changes->texture = gc->state.current.texture[0];
        }
    } else if (gc->state.enables.eval2 & __GL_MAP2_TEXTURE_COORD_2_ENABLE) {
        DoDomain2(&em, u, v, &eval[__GL_T2], &gc->state.current.texture[0].x,
            evalData[__GL_T2]);
        gc->state.current.texture[0].z = __GL_ZERO_F;
        gc->state.current.texture[0].w = __GL_ONE_F;
        if (changes) {
            changes->changed |= CHANGE_TEXTURE;
            changes->texture = gc->state.current.texture[0];
        }
    } else if (gc->state.enables.eval2 & __GL_MAP2_TEXTURE_COORD_1_ENABLE) {
        DoDomain2(&em, u, v, &eval[__GL_T1], &gc->state.current.texture[0].x,
            evalData[__GL_T1]); 
        gc->state.current.texture[0].y = __GL_ZERO_F;
        gc->state.current.texture[0].z = __GL_ZERO_F;
        gc->state.current.texture[0].w = __GL_ONE_F;
        if (changes) {
            changes->changed |= CHANGE_TEXTURE;
            changes->texture = gc->state.current.texture[0];
        }
    }

    if (vertexType == 3) {
        (*gc->dispatchState->fn[__GL_DISP_Vertex3fv])(&vvec.x);
    } else if (vertexType == 4) {
        (*gc->dispatchState->fn[__GL_DISP_Vertex4fv])(&vvec.x);
    }
    if (restoreUserColor) {
        gc->state.current.userColor = userColor;
        (*gc->procs.applyColor)(gc);
    }
}

/* 69098000 -- __glim_EvalCoord2f */
void __stdcall __glim_EvalCoord2f(GLfloat u, GLfloat v)
{
    __GL_SETUP();
    __GL_API_EVAL();
    (*(void (__fastcall *)(__GLcontext *, __GLfloat, __GLfloat))gc->procs.ec2)(gc, u, v);
}

/* 69097a30 -- __glEvalComputeK */
GLint __fastcall __glEvalComputeK(GLenum target)
{
    switch(target) {
      case GL_MAP1_VERTEX_4:
      case GL_MAP1_COLOR_4:
      case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_4:
      case GL_MAP2_COLOR_4:
      case GL_MAP2_TEXTURE_COORD_4:
        return 4;
      case GL_MAP1_VERTEX_3:
      case GL_MAP1_TEXTURE_COORD_3:
      case GL_MAP1_NORMAL:
      case GL_MAP2_VERTEX_3:
      case GL_MAP2_TEXTURE_COORD_3:
      case GL_MAP2_NORMAL:
        return 3;
      case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_2:
        return 2;
      case GL_MAP1_TEXTURE_COORD_1:
      case GL_MAP2_TEXTURE_COORD_1:
      case GL_MAP1_INDEX:
      case GL_MAP2_INDEX:
        return 1;
      default:
        return -1;
    }
}

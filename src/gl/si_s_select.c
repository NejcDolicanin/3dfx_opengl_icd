/*
** si_s_select.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_select.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 690c0bf0 -- __glim_SelectBuffer */
void __stdcall __glim_SelectBuffer(GLsizei bufferLength, GLuint buffer[])
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    if (bufferLength < 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    if (gc->renderMode == GL_SELECT) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }
    gc->select.overFlowed = GL_FALSE;
    gc->select.resultBase = buffer;
    gc->select.resultLength = bufferLength;
    gc->select.result = buffer;
}

/* 690c0ce0 -- __glim_LoadName */
void __stdcall __glim_LoadName(GLuint name)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    if (gc->renderMode == GL_SELECT) {
        if (gc->select.sp == gc->select.stack) {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
        gc->select.sp[ -1 ] = name;
        gc->select.hit = GL_FALSE;
    }
}

/* 690c0dd0 -- __glim_PushName */
void __stdcall __glim_PushName(GLuint name)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    if (gc->renderMode == GL_SELECT) {
        if (gc->select.sp >= &gc->select.stack[gc->constants.maxNameStackDepth]) {
            __glSetError(GL_STACK_OVERFLOW);
            return;
        }
        gc->select.sp[0] = name;
        gc->select.sp = gc->select.sp + 1;
        gc->select.hit = GL_FALSE;
    }
}

/* 690c0f90 -- __glSelectTriangle */
void __fastcall __glSelectTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b,
                        __GLvertexICD *c)
{
    if (gc->state.enables.general & __GL_CULL_FACE_ENABLE) {
        __GLfloat dxAC, dxBC, dyAC, dyBC, area;
        GLboolean ccw, frontFacing;

        /* Compute signed area of the triangle */
        dxAC = a->window.x - c->window.x;
        dxBC = b->window.x - c->window.x;
        dyAC = a->window.y - c->window.y;
        dyBC = b->window.y - c->window.y;
        area = dxAC * dyBC - dxBC * dyAC;
        ccw = area >= __GL_ZERO_F;

        if ((gc->state.polygon.frontFaceDirection == GL_CCW) ^
            (gc->constants.yInverted)) {
            frontFacing = ccw;
        } else {
            frontFacing = !ccw;
        }
        if ((gc->state.polygon.cull == GL_FRONT_AND_BACK) ||
            ((gc->state.polygon.cull == GL_FRONT) && frontFacing) ||
            ((gc->state.polygon.cull == GL_BACK) && !frontFacing)) {
            /* Culled */
            return;
        }
    }
    __glSelectHit(gc, a->window.z);
    __glSelectHit(gc, b->window.z);
    __glSelectHit(gc, c->window.z);
}

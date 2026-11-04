/*
** gl_rect.c -- s_rect.c: glRect*.
*/
#include <stdlib.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "si_names.h"
#include "icdprotos.h"
#include <stddef.h>
#include <math.h>
#include "glshade.h"
#include <string.h>

/************************************************************************/
/* s_rect.c: procs.rect takes four floats on the stack; the ICD waits for the hardware first */
typedef void (__fastcall *__GLrectProcICD)(__GLcontext *gc, GLfloat ax, GLfloat ay, GLfloat bx, GLfloat by);
#define PROCS_RECT(gc, ax, ay, bx, by)    ((*(__GLrectProcICD)(gc)->procs.rect)(gc, ax, ay, bx, by))
/* ---- s_rect.c: glRectf ---------------------------------------------------------------------- */
typedef void (__fastcall *__GLrectfProc)(__GLcontext *gc, __GLfloat x1, __GLfloat y1, __GLfloat x2, __GLfloat y2);
/* the rectangle fill proc procs.rect (690976a0 in wglDevInstallHWProcs' table) */
#define __GL_RECT_PROC(gc)      ((__GLrectfProc)(gc)->procs.rect)
/* ---- s_rect.c: procs.rect (0x25a8, __glRect 690976a0) ------------------------------------------ */
typedef void (__fastcall *__GLrectProc)(__GLcontext *gc, __GLfloat x0, __GLfloat y0,
                                        __GLfloat x1, __GLfloat y1);
#define __GL_PROCS_RECT(gc) (*(__GLrectProc)(gc)->procs.rect)

#pragma optimize("y", off)      /* the original keeps the ebp frame here (no frame-pointer omission) */
/* 69097230 -- __glim_Rectdv */
void __stdcall __glim_Rectdv(const GLdouble v1[2], const GLdouble v2[2])
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    (*gc->procs.waitIdle)(gc);
    PROCS_RECT(gc, v1[0], v1[1], v2[0], v2[1]);
}
#pragma optimize("", on)

/* 690972d0 -- __glim_Rectf */
void __stdcall __glim_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    (*gc->procs.waitIdle)(gc);
    (*__GL_RECT_PROC(gc))(gc, x1, y1, x2, y2);
}

#pragma optimize("y", off)
/* 69097360 -- __glim_Rectfv (s_rect.c): the dispatch tables' glRectfv slot; the parameters are the float
** vectors, read here through GLint-typed pointers (the bits are passed on unchanged) */
void __stdcall __glim_Rectfv(const GLint v1[2], const GLint v2[2])
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    (*gc->procs.waitIdle)(gc);
    PROCS_RECT(gc, *(const GLfloat *)&v1[0], *(const GLfloat *)&v1[1],
	     *(const GLfloat *)&v2[0], *(const GLfloat *)&v2[1]);
}
#pragma optimize("", on)

/* 690973f0 -- s_rect.c __glim_Recti */
void __stdcall __glim_Recti(GLint ax, GLint ay, GLint bx, GLint by)
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_NOTBE_RENDER();

    (*gc->procs.waitIdle)(gc);
    __GL_PROCS_RECT(gc)(gc, ax, ay, bx, by);
}

/* 69097490 -- s_rect.c __glim_Rectiv */
void __stdcall __glim_Rectiv(const GLint v1[2], const GLint v2[2])
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_NOTBE_RENDER();

    (*gc->procs.waitIdle)(gc);
    __GL_PROCS_RECT(gc)(gc, v1[0], v1[1], v2[0], v2[1]);
}

/* 69097530 -- s_rect.c __glim_Rects */
void __stdcall __glim_Rects(GLshort ax, GLshort ay, GLshort bx, GLshort by)
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_NOTBE_RENDER();

    (*gc->procs.waitIdle)(gc);
    __GL_PROCS_RECT(gc)(gc, ax, ay, bx, by);
}

/* 690975e0 -- s_rect.c __glim_Rectsv */
void __stdcall __glim_Rectsv(const GLshort v1[2], const GLshort v2[2])
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_NOTBE_RENDER();

    (*gc->procs.waitIdle)(gc);
    __GL_PROCS_RECT(gc)(gc, v1[0], v1[1], v2[0], v2[1]);
}

#pragma optimize("y", off)
/* 69097190 -- __glim_Rectd.  Frame pointer kept. */
void __stdcall __glim_Rectd(GLdouble ax, GLdouble ay, GLdouble bx, GLdouble by)
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    (*gc->procs.waitIdle)(gc);
    PROCS_RECT(gc, ax, ay, bx, by);
}
#pragma optimize("", on)

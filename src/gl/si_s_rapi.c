/*
** si_s_rapi.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_rapi.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 690c2180 -- __glim_RasterPos2fv */
void __stdcall __glim_RasterPos2fv(const GLfloat v[2])
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    (*gc->procs.rasterPos2)(gc, v);    /* ICD: the caller's array, no copy (690c21d1) */
}

/* 690c21f0 -- __glim_RasterPos2d */
void __stdcall __glim_RasterPos2d(GLdouble x, GLdouble y)
{
    __GLfloat v[2];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    v[0] = x; v[1] = y;
    (*gc->procs.rasterPos2)(gc, v);
}

/* 690c2280 -- __glim_RasterPos2f */
void __stdcall __glim_RasterPos2f(GLfloat x, GLfloat y)
{
    __GLfloat v[2];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    v[0] = x; v[1] = y;
    (*gc->procs.rasterPos2)(gc, v);
}

/* 690c2310 -- __glim_RasterPos2i */
void __stdcall __glim_RasterPos2i(GLint x, GLint y)
{
    __GLfloat v[2];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    v[0] = x; v[1] = y;
    (*gc->procs.rasterPos2)(gc, v);
}

/* 690c23a0 -- __glim_RasterPos2iv */
void __stdcall __glim_RasterPos2iv(const GLint v[2])
{
    __GLfloat vv[2];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    vv[0] = v[0]; vv[1] = v[1];
    (*gc->procs.rasterPos2)(gc, vv);
}

/* 690c2430 -- __glim_RasterPos2s */
void __stdcall __glim_RasterPos2s(GLshort x, GLshort y)
{
    __GLfloat v[2];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    v[0] = x; v[1] = y;
    (*gc->procs.rasterPos2)(gc, v);
}

/* 690c24d0 -- __glim_RasterPos2sv */
void __stdcall __glim_RasterPos2sv(const GLshort v[2])
{
    __GLfloat vv[2];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    vv[0] = v[0]; vv[1] = v[1];
    (*gc->procs.rasterPos2)(gc, vv);
}

/* 690c2600 -- __glim_RasterPos3fv */
void __stdcall __glim_RasterPos3fv(const GLfloat v[3])
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    (*gc->procs.rasterPos3)(gc, v);    /* ICD: the caller's array, no copy */
}

/* 690c2670 -- __glim_RasterPos3d */
void __stdcall __glim_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
    __GLfloat v[3];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    v[0] = x; v[1] = y; v[2] = z;
    (*gc->procs.rasterPos3)(gc, v);
}

/* 690c2700 -- __glim_RasterPos3f */
void __stdcall __glim_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
    __GLfloat v[3];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    v[0] = x; v[1] = y; v[2] = z;
    (*gc->procs.rasterPos3)(gc, v);
}

/* 690c2790 -- __glim_RasterPos3i */
void __stdcall __glim_RasterPos3i(GLint x, GLint y, GLint z)
{
    __GLfloat v[3];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    v[0] = x; v[1] = y; v[2] = z;
    (*gc->procs.rasterPos3)(gc, v);
}

/* 690c2820 -- __glim_RasterPos3iv */
void __stdcall __glim_RasterPos3iv(const GLint v[3])
{
    __GLfloat vv[3];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2];
    (*gc->procs.rasterPos3)(gc, vv);
}

/* 690c28b0 -- __glim_RasterPos3s */
void __stdcall __glim_RasterPos3s(GLshort x, GLshort y, GLshort z)
{
    __GLfloat v[3];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    v[0] = x; v[1] = y; v[2] = z;
    (*gc->procs.rasterPos3)(gc, v);
}

/* 690c2950 -- __glim_RasterPos3sv */
void __stdcall __glim_RasterPos3sv(const GLshort v[3])
{
    __GLfloat vv[3];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2];
    (*gc->procs.rasterPos3)(gc, vv);
}

/* 690c2a90 -- __glim_RasterPos4fv */
void __stdcall __glim_RasterPos4fv(const GLfloat v[4])
{
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    (*gc->procs.rasterPos4)(gc, v);    /* ICD: the caller's array, no copy */
}

/* 690c2b00 -- __glim_RasterPos4d */
void __stdcall __glim_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    __GLfloat v[4];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    v[0] = x; v[1] = y; v[2] = z; v[3] = w;
    (*gc->procs.rasterPos4)(gc, v);
}

/* 690c2b90 -- __glim_RasterPos4f */
void __stdcall __glim_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    __GLfloat v[4];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    v[0] = x; v[1] = y; v[2] = z; v[3] = w;
    (*gc->procs.rasterPos4)(gc, v);
}

/* 690c2c20 -- __glim_RasterPos4i */
void __stdcall __glim_RasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
    __GLfloat v[4];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    v[0] = x; v[1] = y; v[2] = z; v[3] = w;
    (*gc->procs.rasterPos4)(gc, v);
}

/* 690c2cb0 -- __glim_RasterPos4iv */
void __stdcall __glim_RasterPos4iv(const GLint v[4])
{
    __GLfloat vv[4];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = v[3];
    (*gc->procs.rasterPos4)(gc, vv);
}

/* 690c2d50 -- __glim_RasterPos4s */
void __stdcall __glim_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    __GLfloat v[4];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    v[0] = x; v[1] = y; v[2] = z; v[3] = w;
    (*gc->procs.rasterPos4)(gc, v);
}

/* 690c2e00 -- __glim_RasterPos4sv */
void __stdcall __glim_RasterPos4sv(const GLshort v[4])
{
    __GLfloat vv[4];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = v[3];
    (*gc->procs.rasterPos4)(gc, vv);
}

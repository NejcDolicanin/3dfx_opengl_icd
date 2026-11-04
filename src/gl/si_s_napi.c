/*
** si_s_napi.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_napi.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690c2eb0 -- __glim_Normal3d */
void __stdcall __glim_Normal3d(GLdouble x, GLdouble y, GLdouble z)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.normal.x = x;
    gc->state.current.normal.y = y;
    gc->state.current.normal.z = z;
}

/* 690c2ee0 -- __glim_Normal3dv */
void __stdcall __glim_Normal3dv(const GLdouble v[3])
{
    GLdouble x, y, z;
    __GL_SETUP();
    __GL_API_VAPI();

    x = v[0];
    y = v[1];
    z = v[2];
    gc->state.current.normal.x = x;
    gc->state.current.normal.y = y;
    gc->state.current.normal.z = z;
}

/* 690c2f30 -- __glim_Normal3f */
void __stdcall __glim_Normal3f(GLfloat x, GLfloat y, GLfloat z)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.normal.x = x;
    gc->state.current.normal.y = y;
    gc->state.current.normal.z = z;
}

/* 690c2f60 -- __glim_Normal3fv */
void __stdcall __glim_Normal3fv(const GLfloat v[3])
{
    GLfloat x, y, z;
    __GL_SETUP();
    __GL_API_VAPI();

    x = v[0];
    y = v[1];
    z = v[2];
    gc->state.current.normal.x = x;
    gc->state.current.normal.y = y;
    gc->state.current.normal.z = z;
}

/* 690c2fa0 -- __glim_Normal3b */
void __stdcall __glim_Normal3b(GLbyte x, GLbyte y, GLbyte z)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.normal.x = __GL_B_TO_FLOAT(x);
    gc->state.current.normal.y = __GL_B_TO_FLOAT(y);
    gc->state.current.normal.z = __GL_B_TO_FLOAT(z);
}

/* 690c3010 -- __glim_Normal3bv */
void __stdcall __glim_Normal3bv(const GLbyte v[3])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.normal.x = __GL_B_TO_FLOAT(v[0]);
    gc->state.current.normal.y = __GL_B_TO_FLOAT(v[1]);
    gc->state.current.normal.z = __GL_B_TO_FLOAT(v[2]);
}

/* 690c3080 -- __glim_Normal3s */
void __stdcall __glim_Normal3s(GLshort x, GLshort y, GLshort z)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.normal.x = __GL_S_TO_FLOAT(x);
    gc->state.current.normal.y = __GL_S_TO_FLOAT(y);
    gc->state.current.normal.z = __GL_S_TO_FLOAT(z);
}

/* 690c30f0 -- __glim_Normal3sv */
void __stdcall __glim_Normal3sv(const GLshort v[3])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.normal.x = __GL_S_TO_FLOAT(v[0]);
    gc->state.current.normal.y = __GL_S_TO_FLOAT(v[1]);
    gc->state.current.normal.z = __GL_S_TO_FLOAT(v[2]);
}

/* 690c3160 -- __glim_Normal3i */
void __stdcall __glim_Normal3i(GLint x, GLint y, GLint z)
{
    __GL_SETUP();
    __GL_API_VAPI();

    /* Beware of overflow! */
    gc->state.current.normal.x = __GL_I_TO_FLOAT(x);
    gc->state.current.normal.y = __GL_I_TO_FLOAT(y);
    gc->state.current.normal.z = __GL_I_TO_FLOAT(z);
}

/* 690c31c0 -- __glim_Normal3iv */
void __stdcall __glim_Normal3iv(const GLint v[3])
{
    __GL_SETUP();
    __GL_API_VAPI();

    /* Beware of overflow! */
    gc->state.current.normal.x = __GL_I_TO_FLOAT(v[0]);
    gc->state.current.normal.y = __GL_I_TO_FLOAT(v[1]);
    gc->state.current.normal.z = __GL_I_TO_FLOAT(v[2]);
}

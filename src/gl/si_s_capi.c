/*
** si_s_capi.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_capi.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690a5d60 -- __glim_FastColor3ubv */
void __stdcall __glim_FastColor3ubv(const GLubyte v[3])
{
    __GLpixelMachineICD *pm;
    GLubyte r, g, b;
    __GL_SETUP();
    __GL_API_VAPI();

    r = v[0];
    g = v[1];
    b = v[2];

    gc->state.current.userColor.r = __GL_UB_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_UB_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_UB_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_ONE_F;

    /* The incoming values are never below zero or over 255, so
    ** there is no need to do the usual ClampAndScale call.
    ** Scaled values have been calculated and stoed in a table.
    */
    pm = &gc->pixel;
    gc->state.current.color.r = pm->redMap[r];
    gc->state.current.color.g = pm->greenMap[g];
    gc->state.current.color.b = pm->blueMap[b];
    gc->state.current.color.a = gc->frontBuffer.alphaScale;
}

/* 690a6470 -- __glim_Color4ub */
void __stdcall __glim_Color4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_UB_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_UB_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_UB_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_UB_TO_FLOAT(a);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a64f0 -- __glim_Color4ubv */
void __stdcall __glim_Color4ubv(const GLubyte v[4])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_UB_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_UB_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_UB_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_UB_TO_FLOAT(v[3]);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6960 -- __glim_Color4uiv */
void __stdcall __glim_Color4uiv(const GLuint v[4])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_UI_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_UI_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_UI_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_UI_TO_FLOAT(v[3]);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a5c40 -- __glim_FastColor3ub */
void __stdcall __glim_FastColor3ub(GLubyte r, GLubyte g, GLubyte b)
{
    __GLpixelMachineICD *pm;
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_UB_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_UB_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_UB_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;

    /* The incoming values are never below zero or over 255, so
    ** there is no need to do the usual ClampAndScale call.
    ** Scaled values have been calculated and stoed in a table.
    */
    pm = &gc->pixel;
    gc->state.current.color.r = pm->redMap[r];
    gc->state.current.color.g = pm->greenMap[g];
    gc->state.current.color.b = pm->blueMap[b];
    gc->state.current.color.a = gc->frontBuffer.alphaScale;
}

/* 690a5bd0 -- __glim_Color3ub */
void __stdcall __glim_Color3ub(GLubyte r, GLubyte g, GLubyte b)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_UB_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_UB_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_UB_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a5cf0 -- __glim_Color3ubv */
void __stdcall __glim_Color3ubv(const GLubyte v[3])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_UB_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_UB_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_UB_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a61e0 -- __glim_Color3uiv */
void __stdcall __glim_Color3uiv(const GLuint v[3])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_UI_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_UI_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_UI_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a69f0 -- __glim_Color4s */
void __stdcall __glim_Color4s(GLshort r, GLshort g, GLshort b, GLshort a)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_S_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_S_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_S_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_S_TO_FLOAT(a);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6560 -- __glim_Color4b */
void __stdcall __glim_Color4b(GLbyte r, GLbyte g, GLbyte b, GLbyte a)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_B_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_B_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_B_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_B_TO_FLOAT(a);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a63f0 -- __glim_Color3usv */
void __stdcall __glim_Color3usv(const GLushort v[3])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_US_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_US_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_US_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6370 -- __glim_Color3us */
void __stdcall __glim_Color3us(GLushort r, GLushort g, GLushort b)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_US_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_US_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_US_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6df0 -- __glim_Indexubv */
void __stdcall __glim_Indexubv(const GLubyte c[1])
{
    __GL_SETUP();
    __GL_API_VAPI();
    gc->state.current.userColorIndex = c[0];
    gc->colorSet = GL_TRUE;
}

/* 690a6ba0 -- __glim_Color4usv */
void __stdcall __glim_Color4usv(const GLushort v[4])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_US_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_US_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_US_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_US_TO_FLOAT(v[3]);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6b10 -- __glim_Color4us */
void __stdcall __glim_Color4us(GLushort r, GLushort g, GLushort b, GLushort a)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_US_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_US_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_US_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_US_TO_FLOAT(a);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6270 -- __glim_Color3s */
void __stdcall __glim_Color3s(GLshort r, GLshort g, GLshort b)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_S_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_S_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_S_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a5e20 -- __glim_Color3b */
void __stdcall __glim_Color3b(GLbyte r, GLbyte g, GLbyte b)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_B_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_B_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_B_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6dc0 -- __glim_Indexsv */
void __stdcall __glim_Indexsv(const GLshort c[1])
{
    __GL_SETUP();
    __GL_API_VAPI();
    gc->state.current.userColorIndex = c[0];
    gc->colorSet = GL_TRUE;
}

/* 690a6d60 -- __glim_Indexfv */
void __stdcall __glim_Indexfv(const GLfloat c[1])
{
    __GL_SETUP();
    __GL_API_VAPI();
    gc->state.current.userColorIndex = c[0];
    gc->colorSet = GL_TRUE;
}

/* 690a6840 -- __glim_Color4iv */
void __stdcall __glim_Color4iv(const GLint v[4])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_I_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_I_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_I_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_I_TO_FLOAT(v[3]);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a60e0 -- __glim_Color3iv */
void __stdcall __glim_Color3iv(const GLint v[3])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_I_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_I_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_I_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a5fc0 -- __glim_Color3f */
void __stdcall __glim_Color3f(GLfloat r, GLfloat g, GLfloat b)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = r;
    gc->state.current.userColor.g = g;
    gc->state.current.userColor.b = b;
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6d90 -- __glim_Indexiv */
void __stdcall __glim_Indexiv(const GLint c[1])
{
    __GL_SETUP();
    __GL_API_VAPI();
    gc->state.current.userColorIndex = c[0];
    gc->colorSet = GL_TRUE;
}

/* 690a6a80 -- __glim_Color4sv */
void __stdcall __glim_Color4sv(const GLshort v[4])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_S_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_S_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_S_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_S_TO_FLOAT(v[3]);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a65f0 -- __glim_Color4bv */
void __stdcall __glim_Color4bv(const GLbyte v[4])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_B_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_B_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_B_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_B_TO_FLOAT(v[3]);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a62f0 -- __glim_Color3sv */
void __stdcall __glim_Color3sv(const GLshort v[3])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_S_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_S_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_S_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a5ea0 -- __glim_Color3bv */
void __stdcall __glim_Color3bv(const GLbyte v[3])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_B_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_B_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_B_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

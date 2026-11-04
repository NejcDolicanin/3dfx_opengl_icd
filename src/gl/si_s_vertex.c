/*
** si_s_vertex.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_vertex.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 6907a9b0 -- __glSaveN */
void __fastcall __glSaveN(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat x,y,z;
    void (*vertex)(__GLcontext *gc, __GLvertexICD *vx);

    vertex = gc->procs.vertex;
    x = gc->state.current.normal.x;
    y = gc->state.current.normal.y;
    z = gc->state.current.normal.z;
    vx->normal.x = x;
    vx->normal.y = y;
    vx->normal.z = z;
    (*vertex)(gc, vx);
}

/* 6907a9f0 -- __glSaveCI */
void __fastcall __glSaveCI(__GLcontext *gc, __GLvertexICD *vx)
{
    void (*vertex)(__GLcontext *gc, __GLvertexICD *vx);
    __GLfloat index;

    vertex = gc->procs.vertex;
    index = gc->state.current.userColorIndex;
    vx->colors[__GL_FRONTFACE].r = index;
    (*vertex)(gc, vx);
}

/* 6907aa10 -- __glSaveC */
void __fastcall __glSaveC(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat r,g,b,a;
    void (*vertex)(__GLcontext *gc, __GLvertexICD *vx);

    vertex = gc->procs.vertex;
    r = gc->state.current.color.r;
    g = gc->state.current.color.g;
    b = gc->state.current.color.b;
    a = gc->state.current.color.a;
    vx->colors[__GL_FRONTFACE].r = r;
    vx->colors[__GL_FRONTFACE].g = g;
    vx->colors[__GL_FRONTFACE].b = b;
    vx->colors[__GL_FRONTFACE].a = a;
    (*vertex)(gc, vx);
}

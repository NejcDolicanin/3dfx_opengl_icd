/*
** si_dl_table.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/dl_table.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 690bf4f0 -- __glim_ListBase */
void __stdcall __glim_ListBase(GLuint base)
{ 
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    gc->state.list.listBase = base;
}

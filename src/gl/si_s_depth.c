/*
** si_s_depth.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_depth.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690c4210 -- StoreNEVER24: StoreNEVER16 (s_depth.c) as entries 16 and 24 of the depth store table 690f5264 (the
** ICD's 24-bit set) */
GLboolean __fastcall StoreNEVER24(__GLdepthBufferICD *fb,
                            GLint x, GLint y, __GLzValue z)
{
    return GL_FALSE;
}

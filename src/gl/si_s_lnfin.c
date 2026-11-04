/*
** si_s_lnfin.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_lnfin.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 69095670 -- __gl1stLinesVertex */
void __fastcall __gl1stLinesVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    gc->vertex.v0 = v0 + 1;
    gc->procs.vertex = gc->procs.vertex2ndLines;
}

/* 69095b80 -- FirstLLoopVertex */
void __fastcall FirstLLoopVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    gc->vertex.v0 = v0 + 1;
    gc->procs.vertex = gc->procs.vertex2ndLLoop;
}

/* 69095710 -- __gl2ndLinesVertex */
void __fastcall __gl2ndLinesVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    gc->line.notResetStipple = GL_FALSE;

    gc->vertex.v0 = v0 - 1;
    gc->procs.vertex = __gl1stLinesVertex;

    (*gc->procs.clipLine)(gc, v0 - 1, v0);
}

/*
** si_s_pgfin.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_pgfin.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 69094af0 -- SecondTrianglesVertex */
void __fastcall SecondTrianglesVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    v0->flagBits |= gc->state.current.edgeTag;
    gc->vertex.v0 = v0 + 1;
    gc->procs.vertex = gc->procs.vertex3rdTris;
}

/* 690951f0 -- ThirdQuadsVertex */
void __fastcall ThirdQuadsVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    v0->flagBits |= gc->state.current.edgeTag;
    gc->vertex.v0 = v0 + 1;
    gc->procs.vertex = gc->procs.vertex4thQuads;
}

/* 690951c0 -- SecondQuadsVertex */
void __fastcall SecondQuadsVertex(__GLcontext *gc, __GLvertexICD *v0)
{
    v0->flagBits |= gc->state.current.edgeTag;
    gc->vertex.v0 = v0 + 1;
    gc->procs.vertex = ThirdQuadsVertex;
}

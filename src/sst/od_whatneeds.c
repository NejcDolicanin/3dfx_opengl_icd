/*
** od_whatneeds.c -- 6905b610, what_needs of the 3dfx vertex-array code generator (GEOM_OG.C).
** The original compiled it without optimisation (/Od).
*/
#include "wglpriv.h"

/* 6905b610 -- what_needs (GEOM_OG.C): the front-face needs | the vertex needs, the bits the generated
** vertex routines are built for (hwGenVertexArrayFill, key word 20 of make_key).  The ancestor cleared them
** for two-sided / slow lighting; here the mode flags are still fetched but not used. */
#pragma optimize("", off)
GLuint __fastcall what_needs(__GLcontext *gc)
{
  struct {                   /* the /Od frame [ebp-0x8, ebp) */
    GLuint needs;            /* ebp-0x8 */
    GLuint modeFlags;        /* ebp-0x4  unused */
  } od_f;

  od_f.needs = (*gc->vertex.getFaceNeeds0)(gc) | (*gc->vertex.getNeeds)(gc);
  od_f.modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
  return od_f.needs;
}
#pragma optimize("", on)

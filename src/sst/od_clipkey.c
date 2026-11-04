/*
** od_clipkey.c -- og_clipInterpKey, the cache key of the clip-interpolation code generator (og_buildClipInterp).
** The original compiled it without optimisation (/Od).
*/
#include "wglpriv.h"
#include "glsi.h"

/* 6905c180 -- og_clipInterpKey: the one-dword bufRec[4] key (keySize 4, wgldev.c) of og_buildClipInterp's routine:
** bit 2 fog, bit 1 hwcx.nonUnityQ, bit 0 smooth shading, then one bit per TMU (textureEnabled[unit])
** shifted in below them */
#pragma optimize("", off)
void __fastcall og_clipInterpKey(__GLcontext *gc, GLuint *key)
{
  struct {                   /* the /Od frame [ebp-0x8, ebp) */
    GLint unit;              /* ebp-0x8 */
    __GLcontext *gc2;        /* ebp-0x4  a copy of gc the nonUnityQ test reads through */
  } od_f;

  od_f.gc2 = gc;
  *key = ((gc->state.enables.general & __GL_FOG_ENABLE) ? 4 : 0)
       | (od_f.gc2->hwcx.nonUnityQ ? 2 : 0)
       | (gc->state.light.shadingModel == GL_SMOOTH);
  for (od_f.unit = 0; od_f.unit < gc->fx.numTMUs; od_f.unit++) {
    *key = *key << 1;
    *key = *key | gc->texture.textureEnabled[od_f.unit];
  }
}
#pragma optimize("", on)

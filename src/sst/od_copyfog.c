/*
** od_copyfog.c -- CopyFogCoord, the fog-coordinate copy of the vertex-array code generator (an ICD
** addition next to GEOM_OG.C's CopyNormal).
** The original compiled it without optimisation (/Od).
*/
#include "wglpriv.h"
#include "og.h"


/* 69056e00 -- CopyFogCoord: emit vertex->fogCoord (edi + 0xa0) = 1.0 - the fog coordinate at [eax] (the ICD keeps
** fog coordinates as 1 - c, as glFogCoordfEXT does); GL_FLOAT loads directly, other types through
** conv_load.  The element size is computed and never used. */
#pragma optimize("", off)
GLubyte *__fastcall CopyFogCoord(__GLcontext *gc, GLubyte *pc, GLenum type, OgOperand *work)
{
  struct {                   /* the /Od frame [ebp-0x4, ebp) */
    GLint isize;             /* ebp-0x4  unused */
  } od_f;

  switch (type) {
  case GL_FLOAT:
    od_f.isize = 4;
    break;
  case GL_DOUBLE:
    od_f.isize = 8;
    break;
  }
  EMIT(og_fld(gc, og_mem(gc, IMM(&gc->constants.one), NULL, NULL, 1)));
  if (type == GL_FLOAT) {
    EMIT(og_fld(gc, og_mem(gc, NULL, R(eax), NULL, 1)));
  } else {
    pc = conv_load(gc, pc, og_mem(gc, NULL, R(eax), NULL, 1), type, work);
  }
  EMIT(og_fsubp(gc, R(st1), R(st0)));
  EMIT(og_fstp(gc, og_mem(gc, IMM(0xa0), R(edi), NULL, 1)));      /* vertex fogCoord */
  return pc;
}
#pragma optimize("", on)

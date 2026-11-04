/*
** od_doload.c -- 69055860, do_load of the 3dfx vertex-array code generator (GEOM_OG.C).
** The original compiled it without optimisation (/Od).
*/
#include "wglpriv.h"
#include "og.h"


/* 69055860 -- do_load (GEOM_OG.C): emit the x87 load of one coordinate component (vertex and texture
** arrays) at src: a short goes through ebx and the dword scratch; no default case (the ancestor
** asserted) */
#pragma optimize("", off)
GLubyte *__fastcall do_load(__GLcontext *gc, GLubyte *pc, OgOperand *src, GLenum type, OgOperand *scratch)
{
  switch (type) {
  case GL_SHORT:
    EMIT(og_movsx16(gc, R(ebx), src));
    EMIT(og_mov(gc, scratch, R(ebx)));
    EMIT(og_fild(gc, scratch));
    break;
  case GL_INT:
    EMIT(og_fild(gc, src));
    break;
  case GL_FLOAT:
    EMIT(og_fld(gc, src));
    break;
  case GL_DOUBLE:
    EMIT(og_fld64(gc, src));
  }
  return pc;
}
#pragma optimize("", on)

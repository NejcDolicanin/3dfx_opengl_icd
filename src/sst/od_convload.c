/*
** od_convload.c -- 69056560, conv_load of the 3dfx vertex-array code generator (GEOM_OG.C).
** The original compiled it without optimisation (/Od).
*/
#include "wglpriv.h"
#include "og.h"


/* fpk (GEOM_OG.C): a memory operand holding the float constant v (og_fpk takes no EDX argument) */

/* 69056560 -- conv_load (GEOM_OG.C): emit the x87 load of one colour / normal component at src, a signed
** integer c as 2c + 1 (the caller's FAC scale finishes the GL conversion), through ebx and the dword
** `work` where x87 cannot load the type directly.  GL_UNSIGNED_BYTE loads bl only: the caller has
** cleared ebx.  No default case (the ancestor asserted). */
#pragma optimize("", off)
GLubyte *__fastcall conv_load(__GLcontext *gc, GLubyte *pc, OgOperand *src, GLenum type, OgOperand *work)
{
  switch (type) {
  case GL_UNSIGNED_BYTE:
    EMIT(og_mov(gc, R(bl), src));
    EMIT(og_mov(gc, work, R(ebx)));
    EMIT(og_fild(gc, work));
    break;
  case GL_BYTE:
    EMIT(og_movsx8(gc, R(ebx), src));
    EMIT(og_add(gc, R(ebx), R(ebx)));
    EMIT(og_inc(gc, R(ebx)));
    EMIT(og_mov(gc, work, R(ebx)));
    EMIT(og_fild(gc, work));
    break;
  case GL_UNSIGNED_SHORT:
    EMIT(og_movzx16(gc, R(ebx), src));
    EMIT(og_mov(gc, work, R(ebx)));
    EMIT(og_fild(gc, work));
    break;
  case GL_SHORT:
    EMIT(og_movsx16(gc, R(ebx), src));
    EMIT(og_add(gc, R(ebx), R(ebx)));
    EMIT(og_inc(gc, R(ebx)));
    EMIT(og_mov(gc, work, R(ebx)));
    EMIT(og_fild(gc, work));
    break;
  case GL_UNSIGNED_INT:         /* zero-extended to a qword in work (lea ecx, work) */
    EMIT(og_lea(gc, R(ecx), work));
    EMIT(og_mov(gc, R(ebx), src));
    EMIT(og_mov(gc, og_mem(gc, NULL, R(ecx), NULL, 1), R(ebx)));
    EMIT(og_mov(gc, og_mem(gc, IMM(4), R(ecx), NULL, 1), IMM(0)));
    EMIT(og_fild64(gc, og_mem(gc, NULL, R(ecx), NULL, 1)));
    break;
  case GL_INT:                  /* 2c + 1 overflows 32 bits: done on the FPU */
    EMIT(og_fild(gc, src));
    EMIT(og_fmul(gc, FPK(2.0)));
    EMIT(og_fadd(gc, FPK(1.0)));
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

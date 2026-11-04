/*
** od_copynormal.c -- 69056a60, CopyNormal of the 3dfx vertex-array code generator (GEOM_OG.C).
** The original compiled it without optimisation (/Od).
*/
#include "wglpriv.h"
#include "icdprotos.h"
#include "og.h"

/* fpk (GEOM_OG.C): a memory operand holding the float constant v (og_fpk takes no EDX argument) */
/* vx(M) (GEOM_OG.C): the offset of a vertex field, offsetof as the /Od code computes it */
#define VX(m)           ((GLint)&((__GLvertexICD *)0)->m)

/* 69056a60 -- CopyNormal (GEOM_OG.C): emit the copy of the normal at [eax] (array type `type`) to the
** vertex at edi: three dword moves through ebx for GL_FLOAT, else conv_load + FAC scale per component.
** hwGenVertexArrayFill passes an immediate operand as `work`, which og_encModrm cannot encode: the GL_BYTE /
** GL_SHORT loads (mov work, ebx; fild work) come out broken (original bug). */
#pragma optimize("", off)
GLubyte *__fastcall CopyNormal(__GLcontext *gc, GLubyte *pc, GLenum type, OgOperand *work)
{
  struct {                   /* the /Od frame [ebp-0x8, ebp): both addresses go to calc_size_and_scale */
    GLuint isize;            /* ebp-0x8 */
    GLfloat scale;           /* ebp-0x4 */
  } od_f;

  calc_size_and_scale(&od_f.isize, &od_f.scale, type);
  if (type == GL_FLOAT) {
    EMIT(og_mov(gc, R(ebx), og_mem(gc, NULL, R(eax), NULL, 1)));
    EMIT(og_mov(gc, og_mem(gc, IMM(VX(normal.x)), R(edi), NULL, 1), R(ebx)));
    EMIT(og_mov(gc, R(ebx), og_mem(gc, IMM(4), R(eax), NULL, 1)));
    EMIT(og_mov(gc, og_mem(gc, IMM(VX(normal.y)), R(edi), NULL, 1), R(ebx)));
    EMIT(og_mov(gc, R(ebx), og_mem(gc, IMM(8), R(eax), NULL, 1)));
    EMIT(og_mov(gc, og_mem(gc, IMM(VX(normal.z)), R(edi), NULL, 1), R(ebx)));
  } else {
    pc = conv_load(gc, pc, og_mem(gc, NULL, R(eax), NULL, 1), type, work);
    EMIT(og_fmul(gc, FPK(od_f.scale)));
    pc = conv_load(gc, pc, og_mem(gc, IMM(od_f.isize), R(eax), NULL, 1), type, work);
    EMIT(og_fmul(gc, FPK(od_f.scale)));
    pc = conv_load(gc, pc, og_mem(gc, IMM(od_f.isize << 1), R(eax), NULL, 1), type, work);
    EMIT(og_fmul(gc, FPK(od_f.scale)));
    EMIT(og_fxch(gc, R(st2)));
    EMIT(og_fstp(gc, og_mem(gc, IMM(VX(normal.x)), R(edi), NULL, 1)));
    EMIT(og_fstp(gc, og_mem(gc, IMM(VX(normal.y)), R(edi), NULL, 1)));
    EMIT(og_fstp(gc, og_mem(gc, IMM(VX(normal.z)), R(edi), NULL, 1)));
  }
  return pc;
}
#pragma optimize("", on)

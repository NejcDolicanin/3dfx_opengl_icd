/*
** od_copycoord.c -- 690550d0, CopyCoord of the 3dfx vertex-array code generator (GEOM_OG.C).
** The original compiled it without optimisation (/Od).
*/
#include "wglpriv.h"
#include "og.h"

#define FP_ZERO 0x00000000      /* IEEE 0.0f as a dword */
#define FP_ONE  0x3f800000      /* IEEE 1.0f as a dword */

/* 690550d0 -- CopyCoord (GEOM_OG.C): emit the copy of a `size`-component coordinate of `type` at [eax]
** to the four floats at edi + vtxOffset (obj 0x80, texture[unit] 0x20 + unit * 0x10), filling the
** missing ones with (0, 0, 1).  GL_FLOAT is copied through ebx without the FPU; the ICD passes the
** destination offset instead of the ancestor's four operands. */
#pragma optimize("", off)
GLubyte *__fastcall CopyCoord(__GLcontext *gc, GLubyte *pc, GLenum type, GLuint size, GLint vtxOffset)
{
  struct {                   /* the /Od frame [ebp-0x14, ebp) */
    OgOperand *t3;           /* ebp-0x14 */
    GLint isize;             /* ebp-0x10  bytes per input component */
    OgOperand *t2;           /* ebp-0xc */
    OgOperand *t1;           /* ebp-0x8 */
    OgOperand *t0;           /* ebp-0x4 */
  } od_f;

  od_f.t0 = og_memBase(gc, IMM(vtxOffset), R(edi));
  od_f.t1 = og_memBase(gc, IMM(vtxOffset + 4), R(edi));
  od_f.t2 = og_memBase(gc, IMM(vtxOffset + 8), R(edi));
  od_f.t3 = og_memBase(gc, IMM(vtxOffset + 0xc), R(edi));
  switch (type) {
  case GL_SHORT:
    od_f.isize = 2;
    break;
  case GL_INT:
    od_f.isize = 4;
    break;
  case GL_FLOAT:
    od_f.isize = 4;
    break;
  case GL_DOUBLE:
    od_f.isize = 8;
  }
  if (type == GL_FLOAT) {
    /* no FPU work */
    if (size >= 1) {
      EMIT(og_mov(gc, R(ebx), og_mem(gc, NULL, R(eax), NULL, 1)));
      EMIT(og_mov(gc, od_f.t0, R(ebx)));
    }
    if (size >= 2) {
      EMIT(og_mov(gc, R(ebx), og_mem(gc, IMM(4), R(eax), NULL, 1)));
      EMIT(og_mov(gc, od_f.t1, R(ebx)));
    } else {
      EMIT(og_mov(gc, od_f.t1, IMM(FP_ZERO)));
    }
    if (size >= 3) {
      EMIT(og_mov(gc, R(ebx), og_mem(gc, IMM(8), R(eax), NULL, 1)));
      EMIT(og_mov(gc, od_f.t2, R(ebx)));
    } else {
      EMIT(og_mov(gc, od_f.t2, IMM(FP_ZERO)));
    }
    if (size >= 4) {
      EMIT(og_mov(gc, R(ebx), og_mem(gc, IMM(0xc), R(eax), NULL, 1)));
      EMIT(og_mov(gc, od_f.t3, R(ebx)));
    } else {
      EMIT(og_mov(gc, od_f.t3, IMM(FP_ONE)));
    }
  } else {
    switch (size) {
    case 1:
      pc = do_load(gc, pc, og_mem(gc, NULL, R(eax), NULL, 1), type, od_f.t0);
      EMIT(og_mov(gc, R(ebx), IMM(FP_ZERO)));
      EMIT(og_mov(gc, od_f.t3, IMM(FP_ONE)));
      EMIT(og_mov(gc, od_f.t1, R(ebx)));
      EMIT(og_mov(gc, od_f.t2, R(ebx)));
      EMIT(og_fstp(gc, od_f.t0));
      break;
    case 2:
      pc = do_load(gc, pc, og_mem(gc, NULL, R(eax), NULL, 1), type, od_f.t0);
      pc = do_load(gc, pc, og_mem(gc, IMM(od_f.isize), R(eax), NULL, 1), type, od_f.t1);
      EMIT(og_fxch(gc, R(st1)));
      EMIT(og_fstp(gc, od_f.t0));
      EMIT(og_fstp(gc, od_f.t1));
      EMIT(og_mov(gc, od_f.t2, IMM(FP_ZERO)));
      EMIT(og_mov(gc, od_f.t3, IMM(FP_ONE)));
      break;
    case 3:
      pc = do_load(gc, pc, og_mem(gc, NULL, R(eax), NULL, 1), type, od_f.t0);
      pc = do_load(gc, pc, og_mem(gc, IMM(od_f.isize), R(eax), NULL, 1), type, od_f.t1);
      pc = do_load(gc, pc, og_mem(gc, IMM(od_f.isize << 1), R(eax), NULL, 1), type, od_f.t2);
      EMIT(og_fxch(gc, R(st2)));
      EMIT(og_fstp(gc, od_f.t0));
      EMIT(og_fstp(gc, od_f.t1));
      EMIT(og_fstp(gc, od_f.t2));
      EMIT(og_mov(gc, od_f.t3, IMM(FP_ONE)));
      break;
    case 4:
      pc = do_load(gc, pc, og_mem(gc, NULL, R(eax), NULL, 1), type, od_f.t0);
      pc = do_load(gc, pc, og_mem(gc, IMM(od_f.isize), R(eax), NULL, 1), type, od_f.t1);
      pc = do_load(gc, pc, og_mem(gc, IMM(od_f.isize << 1), R(eax), NULL, 1), type, od_f.t2);
      pc = do_load(gc, pc, og_mem(gc, IMM(od_f.isize * 3), R(eax), NULL, 1), type, od_f.t3);
      EMIT(og_fxch(gc, R(st3)));
      EMIT(og_fstp(gc, od_f.t0));    /* st: x, z, y, w after the exchange */
      EMIT(og_fstp(gc, od_f.t2));
      EMIT(og_fstp(gc, od_f.t1));
      EMIT(og_fstp(gc, od_f.t3));
    }
  }
  return pc;
}
#pragma optimize("", on)

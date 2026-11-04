/*
** od_copycolor.c -- 69055970, CopyColor of the 3dfx vertex-array code generator (GEOM_OG.C).
** The original compiled it without optimisation (/Od).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "glshade.h"
#include "og.h"
#include "icdprotos.h"

/* fpk (GEOM_OG.C): a memory operand holding the float constant v (og_fpk takes no EDX argument) */
/* vx(M) (GEOM_OG.C): the offset of a vertex field, offsetof as the /Od code computes it */
#define VX(m)           ((GLint)&((__GLvertexICD *)0)->m)
/* getModeFlags bit the alpha store depends on (the ancestor's __GL_SHADE_BLEND | __GL_SHADE_ALPHA_TEST) */

/* 69055970 -- CopyColor (GEOM_OG.C): emit the copy of the colour (colour index in CI mode) at [eax],
** `size` components of `type`, to vertex->colors[0] at edi, scaled to the colour buffer's range like
** __glClampAndScaleColorf.  Unsigned types are stored straight into the vertex; the others go through
** the scratch operands rr..aa and are clamped (upper bound only, as integers) on the way in.  ICD
** additions: a 4 x GL_UNSIGNED_BYTE colour is swizzled straight into vertex->packedColor when the
** routine packs colours itself (needs & __GL_HAS_PACKED_COLOR without __GL_HAS_FRONT_COLOR), else
** with __GL_HAS_PACKED_COLOR the routine calls hwPackVertexColor(gc, vertex) afterwards; the default
** alpha is alphaScale instead of 1.0. */
#pragma optimize("", off)
GLubyte *__fastcall CopyColor(__GLcontext *gc, GLubyte *pc, GLenum type, GLuint size, OgOperand *work,
                                 OgOperand *rr, OgOperand *gg, OgOperand *bb, OgOperand *aa, GLuint needs)
{
  struct {                   /* the /Od frame [ebp-0x2c, ebp) */
    union {
      GLfloat f;
      GLint i;
    } u[4];                  /* ebp-0x2c  the r, g, b, a scales as clamp bounds */
    GLboolean mustClamp;     /* ebp-0x1c */
    GLubyte pad1d[3];
    GLfloat green_s;         /* ebp-0x18 */
    GLuint isize;            /* ebp-0x14 */
    GLfloat red_s;           /* ebp-0x10 */
    GLfloat blue_s;          /* ebp-0xc */
    GLfloat alpha_s;         /* ebp-0x8 */
    GLfloat scale;           /* ebp-0x4 */
  } od_f;

  od_f.mustClamp = (type != GL_UNSIGNED_BYTE && type != GL_UNSIGNED_SHORT && type != GL_UNSIGNED_INT);
  if ((needs & __GL_HAS_PACKED_COLOR) && !(needs & __GL_HAS_FRONT_COLOR) && type == GL_UNSIGNED_BYTE) {
    /* RGBA bytes -> 0xAARRGGBB: swap R and B */
    EMIT(og_mov(gc, R(ebx), og_mem(gc, NULL, R(eax), NULL, 1)));
    EMIT(og_mov(gc, R(ecx), R(ebx)));
    EMIT(og_and(gc, R(ecx), IMM(0xff00ff)));
    EMIT(og_and(gc, R(ebx), IMM(0xff00ff00)));
    EMIT(og_rol(gc, R(ecx), IMM(0x10)));
    EMIT(og_or(gc, R(ecx), R(ebx)));
    EMIT(og_mov(gc, og_mem(gc, IMM(0x14), R(edi), NULL, 1), R(ecx)));    /* vertex packedColor */
    return pc;
  } else {
    /* no clamping: write the results straight into the vertex */
    if (!od_f.mustClamp) {
      rr = og_memBase(gc, IMM(VX(colors[0].r)), R(edi));
      gg = og_memBase(gc, IMM(VX(colors[0].g)), R(edi));
      bb = og_memBase(gc, IMM(VX(colors[0].b)), R(edi));
      aa = og_memBase(gc, IMM(VX(colors[0].a)), R(edi));
    }
    if (gc->modes.colorIndexMode) {
      size = 1;
    }
    calc_size_and_scale(&od_f.isize, &od_f.scale, type);
    od_f.red_s = od_f.scale * gc->constants.redScale;
    od_f.green_s = od_f.scale * gc->constants.greenScale;
    od_f.blue_s = od_f.scale * gc->constants.blueScale;
    od_f.alpha_s = od_f.scale * gc->constants.alphaScale;
    if (type == GL_UNSIGNED_BYTE) {
      EMIT(og_xor(gc, R(ebx), R(ebx)));     /* conv_load loads bl only */
    }
    switch (size) {
    case 1:
      /* colour index: a straight copy */
      pc = conv_load(gc, pc, og_mem(gc, NULL, R(eax), NULL, 1), type, work);
      EMIT(og_fstp(gc, og_mem(gc, IMM(VX(colors[0].r)), R(edi), NULL, 1)));
      break;
    case 3:
      /* load, scale, store in the scratch operands */
      pc = conv_load(gc, pc, og_mem(gc, NULL, R(eax), NULL, 1), type, work);
      pc = conv_load(gc, pc, og_mem(gc, IMM(od_f.isize), R(eax), NULL, 1), type, work);
      pc = conv_load(gc, pc, og_mem(gc, IMM(od_f.isize << 1), R(eax), NULL, 1), type, work);
      EMIT(og_fld(gc, FPK(od_f.red_s)));
      EMIT(og_fmulp(gc, R(st3), R(st0)));
      EMIT(og_fld(gc, FPK(od_f.green_s)));
      EMIT(og_fmulp(gc, R(st2), R(st0)));
      EMIT(og_fld(gc, FPK(od_f.blue_s)));
      EMIT(og_fmulp(gc, R(st1), R(st0)));
      EMIT(og_fxch(gc, R(st2)));
      EMIT(og_fstp(gc, rr));
      EMIT(og_fstp(gc, gg));
      EMIT(og_fstp(gc, bb));
      break;
    case 4:
      pc = conv_load(gc, pc, og_mem(gc, NULL, R(eax), NULL, 1), type, work);
      EMIT(og_fmul(gc, FPK(od_f.red_s)));
      pc = conv_load(gc, pc, og_mem(gc, IMM(od_f.isize), R(eax), NULL, 1), type, work);
      EMIT(og_fmul(gc, FPK(od_f.green_s)));
      pc = conv_load(gc, pc, og_mem(gc, IMM(od_f.isize << 1), R(eax), NULL, 1), type, work);
      EMIT(og_fmul(gc, FPK(od_f.blue_s)));
      pc = conv_load(gc, pc, og_mem(gc, IMM(od_f.isize * 3), R(eax), NULL, 1), type, work);
      EMIT(og_fmul(gc, FPK(od_f.alpha_s)));
      EMIT(og_fxch(gc, R(st3)));
      EMIT(og_fstp(gc, rr));
      EMIT(og_fstp(gc, bb));
      EMIT(og_fstp(gc, gg));
      EMIT(og_fstp(gc, aa));
      break;
    }
    /* the scratch values are scaled; clamp them into the vertex (unsigned types cannot overflow) */
    if (size >= 3 && od_f.mustClamp) {
      od_f.u[0].f = gc->constants.redScale;
      od_f.u[1].f = gc->constants.greenScale;
      od_f.u[2].f = gc->constants.blueScale;
      od_f.u[3].f = gc->constants.alphaScale;
      EMIT(og_mov(gc, R(ebx), rr));
      pc = og_minSigned(gc, pc, R(ebx), IMM(od_f.u[0].i));
      EMIT(og_mov(gc, og_mem(gc, IMM(VX(colors[0].r)), R(edi), NULL, 1), R(ebx)));
      EMIT(og_mov(gc, R(ebx), gg));
      pc = og_minSigned(gc, pc, R(ebx), IMM(od_f.u[1].i));
      EMIT(og_mov(gc, og_mem(gc, IMM(VX(colors[0].g)), R(edi), NULL, 1), R(ebx)));
      EMIT(og_mov(gc, R(ebx), bb));
      pc = og_minSigned(gc, pc, R(ebx), IMM(od_f.u[2].i));
      EMIT(og_mov(gc, og_mem(gc, IMM(VX(colors[0].b)), R(edi), NULL, 1), R(ebx)));
      if ((*gc->polygon.shader.getModeFlags)(gc) & __GL_SHADE_NEEDS_ALPHA) {
        if (size == 4) {
          EMIT(og_mov(gc, R(ebx), aa));
          pc = og_minSigned(gc, pc, R(ebx), IMM(od_f.u[3].i));
          EMIT(og_mov(gc, og_mem(gc, IMM(VX(colors[0].a)), R(edi), NULL, 1), R(ebx)));
        } else {
          EMIT(og_mov(gc, R(ebx), IMM(od_f.u[3].i)));
          EMIT(og_mov(gc, og_mem(gc, IMM(VX(colors[0].a)), R(edi), NULL, 1), R(ebx)));
        }
      }
    } else {
      if (size == 3) {
        if ((*gc->polygon.shader.getModeFlags)(gc) & __GL_SHADE_NEEDS_ALPHA) {
          /* original bug: the integer value of alphaScale (255 -> 3.6e-43f), not its float bits as
          ** in the clamped path above */
          EMIT(og_mov(gc, R(ebx), IMM((GLint)gc->constants.alphaScale)));
          EMIT(og_mov(gc, og_mem(gc, IMM(VX(colors[0].a)), R(edi), NULL, 1), R(ebx)));
        }
      }
    }
    if (needs & __GL_HAS_PACKED_COLOR) {
      /* hwPackVertexColor(gc, vertex), keeping eax / edx */
      EMIT(og_push(gc, R(eax)));
      EMIT(og_push(gc, R(edx)));
      EMIT(og_mov(gc, R(ecx), IMM(gc)));
      EMIT(og_mov(gc, R(edx), R(edi)));
      pc = og_callRel32Gen(gc, pc, (GLuint)hwPackVertexColor);
      EMIT(og_pop(gc, R(edx)));
      EMIT(og_pop(gc, R(eax)));
    }
  }
  return pc;
}
#pragma optimize("", on)

/*
** od_scaleby.c -- og_scaleby, the ICD's scaleby of the 3dfx vertex-array code generator (GEOM_OG.C).
** The original compiled it without optimisation (/Od).
*/
#include "wglpriv.h"
#include "og.h"

/* 69055020 -- og_scaleby (GEOM_OG.C scaleby), rewritten: emit `reg *= n` for the immediate operand n (an array
** stride): nothing for n <= 1, `shl reg, log2 n` for a power of two, else `imul reg, n`.  The ancestor
** built the product from shifts and adds. */
#pragma optimize("", off)
GLubyte *__fastcall og_scaleby(__GLcontext *gc, GLubyte *pc, OgOperand *reg, OgOperand *n)
{
  struct {                   /* the /Od frame [ebp-0x8, ebp) */
    GLint shifts;            /* ebp-0x8 */
    GLint N;                 /* ebp-0x4 */
  } od_f;

  od_f.N = n->val;
  if (od_f.N <= 1) {
    /* nothing to scale */
  } else {
    if ((od_f.N & -od_f.N) == od_f.N) {
      for (od_f.shifts = 1; (od_f.N & (1 << od_f.shifts)) == 0; od_f.shifts++)
        ;
      EMIT(og_shl(gc, reg, IMM(od_f.shifts)));
    } else {
      EMIT(og_imul(gc, reg, IMM(od_f.N)));
    }
  }
  return pc;
}
#pragma optimize("", on)

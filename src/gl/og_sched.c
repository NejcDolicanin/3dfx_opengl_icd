/*
** og_sched.c -- the scheduling rules of the og_* code generator (3dfx OG.C): the registers an instruction reads
** and writes, the dependency and the Pentium U / V pairing tests og_optimizeBlock (og_util.c) uses.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "glsitypes.h"
#include <stdlib.h>
#include "og.h"

/* ---- the code generator's scheduler rules (3dfx OG.C; og_util.c og_optimizeBlock uses them).  A register use
** mask: bits 0..7 the integer registers, 8 << n the MMX registers, 0x10000 the flags (OG.C USE_FLAGS).
** Opcodes (og_encode.c): 0 mov, 6 push, 7 pop, 0x0a..0x17 Jcc, 0x1e cmp, 0x2d test, 0x56 movd, 0x57 movq ---- */
#define OG_USE_FLAGS        0x10000
extern GLuint __fastcall og_memUse(OgOperand *o);        /* og_util.c takes the OgMem view */
extern GLuint __fastcall og_isBinop(OgInsn *insn);           /* og_isBinop (og_util.c) */
extern GLuint __fastcall og_isMmxShiftOp(OgInsn *insn);           /* og_isMmxShiftOp (og_util.c) */

extern GLuint __fastcall og_readUse(OgInsn *insn);
extern GLuint __fastcall og_writeUse(OgInsn *insn);
/* 690e9330 -- og_dependsOn (OG.C depends_on): does insn `a` read a register insn `b` writes */
int __fastcall og_dependsOn(OgInsn *a, OgInsn *b)
{
    int r;

    if (a && b) {
	r = (og_readUse(a) & og_writeUse(b)) != 0;
	return r;
    }
    return 0;
}

extern GLuint __fastcall og_whatUse(OgOperand *o);
/* 690e9360 -- og_readUse (OG.C read_use): the registers an instruction reads: both operands' address registers,
** plus the source (mov / movd / movq: b; push: a; other two-operand and MMX ops: both); Jcc reads the flags */
GLuint __fastcall og_readUse(OgInsn *insn)
{
    GLuint regs, mask;

    regs = og_memUse(insn->a);
    regs |= og_memUse(insn->b);
    switch (insn->op) {
      case 0x00: case 0x56: case 0x57:          /* mov, movd, movq */
	return og_whatUse(insn->b) | regs;
      case 0x06:                                /* push */
	return og_whatUse(insn->a) | regs;
      default:
	if (og_isBinop(insn)) {
	    mask = og_whatUse(insn->a) | og_whatUse(insn->b);
	    return mask | regs;
	}
	if (insn->op >= 0x0a && insn->op <= 0x17)   /* JA .. JNS */
	    return OG_USE_FLAGS;
	return 0;
    }
}

/* 690e9450 -- og_whatUse (OG.C what_use): the use bit of a register operand (an 8-bit register uses its 32-bit one) */
GLuint __fastcall og_whatUse(OgOperand *o)
{
    if (o == NULL)
	return 0;
    switch (o->kind) {
      case OG_REG8:
	return 1 << (o->val & 3);
      case OG_REG32:
	return 1 << o->val;
      case OG_MMX:
	return 8 << o->val;
      default:
	return 0;
    }
}

/* 690e94c0 -- og_writeUse (OG.C write_use): the registers an instruction writes: its first operand (mov, pop,
** movd, movq and the two-operand / MMX ops); cmp and test write the flags */
GLuint __fastcall og_writeUse(OgInsn *insn)
{
    switch (insn->op) {
      case 0x00: case 0x07: case 0x56: case 0x57:   /* mov, pop, movd, movq */
	break;
      case 0x1e: case 0x2d:                     /* cmp, test */
	return OG_USE_FLAGS;
      default:
	if (og_isBinop(insn))
	    break;
	return 0;
    }
    return og_whatUse(insn->a);
}

/* 690e9560 -- og_cDependsOn (OG.C c_depends_on): og_dependsOn ignoring the flags (cmp / test pair with the Jcc) */
int __fastcall og_cDependsOn(OgInsn *a, OgInsn *b)
{
    int r;

    if (a && b) {
	r = (og_readUse(a) & og_writeUse(b) & ~OG_USE_FLAGS) != 0;
	return r;
    }
    return 0;
}

/* 690e95e0 -- og_canPair (OG.C can_pair): may `a` (U pipe) and `b` (V pipe) pair: not both MMX shifts, no common
** written register, and `b` reads nothing `a` writes (flags aside) */
int __fastcall og_canPair(OgInsn *a, OgInsn *b)
{
    int r;

    if (a == NULL || b == NULL)
	return 1;
    if (og_isMmxShiftOp(a) && og_isMmxShiftOp(b))
	return 0;
    if (og_writeUse(a) & og_writeUse(b))
	return 0;
    r = (og_writeUse(a) & og_readUse(b) & ~OG_USE_FLAGS) == 0;
    return r;
}

/*
** og_encode.c -- og_encode, the x86 instruction encoder of the vertex-program code generator (og_*):
** one OgInsn (op, operands a / b / c) -> machine code at pc.  Every case
** advances the running `pc` (the original keeps it in ESI), case bodies in the original's
** address order.
**
** Codegen notes: the helpers end in one `return pc` with if / else arms (separate returns move
** the blocks and turn `inc esi; mov eax, esi` into `lea`); a byte stored before a test whose
** operand was loaded first (call: E8, MMX shift: regOp) is written in both arms; fixups read
** label->f8 before storing fx->at.  In og_encode the 0F group 9f/a5/a8/... takes `insn->a`
** through a local only so VC6 does not merge it with the identical bodies of 0xc0 and the third
** group (the original keeps all four 0F-zero bodies apart).  regBase / regOp are pushed zero-extended (GLubyte), rmOp
** sign-extended (char).  og_encJmp (690e6f70), og_isImm8 (690e8a60) and og_reset (690e96f0) moved
** here from og_build.c / og_util.c.
*/

typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;

typedef struct OgOperand {
    int kind;                   /* 1 reg8, 3 reg32, 5 mmx, 0 immediate, 8 label, ... */
    int val;                    /* register number / immediate */
    int f8;                     /* label: its fixup chain */
    int fc;                     /* label: its address once placed */
} OgOperand;
typedef struct OgMem {           /* kind 6: [base + index * scale + disp] */
    int kind;
    OgOperand *disp;            /* an immediate operand, or 0 */
    int scale;
    OgOperand *base, *index;
} OgMem;
typedef struct OgInsn {         /* og.h OgInsn (the scheduler's next is at +0x14, not read here) */
    int kind;
    int op;
    OgOperand *a, *b, *c;
} OgInsn;
typedef struct OgFixup {
    int f0;
    int kind;
    int next;
    GLubyte *at;
} OgFixup;

int og_skipping;                                    /* 693c3cbc: set between ops 0xd8 and 0xd9 */
int __fastcall og_isReg32(OgOperand *o);
int __fastcall og_isReg8(OgOperand *o);
int __fastcall og_isMmx(OgOperand *o);
int __fastcall og_isImm(OgOperand *o);
int __fastcall og_isImm8(OgOperand *o);
GLubyte *__fastcall og_encModrm(GLubyte *pc, int reg, OgOperand *rm);
GLubyte *__fastcall og_encAlu(GLubyte *pc, int immOp, char rmOp, GLubyte regOp, int ext, OgOperand *a, OgOperand *b);
GLubyte *__fastcall og_encJcc(void *ctx, GLubyte *pc, int cc, OgOperand *target);
GLubyte *__fastcall og_encJmp(void *ctx, GLubyte *pc, OgOperand *target);
GLubyte *__fastcall og_encX87(GLubyte *pc, int regOp, GLubyte regBase, GLubyte memOp, int ext, OgOperand *a);
GLubyte *__fastcall og_enc0F(GLubyte *pc, GLubyte op, OgOperand *a, OgOperand *b);
GLubyte *__fastcall og_encF30F(GLubyte *pc, GLubyte op, OgOperand *a, OgOperand *b);
GLubyte *__fastcall og_encMmxShift(GLubyte *pc, GLubyte regOp, GLubyte immOp, OgOperand *a, OgOperand *b);
GLubyte *__fastcall og_enc3DNow(GLubyte *pc, GLubyte op, OgOperand *a, OgOperand *b);
GLubyte *__fastcall og_encMovq(GLubyte *pc, OgOperand *a, OgOperand *b, GLubyte load, GLubyte store);
OgFixup *__fastcall og_newFixup(void *ctx);       /* og_build.c: (gc) -> OgLabel *; this file's own view (no og.h) */
GLuint __fastcall og_isReg(OgOperand *o);                 /* og_isReg8 || og_isReg32 */
extern OgOperand og_esp, og_ebp;
void __fastcall og_spoolCode(void *ctx, OgInsn *insn);
GLubyte *__fastcall og_optimizeBlock(void *ctx, GLubyte *pc);    /* og_util.c: ctx is the gc */

#include <stdlib.h>

/* 690e8a60 -- an immediate that fits a sign-extended byte */
int __fastcall og_isImm8(OgOperand *o)
{
    if (og_isImm(o) && o->val >= -0x80 && o->val < 0x80)
        return 1;
    return 0;
}

struct celem {
    int what;                   /* VALUE 0, reg8 1, reg16 2, reg32 3, fpreg 4, mmxreg 5, MEM 6, ... */
    union {
        int value;
        int reg32;
        struct {
            struct celem *disp;
            int scale;
            struct celem *reg1;
            struct celem *reg2;
        } mem;
    } spec;
};
#define reg8_W  1
#define reg32_W 3
#define MEM_W   6

/* 690e8a90 -- the modrm (+ sib, displacement) bytes for reg and the register / memory operand rm:
   3dfx's emit_modrm_code (SRC/RASTER/OG.C) on the celem layout, without its asserts */
GLubyte *__fastcall og_encModrm(GLubyte *cb, int regop, OgOperand *rm)
{
    struct celem *op = (struct celem *)rm;

    switch (op->what) {
    case reg32_W:
    case reg8_W:
        *cb++ = 0xc0 | (regop << 3) | (op->spec.reg32);
        break;
    case MEM_W:
        if (op->spec.mem.disp && !op->spec.mem.reg1 && !op->spec.mem.reg2) {
            *cb++ = 0x00 | (regop << 3) | 5;
            *((long *)cb)++ = op->spec.mem.disp->spec.value;
        } else if (!op->spec.mem.disp && op->spec.mem.reg1 && !op->spec.mem.reg2) {
            *cb++ = 0x00 | (regop << 3) | op->spec.mem.reg1->spec.reg32;
        } else if (op->spec.mem.disp && op->spec.mem.reg1 && !op->spec.mem.reg2 &&
                   op->spec.mem.reg1 != (struct celem *)&og_esp) {
            int disp;

            disp = op->spec.mem.disp->spec.value;

            if (abs(disp) >= 128) {
              *cb++ = 0x80 | (regop << 3) | op->spec.mem.reg1->spec.reg32;
              *((long *)cb)++ = op->spec.mem.disp->spec.value;
            } else {
              *cb++ = 0x40 | (regop << 3) | op->spec.mem.reg1->spec.reg32;
              *cb++ = op->spec.mem.disp->spec.value;
            }
        } else {
            static int scale_to_ss[9] = { 0, 0, 1, 1, 2, 2, 2, 2, 3 };    /* 690f5da0: log2 of a scale */
            int ss;
            int disp;

            if (!op->spec.mem.reg1 && op->spec.mem.reg2) {
              disp = op->spec.mem.disp ? op->spec.mem.disp->spec.value : 0;
              *cb++ = 0x00 | (regop << 3) | 4;

              /* Now for SIB */
              ss = scale_to_ss[op->spec.mem.scale];

              *cb++ =
                (ss << 6) |
                  (op->spec.mem.reg2->spec.reg32 << 3) |
                    5;
              *((long *)cb)++ = op->spec.mem.disp->spec.value;

            } else {
              int needDisp;

              disp = op->spec.mem.disp ? op->spec.mem.disp->spec.value : 0;
              needDisp = (disp != 0) || (op->spec.mem.reg1 == (struct celem *)&og_ebp);

              if (needDisp) {
                if (abs(disp) >= 128)
                  *cb++ = 0x80 | (regop << 3) | 4;
                else
                  *cb++ = 0x40 | (regop << 3) | 4;
              } else
                *cb++ = 0x00 | (regop << 3) | 4;

              /* Now for SIB */
              for (ss = 0; (1 << ss) < op->spec.mem.scale; ss++)
                ;

              *cb++ =
                (ss << 6) |
                  ((op->spec.mem.reg2 ? op->spec.mem.reg2->spec.reg32 : 4) << 3) |
                    (op->spec.mem.reg1->spec.reg32);


              if (needDisp) {
                if (abs(disp) >= 128)
                  *((long *)cb)++ = disp;
                else
                  *((signed char *)cb)++ = disp;
              }
            }
        }
    }
    return cb;
}
#undef scale_to_ss

#define is_imm(op)      og_isImm(op)
#define is_reg8(op)     og_isReg8(op)
#define is_reg32(op)    og_isReg32(op)
#define is_reg(op)      og_isReg(op)
/* 690e8c90 -- an ALU instruction: immOp /ext imm32 (the byte form one below for 8-bit
   registers), or rmOp / regOp with a register on either side.  3dfx's emit_arith_code
   (SRC/RASTER/OG.C; the final assert(0) is gone) */
GLubyte *__fastcall og_encAlu(GLubyte *cb, int imm, char r_rm, GLubyte rm_r, int second, OgOperand *op1, OgOperand *op2)
{
    if (is_imm(op2)) {
	if (is_reg8(op1))
	  *cb++ = imm - 1;
	else
	  *cb++ = imm;
	cb = og_encModrm(cb, second, op1);
	if (is_reg8(op1))
	  *cb++ = op2->val;
	else
	  *((long*)cb)++ = op2->val;
    } else if (is_reg(op2)) {
	if (is_reg32(op2))
	    *cb++ = rm_r;
	else
	    *cb++ = rm_r - 1;
	cb = og_encModrm(cb, op2->val, op1);
    } else if (is_reg(op1)) {
	if (is_reg32(op1))
	    *cb++ = r_rm;
	else
	    *cb++ = r_rm - 1;
	cb = og_encModrm(cb, op1->val, op2);
    }
    return cb;
}
#undef is_imm
#undef is_reg8
#undef is_reg32
#undef is_reg

/* 690e8da0 -- jcc: short or near to a bound label, else a fixup (near when label->f4) */
GLubyte *__fastcall og_encJcc(void *ctx, GLubyte *pc, int cc, OgOperand *label)
{
    OgFixup *fx;
    int ncc = cc + 0x10;                           /* the near form, 0F 8x */

    if (label->fc) {
        if (abs(label->fc - (int)pc) < 0x7f) {
            *pc++ = (GLubyte)cc;
            *pc = (GLubyte)label->fc - (GLubyte)pc - 1;
            return pc + 1;
        }
        *pc++ = 0xf;
        *pc++ = (GLubyte)ncc;
        *(int *)pc = label->fc - (int)pc - 4;
        return pc + 4;
    }
    if (label->val == 0)
        *pc = (GLubyte)cc;
    else {
        *pc++ = 0xf;
        *pc = (GLubyte)ncc;
    }
    pc++;
    fx = og_newFixup(ctx);
    fx->next = label->f8;
    fx->at = pc;
    fx->kind = label->val;
    label->f8 = (int)fx;
    return pc + (label->val ? 4 : 1);
}

/* 690e8e40 -- x87: regOp, regBase + i for st(i), else memOp /ext */
GLubyte *__fastcall og_encX87(GLubyte *pc, int regOp, GLubyte regBase, GLubyte memOp, int ext, OgOperand *a)
{
    if (a->kind == 4) {
        *pc++ = (GLubyte)regOp;
        *pc++ = regBase + (char)a->val;
    } else {
        *pc = memOp;
        pc = og_encModrm(pc + 1, ext, a);
    }
    return pc;
}

/* 690e8e80 -- 0F op, a = register, b = mmx register or memory */
GLubyte *__fastcall og_enc0F(GLubyte *pc, GLubyte op, OgOperand *a, OgOperand *b)
{
    *pc++ = 0xf;
    *pc++ = op;
    if (b->kind == 5)
        *pc++ = (GLubyte)((a->val | 0xf8) << 3) | (GLubyte)b->val;
    else
        pc = og_encModrm(pc, a->val, b);
    return pc;
}

/* 690e8ec0 -- the same with the F3 prefix */
GLubyte *__fastcall og_encF30F(GLubyte *pc, GLubyte op, OgOperand *a, OgOperand *b)
{
    *pc++ = 0xf3;
    *pc++ = 0xf;
    *pc++ = op;
    if (b->kind == 5)
        *pc++ = (GLubyte)((a->val | 0xf8) << 3) | (GLubyte)b->val;
    else
        pc = og_encModrm(pc, a->val, b);
    return pc;
}

/* 690e8f10 -- an MMX shift: by an immediate (immOp, /n from regOp's low bits) or by a register */
GLubyte *__fastcall og_encMmxShift(GLubyte *pc, GLubyte regOp, GLubyte immOp, OgOperand *a, OgOperand *b)
{
    *pc++ = 0xf;
    if (og_isImm(b)) {
        *pc++ = immOp;
        *pc++ = (regOp & 0xf8 | (GLubyte)a->val) | 0xc0;
        *pc++ = (GLubyte)b->val;
    } else if (b->kind == 5) {
        *pc++ = regOp;
        *pc++ = (GLubyte)((a->val | 0xf8) << 3) | (GLubyte)b->val;
    } else {
        *pc = regOp;
        return og_encModrm(pc + 1, a->val, b);
    }
    return pc;
}

/* 690e8f90 -- 3DNow: 0F 0F modrm op */
GLubyte *__fastcall og_enc3DNow(GLubyte *pc, GLubyte op, OgOperand *a, OgOperand *b)
{
    *pc++ = 0xf;
    *pc++ = 0xf;
    if (b->kind == 5)
        *pc++ = (GLubyte)((a->val | 0xf8) << 3) | (GLubyte)b->val;
    else
        pc = og_encModrm(pc, a->val, b);
    *pc++ = op;
    return pc;
}

/* 690e9670 -- movq / movd style: load (a = mmx register) or store */
GLubyte *__fastcall og_encMovq(GLubyte *pc, OgOperand *a, OgOperand *b, GLubyte load, GLubyte store)
{
    *pc++ = 0xf;
    if (og_isMmx(a)) {
        *pc++ = load;
        if (og_isMmx(b))
            *pc++ = (GLubyte)((a->val | 0xf8) << 3) | (GLubyte)b->val;
        else
            pc = og_encModrm(pc, a->val, b);
    } else {
        *pc = store;
        pc = og_encModrm(pc + 1, b->val, a);
    }
    return pc;
}

/* 690e6f70 -- jmp: short or near to a bound label, else near with a fixup */
GLubyte *__fastcall og_encJmp(void *ctx, GLubyte *pc, OgOperand *label)
{
    OgFixup *fx;

    if (label->fc) {
        if (abs(label->fc - (int)pc) <= 0x7f) {
            *pc++ = 0xeb;
            *pc = (GLubyte)label->fc - (GLubyte)pc - 1;
            return pc + 1;
        }
        *pc++ = 0xe9;
        *(int *)pc = label->fc - (int)pc - 4;
        return pc + 4;
    }
    *pc++ = 0xe9;
    fx = og_newFixup(ctx);
    fx->next = label->f8;
    fx->at = pc;
    fx->kind = 1;
    label->f8 = (int)fx;
    return pc + 4;
}

/* 690e7040 -- og_encode: encode the instruction insn as x86 machine code at pc and return the advanced pc --
** a switch on insn->op over integer ops (mov / movzx / movsx / lea / push / pop / inc / dec, the ALU group, imul,
** shifts, test, cmp, ret, int3, prefixes), jcc / jmp / call (rel8 or rel32 to a placed label, else a rel32 or
** rel8 field with a new fixup chained on the label), x87, MMX, 3DNow! and SSE (packed and F3 scalar) through the
** og_enc* helpers; ops without a case emit nothing.  Op 0xd8 starts a block: until op 0xd9 every instruction
** is spooled (og_spoolCode) instead of encoded, and 0xd9 has og_optimizeBlock schedule and encode them.  Quirks:
** mov reg8, imm writes B0+r with a 4-byte immediate; cmp with an imm8 encodes the 83 form with an imm32 and backs
** pc up 3 bytes; a group of ops (0x9f, 0xa0, 0xa3, ...) all encode as 0F 00 + modrm */
GLubyte *__fastcall og_encode(void *ctx, GLubyte *pc, OgInsn *insn)
{
    OgFixup *fx;
    int n;

    if (og_skipping) {
        /* inside a 0xd8 .. 0xd9 block: spool the ops; at 0xd9 og_optimizeBlock schedules and encodes them */
        if (insn->op == 0xd9) {
            og_skipping = 0;
            pc = og_optimizeBlock(ctx, pc);
        } else
            og_spoolCode(ctx, insn);
        return pc;
    }
    switch (insn->op) {
    case 0xd8:
        og_skipping = 1;
        break;
    case 0:                                         /* mov */
        if (og_isReg32(insn->a) && og_isImm(insn->b)) {
            *pc++ = (GLubyte)insn->a->val | 0xb8;
            *(GLuint *)pc = insn->b->val; pc += 4;
        } else if (og_isImm(insn->b)) {
            *pc = 0xc7;
            pc = og_encModrm(pc + 1, 0, insn->a);
            *(GLuint *)pc = insn->b->val; pc += 4;
        } else if (og_isReg32(insn->a)) {
            *pc = 0x8b;
            pc = og_encModrm(pc + 1, insn->a->val, insn->b);
        } else if (og_isReg32(insn->b)) {
            *pc = 0x89;
            pc = og_encModrm(pc + 1, insn->b->val, insn->a);
        } else if (og_isReg8(insn->a) && og_isImm(insn->b)) {
            *pc++ = (GLubyte)insn->a->val | 0xb0;
            *(GLuint *)pc = insn->b->val; pc += 4;
        } else if (og_isReg8(insn->a)) {
            *pc = 0x8a;
            pc = og_encModrm(pc + 1, insn->a->val, insn->b);
        } else if (og_isReg8(insn->b)) {
            *pc = 0x88;
            pc = og_encModrm(pc + 1, insn->b->val, insn->a);
        }
        break;
  case 2:
    *pc++ = 0xf;
    *pc = 0xb6;
    pc = og_encModrm(pc + 1, insn->a->val, insn->b);
    break;
  case 1:
    *pc++ = 0xf;
    *pc = 0xbe;
    pc = og_encModrm(pc + 1, insn->a->val, insn->b);
    break;
  case 4:
    *pc++ = 0xf;
    *pc = 0xb7;
    pc = og_encModrm(pc + 1, insn->a->val, insn->b);
    break;
  case 3:
    *pc++ = 0xf;
    *pc = 0xbf;
    pc = og_encModrm(pc + 1, insn->a->val, insn->b);
    break;
  case 5:
    *pc = 0x8d;
    pc = og_encModrm(pc + 1, insn->a->val, insn->b);
    break;
  case 6:
    if (og_isImm(insn->a)) {
      *pc++ = 0x68;
      *(GLuint *)pc = insn->a->val; pc += 4;
      break;
    }
    if (og_isReg32(insn->a)) {
      *pc++ = (char)insn->a->val + 0x50;
    } else {
      *pc = 0xff;
      pc = og_encModrm(pc + 1, 6, insn->a);
    }
    break;
  case 7:                                        /* pop */
    if (og_isReg32(insn->a)) {
      *pc++ = (char)insn->a->val + 0x58;
    } else {
      *pc = 0x8f;
      pc = og_encModrm(pc + 1, 0, insn->a);
    }
    break;
  case 8:                                        /* inc */
    if (og_isReg32(insn->a)) {
      *pc++ = (char)insn->a->val + 0x40;
    } else {
      *pc = 0xff;
      pc = og_encModrm(pc + 1, 0, insn->a);
    }
    break;
  case 9:
    if (og_isReg32(insn->a)) {
      *pc++ = (char)insn->a->val + 0x48;
    } else {
      *pc = 0xff;
      pc = og_encModrm(pc + 1, 1, insn->a);
    }
    break;
  case 0x1f:
    pc = og_encAlu(pc, 0x81, 3, 1, 0, insn->a, insn->b);
    break;
  case 0x20:
    pc = og_encAlu(pc, 0x81, 0x13, 0x11, 2, insn->a, insn->b);
    break;
  case 0x21:                                      /* imul */
    if (og_isImm(insn->b)) {
        *pc = 0x69;
        pc = og_encModrm(pc + 1, insn->a->val, insn->a);
        *(GLuint *)pc = insn->b->val; pc += 4;
    } else {
        *pc++ = 0xf;
        *pc = 0xaf;
        pc = og_encModrm(pc + 1, insn->a->val, insn->b);
    }
    break;
  case 0x22:
    *pc = 0xf7;
    pc = og_encModrm(pc + 1, 4, insn->a);
    break;
  case 0x23:
    *pc = 0xf7;
    pc = og_encModrm(pc + 1, 7, insn->a);
    break;
  case 0x24:
    pc = og_encAlu(pc, 0x81, 0x2b, 0x29, 5, insn->a, insn->b);
    break;
  case 0x25:
    pc = og_encAlu(pc, 0x81, 0x1b, 0x19, 3, insn->a, insn->b);
    break;
  case 0x26:
    pc = og_encAlu(pc, 0x81, 0x23, 0x21, 4, insn->a, insn->b);
    break;
  case 0x27:
    pc = og_encAlu(pc, 0x81, 0xb, 9, 1, insn->a, insn->b);
    break;
  case 0x28:
    if (og_isReg32(insn->a))
      pc = og_encAlu(pc, 0x81, 0x33, 0x31, 6, insn->a, insn->b);
    else
      pc = og_encAlu(pc, 0x80, 0x32, 0x30, 6, insn->a, insn->b);
    break;
  case 0x1e:                                      /* cmp */
    if (og_isImm8(insn->b))
        pc = og_encAlu(pc, 0x83, 0, 0, 7, insn->a, insn->b) - 3;
    else
        pc = og_encAlu(pc, 0x81, 0x3b, 0x39, 7, insn->a, insn->b);
    break;
  case 0x2d:
    pc = og_encAlu(pc, 0xf7, 0xff, 0x85, 0, insn->a, insn->b);
    break;
  case 10:
    pc = og_encJcc(ctx, pc, 0x77, insn->a);
    break;
  case 0xc:
    pc = og_encJcc(ctx, pc, 0x72, insn->a);
    break;
  case 0xb:
    pc = og_encJcc(ctx, pc, 0x73, insn->a);
    break;
  case 0xd:
    pc = og_encJcc(ctx, pc, 0x76, insn->a);
    break;
  case 0xf:
    pc = og_encJcc(ctx, pc, 0x7c, insn->a);
    break;
  case 0xe:
    pc = og_encJcc(ctx, pc, 0x7d, insn->a);
    break;
  case 0x10:
    pc = og_encJcc(ctx, pc, 0x7e, insn->a);
    break;
  case 0x11:
    pc = og_encJcc(ctx, pc, 0x7f, insn->a);
    break;
  case 0x12:
    pc = og_encJcc(ctx, pc, 0x74, insn->a);
    break;
  case 0x13:
    pc = og_encJcc(ctx, pc, 0x75, insn->a);
    break;
  case 0x14:
    pc = og_encJcc(ctx, pc, 0x70, insn->a);
    break;
  case 0x15:
    pc = og_encJcc(ctx, pc, 0x71, insn->a);
    break;
  case 0x16:
    pc = og_encJcc(ctx, pc, 0x78, insn->a);
    break;
  case 0x17:
    pc = og_encJcc(ctx, pc, 0x79, insn->a);
    break;
  case 0x1a:
    pc = og_encJmp(ctx, pc, insn->a);
    break;
  case 0x1b:
    *pc = 0xff;
    pc = og_encModrm(pc + 1, 4, insn->a);
    break;
  case 0x18:                                      /* call */
    if (insn->a->kind == 8) {
        if (insn->a->fc != 0) {
            *pc = 0xe8;
            pc++;
            *(int *)pc = insn->a->fc - (int)pc - 4;
            return pc + 4;
        } else {
            *pc = 0xe8;
            pc++;
            fx = og_newFixup(ctx);
            fx->next = insn->a->f8;
            fx->at = pc;
            fx->kind = 1;
            insn->a->f8 = (int)fx;
            pc += 4;
        }
    } else {
        *pc = 0xff;
        pc = og_encModrm(pc + 1, 2, insn->a);
    }
    break;
  case 0x29:
    if (0 < insn->b->val) {
      *pc++ = 0xc1;
      *pc++ = (GLubyte)insn->a->val | 0xc0;
      *pc++ = (GLubyte)insn->b->val;
      break;
    }
    break;
  case 0x2a:
    if (og_isImm(insn->b)) {
      n = insn->b->val;
      if (0 < n) {
        if (n == 1) {
          *pc++ = 0xd1;
          *pc++ = (GLubyte)insn->a->val | 0xe0;
        } else {
          *pc++ = 0xc1;
          *pc++ = (GLubyte)insn->a->val | 0xe0;
          *pc++ = (GLubyte)insn->b->val;
        }
      }
    } else {
      *pc++ = 0xd3;
      *pc++ = (GLubyte)insn->a->val | 0xe0;
    }
    break;
  case 0x2b:
    if (og_isImm(insn->b)) {
      n = insn->b->val;
      if (0 < n) {
        if (n == 1) {
          *pc++ = 0xd1;
          *pc++ = (GLubyte)insn->a->val | 0xe8;
        } else {
          *pc++ = 0xc1;
          *pc++ = (GLubyte)insn->a->val | 0xe8;
          *pc++ = (GLubyte)insn->b->val;
        }
      }
    } else {
      *pc++ = 0xd3;
      *pc++ = (GLubyte)insn->a->val | 0xe8;
    }
    break;
  case 0x2c:
    if (0 < insn->b->val) {
      *pc++ = 0xc1;
      *pc++ = (GLubyte)insn->a->val | 0xf8;
      *pc++ = (GLubyte)insn->b->val;
      break;
    }
    break;
  case 0x1d:
    *pc++ = 0xc2;
    *(GLushort *)pc = (GLushort)insn->a->val; pc += 2;
    break;
  case 0x1c:
    *pc++ = 0xc3;
    break;
  case 0x2e:
    *pc++ = 0xcc;
    break;
  case 0x2f:
    *pc++ = 0x9b;
    break;
  case 0x30:
    *pc++ = 0x66;
    break;
  case 0x31:
    *pc++ = 0xf3;
    break;
  case 0x32:
    *pc++ = 0xa5;
    break;
  case 0x33:
    *pc++ = 0x90;
    break;
  case 0x36:
    pc = og_encX87(pc, 0xd9, 0xc0, 0xd9, 0, insn->a);
    break;
  case 0x37:
    pc = og_encX87(pc, 0, 0, 0xdb, 0, insn->a);
    break;
  case 0x38:
    pc = og_encX87(pc, 0, 0, 0xdf, 5, insn->a);
    break;
  case 0x51:
    pc = og_encX87(pc, 0xd9, 0xc0, 0xdd, 0, insn->a);
    break;
  case 0x39:
    pc = og_encX87(pc, 0xd9, 200, 0, 0, insn->a);
    break;
  case 0x3a:
    pc = og_encX87(pc, 0xdd, 0xd0, 0xd9, 2, insn->a);
    break;
  case 0x3b:
    pc = og_encX87(pc, 0xdd, 0xd8, 0xd9, 3, insn->a);
    break;
  case 0x3d:
    pc = og_encX87(pc, 0, 0, 0xdb, 3, insn->a);
    break;
  case 0x3e:
    pc = og_encX87(pc, 0, 0, 0xdb, 2, insn->a);
    break;
  case 0x4a:
    pc = og_encX87(pc, 0, 0, 0xd9, 5, insn->a);
    break;
  case 0x4b:
    pc = og_encX87(pc, 0, 0, 0xd9, 7, insn->a);
    break;
  case 0x3f:
    pc = og_encX87(pc, 0, 0, 0xdf, 7, insn->a);
    break;
  case 0x3c:
    pc = og_encX87(pc, 0xdd, 0xd8, 0xdd, 3, insn->a);
    break;
  case 0x40:
    pc = og_encX87(pc, 0xd8, 200, 0xd8, 1, insn->a);
    break;
  case 0x41:
    pc = og_encX87(pc, 0xde, 200, 0, 0, insn->a);
    break;
  case 0x42:
    pc = og_encX87(pc, 0xd8, 0xc0, 0xd8, 0, insn->a);
    break;
  case 0x52:
    pc = og_encX87(pc, 0xd8, 0xc0, 0xdc, 0, insn->a);
    break;
  case 0x43:
    pc = og_encX87(pc, 0xde, 0xc0, 0, 0, insn->a);
    break;
  case 0x4c:
    pc = og_encX87(pc, 0xde, 0xe8, 0, 0, insn->a);
    break;
  case 0x4d:
    pc = og_encX87(pc, 0xde, 0xe0, 0, 0, insn->a);
    break;
  case 0x44:
    pc = og_encX87(pc, 0xd8, 0xe0, 0xd8, 4, insn->a);
    break;
  case 0x50:
    pc = og_encX87(pc, 0xd8, 0xe8, 0xd8, 5, insn->a);
    break;
  case 0x45:
    pc = og_encX87(pc, 0xd8, 0xf8, 0xd8, 7, insn->a);
    break;
  case 0x4f:
    pc = og_encX87(pc, 0xde, 0xf0, 0, 0, insn->a);
    break;
  case 0x46:
    pc = og_encX87(pc, 0xd8, 0xf0, 0xd8, 6, insn->a);
    break;
  case 0x4e:
    pc = og_encX87(pc, 0xde, 0xf8, 0, 0, insn->a);
    break;
  case 0x47:
    *pc++ = 0xd9;
    *pc++ = 0xeb;
    break;
  case 0x48:
    *pc++ = 0xd9;
    *pc++ = 0xe8;
    break;
  case 0x49:
    *pc++ = 0xd9;
    *pc++ = 0xe1;
    break;
  case 0x53:
    *pc++ = 0xd9;
    *pc++ = 0xe0;
    break;
  case 0x54:
    *pc++ = 0xf;
    *pc++ = 0x31;
    break;
  case 0x34:
    *pc = 0xf;
    pc = og_encAlu(pc + 1, 0, 0x44, 0, 0, insn->a, insn->b);
    break;
  case 0x35:
    *pc++ = 0xf;
    *pc = 0x45;
    pc = og_encModrm(pc + 1, insn->a->val, insn->b);
    break;
  case 0x58:
    pc = og_enc0F(pc, 0x6b, insn->a, insn->b);
    break;
  case 0x59:
    pc = og_enc0F(pc, 99, insn->a, insn->b);
    break;
  case 0x5a:
    pc = og_enc0F(pc, 0x67, insn->a, insn->b);
    break;
  case 0x6a:
    pc = og_enc0F(pc, 0xfc, insn->a, insn->b);
    break;
  case 0x6b:
    pc = og_enc0F(pc, 0xfe, insn->a, insn->b);
    break;
  case 0x6c:
    pc = og_enc0F(pc, 0xec, insn->a, insn->b);
    break;
  case 0x6d:
    pc = og_enc0F(pc, 0xed, insn->a, insn->b);
    break;
  case 0x6e:
    pc = og_enc0F(pc, 0xdc, insn->a, insn->b);
    break;
  case 0x6f:
    pc = og_enc0F(pc, 0xdd, insn->a, insn->b);
    break;
  case 0x70:
    pc = og_enc0F(pc, 0xfd, insn->a, insn->b);
    break;
  case 0x71:
    pc = og_enc0F(pc, 0xdb, insn->a, insn->b);
    break;
  case 0x72:
    pc = og_enc0F(pc, 0xdf, insn->a, insn->b);
    break;
  case 0x73:
    pc = og_enc0F(pc, 0x74, insn->a, insn->b);
    break;
  case 0x74:
    pc = og_enc0F(pc, 0x76, insn->a, insn->b);
    break;
  case 0x75:
    pc = og_enc0F(pc, 0x75, insn->a, insn->b);
    break;
  case 0x76:
    pc = og_enc0F(pc, 100, insn->a, insn->b);
    break;
  case 0x77:
    pc = og_enc0F(pc, 0x65, insn->a, insn->b);
    break;
  case 0x78:
    pc = og_enc0F(pc, 0x66, insn->a, insn->b);
    break;
  case 0x79:
    pc = og_enc0F(pc, 0xf5, insn->a, insn->b);
    break;
  case 0x7a:
    pc = og_enc0F(pc, 0xe5, insn->a, insn->b);
    break;
  case 0x7b:
    pc = og_enc0F(pc, 0xd5, insn->a, insn->b);
    break;
  case 0x83:
    pc = og_enc0F(pc, 0xeb, insn->a, insn->b);
    break;
  case 0x5b:
    pc = og_encMmxShift(pc, 0xf2, 0x72, insn->a, insn->b);
    break;
  case 0x5c:
    pc = og_encMmxShift(pc, 0xf3, 0x73, insn->a, insn->b);
    break;
  case 0x5d:
    pc = og_encMmxShift(pc, 0xf1, 0x71, insn->a, insn->b);
    break;
  case 0x5e:
    pc = og_encMmxShift(pc, 0xe2, 0x72, insn->a, insn->b);
    break;
  case 0x60:
    pc = og_encMmxShift(pc, 0xe1, 0x71, insn->a, insn->b);
    break;
  case 0x61:
    pc = og_encMmxShift(pc, 0xd2, 0x72, insn->a, insn->b);
    break;
  case 0x62:
    pc = og_encMmxShift(pc, 0xd3, 0x73, insn->a, insn->b);
    break;
  case 99:
    pc = og_encMmxShift(pc, 0xd1, 0x71, insn->a, insn->b);
    break;
  case 0x7c:
    pc = og_enc0F(pc, 0xf8, insn->a, insn->b);
    break;
  case 0x7d:
    pc = og_enc0F(pc, 0xf9, insn->a, insn->b);
    break;
  case 0x7e:
    pc = og_enc0F(pc, 0xfa, insn->a, insn->b);
    break;
  case 0x7f:
    pc = og_enc0F(pc, 0xe8, insn->a, insn->b);
    break;
  case 0x80:
    pc = og_enc0F(pc, 0xe9, insn->a, insn->b);
    break;
  case 0x81:
    pc = og_enc0F(pc, 0xd8, insn->a, insn->b);
    break;
  case 0x82:
    pc = og_enc0F(pc, 0xd9, insn->a, insn->b);
    break;
  case 100:
    pc = og_enc0F(pc, 0x68, insn->a, insn->b);
    break;
  case 0x65:
    pc = og_enc0F(pc, 0x6a, insn->a, insn->b);
    break;
  case 0x66:
    pc = og_enc0F(pc, 0x69, insn->a, insn->b);
    break;
  case 0x67:
    pc = og_enc0F(pc, 0x60, insn->a, insn->b);
    break;
  case 0x68:
    pc = og_enc0F(pc, 0x62, insn->a, insn->b);
    break;
  case 0x69:
    pc = og_enc0F(pc, 0x61, insn->a, insn->b);
    break;
  case 0x84:
    pc = og_enc0F(pc, 0xef, insn->a, insn->b);
    break;
  case 0x55:
    *pc++ = 0xf;
    *pc++ = 0x77;
    break;
  case 0x56:                                      /* movd */
    *pc++ = 0xf;
    if (og_isMmx(insn->a)) {
        *pc = 0x6e;
        pc = og_encModrm(pc + 1, insn->a->val, insn->b);
    } else {
        *pc = 0x7e;
        pc = og_encModrm(pc + 1, insn->b->val, insn->a);
    }
    break;
  case 0x57:                                      /* movq */
    *pc++ = 0xf;
    if (og_isMmx(insn->a)) {
        *pc++ = 0x6f;
        if (og_isMmx(insn->b))
            *pc++ = (GLubyte)((((GLubyte)insn->a->val | 0xf8) << 3) | (GLubyte)insn->b->val);
        else
            pc = og_encModrm(pc, insn->a->val, insn->b);
    } else {
        *pc = 0x7f;
        pc = og_encModrm(pc + 1, insn->b->val, insn->a);
    }
    break;
  case 0xb2:
    pc = og_encMovq(pc, insn->a, insn->b, 0x12, 0x13);
    break;
  case 0xb1:
    pc = og_encMovq(pc, insn->a, insn->b, 0x16, 0x17);
    break;
  case 0xb0:
    pc = og_encMovq(pc, insn->a, insn->b, 0x28, 0x29);
    break;
  case 0xb5:
    pc = og_encMovq(pc, insn->a, insn->b, 0x10, 0x11);
    break;
  case 0xb4:
    *pc = 0xf3;
    pc = og_encMovq(pc + 1, insn->a, insn->b, 0x10, 0x11);
    break;
  case 0xbd:
    pc = og_enc0F(pc, 0xc6, insn->a, insn->b);
    *pc++ = (GLubyte)insn->c->val;
    break;
  case 0x9e:
    pc = og_enc0F(pc, 0xc2, insn->a, insn->b);
    *pc++ = (GLubyte)insn->c->val;
    break;
  case 0xd2:
    pc = og_enc0F(pc, 0x70, insn->a, insn->b);
    *pc++ = (GLubyte)insn->c->val;
    break;
  case 0xca:
    pc = og_enc0F(pc, 0xc4, insn->a, insn->b);
    *pc++ = (GLubyte)insn->c->val;
    break;
  case 0xc9:
    pc = og_enc0F(pc, 0xc5, insn->a, insn->b);
    *pc++ = (GLubyte)insn->c->val;
    break;
  case 0x9a:
    pc = og_enc0F(pc, 0x58, insn->a, insn->b);
    break;
  case 0x9d:
    pc = og_enc0F(pc, 0x54, insn->a, insn->b);
    break;
  case 0x9c:
    pc = og_enc0F(pc, 0x55, insn->a, insn->b);
    break;
  case 0xb6:
    pc = og_enc0F(pc, 0x59, insn->a, insn->b);
    break;
  case 0xb8:
    pc = og_enc0F(pc, 0x56, insn->a, insn->b);
    break;
  case 0xb9:
    pc = og_enc0F(pc, 0x53, insn->a, insn->b);
    break;
  case 0xc1:
    pc = og_enc0F(pc, 0x5c, insn->a, insn->b);
    break;
  case 0xc6:
    pc = og_enc0F(pc, 0x57, insn->a, insn->b);
    break;
  case 0xb3:
    pc = og_enc0F(pc, 0x50, insn->a, insn->b);
    break;
  case 0xae:
    pc = og_enc0F(pc, 0x5d, insn->a, insn->b);
    break;
  case 0xac:
    pc = og_enc0F(pc, 0x5f, insn->a, insn->b);
    break;
  case 0xa2:
    pc = og_enc0F(pc, 0x2d, insn->a, insn->b);
    break;
  case 0xa1:
    pc = og_enc0F(pc, 0x2a, insn->a, insn->b);
    break;
  case 0xcf:
    pc = og_enc0F(pc, 0xd7, insn->a, insn->b);
    break;
  case 0xba:
    pc = og_encF30F(pc, 0x53, insn->a, insn->b);
    break;
  case 0x9b:
    pc = og_encF30F(pc, 0x58, insn->a, insn->b);
    break;
  case 0xb7:
    pc = og_encF30F(pc, 0x59, insn->a, insn->b);
    break;
  case 0xc2:
    pc = og_encF30F(pc, 0x5c, insn->a, insn->b);
    break;
  case 0xad:
    pc = og_encF30F(pc, 0x5f, insn->a, insn->b);
    break;
  case 0xa4:
    pc = og_encF30F(pc, 0x2d, insn->a, insn->b);
    break;
  case 0xc0:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xc4:
    { OgOperand *b = insn->b; pc = og_enc0F(pc, 0x15, insn->a, b); }
    break;
  /* 0xc5 and the zero-opcode cases below are separate plain bodies, as in the original source: VC6
  ** rotates eax/ecx/edx between the case bodies' temporaries and then merges bodies whose code came out
  ** identical -- that is what groups 0x9f.. and 0xa3.. under one address each */
  case 0xc5:
    pc = og_enc0F(pc, 0x14, insn->a, insn->b);
    break;
  case 0x9f:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xa5:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xa8:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xab:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xbc:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xcb:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xce:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xd3:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xa3:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xa7:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xaa:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xbb:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xbf:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xc8:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xcd:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xd1:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xd5:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0xd6:
    *pc++ = 0xf;
    *pc = 0x18;
    pc = og_encModrm(pc + 1, insn->b->val, insn->a);
    break;
  case 0xa0: case 0xa6: case 0xa9: case 0xaf: case 0xbe: case 0xc3: case 0xc7: case 0xcc: case 0xd0:
  case 0xd4: case 0xd7:
    pc = og_enc0F(pc, 0, insn->a, insn->b);
    break;
  case 0x85:
    pc = og_enc3DNow(pc, 0xbf, insn->a, insn->b);
    break;
  case 0x86:
    pc = og_enc3DNow(pc, 0x9e, insn->a, insn->b);
    break;
  case 0x87:
    pc = og_enc3DNow(pc, 0x9a, insn->a, insn->b);
    break;
  case 0x88:
    pc = og_enc3DNow(pc, 0xaa, insn->a, insn->b);
    break;
  case 0x89:
    pc = og_enc3DNow(pc, 0xae, insn->a, insn->b);
    break;
  case 0x8a:
    pc = og_enc3DNow(pc, 0xb4, insn->a, insn->b);
    break;
  case 0x8b:
    pc = og_enc3DNow(pc, 0x90, insn->a, insn->b);
    break;
  case 0x8c:
    pc = og_enc3DNow(pc, 0xa0, insn->a, insn->b);
    break;
  case 0x8d:
    pc = og_enc3DNow(pc, 0xb0, insn->a, insn->b);
    break;
  case 0x8e:
    pc = og_enc3DNow(pc, 0x94, insn->a, insn->b);
    break;
  case 0x8f:
    pc = og_enc3DNow(pc, 0xa4, insn->a, insn->b);
    break;
  case 0x90:
    pc = og_enc3DNow(pc, 0xd, insn->a, insn->b);
    break;
  case 0x91:
    pc = og_enc3DNow(pc, 0x1d, insn->a, insn->b);
    break;
  case 0x92:
    pc = og_enc3DNow(pc, 0x96, insn->a, insn->b);
    break;
  case 0x93:
    pc = og_enc3DNow(pc, 0x97, insn->a, insn->b);
    break;
  case 0x94:
    pc = og_enc3DNow(pc, 0xa6, insn->a, insn->b);
    break;
  case 0x95:
    pc = og_enc3DNow(pc, 0xa7, insn->a, insn->b);
    break;
  case 0x96:
    pc = og_enc3DNow(pc, 0xb6, insn->a, insn->b);
    break;
  case 0x97:
    pc = og_enc3DNow(pc, 0xb7, insn->a, insn->b);
    break;
  case 0x98:
    *pc++ = 0xf;
    *pc++ = 0xe;
    break;
  case 0x99:
    *pc++ = 0xf;
    *pc++ = 0xd;
    break;
  }
  return pc;
}

typedef struct OgInsnBuf {
    GLuint insn[8000][6];
    GLuint (*next)[6];
} OgInsnBuf;
/* 690e96f0 -- empty the instruction buffer: gc + 0x4110 holds 8000 24-byte records, then the
   next-free pointer */
void __fastcall og_reset(void *gc)
{
    OgInsnBuf *b = (OgInsnBuf *)((GLubyte *)gc + 0x4110);
    b->next = b->insn;
}

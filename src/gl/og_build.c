/*
** og_build.c -- original VA range 690e4000..690e8000 (this part 690e5120..690e7030): the record
** builders of the og_* code generator (operands, labels, instruction records in gc->ogInsn[]) and
** the one-instruction emitters og_<mnemonic>.  og_encode (src/gl/og_encode.c) turns the records into
** machine code.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "og.h"
#include <glide.h>
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

GLint __fastcall __glLightiv_size(GLenum pname);

/* the og records and emitters: og.h */

/* 690e5120 -- __glLightiv_size: a 5-byte jump to __glLightfv_size (690e50d0), the same counts */
#ifdef _MSC_VER
__declspec(naked) GLint __fastcall __glLightiv_size(GLenum pname) { __asm jmp __glLightfv_size }
#else
GLint __fastcall __glLightiv_size(GLenum pname) { return __glLightfv_size(pname); }
#endif

/* 690e5610 -- og_imm: an immediate operand */
OgOperand *__fastcall og_imm(__GLcontext *gc, GLint val)
{
    OgOperand *o = og_new(gc, OG_IMM);

    o->val = val;
    return o;
}

/* 690e5620 -- og_new: the next free record of gc->ogInsn[], of the given kind */
void *__fastcall og_new(__GLcontext *gc, GLuint kind)
{
    __GLogInsnICD *rec = gc->ogInsnPtr;

    gc->ogInsnPtr = rec + 1;
    rec->w[0] = kind;
    return rec;
}

/* 690e5640 -- og_newFixup: a rel32 fixup (og_encJmp / og_encJcc queue it on an unbound label) */
OgLabel *__fastcall og_newFixup(__GLcontext *gc)
{
    OgLabel *f = og_new(gc, OG_LABEL);

    f->val = 1;
    f->link = 0;
    f->addr = 0;
    f->unused10 = 0;
    return f;
}

/* 690e5660 -- og_newLabel: an unbound label */
OgLabel *__fastcall og_newLabel(__GLcontext *gc)
{
    OgLabel *l = og_new(gc, OG_LABEL);

    l->link = 0;
    l->addr = 0;
    l->unused10 = 0;
    l->val = 0;
    return l;
}

/* 690e5680 -- og_mem: the memory operand [base + index * scale + disp] */
OgOperand *__fastcall og_mem(__GLcontext *gc, OgOperand *disp, OgOperand *base, OgOperand *index, GLint scale)
{
    OgMem *m = og_new(gc, OG_MEM);

    m->scale = scale;
    m->disp = disp;
    m->base = base;
    m->index = index;
    return (OgOperand *)m;
}

/* 690e56b0 -- og_memBase: the memory operand [base + disp] */
OgOperand *__fastcall og_memBase(__GLcontext *gc, OgOperand *disp, OgOperand *base)
{
    OgMem *m = og_new(gc, OG_MEM);

    m->disp = disp;
    m->base = base;
    m->index = 0;
    m->scale = 1;
    return (OgOperand *)m;
}

/* 690e5700 -- og_insn: an instruction record */
OgInsn *__fastcall og_insn(__GLcontext *gc, GLuint op, OgOperand *a, OgOperand *b, OgOperand *c)
{
    OgInsn *insn = og_new(gc, OG_INSN);

    insn->a = a;
    insn->op = op;
    insn->b = b;
    insn->c = c;
    return insn;
}

/*
** The one-instruction emitters: og_<mnemonic>(gc, operands...) = og_insn(gc, op, operands..., 0).
** The opcode numbers are og_encode's; opNN names are ops og_encode does not implement.
*/
#define OG_INSN0(name, op) \
    OgInsn *__fastcall name(__GLcontext *gc) { return og_insn(gc, op, 0, 0, 0); }
#define OG_INSN1(name, op) \
    OgInsn *__fastcall name(__GLcontext *gc, OgOperand *a) { return og_insn(gc, op, a, 0, 0); }
#define OG_INSN2(name, op) \
    OgInsn *__fastcall name(__GLcontext *gc, OgOperand *a, OgOperand *b) { return og_insn(gc, op, a, b, 0); }
#define OG_INSN3(name, op) \
    OgInsn *__fastcall name(__GLcontext *gc, OgOperand *a, OgOperand *b, OgOperand *c) \
    { return og_insn(gc, op, a, b, c); }

OG_INSN0(og_fabs, 0x49)                 /* 690e56e0 (D9 E1) */
OG_INSN0(og_fchs, 0x53)                 /* 690e5730 (D9 E0) */
OG_INSN0(og_fldpi, 0x47)                /* 690e5750 (D9 EB) */
OG_INSN0(og_fld1, 0x48)                 /* 690e5770 */
OG_INSN0(og_int3, 0x2e)                 /* 690e5790 (CC) */
OG_INSN0(og_fwait, 0x2f)                /* 690e57b0 (9B) */
OG_INSN0(og_movsd, 0x32)                /* 690e57d0 (A5) */
OG_INSN0(og_nop, 0x33)                  /* 690e57f0 (90) */
OG_INSN0(og_beginBlock, 0xd8)           /* 690e5810: start a scheduled block (og_encode spools the following insns) */
OG_INSN0(og_endBlock, 0xd9)             /* 690e5830: end the block: og_optimizeBlock (690e9010) schedules and encodes it */
OG_INSN0(og_rdtsc, 0x54)                /* 690e5850 (0F 31) */
OG_INSN0(og_repPrefix, 0x31)            /* 690e5870 (F3 prefix) */
OG_INSN0(og_ret, 0x1c)                  /* 690e5890 */
OG_INSN0(og_opsize, 0x30)               /* 690e58b0 (66 prefix) */
OG_INSN1(og_call, 0x18)                 /* 690e58d0 */
OG_INSN1(og_fadd, 0x42)                 /* 690e58e0 */
OG_INSN1(og_fadd64, 0x52)               /* 690e58f0 (DC /0); ToDo UNUSED CODE: nothing in the driver calls it */
OG_INSN1(og_fdiv, 0x46)                 /* 690e5900 */
OG_INSN1(og_fdivr, 0x45)                /* 690e5910 */
OG_INSN1(og_fild, 0x37)                 /* 690e5920 */
OG_INSN1(og_fild64, 0x38)               /* 690e5930 */
OG_INSN1(og_fist, 0x3e)                 /* 690e5940 (DB /2); ToDo UNUSED CODE: nothing in the driver calls it */
OG_INSN1(og_fistp, 0x3d)                /* 690e5950 */
OG_INSN1(og_fistp64, 0x3f)              /* 690e5960 (DF /7); ToDo UNUSED CODE: nothing in the driver calls it */
OG_INSN1(og_fld, 0x36)                  /* 690e5970 */
OG_INSN1(og_fldcw, 0x4a)                /* 690e5980 */
OG_INSN1(og_fld64, 0x51)                /* 690e5990 */
OG_INSN1(og_fmul, 0x40)                 /* 690e59a0 */
OG_INSN1(og_fst, 0x3a)                  /* 690e59b0 */
OG_INSN1(og_fnstcw, 0x4b)               /* 690e59c0 */
OG_INSN1(og_fstp, 0x3b)                 /* 690e59d0 */
OG_INSN1(og_fstp64, 0x3c)               /* 690e59e0 (DD /3); ToDo UNUSED CODE: nothing in the driver calls it */
OG_INSN1(og_fsub, 0x44)                 /* 690e59f0 */
OG_INSN1(og_fsubr, 0x50)                /* 690e5a00 */
OG_INSN1(og_fxch, 0x39)                 /* 690e5a10 */
OG_INSN1(og_ja, 0x0a)                   /* 690e5a20 */
OG_INSN1(og_jae, 0x0b)                  /* 690e5a30; ToDo UNUSED CODE: nothing in the driver calls it */
OG_INSN1(og_jb, 0x0c)                   /* 690e5a40 */
OG_INSN1(og_jbe, 0x0d)                  /* 690e5a50; ToDo UNUSED CODE: nothing in the driver calls it */
OG_INSN1(og_je, 0x12)                   /* 690e5a60 */
OG_INSN1(og_jg, 0x11)                   /* 690e5a70 */
OG_INSN1(og_jge, 0x0e)                  /* 690e5a80 */
OG_INSN1(og_jl, 0x0f)                   /* 690e5a90 */
OG_INSN1(og_jle, 0x10)                  /* 690e5aa0; ToDo UNUSED CODE: nothing in the driver calls it */
OG_INSN1(og_jmp, 0x1a)                  /* 690e5ab0 */
OG_INSN1(og_jmp_rm, 0x1b)               /* 690e5ac0 */
OG_INSN1(og_jne, 0x13)                  /* 690e5ad0 */
OG_INSN1(og_jno, 0x15)                  /* 690e5ae0; ToDo UNUSED CODE: nothing in the driver calls it */
OG_INSN1(og_jns, 0x17)                  /* 690e5af0; ToDo UNUSED CODE: nothing in the driver calls it */
OG_INSN1(og_jo, 0x14)                   /* 690e5b00; ToDo UNUSED CODE: nothing in the driver calls it */
OG_INSN1(og_js, 0x16)                   /* 690e5b10; ToDo UNUSED CODE: nothing in the driver calls it */
OG_INSN1(og_pop, 0x07)                  /* 690e5b20 */
OG_INSN1(og_push, 0x06)                 /* 690e5b30 */
OG_INSN1(og_ret_imm, 0x1d)              /* 690e5b40 */
OG_INSN2(og_adc, 0x20)                  /* 690e5b50 */
OG_INSN2(og_add, 0x1f)                  /* 690e5b70 */
OG_INSN2(og_and, 0x26)                  /* 690e5b90 */
OG_INSN2(og_cmp, 0x1e)                  /* 690e5bb0 */
OG_INSN1(og_dec, 0x09)                  /* 690e5bd0 */
OG_INSN2(og_faddp, 0x43)                /* 690e5be0 */
OG_INSN2(og_fmulp, 0x41)                /* 690e5c00 */
OG_INSN2(og_fsubp, 0x4c)                /* 690e5c20 */
OG_INSN2(og_fsubrp, 0x4d)               /* 690e5c40 */
OG_INSN2(og_fdivp, 0x4e)                /* 690e5c60 */
OG_INSN2(og_fdivrp, 0x4f)               /* 690e5c80 */
OG_INSN2(og_f7, 0x23)                   /* 690e5ca0 */
OG_INSN2(og_imul, 0x21)                 /* 690e5cc0 */
OG_INSN1(og_mul, 0x22)                  /* 690e5ce0 */
OG_INSN1(og_inc, 0x08)                  /* 690e5cf0 */
OG_INSN2(og_lea, 0x05)                  /* 690e5d00 */
OG_INSN2(og_mov, 0x00)                  /* 690e5d20 */
OG_INSN2(og_movsx8, 0x01)               /* 690e5d40 */
OG_INSN2(og_movzx8, 0x02)               /* 690e5d60 */
OG_INSN2(og_movsx16, 0x03)              /* 690e5d80 */
OG_INSN2(og_movzx16, 0x04)              /* 690e5da0 */
OG_INSN2(og_or, 0x27)                   /* 690e5dc0 */
OG_INSN2(og_rol, 0x29)                  /* 690e5de0 */
OG_INSN2(og_sar, 0x2c)                  /* 690e5e00 */
OG_INSN2(og_sbb, 0x25)                  /* 690e5e20 */
OG_INSN2(og_shl, 0x2a)                  /* 690e5e40 */
OG_INSN2(og_shr, 0x2b)                  /* 690e5e60 */
OG_INSN2(og_sub, 0x24)                  /* 690e5e80 */
OG_INSN2(og_test, 0x2d)                 /* 690e5ea0 */
OG_INSN2(og_xor, 0x28)                 /* 690e5ec0 */
OG_INSN2(og_cmove, 0x34)                /* 690e5ee0 */
OG_INSN2(og_cmovne, 0x35)               /* 690e5f00 */
OG_INSN0(og_emms, 0x55)                 /* 690e5f20 */
OG_INSN2(og_movd, 0x56)                 /* 690e5f40 */
OG_INSN2(og_movq, 0x57)                 /* 690e5f60 */
OG_INSN2(og_packssdw, 0x58)             /* 690e5f80 */
OG_INSN2(og_packsswb, 0x59)             /* 690e5fa0 */
OG_INSN2(og_packuswb, 0x5a)             /* 690e5fc0 */
OG_INSN2(og_paddb, 0x6a)                /* 690e5fe0 */
OG_INSN2(og_paddd, 0x6b)                /* 690e6000 */
OG_INSN2(og_paddsb, 0x6c)               /* 690e6020 */
OG_INSN2(og_paddsw, 0x6d)               /* 690e6040 */
OG_INSN2(og_paddusb, 0x6e)              /* 690e6060 */
OG_INSN2(og_paddusw, 0x6f)              /* 690e6080 */
OG_INSN2(og_paddw, 0x70)                /* 690e60a0 */
OG_INSN2(og_pand, 0x71)                 /* 690e60c0 */
OG_INSN2(og_pandn, 0x72)                /* 690e60e0 */
OG_INSN2(og_pcmpeqb, 0x73)              /* 690e6100 */
OG_INSN2(og_pcmpeqd, 0x74)              /* 690e6120 */
OG_INSN2(og_pcmpeqw, 0x75)              /* 690e6140 */
OG_INSN2(og_pcmpgtb, 0x76)              /* 690e6160 */
OG_INSN2(og_pcmpgtd, 0x78)              /* 690e6180 */
OG_INSN2(og_pcmpgtw, 0x77)              /* 690e61a0 */
OG_INSN2(og_pmaddwd, 0x79)              /* 690e61c0 */
OG_INSN2(og_pmulhw, 0x7a)               /* 690e61e0 */
OG_INSN2(og_pmullw, 0x7b)               /* 690e6200 */
OG_INSN2(og_por, 0x83)                  /* 690e6220 */
OG_INSN2(og_pslld, 0x5b)                /* 690e6240 */
OG_INSN2(og_psllq, 0x5c)                /* 690e6260 */
OG_INSN2(og_psllw, 0x5d)                /* 690e6280 */
OG_INSN2(og_psrad, 0x5e)                /* 690e62a0 */
OG_INSN2(og_op5f, 0x5f)                 /* 690e62c0 */
OG_INSN2(og_psraw, 0x60)                /* 690e62e0 */
OG_INSN2(og_psrld, 0x61)                /* 690e6300 */
OG_INSN2(og_psrlq, 0x62)                /* 690e6320 */
OG_INSN2(og_psrlw, 0x63)                /* 690e6340 */
OG_INSN2(og_psubb, 0x7c)                /* 690e6360 */
OG_INSN2(og_psubd, 0x7e)                /* 690e6380 */
OG_INSN2(og_psubsb, 0x7f)               /* 690e63a0 */
OG_INSN2(og_psubsw, 0x80)               /* 690e63c0 */
OG_INSN2(og_psubusb, 0x81)              /* 690e63e0 */
OG_INSN2(og_psubusw, 0x82)              /* 690e6400 */
OG_INSN2(og_psubw, 0x7d)                /* 690e6420 */
OG_INSN2(og_punpckhbw, 0x64)            /* 690e6440 */
OG_INSN2(og_punpckhdq, 0x65)            /* 690e6460 */
OG_INSN2(og_punpckhwd, 0x66)            /* 690e6480 */
OG_INSN2(og_punpcklbw, 0x67)            /* 690e64a0 */
OG_INSN2(og_punpckldq, 0x68)            /* 690e64c0 */
OG_INSN2(og_punpcklwd, 0x69)            /* 690e64e0 */
OG_INSN2(og_pxor, 0x84)                 /* 690e6500 */
OG_INSN2(og_pavgusb, 0x85)              /* 690e6520 */
OG_INSN2(og_pfadd, 0x86)                /* 690e6540 */
OG_INSN2(og_pfsub, 0x87)                /* 690e6560 */
OG_INSN2(og_pfsubr, 0x88)               /* 690e6580 */
OG_INSN2(og_pfacc, 0x89)                /* 690e65a0 */
OG_INSN2(og_pfmul, 0x8a)                /* 690e65c0 */
OG_INSN2(og_pfcmpge, 0x8b)              /* 690e65e0 */
OG_INSN2(og_pfcmpgt, 0x8c)              /* 690e6600 */
OG_INSN2(og_pfcmpeq, 0x8d)              /* 690e6620 */
OG_INSN2(og_pfmin, 0x8e)                /* 690e6640 */
OG_INSN2(og_pfmax, 0x8f)                /* 690e6660 */
OG_INSN2(og_pi2fd, 0x90)                /* 690e6680 */
OG_INSN2(og_pf2id, 0x91)                /* 690e66a0 */
OG_INSN2(og_pfrcp, 0x92)                /* 690e66c0 */
OG_INSN2(og_pfrsqrt, 0x93)              /* 690e66e0 */
OG_INSN2(og_pfrcpit1, 0x94)             /* 690e6700 */
OG_INSN2(og_pfrsqit1, 0x95)             /* 690e6720 */
OG_INSN2(og_pfrcpit2, 0x96)             /* 690e6740 */
OG_INSN2(og_pmulhrw, 0x97)              /* 690e6760 */
OG_INSN1(og_prefetch_3dn, 0x99)         /* 690e6780 */
OG_INSN0(og_femms, 0x98)                /* 690e6790 */
OG_INSN2(og_addps, 0x9a)                /* 690e67b0 */
OG_INSN2(og_addss, 0x9b)                /* 690e67d0 */
OG_INSN2(og_andnps, 0x9c)               /* 690e67f0 */
OG_INSN2(og_andps, 0x9d)                /* 690e6810 */
OG_INSN3(og_cmpps, 0x9e)                /* 690e6830 */
OG_INSN3(og_op9f, 0x9f)                 /* 690e6850 */
OG_INSN2(og_opa0, 0xa0)                 /* 690e6870 */
OG_INSN2(og_cvtpi2ps, 0xa1)             /* 690e6890 */
OG_INSN2(og_cvtps2pi, 0xa2)             /* 690e68b0 */
OG_INSN2(og_opa3, 0xa3)                 /* 690e68d0 */
OG_INSN2(og_cvtss2si, 0xa4)             /* 690e68f0 */
OG_INSN2(og_opa5, 0xa5)                 /* 690e6910 */
OG_INSN2(og_opa6, 0xa6)                 /* 690e6930 */
OG_INSN2(og_opa7, 0xa7)                 /* 690e6950 */
OG_INSN2(og_opa8, 0xa8)                 /* 690e6970 */
OG_INSN2(og_opa9, 0xa9)                 /* 690e6990 */
OG_INSN2(og_opaa, 0xaa)                 /* 690e69b0 */
OG_INSN2(og_opab, 0xab)                 /* 690e69d0 */
OG_INSN2(og_maxps, 0xac)                /* 690e69f0 */
OG_INSN2(og_maxss, 0xad)                /* 690e6a10 */
OG_INSN2(og_minps, 0xae)                /* 690e6a30 */
OG_INSN2(og_opaf, 0xaf)                 /* 690e6a50 */
OG_INSN2(og_movaps, 0xb0)               /* 690e6a70 */
OG_INSN2(og_movhps, 0xb1)               /* 690e6a90 */
OG_INSN2(og_movlps, 0xb2)               /* 690e6ab0 */
OG_INSN2(og_movmskps, 0xb3)             /* 690e6ad0 */
OG_INSN2(og_movss, 0xb4)                  /* 690e6af0 */
OG_INSN2(og_movups, 0xb5)               /* 690e6b10 */
OG_INSN2(og_mulps, 0xb6)                /* 690e6b30 */
OG_INSN2(og_mulss, 0xb7)                /* 690e6b50 */
OG_INSN2(og_orps, 0xb8)                 /* 690e6b70 */
OG_INSN2(og_rcpps, 0xb9)                /* 690e6b90 */
OG_INSN2(og_rcpss, 0xba)                /* 690e6bb0 */
OG_INSN2(og_opbb, 0xbb)                 /* 690e6bd0 */
OG_INSN2(og_opbc, 0xbc)                 /* 690e6bf0 */
OG_INSN3(og_shufps, 0xbd)               /* 690e6c10 */
OG_INSN2(og_opbe, 0xbe)                 /* 690e6c30 */
OG_INSN2(og_opbf, 0xbf)                 /* 690e6c50 */
OG_INSN2(og_opc0, 0xc0)                 /* 690e6c70 */
OG_INSN2(og_subps, 0xc1)                /* 690e6c90 */
OG_INSN2(og_subss, 0xc2)                /* 690e6cb0 */
OG_INSN2(og_opc3, 0xc3)                 /* 690e6cd0 */
OG_INSN2(og_unpckhps, 0xc4)             /* 690e6cf0 */
OG_INSN2(og_unpcklps, 0xc5)             /* 690e6d10 */
OG_INSN2(og_xorps, 0xc6)                /* 690e6d30 */
OG_INSN2(og_opc7, 0xc7)                 /* 690e6d50 */
OG_INSN2(og_opc8, 0xc8)                 /* 690e6d70 */
OG_INSN3(og_pextrw, 0xc9)               /* 690e6d90 */
OG_INSN3(og_pinsrw, 0xca)               /* 690e6db0 */
OG_INSN2(og_opcb, 0xcb)                 /* 690e6dd0 */
OG_INSN2(og_opcc, 0xcc)                 /* 690e6df0 */
OG_INSN2(og_opcd, 0xcd)                 /* 690e6e10 */
OG_INSN2(og_opce, 0xce)                 /* 690e6e30 */
OG_INSN2(og_pmovmskb, 0xcf)             /* 690e6e50 */
OG_INSN2(og_opd0, 0xd0)                 /* 690e6e70 */
OG_INSN2(og_opd1, 0xd1)                 /* 690e6e90 */
OG_INSN3(og_pshufw, 0xd2)               /* 690e6eb0 */
OG_INSN2(og_opd3, 0xd3)                 /* 690e6ed0 */
OG_INSN2(og_opd4, 0xd4)                 /* 690e6ef0 */
OG_INSN2(og_opd5, 0xd5)                 /* 690e6f10 */
OG_INSN2(og_prefetch, 0xd6)             /* 690e6f30 */
OG_INSN2(og_opd7, 0xd7)                 /* 690e6f50 */

/* 690e6fe0 -- og_bindLabel: the label is now at pc: patch the fixups queued on it */
void __fastcall og_bindLabel(__GLcontext *gc, OgLabel *label, GLubyte *pc)
{
    og_patchRefs(label->link, pc);
}

/* 690e7000 -- og_patchRefs: point each fixup of the chain at pc (rel8 or rel32) */
void __fastcall og_patchRefs(OgLabel *fixup, GLubyte *pc)
{
    for (; fixup != NULL; fixup = fixup->link) {
	if (fixup->val == 0) {
	    *fixup->addr = (GLubyte)pc - (GLubyte)fixup->addr - 1;
	} else {
	    *(GLint *)fixup->addr = pc - fixup->addr - 4;
	}
    }
}

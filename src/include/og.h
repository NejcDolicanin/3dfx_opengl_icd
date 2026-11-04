/*
** og.h -- the OG x86 code generator: the 24-byte records it appends to gc->ogInsn[] (operands,
** memory operands, instructions, labels), the og_* emitters (og_build.c: og_<mnemonic>(gc, operands)
** = og_insn(gc, op, operands)), the encoder og_encode (og_encode.c) and the helper macros the
** generators (src/sst/sst_*.c) build routines with:
**     EMIT(og_mov(gc, R(eax), MEM(&gc->hwcx.vtxBase)));
** Include after wglpriv.h (__GLogOperand and the static register operands og_eax .. og_mm7).
*/
#ifndef __OG_H__
#define __OG_H__


/* record kinds (the first word) */
#define OG_IMM          0       /* immediate: val */
#define OG_REG8         1       /* val = register number */
#define OG_REG32        3
#define OG_X87          4       /* st(val) */
#define OG_MMX          5       /* MMX / SSE register */
#define OG_MEM          6       /* [base + index * scale + disp] */
#define OG_INSN         7       /* op, operands a, b, c */
#define OG_LABEL        8       /* a label (val 0) or a fixup (val 1) */

typedef __GLogOperand OgOperand;            /* immediates, registers and labels */
typedef __GLogOperand OgLabel;              /* a label: link = its fixup chain, addr = its address once bound */
typedef struct OgMemRec {                   /* OG_MEM */
    GLuint      kind;                       /* +0x00 */
    OgOperand  *disp;                       /* +0x04  an immediate, or NULL */
    GLint       scale;                      /* +0x08 */
    OgOperand  *base, *index;               /* +0x0c */
} OgMem;
typedef struct OgInsnRec {                  /* OG_INSN */
    GLuint      kind;                       /* +0x00 */
    GLint       op;                         /* +0x04  og_encode's opcode number (signed: the scheduler's range
                                                 tests use jl / jg) */
    OgOperand  *a, *b, *c;                  /* +0x08 */
    struct OgInsnRec *next;                 /* +0x14  the scheduler's pending list (og_util.c) */
} OgInsn;

/* records (og_build.c).  og_mem / og_memBase build an OgMem; it is passed on as an operand. */
void       *__fastcall og_new(__GLcontext *gc, GLuint kind);                 /* 690e5620 */
OgOperand  *__fastcall og_imm(__GLcontext *gc, GLint val);                   /* 690e5610 */
OgLabel    *__fastcall og_newFixup(__GLcontext *gc);                         /* 690e5640 */
OgLabel    *__fastcall og_newLabel(__GLcontext *gc);                         /* 690e5660 */
OgOperand  *__fastcall og_mem(__GLcontext *gc, OgOperand *disp, OgOperand *base, OgOperand *index,
                              GLint scale);                                  /* 690e5680 */
OgOperand  *__fastcall og_memBase(__GLcontext *gc, OgOperand *disp, OgOperand *base);   /* 690e56b0 */
OgInsn     *__fastcall og_insn(__GLcontext *gc, GLuint op, OgOperand *a, OgOperand *b, OgOperand *c);   /* 690e5700 */
void        __fastcall og_bindLabel(__GLcontext *gc, OgLabel *label, GLubyte *pc);  /* 690e6fe0 */
void        __fastcall og_patchRefs(OgLabel *fixup, GLubyte *pc);            /* 690e7000 */
/* og_encode.c */
GLubyte    *__fastcall og_encode(__GLcontext *gc, GLubyte *pc, OgInsn *insn);  /* 690e7040 */
void        __fastcall og_reset(__GLcontext *gc);                            /* 690e96f0: empty gc->ogInsn[] */
/* operand predicates (og_util.c) */
int         __fastcall og_isReg32(OgOperand *o);                             /* 690e8a20 */
int         __fastcall og_isReg8(OgOperand *o);                              /* 690e8a30 */
int         __fastcall og_isMmx(OgOperand *o);                               /* 690e8a40 */
int         __fastcall og_isImm(OgOperand *o);                               /* 690e8a50 */
/* generator helpers */
OgOperand  *__fastcall og_fpk(__GLcontext *gc, GLuint edx_unused, double v);   /* 690e9710 (og_util.c): [v in the float pool] */
GLubyte    *__fastcall og_lostContextCheck(__GLcontext *gc, GLubyte *pc, OgOperand *reg, OgOperand *retImm);   /* 6902e9f0 (sst_lostctx.c) */
GLubyte    *__fastcall conv_load(__GLcontext *gc, GLubyte *pc, OgOperand *src, GLenum type, OgOperand *work);   /* 69056560 */

/* the one-instruction emitters (og_build.c; opNN = an op og_encode does not implement) */
OgInsn *__fastcall og_fld1(__GLcontext *gc);                                  /* 690e5770 op 0x48 */
OgInsn *__fastcall og_ret(__GLcontext *gc);                                   /* 690e5890 op 0x1c */
OgInsn *__fastcall og_call(__GLcontext *gc, OgOperand *a);                    /* 690e58d0 op 0x18 */
OgInsn *__fastcall og_fadd(__GLcontext *gc, OgOperand *a);                    /* 690e58e0 op 0x42 */
OgInsn *__fastcall og_fdiv(__GLcontext *gc, OgOperand *a);                    /* 690e5900 op 0x46 */
OgInsn *__fastcall og_fdivr(__GLcontext *gc, OgOperand *a);                   /* 690e5910 op 0x45 */
OgInsn *__fastcall og_fild(__GLcontext *gc, OgOperand *a);                    /* 690e5920 op 0x37 */
OgInsn *__fastcall og_fild64(__GLcontext *gc, OgOperand *a);                  /* 690e5930 op 0x38 */
OgInsn *__fastcall og_fistp(__GLcontext *gc, OgOperand *a);                   /* 690e5950 op 0x3d */
OgInsn *__fastcall og_fld(__GLcontext *gc, OgOperand *a);                     /* 690e5970 op 0x36 */
OgInsn *__fastcall og_fldcw(__GLcontext *gc, OgOperand *a);                   /* 690e5980 op 0x4a */
OgInsn *__fastcall og_fld64(__GLcontext *gc, OgOperand *a);                   /* 690e5990 op 0x51: fld qword (DD /0) */
OgInsn *__fastcall og_fmul(__GLcontext *gc, OgOperand *a);                    /* 690e59a0 op 0x40 */
OgInsn *__fastcall og_fst(__GLcontext *gc, OgOperand *a);                     /* 690e59b0 op 0x3a */
OgInsn *__fastcall og_fnstcw(__GLcontext *gc, OgOperand *a);                  /* 690e59c0 op 0x4b */
OgInsn *__fastcall og_fstp(__GLcontext *gc, OgOperand *a);                    /* 690e59d0 op 0x3b */
OgInsn *__fastcall og_fsub(__GLcontext *gc, OgOperand *a);                    /* 690e59f0 op 0x44 */
OgInsn *__fastcall og_fsubr(__GLcontext *gc, OgOperand *a);                   /* 690e5a00 op 0x50 */
OgInsn *__fastcall og_fxch(__GLcontext *gc, OgOperand *a);                    /* 690e5a10 op 0x39 */
OgInsn *__fastcall og_ja(__GLcontext *gc, OgOperand *a);                      /* 690e5a20 op 0x0a */
OgInsn *__fastcall og_jb(__GLcontext *gc, OgOperand *a);                      /* 690e5a40 op 0x0c */
OgInsn *__fastcall og_je(__GLcontext *gc, OgOperand *a);                      /* 690e5a60 op 0x12 */
OgInsn *__fastcall og_jg(__GLcontext *gc, OgOperand *a);                      /* 690e5a70 op 0x11 */
OgInsn *__fastcall og_jge(__GLcontext *gc, OgOperand *a);                     /* 690e5a80 op 0x0e */
OgInsn *__fastcall og_jl(__GLcontext *gc, OgOperand *a);                      /* 690e5a90 op 0x0f */
OgInsn *__fastcall og_jmp(__GLcontext *gc, OgOperand *a);                     /* 690e5ab0 op 0x1a */
OgInsn *__fastcall og_jmp_rm(__GLcontext *gc, OgOperand *a);                  /* 690e5ac0 op 0x1b */
OgInsn *__fastcall og_jne(__GLcontext *gc, OgOperand *a);                     /* 690e5ad0 op 0x13 */
OgInsn *__fastcall og_pop(__GLcontext *gc, OgOperand *a);                     /* 690e5b20 op 0x07 */
OgInsn *__fastcall og_push(__GLcontext *gc, OgOperand *a);                    /* 690e5b30 op 0x06 */
OgInsn *__fastcall og_ret_imm(__GLcontext *gc, OgOperand *a);                 /* 690e5b40 op 0x1d */
OgInsn *__fastcall og_adc(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e5b50 op 0x20 */
OgInsn *__fastcall og_add(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e5b70 op 0x1f */
OgInsn *__fastcall og_and(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e5b90 op 0x26 */
OgInsn *__fastcall og_cmp(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e5bb0 op 0x1e */
OgInsn *__fastcall og_dec(__GLcontext *gc, OgOperand *a);                     /* 690e5bd0 op 0x09 */
OgInsn *__fastcall og_faddp(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e5be0 op 0x43 */
OgInsn *__fastcall og_fmulp(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e5c00 op 0x41 */
OgInsn *__fastcall og_fsubp(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e5c20 op 0x4c */
OgInsn *__fastcall og_fsubrp(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e5c40 op 0x4d */
OgInsn *__fastcall og_fdivp(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e5c60 op 0x4e */
OgInsn *__fastcall og_fdivrp(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e5c80 op 0x4f */
OgInsn *__fastcall og_f7(__GLcontext *gc, OgOperand *a, OgOperand *b);        /* 690e5ca0 op 0x23 */
OgInsn *__fastcall og_imul(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e5cc0 op 0x21 */
OgInsn *__fastcall og_mul(__GLcontext *gc, OgOperand *a);                     /* 690e5ce0 op 0x22 */
OgInsn *__fastcall og_inc(__GLcontext *gc, OgOperand *a);                     /* 690e5cf0 op 0x08 */
OgInsn *__fastcall og_lea(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e5d00 op 0x05 */
OgInsn *__fastcall og_mov(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e5d20 op 0x00 */
OgInsn *__fastcall og_movsx8(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e5d40 op 0x01 */
OgInsn *__fastcall og_movzx8(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e5d60 op 0x02 */
OgInsn *__fastcall og_movsx16(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e5d80 op 0x03 */
OgInsn *__fastcall og_movzx16(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e5da0 op 0x04 */
OgInsn *__fastcall og_or(__GLcontext *gc, OgOperand *a, OgOperand *b);        /* 690e5dc0 op 0x27 */
OgInsn *__fastcall og_rol(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e5de0 op 0x29 */
OgInsn *__fastcall og_sar(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e5e00 op 0x2c */
OgInsn *__fastcall og_sbb(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e5e20 op 0x25 */
OgInsn *__fastcall og_shl(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e5e40 op 0x2a */
OgInsn *__fastcall og_shr(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e5e60 op 0x2b */
OgInsn *__fastcall og_sub(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e5e80 op 0x24 */
OgInsn *__fastcall og_test(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e5ea0 op 0x2d */
OgInsn *__fastcall og_xor(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e5ec0 op 0x28 (8 or 32 bit) */
OgInsn *__fastcall og_cmove(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e5ee0 op 0x34 */
OgInsn *__fastcall og_cmovne(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e5f00 op 0x35 */
OgInsn *__fastcall og_emms(__GLcontext *gc);                                  /* 690e5f20 op 0x55 */
OgInsn *__fastcall og_movd(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e5f40 op 0x56 */
OgInsn *__fastcall og_movq(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e5f60 op 0x57 */
OgInsn *__fastcall og_packssdw(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e5f80 op 0x58 */
OgInsn *__fastcall og_packsswb(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e5fa0 op 0x59 */
OgInsn *__fastcall og_packuswb(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e5fc0 op 0x5a */
OgInsn *__fastcall og_paddb(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e5fe0 op 0x6a */
OgInsn *__fastcall og_paddd(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6000 op 0x6b */
OgInsn *__fastcall og_paddsb(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e6020 op 0x6c */
OgInsn *__fastcall og_paddsw(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e6040 op 0x6d */
OgInsn *__fastcall og_paddusb(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e6060 op 0x6e */
OgInsn *__fastcall og_paddusw(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e6080 op 0x6f */
OgInsn *__fastcall og_paddw(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e60a0 op 0x70 */
OgInsn *__fastcall og_pand(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e60c0 op 0x71 */
OgInsn *__fastcall og_pandn(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e60e0 op 0x72 */
OgInsn *__fastcall og_pcmpeqb(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e6100 op 0x73 */
OgInsn *__fastcall og_pcmpeqd(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e6120 op 0x74 */
OgInsn *__fastcall og_pcmpeqw(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e6140 op 0x75 */
OgInsn *__fastcall og_pcmpgtb(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e6160 op 0x76 */
OgInsn *__fastcall og_pcmpgtd(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e6180 op 0x78 */
OgInsn *__fastcall og_pcmpgtw(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e61a0 op 0x77 */
OgInsn *__fastcall og_pmaddwd(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e61c0 op 0x79 */
OgInsn *__fastcall og_pmulhw(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e61e0 op 0x7a */
OgInsn *__fastcall og_pmullw(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e6200 op 0x7b */
OgInsn *__fastcall og_por(__GLcontext *gc, OgOperand *a, OgOperand *b);       /* 690e6220 op 0x83 */
OgInsn *__fastcall og_pslld(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6240 op 0x5b */
OgInsn *__fastcall og_psllq(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6260 op 0x5c */
OgInsn *__fastcall og_psllw(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6280 op 0x5d */
OgInsn *__fastcall og_psrad(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e62a0 op 0x5e */
OgInsn *__fastcall og_op5f(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e62c0 op 0x5f */
OgInsn *__fastcall og_psraw(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e62e0 op 0x60 */
OgInsn *__fastcall og_psrld(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6300 op 0x61 */
OgInsn *__fastcall og_psrlq(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6320 op 0x62 */
OgInsn *__fastcall og_psrlw(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6340 op 0x63 */
OgInsn *__fastcall og_psubb(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6360 op 0x7c */
OgInsn *__fastcall og_psubd(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6380 op 0x7e */
OgInsn *__fastcall og_psubsb(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e63a0 op 0x7f */
OgInsn *__fastcall og_psubsw(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e63c0 op 0x80 */
OgInsn *__fastcall og_psubusb(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e63e0 op 0x81 */
OgInsn *__fastcall og_psubusw(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e6400 op 0x82 */
OgInsn *__fastcall og_psubw(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6420 op 0x7d */
OgInsn *__fastcall og_punpckhbw(__GLcontext *gc, OgOperand *a, OgOperand *b); /* 690e6440 op 0x64 */
OgInsn *__fastcall og_punpckhdq(__GLcontext *gc, OgOperand *a, OgOperand *b); /* 690e6460 op 0x65 */
OgInsn *__fastcall og_punpckhwd(__GLcontext *gc, OgOperand *a, OgOperand *b); /* 690e6480 op 0x66 */
OgInsn *__fastcall og_punpcklbw(__GLcontext *gc, OgOperand *a, OgOperand *b); /* 690e64a0 op 0x67 */
OgInsn *__fastcall og_punpckldq(__GLcontext *gc, OgOperand *a, OgOperand *b); /* 690e64c0 op 0x68 */
OgInsn *__fastcall og_punpcklwd(__GLcontext *gc, OgOperand *a, OgOperand *b); /* 690e64e0 op 0x69 */
OgInsn *__fastcall og_pxor(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6500 op 0x84 */
OgInsn *__fastcall og_pavgusb(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e6520 op 0x85 */
OgInsn *__fastcall og_pfadd(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6540 op 0x86 */
OgInsn *__fastcall og_pfsub(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6560 op 0x87 */
OgInsn *__fastcall og_pfsubr(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e6580 op 0x88 */
OgInsn *__fastcall og_pfacc(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e65a0 op 0x89 */
OgInsn *__fastcall og_pfmul(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e65c0 op 0x8a */
OgInsn *__fastcall og_pfcmpge(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e65e0 op 0x8b */
OgInsn *__fastcall og_pfcmpgt(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e6600 op 0x8c */
OgInsn *__fastcall og_pfcmpeq(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e6620 op 0x8d */
OgInsn *__fastcall og_pfmin(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6640 op 0x8e */
OgInsn *__fastcall og_pfmax(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6660 op 0x8f */
OgInsn *__fastcall og_pi2fd(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6680 op 0x90 */
OgInsn *__fastcall og_pf2id(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e66a0 op 0x91 */
OgInsn *__fastcall og_pfrcp(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e66c0 op 0x92 */
OgInsn *__fastcall og_pfrsqrt(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e66e0 op 0x93 */
OgInsn *__fastcall og_pfrcpit1(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e6700 op 0x94 */
OgInsn *__fastcall og_pfrsqit1(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e6720 op 0x95 */
OgInsn *__fastcall og_pfrcpit2(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e6740 op 0x96 */
OgInsn *__fastcall og_pmulhrw(__GLcontext *gc, OgOperand *a, OgOperand *b);   /* 690e6760 op 0x97 */
OgInsn *__fastcall og_prefetch_3dn(__GLcontext *gc, OgOperand *a);            /* 690e6780 op 0x99 */
OgInsn *__fastcall og_femms(__GLcontext *gc);                                 /* 690e6790 op 0x98 */
OgInsn *__fastcall og_addps(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e67b0 op 0x9a */
OgInsn *__fastcall og_addss(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e67d0 op 0x9b */
OgInsn *__fastcall og_andnps(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e67f0 op 0x9c */
OgInsn *__fastcall og_andps(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6810 op 0x9d */
OgInsn *__fastcall og_cmpps(__GLcontext *gc, OgOperand *a, OgOperand *b, OgOperand *c);/* 690e6830 op 0x9e */
OgInsn *__fastcall og_op9f(__GLcontext *gc, OgOperand *a, OgOperand *b, OgOperand *c);/* 690e6850 op 0x9f */
OgInsn *__fastcall og_opa0(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6870 op 0xa0 */
OgInsn *__fastcall og_cvtpi2ps(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e6890 op 0xa1 */
OgInsn *__fastcall og_cvtps2pi(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e68b0 op 0xa2 */
OgInsn *__fastcall og_opa3(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e68d0 op 0xa3 */
OgInsn *__fastcall og_cvtss2si(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e68f0 op 0xa4 */
OgInsn *__fastcall og_opa5(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6910 op 0xa5 */
OgInsn *__fastcall og_opa6(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6930 op 0xa6 */
OgInsn *__fastcall og_opa7(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6950 op 0xa7 */
OgInsn *__fastcall og_opa8(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6970 op 0xa8 */
OgInsn *__fastcall og_opa9(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6990 op 0xa9 */
OgInsn *__fastcall og_opaa(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e69b0 op 0xaa */
OgInsn *__fastcall og_opab(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e69d0 op 0xab */
OgInsn *__fastcall og_maxps(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e69f0 op 0xac */
OgInsn *__fastcall og_maxss(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6a10 op 0xad */
OgInsn *__fastcall og_minps(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6a30 op 0xae */
OgInsn *__fastcall og_opaf(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6a50 op 0xaf */
OgInsn *__fastcall og_movaps(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e6a70 op 0xb0 */
OgInsn *__fastcall og_movhps(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e6a90 op 0xb1 */
OgInsn *__fastcall og_movlps(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e6ab0 op 0xb2 */
OgInsn *__fastcall og_movmskps(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e6ad0 op 0xb3 */
OgInsn *__fastcall og_movss(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6af0 op 0xb4: F3 0F 10/11 */
OgInsn *__fastcall og_movups(__GLcontext *gc, OgOperand *a, OgOperand *b);    /* 690e6b10 op 0xb5 */
OgInsn *__fastcall og_mulps(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6b30 op 0xb6 */
OgInsn *__fastcall og_mulss(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6b50 op 0xb7 */
OgInsn *__fastcall og_orps(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6b70 op 0xb8 */
OgInsn *__fastcall og_rcpps(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6b90 op 0xb9 */
OgInsn *__fastcall og_rcpss(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6bb0 op 0xba */
OgInsn *__fastcall og_opbb(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6bd0 op 0xbb */
OgInsn *__fastcall og_opbc(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6bf0 op 0xbc */
OgInsn *__fastcall og_shufps(__GLcontext *gc, OgOperand *a, OgOperand *b, OgOperand *c);/* 690e6c10 op 0xbd */
OgInsn *__fastcall og_opbe(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6c30 op 0xbe */
OgInsn *__fastcall og_opbf(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6c50 op 0xbf */
OgInsn *__fastcall og_opc0(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6c70 op 0xc0 */
OgInsn *__fastcall og_subps(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6c90 op 0xc1 */
OgInsn *__fastcall og_subss(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6cb0 op 0xc2 */
OgInsn *__fastcall og_opc3(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6cd0 op 0xc3 */
OgInsn *__fastcall og_unpckhps(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e6cf0 op 0xc4 */
OgInsn *__fastcall og_unpcklps(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e6d10 op 0xc5 */
OgInsn *__fastcall og_xorps(__GLcontext *gc, OgOperand *a, OgOperand *b);     /* 690e6d30 op 0xc6 */
OgInsn *__fastcall og_opc7(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6d50 op 0xc7 */
OgInsn *__fastcall og_opc8(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6d70 op 0xc8 */
OgInsn *__fastcall og_pextrw(__GLcontext *gc, OgOperand *a, OgOperand *b, OgOperand *c);/* 690e6d90 op 0xc9 */
OgInsn *__fastcall og_pinsrw(__GLcontext *gc, OgOperand *a, OgOperand *b, OgOperand *c);/* 690e6db0 op 0xca */
OgInsn *__fastcall og_opcb(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6dd0 op 0xcb */
OgInsn *__fastcall og_opcc(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6df0 op 0xcc */
OgInsn *__fastcall og_opcd(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6e10 op 0xcd */
OgInsn *__fastcall og_opce(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6e30 op 0xce */
OgInsn *__fastcall og_pmovmskb(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e6e50 op 0xcf */
OgInsn *__fastcall og_opd0(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6e70 op 0xd0 */
OgInsn *__fastcall og_opd1(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6e90 op 0xd1 */
OgInsn *__fastcall og_pshufw(__GLcontext *gc, OgOperand *a, OgOperand *b, OgOperand *c);/* 690e6eb0 op 0xd2 */
OgInsn *__fastcall og_opd3(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6ed0 op 0xd3 */
OgInsn *__fastcall og_opd4(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6ef0 op 0xd4 */
OgInsn *__fastcall og_opd5(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6f10 op 0xd5 */
OgInsn *__fastcall og_prefetch(__GLcontext *gc, OgOperand *a, OgOperand *b);  /* 690e6f30 op 0xd6 */
OgInsn *__fastcall og_opd7(__GLcontext *gc, OgOperand *a, OgOperand *b);      /* 690e6f50 op 0xd7 */

/* the zero-operand emitters and the scheduler's block markers (og_build.c; 690e5870 og_repPrefix, op 0x31, the F3
** prefix, has no caller) */
OgInsn *__fastcall og_fabs(__GLcontext *gc);                                  /* 690e56e0 op 0x49 (D9 E1) */
OgInsn *__fastcall og_fchs(__GLcontext *gc);                                  /* 690e5730 op 0x53 (D9 E0) */
OgInsn *__fastcall og_fldpi(__GLcontext *gc);                                 /* 690e5750 op 0x47 (D9 EB) */
OgInsn *__fastcall og_int3(__GLcontext *gc);                                  /* 690e5790 op 0x2e (CC) */
OgInsn *__fastcall og_fwait(__GLcontext *gc);                                 /* 690e57b0 op 0x2f (9B) */
OgInsn *__fastcall og_movsd(__GLcontext *gc);                                 /* 690e57d0 op 0x32 (A5) */
OgInsn *__fastcall og_nop(__GLcontext *gc);                                   /* 690e57f0 op 0x33 (90) */
OgInsn *__fastcall og_beginBlock(__GLcontext *gc);    /* 690e5810 op 0xd8: og_encode spools the following insns */
OgInsn *__fastcall og_endBlock(__GLcontext *gc);      /* 690e5830 op 0xd9: og_optimizeBlock schedules and encodes */
OgInsn *__fastcall og_rdtsc(__GLcontext *gc);                                 /* 690e5850 op 0x54 (0F 31) */
OgInsn *__fastcall og_opsize(__GLcontext *gc);                                /* 690e58b0 op 0x30 (66 prefix) */
/* the constant pools (3dfx OG.C).  og_fpk (690e9710, og_util.c) takes (gc, double v) with EDX unused: the
** generators call it through a cast, (OgOperand *(__fastcall *)(__GLcontext *, double)), so EDX is not loaded */
OgOperand *__fastcall og_mxk4(__GLcontext *gc, GLushort a, GLushort b, GLushort c, GLushort d);   /* 690e97b0 */

/* generator helpers: `pc` is the running output pointer, `gc` the context */
#define EMIT(i)         (pc = og_encode(gc, pc, (i)))
#define R(r)            (&og_##r)
#define IMM(v)          og_imm(gc, (GLint)(v))
#define MEM(d)          og_mem(gc, IMM(d), NULL, NULL, 1)               /* [d] */
#define MEMR(d, r)      og_mem(gc, IMM(d), R(r), NULL, 1)               /* [r + d] */
#define LABEL(l)        ((l)->addr = pc, og_bindLabel(gc, (l), pc))
/* operand shapes several generators share (their local copies were identical) */
#define MEM0(r)         og_mem(gc, NULL, R(r), NULL, 1)                 /* [r] */
#define MEMB(d, b)      og_mem(gc, IMM(d), (b), NULL, 1)                /* [b + d], b an operand */
#define TAB(a, i)       og_mem(gc, IMM(a), NULL, R(i), 4)               /* [a + i * 4] */
#define BASE(d, b)      og_memBase(gc, IMM(d), R(b))                    /* [b + d] (og_memBase form) */
#define VOFS(m)         ((GLint)&((__GLvertexICD *)0)->m)               /* a vertex field's offset (the /Od code's form) */
/* og_fpk (3dfx OG.C fpk): a memory operand holding the float constant v; called through the cast so EDX
** (unused) is not loaded */
#define FPK(v)          ((OgOperand *(__fastcall *)(__GLcontext *, double))og_fpk)(gc, v)

/* ---- hoisted prototypes: one per function, written from its definition ---- */
/* gl/og_sched.c */
int __fastcall og_canPair(OgInsn *a, OgInsn *b);
int __fastcall og_cDependsOn(OgInsn *a, OgInsn *b);
int __fastcall og_dependsOn(OgInsn *a, OgInsn *b);
GLuint __fastcall og_whatUse(OgOperand *o);
/* sst/od_copycolor.c */
GLubyte *__fastcall CopyColor(__GLcontext *gc, GLubyte *pc, GLenum type, GLuint size, OgOperand *work, OgOperand *rr,
                              OgOperand *gg, OgOperand *bb, OgOperand *aa, GLuint needs);
/* sst/od_copyfog.c */
GLubyte *__fastcall CopyFogCoord(__GLcontext *gc, GLubyte *pc, GLenum type, OgOperand *work);
/* sst/od_copynormal.c */
GLubyte *__fastcall CopyNormal(__GLcontext *gc, GLubyte *pc, GLenum type, OgOperand *work);
/* sst/od_doload.c */
GLubyte *__fastcall do_load(__GLcontext *gc, GLubyte *pc, OgOperand *src, GLenum type, OgOperand *scratch);
/* sst/od_domatrix.c */
GLubyte *__fastcall doMatrix(__GLcontext *gc, GLubyte *pc, OgOperand *outx, OgOperand *outy, OgOperand *outz,
                             OgOperand *outw, GLenum matrixType, const __GLfloat *mat, GLuint vertexSize);
/* sst/od_scaleby.c */
GLubyte *__fastcall og_scaleby(__GLcontext *gc, GLubyte *pc, OgOperand *reg, OgOperand *n);
/* sst/sst_glideinit.c */
GLubyte *__fastcall og_callOperand(__GLcontext *gc, GLubyte *pc, OgOperand *target, OgOperand *line);
/* sst/sst_linepoint.c */
GLubyte *__fastcall og_lerpFloat(__GLcontext *gc, GLubyte *pc, OgOperand *disp);
/* sst/sst_ogvtx.c */
GLubyte *__fastcall og_minSigned(__GLcontext *gc, GLubyte *pc, OgOperand *d, OgOperand *s);
/* sst/sst_tribuild.c */
GLubyte *__fastcall hwCopyArrayVertex(__GLcontext *gc, GLubyte *pc, OgOperand *idx, GLint *off, GLuint flags);
/* sst/sst_trigen.c */
GLubyte *__fastcall og_fifoMakeRoom(__GLcontext *gc, GLubyte *pc, OgOperand *size, OgLabel *makeRoom);
/* sst/sst_vtxcopy.c */
GLubyte *__fastcall hwCopyVertexFloat(__GLcontext *gc, GLubyte *pc, OgOperand *src, GLint *off, GLubyte flags,
                                      OgOperand *tmp);
GLubyte *__fastcall hwCopyVertexIndexed(__GLcontext *gc, GLubyte *pc, OgOperand *src, GLint *off, GLubyte flags,
                                        OgOperand *tmp, OgOperand *unused);
GLubyte *__fastcall hwCopyVertexPacked(__GLcontext *gc, GLubyte *pc, OgOperand *src, GLint *off, GLubyte flags,
                                       OgOperand *tmp, OgOperand *csrc);
/* ---- end of hoisted prototypes ---- */

#endif /* __OG_H__ */

/*
** og_regs.c -- the OG code generator's static register operands (original 690f5aa0..690f5d9f).
** Each is a 24-byte operand record {kind, number}: kind 3 = 32-bit register, 1 = 8-bit register,
** 4 = x87 st(i), 5 = MMX / SSE register (encoded by register number).  Generated code passes their
** addresses to the og_* instruction helpers (og_mov(gc, &og_eax, og_imm(gc, 1)) ...).
*/
#include "wglpriv.h"

__GLogOperand og_eax = { 3, 0 };   /* 690f5aa0 */
__GLogOperand og_ecx = { 3, 1 };   /* 690f5ab8 */
__GLogOperand og_edx = { 3, 2 };   /* 690f5ad0 */
__GLogOperand og_ebx = { 3, 3 };   /* 690f5ae8 */
__GLogOperand og_esp = { 3, 4 };   /* 690f5b00 */
__GLogOperand og_ebp = { 3, 5 };   /* 690f5b18 */
__GLogOperand og_esi = { 3, 6 };   /* 690f5b30 */
__GLogOperand og_edi = { 3, 7 };   /* 690f5b48 */
__GLogOperand og_al = { 1, 0 };   /* 690f5b60 */
__GLogOperand og_cl = { 1, 1 };   /* 690f5b78 */
__GLogOperand og_dl = { 1, 2 };   /* 690f5b90 */
__GLogOperand og_bl = { 1, 3 };   /* 690f5ba8 */
__GLogOperand og_ah = { 1, 4 };   /* 690f5bc0 */
__GLogOperand og_ch = { 1, 5 };   /* 690f5bd8 */
__GLogOperand og_dh = { 1, 6 };   /* 690f5bf0 */
__GLogOperand og_bh = { 1, 7 };   /* 690f5c08 */
__GLogOperand og_st0 = { 4, 0 };   /* 690f5c20 */
__GLogOperand og_st1 = { 4, 1 };   /* 690f5c38 */
__GLogOperand og_st2 = { 4, 2 };   /* 690f5c50 */
__GLogOperand og_st3 = { 4, 3 };   /* 690f5c68 */
__GLogOperand og_st4 = { 4, 4 };   /* 690f5c80 */
__GLogOperand og_st5 = { 4, 5 };   /* 690f5c98 */
__GLogOperand og_st6 = { 4, 6 };   /* 690f5cb0 */
__GLogOperand og_st7 = { 4, 7 };   /* 690f5cc8 */
__GLogOperand og_mm0 = { 5, 0 };   /* 690f5ce0 */
__GLogOperand og_mm1 = { 5, 1 };   /* 690f5cf8 */
__GLogOperand og_mm2 = { 5, 2 };   /* 690f5d10 */
__GLogOperand og_mm3 = { 5, 3 };   /* 690f5d28 */
__GLogOperand og_mm4 = { 5, 4 };   /* 690f5d40 */
__GLogOperand og_mm5 = { 5, 5 };   /* 690f5d58 */
__GLogOperand og_mm6 = { 5, 6 };   /* 690f5d70 */
__GLogOperand og_mm7 = { 5, 7 };   /* 690f5d88 */

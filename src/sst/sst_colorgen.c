/*
** sst_colorgen.c -- og_genColor4f: the og_* generator of the glColor4f routine (userColor, the colour
** clamped and scaled to 0..255, the packed ARGB) in its 3DNow, SSE and x87 versions.
*/
#include "wglpriv.h"
#include "og.h"
#include <glide.h>
#include "icdglob.h"

/* ---- og_* code generator (og.h) ---- */

/* g_floatFrac8Bias (icdglob.h): 49152.0f (1.5 * 2^15): x + 49152 leaves x * 256 in the low mantissa bits
   (above 0x400000) of the float's bit pattern */

#define IDX(b, i)       og_mem(gc, NULL, R(b), R(i), 4)             /* [b + i * 4] */

/* the generated routine is __stdcall (r, g, b, a): [esp + ARG(i)] on entry */
#define ARG(i)          (4 + (i) * 4)

/* 69045e10 -- og_genColor4f: the glColor4f routine: userColor = (r, g, b, a); color = clamp(c, 0, 1) * 255;
   packedColor = color as 0xAARRGGBB; ret 0x10.  *pool is the constant pool cursor: four 1.0f (unused)
   and four 255.0f are appended.  Returns the new pc. */
GLubyte *__fastcall og_genColor4f(__GLcontext *gc, GLubyte *pc, GLuint **pool)
{
    OgLabel *clamp, *pack, *loop;
    GLuint *k;
    OgInsn *tail;
    int i;

    clamp = og_newLabel(gc);
    pack = og_newLabel(gc);
    og_newLabel(gc);
    og_newLabel(gc);
    k = *pool;
    for (i = 0; i < 4; i++) {
        k[i] = 0x3f800000;                      /* 1.0f */
        k[i + 4] = 0x437f0000;                  /* 255.0f */
    }
    *pool += 8;

    if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
        /* 3DNow: mm0 = r, g, mm1 = b, a; max 0, times 255, min 255 */
        EMIT(og_movq(gc, R(mm0), MEMR(ARG(0), esp)));
        EMIT(og_movq(gc, R(mm1), MEMR(ARG(2), esp)));
        EMIT(og_movq(gc, R(mm2), MEM(k + 4)));
        EMIT(og_pxor(gc, R(mm3), R(mm3)));
        EMIT(og_movq(gc, R(mm6), R(mm0)));
        EMIT(og_pfmax(gc, R(mm0), R(mm3)));
        EMIT(og_movq(gc, R(mm7), R(mm1)));
        EMIT(og_pfmax(gc, R(mm1), R(mm3)));
        EMIT(og_movq(gc, MEM(&gc->state.current.userColor.r), R(mm6)));
        EMIT(og_pfmul(gc, R(mm0), R(mm2)));
        EMIT(og_movq(gc, MEM(&gc->state.current.userColor.b), R(mm7)));
        EMIT(og_pfmul(gc, R(mm1), R(mm2)));
        EMIT(og_pfmin(gc, R(mm0), R(mm2)));
        EMIT(og_pfmin(gc, R(mm1), R(mm2)));
        EMIT(og_movq(gc, MEM(&gc->state.current.color.r), R(mm0)));
        EMIT(og_pf2id(gc, R(mm0), R(mm0)));
        EMIT(og_movq(gc, MEM(&gc->state.current.color.b), R(mm1)));
        EMIT(og_pf2id(gc, R(mm1), R(mm1)));
        EMIT(og_packssdw(gc, R(mm0), R(mm1)));
        EMIT(og_packuswb(gc, R(mm0), R(mm0)));
        /* eax = 0xAABBGGRR -> 0xAARRGGBB */
        EMIT(og_movd(gc, R(eax), R(mm0)));
        EMIT(og_mov(gc, R(edx), R(eax)));
        EMIT(og_and(gc, R(eax), IMM(0xff00ff)));
        EMIT(og_and(gc, R(edx), IMM(0xff00ff00)));
        EMIT(og_rol(gc, R(eax), IMM(16)));
        EMIT(og_or(gc, R(eax), R(edx)));
        EMIT(og_mov(gc, MEM(&gc->state.current.packedColor), R(eax)));
        tail = og_femms(gc);
    } else if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        /* SSE: xmm1 = r, g, b, a.  Original bug: b and a are loaded only after their userColor stores,
           so userColor.b, .a get r, g (the last two loads are dead) */
        EMIT(og_movups(gc, R(mm1), MEMR(ARG(0), esp)));
        EMIT(og_mov(gc, R(eax), MEMR(ARG(0), esp)));
        EMIT(og_mov(gc, R(edx), MEMR(ARG(1), esp)));
        EMIT(og_xorps(gc, R(mm2), R(mm2)));
        EMIT(og_mov(gc, MEM(&gc->state.current.userColor.r), R(eax)));
        EMIT(og_mov(gc, MEM(&gc->state.current.userColor.g), R(edx)));
        EMIT(og_maxps(gc, R(mm1), R(mm2)));
        EMIT(og_mov(gc, MEM(&gc->state.current.userColor.b), R(eax)));
        EMIT(og_mov(gc, MEM(&gc->state.current.userColor.a), R(edx)));
        EMIT(og_movaps(gc, R(mm4), MEM(k + 4)));
        EMIT(og_mov(gc, R(eax), MEMR(ARG(2), esp)));
        EMIT(og_mov(gc, R(edx), MEMR(ARG(3), esp)));
        EMIT(og_mulps(gc, R(mm1), R(mm4)));
        EMIT(og_minps(gc, R(mm1), R(mm4)));
        EMIT(og_movups(gc, MEM(&gc->state.current.color), R(mm1)));
        EMIT(og_cvtps2pi(gc, R(mm0), R(mm1)));
        EMIT(og_shufps(gc, R(mm1), R(mm1), IMM(0xe)));
        EMIT(og_cvtps2pi(gc, R(mm1), R(mm1)));
        EMIT(og_packssdw(gc, R(mm0), R(mm1)));
        EMIT(og_pshufw(gc, R(mm0), R(mm0), IMM(0xc6)));     /* words r, g, b, a -> b, g, r, a */
        EMIT(og_packuswb(gc, R(mm0), R(mm0)));
        EMIT(og_movd(gc, MEM(&gc->state.current.packedColor), R(mm0)));
        tail = og_emms(gc);
    } else {
        /* x87: esi = &userColor, edi = &color; the arguments at ARG(i) + 0x10 after the four pushes */
        EMIT(og_push(gc, R(esi)));
        EMIT(og_mov(gc, R(esi), IMM(&gc->state.current.userColor)));
        EMIT(og_push(gc, R(edi)));
        EMIT(og_mov(gc, R(edi), IMM(&gc->state.current.color)));
        EMIT(og_push(gc, R(ebx)));
        EMIT(og_push(gc, R(ebp)));
        EMIT(og_fld(gc, MEM(&gc->constants.val255)));
        EMIT(og_mov(gc, R(eax), MEMR(ARG(0) + 0x10, esp)));
        EMIT(og_mov(gc, MEMR(0, esi), R(eax)));
        EMIT(og_fld(gc, MEMR(ARG(0) + 0x10, esp)));
        EMIT(og_fmul(gc, R(st1)));
        EMIT(og_mov(gc, R(eax), MEMR(ARG(1) + 0x10, esp)));
        EMIT(og_mov(gc, MEMR(4, esi), R(eax)));
        EMIT(og_fld(gc, MEMR(ARG(1) + 0x10, esp)));
        EMIT(og_fmul(gc, R(st2)));
        EMIT(og_mov(gc, R(eax), MEMR(ARG(2) + 0x10, esp)));
        EMIT(og_mov(gc, MEMR(8, esi), R(eax)));
        EMIT(og_fld(gc, MEMR(ARG(2) + 0x10, esp)));
        EMIT(og_fmul(gc, R(st3)));
        EMIT(og_mov(gc, R(eax), MEMR(ARG(3) + 0x10, esp)));
        EMIT(og_mov(gc, MEMR(0xc, esi), R(eax)));
        EMIT(og_fld(gc, MEMR(ARG(3) + 0x10, esp)));
        EMIT(og_fmulp(gc, R(st4), R(st0)));
        /* st0..st3 = b, g, r, a * 255: store each to color (unclamped) and add the 49152 bias */
        EMIT(og_fxch(gc, R(st2)));
        EMIT(og_fst(gc, MEMR(0, edi)));
        EMIT(og_fadd(gc, MEM(g_floatFrac8Bias)));
        EMIT(og_fxch(gc, R(st1)));
        EMIT(og_fst(gc, MEMR(4, edi)));
        EMIT(og_fadd(gc, MEM(g_floatFrac8Bias)));
        EMIT(og_fxch(gc, R(st2)));
        EMIT(og_fst(gc, MEMR(8, edi)));
        EMIT(og_fadd(gc, MEM(g_floatFrac8Bias)));
        EMIT(og_fxch(gc, R(st3)));
        EMIT(og_fst(gc, MEMR(0xc, edi)));
        EMIT(og_fadd(gc, MEM(g_floatFrac8Bias)));
        EMIT(og_fxch(gc, R(st1)));
        /* the biased bit patterns through the red zone below esp: eax = r, edx = a, ebx = g, ecx = b */
        EMIT(og_fstp(gc, MEMR(-4, esp)));
        EMIT(og_mov(gc, R(eax), MEMR(-4, esp)));
        EMIT(og_fstp(gc, MEMR(-8, esp)));
        EMIT(og_mov(gc, R(edx), MEMR(-8, esp)));
        EMIT(og_fstp(gc, MEMR(-0xc, esp)));
        EMIT(og_mov(gc, R(ebx), MEMR(-0xc, esp)));
        EMIT(og_fstp(gc, MEMR(-0x10, esp)));
        EMIT(og_mov(gc, R(ecx), MEMR(-0x10, esp)));
        /* (bits & 0x7fff00) - 0x400000 = the integer part << 8; outside 0..255 sets bits 16..31 */
        EMIT(og_and(gc, R(eax), IMM(0x7fff00)));
        EMIT(og_and(gc, R(ebx), IMM(0x7fff00)));
        EMIT(og_and(gc, R(ecx), IMM(0x7fff00)));
        EMIT(og_and(gc, R(edx), IMM(0x7fff00)));
        EMIT(og_sub(gc, R(eax), IMM(0x400000)));
        EMIT(og_sub(gc, R(ebx), IMM(0x400000)));
        EMIT(og_sub(gc, R(ecx), IMM(0x400000)));
        EMIT(og_sub(gc, R(edx), IMM(0x400000)));
        EMIT(og_mov(gc, R(ebp), R(eax)));
        EMIT(og_or(gc, R(ebp), R(ebx)));
        EMIT(og_or(gc, R(ebp), R(ecx)));
        EMIT(og_or(gc, R(ebp), R(edx)));
        EMIT(og_test(gc, R(ebp), IMM(0xffff0000)));
        EMIT(og_jne(gc, clamp));

        /* pack r << 16 | g << 8 | b | a << 24 (each still << 8) */
        LABEL(pack);
        EMIT(og_shl(gc, R(eax), IMM(8)));
        EMIT(og_shr(gc, R(ecx), IMM(8)));
        EMIT(og_shl(gc, R(edx), IMM(16)));
        EMIT(og_pop(gc, R(ebp)));
        EMIT(og_or(gc, R(eax), R(ebx)));
        EMIT(og_pop(gc, R(ebx)));
        EMIT(og_or(gc, R(ecx), R(edx)));
        EMIT(og_pop(gc, R(edi)));
        EMIT(og_or(gc, R(eax), R(ecx)));
        EMIT(og_pop(gc, R(esi)));
        EMIT(og_mov(gc, MEM(&gc->state.current.packedColor), R(eax)));
        EMIT(og_ret_imm(gc, IMM(0x10)));

        /* a component outside 0..255: clamp color[3..0] in place on the bit patterns
           (negative -> 0, above 255.0f -> 255.0f), bias them again and pack */
        LABEL(clamp);
        loop = og_newLabel(gc);
        EMIT(og_mov(gc, R(ecx), IMM(3)));
        LABEL(loop);
        EMIT(og_mov(gc, R(eax), IDX(edi, ecx)));
        EMIT(og_mov(gc, R(ebx), R(eax)));
        EMIT(og_sar(gc, R(ebx), IMM(31)));                   /* sar: ebx = sign ? 0 : -1 after the xor */
        EMIT(og_xor(gc, R(ebx), IMM(0xffffffff)));
        EMIT(og_mov(gc, R(edx), IMM(0x437f0000)));             /* 255.0f */
        EMIT(og_sub(gc, R(edx), R(eax)));
        EMIT(og_sbb(gc, R(ebp), R(ebp)));
        EMIT(og_and(gc, R(edx), R(ebp)));
        EMIT(og_add(gc, R(eax), R(edx)));
        EMIT(og_and(gc, R(eax), R(ebx)));
        EMIT(og_mov(gc, IDX(edi, ecx), R(eax)));
        EMIT(og_fld(gc, IDX(edi, ecx)));
        EMIT(og_fadd(gc, MEM(g_floatFrac8Bias)));
        EMIT(og_dec(gc, R(ecx)));
        EMIT(og_jge(gc, loop));
        /* st0..st3 = r, g, b, a: edx = a, ebx = g, ecx = b, eax = r; the 0x400000 bias is masked off */
        EMIT(og_fxch(gc, R(st3)));
        EMIT(og_fstp(gc, MEMR(-4, esp)));
        EMIT(og_mov(gc, R(edx), MEMR(-4, esp)));
        EMIT(og_fstp(gc, MEMR(-8, esp)));
        EMIT(og_mov(gc, R(ebx), MEMR(-8, esp)));
        EMIT(og_fstp(gc, MEMR(-0xc, esp)));
        EMIT(og_mov(gc, R(ecx), MEMR(-0xc, esp)));
        EMIT(og_fstp(gc, MEMR(-0x10, esp)));
        EMIT(og_mov(gc, R(eax), MEMR(-0x10, esp)));
        EMIT(og_and(gc, R(eax), IMM(0xff00)));
        EMIT(og_and(gc, R(ebx), IMM(0xff00)));
        EMIT(og_and(gc, R(ecx), IMM(0xff00)));
        EMIT(og_and(gc, R(edx), IMM(0xff00)));
        return og_encode(gc, pc, og_jmp(gc, pack));
    }
    EMIT(tail);
    return og_encode(gc, pc, og_ret_imm(gc, IMM(0x10)));
}

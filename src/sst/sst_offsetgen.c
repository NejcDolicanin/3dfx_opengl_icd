/*
** sst_offsetgen.c -- og_genOffsetFifo: generates the three bufRec[9] routines (gc->hwcx.genProcs9) that
** write the polygon-offset depth bias straight into the command FIFO: [0] makes room in the FIFO,
** [1] / [2] compute units * scale + factor * max |dz/dx|, |dz/dy| for a triangle whose vertices are in
** eax / ebx / ecx (fixed offsets / indexed hardware vertices).  Each computation has an SSE (cpuCaps
** & 4), a 3DNow (& 2) and an x87 version.
*/
#include "wglpriv.h"
#include "og.h"
#include "icdprotos.h"
#include <glide.h>

/* The bufRec[9] data page (a cache entry's valB, as sst_trigen.c's TriGenBlock): the routines, then the
** constants and scratch they address absolutely */
typedef struct OffsetGenBlockRec {
    GLuint      proc[3];                    /* +0x00  the three routines (-> hwcx.genProcs9) */
    GLuint      saveEax;                    /* +0x0c  eax across [1] / [2] */
    GLuint      pad10[3];
    __GLfloat   area;                       /* +0x1c  edx at entry: the triangle's area (the gradients are divided by it) */
    GLuint      absMask[4];                 /* +0x20  0x7fffffff (andps / pand: |x|) */
    __GLfloat   scaled[2];                  /* +0x30  x87: two edge deltas times 1 / area */
    __GLfloat   grad[2];                    /* +0x38  x87: dz/dx, dz/dy (made absolute in place) */
    GLint       gradBias[2];                /* +0x40  x87: factor * |dz/dx|, factor * |dz/dy| */
    GLuint      pad48[10];
    GLint       unitsBias;                  /* +0x70  polygon.units scaled to the depth buffer (* 256 or << 8) */
    GLint       slopeBias;                  /* +0x74  factor * max(|dz/dx|, |dz/dy|) */
    __GLfloat   zScale24;                   /* +0x78  256.0f: units for 24-bit z with fx.zPrecision32 */
} OffsetGenBlock;

/* the 8-byte constant pool entry of a double: a float-only callee (no EDX load) */
#define W_FCONST(gc, v)  ((OgOperand *(__fastcall *)(__GLcontext *, double))og_fpk)(gc, v)
#define HWV(k)          ((GLuint)gc->hwcx.vtxBase + gc->hwcx.vtxCapacity * (k))

/* 6902ad50 -- og_genOffsetFifo: generates the polygon-offset command-FIFO routines bufRec[9] (sst_offsetgen.c
** header) */
void __fastcall og_genOffsetFifo(__GLcontext *gc)
{
    OffsetGenBlock *blk;
    GLint fifo, room, info, ptr;      /* info: the transport info copy; ptr = &info->fifoPtr (+0xc), a separate variable */
    GLubyte *pc;
    OgLabel *makeRoom, *l1, *l2;
    GLuint zbits;
    int i;

    fifo = (GLint)gc->hwcx.cmdTransportInfo;
    info = fifo;
    if (fifo == 0)
        return;
    {   /* key lives in this block (its slot is reused) */
        GLuint key;
        zbits = (gc->modes.depthBits > 16 && gc->fx.zPrecision32) ? 4 : 0;
        key = (gc->modes.depthBits > 16 ? 2 : 0) | zbits | 1;
        if (((GLint (__fastcall *)(__GLcontext *, __GLhwBufRecICD *, GLuint *, OffsetGenBlock **))gc->hwcx.bufRec[9].alloc)
                (gc, &gc->hwcx.bufRec[9], &key, &blk)) {
            gc->hwcx.genProcs9[0] = (__GLproc)blk->proc[0];
            gc->hwcx.genProcs9[1] = (__GLproc)blk->proc[1];
            gc->hwcx.genProcs9[2] = (__GLproc)blk->proc[2];
            return;
        }
        pc = hwPageCacheInsert(gc, &gc->hwcx.bufRec[9], &key, &blk);
    }
    og_reset(gc);
    makeRoom = og_newFixup(gc);
    for (i = 0; i < 4; i++)
        blk->absMask[i] = 0x7fffffff;
    blk->zScale24 = 256.0f;

    /* [0]: make room for edi bytes */
    LABEL(makeRoom);
    EMIT(og_push(gc, R(ecx)));
    EMIT(og_push(gc, R(edx)));
    EMIT(og_push(gc, IMM(0)));
    EMIT(og_push(gc, IMM(0)));
    EMIT(og_push(gc, R(edi)));
    EMIT(og_call(gc, MEM(&gc->hwcx.grCommandTransportMakeRoomExt2)));
    EMIT(og_pop(gc, &og_edx));
    EMIT(og_pop(gc, &og_ecx));
    fifo += 0x14;
    room = fifo;
    EMIT(og_mov(gc, R(eax), MEM(fifo)));
    EMIT(og_sub(gc, &og_eax, R(edi)));
    EMIT(og_ret(gc));
    blk->proc[0] = (GLuint)pc;
    gc->hwcx.genProcs9[0] = (__GLproc)blk->proc[0];

    /* [1]: the bias of a triangle in eax / ebx / ecx */
    EMIT(og_mov(gc, MEM(&blk->area), R(edx)));
    pc = og_lostContextCheck(gc, pc, &og_edx, IMM(0));
    EMIT(og_mov(gc, MEM(&blk->saveEax), R(eax)));
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        EMIT(og_movss(gc, &og_mm0, MEM(&gc->state.polygon.units)));
        if (gc->modes.depthBits > 16 && gc->fx.zPrecision32) {
            EMIT(og_movss(gc, &og_mm1, MEM(&blk->zScale24)));
            EMIT(og_mulps(gc, &og_mm0, &og_mm1));
            EMIT(og_cvtss2si(gc, R(eax), &og_mm0));
        } else {
            EMIT(og_cvtss2si(gc, R(eax), &og_mm0));
            if (gc->modes.depthBits > 16)
                EMIT(og_shl(gc, &og_eax, IMM(8)));
        }
        EMIT(og_mov(gc, MEM(&blk->unitsBias), R(eax)));
    } else if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
        EMIT(og_movd(gc, R(mm0), MEM(&gc->state.polygon.units)));
        if (gc->modes.depthBits > 16 && gc->fx.zPrecision32) {
            EMIT(og_pfmul(gc, &og_mm0, MEM(&blk->zScale24)));
            EMIT(og_pf2id(gc, &og_mm0, &og_mm0));
        } else {
            EMIT(og_pf2id(gc, &og_mm0, &og_mm0));
            if (gc->modes.depthBits > 16)
                EMIT(og_pslld(gc, &og_mm0, IMM(8)));
        }
        EMIT(og_movd(gc, MEM(&blk->unitsBias), R(mm0)));
    } else {
        EMIT(og_fld(gc, MEM(&gc->state.polygon.units)));
        if (gc->modes.depthBits > 16 && gc->fx.zPrecision32) {
            EMIT(og_fmul(gc, MEM(&blk->zScale24)));
            EMIT(og_fistp(gc, MEM(&blk->unitsBias)));
            EMIT(og_mov(gc, R(eax), MEM(&blk->unitsBias)));
        } else {
            EMIT(og_fistp(gc, MEM(&blk->unitsBias)));
            EMIT(og_mov(gc, R(eax), MEM(&blk->unitsBias)));
            if (gc->modes.depthBits > 16)
                EMIT(og_shl(gc, &og_eax, IMM(8)));
        }
        EMIT(og_mov(gc, MEM(&blk->unitsBias), R(eax)));
    }
    l1 = og_newLabel(gc);
    EMIT(og_mov(gc, R(eax), MEM(room)));
    EMIT(og_mov(gc, R(edi), IMM(8)));
    EMIT(og_sub(gc, &og_eax, R(edi)));
    EMIT(og_jge(gc, l1));
    EMIT(og_call(gc, makeRoom));
    LABEL(l1);
    EMIT(og_mov(gc, MEM(room), R(eax)));
    EMIT(og_mov(gc, R(eax), MEM(&blk->saveEax)));
    ptr = info + 0xc;                   /* a new variable (not info += 0xc): it gets ebp from here on */
    EMIT(og_mov(gc, R(edi), MEM(ptr)));
    EMIT(og_mov(gc, og_mem(gc, 0, &og_edi, 0, 1), IMM(0x18261)));
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        EMIT(og_movss(gc, &og_mm0, MEM(&blk->area)));
        EMIT(og_movss(gc, &og_mm4, og_mem(gc, IMM(8), &og_eax, 0, 1)));
        EMIT(og_subss(gc, &og_mm4, og_mem(gc, IMM(8), &og_ecx, 0, 1)));
        EMIT(og_rcpss(gc, &og_mm6, &og_mm0));
        EMIT(og_movss(gc, &og_mm5, og_mem(gc, IMM(8), &og_ebx, 0, 1)));
        EMIT(og_subss(gc, &og_mm5, og_mem(gc, IMM(8), &og_ecx, 0, 1)));
        EMIT(og_movss(gc, &og_mm0, og_mem(gc, IMM(4), &og_eax, 0, 1)));
        EMIT(og_subss(gc, &og_mm0, og_mem(gc, IMM(4), &og_ecx, 0, 1)));
        EMIT(og_movss(gc, &og_mm1, og_mem(gc, IMM(4), &og_ebx, 0, 1)));
        EMIT(og_subss(gc, &og_mm1, og_mem(gc, IMM(4), &og_ecx, 0, 1)));
        EMIT(og_movss(gc, &og_mm2, og_mem(gc, IMM(0), &og_eax, 0, 1)));
        EMIT(og_subss(gc, &og_mm2, og_mem(gc, IMM(0), &og_ecx, 0, 1)));
        EMIT(og_movss(gc, &og_mm3, og_mem(gc, IMM(0), &og_ebx, 0, 1)));
        EMIT(og_subss(gc, &og_mm3, og_mem(gc, IMM(0), &og_ecx, 0, 1)));
        EMIT(og_mulss(gc, &og_mm0, R(mm6)));
        EMIT(og_mulss(gc, &og_mm1, R(mm6)));
        EMIT(og_mulss(gc, &og_mm2, R(mm6)));
        EMIT(og_mulss(gc, &og_mm3, R(mm6)));
        EMIT(og_mulss(gc, &og_mm0, R(mm5)));
        EMIT(og_mulss(gc, &og_mm1, R(mm4)));
        EMIT(og_mulss(gc, &og_mm2, R(mm5)));
        EMIT(og_mulss(gc, &og_mm3, R(mm4)));
        EMIT(og_subss(gc, &og_mm1, R(mm0)));
        EMIT(og_subss(gc, &og_mm2, R(mm3)));
        EMIT(og_movss(gc, &og_mm5, MEM(&blk->absMask)));
        EMIT(og_andps(gc, &og_mm1, &og_mm5));
        EMIT(og_andps(gc, &og_mm2, &og_mm5));
        EMIT(og_maxss(gc, &og_mm1, &og_mm2));
        EMIT(og_mulss(gc, &og_mm1, MEM(&gc->state.polygon.factor)));
        EMIT(og_cvtss2si(gc, MEM(&blk->slopeBias), &og_mm1));
    } else if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
        EMIT(og_movd(gc, R(mm0), MEM(&blk->area)));
        EMIT(og_movd(gc, R(mm4), og_mem(gc, IMM(8), &og_eax, 0, 1)));
        EMIT(og_movd(gc, R(mm7), og_mem(gc, IMM(8), &og_ecx, 0, 1)));
        EMIT(og_pfrcp(gc, &og_mm6, &og_mm0));
        EMIT(og_pfsub(gc, &og_mm4, &og_mm7));
        EMIT(og_movd(gc, R(mm5), og_mem(gc, IMM(8), &og_ebx, 0, 1)));
        EMIT(og_pfsub(gc, &og_mm5, &og_mm7));
        EMIT(og_movd(gc, R(mm0), og_mem(gc, IMM(4), &og_eax, 0, 1)));
        EMIT(og_movd(gc, R(mm7), og_mem(gc, IMM(4), &og_ecx, 0, 1)));
        EMIT(og_pfsub(gc, &og_mm0, &og_mm7));
        EMIT(og_movd(gc, R(mm1), og_mem(gc, IMM(4), &og_ebx, 0, 1)));
        EMIT(og_pfsub(gc, &og_mm1, &og_mm7));
        EMIT(og_movd(gc, R(mm2), og_mem(gc, IMM(0), &og_eax, 0, 1)));
        EMIT(og_movd(gc, R(mm7), og_mem(gc, IMM(0), &og_ecx, 0, 1)));
        EMIT(og_pfsub(gc, &og_mm2, &og_mm7));
        EMIT(og_movd(gc, R(mm3), og_mem(gc, IMM(0), &og_ebx, 0, 1)));
        EMIT(og_pfsub(gc, &og_mm3, &og_mm7));
        EMIT(og_pfmul(gc, &og_mm4, R(mm6)));
        EMIT(og_pfmul(gc, &og_mm5, R(mm6)));
        EMIT(og_pfmul(gc, &og_mm0, R(mm5)));
        EMIT(og_pfmul(gc, &og_mm1, R(mm4)));
        EMIT(og_pfmul(gc, &og_mm2, R(mm5)));
        EMIT(og_pfmul(gc, &og_mm3, R(mm4)));
        EMIT(og_pfsub(gc, &og_mm1, &og_mm0));
        EMIT(og_pfsub(gc, &og_mm2, &og_mm3));
        EMIT(og_pand(gc, &og_mm1, MEM(&blk->absMask)));
        EMIT(og_pand(gc, &og_mm2, MEM(&blk->absMask)));
        EMIT(og_pfmax(gc, &og_mm1, &og_mm2));
        EMIT(og_pfmul(gc, &og_mm1, MEM(&gc->state.polygon.factor)));
        EMIT(og_pf2id(gc, &og_mm1, &og_mm1));
        EMIT(og_movd(gc, MEM(&blk->slopeBias), R(mm1)));
    } else {
        l1 = og_newLabel(gc);
        l2 = og_newLabel(gc);
        EMIT(og_fld(gc, MEM(&blk->area)));
        EMIT(og_fdivr(gc, W_FCONST(gc, 1.0)));
        EMIT(og_fld(gc, og_mem(gc, IMM(8), &og_eax, 0, 1)));
        EMIT(og_fsub(gc, og_mem(gc, IMM(8), &og_ecx, 0, 1)));
        EMIT(og_fld(gc, og_mem(gc, IMM(8), &og_ebx, 0, 1)));
        EMIT(og_fsub(gc, og_mem(gc, IMM(8), &og_ecx, 0, 1)));
        EMIT(og_fld(gc, og_mem(gc, IMM(4), &og_eax, 0, 1)));
        EMIT(og_fsub(gc, og_mem(gc, IMM(4), &og_ecx, 0, 1)));
        EMIT(og_fld(gc, og_mem(gc, IMM(4), &og_ebx, 0, 1)));
        EMIT(og_fsub(gc, og_mem(gc, IMM(4), &og_ecx, 0, 1)));
        EMIT(og_fld(gc, og_mem(gc, IMM(0), &og_eax, 0, 1)));
        EMIT(og_fsub(gc, og_mem(gc, IMM(0), &og_ecx, 0, 1)));
        EMIT(og_fld(gc, og_mem(gc, IMM(0), &og_ebx, 0, 1)));
        EMIT(og_fsub(gc, og_mem(gc, IMM(0), &og_ecx, 0, 1)));
        EMIT(og_fxch(gc, &og_st4));
        EMIT(og_fmul(gc, R(st6)));
        EMIT(og_fstp(gc, MEM(&blk->scaled[0])));
        EMIT(og_fxch(gc, &og_st4));
        EMIT(og_fmul(gc, R(st5)));
        EMIT(og_fstp(gc, MEM(&blk->scaled[1])));
        EMIT(og_fmul(gc, MEM(&blk->scaled[1])));
        EMIT(og_fxch(gc, &og_st1));
        EMIT(og_fmul(gc, MEM(&blk->scaled[0])));
        EMIT(og_fsubp(gc, &og_st1, &og_st0));
        EMIT(og_fstp(gc, MEM(&blk->grad[0])));
        EMIT(og_mov(gc, R(edx), MEM(&blk->grad[0])));
        EMIT(og_and(gc, &og_edx, IMM(0x7fffffff)));
        EMIT(og_mov(gc, MEM(&blk->grad[0]), R(edx)));
        EMIT(og_fld(gc, MEM(&blk->grad[0])));
        EMIT(og_fmul(gc, MEM(&gc->state.polygon.factor)));
        EMIT(og_fistp(gc, MEM(&blk->gradBias[0])));
        EMIT(og_fmul(gc, MEM(&blk->scaled[1])));
        EMIT(og_fxch(gc, &og_st1));
        EMIT(og_fmul(gc, MEM(&blk->scaled[0])));
        EMIT(og_fxch(gc, &og_st1));
        EMIT(og_fsubp(gc, &og_st1, &og_st0));
        EMIT(og_fstp(gc, MEM(&blk->grad[1])));
        EMIT(og_mov(gc, R(edx), MEM(&blk->grad[1])));
        EMIT(og_and(gc, &og_edx, IMM(0x7fffffff)));
        EMIT(og_mov(gc, MEM(&blk->grad[1]), R(edx)));
        EMIT(og_fld(gc, MEM(&blk->grad[1])));
        EMIT(og_fmul(gc, MEM(&gc->state.polygon.factor)));
        EMIT(og_fistp(gc, MEM(&blk->gradBias[1])));
        EMIT(og_fstp(gc, R(st0)));
        EMIT(og_mov(gc, R(eax), MEM(&blk->grad[0])));
        EMIT(og_and(gc, &og_eax, IMM(0x7fffffff)));
        EMIT(og_mov(gc, R(edx), MEM(&blk->grad[1])));
        EMIT(og_and(gc, &og_edx, IMM(0x7fffffff)));
        EMIT(og_cmp(gc, &og_edx, R(eax)));
        EMIT(og_jg(gc, l1));
        EMIT(og_mov(gc, R(edx), MEM(&blk->gradBias[0])));
        EMIT(og_jmp(gc, l2));
        LABEL(l1);
        EMIT(og_mov(gc, R(edx), MEM(&blk->gradBias[1])));
        LABEL(l2);
        EMIT(og_mov(gc, MEM(&blk->slopeBias), R(edx)));
        EMIT(og_mov(gc, R(eax), MEM(&blk->saveEax)));
    }
    EMIT(og_mov(gc, R(edx), MEM(&blk->slopeBias)));
    EMIT(og_add(gc, &og_edx, MEM(&blk->unitsBias)));
    EMIT(og_mov(gc, og_mem(gc, IMM(4), &og_edi, 0, 1), R(edx)));
    EMIT(og_add(gc, &og_edi, IMM(8)));
    EMIT(og_mov(gc, MEM(ptr), R(edi)));
    EMIT(og_mov(gc, R(eax), MEM(&blk->saveEax)));
    EMIT(og_ret(gc));
    blk->proc[1] = (GLuint)pc;
    gc->hwcx.genProcs9[1] = (__GLproc)blk->proc[1];

    /* [2]: the same for indexed hardware vertices */
    EMIT(og_mov(gc, MEM(&blk->area), R(edx)));
    pc = og_lostContextCheck(gc, pc, &og_edx, IMM(0));
    EMIT(og_mov(gc, MEM(&blk->saveEax), R(eax)));
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        EMIT(og_movss(gc, &og_mm0, MEM(&gc->state.polygon.units)));
        if (gc->modes.depthBits > 16 && gc->fx.zPrecision32) {
            EMIT(og_movss(gc, &og_mm1, MEM(&blk->zScale24)));
            EMIT(og_mulps(gc, &og_mm0, &og_mm1));
            EMIT(og_cvtss2si(gc, R(eax), &og_mm0));
        } else {
            EMIT(og_cvtss2si(gc, R(eax), &og_mm0));
            if (gc->modes.depthBits > 16)
                EMIT(og_shl(gc, &og_eax, IMM(8)));
        }
        EMIT(og_mov(gc, MEM(&blk->unitsBias), R(eax)));
    } else if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
        EMIT(og_movd(gc, R(mm0), MEM(&gc->state.polygon.units)));
        if (gc->modes.depthBits > 16 && gc->fx.zPrecision32) {
            EMIT(og_pfmul(gc, &og_mm0, MEM(&blk->zScale24)));
            EMIT(og_pf2id(gc, &og_mm0, &og_mm0));
        } else {
            EMIT(og_pf2id(gc, &og_mm0, &og_mm0));
            if (gc->modes.depthBits > 16)
                EMIT(og_pslld(gc, &og_mm0, IMM(8)));
        }
        EMIT(og_movd(gc, MEM(&blk->unitsBias), R(mm0)));
    } else {
        EMIT(og_fld(gc, MEM(&gc->state.polygon.units)));
        if (gc->modes.depthBits > 16 && gc->fx.zPrecision32) {
            EMIT(og_fmul(gc, MEM(&blk->zScale24)));
            EMIT(og_fistp(gc, MEM(&blk->unitsBias)));
            EMIT(og_mov(gc, R(eax), MEM(&blk->unitsBias)));
        } else {
            EMIT(og_fistp(gc, MEM(&blk->unitsBias)));
            EMIT(og_mov(gc, R(eax), MEM(&blk->unitsBias)));
            if (gc->modes.depthBits > 16)
                EMIT(og_shl(gc, &og_eax, IMM(8)));
        }
        EMIT(og_mov(gc, MEM(&blk->unitsBias), R(eax)));
    }
    l1 = og_newLabel(gc);
    EMIT(og_mov(gc, R(eax), MEM(room)));
    EMIT(og_mov(gc, R(edi), IMM(8)));
    EMIT(og_sub(gc, &og_eax, R(edi)));
    EMIT(og_jge(gc, l1));
    EMIT(og_call(gc, makeRoom));
    LABEL(l1);
    EMIT(og_mov(gc, MEM(room), R(eax)));
    EMIT(og_mov(gc, R(eax), MEM(&blk->saveEax)));
    EMIT(og_mov(gc, R(edi), MEM(ptr)));
    EMIT(og_mov(gc, og_mem(gc, 0, &og_edi, 0, 1), IMM(0x18261)));
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        EMIT(og_movss(gc, &og_mm0, MEM(&blk->area)));
        EMIT(og_movss(gc, &og_mm4, og_mem(gc, IMM(HWV(8)), 0, &og_eax, 4)));
        EMIT(og_subss(gc, &og_mm4, og_mem(gc, IMM(HWV(8)), 0, &og_ecx, 4)));
        EMIT(og_rcpss(gc, &og_mm6, &og_mm0));
        EMIT(og_movss(gc, &og_mm5, og_mem(gc, IMM(HWV(8)), 0, &og_ebx, 4)));
        EMIT(og_subss(gc, &og_mm5, og_mem(gc, IMM(HWV(8)), 0, &og_ecx, 4)));
        EMIT(og_movss(gc, &og_mm0, og_mem(gc, IMM(HWV(4)), 0, &og_eax, 4)));
        EMIT(og_subss(gc, &og_mm0, og_mem(gc, IMM(HWV(4)), 0, &og_ecx, 4)));
        EMIT(og_movss(gc, &og_mm1, og_mem(gc, IMM(HWV(4)), 0, &og_ebx, 4)));
        EMIT(og_subss(gc, &og_mm1, og_mem(gc, IMM(HWV(4)), 0, &og_ecx, 4)));
        EMIT(og_movss(gc, &og_mm2, og_mem(gc, IMM(gc->hwcx.vtxBase), 0, &og_eax, 4)));
        EMIT(og_subss(gc, &og_mm2, og_mem(gc, IMM(gc->hwcx.vtxBase), 0, &og_ecx, 4)));
        EMIT(og_movss(gc, &og_mm3, og_mem(gc, IMM(gc->hwcx.vtxBase), 0, &og_ebx, 4)));
        EMIT(og_subss(gc, &og_mm3, og_mem(gc, IMM(gc->hwcx.vtxBase), 0, &og_ecx, 4)));
        EMIT(og_mulss(gc, &og_mm0, R(mm6)));
        EMIT(og_mulss(gc, &og_mm1, R(mm6)));
        EMIT(og_mulss(gc, &og_mm2, R(mm6)));
        EMIT(og_mulss(gc, &og_mm3, R(mm6)));
        EMIT(og_mulss(gc, &og_mm0, R(mm5)));
        EMIT(og_mulss(gc, &og_mm1, R(mm4)));
        EMIT(og_mulss(gc, &og_mm2, R(mm5)));
        EMIT(og_mulss(gc, &og_mm3, R(mm4)));
        EMIT(og_subss(gc, &og_mm1, R(mm0)));
        EMIT(og_subss(gc, &og_mm2, R(mm3)));
        EMIT(og_movss(gc, &og_mm5, MEM(&blk->absMask)));
        EMIT(og_andps(gc, &og_mm1, &og_mm5));
        EMIT(og_andps(gc, &og_mm2, &og_mm5));
        EMIT(og_maxss(gc, &og_mm1, &og_mm2));
        EMIT(og_mulss(gc, &og_mm1, MEM(&gc->state.polygon.factor)));
        EMIT(og_cvtss2si(gc, MEM(&blk->slopeBias), &og_mm1));
    } else if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
        EMIT(og_movd(gc, R(mm0), MEM(&blk->area)));
        EMIT(og_movd(gc, R(mm4), og_mem(gc, IMM(HWV(8)), 0, &og_eax, 4)));
        EMIT(og_movd(gc, R(mm7), og_mem(gc, IMM(HWV(8)), 0, &og_ecx, 4)));
        EMIT(og_pfrcp(gc, &og_mm6, &og_mm0));
        EMIT(og_pfsub(gc, &og_mm4, &og_mm7));
        EMIT(og_movd(gc, R(mm5), og_mem(gc, IMM(HWV(8)), 0, &og_ebx, 4)));
        EMIT(og_pfsub(gc, &og_mm5, &og_mm7));
        EMIT(og_movd(gc, R(mm0), og_mem(gc, IMM(HWV(4)), 0, &og_eax, 4)));
        EMIT(og_movd(gc, R(mm7), og_mem(gc, IMM(HWV(4)), 0, &og_ecx, 4)));
        EMIT(og_pfsub(gc, &og_mm0, &og_mm7));
        EMIT(og_movd(gc, R(mm1), og_mem(gc, IMM(HWV(4)), 0, &og_ebx, 4)));
        EMIT(og_pfsub(gc, &og_mm1, &og_mm7));
        EMIT(og_movd(gc, R(mm2), og_mem(gc, IMM(gc->hwcx.vtxBase), 0, &og_eax, 4)));
        EMIT(og_movd(gc, R(mm7), og_mem(gc, IMM(gc->hwcx.vtxBase), 0, &og_ecx, 4)));
        EMIT(og_pfsub(gc, &og_mm2, &og_mm7));
        EMIT(og_movd(gc, R(mm3), og_mem(gc, IMM(gc->hwcx.vtxBase), 0, &og_ebx, 4)));
        EMIT(og_pfsub(gc, &og_mm3, &og_mm7));
        EMIT(og_pfmul(gc, &og_mm4, R(mm6)));
        EMIT(og_pfmul(gc, &og_mm5, R(mm6)));
        EMIT(og_pfmul(gc, &og_mm0, R(mm5)));
        EMIT(og_pfmul(gc, &og_mm1, R(mm4)));
        EMIT(og_pfmul(gc, &og_mm2, R(mm5)));
        EMIT(og_pfmul(gc, &og_mm3, R(mm4)));
        EMIT(og_pfsub(gc, &og_mm1, &og_mm0));
        EMIT(og_pfsub(gc, &og_mm2, &og_mm3));
        EMIT(og_pand(gc, &og_mm1, MEM(&blk->absMask)));
        EMIT(og_pand(gc, &og_mm2, MEM(&blk->absMask)));
        EMIT(og_pfmax(gc, &og_mm1, &og_mm2));
        EMIT(og_pfmul(gc, &og_mm1, MEM(&gc->state.polygon.factor)));
        EMIT(og_pf2id(gc, &og_mm1, &og_mm1));
        EMIT(og_movd(gc, MEM(&blk->slopeBias), R(mm1)));
    } else {
        l1 = og_newLabel(gc);
        l2 = og_newLabel(gc);
        EMIT(og_fld(gc, MEM(&blk->area)));
        EMIT(og_fdivr(gc, W_FCONST(gc, 1.0)));
        EMIT(og_fld(gc, og_mem(gc, IMM(HWV(8)), 0, &og_eax, 4)));
        EMIT(og_fsub(gc, og_mem(gc, IMM(HWV(8)), 0, &og_ecx, 4)));
        EMIT(og_fld(gc, og_mem(gc, IMM(HWV(8)), 0, &og_ebx, 4)));
        EMIT(og_fsub(gc, og_mem(gc, IMM(HWV(8)), 0, &og_ecx, 4)));
        EMIT(og_fld(gc, og_mem(gc, IMM(HWV(4)), 0, &og_eax, 4)));
        EMIT(og_fsub(gc, og_mem(gc, IMM(HWV(4)), 0, &og_ecx, 4)));
        EMIT(og_fld(gc, og_mem(gc, IMM(HWV(4)), 0, &og_ebx, 4)));
        EMIT(og_fsub(gc, og_mem(gc, IMM(HWV(4)), 0, &og_ecx, 4)));
        EMIT(og_fld(gc, og_mem(gc, IMM(gc->hwcx.vtxBase), 0, &og_eax, 4)));
        EMIT(og_fsub(gc, og_mem(gc, IMM(gc->hwcx.vtxBase), 0, &og_ecx, 4)));
        EMIT(og_fld(gc, og_mem(gc, IMM(gc->hwcx.vtxBase), 0, &og_ebx, 4)));
        EMIT(og_fsub(gc, og_mem(gc, IMM(gc->hwcx.vtxBase), 0, &og_ecx, 4)));
        EMIT(og_fxch(gc, &og_st4));
        EMIT(og_fmul(gc, R(st6)));
        EMIT(og_fstp(gc, MEM(&blk->scaled[0])));
        EMIT(og_fxch(gc, &og_st4));
        EMIT(og_fmul(gc, R(st5)));
        EMIT(og_fstp(gc, MEM(&blk->scaled[1])));
        EMIT(og_fmul(gc, MEM(&blk->scaled[1])));
        EMIT(og_fxch(gc, &og_st1));
        EMIT(og_fmul(gc, MEM(&blk->scaled[0])));
        EMIT(og_fsubp(gc, &og_st1, &og_st0));
        EMIT(og_fstp(gc, MEM(&blk->grad[0])));
        EMIT(og_mov(gc, R(edx), MEM(&blk->grad[0])));
        EMIT(og_and(gc, &og_edx, IMM(0x7fffffff)));
        EMIT(og_mov(gc, MEM(&blk->grad[0]), R(edx)));
        EMIT(og_fld(gc, MEM(&blk->grad[0])));
        EMIT(og_fmul(gc, MEM(&gc->state.polygon.factor)));
        EMIT(og_fistp(gc, MEM(&blk->gradBias[0])));
        EMIT(og_fmul(gc, MEM(&blk->scaled[1])));
        EMIT(og_fxch(gc, &og_st1));
        EMIT(og_fmul(gc, MEM(&blk->scaled[0])));
        EMIT(og_fxch(gc, &og_st1));
        EMIT(og_fsubp(gc, &og_st1, &og_st0));
        EMIT(og_fstp(gc, MEM(&blk->grad[1])));
        EMIT(og_mov(gc, R(edx), MEM(&blk->grad[1])));
        EMIT(og_and(gc, &og_edx, IMM(0x7fffffff)));
        EMIT(og_mov(gc, MEM(&blk->grad[1]), R(edx)));
        EMIT(og_fld(gc, MEM(&blk->grad[1])));
        EMIT(og_fmul(gc, MEM(&gc->state.polygon.factor)));
        EMIT(og_fistp(gc, MEM(&blk->gradBias[1])));
        EMIT(og_fstp(gc, R(st0)));
        EMIT(og_mov(gc, R(eax), MEM(&blk->grad[0])));
        EMIT(og_and(gc, &og_eax, IMM(0x7fffffff)));
        EMIT(og_mov(gc, R(edx), MEM(&blk->grad[1])));
        EMIT(og_and(gc, &og_edx, IMM(0x7fffffff)));
        EMIT(og_cmp(gc, &og_edx, R(eax)));
        EMIT(og_jg(gc, l1));
        EMIT(og_mov(gc, R(edx), MEM(&blk->gradBias[0])));
        EMIT(og_jmp(gc, l2));
        LABEL(l1);
        EMIT(og_mov(gc, R(edx), MEM(&blk->gradBias[1])));
        LABEL(l2);
        EMIT(og_mov(gc, MEM(&blk->slopeBias), R(edx)));
        EMIT(og_mov(gc, R(eax), MEM(&blk->saveEax)));
    }
    EMIT(og_mov(gc, R(edx), MEM(&blk->slopeBias)));
    EMIT(og_add(gc, &og_edx, MEM(&blk->unitsBias)));
    EMIT(og_mov(gc, og_mem(gc, IMM(4), &og_edi, 0, 1), R(edx)));
    EMIT(og_add(gc, &og_edi, IMM(8)));
    EMIT(og_mov(gc, MEM(ptr), R(edi)));
    EMIT(og_mov(gc, R(eax), MEM(&blk->saveEax)));
    EMIT(og_ret(gc));
    blk->proc[2] = (GLuint)pc;
    gc->hwcx.genProcs9[2] = (__GLproc)blk->proc[2];

    /* the tail: no offset, just the depth bias register */
    pc = og_lostContextCheck(gc, pc, &og_edx, IMM(0));
    EMIT(og_mov(gc, MEM(&blk->saveEax), R(eax)));
    l1 = og_newLabel(gc);
    EMIT(og_mov(gc, R(eax), MEM(room)));
    EMIT(og_mov(gc, R(edi), IMM(8)));
    EMIT(og_sub(gc, &og_eax, R(edi)));
    EMIT(og_jge(gc, l1));
    EMIT(og_call(gc, makeRoom));
    LABEL(l1);
    EMIT(og_mov(gc, MEM(room), R(eax)));
    EMIT(og_mov(gc, R(edi), MEM(ptr)));
    EMIT(og_mov(gc, og_mem(gc, 0, &og_edi, 0, 1), IMM(0x18261)));
    EMIT(og_mov(gc, R(edx), MEM(gc->hwcx.grState[__GL_GR_DEPTH_BIAS])));
    EMIT(og_mov(gc, og_mem(gc, IMM(4), &og_edi, 0, 1), R(edx)));
    EMIT(og_add(gc, &og_edi, IMM(8)));
    EMIT(og_mov(gc, MEM(ptr), R(edi)));
    EMIT(og_mov(gc, R(eax), MEM(&blk->saveEax)));
    og_encode(gc, pc, og_ret(gc));
}

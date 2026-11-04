/*
** od_clipinterp.c -- og_buildClipInterp: generates the clip interpolation routine (hwcx.ogLazyProcs[0]),
** compiled without optimisation (/Od) in the original.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "og.h"
#include "icdprotos.h"

/* 6905b650 -- og_buildClipInterp: build (or fetch from the bufRec[4] code cache) the clip interpolation routine
** that the triangle and line clippers call through hwcx.ogLazyProcs[0]: __fastcall (gc, a, b, out, plane), `ret 0xc`.
** It runs the clip-edge routine genCode[0] (t, the new clip coordinates and window coordinates; a's and b's clip.w
** are left at [ebp + 0] / [ebp + 4] of the data page) and then fills in what the state needs: the colours by
** genCode[1] (GL_SMOOTH) or a's packed colour, the fog value, and each enabled unit's s, t (and q when some q != 1),
** un-projected by w before they are interpolated.  SSE (cpuCaps & 4), 3DNow (& 2) or x87; registers in the routine:
** eax = a, ebx = b, edi = out. */
#pragma optimize("", off)
void __fastcall og_buildClipInterp(__GLcontext *gc)
{
    struct {                    /* the /Od frame [ebp-0x20, ebp): each slot at the original's ebp offset */
        __GLviewport *vp;       /* ebp-0x20  unused */
        GLubyte *entry;         /* ebp-0x1c  the routine's address in the cache */
        GLint unit;             /* ebp-0x18 */
        GLuint modeFlags;       /* ebp-0x14  unused */
        __GLcontext *hwgc;      /* ebp-0x10  gc again, for the hwcx fields */
        GLuint key;             /* ebp-0x0c  the cache key (og_clipInterpKey) */
        GLubyte *pc;            /* ebp-0x08 */
        GLuint *blk;            /* ebp-0x04  the entry's data page: [0] / [1] = a's / b's clip.w */
    } od_f;
#define vp od_f.vp
#define entry od_f.entry
#define unit od_f.unit
#define modeFlags od_f.modeFlags
#define hwgc od_f.hwgc
#define key od_f.key
#define pc od_f.pc
#define blk od_f.blk

    hwgc = gc;
    modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    vp = &gc->state.viewport;
    og_clipInterpKey(gc, &key);
    entry = ((GLubyte *(__fastcall *)(__GLcontext *, __GLhwBufRecICD *, GLuint *, GLuint))hwgc->hwcx.bufRec[4].alloc)
            (gc, &hwgc->hwcx.bufRec[4], &key, 0);
    if (entry != 0) {
        hwgc->hwcx.ogLazyProcs[0] = (__GLproc)entry;
    } else {
        pc = hwPageCacheInsert(gc, &hwgc->hwcx.bufRec[4], &key, &blk);
        entry = pc;
        hwgc->hwcx.ogLazyProcs[0] = (__GLproc)entry;
        og_reset(gc);
        EMIT(og_push(gc, R(ebp)));
        EMIT(og_push(gc, R(edi)));
        EMIT(og_push(gc, R(esi)));
        EMIT(og_push(gc, R(ebx)));
        if ((gc->constants.cpuCaps & __GL_CPU_3DNOW) != 0) {
            EMIT(og_femms(gc));
        }
        EMIT(og_mov(gc, R(ebp), IMM(blk)));
        pc = og_callRel32Gen(gc, pc, hwgc->hwcx.genCode[__GL_GEN_CLIP_EDGE]);
        if (gc->state.light.shadingModel == GL_SMOOTH) {
            /* colors[0] by the 4-float interpolator */
            EMIT(og_mov(gc, R(ecx), IMM(VOFS(colors[0]))));
            pc = og_callRel32Gen(gc, pc, hwgc->hwcx.genCode[__GL_GEN_INTERP]);
        } else {
            EMIT(og_mov(gc, R(ecx), MEMR(VOFS(packedColor), eax)));
            EMIT(og_mov(gc, MEMR(VOFS(packedColor), edi), R(ecx)));
        }
        if ((gc->state.enables.general & __GL_FOG_ENABLE) != 0) {
            pc = og_lerpFloat(gc, pc, IMM(VOFS(fog)));
        }
        if ((gc->constants.cpuCaps & __GL_CPU_SSE) != 0) {
            /* SSE: the weights (xmm0 / xmm1) times xmm2 and b's / a's w, then both units' s, t in one register
            ** (unit 1 in the high half) */
            EMIT(og_mulps(gc, R(mm0), R(mm2)));
            EMIT(og_movss(gc, R(mm3), MEM(blk + 1)));
            EMIT(og_mulps(gc, R(mm1), R(mm2)));
            EMIT(og_shufps(gc, R(mm3), R(mm3), IMM(0)));
            EMIT(og_movss(gc, R(mm4), MEM(blk)));
            EMIT(og_shufps(gc, R(mm4), R(mm4), IMM(0)));
            EMIT(og_mulps(gc, R(mm0), R(mm3)));
            EMIT(og_mulps(gc, R(mm1), R(mm4)));
            if (gc->texture.textureEnabled[0] != 0) {
                EMIT(og_movlps(gc, R(mm5), MEMR(VOFS(texture[0]), eax)));
                EMIT(og_movlps(gc, R(mm6), MEMR(VOFS(texture[0]), ebx)));
            }
            if (gc->texture.textureEnabled[1] != 0) {
                EMIT(og_movhps(gc, R(mm5), MEMR(VOFS(texture[1]), eax)));
                EMIT(og_movhps(gc, R(mm6), MEMR(VOFS(texture[1]), ebx)));
            }
            EMIT(og_mulps(gc, R(mm5), R(mm1)));
            EMIT(og_mulps(gc, R(mm6), R(mm0)));
            EMIT(og_addps(gc, R(mm5), R(mm6)));
            if (gc->texture.textureEnabled[0] != 0) {
                EMIT(og_movlps(gc, MEMR(VOFS(texture[0]), edi), R(mm5)));
            }
            if (gc->texture.textureEnabled[1] != 0) {
                EMIT(og_movhps(gc, MEMR(VOFS(texture[1]), edi), R(mm5)));
            }
            if (hwgc->hwcx.nonUnityQ != 0) {
                for (unit = 0; unit < gc->fx.numTMUs; unit++) {
                    if (gc->texture.textureEnabled[unit] != 0) {
                        pc = og_lerpFloat(gc, pc, IMM(VOFS(texture[unit].w)));
                    }
                }
            }
        } else if ((gc->constants.cpuCaps & __GL_CPU_3DNOW) != 0) {
            /* 3DNow: the weights (mm0 / mm1) likewise, then each unit's s, t by the 2-float interpolator genCode[2]
            ** (__GL_GEN_INTERP2) */
            EMIT(og_pfmul(gc, R(mm0), R(mm2)));
            EMIT(og_movd(gc, R(mm3), MEM(blk + 1)));
            EMIT(og_pfmul(gc, R(mm1), R(mm2)));
            EMIT(og_punpckldq(gc, R(mm3), R(mm3)));
            EMIT(og_movd(gc, R(mm4), MEM(blk)));
            EMIT(og_punpckldq(gc, R(mm4), R(mm4)));
            EMIT(og_pfmul(gc, R(mm0), R(mm3)));
            EMIT(og_pfmul(gc, R(mm1), R(mm4)));
            for (unit = 0; unit < gc->fx.numTMUs; unit++) {
                if (gc->texture.textureEnabled[unit] != 0) {
                    EMIT(og_mov(gc, R(ecx), IMM(VOFS(texture[unit]))));
                    pc = og_callRel32Gen(gc, pc, hwgc->hwcx.genCode[__GL_GEN_INTERP2]);
                    if (hwgc->hwcx.nonUnityQ != 0) {
                        pc = og_lerpFloat(gc, pc, IMM(VOFS(texture[unit].w)));
                    }
                }
            }
        } else {
            /* x87: the same with the weights on the FPU stack, popped at the end */
            EMIT(og_fld(gc, MEM(blk + 1)));
            EMIT(og_fmul(gc, R(st3)));
            EMIT(og_fxch(gc, R(st3)));
            EMIT(og_fld(gc, MEM(blk)));
            EMIT(og_fmulp(gc, R(st1), R(st0)));
            EMIT(og_fxch(gc, R(st2)));
            EMIT(og_fmulp(gc, R(st3), R(st0)));
            EMIT(og_fmulp(gc, R(st1), R(st0)));
            for (unit = 0; unit < gc->fx.numTMUs; unit++) {
                if (gc->texture.textureEnabled[unit] != 0) {
                    EMIT(og_mov(gc, R(ecx), IMM(VOFS(texture[unit]))));
                    pc = og_callRel32Gen(gc, pc, hwgc->hwcx.genCode[__GL_GEN_INTERP2]);
                    if (hwgc->hwcx.nonUnityQ != 0) {
                        pc = og_lerpFloat(gc, pc, IMM(VOFS(texture[unit].w)));
                    }
                }
            }
            EMIT(og_fstp(gc, R(st0)));
            EMIT(og_fstp(gc, R(st0)));
        }
        if ((gc->constants.cpuCaps & __GL_CPU_3DNOW) != 0) {
            EMIT(og_femms(gc));
        }
        EMIT(og_pop(gc, R(ebx)));
        EMIT(og_pop(gc, R(esi)));
        EMIT(og_pop(gc, R(edi)));
        EMIT(og_pop(gc, R(ebp)));
        EMIT(og_ret_imm(gc, IMM(0xc)));
    }
}
#undef vp
#undef entry
#undef unit
#undef modeFlags
#undef hwgc
#undef key
#undef pc
#undef blk
#pragma optimize("", on)

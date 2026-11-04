/*
** sst_tmumem.c -- 3dfx back end, original VA range 6903c000..69044000: the TMU memory blocks (split, merge),
** the sst texture object's unmakeResident / releaseLevel / free hooks, and the og_* generators of the
** clip-edge (x87 / SSE / 3DNow), clipped-quad and immediate-vertex transform routines.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "icdprotos.h"
#include "og.h"
#include <glide.h>
#include "icdglob.h"

/* ---- og_* code generator: og.h ---- */

/* tables the generated clip code indexes (their addresses go into the code; the shared ones are in src/data/gdata.c) */
/* 69003b20 -- the sign of the clip plane, by plane code & 1 */
static const GLfloat g_clipSign[2] = { 1.0f, -1.0f };
/* 69003b28 -- the same as a sign-bit mask (SSE / 3DNow pxor) */
static const GLuint g_clipSignMask[2] = { 0, 0x80000000 };
extern GLuint g_clipKeepBits[5];   /* gdata.c: by plane code - 0x10, the clip bits the new vertex keeps */

#define IDX(d, b, i)    og_mem(gc, IMM(d), R(b), R(i), 4)           /* [b + i * 4 + d] */

/* 6903c330 -- texMgrDebugPrintf: the compiled-out debug printf (a bare ret; sst_texmgr.c calls it as __cdecl) */
void __fastcall texMgrDebugPrintf(void)
{
}

/* 6903c5a0 -- texMgrSplit: split a TMU memory block after `size` bytes: the rest becomes a new free
** block behind it */
__GLtexMemBlockICD *__fastcall texMgrSplit(__GLcontext *gc, __GLtexMemBlockICD *blk, GLint size)
{
    __GLtexMemBlockICD *rest = (__GLtexMemBlockICD *)__GL_MALLOC(gc, sizeof(__GLtexMemBlockICD));

    rest->next = blk->next;
    rest->size = blk->size - size;
    rest->start = blk->start + size;
    rest->tex = NULL;
    blk->next = rest;
    blk->size = size;
    return rest;
}

/* 6903c5e0 -- texMgrCoalesce: merge a TMU memory block with the block behind it (the rover moves back to it) */
void __fastcall texMgrCoalesce(__GLcontext *gc, __GLtexMemBlockICD *blk, GLuint tmu)
{
    __GLtexMemBlockICD *next = blk->next;
    __GLtexMgrFxICD *m = (__GLtexMgrFxICD *)gc->texMgr;
    __GLtexMemBlockICD **rover;

    blk->size += next->size;
    blk->next = next->next;
    rover = &m->rover[__GL_TMU_SLOT(tmu)];
    if (*rover == next)
        *rover = blk;
    __GL_FREE(gc, next);
}

/* 6903c630 -- __glSSTFreeTextureMemory: take a texture off a TMU: free its memory block, clear its per-TMU state
** and the unit's current-texture cache */
void __fastcall __glSSTFreeTextureMemory(__GLcontext *gc, __GLtextureFxICD *tex, GLuint tmu)
{
    __GLtexMemBlockICD *blk = tex->mem[__GL_TMU_SLOT(tmu)];

    if (blk != NULL) {
        texMgrFreeNode(gc, blk, tmu);
        tex->mem[__GL_TMU_SLOT(tmu)] = NULL;
    }
    tex->onTmu[__GL_TMU_SLOT(tmu)] = 0;
    tex->t.hwHandle[__GL_TMU_SLOT(tmu)] = 0;
    if (gc->hwcx.unitTexture[tmu] == tex)
        gc->hwcx.unitTexture[tmu] = (void *)-1;
}

/* 6903c760 -- UnmakeResident: tex->unmakeResident (+0x44) of the sst texture object */
GLboolean __fastcall UnmakeResident(__GLcontext *gc, __GLtextureFxICD *tex, GLuint tmu)
{
    __glSSTFreeTextureMemory(gc, tex, tmu);
    return GL_TRUE;
}

/* 6903c980 -- __glSSTTexReleaseLevel: tex->releaseLevel (+0x98): free the level's compressed image (the pointer is
** not cleared), then the generic __glTextureDeleteLevel */
void __fastcall __glSSTTexReleaseLevel(__GLcontext *gc, __GLtextureFxICD *tex, GLint level)
{
    void *image = tex->t.level[level]->compressedImage;

    if (image != NULL)
        __GL_FREE(gc, image);
    __glTextureDeleteLevel(gc, &tex->t, level);
}

/* 6903c9b0 -- __glSSTFreeTexObj: tex->free (+0x3c): off every TMU, drop it as the palette texture, release every
** level, then free the level records, the level array, its own palette and the object */
void __fastcall __glSSTFreeTexObj(__GLcontext *gc, __GLtextureFxICD *tex)
{
    GLint i, n;

    for (i = 0; i < gc->fx.numTMUs; i++)
        UnmakeResident(gc, tex, i);
    if (gc->hwcx.paletteTexture == tex)
        gc->hwcx.paletteTexture = NULL;
    n = gc->constants.maxMipMapLevel;
    for (i = 0; i < n; i++)
        (*tex->t.releaseLevel)(gc, tex, i);
    __GL_FREE(gc, tex->t.level[0]);
    __GL_FREE(gc, tex->t.level);
    if (tex->palette != NULL)
        __GL_FREE(gc, tex->palette);
    __GL_FREE(gc, tex);
}

/* 6903cb50 -- og_genClipEdgeX87: generates the x87 clip-edge routine (genCode[0]): edx = vertex a, the stack holds
** vertex b (+0x18), the new vertex (+0x1c) and the plane code (+0x20: 0x10 + axis * 2 + side).  t = da / (da - db)
** with d = clip.w +- clip[axis]; [ebp + 0] / [ebp + 4] = a's / b's clip.w (1.0 when flagBits & 0x100000); genCode[1]
** interpolates the vertex (ecx = 0x40: from the clip coordinates on); then the new vertex's clip bits (sign /
** magnitude outcode tables) and, unless flagBits & 0x100000, its window coordinates (window.w = 1/clip.w, else 1.0). */
GLubyte *__fastcall og_genClipEdgeX87(__GLcontext *gc, GLubyte *pc)
{
    OgLabel *noWindow, *done;

    noWindow = og_newLabel(gc);
    done = og_newLabel(gc);
    /* eax = a, ebx = b, edi = the new vertex, edx = the plane code; esi = its axis */
    EMIT(og_mov(gc, R(eax), R(edx)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x18, esp)));
    EMIT(og_mov(gc, R(edi), MEMR(0x1c, esp)));
    EMIT(og_mov(gc, R(edx), MEMR(0x20, esp)));
    EMIT(og_mov(gc, R(esi), R(edx)));
    EMIT(og_sub(gc, R(esi), IMM(0x10)));
    EMIT(og_shr(gc, R(esi), IMM(1)));
    EMIT(og_fld(gc, MEMR(0x4c, eax)));
    EMIT(og_fld(gc, MEMR(0x4c, ebx)));
    EMIT(og_fld(gc, IDX(0x40, eax, esi)));
    EMIT(og_fld(gc, IDX(0x40, ebx, esi)));
    /* t = da / (da - db), d = clip.w + sign * clip[axis], sign = g_clipSign[code & 1] */
    EMIT(og_mov(gc, R(esi), R(edx)));
    EMIT(og_and(gc, R(esi), IMM(1)));
    EMIT(og_fld(gc, TAB(g_clipSign, esi)));
    EMIT(og_fxch(gc, R(st2)));
    EMIT(og_fmul(gc, R(st2)));
    EMIT(og_fxch(gc, R(st2)));
    EMIT(og_fmulp(gc, R(st1), R(st0)));
    EMIT(og_faddp(gc, R(st2), R(st0)));
    EMIT(og_faddp(gc, R(st2), R(st0)));
    EMIT(og_fsubr(gc, R(st1)));
    EMIT(og_fdivp(gc, R(st1), R(st0)));
    /* [ebp + 0] / [ebp + 4] = a's / b's clip.w, or 1.0 when its flagBits & 0x100000 */
    EMIT(og_mov(gc, R(esi), MEMR(0x10, eax)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x4c, eax)));
    EMIT(og_and(gc, R(esi), IMM(0x100000)));
    EMIT(og_sub(gc, R(esi), IMM(0x100000)));
    EMIT(og_sbb(gc, R(esi), R(esi)));
    EMIT(og_xor(gc, R(ecx), IMM(0x3f800000)));
    EMIT(og_and(gc, R(ecx), R(esi)));
    EMIT(og_xor(gc, R(ecx), IMM(0x3f800000)));
    EMIT(og_mov(gc, MEMR(0, ebp), R(ecx)));
    EMIT(og_mov(gc, R(esi), MEMR(0x10, ebx)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x4c, ebx)));
    EMIT(og_and(gc, R(esi), IMM(0x100000)));
    EMIT(og_sub(gc, R(esi), IMM(0x100000)));
    EMIT(og_sbb(gc, R(esi), R(esi)));
    EMIT(og_xor(gc, R(ecx), IMM(0x3f800000)));
    EMIT(og_and(gc, R(ecx), R(esi)));
    EMIT(og_xor(gc, R(ecx), IMM(0x3f800000)));
    EMIT(og_mov(gc, MEMR(4, ebp), R(ecx)));
    EMIT(og_fld(gc, MEM(&gc->constants.one)));
    EMIT(og_fsub(gc, R(st1)));
    /* genCode[1]: interpolate the vertex from +0x40 (clip) on */
    EMIT(og_mov(gc, R(ecx), IMM(0x40)));
    pc = og_callAbs(gc, pc, gc->hwcx.genCode[__GL_GEN_INTERP]);
    EMIT(og_fld(gc, MEM(&gc->constants.one)));
    EMIT(og_fdiv(gc, MEMR(0x4c, edi)));
    /* the new clip bits: signs and |x|, |y|, |z| > |w| through the outcode tables, masked by the plane */
    EMIT(og_mov(gc, R(ebp), MEMR(0x4c, edi)));
    EMIT(og_add(gc, R(ebp), R(ebp)));
    EMIT(og_push(gc, R(eax)));
    EMIT(og_sbb(gc, R(esi), R(esi)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x48, edi)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x44, edi)));
    EMIT(og_mov(gc, R(eax), MEMR(0x40, edi)));
    EMIT(og_adc(gc, R(ecx), R(ecx)));
    EMIT(og_adc(gc, R(esi), R(esi)));
    EMIT(og_add(gc, R(ebx), R(ebx)));
    EMIT(og_adc(gc, R(esi), R(esi)));
    EMIT(og_add(gc, R(eax), R(eax)));
    EMIT(og_adc(gc, R(esi), R(esi)));
    EMIT(og_cmp(gc, R(ebp), R(ecx)));
    EMIT(og_adc(gc, R(ecx), R(ecx)));
    EMIT(og_cmp(gc, R(ebp), R(ebx)));
    EMIT(og_adc(gc, R(ecx), R(ecx)));
    EMIT(og_cmp(gc, R(ebp), R(eax)));
    EMIT(og_adc(gc, R(ecx), R(ecx)));
    EMIT(og_mov(gc, R(ebx), R(esi)));
    EMIT(og_and(gc, R(ebx), IMM(0xf)));
    EMIT(og_and(gc, R(ecx), IMM(0xf)));
    EMIT(og_mov(gc, R(ecx), TAB(g_clipAxisBits, ecx)));
    EMIT(og_mov(gc, R(esi), TAB(g_clipSideBits, ebx)));
    EMIT(og_and(gc, R(esi), R(ecx)));
    EMIT(og_mov(gc, R(ecx), TAB(g_clipWNegBits, ebx)));
    EMIT(og_pop(gc, R(eax)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x18, esp)));
    EMIT(og_or(gc, R(esi), R(ecx)));
    EMIT(og_mov(gc, R(edx), MEMR(0x20, esp)));
    EMIT(og_sub(gc, R(edx), IMM(0x10)));
    EMIT(og_and(gc, R(esi), TAB(g_clipKeepBits, edx)));
    EMIT(og_mov(gc, MEMR(0x10, edi), R(esi)));
    /* window = clip * (1 / w) * viewport.scale + viewport.center */
    EMIT(og_test(gc, R(esi), IMM(0x100000)));
    EMIT(og_jne(gc, noWindow));
    EMIT(og_fld(gc, MEMR(0x40, edi)));
    EMIT(og_fmul(gc, R(st1)));
    EMIT(og_fmul(gc, MEM(&gc->state.viewport.scale)));
    EMIT(og_fadd(gc, MEM(&gc->state.viewport.center)));
    EMIT(og_fstp(gc, MEMR(0, edi)));
    EMIT(og_fld(gc, MEMR(0x44, edi)));
    EMIT(og_fmul(gc, R(st1)));
    EMIT(og_fmul(gc, MEM(&gc->state.viewport.scale.y)));
    EMIT(og_fadd(gc, MEM(&gc->state.viewport.center.y)));
    EMIT(og_fstp(gc, MEMR(4, edi)));
    EMIT(og_fld(gc, MEMR(0x48, edi)));
    EMIT(og_fmul(gc, R(st1)));
    EMIT(og_fmul(gc, MEM(&gc->state.viewport.scale.z)));
    EMIT(og_fadd(gc, MEM(&gc->state.viewport.center.z)));
    EMIT(og_fstp(gc, MEMR(8, edi)));
    EMIT(og_jmp(gc, done));
    LABEL(noWindow);
    EMIT(og_fstp(gc, R(st0)));
    EMIT(og_fld(gc, MEM(&gc->constants.one)));
    LABEL(done);
    EMIT(og_fst(gc, MEMR(0xc, edi)));
    EMIT(og_fxch(gc, R(st2)));
    EMIT(og_fxch(gc, R(st1)));
    return og_encode(gc, pc, og_ret(gc));
}

/* 6903dc10 -- og_callAbs: emit `call target` (an absolute address, through a fixup bound to it) */
GLubyte *__fastcall og_callAbs(__GLcontext *gc, GLubyte *pc, GLuint target)
{
    OgLabel *fix = og_newFixup(gc);

    fix->addr = (GLubyte *)target;
    return og_encode(gc, pc, og_call(gc, fix));
}

/* 6903dc40 -- og_genClipEdgeSSE: generates the SSE clip-edge routine (genCode[0]): as the x87 one (6903cb50); t and
** 1 - t in xmm0 / xmm1 for genCode[1], 1/w by rcpss and one Newton step (og_movss = movss) */
GLubyte *__fastcall og_genClipEdgeSSE(__GLcontext *gc, GLubyte *pc)
{
    OgLabel *noWindow, *done;

    noWindow = og_newLabel(gc);
    done = og_newLabel(gc);
    /* eax = a, ebx = b, edi = the new vertex, edx = the plane code; esi = its axis */
    EMIT(og_mov(gc, R(eax), R(edx)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x18, esp)));
    EMIT(og_mov(gc, R(edi), MEMR(0x1c, esp)));
    EMIT(og_mov(gc, R(edx), MEMR(0x20, esp)));
    EMIT(og_mov(gc, R(esi), R(edx)));
    EMIT(og_sub(gc, R(esi), IMM(0x10)));
    EMIT(og_shr(gc, R(esi), IMM(1)));
    EMIT(og_movss(gc, R(mm0), MEMR(0x4c, eax)));
    EMIT(og_movss(gc, R(mm1), MEMR(0x4c, ebx)));
    EMIT(og_movss(gc, R(mm2), IDX(0x40, eax, esi)));
    EMIT(og_movss(gc, R(mm3), IDX(0x40, ebx, esi)));
    /* t = da / (da - db), d = clip.w + sign * clip[axis] (the sign bit from g_clipSignMask[code & 1]) */
    EMIT(og_mov(gc, R(esi), R(edx)));
    EMIT(og_and(gc, R(esi), IMM(1)));
    EMIT(og_movss(gc, R(mm4), TAB(g_clipSignMask, esi)));
    EMIT(og_xorps(gc, R(mm2), R(mm4)));
    EMIT(og_xorps(gc, R(mm3), R(mm4)));
    EMIT(og_addss(gc, R(mm0), R(mm2)));
    EMIT(og_addss(gc, R(mm1), R(mm3)));
    EMIT(og_movss(gc, R(mm2), R(mm0)));
    EMIT(og_subss(gc, R(mm2), R(mm1)));
    EMIT(og_rcpss(gc, R(mm1), R(mm2)));
    EMIT(og_mulss(gc, R(mm2), R(mm1)));
    EMIT(og_mulss(gc, R(mm2), R(mm1)));
    EMIT(og_addss(gc, R(mm1), R(mm1)));
    EMIT(og_subss(gc, R(mm1), R(mm2)));
    EMIT(og_mulss(gc, R(mm0), R(mm1)));
    EMIT(og_movss(gc, R(mm1), MEM(&gc->constants.one)));
    EMIT(og_subss(gc, R(mm1), R(mm0)));
    EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
    EMIT(og_shufps(gc, R(mm1), R(mm1), IMM(0)));
    /* [ebp + 0] / [ebp + 4] = a's / b's clip.w, or 1.0 when its flagBits & 0x100000 */
    EMIT(og_mov(gc, R(esi), MEMR(0x10, eax)));
    EMIT(og_test(gc, R(esi), IMM(0x100000)));
    EMIT(og_mov(gc, R(esi), IMM(0x3f800000)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x4c, eax)));
    EMIT(og_cmovne(gc, R(ecx), R(esi)));
    EMIT(og_mov(gc, MEMR(0, ebp), R(ecx)));
    EMIT(og_mov(gc, R(esi), MEMR(0x10, ebx)));
    EMIT(og_test(gc, R(esi), IMM(0x100000)));
    EMIT(og_mov(gc, R(esi), IMM(0x3f800000)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x4c, ebx)));
    EMIT(og_cmovne(gc, R(ecx), R(esi)));
    EMIT(og_mov(gc, MEMR(4, ebp), R(ecx)));
    /* genCode[1]: interpolate the vertex from +0x40 (clip) on */
    EMIT(og_mov(gc, R(ecx), IMM(0x40)));
    pc = og_callAbs(gc, pc, gc->hwcx.genCode[__GL_GEN_INTERP]);
    EMIT(og_movss(gc, R(mm3), MEMR(0x4c, edi)));
    EMIT(og_rcpss(gc, R(mm2), R(mm3)));
    EMIT(og_mulss(gc, R(mm3), R(mm2)));
    EMIT(og_mulss(gc, R(mm3), R(mm2)));
    EMIT(og_addss(gc, R(mm2), R(mm2)));
    EMIT(og_subss(gc, R(mm2), R(mm3)));
    EMIT(og_shufps(gc, R(mm2), R(mm2), IMM(0)));
    /* the new clip bits: signs and |x|, |y|, |z| > |w| through the outcode tables, masked by the plane */
    EMIT(og_mov(gc, R(ebp), MEMR(0x4c, edi)));
    EMIT(og_add(gc, R(ebp), R(ebp)));
    EMIT(og_push(gc, R(eax)));
    EMIT(og_sbb(gc, R(esi), R(esi)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x48, edi)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x44, edi)));
    EMIT(og_mov(gc, R(eax), MEMR(0x40, edi)));
    EMIT(og_adc(gc, R(ecx), R(ecx)));
    EMIT(og_adc(gc, R(esi), R(esi)));
    EMIT(og_add(gc, R(ebx), R(ebx)));
    EMIT(og_adc(gc, R(esi), R(esi)));
    EMIT(og_add(gc, R(eax), R(eax)));
    EMIT(og_adc(gc, R(esi), R(esi)));
    EMIT(og_cmp(gc, R(ebp), R(ecx)));
    EMIT(og_adc(gc, R(ecx), R(ecx)));
    EMIT(og_cmp(gc, R(ebp), R(ebx)));
    EMIT(og_adc(gc, R(ecx), R(ecx)));
    EMIT(og_cmp(gc, R(ebp), R(eax)));
    EMIT(og_adc(gc, R(ecx), R(ecx)));
    EMIT(og_mov(gc, R(ebx), R(esi)));
    EMIT(og_and(gc, R(ebx), IMM(0xf)));
    EMIT(og_and(gc, R(ecx), IMM(0xf)));
    EMIT(og_mov(gc, R(ecx), TAB(g_clipAxisBits, ecx)));
    EMIT(og_mov(gc, R(esi), TAB(g_clipSideBits, ebx)));
    EMIT(og_and(gc, R(esi), R(ecx)));
    EMIT(og_mov(gc, R(ecx), TAB(g_clipWNegBits, ebx)));
    EMIT(og_pop(gc, R(eax)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x18, esp)));
    EMIT(og_or(gc, R(esi), R(ecx)));
    EMIT(og_mov(gc, R(edx), MEMR(0x20, esp)));
    EMIT(og_sub(gc, R(edx), IMM(0x10)));
    EMIT(og_and(gc, R(esi), TAB(g_clipKeepBits, edx)));
    EMIT(og_mov(gc, MEMR(0x10, edi), R(esi)));
    /* window = clip * (1 / w) * viewport.scale + viewport.center */
    EMIT(og_test(gc, R(esi), IMM(0x100000)));
    EMIT(og_jne(gc, noWindow));
    EMIT(og_movups(gc, R(mm3), MEM(&gc->state.viewport.scale)));
    EMIT(og_movups(gc, R(mm4), MEM(&gc->state.viewport.center)));
    EMIT(og_movaps(gc, R(mm5), MEMR(0x40, edi)));
    EMIT(og_mulps(gc, R(mm5), R(mm2)));
    EMIT(og_mulps(gc, R(mm5), R(mm3)));
    EMIT(og_addps(gc, R(mm5), R(mm4)));
    EMIT(og_movaps(gc, MEMR(0, edi), R(mm5)));
    EMIT(og_jmp(gc, done));
    LABEL(noWindow);
    EMIT(og_movss(gc, R(mm2), MEM(&gc->constants.one)));
    EMIT(og_shufps(gc, R(mm2), R(mm2), IMM(0)));
    LABEL(done);
    EMIT(og_movss(gc, MEMR(0xc, edi), R(mm2)));
    return og_encode(gc, pc, og_ret(gc));
}

/* 6903eb40 -- og_genClipEdge3DNow: generates the 3DNow clip-edge routine (genCode[0]): as the x87 one (6903cb50); t
** and 1 - t in mm0 / mm1 for genCode[1], the divisions by pfrcp + pfrcpit1 / pfrcpit2 */
GLubyte *__fastcall og_genClipEdge3DNow(__GLcontext *gc, GLubyte *pc)
{
    OgLabel *noWindow, *done;

    noWindow = og_newLabel(gc);
    done = og_newLabel(gc);
    /* eax = a, ebx = b, edi = the new vertex, edx = the plane code; esi = its axis */
    EMIT(og_mov(gc, R(eax), R(edx)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x18, esp)));
    EMIT(og_mov(gc, R(edi), MEMR(0x1c, esp)));
    EMIT(og_mov(gc, R(edx), MEMR(0x20, esp)));
    EMIT(og_mov(gc, R(esi), R(edx)));
    EMIT(og_sub(gc, R(esi), IMM(0x10)));
    EMIT(og_shr(gc, R(esi), IMM(1)));
    EMIT(og_movd(gc, R(mm0), MEMR(0x4c, eax)));
    EMIT(og_movd(gc, R(mm1), MEMR(0x4c, ebx)));
    EMIT(og_movd(gc, R(mm2), IDX(0x40, eax, esi)));
    EMIT(og_movd(gc, R(mm3), IDX(0x40, ebx, esi)));
    /* t = da / (da - db), d = clip.w + sign * clip[axis] (the sign bit from g_clipSignMask[code & 1]) */
    EMIT(og_mov(gc, R(esi), R(edx)));
    EMIT(og_and(gc, R(esi), IMM(1)));
    EMIT(og_movd(gc, R(mm4), TAB(g_clipSignMask, esi)));
    EMIT(og_pxor(gc, R(mm2), R(mm4)));
    EMIT(og_pxor(gc, R(mm3), R(mm4)));
    EMIT(og_pfadd(gc, R(mm0), R(mm2)));
    EMIT(og_pfadd(gc, R(mm1), R(mm3)));
    EMIT(og_movq(gc, R(mm2), R(mm0)));
    EMIT(og_pfsub(gc, R(mm2), R(mm1)));
    EMIT(og_pfrcp(gc, R(mm7), R(mm2)));
    EMIT(og_pfrcpit1(gc, R(mm2), R(mm7)));
    EMIT(og_pfrcpit2(gc, R(mm2), R(mm7)));
    EMIT(og_pfmul(gc, R(mm0), R(mm2)));
    EMIT(og_movd(gc, R(mm1), MEM(&gc->constants.one)));
    EMIT(og_pfsub(gc, R(mm1), R(mm0)));
    EMIT(og_punpckldq(gc, R(mm0), R(mm0)));
    EMIT(og_punpckldq(gc, R(mm1), R(mm1)));
    /* [ebp + 0] / [ebp + 4] = a's / b's clip.w, or 1.0 when its flagBits & 0x100000 */
    EMIT(og_mov(gc, R(esi), MEMR(0x10, eax)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x4c, eax)));
    EMIT(og_and(gc, R(esi), IMM(0x100000)));
    EMIT(og_sub(gc, R(esi), IMM(0x100000)));
    EMIT(og_sbb(gc, R(esi), R(esi)));
    EMIT(og_xor(gc, R(ecx), IMM(0x3f800000)));
    EMIT(og_and(gc, R(ecx), R(esi)));
    EMIT(og_xor(gc, R(ecx), IMM(0x3f800000)));
    EMIT(og_mov(gc, MEMR(0, ebp), R(ecx)));
    EMIT(og_mov(gc, R(esi), MEMR(0x10, ebx)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x4c, ebx)));
    EMIT(og_and(gc, R(esi), IMM(0x100000)));
    EMIT(og_sub(gc, R(esi), IMM(0x100000)));
    EMIT(og_sbb(gc, R(esi), R(esi)));
    EMIT(og_xor(gc, R(ecx), IMM(0x3f800000)));
    EMIT(og_and(gc, R(ecx), R(esi)));
    EMIT(og_xor(gc, R(ecx), IMM(0x3f800000)));
    EMIT(og_mov(gc, MEMR(4, ebp), R(ecx)));
    /* genCode[1]: interpolate the vertex from +0x40 (clip) on */
    EMIT(og_mov(gc, R(ecx), IMM(0x40)));
    pc = og_callAbs(gc, pc, gc->hwcx.genCode[__GL_GEN_INTERP]);
    EMIT(og_movd(gc, R(mm2), MEMR(0x4c, edi)));
    EMIT(og_pfrcp(gc, R(mm6), R(mm2)));
    EMIT(og_pfrcpit1(gc, R(mm2), R(mm6)));
    EMIT(og_pfrcpit2(gc, R(mm2), R(mm6)));
    EMIT(og_punpckldq(gc, R(mm2), R(mm2)));
    /* the new clip bits: signs and |x|, |y|, |z| > |w| through the outcode tables, masked by the plane */
    EMIT(og_mov(gc, R(ebp), MEMR(0x4c, edi)));
    EMIT(og_add(gc, R(ebp), R(ebp)));
    EMIT(og_push(gc, R(eax)));
    EMIT(og_sbb(gc, R(esi), R(esi)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x48, edi)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x44, edi)));
    EMIT(og_mov(gc, R(eax), MEMR(0x40, edi)));
    EMIT(og_adc(gc, R(ecx), R(ecx)));
    EMIT(og_adc(gc, R(esi), R(esi)));
    EMIT(og_add(gc, R(ebx), R(ebx)));
    EMIT(og_adc(gc, R(esi), R(esi)));
    EMIT(og_add(gc, R(eax), R(eax)));
    EMIT(og_adc(gc, R(esi), R(esi)));
    EMIT(og_cmp(gc, R(ebp), R(ecx)));
    EMIT(og_adc(gc, R(ecx), R(ecx)));
    EMIT(og_cmp(gc, R(ebp), R(ebx)));
    EMIT(og_adc(gc, R(ecx), R(ecx)));
    EMIT(og_cmp(gc, R(ebp), R(eax)));
    EMIT(og_adc(gc, R(ecx), R(ecx)));
    EMIT(og_mov(gc, R(ebx), R(esi)));
    EMIT(og_and(gc, R(ebx), IMM(0xf)));
    EMIT(og_and(gc, R(ecx), IMM(0xf)));
    EMIT(og_mov(gc, R(ecx), TAB(g_clipAxisBits, ecx)));
    EMIT(og_mov(gc, R(esi), TAB(g_clipSideBits, ebx)));
    EMIT(og_and(gc, R(esi), R(ecx)));
    EMIT(og_mov(gc, R(ecx), TAB(g_clipWNegBits, ebx)));
    EMIT(og_pop(gc, R(eax)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x18, esp)));
    EMIT(og_or(gc, R(esi), R(ecx)));
    EMIT(og_mov(gc, R(edx), MEMR(0x20, esp)));
    EMIT(og_sub(gc, R(edx), IMM(0x10)));
    EMIT(og_and(gc, R(esi), TAB(g_clipKeepBits, edx)));
    EMIT(og_mov(gc, MEMR(0x10, edi), R(esi)));
    /* window = clip * (1 / w) * viewport.scale + viewport.center */
    EMIT(og_test(gc, R(esi), IMM(0x100000)));
    EMIT(og_jne(gc, noWindow));
    EMIT(og_movq(gc, R(mm3), MEM(&gc->state.viewport.scale)));
    EMIT(og_movq(gc, R(mm4), MEM(&gc->state.viewport.center)));
    EMIT(og_movq(gc, R(mm5), MEMR(0x40, edi)));
    EMIT(og_pfmul(gc, R(mm5), R(mm2)));
    EMIT(og_pfmul(gc, R(mm5), R(mm3)));
    EMIT(og_pfadd(gc, R(mm5), R(mm4)));
    EMIT(og_movq(gc, MEMR(0, edi), R(mm5)));
    EMIT(og_movd(gc, R(mm3), MEM(&gc->state.viewport.scale.z)));
    EMIT(og_movd(gc, R(mm4), MEM(&gc->state.viewport.center.z)));
    EMIT(og_movd(gc, R(mm5), MEMR(0x48, edi)));
    EMIT(og_pfmul(gc, R(mm5), R(mm2)));
    EMIT(og_pfmul(gc, R(mm5), R(mm3)));
    EMIT(og_pfadd(gc, R(mm5), R(mm4)));
    EMIT(og_movd(gc, MEMR(8, edi), R(mm5)));
    EMIT(og_jmp(gc, done));
    LABEL(noWindow);
    EMIT(og_movd(gc, R(mm2), MEM(&gc->constants.one)));
    EMIT(og_punpckldq(gc, R(mm2), R(mm2)));
    LABEL(done);
    EMIT(og_movd(gc, MEMR(0xc, edi), R(mm2)));
    return og_encode(gc, pc, og_ret(gc));
}

/* 69042180 -- og_genClipAndDrawQuad: generates the clipped-quad routine (genCode[4]): the quad v0..v3 (stack) as
** the two triangles (v0, v1, v2) and (v0, v2, v3) through the clipped-triangle routine genCode[3] (ecx / edx passed
** to both) */
GLubyte *__fastcall og_genClipAndDrawQuad(__GLcontext *gc, GLubyte *pc)
{
    EMIT(og_push(gc, R(ebp)));
    EMIT(og_lea(gc, R(ebp), MEMR(8, esp)));
    EMIT(og_push(gc, R(ecx)));
    EMIT(og_push(gc, R(edx)));
    EMIT(og_push(gc, MEMR(8, ebp)));
    EMIT(og_push(gc, MEMR(4, ebp)));
    EMIT(og_push(gc, MEMR(0, ebp)));
    pc = og_callAbs(gc, pc, gc->hwcx.genCode[3]);
    EMIT(og_pop(gc, R(edx)));
    EMIT(og_pop(gc, R(ecx)));
    EMIT(og_push(gc, MEMR(0xc, ebp)));
    EMIT(og_push(gc, MEMR(8, ebp)));
    EMIT(og_push(gc, MEMR(0, ebp)));
    pc = og_callAbs(gc, pc, gc->hwcx.genCode[3]);
    EMIT(og_pop(gc, R(ebp)));
    return og_encode(gc, pc, og_ret_imm(gc, IMM(0x10)));
}

/* 690439e0 -- og_genVertexTransform: generates the immediate-vertex transform routine (genCode[5]): eax = the
** object x, y, z (w = 1); ebp = hwcx.immVertex (the vertex being built) gets both units' current texture coordinates
** scaled by the TMU exponents (+0x20, +0x30) and clip = the MVP matrix (modelView + 0xc0) * obj (+0x40). 3DNow
** (cpuCaps & 2), SSE (& 4) or x87 */
GLubyte *__fastcall og_genVertexTransform(__GLcontext *gc, GLubyte *pc)
{
    if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
        /* 3DNow: the scale is a paddd of the exponents; clip = x * row0 + y * row1 + z * row2 + row3 */
        EMIT(og_femms(gc));
        EMIT(og_mov(gc, R(ebp), MEM(&gc->hwcx.immVertex)));
        EMIT(og_movq(gc, R(mm0), MEM0(eax)));
        EMIT(og_mov(gc, R(ecx), MEM(&gc->transform.modelView)));
        EMIT(og_movq(gc, R(mm6), MEM(&gc->state.current.texture[0])));
        EMIT(og_add(gc, R(ecx), IMM(0xc0)));
        EMIT(og_movq(gc, R(mm7), MEM(&gc->hwcx.texScaleExp[0][0])));
        EMIT(og_movq(gc, R(mm1), R(mm0)));
        EMIT(og_punpckldq(gc, R(mm0), R(mm0)));
        EMIT(og_movq(gc, R(mm2), BASE(0, ecx)));
        EMIT(og_movq(gc, R(mm3), BASE(8, ecx)));
        EMIT(og_punpckhdq(gc, R(mm1), R(mm1)));
        EMIT(og_pfmul(gc, R(mm2), R(mm0)));
        EMIT(og_paddd(gc, R(mm6), R(mm7)));
        EMIT(og_pfmul(gc, R(mm3), R(mm0)));
        EMIT(og_movq(gc, R(mm4), BASE(0x10, ecx)));
        EMIT(og_movq(gc, R(mm5), BASE(0x18, ecx)));
        EMIT(og_movq(gc, MEMR(0x20, ebp), R(mm6)));
        EMIT(og_movq(gc, R(mm6), MEM(&gc->state.current.texture[1])));
        EMIT(og_pfmul(gc, R(mm4), R(mm1)));
        EMIT(og_movd(gc, R(mm0), MEMR(8, eax)));
        EMIT(og_pfmul(gc, R(mm5), R(mm1)));
        EMIT(og_movq(gc, R(mm7), MEM(&gc->hwcx.texScaleExp[1][0])));
        EMIT(og_pfadd(gc, R(mm2), R(mm4)));
        EMIT(og_pfadd(gc, R(mm3), R(mm5)));
        EMIT(og_movq(gc, R(mm1), BASE(0x38, ecx)));
        EMIT(og_punpckldq(gc, R(mm0), R(mm0)));
        EMIT(og_movq(gc, R(mm4), BASE(0x20, ecx)));
        EMIT(og_paddd(gc, R(mm6), R(mm7)));
        EMIT(og_movq(gc, R(mm5), BASE(0x28, ecx)));
        EMIT(og_pfmul(gc, R(mm4), R(mm0)));
        EMIT(og_pfadd(gc, R(mm3), R(mm1)));
        EMIT(og_movq(gc, MEMR(0x30, ebp), R(mm6)));
        EMIT(og_pfmul(gc, R(mm5), R(mm0)));
        EMIT(og_movq(gc, R(mm0), BASE(0x30, ecx)));
        EMIT(og_pfadd(gc, R(mm2), R(mm4)));
        EMIT(og_pfadd(gc, R(mm3), R(mm5)));
        EMIT(og_pfadd(gc, R(mm2), R(mm0)));
        EMIT(og_movq(gc, MEMR(0x40, ebp), R(mm2)));
        EMIT(og_movq(gc, MEMR(0x48, ebp), R(mm3)));
        return og_encode(gc, pc, og_femms(gc));
    }
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        /* SSE: the texture coordinates through integer adds, the matrix rows with mulps / addps */
        EMIT(og_mov(gc, R(ebp), MEM(&gc->hwcx.immVertex)));
        EMIT(og_mov(gc, R(ecx), MEM(&gc->transform.modelView)));
        EMIT(og_add(gc, R(ecx), IMM(0xc0)));
        EMIT(og_movups(gc, R(mm0), BASE(0, ecx)));
        EMIT(og_mov(gc, R(ebx), MEM(&gc->state.current.texture[0])));
        EMIT(og_mov(gc, R(edx), MEM(&gc->state.current.texture[0].y)));
        EMIT(og_movss(gc, R(mm1), MEM0(eax)));
        EMIT(og_mov(gc, R(esi), MEM(&gc->hwcx.texScaleExp[0][0])));
        EMIT(og_mov(gc, R(edi), MEM(&gc->hwcx.texScaleExp[0][1])));
        EMIT(og_movups(gc, R(mm2), BASE(0x10, ecx)));
        EMIT(og_add(gc, R(ebx), R(esi)));
        EMIT(og_add(gc, R(edx), R(edi)));
        EMIT(og_movss(gc, R(mm3), MEMR(4, eax)));
        EMIT(og_mov(gc, MEMR(0x20, ebp), R(ebx)));
        EMIT(og_mov(gc, MEMR(0x24, ebp), R(edx)));
        EMIT(og_shufps(gc, R(mm1), R(mm1), IMM(0)));
        EMIT(og_mov(gc, R(ebx), MEM(&gc->state.current.texture[1])));
        EMIT(og_mov(gc, R(edx), MEM(&gc->state.current.texture[1].y)));
        EMIT(og_movups(gc, R(mm4), BASE(0x20, ecx)));
        EMIT(og_mov(gc, R(esi), MEM(&gc->hwcx.texScaleExp[1][0])));
        EMIT(og_mov(gc, R(edi), MEM(&gc->hwcx.texScaleExp[1][1])));
        EMIT(og_shufps(gc, R(mm3), R(mm3), IMM(0)));
        EMIT(og_add(gc, R(ebx), R(esi)));
        EMIT(og_add(gc, R(edx), R(edi)));
        EMIT(og_movss(gc, R(mm5), MEMR(8, eax)));
        EMIT(og_mov(gc, MEMR(0x30, ebp), R(ebx)));
        EMIT(og_mov(gc, MEMR(0x34, ebp), R(edx)));
        EMIT(og_mulps(gc, R(mm1), R(mm0)));
        EMIT(og_mulps(gc, R(mm3), R(mm2)));
        EMIT(og_shufps(gc, R(mm5), R(mm5), IMM(0)));
        EMIT(og_addps(gc, R(mm1), R(mm3)));
        EMIT(og_movups(gc, R(mm6), BASE(0x30, ecx)));
        EMIT(og_mulps(gc, R(mm5), R(mm4)));
        EMIT(og_addps(gc, R(mm5), R(mm6)));
        EMIT(og_addps(gc, R(mm1), R(mm5)));
        return og_encode(gc, pc, og_movaps(gc, MEMR(0x40, ebp), R(mm1)));
    }
    EMIT(og_mov(gc, R(ebp), MEM(&gc->hwcx.immVertex)));
    EMIT(og_mov(gc, R(ecx), MEM(&gc->transform.modelView)));
    EMIT(og_add(gc, R(ecx), IMM(0xc0)));
    pc = hwCopyTexCoords(gc, pc);             /* x87: the texture coordinates through the shared emitter */
    EMIT(og_fld(gc, BASE(0x3c, ecx)));
    EMIT(og_fld(gc, BASE(0x38, ecx)));
    EMIT(og_fld(gc, BASE(0x34, ecx)));
    EMIT(og_fld(gc, BASE(0x30, ecx)));
    EMIT(og_fld(gc, MEMR(8, eax)));
    EMIT(og_fmul(gc, BASE(0x2c, ecx)));
    EMIT(og_fld(gc, MEMR(8, eax)));
    EMIT(og_fmul(gc, BASE(0x28, ecx)));
    EMIT(og_fld(gc, MEMR(8, eax)));
    EMIT(og_fmul(gc, BASE(0x24, ecx)));
    EMIT(og_fld(gc, MEMR(8, eax)));
    EMIT(og_fmul(gc, BASE(0x20, ecx)));
    EMIT(og_fxch(gc, R(st3)));
    EMIT(og_faddp(gc, R(st7), R(st0)));
    EMIT(og_faddp(gc, R(st4), R(st0)));
    EMIT(og_faddp(gc, R(st4), R(st0)));
    EMIT(og_faddp(gc, R(st1), R(st0)));
    EMIT(og_fld(gc, MEMR(4, eax)));
    EMIT(og_fmul(gc, BASE(0x1c, ecx)));
    EMIT(og_fld(gc, MEMR(4, eax)));
    EMIT(og_fmul(gc, BASE(0x18, ecx)));
    EMIT(og_fld(gc, MEMR(4, eax)));
    EMIT(og_fmul(gc, BASE(0x14, ecx)));
    EMIT(og_fld(gc, MEMR(4, eax)));
    EMIT(og_fmul(gc, BASE(0x10, ecx)));
    EMIT(og_fxch(gc, R(st3)));
    EMIT(og_faddp(gc, R(st7), R(st0)));
    EMIT(og_faddp(gc, R(st4), R(st0)));
    EMIT(og_faddp(gc, R(st4), R(st0)));
    EMIT(og_faddp(gc, R(st1), R(st0)));
    EMIT(og_fld(gc, MEM0(eax)));
    EMIT(og_fmul(gc, BASE(0xc, ecx)));
    EMIT(og_fld(gc, MEM0(eax)));
    EMIT(og_fmul(gc, BASE(8, ecx)));
    EMIT(og_fld(gc, MEM0(eax)));
    EMIT(og_fmul(gc, BASE(4, ecx)));
    EMIT(og_fld(gc, MEM0(eax)));
    EMIT(og_fmul(gc, BASE(0, ecx)));
    EMIT(og_fxch(gc, R(st3)));
    EMIT(og_faddp(gc, R(st7), R(st0)));
    EMIT(og_faddp(gc, R(st4), R(st0)));
    EMIT(og_faddp(gc, R(st4), R(st0)));
    EMIT(og_faddp(gc, R(st1), R(st0)));
    EMIT(og_fxch(gc, R(st3)));
    EMIT(og_fstp(gc, MEMR(0x4c, ebp)));
    EMIT(og_fstp(gc, MEMR(0x44, ebp)));
    EMIT(og_fstp(gc, MEMR(0x48, ebp)));
    return og_encode(gc, pc, og_fstp(gc, MEMR(0x40, ebp)));
}


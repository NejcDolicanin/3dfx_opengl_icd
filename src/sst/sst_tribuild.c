/*
** sst_tribuild.c -- og code generators of the hardware triangle path: the clipped / subdivided triangle
** (genCode[3]), the bufRec[1] triangle, the bufRec[3] indexed triangle loop, the bufRec[0] immediate-mode
** vertex proc and the vertex-array FIFO vertex copy.  Every branch's emits
** are written out in full (VC6 tail-merges and hoists them like the original).
*/
#include "wglpriv.h"
#include "og.h"
#include <glide.h>
#include "glsi.h"
#include "glshade.h"
#include "icdprotos.h"

/* 69003b70 -- the clip subdivision table (gdata.c: 10-byte rows by the 3-bit clip code - 1), which the original
** declared as a function: `(GLuint)tbl - 10` is then `mov edx,tbl; sub edx,10` instead of one folded constant */
void __cdecl g_clipSubdivTable(void);   /* __cdecl: the plain _g_clipSubdivTable symbol of the gdata.c table */

/* og.h emitters (EMIT / R / IMM / MEM / MEMR / LABEL) and: */
#define IDX(b, i)       og_mem(gc, NULL, R(b), R(i), 4)                 /* [b + i * 4] */

/* offset of a __GLvertexICD field (the generated code addresses vertices by register) */
#define VX_GBCODES      VOFS(fog)       /* vertex-array cache vertices keep their guard-band codes here (fillRange) */

/* 690402a0's clip frame at ebp (32-byte aligned): [0..4] the vertex pointers the subdivision table indexes
   (a, b, c, then the two new vertices), the new vertices themselves at +0x20 and +0xe0 */
#define CLIPF_VTX(i)    ((i) * 4)
#define CLIPF_NEWA      0x20
#define CLIPF_NEWB      (CLIPF_NEWA + sizeof(__GLvertexICD))

/* the bufRec cache: insert a key, returns the new entry's code page (blk: its data page) */
typedef GLubyte *(__fastcall *BufRecLookup)(__GLcontext *gc, __GLhwBufRecICD *rec, void *key, void *blk);

/* 690402a0 -- hwGenClipTriangle: generate genCode[3] (__GL_GEN_CLIP_TRIANGLE), the clipped / subdivided
** triangle routine (edx = the highest clip-code bit to test, stack: the 3 vertices; ret 0xc): all three outside one plane -> done; none outside ->
** the back-face test (SSE / 3DNow / x87 signed area against the cull face), then drawTriPackColor with the
** polygon-offset hooks genProcs9[0] / [2] when enabled.  Partly outside: the highest clip bit picks a row of
** the subdivision table (clipSubdivTable), the interpolator ogLazyProcs[0] makes the two new vertices in a
** 32-byte aligned frame, and the routine calls itself for the (up to) two sub-triangles.  Returns pc.
** The original passes IMM(&gc), the address of its own parameter slot, as the callees' ecx (kept; harmless:
** the interpolator uses ecx as scratch and the routine itself saves it): gc is therefore address-taken
** (reloaded from its slot for every call); hw / g2 are the two register / memory copies the original keeps.
*/
GLubyte *__fastcall hwGenClipTriangle(__GLcontext *gc, GLubyte *pc)
{
    __GLcontext *hw, *g2;
    OgLabel *self, *clip, *done, *noOffset, *noOffset2, *l, *l2;
    GLuint lazy;

    hw = gc;
    g2 = gc;
    self = og_newFixup(gc);
    clip = og_newFixup(gc);
    done = og_newFixup(gc);
    noOffset = og_newFixup(gc);
    noOffset2 = og_newFixup(gc);
    LABEL(self);
    EMIT(og_push(gc, R(esi)));
    EMIT(og_push(gc, R(edi)));
    EMIT(og_push(gc, R(ebx)));
    EMIT(og_push(gc, R(ecx)));
    EMIT(og_push(gc, R(ebp)));
    EMIT(og_mov(gc, R(eax), MEMR(0x18, esp)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x1c, esp)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x20, esp)));
    EMIT(og_mov(gc, R(esi), MEMR(VOFS(flagBits), eax)));
    EMIT(og_mov(gc, R(ebp), MEMR(VOFS(flagBits), ebx)));
    EMIT(og_and(gc, R(esi), R(ebp)));
    EMIT(og_mov(gc, R(edi), MEMR(VOFS(flagBits), ecx)));
    EMIT(og_and(gc, R(esi), R(edi)));
    EMIT(og_and(gc, R(esi), IMM(0xffff0000)));
    EMIT(og_jne(gc, done));
    EMIT(og_mov(gc, R(esi), MEMR(VOFS(flagBits), eax)));
    EMIT(og_or(gc, R(esi), R(ebp)));
    EMIT(og_or(gc, R(esi), R(edi)));
    EMIT(og_and(gc, R(esi), IMM(0xffff0000)));
    EMIT(og_jne(gc, clip));

    /* the signed area into [esp-4] */
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        EMIT(og_movss(gc, R(mm0), MEMR(VOFS(window.x), eax)));
        EMIT(og_movss(gc, R(mm1), MEMR(VOFS(window.x), ebx)));
        EMIT(og_movss(gc, R(mm2), MEMR(VOFS(window.x), ecx)));
        EMIT(og_movss(gc, R(mm3), MEMR(VOFS(window.y), eax)));
        EMIT(og_movss(gc, R(mm4), MEMR(VOFS(window.y), ebx)));
        EMIT(og_movss(gc, R(mm5), MEMR(VOFS(window.y), ecx)));
        EMIT(og_subss(gc, R(mm0), R(mm2)));
        EMIT(og_subss(gc, R(mm1), R(mm2)));
        EMIT(og_subss(gc, R(mm3), R(mm5)));
        EMIT(og_subss(gc, R(mm4), R(mm5)));
        EMIT(og_mulss(gc, R(mm0), R(mm4)));
        EMIT(og_mulss(gc, R(mm1), R(mm3)));
        EMIT(og_subss(gc, R(mm0), R(mm1)));
        EMIT(og_movss(gc, MEMR(-4, esp), R(mm0)));
    } else if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
        EMIT(og_movd(gc, R(mm0), MEMR(VOFS(window.x), eax)));
        EMIT(og_movd(gc, R(mm1), MEMR(VOFS(window.x), ebx)));
        EMIT(og_movd(gc, R(mm2), MEMR(VOFS(window.x), ecx)));
        EMIT(og_movd(gc, R(mm3), MEMR(VOFS(window.y), eax)));
        EMIT(og_movd(gc, R(mm4), MEMR(VOFS(window.y), ebx)));
        EMIT(og_movd(gc, R(mm5), MEMR(VOFS(window.y), ecx)));
        EMIT(og_pfsub(gc, R(mm0), R(mm2)));
        EMIT(og_pfsub(gc, R(mm1), R(mm2)));
        EMIT(og_pfsub(gc, R(mm3), R(mm5)));
        EMIT(og_pfsub(gc, R(mm4), R(mm5)));
        EMIT(og_pfmul(gc, R(mm0), R(mm4)));
        EMIT(og_pfmul(gc, R(mm1), R(mm3)));
        EMIT(og_pfsub(gc, R(mm0), R(mm1)));
        EMIT(og_movd(gc, MEMR(-4, esp), R(mm0)));
    } else {
        EMIT(og_fld(gc, MEMR(VOFS(window.x), eax)));
        EMIT(og_fsub(gc, MEMR(VOFS(window.x), ecx)));
        EMIT(og_fld(gc, MEMR(VOFS(window.x), ebx)));
        EMIT(og_fsub(gc, MEMR(VOFS(window.x), ecx)));
        EMIT(og_fld(gc, MEMR(VOFS(window.y), eax)));
        EMIT(og_fsub(gc, MEMR(VOFS(window.y), ecx)));
        EMIT(og_fld(gc, MEMR(VOFS(window.y), ebx)));
        EMIT(og_fsub(gc, MEMR(VOFS(window.y), ecx)));
        EMIT(og_fmulp(gc, R(st3), R(st0)));
        EMIT(og_fmulp(gc, R(st1), R(st0)));
        EMIT(og_fsubp(gc, R(st1), R(st0)));
        EMIT(og_fstp(gc, MEMR(-4, esp)));
    }
    /* culled when the face of the sign is the cull face */
    EMIT(og_mov(gc, R(eax), MEMR(-4, esp)));
    EMIT(og_shr(gc, R(eax), IMM(0x1f)));
    EMIT(og_xor(gc, R(eax), IMM(1)));
    EMIT(og_movzx8(gc, R(al), MEMR(&gc->polygon.face[0], eax)));
    EMIT(og_movzx8(gc, R(bl), MEM(&gc->polygon.cullFace)));
    EMIT(og_cmp(gc, R(al), R(bl)));
    EMIT(og_je(gc, done));
    EMIT(og_mov(gc, R(eax), MEMR(0x18, esp)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x1c, esp)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x20, esp)));
    EMIT(og_mov(gc, R(edx), MEM(&gc->state.enables.general)));
    EMIT(og_test(gc, R(edx), IMM(__GL_POLYGON_OFFSET_FILL_ENABLE)));
    EMIT(og_je(gc, noOffset));
    EMIT(og_mov(gc, R(edx), MEMR(-4, esp)));
    EMIT(og_call(gc, MEM(&hw->hwcx.genProcs9[0])));
    EMIT(og_mov(gc, R(eax), MEMR(0x18, esp)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x1c, esp)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x20, esp)));
    LABEL(noOffset);
    EMIT(og_push(gc, R(ecx)));
    EMIT(og_mov(gc, R(edx), R(ebx)));
    EMIT(og_mov(gc, R(ecx), R(eax)));
    EMIT(og_call(gc, MEM(&hw->hwcx.drawTriPackColor)));
    EMIT(og_mov(gc, R(edx), MEM(&gc->state.enables.general)));
    EMIT(og_test(gc, R(edx), IMM(__GL_POLYGON_OFFSET_FILL_ENABLE)));
    EMIT(og_je(gc, noOffset2));
    EMIT(og_call(gc, MEM(&hw->hwcx.genProcs9[2])));
    EMIT(og_mov(gc, R(eax), MEMR(0x18, esp)));
    LABEL(noOffset2);
    EMIT(og_jmp(gc, done));

    /* partly outside: the highest set clip bit (edx = its mask, ecx = its number) */
    LABEL(clip);
    EMIT(og_mov(gc, R(ecx), R(edx)));
    EMIT(og_mov(gc, R(edx), IMM(1)));
    EMIT(og_shl(gc, R(edx), R(cl)));
    l = og_newLabel(gc);
    l2 = og_newLabel(gc);
    LABEL(l);
    EMIT(og_test(gc, R(esi), R(edx)));
    EMIT(og_jne(gc, l2));
    EMIT(og_dec(gc, R(ecx)));
    EMIT(og_shr(gc, R(edx), IMM(1)));
    EMIT(og_jmp(gc, l));
    LABEL(l2);
    EMIT(og_mov(gc, R(esi), MEMR(VOFS(flagBits), eax)));
    EMIT(og_and(gc, R(ebp), R(edx)));
    EMIT(og_and(gc, R(esi), R(edx)));
    EMIT(og_shl(gc, R(esi), IMM(2)));
    EMIT(og_shl(gc, R(ebp), IMM(1)));
    EMIT(og_and(gc, R(edi), R(edx)));
    EMIT(og_or(gc, R(esi), R(ebp)));
    EMIT(og_or(gc, R(esi), R(edi)));
    EMIT(og_shr(gc, R(esi), R(cl)));
    EMIT(og_shl(gc, R(esi), IMM(1)));
    EMIT(og_lea(gc, R(esi), og_mem(gc, IMM((GLuint)g_clipSubdivTable - 10), R(esi), R(esi), 4)));
    EMIT(og_mov(gc, R(edi), R(ecx)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x20, esp)));
    /* a 32-byte aligned frame for the clipped vertices */
    EMIT(og_mov(gc, R(edx), R(esp)));
    EMIT(og_sub(gc, R(esp), IMM(0x1a0)));
    EMIT(og_and(gc, R(esp), IMM(0xffffffe0)));
    EMIT(og_mov(gc, R(ebp), R(esp)));
    EMIT(og_push(gc, R(edx)));
    EMIT(og_mov(gc, MEMR(CLIPF_VTX(0), ebp), R(eax)));
    EMIT(og_mov(gc, MEMR(CLIPF_VTX(1), ebp), R(ebx)));
    EMIT(og_mov(gc, MEMR(CLIPF_VTX(2), ebp), R(ecx)));
    EMIT(og_lea(gc, R(eax), MEMR(CLIPF_NEWA, ebp)));
    EMIT(og_mov(gc, MEMR(CLIPF_VTX(3), ebp), R(eax)));
    EMIT(og_lea(gc, R(eax), MEMR(CLIPF_NEWB, ebp)));
    EMIT(og_mov(gc, MEMR(CLIPF_VTX(4), ebp), R(eax)));
    /* the two new vertices on the plane */
    EMIT(og_push(gc, R(edi)));
    EMIT(og_lea(gc, R(eax), MEMR(CLIPF_NEWA, ebp)));
    EMIT(og_push(gc, R(eax)));
    EMIT(og_movzx8(gc, R(eax), MEMR(1, esi)));
    EMIT(og_push(gc, IDX(ebp, eax)));
    EMIT(og_movzx8(gc, R(eax), MEMR(0, esi)));
    EMIT(og_mov(gc, R(edx), IDX(ebp, eax)));
    EMIT(og_mov(gc, R(ecx), IMM(&gc)));
    lazy = (GLuint)&g2->hwcx.ogLazyProcs[0];
    EMIT(og_call(gc, MEM(lazy)));
    EMIT(og_push(gc, R(edi)));
    EMIT(og_lea(gc, R(eax), MEMR(CLIPF_NEWB, ebp)));
    EMIT(og_push(gc, R(eax)));
    EMIT(og_movzx8(gc, R(eax), MEMR(3, esi)));
    EMIT(og_push(gc, IDX(ebp, eax)));
    EMIT(og_movzx8(gc, R(eax), MEMR(2, esi)));
    EMIT(og_mov(gc, R(edx), IDX(ebp, eax)));
    EMIT(og_mov(gc, R(ecx), IMM(&gc)));
    EMIT(og_call(gc, MEM(lazy)));
    /* the (up to) two sub-triangles, recursively */
    EMIT(og_dec(gc, R(edi)));
    l2 = og_newLabel(gc);
    EMIT(og_movzx8(gc, R(eax), MEMR(6, esi)));
    EMIT(og_test(gc, R(eax), R(eax)));
    EMIT(og_je(gc, l2));
    EMIT(og_push(gc, IDX(ebp, eax)));
    EMIT(og_movzx8(gc, R(eax), MEMR(5, esi)));
    EMIT(og_push(gc, IDX(ebp, eax)));
    EMIT(og_movzx8(gc, R(eax), MEMR(4, esi)));
    EMIT(og_push(gc, IDX(ebp, eax)));
    EMIT(og_mov(gc, R(edx), R(edi)));
    EMIT(og_mov(gc, R(ecx), IMM(&gc)));
    EMIT(og_call(gc, self));
    LABEL(l2);
    EMIT(og_movzx8(gc, R(eax), MEMR(9, esi)));
    EMIT(og_push(gc, IDX(ebp, eax)));
    EMIT(og_movzx8(gc, R(eax), MEMR(8, esi)));
    EMIT(og_push(gc, IDX(ebp, eax)));
    EMIT(og_movzx8(gc, R(eax), MEMR(7, esi)));
    EMIT(og_push(gc, IDX(ebp, eax)));
    EMIT(og_mov(gc, R(edx), R(edi)));
    EMIT(og_mov(gc, R(ecx), IMM(&gc)));
    EMIT(og_call(gc, self));
    EMIT(og_pop(gc, R(esp)));
    LABEL(done);
    EMIT(og_pop(gc, R(ebp)));
    EMIT(og_pop(gc, R(ecx)));
    EMIT(og_pop(gc, R(ebx)));
    EMIT(og_pop(gc, R(edi)));
    EMIT(og_pop(gc, R(esi)));
    return og_encode(gc, pc, og_ret_imm(gc, IMM(0xc)));
}

/* 69062310 -- hwBuildTriangleProc: the bufRec[1] block, the hardware-path triangle (edx = a, stack b, c;
** ret 8) with culling (signed area against the cull face, eax / ebx = vertices kept in genConst[0] / [1]),
** two-sided lighting (the face's needs and each vertex's colour pointer to colors[face]), validation of
** the vertices that lack needed data (procs.validateVertex[flagBits >> 14 & 3]), then grDrawTriangle.
** Returns the block's start. */
GLubyte *__fastcall hwBuildTriangleProc(__GLcontext *gc)
{
    GLuint flags;
    GLubyte *pc, *start;
    OgLabel *done, *valid, *loop, *skip, *draw;
    GLuint key[3];

    flags = ((GLuint (__fastcall *)(__GLcontext *))gc->polygon.shader.getModeFlags)(gc);
    hwCullTriangleKey(gc, key);
    start = ((BufRecLookup)gc->hwcx.bufRec[1].alloc)(gc, &gc->hwcx.bufRec[1], key, NULL);
    if (start != 0)
        return start;
    start = hwPageCacheInsert(gc, &gc->hwcx.bufRec[1], key, NULL);
    og_reset(gc);
    done = og_newFixup(gc);
    valid = og_newFixup(gc);
    pc = start;
    EMIT(og_push(gc, R(ebp)));
    EMIT(og_push(gc, R(edi)));
    EMIT(og_push(gc, R(esi)));
    EMIT(og_push(gc, R(ebx)));
    EMIT(og_mov(gc, R(eax), R(edx)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x14, esp)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x18, esp)));
    if ((gc->state.enables.general & __GL_CULL_FACE_ENABLE) || (flags & __GL_SHADE_TWOSIDED)) {
        /* the signed area -> the face */
        EMIT(og_fld(gc, MEMR(VOFS(window.x), eax)));
        EMIT(og_fsub(gc, MEMR(VOFS(window.x), ecx)));
        EMIT(og_fld(gc, MEMR(VOFS(window.x), ebx)));
        EMIT(og_fsub(gc, MEMR(VOFS(window.x), ecx)));
        EMIT(og_fld(gc, MEMR(VOFS(window.y), eax)));
        EMIT(og_fsub(gc, MEMR(VOFS(window.y), ecx)));
        EMIT(og_fld(gc, MEMR(VOFS(window.y), ebx)));
        EMIT(og_fsub(gc, MEMR(VOFS(window.y), ecx)));
        EMIT(og_fmulp(gc, R(st3), R(st0)));
        EMIT(og_fmulp(gc, R(st1), R(st0)));
        EMIT(og_fsubp(gc, R(st1), R(st0)));
        EMIT(og_fstp(gc, MEMR(-4, esp)));
        EMIT(og_mov(gc, MEM(&gc->hwcx.genConst[0]), R(eax)));
        EMIT(og_mov(gc, MEM(&gc->hwcx.genConst[1]), R(ebx)));
        EMIT(og_mov(gc, R(eax), MEMR(-4, esp)));
        EMIT(og_shr(gc, R(eax), IMM(0x1f)));
        EMIT(og_xor(gc, R(eax), IMM(1)));
        EMIT(og_movzx8(gc, R(al), MEMR(&gc->polygon.face[0], eax)));
        EMIT(og_movzx8(gc, R(bl), MEM(&gc->polygon.cullFace)));
        EMIT(og_cmp(gc, R(al), R(bl)));
        EMIT(og_mov(gc, R(eax), MEM(&gc->hwcx.genConst[0])));
        EMIT(og_mov(gc, R(ebx), MEM(&gc->hwcx.genConst[1])));
        EMIT(og_je(gc, done));
    }
    if (flags & __GL_SHADE_TWOSIDED) {
        /* two-sided: each vertex's colour pointer to the face's colour, the face's needs */
        EMIT(og_shr(gc, R(edx), IMM(0x1f)));
        EMIT(og_xor(gc, R(edx), IMM(1)));
        EMIT(og_mov(gc, R(dl), MEMR(&gc->polygon.face[0], edx)));
        EMIT(og_mov(gc, R(esi), R(edx)));
        EMIT(og_shl(gc, R(esi), IMM(4)));
        EMIT(og_lea(gc, R(ebp), og_mem(gc, IMM(VOFS(colors[0])), R(eax), R(esi), 1)));
        EMIT(og_mov(gc, MEMR(VOFS(color), eax), R(ebp)));
        EMIT(og_lea(gc, R(ebp), og_mem(gc, IMM(VOFS(colors[0])), R(ebx), R(esi), 1)));
        EMIT(og_mov(gc, MEMR(VOFS(color), ebx), R(ebp)));
        EMIT(og_lea(gc, R(ebp), og_mem(gc, IMM(VOFS(colors[0])), R(ecx), R(esi), 1)));
        EMIT(og_mov(gc, MEMR(VOFS(color), ecx), R(ebp)));
        EMIT(og_mov(gc, R(ebp), og_mem(gc, IMM(&gc->vertex.faceNeeds[0]), NULL, R(edx), 4)));
        EMIT(og_or(gc, R(ebp), MEM(&gc->vertex.needs)));
    } else {
        EMIT(og_mov(gc, R(ebp), MEM(&gc->vertex.needs)));
        EMIT(og_or(gc, R(ebp), MEM(&gc->vertex.faceNeeds[0])));
    }
    /* any vertex lacking a needed part (the and of the inverted has-flags) -> validate the three */
    EMIT(og_mov(gc, R(edx), MEMR(VOFS(flagBits), eax)));
    EMIT(og_mov(gc, R(esi), MEMR(VOFS(flagBits), ebx)));
    EMIT(og_and(gc, R(edx), R(esi)));
    EMIT(og_mov(gc, R(esi), MEMR(VOFS(flagBits), ecx)));
    EMIT(og_and(gc, R(edx), R(esi)));
    EMIT(og_xor(gc, R(edx), IMM(0xffffffff)));
    EMIT(og_test(gc, R(edx), R(ebp)));
    EMIT(og_je(gc, valid));
    loop = og_newFixup(gc);
    skip = og_newLabel(gc);
    EMIT(og_push(gc, R(eax)));
    EMIT(og_sub(gc, R(esp), IMM(0xc)));
    EMIT(og_mov(gc, MEMR(0, esp), R(eax)));
    EMIT(og_mov(gc, MEMR(4, esp), R(ebx)));
    EMIT(og_mov(gc, MEMR(8, esp), R(ecx)));
    EMIT(og_xor(gc, R(eax), R(eax)));
    LABEL(loop);
    EMIT(og_mov(gc, R(esi), R(esp)));
    EMIT(og_push(gc, R(eax)));
    EMIT(og_mov(gc, R(eax), IDX(esi, eax)));
    EMIT(og_mov(gc, R(esi), MEMR(VOFS(flagBits), eax)));
    EMIT(og_xor(gc, R(esi), IMM(0xffffffff)));
    EMIT(og_test(gc, R(ebp), R(esi)));
    EMIT(og_je(gc, skip));
    EMIT(og_push(gc, R(ebp)));
    EMIT(og_mov(gc, R(edx), R(eax)));
    EMIT(og_mov(gc, R(ecx), IMM(gc)));
    EMIT(og_mov(gc, R(eax), MEMR(VOFS(flagBits), eax)));
    EMIT(og_and(gc, R(eax), IMM(0xc000)));
    EMIT(og_shr(gc, R(eax), IMM(0xe)));
    EMIT(og_call(gc, og_mem(gc, IMM(&gc->procs.validateVertex[0]), NULL, R(eax), 4)));
    LABEL(skip);
    EMIT(og_pop(gc, R(eax)));
    EMIT(og_inc(gc, R(eax)));
    EMIT(og_cmp(gc, R(eax), IMM(3)));
    EMIT(og_jne(gc, loop));
    EMIT(og_add(gc, R(esp), IMM(0xc)));
    EMIT(og_pop(gc, R(eax)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x18, esp)));
    LABEL(valid);
    draw = og_newFixup(gc);
    draw->addr = (GLubyte *)grDrawTriangle;     /* a fixup bound to grDrawTriangle */
    EMIT(og_push(gc, R(eax)));
    EMIT(og_push(gc, R(ecx)));
    EMIT(og_push(gc, R(ebx)));
    EMIT(og_push(gc, R(eax)));
    EMIT(og_call(gc, draw));
    EMIT(og_pop(gc, R(eax)));
    EMIT(og_mov(gc, R(ebx), MEMR(0x14, esp)));
    EMIT(og_mov(gc, R(ecx), MEMR(0x18, esp)));
    LABEL(done);
    EMIT(og_pop(gc, R(ebx)));
    EMIT(og_pop(gc, R(esi)));
    EMIT(og_pop(gc, R(edi)));
    EMIT(og_pop(gc, R(ebp)));
    og_encode(gc, pc, og_ret_imm(gc, IMM(8)));
    return start;
}

/* the bufRec[3] block's data page */
typedef struct IdxTriBlockRec {
    __GLvertexICD *strip[3];            /* +0x000  the last triangle's b, c, a: the strip test compares a, b with
                                                    strip[parity + 1], strip[parity] */
    __GLvertexICD *clipA;               /* +0x00c  the clipped triangle's vertices (0xc0 apart) */
    GLuint      pad010[0x2f];
    __GLvertexICD *clipB;               /* +0x0cc */
    GLuint      pad0d0[0x2f];
    __GLvertexICD *clipC;               /* +0x18c  also c while the loop tests a triangle */
    GLuint      pad190[0x2f];
    GLuint     *indexCursor;            /* +0x24c  the next index */
    void       *vtxBuffer;              /* +0x250  imports.malloc(24000), unused: the routine buffers into
                                                    hwcx.alloc_365f8 (also 24000 bytes) */
    GLuint      pad254[3];
    __GLvertexICD *saveA, *saveB;       /* +0x260  eax / ebx across the area test */
} IdxTriBlock;

/* 69063290 -- hwBuildIndexedTriangles: the bufRec[3] block, procs.drawIndexedTriangles of the vertex-array
** path (fastcall: edx = the index array, stack: vertex cache base, count; ret 8).  Each pass loads three
** indices, scales them by the 0xc0-byte cache vertex, rejects all-out / culled triangles, sends the clipped
** ones through the clip proc (__glDoClipTriangle with user clip planes, else fetchVertex x 3 and genCode[3]),
** and the others to the FIFO (genBlock_10 starts a strip, genBlock_14 continues it) or into the strip
** buffer that the flush subroutine hands to grDrawVertexArray(GR_TRIANGLE_STRIP, n, buf).  esi: bit 1 =
** the next triangle starts a new strip, bit 0 = the strip parity.  Returns the block's start.
*/
GLubyte *__fastcall hwBuildIndexedTriangles(__GLcontext *gc, int unused)
{
    GLuint flags, fifo;
    GLubyte *pc, *start;
    OgLabel *loop, *clipDone, *culled, *second, *flush, *flushSkip, *clip, *next, *l;
    IdxTriBlock *blk;
    GLuint key;                         /* hwPixelFormatCaps writes one dword */

    flags = ((GLuint (__fastcall *)(__GLcontext *))gc->polygon.shader.getModeFlags)(gc);
    fifo = gc->hwcx.cmdTransportInfo != 0;
    hwPixelFormatCaps(gc, &key);
    start = ((BufRecLookup)gc->hwcx.bufRec[3].alloc)(gc, &gc->hwcx.bufRec[3], &key, NULL);
    if (start != 0)
        return start;
    start = hwPageCacheInsert(gc, &gc->hwcx.bufRec[3], &key, &blk);
    blk->vtxBuffer = ((void *(__fastcall *)(__GLcontext *, int))gc->imports[0])(gc, 24000);
    og_reset(gc);
    loop = og_newFixup(gc);
    clipDone = og_newFixup(gc);
    culled = og_newFixup(gc);
    second = og_newFixup(gc);
    flush = og_newFixup(gc);
    flushSkip = og_newFixup(gc);
    og_newFixup(gc);
    clip = og_newFixup(gc);
    next = og_newFixup(gc);
    og_newFixup(gc);
    pc = start;
    if (gc->constants.cpuCaps & __GL_CPU_3DNOW)
        EMIT(og_femms(gc));
    EMIT(og_push(gc, R(ebp)));
    EMIT(og_push(gc, R(edi)));
    EMIT(og_push(gc, R(esi)));
    EMIT(og_push(gc, R(ebx)));
    EMIT(og_mov(gc, MEM(&blk->indexCursor), R(edx)));
    EMIT(og_mov(gc, R(esi), IMM(2)));
    EMIT(og_mov(gc, R(edi), MEM(&gc->hwcx.alloc_365f8)));
    EMIT(og_mov(gc, R(eax), MEMR(0x18, esp)));

    /* the loop: three indices -> cache vertices eax, ebx, ecx */
    LABEL(loop);
    EMIT(og_mov(gc, MEMR(0x18, esp), R(eax)));
    EMIT(og_mov(gc, R(edx), MEM(&blk->indexCursor)));
    EMIT(og_mov(gc, R(eax), MEM0(edx)));
    EMIT(og_mov(gc, R(ebx), MEMR(4, edx)));
    EMIT(og_mov(gc, R(ecx), MEMR(8, edx)));
    EMIT(og_add(gc, R(edx), IMM(0xc)));
    EMIT(og_mov(gc, MEM(&blk->indexCursor), R(edx)));
    EMIT(og_mov(gc, R(edx), MEMR(0x14, esp)));
    EMIT(og_imul(gc, R(eax), IMM(0xc0)));
    EMIT(og_imul(gc, R(ebx), IMM(0xc0)));
    EMIT(og_imul(gc, R(ecx), IMM(0xc0)));
    EMIT(og_add(gc, R(eax), R(edx)));
    EMIT(og_add(gc, R(ebx), R(edx)));
    EMIT(og_add(gc, R(ecx), R(edx)));
    EMIT(og_mov(gc, MEM(&blk->clipC), R(ecx)));
    /* all out -> culled; any out -> clip */
    EMIT(og_mov(gc, R(edx), MEMR(VOFS(flagBits), eax)));
    EMIT(og_and(gc, R(edx), MEMR(VOFS(flagBits), ebx)));
    EMIT(og_mov(gc, R(ecx), MEMR(VOFS(flagBits), ecx)));
    EMIT(og_and(gc, R(edx), R(ecx)));
    EMIT(og_test(gc, R(edx), IMM(0xffff2000)));
    EMIT(og_jne(gc, culled));
    EMIT(og_mov(gc, R(ebp), MEMR(VOFS(flagBits), eax)));
    EMIT(og_or(gc, R(ebp), MEMR(VOFS(flagBits), ebx)));
    EMIT(og_or(gc, R(ebp), R(ecx)));
    EMIT(og_mov(gc, R(ecx), MEM(&blk->clipC)));
    EMIT(og_test(gc, R(ebp), IMM(0xfff00000)));
    EMIT(og_jne(gc, clip));
    if ((gc->state.enables.general & __GL_CULL_FACE_ENABLE) || (flags & __GL_SHADE_TWOSIDED)) {
        /* the signed area -> the face against the cull face */
        if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
            EMIT(og_movq(gc, R(mm0), MEMR(VOFS(window.x), eax)));
            EMIT(og_movq(gc, R(mm2), MEMR(VOFS(window.x), ecx)));
            EMIT(og_pfsub(gc, R(mm0), R(mm2)));
            EMIT(og_movq(gc, R(mm1), MEMR(VOFS(window.x), ebx)));
            EMIT(og_pfsub(gc, R(mm1), R(mm2)));
            EMIT(og_movq(gc, R(mm3), MEMR(VOFS(window.y), eax)));
            EMIT(og_movq(gc, R(mm5), MEMR(VOFS(window.y), ecx)));
            EMIT(og_pfsub(gc, R(mm3), R(mm5)));
            EMIT(og_pfmul(gc, R(mm1), R(mm3)));
            EMIT(og_movq(gc, R(mm4), MEMR(VOFS(window.y), ebx)));
            EMIT(og_pfsub(gc, R(mm4), R(mm5)));
            EMIT(og_pfmul(gc, R(mm0), R(mm4)));
            EMIT(og_pfsub(gc, R(mm0), R(mm1)));
            EMIT(og_movd(gc, MEMR(-4, esp), R(mm0)));
        } else {
            EMIT(og_fld(gc, MEMR(VOFS(window.x), eax)));
            EMIT(og_fsub(gc, MEMR(VOFS(window.x), ecx)));
            EMIT(og_fld(gc, MEMR(VOFS(window.x), ebx)));
            EMIT(og_fsub(gc, MEMR(VOFS(window.x), ecx)));
            EMIT(og_fld(gc, MEMR(VOFS(window.y), eax)));
            EMIT(og_fsub(gc, MEMR(VOFS(window.y), ecx)));
            EMIT(og_fld(gc, MEMR(VOFS(window.y), ebx)));
            EMIT(og_fsub(gc, MEMR(VOFS(window.y), ecx)));
            EMIT(og_fmulp(gc, R(st3), R(st0)));
            EMIT(og_fmulp(gc, R(st1), R(st0)));
            EMIT(og_fsubp(gc, R(st1), R(st0)));
            EMIT(og_fstp(gc, MEMR(-4, esp)));
        }
        EMIT(og_mov(gc, MEM(&blk->saveA), R(eax)));
        EMIT(og_mov(gc, MEM(&blk->saveB), R(ebx)));
        EMIT(og_mov(gc, R(eax), MEMR(-4, esp)));
        EMIT(og_shr(gc, R(eax), IMM(0x1f)));
        EMIT(og_xor(gc, R(eax), IMM(1)));
        EMIT(og_movzx8(gc, R(al), MEMR(&gc->polygon.face[0], eax)));
        EMIT(og_movzx8(gc, R(bl), MEM(&gc->polygon.cullFace)));
        EMIT(og_cmp(gc, R(al), R(bl)));
        EMIT(og_mov(gc, R(eax), MEM(&blk->saveA)));
        EMIT(og_mov(gc, R(ebx), MEM(&blk->saveB)));
        EMIT(og_je(gc, culled));
    }
    if (gc->state.enables.clipPlanes) {
        EMIT(og_test(gc, R(ebp), IMM(0xffc00000)));
        EMIT(og_jne(gc, clip));
    }
    if (!gc->fx.fullscreen || !gc->fx.guardBand || !(gc->constants.cpuCaps & (__GL_CPU_3DNOW | __GL_CPU_SSE))) {
        EMIT(og_test(gc, R(ebp), IMM(0xffff2000)));
        EMIT(og_jne(gc, clip));
    } else {
        /* guard band: clip only what is outside the guard band in both coordinates */
        EMIT(og_mov(gc, MEM(&blk->saveA), R(eax)));
        EMIT(og_mov(gc, R(ebp), MEMR(VOFS(flagBits), eax)));
        EMIT(og_and(gc, R(ebp), MEMR(VX_GBCODES, eax)));
        EMIT(og_mov(gc, R(eax), MEMR(VOFS(flagBits), ebx)));
        EMIT(og_and(gc, R(eax), MEMR(VX_GBCODES, ebx)));
        EMIT(og_or(gc, R(ebp), R(eax)));
        EMIT(og_mov(gc, R(eax), MEMR(VOFS(flagBits), ecx)));
        EMIT(og_and(gc, R(eax), MEMR(VX_GBCODES, ecx)));
        EMIT(og_or(gc, R(ebp), R(eax)));
        EMIT(og_mov(gc, R(eax), MEM(&blk->saveA)));
        EMIT(og_test(gc, R(ebp), IMM(0xf0000)));
        EMIT(og_jne(gc, clip));
    }
    LABEL(clipDone);
    /* strip continuation: does this triangle share the last two vertices (esi bit 1) */
    EMIT(og_mov(gc, R(edx), R(esi)));
    EMIT(og_and(gc, R(edx), IMM(1)));
    EMIT(og_mov(gc, R(ebp), R(eax)));
    EMIT(og_xor(gc, R(ebp), TAB(&blk->strip[1], edx)));
    EMIT(og_mov(gc, R(ecx), R(ebx)));
    EMIT(og_xor(gc, R(ecx), TAB(&blk->strip[0], edx)));
    EMIT(og_or(gc, R(ecx), R(ebp)));
    EMIT(og_add(gc, R(ecx), IMM(0xffffffff)));
    EMIT(og_sbb(gc, R(ecx), R(ecx)));
    EMIT(og_and(gc, R(ecx), IMM(2)));
    EMIT(og_or(gc, R(esi), R(ecx)));
    EMIT(og_mov(gc, R(ecx), MEM(&blk->clipC)));
    EMIT(og_mov(gc, MEM(&blk->strip[2]), R(eax)));
    EMIT(og_mov(gc, MEM(&blk->strip[0]), R(ebx)));
    EMIT(og_mov(gc, MEM(&blk->strip[1]), R(ecx)));
    if (gc->state.enables.general & __GL_POLYGON_OFFSET_ANY_ENABLE) {
        EMIT(og_mov(gc, R(edx), MEMR(-4, esp)));
        EMIT(og_call(gc, MEM(&gc->hwcx.genProcs9[0])));
    }
    EMIT(og_test(gc, R(esi), IMM(2)));
    EMIT(og_je(gc, second));
    if (fifo) {
        EMIT(og_call(gc, MEM(&gc->hwcx.genBlock_10)));
        EMIT(og_mov(gc, R(esi), IMM(2)));
    } else {
        EMIT(og_call(gc, flush));
        EMIT(og_mov(gc, R(eax), MEM(&blk->strip[2])));
        EMIT(og_mov(gc, R(ebx), MEM(&blk->strip[0])));
        EMIT(og_mov(gc, R(ecx), MEM(&blk->strip[1])));
        EMIT(og_mov(gc, MEM0(edi), R(eax)));
        EMIT(og_mov(gc, MEMR(4, edi), R(ebx)));
        EMIT(og_mov(gc, MEMR(8, edi), R(ecx)));
        EMIT(og_add(gc, R(edi), IMM(0xc)));
    }
    EMIT(og_mov(gc, R(esi), IMM(0)));
    EMIT(og_mov(gc, R(eax), MEMR(0x18, esp)));
    EMIT(og_sub(gc, R(eax), IMM(3)));
    EMIT(og_ja(gc, loop));
    EMIT(og_jmp(gc, next));

    /* a strip continuation: only the new vertex */
    LABEL(second);
    EMIT(og_mov(gc, R(ecx), MEM(&blk->strip[1])));
    if (fifo) {
        EMIT(og_call(gc, MEM(&gc->hwcx.genBlock_14)));
    } else {
        EMIT(og_mov(gc, MEM0(edi), R(ecx)));
        EMIT(og_add(gc, R(edi), IMM(4)));
    }
    EMIT(og_xor(gc, R(esi), IMM(1)));
    EMIT(og_mov(gc, R(eax), MEMR(0x18, esp)));
    EMIT(og_sub(gc, R(eax), IMM(3)));
    EMIT(og_ja(gc, loop));
    EMIT(og_jmp(gc, next));

    LABEL(culled);
    EMIT(og_mov(gc, R(esi), IMM(2)));
    EMIT(og_mov(gc, R(eax), MEMR(0x18, esp)));
    EMIT(og_sub(gc, R(eax), IMM(3)));
    EMIT(og_ja(gc, loop));
    EMIT(og_jmp(gc, next));

    /* clipped: flush, then the clip proc (user planes) or fetch + clip the triangle */
    LABEL(clip);
    EMIT(og_mov(gc, MEM(&blk->clipA), R(eax)));
    EMIT(og_mov(gc, MEM(&blk->clipB), R(ebx)));
    if (!fifo)
        EMIT(og_call(gc, flush));
    else
        EMIT(og_mov(gc, R(esi), IMM(2)));
    if (gc->constants.cpuCaps & __GL_CPU_3DNOW)
        EMIT(og_femms(gc));
    if (gc->state.enables.clipPlanes) {
        l = og_newLabel(gc);
        EMIT(og_test(gc, R(ebp), IMM(0xffc00000)));
        EMIT(og_je(gc, l));
        EMIT(og_mov(gc, R(ebx), MEM(&blk->clipB)));
        EMIT(og_mov(gc, R(ecx), MEM(&blk->clipC)));
        EMIT(og_push(gc, R(ecx)));
        EMIT(og_push(gc, R(ebx)));
        EMIT(og_mov(gc, R(edx), MEM(&blk->clipA)));
        EMIT(og_mov(gc, R(ecx), IMM(gc)));
        pc = og_callRel32Tri(gc, pc, (GLuint)__glDoClipTriangle);
        EMIT(og_mov(gc, R(eax), MEMR(0x18, esp)));
        EMIT(og_sub(gc, R(eax), IMM(3)));
        EMIT(og_ja(gc, loop));
        EMIT(og_jmp(gc, next));
        LABEL(l);
    }
    EMIT(og_mov(gc, R(ecx), IMM(gc)));
    EMIT(og_mov(gc, R(edx), MEM(&blk->clipA)));
    EMIT(og_call(gc, MEM(&gc->vertexArray.fetchVertex)));
    EMIT(og_mov(gc, R(ecx), IMM(gc)));
    EMIT(og_mov(gc, R(edx), MEM(&blk->clipB)));
    EMIT(og_call(gc, MEM(&gc->vertexArray.fetchVertex)));
    EMIT(og_mov(gc, R(ecx), IMM(gc)));
    EMIT(og_mov(gc, R(edx), MEM(&blk->clipC)));
    EMIT(og_call(gc, MEM(&gc->vertexArray.fetchVertex)));
    EMIT(og_mov(gc, R(eax), MEM(&blk->clipA)));
    EMIT(og_mov(gc, R(ebx), MEM(&blk->clipB)));
    EMIT(og_mov(gc, R(ecx), MEM(&blk->clipC)));
    EMIT(og_push(gc, R(ecx)));
    EMIT(og_push(gc, R(ebx)));
    EMIT(og_push(gc, R(eax)));
    EMIT(og_mov(gc, R(edx), IMM(0x15)));
    EMIT(og_mov(gc, R(ecx), IMM(gc)));
    pc = og_callRel32Tri(gc, pc, gc->hwcx.genCode[3]);
    EMIT(og_mov(gc, R(eax), MEMR(0x18, esp)));
    EMIT(og_sub(gc, R(eax), IMM(3)));
    EMIT(og_ja(gc, loop));

    /* the end: flush, the polygon-offset end hook, return */
    LABEL(next);
    if (!fifo)
        EMIT(og_call(gc, flush));
    if (gc->state.enables.general & __GL_POLYGON_OFFSET_ANY_ENABLE)
        EMIT(og_call(gc, MEM(&gc->hwcx.genProcs9[2])));
    EMIT(og_pop(gc, R(ebx)));
    EMIT(og_pop(gc, R(esi)));
    EMIT(og_pop(gc, R(edi)));
    EMIT(og_pop(gc, R(ebp)));
    if (gc->constants.cpuCaps & __GL_CPU_3DNOW)
        EMIT(og_femms(gc));
    EMIT(og_ret_imm(gc, IMM(8)));
    if (!fifo) {
        /* the flush subroutine: the buffered strip to grDrawVertexArray(GR_TRIANGLE_STRIP, n, buf) */
        LABEL(flush);
        EMIT(og_mov(gc, R(edx), MEM(&gc->hwcx.alloc_365f8)));
        EMIT(og_sub(gc, R(edi), R(edx)));
        EMIT(og_je(gc, flushSkip));
        EMIT(og_push(gc, R(edx)));
        EMIT(og_shr(gc, R(edi), IMM(2)));
        EMIT(og_push(gc, R(edi)));
        EMIT(og_push(gc, IMM(GR_TRIANGLE_STRIP)));
        pc = og_callRel32Tri(gc, pc, (GLuint)grDrawVertexArray);
        LABEL(flushSkip);
        EMIT(og_mov(gc, R(edi), MEM(&gc->hwcx.alloc_365f8)));
        EMIT(og_mov(gc, R(esi), IMM(2)));
        og_encode(gc, pc, og_ret(gc));
    }
    return start;
}

/* 69020c90 -- hwBuildImmProc: hwcx.buildImmProc(gc, n), the bufRec[0] immediate-mode vertex proc for an
** n-component glVertex (eax = the coordinates; the vertex in ebp): transform (lazy proc n, or genCode[5] for
** a 3-component vertex with a general mvp), copy the object coordinates when lighting / texgen needs them
** (lazy proc 3 + n for the eye coordinates), else the eye z alone (3DNow / x87) for point attenuation, the
** normal (lazy proc 8), texture / window coordinates (hwGenTexCoords / 690225f0), lighting (lazy proc 1),
** then the primitive tail (6902431b).  Returns the block's start.
*/
GLubyte *__fastcall hwBuildImmProc(__GLcontext *gc, GLuint n)
{
    GLuint needs, tex;
    GLubyte *pc, *start;
    OgLabel *done;
    int prim, i;
    char eyeZ;
    GLboolean objDone;
    GLuint blkInfo;
    GLuint key[3];

    needs = ((GLuint (__fastcall *)(__GLcontext *))gc->vertex.getFaceNeeds0)(gc)
          | ((GLuint (__fastcall *)(__GLcontext *))gc->vertex.getNeeds)(gc);
    hwVertexProcKey(gc, key, n);
    start = ((BufRecLookup)gc->hwcx.bufRec[0].alloc)(gc, &gc->hwcx.bufRec[0], key, &blkInfo);
    if (start != 0)
        return start;
    start = hwPageCacheInsert(gc, &gc->hwcx.bufRec[0], key, &blkInfo);
    tex = 0;
    for (i = 0; i < gc->fx.numTMUs; i++)
        tex |= gc->state.enables.texture[i];
    og_reset(gc);
    done = og_newFixup(gc);
    prim = gc->hwcx.primClass;
    /* eyeZ: points (primClass 0) whose size is not 1.0f (compared as bits) need the eye z */
    if (prim != 0 || (eyeZ = 1, *(GLint *)&gc->state.point.requestedSize == 0x3f800000))
        eyeZ = 0;
    objDone = GL_FALSE;
    pc = start;
    EMIT(og_push(gc, R(ebp)));
    EMIT(og_push(gc, R(edi)));
    EMIT(og_push(gc, R(esi)));
    EMIT(og_push(gc, R(ebx)));
    EMIT(og_mov(gc, R(eax), MEMR(0x14, esp)));
    if (n == 3 && gc->transform.modelView->mvp.matrixType == __GL_MT_GENERAL) {
        pc = og_callRel32(gc, pc, gc->hwcx.genCode[5]);
    } else {
        pc = hwCopyTexCoords(gc, og_encode(gc, pc, og_mov(gc, R(ebp), MEM(&gc->hwcx.immVertex))));
        EMIT(og_push(gc, R(eax)));
        EMIT(og_lea(gc, R(edx), MEMR(VOFS(clip), ebp)));
        EMIT(og_mov(gc, R(ecx), IMM(gc)));
        EMIT(og_call(gc, MEM((GLuint)&gc->hwcx.ogLazyProcs[0] + n * 4)));
        pc = og_callRel32(gc, pc, gc->hwcx.genCode[6]);
    }
    if ((gc->state.enables.general & __GL_LIGHTING_ENABLE) || (tex & __GL_TEXTURE_GEN_ANY_ENABLE)) {
        /* the object coordinates (+0x80), w = 1 when absent */
        EMIT(og_mov(gc, R(eax), MEMR(0x14, esp)));
        EMIT(og_mov(gc, R(ebx), MEMR(0, eax)));
        EMIT(og_mov(gc, MEMR(VOFS(obj.x), ebp), R(ebx)));
        EMIT(og_mov(gc, R(ebx), MEMR(4, eax)));
        EMIT(og_mov(gc, MEMR(VOFS(obj.y), ebp), R(ebx)));
        if (n > 2)
            EMIT(og_mov(gc, R(ebx), MEMR(8, eax)));
        else
            EMIT(og_xor(gc, R(ebx), R(ebx)));
        EMIT(og_mov(gc, MEMR(VOFS(obj.z), ebp), R(ebx)));
        EMIT(og_mov(gc, R(ebx), n > 3 ? MEMR(0xc, eax) : IMM(0x3f800000)));
        EMIT(og_mov(gc, MEMR(VOFS(obj.w), ebp), R(ebx)));
        if (needs & __GL_HAS_EYE) {
            /* the eye coordinates (+0x50) */
            objDone = GL_TRUE;
            EMIT(og_push(gc, R(edx)));
            EMIT(og_lea(gc, R(eax), MEMR(VOFS(obj.x), ebp)));
            EMIT(og_push(gc, R(eax)));
            EMIT(og_lea(gc, R(edx), MEMR(VOFS(eye), ebp)));
            EMIT(og_mov(gc, R(ecx), IMM(gc)));
            EMIT(og_call(gc, MEM((GLuint)&gc->hwcx.ogLazyProcs[3] + n * 4)));
            EMIT(og_pop(gc, R(edx)));
        }
    }
    if (eyeZ && !objDone) {
        /* only the eye z (+0x58): row 2 of the modelview matrix */
        EMIT(og_mov(gc, R(ecx), MEM(&gc->transform.modelView)));
        EMIT(og_add(gc, R(ecx), IMM(8)));
        EMIT(og_mov(gc, R(eax), MEMR(0x14, esp)));
        if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
            EMIT(og_femms(gc));
            EMIT(og_movd(gc, R(mm0), MEM0(ecx)));
            EMIT(og_movd(gc, R(mm1), MEMR(0x10, ecx)));
            EMIT(og_movd(gc, R(mm2), MEMR(0x20, ecx)));
            EMIT(og_movd(gc, R(mm3), MEMR(0x30, ecx)));
            if (n == 2) {
                EMIT(og_pfmul(gc, R(mm0), MEM0(eax)));
                EMIT(og_pfmul(gc, R(mm1), MEMR(4, eax)));
                EMIT(og_pfadd(gc, R(mm2), R(mm3)));
                EMIT(og_pfadd(gc, R(mm0), R(mm1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm2)));
            } else if (n == 3) {
                EMIT(og_pfmul(gc, R(mm0), MEM0(eax)));
                EMIT(og_pfmul(gc, R(mm1), MEMR(4, eax)));
                EMIT(og_pfmul(gc, R(mm2), MEMR(8, eax)));
                EMIT(og_pfadd(gc, R(mm0), R(mm1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm2)));
                EMIT(og_pfadd(gc, R(mm0), R(mm3)));
            } else if (n == 4) {
                EMIT(og_pfmul(gc, R(mm0), MEM0(eax)));
                EMIT(og_pfmul(gc, R(mm1), MEMR(4, eax)));
                EMIT(og_pfmul(gc, R(mm2), MEMR(8, eax)));
                EMIT(og_pfmul(gc, R(mm3), MEMR(0xc, eax)));
                EMIT(og_pfadd(gc, R(mm0), R(mm1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm2)));
                EMIT(og_pfadd(gc, R(mm0), R(mm3)));
            }
            EMIT(og_movd(gc, MEMR(VOFS(eye.z), ebp), R(mm0)));
            EMIT(og_femms(gc));
        } else {
            EMIT(og_fld(gc, MEM0(eax)));
            EMIT(og_fmul(gc, MEM0(ecx)));
            EMIT(og_fld(gc, MEMR(4, eax)));
            EMIT(og_fmul(gc, MEMR(0x10, ecx)));
            if (n > 2) {
                EMIT(og_fld(gc, MEMR(8, eax)));
                EMIT(og_fmul(gc, MEMR(0x20, ecx)));
            }
            if (n > 3) {
                EMIT(og_fld(gc, MEMR(0xc, eax)));
                EMIT(og_fmul(gc, MEMR(0x30, ecx)));
            } else {
                EMIT(og_fld(gc, MEMR(0x30, ecx)));
            }
            if (n == 2) {
                EMIT(og_fxch(gc, R(st2)));
            } else {
                EMIT(og_fxch(gc, R(st3)));
                EMIT(og_faddp(gc, R(st1), R(st0)));
            }
            EMIT(og_faddp(gc, R(st1), R(st0)));
            EMIT(og_faddp(gc, R(st1), R(st0)));
            EMIT(og_fstp(gc, MEMR(VOFS(eye.z), ebp)));
        }
    }
    if (needs & __GL_HAS_NORMAL) {
        /* the normal (+0x90) */
        EMIT(og_push(gc, R(edx)));
        EMIT(og_mov(gc, R(eax), IMM(&gc->state.current.normal)));
        EMIT(og_push(gc, R(eax)));
        EMIT(og_lea(gc, R(edx), MEMR(VOFS(normal), ebp)));
        EMIT(og_mov(gc, R(ecx), IMM(gc)));
        EMIT(og_call(gc, MEM(&gc->hwcx.ogLazyProcs[8])));
        EMIT(og_pop(gc, R(edx)));
    }
    if (prim == 0)
        EMIT(og_mov(gc, R(edx), R(ebp)));
    pc = og_windowTransform(gc, hwGenTexCoords(gc, pc));
    if (!(gc->constants.cpuCaps & (__GL_CPU_3DNOW | __GL_CPU_SSE)))
        EMIT(og_fstp(gc, MEMR(VOFS(window.w), ebp)));
    if (gc->state.enables.general & __GL_LIGHTING_ENABLE) {
        /* lighting */
        EMIT(og_push(gc, R(ebp)));
        EMIT(og_push(gc, R(edx)));
        EMIT(og_mov(gc, R(ecx), IMM(gc)));
        EMIT(og_mov(gc, R(edx), R(ebp)));
        EMIT(og_call(gc, MEM(&gc->hwcx.ogLazyProcs[1])));
        EMIT(og_pop(gc, R(edx)));
        EMIT(og_pop(gc, R(ebp)));
    }
    if (prim != 0) {
        EMIT(og_test(gc, R(edx), R(edx)));
        EMIT(og_jne(gc, done));
        EMIT(og_pop(gc, R(ebx)));
        EMIT(og_pop(gc, R(esi)));
        EMIT(og_pop(gc, R(edi)));
        EMIT(og_pop(gc, R(ebp)));
        EMIT(og_ret_imm(gc, IMM(4)));
    }
    LABEL(done);
    og_genRenderPrim(gc, pc, prim, eyeZ);
    return start;
}

/* the hardware vertex field k (field-major: vtxBase + k * vtxCapacity) of the vertex numbered `idx` */
#define HWV(k)          og_mem(gc, IMM((GLuint)gc->hwcx.vtxBase + gc->hwcx.vtxCapacity * (k)), NULL, idx, 4)
/* the packet slot [edi + *off] */
#define PKT             MEMR(*off, edi)
/* the array texcoords of TMU t's unit: edx = the array, [edx + idx * 8] */
#define TEXARRAY(t)     EMIT(og_mov(gc, R(edx), MEM((GLuint)&gc->vertexArray.texture + gc->fx.tmuTexUnit[t] * 4)))
#define TEXSCALE(t, c)  MEM((GLuint)&gc->hwcx.texScale[0][c] + gc->fx.tmuTexUnit[t] * 8)
#define ARR(r, i, sc)   og_mem(gc, NULL, R(r), (i), (sc))                   /* [r + i * sc], i an operand */

/* 6903a620 -- hwCopyArrayVertex: the vertex-array variant of the FIFO vertex copy: the fields of the array
** vertex in the index register `idx` into the packet at [edi + *off] -- x, y, the colour (the colour array
** swapped to ARGB, or the current colour; the glow-worm colour for the marked object), z, w, and the TMU
** texcoords from the arrays scaled by the TMU range and q (SSE / 3DNow / x87).  Returns pc.
*/
GLubyte *__fastcall hwCopyArrayVertex(__GLcontext *gc, GLubyte *pc, OgOperand *idx, GLint *off, GLuint flags)
{
    EMIT(og_mov(gc, R(edx), og_mem(gc, IMM(gc->hwcx.vtxBase), NULL, idx, 4)));
    EMIT(og_mov(gc, PKT, R(edx)));
    *off += 4;
    EMIT(og_mov(gc, R(edx), HWV(4)));
    EMIT(og_mov(gc, PKT, R(edx)));
    *off += 4;
    if (flags & 3) {
        if (gc->vertexArray.enableFlags & __GL_VA_COLOR_ENABLE) {
            EMIT(og_mov(gc, R(edx), MEM(&gc->vertexArray.color)));
            EMIT(og_mov(gc, R(edx), ARR(edx, idx, 4)));
            EMIT(og_push(gc, R(eax)));
            EMIT(og_mov(gc, R(eax), R(edx)));
            EMIT(og_and(gc, R(eax), IMM(0xff00ff)));
            EMIT(og_and(gc, R(edx), IMM(0xff00ff00)));
            EMIT(og_rol(gc, R(eax), IMM(0x10)));
            EMIT(og_or(gc, R(edx), R(eax)));
            EMIT(og_pop(gc, R(eax)));
            if ((gc->fx.glowWorm && gc->fx.objectKey == gc->vertexArray.lockCount) || gc->fx.glowWorm == 3)
                EMIT(og_mov(gc, R(edx), MEM(&gc->fx.glowColor)));
        } else {
            EMIT(og_mov(gc, R(edx), MEM(&gc->state.current.packedColor)));
            if ((gc->fx.glowWorm && gc->fx.objectKey == gc->vertexArray.lockCount) || gc->fx.glowWorm == 3)
                EMIT(og_mov(gc, R(edx), MEM(&gc->fx.glowColor)));
        }
        EMIT(og_mov(gc, PKT, R(edx)));
        *off += 4;
    }
    if (flags & 4) {
        EMIT(og_mov(gc, R(edx), HWV(8)));
        EMIT(og_mov(gc, PKT, R(edx)));
        *off += 4;
    }
    if (flags & 8) {
        EMIT(og_mov(gc, R(edx), HWV(0xc)));
        EMIT(og_mov(gc, PKT, R(edx)));
        *off += 4;
    }
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        if (flags & 0x20) {
            TEXARRAY(0);
            EMIT(og_movlps(gc, R(mm0), ARR(edx, idx, 8)));
        }
        if (flags & 0x80) {
            TEXARRAY(1);
            EMIT(og_movhps(gc, R(mm0), ARR(edx, idx, 8)));
        }
        EMIT(og_movss(gc, R(mm1), HWV(0xc)));
        EMIT(og_shufps(gc, R(mm1), R(mm1), IMM(0)));
        EMIT(og_mulps(gc, R(mm0), R(mm1)));
        EMIT(og_mulps(gc, R(mm0), R(mm7)));
        if ((flags & 0x20) && (flags & 0x80)) {
            EMIT(og_movups(gc, PKT, R(mm0)));
            *off += 0x10;
        } else {
            if (flags & 0x20) {
                EMIT(og_movss(gc, PKT, R(mm0)));
                *off += 4;
                EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0xe5)));
                EMIT(og_movss(gc, PKT, R(mm0)));
                *off += 4;
            }
            if (flags & 0x80) {
                EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0xe6)));
                EMIT(og_movss(gc, PKT, R(mm0)));
                *off += 4;
                EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0xe7)));
                EMIT(og_movss(gc, PKT, R(mm0)));
                *off += 4;
            }
        }
    } else if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
        if (flags & 0x20) {
            TEXARRAY(0);
            EMIT(og_movq(gc, R(mm0), ARR(edx, idx, 8)));
            EMIT(og_pfmul(gc, R(mm0), TEXSCALE(0, 0)));
        }
        if (flags & 0x80) {
            TEXARRAY(1);
            EMIT(og_movq(gc, R(mm1), ARR(edx, idx, 8)));
            EMIT(og_pfmul(gc, R(mm1), TEXSCALE(1, 0)));
        }
        EMIT(og_movd(gc, R(mm2), HWV(0xc)));
        EMIT(og_punpckldq(gc, R(mm2), R(mm2)));
        EMIT(og_pfmul(gc, R(mm0), R(mm2)));
        EMIT(og_pfmul(gc, R(mm1), R(mm2)));
        if (flags & 0x20) {
            EMIT(og_movd(gc, PKT, R(mm0)));
            *off += 4;
            EMIT(og_punpckhdq(gc, R(mm0), R(mm0)));
            EMIT(og_movd(gc, PKT, R(mm0)));
            *off += 4;
        }
        if (flags & 0x80) {
            EMIT(og_movd(gc, PKT, R(mm1)));
            *off += 4;
            EMIT(og_punpckhdq(gc, R(mm1), R(mm1)));
            EMIT(og_movd(gc, PKT, R(mm1)));
            *off += 4;
        }
    } else {
        if (flags & 0x20) {
            TEXARRAY(0);
            EMIT(og_fld(gc, ARR(edx, idx, 8)));
            EMIT(og_fmul(gc, TEXSCALE(0, 0)));
            EMIT(og_fmul(gc, HWV(0xc)));
            EMIT(og_fstp(gc, PKT));
            *off += 4;
            EMIT(og_add(gc, R(edx), IMM(4)));
            EMIT(og_fld(gc, ARR(edx, idx, 8)));
            EMIT(og_fmul(gc, TEXSCALE(0, 1)));
            EMIT(og_fmul(gc, HWV(0xc)));
            EMIT(og_fstp(gc, PKT));
            *off += 4;
        }
        if (flags & 0x80) {
            TEXARRAY(1);
            EMIT(og_fld(gc, ARR(edx, idx, 8)));
            EMIT(og_fmul(gc, TEXSCALE(1, 0)));
            EMIT(og_fmul(gc, HWV(0xc)));
            EMIT(og_fstp(gc, PKT));
            *off += 4;
            EMIT(og_add(gc, R(edx), IMM(4)));
            EMIT(og_fld(gc, ARR(edx, idx, 8)));
            EMIT(og_fmul(gc, TEXSCALE(1, 1)));
            EMIT(og_fmul(gc, HWV(0xc)));
            EMIT(og_fstp(gc, PKT));
            *off += 4;
        }
    }
    return pc;
}
#undef HWV
#undef PKT
#undef TEXARRAY
#undef TEXSCALE
#undef ARR

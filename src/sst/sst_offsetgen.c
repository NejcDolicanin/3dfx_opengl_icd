/*
** sst_offsetgen.c -- og_genOffsetFifo: generates the three bufRec[9] routines (gc->hwcx.genProcs9) of polygon
** offset on the direct-FIFO paths.  The original's routines computed units * scale + factor * max |dz/dx|, |dz/dy|
** for a triangle whose vertices are in eax / ebx / ecx (SSE / 3DNow / x87 versions, [0] vertex pointers, [1]
** indexed hardware vertices) and wrote it into the chip's depth-bias register (zaColor) before the triangle, [2]
** restoring it after: two register writes per offset triangle, each draining the pixel pipeline.  Now the
** routines are stubs that call sst_render.c's hwOffsetApply / hwOffsetApplyIdx / hwOffsetUndo, which put the same
** amount into the vertices' window z instead (MesaFX's way; SoF's decals cost 3 fps through the register).  The
** callers' registers are preserved (eax, ecx, edx saved; ebx, esi, edi, ebp by the callee); femms first on a 3DNow
** CPU, as the callers leave the MMX state in use and the helpers use x87.
*/
#include "wglpriv.h"
#include "og.h"
#include "icdprotos.h"
#include <glide.h>

/* the bufRec[9] data page: the three entry points (-> hwcx.genProcs9) */
typedef struct OffsetGenBlockRec {
    GLuint      proc[3];
} OffsetGenBlock;

/* one stub: save eax / ecx / edx, [femms], push the arguments (right to left), call the __cdecl helper, pop */
static GLubyte *og_offsetStub(__GLcontext *gc, GLubyte *pc, void *helper, GLboolean withVertices)
{
    EMIT(og_push(gc, R(eax)));
    EMIT(og_push(gc, R(ecx)));
    EMIT(og_push(gc, R(edx)));
    if (gc->constants.cpuCaps & __GL_CPU_3DNOW)
        EMIT(og_femms(gc));
    if (withVertices) {
        EMIT(og_push(gc, R(ecx)));          /* c */
        EMIT(og_push(gc, R(ebx)));          /* b */
        EMIT(og_push(gc, R(eax)));          /* a */
        EMIT(og_push(gc, R(edx)));          /* the area's bits */
    }
    EMIT(og_push(gc, IMM(gc)));
    pc = og_callAbs(gc, pc, (GLuint)helper);
    EMIT(og_add(gc, R(esp), IMM(withVertices ? 20 : 4)));
    EMIT(og_pop(gc, R(edx)));
    EMIT(og_pop(gc, R(ecx)));
    EMIT(og_pop(gc, R(eax)));
    return og_encode(gc, pc, og_ret(gc));
}

/* 6902ad50 -- og_genOffsetFifo: generates the polygon-offset command-FIFO routines bufRec[9] (sst_offsetgen.c
** header): [0] a triangle of vertex pointers (eax / ebx / ecx, edx = area), [1] of indexed hardware vertices,
** [2] the undo after the draw */
void __fastcall og_genOffsetFifo(__GLcontext *gc)
{
    OffsetGenBlock *blk;
    GLubyte *pc;
    GLuint key;

    if (gc->hwcx.cmdTransportInfo == 0)
        return;
    key = (gc->modes.depthBits > 16 ? 2 : 0) | ((gc->modes.depthBits > 16 && gc->fx.zPrecision32) ? 4 : 0) | 1;
    if (((GLint (__fastcall *)(__GLcontext *, __GLhwBufRecICD *, GLuint *, OffsetGenBlock **))gc->hwcx.bufRec[9].alloc)
            (gc, &gc->hwcx.bufRec[9], &key, &blk)) {
        gc->hwcx.genProcs9[0] = (__GLproc)blk->proc[0];
        gc->hwcx.genProcs9[1] = (__GLproc)blk->proc[1];
        gc->hwcx.genProcs9[2] = (__GLproc)blk->proc[2];
        return;
    }
    pc = hwPageCacheInsert(gc, &gc->hwcx.bufRec[9], &key, &blk);
    og_reset(gc);
    blk->proc[0] = (GLuint)pc;
    pc = og_offsetStub(gc, pc, (void *)hwOffsetApply, GL_TRUE);
    blk->proc[1] = (GLuint)pc;
    pc = og_offsetStub(gc, pc, (void *)hwOffsetApplyIdx, GL_TRUE);
    blk->proc[2] = (GLuint)pc;
    og_offsetStub(gc, pc, (void *)hwOffsetUndo, GL_FALSE);
    gc->hwcx.genProcs9[0] = (__GLproc)blk->proc[0];
    gc->hwcx.genProcs9[1] = (__GLproc)blk->proc[1];
    gc->hwcx.genProcs9[2] = (__GLproc)blk->proc[2];
}

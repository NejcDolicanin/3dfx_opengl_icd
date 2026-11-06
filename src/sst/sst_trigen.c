/*
** sst_trigen.c -- hwBuildFifoTriProcs, the og generator of the bufRec[7] triangle routines that write vertices
** straight into the command FIFO.
** Then the command-FIFO helpers: the vertex write, the FIFO keys and the room check, in address order.
*/
#include "wglpriv.h"
#include "og.h"
#include <glide.h>
#include "glsi.h"
#include "icdprotos.h"
#include "glshade.h"
#include "icdglob.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* the bufRec[7] block: the entry points of the generated routines */
typedef struct TriGenBlockRec {
    GLuint      unk_00;
    GLuint      drawTri;                    /* +0x04  a triangle, vertices ecx, edx, [esp+8] (ret 4) */
    GLuint      drawQuad;                   /* +0x08  a quad as a fan (ret 8) */
    GLuint      drawTriPackColor;           /* +0x0c  the triangle with each vertex's own colour (smooth), else drawTri */
    GLuint      tri3Reg;                    /* +0x10  a triangle, vertices eax, ebx, ecx (-> hwcx.genBlock_10) */
    GLuint      oneVertex;                  /* +0x14  one strip vertex in ecx (-> hwcx.genBlock_14) */
    GLuint      drawTriIndexed;             /* +0x18  drawTri for indexed hardware vertices (-> hwcx.genBlock_18) */
    GLuint      unk_1c[5];
    GLuint      limit[4];                   /* +0x30  0x7fffffff (as 6902ad50's block +0x20) */
} TriGenBlock;

extern void __fastcall hwFifoVertexKey(__GLcontext *gc, GLuint *key);    /* the FIFO vertex routines' key */

/* 6902eaa0 -- hwBuildFifoTriProcs: (re)build or look up the bufRec[7] block for the current vertex format
** and install its routines (hwcx.drawTri, drawQuad, drawTriPackColor, genBlock_10 / _14 / _18).  Each
** routine returns at once while the Glide context is lost (og_lostContextCheck), makes room for its packet (the
** shared makeRoom subroutine, edi = bytes), writes the packet-3 header (vertex count, strip / independent
** command, packed colour bit 0x10000000, the parameter mask fmt << 10) and copies the enabled vertex fields
** (hwCopyVertexPacked / hwCopyVertexIndexed / hwCopyVertexFloat) to fifoPtr.  Nothing without the
** command-transport FIFO.
*/
void __fastcall hwBuildFifoTriProcs(__GLcontext *gc)
{
    GLint fifo, room, ptr;      /* the transport info; then &fifo->fifoRoom (+0x14), &fifo->fifoPtr (+0xc) */
    GLuint key, hdr, hdr2;      /* key: the bufRec[7] lookup key, then the parameter mask fmt << 10 */
    TriGenBlock *blk;
    GLubyte *pc;
    OgLabel *makeRoom, *triRoom, *idxRoom, *quadRoom, *reg3Room, *vtxRoom, *packRoom;
    GLuint fmt;                 /* the packet's vertex fields: the setup parameter mask SST_SETUP_RGB | _A (3), _Z (4),
                                   _Wfbi (8), _W0 (0x10), _ST0 (0x20), _W1 (0x40), _ST1 (0x80) */
    GLint size, size3, off;     /* bytes per vertex; the 3-vertex packet; the running packet offset */
    GLubyte flags;
    int i;

    fifo = (GLint)gc->hwcx.cmdTransportInfo;
    if (fifo == 0)
        return;
    ptr = fifo;
    hwFifoVertexKey(gc, &key);
    if (((GLint (__fastcall *)(__GLcontext *, __GLhwBufRecICD *, GLuint *, TriGenBlock **))gc->hwcx.bufRec[7].alloc)
            (gc, &gc->hwcx.bufRec[7], &key, &blk)) {
        gc->hwcx.drawTri = (__GLproc)blk->drawTri;
        gc->hwcx.genBlock_18 = blk->drawTriIndexed;
        gc->hwcx.drawQuad = (__GLproc)blk->drawQuad;
        gc->hwcx.drawTriPackColor = (__GLproc)blk->drawTriPackColor;
        gc->hwcx.genBlock_10 = blk->tri3Reg;
        gc->hwcx.genBlock_14 = blk->oneVertex;
        return;
    }
    pc = hwPageCacheInsert(gc, &gc->hwcx.bufRec[7], &key, &blk);
    og_reset(gc);
    makeRoom = og_newFixup(gc);
    for (i = 0; i < 4; i++)
        blk->limit[i] = 0x7fffffff;

    /* make room for edi bytes */
    LABEL(makeRoom);
    EMIT(og_push(gc, R(ecx)));
    EMIT(og_push(gc, R(edx)));
    EMIT(og_push(gc, IMM(0)));
    EMIT(og_push(gc, IMM(0)));
    EMIT(og_push(gc, R(edi)));
    EMIT(og_call(gc, MEM(&gc->hwcx.grCommandTransportMakeRoomExt2)));
    EMIT(og_pop(gc, R(edx)));
    EMIT(og_pop(gc, R(ecx)));
    fifo += 0x14;
    room = fifo;
    EMIT(og_mov(gc, R(eax), MEM(room)));
    EMIT(og_sub(gc, R(eax), R(edi)));
    EMIT(og_ret(gc));

    /* the vertex format and its size in the packet: x, y, colour; z with depth testing; s, t per enabled
       unit (unit 0 on TMU1 with two TMUs) and the TMU q with non-unity q; w always */
    fmt = 3;
    size = 0xc;
    if (gc->state.enables.general & __GL_DEPTH_TEST_ENABLE) {
        fmt = 7;
        size = 0x10;
    }
    if (gc->fx.numTMUs == 1) {
        if (gc->texture.textureEnabled[0]) {
            fmt |= 0x20;
            size += 8;
        }
        if ((fmt & 0x20) && gc->hwcx.nonUnityQ) {
            fmt |= 0x10;
            size += 4;
        }
    } else {
        if (gc->texture.textureEnabled[0]) {
            fmt |= 0x80;
            size += 8;
        }
        if (gc->texture.textureEnabled[1]) {
            fmt |= 0x20;
            size += 8;
        }
        if ((fmt & 0xa0) && gc->hwcx.nonUnityQ) {
            fmt |= 0x50;
            size += 8;
        }
    }
    size += 4;
    fmt |= 8;

    /* +4: a triangle, vertices ecx, edx, [esp+8] (ret 4; the flat colour from edx) */
    blk->drawTri = (GLuint)pc;
    gc->hwcx.drawTri = (__GLproc)blk->drawTri;
    pc = og_lostContextCheck(gc, pc, R(eax), IMM(4));
    EMIT(og_push(gc, R(edi)));
    triRoom = og_newLabel(gc);
    EMIT(og_mov(gc, R(eax), MEM(room)));
    size3 = size * 3 + 4;
    EMIT(og_mov(gc, R(edi), IMM(size3)));
    EMIT(og_sub(gc, R(eax), R(edi)));
    EMIT(og_jge(gc, triRoom));
    EMIT(og_call(gc, makeRoom));
    LABEL(triRoom);
    EMIT(og_mov(gc, MEM(room), R(eax)));
    ptr += 0xc;
    EMIT(og_mov(gc, R(edi), MEM(ptr)));
    key = fmt << 10;
    hdr = key | 0x100000c3;
    EMIT(og_mov(gc, R(eax), IMM(hdr)));
    EMIT(og_mov(gc, MEM0(edi), R(eax)));
    off = 4;
    if (gc->constants.cpuCaps & __GL_CPU_3DNOW)
        EMIT(og_femms(gc));
    flags = (GLubyte)fmt;
    pc = hwCopyVertexPacked(gc, pc, R(ecx), &off, flags, R(eax), R(edx));
    pc = hwCopyVertexPacked(gc, pc, R(edx), &off, flags, R(eax), R(edx));
    EMIT(og_mov(gc, R(ecx), MEMR(8, esp)));
    pc = hwCopyVertexPacked(gc, pc, R(ecx), &off, flags, R(eax), R(edx));
    if (gc->constants.cpuCaps & __GL_CPU_3DNOW)
        EMIT(og_femms(gc));
    EMIT(og_add(gc, R(edi), IMM(off)));
    EMIT(og_mov(gc, MEM(ptr), R(edi)));
    EMIT(og_pop(gc, R(edi)));
    EMIT(og_ret_imm(gc, IMM(4)));

    /* +0x18: the same for indexed hardware vertices */
    blk->drawTriIndexed = (GLuint)pc;
    gc->hwcx.genBlock_18 = blk->drawTriIndexed;
    pc = og_lostContextCheck(gc, pc, R(eax), IMM(4));
    EMIT(og_push(gc, R(edi)));
    idxRoom = og_newLabel(gc);
    EMIT(og_mov(gc, R(eax), MEM(room)));
    EMIT(og_mov(gc, R(edi), IMM(size3)));
    EMIT(og_sub(gc, R(eax), R(edi)));
    EMIT(og_jge(gc, idxRoom));
    EMIT(og_call(gc, makeRoom));
    LABEL(idxRoom);
    EMIT(og_mov(gc, MEM(room), R(eax)));
    EMIT(og_mov(gc, R(edi), MEM(ptr)));
    EMIT(og_mov(gc, R(eax), IMM(hdr)));
    EMIT(og_mov(gc, MEM0(edi), R(eax)));
    off = 4;
    if (gc->constants.cpuCaps & __GL_CPU_3DNOW)
        EMIT(og_femms(gc));
    pc = hwCopyVertexIndexed(gc, pc, R(ecx), &off, flags, R(eax), R(edx));
    pc = hwCopyVertexIndexed(gc, pc, R(edx), &off, flags, R(eax), R(edx));
    EMIT(og_mov(gc, R(ecx), MEMR(8, esp)));
    pc = hwCopyVertexIndexed(gc, pc, R(ecx), &off, flags, R(eax), R(edx));
    if (gc->constants.cpuCaps & __GL_CPU_3DNOW)
        EMIT(og_femms(gc));
    EMIT(og_add(gc, R(edi), IMM(off)));
    EMIT(og_mov(gc, MEM(ptr), R(edi)));
    EMIT(og_pop(gc, R(edi)));
    EMIT(og_ret_imm(gc, IMM(4)));

    /* +8: a quad, vertices ecx, edx, [esp+8], [esp+0xc] (ret 8) */
    hdr2 = key | 0x1400010b;
    blk->drawQuad = (GLuint)pc;
    gc->hwcx.drawQuad = (__GLproc)blk->drawQuad;
    pc = og_lostContextCheck(gc, pc, R(eax), IMM(8));
    EMIT(og_push(gc, R(edi)));
    quadRoom = og_newLabel(gc);
    EMIT(og_mov(gc, R(eax), MEM(room)));
    EMIT(og_mov(gc, R(edi), IMM(size * 4 + 4)));
    EMIT(og_sub(gc, R(eax), R(edi)));
    EMIT(og_jge(gc, quadRoom));
    EMIT(og_call(gc, makeRoom));
    LABEL(quadRoom);
    EMIT(og_mov(gc, MEM(room), R(eax)));
    EMIT(og_mov(gc, R(edi), MEM(ptr)));
    EMIT(og_mov(gc, R(eax), IMM(hdr2)));
    EMIT(og_mov(gc, MEM0(edi), R(eax)));
    off = 4;
    if (gc->constants.cpuCaps & __GL_CPU_3DNOW)
        EMIT(og_femms(gc));
    pc = hwCopyVertexPacked(gc, pc, R(ecx), &off, flags, R(eax), R(edx));
    pc = hwCopyVertexPacked(gc, pc, R(edx), &off, flags, R(eax), R(edx));
    EMIT(og_mov(gc, R(ecx), MEMR(8, esp)));
    pc = hwCopyVertexPacked(gc, pc, R(ecx), &off, flags, R(eax), R(edx));
    EMIT(og_mov(gc, R(ecx), MEMR(0xc, esp)));
    pc = hwCopyVertexPacked(gc, pc, R(ecx), &off, flags, R(eax), R(edx));
    if (gc->constants.cpuCaps & __GL_CPU_3DNOW)
        EMIT(og_femms(gc));
    EMIT(og_add(gc, R(edi), IMM(off)));
    EMIT(og_mov(gc, MEM(ptr), R(edi)));
    EMIT(og_pop(gc, R(edi)));
    EMIT(og_ret_imm(gc, IMM(8)));

    /* +0x10: a triangle, vertices eax, ebx, ecx (begins a strip; flat colour from eax; preserves eax) */
    blk->tri3Reg = (GLuint)pc;
    gc->hwcx.genBlock_10 = (GLuint)pc;
    pc = og_lostContextCheck(gc, pc, R(edx), IMM(0));
    reg3Room = og_newLabel(gc);
    EMIT(og_mov(gc, R(edx), MEM(room)));
    EMIT(og_mov(gc, R(edi), IMM(size3)));
    EMIT(og_sub(gc, R(edx), R(edi)));
    EMIT(og_jge(gc, reg3Room));
    EMIT(og_push(gc, R(eax)));
    EMIT(og_call(gc, makeRoom));
    EMIT(og_mov(gc, R(edx), R(eax)));
    EMIT(og_pop(gc, R(eax)));
    LABEL(reg3Room);
    EMIT(og_mov(gc, MEM(room), R(edx)));
    EMIT(og_mov(gc, R(edi), MEM(ptr)));
    EMIT(og_mov(gc, R(edx), IMM(key | 0x100000cb)));
    EMIT(og_mov(gc, MEM0(edi), R(edx)));
    off = 4;
    pc = hwCopyVertexPacked(gc, pc, R(eax), &off, flags, R(edx), R(eax));
    pc = hwCopyVertexPacked(gc, pc, R(ebx), &off, flags, R(edx), R(eax));
    pc = hwCopyVertexPacked(gc, pc, R(ecx), &off, flags, R(edx), R(eax));
    EMIT(og_add(gc, R(edi), IMM(off)));
    EMIT(og_mov(gc, MEM(ptr), R(edi)));
    EMIT(og_ret(gc));

    /* +0x14: the strip's next vertex in ecx */
    blk->oneVertex = (GLuint)pc;
    gc->hwcx.genBlock_14 = (GLuint)pc;
    pc = og_lostContextCheck(gc, pc, R(eax), IMM(0));
    vtxRoom = og_newLabel(gc);
    EMIT(og_mov(gc, R(eax), MEM(room)));
    EMIT(og_mov(gc, R(edi), IMM(size + 4)));
    EMIT(og_sub(gc, R(eax), R(edi)));
    EMIT(og_jge(gc, vtxRoom));
    EMIT(og_call(gc, makeRoom));
    LABEL(vtxRoom);
    EMIT(og_mov(gc, MEM(room), R(eax)));
    EMIT(og_mov(gc, R(edi), MEM(ptr)));
    EMIT(og_mov(gc, R(eax), IMM(key | 0x10000053)));
    EMIT(og_mov(gc, MEM0(edi), R(eax)));
    off = 4;
    pc = hwCopyVertexPacked(gc, pc, R(ecx), &off, flags, R(edx), R(ecx));
    EMIT(og_add(gc, R(edi), IMM(off)));
    EMIT(og_mov(gc, MEM(ptr), R(edi)));
    EMIT(og_ret(gc));

    /* +0xc: smooth shading: the triangle with each vertex's float colour (no packed-colour bit); flat:
       drawTri */
    if (gc->state.light.shadingModel != GL_SMOOTH) {
        blk->drawTriPackColor = blk->drawTri;
    } else {
        blk->drawTriPackColor = (GLuint)pc;
        pc = og_lostContextCheck(gc, pc, R(eax), IMM(4));
        EMIT(og_push(gc, R(edi)));
        packRoom = og_newLabel(gc);
        EMIT(og_mov(gc, R(eax), MEM(room)));
        EMIT(og_mov(gc, R(edi), IMM(size * 3 + 0x28)));
        EMIT(og_sub(gc, R(eax), R(edi)));
        EMIT(og_jge(gc, packRoom));
        EMIT(og_call(gc, makeRoom));
        LABEL(packRoom);
        EMIT(og_mov(gc, MEM(room), R(eax)));
        EMIT(og_mov(gc, R(edi), MEM(ptr)));
        EMIT(og_mov(gc, R(eax), IMM(key | 0xc3)));
        EMIT(og_mov(gc, MEM0(edi), R(eax)));
        off = 4;
        if (gc->constants.cpuCaps & __GL_CPU_3DNOW)
            EMIT(og_femms(gc));
        pc = hwCopyVertexFloat(gc, pc, R(ecx), &off, flags, R(eax));
        pc = hwCopyVertexFloat(gc, pc, R(edx), &off, flags, R(eax));
        EMIT(og_mov(gc, R(ecx), MEMR(8, esp)));
        pc = hwCopyVertexFloat(gc, pc, R(ecx), &off, flags, R(eax));
        if (gc->constants.cpuCaps & __GL_CPU_3DNOW)
            EMIT(og_femms(gc));
        EMIT(og_add(gc, R(edi), IMM(off)));
        EMIT(og_mov(gc, MEM(ptr), R(edi)));
        EMIT(og_pop(gc, R(edi)));
        og_encode(gc, pc, og_ret_imm(gc, IMM(4)));
    }
    gc->hwcx.drawTriPackColor = (__GLproc)blk->drawTriPackColor;
}

/* 6902ad10 (size 53) -- hwFifoWriteVertex3: write three words of a vertex (0, 1, 5: x, y and the packed colour) at
** the command FIFO pointer */
void __fastcall hwFifoWriteVertex3(GrCmdTransportInfo *fifo, GLuint *v)
{
    *fifo->fifoPtr = v[0];
    fifo->fifoPtr++;
    *fifo->fifoPtr = v[1];
    fifo->fifoPtr++;
    *fifo->fifoPtr = v[5];
    fifo->fifoPtr++;
}

/* 690300b0 (size 106) -- hwFifoVertexKey: the key of the generated FIFO vertex routines (0 without the FIFO path) */
void __fastcall hwFifoVertexKey(__GLcontext *gc, GLuint *key)
{
    if (gc->hwcx.cmdTransportInfo) {
        *key = (gc->hwcx.nonUnityQ ? 0x20 : 0)
             | (gc->texture.textureEnabled[1] ? 8 : 0)
             | (gc->texture.textureEnabled[0] ? 4 : 0)
             | (gc->state.light.shadingModel == GL_SMOOTH ? 2 : 0)
             | (gc->state.enables.general & __GL_DEPTH_TEST_ENABLE)
             | 1;
        return;
    }
    *key = 0;
}

/* 6903a4e0 (size 317) -- hwFifoStateKey: the key of the generated FIFO routines for the current state: [0] mode bits,
** [1] / [2] the hardware vertex array, [3] z-cull / glow-worm bits (all zero without the FIFO path) */
void __fastcall hwFifoStateKey(__GLcontext *gc, GLuint *key)
{
    if (gc->hwcx.cmdTransportInfo) {
        GLuint gb, z, general;

        if (gc->fx.fullscreen && gc->fx.guardBand)
            gb = 0x10;
        else
            gb = 0;
        general = gc->state.enables.general;
        key[0] = ((((gc->vertexArray.enableFlags & 2) << 4) | (general & __GL_DEPTH_TEST_ENABLE)) << 2)
               | (gc->texture.textureEnabled[1] ? 4 : 0)
               | (gc->texture.textureEnabled[0] ? 2 : 0)
               | (((general & __GL_CULL_FACE_ENABLE) | 0x200) >> 9)
               | ((general & __GL_POLYGON_OFFSET_ANY_ENABLE) ? 0x20 : 0)
               | gb;
        key[1] = (GLuint)gc->hwcx.vtxBase;
        key[2] = (GLuint)gc->hwcx.vtxCapacity;
        if (gc->fx.fullscreen && gc->fx.zcullEnable && gc->state.viewport.zFar == 1.0 &&
            gc->state.viewport.zNear == 0.0)
            z = 2;
        else
            z = 0;
        key[3] = (((gc->fx.glowWorm && gc->fx.objectKey == gc->vertexArray.lockCount) ||
                   gc->fx.glowWorm == 3) ? 4 : 0) | z | 1;
    } else {
        key[0] = 0;
        key[1] = 0;
        key[2] = 0;
        key[3] = 0;
    }
}

/* ---- og code generator: og.h ---- */

/* 6903b520 (size 243) -- og_fifoMakeRoom: emit "make room for size bytes in the command FIFO": edx = fifoRoom - size;
** if negative call makeRoom with edx = size; fifoRoom = edx */
GLubyte *__fastcall og_fifoMakeRoom(__GLcontext *gc, GLubyte *pc, OgOperand *size, OgLabel *makeRoom)
{
    GrCmdTransportInfo *ti = gc->hwcx.cmdTransportInfo;
    OgLabel *l = og_newLabel(gc);

    EMIT(og_mov(gc, R(edx), MEM(&ti->fifoRoom)));
    EMIT(og_sub(gc, R(edx), size));
    EMIT(og_jge(gc, l));
    EMIT(og_mov(gc, R(edx), size));
    EMIT(og_call(gc, makeRoom));
    LABEL(l);
    EMIT(og_mov(gc, MEM(&ti->fifoRoom), R(edx)));
    return pc;
}

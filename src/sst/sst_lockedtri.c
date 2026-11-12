/*
** sst_lockedtri.c -- hwGenLockedTriangles, the og_* generator of the locked-array triangle routine (hwcx.genProc8, cached in
** bufRec[8]): the GL_TRIANGLES / GL_UNSIGNED_INT glDrawElements fast path of FX_GL_HSR (sst_varray.c).
*/
#include <stddef.h>
#include "wglpriv.h"
#include "og.h"
#include "glsi.h"
#include "icdglob.h"
#include "icdprotos.h"
#include <glide.h>

                                                                /* insert a key in a page cache (sst_codebuf.c) */
                                                                /* emit: ret retImm while the context is lost */
                                                                /* emit: array vertex idx -> packet [edi + *off] */
                                                                /* emit: make room for size bytes in the FIFO */

/* the routine's data page (bufRec[8] arena B; the generated code addresses it absolutely) */
typedef struct VsBlockRec {
    __GLfloat   xyzw[4][4];         /* 0x000  SSE: the clip x, y, z, w of four vertices ([c][v]); 3DNow / x87: [0]
                                               = one vertex's x, y, z, w; clipped path: the colour bytes as ints */
    __GLfloat   mvp[16][4];         /* 0x040  SSE: each mvp element splatted */
    __GLfloat   scale[3][4];        /* 0x140  SSE: viewport scale x, y, z splatted */
    __GLfloat   center[3][4];       /* 0x170  SSE: viewport center x, y, z splatted */
    GLuint      clipBits[6][4];     /* 0x1a0  SSE: the clip code bit of each test (__GL_CLIP_LEFT << k) splatted */
    GLuint      negOne[4];          /* 0x200  -1.0f (w * -1 = -w) */
    GLuint      absMask[4];         /* 0x210  0x7fffffff (set, not read) */
    GLuint      count;              /* 0x220  ecx at entry: the index count */
    GLuint      indices;            /* 0x224  edx at entry: the GLuint index list */
    GLuint      last;               /* 0x228  indices + count * 4 - 8: the triangle loop runs while esi is below */
    GLuint      tri[3];             /* 0x22c  the clipped triangle's vertex numbers (eax, ebx, ecx) */
    GLuint      prev[3];            /* 0x238  the last triangle's ebx, ecx, eax (the strip test) */
    __GLfloat   area;               /* 0x244  twice the signed window area (culling, polygon offset) */
    GLuint      pad248[2];
    __GLfloat   gb[4][4];           /* 0x250  guard band: SSE gbLeft, gbRight, gbBottom, gbTop splatted; 3DNow
                                               [0] = {gbRight, gbTop, gbLeft, gbBottom} */
    GLuint      clipBits3dn[3][2];  /* 0x290  3DNow: the clip bits of the paired tests (> w, < -w, z) */
    GLuint      pad2a8[2];
    __GLfloat   zFar[4];            /* 0x2b0  z culling: fx.zcullFar (SSE splatted) */
    __GLfloat   zNear[4];           /* 0x2c0  fx.zcullNear */
} VsBlock;

#define ELT(b, i, s)    og_mem(gc, NULL, R(b), R(i), s)             /* [b + i * s] */
#define IDX(d, b, i, s) og_mem(gc, IMM(d), R(b), R(i), s)           /* [b + i * s + d] */
/* the field-major hardware vertex arrays (hwcx.vtxBase): field f (a byte offset of the field in a 4-byte
   slot layout) of vertex i is at vtxBase + vtxCapacity * f + i * 4 */
#define HWV(f, i)       og_mem(gc, IMM((GLint)gc->hwcx.vtxBase + gc->hwcx.vtxCapacity * (f)), NULL, R(i), 4)
#define HW_X            0x00            /* window x */
#define HW_Y            0x04
#define HW_Z            0x08
#define HW_W            0x0c            /* 1 / clip w */
#define HW_CLIP         0x10            /* clip code (__GL_CLIP_*) */
#define HW_GB           0x14            /* guard-band clip code */
#define VX(f)           offsetof(__GLvertexICD, f)                  /* a staging vertex field (eax) */
#define VP(f)           offsetof(__GLviewport, f)                   /* a viewport field (ecx) */
#define MX(r, c)        offsetof(__GLmatrixICD, matrix[r][c])       /* an mvp element (ecx) */
/* the float-only callee (no EDX load) */
#define W_FCONST(gc, v) ((OgOperand *(__fastcall *)(__GLcontext *, double))og_fpk)(gc, v)
/* z culling in the generated code: full screen, the z-cull setting and the default depth range */
#define ZCULL           (gc->fx.fullscreen && gc->fx.zcullEnable && gc->state.viewport.zFar == 1.0 && \
                         gc->state.viewport.zNear == 0.0)

/* 690312c0 -- hwGenLockedTriangles: build (or find in the bufRec[8] cache) the routine hwcx.genProc8
   ** (ecx = count, edx = GLuint indices) that draws locked GL_TRIANGLES straight into the command FIFO:
   **   1. while vertexArray.flags has __GL_VERTARRAY_CHANGED: transform the lockCount array vertices (3 floats,
   **      w = 1) by the mvp into the hardware vertex arrays: window x, y, z, 1/w, the clip code and the
   **      guard-band clip code (SSE four at a time, 3DNow / x87 one at a time); clear the flag;
   **   2. per triangle (eax, ebx, ecx = its vertex numbers): trivially reject (all clip codes share a bit,
   **      z-cull planes), cull by the window area, send it through the clipper (genCode[CLIP_TRIANGLE] on staging
   **      vertices) when a clip code (or, with the guard band, a guard-band code) is set, else write it as a
   **      packet-3 triangle, or as a strip continuation when it shares an edge with the last one (ebp: 0 / 1
   **      the strip parity, 2 = start a new packet); polygon offset through hwcx.genProcs9[1] / [2].
   ** Subroutines: makeRoom (FIFO wrap), fifoPad (a 0 word to 8-byte align the FIFO, 3DNow), toStaging. */
void __fastcall hwGenLockedTriangles(__GLcontext *gc)
{
    VsBlock *blk;         /* address-taken: the block pointer, the packet-offset cell and the 16-byte cache key */
    GLuint regCount;      /* are separate locals (frame 0x48: blk 0x14, regCount 0x1c, key 0x48) */
    GLuint key[4];
    OgLabel *sendTri;     /* a new triangle packet (bound, not jumped to) */
    OgLabel *toStaging;   /* subroutine: hardware vertex edx -> the staging __GLvertexICD at eax */
    OgLabel *fifoPad;     /* subroutine: write a 0 word to the FIFO */
    OgLabel *triLoop;     /* the triangle loop */
    OgLabel *sendStrip;   /* a one-vertex strip continuation packet */
    OgLabel *makeRoom;    /* subroutine: wait for edx bytes of FIFO (grCommandTransportMakeRoomExt2) */
    OgLabel *nextTri;     /* the next triangle */
    OgLabel *clipTri;     /* the triangle through the clipper */
    GrCmdTransportInfo *fifo;
    OgLabel *xformDone;   /* the vertex transform is up to date */
    GLubyte *start;
    int k;
    OgLabel *xformLoop;   /* the vertex transform loop */
    OgLabel *roomOk;
    OgLabel *skip;
    int sz;               /* bytes per vertex in the packet */
    int sz3;
    int j;
    int i;
    __GLvertArrayMachine *va;
    GLubyte *pc;
    GLuint vflags;        /* the packet's parameter mask (<< 10 in the header) */

    fifo = gc->hwcx.cmdTransportInfo;
    if (fifo == NULL) {
        return;
    }
    hwFifoStateKey(gc, key);
    k = ((GLint (__fastcall *)(__GLcontext *, __GLhwBufRecICD *, GLuint *, VsBlock **))gc->hwcx.bufRec[8].alloc)
                    (gc, &gc->hwcx.bufRec[8], key, &blk);
    if (k) {
        gc->hwcx.genProc8 = (__GLproc)k;
        return;
    }
    start = hwPageCacheInsert(gc, &gc->hwcx.bufRec[8], key, &blk);
    gc->hwcx.genProc8 = (__GLproc)start;
    og_reset(gc);
    for (k = 0; k < 4; k++) {
        blk->negOne[k] = 0xbf800000;                /* -1.0f */
        blk->absMask[k] = 0x7fffffff;
        blk->clipBits[0][k] = __GL_CLIP_LEFT;
        blk->clipBits[1][k] = __GL_CLIP_RIGHT;
        blk->clipBits[2][k] = __GL_CLIP_BOTTOM;
        blk->clipBits[3][k] = __GL_CLIP_TOP;
        blk->clipBits[4][k] = __GL_CLIP_NEAR;
        blk->clipBits[5][k] = __GL_CLIP_FAR;
    }
    blk->clipBits3dn[0][0] = __GL_CLIP_RIGHT;
    blk->clipBits3dn[0][1] = __GL_CLIP_TOP;
    blk->clipBits3dn[1][0] = __GL_CLIP_LEFT;
    blk->clipBits3dn[1][1] = __GL_CLIP_BOTTOM;
    blk->clipBits3dn[2][0] = __GL_CLIP_FAR;
    blk->clipBits3dn[2][1] = __GL_CLIP_NEAR;
    xformLoop = og_newFixup(gc);
    triLoop = og_newFixup(gc);
    sendTri = og_newFixup(gc);
    sendStrip = og_newFixup(gc);
    clipTri = og_newFixup(gc);
    nextTri = og_newFixup(gc);
    makeRoom = og_newFixup(gc);
    fifoPad = og_newFixup(gc);
    toStaging = og_newFixup(gc);
    xformDone = og_newFixup(gc);

    /* entry: save the registers, the index range */
    pc = og_lostContextCheck(gc, start, R(eax), IMM(0));
    EMIT(og_push(gc, R(ebx)));
    EMIT(og_push(gc, R(ebp)));
    EMIT(og_push(gc, R(edi)));
    EMIT(og_push(gc, R(esi)));
    EMIT(og_mov(gc, MEM(&blk->count), R(ecx)));
    EMIT(og_mov(gc, MEM(&blk->indices), R(edx)));
    EMIT(og_lea(gc, R(eax), IDX(-8, edx, ecx, 4)));
    EMIT(og_mov(gc, MEM(&blk->last), R(eax)));
    /* the guard band and z-cull factors into the data page */
    if (gc->fx.fullscreen) {
        if (gc->fx.guardBand) {
            if (gc->constants.cpuCaps & __GL_CPU_SSE) {
                EMIT(og_movss(gc, R(mm0), MEM(&gc->fx.gbLeft)));
                EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
                EMIT(og_movaps(gc, MEM(&blk->gb[0]), R(mm0)));
                EMIT(og_movss(gc, R(mm0), MEM(&gc->fx.gbRight)));
                EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
                EMIT(og_movaps(gc, MEM(&blk->gb[1]), R(mm0)));
                EMIT(og_movss(gc, R(mm0), MEM(&gc->fx.gbBottom)));
                EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
                EMIT(og_movaps(gc, MEM(&blk->gb[2]), R(mm0)));
                EMIT(og_movss(gc, R(mm0), MEM(&gc->fx.gbTop)));
                EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
                EMIT(og_movaps(gc, MEM(&blk->gb[3]), R(mm0)));
            }
            else if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
                EMIT(og_mov(gc, R(eax), MEM(&gc->fx.gbLeft)));
                EMIT(og_mov(gc, MEM(&blk->gb[0][2]), R(eax)));
                EMIT(og_mov(gc, R(eax), MEM(&gc->fx.gbTop)));
                EMIT(og_mov(gc, MEM(&blk->gb[0][1]), R(eax)));
                EMIT(og_mov(gc, R(eax), MEM(&gc->fx.gbRight)));
                EMIT(og_mov(gc, MEM(&blk->gb[0]), R(eax)));
                EMIT(og_mov(gc, R(eax), MEM(&gc->fx.gbBottom)));
                EMIT(og_mov(gc, MEM(&blk->gb[0][3]), R(eax)));
                EMIT(og_mov(gc, R(eax), MEM(&gc->fx.zcullFar)));
                EMIT(og_mov(gc, MEM(&blk->zFar), R(eax)));
                EMIT(og_mov(gc, R(eax), MEM(&gc->fx.zcullNear)));
                EMIT(og_mov(gc, MEM(&blk->zNear), R(eax)));
            }
        }
        if (ZCULL) {
            if (gc->constants.cpuCaps & __GL_CPU_SSE) {
                EMIT(og_movss(gc, R(mm0), MEM(&gc->fx.zcullFar)));
                EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
                EMIT(og_movaps(gc, MEM(&blk->zFar), R(mm0)));
                EMIT(og_movss(gc, R(mm0), MEM(&gc->fx.zcullNear)));
                EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
                EMIT(og_movaps(gc, MEM(&blk->zNear), R(mm0)));
            }
            else if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
                EMIT(og_mov(gc, R(eax), MEM(&gc->fx.zcullFar)));
                EMIT(og_mov(gc, MEM(&blk->zFar), R(eax)));
                EMIT(og_mov(gc, R(eax), MEM(&gc->fx.zcullNear)));
                EMIT(og_mov(gc, MEM(&blk->zNear), R(eax)));
            }
        }
    }
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        /* SSE: the mvp elements and the viewport scale / center splatted (ecx = &modelView->mvp) */
        EMIT(og_mov(gc, R(ecx), MEM(&gc->transform.modelView)));
        EMIT(og_add(gc, R(ecx), IMM(offsetof(struct __GLtransformICDRec, mvp))));
        for (i = 0; i < 16; i++) {
            EMIT(og_movss(gc, R(mm0), MEMR(MX(0, 0) + i * 4, ecx)));
            EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
            EMIT(og_movaps(gc, MEM(&blk->mvp[i]), R(mm0)));
        }
        EMIT(og_movss(gc, R(mm0), MEM(&gc->state.viewport.scale)));
        EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
        EMIT(og_movaps(gc, MEM(&blk->scale[0]), R(mm0)));
        EMIT(og_movss(gc, R(mm0), MEM(&gc->state.viewport.center)));
        EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
        EMIT(og_movaps(gc, MEM(&blk->center[0]), R(mm0)));
        EMIT(og_movss(gc, R(mm0), MEM(&gc->state.viewport.scale.y)));
        EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
        EMIT(og_movaps(gc, MEM(&blk->scale[1]), R(mm0)));
        EMIT(og_movss(gc, R(mm0), MEM(&gc->state.viewport.center.y)));
        EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
        EMIT(og_movaps(gc, MEM(&blk->center[1]), R(mm0)));
        EMIT(og_movss(gc, R(mm0), MEM(&gc->state.viewport.scale.z)));
        EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
        EMIT(og_movaps(gc, MEM(&blk->scale[2]), R(mm0)));
        EMIT(og_movss(gc, R(mm0), MEM(&gc->state.viewport.center.z)));
        EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
        EMIT(og_movaps(gc, MEM(&blk->center[2]), R(mm0)));
    }
    /* 1. the vertex transform into the hardware arrays: esi = the array vertex, edx = its number; only when
       the arrays changed */
    EMIT(og_mov(gc, R(esi), MEM(&gc->vertexArray.vertex.pointer)));
    EMIT(og_xor(gc, R(edx), R(edx)));
    EMIT(og_mov(gc, R(eax), MEM(&gc->vertexArray.flags)));
    EMIT(og_test(gc, R(eax), IMM(__GL_VERTARRAY_CHANGED)));
    EMIT(og_je(gc, xformDone));
    LABEL(xformLoop);
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        /* SSE: four vertices: clip = mvp * (x, y, z, 1) with x, y, z transposed into xmm4 / xmm5 / xmm6.
           Original bugs: the prefetch address is edx + 0x80 (edx = the vertex number, not esi), and esi steps
           0x40 per group whatever vertex.stride is (right only for 16-byte vertices) */
        EMIT(og_prefetch(gc, MEMR(0x80, edx), IMM(1)));
        EMIT(og_mov(gc, R(eax), MEM(&gc->vertexArray.vertex.stride)));
        EMIT(og_mov(gc, R(ecx), R(esi)));
        EMIT(og_movlps(gc, R(mm4), MEM0(esi)));
        EMIT(og_add(gc, R(esi), R(eax)));
        EMIT(og_movhps(gc, R(mm4), MEM0(esi)));
        EMIT(og_add(gc, R(esi), R(eax)));
        EMIT(og_movlps(gc, R(mm0), MEM0(esi)));
        EMIT(og_add(gc, R(esi), R(eax)));
        EMIT(og_movhps(gc, R(mm0), MEM0(esi)));
        EMIT(og_movaps(gc, R(mm5), R(mm4)));
        EMIT(og_shufps(gc, R(mm4), R(mm0), IMM(0x88)));
        EMIT(og_shufps(gc, R(mm5), R(mm0), IMM(0xdd)));
        EMIT(og_mov(gc, R(esi), R(ecx)));
        EMIT(og_movlps(gc, R(mm6), MEMR(8, esi)));
        EMIT(og_add(gc, R(esi), R(eax)));
        EMIT(og_movhps(gc, R(mm6), MEMR(8, esi)));
        EMIT(og_add(gc, R(esi), R(eax)));
        EMIT(og_movlps(gc, R(mm1), MEMR(8, esi)));
        EMIT(og_add(gc, R(esi), R(eax)));
        EMIT(og_movhps(gc, R(mm1), MEMR(8, esi)));
        EMIT(og_shufps(gc, R(mm6), R(mm1), IMM(0x88)));
        EMIT(og_mov(gc, R(esi), R(ecx)));
        for (j = 0; j < 4; j++) {
            EMIT(og_movaps(gc, R(mm0), MEM(&blk->mvp[j])));
            EMIT(og_mulps(gc, R(mm0), R(mm4)));
            EMIT(og_movaps(gc, R(mm1), MEM(&blk->mvp[4 + j])));
            EMIT(og_mulps(gc, R(mm1), R(mm5)));
            EMIT(og_movaps(gc, R(mm2), MEM(&blk->mvp[8 + j])));
            EMIT(og_mulps(gc, R(mm2), R(mm6)));
            EMIT(og_movaps(gc, R(mm3), MEM(&blk->mvp[12 + j])));
            EMIT(og_addps(gc, R(mm0), R(mm1)));
            EMIT(og_addps(gc, R(mm2), R(mm3)));
            EMIT(og_addps(gc, R(mm0), R(mm2)));
            EMIT(og_movaps(gc, MEM(&blk->xyzw[j]), R(mm0)));
        }
        EMIT(og_movaps(gc, R(mm4), MEM(&blk->xyzw[0][0])));
        EMIT(og_movaps(gc, R(mm5), MEM(&blk->xyzw[1])));
        EMIT(og_movaps(gc, R(mm6), MEM(&blk->xyzw[2])));
        EMIT(og_movaps(gc, R(mm7), MEM(&blk->xyzw[3])));
        EMIT(og_movaps(gc, R(mm3), R(mm7)));
        EMIT(og_mulps(gc, R(mm3), MEM(&blk->negOne)));
        /* the guard-band clip code: x against -w * gbLeft / w * gbRight, y against bottom / top */
        if (gc->fx.fullscreen && gc->fx.guardBand) {
            EMIT(og_movaps(gc, R(mm2), R(mm3)));
            EMIT(og_mulps(gc, R(mm7), MEM(&blk->gb[1])));
            EMIT(og_mulps(gc, R(mm3), MEM(&blk->gb[0])));
            EMIT(og_movaps(gc, R(mm0), R(mm4)));
            EMIT(og_cmpps(gc, R(mm0), R(mm3), IMM(1)));
            EMIT(og_andps(gc, R(mm0), MEM(&blk->clipBits[0])));
            EMIT(og_movaps(gc, R(mm1), R(mm4)));
            EMIT(og_cmpps(gc, R(mm1), R(mm7), IMM(6)));
            EMIT(og_andps(gc, R(mm1), MEM(&blk->clipBits[1])));
            EMIT(og_orps(gc, R(mm0), R(mm1)));
            EMIT(og_movaps(gc, R(mm7), MEM(&blk->xyzw[3])));
            EMIT(og_movaps(gc, R(mm3), R(mm2)));
            EMIT(og_mulps(gc, R(mm7), MEM(&blk->gb[3])));
            EMIT(og_mulps(gc, R(mm3), MEM(&blk->gb[2])));
            EMIT(og_movaps(gc, R(mm1), R(mm5)));
            EMIT(og_cmpps(gc, R(mm1), R(mm3), IMM(1)));
            EMIT(og_andps(gc, R(mm1), MEM(&blk->clipBits[2])));
            EMIT(og_orps(gc, R(mm0), R(mm1)));
            EMIT(og_movaps(gc, R(mm1), R(mm5)));
            EMIT(og_cmpps(gc, R(mm1), R(mm7), IMM(6)));
            EMIT(og_andps(gc, R(mm1), MEM(&blk->clipBits[3])));
            EMIT(og_orps(gc, R(mm0), R(mm1)));
            EMIT(og_movaps(gc, HWV(HW_GB, edx), R(mm0)));
            EMIT(og_movaps(gc, R(mm7), MEM(&blk->xyzw[3])));
            EMIT(og_movaps(gc, R(mm3), R(mm2)));
        }
        /* the clip code: x, y, z against -w / w */
        EMIT(og_movaps(gc, R(mm0), R(mm4)));
        EMIT(og_cmpps(gc, R(mm0), R(mm3), IMM(1)));
        EMIT(og_andps(gc, R(mm0), MEM(&blk->clipBits[0])));
        EMIT(og_movaps(gc, R(mm1), R(mm4)));
        EMIT(og_cmpps(gc, R(mm1), R(mm7), IMM(6)));
        EMIT(og_andps(gc, R(mm1), MEM(&blk->clipBits[1])));
        EMIT(og_orps(gc, R(mm0), R(mm1)));
        EMIT(og_movaps(gc, R(mm1), R(mm5)));
        EMIT(og_cmpps(gc, R(mm1), R(mm3), IMM(1)));
        EMIT(og_andps(gc, R(mm1), MEM(&blk->clipBits[2])));
        EMIT(og_orps(gc, R(mm0), R(mm1)));
        EMIT(og_movaps(gc, R(mm1), R(mm5)));
        EMIT(og_cmpps(gc, R(mm1), R(mm7), IMM(6)));
        EMIT(og_andps(gc, R(mm1), MEM(&blk->clipBits[3])));
        EMIT(og_orps(gc, R(mm0), R(mm1)));
        /* z culling: the far / near tests against w * zcullFar / -w * zcullNear */
        if (ZCULL) {
            EMIT(og_mulps(gc, R(mm7), MEM(&blk->zFar)));
            EMIT(og_mulps(gc, R(mm3), MEM(&blk->zNear)));
        }
        EMIT(og_movaps(gc, R(mm1), R(mm6)));
        EMIT(og_cmpps(gc, R(mm1), R(mm7), IMM(6)));
        EMIT(og_andps(gc, R(mm1), MEM(&blk->clipBits[5])));
        EMIT(og_orps(gc, R(mm0), R(mm1)));
        EMIT(og_movaps(gc, R(mm1), R(mm6)));
        EMIT(og_cmpps(gc, R(mm1), R(mm3), IMM(1)));
        EMIT(og_movaps(gc, R(mm3), R(mm1)));
        EMIT(og_andps(gc, R(mm1), MEM(&blk->clipBits[4])));
        EMIT(og_orps(gc, R(mm0), R(mm1)));
        EMIT(og_movaps(gc, HWV(HW_CLIP, edx), R(mm0)));
        /* window = clip * viewport.scale / w + viewport.center (1/w: rcpps and one Newton step) */
        EMIT(og_movaps(gc, R(mm1), MEM(&blk->scale[0])));
        EMIT(og_movaps(gc, R(mm0), MEM(&blk->xyzw[3])));
        EMIT(og_rcpps(gc, R(mm7), R(mm0)));
        EMIT(og_movaps(gc, R(mm2), MEM(&blk->scale[1])));
        EMIT(og_mulps(gc, R(mm0), R(mm7)));
        EMIT(og_mulps(gc, R(mm0), R(mm7)));
        EMIT(og_movaps(gc, R(mm3), MEM(&blk->scale[2])));
        EMIT(og_addps(gc, R(mm7), R(mm7)));
        EMIT(og_subps(gc, R(mm7), R(mm0)));
        EMIT(og_mulps(gc, R(mm4), R(mm1)));
        EMIT(og_mulps(gc, R(mm5), R(mm2)));
        EMIT(og_mulps(gc, R(mm6), R(mm3)));
        EMIT(og_mulps(gc, R(mm4), R(mm7)));
        EMIT(og_movaps(gc, R(mm1), MEM(&blk->center[0])));
        EMIT(og_mulps(gc, R(mm5), R(mm7)));
        EMIT(og_movaps(gc, R(mm2), MEM(&blk->center[1])));
        EMIT(og_mulps(gc, R(mm6), R(mm7)));
        EMIT(og_movaps(gc, R(mm3), MEM(&blk->center[2])));
        EMIT(og_addps(gc, R(mm4), R(mm1)));
        EMIT(og_addps(gc, R(mm5), R(mm2)));
        EMIT(og_addps(gc, R(mm6), R(mm3)));
        EMIT(og_movaps(gc, HWV(HW_X, edx), R(mm4)));
        EMIT(og_movaps(gc, HWV(HW_Y, edx), R(mm5)));
        EMIT(og_movaps(gc, HWV(HW_Z, edx), R(mm6)));
        EMIT(og_movaps(gc, HWV(HW_W, edx), R(mm7)));
        EMIT(og_add(gc, R(esi), IMM(0x40)));
        EMIT(og_add(gc, R(edx), IMM(4)));
        EMIT(og_cmp(gc, R(edx), MEM(&gc->vertexArray.lockCount)));
        EMIT(og_jl(gc, xformLoop));
    }
    else {
        /* 3DNow: one vertex: clip = mvp * (x, y, z, 1), 1/w by pfrcp and two iterations */
        if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
            EMIT(og_mov(gc, R(ecx), MEM(&gc->transform.modelView)));
            EMIT(og_add(gc, R(ecx), IMM(offsetof(struct __GLtransformICDRec, mvp))));
            EMIT(og_movq(gc, R(mm0), MEMR(0, esi)));
            EMIT(og_movq(gc, R(mm1), R(mm0)));
            EMIT(og_punpckldq(gc, R(mm0), R(mm0)));
            EMIT(og_punpckhdq(gc, R(mm1), R(mm1)));
            EMIT(og_movq(gc, R(mm2), BASE(MX(0, 0), ecx)));
            EMIT(og_pfmul(gc, R(mm2), R(mm0)));
            EMIT(og_movq(gc, R(mm3), BASE(MX(0, 2), ecx)));
            EMIT(og_pfmul(gc, R(mm3), R(mm0)));
            EMIT(og_movq(gc, R(mm4), BASE(MX(1, 0), ecx)));
            EMIT(og_pfmul(gc, R(mm4), R(mm1)));
            EMIT(og_movq(gc, R(mm5), BASE(MX(1, 2), ecx)));
            EMIT(og_pfmul(gc, R(mm5), R(mm1)));
            EMIT(og_pfadd(gc, R(mm2), R(mm4)));
            EMIT(og_pfadd(gc, R(mm3), R(mm5)));
            EMIT(og_movd(gc, R(mm0), MEMR(8, esi)));
            EMIT(og_punpckldq(gc, R(mm0), R(mm0)));
            EMIT(og_movq(gc, R(mm4), BASE(MX(2, 0), ecx)));
            EMIT(og_pfmul(gc, R(mm4), R(mm0)));
            EMIT(og_movq(gc, R(mm5), BASE(MX(2, 2), ecx)));
            EMIT(og_pfmul(gc, R(mm5), R(mm0)));
            EMIT(og_pfadd(gc, R(mm2), R(mm4)));
            EMIT(og_pfadd(gc, R(mm3), R(mm5)));
            EMIT(og_movq(gc, R(mm4), BASE(MX(3, 0), ecx)));
            EMIT(og_movq(gc, R(mm5), BASE(MX(3, 2), ecx)));
            EMIT(og_pfadd(gc, R(mm2), R(mm4)));
            EMIT(og_pfadd(gc, R(mm3), R(mm5)));
            EMIT(og_movq(gc, MEM(&blk->xyzw[0][0]), R(mm2)));
            EMIT(og_movq(gc, MEM(&blk->xyzw[0][2]), R(mm3)));
            EMIT(og_movd(gc, R(mm7), MEM(&blk->xyzw[0][3])));
            EMIT(og_pfrcp(gc, R(mm1), R(mm7)));
            EMIT(og_pfrcpit1(gc, R(mm7), R(mm1)));
            EMIT(og_pfrcpit2(gc, R(mm7), R(mm1)));
            EMIT(og_movd(gc, HWV(HW_W, edx), R(mm7)));
            if (gc->fx.fullscreen && gc->fx.guardBand) {
                EMIT(og_movd(gc, R(mm4), MEM(&blk->xyzw[0][3])));
                EMIT(og_punpckldq(gc, R(mm4), R(mm4)));
                EMIT(og_movd(gc, R(mm5), MEM(&blk->xyzw[0][3])));
                EMIT(og_punpckldq(gc, R(mm5), R(mm5)));
                EMIT(og_pfmul(gc, R(mm4), MEM(&blk->gb[0])));
                EMIT(og_pfmul(gc, R(mm5), MEM(&blk->gb[0][2])));
                EMIT(og_pxor(gc, R(mm3), R(mm3)));
                EMIT(og_pfmul(gc, R(mm5), MEM(&blk->negOne)));
                EMIT(og_movq(gc, R(mm2), MEM(&blk->xyzw[0][0])));
                EMIT(og_pfcmpgt(gc, R(mm2), R(mm4)));
                EMIT(og_pand(gc, R(mm2), MEM(&blk->clipBits3dn[0])));
                EMIT(og_por(gc, R(mm3), R(mm2)));
                EMIT(og_movq(gc, R(mm2), MEM(&blk->xyzw[0][0])));
                EMIT(og_pfcmpgt(gc, R(mm5), R(mm2)));
                EMIT(og_pand(gc, R(mm5), MEM(&blk->clipBits3dn[1])));
                EMIT(og_por(gc, R(mm3), R(mm5)));
                EMIT(og_movq(gc, R(mm4), R(mm3)));
                EMIT(og_punpckhdq(gc, R(mm4), R(mm4)));
                EMIT(og_por(gc, R(mm3), R(mm4)));
                EMIT(og_movd(gc, HWV(HW_GB, edx), R(mm3)));
            }
            if (ZCULL) {
                EMIT(og_movd(gc, R(mm4), MEM(&blk->xyzw[0][3])));
                EMIT(og_punpckldq(gc, R(mm4), R(mm4)));
                EMIT(og_movd(gc, R(mm5), MEM(&blk->xyzw[0][3])));
                EMIT(og_punpckldq(gc, R(mm5), R(mm5)));
                EMIT(og_pxor(gc, R(mm3), R(mm3)));
                EMIT(og_pfmul(gc, R(mm5), MEM(&blk->negOne)));
                EMIT(og_movq(gc, R(mm6), R(mm4)));
                EMIT(og_movq(gc, R(mm7), R(mm5)));
                EMIT(og_movq(gc, R(mm2), MEM(&blk->xyzw[0][0])));
                EMIT(og_pfcmpgt(gc, R(mm2), R(mm4)));
                EMIT(og_pand(gc, R(mm2), MEM(&blk->clipBits3dn[0])));
                EMIT(og_por(gc, R(mm3), R(mm2)));
                EMIT(og_movq(gc, R(mm2), MEM(&blk->xyzw[0][0])));
                EMIT(og_pfcmpgt(gc, R(mm5), R(mm2)));
                EMIT(og_pand(gc, R(mm5), MEM(&blk->clipBits3dn[1])));
                EMIT(og_por(gc, R(mm3), R(mm5)));
                EMIT(og_pfmul(gc, R(mm6), MEM(&blk->zFar)));
                EMIT(og_pfmul(gc, R(mm7), MEM(&blk->zNear)));
                EMIT(og_movd(gc, R(mm2), MEM(&blk->xyzw[0][2])));
                EMIT(og_punpckldq(gc, R(mm2), R(mm7)));
                EMIT(og_punpckldq(gc, R(mm6), R(mm2)));
                EMIT(og_pfcmpgt(gc, R(mm2), R(mm6)));
                EMIT(og_pand(gc, R(mm2), MEM(&blk->clipBits3dn[2])));
                EMIT(og_por(gc, R(mm3), R(mm2)));
                EMIT(og_movq(gc, R(mm4), R(mm3)));
                EMIT(og_punpckhdq(gc, R(mm4), R(mm4)));
                EMIT(og_por(gc, R(mm3), R(mm4)));
                EMIT(og_movd(gc, R(mm7), HWV(HW_W, edx)));
                EMIT(og_movd(gc, HWV(HW_CLIP, edx), R(mm3)));
            }
            else {
                /* the clip code from the sign bits and the |c| > |w| bits through the g_clip* tables (as sst_tmumem.c) */
                EMIT(og_mov(gc, R(ebp), MEM(&blk->xyzw[0][3])));
                EMIT(og_add(gc, R(ebp), R(ebp)));
                EMIT(og_sbb(gc, R(edi), R(edi)));
                EMIT(og_mov(gc, R(ecx), MEM(&blk->xyzw[0][2])));
                EMIT(og_mov(gc, R(ebx), MEM(&blk->xyzw[0][1])));
                EMIT(og_mov(gc, R(eax), MEM(&blk->xyzw[0][0])));
                EMIT(og_adc(gc, R(ecx), R(ecx)));
                EMIT(og_adc(gc, R(edi), R(edi)));
                EMIT(og_add(gc, R(ebx), R(ebx)));
                EMIT(og_adc(gc, R(edi), R(edi)));
                EMIT(og_add(gc, R(eax), R(eax)));
                EMIT(og_adc(gc, R(edi), R(edi)));
                EMIT(og_cmp(gc, R(ebp), R(ecx)));
                EMIT(og_adc(gc, R(ecx), R(ecx)));
                EMIT(og_cmp(gc, R(ebp), R(ebx)));
                EMIT(og_adc(gc, R(ecx), R(ecx)));
                EMIT(og_cmp(gc, R(ebp), R(eax)));
                EMIT(og_adc(gc, R(ecx), R(ecx)));
                EMIT(og_mov(gc, R(ebx), R(edi)));
                EMIT(og_and(gc, R(ebx), IMM(0xf)));
                EMIT(og_and(gc, R(ecx), IMM(0xf)));
                EMIT(og_mov(gc, R(ecx), TAB(g_clipAxisBits, ecx)));
                EMIT(og_mov(gc, R(edi), TAB(g_clipSideBits, ebx)));
                EMIT(og_and(gc, R(edi), R(ecx)));
                EMIT(og_mov(gc, R(ecx), TAB(g_clipWNegBits, ebx)));
                EMIT(og_or(gc, R(edi), R(ecx)));
                EMIT(og_mov(gc, HWV(HW_CLIP, edx), R(edi)));
            }
            EMIT(og_punpckldq(gc, R(mm7), R(mm7)));
            /* window = clip * viewport.scale / w + viewport.center (mm7 = 1/w; an unused fixup, as the original) */
            og_newFixup(gc);
            EMIT(og_movq(gc, R(mm0), MEM(&blk->xyzw[0][0])));
            EMIT(og_mov(gc, R(ecx), IMM(&gc->state.viewport)));
            EMIT(og_movd(gc, R(mm1), MEM(&blk->xyzw[0][2])));
            EMIT(og_movd(gc, R(mm3), MEMR(VP(scale.z), ecx)));
            EMIT(og_pfmul(gc, R(mm0), R(mm7)));
            EMIT(og_movq(gc, R(mm2), MEMR(VP(scale.x), ecx)));
            EMIT(og_movq(gc, R(mm4), MEMR(VP(center.x), ecx)));
            EMIT(og_pfmul(gc, R(mm1), R(mm7)));
            EMIT(og_movd(gc, R(mm5), MEMR(VP(center.z), ecx)));
            EMIT(og_pfmul(gc, R(mm0), R(mm2)));
            EMIT(og_pfmul(gc, R(mm1), R(mm3)));
            EMIT(og_pfadd(gc, R(mm0), R(mm4)));
            EMIT(og_pfadd(gc, R(mm1), R(mm5)));
            EMIT(og_movd(gc, HWV(HW_X, edx), R(mm0)));
            EMIT(og_punpckhdq(gc, R(mm0), R(mm0)));
            EMIT(og_movd(gc, HWV(HW_Y, edx), R(mm0)));
            EMIT(og_movd(gc, HWV(HW_Z, edx), R(mm1)));
            EMIT(og_add(gc, R(esi), MEM(&gc->vertexArray.vertex.stride)));
            EMIT(og_inc(gc, R(edx)));
            EMIT(og_cmp(gc, R(edx), MEM(&gc->vertexArray.lockCount)));
            EMIT(og_jl(gc, xformLoop));
        }
        else {
            /* x87: one vertex; window = clip * viewport.scale / w + viewport.center */
            EMIT(og_mov(gc, R(ecx), MEM(&gc->transform.modelView)));
            EMIT(og_add(gc, R(ecx), IMM(offsetof(struct __GLtransformICDRec, mvp))));
            EMIT(og_fld(gc, BASE(MX(3, 3), ecx)));
            EMIT(og_fld(gc, BASE(MX(3, 2), ecx)));
            EMIT(og_fld(gc, BASE(MX(3, 1), ecx)));
            EMIT(og_fld(gc, BASE(MX(3, 0), ecx)));
            EMIT(og_fld(gc, MEMR(8, esi)));
            EMIT(og_fmul(gc, BASE(MX(2, 3), ecx)));
            EMIT(og_fld(gc, MEMR(8, esi)));
            EMIT(og_fmul(gc, BASE(MX(2, 2), ecx)));
            EMIT(og_fld(gc, MEMR(8, esi)));
            EMIT(og_fmul(gc, BASE(MX(2, 1), ecx)));
            EMIT(og_fld(gc, MEMR(8, esi)));
            EMIT(og_fmul(gc, BASE(MX(2, 0), ecx)));
            EMIT(og_fxch(gc, R(st3)));
            EMIT(og_faddp(gc, R(st7), R(st0)));
            EMIT(og_faddp(gc, R(st4), R(st0)));
            EMIT(og_faddp(gc, R(st4), R(st0)));
            EMIT(og_faddp(gc, R(st1), R(st0)));
            EMIT(og_fld(gc, MEMR(4, esi)));
            EMIT(og_fmul(gc, BASE(MX(1, 3), ecx)));
            EMIT(og_fld(gc, MEMR(4, esi)));
            EMIT(og_fmul(gc, BASE(MX(1, 2), ecx)));
            EMIT(og_fld(gc, MEMR(4, esi)));
            EMIT(og_fmul(gc, BASE(MX(1, 1), ecx)));
            EMIT(og_fld(gc, MEMR(4, esi)));
            EMIT(og_fmul(gc, BASE(MX(1, 0), ecx)));
            EMIT(og_fxch(gc, R(st3)));
            EMIT(og_faddp(gc, R(st7), R(st0)));
            EMIT(og_faddp(gc, R(st4), R(st0)));
            EMIT(og_faddp(gc, R(st4), R(st0)));
            EMIT(og_faddp(gc, R(st1), R(st0)));
            EMIT(og_fld(gc, MEM0(esi)));
            EMIT(og_fmul(gc, BASE(MX(0, 3), ecx)));
            EMIT(og_fld(gc, MEM0(esi)));
            EMIT(og_fmul(gc, BASE(MX(0, 2), ecx)));
            EMIT(og_fld(gc, MEM0(esi)));
            EMIT(og_fmul(gc, BASE(MX(0, 1), ecx)));
            EMIT(og_fld(gc, MEM0(esi)));
            EMIT(og_fmul(gc, BASE(MX(0, 0), ecx)));
            EMIT(og_fxch(gc, R(st3)));
            EMIT(og_faddp(gc, R(st7), R(st0)));
            EMIT(og_faddp(gc, R(st4), R(st0)));
            EMIT(og_faddp(gc, R(st4), R(st0)));
            EMIT(og_faddp(gc, R(st1), R(st0)));
            EMIT(og_fxch(gc, R(st3)));
            EMIT(og_fstp(gc, MEM(&blk->xyzw[0][3])));
            EMIT(og_fstp(gc, MEM(&blk->xyzw[0][1])));
            EMIT(og_fstp(gc, MEM(&blk->xyzw[0][2])));
            EMIT(og_fstp(gc, MEM(&blk->xyzw[0][0])));
            EMIT(og_mov(gc, R(ebp), MEM(&blk->xyzw[0][3])));
            EMIT(og_add(gc, R(ebp), R(ebp)));
            EMIT(og_sbb(gc, R(edi), R(edi)));
            EMIT(og_mov(gc, R(ecx), MEM(&blk->xyzw[0][2])));
            EMIT(og_mov(gc, R(ebx), MEM(&blk->xyzw[0][1])));
            EMIT(og_mov(gc, R(eax), MEM(&blk->xyzw[0][0])));
            EMIT(og_adc(gc, R(ecx), R(ecx)));
            EMIT(og_adc(gc, R(edi), R(edi)));
            EMIT(og_add(gc, R(ebx), R(ebx)));
            EMIT(og_adc(gc, R(edi), R(edi)));
            EMIT(og_add(gc, R(eax), R(eax)));
            EMIT(og_adc(gc, R(edi), R(edi)));
            EMIT(og_cmp(gc, R(ebp), R(ecx)));
            EMIT(og_adc(gc, R(ecx), R(ecx)));
            EMIT(og_cmp(gc, R(ebp), R(ebx)));
            EMIT(og_adc(gc, R(ecx), R(ecx)));
            EMIT(og_cmp(gc, R(ebp), R(eax)));
            EMIT(og_adc(gc, R(ecx), R(ecx)));
            EMIT(og_mov(gc, R(ebx), R(edi)));
            EMIT(og_and(gc, R(ebx), IMM(0xf)));
            EMIT(og_and(gc, R(ecx), IMM(0xf)));
            EMIT(og_mov(gc, R(ecx), TAB(g_clipAxisBits, ecx)));
            EMIT(og_mov(gc, R(edi), TAB(g_clipSideBits, ebx)));
            EMIT(og_and(gc, R(edi), R(ecx)));
            EMIT(og_mov(gc, R(ecx), TAB(g_clipWNegBits, ebx)));
            EMIT(og_or(gc, R(edi), R(ecx)));
            EMIT(og_mov(gc, HWV(HW_CLIP, edx), R(edi)));
            EMIT(og_fld(gc, MEM(&blk->xyzw[0][3])));
            EMIT(og_fdivr(gc, W_FCONST(gc, 1.0)));
            EMIT(og_fstp(gc, HWV(HW_W, edx)));
            EMIT(og_fld(gc, MEM(&blk->xyzw[0][0])));
            EMIT(og_mov(gc, R(ecx), IMM(&gc->state.viewport)));
            EMIT(og_fmul(gc, MEMR(VP(scale.x), ecx)));
            EMIT(og_fld(gc, MEM(&blk->xyzw[0][1])));
            EMIT(og_fmul(gc, MEMR(VP(scale.y), ecx)));
            EMIT(og_fld(gc, MEM(&blk->xyzw[0][2])));
            EMIT(og_fmul(gc, MEMR(VP(scale.z), ecx)));
            EMIT(og_fxch(gc, R(st2)));
            EMIT(og_fmul(gc, HWV(HW_W, edx)));
            EMIT(og_fld(gc, HWV(HW_W, edx)));
            EMIT(og_fmulp(gc, R(st2), R(st0)));
            EMIT(og_fld(gc, HWV(HW_W, edx)));
            EMIT(og_fmulp(gc, R(st3), R(st0)));
            EMIT(og_fadd(gc, MEMR(VP(center.x), ecx)));
            EMIT(og_fxch(gc, R(st1)));
            EMIT(og_fadd(gc, MEMR(VP(center.y), ecx)));
            EMIT(og_fxch(gc, R(st2)));
            EMIT(og_fadd(gc, MEMR(VP(center.z), ecx)));
            EMIT(og_fxch(gc, R(st1)));
            EMIT(og_fstp(gc, HWV(HW_X, edx)));
            EMIT(og_fstp(gc, HWV(HW_Z, edx)));
            EMIT(og_fstp(gc, HWV(HW_Y, edx)));
            EMIT(og_add(gc, R(esi), MEM(&gc->vertexArray.vertex.stride)));
            EMIT(og_inc(gc, R(edx)));
            EMIT(og_cmp(gc, R(edx), MEM(&gc->vertexArray.lockCount)));
            EMIT(og_jl(gc, xformLoop));
        }
    }
    /* the transform is current */
    EMIT(og_mov(gc, R(eax), MEM(&gc->vertexArray.flags)));
    EMIT(og_and(gc, R(eax), IMM(~__GL_VERTARRAY_CHANGED)));
    EMIT(og_mov(gc, MEM(&gc->vertexArray.flags), R(eax)));
    LABEL(xformDone);
    /* 2. the triangles: esi = the index list, edi = the FIFO pointer, ebp = 2 (new strip); SSE: xmm7 = the
       TMU texture scales (with two TMUs unit 1 low, unit 0 high) */
    EMIT(og_mov(gc, R(esi), MEM(&blk->indices)));

    EMIT(og_mov(gc, R(edi), MEM(&fifo->fifoPtr)));
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        if (gc->fx.numTMUs == 1) {
            EMIT(og_movlps(gc, R(mm7), MEM(&gc->hwcx.texScale[0][0])));
        }
        else {
            EMIT(og_movlps(gc, R(mm7), MEM(&gc->hwcx.texScale[1][0])));
            EMIT(og_movhps(gc, R(mm7), MEM(&gc->hwcx.texScale[0][0])));
        }
    }
    EMIT(og_mov(gc, R(ebp), IMM(2)));
    LABEL(triLoop);
    /* eax, ebx, ecx = the vertex numbers: rejected when their clip codes share a bit or (z cull) one is
       beyond the far plane, or (super mode) the near plane; near / far clipped ones through the clipper */
    EMIT(og_mov(gc, R(eax), MEM0(esi)));
    EMIT(og_mov(gc, R(ebx), MEMR(4, esi)));
    EMIT(og_mov(gc, R(ecx), MEMR(8, esi)));
    EMIT(og_mov(gc, R(edx), HWV(HW_CLIP, eax)));
    EMIT(og_and(gc, R(edx), HWV(HW_CLIP, ebx)));
    EMIT(og_and(gc, R(edx), HWV(HW_CLIP, ecx)));
    EMIT(og_jne(gc, nextTri));
    EMIT(og_mov(gc, R(edx), HWV(HW_CLIP, eax)));
    EMIT(og_or(gc, R(edx), HWV(HW_CLIP, ebx)));
    EMIT(og_or(gc, R(edx), HWV(HW_CLIP, ecx)));
    if (gc->fx.fullscreen) {
        if (gc->fx.zcullEnable && gc->state.viewport.zFar == 1.0 && gc->state.viewport.zNear == 0.0) {
            EMIT(og_test(gc, R(edx), IMM(__GL_CLIP_FAR)));
            EMIT(og_jne(gc, nextTri));
        }
        if (ZCULL && gc->fx.superMode) {
            EMIT(og_test(gc, R(edx), IMM(__GL_CLIP_NEAR)));
            EMIT(og_jne(gc, nextTri));
        }
    }
    EMIT(og_test(gc, R(edx), IMM(__GL_CLIP_NEAR | __GL_CLIP_FAR)));
    EMIT(og_jne(gc, clipTri));
    /* face culling: the sign of the area picks face[0 / 1]; culled when it is cullFace */
    if (gc->state.enables.general & __GL_CULL_FACE_ENABLE) {
        if (gc->constants.cpuCaps & __GL_CPU_SSE) {
            EMIT(og_movss(gc, R(mm0), HWV(HW_X, eax)));
            EMIT(og_movss(gc, R(mm1), HWV(HW_X, ebx)));
            EMIT(og_movss(gc, R(mm2), HWV(HW_X, ecx)));
            EMIT(og_movss(gc, R(mm3), HWV(HW_Y, eax)));
            EMIT(og_movss(gc, R(mm4), HWV(HW_Y, ebx)));
            EMIT(og_movss(gc, R(mm5), HWV(HW_Y, ecx)));
            EMIT(og_subss(gc, R(mm0), R(mm2)));
            EMIT(og_subss(gc, R(mm1), R(mm2)));
            EMIT(og_subss(gc, R(mm3), R(mm5)));
            EMIT(og_subss(gc, R(mm4), R(mm5)));
            EMIT(og_mulss(gc, R(mm0), R(mm4)));
            EMIT(og_mulss(gc, R(mm1), R(mm3)));
            EMIT(og_subss(gc, R(mm0), R(mm1)));
            EMIT(og_movss(gc, MEM(&blk->area), R(mm0)));
        }
        else {
            if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
                EMIT(og_movd(gc, R(mm0), HWV(HW_X, eax)));
                EMIT(og_movd(gc, R(mm1), HWV(HW_X, ebx)));
                EMIT(og_movd(gc, R(mm2), HWV(HW_X, ecx)));
                EMIT(og_movd(gc, R(mm3), HWV(HW_Y, eax)));
                EMIT(og_movd(gc, R(mm4), HWV(HW_Y, ebx)));
                EMIT(og_movd(gc, R(mm5), HWV(HW_Y, ecx)));
                EMIT(og_pfsub(gc, R(mm0), R(mm2)));
                EMIT(og_pfsub(gc, R(mm1), R(mm2)));
                EMIT(og_pfsub(gc, R(mm3), R(mm5)));
                EMIT(og_pfsub(gc, R(mm4), R(mm5)));
                EMIT(og_pfmul(gc, R(mm0), R(mm4)));
                EMIT(og_pfmul(gc, R(mm1), R(mm3)));
                EMIT(og_pfsub(gc, R(mm0), R(mm1)));
                EMIT(og_movd(gc, MEM(&blk->area), R(mm0)));
            }
            else {
                EMIT(og_fld(gc, HWV(HW_X, eax)));
                EMIT(og_fsub(gc, HWV(HW_X, ecx)));
                EMIT(og_fld(gc, HWV(HW_X, ebx)));
                EMIT(og_fsub(gc, HWV(HW_X, ecx)));
                EMIT(og_fld(gc, HWV(HW_Y, eax)));
                EMIT(og_fsub(gc, HWV(HW_Y, ecx)));
                EMIT(og_fld(gc, HWV(HW_Y, ebx)));
                EMIT(og_fsub(gc, HWV(HW_Y, ecx)));
                EMIT(og_fmulp(gc, R(st3), R(st0)));
                EMIT(og_fmulp(gc, R(st1), R(st0)));
                EMIT(og_fsubp(gc, R(st1), R(st0)));
                EMIT(og_fstp(gc, MEM(&blk->area)));
            }
        }
        EMIT(og_mov(gc, MEM(&blk->xyzw[0][0]), R(eax)));
        EMIT(og_mov(gc, MEM(&blk->xyzw[0][1]), R(ebx)));
        EMIT(og_mov(gc, R(eax), MEM(&blk->area)));
                EMIT(og_test(gc, R(eax), IMM(0x7fffffff)));           /* a zero-area triangle draws nothing: dropped, as Glide's own */
        EMIT(og_je(gc, nextTri));
EMIT(og_shr(gc, R(eax), IMM(0x1f)));
        EMIT(og_xor(gc, R(eax), IMM(1)));
        EMIT(og_movzx8(gc, R(al), MEMR(&gc->polygon.face[0], eax)));
        EMIT(og_movzx8(gc, R(bl), MEM(&gc->polygon.cullFace)));
        EMIT(og_cmp(gc, R(al), R(bl)));
        EMIT(og_mov(gc, R(eax), MEM(&blk->xyzw[0][0])));
        EMIT(og_mov(gc, R(ebx), MEM(&blk->xyzw[0][1])));
        EMIT(og_je(gc, nextTri));
    }
    /* through the clipper: any clip bit, or with the guard band (SSE / 3DNow) only a guard-band bit */
    if (!gc->fx.fullscreen || !gc->fx.guardBand || !(gc->constants.cpuCaps & (__GL_CPU_SSE | __GL_CPU_3DNOW))) {
        EMIT(og_test(gc, R(edx), R(edx)));
        EMIT(og_jne(gc, clipTri));
    }
    else {
        EMIT(og_mov(gc, MEM(&blk->xyzw[0][0]), R(eax)));
        EMIT(og_mov(gc, R(edx), HWV(HW_CLIP, eax)));
        EMIT(og_and(gc, R(edx), HWV(HW_GB, eax)));
        EMIT(og_mov(gc, R(eax), HWV(HW_CLIP, ebx)));
        EMIT(og_and(gc, R(eax), HWV(HW_GB, ebx)));
        EMIT(og_or(gc, R(edx), R(eax)));
        EMIT(og_mov(gc, R(eax), HWV(HW_CLIP, ecx)));
        EMIT(og_and(gc, R(eax), HWV(HW_GB, ecx)));
        EMIT(og_or(gc, R(edx), R(eax)));
        EMIT(og_mov(gc, R(eax), MEM(&blk->xyzw[0][0])));
        EMIT(og_test(gc, R(edx), IMM(__GL_CLIP_LEFT | __GL_CLIP_RIGHT | __GL_CLIP_BOTTOM | __GL_CLIP_TOP)));
        EMIT(og_jne(gc, clipTri));
    }
    /* polygon offset: genProcs9[1] writes the depth bias of the indexed triangle (edx = area) */
    if (gc->state.enables.general & __GL_POLYGON_OFFSET_ANY_ENABLE) {
        EMIT(og_mov(gc, MEM(&fifo->fifoPtr), R(edi)));
        EMIT(og_mov(gc, R(edx), MEM(&blk->area)));
        EMIT(og_call(gc, MEM(&gc->hwcx.genProcs9[1])));
        EMIT(og_mov(gc, R(edi), MEM(&fifo->fifoPtr)));
    }
    /* strip test: ebp & 1 = the parity; the triangle continues the strip when (eax, ebx) is the last
       one's (ecx, ebx) (even) or (eax, ecx) (odd), else ebp |= 2 */
    EMIT(og_mov(gc, R(edx), R(ebp)));
    EMIT(og_and(gc, R(edx), IMM(1)));
    EMIT(og_xor(gc, R(eax), TAB(&blk->prev[1], edx)));
    EMIT(og_xor(gc, R(ebx), TAB(&blk->prev[0], edx)));
    EMIT(og_or(gc, R(eax), R(ebx)));
    EMIT(og_add(gc, R(eax), IMM(-1)));
    EMIT(og_sbb(gc, R(eax), R(eax)));
    EMIT(og_and(gc, R(eax), IMM(2)));
    EMIT(og_or(gc, R(ebp), R(eax)));
    if (gc->state.enables.general & __GL_POLYGON_OFFSET_ANY_ENABLE)
        EMIT(og_or(gc, R(ebp), IMM(2)));                                /* offset in the vertices' z: no strips */
    EMIT(og_mov(gc, R(eax), MEM0(esi)));
    EMIT(og_mov(gc, R(ebx), MEMR(4, esi)));
    EMIT(og_mov(gc, MEM(&blk->prev[2]), R(eax)));
    EMIT(og_mov(gc, MEM(&blk->prev[0]), R(ebx)));
    EMIT(og_mov(gc, MEM(&blk->prev[1]), R(ecx)));
    EMIT(og_test(gc, R(ebp), IMM(2)));
    EMIT(og_je(gc, sendStrip));
    LABEL(sendTri);
    /* a new packet: header (packet 3, three vertices; vflags << 10 = the parameter mask) and the vertices
       (x, y, ARGB [, z], q [, s, t per TMU]; with two TMUs unit 0 is ST1 (0x80), unit 1 ST0 (0x20)) */
    vflags = 3;
    sz = 0xc;
    if (gc->state.enables.general & __GL_DEPTH_TEST_ENABLE) {
        vflags = 7;
        sz = 0x10;
    }
    if (gc->fx.numTMUs == 1) {
        if (gc->texture.textureEnabled[0]) {
            vflags |= 0x20;
            sz += 8;
        }
    }
    else {
        if (gc->texture.textureEnabled[0]) {
            vflags |= 0x80;
            sz += 8;
        }
        if (gc->texture.textureEnabled[1]) {
            vflags |= 0x20;
            sz += 8;
        }
    }
    sz += 4;
    sz3 = sz * 3 + 4;                     /* header + three vertices */
    vflags |= 8;                          /* q */
    pc = og_fifoMakeRoom(gc, pc, IMM(sz3), makeRoom);
    EMIT(og_mov(gc, R(edx), IMM(vflags << 10 | 0x100000c3)));
    EMIT(og_mov(gc, MEM0(edi), R(edx)));
    regCount = 4;                         /* the vertices follow the header */
    pc = hwCopyArrayVertex(gc, pc, R(eax), &regCount, vflags);
    pc = hwCopyArrayVertex(gc, pc, R(ebx), &regCount, vflags);
    pc = hwCopyArrayVertex(gc, pc, R(ecx), &regCount, vflags);
    EMIT(og_add(gc, R(edi), IMM(sz3)));
    EMIT(og_mov(gc, R(ebp), IMM(0)));
    EMIT(og_jmp(gc, nextTri));
    /* a strip continuation: the header and ecx; the parity flips */
    LABEL(sendStrip);
    pc = og_fifoMakeRoom(gc, pc, IMM(sz + 4), makeRoom);
    EMIT(og_mov(gc, R(edx), IMM(vflags << 10 | 0x10000053)));
    EMIT(og_mov(gc, MEM0(edi), R(edx)));
    regCount = 4;
    pc = hwCopyArrayVertex(gc, pc, R(ecx), &regCount, vflags);
    EMIT(og_add(gc, R(edi), IMM(sz + 4)));
    EMIT(og_xor(gc, R(ebp), IMM(1)));
    LABEL(nextTri);
    /* the next triangle; after the last: store the FIFO pointer, reset the depth bias (genProcs9[2]),
       restore the registers, leave the MMX state */
    EMIT(og_add(gc, R(esi), IMM(0xc)));
    EMIT(og_mov(gc, R(eax), MEM(&blk->last)));
    EMIT(og_cmp(gc, R(esi), R(eax)));
    EMIT(og_jb(gc, triLoop));
    EMIT(og_mov(gc, MEM(&fifo->fifoPtr), R(edi)));
    if (gc->state.enables.general & __GL_POLYGON_OFFSET_ANY_ENABLE) {
        EMIT(og_call(gc, MEM(&gc->hwcx.genProcs9[2])));
    }
    EMIT(og_pop(gc, R(esi)));
    EMIT(og_pop(gc, R(edi)));
    EMIT(og_pop(gc, R(ebp)));
    EMIT(og_pop(gc, R(ebx)));
    if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
        EMIT(og_femms(gc));
    }
    else if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        EMIT(og_emms(gc));
    }
    EMIT(og_ret_imm(gc, IMM(0)));
    /* the clipper: the three vertices to the staging vertices, then genCode[CLIP_TRIANGLE](gc, 0x15, v0, v1,
       v2) writes the FIFO itself; 3DNow pads the FIFO pointer to 8 bytes after it; then a new strip */
    LABEL(clipTri);
    EMIT(og_mov(gc, MEM(&blk->tri[0]), R(eax)));
    EMIT(og_mov(gc, MEM(&blk->tri[1]), R(ebx)));
    EMIT(og_mov(gc, MEM(&blk->tri[2]), R(ecx)));
    EMIT(og_mov(gc, R(eax), IMM(gc->hwcx.stagingBase)));
    EMIT(og_mov(gc, R(edx), MEM(&blk->tri[0])));
    EMIT(og_call(gc, toStaging));
    EMIT(og_mov(gc, R(eax), IMM(&gc->hwcx.stagingBase[1])));
    EMIT(og_mov(gc, R(edx), MEM(&blk->tri[1])));
    EMIT(og_call(gc, toStaging));
    EMIT(og_mov(gc, R(eax), IMM(&gc->hwcx.stagingBase[2])));
    EMIT(og_mov(gc, R(edx), MEM(&blk->tri[2])));
    EMIT(og_call(gc, toStaging));
    EMIT(og_mov(gc, MEM(&fifo->fifoPtr), R(edi)));
    EMIT(og_push(gc, IMM(&gc->hwcx.stagingBase[2])));
    EMIT(og_push(gc, IMM(&gc->hwcx.stagingBase[1])));
    EMIT(og_push(gc, IMM(gc->hwcx.stagingBase)));
    EMIT(og_mov(gc, R(edx), IMM(0x15)));
    EMIT(og_mov(gc, R(ecx), IMM(gc)));
    pc = og_callRel32Locked(gc, pc, gc->hwcx.genCode[__GL_GEN_CLIP_TRIANGLE]);
    EMIT(og_mov(gc, R(edi), MEM(&fifo->fifoPtr)));
    if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
        skip = og_newLabel(gc);
        EMIT(og_test(gc, R(edi), IMM(4)));
        EMIT(og_je(gc, skip));
        EMIT(og_call(gc, fifoPad));
        LABEL(skip);
    }
    EMIT(og_mov(gc, R(ebp), IMM(2)));
    EMIT(og_jmp(gc, nextTri));
    /* subroutine: a 0 word (packet-0 nop) into the FIFO */
    LABEL(fifoPad);
    roomOk = og_newLabel(gc);
    EMIT(og_mov(gc, R(edx), MEM(&fifo->fifoRoom)));
    EMIT(og_sub(gc, R(edx), IMM(4)));
    EMIT(og_jge(gc, roomOk));
    EMIT(og_mov(gc, R(edx), IMM(4)));
    EMIT(og_call(gc, makeRoom));
    LABEL(roomOk);
    EMIT(og_mov(gc, MEM(&fifo->fifoRoom), R(edx)));
    EMIT(og_mov(gc, MEM0(edi), IMM(0)));
    EMIT(og_add(gc, R(edi), IMM(4)));
    EMIT(og_ret(gc));
    /* subroutine: wait for edx bytes of FIFO; returns edx = fifoRoom - size; a new strip after it (ebp = 2) */
    LABEL(makeRoom);
    EMIT(og_mov(gc, MEM(&fifo->fifoPtr), R(edi)));
    EMIT(og_push(gc, R(eax)));
    EMIT(og_push(gc, R(ecx)));
    EMIT(og_push(gc, R(edx)));
    EMIT(og_push(gc, IMM(0)));
    EMIT(og_push(gc, IMM(0)));
    EMIT(og_push(gc, R(edx)));
    EMIT(og_call(gc, MEM(&gc->hwcx.grCommandTransportMakeRoomExt2)));
    EMIT(og_pop(gc, R(eax)));
    EMIT(og_mov(gc, R(edx), MEM(&fifo->fifoRoom)));
    EMIT(og_sub(gc, R(edx), R(eax)));
    EMIT(og_pop(gc, R(ecx)));
    EMIT(og_pop(gc, R(eax)));
    EMIT(og_mov(gc, R(edi), MEM(&fifo->fifoPtr)));
    EMIT(og_mov(gc, R(ebp), IMM(2)));
    EMIT(og_ret(gc));
    /* subroutine: hardware vertex edx -> the staging __GLvertexICD at eax: window, packedColor, colors[0],
       clip, flagBits and the texture coordinates */
    LABEL(toStaging);
    EMIT(og_mov(gc, R(ebx), HWV(HW_X, edx)));
    EMIT(og_mov(gc, MEMR(VX(window.x), eax), R(ebx)));
    EMIT(og_mov(gc, R(ebx), HWV(HW_Y, edx)));
    EMIT(og_mov(gc, MEMR(VX(window.y), eax), R(ebx)));
    EMIT(og_mov(gc, R(ebx), HWV(HW_Z, edx)));
    EMIT(og_mov(gc, MEMR(VX(window.z), eax), R(ebx)));
    EMIT(og_mov(gc, R(ebx), HWV(HW_W, edx)));
    EMIT(og_mov(gc, MEMR(VX(window.w), eax), R(ebx)));
    /* packedColor (ARGB) and colors[0] (0..255): the colour array (RGBA bytes, R and B swapped for ARGB) or
       the current colour (ARGB, swapped back to RGBA for the bytes) */
    if (gc->vertexArray.enableFlags & __GL_VA_COLOR_ENABLE) {
        EMIT(og_mov(gc, R(ecx), MEM(&gc->vertexArray.color.pointer)));
        EMIT(og_mov(gc, R(ebx), ELT(ecx, edx, 4)));
        EMIT(og_mov(gc, R(ebp), R(ebx)));
        EMIT(og_and(gc, R(ebp), IMM(0xff00ff)));
        EMIT(og_and(gc, R(ebx), IMM(0xff00ff00)));
        EMIT(og_rol(gc, R(ebp), IMM(0x10)));
        EMIT(og_or(gc, R(ebx), R(ebp)));
        EMIT(og_mov(gc, MEMR(VX(packedColor), eax), R(ebx)));
        if (!(gc->constants.cpuCaps & (__GL_CPU_SSE | __GL_CPU_3DNOW))) {
            EMIT(og_mov(gc, R(ebx), ELT(ecx, edx, 4)));
            EMIT(og_mov(gc, MEM(&blk->xyzw[0][0]), R(ebx)));
        }
        else {
            EMIT(og_movd(gc, R(mm0), ELT(ecx, edx, 4)));
        }
    }
    else {
        EMIT(og_mov(gc, R(ebx), MEM(&gc->state.current.packedColor)));
        EMIT(og_mov(gc, MEMR(VX(packedColor), eax), R(ebx)));
        EMIT(og_mov(gc, R(ebp), R(ebx)));
        EMIT(og_and(gc, R(ebp), IMM(0xff00ff)));
        EMIT(og_and(gc, R(ebx), IMM(0xff00ff00)));
        EMIT(og_rol(gc, R(ebp), IMM(0x10)));
        EMIT(og_or(gc, R(ebx), R(ebp)));
        if (!(gc->constants.cpuCaps & (__GL_CPU_SSE | __GL_CPU_3DNOW))) {
            EMIT(og_mov(gc, MEM(&blk->xyzw[0][0]), R(ebx)));
        }
        else {
            EMIT(og_movd(gc, R(mm0), R(ebx)));
        }
    }
    /* the channel bytes as ints: blk->xyzw[0] (x87) or mm0 = r, g / mm1 = b, a (MMX); then colors[0] */
    if (!(gc->constants.cpuCaps & (__GL_CPU_SSE | __GL_CPU_3DNOW))) {
        EMIT(og_mov(gc, R(ebx), MEM(&blk->xyzw[0][0])));
        EMIT(og_mov(gc, R(ebp), R(ebx)));
        EMIT(og_and(gc, R(ebp), IMM(0xff)));
        EMIT(og_mov(gc, MEM(&blk->xyzw[0][0]), R(ebp)));
        EMIT(og_mov(gc, R(ebp), R(ebx)));
        EMIT(og_shr(gc, R(ebp), IMM(8)));
        EMIT(og_and(gc, R(ebp), IMM(0xff)));
        EMIT(og_mov(gc, MEM(&blk->xyzw[0][1]), R(ebp)));
        EMIT(og_mov(gc, R(ebp), R(ebx)));
        EMIT(og_shr(gc, R(ebp), IMM(0x10)));
        EMIT(og_and(gc, R(ebp), IMM(0xff)));
        EMIT(og_mov(gc, MEM(&blk->xyzw[0][2]), R(ebp)));
        EMIT(og_mov(gc, R(ebp), R(ebx)));
        EMIT(og_shr(gc, R(ebp), IMM(0x18)));
        EMIT(og_and(gc, R(ebp), IMM(0xff)));
        EMIT(og_mov(gc, MEM(&blk->xyzw[0][3]), R(ebp)));
    }
    else {
        EMIT(og_pxor(gc, R(mm4), R(mm4)));
        EMIT(og_punpcklbw(gc, R(mm0), R(mm4)));
        EMIT(og_movq(gc, R(mm1), R(mm0)));
        EMIT(og_punpcklwd(gc, R(mm0), R(mm4)));
        EMIT(og_punpckhwd(gc, R(mm1), R(mm4)));
    }
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        EMIT(og_movq(gc, MEM(&blk->xyzw[0][0]), R(mm0)));
        EMIT(og_movq(gc, MEM(&blk->xyzw[0][2]), R(mm1)));
        EMIT(og_cvtpi2ps(gc, R(mm0), MEM(&blk->xyzw[0][0])));
        EMIT(og_cvtpi2ps(gc, R(mm1), MEM(&blk->xyzw[0][2])));
        EMIT(og_shufps(gc, R(mm0), R(mm1), IMM(0x44)));
        EMIT(og_movaps(gc, MEMR(VX(colors[0].r), eax), R(mm0)));
    }
    else {
        if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
            EMIT(og_pi2fd(gc, R(mm0), R(mm0)));
            EMIT(og_pi2fd(gc, R(mm1), R(mm1)));
            EMIT(og_movq(gc, MEMR(VX(colors[0].r), eax), R(mm0)));
            EMIT(og_movq(gc, MEMR(VX(colors[0].b), eax), R(mm1)));
        }
        else {
            EMIT(og_fild(gc, MEM(&blk->xyzw[0][0])));
            EMIT(og_fstp(gc, MEMR(VX(colors[0].r), eax)));
            EMIT(og_fild(gc, MEM(&blk->xyzw[0][1])));
            EMIT(og_fstp(gc, MEMR(VX(colors[0].g), eax)));
            EMIT(og_fild(gc, MEM(&blk->xyzw[0][2])));
            EMIT(og_fstp(gc, MEMR(VX(colors[0].b), eax)));
            EMIT(og_fild(gc, MEM(&blk->xyzw[0][3])));
            EMIT(og_fstp(gc, MEMR(VX(colors[0].a), eax)));
        }
    }
    /* clip = mvp * the array vertex (ebp = its address), flagBits = its clip code; the enabled units' s, t
       scaled by the TMU range and, unless __GL_CLIP_NEAR, by window.w (1/w) */
    va = &gc->vertexArray;
    EMIT(og_mov(gc, R(ecx), MEM(&va->vertex.pointer)));
    EMIT(og_mov(gc, R(ebp), R(edx)));
    EMIT(og_imul(gc, R(ebp), MEM(&gc->vertexArray.vertex.stride)));
    EMIT(og_add(gc, R(ebp), R(ecx)));
    EMIT(og_mov(gc, R(ecx), MEM(&gc->transform.modelView)));
    EMIT(og_add(gc, R(ecx), IMM(offsetof(struct __GLtransformICDRec, mvp))));
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        EMIT(og_movups(gc, R(mm0), BASE(MX(0, 0), ecx)));
        EMIT(og_movss(gc, R(mm1), MEMR(0, ebp)));
        EMIT(og_movups(gc, R(mm2), BASE(MX(1, 0), ecx)));
        EMIT(og_movss(gc, R(mm3), MEMR(4, ebp)));
        EMIT(og_shufps(gc, R(mm1), R(mm1), IMM(0)));
        EMIT(og_movups(gc, R(mm4), BASE(MX(2, 0), ecx)));
        EMIT(og_shufps(gc, R(mm3), R(mm3), IMM(0)));
        EMIT(og_movss(gc, R(mm5), MEMR(8, ebp)));
        EMIT(og_mulps(gc, R(mm1), R(mm0)));
        EMIT(og_mulps(gc, R(mm3), R(mm2)));
        EMIT(og_shufps(gc, R(mm5), R(mm5), IMM(0)));
        EMIT(og_addps(gc, R(mm1), R(mm3)));
        EMIT(og_movups(gc, R(mm6), BASE(MX(3, 0), ecx)));
        EMIT(og_mulps(gc, R(mm5), R(mm4)));
        EMIT(og_addps(gc, R(mm5), R(mm6)));
        EMIT(og_addps(gc, R(mm1), R(mm5)));
        EMIT(og_movaps(gc, MEMR(VX(clip.x), eax), R(mm1)));
        if (gc->fx.numTMUs == 1) {
            if (gc->texture.textureEnabled[0]) {
                EMIT(og_mov(gc, R(ecx), MEM(&va->texture.pointer[0])));
                EMIT(og_movlps(gc, R(mm0), ELT(ecx, edx, 8)));
            }
        }
        else {
            if (gc->texture.textureEnabled[0]) {
                EMIT(og_mov(gc, R(ecx), MEM(&va->texture.pointer[0])));
                EMIT(og_movhps(gc, R(mm0), ELT(ecx, edx, 8)));
            }
            if (gc->texture.textureEnabled[1]) {
                EMIT(og_mov(gc, R(ecx), MEM(&va->texture.pointer[1])));
                EMIT(og_movlps(gc, R(mm0), ELT(ecx, edx, 8)));
            }
        }
        EMIT(og_mulps(gc, R(mm0), R(mm7)));
        EMIT(og_mov(gc, R(ebx), HWV(HW_CLIP, edx)));
        EMIT(og_mov(gc, MEMR(VX(flagBits), eax), R(ebx)));
        skip = og_newLabel(gc);
        EMIT(og_test(gc, R(ebx), IMM(__GL_CLIP_NEAR)));
        EMIT(og_jne(gc, skip));
        EMIT(og_movss(gc, R(mm1), MEMR(VX(window.w), eax)));
        EMIT(og_shufps(gc, R(mm1), R(mm1), IMM(0)));
        EMIT(og_mulps(gc, R(mm0), R(mm1)));
        LABEL(skip);
        if (gc->fx.numTMUs == 1) {
            EMIT(og_movlps(gc, MEMR(VX(texture[0].x), eax), R(mm0)));
        }
        else {
            EMIT(og_movhps(gc, MEMR(VX(texture[0].x), eax), R(mm0)));
            EMIT(og_movlps(gc, MEMR(VX(texture[1].x), eax), R(mm0)));
        }
    }
    else {
        if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
            EMIT(og_movq(gc, R(mm0), MEMR(0, ebp)));
            EMIT(og_movq(gc, R(mm1), R(mm0)));
            EMIT(og_punpckldq(gc, R(mm0), R(mm0)));
            EMIT(og_punpckhdq(gc, R(mm1), R(mm1)));
            EMIT(og_movq(gc, R(mm2), BASE(MX(0, 0), ecx)));
            EMIT(og_pfmul(gc, R(mm2), R(mm0)));
            EMIT(og_movq(gc, R(mm3), BASE(MX(0, 2), ecx)));
            EMIT(og_pfmul(gc, R(mm3), R(mm0)));
            EMIT(og_movq(gc, R(mm4), BASE(MX(1, 0), ecx)));
            EMIT(og_pfmul(gc, R(mm4), R(mm1)));
            EMIT(og_movq(gc, R(mm5), BASE(MX(1, 2), ecx)));
            EMIT(og_pfmul(gc, R(mm5), R(mm1)));
            EMIT(og_pfadd(gc, R(mm2), R(mm4)));
            EMIT(og_pfadd(gc, R(mm3), R(mm5)));
            EMIT(og_movd(gc, R(mm0), MEMR(8, ebp)));
            EMIT(og_punpckldq(gc, R(mm0), R(mm0)));
            EMIT(og_movq(gc, R(mm4), BASE(MX(2, 0), ecx)));
            EMIT(og_pfmul(gc, R(mm4), R(mm0)));
            EMIT(og_movq(gc, R(mm5), BASE(MX(2, 2), ecx)));
            EMIT(og_pfmul(gc, R(mm5), R(mm0)));
            EMIT(og_pfadd(gc, R(mm2), R(mm4)));
            EMIT(og_pfadd(gc, R(mm3), R(mm5)));
            EMIT(og_movq(gc, R(mm4), BASE(MX(3, 0), ecx)));
            EMIT(og_movq(gc, R(mm5), BASE(MX(3, 2), ecx)));
            EMIT(og_pfadd(gc, R(mm2), R(mm4)));
            EMIT(og_pfadd(gc, R(mm3), R(mm5)));
            EMIT(og_movq(gc, MEMR(VX(clip.x), eax), R(mm2)));
            EMIT(og_movq(gc, MEMR(VX(clip.z), eax), R(mm3)));
            if (gc->texture.textureEnabled[0]) {
                EMIT(og_mov(gc, R(ecx), MEM(&va->texture.pointer[0])));
                EMIT(og_movq(gc, R(mm0), ELT(ecx, edx, 8)));
            }
            if (gc->texture.textureEnabled[1]) {
                EMIT(og_mov(gc, R(ecx), MEM(&va->texture.pointer[1])));
                EMIT(og_movq(gc, R(mm1), ELT(ecx, edx, 8)));
            }
            EMIT(og_pfmul(gc, R(mm0), MEM(&gc->hwcx.texScale[0][0])));
            EMIT(og_pfmul(gc, R(mm1), MEM(&gc->hwcx.texScale[1][0])));
            EMIT(og_mov(gc, R(ebx), HWV(HW_CLIP, edx)));
            EMIT(og_mov(gc, MEMR(VX(flagBits), eax), R(ebx)));
            skip = og_newLabel(gc);
            EMIT(og_test(gc, R(ebx), IMM(__GL_CLIP_NEAR)));
            EMIT(og_jne(gc, skip));
            EMIT(og_movd(gc, R(mm2), MEMR(VX(window.w), eax)));
            EMIT(og_punpckldq(gc, R(mm2), R(mm2)));
            EMIT(og_pfmul(gc, R(mm0), R(mm2)));
            EMIT(og_pfmul(gc, R(mm1), R(mm2)));
            LABEL(skip);
            EMIT(og_movq(gc, MEMR(VX(texture[0].x), eax), R(mm0)));
            EMIT(og_movq(gc, MEMR(VX(texture[1].x), eax), R(mm1)));
        }
        else {
            EMIT(og_fld(gc, BASE(MX(3, 3), ecx)));
            EMIT(og_fld(gc, BASE(MX(3, 2), ecx)));
            EMIT(og_fld(gc, BASE(MX(3, 1), ecx)));
            EMIT(og_fld(gc, BASE(MX(3, 0), ecx)));
            EMIT(og_fld(gc, MEMR(8, ebp)));
            EMIT(og_fmul(gc, BASE(MX(2, 3), ecx)));
            EMIT(og_fld(gc, MEMR(8, ebp)));
            EMIT(og_fmul(gc, BASE(MX(2, 2), ecx)));
            EMIT(og_fld(gc, MEMR(8, ebp)));
            EMIT(og_fmul(gc, BASE(MX(2, 1), ecx)));
            EMIT(og_fld(gc, MEMR(8, ebp)));
            EMIT(og_fmul(gc, BASE(MX(2, 0), ecx)));
            EMIT(og_fxch(gc, R(st3)));
            EMIT(og_faddp(gc, R(st7), R(st0)));
            EMIT(og_faddp(gc, R(st4), R(st0)));
            EMIT(og_faddp(gc, R(st4), R(st0)));
            EMIT(og_faddp(gc, R(st1), R(st0)));
            EMIT(og_fld(gc, MEMR(4, ebp)));
            EMIT(og_fmul(gc, BASE(MX(1, 3), ecx)));
            EMIT(og_fld(gc, MEMR(4, ebp)));
            EMIT(og_fmul(gc, BASE(MX(1, 2), ecx)));
            EMIT(og_fld(gc, MEMR(4, ebp)));
            EMIT(og_fmul(gc, BASE(MX(1, 1), ecx)));
            EMIT(og_fld(gc, MEMR(4, ebp)));
            EMIT(og_fmul(gc, BASE(MX(1, 0), ecx)));
            EMIT(og_fxch(gc, R(st3)));
            EMIT(og_faddp(gc, R(st7), R(st0)));
            EMIT(og_faddp(gc, R(st4), R(st0)));
            EMIT(og_faddp(gc, R(st4), R(st0)));
            EMIT(og_faddp(gc, R(st1), R(st0)));
            EMIT(og_fld(gc, MEMR(0, ebp)));
            EMIT(og_fmul(gc, BASE(MX(0, 3), ecx)));
            EMIT(og_fld(gc, MEMR(0, ebp)));
            EMIT(og_fmul(gc, BASE(MX(0, 2), ecx)));
            EMIT(og_fld(gc, MEMR(0, ebp)));
            EMIT(og_fmul(gc, BASE(MX(0, 1), ecx)));
            EMIT(og_fld(gc, MEMR(0, ebp)));
            EMIT(og_fmul(gc, BASE(MX(0, 0), ecx)));
            EMIT(og_fxch(gc, R(st3)));
            EMIT(og_faddp(gc, R(st7), R(st0)));
            EMIT(og_faddp(gc, R(st4), R(st0)));
            EMIT(og_faddp(gc, R(st4), R(st0)));
            EMIT(og_faddp(gc, R(st1), R(st0)));
            EMIT(og_fxch(gc, R(st3)));
            EMIT(og_fstp(gc, MEMR(VX(clip.w), eax)));
            EMIT(og_fstp(gc, MEMR(VX(clip.y), eax)));
            EMIT(og_fstp(gc, MEMR(VX(clip.z), eax)));
            EMIT(og_fstp(gc, MEMR(VX(clip.x), eax)));
            EMIT(og_mov(gc, R(ebx), HWV(HW_CLIP, edx)));
            EMIT(og_mov(gc, MEMR(VX(flagBits), eax), R(ebx)));
            if (gc->texture.textureEnabled[0]) {
                EMIT(og_mov(gc, R(ecx), MEM(&va->texture.pointer[0])));
                EMIT(og_fld(gc, ELT(ecx, edx, 8)));
                EMIT(og_fmul(gc, MEM(&gc->hwcx.texScale[0][0])));
                skip = og_newLabel(gc);
                EMIT(og_test(gc, R(ebx), IMM(__GL_CLIP_NEAR)));
                EMIT(og_jne(gc, skip));
                EMIT(og_fmul(gc, MEMR(VX(window.w), eax)));
                LABEL(skip);
                EMIT(og_fstp(gc, MEMR(VX(texture[0].x), eax)));
                EMIT(og_add(gc, R(ecx), IMM(4)));
                EMIT(og_fld(gc, ELT(ecx, edx, 8)));
                EMIT(og_fmul(gc, MEM(&gc->hwcx.texScale[0][1])));
                skip = og_newLabel(gc);
                EMIT(og_test(gc, R(ebx), IMM(__GL_CLIP_NEAR)));
                EMIT(og_jne(gc, skip));
                EMIT(og_fmul(gc, MEMR(VX(window.w), eax)));
                LABEL(skip);
                EMIT(og_fstp(gc, MEMR(VX(texture[0].y), eax)));
            }
            if (gc->texture.textureEnabled[1]) {
                EMIT(og_mov(gc, R(ecx), MEM(&va->texture.pointer[1])));
                EMIT(og_fld(gc, ELT(ecx, edx, 8)));
                EMIT(og_fmul(gc, MEM(&gc->hwcx.texScale[1][0])));
                skip = og_newLabel(gc);
                EMIT(og_test(gc, R(ebx), IMM(__GL_CLIP_NEAR)));
                EMIT(og_jne(gc, skip));
                EMIT(og_fmul(gc, MEMR(VX(window.w), eax)));
                LABEL(skip);
                EMIT(og_fstp(gc, MEMR(VX(texture[1].x), eax)));
                EMIT(og_add(gc, R(ecx), IMM(4)));
                EMIT(og_fld(gc, ELT(ecx, edx, 8)));
                EMIT(og_fmul(gc, MEM(&gc->hwcx.texScale[1][1])));
                skip = og_newLabel(gc);
                EMIT(og_test(gc, R(ebx), IMM(__GL_CLIP_NEAR)));
                EMIT(og_jne(gc, skip));
                EMIT(og_fmul(gc, MEMR(VX(window.w), eax)));
                LABEL(skip);
                EMIT(og_fstp(gc, MEMR(VX(texture[1].y), eax)));
            }
        }
    }
    og_encode(gc, pc, og_ret(gc));
}

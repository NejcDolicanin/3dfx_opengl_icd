/*
** sst_lazyprocs.c -- 3dfx back end, original VA range 69044000..6904c000: og generators of the hardware glEnd and
** glTexCoord2f / glMultiTexCoord2fARB / colour re-apply entries, the lazy draw-proc stubs, glColor* entries
** of the hardware path, and the 32-bit depth / stencil buffer procs (stencil in the top byte).
** Then the rest of the immediate-mode code generators (context code, interpolators, vertex finish, glBegin,
** the lazy-proc builder and its key, the matrix vertex) and the page-cache comparator of the generated code,
** in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "og.h"
#include <glide.h>
#include "glsi.h"
#include "icdprotos.h"
#include "glshade.h"
#include "si_names.h"
#include "icdglob.h"
#include "glsitypes.h"

/* 690efd90 -- the immediate-proc class by the glBegin mode: 0 points, 1 lines, 2 triangles, 3 quads */
GLuint g_immProcClassByMode[10] = { 0, 1, 1, 1, 2, 2, 2, 3, 3, 2 };
/* 690efdb8 -- the vertex procs by size 2..4; ToDo UNUSED CODE: nothing in the driver reads it */
void *g_vertexProcsBySizeUnused[3] = { __glim_Vertex2fv, __glim_Vertex3fv, __glim_Vertex4fv };
/* 690efdd0 -- the primitives' vertex state machines: {flags, the state after the next vertex} */
__GLhwPrimDesc g_hwPrimDescs[39] = {
    {   0x0, &g_hwPrimDescs[ 0] },   /*  0 */
    {   0x0, &g_hwPrimDescs[ 2] },   /*  1 */
    {  0x11, &g_hwPrimDescs[ 1] },   /*  2 */
    {   0x0, &g_hwPrimDescs[ 4] },   /*  3 */
    {  0x11, &g_hwPrimDescs[ 5] },   /*  4 */
    {  0x26, &g_hwPrimDescs[ 6] },   /*  5 */
    {  0x19, &g_hwPrimDescs[ 5] },   /*  6 */
    {   0x0, &g_hwPrimDescs[ 8] },   /*  7 */
    {  0x11, &g_hwPrimDescs[ 9] },   /*  8 */
    {   0x4, &g_hwPrimDescs[ 8] },   /*  9 */
    {   0x0, &g_hwPrimDescs[11] },   /* 10 */
    {   0x1, &g_hwPrimDescs[12] },   /* 11 */
    {  0x92, &g_hwPrimDescs[10] },   /* 12 */
    {   0x0, &g_hwPrimDescs[14] },   /* 13 */
    {   0x1, &g_hwPrimDescs[15] },   /* 14 */
    {  0x92, &g_hwPrimDescs[16] },   /* 15 */
    {  0x18, &g_hwPrimDescs[17] },   /* 16 */
    {  0x49, &g_hwPrimDescs[18] },   /* 17 */
    {  0x86, &g_hwPrimDescs[19] },   /* 18 */
    {  0x24, &g_hwPrimDescs[20] },   /* 19 */
    {  0x61, &g_hwPrimDescs[15] },   /* 20 */
    {   0x0, &g_hwPrimDescs[22] },   /* 21 */
    {   0x1, &g_hwPrimDescs[23] },   /* 22 */
    {  0x92, &g_hwPrimDescs[24] },   /* 23 */
    {  0x61, &g_hwPrimDescs[23] },   /* 24 */
    {   0x0, &g_hwPrimDescs[26] },   /* 25 */
    {   0x1, &g_hwPrimDescs[27] },   /* 26 */
    {   0x2, &g_hwPrimDescs[28] },   /* 27 */
    { 0x393, &g_hwPrimDescs[25] },   /* 28 */
    {   0x0, &g_hwPrimDescs[30] },   /* 29 */
    {   0x1, &g_hwPrimDescs[31] },   /* 30 */
    {   0x2, &g_hwPrimDescs[32] },   /* 31 */
    { 0x34b, &g_hwPrimDescs[33] },   /* 32 */
    {   0x0, &g_hwPrimDescs[34] },   /* 33 */
    { 0x1e1, &g_hwPrimDescs[31] },   /* 34 */
    {   0x0, &g_hwPrimDescs[36] },   /* 35 */
    {   0x1, &g_hwPrimDescs[37] },   /* 36 */
    {  0x26, &g_hwPrimDescs[38] },   /* 37 */
    {  0x19, &g_hwPrimDescs[37] },   /* 38 */
};
/* 690eff08 -- the "no primitive" state; 690eff10 -- the first state by the glBegin mode; 690eff38 -- the address the
** generated code loads the "no primitive" state from */
__GLhwPrimDesc g_hwPrimDescNone = { 0, &g_hwPrimDescNone };
__GLhwPrimDesc *g_hwPrimDescByMode[10] = {
    &g_hwPrimDescs[ 0],   /* GL_POINTS */
    &g_hwPrimDescs[ 1],   /* GL_LINES */
    &g_hwPrimDescs[ 3],   /* GL_LINE_LOOP */
    &g_hwPrimDescs[ 7],   /* GL_LINE_STRIP */
    &g_hwPrimDescs[10],   /* GL_TRIANGLES */
    &g_hwPrimDescs[13],   /* GL_TRIANGLE_STRIP */
    &g_hwPrimDescs[21],   /* GL_TRIANGLE_FAN */
    &g_hwPrimDescs[25],   /* GL_QUADS */
    &g_hwPrimDescs[29],   /* GL_QUAD_STRIP */
    &g_hwPrimDescs[35],   /* GL_POLYGON */
};
static __GLhwPrimDesc *g_hwPrimDescNonePtr = &g_hwPrimDescNone;

/* ---- callees of the generated code / generators (og_*: og.h) ---- */
extern int __cdecl og_cacheCompare(const void *key, const void *elem);
extern void __fastcall og_buildLazyProc(__GLcontext *gc, int n);

/* og.h emitters: pc is the running code pointer, gc the context */
#define IDX(r, d)       MEMR(d, r)                                      /* [r + d] */
#define CALL(f)         (pc = og_callAbs(gc, pc, (GLuint)(f)))

/* the 32-bit depth / stencil word of pixel (x, y) in the drawable buffer */
#define DEPTH_ADDR(gc, buf, x, y) \
    ((GLuint *)((((y) - (gc)->constants.viewportYAdjust) * (buf)->outerWidth \
                 - (gc)->constants.viewportXAdjust + (x)) * (buf)->elementSize + (GLint)(buf)->base))

/* ---- forward declarations ---- */
GLubyte *__fastcall og_genEnd(__GLcontext *gc, GLubyte *pc);
GLubyte *__fastcall og_genTexCoord2f(__GLcontext *gc, GLubyte *pc);
GLubyte *__fastcall og_genMultiTexCoord2fARB(__GLcontext *gc, GLubyte *pc);
GLubyte *__fastcall og_genColorReapply(__GLcontext *gc, GLubyte *pc);
void __fastcall og_buildCacheLookup(__GLcontext *gc, __GLhwBufRecICD *rec, GLubyte *pc);
void __fastcall og_lazyStub2_94(__GLcontext *gc, int a, int b);
void __fastcall og_lazyStub3_98(__GLcontext *gc, int a, int b);
void __fastcall og_lazyStub4_9c(__GLcontext *gc, int a, int b);
void __fastcall og_lazyStub2_88(__GLcontext *gc, int a, int b);
void __fastcall og_lazyStub3_8c(__GLcontext *gc, int a, int b);
void __fastcall og_lazyStub4_90(__GLcontext *gc, int a, int b);
void __fastcall og_lazyStub3_a0(__GLcontext *gc, int a, int b);
void __fastcall og_lazyStub3_a8(__GLcontext *gc, int a, int b);
void __fastcall og_lazyStub3_b4(__GLcontext *gc, int a, int b);
void __stdcall __glsstim_Color3dv(const GLdouble *v);
void __stdcall __glsstim_Color3i(GLint red, GLint green, GLint blue);
void __stdcall __glsstim_Color3ui(GLuint red, GLuint green, GLuint blue);
void __stdcall __glsstim_Color4dv(const GLdouble *v);
void __stdcall __glsstim_Color4i(GLint red, GLint green, GLint blue, GLint alpha);
void __stdcall __glsstim_Color4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha);
GLboolean __fastcall Stencil32_TestFunc(__GLstencilBufferICD *sfb, GLint x, GLint y);
void __fastcall Stencil32_FailOp(__GLstencilBufferICD *sfb, GLint x, GLint y);
void __fastcall Stencil32_PassDepthFailOp(__GLstencilBufferICD *sfb, GLint x, GLint y);
void __fastcall Stencil32_DepthPassOp(__GLstencilBufferICD *sfb, GLint x, GLint y);
void __fastcall Stencil32_Clear(void);
void __fastcall Stencil32_MakeCurrent(__GLstencilBufferICD *sfb, GLubyte *other, __GLcontext *gc);
int __fastcall Stencil32_LoseCurrent(void);
GLboolean __fastcall Stencil32_Resize(__GLstencilBufferICD *sfb, GLint x, GLint y, GLuint w, GLuint h, GLuint mask);
void __fastcall sstSten_FreeStencilBuf(__GLstencilBufferICD *sfb, __GLcontext *gc);

/* 69045690 -- og_genEnd: generate glEnd of the hardware path (cf. 69027060): inside glBegin, finish the primitive
** (hwEndLineLoop re-sends the staged vertex for hwPrim 2; hwDirtyTexCombine marks the texture / combine state dirty
** while flag_355be), restore the "no primitive" descriptor and leave glBegin; else GL_INVALID_OPERATION */
GLubyte *__fastcall og_genEnd(__GLcontext *gc, GLubyte *pc)
{
    OgLabel *notInBegin, *noFlush, *noHook;

    notInBegin = og_newLabel(gc);
    noFlush = og_newLabel(gc);
    noHook = og_newLabel(gc);
    EMIT(og_mov(gc, R(eax), MEM(&gc->beginMode)));
    EMIT(og_cmp(gc, R(eax), IMM(__GL_IN_BEGIN)));
    EMIT(og_jne(gc, notInBegin));
    EMIT(og_mov(gc, R(ecx), IMM(gc)));
    EMIT(og_mov(gc, R(eax), MEM(&gc->hwcx.hwPrim)));
    EMIT(og_cmp(gc, R(eax), IMM(2)));
    EMIT(og_jne(gc, noFlush));
    CALL(hwEndLineLoop);
    LABEL(noFlush);
    EMIT(og_mov(gc, R(eax), MEM(&gc->hwcx.flag_355be)));
    EMIT(og_test(gc, R(eax), R(eax)));
    EMIT(og_je(gc, noHook));
    CALL(hwDirtyTexCombine);
    LABEL(noHook);
    EMIT(og_mov(gc, R(eax), MEM(&g_hwPrimDescNonePtr)));
    EMIT(og_mov(gc, MEM(&gc->hwcx.hwPrimDesc), R(eax)));
    EMIT(og_mov(gc, MEM(&gc->hwcx.hwPrim), IMM(-1)));
    EMIT(og_mov(gc, MEM(&gc->beginMode), IMM(__GL_NOT_IN_BEGIN)));
    EMIT(og_ret(gc));
    LABEL(notInBegin);
    EMIT(og_mov(gc, R(ecx), IMM(GL_INVALID_OPERATION)));
    CALL(__glSetError);
    return og_encode(gc, pc, og_ret(gc));
}

/* 690459c0 -- og_genTexCoord2f: generate glTexCoord2f(s, t) (stdcall): (s, t, 0, 1) into
** *state.current.activeTexture */
GLubyte *__fastcall og_genTexCoord2f(__GLcontext *gc, GLubyte *pc)
{
    EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.activeTexture)));
    EMIT(og_mov(gc, R(ecx), IDX(esp, 4)));
    EMIT(og_mov(gc, IDX(eax, 0), R(ecx)));
    EMIT(og_mov(gc, R(ecx), IDX(esp, 8)));
    EMIT(og_mov(gc, IDX(eax, 4), R(ecx)));
    EMIT(og_xor(gc, R(ecx), R(ecx)));
    EMIT(og_mov(gc, IDX(eax, 8), R(ecx)));
    EMIT(og_mov(gc, R(ecx), IMM(0x3f800000)));         /* 1.0f */
    EMIT(og_mov(gc, IDX(eax, 0xc), R(ecx)));
    return og_encode(gc, pc, og_ret_imm(gc, IMM(8)));
}

/* 69045bb0 -- og_genMultiTexCoord2fARB: generate glMultiTexCoord2fARB(target, s, t) (stdcall): (s, t, 0, 1) into
** state.current.texture[target & 1] */
GLubyte *__fastcall og_genMultiTexCoord2fARB(__GLcontext *gc, GLubyte *pc)
{
    EMIT(og_mov(gc, R(eax), IDX(esp, 4)));
    EMIT(og_and(gc, R(eax), IMM(1)));
    EMIT(og_shl(gc, R(eax), IMM(4)));
    EMIT(og_add(gc, R(eax), IMM(&gc->state.current.texture[0])));
    EMIT(og_mov(gc, R(ecx), IDX(esp, 8)));
    EMIT(og_mov(gc, IDX(eax, 0), R(ecx)));
    EMIT(og_mov(gc, R(ecx), IDX(esp, 0xc)));
    EMIT(og_mov(gc, IDX(eax, 4), R(ecx)));
    EMIT(og_xor(gc, R(ecx), R(ecx)));
    EMIT(og_mov(gc, IDX(eax, 8), R(ecx)));
    EMIT(og_mov(gc, R(ecx), IMM(0x3f800000)));         /* 1.0f */
    EMIT(og_mov(gc, IDX(eax, 0xc), R(ecx)));
    return og_encode(gc, pc, og_ret_imm(gc, IMM(0xc)));
}

/* 69047600 -- og_genColorReapply: generate the colour re-apply: push the current user colour (a, b, g, r) and call
** the generated glColor4f (hwcx.genColor4f) */
GLubyte *__fastcall og_genColorReapply(__GLcontext *gc, GLubyte *pc)
{
    EMIT(og_mov(gc, R(eax), IMM(&gc->state.current.userColor)));
    EMIT(og_mov(gc, R(ecx), IDX(eax, 0xc)));
    EMIT(og_push(gc, R(ecx)));
    EMIT(og_mov(gc, R(ecx), IDX(eax, 8)));
    EMIT(og_push(gc, R(ecx)));
    EMIT(og_mov(gc, R(ecx), IDX(eax, 4)));
    EMIT(og_push(gc, R(ecx)));
    EMIT(og_mov(gc, R(ecx), IDX(eax, 0)));
    EMIT(og_push(gc, R(ecx)));
    CALL(gc->hwcx.genColor4f);
    return og_encode(gc, pc, og_ret(gc));
}

/*
** 69047790 -- og_buildCacheLookup: generate a page cache's own lookup routine at pc and install it as rec->alloc. 
** The routine is called as (gc, rec, key, blk): the most recent entry (rec->tail) is compared dword by
** dword first; else bsearch the sorted array (comparator og_cacheCompare) and make the hit the most recent
** (hwPageCacheTouch).  Returns the entry's valA, valB through blk when blk != NULL, 0 when absent.
** Original quirk kept: with no tail the bsearch key is EDX (rec), not the key; the cache is then empty
** (count 0), so bsearch does not compare.
*/
void __fastcall og_buildCacheLookup(__GLcontext *gc, __GLhwBufRecICD *rec, GLubyte *pc)
{
    OgLabel *found, *hit, *noBlk, *search;
    GLint i;

    og_reset(gc);
    found = og_newFixup(gc);
    hit = og_newFixup(gc);
    noBlk = og_newFixup(gc);
    search = og_newFixup(gc);
    rec->alloc = (__GLproc)pc;
    EMIT(og_push(gc, R(ebx)));
    EMIT(og_mov(gc, R(eax), MEM(&rec->tail)));
    EMIT(og_test(gc, R(eax), R(eax)));
    EMIT(og_je(gc, search));
    EMIT(og_mov(gc, R(edx), IDX(esp, 8)));                         /* key */
    EMIT(og_mov(gc, R(eax), IDX(eax, 0)));                         /* tail->key */
    EMIT(og_mov(gc, R(ebx), og_mem(gc, NULL, R(edx), NULL, 1)));
    EMIT(og_sub(gc, R(ebx), og_mem(gc, NULL, R(eax), NULL, 1)));
    i = 4;
    if (4 < (GLint)rec->keySize) {
        do {
            EMIT(og_mov(gc, R(ecx), IDX(edx, i)));
            EMIT(og_sub(gc, R(ecx), IDX(eax, i)));
            EMIT(og_or(gc, R(ebx), R(ecx)));
            i += 4;
        } while (i < (GLint)rec->keySize);
    }
    EMIT(og_je(gc, found));
    LABEL(search);
    EMIT(og_push(gc, IMM(og_cacheCompare)));
    EMIT(og_push(gc, IMM(4)));
    EMIT(og_mov(gc, R(eax), MEM(&rec->count)));
    EMIT(og_push(gc, R(eax)));
    EMIT(og_push(gc, IMM(rec->sorted)));
    EMIT(og_push(gc, R(edx)));
    CALL(bsearch);
    EMIT(og_add(gc, R(esp), IMM(0x14)));
    EMIT(og_test(gc, R(eax), R(eax)));
    EMIT(og_jne(gc, hit));
    EMIT(og_pop(gc, R(ebx)));
    EMIT(og_ret_imm(gc, IMM(8)));
    LABEL(hit);
    EMIT(og_mov(gc, R(edx), R(eax)));
    EMIT(og_mov(gc, R(ecx), IMM(rec)));
    CALL(hwPageCacheTouch);
    LABEL(found);
    EMIT(og_mov(gc, R(eax), MEM(&rec->tail)));
    EMIT(og_mov(gc, R(ebx), IDX(esp, 0xc)));                       /* blk */
    EMIT(og_test(gc, R(ebx), R(ebx)));
    EMIT(og_je(gc, noBlk));
    EMIT(og_mov(gc, R(ecx), IDX(eax, 8)));                         /* valB */
    EMIT(og_mov(gc, og_mem(gc, NULL, R(ebx), NULL, 1), R(ecx)));
    LABEL(noBlk);
    EMIT(og_mov(gc, R(eax), IDX(eax, 4)));                         /* valA */
    EMIT(og_pop(gc, R(ebx)));
    og_encode(gc, pc, og_ret_imm(gc, IMM(8)));
}

/* 69047d70 -- hwClassifyMatrix: procs.classifyMatrix of the hardware path (installed by wgldev.c; 690480d0 calls it
** too): marks every element of the matrix general (elementClass 01 per element), so the generated transforms skip no
** term */
void __fastcall hwClassifyMatrix(__GLcontext *gc, __GLmatrixICD *m)
{
    m->elementClass = 0x55555555;
}

/* 69047d80 -- start the lazy draw procs as their stubs (each compiles its slot on the first call) */
void __fastcall og_initLazyProcs(__GLcontext *gc)
{
    gc->hwcx.ogLazyProcs[2] = (__GLproc)og_lazyStub2_88;
    gc->hwcx.ogLazyProcs[11] = (__GLproc)og_lazyStub3_a8;
    gc->hwcx.ogLazyProcs[10] = (__GLproc)og_lazyStub3_a8;
    gc->hwcx.ogLazyProcs[9] = (__GLproc)og_lazyStub3_a8;
    gc->hwcx.ogLazyProcs[3] = (__GLproc)og_lazyStub3_8c;
    gc->hwcx.ogLazyProcs[4] = (__GLproc)og_lazyStub4_90;
    gc->hwcx.ogLazyProcs[5] = (__GLproc)og_lazyStub2_94;
    gc->hwcx.ogLazyProcs[6] = (__GLproc)og_lazyStub3_98;
    gc->hwcx.ogLazyProcs[7] = (__GLproc)og_lazyStub4_9c;
    gc->hwcx.ogLazyProcs[8] = (__GLproc)og_lazyStub3_a0;
    gc->hwcx.ogLazyProcs[14] = (__GLproc)og_lazyStub3_b4;
    gc->hwcx.ogLazyProcs[13] = (__GLproc)og_lazyStub3_b4;
    gc->hwcx.ogLazyProcs[12] = (__GLproc)og_lazyStub3_b4;
}

/* 69047e00 -- lazy stub of ogLazyProcs[5]: compile the 2-component procs, then call the compiled one */
void __fastcall og_lazyStub2_94(__GLcontext *gc, int a, int b)
{
    og_buildLazyProc(gc, 2);
    (*gc->hwcx.ogLazyProcs[5])(gc, a, b);
}

/* 69048e00 -- lazy stub of ogLazyProcs[6] (3 components) */
void __fastcall og_lazyStub3_98(__GLcontext *gc, int a, int b)
{
    og_buildLazyProc(gc, 3);
    (*gc->hwcx.ogLazyProcs[6])(gc, a, b);
}

/* 69048e30 -- lazy stub of ogLazyProcs[7] (4 components) */
void __fastcall og_lazyStub4_9c(__GLcontext *gc, int a, int b)
{
    og_buildLazyProc(gc, 4);
    (*gc->hwcx.ogLazyProcs[7])(gc, a, b);
}

/* 69048e60 -- lazy stub of ogLazyProcs[2] (2 components) */
void __fastcall og_lazyStub2_88(__GLcontext *gc, int a, int b)
{
    og_buildLazyProc(gc, 2);
    (*gc->hwcx.ogLazyProcs[2])(gc, a, b);
}

/* 69048e90 -- lazy stub of ogLazyProcs[3] (3 components) */
void __fastcall og_lazyStub3_8c(__GLcontext *gc, int a, int b)
{
    og_buildLazyProc(gc, 3);
    (*gc->hwcx.ogLazyProcs[3])(gc, a, b);
}

/* 69048ec0 -- lazy stub of ogLazyProcs[4] (4 components) */
void __fastcall og_lazyStub4_90(__GLcontext *gc, int a, int b)
{
    og_buildLazyProc(gc, 4);
    (*gc->hwcx.ogLazyProcs[4])(gc, a, b);
}

/* 69048ef0 -- lazy stub of ogLazyProcs[8] (3 components) */
void __fastcall og_lazyStub3_a0(__GLcontext *gc, int a, int b)
{
    og_buildLazyProc(gc, 3);
    (*gc->hwcx.ogLazyProcs[8])(gc, a, b);
}

/* 69048f20 -- lazy stub of ogLazyProcs[9..11] (3 components; calls slot 10) */
void __fastcall og_lazyStub3_a8(__GLcontext *gc, int a, int b)
{
    og_buildLazyProc(gc, 3);
    (*gc->hwcx.ogLazyProcs[10])(gc, a, b);
}

/* 69048f50 -- lazy stub of ogLazyProcs[12..14] (3 components; calls slot 13) */
void __fastcall og_lazyStub3_b4(__GLcontext *gc, int a, int b)
{
    og_buildLazyProc(gc, 3);
    (*gc->hwcx.ogLazyProcs[13])(gc, a, b);
}

/* 690491b0 -- __glsstim_Color3dv: glColor3dv, hardware path */
void __stdcall __glsstim_Color3dv(const GLdouble *v)
{
    __GL_SETUP();

    gc->state.current.userColor.r = (__GLfloat)v[0];
    gc->state.current.userColor.g = (__GLfloat)v[1];
    gc->state.current.userColor.b = (__GLfloat)v[2];
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 69049470 -- __glsstim_Color3i: glColor3i, hardware path */
void __stdcall __glsstim_Color3i(GLint red, GLint green, GLint blue)
{
    __GL_SETUP();

    gc->state.current.userColor.r = __GL_I_TO_FLOAT(red);
    gc->state.current.userColor.g = __GL_I_TO_FLOAT(green);
    gc->state.current.userColor.b = __GL_I_TO_FLOAT(blue);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 69049570 -- __glsstim_Color3ui: glColor3ui, hardware path */
void __stdcall __glsstim_Color3ui(GLuint red, GLuint green, GLuint blue)
{
    __GL_SETUP();

    gc->state.current.userColor.r = __GL_UI_TO_FLOAT(red);
    gc->state.current.userColor.g = __GL_UI_TO_FLOAT(green);
    gc->state.current.userColor.b = __GL_UI_TO_FLOAT(blue);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 69049b40 -- __glsstim_Color4dv: glColor4dv, hardware path */
void __stdcall __glsstim_Color4dv(const GLdouble *v)
{
    __GL_SETUP();

    gc->state.current.userColor.r = (__GLfloat)v[0];
    gc->state.current.userColor.g = (__GLfloat)v[1];
    gc->state.current.userColor.b = (__GLfloat)v[2];
    gc->state.current.userColor.a = (__GLfloat)v[3];
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 69049f80 -- __glsstim_Color4i: glColor4i, hardware path */
void __stdcall __glsstim_Color4i(GLint red, GLint green, GLint blue, GLint alpha)
{
    __GL_SETUP();

    gc->state.current.userColor.r = __GL_I_TO_FLOAT(red);
    gc->state.current.userColor.g = __GL_I_TO_FLOAT(green);
    gc->state.current.userColor.b = __GL_I_TO_FLOAT(blue);
    gc->state.current.userColor.a = __GL_I_TO_FLOAT(alpha);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 6904a090 -- __glsstim_Color4ui: glColor4ui, hardware path */
void __stdcall __glsstim_Color4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    __GL_SETUP();

    gc->state.current.userColor.r = __GL_UI_TO_FLOAT(red);
    gc->state.current.userColor.g = __GL_UI_TO_FLOAT(green);
    gc->state.current.userColor.b = __GL_UI_TO_FLOAT(blue);
    gc->state.current.userColor.a = __GL_UI_TO_FLOAT(alpha);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 6904a5a0 -- __glInitStencil32: initialise the stencil buffer that lives in the top byte of the 32-bit depth
** buffer */
void __fastcall __glInitStencil32(__GLstencilBufferICD *sfb)
{
    __glInitBuffer(&sfb->buf);
    sfb->buf.makeCurrent = (__GLproc)Stencil32_MakeCurrent;
    sfb->buf.loseCurrent = (__GLproc)Stencil32_LoseCurrent;
    sfb->buf.resize = (__GLproc)Stencil32_Resize;
    sfb->buf.freeBuf = (__GLproc)sstSten_FreeStencilBuf;
    sfb->pick = (__GLproc)__glSSTValidateStencil;
    sfb->store = (__GLproc)Stencil32_Store;
    sfb->fetch = (__GLproc)Stencil32_Fetch;
    sfb->testFunc = Stencil32_TestFunc;
    sfb->failOp = (__GLproc)Stencil32_FailOp;
    sfb->passDepthFailOp = (__GLproc)Stencil32_PassDepthFailOp;
    sfb->depthPassOp = (__GLproc)Stencil32_DepthPassOp;
    sfb->clear = (__GLproc)Stencil32_Clear;
}

/* 6904a6a0 -- Stencil32_TestFunc: stencil test of pixel (x, y) through the test table (s_stencil.c TestFunc) */
GLboolean __fastcall Stencil32_TestFunc(__GLstencilBufferICD *sfb, GLint x, GLint y)
{
    __GLcontext *gc = sfb->buf.gc;
    __WGLbuffer *buf = sfb->buf.wglBuf;

    return sfb->testFuncTable[*DEPTH_ADDR(gc, buf, x, y) >> 24 & gc->state.stencil.mask];
}

/* 6904a6f0 -- Stencil32_FailOp: stencil fail op of pixel (x, y) through its table (s_stencil.c FailOp) */
void __fastcall Stencil32_FailOp(__GLstencilBufferICD *sfb, GLint x, GLint y)
{
    __WGLbuffer *buf = sfb->buf.wglBuf;
    GLuint *p = DEPTH_ADDR(sfb->buf.gc, buf, x, y);
    GLuint w = *p;

    *p = (GLuint)sfb->failOpTable[w >> 24] << 24 | w & 0xffffff;
}

/* 6904a740 -- Stencil32_PassDepthFailOp: stencil depth-fail op (s_stencil.c PassDepthFailOp) */
void __fastcall Stencil32_PassDepthFailOp(__GLstencilBufferICD *sfb, GLint x, GLint y)
{
    __WGLbuffer *buf = sfb->buf.wglBuf;
    GLuint *p = DEPTH_ADDR(sfb->buf.gc, buf, x, y);
    GLuint w = *p;

    *p = (GLuint)sfb->depthFailOpTable[w >> 24] << 24 | w & 0xffffff;
}

/* 6904a790 -- Stencil32_DepthPassOp: stencil depth-pass op (s_stencil.c DepthPassOp) */
void __fastcall Stencil32_DepthPassOp(__GLstencilBufferICD *sfb, GLint x, GLint y)
{
    __WGLbuffer *buf = sfb->buf.wglBuf;
    GLuint *p = DEPTH_ADDR(sfb->buf.gc, buf, x, y);
    GLuint w = *p;

    *p = (GLuint)sfb->depthPassOpTable[w >> 24] << 24 | w & 0xffffff;
}

/* 6904a7e0 -- Stencil32_Clear: stencil clear: nothing (the hardware clears it) */
void __fastcall Stencil32_Clear(void)
{
}

/* 6904a7f0 -- Stencil32_MakeCurrent: stencil makeCurrent: on the depth buffer's drawable buffer (the shared 32-bit
** word) */
void __fastcall Stencil32_MakeCurrent(__GLstencilBufferICD *sfb, GLubyte *other, __GLcontext *gc)
{
    __glMakeCurrentBuffer(&sfb->buf, gc->depthBuffer.buf.wglBuf, gc);
}

/* 6904a810 -- Stencil32_LoseCurrent: the stencil buffer's loseCurrent, a jump to the generic __glLoseCurrentBuffer */
#ifdef _MSC_VER
/* 6904a810 -- Stencil32_LoseCurrent: (VC6 build) the tail jump to __glLoseCurrentBuffer */
__declspec(naked) int __fastcall Stencil32_LoseCurrent() { __asm jmp __glLoseCurrentBuffer }
#else
/* Stencil32_LoseCurrent: the stand-in that lets the non-MSVC syntax check compile (returns 0) */
int __fastcall Stencil32_LoseCurrent() { return 0; }
#endif

/* 6904a820 -- Stencil32_Resize: stencil resize: nothing to allocate */
GLboolean __fastcall Stencil32_Resize(__GLstencilBufferICD *sfb, GLint x, GLint y, GLuint w, GLuint h, GLuint mask)
{
    return GL_TRUE;
}

/* 6904a830 -- sstSten_FreeStencilBuf: FreeStencilBuf: stencil freeBuf: free the four op tables */
void __fastcall sstSten_FreeStencilBuf(__GLstencilBufferICD *sfb, __GLcontext *gc)
{
    __GL_FREE(gc, sfb->testFuncTable);
    __GL_FREE(gc, sfb->failOpTable);
    __GL_FREE(gc, sfb->depthFailOpTable);
    __GL_FREE(gc, sfb->depthPassOpTable);
    sfb->testFuncTable = 0;
    sfb->failOpTable = 0;
    sfb->depthFailOpTable = 0;
    sfb->depthPassOpTable = 0;
}

/* 6904ad20 -- sstDepth_FreeBuf: depth buffer freeBuf: free the private block */
void __fastcall sstDepth_FreeBuf(__GLdepthBufferICD *dfb, __GLcontext *gc)
{
    if (dfb->buf.drawableBuf) {
        __GL_FREE(gc, dfb->buf.drawableBuf);
    }
    dfb->buf.makeCurrent = 0;
    dfb->buf.resize = 0;
}

/* 6904b220 -- sstRGB_Fetch_32A: Fetch_32A: colour fetch of a 32-bit (ARGB 8888) buffer: the components by the
** visual's masks and the buffer's shifts, unscaled */
void __fastcall sstRGB_Fetch_32A(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *buf = cfb->buf.wglBuf;
    GLuint pixel = *DEPTH_ADDR(gc, buf, x, y);

    result->r = (__GLfloat)((gc->modes.redMask & pixel) >> cfb->redShift);
    result->g = (__GLfloat)((gc->modes.greenMask & pixel) >> cfb->greenShift);
    result->b = (__GLfloat)((gc->modes.blueMask & pixel) >> cfb->blueShift);
    result->a = (__GLfloat)((gc->modes.alphaMask & pixel) >> cfb->alphaShift);
}
/* og.h emitters (EMIT / R / IMM / MEM / MEMR / LABEL) and: */
#define BI(b, i)        og_mem(gc, NULL, R(b), R(i), 1)             /* [b + i] */
#define BID(d, b, i)    og_mem(gc, IMM(d), R(b), R(i), 1)           /* [b + i + d] */
/* the hardware primitive tables indexed by the glBegin mode: g_hwPrimDescByMode / g_immProcClassByMode (icdglob.h) */
extern GLubyte *__fastcall og_genVertexFinish(__GLcontext *gc, GLubyte *pc, GLubyte **data);
extern GLubyte *__fastcall og_genInterpolators(__GLcontext *gc, GLubyte *pc);
extern GLubyte *__fastcall og_genEnd(__GLcontext *gc, GLubyte *pc);
extern GLubyte *__fastcall og_genTexCoord2f(__GLcontext *gc, GLubyte *pc);
extern GLubyte *__fastcall og_genMultiTexCoord2fARB(__GLcontext *gc, GLubyte *pc);
extern GLubyte *__fastcall og_genColorReapply(__GLcontext *gc, GLubyte *pc);
extern void __fastcall og_lazyProcKey(__GLcontext *gc, GLuint *key, GLuint n);
extern GLubyte *__fastcall og_genMatrixVertex(__GLcontext *gc, GLubyte *pc, GLuint *m, GLint size);

extern GLubyte *__fastcall og_genBegin(__GLcontext *gc, GLubyte *pc);
/* 6903ca40 -- og_genContextCode: generates the context's code into the code buffer (69027630 sets it up): each
** generator returns the end of its routine, which is where the next one starts and is recorded: the interpolators
** (genCode[1] / [2], 6903fba0), the clip-edge routine ([0]: SSE, 3DNow or x87), [3] 690402a0, [4] the clipped quad,
** [5] the immediate-vertex transform, [6] the vertex finish (690423c0), [7] glBegin, [8] glEnd, then glTexCoord2f,
** glMultiTexCoord2fARB, glColor4f (69045e10) and the colour re-apply.  The constant pool the SSE code reads is a
** 32-byte aligned malloc(0x1000), never freed */
void __fastcall og_genContextCode(__GLcontext *gc)
{
    GLubyte *pc;
    GLubyte *data;

    og_reset(gc);
    hwAllocCodeBuffers(gc);
    pc = (GLubyte *)gc->hwcx.codeBufPage;
    data = (GLubyte *)(((GLuint)malloc(0x1000) + 0x1f) & ~0x1f);
    pc = og_genInterpolators(gc, pc);
    gc->hwcx.genCode[0] = (GLuint)pc;
    if (gc->constants.cpuCaps & 4) {
        pc = og_genClipEdgeSSE(gc, pc);
    }
    else if (gc->constants.cpuCaps & 2) {
        pc = og_genClipEdge3DNow(gc, pc);
    }
    else {
        pc = og_genClipEdgeX87(gc, pc);
    }
    gc->hwcx.genCode[3] = (GLuint)pc;
    pc = hwGenClipTriangle(gc, pc);
    gc->hwcx.genCode[4] = (GLuint)pc;
    pc = og_genClipAndDrawQuad(gc, pc);
    gc->hwcx.genCode[5] = (GLuint)pc;
    pc = og_genVertexTransform(gc, pc);
    gc->hwcx.genCode[6] = (GLuint)pc;
    pc = og_genVertexFinish(gc, pc, &data);
    gc->hwcx.genCode[7] = (GLuint)pc;
    pc = og_genBegin(gc, pc);
    gc->hwcx.genCode[8] = (GLuint)pc;
    pc = og_genEnd(gc, pc);
    gc->hwcx.genTexCoord2f = pc;
    pc = og_genTexCoord2f(gc, pc);
    gc->hwcx.genMultiTexCoord2fARB = pc;
    pc = og_genMultiTexCoord2fARB(gc, pc);
    gc->hwcx.genColor4f = pc;
    pc = og_genColor4f(gc, pc, (GLuint **)&data);
    gc->hwcx.genApplyColor = (GLuint)pc;
    og_genColorReapply(gc, pc);
}

/* 6903fba0 -- og_genInterpolators: generates the attribute interpolators genCode[1] (4 floats) and genCode[2] (2
** floats): [edi + ecx] = [eax + ecx] * t + [ebx + ecx] * (1 - t), t / 1 - t in xmm1 / xmm0, mm1 / mm0 or st1 / st3.
** SSE (cpuCaps & 4), 3DNow (& 2) or x87, where the 4-float entry calls the 2-float one and falls into it */
GLubyte *__fastcall og_genInterpolators(__GLcontext *gc, GLubyte *pc)
{
    OgLabel *half = og_newFixup(gc);

    if (gc->constants.cpuCaps & 4) {
        gc->hwcx.genCode[1] = (GLuint)pc;
        EMIT(og_movaps(gc, R(mm4), BI(eax, ecx)));
        EMIT(og_movaps(gc, R(mm3), BI(ebx, ecx)));
        EMIT(og_mulps(gc, R(mm4), R(mm1)));
        EMIT(og_mulps(gc, R(mm3), R(mm0)));
        EMIT(og_addps(gc, R(mm4), R(mm3)));
        EMIT(og_movaps(gc, BI(edi, ecx), R(mm4)));
        EMIT(og_ret(gc));
        gc->hwcx.genCode[2] = (GLuint)pc;
        EMIT(og_movlps(gc, R(mm4), BI(eax, ecx)));
        EMIT(og_movlps(gc, R(mm3), BI(ebx, ecx)));
        EMIT(og_mulps(gc, R(mm4), R(mm1)));
        EMIT(og_mulps(gc, R(mm3), R(mm0)));
        EMIT(og_addps(gc, R(mm4), R(mm3)));
        EMIT(og_movlps(gc, BI(edi, ecx), R(mm4)));
    }
    else if (gc->constants.cpuCaps & 2) {
        gc->hwcx.genCode[1] = (GLuint)pc;
        EMIT(og_movq(gc, R(mm4), BI(eax, ecx)));
        EMIT(og_movq(gc, R(mm3), BI(ebx, ecx)));
        EMIT(og_pfmul(gc, R(mm4), R(mm1)));
        EMIT(og_pfmul(gc, R(mm3), R(mm0)));
        EMIT(og_movq(gc, R(mm6), BID(8, eax, ecx)));
        EMIT(og_pfadd(gc, R(mm4), R(mm3)));
        EMIT(og_movq(gc, R(mm5), BID(8, ebx, ecx)));
        EMIT(og_movq(gc, BI(edi, ecx), R(mm4)));
        EMIT(og_pfmul(gc, R(mm6), R(mm1)));
        EMIT(og_pfmul(gc, R(mm5), R(mm0)));
        EMIT(og_pfadd(gc, R(mm6), R(mm5)));
        EMIT(og_movq(gc, BID(8, edi, ecx), R(mm6)));
        EMIT(og_ret(gc));
        gc->hwcx.genCode[2] = (GLuint)pc;
        EMIT(og_movq(gc, R(mm4), BI(eax, ecx)));
        EMIT(og_movq(gc, R(mm3), BI(ebx, ecx)));
        EMIT(og_pfmul(gc, R(mm4), R(mm1)));
        EMIT(og_pfmul(gc, R(mm3), R(mm0)));
        EMIT(og_pfadd(gc, R(mm4), R(mm3)));
        EMIT(og_movq(gc, BI(edi, ecx), R(mm4)));
    }
    else {
        gc->hwcx.genCode[1] = (GLuint)pc;
        EMIT(og_call(gc, half));
        EMIT(og_add(gc, R(ecx), IMM(8)));
        LABEL(half);
        gc->hwcx.genCode[2] = (GLuint)pc;
        EMIT(og_fld(gc, BI(eax, ecx)));
        EMIT(og_fmul(gc, R(st1)));
        EMIT(og_fld(gc, BI(ebx, ecx)));
        EMIT(og_fmul(gc, R(st3)));
        EMIT(og_faddp(gc, R(st1), R(st0)));
        EMIT(og_fstp(gc, BI(edi, ecx)));
        EMIT(og_fld(gc, BID(4, eax, ecx)));
        EMIT(og_fmul(gc, R(st1)));
        EMIT(og_fld(gc, BID(4, ebx, ecx)));
        EMIT(og_fmul(gc, R(st3)));
        EMIT(og_faddp(gc, R(st1), R(st0)));
        EMIT(og_fstp(gc, BID(4, edi, ecx)));
    }
    return og_encode(gc, pc, og_ret(gc));
}

/* 690423c0 -- og_genVertexFinish: generates the immediate-vertex finish routine: ebp = the staging vertex; its clip
** code from the clip coordinates (+0x40..+0x4c: sign bits and |c| > |w| compares through the three tables) into
** flagBits (+0x10), the current colour into colors[0] (+0x60) and packedColor (+0x14), then the next primitive
** descriptor (hwPrimDesc = desc->next) and its staging vertex (hwcx.immVertex).  3DNow (cpuCaps & 2), SSE (& 4: the
** {0, -0, 0, -0} sign mask goes to the constant pool *data, 16 bytes) or x87 */
GLubyte *__fastcall og_genVertexFinish(__GLcontext *gc, GLubyte *pc, GLubyte **data)
{
    GLuint *signMask = (GLuint *)*data;

    signMask[0] = 0;
    signMask[1] = 0x80000000;
    signMask[2] = 0;
    signMask[3] = 0x80000000;
    *data += 0x10;
    if (gc->constants.cpuCaps & 2) {
        EMIT(og_femms(gc));
        EMIT(og_movd(gc, R(mm7), MEMR(0x4c, ebp)));
        EMIT(og_mov(gc, R(edx), MEMR(0x4c, ebp)));
        EMIT(og_add(gc, R(edx), R(edx)));
        EMIT(og_sbb(gc, R(esi), R(esi)));
        EMIT(og_mov(gc, R(ecx), MEMR(0x48, ebp)));
        EMIT(og_mov(gc, R(ebx), MEMR(0x44, ebp)));
        EMIT(og_pfrcp(gc, R(mm1), R(mm7)));
        EMIT(og_mov(gc, R(eax), MEMR(0x40, ebp)));
        EMIT(og_adc(gc, R(ecx), R(ecx)));
        EMIT(og_adc(gc, R(esi), R(esi)));
        EMIT(og_add(gc, R(ebx), R(ebx)));
        EMIT(og_pfrcpit1(gc, R(mm7), R(mm1)));
        EMIT(og_adc(gc, R(esi), R(esi)));
        EMIT(og_add(gc, R(eax), R(eax)));
        EMIT(og_movq(gc, R(mm4), MEM(&gc->state.current.color)));
        EMIT(og_adc(gc, R(esi), R(esi)));
        EMIT(og_cmp(gc, R(edx), R(ecx)));
        EMIT(og_pfrcpit2(gc, R(mm7), R(mm1)));
        EMIT(og_adc(gc, R(ecx), R(ecx)));
        EMIT(og_cmp(gc, R(edx), R(ebx)));
        EMIT(og_movq(gc, R(mm5), MEM(&gc->state.current.color.b)));
        EMIT(og_adc(gc, R(ecx), R(ecx)));
        EMIT(og_cmp(gc, R(edx), R(eax)));
        EMIT(og_movd(gc, MEMR(0xc, ebp), R(mm7)));
        EMIT(og_adc(gc, R(ecx), R(ecx)));
        EMIT(og_mov(gc, R(ebx), R(esi)));
        EMIT(og_movq(gc, MEMR(0x60, ebp), R(mm4)));
        EMIT(og_and(gc, R(ebx), IMM(0xf)));
        EMIT(og_and(gc, R(ecx), IMM(0xf)));
        EMIT(og_movq(gc, MEMR(0x68, ebp), R(mm5)));
        EMIT(og_mov(gc, R(ecx), TAB(g_clipAxisBits, ecx)));
        EMIT(og_mov(gc, R(esi), TAB(g_clipSideBits, ebx)));
        EMIT(og_and(gc, R(esi), R(ecx)));
        EMIT(og_mov(gc, R(ecx), TAB(g_clipWNegBits, ebx)));
        EMIT(og_or(gc, R(esi), R(ecx)));
        EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.packedColor)));
        EMIT(og_mov(gc, MEMR(0x14, ebp), R(eax)));
        EMIT(og_mov(gc, MEMR(0x10, ebp), R(esi)));
        EMIT(og_mov(gc, R(eax), MEM(&gc->hwcx.hwPrimDesc)));
        EMIT(og_mov(gc, R(ebx), MEMR(4, eax)));
        EMIT(og_mov(gc, R(edx), MEMR(0, eax)));
        EMIT(og_mov(gc, MEM(&gc->hwcx.hwPrimDesc), R(ebx)));
        EMIT(og_mov(gc, R(ecx), MEMR(0, ebx)));
        EMIT(og_and(gc, R(ecx), IMM(3)));
        EMIT(og_shr(gc, R(edx), IMM(2)));
        EMIT(og_mov(gc, R(ecx), TAB(&gc->hwcx.stagingVtx[0], ecx)));
        EMIT(og_mov(gc, MEM(&gc->hwcx.immVertex), R(ecx)));
        EMIT(og_femms(gc));
    }
    else if (gc->constants.cpuCaps & 4) {
        EMIT(og_movss(gc, R(mm0), MEMR(0x4c, ebp)));
        EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.packedColor)));
        EMIT(og_movaps(gc, R(mm2), MEMR(0x40, ebp)));
        EMIT(og_mov(gc, MEMR(0x14, ebp), R(eax)));
        EMIT(og_movaps(gc, R(mm7), MEM(signMask)));
        EMIT(og_rcpss(gc, R(mm1), R(mm0)));
        EMIT(og_mov(gc, R(eax), MEM(&gc->hwcx.hwPrimDesc)));
        EMIT(og_movaps(gc, R(mm5), R(mm2)));
        EMIT(og_mov(gc, R(ecx), MEMR(4, eax)));
        EMIT(og_movaps(gc, R(mm6), R(mm5)));
        EMIT(og_mov(gc, R(edx), MEMR(0, eax)));
        EMIT(og_shufps(gc, R(mm2), R(mm2), IMM(0xff)));
        EMIT(og_mov(gc, MEM(&gc->hwcx.hwPrimDesc), R(ecx)));
        EMIT(og_mulss(gc, R(mm0), R(mm1)));
        EMIT(og_mov(gc, R(ecx), MEMR(0, ecx)));
        EMIT(og_shufps(gc, R(mm5), R(mm5), IMM(0x50)));
        EMIT(og_and(gc, R(ecx), IMM(3)));
        EMIT(og_shufps(gc, R(mm6), R(mm6), IMM(10)));
        EMIT(og_shr(gc, R(edx), IMM(2)));
        EMIT(og_mulss(gc, R(mm0), R(mm1)));
        EMIT(og_mov(gc, R(ecx), TAB(&gc->hwcx.stagingVtx[0], ecx)));
        EMIT(og_xorps(gc, R(mm5), R(mm7)));
        EMIT(og_mov(gc, MEM(&gc->hwcx.immVertex), R(ecx)));
        EMIT(og_xorps(gc, R(mm6), R(mm7)));
        EMIT(og_addss(gc, R(mm1), R(mm1)));
        EMIT(og_addps(gc, R(mm5), R(mm2)));
        EMIT(og_addps(gc, R(mm6), R(mm2)));
        EMIT(og_subss(gc, R(mm1), R(mm0)));
        EMIT(og_movups(gc, R(mm0), MEM(&gc->state.current.color)));
        EMIT(og_movmskps(gc, R(esi), R(mm5)));
        EMIT(og_movmskps(gc, R(ebx), R(mm6)));
        EMIT(og_and(gc, R(ebx), IMM(3)));
        EMIT(og_shl(gc, R(ebx), IMM(4)));
        EMIT(og_or(gc, R(esi), R(ebx)));
        EMIT(og_shl(gc, R(esi), IMM(0x10)));
        EMIT(og_movaps(gc, MEMR(0x60, ebp), R(mm0)));
        EMIT(og_mov(gc, MEMR(0x10, ebp), R(esi)));
    }
    else {
        EMIT(og_fld1(gc));
        EMIT(og_mov(gc, R(edx), MEMR(0x4c, ebp)));
        EMIT(og_fdiv(gc, MEMR(0x4c, ebp)));
        EMIT(og_add(gc, R(edx), R(edx)));
        EMIT(og_sbb(gc, R(esi), R(esi)));
        EMIT(og_mov(gc, R(ecx), MEMR(0x48, ebp)));
        EMIT(og_mov(gc, R(ebx), MEMR(0x44, ebp)));
        EMIT(og_mov(gc, R(eax), MEMR(0x40, ebp)));
        EMIT(og_adc(gc, R(ecx), R(ecx)));
        EMIT(og_adc(gc, R(esi), R(esi)));
        EMIT(og_add(gc, R(ebx), R(ebx)));
        EMIT(og_adc(gc, R(esi), R(esi)));
        EMIT(og_add(gc, R(eax), R(eax)));
        EMIT(og_adc(gc, R(esi), R(esi)));
        EMIT(og_cmp(gc, R(edx), R(ecx)));
        EMIT(og_adc(gc, R(ecx), R(ecx)));
        EMIT(og_cmp(gc, R(edx), R(ebx)));
        EMIT(og_adc(gc, R(ecx), R(ecx)));
        EMIT(og_cmp(gc, R(edx), R(eax)));
        EMIT(og_adc(gc, R(ecx), R(ecx)));
        EMIT(og_mov(gc, R(ebx), R(esi)));
        EMIT(og_and(gc, R(ebx), IMM(0xf)));
        EMIT(og_and(gc, R(ecx), IMM(0xf)));
        EMIT(og_mov(gc, R(ecx), TAB(g_clipAxisBits, ecx)));
        EMIT(og_mov(gc, R(esi), TAB(g_clipSideBits, ebx)));
        EMIT(og_and(gc, R(esi), R(ecx)));
        EMIT(og_mov(gc, R(ecx), TAB(g_clipWNegBits, ebx)));
        EMIT(og_or(gc, R(esi), R(ecx)));
        EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.color)));
        EMIT(og_mov(gc, R(ebx), MEM(&gc->state.current.color.g)));
        EMIT(og_mov(gc, R(ecx), MEM(&gc->state.current.color.b)));
        EMIT(og_mov(gc, R(edx), MEM(&gc->state.current.color.a)));
        EMIT(og_mov(gc, MEMR(0x60, ebp), R(eax)));
        EMIT(og_mov(gc, MEMR(100, ebp), R(ebx)));
        EMIT(og_mov(gc, MEMR(0x68, ebp), R(ecx)));
        EMIT(og_mov(gc, MEMR(0x6c, ebp), R(edx)));
        EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.packedColor)));
        EMIT(og_mov(gc, MEMR(0x14, ebp), R(eax)));
        EMIT(og_mov(gc, MEMR(0x10, ebp), R(esi)));
        EMIT(og_mov(gc, R(eax), MEM(&gc->hwcx.hwPrimDesc)));
        EMIT(og_mov(gc, R(ebx), MEMR(4, eax)));
        EMIT(og_mov(gc, R(edx), MEMR(0, eax)));
        EMIT(og_mov(gc, MEM(&gc->hwcx.hwPrimDesc), R(ebx)));
        EMIT(og_mov(gc, R(ecx), MEMR(0, ebx)));
        EMIT(og_and(gc, R(ecx), IMM(3)));
        EMIT(og_shr(gc, R(edx), IMM(2)));
        EMIT(og_mov(gc, R(ecx), TAB(&gc->hwcx.stagingVtx[0], ecx)));
        EMIT(og_mov(gc, MEM(&gc->hwcx.immVertex), R(ecx)));
    }
    return og_encode(gc, pc, og_ret(gc));
}

/* 69044ef0 -- og_genBegin: generates the hardware path's glBegin(mode): stores primMode; with beginMode 2 (validation
** pending) validates (69015850), clears beginMode and re-enters through the dispatch slot; otherwise
** GL_INVALID_OPERATION inside begin, GL_INVALID_ENUM for mode > GL_POLYGON; then begins the primitive (the
** staging vertex, hwPrim, its descriptor) and, when the immediate-proc class changed (hwcx.primClass),
** installs the lazy glVertex2fv / 3fv / 4fv builders */
GLubyte *__fastcall og_genBegin(__GLcontext *gc, GLubyte *pc)
{
    OgLabel *notInBegin = og_newLabel(gc);
    OgLabel *badEnum = og_newLabel(gc);
    OgLabel *inBegin = og_newFixup(gc);
    OgLabel *sameClass = og_newLabel(gc);

    EMIT(og_mov(gc, R(eax), MEMR(4, esp)));
    EMIT(og_mov(gc, MEM(&gc->fx.primMode), R(eax)));
    EMIT(og_mov(gc, R(eax), MEM(&gc->beginMode)));
    EMIT(og_cmp(gc, R(eax), IMM(0)));
    EMIT(og_je(gc, notInBegin));
    EMIT(og_cmp(gc, R(eax), IMM(2)));
    EMIT(og_jne(gc, inBegin));
    /* validation pending: validate, then dispatch glBegin again */
    EMIT(og_mov(gc, R(ecx), IMM(gc)));
    pc = og_callAbs(gc, pc, (GLuint)__glSSTValidate);
    EMIT(og_mov(gc, MEM(&gc->beginMode), IMM(0)));
    EMIT(og_jmp_rm(gc, MEM(&gc->currentDispatchState.fn[8])));

    LABEL(notInBegin);
    EMIT(og_mov(gc, R(eax), MEMR(4, esp)));
    EMIT(og_cmp(gc, R(eax), IMM(GL_POLYGON)));
    EMIT(og_ja(gc, badEnum));
    EMIT(og_mov(gc, MEM(&gc->beginMode), IMM(1)));
    EMIT(og_mov(gc, R(edx), IMM(gc->hwcx.stagingBase)));
    EMIT(og_mov(gc, MEM(&gc->hwcx.immVertex), R(edx)));
    EMIT(og_mov(gc, MEM(&gc->hwcx.hwPrim), R(eax)));
    EMIT(og_mov(gc, MEM(&gc->hwcx.stripKind), IMM(0)));            /* no FIFO strip is open (hwFifoStripTri) */
    EMIT(og_mov(gc, R(ecx), TAB(g_hwPrimDescByMode, eax)));
    EMIT(og_mov(gc, MEM(&gc->hwcx.hwPrimDesc), R(ecx)));
    EMIT(og_mov(gc, R(edx), TAB(g_immProcClassByMode, eax)));
    EMIT(og_mov(gc, R(ecx), MEM(&gc->hwcx.primClass)));
    EMIT(og_cmp(gc, R(edx), R(ecx)));
    EMIT(og_je(gc, sameClass));
    EMIT(og_mov(gc, MEM(&gc->hwcx.primClass), R(edx)));
    if (gc->modes.layerPlane) {
        EMIT(og_mov(gc, R(ecx), IMM(hwLazyVertex2fv)));
        EMIT(og_mov(gc, MEM(&gc->dispatchState->fn[130]), R(ecx)));     /* glVertex2fv */
        EMIT(og_mov(gc, R(ecx), IMM(hwLazyVertex3fv)));
        EMIT(og_mov(gc, MEM(&gc->dispatchState->fn[138]), R(ecx)));     /* glVertex3fv */
        EMIT(og_mov(gc, R(ecx), IMM(hwLazyVertex4fv)));
        EMIT(og_mov(gc, MEM(&gc->dispatchState->fn[146]), R(ecx)));     /* glVertex4fv */
    } else {
        EMIT(og_mov(gc, R(ecx), IMM(hwLazyVertex2fv)));
        EMIT(og_mov(gc, MEM(&gc->hwcx.immProcs[0]), R(ecx)));
        EMIT(og_mov(gc, R(ecx), IMM(hwLazyVertex3fv)));
        EMIT(og_mov(gc, MEM(&gc->hwcx.immProcs[1]), R(ecx)));
        EMIT(og_mov(gc, R(ecx), IMM(hwLazyVertex4fv)));
        EMIT(og_mov(gc, MEM(&gc->hwcx.immProcs[2]), R(ecx)));
    }
    LABEL(sameClass);
    EMIT(og_ret_imm(gc, IMM(4)));

    LABEL(inBegin);
    EMIT(og_mov(gc, R(ecx), IMM(GL_INVALID_OPERATION)));
    pc = og_callAbs(gc, pc, (GLuint)__glSetError);
    EMIT(og_ret_imm(gc, IMM(4)));

    LABEL(badEnum);
    EMIT(og_mov(gc, R(ecx), IMM(GL_INVALID_ENUM)));
    pc = og_callAbs(gc, pc, (GLuint)__glSetError);
    return og_encode(gc, pc, og_ret_imm(gc, IMM(4)));
}

/* 69047d40 -- og_cacheCompare: bsearch comparator of the generated code: the key bytes against the entry's data,
** over the length in the entry's descriptor (+0x0c -> +0x08) */
int __cdecl og_cacheCompare(const void *key, const void *elem)
{
    const GLuint *e = *(const GLuint * const *)elem;

    return memcmp(key, (const void *)e[0], ((const GLuint *)e[3])[2]);
}

/* 69047e30 -- og_buildLazyProc -- compiles the lazy draw procs of slot n (bufRec[6] cache keyed
** by og_lazyProcKey) and stores their entry points in gc->hwcx.ogLazyProcs. */
void __fastcall og_buildLazyProc(__GLcontext *gc, int n)
{
    GLuint *rec;
    int i;
    GLuint key[6];
    GLubyte *pc;
    __GLproc *lp;

    og_lazyProcKey(gc, key, n);
    if (!((GLint (__fastcall *)(__GLcontext *, __GLhwBufRecICD *, GLuint *, GLuint **))gc->hwcx.bufRec[6].alloc)
            (gc, &gc->hwcx.bufRec[6], key, &rec)) {
        pc = hwPageCacheInsert(gc, &gc->hwcx.bufRec[6], key, &rec);
        og_reset(gc);
        rec[0] = (GLuint)pc;
        EMIT(og_mov(gc, R(ecx), MEM(&gc->transform.modelView)));
        EMIT(og_add(gc, R(ecx), IMM(0)));
        pc = og_genMatrixVertex(gc, pc, (GLuint *)&gc->transform.modelView->matrix, n);
        rec[1] = (GLuint)pc;
        EMIT(og_mov(gc, R(ecx), MEM(&gc->transform.modelView)));
        EMIT(og_add(gc, R(ecx), IMM(0xc0)));
        pc = og_genMatrixVertex(gc, pc, (GLuint *)&gc->transform.modelView->mvp, n);
        rec[2] = (GLuint)pc;
        EMIT(og_mov(gc, R(ecx), MEM(&gc->transform.modelView)));
        EMIT(og_add(gc, R(ecx), IMM(0x60)));
        pc = og_genMatrixVertex(gc, pc, (GLuint *)&gc->transform.modelView->inverseTranspose, 3);
        for (i = 0; i < 2; i++) {
            rec[3 + i] = (GLuint)pc;
            EMIT(og_mov(gc, R(ecx), MEM(&gc->transform.texture[i])));
            pc = og_genMatrixVertex(gc, pc, (GLuint *)&gc->transform.texture[i]->matrix, 4);
        }
    }
    gc->hwcx.ogLazyProcs[3 + n] = (__GLproc)rec[0];
    gc->hwcx.ogLazyProcs[n] = (__GLproc)rec[1];
    gc->hwcx.ogLazyProcs[8] = (__GLproc)rec[2];
    lp = &gc->hwcx.ogLazyProcs[10];
    for (i = 0; i < 2; i++) {
        lp[-1] = lp[0] = lp[1] = (__GLproc)rec[3 + i];
        lp += 3;
    }
}

/* 69048090 -- og_lazyProcKey: the bufRec[6] cache key of og_buildLazyProc: {n, the modelview's mvp and matrix procs
** (+0x104 / +0x44), 3 x 0x55555555} (key[3] stored by itself, key[4..5] by a 2-step loop: VC6 unrolls it
** with the address in eax) */
void __fastcall og_lazyProcKey(__GLcontext *gc, GLuint *key, GLuint n)
{
    int i;

    key[0] = n;
    key[1] = gc->transform.modelView->mvp.elementClass;
    key[2] = gc->transform.modelView->matrix.elementClass;
    key[3] = 0x55555555;
    for (i = 4; i < 6; i++)
        key[i] = 0x55555555;
}

/* the source-vector masks g_vtxSizeMask(size) (icdglob.h), 2 bits per component (00 zero, 01 any, 11 one) as
** 69048c70 reads them: size 2 0xc5 (z 0, w 1), 3 0xd5 (w 1), 4 0x55 */

/*
** 690480d0 -- og_genMatrixVertex: generated code: transforms one vertex (eax = [esp + 4]) by the 4x4 matrix at ecx
** (`m` is the same matrix now, for its element classes) into [edx]: with 3DNow! (pfmul / pfadd, `size` = 2..4
** source components) when the CPU has it, else four x87 row dot products (69048c70) stored w, z, y, x.
** 69047d70 first marks every element of `m` general (so no term is skipped).  The three 3DNow! arms end
** alike (VC6 merges their tails).  Returns the new output position (eax of the last og_encode).
*/
GLubyte *__fastcall og_genMatrixVertex(__GLcontext *gc, GLubyte *pc, GLuint *m, GLint size)
{
    GLuint mask;

    mask = g_vtxSizeMask(size);
    hwClassifyMatrix(gc, (__GLmatrixICD *)m);
    EMIT(og_mov(gc, R(eax), MEMR(4, esp)));
    if (*(GLubyte *)&gc->constants.cpuCaps & __GL_CPU_3DNOW) {           /* 3DNow! */
        EMIT(og_femms(gc));
        if (size == 2) {
            EMIT(og_movd(gc, R(mm0), MEM0(eax)));
            EMIT(og_movd(gc, R(mm2), MEMR(4, eax)));
            EMIT(og_punpckldq(gc, R(mm0), R(mm0)));
            EMIT(og_punpckldq(gc, R(mm2), R(mm2)));
            EMIT(og_movq(gc, R(mm1), R(mm0)));
            EMIT(og_movq(gc, R(mm3), R(mm2)));
            EMIT(og_pfmul(gc, R(mm0), MEM0(ecx)));
            EMIT(og_pfmul(gc, R(mm1), MEMR(8, ecx)));
            EMIT(og_pfmul(gc, R(mm2), MEMR(0x10, ecx)));
            EMIT(og_pfmul(gc, R(mm3), MEMR(0x18, ecx)));
            EMIT(og_pfadd(gc, R(mm2), MEMR(0x30, ecx)));
            EMIT(og_pfadd(gc, R(mm3), MEMR(0x38, ecx)));
            EMIT(og_pfadd(gc, R(mm0), R(mm2)));
            EMIT(og_pfadd(gc, R(mm1), R(mm3)));
            EMIT(og_movq(gc, MEMR(0, edx), R(mm0)));
            EMIT(og_movq(gc, MEMR(8, edx), R(mm1)));
        } else if (size == 3) {
            EMIT(og_movd(gc, R(mm0), MEM0(eax)));
            EMIT(og_movd(gc, R(mm2), MEMR(4, eax)));
            EMIT(og_movd(gc, R(mm4), MEMR(8, eax)));
            EMIT(og_punpckldq(gc, R(mm0), R(mm0)));
            EMIT(og_punpckldq(gc, R(mm2), R(mm2)));
            EMIT(og_punpckldq(gc, R(mm4), R(mm4)));
            EMIT(og_movq(gc, R(mm1), R(mm0)));
            EMIT(og_movq(gc, R(mm3), R(mm2)));
            EMIT(og_movq(gc, R(mm5), R(mm4)));
            EMIT(og_pfmul(gc, R(mm0), MEM0(ecx)));
            EMIT(og_pfmul(gc, R(mm1), MEMR(8, ecx)));
            EMIT(og_pfmul(gc, R(mm2), MEMR(0x10, ecx)));
            EMIT(og_pfmul(gc, R(mm3), MEMR(0x18, ecx)));
            EMIT(og_pfmul(gc, R(mm4), MEMR(0x20, ecx)));
            EMIT(og_pfmul(gc, R(mm5), MEMR(0x28, ecx)));
            EMIT(og_pfadd(gc, R(mm0), MEMR(0x30, ecx)));
            EMIT(og_pfadd(gc, R(mm1), MEMR(0x38, ecx)));
            EMIT(og_pfadd(gc, R(mm2), R(mm4)));
            EMIT(og_pfadd(gc, R(mm3), R(mm5)));
            EMIT(og_pfadd(gc, R(mm0), R(mm2)));
            EMIT(og_pfadd(gc, R(mm1), R(mm3)));
            EMIT(og_movq(gc, MEMR(0, edx), R(mm0)));
            EMIT(og_movq(gc, MEMR(8, edx), R(mm1)));
        } else if (size == 4) {
            EMIT(og_movd(gc, R(mm0), MEM0(eax)));
            EMIT(og_movd(gc, R(mm2), MEMR(4, eax)));
            EMIT(og_movd(gc, R(mm4), MEMR(8, eax)));
            EMIT(og_movd(gc, R(mm6), MEMR(0xc, eax)));
            EMIT(og_punpckldq(gc, R(mm0), R(mm0)));
            EMIT(og_punpckldq(gc, R(mm2), R(mm2)));
            EMIT(og_punpckldq(gc, R(mm4), R(mm4)));
            EMIT(og_punpckldq(gc, R(mm6), R(mm6)));
            EMIT(og_movq(gc, R(mm1), R(mm0)));
            EMIT(og_movq(gc, R(mm3), R(mm2)));
            EMIT(og_movq(gc, R(mm5), R(mm4)));
            EMIT(og_movq(gc, R(mm7), R(mm6)));
            EMIT(og_pfmul(gc, R(mm0), MEM0(ecx)));
            EMIT(og_pfmul(gc, R(mm1), MEMR(8, ecx)));
            EMIT(og_pfmul(gc, R(mm2), MEMR(0x10, ecx)));
            EMIT(og_pfmul(gc, R(mm3), MEMR(0x18, ecx)));
            EMIT(og_pfmul(gc, R(mm4), MEMR(0x20, ecx)));
            EMIT(og_pfmul(gc, R(mm5), MEMR(0x28, ecx)));
            EMIT(og_pfmul(gc, R(mm6), MEMR(0x30, ecx)));
            EMIT(og_pfmul(gc, R(mm7), MEMR(0x38, ecx)));
            EMIT(og_pfadd(gc, R(mm0), R(mm2)));
            EMIT(og_pfadd(gc, R(mm1), R(mm3)));
            EMIT(og_pfadd(gc, R(mm0), R(mm4)));
            EMIT(og_pfadd(gc, R(mm1), R(mm5)));
            EMIT(og_pfadd(gc, R(mm0), R(mm6)));
            EMIT(og_pfadd(gc, R(mm1), R(mm7)));
            EMIT(og_movq(gc, MEMR(0, edx), R(mm0)));
            EMIT(og_movq(gc, MEMR(8, edx), R(mm1)));
        }
        EMIT(og_femms(gc));
    } else {
        pc = og_genMatrixRow(gc, pc, 0, m, mask);
        pc = og_genMatrixRow(gc, pc, 1, m, mask);
        pc = og_genMatrixRow(gc, pc, 2, m, mask);
        pc = og_genMatrixRow(gc, pc, 3, m, mask);
        EMIT(og_fstp(gc, MEMR(0xc, edx)));
        EMIT(og_fstp(gc, MEMR(8, edx)));
        EMIT(og_fstp(gc, MEMR(4, edx)));
        EMIT(og_fstp(gc, MEMR(0, edx)));
    }
    return og_encode(gc, pc, og_ret_imm(gc, IMM(4)));
}

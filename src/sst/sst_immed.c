/*
** sst_immed.c -- the immediate-mode path: the lazy glVertex procs, glBegin, the vertex-proc keys and the
** per-vertex texture coordinates.
*/
#include "wglpriv.h"
#include "og.h"
#include "glsi.h"
#include "glshade.h"
#include "icdglob.h"
#include "icdprotos.h"
#include <glide.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "si_names.h"
#include <math.h>

#define MATRIX(t, off)  ((__GLmatrixICD *)((GLubyte *)(t) + (off)))
/* (moved from sst_vtxcopy.c) */
#define VTX_TEX(u)      (0x20 + (u) * 0x10)         /* __GLvertex texture[u] relative to the vertex in ebp */
typedef void (APIENTRY *__GLVertexfvProc)(const GLfloat *v);
#define g_hwPrimDescLineLoop    ((GLuint)&g_hwPrimDescs[3])  /* 690efde8: GL_LINE_LOOP's descriptor
                                                                             ** chain (sst_lazyprocs.c) */
typedef void (__fastcall *__GLVertexProc)(__GLcontext *gc, __GLvertexICD *vx);
typedef void (__fastcall *XF_6905ecc0)(__GLcoord *, __GLcoord *, void *);

/* 69021c70 (size 298) -- hwVertexProcKey: the key of the generated vertex procs: [0] primitive, matrix type,
** primitive class, fog, colour sum, big points and q / mode bits, [1] four bits per TMU, [2] face needs; returns its
** size */
GLuint __fastcall hwVertexProcKey(__GLcontext *gc, GLuint *key, GLint prim)
{
    GLuint primClass, needs, general, k1;
    GLboolean bigPoint;
    GLint i;

    primClass = gc->hwcx.primClass;     /* g_immProcClassByMode[prim]: 0 points, 1 lines, 2 triangles, 3 quads */
    needs = (*gc->vertex.getFaceNeeds0)(gc) | (*gc->vertex.getNeeds)(gc);
    bigPoint = !primClass && gc->state.point.requestedSize != 1.0f;
    general = gc->state.enables.general;
    key[0] = (((GLuint)(*gc->polygon.shader.getModeFlags)(gc) >> 7) & 0x400)
           | ((((((((GLuint)gc->hwcx.nonUnityQ << 1 | (GLuint)bigPoint) << 5 | (general & __GL_CULL_FACE_ENABLE)) << 1
                | (general & __GL_FOG_ENABLE)) << 2 | (general & __GL_LIGHTING_ENABLE) | primClass) << 3)
              | MATRIX(gc->transform.modelView, 0xc0)->matrixType) << 2)
           | (prim - 2);
    k1 = 0;
    for (i = 0; i < gc->fx.numTMUs; i++) {
        k1 <<= 4;
        if (gc->texture.textureEnabled[i])
            k1 |= (((GLuint)((gc->state.enables.texture[i] & __GL_TEXTURE_GEN_ANY_ENABLE) != 0) << 1
                    | (GLuint)(MATRIX(gc->transform.texture[i], 0)->matrixType != 4)) << 1) | 1;
    }
    key[1] = k1;
    key[2] = needs & 0x18;
    return 0xc;
}

/* 690221e0 -- og_texCoordXform: one unit's texture coordinate in the vertex [ebp]: through the texture matrix and
   into Glide's range (scaled by texScale) unless the matrix is the identity, where only a
   non-unity q is copied */
GLubyte *__fastcall og_texCoordXform(__GLcontext *gc, GLubyte *pc, int unit)
{
    if (((__GLmatrixICD *)gc->transform.texture[unit])->matrixType == 4) {
        if (gc->hwcx.nonUnityQ) {
            EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.texture[unit].w)));
            EMIT(og_mov(gc, MEMR(VTX_TEX(unit) + 0xc, ebp), R(eax)));
        }
    } else {
        EMIT(og_push(gc, R(edx)));
        EMIT(og_mov(gc, R(eax), IMM(&gc->state.current.texture[unit])));
        EMIT(og_push(gc, R(eax)));
        EMIT(og_lea(gc, R(edx), MEMR(VTX_TEX(unit), ebp)));
        EMIT(og_mov(gc, R(ecx), IMM(gc)));
        EMIT(og_call(gc, MEM(&gc->hwcx.ogLazyProcs[11 + unit * 3])));
        EMIT(og_pop(gc, R(edx)));
        if (gc->constants.cpuCaps & 2) {
            EMIT(og_femms(gc));
            EMIT(og_movq(gc, R(mm0), MEMR(VTX_TEX(unit), ebp)));
            EMIT(og_pfmul(gc, R(mm0), MEM(&gc->hwcx.texScale[unit][0])));
            EMIT(og_movq(gc, MEMR(VTX_TEX(unit), ebp), R(mm0)));
            EMIT(og_femms(gc));
        } else {
            EMIT(og_fld(gc, MEMR(VTX_TEX(unit), ebp)));
            EMIT(og_fld(gc, MEM(&gc->hwcx.texScale[unit][0])));
            EMIT(og_fmulp(gc, R(st1), R(st0)));
            EMIT(og_fld(gc, MEMR(VTX_TEX(unit) + 4, ebp)));
            EMIT(og_fld(gc, MEM(&gc->hwcx.texScale[unit][1])));
            EMIT(og_fmulp(gc, R(st1), R(st0)));
            EMIT(og_fxch(gc, R(st1)));
            EMIT(og_fstp(gc, MEMR(VTX_TEX(unit), ebp)));
            EMIT(og_fstp(gc, MEMR(VTX_TEX(unit) + 4, ebp)));
        }
    }
    return pc;
}

/* 690262e0 -- hwLazyVertex2fv: the lazy glVertex2fv: builds the hardware routine for 2 components
** (hwcx.buildImmProc) on first use inside glBegin, installs it (a layer context's dispatch table, else
** hwcx.immProcs[0]) and calls it */
void __stdcall hwLazyVertex2fv(const GLfloat *v)
{
    __GL_SETUP();

    if (gc->hwcx.hwPrim != -1) {
        if (gc->modes.layerPlane) {
            gc->dispatchState->fn[130] = (__GLdispatchFn)(*gc->hwcx.buildImmProc)(gc, 2);
        }
        else {
            gc->hwcx.immProcs[0] = (__GLproc)(*gc->hwcx.buildImmProc)(gc, 2);
        }
        (*(__GLVertexfvProc)gc->dispatchState->fn[130])(v);
    }
}

/* 69026350 -- hwLazyVertex3fv: the lazy glVertex3fv: builds the hardware routine for 3 components
** (hwcx.buildImmProc) on first use inside glBegin, installs it (a layer context's dispatch table, else
** hwcx.immProcs[1]) and calls it */
void __stdcall hwLazyVertex3fv(const GLfloat *v)
{
    __GL_SETUP();

    if (gc->hwcx.hwPrim != -1) {
        if (gc->modes.layerPlane) {
            gc->dispatchState->fn[138] = (__GLdispatchFn)(*gc->hwcx.buildImmProc)(gc, 3);
        }
        else {
            gc->hwcx.immProcs[1] = (__GLproc)(*gc->hwcx.buildImmProc)(gc, 3);
        }
        (*(__GLVertexfvProc)gc->dispatchState->fn[138])(v);
    }
}

/* 690263c0 -- hwLazyVertex4fv: the lazy glVertex4fv: builds the hardware routine for 4 components
** (hwcx.buildImmProc) on first use inside glBegin, installs it (a layer context's dispatch table, else
** hwcx.immProcs[2]) and calls it */
void __stdcall hwLazyVertex4fv(const GLfloat *v)
{
    __GL_SETUP();

    if (gc->hwcx.hwPrim != -1) {
        if (gc->modes.layerPlane) {
            gc->dispatchState->fn[146] = (__GLdispatchFn)(*gc->hwcx.buildImmProc)(gc, 4);
        }
        else {
            gc->hwcx.immProcs[2] = (__GLproc)(*gc->hwcx.buildImmProc)(gc, 4);
        }
        (*(__GLVertexfvProc)gc->dispatchState->fn[146])(v);
    }
}

/* 69026430 -- hwInstallInlineImmProcs: the stand-alone context's glVertex2fv / 3fv / 4fv, glBegin and glEnd: a `jmp
** [hwcx.immProcs[i]]` each in hwcx.inlineCode, installed in the current dispatch table, which is then re-copied onto
** itself (__glCopyDispatch passes it on to wglRcCallback) */
void __fastcall hwInstallInlineImmProcs(__GLcontext *gc)
{
    GLubyte *pc = &gc->hwcx.inlineCode[0];

    og_reset(gc);
    gc->currentDispatchState.fn[130] = (__GLdispatchFn)pc;         /* glVertex2fv */
    EMIT(og_jmp_rm(gc, MEM(&gc->hwcx.immProcs[0])));
    gc->currentDispatchState.fn[138] = (__GLdispatchFn)pc;         /* glVertex3fv */
    EMIT(og_jmp_rm(gc, MEM(&gc->hwcx.immProcs[1])));
    gc->currentDispatchState.fn[146] = (__GLdispatchFn)pc;         /* glVertex4fv */
    EMIT(og_jmp_rm(gc, MEM(&gc->hwcx.immProcs[2])));
    gc->currentDispatchState.fn[8] = (__GLdispatchFn)pc;           /* glBegin */
    EMIT(og_jmp_rm(gc, MEM(&gc->hwcx.immProcs[3])));
    gc->currentDispatchState.fn[44] = (__GLdispatchFn)pc;          /* glEnd */
    og_encode(gc, pc, og_jmp_rm(gc, MEM(&gc->hwcx.immProcs[4])));
    __glCopyDispatchTable(&__glCurrentGC()->currentDispatchState, &__glCurrentGC()->currentDispatchState);
}

/* 69026590 -- hwPatchVertex3fvStub: (unreferenced) patches the glVertex3fv stub into `jmp
** [dispatchState->fn[138]]`: the page made writable, the jump encoded, then PAGE_EXECUTE from the end of the jump
** (not the stub's start) */
void __fastcall hwPatchVertex3fvStub(__GLcontext *gc)
{
    DWORD oldProtect;
    GLubyte *end;

    VirtualProtect(glVertex3fv, 0x1000, PAGE_READWRITE, &oldProtect);
    end = og_encode(gc, (GLubyte *)glVertex3fv, og_jmp_rm(gc, MEM(&gc->dispatchState->fn[138])));
    VirtualProtect(end, 0x1000, PAGE_EXECUTE, &oldProtect);
}

/* 69026ea0 -- hwEndLineLoop of a hardware GL_LINE_LOOP: the closing line from the last staged vertex (the descriptor
** chain of GL_LINE_LOOP starts at 690efde8: the 3rd entry -> staging vertex 1, the 4th -> vertex 2) back to
** vertex 0, through the line clipper from the top clip plane; flat shaded, the last vertex takes vertex 0's
** packed colour for it.  v stays uninitialised for a later entry, as in the original */
void __fastcall hwEndLineLoop(__GLcontext *gc)
{
    __GLvertexICD *v;
    GLuint save;

    if (gc->hwcx.hwPrim == GL_LINE_LOOP) {
        switch ((GLint)(gc->hwcx.hwPrimDesc - g_hwPrimDescLineLoop) >> 3) {
        case 0:
        case 1:
            return;
        case 2:
            v = &gc->hwcx.stagingBase[1];
            break;
        case 3:
            v = &gc->hwcx.stagingBase[2];
            break;
        }
        if (v != NULL) {
            if ((__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_SMOOTH_LIGHT) == 0) {       /* flat */
                save = v->packedColor;
                v->packedColor = gc->hwcx.stagingBase->packedColor;
                __glSSTClipAndDrawLine(gc, 0x15, v, gc->hwcx.stagingBase);
                v->packedColor = save;
            }
            else {
                __glSSTClipAndDrawLine(gc, 0x15, v, gc->hwcx.stagingBase);
            }
        }
    }
}

/* 69026fd0 (size 138): __glsstim_Begin of the hardware path */
void __stdcall __glsstim_Begin(GLenum mode)
{
    __GLcontext *gc = __glCurrentGC();

    gc->fx.primMode = mode;
    if (gc->beginMode != 0) {
        if (gc->beginMode == 2) {
            (*gc->procs.validate)(gc);
            gc->beginMode = 0;
            ((void (__stdcall *)(GLenum))gc->currentDispatchState.fn[8])(mode);
            return;
        }
        __glSetError(GL_INVALID_OPERATION);
        return;
    }
    if (mode > 9) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->beginMode = 1;
    (*gc->procs.beginPrim[mode])(gc);
}

/* 6905f470 -- hwTexCoordsXform0: as 6905f4f0 with unit 0's texture proc always called */
void __fastcall hwTexCoordsXform0(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLtextureFxICD *tex;
    GLint i;

    (*(__GLVertexProc)gc->procs.calcTextureUnit[0])(gc, vx);
    for (i = 0; i < gc->fx.numTMUs; i++) {
        tex = (__GLtextureFxICD *)__glCurrentTexture(gc, i);
        if (!(vx->flagBits & __GL_CLIP_NEAR)) {     /* not behind the eye: project */
            vx->texture[i].x = vx->texture[i].x * vx->window.w;
            vx->texture[i].y = vx->texture[i].y * vx->window.w;
            vx->texture[i].w = vx->texture[i].w * vx->window.w;
        }
        if (tex) {
            vx->texture[i].x = tex->texScale[0] * vx->texture[i].x;
            vx->texture[i].y = tex->texScale[1] * vx->texture[i].y;
        }
    }
}

/* 6905f4f0 -- hwTexCoords: the vertex's texture coordinates for Glide: unit 0's texture proc when a texture matrix is
** not the identity, then per TMU s, t, q times window.w (unless behind the near plane) and s, t times the
** texture's Glide scale.  Indexed vx->texture[i] (strength-reduced) gives the original fld/fmul order */
void __fastcall hwTexCoords(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLtextureFxICD *tex;
    GLint i;

    if (gc->transform.texture[0]->matrix.matrixType != __GL_MT_IDENTITY ||
        gc->transform.texture[1]->matrix.matrixType != __GL_MT_IDENTITY) {
        (*(__GLVertexProc)gc->procs.calcTextureUnit[0])(gc, vx);
    }
    for (i = 0; i < gc->fx.numTMUs; i++) {
        tex = (__GLtextureFxICD *)__glCurrentTexture(gc, i);
        if (!(vx->flagBits & __GL_CLIP_NEAR)) {     /* not behind the eye: project */
            vx->texture[i].x = vx->texture[i].x * vx->window.w;
            vx->texture[i].y = vx->texture[i].y * vx->window.w;
            vx->texture[i].w = vx->texture[i].w * vx->window.w;
        }
        if (tex) {
            vx->texture[i].x = tex->texScale[0] * vx->texture[i].x;
            vx->texture[i].y = tex->texScale[1] * vx->texture[i].y;
        }
    }
}

/* 6905ecc0 (size 105) -- hwXformTexCoords: transforms vertex texture[0] / texture[1] through the per-unit texture
** transform's proc at +0x54 (dst, src copy, transform); the copy is a member-wise __GLcoord local
** (this gives the original's 0x10-byte frame). */
void __fastcall hwXformTexCoords(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLcoord tc;

    tc.x = vx->texture[0].x; tc.y = vx->texture[0].y; tc.z = vx->texture[0].z; tc.w = vx->texture[0].w;
    (*(XF_6905ecc0 *)&gc->transform.texture[0]->matrix.xf4)(&vx->texture[0], &tc, gc->transform.texture[0]);
    tc.x = vx->texture[1].x; tc.y = vx->texture[1].y; tc.z = vx->texture[1].z; tc.w = vx->texture[1].w;
    (*(XF_6905ecc0 *)&gc->transform.texture[1]->matrix.xf4)(&vx->texture[1], &tc, gc->transform.texture[1]);
}

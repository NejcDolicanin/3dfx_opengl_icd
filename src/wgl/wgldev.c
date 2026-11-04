/*
** wgldev.c -- the 3dfx "device" side of the window layer: context creation, the
** gc->exports callbacks (makeCurrent, loseCurrent, notifyResize, ...), the switch between
** hardware and software rendering, and the device callback table (690eee48).
** Ancestor: GLIDE3X/SST/sst_export.c of the 3dfx
** ICD source (ref/3dfx-opengl), which has CheckViewport .. MakeCurrent in the same order.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wglpriv.h"
#include <glide.h>
#include "icdprotos.h"
#include "glsi.h"
#include "siprotos.h"
#include "icdglob.h"
#include "gldisp.h"

/* 690f66ac -- swaps counted while glowWorm == 4 */
GLuint g_swapDumpCount = 0;

/* functions of other files this one calls or installs in the proc tables (their definitions' signatures) */
/* gl_accum.c */
/* gl_begin.c */
/* gl_bitmap.c */
/* gl_buffer.c */
/* gl_ci.c */
/* gl_clip.c */
/* gl_context.c */
/* gl_ctable.c */
/* gl_dlist.c */
/* gl_export.c */
/* gl_light.c */
/* gl_pgclip.c */
/* gl_pick.c */
/* gl_pickspan.c */
/* gl_pxapi.c */
/* gl_pxfast.c */
/* gl_pxpaths.c */
/* gl_rasterpos.c */
/* gl_stencil.c */
/* gl_store.c */
/* gl_texextract.c */
/* gl_texstate.c */
/* gl_vertex.c */
/* gl_xform.c */
/* og_util.c */
/* si_px_read.c */
/* si_px_rend.c */
/* si_s_eval.c */
/* si_s_image.c */
/* si_s_math.c */
/* si_s_rect.c */
/* si_s_xform.c */
/* sst_buffers.c */
/* sst_codebuf.c */
/* sst_color.c */
/* sst_depth.c */
/* sst_grstate.c */
/* sst_immed.c */
/* sst_lazyprocs.c */
/* sst_pick.c */
/* sst_rgb.c */
/* sst_settings.c */
/* sst_tex.c */
/* sst_texmgr.c */
/* sst_validate.c */
/* sst_varray.c */
/* sststate.c */
/* sstsurf.c */

typedef char (__fastcall *GCfn_bufresize)(void *buf, GLint x, GLint y, GLuint w, GLuint h, GLuint which);   /* buf.resize */

/* gc->mcFlags: the SI gcState bits __GL_INIT_HW / __GL_HW_MODE / __GL_PIXMAP_MODE (glsi.h) */
#define __GL_DIRTY_ALL_ICD          0xffffffff     /* every dirty bit (the SI's __GL_DIRTY_ALL is 0x7ff) */

/* ---- callees in the sst/ and gl/ modules ---- */
extern int __fastcall __glGenericDlistOptimizer();      /* 6907f550: jmp __glDlistOptimizeMaterial */

/* debug trace, empty in release (69011d20) */
void __cdecl wglDevTrace(const char *fmt, ...) { (void)fmt; }

/* 6901369d -- procs.error: installed unless GL_IGNORE_API_ERRORS is set */
void __fastcall wglDevApiError(__GLcontext *gc, GLenum code)
{
    wglDevTrace("OpenGL API error, code: 0x%x", code);
}

/* ------------------------------------------------------------ dispatch hooks */
/* 69012060 -- procs.waitIdle (hw): flush before reading back from a windowed front buffer */
void __fastcall wglDevFinishFront(__GLcontext *gc)
{
    if (GC_imports_wscx(gc)->isCurrent && gc->state.raster.drawBuffer == GL_FRONT && gc->hwcx.windowed) {
        grFinish();
        winPresent(gc);
    }
}
/* 69011d30 -- procs.waitIdle (sw) */
void __fastcall wglDevFinishFrontSW(__GLcontext *gc)
{
    if (GC_imports_wscx(gc)->isCurrent && gc->state.raster.drawBuffer == GL_FRONT && gc->hwcx.windowed)
        winPresent(gc);
}

/* 69011e20 / 69011e90 / 69012170 / 690121e0 -- Flush / Finish (sst_export.c) variants: a windowed
** context presents the drawable when it renders to the front buffer */
static __inline void __fastcall devSyncWindowed(__GLcontext *gc)   /* inlined at every caller in the original */
{
    GLint mode;
    winPresent(gc);
    mode = gc->state.raster.drawBuffer;
    if (mode == GL_FRONT || mode == GL_FRONT_LEFT || mode == GL_LEFT || mode == GL_FRONT_AND_BACK)   /* the original tests GL_LEFT (0x406), not GL_FRONT_RIGHT */
        gc->hwcx.depthFlag = 1;             /* two stores in the original, not a materialised boolean */
    else
        gc->hwcx.depthFlag = 0;
}
/* 69011e20 -- wglDevFlushSW: a current windowed context drawing single-buffered or to the front presents */
void __fastcall wglDevFlushSW(__GLcontext *gc)                  /* 69011e20: procs.finish (sw) */
{
    if (GC_imports_wscx(gc)->isCurrent && gc->hwcx.windowed &&
        (!gc->modes.doubleBufferMode || gc->hwcx.depthFlag))
        devSyncWindowed(gc);
}
/* 69011e90 -- wglDevFlushSW2: the same body as wglDevFlushSW (procs.flush, where that one is procs.finish) */
void __fastcall wglDevFlushSW2(__GLcontext *gc)                 /* 69011e90: procs.flush (sw), a second identical copy */
{
    if (GC_imports_wscx(gc)->isCurrent && gc->hwcx.windowed &&
        (!gc->modes.doubleBufferMode || gc->hwcx.depthFlag))
        devSyncWindowed(gc);
}
/* 69012170 -- wglDevFinish: when current, grFinish, then present as wglDevFlushSW does */
void __fastcall wglDevFinish(__GLcontext *gc)                   /* 69012170: procs.finish (hw) */
{
    if (GC_imports_wscx(gc)->isCurrent) {
        grFinish();
        if (gc->hwcx.windowed && (!gc->modes.doubleBufferMode || gc->hwcx.depthFlag))
            devSyncWindowed(gc);
    }
}
/* 690121e0 -- wglDevFlush: the hardware glFlush, for a current context only: windowed, grFinish and then, when
** single-buffered or hwcx.depthFlag (front-buffer rendering) is set, winPresent and update that flag from the draw
** buffer; full-screen, grFlush if Glide is open */
void __fastcall wglDevFlush(__GLcontext *gc)                    /* 690121e0: procs.flush (hw) */
{
    if (GC_imports_wscx(gc)->isCurrent) {
        if (gc->hwcx.windowed) {
            grFinish();
            if (!gc->modes.doubleBufferMode || gc->hwcx.depthFlag) devSyncWindowed(gc);
        } else {
            if (gc->fx.glideOpen) grFlush();
        }
    }
}
/* 690120a0 -- wglDevNop1: the hardware lockDevice: nothing to do */
void __fastcall wglDevNop1(void) {}     /* 690120a0: buffers.lockDevice (hw) */
/* 690120b0 -- wglDevNop2: the hardware unlockDevice: nothing to do */
void __fastcall wglDevNop2(void) {}     /* 690120b0: buffers.unlockDevice (hw) */

/* 69011d80 -- CheckViewport (sst_export.c): is the viewport fully inside the window clip box? */
void __fastcall wglDevCheckViewportInside(__GLcontext *gc)
{
    GLint llx = gc->constants.viewportXAdjust + gc->state.viewport.x;
    GLint lly = gc->constants.viewportYAdjust + gc->state.viewport.y;
    GLboolean oldReasonableViewport = gc->transform.reasonableViewport;
    if (llx < gc->transform.clipX0 || lly < gc->transform.clipY0 ||
        gc->state.viewport.width + llx > gc->transform.clipX1 ||
        gc->state.viewport.height + lly > gc->transform.clipY1)
        gc->transform.reasonableViewport = GL_FALSE;
    else
        gc->transform.reasonableViewport = GL_TRUE;
    if (oldReasonableViewport != gc->transform.reasonableViewport) {
        __glCurrentGC()->beginMode = __GL_NEED_VALIDATE;     /* __GL_DELAY_VALIDATE */
        gc->dirtyMask |= __GL_DIRTY_GENERIC;
    }
}
/* 69011d60 -- procs.applyScissor (sw): only the HAL_CSIM window-size check */
GLboolean __fastcall wglDevCheckViewportSW(__GLcontext *gc)
{
    if (__glFindWindowSize(gc) == 1) { wglDevCheckViewportInside(gc); return GL_TRUE; }
    return GL_FALSE;
}
/* 690120c0 -- ApplyScissor (sst_export.c), procs.applyScissor (hw): the Glide clip window from the
** window clip box, clamped to the screen when fullscreen (the ancestor used the scissor box) */
GLboolean __fastcall wglDevClipWindow(__GLcontext *gc)
{
    GLboolean sim = GL_FALSE;
    __WGLrc *rc = GC_imports_wscx(gc);
    GLint x0, y0, x1, y1;
    if (__glFindWindowSize(gc) == 1) { wglDevCheckViewportInside(gc); sim = GL_TRUE; }   /* __glFindWindowSize */
    x0 = gc->transform.clipX0; x1 = gc->transform.clipX1;
    y0 = gc->transform.clipY0; y1 = gc->transform.clipY1;
    if (!gc->hwcx.windowed) {
        if (x1 > gc->constants.maxViewportWidth)  x1 = gc->constants.maxViewportWidth;
        if (y1 > gc->constants.maxViewportHeight) y1 = gc->constants.maxViewportHeight;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
    }
    if (rc->window->dp.childWindow) wglWindowResized(rc->window);
    grClipWindow(x0, y0, x1, y1);
    return sim;
}
/* 69013470 -- ApplyViewport (sst_export.c), procs.applyViewport: keeps the raster position
** where it was on screen when the window height changes; TRUE when checked under HAL_CSIM */
GLboolean __fastcall wglDevViewportChanged(__GLcontext *gc)
{
    GLboolean sim = GL_FALSE;
    GLfloat height = (GLfloat)(GLuint)gc->constants.height;
    if (__glFindWindowSize(gc) == 1) {                   /* __glFindWindowSize */
        __glUpdateViewport(gc);                               /* __glUpdateViewport */
        wglDevCheckViewportInside(gc);
        sim = GL_TRUE;
    }
    if (gc->constants.yInverted) {
        GLfloat newHeight = (GLfloat)(GLuint)gc->constants.height;
        if (height != newHeight) gc->state.current.rasterPos.window.y += newHeight - height;
    }
    return sim;
}

/* 6901239d -- screen-saver detection by module name */
void __fastcall wglDevDetectScreenSaver(__GLcontext *gc)
{
    HWND hwnd = GC_imports_wscx(gc)->window->hwnd;
    HMODULE h = (HMODULE)GetWindowLongA(hwnd, GWL_HINSTANCE);
    CHAR name[256];
    GetModuleFileNameA(h, name, 0xff);
    if (strstr(name, ".SCR") != NULL || strstr(name, ".scr") != NULL) gc->fx.isScreenSaver = 1;
    else                                                              gc->fx.isScreenSaver = 0;
}

/* 69012260 -- create the code-page caches (hwcx.bufRec[]: entries, key size, arena sizes) */
void __fastcall wglDevInitPools(__GLcontext *gc)
{
    hwPageCacheInit(gc, 0, &gc->hwcx.bufRec[4], 0x20,   4,    0x760, 0x800);
    hwPageCacheInit(gc, 0, &gc->hwcx.bufRec[5], 0x10,   4,    0x800, 0);
    hwPageCacheInit(gc, 0, &gc->hwcx.bufRec[6], 0x40,   0x18, 0x800, 0);
    hwPageCacheInit(gc, 0, &gc->hwcx.bufRec[0], 0x1000, 0xc,  0xb00, 0);
    hwPageCacheInit(gc, 0, &gc->hwcx.bufRec[7], 0x20,   4,    0xf00, 0);
    hwPageCacheInit(gc, 0, &gc->hwcx.bufRec[9], 4,      4,    0xf00, 0);
    hwPageCacheInit(gc, 0, &gc->hwcx.bufRec[8], 0x800,  0x10, 0xa00, 0x100);
    hwPageCacheInit(gc, 0, &gc->hwcx.bufRec[2], 0x1000, 0x5c, 0xa00, 0);
    hwPageCacheInit(gc, 0, &gc->hwcx.bufRec[3], 0x20,   4,    0xd00, 0x100);
    hwPageCacheInit(gc, 0, &gc->hwcx.bufRec[1], 8,      0xc,  0xa00, 0);
    hwInitImmProcBuilder(gc);                                   /* install the immediate-mode proc builder */
}

/* 69012430 -- procs.nonUnityQ, "Had non unity Q": switch to a vertex layout carrying q */
void __fastcall wglDevNonUnityQ(__GLcontext *gc, void *arg)
{
    (void)arg;
    if (gc->hwcx.nonUnityQ == 0) {
        gc->hwcx.nonUnityQ = 1;
        gc->dirtyMask |= __GL_DIRTY_NONUNITYQ_ICD;
        wglDevTrace("Had non unity Q");
        switch (gc->fx.numTMUs) {
        case 1:
            grVertexLayout(GR_PARAM_Q0, 0x2c, GR_PARAM_ENABLE);
            break;
        case 2:
            grVertexLayout(GR_PARAM_Q0, 0x3c, GR_PARAM_ENABLE);
            grVertexLayout(GR_PARAM_Q1, 0x2c, GR_PARAM_ENABLE);
            break;
        default:
            grVertexLayout(GR_PARAM_Q0, 0x3c, GR_PARAM_ENABLE);
            grVertexLayout(GR_PARAM_Q1, 0x2c, GR_PARAM_ENABLE);
            break;
        }
        hwPickImmedProcs(gc);                               /* re-pick the immediate-mode procs */
    }
}

/* 69013620 -- wglDevAddRef: buffers.obtainSLock: count a share lock; nonzero if one was already held */
GLboolean __fastcall wglDevAddRef(__GLcontext *gc)  { return gc->buffers.sLockCnt++ != 0; }
/* 69013640 -- wglDevRelease: buffers.releaseSLock: drop a share lock; nonzero while others remain */
GLboolean __fastcall wglDevRelease(__GLcontext *gc) { return --gc->buffers.sLockCnt != 0; }
/* 69013660 -- ShareContext (sst_export.c), exports.shareContext: share share's display lists and texture objects */
GLboolean __fastcall wglDevShareContext(__GLcontext *gc, __GLcontext *share)
{
    __glShareDlist(gc, share);                  /* __glShareDlist */
    __glShareTextureObjects(gc, share);                            /* __glShareTextureObjects */
    return GL_TRUE;
}

/* ------------------------------------------------- hardware <-> software switch */
/* 69011c0c -- "ForceSWRendering": the software procs and dispatch tables */
void __fastcall wglDevForceSWRendering(__GLcontext *gc, void *arg)
{
    (void)arg;
    wglDevTrace("ForceSWRendering");
    wglMessageInt("ForceSWRendering(), gc ", (int)gc);
    gc->hwcx.swRendering = 1;
    gc->dirtyMask = __GL_DIRTY_ALL_ICD;
    gc->procs.validate = (__GLproc)wglGlideValidateSW;
    gc->procs.revalidate = (__GLproc)__glSSTRevalidate;
    gc->procs.pickBufferProcs = (__GLproc)__glGenericPickBufferProcs;
    gc->procs.pickAllProcsPhase2 = (__GLproc)__glGenericPickAllProcsPhase2;
    gc->procs.pickFogProcs = (__GLproc)__glGenericPickFogProcs;
    gc->procs.pickLineProcs = (__GLproc)__glGenericPickLineProcs;
    gc->procs.pickPixelProcs = (__GLproc)__glGenericPickPixelProcs;
    gc->procs.pickPointProcs = (__GLproc)__glGenericPickPointProcs;
    gc->procs.pickTriangleProcs = (__GLproc)__glGenericPickTriangleProcs;
    gc->procs.pickTextureProcs = (__GLproc)__glGenericPickTextureProcs;
    gc->procs.pickVertexProcs = (__GLproc)__glGenericPickVertexProcs;
    gc->procs.waitIdle = (__GLproc)wglDevFinishFrontSW;
    gc->procs.flush = (__GLproc)wglDevFlushSW2;
    gc->procs.finish = (__GLproc)wglDevFlushSW;
    gc->procs.applyScissor = (__GLproc)wglDevCheckViewportSW;
    gc->buffers.lockDevice = (__GLproc)__glNop1;      /* __glNop1 */
    gc->buffers.unlockDevice = (__GLproc)__glNop1;
    memcpy(&gc->currentDispatchState, &__glImmedState, sizeof(__GLdispatchTable));
    memcpy(&gc->listCompState, &__glListCompState, sizeof(__GLdispatchTable));
    ((GCfn_void_gc)wglGlideValidateSW)(gc);
}

/* 69011f14 -- "ReturnToHWRendering": the hardware procs and dispatch tables again */
void __fastcall wglDevReturnToHWRendering(__GLcontext *gc, void *arg)
{
    (void)arg;
    wglDevTrace("ReturnToHWRendering");
    gc->hwcx.swRendering = 0;
    gc->dirtyMask = __GL_DIRTY_ALL_ICD;
    gc->procs.validate = (__GLproc)wglGlideValidate;
    gc->procs.revalidate = (__GLproc)__glSSTRevalidate;
    gc->procs.pickBufferProcs = (__GLproc)wglGlideSetupDrawBuffers;
    gc->procs.pickAllProcsPhase2 = (__GLproc)__glSSTValidateWindowed;
    gc->procs.pickFogProcs = (__GLproc)hwPickFogProcs;
    gc->procs.pickLineProcs = (__GLproc)hwPickLineProcs;
    gc->procs.pickPixelProcs = (__GLproc)__glSSTPickPixelProcs;
    gc->procs.pickPointProcs = (__GLproc)hwPickPointProcs;
    gc->procs.pickTriangleProcs = (__GLproc)hwPickTriangleProcs;
    gc->procs.pickTextureProcs = (__GLproc)hwValidateTextures;
    gc->procs.pickVertexProcs = (__GLproc)hwPickVertexProcs;
    gc->procs.pickVertexArrayProcs = (__GLproc)hwPickVertexArrayProcs;
    gc->procs.waitIdle = (__GLproc)wglDevFinishFront;
    gc->procs.flush = (__GLproc)wglDevFlush;
    gc->procs.finish = (__GLproc)wglDevFinish;
    gc->procs.applyScissor = (__GLproc)wglDevClipWindow;
    gc->buffers.lockDevice = (__GLproc)wglDevNop1;
    gc->buffers.unlockDevice = (__GLproc)wglDevNop2;
    memcpy(&gc->currentDispatchState, &__glSSTImmedState, sizeof(__GLdispatchTable));
    gc->currentDispatchState.fn[__GL_DISP_TexCoord2f] = (__GLdispatchFn)gc->hwcx.genTexCoord2f;   /* the generated entries */
    gc->currentDispatchState.fn[__GL_DISP_MultiTexCoord2fARB] = (__GLdispatchFn)gc->hwcx.genMultiTexCoord2fARB;
    gc->currentDispatchState.fn[__GL_DISP_Color4f] = (__GLdispatchFn)gc->hwcx.genColor4f;
    memcpy(&gc->listCompState, &__glSSTListCompState, sizeof(__GLdispatchTable));
    if (gc->modes.layerPlane == 0) hwInstallInlineImmProcs(gc);    /* the immediate-mode procs go through hwcx.immProcs */
    hwLoadAllState(gc);                                   /* load the whole GL state into the Glide state records */
}

/* 690130b5 -- InitFnPtrs (sst_export.c): install the hardware proc table; fall back to software
** when rendering to a DIB or __GL_FORCE_SW is set */
void __fastcall wglDevInstallHWProcs(__GLcontext *gc)
{
    gc->hwcx.swRendering = 0;
    gc->procs.ec1 = (__GLproc)__glDoEvalCoord1;
    gc->procs.ec2 = (__GLproc)__glDoEvalCoord2;
    gc->procs.bitmap = (__GLbitmapProc)__glDrawBitmap;
    gc->procs.rect = (__GLproc)__glRect;
    gc->procs.clipPolygon = (__GLproc)__glClipPolygon;
    gc->procs.validate = (__GLproc)wglGlideValidate;
    gc->procs.revalidate = (__GLproc)__glSSTRevalidate;
    gc->procs.convertPolygonStipple = (__GLproc)__glConvertStipple;
    gc->procs.pushMatrix = (__GLproc)__glPushModelViewMatrix;
    gc->procs.popMatrix = (__GLproc)__glPopModelViewMatrix;
    gc->procs.loadIdentity = (__GLproc)__glLoadIdentityModelViewMatrix;
    gc->procs.matrix[0] = (__GLproc)__glCopyMatrix;
    gc->procs.matrix[1] = (__GLproc)__glInvertTransposeMatrix;
    gc->procs.matrix[2] = (__GLproc)__glMakeIdentity;
    gc->procs.matrix[3] = (__GLproc)__glMultMatrix;
    gc->procs.computeInverseTranspose = (__GLproc)__glComputeInverseTranspose;
    gc->procs.normalize = (__GLproc)__glNormalize;
    gc->procs.packVertexColor = (__GLproc)hwPackColor;
    gc->procs.packColor = (__GLproc)packColor255;
    gc->procs.beginPrim[2] = (__GLproc)__glBeginLLoop;      /* indexed by the GL primitive */
    gc->procs.beginPrim[3] = (__GLproc)__glBeginLStrip;
    gc->procs.beginPrim[1] = (__GLproc)__glBeginLines;
    gc->procs.beginPrim[0] = (__GLproc)__glBeginPoints;
    gc->procs.beginPrim[9] = (__GLproc)__glBeginPolygon;
    gc->procs.beginPrim[5] = (__GLproc)__glBeginTStrip;
    gc->procs.beginPrim[6] = (__GLproc)__glBeginTFan;
    gc->procs.beginPrim[4] = (__GLproc)__glBeginTriangles;
    gc->procs.beginPrim[8] = (__GLproc)__glBeginQStrip;
    gc->procs.beginPrim[7] = (__GLproc)__glBeginQuads;
    gc->procs.endPrim = (__GLproc)__glEndPrim;
    gc->procs.vertex = (__GLproc)__glNop2;            /* __glNop */
    gc->procs.rasterPos2 = (__GLproc)__glRasterPos2;
    gc->procs.rasterPos3 = (__GLproc)__glRasterPos3;
    gc->procs.rasterPos4 = (__GLproc)__glRasterPos4;
    gc->procs.pickAllProcsPhase2 = (__GLproc)__glSSTValidateWindowed;
    gc->procs.pickBlendProcs = (__GLproc)__glGenericPickBlendProcs;
    gc->procs.pickBufferProcs = (__GLproc)wglGlideSetupDrawBuffers;
    gc->procs.pickClipProcs = (__GLproc)__glGenericPickClipProcs;
    gc->procs.pickColorMaterialProcs = (__GLproc)__glGenericPickColorMaterialProcs;
    gc->procs.pickFogProcs = (__GLproc)hwPickFogProcs;
    gc->procs.pickTransformProcs = (__GLproc)__glGenericPickTransformProcs;
    gc->procs.pickLineProcs = (__GLproc)hwPickLineProcs;
    gc->procs.pickMatrixProcs = (__GLproc)__glGenericPickMatrixProcs;
    gc->procs.pickInvTransposeProcs = (__GLproc)__glGenericPickInvTransposeProcs;
    gc->procs.pickMvpMatrixProcs = (__GLproc)__glGenericPickMvpMatrixProcs;
    gc->procs.classifyMatrix = (__GLproc)hwClassifyMatrix;
    gc->procs.pickParameterClipProcs = (__GLproc)__glGenericPickParameterClipProcs;
    gc->procs.pickPixelProcs = (__GLproc)__glSSTPickPixelProcs;
    gc->procs.pickPointProcs = (__GLproc)hwPickPointProcs;
    gc->procs.pickRenderBitmapProcs = (__GLproc)__glOptPickRenderBitmapProcs;
    gc->procs.pickSpanProcs = (__GLproc)__glGenericPickSpanProcs;
    gc->procs.pickStoreProcs = (__GLproc)__glGenericPickStoreProcs;
    gc->procs.pickTextureProcs = (__GLproc)hwValidateTextures;
    gc->procs.pickCalcTextureProcs = (__GLproc)hwPickCalcTextureProcs;
    gc->procs.pickTriangleProcs = (__GLproc)hwPickTriangleProcs;
    gc->procs.pickVertexProcs = (__GLproc)hwPickVertexProcs;
    gc->procs.pickVertexArrayProcs = (__GLproc)hwPickVertexArrayProcs;
    gc->procs.pickAllProcsPhase1 = (__GLproc)__glGenericPickDepthProcs;
    gc->procs.copyImage = (__GLproc)__glGenericPickCopyImage;
    gc->procs.fetchTex = (__GLproc)__glGenericFetchTexImage;
    gc->procs.storeTex = (__GLproc)__glGenericStoreTexImage;
    gc->procs.readImage = (__GLproc)__glGenericReadTexImage;
    gc->procs.pixel.spanReadCI = (__GLproc)__glSpanReadCI;
    gc->procs.pixel.spanReadCI2 = (__GLproc)__glSpanReadCI2;
    gc->procs.pixel.spanReadRGBA = (__GLproc)__glSpanReadRGBA;
    gc->procs.pixel.spanReadRGBA2 = (__GLproc)__glSpanReadRGBA2;
    gc->procs.pixel.spanReadDepth = (__GLproc)__glSpanReadDepth;
    gc->procs.pixel.spanReadDepth2 = (__GLproc)__glSpanReadDepth2;
    gc->procs.pixel.spanReadStencil = (__GLproc)__glSpanReadStencil;
    gc->procs.pixel.spanReadStencil2 = (__GLproc)__glSpanReadStencil2;
    gc->procs.pixel.spanRenderCI = (__GLproc)__glSpanRenderCI;
    gc->procs.pixel.spanRenderCI2 = (__GLproc)__glSpanRenderCI2;
    gc->procs.pixel.spanRenderRGBA = (__GLproc)__glSpanRenderRGBA;
    gc->procs.pixel.spanRenderRGBA2 = (__GLproc)__glSpanRenderRGBA2;
    gc->procs.pixel.spanRenderDepth = (__GLproc)__glSpanRenderDepth;
    gc->procs.pixel.spanRenderDepth2 = (__GLproc)__glSpanRenderDepth2;
    gc->procs.pixel.spanRenderStencil = (__GLproc)__glSpanRenderStencil;
    gc->procs.pixel.spanRenderStencil2 = (__GLproc)__glSpanRenderStencil2;
    gc->procs.nonUnityQ = (__GLproc)wglDevNonUnityQ;
    gc->procs.applyScissor = (__GLproc)wglDevClipWindow;
    gc->procs.applyViewport = (__GLproc)wglDevViewportChanged;
    gc->procs.computeClipBox = (__GLproc)__glComputeClipBox;
    gc->procs.finish = (__GLproc)wglDevFinish;
    gc->procs.flush = (__GLproc)wglDevFlush;
    gc->procs.waitIdle = (__GLproc)wglDevFinishFront;
    gc->procs.colorTable = (__GLproc)__glColorTableEXT;
    gc->procs.colorSubTable = (__GLproc)__glColorSubTableEXT;
    if (gc->drawablePrivate->modes->pixmapMode == 1 || getenv("__GL_FORCE_SW") != NULL) {
        wglDevTrace("rendering to a DIB.  Fall back to sw");
        ((void (__fastcall *)(__GLcontext *))wglDevForceSWRendering)(gc);     /* EDX not set */
    }
}

/* 69013500 -- InitBuffers (sst_export.c): set up the gc's own buffer objects (front/back/accum/depth/stencil) */
void __fastcall wglDevInitBuffers(__GLcontext *gc)
{
    typedef void (__fastcall *BufInit)(void *buf, __GLcontext *gc);
    void *front;
    __GL_GET_DRAWABLE_PRIVATE(gc);                      /* (result unused) */
    __glCurrentGC()->beginMode = __GL_NEED_VALIDATE;  /* __GL_DELAY_VALIDATE_MASK(gc, everything) */
    gc->dirtyMask = __GL_DIRTY_ALL_ICD;
    gc->front = front = &gc->frontBuffer;
    if (gc->modes.doubleBufferMode) {
        gc->back = &gc->backBuffer;
        if (gc->modes.colorIndexMode) {
            __glInitCI(front, gc);          /* __glInitCI */
            __glInitCI(gc->back, gc);
        } else {
            sstRGB_Init(front, gc);          /* __glInitRGB (hw copy) */
            sstRGB_Init(gc->back, gc);
        }
    } else {
        if (gc->modes.colorIndexMode) __glInitCI(front, gc);
        else                          sstRGB_Init(front, gc);
    }
    if (gc->modes.haveAccumBuffer) __glInitAccum64(&gc->accumBuffer, gc);    /* __glInitAccum64 */
    if (gc->modes.haveDepthBuffer) __glSSTInitDepth(&gc->depthBuffer, gc);    /* the hardware depth buffer */
    else gc->depthBuffer.scale = 0x7fffff80;
    if (gc->modes.haveStencilBuffer) {
        /* 32 bpp: the stencil lives in the top byte of the depth buffer */
        if (gc->hwcx.has32bpp != 0) ((BufInit)__glInitStencil32)(&gc->stencilBuffer, gc);
        else                        ((BufInit)__glInitStencil8)(&gc->stencilBuffer, gc);   /* __glInitStencil8 */
    }
    gc->buffers.obtainSLock = wglDevAddRef;
    gc->buffers.releaseSLock = (__GLproc)wglDevRelease;
    gc->buffers.lockDevice = (__GLproc)wglDevNop1;
    gc->buffers.unlockDevice = (__GLproc)wglDevNop2;
}

/* ------------------------------------------------------------ gc->exports */
/* 690124b5 -- __glSSTCreateContext (sst_export.c), device->createContext: allocate and initialise a 3dfx gc */
__GLcontext *__fastcall wglDevCreateContext(void *imports, const __WGLmodes *modes)
{
    typedef void *(__fastcall *ImpCalloc)(__GLcontext *, size_t, size_t);   /* imports.calloc(gc, n, size) */
    typedef void *(__fastcall *ImpMalloc)(__GLcontext *, size_t);           /* imports.malloc(gc, n) */
    BYTE *raw;
    __GLcontext *gc;

    raw = (BYTE *)((ImpCalloc *)imports)[1](NULL, 1, 0x38527);         /* imports.calloc: no gc yet */
    if (raw == NULL) return NULL;
    gc = (__GLcontext *)(((ULONG_PTR)raw + 7) & ~7u);                    /* 8-byte aligned */
    gc->hwcx.allocBase = raw;
    memcpy(gc->imports, imports, sizeof(gc->imports));
    memcpy(&gc->modes, modes, sizeof(gc->modes));
    if (!((GLboolean (__fastcall *)(__GLcontext *))hwInitGrState)(gc)) return NULL;   /* the Glide state records */
    gc->constants.maxTextureUnits = 2;
    gc->constants.maxViewportWidth  = 0x800;
    gc->constants.maxViewportHeight = 0x800;
    gc->constants.viewportXAdjust = 0;
    gc->constants.viewportYAdjust = 0;
    gc->constants.subpixelBits = 3;
    gc->constants.redScale = 255.0f;
    gc->constants.greenScale = 255.0f;
    gc->constants.blueScale = 255.0f;
    gc->constants.alphaScale = 255.0f;
    if (modes->depthBits == 24) gc->constants.depthScale = 16777215.0f;
    else                        gc->constants.depthScale = 65535.0f;
    gc->constants.numberOfLights = 8;
    gc->constants.numberOfClipPlanes = 6;
    gc->constants.numberOfTextures = 1;
    gc->constants.numberOfTextureEnvs = 1;
    if (gc->hwcx.hasTexFmtLarge != 0) { gc->constants.maxTextureSize = 0x800; gc->constants.maxMipMapLevel = 0xc; }
    else                              { gc->constants.maxTextureSize = 0x100; gc->constants.maxMipMapLevel = 9; }
    gc->constants.maxListNesting = 0x40;
    gc->constants.maxEvalOrder = 0x1e;
    gc->constants.maxPixelMapTable = 0x10000;
    gc->constants.maxAttribStackDepth = 0x10;
    gc->constants.maxClientAttribStackDepth = 0x10;
    gc->constants.maxNameStackDepth = 0x80;
    gc->constants.maxModelViewStackDepth = 0x20;
    gc->constants.maxProjectionStackDepth = 2;
    gc->constants.maxTextureStackDepth = 10;
    gc->constants.pointSizeMinimum = 1.0f;
    gc->constants.pointSizeMaximum = 32.0f;
    gc->constants.pointSizeGranularity = 0.125f;
    gc->constants.lineWidthMinimum = 0.5f;
    gc->constants.lineWidthMaximum = 10.0f;
    gc->constants.lineWidthGranularity = 0.125f;
    if (getenv("GL_IGNORE_API_ERRORS") == NULL) gc->procs.error = (__GLproc)wglDevApiError;
    gc->state.point.ext[1] = gc->constants.pointSizeMaximum;   /* EXT_point_parameters maximum (a plain copy of the bits) */
    gc->dlist.optimizer = __glGenericDlistOptimizer;                    /* 6907f550 */
    gc->dlist.compiler = __glGenericDlistCompiler;                           /* __glGenericDlistCompiler */
    gc->dlist.listExec = __gl_GenericDlOps;
    gc->dlist.baseListExec = __glListExecTable;
    gc->dlist.machineListExec = NULL;
    gc->dlist.checkOp = __glNop2;                            /* __glNop */
    gc->dlist.initState = __glNop1;                          /* __glNop1 */
    GC_exports_destroyContext(gc)    = wglDevDestroyContext;
    GC_exports_loseCurrent(gc)       = wglDevLoseCurrent;
    GC_exports_makeCurrent(gc)       = wglDevMakeCurrent;
    GC_exports_shareContext(gc)      = wglDevShareContext;
    GC_exports_notifyResize(gc)      = wglDevNotifyResize;
    GC_exports_notifyDestroy(gc)     = wglDevNotifyDestroy;
    GC_exports_notifySwapBuffers(gc) = wglDevSwapBuffers;
    __glEarlyInitContext(gc);                                            /* __glEarlyInitContext */
    gc->hwcx.swRendering = 0;
    gc->hwcx.swRequested = 0;
    gc->fx.numSamples = 0;
    gc->hwcx.depthFlag = 0;
    {
        /* four 0xc0-byte staging vertices, 32-byte aligned */
        void *p = ((ImpMalloc)GC_imports_malloc(gc))(gc, 0x320);
        __GLvertexICD *v;
        gc->hwcx.stagingAlloc = p;
        v = (__GLvertexICD *)(((ULONG_PTR)p + 0x1f) & ~0x1fu);
        gc->hwcx.stagingBase = v;
        gc->hwcx.stagingVtx[0] = v;
        gc->hwcx.stagingVtx[1] = v + 1;
        gc->hwcx.stagingVtx[2] = v + 2;
        gc->hwcx.stagingVtx[3] = v + 3;
    }
    {
        int i;
        for (i = 0; i < 2; i++) gc->hwcx.unitTexture[i] = NULL;   /* (a loop: VC6 unrolls it with a fresh zero) */
    }
    gc->hwcx.scratchSize = 0;
    gc->hwcx.scratch = NULL;
    return gc;
}

/* 69012a01 -- DestroyContext (sst_export.c), exports.destroyContext: free everything the gc allocated,
** then the gc itself.  imports.free is __fastcall (gc, p).  (A first version used a one-argument free: every
** call passed p as the gc and freed whatever was left in EDX, GlobalFree on garbage at each wglDeleteContext;
** fixed and checked against the original instruction by instruction, 2026-10-29.)
*/
GLboolean __fastcall wglDevDestroyContext(__GLcontext *gc)
{
    __glFreeBufData(gc);                                 /* __glFreeBufData */
    __glDestroyContext(gc);                                      /* __glDestroyContext */
    __GL_FREE(gc, gc->hwcx.stagingAlloc);          /* (wglDevCreateContext) */
    __GL_FREE(gc, gc->hwcx.vtxAlloc);                      /* 69019bc0's vertex arrays */
    __GL_FREE(gc, gc->hwcx.alloc_365f8);
    if (gc->hwcx.growBufA != NULL) {
        __GL_FREE(gc, gc->hwcx.growBufA);
        gc->hwcx.growBufA = NULL;
        gc->hwcx.growBufACap = 0;
    }
    hwPageCacheFree(gc, &gc->hwcx.bufRec[4]); hwPageCacheFree(gc, &gc->hwcx.bufRec[5]);   /* the code-page caches */
    hwPageCacheFree(gc, &gc->hwcx.bufRec[6]); hwPageCacheFree(gc, &gc->hwcx.bufRec[0]);
    hwPageCacheFree(gc, &gc->hwcx.bufRec[7]); hwPageCacheFree(gc, &gc->hwcx.bufRec[8]);
    hwPageCacheFree(gc, &gc->hwcx.bufRec[9]); hwPageCacheFree(gc, &gc->hwcx.bufRec[2]);
    hwPageCacheFree(gc, &gc->hwcx.bufRec[3]); hwPageCacheFree(gc, &gc->hwcx.bufRec[1]);
    __GL_FREE(gc, gc->hwcx.codeBufRaw);            /* the two code buffers (hwAllocCodeBuffers) */
    __GL_FREE(gc, gc->hwcx.codeBuf2Raw);
    og_stream(gc, 0);                                   /* an empty proc */
    hwFreeGrState(gc);                                      /* the Glide state records */
    if (gc->hwcx.scratch != NULL) __GL_FREE(gc, gc->hwcx.scratch);
    if (gc->hwcx.sharedPalette.table != NULL) __GL_FREE(gc, gc->hwcx.sharedPalette.table);
    __GL_FREE(gc, (void *)gc->constants.renderer);         /* malloc'd by 6902a436 (sst_settings.c) */
    gc->constants.renderer = (const GLubyte *)"";          /* 69012a00: a zero byte in the original .text (an empty string) */
    __GL_FREE(gc, gc->hwcx.allocBase);                     /* the gc itself */
    return GL_TRUE;
}

/* 69012b50 -- LoseCurrent (sst_export.c), exports.loseCurrent: release the buffers, then the
** Glide surface (windowed) or close the fullscreen Glide window */
GLboolean __fastcall wglDevLoseCurrent(__GLcontext *gc)
{
    if (gc->renderMode == GL_RENDER && __glCurrentGC()->beginMode != 1 /* __GL_IN_BEGIN */) {
        hwLoseBuffersCurrent(gc);                               /* loseCurrent on each buffer */
        if (gc->hwcx.windowed) winReleaseSurfaceContext(gc);        /* grSurfaceReleaseContextExt */
        else wglGlideClose(gc);                         /* 69015710 */
        gc->beginMode = __glCurrentGC()->beginMode;
        return GL_TRUE;
    }
    return GL_FALSE;
}

/* 69012790 -- exports.notifyResize: resize the gc buffers; 0xf1 = switched hw/sw */
GLubyte __fastcall wglDevNotifyResize(__GLcontext *gc, GLint x, GLint y, GLuint w, GLuint h)
{
    __WGLdrawable *dp = gc->drawablePrivate;
    GLboolean fallback = GL_FALSE;
    __WGLwindowRect *wr = dp->windowRect;
    GLboolean wasSW = (GLboolean)gc->hwcx.swRendering;
    char r;

    if (!dp->regionValid) {
        dp->regionValid = 1;
        if (gc->hwcx.windowed) winRestoreSurfaces(dp, gc);    /* restore the DirectDraw surfaces */
    }
    gc->buffers.accelBufferMask = dp->accelBufferMask;
    if (gc->frontBuffer.buf.gc == NULL) return 0;
    if (gc->frontBuffer.buf.resize != NULL) {
        r = ((GCfn_bufresize)gc->frontBuffer.buf.resize)(&gc->frontBuffer, x, y, w, h, 1);
        if (r == 0) return 0;
        if (r == (char)0xf0) fallback = GL_TRUE;
    }
    if (gc->modes.doubleBufferMode && gc->backBuffer.buf.resize != NULL) {
        r = ((GCfn_bufresize)gc->backBuffer.buf.resize)(&gc->backBuffer, x, y, w, h, 4);
        if (r == 0) return 0;
        if (r == (char)0xf0) fallback = GL_TRUE;
    }
    if (gc->modes.haveAccumBuffer && gc->accumBuffer.buf.resize != NULL) {
        r = ((GCfn_bufresize)gc->accumBuffer.buf.resize)(&gc->accumBuffer, x, y, w, h, 0x10);
        if (r == 0) return 0;
    }
    if (gc->modes.haveDepthBuffer && gc->depthBuffer.buf.resize != NULL) {
        r = ((GCfn_bufresize)gc->depthBuffer.buf.resize)(&gc->depthBuffer, x, y, w, h, 0x20);
        if (r == 0) return 0;
        if (r == (char)0xf0) fallback = GL_TRUE;
    }
    if (gc->modes.haveStencilBuffer && gc->stencilBuffer.buf.resize != NULL) {
        r = ((GCfn_bufresize)gc->stencilBuffer.buf.resize)(&gc->stencilBuffer, x, y, w, h, 0x40);
        if (r == 0) return 0;
    }
    if ((GLint)w != gc->constants.width || (GLint)h != gc->constants.height) {
        if (gc->constants.yInverted && (GLint)h != gc->constants.height)
            gc->state.current.rasterPos.window.y += (GLfloat)(GLuint)((GLint)h - gc->constants.height);
        gc->constants.width = w;
        gc->constants.height = h;
        (*gc->procs.computeClipBox)(gc);
        (*gc->procs.applyScissor)(gc);
    }
    __glUpdateViewport(gc);                                   /* __glUpdateViewport */
    wglDevCheckViewportInside(gc);
    if (fallback) {
        wr->swFallback = 1;
        if (!wasSW) ((void (__fastcall *)(__GLcontext *))wglDevForceSWRendering)(gc);       /* EDX not set */
        return 0xf1;
    }
    wr->swFallback = 0;
    if (wasSW) { ((void (__fastcall *)(__GLcontext *))wglDevReturnToHWRendering)(gc); return 0xf1; }   /* EDX not set */
    return 1;
}

/* 690129f0 -- exports.notifyDestroy: wglDevNotifyResize to an empty 0 x 0 drawable at (0, 0) */
void __fastcall wglDevNotifyDestroy(__GLcontext *gc) { wglDevNotifyResize(gc, 0, 0, 0, 0); }

/* 69012bd1 -- MakeCurrent (sst_export.c), exports.makeCurrent.  Opens the Glide screen (fullscreen)
** or a Glide surface (windowed), installs dispatch tables and procs, and does the one-time
** initialisation on the first make-current of a context.
*/
GLboolean __fastcall wglDevMakeCurrent(__GLcontext *gc)
{
    typedef void *(__fastcall *DevMalloc)(__GLcontext *, size_t);            /* imports.malloc(gc, n) */
    typedef void *(__fastcall *DevRealloc)(__GLcontext *, void *, size_t);   /* imports.realloc(gc, p, n) */
    typedef void (__fastcall *DevProc)(__GLcontext *);
    typedef void (__stdcall *DevRect)(GLint, GLint, GLint, GLint);           /* glViewport / glScissor */
    __WGLrc *rc = GC_imports_wscx(gc);
    struct { PIXELFORMATDESCRIPTOR pfd; GLint samples; } fmt;   /* the 3dfx table row: the multisample count follows */

    __glSetCurrentGC(gc);                                             /* the inline TEB store */
    if (wglDisplayQuery() != 0) goto fail;                      /* 69014c10: ExtEscape 0x3df7 */
    if (rc->window->dp.fullScreen) {
        if (wglGlideDescribePixelFormat(rc->hdc, &fmt.pfd, rc->iPixelFormat - 1) != 0)   /* 69014db8 */
            gc->fx.numSamples = fmt.samples;
        if ((GLint)(gc->hwcx.glideContext = wglGlideOpen(gc)) != -1) {          /* 69015619 */
            gc->fx.fullscreen = 1;
            gc->hwcx.windowed = 0;
        } else {
            if ((GLint)(gc->hwcx.glideContext = PrimaryMakeCurrent(gc)) == -1) goto fail;
            gc->fx.fullscreen = 0;
            gc->hwcx.windowed = 1;
        }
    } else {
        if (gc->fx.glideOpen == 1) goto fail;          /* already open fullscreen */
        /* windowed: Glide surface on the primary */
        if ((GLint)(gc->hwcx.glideContext = PrimaryMakeCurrent(gc)) != -1) {    /* "PrimaryMakeCurrent" */
            gc->fx.fullscreen = 0;
            gc->hwcx.windowed = 1;
        } else goto fail;
    }
    wglDevDetectScreenSaver(gc);
    if (!(gc->mcFlags & __GL_INIT_HW)) {                              /* first time: dispatch tables, buffers */
        gc->dispatchState = &gc->currentDispatchState;
        memcpy(&gc->currentDispatchState, &__glSSTImmedState, sizeof(__GLdispatchTable));
        __glDetectCPU(gc);                                             /* CPU feature detection */
        og_genContextCode(gc);                                             /* generate the context's code */
        gc->currentDispatchState.fn[__GL_DISP_TexCoord2f] = (__GLdispatchFn)gc->hwcx.genTexCoord2f;
        gc->currentDispatchState.fn[__GL_DISP_MultiTexCoord2fARB] = (__GLdispatchFn)gc->hwcx.genMultiTexCoord2fARB;
        gc->currentDispatchState.fn[__GL_DISP_Color4f] = (__GLdispatchFn)gc->hwcx.genColor4f;
        memcpy(&gc->listCompState, &__glSSTListCompState, sizeof(__GLdispatchTable));
        wglDevInitBuffers(gc);
        hwReadDepthPrecision(gc);                                             /* depth precision settings */
    }
    if (gc->modes.layerPlane == 0) hwInstallInlineImmProcs(gc);                  /* immediate-mode procs through hwcx.immProcs */
    ((DevProc)hwMakeBuffersCurrent)(gc);                                      /* make the buffers current; EDX not set */
    if (!(gc->mcFlags & __GL_HW_MODE)) wglDevInstallHWProcs(gc);
    if (gc->hwcx.swRequested != 0) gc->hwcx.swRendering = 1;
    if (gc->hwcx.swRendering != 0) {
        ((DevProc)wglDevForceSWRendering)(gc);                        /* EDX not set */
    } else {
        if (gc->hwcx.hasTexUMA != 0) grEnable(6 /* GR_TEXTURE_UMA_EXT (g3ext.h) */);
        if (gc->fx.fullscreen != 0) texMgrSetMemoryEnd(gc, grTexMaxAddress(0));   /* texture memory ends at the TMU's top */
        gc->constants.maxTextureUnits = gc->fx.numTMUs = grGetInteger(GR_NUM_TMU);
        hwSetupVertexLayout(gc);                                             /* the Glide vertex layout */
        if (!gc->modes.doubleBufferMode || getenv("SST_SINGLEBUFFER") != NULL) {
            /* grRenderBuffer(GR_BUFFER_FRONTBUFFER) through the Glide state record */
            GLint *p = gc->hwcx.grState[14];
            if (*p != 0) { *p = 0; gc->hwcx.grDirty |= 1 << 14; }
        }
    }
    if (!(gc->mcFlags & (__GL_HW_MODE | __GL_PIXMAP_MODE))) {        /* first make-current: reset to the default state */
        GLint sx, sy, w, h;

        __glSoftResetContext(gc);                                             /* __glSoftResetContext */
        sstVarray_AllocateVbuf(gc, 0, 3000);                          /* AllocateVbuf */
        hwGrowVertexArrays(gc, 3000);
        gc->hwcx.alloc_365f8 = ((DevMalloc)GC_imports_malloc(gc))(gc, 0x5dc0);   /* 3000 x 8 bytes */
        if (gc->hwcx.growBufACap < 0x400) {
            gc->hwcx.growBufA = gc->hwcx.growBufA ? ((DevRealloc)GC_imports_realloc(gc))(gc, gc->hwcx.growBufA, 0x1020)
                                                  : ((DevMalloc)GC_imports_malloc(gc))(gc, 0x1020);
            gc->hwcx.growBufACap = 0x400;
        }
        if (gc->hwcx.growBufBCap < 0x400) {
            gc->hwcx.growBufB = gc->hwcx.growBufB ? ((DevRealloc)GC_imports_realloc(gc))(gc, gc->hwcx.growBufB, 0x1020)
                                                  : ((DevMalloc)GC_imports_malloc(gc))(gc, 0x1020);
            gc->hwcx.growBufBCap = 0x400;
        }
        wglDevInitPools(gc);
        hwPickDrawRoutines(gc);                                             /* direct command-FIFO routines */
        gc->constants.viewportEpsilon = 0.0625f;
        gc->constants.viewportAlmostHalf = (__GLfloat)(gc->constants.half) - 0.0625;   /* (the cast keeps the original x87 schedule) */
        if (gc->constants.yInverted != 0)
            gc->state.current.rasterPos.window.y = ((GLfloat)(GLuint)gc->constants.height - 0.0625f) + gc->constants.fviewportYAdjust;
        gc->state.current.packedColor = 0xffffffff;
        gc->fx.primMode = 0xffffffff;
        hwBuildGetStrings(gc);                                             /* the glGetString strings */
        /* the spec wants the viewport and scissor set on the first make-current */
        GC_imports_getDrawableSize(gc)(gc, &sx, &sy, (GLuint *)&w, (GLuint *)&h);
        ((DevRect)gc->currentDispatchState.fn[__GL_DISP_Viewport])(0, 0, w, h);
        ((DevRect)gc->currentDispatchState.fn[__GL_DISP_Scissor])(0, 0, w, h);
    } else {
        (*gc->procs.applyViewport)(gc);
        (*gc->procs.applyScissor)(gc);
    }
    if (!(gc->mcFlags & __GL_INIT_HW)) {
        GC_imports_wscx(gc)->flags |= 1;
        (*gc->procs.validate)(gc);
        __glContextSetColorScales(gc);                                             /* __glContextSetColorScales */
    }
    if (gc->dlist.currentList != 0) {
        gc->dispatchState = &gc->savedDispatchState;
        __glCopyDispatchTable(&gc->listCompState, &__glCurrentGC()->currentDispatchState);   /* copy the table */
    } else {
        gc->dispatchState = &__glCurrentGC()->currentDispatchState;
    }
    wglGlideSetupBufferModes(gc);                                     /* 69015cd0 */
    __glCurrentGC()->beginMode = __GL_NEED_VALIDATE;              /* __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH) */
    gc->dirtyMask |= __GL_DIRTY_DEPTH;
    hwDirtyAllGrState(gc);                                                 /* every Glide state record dirty */
    __glUpdateDepthRange(gc);                                                 /* __glUpdateDepthRange */
    hwReadGuardBandTweaks(gc);                                                 /* guard band / Z-cull settings */
    hwReadForce16bppTextures(gc);                                                 /* FX_GL_FORCE_16BPP_TEXTURES */
    hwReadPerfSettings(gc);                                                 /* FX_GL_SUPER, pending buffers, HSR */
    hwReadObjectKeyGlow(gc);                                                 /* object key / glow worm */
    gc->fx.lastHsrMode = -1;
    gc->fx.swapPending = 0;
    gc->fx.flags |= 4;
    gc->fx.depthDirection = GL_LEQUAL;
    gc->fx.depthClearPending = 1;
    gc->fx.guardBandOk = gc->hwcx.hasTexFmtNapalm;
    gc->mcFlags = (gc->mcFlags & ~__GL_PIXMAP_MODE) | (__GL_INIT_HW | __GL_HW_MODE);
    return GL_TRUE;
fail:
    return GL_FALSE;
}

/* 69014a3b -- SwapBuffers (sst_export.c), exports.notifySwapBuffers */
void __fastcall wglDevSwapBuffers(__GLcontext *gc)
{
    __WGLrc *rc = GC_imports_wscx(gc);

    if (**gc->hwcx.cmdTransportInfo->ptrLostContext != 0) {   /* Glide lost its context: no swap */
        if (gc->hwcx.windowed && wglDisplayQuery() != 1)
            winRestoreSurfaces(&rc->window->dp, gc);              /* restore the DirectDraw surfaces */
        return;
    }
    g_elementsLastFrame = g_elementsThisFrame;
    if (gc->fx.glowWorm == 4) {
        if (g_swapDumpCount > 0x50) {
            char name[256];
            FILE *f;
            sprintf(name, "c:\\swap%i_%i.txt", g_elementsThisFrame, g_swapDumpCount);
            f = fopen(name, "wt");
            fprintf(f, "%s", name);
            fclose(f);
        }
        g_swapDumpCount++;
    }
    g_elementsThisFrame = 0;
    gc->fx.swapCount++;
    (*gc->procs.flush)(gc);
    if (!gc->modes.doubleBufferMode)
        return;
    if (gc->hwcx.windowed) {
        if (gc->drawablePrivate->window->dp.modes->singleBuffered) {   /* (sic) byte 0x55 of the window modes */
            __WGLwinSurface *frontSurf = (__WGLwinSurface *)gc->frontBuffer.buf.wglBuf->private_;
            __WGLwinSurface *backSurf = (__WGLwinSurface *)gc->backBuffer.buf.wglBuf->private_;
            __WGLwinSurface *front = gc->frontBuffer.buf.drawableBuf;
            __WGLwinSurface *back = gc->backBuffer.buf.drawableBuf;
            LPDIRECTDRAWSURFACE t = front->surf;                       /* swap the surface handles */

            front->surf = back->surf;
            back->surf = t;
            if (frontSurf->inVideo && backSurf->inVideo) {
                if ((gc->hwcx.glideVerFlags & ~1u) != 0)
                    grSurfaceSetRenderingSurfaceExt2(gc->drawBuffer->buf.drawableBuf->surf, 0);
                else
                    grSurfaceSetRenderingSurfaceExt1(gc->drawBuffer->buf.drawableBuf->surf);
            }
        }
        winPresent(gc);                                              /* present the windowed drawable */
    } else if (rc->isCurrent) {
        grBufferSwap(gc->hwcx.swapInterval);
    }
    gc->fx.swapPending = 0;
}

/* ------------------------------------------------------------ device table */

__WGLdevice g_deviceTable = {                                                      /* 690eee48 */
    0x24, 0,
    (void *(__fastcall *)(__GLcontext *, GLenum, GLenum *))hwTexelFormat,
    wglGlideProcessAttach, wglGlideProcessDetach, wglGlideThreadAttach, NULL,
    wglGlideDescribePixelFormatCB, (void (__fastcall *)(void))__glResetPixelFormats, (void *)__glSSTInitTextureManager,
    wglDevCreateContext, wglGlideInitWindow, wglDrawInitDIB, wglGlideDestroyWindow, NULL };
__WGLdevice *g_device = &g_deviceTable;                                            /* 690eee84 */

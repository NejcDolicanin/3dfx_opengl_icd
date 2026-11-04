/*
** sst_buffers.c -- 3dfx back end, original VA range 69028000..6902c000: the Glide version parse, freeing the Glide
** state records, the hardware buffer-head procs, making the drawable's buffers current / not current, and
** the triangle draw procs.
** Then the buffer make-current / lose-current / resize procs, the drawable buffers' free, glReadPixels,
** glClear and the glFinish of the hardware path, in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "siprotos.h"
#include "og.h"
#include "glshade.h"
#include "icdglob.h"
#include "icdprotos.h"
#include "si_names.h"

/* 690fc4dc -- opengl32!wglSwapBuffers, looked up once */
FARPROC g_opengl32SwapBuffers;

typedef void *(__fastcall *SstCallocFn)(__GLcontext *gc, GLuint size, GLuint n);
typedef void (__fastcall *SstFreeFn)(__GLcontext *gc, void *p);
typedef __WGLdrawable *(__fastcall *SstGetDrawableFn)(__GLcontext *gc);
#define SST_CALLOC(gc)  (*(SstCallocFn *)&(gc)->imports[1])
#define SST_FREE(gc)    (*(SstFreeFn *)&(gc)->imports[3])
#define SST_GET_DRAWABLE_PRIVATE(gc)    (*(SstGetDrawableFn *)&(gc)->imports[6])

extern void __fastcall hwBufferMakeCurrent(__GLbufferHeadICD *buf, __WGLbuffer *other, __GLcontext *gc);  /* makeCurrent */
extern void __fastcall hwLoseCurrentBuffer(__GLbufferHeadICD *buf, __GLcontext *gc);                       /* loseCurrent */
extern GLubyte __fastcall hwBufferResize(__GLbufferHeadICD *buf, GLuint edx_unused, GLuint unused3, GLint w,
                                       GLint h, GLint mode);                                         /* resize */

/* ---- forward declarations ---- */
void __fastcall hwInitBufferHead(__GLbufferHeadICD *buf, __GLcontext *gc);
void __fastcall hwBufferFree(__GLbufferHeadICD *buf, __GLcontext *gc);
void __fastcall hwNopProc(void);
void __fastcall hwDrawTriPackColor(__GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall hwDrawTri(__GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall hwDrawQuad(__GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c, __GLvertexICD *d);
void __fastcall hwFifoWriteVertex3(GrCmdTransportInfo *fifo, GLuint *v);                 /* sst_trigen.c */

/* 69028953 -- hwParseGlideVersion: parse the Glide version string (grGetString(GR_VERSION), "3.10.00.0662"-style)
** and set glideVerFlags: 1 = build >= 504 before minor 4 (or minor 4), 2 = build >= 510 (or minor 4) */
void __fastcall hwParseGlideVersion(__GLcontext *gc)
{
    const char *version;
    GLuint minor;

    version = grGetString(GR_VERSION);
    gc->hwcx.glideVersionStr = version;
    sscanf(version, "%1d.%1d%1d.%2d.%4d", &gc->hwcx.glideVerMajor, &gc->hwcx.glideVerMinor,
           &gc->hwcx.glideVerMinorSub, &gc->hwcx.glideVerRev, &gc->hwcx.glideVerBuild);
    minor = gc->hwcx.glideVerMinor;
    gc->hwcx.glideVerFlags = 0;
    if ((minor < 4 && gc->hwcx.glideVerBuild >= 0x1f8) || minor == 4) {
        gc->hwcx.glideVerFlags = 1;
    }
    if ((minor < 4 && gc->hwcx.glideVerBuild >= 0x1fe) || minor == 4) {
        gc->hwcx.glideVerFlags |= 2;
    }
}

/* 69028ec0 -- hwFreeGrState: free the Glide state records (grState[], and grStateExt[] when the combine extension is
** used: then the texture / colour / alpha combine records are the extension ones) */
void __fastcall hwFreeGrState(__GLcontext *gc)
{
    GLint *last;

    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_ALPHA_BLEND]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_DEPTH_FUNC]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_DEPTH_MODE]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_DEPTH_MASK]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_TMU0_FILTER]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_TMU0_CLAMP]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_TMU0_MIPMAP]);
    if (gc->hwcx.numTMU == 2) {
        SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_TMU1_FILTER]);
        SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_TMU1_CLAMP]);
        SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_TMU1_MIPMAP]);
        if (!gc->hwcx.hasCombineExt) {
            last = gc->hwcx.grState[__GL_GR_TMU1_COMBINE];
        } else {
            SST_FREE(gc)(gc, gc->hwcx.grStateExt[4]);
            SST_FREE(gc)(gc, gc->hwcx.grStateExt[5]);
            last = gc->hwcx.grStateExt[7];
        }
        SST_FREE(gc)(gc, last);
    }
    if (!gc->hwcx.hasCombineExt) {
        SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_TMU0_COMBINE]);
        SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_COLOR_COMBINE]);
        last = gc->hwcx.grState[__GL_GR_ALPHA_COMBINE];
    } else {
        SST_FREE(gc)(gc, gc->hwcx.grStateExt[2]);
        SST_FREE(gc)(gc, gc->hwcx.grStateExt[3]);
        SST_FREE(gc)(gc, gc->hwcx.grStateExt[0]);
        SST_FREE(gc)(gc, gc->hwcx.grStateExt[1]);
        last = gc->hwcx.grStateExt[6];
    }
    SST_FREE(gc)(gc, last);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_RENDER_BUFFER]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_CULL]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_ALPHA_TEST_FUNC]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_ALPHA_TEST_REF]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_DEPTH_BIAS]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_COLOR_MASK]);
    if (gc->hwcx.has32bpp) {
        SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_STENCIL_FUNC]);
        SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_STENCIL_OP]);
        SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_STENCIL_MASK]);
    }
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_DITHER]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_FOG_COLOR]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_FOG]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_FOG_MODE]);
    SST_FREE(gc)(gc, gc->hwcx.grState[__GL_GR_FOG_TABLE]);
}

/* 690297c0 -- hwInitBufferHead: initialise a hardware buffer head: a 0x14-byte private block and the buffer procs */
void __fastcall hwInitBufferHead(__GLbufferHeadICD *buf, __GLcontext *gc)
{
    buf->gc = 0;
    buf->drawableBuf = (__WGLwinSurface *)SST_CALLOC(gc)(gc, 0x14, 1);
    buf->wglBuf = NULL;
    buf->resize = (__GLproc)hwBufferResize;
    buf->makeCurrent = (__GLproc)hwBufferMakeCurrent;
    buf->loseCurrent = (__GLproc)hwLoseCurrentBuffer;
    buf->freeBuf = (__GLproc)hwBufferFree;
}

/* 690299e0 -- hwBufferFree: buf->freeBuf: free the private block */
void __fastcall hwBufferFree(__GLbufferHeadICD *buf, __GLcontext *gc)
{
    if (buf->drawableBuf) {
        SST_FREE(gc)(gc, buf->drawableBuf);
    }
    buf->makeCurrent = 0;
    buf->resize = 0;
}

/* 69029a00 -- hwMakeBuffersCurrent: make the drawable's buffers current: fetch the drawable (its freeBuffers proc is
** __glFreePrivate), follow its y orientation (a change revalidates everything), and under the drawable
** lock take its size and make each buffer of the visual current on the drawable's buffer */
void __fastcall hwMakeBuffersCurrent(__GLcontext *gc, GLuint edxUnused)
{
    __WGLdrawable *dp;

    dp = SST_GET_DRAWABLE_PRIVATE(gc)(gc);
    gc->drawablePrivate = dp;
    dp->freeBuffers = (void (__fastcall *)(__WGLdrawable *))__glFreePrivate;
    if ((GLuint)gc->drawablePrivate->yInverted != gc->constants.yInverted) {
        __gl_beginMode = __GL_NEED_VALIDATE;
        gc->dirtyMask = 0xffffffff;
    }
    if (gc->drawablePrivate->yInverted) {
        gc->constants.yInverted = GL_TRUE;
        gc->constants.ySign = -1;
    } else {
        gc->constants.yInverted = GL_FALSE;
        gc->constants.ySign = 1;
    }
    (*dp->lockDP)(dp, gc);
    gc->constants.width = dp->width;
    gc->constants.height = dp->height;
    gc->buffers.accelBufferMask = dp->accelBufferMask;
    (*gc->frontBuffer.buf.makeCurrent)(&gc->frontBuffer, &dp->frontBuffer, gc);
    if (gc->modes.doubleBufferMode) {
        (*gc->backBuffer.buf.makeCurrent)(&gc->backBuffer, &dp->backBuffer, gc);
    }
    if (gc->modes.haveAccumBuffer) {
        (*gc->accumBuffer.buf.makeCurrent)(&gc->accumBuffer, &dp->accumBuffer, gc);
    }
    if (gc->modes.haveDepthBuffer) {
        (*gc->depthBuffer.buf.makeCurrent)(&gc->depthBuffer, &dp->depthBuffer, gc);
    }
    if (gc->modes.haveStencilBuffer) {
        (*gc->stencilBuffer.buf.makeCurrent)(&gc->stencilBuffer, &dp->stencilBuffer, gc);
    }
    (*dp->unlockDP)(dp);
}

/* 69029b30 -- hwLoseBuffersCurrent: the buffers are no longer current: loseCurrent on each, under the
** drawable lock */
void __fastcall hwLoseBuffersCurrent(__GLcontext *gc)
{
    __WGLdrawable *dp;

    dp = gc->drawablePrivate;
    (*dp->lockDP)(dp, gc);
    (*gc->frontBuffer.buf.loseCurrent)(&gc->frontBuffer, gc);
    if (gc->modes.doubleBufferMode) {
        (*gc->backBuffer.buf.loseCurrent)(&gc->backBuffer, gc);
    }
    if (gc->modes.haveAccumBuffer) {
        (*gc->accumBuffer.buf.loseCurrent)(&gc->accumBuffer, gc);
    }
    if (gc->modes.haveDepthBuffer) {
        (*gc->depthBuffer.buf.loseCurrent)(&gc->depthBuffer, gc);
    }
    if (gc->modes.haveStencilBuffer) {
        (*gc->stencilBuffer.buf.loseCurrent)(&gc->stencilBuffer, gc);
    }
    (*dp->unlockDP)(dp);
}

/* hwNopProc is also the debug printf of this module, compiled out: the two unused FIFO procs below call it
** printf-style (cdecl, the caller pops the arguments) and it prints nothing */
#define HW_DEBUG_PRINTF     ((void (__cdecl *)(const char *fmt, ...))hwNopProc)

/* 6902aa24 (size 61) -- hwFifoDumpState: debug dump of Glide's command FIFO state (the transport record, fifoPtr,
** fifoRoom) through the compiled-out debug printf, so it does nothing but read them; ToDo UNUSED CODE: nothing in
** the driver calls it */
void __fastcall hwFifoDumpState(__GLcontext *gc)
{
    GrCmdTransportInfo *fifo = gc->hwcx.cmdTransportInfo;

    HW_DEBUG_PRINTF("PETER'S GHOST SAYS:");
    HW_DEBUG_PRINTF("  struct at %p", fifo);
    HW_DEBUG_PRINTF("  fifoPtr =  %p", fifo->fifoPtr);
    HW_DEBUG_PRINTF("  fifoRoom = %d", fifo->fifoRoom);
}

/* 6902aa70 -- hwNopProc: an empty proc (a lone ret) */
void __fastcall hwNopProc(void)
{
}

/* 6902abf0 -- draw a triangle, packing the vertex colours first (emms around the MMX packing) */
void __fastcall hwDrawTriPackColor(__GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    __GLcontext *gc;

    gc = __glCurrentGC();
    GC_EMMS_IF_MMX(gc);
    hwPackVertexColor(gc, a);
    hwPackVertexColor(gc, b);
    hwPackVertexColor(gc, c);
    GC_EMMS_IF_MMX(gc);
    grDrawTriangle(a, b, c);
    GC_EMMS_IF_MMX(gc);
}

/* 6902ac60 -- gc->hwcx.drawTri without generated code: grDrawTriangle */
void __fastcall hwDrawTri(__GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    grDrawTriangle(a, b, c);
}

/* 6902ac70 -- gc->hwcx.drawQuad without generated code: the triangles (b, a, c) and (b, c, d) */
void __fastcall hwDrawQuad(__GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c, __GLvertexICD *d)
{
    grDrawTriangle(b, a, c);
    grDrawTriangle(b, c, d);
}

/* 6902aca9 (size 84) -- hwFifoDrawTriFlat: a flat-shaded triangle straight into the command FIFO: the triangle
** packet header (top three bits cleared), then x, y and the packed colour of each vertex (hwFifoWriteVertex3); no
** room check (no grCommandTransportMakeRoomExt2) and no fifoRoom update; ToDo UNUSED CODE: nothing in the driver
** calls it */
void __fastcall hwFifoDrawTriFlat(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    GrCmdTransportInfo *fifo = gc->hwcx.cmdTransportInfo;

    HW_DEBUG_PRINTF("FLAT %#x", fifo->triPacketHdr);
    *fifo->fifoPtr = fifo->triPacketHdr & 0x1fffffff;
    fifo->fifoPtr++;
    hwFifoWriteVertex3(fifo, (GLuint *)a);
    hwFifoWriteVertex3(fifo, (GLuint *)b);
    hwFifoWriteVertex3(fifo, (GLuint *)c);
}

#pragma warning(disable: 4273)
#define GR_FLUSH_STATE(gc)  ((void (__fastcall *)(__GLcontext *))hwFlushGrState)(gc)
/* ---- glReadPixels (depth / stencil reads go through the drawable's own lock) --------------------- */
typedef struct SstDrawProcsRec {            /* drawable +0x158 */
    GLuint      unk_00[0x1c / 4];
    void (__fastcall *lock)(void *buf, __WGLdrawable *dp);       /* +0x1c */
    void (__fastcall *unlock)(void *buf, __WGLdrawable *dp);     /* +0x20 */
} SstDrawProcs;
HMODULE WINAPI GetModuleHandleA(LPCSTR lpModuleName);
FARPROC WINAPI GetProcAddress(HMODULE hModule, LPCSTR lpProcName);

/* the Glide surface record of a buffer: buf->drawableBuf, a copy of the head of the WGL buffer's private_
** (__WGLwinSurface, wgldraw.h) */

/* 69029800 (size 112) -- hwBufferMakeCurrent: buffer makeCurrent of the Glide path: copy the WGL buffer's surface
** record and select the surface (the extension takes a second argument from Glide version 2 on) */
void __fastcall hwBufferMakeCurrent(__GLbufferHeadICD *buf, __WGLbuffer *other, __GLcontext *gc)
{
    __WGLwinSurface *src = (__WGLwinSurface *)other->private_;
    __WGLwinSurface *dst = buf->drawableBuf;

    __glMakeCurrentBuffer(buf, other, gc);
    if (gc->hwcx.windowed) {
        dst->surf = src->surf;
        dst->rect = src->rect;
        if (src->inVideo) {
            if (gc->hwcx.glideVerFlags & 0xfffffffe) {
                grSurfaceSetRenderingSurfaceExt2(dst->surf, 0);
                return;
            }
            grSurfaceSetRenderingSurfaceExt1(dst->surf);
        }
    }
}

/* 69029870 -- hwLoseCurrentBuffer of a hardware colour buffer: __glLoseCurrentBuffer; windowed, Glide's rendering
** surface is reset (grSurfaceSetRenderingSurfaceExt(0), newer Glide versions (0, 0)) and the drawable
** block's surface word cleared */
void __fastcall hwLoseCurrentBuffer(__GLbufferHeadICD *buf, __GLcontext *gc)
{
    __WGLwinSurface *surf = buf->drawableBuf;

    __glLoseCurrentBuffer(buf);
    if (gc->hwcx.windowed != 0) {
        if (gc->hwcx.glideVerFlags & ~1) {
            grSurfaceSetRenderingSurfaceExt2(0, 0);
        }
        else {
            grSurfaceSetRenderingSurfaceExt1(0);
        }
        surf->surf = NULL;
    }
}

/* 690298c0 (size 281) -- hwBufferResize: buffer resize of the Glide path: follow the WGL buffer's surface (re-attach
** through the drawable when it changed), copy its rectangle and re-select it when this is the draw
** buffer; returns 0 for an empty size, else 1 or 0xf0 (no surface / software rendering requested).
** The gc test comes after gc->hwcx.windowed was read (as in the original). */
GLubyte __fastcall hwBufferResize(__GLbufferHeadICD *buf, GLuint edx_unused, GLuint unused3, GLint w,
                                GLint h, GLint mode)
{
    __GLcontext *gc = buf->gc;
    __WGLwinSurface *dst = buf->drawableBuf;
    __WGLwinSurface *src = (__WGLwinSurface *)buf->wglBuf->private_;
    __WGLdrawable *dp = gc->drawablePrivate;

    if (w == 0 && h == 0)
        return 0;
    if (!gc->hwcx.windowed)
        return 1;
    if (mode == 1 && !dp->modes->singleBuffered)
        return 1;
    if (gc != 0) {
        if (dst->surf != src->surf && src->surf != 0) {
            (*dp->lockDP)(dp, gc);
            dst->surf = src->surf;
            (*buf->gc->drawablePrivate->unlockDP)(buf->gc->drawablePrivate);
        }
        dst->rect = src->rect;
        if (buf->gc->drawBuffer != 0 && src->inVideo && (void *)buf->gc->drawBuffer == (void *)buf &&
            h != 0 && w != 0 && gc->hwcx.glideContext != 0) {
            if (gc->hwcx.glideVerFlags & 0xfffffffe)
                grSurfaceSetRenderingSurfaceExt2(dst->surf, 0);
            else
                grSurfaceSetRenderingSurfaceExt1(dst->surf);
        }
    }
    if (src->inVideo == 0 || gc->hwcx.swRequested != 0)
        return 0xf0;
    return 1;
}

/* 6905fbe0 (size 488) -- hwReadPixels: glReadPixels after the argument checks */
void __fastcall hwReadPixels(__GLcontext *gc, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
                             GLenum type, GLvoid *pixels)
{
    __GLpixelSpanInfoICD spanInfo;
    __WGLdrawable *dp;
    SstDrawProcs *dprocs;
    GLboolean drawableLock;
    GLuint mask;
    __WGLbuffer *db;

    dp = (__WGLdrawable *)gc->drawablePrivate;
    db = &dp->depthBuffer;
    dprocs = (SstDrawProcs *)db->private_;
    if (format == GL_DEPTH_COMPONENT) {
        if ((gc->fx.zPrecision32 || gc->fx.zPrecision16) && gc->fx.fullscreen) {
            gc->fx.zPrecision32 = 0;
            gc->fx.zPrecision16 = 0;
            if (gc->fx.lastDepthFunc != gc->fx.depthDirection) {
                GLdouble f;

                __glsstim_DepthFunc(gc->fx.lastDepthFunc);
                GR_FLUSH_STATE(gc);
                f = (GLdouble)((__GLfloat)(gc->constants.half) * gc->constants.depthScale);
                gc->state.viewport.scale.z = (gc->state.viewport.zFar - gc->state.viewport.zNear) * f;
                gc->state.viewport.center.z = (gc->state.viewport.zNear + gc->state.viewport.zFar) * f;
                gc->state.polygon.factor = gc->fx.savedPolygonFactor;
                gc->state.polygon.units = gc->fx.savedPolygonUnits;
            }
        }
    }
    if (format == GL_DEPTH_COMPONENT || (format == GL_STENCIL_INDEX && gc->modes.depthBits == 24)) {
        drawableLock = GL_TRUE;
    } else {
        drawableLock = GL_FALSE;
    }
    __glInitReadPixelsInfo(gc, &spanInfo, x, y, width, height, format, type, pixels);
    if (!__glClipReadPixels(gc, &spanInfo)) return;
    __glInitPacker(gc, &spanInfo);
    if (drawableLock) {
        (*dprocs->lock)(db, dp);
    } else {
        mask = gc->buffers.readBufferMask;
        if (format == GL_DEPTH_COMPONENT) {
            mask = 0x20;
        } else if (format == GL_STENCIL_INDEX) {
            mask = 0x40;
        }
        (*gc->buffers.lockBuffers)(gc, mask);
    }
    __glGenericPickReadPixels(gc, &spanInfo);
    if (drawableLock) {
        (*dprocs->unlock)(db, dp);
    } else {
        mask = gc->buffers.readBufferMask;
        if (format == GL_DEPTH_COMPONENT) {
            mask = 0x20;
        } else if (format == GL_STENCIL_INDEX) {
            mask = 0x40;
        }
        (*gc->buffers.unlockBuffers)(gc, mask);
    }
}

/* 69067cc9 (size 200) -- hwFinishFromCaller: called (through 69067da0) with the caller's return address: a call
** from inside opengl32!wglSwapBuffers swaps (NT4) / is ignored (2000, 9x), anything else just finishes */
void __fastcall hwFinishFromCaller(int retaddr)
{
    int d;
    __GL_SETUP_NOT_IN_BEGIN();

    if (!g_opengl32SwapBuffers) {
        g_opengl32SwapBuffers = GetProcAddress(GetModuleHandleA("opengl32"), "wglSwapBuffers");
    }
    if (g_isNT4) {
        if (!g_opengl32SwapBuffers) return;
        d = retaddr - (int)g_opengl32SwapBuffers < 0 ? (int)g_opengl32SwapBuffers - retaddr : retaddr - (int)g_opengl32SwapBuffers;
        if (d < 0x100) {
            wglRcSwapBuffers(gc);
            return;
        }
        (*gc->procs.finish)(gc);
        return;
    } else if (g_isWin2000) {
        d = retaddr - (int)g_opengl32SwapBuffers < 0 ? (int)g_opengl32SwapBuffers - retaddr : retaddr - (int)g_opengl32SwapBuffers;
        if (d <= 0x10000) return;
        (*gc->procs.finish)(gc);
        return;
    } else {
        d = retaddr - (int)g_opengl32SwapBuffers < 0 ? (int)g_opengl32SwapBuffers - retaddr : retaddr - (int)g_opengl32SwapBuffers;
        if (d < 0x100) return;
    }
    (*gc->procs.finish)(gc);
}

/* 69067db0 (size 1311) -- __glsstim_Clear: glClear */
void __stdcall __glsstim_Clear(GLbitfield mask)
{
    GLuint bits;
    GLint depthClear;
    __GLfloat scale;
    GLubyte alpha, stencil;
    GLuint color;
    GLint depthBits;
    GLint beginMode;
    __GL_SETUP();

    bits = 0;
    beginMode = __gl_beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
        if (beginMode == __GL_NEED_VALIDATE) {
            (*gc->procs.validate)(gc);
            __gl_beginMode = __GL_NOT_IN_BEGIN;
        } else {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
    } else {
        (*gc->drawablePrivate->lockDP)(gc->drawablePrivate, gc);
        (*gc->drawablePrivate->unlockDP)(gc->drawablePrivate);
    }
    (*gc->procs.waitIdle)(gc);

    if (mask & ~(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    if (gc->renderMode != GL_RENDER) return;

    if (mask & GL_COLOR_BUFFER_BIT) {
        switch (gc->state.raster.drawBuffer) {
          case GL_FRONT:
            if (!gc->hwcx.has32bpp && (gc->state.raster.rMask != gc->state.raster.gMask ||
                                       gc->state.raster.rMask != gc->state.raster.bMask)) {
                (*gc->front->clear)(gc->front);
            } else {
                bits = 1;
            }
            break;
          case GL_BACK:
            if (!gc->hwcx.has32bpp && (gc->state.raster.rMask != gc->state.raster.gMask ||
                                       gc->state.raster.rMask != gc->state.raster.bMask)) {
                (*gc->back->clear)(gc->back);
            } else {
                bits = 1;
            }
            break;
          case GL_FRONT_AND_BACK:
            (*gc->front->clear)(gc->front);
            (*gc->back->clear)(gc->back);
            break;
        }
    }

    if ((mask & GL_DEPTH_BUFFER_BIT) && gc->modes.haveDepthBuffer) {
        if (gc->fx.fullscreen &&
            (((depthBits = gc->modes.depthBits) > 16 && gc->fx.zPrecision32) ||
             (depthBits == 16 && gc->fx.zPrecision16))) {
            if (gc->fx.depthClearPending == 1) {
                bits |= 2;
                depthClear = (GLint)(gc->constants.half * gc->constants.depthScale * gc->state.depth.clear);
                gc->fx.depthClearPending = 0;
            }
            if (!gc->fx.swapPending) {
                GLdouble f;

                scale = gc->constants.half * gc->constants.depthScale;
                f = (GLdouble)gc->constants.half * scale;
                if (gc->fx.depthDirection == GL_GEQUAL) {
                    gc->fx.depthDirection = 0x203;
                    gc->state.viewport.scale.z = (__GLfloat)((gc->state.viewport.zFar - gc->state.viewport.zNear) * f);
                    gc->state.viewport.center.z = (__GLfloat)((gc->state.viewport.zNear + gc->state.viewport.zFar) * f);
                    if (*gc->hwcx.grState[__GL_GR_DEPTH_FUNC] != 3) {
                        *gc->hwcx.grState[__GL_GR_DEPTH_FUNC] = 3;
                        gc->hwcx.grDirty |= 1 << 1;
                    }
                    GR_FLUSH_STATE(gc);
                } else if (gc->fx.depthDirection == GL_LEQUAL) {
                    gc->fx.depthDirection = 0x206;
                    if ((depthBits > 16 && gc->fx.zPrecision32 == 1) || (depthBits == 16 && gc->fx.zPrecision16 == 1)) {
                        bits |= 2;
                        depthClear = (GLint)(scale * gc->state.depth.clear);
                    }
                    gc->state.viewport.scale.z = (__GLfloat)-((gc->state.viewport.zFar - gc->state.viewport.zNear) * f);
                    gc->state.viewport.center.z = (__GLfloat)(gc->constants.depthScale -
                                                              (gc->state.viewport.zNear + gc->state.viewport.zFar) * f);
                    if (*gc->hwcx.grState[__GL_GR_DEPTH_FUNC] != 6) {
                        *gc->hwcx.grState[__GL_GR_DEPTH_FUNC] = 6;
                        gc->hwcx.grDirty |= 1 << 1;
                    }
                    GR_FLUSH_STATE(gc);
                }
                if ((gc->fx.depthDirection == GL_GEQUAL &&
                     (gc->fx.offsetDepthFunc == GL_LESS || gc->fx.offsetDepthFunc == GL_LEQUAL)) ||
                    (gc->fx.depthDirection == GL_LEQUAL &&
                     (gc->fx.offsetDepthFunc == GL_GREATER || gc->fx.offsetDepthFunc == GL_GEQUAL))) {
                    gc->fx.swapPending = 1;
                    gc->state.polygon.factor = gc->fx.savedPolygonFactor * -1.0f;
                    gc->state.polygon.units = gc->fx.savedPolygonUnits * -1.0f;
                } else {
                    gc->state.polygon.factor = gc->fx.savedPolygonFactor;
                    gc->state.polygon.units = gc->fx.savedPolygonUnits;
                    gc->fx.swapPending = 1;
                }
            } else {
                bits |= 2;
                depthClear = (GLint)(gc->constants.half * gc->constants.depthScale * gc->state.depth.clear);
            }
        } else {
            bits |= 2;
            depthClear = (GLint)(gc->constants.depthScale * gc->state.depth.clear);
        }
    }

    if ((mask & GL_STENCIL_BUFFER_BIT) && gc->modes.haveStencilBuffer) {
        if (gc->hwcx.has32bpp) {
            bits |= 4;
        } else {
            (*gc->stencilBuffer.clear)(&gc->stencilBuffer);
        }
    }

    if (bits) {
        if (bits != (GLuint)(gc->hwcx.has32bpp ? 7 : 3)) {
            hwMaskBufferWrites(gc, bits);
        }
        color = (((GLint)(gc->state.raster.clear.r * 255.0f) << 8 |
                  (GLint)(gc->state.raster.clear.g * 255.0f)) << 8) |
                (GLint)(gc->state.raster.clear.b * 255.0f);
        alpha = (GLubyte)(GLint)(gc->state.raster.clear.a * 255.0f);
        stencil = (GLubyte)gc->state.stencil.clear;
        if ((bits & 2) && !(*(GLubyte *)&gc->state.enables.general & __GL_DEPTH_TEST_ENABLE)) {
            grDepthMask(FXTRUE);
            grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
            gc->hwcx.grDirty |= 1 << 3;
        }
        if (gc->hwcx.has32bpp) {
            if ((bits & 4) && !(gc->state.enables.general & __GL_STENCIL_TEST_ENABLE)) {
                grEnable(7 /* GR_STENCIL_MODE_EXT (g3ext.h) */);
            }
            ((void (__stdcall *)(GLuint, GLubyte, GLint, GLubyte))gc->hwcx.grBufferClearExt)
                ((GLuint)alpha << 24 | color, alpha, depthClear, stencil);
            if ((bits & 4) && !(gc->state.enables.general & __GL_STENCIL_TEST_ENABLE)) {
                grDisable(7 /* GR_STENCIL_MODE_EXT (g3ext.h) */);
            }
        } else {
            grBufferClear(color, alpha, depthClear);
        }
        GR_FLUSH_STATE(gc);
    }

    if ((mask & GL_ACCUM_BUFFER_BIT) && gc->modes.haveAccumBuffer) {
        (*gc->accumBuffer.clear)(&gc->accumBuffer);
    }
}

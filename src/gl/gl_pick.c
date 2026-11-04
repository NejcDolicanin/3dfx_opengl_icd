/*
** gl_pick.c -- original VA range 6907c000..69080000: s_pick.c (render-bitmap, clip, fog and
** transform pick procs, __glGenericValidate), the empty procs (__glNop*), __glEndPrim,
** s_buffer.c (buffer init / make current / lose current, render-buffer and colour+depth locks,
** __glFreePrivate, __glMakeCurrentBuffers, __glLoseCurrentBuffers, __glFreeBufData), the core
** exports 8..10, s_contxt.c __glSetError, dl_opt.c (list-execute Begin per primitive, the
** list-compiled errors), the dlist sentinel / nop / execute procs, __glMultiplyMatrix and
** __glUpdateDepthRange.
** Then the rest of s_pick.c, in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "siprotos.h"
#include "glsitypes.h"                      /* __GLbitmap */
#include "glshade.h"
#include "icdprotos.h"
#include <stddef.h>
#include "si_names.h"
#include "icdglob.h"


#define GL_TABLE_TOO_LARGE_EXT  0x8031
/* dl_opt.c: the generic list opcodes after the API ones */

typedef void (APIENTRY *__GLbeginProc7c0)(GLenum mode);
typedef void (__fastcall *__GLdlFreeProc7c0)(__GLcontext *gc, void *p);
#define __GL_BEGIN_DISPATCH()   ((__GLbeginProc7c0)__glCurrentGC()->currentDispatchState.fn[8])   /* glBegin */

/* ---- forward declarations ---- */
int __fastcall __glGenericDlistOptimizer(void);
const GLubyte *__fastcall __glle_TableTooLarge(const GLubyte *PC);
const GLubyte *__fastcall __glle_UnimplementedExtension(const GLubyte *PC);

/* 6907c710 -- __glGenericPickRenderBitmapProcs (s_pick.c) */
void __fastcall __glGenericPickRenderBitmapProcs(__GLcontext *gc)
{
    gc->procs.renderBitmap = (__GLproc)__glRenderBitmap;    /* __glRenderBitmap */
}

/* 6907c720 -- __glGenericPickClipProcs (s_pick.c): the fast line clippers by shading model */
void __fastcall __glGenericPickClipProcs(__GLcontext *gc)
{
    if (gc->state.light.shadingModel == GL_FLAT) {
        gc->procs.clipLine = (__GLproc)__glFastClipFlatLine;    /* __glFastClipFlatLine */
        gc->procs.clipTriangle = (__GLproc)__glClipTriangle;
    } else {
        gc->procs.clipLine = (__GLproc)__glFastClipSmoothLine;    /* __glFastClipSmoothLine */
        gc->procs.clipTriangle = (__GLproc)__glClipTriangle;
    }
}

/* 6907cf30 -- __glGenericPickFogProcs (s_pick.c).  ICD: EXT_fog_coord picks the vertex fog from the
** eye z (GL_FRAGMENT_DEPTH_EXT) or from the fog coordinate.
*/
void __fastcall __glGenericPickFogProcs(__GLcontext *gc)
{
    if (gc->state.enables.general & __GL_FOG_ENABLE) {
        if (gc->state.fog.coordSource == GL_FRAGMENT_DEPTH_EXT) {
            if (gc->state.hints.fog == GL_NICEST) {
                gc->procs.fogVertex = 0;    /* better not be called */
            } else if (gc->state.fog.mode == GL_LINEAR) {
                gc->procs.fogVertex = (__GLfogVertexProc)__glFogVertexLinear;
            } else {
                gc->procs.fogVertex = (__GLfogVertexProc)__glFogVertex;
            }
        } else if (gc->state.hints.fog == GL_NICEST) {
            gc->procs.fogVertex = 0;
        } else if (gc->state.fog.mode == GL_LINEAR) {
            gc->procs.fogVertex = (__GLfogVertexProc)__glFogVertexLinearFogCoord;
        } else {
            gc->procs.fogVertex = (__GLfogVertexProc)__glFogVertexFogCoord;
        }
        gc->procs.fogPoint = (__GLproc)__glFogFragmentSlow;    /* __glFogFragmentSlow */
        gc->procs.fogColor = (__GLproc)__glFogColorSlow;    /* __glFogColorSlow */
    } else {
        gc->procs.fogVertex = 0;
        gc->procs.fogPoint = 0;
        gc->procs.fogColor = 0;
    }
}

/* 6907d5f0 -- __glGenericPickTransformProcs (s_pick.c): the matrix stack procs of the matrix mode */
void __fastcall __glGenericPickTransformProcs(__GLcontext *gc)
{
    switch (gc->state.transform.matrixMode) {
      case GL_MODELVIEW:
        gc->procs.pushMatrix = (__GLproc)__glPushModelViewMatrix;
        gc->procs.popMatrix = (__GLproc)__glPopModelViewMatrix;
        gc->procs.loadIdentity = (__GLproc)__glLoadIdentityModelViewMatrix;
        break;
      case GL_PROJECTION:
        gc->procs.pushMatrix = (__GLproc)__glPushProjectionMatrix;
        gc->procs.popMatrix = (__GLproc)__glPopProjectionMatrix;
        gc->procs.loadIdentity = (__GLproc)__glLoadIdentityProjectionMatrix;
        break;
      case GL_TEXTURE:
        gc->procs.pushMatrix = (__GLproc)__glPushTextureMatrix;
        gc->procs.popMatrix = (__GLproc)__glPopTextureMatrix;
        gc->procs.loadIdentity = (__GLproc)__glLoadIdentityTextureMatrix;
        break;
    }
}

/* 6907d6d0 -- __glGenericValidate (s_pick.c; procs.validate): the ICD's second pick phase, then the
** current dispatch table is copied onto itself (EDX passes through to the pick).
*/
void __fastcall __glGenericValidate(__GLcontext *gc, int arg)
{
    (*gc->procs.pickAllProcsPhase2)(gc, arg);
    __glCopyDispatchTable(&__glCurrentGC()->currentDispatchState, &__glCurrentGC()->currentDispatchState);
}

/* 6907da10 -- __glNop1 */
void __fastcall __glNop1()
{
}

/* 6907da20 -- __glNop2 (s_prim.c; the third of the fastcall __glNop0 .. __glNop6 run): procs.vertex outside
** Begin / End */
void __fastcall __glNop2()
{
}

/* 6907da80 -- __glNop4b: a second empty 4-argument proc (__glNop4; texture fetch) */
void __fastcall __glNop4b(int a1, int a2, int a3, int a4)
{
}

/* 6907da90 -- __glEndPrim (s_prim.c) */
void __fastcall __glEndPrim(__GLcontext *gc)
{
    gc->procs.vertex = (__GLproc)__glNop2;
    gc->procs.endPrim = (__GLproc)__glEndPrim;
}

/* 6907dac0 -- __glInitBuffer (s_buffer.c): no gc parameter in the ICD */
void __fastcall __glInitBuffer(__GLbufferICD *buf)
{
    buf->gc = NULL;
    buf->drawableBuf = NULL;
    buf->wglBuf = NULL;
    buf->resize = NULL;
    buf->makeCurrent = (__GLproc)__glMakeCurrentBuffer;
    buf->loseCurrent = (__GLproc)__glLoseCurrentBuffer;
    buf->freeBuf = NULL;
}

/* 6907dae0 -- __glMakeCurrentBuffer (s_buffer.c): the ICD keeps the WGL buffer in buf->wglBuf (SI: other) */
void __fastcall __glMakeCurrentBuffer(__GLbufferICD *buf, __WGLbuffer *drawableBuf, __GLcontext *gc)
{
    buf->wglBuf = drawableBuf;
    buf->gc = gc;
}

/* 6907daf0 -- __glLoseCurrentBuffer (s_buffer.c); the gc argument is not declared (EDX, unused) */
void __fastcall __glLoseCurrentBuffer(__GLbufferICD *buf)
{
    buf->gc = NULL;
    buf->wglBuf = NULL;
}

/* 6907dca0 -- __glLockRenderBuffers (s_buffer.c) */
void __fastcall __glLockRenderBuffers(__GLcontext *gc)
{
    __glLockBuffer(gc, gc->buffers.renderBufferMask);
}

/* 6907dcb0 -- __glUnlockRenderBuffers (s_buffer.c) */
void __fastcall __glUnlockRenderBuffers(__GLcontext *gc)
{
    __glUnlockBuffer(gc, gc->buffers.renderBufferMask);
}

/* 6907dd60 -- __glLockColorBufferDepthBuffer (s_buffer.c): the ICD locks the depth buffer first */
void __fastcall __glLockColorBufferDepthBuffer(__GLcontext *gc)
{
    __WGLdrawable *dp = gc->drawablePrivate;
    __WGLbuffer *drawableBuf;

    (*dp->lockDP)(dp, gc);

    if ((*gc->buffers.obtainSLock)(gc)) return;

    /* lock depth */
    drawableBuf = gc->depthBuffer.buf.wglBuf;
    (*drawableBuf->lock)(drawableBuf, dp);

    /* lock color */
    drawableBuf = gc->buffers.bufferToLock->wglBuf;
    (*drawableBuf->lock)(drawableBuf, dp);

    if (gc->buffers.lockDevice) (*gc->buffers.lockDevice)(gc);
}

/* 6907de60 -- __glFreePrivate (s_buffer.c; dp->freeBuffers): each buffer's freePrivate proc, then
** dp->windowRect is cleared.
*/
void __fastcall __glFreePrivate(__WGLdrawable *dp)
{
    if (dp->frontBuffer.freePrivate) {
        (*dp->frontBuffer.freePrivate)(&dp->frontBuffer, dp);
    }
    if (dp->backBuffer.freePrivate) {
        (*dp->backBuffer.freePrivate)(&dp->backBuffer, dp);
    }
    if (dp->accumBuffer.freePrivate) {
        (*dp->accumBuffer.freePrivate)(&dp->accumBuffer, dp);
    }
    if (dp->depthBuffer.freePrivate) {
        (*dp->depthBuffer.freePrivate)(&dp->depthBuffer, dp);
    }
    if (dp->stencilBuffer.freePrivate) {
        (*dp->stencilBuffer.freePrivate)(&dp->stencilBuffer, dp);
    }
    dp->windowRect = NULL;
}

/* 6907e130 -- __glMakeCurrentBuffers (s_buffer.c) */
void __fastcall __glMakeCurrentBuffers(__GLcontext *gc)
{
    __WGLdrawable *dp = ((__WGLdrawable *(__fastcall *)(__GLcontext *))gc->imports[6])(gc);   /* getDrawablePrivate */

    /* get the drawable private */
    gc->drawablePrivate = dp;

    /* update the call back so that we will free anything we allocate */
    dp->freeBuffers = __glFreePrivate;

    /* load updated info from the drawable */
    if ((GLuint)gc->drawablePrivate->yInverted != gc->constants.yInverted) {
        __GL_DELAY_VALIDATE_MASK(gc, 0xffffffff);
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

    (*gc->frontBuffer.buf.makeCurrent)(&gc->frontBuffer.buf, &dp->frontBuffer, gc);
    if (gc->modes.doubleBufferMode) {
        (*gc->backBuffer.buf.makeCurrent)(&gc->backBuffer.buf, &dp->backBuffer, gc);
    }
    if (gc->modes.haveAccumBuffer) {
        (*gc->accumBuffer.buf.makeCurrent)(&gc->accumBuffer.buf, &dp->accumBuffer, gc);
    }
    if (gc->modes.haveDepthBuffer) {
        (*gc->depthBuffer.buf.makeCurrent)(&gc->depthBuffer.buf, &dp->depthBuffer, gc);
    }
    if (gc->modes.haveStencilBuffer) {
        (*gc->stencilBuffer.buf.makeCurrent)(&gc->stencilBuffer.buf, &dp->stencilBuffer, gc);
    }

    (*dp->unlockDP)(dp);
}

/* 6907e260 -- __glLoseCurrentBuffers (s_buffer.c): the ICD keeps gc->drawablePrivate */
void __fastcall __glLoseCurrentBuffers(__GLcontext *gc)
{
    __WGLdrawable *dp = gc->drawablePrivate;

    (*dp->lockDP)(dp, gc);

    (*gc->frontBuffer.buf.loseCurrent)(&gc->frontBuffer.buf, gc);
    if (gc->modes.doubleBufferMode) {
        (*gc->backBuffer.buf.loseCurrent)(&gc->backBuffer.buf, gc);
    }
    if (gc->modes.haveAccumBuffer) {
        (*gc->accumBuffer.buf.loseCurrent)(&gc->accumBuffer.buf, gc);
    }
    if (gc->modes.haveDepthBuffer) {
        (*gc->depthBuffer.buf.loseCurrent)(&gc->depthBuffer.buf, gc);
    }
    if (gc->modes.haveStencilBuffer) {
        (*gc->stencilBuffer.buf.loseCurrent)(&gc->stencilBuffer.buf, gc);
    }

    (*dp->unlockDP)(dp);
}

/* 6907e2f0 -- __glFreeBufData (s_buffer.c): per-context tables of the buffers.  The back buffer
** goes through the front buffer's free proc (as in the SI).
*/
void __fastcall __glFreeBufData(__GLcontext *gc)
{
    if (gc->frontBuffer.buf.freeBuf) {
        (*gc->frontBuffer.buf.freeBuf)(&gc->frontBuffer.buf, gc);
    }
    if (gc->backBuffer.buf.freeBuf) {
        (*gc->frontBuffer.buf.freeBuf)(&gc->backBuffer.buf, gc);
    }
    if (gc->stencilBuffer.buf.freeBuf) {
        (*gc->stencilBuffer.buf.freeBuf)(&gc->stencilBuffer.buf, gc);
    }
    if (gc->depthBuffer.buf.freeBuf) {
        (*gc->depthBuffer.buf.freeBuf)(&gc->depthBuffer.buf, gc);
    }
    if (gc->accumBuffer.buf.freeBuf) {
        (*gc->accumBuffer.buf.freeBuf)(&gc->accumBuffer.buf, gc);
    }
}

/* 6907e6b0 -- __glDispatchExec: exports[8] (SI dispatchExec), the current dispatch table */
__GLdispatchTable *__fastcall __glDispatchExec(__GLcontext *gc)
{
    return &gc->currentDispatchState;
}

/* 6907e6c0 -- __glBeginDispatchOverride: exports[9] (SI beginDispatchOverride), nothing to do */
void __fastcall __glBeginDispatchOverride()
{
}

/* 6907e6d0 -- __glEndDispatchOverride: exports[10] (SI endDispatchOverride), nothing to do */
void __fastcall __glEndDispatchOverride()
{
}

/* 6907f36d -- __glSetError (s_contxt.c): the C runtime's getenv / abort instead of the imports.
** The first error sticks until glGetError; procs.error sees every one.
*/
void __fastcall __glSetError(GLenum code)
{
    __GL_SETUP();

    if (getenv("GLERRORABORT")) {
        fprintf(stderr, "__glSetError(): GLERRORABORT set, aborting.\n");
        abort();
    }

    if (!gc->error) {
        gc->error = code;
    }
    if (gc->procs.error) (*gc->procs.error)(gc, code);
}

/* 6907f550 -- __glGenericDlistOptimizer (gc->dlist.optimizer): the material optimizer is its only pass, so it is
** a 5-byte jump to __glDlistOptimizeMaterial */
#ifdef _MSC_VER
__declspec(naked) int __fastcall __glGenericDlistOptimizer() { __asm jmp __glDlistOptimizeMaterial }
#else
int __fastcall __glGenericDlistOptimizer() { return 0; }
#endif

/* 6907f680 -- __glle_Begin_LineLoop (dl_opt.c): glBegin through the current dispatch table */
const GLubyte *__fastcall __glle_Begin_LineLoop(const GLubyte *PC)
{
    (*__GL_BEGIN_DISPATCH())(GL_LINE_LOOP);
    return PC;
}

/* 6907f6a0 -- __glle_Begin_LineStrip (dl_opt.c): glBegin through the current dispatch table */
const GLubyte *__fastcall __glle_Begin_LineStrip(const GLubyte *PC)
{
    (*__GL_BEGIN_DISPATCH())(GL_LINE_STRIP);
    return PC;
}

/* 6907f6c0 -- __glle_Begin_Lines (dl_opt.c): glBegin through the current dispatch table */
const GLubyte *__fastcall __glle_Begin_Lines(const GLubyte *PC)
{
    (*__GL_BEGIN_DISPATCH())(GL_LINES);
    return PC;
}

/* 6907f6e0 -- __glle_Begin_Points (dl_opt.c): glBegin through the current dispatch table */
const GLubyte *__fastcall __glle_Begin_Points(const GLubyte *PC)
{
    (*__GL_BEGIN_DISPATCH())(GL_POINTS);
    return PC;
}

/* 6907f700 -- __glle_Begin_Polygon (dl_opt.c): glBegin through the current dispatch table */
const GLubyte *__fastcall __glle_Begin_Polygon(const GLubyte *PC)
{
    (*__GL_BEGIN_DISPATCH())(GL_POLYGON);
    return PC;
}

/* 6907f720 -- __glle_Begin_TriangleStrip (dl_opt.c): glBegin through the current dispatch table */
const GLubyte *__fastcall __glle_Begin_TriangleStrip(const GLubyte *PC)
{
    (*__GL_BEGIN_DISPATCH())(GL_TRIANGLE_STRIP);
    return PC;
}

/* 6907f740 -- __glle_Begin_TriangleFan (dl_opt.c): glBegin through the current dispatch table */
const GLubyte *__fastcall __glle_Begin_TriangleFan(const GLubyte *PC)
{
    (*__GL_BEGIN_DISPATCH())(GL_TRIANGLE_FAN);
    return PC;
}

/* 6907f760 -- __glle_Begin_Triangles (dl_opt.c): glBegin through the current dispatch table */
const GLubyte *__fastcall __glle_Begin_Triangles(const GLubyte *PC)
{
    (*__GL_BEGIN_DISPATCH())(GL_TRIANGLES);
    return PC;
}

/* 6907f780 -- __glle_Begin_QuadStrip (dl_opt.c): glBegin through the current dispatch table */
const GLubyte *__fastcall __glle_Begin_QuadStrip(const GLubyte *PC)
{
    (*__GL_BEGIN_DISPATCH())(GL_QUAD_STRIP);
    return PC;
}

/* 6907f7a0 -- __glle_Begin_Quads (dl_opt.c): glBegin through the current dispatch table */
const GLubyte *__fastcall __glle_Begin_Quads(const GLubyte *PC)
{
    (*__GL_BEGIN_DISPATCH())(GL_QUADS);
    return PC;
}

/* 6907f7c0 -- __gllc_InvalidValue (dl_opt.c): store the error in the list */
void __stdcall __gllc_InvalidValue(__GLcontext *gc)
{
    __GLdlistOpICD *dlop;

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_InvalidValue;
    __glDlistAppendOp(gc, dlop, __glle_InvalidValue);     /* __glle_InvalidValue */
}

/* 6907f7f0 -- __gllc_InvalidEnum (dl_opt.c): store the error in the list */
void __stdcall __gllc_InvalidEnum(__GLcontext *gc)
{
    __GLdlistOpICD *dlop;

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_InvalidEnum;
    __glDlistAppendOp(gc, dlop, __glle_InvalidEnum);     /* __glle_InvalidEnum */
}

/* 6907f820 -- __gllc_InvalidOperation (dl_opt.c): store the error in the list */
void __stdcall __gllc_InvalidOperation(__GLcontext *gc)
{
    __GLdlistOpICD *dlop;

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_InvalidOperation;
    __glDlistAppendOp(gc, dlop, __glle_InvalidOperation);     /* __glle_InvalidOperation */
}

/* 6907f850 -- __gllc_TableTooLarge (dl_opt.c): store the error in the list */
void __stdcall __gllc_TableTooLarge(__GLcontext *gc)
{
    __GLdlistOpICD *dlop;

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_TableTooLarge;
    __glDlistAppendOp(gc, dlop, __glle_TableTooLarge);     /* __glle_TableTooLarge */
}

/* 6907f880 -- __gllc_UnimplementedExtension (dl_opt.c): store the error in the list */
void __fastcall __gllc_UnimplementedExtension()
{
    __GLdlistOpICD *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_UnimplementedExtension;
    __glDlistAppendOp(gc, dlop, __glle_UnimplementedExtension);     /* __glle_UnimplementedExtension */
}

/* 6907f980 -- __glle_TableTooLarge (dl_opt.c) */
const GLubyte *__fastcall __glle_TableTooLarge(const GLubyte *PC)
{
    __glSetError(GL_TABLE_TOO_LARGE_EXT);
    return PC;
}

/* 6907f9a0 -- __glle_UnimplementedExtension (dl_opt.c) */
const GLubyte *__fastcall __glle_UnimplementedExtension(const GLubyte *PC)
{
    __glSetError(GL_INVALID_OPERATION);
    return PC;
}

/* 6907fa30 -- __glle_Sentinel (s_dlist.c): the dlist sentinel op (ends a segment; __glDoInterpret stops on its
** address) */
const GLubyte *__fastcall __glle_Sentinel()
{
    return 0;
}

/* 6907fa60 -- __glGenericExecuteDlist (s_dlist.c): the execute proc of a generic dlist, run its segment (gc
** unused) */
void __fastcall __glGenericExecuteDlist(__GLcontext *gc, __GLdlistICD *dlist)
{
    __glDoInterpret(dlist->segment);
}

/* 6907fae0 -- __glle_Nop (s_dlist.c): the dlist nop op (pads data to 8 bytes) */
const GLubyte *__fastcall __glle_Nop(const GLubyte *PC)
{
    return PC;
}

/* 6907fcc0 -- __glNopFreeDlist (s_dlist.c): the free proc of a machine dlist block, dlist.free (a tail jump) */
void __fastcall __glNopFreeDlist(__GLcontext *gc, __GLdlistICD *dlist)
{
    (*(__GLdlFreeProc7c0)gc->dlist.free)(gc, dlist);
}

/* 6907fcd0 -- __glNopExecuteDlist (s_dlist.c): the execute proc of a machine dlist block, nothing */
void __fastcall __glNopExecuteDlist(__GLcontext *gc, __GLdlistICD *dlist)
{
}

/* 6907fe30 -- __glMultiplyMatrix (s_xform.c): m = data * m, keeping its matrix type; the ICD
** also picks the depth procs.
*/
void __fastcall __glMultiplyMatrix(__GLcontext *gc, __GLmatrixICD *m, __GLmatrixICD *tm)
{
    (*gc->procs.matrix[3])(m, tm, m);       /* mult */
    __glPickMatrixType(m, tm, m);
    (*gc->procs.classifyMatrix)(gc, m);
}

/* 6907fe60 -- __glUpdateDepthRange (s_xform.c) */
void __fastcall __glUpdateDepthRange(__GLcontext *gc)
{
    __GLviewport *vp = &gc->state.viewport;

    ((void (__fastcall *)(__GLcontext *, GLdouble, GLdouble))SetDepthRange)(gc, vp->zNear, vp->zFar);
}

/* ---- s_pick.c: __glGenericPickTriangleProcs ------------------------------------------------------ */
#define __GL_CULLFLAG_FRONT     __GL_FRONTFACE
#define __GL_CULLFLAG_BACK      __GL_BACKFACE
#define __GL_CULLFLAG_DONTCULL  2
#define __GL_CW                 0
#define __GL_CCW                1
/* texture coordinate procs (gl_texstate.c / gl_accum.c) */
#define __GL_SHADE_PROJSCALED_TEXTURE_ICD   0x800000
#define __GL_SHADE_UVSCALED_TEXTURE_ICD     0x1000000
/* texture object lookups (gl_texstate.c / gl_accum.c) */
#define __GL_SHADE_UVSCALED_TEXTURE 0x1000000
/* s_pick.c __glGenericPickTextureProcs: the procs it installs (only their addresses are used here) */
extern void __fastcall __glNMNFilter(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod, __GLfloat s, __GLfloat t);
extern void __fastcall __glLMNFilter(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod, __GLfloat s, __GLfloat t);
#define __glNop3                        NopSpanRender
/* the texture object as the pick code sees it: glctx.h __GLtextureICD */
typedef void (__fastcall *__GLtexMakeResidentProc)(__GLcontext *gc, __GLtextureICD *tex, GLuint w, GLint unit);
#define __GL_PICK_TEX(gc, i)    __glCurrentTexture(gc, i)
#define DRAWABLE(cfb)                ((cfb)->buf.wglBuf)
/* ---- s_pick.c: __glGenericPickPixelProcs -----------------------------------------------------
** SI 1.1 form: no colour tables / convolution / pixelModeFlags; the pixel maps live in
** gc->state.pixel.map[] ({size, ?, base}); fastRGBA is decided here. */
/* ---- s_depth.c: pick the depth procs ------------------------------------------------------------ */
typedef void (__fastcall *__GLdepthPick)(__GLcontext *gc, __GLdepthBufferICD *dfb, GLint ix);
/* ---- gc validation and drawable buffer lock / unlock (gl_pick.c range) ---- */
extern void __fastcall __glGenericPickTextureEnables(__GLcontext *gc);

/* 6907c540 -- __glGenericPickTriangleProcs (sststate.c calls it with two parameters) */
void __fastcall __glGenericPickTriangleProcs(__GLcontext *gc, GLuint edx_unused)
{
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);

    gc->procs.vertexTFan = (__GLproc)__glOtherTFanVertexLock;
    gc->procs.vertexEvenTStrip = (__GLproc)__glEvenTStripVertexLock;
    gc->procs.vertexOddTStrip = (__GLproc)__glOddTStripVertexLock;
    gc->procs.vertex3rdTris = (__GLproc)__gl3rdTrianglesVertexLock;
    gc->procs.vertex4thQStrip = (__GLproc)__gl4thQStripVertexLock;
    gc->procs.vertex4thQuads = (__GLproc)__gl4thQuadsVertexLock;
    gc->procs.vertexPoly = (__GLproc)__glPolygonVertexLock;
    gc->procs.endPoly = (__GLproc)__glEndPolygonLock;

    /* Set the face cull and front face directions */
    if (modeFlags & __GL_SHADE_CULL_FACE) {
	switch (gc->state.polygon.cull) {
	  case GL_FRONT:
	    gc->polygon.cullFace = __GL_CULLFLAG_FRONT;
	    break;
	  case GL_BACK:
	    gc->polygon.cullFace = __GL_CULLFLAG_BACK;
	    break;
	  case GL_FRONT_AND_BACK:
	    gc->procs.renderTriangle = (__GLproc)__glDontRenderTriangle;
	    gc->procs.fillTriangle = 0;		/* Done to find bugs */
	    return;
	}
    } else {
	gc->polygon.cullFace = __GL_CULLFLAG_DONTCULL;
    }

    /* Set front face direction */
    switch (gc->state.polygon.frontFaceDirection) {
      case GL_CW:
	if (gc->constants.yInverted) {
	    gc->polygon.face[__GL_CW] = __GL_BACKFACE;
	    gc->polygon.face[__GL_CCW] = __GL_FRONTFACE;
	} else {
	    gc->polygon.face[__GL_CW] = __GL_FRONTFACE;
	    gc->polygon.face[__GL_CCW] = __GL_BACKFACE;
	}
	break;
      case GL_CCW:
	if (gc->constants.yInverted) {
	    gc->polygon.face[__GL_CW] = __GL_FRONTFACE;
	    gc->polygon.face[__GL_CCW] = __GL_BACKFACE;
	} else {
	    gc->polygon.face[__GL_CW] = __GL_BACKFACE;
	    gc->polygon.face[__GL_CCW] = __GL_FRONTFACE;
	}
	break;
    }

    /* Make polygon mode indexable and zero based */
    gc->polygon.mode[__GL_FRONTFACE] = (GLubyte) (gc->state.polygon.frontMode & 0xf);
    gc->polygon.mode[__GL_BACKFACE] = (GLubyte) (gc->state.polygon.backMode & 0xf);

    if (gc->renderMode == GL_FEEDBACK) {
	gc->procs.renderTriangle = (__GLproc)__glFeedbackTriangle;
	gc->procs.fillTriangle = 0;		/* Done to find bugs */
	return;
    }
    if (gc->renderMode == GL_SELECT) {
	gc->procs.renderTriangle = (__GLproc)__glSelectTriangle;
	gc->procs.fillTriangle = 0;		/* Done to find bugs */
	return;
    }
    if ((gc->state.polygon.frontMode == gc->state.polygon.backMode) &&
	(gc->state.polygon.frontMode == GL_FILL)) {
	if (modeFlags & __GL_SHADE_SMOOTH_LIGHT) {
	    gc->procs.renderTriangle = (__GLproc)__glRenderSmoothTriangle;
	} else {
	    gc->procs.renderTriangle = (__GLproc)__glRenderFlatTriangle;
	}
    } else {
	gc->procs.renderTriangle = (__GLproc)__glRenderTriangle;
    }
    if (gc->state.enables.general & __GL_POLYGON_SMOOTH_ENABLE) {
	gc->procs.fillTriangle = (__GLproc)__glFillAntiAliasedTriangle;
    } else {
	gc->procs.fillTriangle = (__GLproc)__glFillTriangle;
    }
    if ((modeFlags & __GL_SHADE_CHEAP_FOG) &&
	!(modeFlags & __GL_SHADE_SMOOTH_LIGHT)) {
	gc->procs.fillTriangle2 = gc->procs.fillTriangle;
	gc->procs.fillTriangle = (__GLproc)__glFillFlatFogTriangle;
    }
}

/* 6907c760 -- __glGenericPickCalcTextureProcs (s_pick.c, one texture unit): calcTextureUnit[1]
** is the raster proc, calcTextureUnit[0] the second-stage (calcTexture2) proc */
void __fastcall __glGenericPickCalcTextureProcs(__GLcontext *gc)
{
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);

    /* Pick coordinate generation function */
    if ((gc->state.enables.texture[0] & __GL_TEXTURE_GEN_S_ENABLE) &&
	(gc->state.enables.texture[0] & __GL_TEXTURE_GEN_T_ENABLE) &&
	!(gc->state.enables.texture[0] & __GL_TEXTURE_GEN_R_ENABLE) &&
	!(gc->state.enables.texture[0] & __GL_TEXTURE_GEN_Q_ENABLE) &&
	(gc->state.texture.unit[0].s.mode ==
		    gc->state.texture.unit[0].t.mode)) {
	/* Use a special function when both s&t are enabled and identical */
	switch (gc->state.texture.unit[0].s.mode) {
	  case GL_EYE_LINEAR:
	    gc->procs.calcTexture = (__GLproc)texGenEyeLinearST;
	    break;
	  case GL_OBJECT_LINEAR:
	    gc->procs.calcTexture = (__GLproc)texGenObjectLinearST;
	    break;
	  case GL_SPHERE_MAP:
	    gc->procs.calcTexture = (__GLproc)__glCalcSphereMap;
	    break;
	}
    } else {
	if (!(gc->state.enables.texture[0] & __GL_TEXTURE_GEN_S_ENABLE) &&
	    !(gc->state.enables.texture[0] & __GL_TEXTURE_GEN_T_ENABLE) &&
	    !(gc->state.enables.texture[0] & __GL_TEXTURE_GEN_R_ENABLE) &&
	    !(gc->state.enables.texture[0] & __GL_TEXTURE_GEN_Q_ENABLE)) {
	    /* Use fast function when both are disabled */
	    gc->procs.calcTexture = (__GLproc)__glCalcTexture;
	} else {
	    gc->procs.calcTexture = (__GLproc)__glCalcMixedTexture;
	}
    }
    gc->procs.calcTextureUnit[1] = gc->procs.calcTexture;

    if ((modeFlags & __GL_SHADE_PROJSCALED_TEXTURE_ICD) &&
	(modeFlags & __GL_SHADE_UVSCALED_TEXTURE_ICD)) {
	gc->procs.calcTextureUnit[0] = gc->procs.calcTexture;
	if (gc->procs.calcTexture == (__GLproc)__glCalcTexture) {
	    gc->procs.calcTexture = (__GLproc)__glFastCalcTexturePerspUVScale;
	} else {
	    gc->procs.calcTexture = (__GLproc)__glCalcTexturePerspUVScale;
	}
    } else if (modeFlags & __GL_SHADE_PROJSCALED_TEXTURE_ICD) {
	gc->procs.calcTextureUnit[0] = gc->procs.calcTexture;
	if (gc->procs.calcTexture == (__GLproc)__glCalcTexture) {
	    gc->procs.calcTexture = (__GLproc)__glFastCalcTexturePersp;
	} else {
	    gc->procs.calcTexture = (__GLproc)__glCalcTexturePersp;
	}
    } else if (modeFlags & __GL_SHADE_UVSCALED_TEXTURE_ICD) {
	gc->procs.calcTextureUnit[0] = gc->procs.calcTexture;
	if (gc->procs.calcTexture == (__GLproc)__glCalcTexture) {
	    gc->procs.calcTexture = (__GLproc)__glFastCalcTextureUVScale;
	} else {
	    gc->procs.calcTexture = (__GLproc)__glCalcTextureUVScale;
	}
    }
}

/* 6907c890 -- __glGenericPickTextureEnables (s_pick.c) for the ICD's TMUs: the current target
** index per unit (-1 = none), its parameters copied into the current texture */
void __fastcall __glGenericPickTextureEnables(__GLcontext *gc)
{
    GLint i, idx;
    __GLtexObjParamICD *params;

    for (i = 0; i < gc->fx.numTMUs; i++) {
	gc->texture.currentIndex[i] = -1;
	if (gc->state.enables.texture[i] & __GL_TEXTURE_2D_ENABLE) {
	    idx = __glTexTargetIndex(gc, GL_TEXTURE_2D, i);
	    if (idx >= 0 && idx < 4) {
		if (__glIsTextureConsistent(gc, __glLookUpTexture(gc, GL_TEXTURE_2D, i), i)) {
		    gc->texture.currentIndex[i] = idx;
		    params = __glLookUpTextureParams(gc, GL_TEXTURE_2D, i);
		}
	    }
	} else if (gc->state.enables.texture[i] & __GL_TEXTURE_1D_ENABLE) {
	    idx = __glTexTargetIndex(gc, GL_TEXTURE_1D, i);
	    if (idx >= 0 && idx < 4) {
		if (__glIsTextureConsistent(gc, __glLookUpTexture(gc, GL_TEXTURE_1D, i), i)) {
		    gc->texture.currentIndex[i] = idx;
		    params = __glLookUpTextureParams(gc, GL_TEXTURE_1D, i);
		}
	    }
	}
	if (__glCurrentTexture(gc, i)) {
	    gc->texture.textureEnabled[i] = GL_TRUE;
	    __glCurrentTexture(gc, i)->params = *params;
	    gc->dirtyMask |= __GL_DIRTY_TEXENABLE_ICD;
	} else {
	    gc->texture.textureEnabled[i] = GL_FALSE;
	    gc->dirtyMask |= __GL_DIRTY_TEXENABLE_ICD;
	}
    }
}

/* 6907c9d0 -- s_pick.c __glGenericPickTextureProcs */
void __fastcall __glGenericPickTextureProcs(__GLcontext *gc)
{
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    GLint i;

    for (i = 0; i < gc->fx.numTMUs; ++i) {
	__GLtextureICD *current = __GL_PICK_TEX(gc, i);

	/* Pick texturing function for the current texture */
	if (current) {
	    GLenum baseFormat;

	    gc->procs.calcLineRho[i] = (__GLproc)__glComputeLineRho;
	    gc->procs.calcPolygonRho[i] = (__GLproc)__glComputePolygonRho;
	    if ((current->params.minFilter != GL_LINEAR) &&
		(current->params.minFilter != GL_NEAREST)) {
		/* mipmapping */
		current->textureFunc = (__GLproc)__glMipMapFragment;

		if ((current->params.magFilter == GL_LINEAR) &&
		    ((current->params.minFilter == GL_NEAREST_MIPMAP_NEAREST) ||
		     (current->params.minFilter == GL_LINEAR_MIPMAP_NEAREST))) {
		    current->c = (__GLfloat)2.0;
		} else {
		    current->c = __GL_ONE_F;
		}
	    } else {
		/* No mipmapping needed */
		if (current->params.minFilter == current->params.magFilter) {
		    /* No rho needed as min/mag application is identical */
		    current->textureFunc = (__GLproc)__glFastTextureFragment;
		    gc->procs.calcLineRho[i] = (__GLproc)__glNopLineRho;
		    gc->procs.calcPolygonRho[i] = (__GLproc)__glNopPolygonRho;
		} else {
		    current->textureFunc = (__GLproc)__glTextureFragment;

		    if ((current->params.magFilter == GL_LINEAR) &&
			((current->params.minFilter == GL_NEAREST_MIPMAP_NEAREST) ||
			 (current->params.minFilter == GL_LINEAR_MIPMAP_NEAREST))) {
			current->c = (__GLfloat)2.0;
		    } else {
			current->c = __GL_ONE_F;
		    }
		}
	    }

	    /* Pick environment function according to base format and mode */
	    baseFormat = current->level[0]->baseFormat;
	    current->baseFormat = baseFormat;
	    switch (gc->state.texture.unit[i].env->mode) {
	      case GL_MODULATE:
		switch (baseFormat) {
		  case GL_LUMINANCE:
		    current->env = (__GLproc)__glTextureModulateL;
		    break;
		  case GL_LUMINANCE_ALPHA:
		    current->env = (__GLproc)__glTextureModulateLA;
		    break;
		  case GL_RGB:
		    current->env = (__GLproc)__glTextureModulateRGB;
		    break;
		  case GL_RGBA:
		    current->env = (__GLproc)__glTextureModulateRGBA;
		    break;
		  case GL_ALPHA:
		    current->env = (__GLproc)__glTextureModulateA;
		    break;
		  case GL_INTENSITY:
		    current->env = (__GLproc)__glTextureModulateI;
		    break;
		  case GL_COLOR_INDEX:
		    current->env = (__GLproc)__glTextureModulateCI;
		    break;
		}
		break;
	      case GL_DECAL:
		switch (baseFormat) {
		  case GL_LUMINANCE:
		    current->env = (__GLproc)__glNop3;
		    break;
		  case GL_LUMINANCE_ALPHA:
		    current->env = (__GLproc)__glNop3;
		    break;
		  case GL_RGB:
		    current->env = (__GLproc)__glTextureDecalRGB;
		    break;
		  case GL_RGBA:
		    current->env = (__GLproc)__glTextureDecalRGBA;
		    break;
		  case GL_ALPHA:
		    current->env = (__GLproc)__glNop3;
		    break;
		  case GL_INTENSITY:
		    current->env = (__GLproc)__glNop3;
		    break;
		}
		break;
	      case GL_BLEND:
		switch (baseFormat) {
		  case GL_LUMINANCE:
		    current->env = (__GLproc)__glTextureBlendL;
		    break;
		  case GL_LUMINANCE_ALPHA:
		    current->env = (__GLproc)__glTextureBlendLA;
		    break;
		  case GL_RGB:
		    current->env = (__GLproc)__glTextureBlendRGB;
		    break;
		  case GL_RGBA:
		    current->env = (__GLproc)__glTextureBlendRGBA;
		    break;
		  case GL_ALPHA:
		    current->env = (__GLproc)__glTextureBlendA;
		    break;
		  case GL_INTENSITY:
		    current->env = (__GLproc)__glTextureBlendI;
		    break;
		}
		break;
	      case GL_REPLACE:
		switch (baseFormat) {
		  case GL_LUMINANCE:
		    current->env = (__GLproc)__glTextureReplaceL;
		    break;
		  case GL_LUMINANCE_ALPHA:
		    current->env = (__GLproc)__glTextureReplaceLA;
		    break;
		  case GL_RGB:
		    current->env = (__GLproc)__glTextureReplaceRGB;
		    break;
		  case GL_RGBA:
		    current->env = (__GLproc)__glTextureReplaceRGBA;
		    break;
		  case GL_ALPHA:
		    current->env = (__GLproc)__glTextureReplaceA;
		    break;
		  case GL_INTENSITY:
		    current->env = (__GLproc)__glTextureReplaceI;
		    break;
		  case GL_COLOR_INDEX:
		    current->env = (__GLproc)__glTextureReplaceCI;
		    break;
		}
		break;
	      case GL_ADD:
		switch (baseFormat) {
		  case GL_COLOR_INDEX:
		    current->env = (__GLproc)__glTextureAddCI;
		    break;
		}
		break;
	    }

	    /* Pick mag/min functions */
	    switch (current->dim) {
	      case 1:
		current->nearest = (__GLproc)__glNearestFilter1;
		current->linear = (__GLproc)__glLinearFilter1;
		break;
	      case 2:
		current->nearest = (__GLproc)__glNearestFilter2;
		current->linear = (__GLproc)__glLinearFilter2;
		break;
	    }

	    /* set min filter function */
	    switch (current->params.minFilter) {
	      case GL_LINEAR:
		if (modeFlags & __GL_SHADE_UVSCALED_TEXTURE) {
		    current->minify = (__GLproc)__glLinearFilterUVScaled;
		} else {
		    current->minify = (__GLproc)__glLinearFilter;
		}
		break;
	      case GL_NEAREST:
		if (modeFlags & __GL_SHADE_UVSCALED_TEXTURE) {
		    current->minify = (__GLproc)__glNearestFilterUVScaled;
		} else {
		    current->minify = (__GLproc)__glNearestFilter;
		}
		break;
	      case GL_NEAREST_MIPMAP_NEAREST:
		current->minify = (__GLproc)__glNMNFilter;
		break;
	      case GL_LINEAR_MIPMAP_NEAREST:
		current->minify = (__GLproc)__glLMNFilter;
		break;
	      case GL_NEAREST_MIPMAP_LINEAR:
		current->minify = (__GLproc)__glNMLFilter;
		break;
	      case GL_LINEAR_MIPMAP_LINEAR:
		current->minify = (__GLproc)__glLMLFilter;
		break;
	    }

	    /* set mag filter function */
	    switch (current->params.magFilter) {
	      case GL_LINEAR:
		if (modeFlags & __GL_SHADE_UVSCALED_TEXTURE) {
		    current->magnify = (__GLproc)__glLinearFilterUVScaled;
		} else {
		    current->magnify = (__GLproc)__glLinearFilter;
		}
		break;
	      case GL_NEAREST:
		if (modeFlags & __GL_SHADE_UVSCALED_TEXTURE) {
		    current->magnify = (__GLproc)__glNearestFilterUVScaled;
		} else {
		    current->magnify = (__GLproc)__glNearestFilter;
		}
		break;
	    }

	    gc->procs.texture[i] = current->textureFunc;
	    if (modeFlags & __GL_SHADE_UVSCALED_TEXTURE) {
		gc->procs.textureRaster[i] = (__GLproc)__glTextureFragmentUVScale;
	    } else {
		gc->procs.textureRaster[i] = current->textureFunc;
	    }
	    gc->constants.texSScale = current->sScale;
	    gc->constants.texTScale = current->tScale;
	} else {
	    gc->procs.texture[i] = 0;
	    gc->procs.textureRaster[i] = 0;
	    gc->dirtyMask |= __GL_DIRTY_TEXENABLE_ICD;
	}

	/* ICD: (re)load the texture into the TMU when it is not resident there */
	if (__GL_PICK_TEX(gc, i) &&
	    !__GL_PICK_TEX(gc, i)->hwHandle[__GL_TMU_SLOT(i)]) {
	    (*(__GLtexMakeResidentProc)__GL_PICK_TEX(gc, i)->makeResident)(gc, __GL_PICK_TEX(gc, i), __GL_PICK_TEX(gc, i)->makeResidentArg, i);
	}
    }
}

/* 6907cff0 -- __glGenericPickBufferProcs */
void __fastcall __glGenericPickBufferProcs(__GLcontext *gc)
{
    __GLbufferMachineICD *buffers;
    GLuint renderBufferMask = 0;
    GLuint drawBufferMask, readBufferMask;

    buffers = &gc->buffers;
    buffers->doubleStore = GL_FALSE;

    gc->buffers.bufferToLock = NULL;

    switch (gc->state.raster.drawBuffer) {
      case GL_FRONT:
        gc->drawBuffer = gc->front;
        drawBufferMask = __GL_FRONT_BUFFER_MASK;
        if (DRAWABLE(gc->front)->lock &&
            DRAWABLE(gc->front)->unlock) {
            renderBufferMask = __GL_FRONT_BUFFER_MASK;
            gc->buffers.bufferToLock = &gc->front->buf;
        }
        break;
      case GL_FRONT_AND_BACK:
        if (gc->modes.doubleBufferMode) {
            gc->drawBuffer = gc->back;
            buffers->doubleStore = GL_TRUE;
            drawBufferMask = __GL_FRONT_BUFFER_MASK | __GL_BACK_BUFFER_MASK;
            if (DRAWABLE(gc->front)->lock &&
                DRAWABLE(gc->front)->unlock) {
                renderBufferMask = __GL_FRONT_BUFFER_MASK;
            }
            if (DRAWABLE(gc->back)->lock &&
                DRAWABLE(gc->back)->unlock) {
                renderBufferMask |= __GL_BACK_BUFFER_MASK;
            }
        } else {
            gc->drawBuffer = gc->front;
            drawBufferMask = __GL_FRONT_BUFFER_MASK;
            if (DRAWABLE(gc->front)->lock &&
                DRAWABLE(gc->front)->unlock) {
                renderBufferMask = __GL_FRONT_BUFFER_MASK;
                gc->buffers.bufferToLock = &gc->front->buf;
            }
        }
        break;
      case GL_BACK:
        gc->drawBuffer = gc->back;
        drawBufferMask = __GL_BACK_BUFFER_MASK;
        if (DRAWABLE(gc->back)->lock &&
            DRAWABLE(gc->back)->unlock) {
            renderBufferMask = __GL_BACK_BUFFER_MASK;
            gc->buffers.bufferToLock = &gc->back->buf;
        }
        break;
      default:
        break;
    }

    switch (gc->state.pixel.readBuffer) {
      case GL_FRONT:
          readBufferMask = __GL_FRONT_BUFFER_MASK;
          break;
      case GL_BACK:
          readBufferMask = __GL_BACK_BUFFER_MASK;
          break;
      default:
          break;
    }

    gc->buffers.lockBuffers = (__GLproc)__glLockBuffer;
    gc->buffers.unlockBuffers = (__GLproc)__glUnlockBuffer;
    gc->buffers.lockRenderBuffers = (__GLproc)__glLockRenderBuffers;
    gc->buffers.unlockRenderBuffers = (__GLproc)__glUnlockRenderBuffers;

    if (__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_DEPTH_TEST) {
        renderBufferMask |= __GL_DEPTH_BUFFER_MASK;
    }
    if (__GL_SHADE_MODEFLAGS(gc) & __GL_SHADE_STENCIL_TEST) {
        renderBufferMask |= __GL_STENCIL_BUFFER_MASK;
    }

    if (((drawBufferMask | __GL_DEPTH_BUFFER_MASK) == renderBufferMask) &&
        (buffers->doubleStore == GL_FALSE)) {
        gc->buffers.lockRenderBuffers = (__GLproc)__glLockColorBufferDepthBuffer;
        gc->buffers.unlockRenderBuffers = (__GLproc)__glUnlockColorBufferDepthBuffer;
    }
    if ((drawBufferMask == renderBufferMask) &&
        (buffers->doubleStore == GL_FALSE)) {
        gc->buffers.lockRenderBuffers = (__GLproc)__glLockColorBuffer;
        gc->buffers.unlockRenderBuffers = (__GLproc)__glUnlockColorBuffer;
    }

    gc->buffers.renderBufferMask = renderBufferMask;
    gc->buffers.drawBufferMask = drawBufferMask;
    gc->buffers.readBufferMask = readBufferMask;
}

/* 6907d1a0 -- __glGenericPickPixelProcs */
void __fastcall __glGenericPickPixelProcs(__GLcontext *gc)
{
    __GLpixelTransferMode *tm;
    __GLpixelMachineICD *pm;
    GLboolean mapColor;
    GLfloat red, green, blue, alpha;
    GLint entry;
    GLuint enables = gc->state.enables.general;
    __GLpixelMapHead *pmap;

    switch (gc->state.pixel.readBuffer) {
      case GL_FRONT:
        gc->readBuffer = gc->front;
        break;
      case GL_BACK:
        gc->readBuffer = gc->back;
        break;
    }

    if (gc->texture.textureEnabled[0] || (enables & __GL_FOG_ENABLE)) {
        gc->procs.pxStore = (__GLproc)__glSlowDrawPixelsStore;
    } else {
        gc->procs.pxStore = gc->procs.store;
    }

    tm = &gc->state.pixel.transferMode;
    pm = &(gc->pixel);
    mapColor = tm->mapColor;
    if (mapColor || gc->modes.rgbMode || tm->indexShift || tm->indexOffset) {
        pm->iToICurrent = GL_FALSE;
        pm->iToRGBACurrent = GL_FALSE;
        pm->modifyCI = GL_TRUE;
    } else {
        pm->modifyCI = GL_FALSE;
    }
    if (tm->mapStencil || tm->indexShift || tm->indexOffset) {
        pm->modifyStencil = GL_TRUE;
    } else {
        pm->modifyStencil = GL_FALSE;
    }
    if (tm->depthScale != __GL_ONE_F || tm->depthBias) {
        pm->modifyDepth = GL_TRUE;
    } else {
        pm->modifyDepth = GL_FALSE;
    }
    if (mapColor || tm->bias.r || tm->bias.g || tm->bias.b || tm->bias.a ||
        tm->scale.r != __GL_ONE_F || tm->scale.g != __GL_ONE_F ||
        tm->scale.b != __GL_ONE_F || tm->scale.a != __GL_ONE_F) {
        pm->modifyRGBA = GL_TRUE;
        pm->rgbaCurrent = GL_FALSE;
    } else {
        pm->modifyRGBA = GL_FALSE;
    }

    if (pm->modifyRGBA) {
        red = gc->state.pixel.transferMode.bias.r;
        green = gc->state.pixel.transferMode.bias.g;
        blue = gc->state.pixel.transferMode.bias.b;
        alpha = gc->state.pixel.transferMode.scale.a +
            gc->state.pixel.transferMode.bias.a;
        if (mapColor) {
            pmap = (__GLpixelMapHead *)&gc->state.pixel.map[__GL_PIXEL_MAP_R_TO_R];
            entry = (GLint)((pmap->size-1) * red + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > pmap->size-1) entry = pmap->size-1;
            red = pmap->base.mapF[entry];

            pmap = (__GLpixelMapHead *)&gc->state.pixel.map[__GL_PIXEL_MAP_G_TO_G];
            entry = (GLint)((pmap->size-1) * green + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > pmap->size-1) entry = pmap->size-1;
            green = pmap->base.mapF[entry];

            pmap = (__GLpixelMapHead *)&gc->state.pixel.map[__GL_PIXEL_MAP_B_TO_B];
            entry = (GLint)((pmap->size-1) * blue + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > pmap->size-1) entry = pmap->size-1;
            blue = pmap->base.mapF[entry];

            pmap = (__GLpixelMapHead *)&gc->state.pixel.map[__GL_PIXEL_MAP_A_TO_A];
            entry = (GLint)((pmap->size-1) * alpha + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > pmap->size-1) entry = pmap->size-1;
            alpha = pmap->base.mapF[entry];
        } else {
            if (red > __GL_ONE_F) red = __GL_ONE_F;
            else if (red < 0) red = 0;
            if (green > __GL_ONE_F) green = __GL_ONE_F;
            else if (green < 0) green = 0;
            if (blue > __GL_ONE_F) blue = __GL_ONE_F;
            else if (blue < 0) blue = 0;
            if (alpha > __GL_ONE_F) alpha = __GL_ONE_F;
            else if (alpha < 0) alpha = 0;
        }
        pm->red0Mod = red * gc->frontBuffer.redScale;
        pm->green0Mod = green * gc->frontBuffer.greenScale;
        pm->blue0Mod = blue * gc->frontBuffer.blueScale;
        pm->alpha1Mod = alpha * gc->frontBuffer.alphaScale;
    } else {
        pm->red0Mod = __GL_ZERO_F;
        pm->green0Mod = __GL_ZERO_F;
        pm->blue0Mod = __GL_ZERO_F;
        pm->alpha1Mod = gc->frontBuffer.alphaScale;
    }

    if (!(enables & (__GL_ALPHA_TEST_ENABLE | __GL_DEPTH_TEST_ENABLE)) && !(enables & __GL_STENCIL_TEST_ENABLE) &&
            gc->state.raster.drawBuffer != GL_NONE &&
            gc->state.raster.drawBuffer != GL_FRONT_AND_BACK &&
            (enables & __GL_DITHER_ENABLE) && !(enables & __GL_BLEND_ENABLE) &&
            !gc->texture.textureEnabled[0] && !(enables & __GL_FOG_ENABLE)) {
        pm->fastRGBA = GL_TRUE;
    } else {
        pm->fastRGBA = GL_FALSE;
    }

    gc->procs.drawPixels = (__GLproc)__glOptPickDrawPixels;
    gc->procs.readPixels = (__GLproc)__glSlowPickReadPixels;
    gc->procs.copyPixels = (__GLproc)__glSlowPickCopyPixels;
}

/* 6907d660 -- __glGenericPickDepthProcs: index = function - GL_NEVER, +8 without depth writes,
** +16 for a buffer deeper than 16 bits, +16 for more than 24 depth bits */
GLint __fastcall __glGenericPickDepthProcs(__GLcontext *gc)
{
    GLint ix;

    __glValidateZCount(&gc->depthBuffer);
    ix = gc->depthBuffer.testFunc - GL_NEVER;
    if (!gc->state.depth.writeEnable) {
	ix += 8;
    }
    if ((gc->depthBuffer.buf.wglBuf)->depth > 16) {
	ix += 16;
    }
    if (gc->modes.depthBits > 24) {
	ix += 16;
    }
    if (gc->modes.haveDepthBuffer) {
	(*(__GLdepthPick)gc->depthBuffer.pick)(gc, &gc->depthBuffer, ix);
    }
    gc->procs.DTPixel = (__GLproc)__glCDTPixel[ix];
    return ix;
}

/* 6907d710 -- __glGenericPickAllProcsPhase2 (s_pick.c): procs.pickAllProcsPhase2, the dirty-bit driven body of
** the older SI's __glGenericValidate */
void __fastcall __glGenericPickAllProcsPhase2(__GLcontext *gc)
{
    if (gc->dirtyMask & __GL_DIRTY_MODELVIEW) {
        __glValidateModelViewForLighting(gc);
        __glValidateCullVertexFromModelView(gc);
        gc->dirtyMask &= ~__GL_DIRTY_MODELVIEW;
    }
    if (gc->dirtyMask || gc->validateMask) {
        if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LIGHTING)) {
            __glGenericPickTextureEnables(gc);
        }
        if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LIGHTING)) {
            (*(__GLproc)gc->front->pick)(gc, gc->front);
            if (gc->modes.doubleBufferMode) {
                (*(__GLproc)gc->back->pick)(gc, gc->back);
            }
            if (gc->modes.haveStencilBuffer) {
                (*gc->stencilBuffer.pick)(gc, &gc->stencilBuffer);
            }
            (*gc->procs.pickBufferProcs)(gc);
            (*gc->procs.pickStoreProcs)(gc);
            (*gc->procs.pickTransformProcs)(gc);
            __glValidateLighting(gc);
            __glValidateCullVertexFromModelView(gc);
            (*gc->procs.pickColorMaterialProcs)(gc);
            (*gc->procs.pickTextureProcs)(gc);
            (*gc->procs.pickCalcTextureProcs)(gc);
            (*gc->procs.pickBlendProcs)(gc);
            (*gc->procs.pickFogProcs)(gc);
            (*gc->procs.pickParameterClipProcs)(gc);
            (*gc->procs.pickClipProcs)(gc);
            (*gc->procs.pickRenderBitmapProcs)(gc);
            if (gc->validateMask & 1) {
                __glValidateAlphaTest(gc);
            }
            if (gc->validateMask & 8) {
                __glValidateIndexTest(gc);
            }
            (*gc->procs.computeClipBox)(gc);
        }
        if (gc->dirtyMask & __GL_DIRTY_POLYGON_STIPPLE) {
            (*gc->procs.convertPolygonStipple)(gc);
        }
        if (gc->dirtyMask & __GL_DIRTY_DEPTH) {
            (*gc->procs.pickAllProcsPhase1)(gc);
        }
        if (gc->dirtyMask & __GL_DIRTY_SCISSOR) {
            (*gc->procs.computeClipBox)(gc);
            (*gc->procs.applyScissor)(gc);
        }
        if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_POLYGON | __GL_DIRTY_LIGHTING | __GL_DIRTY_DEPTH)) {
            (*gc->procs.pickVertexProcs)(gc);
            (*gc->procs.pickSpanProcs)(gc);
            (*gc->procs.pickTriangleProcs)(gc);
        }
        if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_POINT | __GL_DIRTY_LIGHTING)) {
            (*gc->procs.pickPointProcs)(gc);
        }
        if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LINE | __GL_DIRTY_LIGHTING)) {
            (*gc->procs.pickLineProcs)(gc);
        }
        if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_PIXEL | __GL_DIRTY_LIGHTING)) {
            (*gc->procs.pickPixelProcs)(gc);
        }
        if (gc->dirtyMask & (__GL_DIRTY_GENERIC | __GL_DIRTY_LIGHTING)) {
            (*gc->procs.pickVertexArrayProcs)(gc);
        }
        gc->validateMask = 0;
        gc->dirtyMask = 0;
    }
}
#define SHADE_RGB                0x00000001
#define SHADE_SMOOTH             0x00000002
#define SHADE_DEPTH_TEST         0x00000004
#define SHADE_TEXTURE            0x00000008
#define SHADE_STENCIL_TEST       0x00000020
#define SHADE_DITHER             0x00000040
#define SHADE_LOGICOP            0x00000080
#define SHADE_BLEND              0x00000100
#define SHADE_ALPHA_TEST         0x00000200
#define SHADE_SLOW_FOG           0x00001000
#define SHADE_ICD_200000         0x00200000  /* a second per-fragment test, with the alpha test */
#define SHADE_LINE_STIPPLE       0x00008000
#define SHADE_CHEAP_FOG          0x00002000
#define SHADE_SMOOTH_LIGHT       0x00020000
#define SHADE_OWNERSHIP_TEST     0x00400000
#define CFB_ELEMENTSIZE(cfb)     ((cfb)->buf.wglBuf->elementSize)

/*
** 6907c090 -- __glGenericPickLineProcs (s_pick.c; gc->procs.pickLineProcs).  The SI routine without
** the secondary-colour span and the stencil+depth offset variants; faceNeeds[0] comes from the ICD's
** getFaceNeeds0, and the second ICD fragment test (mode bit 0x200000, 690d1090) follows the alpha test.
*/
void __fastcall __glGenericPickLineProcs(__GLcontext *gc, GLint unused)
{
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    __GLproc *sp;
    __GLproc *ssp;
    int spanCount;
    GLboolean wideLine;
    GLboolean replicateLine;
    GLuint aaline;

    /* first initialize the lock versions */
    if ((*gc->vertex.getFaceNeeds0)(gc) == 0) {
        gc->procs.vertexLStrip = (__GLproc)__glOtherLStripVertexFastLock;
    } else {
        gc->procs.vertexLStrip = (__GLproc)__glOtherLStripVertexLock;
    }
    gc->procs.vertex2ndLines = (__GLproc)__glSecondLStripVertex;
    gc->procs.vertex2ndLLoop = (__GLproc)__gl2ndLLoopVertexLock;
    gc->procs.endLLoop = (__GLproc)__glEndLLoopLock;

    gc->procs.renderTrianglePolygonModeLine = (__GLproc)__glRenderTriangleLine;

    if (gc->renderMode == GL_FEEDBACK) {
        gc->procs.renderLine = (__GLproc)__glFeedbackLine;
    } else if (gc->renderMode == GL_SELECT) {
        gc->procs.renderLine = (__GLproc)__glSelectLine;
    } else {
        replicateLine = wideLine = GL_FALSE;

        aaline = gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE;
        if (aaline) {
            gc->procs.renderLine = (__GLproc)__glRenderAntiAliasLine;
        } else {
            gc->procs.renderLine = (__GLproc)__glRenderAliasLine;
        }

        sp = gc->procs.line.lineFuncs;
        ssp = gc->procs.line.stippledLineFuncs;

        if (!aaline && (modeFlags & SHADE_LINE_STIPPLE)) {
            *sp++ = (__GLproc)__glStippleLine;
            *ssp++ = NULL;
        }

        if (!aaline && gc->state.line.aliasedWidth > 1) {
            wideLine = GL_TRUE;
        }
        spanCount = sp - gc->procs.line.lineFuncs;
        gc->procs.line.n = spanCount;

        *sp++ = (__GLproc)__glScissorLine;
        *ssp++ = (__GLproc)__glScissorStippledLine;

        if (!aaline) {
            if (modeFlags & SHADE_STENCIL_TEST) {
                *sp++ = (__GLproc)__glStencilTestLine;
                *ssp++ = (__GLproc)__glStencilTestStippledLine;
                if (modeFlags & SHADE_DEPTH_TEST) {
                    *sp = (__GLproc)__glDepthTestStencilLine;
                    *ssp = (__GLproc)__glDepthTestStencilStippledLine;
                } else {
                    *sp = (__GLproc)__glDepthPassLine;
                    *ssp = (__GLproc)__glDepthPassStippledLine;
                }
                sp++;
                ssp++;
            } else {
                if (modeFlags & SHADE_DEPTH_TEST) {
                    if (gc->depthBuffer.testFunc == GL_NEVER) {
                        gc->procs.line.processLine = (__GLproc)__glNop1;
                        return;
                    } else {
                        *sp++ = (__GLproc)__glDepthTestLine;
                    }
                    *ssp++ = (__GLproc)__glDepthTestStippledLine;
                }
            }
        }

        /* Load phase three procs */
        if (modeFlags & SHADE_RGB) {
            if (modeFlags & SHADE_SMOOTH) {
                *sp = (__GLproc)__glShadeRGBASpan;
                *ssp = (__GLproc)__glShadeRGBASpan;
            } else {
                *sp = (__GLproc)__glFlatRGBASpan;
                *ssp = (__GLproc)__glFlatRGBASpan;
            }
        } else {
            if (modeFlags & SHADE_SMOOTH) {
                *sp = (__GLproc)__glShadeCISpan;
                *ssp = (__GLproc)__glShadeCISpan;
            } else {
                *sp = (__GLproc)__glFlatCISpan;
                *ssp = (__GLproc)__glFlatCISpan;
            }
        }
        sp++;
        ssp++;
        if (modeFlags & SHADE_TEXTURE) {
            *sp++ = (__GLproc)__glTextureSpan;
            *ssp++ = (__GLproc)__glTextureStippledSpan;
        }
        if (modeFlags & SHADE_SLOW_FOG) {
            if (gc->state.hints.fog == GL_NICEST) {
                *sp = (__GLproc)__glFogSpanSlow;
                *ssp = (__GLproc)__glFogStippledSpanSlow;
            } else {
                *sp = (__GLproc)__glFogSpan;
                *ssp = (__GLproc)__glFogStippledSpan;
            }
            sp++;
            ssp++;
        }

        if (aaline) {
            *sp++ = (__GLproc)__glAntiAliasLine;
            *ssp++ = (__GLproc)__glAntiAliasStippledLine;
        }

        if (aaline) {
            if (modeFlags & SHADE_STENCIL_TEST) {
                *sp++ = (__GLproc)__glStencilTestLine;
                *ssp++ = (__GLproc)__glStencilTestStippledLine;
                if (modeFlags & SHADE_DEPTH_TEST) {
                    *sp = (__GLproc)__glDepthTestStencilLine;
                    *ssp = (__GLproc)__glDepthTestStencilStippledLine;
                } else {
                    *sp = (__GLproc)__glDepthPassLine;
                    *ssp = (__GLproc)__glDepthPassStippledLine;
                }
                sp++;
                ssp++;
            } else {
                if (modeFlags & SHADE_DEPTH_TEST) {
                    if (gc->depthBuffer.testFunc == GL_NEVER) {
                        gc->procs.line.processLine = (__GLproc)__glNop1;
                        return;
                    } else {
                        *sp++ = (__GLproc)__glDepthTestLine;
                    }
                    *ssp++ = (__GLproc)__glDepthTestStippledLine;
                }
            }
        }

        if (modeFlags & SHADE_ALPHA_TEST) {
            *sp++ = (__GLproc)__glAlphaTestSpan;
            *ssp++ = (__GLproc)__glAlphaTestStippledSpan;
        }
        if (modeFlags & SHADE_ICD_200000) {
            *sp++ = (__GLproc)__glIndexTestSpan;
            *ssp++ = (__GLproc)__glIndexTestStippledSpan;
        }

        if (gc->buffers.doubleStore) {
            replicateLine = GL_TRUE;
        }
        spanCount = sp - gc->procs.line.lineFuncs;
        gc->procs.line.m = spanCount;

        if (0 == gc->drawBuffer->destMask &&
            0 == (modeFlags & (SHADE_LOGICOP | SHADE_BLEND | SHADE_OWNERSHIP_TEST))) {
            if (gc->modes.colorIndexMode) {
                if (CFB_ELEMENTSIZE(gc->drawBuffer) == 1) {
                    /* 8-bit */
                    if (modeFlags & SHADE_DITHER) {
                        if (modeFlags & (SHADE_SMOOTH | SHADE_TEXTURE | SHADE_SLOW_FOG)) {
                            *sp++ = (__GLproc)__glStoreLine_CI_8_Smooth_Dither;
                            *ssp++ = (__GLproc)__glStoreStippledLine_CI_8_Smooth_Dither;
                        } else {
                            *sp++ = (__GLproc)__glStoreLine_CI_8_Flat_Dither;
                            *ssp++ = (__GLproc)__glStoreStippledLine_CI_8_Flat_Dither;
                        }
                    } else {
                        if (modeFlags & (SHADE_SMOOTH | SHADE_TEXTURE | SHADE_SLOW_FOG)) {
                            *sp++ = (__GLproc)__glStoreLine_CI_8_Smooth;
                            *ssp++ = (__GLproc)__glStoreStippledLine_CI_8_Smooth;
                        } else {
                            *sp++ = (__GLproc)__glStoreLine_CI_8_Flat;
                            *ssp++ = (__GLproc)__glStoreStippledLine_CI_8_Flat;
                        }
                    }
                } else {
                    /* Not fast path (yet) */
                    *sp++ = (__GLproc)__glStoreLine;
                    *ssp++ = (__GLproc)__glStoreStippledLine;
                }
            } else { /* RGBA */
                if (CFB_ELEMENTSIZE(gc->drawBuffer) == 2) {
                    /* 16-bit */
                    if (modeFlags & SHADE_DITHER) {
                        *sp++ = (__GLproc)__glStoreLine_RGB_16_Dither;
                        *ssp++ = (__GLproc)__glStoreStippledLine_RGB_16_Dither;
                    } else {
                        if (modeFlags & (SHADE_SMOOTH | SHADE_TEXTURE | SHADE_SLOW_FOG)) {
                            *sp++ = (__GLproc)__glStoreLine_RGB_16_Smooth;
                            *ssp++ = (__GLproc)__glStoreStippledLine_RGB_16_Smooth;
                        } else {
                            *sp++ = (__GLproc)__glStoreLine_RGB_16_Flat;
                            *ssp++ = (__GLproc)__glStoreStippledLine_RGB_16_Flat;
                        }
                    }
                } else {
                    /* Not fast path (yet) */
                    *sp++ = (__GLproc)__glStoreLine;
                    *ssp++ = (__GLproc)__glStoreStippledLine;
                }
            }
        } else {
            *sp++ = (__GLproc)__glStoreLine;
            *ssp++ = (__GLproc)__glStoreStippledLine;
        }

        spanCount = sp - gc->procs.line.lineFuncs;
        gc->procs.line.l = spanCount;

        sp = &gc->procs.line.wideLineRep;
        ssp = &gc->procs.line.wideStippledLineRep;
        if (wideLine) {
            *sp = (__GLproc)__glWideLineRep;
            *ssp = (__GLproc)__glWideStippleLineRep;
            sp = &gc->procs.line.drawLine;
            ssp = &gc->procs.line.drawStippledLine;
        }
        if (replicateLine) {
            *sp = (__GLproc)__glDrawBothLine;
            *ssp = (__GLproc)__glDrawBothStippledLine;
        } else {
            *sp = (__GLproc)__glNop1;
            *ssp = (__GLproc)__glNop1;
            gc->procs.line.m = gc->procs.line.l;
        }
        if (!wideLine) {
            gc->procs.line.n = gc->procs.line.m;
        }

        if (!wideLine && !replicateLine && spanCount == 3) {
            gc->procs.line.processLine = (__GLproc)__glProcessLine3NW;
        } else {
            gc->procs.line.processLine = (__GLproc)__glProcessLine;
        }
        if ((modeFlags & SHADE_CHEAP_FOG) && !(modeFlags & SHADE_SMOOTH_LIGHT)) {
            gc->procs.renderLine2 = gc->procs.renderLine;
            gc->procs.renderLine = (__GLproc)__glRenderFlatFogLine;
        }
    }
}

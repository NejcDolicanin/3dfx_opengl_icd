/*
** gl_export.c -- original VA range 69078e60..6907c000: s_export.c (the core exports: notifyResize,
** notifySwapBuffers, destroyContext, shareContext; the proc table init; ApplyViewport /
** ApplyScissor; the empty procs), s_math.c matrix copy / identity / transpose, s_vertex.c clip
** checks and validateVertex procs, s_pick.c __glGenericPickPointProcs.  69078e80..69078f4a are
** jump thunks without a body here.
** Then the rest of s_export.c, in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "siprotos.h"
#include "glshade.h"
#include "icdprotos.h"
#include "glsitypes.h"
#include "si_names.h"
#include "icdglob.h"
#include <stddef.h>

/* ---- callees and the procs __glInitProcs stores (parameter sizes as the definitions: @N) ---- */
extern void __fastcall CheckViewport(__GLcontext *gc);

/* s_vertex.c: what a vertex has / wants (flagBits): glsi.h __GL_HAS_*, glshade.h __GL_SHADE_* */

typedef void (__fastcall *__GLfreeProc78e)(__GLcontext *gc, void *p);
typedef void (__fastcall *__GLxformProc78e)(__GLcoord *res, const __GLfloat *v, const __GLmatrixICD *m);
typedef void (__fastcall *__GLnormalizeProc78e)(__GLfloat *dst, const __GLfloat *src);

/* ---- forward declarations ---- */
GLboolean __fastcall ApplyViewport(__GLcontext *gc);
GLboolean __fastcall ApplyScissor(__GLcontext *gc);
void __fastcall export_Finish(void);
void __fastcall export_Flush(void);
void __fastcall __glNopWaitIdle(void);

/* 69078f90 -- null_DestroyContext: DestroyContext of the null context: free the gc */
GLboolean __fastcall null_DestroyContext(__GLcontext *gc)
{
    (*(__GLfreeProc78e)gc->imports[3])(gc, gc);
    return GL_TRUE;
}

/* 69079180 -- ChangeDrawableSize (3dfx S_EXPORT.C): the notifyResize export, resize the buffers,
** then the viewport and the reasonable-viewport flag.
*/
GLboolean __fastcall ChangeDrawableSize(__GLcontext *gc, GLint x, GLint y, GLint width, GLint height)
{
    if (__glResizeBuffers(gc, x, y, width, height) == GL_TRUE) {
        __glUpdateViewport(gc);
        CheckViewport(gc);
        return GL_TRUE;
    }
    return GL_FALSE;
}

/* 690792d0 -- export_SwapBuffers: SwapBuffers (S_EXPORT.C): the notifySwapBuffers export, procs.flush (a tail
** jump) */
void __fastcall export_SwapBuffers(__GLcontext *gc, int arg)
{
    (*gc->procs.flush)(gc, arg);
}

/* 690792e0 -- export_DestroyContext: DestroyContext (S_EXPORT.C): the destroyContext export */
GLboolean __fastcall export_DestroyContext(__GLcontext *gc)
{
    /* free ancillary buffer related data, then the rest of the software context */
    __glFreeBufData(gc);
    __glDestroyContext(gc);
    /* free memory for the host cpu hw context */
    (*(__GLfreeProc78e)gc->imports[3])(gc, gc);
    return GL_TRUE;
}

/* 69079500 -- InitFnPtrs (3dfx S_EXPORT.C): the generic procs of a new context.  The ICD has two
** pickAllProcs phases, waitIdle, storeTex / fetchTex, and no pickBufferProcs /
** pickCullVertexProcs / pickVertexArrayProcs / classifyMatrix here.
*/
void __fastcall InitFnPtrs(__GLcontext *gc)
{
    gc->procs.ec1 = (__GLproc)__glDoEvalCoord1;                     /* __glDoEvalCoord1 */
    gc->procs.ec2 = (__GLproc)__glDoEvalCoord2;                     /* __glDoEvalCoord2 */
    gc->procs.bitmap = (__GLbitmapProc)__glDrawBitmap;                  /* __glDrawBitmap */
    gc->procs.rect = (__GLproc)__glRect;                    /* __glRect */
    gc->procs.clipPolygon = (__GLproc)__glClipPolygon;             /* __glClipPolygon */
    gc->procs.validate = (__GLproc)__glGenericValidate;                /* __glGenericValidate */
    gc->procs.convertPolygonStipple = (__GLproc)__glConvertStipple;   /* __glConvertStipple */
    gc->procs.pushMatrix = (__GLproc)__glPushModelViewMatrix;              /* __glPushModelViewMatrix */
    gc->procs.popMatrix = (__GLproc)__glPopModelViewMatrix;               /* __glPopModelViewMatrix */
    gc->procs.loadIdentity = (__GLproc)__glLoadIdentityModelViewMatrix;            /* __glLoadIdentityModelViewMatrix */
    gc->procs.matrix[0] = (__GLproc)__glCopyMatrix;               /* __glCopyMatrix */
    gc->procs.matrix[1] = (__GLproc)__glInvertTransposeMatrix;               /* __glInvertTransposeMatrix */
    gc->procs.matrix[2] = (__GLproc)__glMakeIdentity;               /* __glMakeIdentity */
    gc->procs.matrix[3] = (__GLproc)__glMultMatrix;               /* __glMultMatrix */
    gc->procs.computeInverseTranspose = (__GLproc)__glComputeInverseTranspose; /* __glComputeInverseTranspose */
    gc->procs.normalize = (__GLproc)__glNormalize;               /* __glNormalize */
    gc->procs.beginPrim[GL_LINE_LOOP] = (__GLproc)__glBeginLLoop;     /* __glBeginLLoop */
    gc->procs.beginPrim[GL_LINE_STRIP] = (__GLproc)__glBeginLStrip;    /* __glBeginLStrip */
    gc->procs.beginPrim[GL_LINES] = (__GLproc)__glBeginLines;         /* __glBeginLines */
    gc->procs.beginPrim[GL_POINTS] = (__GLproc)__glBeginPoints;        /* __glBeginPoints */
    gc->procs.beginPrim[GL_POLYGON] = (__GLproc)__glBeginPolygon;       /* __glBeginPolygon */
    gc->procs.beginPrim[GL_TRIANGLE_STRIP] = (__GLproc)__glBeginTStrip; /* __glBeginTStrip */
    gc->procs.beginPrim[GL_TRIANGLE_FAN] = (__GLproc)__glBeginTFan;  /* __glBeginTFan */
    gc->procs.beginPrim[GL_TRIANGLES] = (__GLproc)__glBeginTriangles;     /* __glBeginTriangles */
    gc->procs.beginPrim[GL_QUAD_STRIP] = (__GLproc)__glBeginQStrip;    /* __glBeginQStrip */
    gc->procs.beginPrim[GL_QUADS] = (__GLproc)__glBeginQuads;         /* __glBeginQuads */
    gc->procs.endPrim = (__GLproc)__glEndPrim;                 /* __glEndPrim */
    gc->procs.vertex = (__GLproc)__glNop2;                  /* __glNop */
    gc->procs.rasterPos2 = (__GLproc)__glRasterPos2;              /* __glRasterPos2 */
    gc->procs.rasterPos3 = (__GLproc)__glRasterPos3;              /* __glRasterPos3 */
    gc->procs.rasterPos4 = (__GLproc)__glRasterPos4;              /* __glRasterPos4 */
    gc->procs.pickAllProcsPhase2 = (__GLproc)__glGenericPickAllProcsPhase2;
    gc->procs.pickBlendProcs = (__GLproc)__glGenericPickBlendProcs;          /* __glGenericPickBlendProcs */
    gc->procs.pickClipProcs = (__GLproc)__glGenericPickClipProcs;           /* __glGenericPickClipProcs */
    gc->procs.pickColorMaterialProcs = (__GLproc)__glGenericPickColorMaterialProcs;  /* __glGenericPickColorMaterialProcs */
    gc->procs.pickFogProcs = (__GLproc)__glGenericPickFogProcs;            /* __glGenericPickFogProcs */
    gc->procs.pickTransformProcs = (__GLproc)__glGenericPickTransformProcs;      /* __glGenericPickTransformProcs */
    gc->procs.pickLineProcs = (__GLproc)__glGenericPickLineProcs;           /* __glGenericPickLineProcs */
    gc->procs.pickMatrixProcs = (__GLproc)__glGenericPickMatrixProcs;         /* __glGenericPickMatrixProcs */
    gc->procs.pickInvTransposeProcs = (__GLproc)__glGenericPickInvTransposeProcs;   /* __glGenericPickInvTransposeProcs */
    gc->procs.pickMvpMatrixProcs = (__GLproc)__glGenericPickMvpMatrixProcs;      /* __glGenericPickMvpMatrixProcs */
    gc->procs.pickParameterClipProcs = (__GLproc)__glGenericPickParameterClipProcs;  /* __glGenericPickParameterClipProcs */
    gc->procs.pickPixelProcs = (__GLproc)__glGenericPickPixelProcs;          /* __glGenericPickPixelProcs */
    gc->procs.pickPointProcs = (__GLproc)__glGenericPickPointProcs;          /* __glGenericPickPointProcs */
    gc->procs.pickRenderBitmapProcs = (__GLproc)__glOptPickRenderBitmapProcs;   /* __glOptPickRenderBitmapProcs */
    gc->procs.pickSpanProcs = (__GLproc)__glGenericPickSpanProcs;           /* __glGenericPickSpanProcs */
    gc->procs.pickStoreProcs = (__GLproc)__glGenericPickStoreProcs;          /* __glGenericPickStoreProcs */
    gc->procs.pickTextureProcs = (__GLproc)__glGenericPickTextureProcs;        /* __glGenericPickTextureProcs */
    gc->procs.pickCalcTextureProcs = (__GLproc)__glGenericPickCalcTextureProcs;    /* __glGenericPickCalcTextureProcs */
    gc->procs.pickTriangleProcs = (__GLproc)__glGenericPickTriangleProcs;       /* __glGenericPickTriangleProcs */
    gc->procs.pickVertexProcs = (__GLproc)__glGenericPickVertexProcs;         /* __glGenericPickVertexProcs */
    gc->procs.pickAllProcsPhase1 = (__GLproc)__glGenericPickDepthProcs;
    gc->procs.copyImage = (__GLproc)__glGenericPickCopyImage;               /* __glGenericPickCopyImage */
    gc->procs.readImage = (__GLproc)__glGenericReadTexImage;               /* __glGenericPickReadImage */
    gc->procs.fetchTex = (__GLproc)__glGenericFetchTexImage;
    gc->procs.storeTex = (__GLproc)__glGenericStoreTexImage;
    gc->procs.pixel.spanReadCI = (__GLproc)__glSpanReadCI;        /* __glSpanReadCI */
    gc->procs.pixel.spanReadCI2 = (__GLproc)__glSpanReadCI2;       /* __glSpanReadCI2 */
    gc->procs.pixel.spanReadRGBA = (__GLproc)__glSpanReadRGBA;      /* __glSpanReadRGBA */
    gc->procs.pixel.spanReadRGBA2 = (__GLproc)__glSpanReadRGBA2;     /* __glSpanReadRGBA2 */
    gc->procs.pixel.spanReadDepth = (__GLproc)__glSpanReadDepth;     /* __glSpanReadDepth */
    gc->procs.pixel.spanReadDepth2 = (__GLproc)__glSpanReadDepth2;    /* __glSpanReadDepth2 */
    gc->procs.pixel.spanReadStencil = (__GLproc)__glSpanReadStencil;   /* __glSpanReadStencil */
    gc->procs.pixel.spanReadStencil2 = (__GLproc)__glSpanReadStencil2;  /* __glSpanReadStencil2 */
    gc->procs.pixel.spanRenderCI = (__GLproc)__glSpanRenderCI;      /* __glSpanRenderCI */
    gc->procs.pixel.spanRenderCI2 = (__GLproc)__glSpanRenderCI2;     /* __glSpanRenderCI2 */
    gc->procs.pixel.spanRenderRGBA = (__GLproc)__glSpanRenderRGBA;    /* __glSpanRenderRGBA */
    gc->procs.pixel.spanRenderRGBA2 = (__GLproc)__glSpanRenderRGBA2;   /* __glSpanRenderRGBA2 */
    gc->procs.pixel.spanRenderDepth = (__GLproc)__glSpanRenderDepth;   /* __glSpanRenderDepth */
    gc->procs.pixel.spanRenderDepth2 = (__GLproc)__glSpanRenderDepth2;  /* __glSpanRenderDepth2 */
    gc->procs.pixel.spanRenderStencil = (__GLproc)__glSpanRenderStencil; /* __glSpanRenderStencil */
    gc->procs.pixel.spanRenderStencil2 = (__GLproc)__glSpanRenderStencil2;/* __glSpanRenderStencil2 */
    gc->procs.applyScissor = (__GLproc)ApplyScissor;            /* ApplyScissor */
    gc->procs.applyViewport = (__GLproc)ApplyViewport;           /* ApplyViewport */
    gc->procs.computeClipBox = (__GLproc)__glComputeClipBox;          /* __glComputeClipBox */
    gc->procs.finish = (__GLproc)export_Finish;                  /* Finish */
    gc->procs.flush = (__GLproc)export_Flush;                   /* Flush */
    gc->procs.waitIdle = (__GLproc)__glNopWaitIdle;
    gc->procs.colorTable = (__GLproc)__glColorTableEXT;              /* __glColorTableEXT */
    gc->procs.colorSubTable = (__GLproc)__glColorSubTableEXT;           /* __glColorSubTableEXT */
}

/* 69079830 -- ApplyViewport (S_EXPORT.C): only when the window size could be found */
GLboolean __fastcall ApplyViewport(__GLcontext *gc)
{
    if (__glFindWindowSize(gc) == GL_TRUE) {
        __glUpdateViewport(gc);
        CheckViewport(gc);
        return GL_TRUE;
    }
    return GL_FALSE;
}

/* 69079860 -- ApplyScissor (S_EXPORT.C): CheckViewport if __glFindWindowSize finds the size (then GL_TRUE) */
GLboolean __fastcall ApplyScissor(__GLcontext *gc)
{
    if (__glFindWindowSize(gc) == GL_TRUE) {
        CheckViewport(gc);
        return GL_TRUE;
    }
    return GL_FALSE;
}

/* 69079880 -- export_Finish: Finish (S_EXPORT.C), nothing to do */
void __fastcall export_Finish()
{
}

/* 69079890 -- export_Flush: Flush (S_EXPORT.C), nothing to do */
void __fastcall export_Flush()
{
}

/* 690798a0 -- __glNopWaitIdle: procs.waitIdle, nothing to do (the ancestor's sstcontext.c installs __glNop) */
void __fastcall __glNopWaitIdle()
{
}

/* 690799e0 -- ReleaseSLock (3dfx sstcontext.c): buffers.releaseSLock, drop one share-lock level, TRUE while
** still locked */
GLboolean __fastcall ReleaseSLock(__GLcontext *gc)
{
    return --gc->buffers.sLockCnt != 0;
}

/* 69079a00 -- __glNopLockDevice: buffers.lockDevice, nothing to do */
void __fastcall __glNopLockDevice()
{
}

/* 69079a10 -- __glNopUnlockDevice: buffers.unlockDevice, nothing to do */
void __fastcall __glNopUnlockDevice()
{
}

/* 69079a20 -- ShareContext (S_EXPORT.C): the shareContext export (S_EXPORT.C ShareContext): display lists,
** texture objects
** and (ICD) the texture manager state.
*/
GLboolean __fastcall ShareContext(__GLcontext *gc, __GLcontext *gcShare)
{
    __glShareDlist(gc, gcShare);
    __glShareTextureObjects(gc, gcShare);
    __glShareTextureManager(gc, gcShare);
    return GL_TRUE;
}

/* 69079af0 -- __glCopyMatrix (s_math.c) */
void __fastcall __glCopyMatrix(__GLmatrixICD *dst, const __GLmatrixICD *src)
{
    dst->matrixType = src->matrixType;
    dst->matrix[0][0] = src->matrix[0][0];
    dst->matrix[0][1] = src->matrix[0][1];
    dst->matrix[0][2] = src->matrix[0][2];
    dst->matrix[0][3] = src->matrix[0][3];

    dst->matrix[1][0] = src->matrix[1][0];
    dst->matrix[1][1] = src->matrix[1][1];
    dst->matrix[1][2] = src->matrix[1][2];
    dst->matrix[1][3] = src->matrix[1][3];

    dst->matrix[2][0] = src->matrix[2][0];
    dst->matrix[2][1] = src->matrix[2][1];
    dst->matrix[2][2] = src->matrix[2][2];
    dst->matrix[2][3] = src->matrix[2][3];

    dst->matrix[3][0] = src->matrix[3][0];
    dst->matrix[3][1] = src->matrix[3][1];
    dst->matrix[3][2] = src->matrix[3][2];
    dst->matrix[3][3] = src->matrix[3][3];
}

/* 69079b60 -- __glMakeIdentity (s_math.c) */
void __fastcall __glMakeIdentity(__GLmatrixICD *m)
{
    __GLfloat zer = __GL_ZERO_F;
    __GLfloat one = ((__GLfloat) 1.0);

    m->matrix[0][0] = one; m->matrix[0][1] = zer;
        m->matrix[0][2] = zer; m->matrix[0][3] = zer;
    m->matrix[1][0] = zer; m->matrix[1][1] = one;
        m->matrix[1][2] = zer; m->matrix[1][3] = zer;
    m->matrix[2][0] = zer; m->matrix[2][1] = zer;
        m->matrix[2][2] = one; m->matrix[2][3] = zer;
    m->matrix[3][0] = zer; m->matrix[3][1] = zer;
        m->matrix[3][2] = zer; m->matrix[3][3] = one;
    m->matrixType = __GL_MT_IDENTITY;
}

/* 6907a570 -- __glTransposeMatrix (s_math.c) */
void __fastcall __glTransposeMatrix(__GLmatrixICD *transpose, const __GLmatrixICD *src)
{
    transpose->matrix[0][0] = src->matrix[0][0];
    transpose->matrix[0][1] = src->matrix[1][0];
    transpose->matrix[0][2] = src->matrix[2][0];
    transpose->matrix[0][3] = src->matrix[3][0];
    transpose->matrix[1][0] = src->matrix[0][1];
    transpose->matrix[1][1] = src->matrix[1][1];
    transpose->matrix[1][2] = src->matrix[2][1];
    transpose->matrix[1][3] = src->matrix[3][1];
    transpose->matrix[2][0] = src->matrix[0][2];
    transpose->matrix[2][1] = src->matrix[1][2];
    transpose->matrix[2][2] = src->matrix[2][2];
    transpose->matrix[2][3] = src->matrix[3][2];
    transpose->matrix[3][0] = src->matrix[0][3];
    transpose->matrix[3][1] = src->matrix[1][3];
    transpose->matrix[3][2] = src->matrix[2][3];
    transpose->matrix[3][3] = src->matrix[3][3];
}

/* 6907a7b0 -- __glClipCheckAll2 (s_vertex.c): z = 0, w = 1, mark the vertex 2-D (the ICD clears only
** the other size bit) and tail-jump to __glClipCheckAll.
*/
GLuint __fastcall __glClipCheckAll2(__GLcontext *gc, __GLvertexICD *vx)
{
    vx->obj.z = __GL_ZERO_F;
    vx->obj.w = __GL_ONE_F;
    vx->flagBits = (vx->flagBits & ~__GL_VERTEX_3D_SIZE) | __GL_VERTEX_2D_SIZE;
    return __glClipCheckAll(gc, vx);
}

/* 6907a7e0 -- __glClipCheckAll3 (s_vertex.c) */
GLuint __fastcall __glClipCheckAll3(__GLcontext *gc, __GLvertexICD *vx)
{
    vx->obj.w = __GL_ONE_F;
    vx->flagBits = (vx->flagBits & ~__GL_VERTEX_2D_SIZE) | __GL_VERTEX_3D_SIZE;
    return __glClipCheckAll(gc, vx);
}

/* 6907a800 -- __glClipCheckAll4 (s_vertex.c) */
GLuint __fastcall __glClipCheckAll4(__GLcontext *gc, __GLvertexICD *vx)
{
    vx->flagBits |= __GL_VERTEX_4D_SIZE;
    return __glClipCheckAll(gc, vx);
}

/* 6907b6a0 -- __glValidateVertex2 (s_vertex.c).  ICD: no GL_RESCALE_NORMAL branch, and the packed
** colour (__GL_HAS_PACKED_COLOR) is one more thing a vertex can want.
*/
void __fastcall __glValidateVertex2(__GLcontext *gc, __GLvertexICD *vx, GLuint needs)
{
    GLuint has = vx->flagBits;
    GLuint wants = needs & ~has;

    if (wants & __GL_HAS_EYE) {
        __GLtransformICD *tr = gc->transform.modelView;
        (*(__GLxformProc78e)tr->matrix.xf2)(&vx->eye, &vx->obj.x, &tr->matrix);
    }
    if (wants & __GL_HAS_NORMAL) {
        __GLcoord ne;
        __GLtransformICD *tr = gc->transform.modelView;

        if (tr->inverseTranspose.matrixType == __GL_MT_GENERAL) {
            /* this is needed only if there is projection info */
            vx->normal.w = -(vx->normal.x * vx->obj.x + vx->normal.y * vx->obj.y);
        }
        if (gc->state.enables.general & __GL_NORMALIZE_ENABLE) {
            (*(__GLxformProc78e)tr->inverseTranspose.xf3)(&ne, &vx->normal.x, &tr->inverseTranspose);
            (*(__GLnormalizeProc78e)gc->procs.normalize)(&vx->normal.x, &ne.x);
        } else {
            (*(__GLxformProc78e)tr->inverseTranspose.xf3)(&vx->normal, &vx->normal.x, &tr->inverseTranspose);
        }
    }
    if (wants & __GL_HAS_FOG) {
        vx->fog = (*gc->procs.fogVertex)(gc, vx);
    }
    if (wants & __GL_HAS_TEXTURE) {
        vx->obj.z = __GL_ZERO_F;
        vx->obj.w = __GL_ONE_F;
        (*gc->procs.calcTexture)(gc, vx);
    }
    if (wants & __GL_HAS_FRONT_COLOR) {
        (*gc->procs.calcColor)(gc, __GL_FRONTFACE, vx);
    }
    if (wants & __GL_HAS_BACK_COLOR) {
        (*gc->procs.calcColor)(gc, __GL_BACKFACE, vx);
    }
    if (wants & __GL_HAS_PACKED_COLOR) {
        (*gc->procs.packVertexColor)(gc, vx);
    }

    vx->flagBits = has | wants;
}

/* 6907b7d0 -- __glValidateVertex3 (s_vertex.c) */
void __fastcall __glValidateVertex3(__GLcontext *gc, __GLvertexICD *vx, GLuint needs)
{
    GLuint has = vx->flagBits;
    GLuint wants = needs & ~has;

    if (wants & __GL_HAS_EYE) {
        __GLtransformICD *tr = gc->transform.modelView;
        (*(__GLxformProc78e)tr->matrix.xf3)(&vx->eye, &vx->obj.x, &tr->matrix);
    }
    if (wants & __GL_HAS_NORMAL) {
        __GLcoord ne;
        __GLtransformICD *tr = gc->transform.modelView;

        if (tr->inverseTranspose.matrixType == __GL_MT_GENERAL) {
            /* this is needed only if there is projection info */
            vx->normal.w = -(vx->normal.x * vx->obj.x + vx->normal.y * vx->obj.y
                             + vx->normal.z * vx->obj.z);
        }
        if (gc->state.enables.general & __GL_NORMALIZE_ENABLE) {
            (*(__GLxformProc78e)tr->inverseTranspose.xf3)(&ne, &vx->normal.x, &tr->inverseTranspose);
            (*(__GLnormalizeProc78e)gc->procs.normalize)(&vx->normal.x, &ne.x);
        } else {
            (*(__GLxformProc78e)tr->inverseTranspose.xf3)(&vx->normal, &vx->normal.x, &tr->inverseTranspose);
        }
    }
    if (wants & __GL_HAS_FOG) {
        vx->fog = (*gc->procs.fogVertex)(gc, vx);
    }
    if (wants & __GL_HAS_TEXTURE) {
        vx->obj.w = __GL_ONE_F;
        (*gc->procs.calcTexture)(gc, vx);
    }
    if (wants & __GL_HAS_FRONT_COLOR) {
        (*gc->procs.calcColor)(gc, __GL_FRONTFACE, vx);
    }
    if (wants & __GL_HAS_BACK_COLOR) {
        (*gc->procs.calcColor)(gc, __GL_BACKFACE, vx);
    }
    if (wants & __GL_HAS_PACKED_COLOR) {
        (*gc->procs.packVertexColor)(gc, vx);
    }

    vx->flagBits = has | wants;
}

/* 6907b900 -- __glValidateVertex4 (s_vertex.c) */
void __fastcall __glValidateVertex4(__GLcontext *gc, __GLvertexICD *vx, GLuint needs)
{
    GLuint has = vx->flagBits;
    GLuint wants = needs & ~has;

    if (wants & __GL_HAS_EYE) {
        __GLtransformICD *tr = gc->transform.modelView;
        (*(__GLxformProc78e)tr->matrix.xf4)(&vx->eye, &vx->obj.x, &tr->matrix);
    }
    if (wants & __GL_HAS_NORMAL) {
        __GLcoord ne;
        __GLtransformICD *tr = gc->transform.modelView;

        if (tr->inverseTranspose.matrixType == __GL_MT_GENERAL && vx->obj.w) {
            /* this is needed only if there is projection info */
            vx->normal.w = -(vx->normal.x * vx->obj.x +
                             vx->normal.y * vx->obj.y +
                             vx->normal.z * vx->obj.z) / vx->obj.w;
        } else {
            vx->normal.w = __GL_ZERO_F;
        }
        if (gc->state.enables.general & __GL_NORMALIZE_ENABLE) {
            (*(__GLxformProc78e)tr->inverseTranspose.xf3)(&ne, &vx->normal.x, &tr->inverseTranspose);
            (*(__GLnormalizeProc78e)gc->procs.normalize)(&vx->normal.x, &ne.x);
        } else {
            (*(__GLxformProc78e)tr->inverseTranspose.xf3)(&vx->normal, &vx->normal.x, &tr->inverseTranspose);
        }
    }
    if (wants & __GL_HAS_FOG) {
        vx->fog = (*gc->procs.fogVertex)(gc, vx);
    }
    if (wants & __GL_HAS_TEXTURE) {
        (*gc->procs.calcTexture)(gc, vx);
    }
    if (wants & __GL_HAS_FRONT_COLOR) {
        (*gc->procs.calcColor)(gc, __GL_FRONTFACE, vx);
    }
    if (wants & __GL_HAS_BACK_COLOR) {
        (*gc->procs.calcColor)(gc, __GL_BACKFACE, vx);
    }
    if (wants & __GL_HAS_PACKED_COLOR) {
        (*gc->procs.packVertexColor)(gc, vx);
    }

    vx->flagBits = has | wants;
}

/* 6907bfa0 -- __glGenericPickPointProcs (s_pick.c: the lock versions of the vertex procs; the
** renderers as the 3dfx ancestor's S_PICK.C).  EDX passes through to getModeFlags.
*/
void __fastcall __glGenericPickPointProcs(__GLcontext *gc, int unused)
{
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);

    if ((*gc->vertex.getFaceNeeds0)(gc) == 0) {
        gc->procs.vertexPoints = (__GLproc)__glPointFastLock;       /* __glPointFastLock */
    } else {
        gc->procs.vertexPoints = (__GLproc)__glPointVertexLock;       /* __glPointLock */
    }

    gc->procs.renderTrianglePolygonModePoint = (__GLproc)__glRenderTrianglePoint;    /* __glRenderTrianglePoint */

    if (gc->renderMode == GL_FEEDBACK) {
        gc->procs.renderPoint = (__GLproc)__glFeedbackPoint;        /* __glFeedbackPoint */
        return;
    }
    if (gc->renderMode == GL_SELECT) {
        gc->procs.renderPoint = (__GLproc)__glSelectPoint;        /* __glSelectPoint */
        return;
    }
    if (gc->state.enables.general & __GL_POINT_SMOOTH_ENABLE) {
        if (gc->modes.colorIndexMode) {
            gc->procs.renderPoint = (__GLproc)__glRenderAntiAliasedCIPoint;    /* __glRenderAntiAliasedCIPoint */
        } else {
            gc->procs.renderPoint = (__GLproc)__glRenderAntiAliasedRGBPoint;    /* __glRenderAntiAliasedRGBPoint */
        }
    } else if (gc->state.point.aliasedSize != 1) {
        gc->procs.renderPoint = (__GLproc)__glRenderAliasedPointN;        /* __glRenderAliasedPointN */
    } else if (modeFlags & __GL_SHADE_TEXTURE) {
        gc->procs.renderPoint = (__GLproc)__glRenderAliasedPoint1;        /* __glRenderAliasedPoint1 */
    } else {
        gc->procs.renderPoint = (__GLproc)__glRenderAliasedPoint1_NoTex;        /* __glRenderAliasedPoint1_NoTex */
    }

    if (((modeFlags & __GL_SHADE_CHEAP_FOG) &&
            !(modeFlags & __GL_SHADE_SMOOTH_LIGHT)) ||
            (modeFlags & __GL_SHADE_SLOW_FOG)) {
        gc->procs.renderPoint2 = gc->procs.renderPoint;
        gc->procs.renderPoint = (__GLproc)__glRenderFlatFogPoint;        /* __glRenderFlatFogPoint */
    }
}

#define __GL_DISPATCH_SIZE      (0x19a * 4)     /* sizeof(__GLdispatchState) */
/* imports[7]: the drawable's origin and size */
typedef void (__fastcall *__GLgetDrawableSize)(__GLcontext *gc, GLint *x, GLint *y, GLint *width, GLint *height);
/* ---- s_buffers.c-like: first-time buffer setup at make current ------------------------------ */
extern void __fastcall __glInitBuffers(__GLcontext *gc);
extern void __fastcall InitFnPtrs(__GLcontext *gc);
extern GLboolean __fastcall ReleaseSLock(__GLcontext *gc);
extern void __fastcall __glNopLockDevice();
extern void __fastcall __glNopUnlockDevice();
extern GLboolean __fastcall ChangeDrawableSize(__GLcontext *gc, GLint x, GLint y, GLint width, GLint height);

/* 690791c0 -- CheckViewport (3dfx S_EXPORT.C): recompute transform.reasonableViewport (the viewport lies
** inside the window clip box);
** a change revalidates, at once when inside glBegin */
void __fastcall CheckViewport(__GLcontext *gc)
{
    GLint xlow, ylow, xhigh, yhigh;
    GLboolean oldReasonable;

    xlow = gc->constants.viewportXAdjust + gc->state.viewport.x;
    ylow = gc->constants.viewportYAdjust + gc->state.viewport.y;
    oldReasonable = gc->transform.reasonableViewport;
    xhigh = gc->state.viewport.width + xlow;
    yhigh = gc->state.viewport.height + ylow;

    if (xlow >= gc->transform.clipX0 && ylow >= gc->transform.clipY0 &&
            xhigh <= gc->transform.clipX1 && yhigh <= gc->transform.clipY1) {
        gc->transform.reasonableViewport = GL_TRUE;
    } else {
        gc->transform.reasonableViewport = GL_FALSE;
    }
    if (oldReasonable != gc->transform.reasonableViewport) {
        if (__gl_beginMode == __GL_IN_BEGIN) {
            __GL_DELAY_VALIDATE(gc);
            (*gc->procs.validate)(gc);
            __gl_beginMode = __GL_IN_BEGIN;
        } else {
            __GL_DELAY_VALIDATE(gc);
        }
    }
}

/* 69079370 -- export_MakeCurrent: MakeCurrent (3dfx S_EXPORT.C): install the dispatch tables, first-time init of the
** buffers, procs and state, viewport and scissor from the drawable size, validate */
GLboolean __fastcall export_MakeCurrent(__GLcontext *gc)
{
    __glSetCurrentGC(gc);
    __gl_beginMode = gc->beginMode;

    /* Initialize dispatch tables */
    gc->dispatchState = &gc->currentDispatchState;
    memcpy(&gc->currentDispatchState, &__glImmedState, __GL_DISPATCH_SIZE);
    memcpy(&gc->listCompState, &__glListCompState, __GL_DISPATCH_SIZE);

    /* Dirty the depth mask, since we may need to update depth pointers */
    __gl_beginMode = __GL_NEED_VALIDATE;
    gc->dirtyMask |= __GL_DIRTY_DEPTH;

    if (!(gc->mcFlags & __GL_HW_MODE)) {
	__glInitBuffers(gc);
    }
    __glMakeCurrentBuffers(gc);
    if (!(gc->mcFlags & __GL_HW_MODE)) {
	InitFnPtrs(gc);
    }
    __glUpdateDepthRange(gc);

    /* Reset the context if this is the first MakeCurrent */
    if (!(gc->mcFlags & (__GL_HW_MODE | __GL_PIXMAP_MODE))) {
	GLint x, y, width, height;

	__glSoftResetContext(gc);
	(*(__GLgetDrawableSize)gc->imports[7])(gc, &x, &y, &width, &height);
	glViewport(0, 0, width, height);
	glScissor(0, 0, width, height);
    } else {
	(*gc->procs.applyViewport)(gc);
	(*gc->procs.applyScissor)(gc);
    }

    /* Reset the HW context if this is the first device dependent MakeCurrent */
    if (!(gc->mcFlags & __GL_INIT_HW)) {
	__glContextSetColorScales(gc);
	(*gc->procs.validate)(gc);
    }

    if (gc->dlist.currentList) {
	gc->dispatchState = &gc->savedDispatchState;
	__glCopyDispatchTable(&gc->listCompState, &__gl_context->currentDispatchState);
    } else {
	gc->dispatchState = &__gl_context->currentDispatchState;
    }
    gc->mcFlags = (gc->mcFlags & ~__GL_PIXMAP_MODE) | __GL_INIT_HW | __GL_HW_MODE;
    return GL_TRUE;
}

extern GLuint __fastcall ObtainSLock(__GLcontext *gc);
/* 690798b0 -- __glInitBuffers: the draw buffers, accumulation, depth and stencil buffers, the
** share lock procs */
void __fastcall __glInitBuffers(__GLcontext *gc)
{
    __glCurrentGC()->beginMode = 2;
    gc->dirtyMask = ~0;
    gc->front = &gc->frontBuffer;
    if (gc->modes.doubleBufferMode) {
	gc->back = &gc->backBuffer;
	if (gc->modes.colorIndexMode) {
	    __glInitCI(&gc->frontBuffer, gc);
	    __glInitCI(gc->back, gc);
	} else {
	    __glInitRGB(&gc->frontBuffer, gc);
	    __glInitRGB(gc->back, gc);
	}
    } else {
	if (gc->modes.colorIndexMode) {
	    __glInitCI(&gc->frontBuffer, gc);
	} else {
	    __glInitRGB(&gc->frontBuffer, gc);
	}
    }
    if (gc->modes.haveAccumBuffer) {
	__glInitAccum64(&gc->accumBuffer, gc);
    }
    if (gc->modes.haveDepthBuffer) {
	__glInitDepth(&gc->depthBuffer, gc);
    } else {
	gc->depthBuffer.scale = 0x7fffff80;
    }
    if (gc->modes.haveStencilBuffer) {
	((void (__fastcall *)(__GLstencilBufferICD *, __GLcontext *))__glInitStencil8)(&gc->stencilBuffer, gc);
    }
    gc->buffers.obtainSLock = (__GLobtainSLockProc)ObtainSLock;
    gc->buffers.releaseSLock = (__GLproc)ReleaseSLock;
    gc->buffers.lockDevice = (__GLproc)__glNopLockDevice;
    gc->buffers.unlockDevice = (__GLproc)__glNopUnlockDevice;
}

/* ---- 3dfx S_EXPORT.C: share lock, lose / make current (the glcore side of wglMakeCurrent) ---- */
/* 690799c0 -- ObtainSLock (3dfx S_EXPORT.C): bump the share lock count; true (no need to lock the
** buffers) when it was already held */
GLuint __fastcall ObtainSLock(__GLcontext *gc)
{
    if (gc->buffers.sLockCnt++) {
	return 1;
    }
    return 0;
}

/* 69079a80 -- __glCopyDispatchTable: copy a dispatch table; a copy into the current context's dispatch state
** goes on to
** wglRcCallback (called with the source table in EDX as a second fastcall argument) */
void __fastcall __glCopyDispatchTable(const __GLdispatchTable *src, __GLdispatchTable *dst)
{
    if (dst != src && src && dst) {
	*dst = *src;
    }
    if (dst == &__glCurrentGC()->currentDispatchState) {
	((void (__fastcall *)(__GLcontext *, const __GLdispatchTable *))wglRcCallback)
	    (__glCurrentGC(), src);
    }
}

/* 690792c0 -- __glNotifyDestroy: exports[6] (SI notifyDestroy): ChangeDrawableSize (69079180) to an empty
** rectangle; the gc passes straight through in ECX */
void __fastcall __glNotifyDestroy(__GLcontext *gc) { ChangeDrawableSize(gc, 0, 0, 0, 0); }

typedef void *(__fastcall *__GLfn_calloc)(__GLcontext *gc, size_t n, size_t size);
typedef struct { GLuint proc[9]; } __GLimportProcs;      /* gc->imports: 9 procs, copied whole */
extern int __fastcall __glGenericDlistOptimizer(void);
extern void __fastcall __glNotifyDestroy(__GLcontext *gc);
extern void __fastcall export_SwapBuffers(__GLcontext *gc, int arg);
extern GLboolean __fastcall export_DestroyContext(__GLcontext *gc);
extern GLboolean __fastcall export_MakeCurrent(__GLcontext *gc);
extern GLboolean __fastcall ShareContext(__GLcontext *gc, __GLcontext *gcShare);

/* 69078fa0 -- __glCoreCreateContext (s_export.c, 3dfx SRC/GLCORE flavour: device colour / depth scale
** constants).  No callers in the original (dead).  Allocates only the 0x35150-byte core part of the
** context (as the original does); the imports (9 procs) and modes are copied whole.  Compiled with
** the normal flags (the struct copies are the two rep movsd; no /Ow needed).
*/
__GLcontext *__fastcall __glCoreCreateContext(__GLimportProcs *imports, __GLcontextModesICD *modes)
{
    __GLcontext *gc;

    gc = (__GLcontext *) (*(__GLfn_calloc) imports->proc[1])(0, 1, GLCTX_FO(__GLcontext, hwcx));
    if (!gc) {
        return 0;
    }
    *(__GLimportProcs *) gc->imports = *imports;
    gc->modes = *modes;

    gc->constants.maxViewportWidth = 2048;           /* __GL_DEFAULT_MAX_WINDOW_WIDTH */
    gc->constants.maxViewportHeight = 2048;
    gc->constants.viewportXAdjust = 0x1800;          /* __GL_DEFAULT_VERTEX_X_BIAS + _X_FIX */
    gc->constants.viewportYAdjust = 0x1800;
    gc->constants.subpixelBits = 3;

    gc->constants.redScale = 255.0F;
    gc->constants.greenScale = 255.0F;
    gc->constants.blueScale = 255.0F;
    gc->constants.alphaScale = 255.0F;

    gc->constants.depthScale = 1.0F;

    gc->constants.numberOfLights = 8;
    gc->constants.numberOfClipPlanes = 6;
    gc->constants.numberOfTextures = 1;
    gc->constants.numberOfTextureEnvs = 1;
    gc->constants.maxTextureSize = 11;
    gc->constants.maxMipMapLevel = 11;
    gc->constants.maxListNesting = 64;
    gc->constants.maxEvalOrder = 30;
    gc->constants.maxPixelMapTable = 65536;
    gc->constants.maxAttribStackDepth = 16;
    gc->constants.maxClientAttribStackDepth = 16;
    gc->constants.maxNameStackDepth = 128;
    gc->constants.maxModelViewStackDepth = 32;
    gc->constants.maxProjectionStackDepth = 2;
    gc->constants.maxTextureStackDepth = 10;

    gc->constants.pointSizeMinimum = 0.5F;
    gc->constants.pointSizeMaximum = 10.0F;
    gc->constants.pointSizeGranularity = 0.125F;
    gc->constants.lineWidthMinimum = 0.5F;
    gc->constants.lineWidthMaximum = 10.0F;
    gc->constants.lineWidthGranularity = 0.125F;

    gc->dlist.optimizer = (void *) __glGenericDlistOptimizer;
    gc->dlist.compiler = (void *) __glGenericDlistCompiler;
    gc->dlist.listExec = __gl_GenericDlOps;
    gc->dlist.baseListExec = __glListExecTable;
    gc->dlist.machineListExec = NULL;
    gc->dlist.checkOp = (void *) __glNop2;
    gc->dlist.initState = (void *) __glNop1;

    gc->exports[__GL_EXP_DESTROY_CONTEXT] = (GLuint) export_DestroyContext;
    gc->exports[__GL_EXP_LOSE_CURRENT] = (GLuint) LoseCurrent;
    gc->exports[__GL_EXP_MAKE_CURRENT] = (GLuint) export_MakeCurrent;
    gc->exports[__GL_EXP_SHARE_CONTEXT] = (GLuint) ShareContext;
    gc->exports[__GL_EXP_NOTIFY_RESIZE] = (GLuint) ChangeDrawableSize;
    gc->exports[__GL_EXP_NOTIFY_DESTROY] = (GLuint) __glNotifyDestroy;
    gc->exports[__GL_EXP_SWAP_BUFFERS] = (GLuint) export_SwapBuffers;

    __glEarlyInitContext(gc);

    return gc;
}

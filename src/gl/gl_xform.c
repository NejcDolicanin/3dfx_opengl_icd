/*
** gl_xform.c -- original VA range 69080000..69084000: s_xform.c (sequence numbers, viewport, the
** modelview / projection / texture stack procs, scale / translate, the XForm helpers, the mvp
** pick), s_ctable.c (CheckColorSubTableArgs, InitColorTableStore, GetColorTableParameter) and two
** empty procs.  The procs.matrix[] slots are the SI's __GLmatrixProcs: copy, invertTranspose,
** makeIdentity, mult.
** Then the rest of s_xform.c, in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "icdprotos.h"
#include <stddef.h>
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "icdglob.h"
extern void __fastcall __glDoRotate(__GLcontext *gc, GLuint unused, __GLfloat angle, __GLfloat ax, __GLfloat ay,
                                    __GLfloat az);                         /* __glDoRotate */
extern void __fastcall __glDoScale(__GLcontext *gc, GLuint edx_unused, __GLfloat x, __GLfloat y, __GLfloat z);
extern void __fastcall __glDoTranslate(__GLcontext *gc, GLuint edx_unused, __GLfloat x, __GLfloat y, __GLfloat z);
/* called with the floats only: nothing goes in EDX */
typedef void (__fastcall *__GLdoRotateProc800)(__GLcontext *gc, __GLfloat angle, __GLfloat x, __GLfloat y, __GLfloat z);
typedef void (__fastcall *__GLdoXformProc800)(__GLcontext *gc, __GLfloat x, __GLfloat y, __GLfloat z);

#define __GL_MAX_ORDER_IS2DNR   __GL_MT_IS2DNR

/* the SI's __GLmatrixProcs (procs.matrix[0..3]): glctx.h __GL_MATRIX_*(gc) */

/* s_xform.c scale / translate argument records */
struct __glScaleRec { __GLfloat x, y, z; };
struct __glTranslationRec { __GLfloat x, y, z; };

/* EXT_paletted_texture formats the kit's GL.H lacks */
#ifndef GL_BGR_EXT
#define GL_BGR_EXT      0x80e0
#endif
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT     0x80e1
#endif

/* ---- forward declarations ---- */
void __fastcall __glInvalidateSequenceNumbers(__GLcontext *gc);
void __fastcall __glGetColorTableParameterv(__GLcontext *gc, GLenum target, GLenum pname, GLint *ip, GLfloat *fp);

/* 69080340 -- __glInvalidateSequenceNumbers (s_xform.c): the modelview entries to sequence 0, the
** projection entries to 1, 2, ...
*/
void __fastcall __glInvalidateSequenceNumbers(__GLcontext *gc)
{
    __GLtransformICD *tr, *lasttr;
    GLuint s;

    /* make all mvp matrices refer to sequence number zero */
    s = 0;
    tr = &gc->transform.modelViewStack[0];
    lasttr = tr + gc->constants.maxModelViewStackDepth;
    while (tr < lasttr) {
        tr->sequence = s;
        tr++;
    }

    /* make all projection matrices sequence up starting at one */
    s = 1;
    tr = &gc->transform.projectionStack[0];
    lasttr = tr + gc->constants.maxProjectionStackDepth;
    while (tr < lasttr) {
        tr->sequence = s++;
        tr++;
    }
    gc->transform.projectionSequence = s;
}

/* 69080760 -- __glim_Rotatef (s_xform.c) */
void __stdcall __glim_Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    __GL_SETUP_NOT_IN_BEGIN();

    (*(__GLdoRotateProc800)__glDoRotate)(gc, angle, x, y, z);     /* __glDoRotate */
}

/* 69080820 -- __glim_Scalef (s_xform.c) */
void __stdcall __glim_Scalef(GLfloat x, GLfloat y, GLfloat z)
{
    __GL_SETUP_NOT_IN_BEGIN();

    (*(__GLdoXformProc800)__glDoScale)(gc, x, y, z);              /* __glDoScale */
}

/* 690808d0 -- __glim_Translatef (s_xform.c) */
void __stdcall __glim_Translatef(GLfloat x, GLfloat y, GLfloat z)
{
    __GL_SETUP_NOT_IN_BEGIN();

    (*(__GLdoXformProc800)__glDoTranslate)(gc, x, y, z);              /* __glDoTranslate */
}

/* 69080cc0 -- __glUpdateViewport (s_xform.c): viewport scale and centre (no half-pixel offset).
** The window height is converted as unsigned.
*/
void __fastcall __glUpdateViewport(__GLcontext *gc)
{
    __GLfloat ww, hh;

    /* compute operational viewport values */
    ww = gc->state.viewport.width * __glHalf;
    hh = gc->state.viewport.height * __glHalf;

    gc->state.viewport.scale.x = ww;
    gc->state.viewport.center.x = gc->state.viewport.x + gc->constants.fviewportXAdjust + ww;

    if (gc->constants.yInverted) {
        gc->state.viewport.scale.y = -hh;
        gc->state.viewport.center.y =
            ((GLuint)gc->constants.height - gc->constants.viewportEpsilon) -
            (gc->state.viewport.y + hh) + gc->constants.fviewportYAdjust;
    } else {
        gc->state.viewport.scale.y = hh;
        gc->state.viewport.center.y = gc->state.viewport.y + gc->constants.fviewportYAdjust + hh;
    }

    __glUpdateViewportTransform(gc);
}

/* 69080e40 -- __glReturnWindowY (s_xform.c): no half-pixel offset; the window height converts as
** unsigned.  EDX is a dummy (the y argument is on the stack).
*/
double __fastcall __glReturnWindowY(__GLcontext *gc, __GLfloat y)
{
    if (gc->constants.yInverted) {
        return (((GLuint)gc->constants.height - gc->constants.viewportEpsilon) -
                (y - gc->constants.fviewportYAdjust));
    } else {
        return (y - gc->constants.fviewportYAdjust);
    }
}

/* 69081240 -- __glPushModelViewMatrix (s_xform.c) */
void __fastcall __glPushModelViewMatrix(__GLcontext *gc)
{
    __GLtransformICD **trp, *tr, *stack;
    GLint num;

    num = gc->constants.maxModelViewStackDepth;
    trp = &gc->transform.modelView;
    stack = gc->transform.modelViewStack;
    tr = *trp;
    if (tr < &stack[num-1]) {
        tr[1] = tr[0];
        *trp = tr + 1;
    } else {
        __glSetError(GL_STACK_OVERFLOW);
    }
}

/* 69081320 -- __glLoadIdentityModelViewMatrix (s_xform.c).  ICD: also classifyMatrix on the
** matrix.
*/
void __fastcall __glLoadIdentityModelViewMatrix(__GLcontext *gc)
{
    __GLtransformICD *mvtr, *ptr;

    mvtr = gc->transform.modelView;
    (*__GL_MATRIX_MAKE_IDENTITY(gc))(&mvtr->matrix);
    (*__GL_MATRIX_MAKE_IDENTITY(gc))(&mvtr->inverseTranspose);
    (*gc->procs.pickMatrixProcs)(gc, &mvtr->matrix);
    (*gc->procs.pickInvTransposeProcs)(gc, &mvtr->inverseTranspose);
    mvtr->updateInverse = GL_FALSE;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_MODELVIEW);
    (*gc->procs.classifyMatrix)(gc, &mvtr->matrix);

    /* update mvp matrix */
    ptr = gc->transform.projection;
    mvtr->sequence = ptr->sequence;
    (*__GL_MATRIX_MULT(gc))(&mvtr->mvp, &mvtr->matrix, &ptr->matrix);
    (*gc->procs.pickMvpMatrixProcs)(gc, &mvtr->mvp);
}

/* 690813c0 -- __glComputeInverseTranspose (s_xform.c), without GL_RESCALE_NORMAL */
void __fastcall __glComputeInverseTranspose(__GLcontext *gc, __GLtransformICD *tr)
{
    (*__GL_MATRIX_INVERT_TRANSPOSE(gc))(&tr->inverseTranspose, &tr->matrix);
    (*gc->procs.pickInvTransposeProcs)(gc, &tr->inverseTranspose);
    tr->updateInverse = GL_FALSE;
}

/* 690814a0 -- __glPushProjectionMatrix (s_xform.c) */
void __fastcall __glPushProjectionMatrix(__GLcontext *gc)
{
    __GLtransformICD **trp, *tr, *stack;
    GLint num;

    num = gc->constants.maxProjectionStackDepth;
    trp = &gc->transform.projection;
    stack = gc->transform.projectionStack;
    tr = *trp;
    if (tr < &stack[num-1]) {
        tr[1].matrix = tr[0].matrix;
        tr[1].sequence = tr[0].sequence;
        *trp = tr + 1;
    } else {
        __glSetError(GL_STACK_OVERFLOW);
    }
}

/* 69081570 -- __glLoadIdentityProjectionMatrix (s_xform.c) */
void __fastcall __glLoadIdentityProjectionMatrix(__GLcontext *gc)
{
    __GLtransformICD *mvtr, *ptr;

    ptr = gc->transform.projection;
    (*__GL_MATRIX_MAKE_IDENTITY(gc))(&ptr->matrix);
    (*gc->procs.pickMatrixProcs)(gc, &ptr->matrix);
    if (++gc->transform.projectionSequence == 0) {
        __glInvalidateSequenceNumbers(gc);
    } else {
        ptr->sequence = gc->transform.projectionSequence;
    }

    /* update mvp matrix */
    mvtr = gc->transform.modelView;
    mvtr->sequence = ptr->sequence;
    (*__GL_MATRIX_MULT(gc))(&mvtr->mvp, &mvtr->matrix, &ptr->matrix);
    (*gc->procs.pickMvpMatrixProcs)(gc, &mvtr->mvp);
}

/* 690815e0 -- __glPushTextureMatrix (s_xform.c): the stack of the active texture unit */
void __fastcall __glPushTextureMatrix(__GLcontext *gc)
{
    __GLtransformICD **trp, *tr, *stack;
    GLint num;

    num = gc->constants.maxTextureStackDepth;
    trp = &gc->transform.texture[gc->fx.activeTexture];
    stack = gc->transform.textureStack[gc->fx.activeTexture];
    tr = *trp;
    if (tr < &stack[num-1]) {
        tr[1].matrix = tr[0].matrix;
        *trp = tr + 1;
    } else {
        __glSetError(GL_STACK_OVERFLOW);
    }
}

/* 69081680 -- __glLoadIdentityTextureMatrix (s_xform.c); the ICD schedules a validation */
void __fastcall __glLoadIdentityTextureMatrix(__GLcontext *gc)
{
    __GLtransformICD *tr = gc->transform.texture[gc->fx.activeTexture];

    (*__GL_MATRIX_MAKE_IDENTITY(gc))(&tr->matrix);
    (*gc->procs.pickMatrixProcs)(gc, &tr->matrix);
    __GL_DELAY_VALIDATE(gc);
}

/* 69081ae0 -- __glScaleMatrix (s_xform.c); the ICD also picks the depth procs */
void __fastcall __glScaleMatrix(__GLcontext *gc, __GLmatrixICD *m, const struct __glScaleRec *scale)
{
    __GLfloat x, y, z;

    if (m->matrixType > __GL_MT_IS2DNR) {
        m->matrixType = __GL_MT_IS2DNR;
    }
    x = scale->x;
    y = scale->y;
    z = scale->z;

    m->matrix[0][0] = x * m->matrix[0][0];
    m->matrix[0][1] = x * m->matrix[0][1];
    m->matrix[0][2] = x * m->matrix[0][2];
    m->matrix[0][3] = x * m->matrix[0][3];

    m->matrix[1][0] = y * m->matrix[1][0];
    m->matrix[1][1] = y * m->matrix[1][1];
    m->matrix[1][2] = y * m->matrix[1][2];
    m->matrix[1][3] = y * m->matrix[1][3];

    m->matrix[2][0] = z * m->matrix[2][0];
    m->matrix[2][1] = z * m->matrix[2][1];
    m->matrix[2][2] = z * m->matrix[2][2];
    m->matrix[2][3] = z * m->matrix[2][3];
    (*gc->procs.classifyMatrix)(gc, m);
}

/* 69081bb0 -- __glTranslateMatrix (s_xform.c); the ICD also picks the depth procs */
void __fastcall __glTranslateMatrix(__GLcontext *gc, __GLmatrixICD *m, const struct __glTranslationRec *trans)
{
    __GLfloat x, y, z;

    if (m->matrixType > __GL_MT_IS2DNR) {
        m->matrixType = __GL_MT_IS2DNR;
    }
    x = trans->x;
    y = trans->y;
    z = trans->z;
    m->matrix[3][0] = x * m->matrix[0][0] + y * m->matrix[1][0] + z * m->matrix[2][0] + m->matrix[3][0];
    m->matrix[3][1] = x * m->matrix[0][1] + y * m->matrix[1][1] + z * m->matrix[2][1] + m->matrix[3][1];
    m->matrix[3][2] = x * m->matrix[0][2] + y * m->matrix[1][2] + z * m->matrix[2][2] + m->matrix[3][2];
    m->matrix[3][3] = x * m->matrix[0][3] + y * m->matrix[1][3] + z * m->matrix[2][3] + m->matrix[3][3];
    (*gc->procs.classifyMatrix)(gc, m);
}

/* 69081d70 -- __glXForm2 (s_xform.c) */
void __fastcall __glXForm2(__GLcoord *res, const __GLfloat v[2], const __GLmatrixICD *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];

    res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
    res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
    res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + m->matrix[3][2];
    res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + m->matrix[3][3];
}

/* 69081dc0 -- __glXForm3 (s_xform.c); ToDo UNUSED CODE: nothing in the driver calls it (the ICD transforms
** through the gc procs and the x87 / SSE batch routines) */
void __fastcall __glXForm3(__GLcoord *res, const __GLfloat v[3], const __GLmatrixICD *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];

    res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0] + m->matrix[3][0];
    res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1] + m->matrix[3][1];
    res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2] + m->matrix[3][2];
    res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + z*m->matrix[2][3] + m->matrix[3][3];
}

/* 69081e30 -- __glXForm3_Batch (s_xform.c): n vertices, byte strides; ToDo UNUSED CODE: nothing in the
** driver calls it */
void __fastcall __glXForm3_Batch(__GLcoord *res, const __GLfloat v[3], const __GLmatrixICD *m,
                             GLint sstride, GLint dstride, GLint n)
{
    while (n) {
        __GLfloat x = v[0];
        __GLfloat y = v[1];
        __GLfloat z = v[2];

        res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0] + m->matrix[3][0];
        res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1] + m->matrix[3][1];
        res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2] + m->matrix[3][2];
        res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + z*m->matrix[2][3] + m->matrix[3][3];

        v   = (const __GLfloat *)(((const char *)v) + sstride);
        res = (__GLcoord *)(((char *)res) + dstride);
        --n;
    }
}

/* 69081ec0 -- __glXForm4 (s_xform.c) */
void __fastcall __glXForm4(__GLcoord *res, const __GLfloat v[4], const __GLmatrixICD *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];
    __GLfloat w = v[3];

    if (w == ((__GLfloat) 1.0)) {
        res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0] + m->matrix[3][0];
        res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1] + m->matrix[3][1];
        res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2] + m->matrix[3][2];
        res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + z*m->matrix[2][3] + m->matrix[3][3];
    } else {
        res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0] + w*m->matrix[3][0];
        res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1] + w*m->matrix[3][1];
        res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2] + w*m->matrix[3][2];
        res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + z*m->matrix[2][3] + w*m->matrix[3][3];
    }
}

/* 69081fc0 -- __glXForm4_Batch (s_xform.c): n vertices, byte strides */
void __fastcall __glXForm4_Batch(__GLcoord *res, const __GLfloat v[4], const __GLmatrixICD *m,
                             GLint sstride, GLint dstride, GLint n)
{
    while (n) {
        __GLfloat x = v[0];
        __GLfloat y = v[1];
        __GLfloat z = v[2];
        __GLfloat w = v[3];

        if (w == ((__GLfloat) 1.0)) {
            res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0] + m->matrix[3][0];
            res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1] + m->matrix[3][1];
            res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2] + m->matrix[3][2];
            res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + z*m->matrix[2][3] + m->matrix[3][3];
        } else {
            res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0] + w*m->matrix[3][0];
            res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1] + w*m->matrix[3][1];
            res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2] + w*m->matrix[3][2];
            res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + z*m->matrix[2][3] + w*m->matrix[3][3];
        }

        v   = (const __GLfloat *)(((const char *)v) + sstride);
        res = (__GLcoord *)(((char *)res) + dstride);
        --n;
    }
}

/* 690820e0 -- __glXForm4_Pre (s_xform.c): v times the transposed matrix */
void __fastcall __glXForm4_Pre(__GLcoord *res, const __GLfloat v[4], const __GLmatrixICD *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];
    __GLfloat w = v[3];

    res->x = x*m->matrix[0][0] + y*m->matrix[0][1] + z*m->matrix[0][2] + w*m->matrix[0][3];
    res->y = x*m->matrix[1][0] + y*m->matrix[1][1] + z*m->matrix[1][2] + w*m->matrix[1][3];
    res->z = x*m->matrix[2][0] + y*m->matrix[2][1] + z*m->matrix[2][2] + w*m->matrix[2][3];
    res->w = x*m->matrix[3][0] + y*m->matrix[3][1] + z*m->matrix[3][2] + w*m->matrix[3][3];
}

/* 69082590 -- __glGenericPickMvpMatrixProcs (s_xform.c); the ICD also picks the depth procs */
void __fastcall __glGenericPickMvpMatrixProcs(__GLcontext *gc, __GLmatrixICD *m)
{
    __glPickMatrixType(m, &gc->transform.modelView->matrix, &gc->transform.projection->matrix);
    ReCompute2DMatrix(gc, m);
    (*gc->procs.pickMatrixProcs)(gc, m);
    (*gc->procs.pickVertexProcs)(gc);
    (*gc->procs.classifyMatrix)(gc, m);
}

/* 69082bb0 -- __glCheckColorSubTableArgs (s_ctable.c): the texture targets only, no packed pixel
** types.  Returns the error, 0 if the arguments are good.
*/
GLenum __fastcall __glCheckColorSubTableArgs(__GLcontext *gc, GLenum target, GLsizei start, GLsizei count,
                               GLenum format, GLenum type)
{
    switch (target) {
    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
        break;
    default:
        return GL_INVALID_ENUM;
    }

    if (start < 0 || count < 0) {
        return GL_INVALID_VALUE;
    }

    switch (format) {
    case GL_RED:
    case GL_GREEN:
    case GL_BLUE:
    case GL_ALPHA:
    case GL_RGB:
    case GL_RGBA:
    case GL_ABGR_EXT:
    case GL_BGR_EXT:
    case GL_BGRA_EXT:
    case GL_LUMINANCE:
    case GL_LUMINANCE_ALPHA:
        break;
    default:
        return GL_INVALID_ENUM;
    }

    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
        break;
    default:
        return GL_INVALID_ENUM;
    }
    return 0;
}

/* 69082f70 -- __glInitColorTableStore (s_ctable.c): the colour table as the pixel destination */
void __fastcall __glInitColorTableStore(__GLcolorTableICD *ct, __GLpixelSpanInfoICD *spanInfo)
{
    spanInfo->dstImage = ct->table;
    spanInfo->dstSkipPixels = 0;
    spanInfo->dstSkipLines = 0;
    spanInfo->dstSwapBytes = GL_FALSE;
    spanInfo->dstLsbFirst = GL_TRUE;
    spanInfo->dstLineLength = ct->width;
    spanInfo->dstFormat = ct->baseFormat;
    spanInfo->dstType = ct->type;
    spanInfo->dstAlignment = __glBytesPerElement(ct->type);
}

/* 690833e0 -- __glim_GetColorTableParameterivEXT */
void __stdcall __glim_GetColorTableParameterivEXT(GLenum target, GLenum pname, GLint *params)
{
    __glGetColorTableParameterv(__glCurrentGC(), target, pname, params, NULL);
}

/* 69083410 -- __glGetColorTableParameterv (s_ctable.c): the common part of glGetColorTableParameter[if]vEXT,
** the value as float and / or int */
void __fastcall __glGetColorTableParameterv(__GLcontext *gc, GLenum target, GLenum pname, GLint *ip, GLfloat *fp)
{
    GLint *value;

    value = __glColorTableModifiers(gc, target, pname);
    if (value != NULL) {
        if (fp != NULL) {
            *fp = (GLfloat)*value;
        }
        if (ip != NULL) {
            *ip = *value;
        }
    }
}

/* 69083440 -- __glim_GetColorTableParameterfvEXT */
void __stdcall __glim_GetColorTableParameterfvEXT(GLenum target, GLenum pname, GLfloat *params)
{
    __glGetColorTableParameterv(__glCurrentGC(), target, pname, NULL, params);
}

/* 69083610 -- __glInitColorTables (s_ctable.c, empty): the first call of the init sequence in
** __glCreateContext (glcore_hand.c); the
** evaluator state is initialised by 69097ab0 */
void __fastcall __glInitColorTables(__GLcontext *gc)
{
}

/* 69083620 -- __glFreeColorTables (s_ctable.c, empty), the counterpart of 69083610; the evaluator state is
** freed by 69097be0 */
void __fastcall __glFreeColorTables()
{
}

/* ---- glDepthRange apply (the ICD's z-precision mode: FX_GL_*BPP_ZPRECISION inverts the depth range
** on a fullscreen Glide surface; an inverted range turns the mode off and replays the depth func) --- */
#ifndef GL_LEQUAL
#define GL_LEQUAL                       0x0203
#define GL_GEQUAL                       0x0206
#endif
/* the depth buffer's WGL buffer private block (wgldraw.h __WGLdepthPrivate) */
#define DEPTH_PRIVATE(gc)    ((__WGLdepthPrivate *)(gc)->depthBuffer.buf.wglBuf->depthPrivate)
/* ---- s_xform.c: __glInitTransformState ----------------------------------------------------- */
typedef void *(__fastcall *__GLcallocProc)(__GLcontext *gc, size_t n, size_t size);
#define __GL_VERTEX_SIZE_MASK       0xc000
#define __GL_TRANSFORM_ALLOC        (sizeof(__GLtransformICD) + 16)    /* room for the 16-byte rounding */
typedef void (__fastcall *__GLdoRotate)(__GLcontext *gc, __GLfloat angle, __GLfloat x, __GLfloat y, __GLfloat z);
typedef void (__fastcall *__GLdoXform)(__GLcontext *gc, __GLfloat x, __GLfloat y, __GLfloat z);
/* ---- s_xform.c: double-precision matrix / depth range entries -------------------------------- */
typedef void (__fastcall *__GLsetDepthRange)(__GLcontext *gc, GLdouble zNear, GLdouble zFar);
/* ---- s_xform.c: glClipPlane ------------------------------------------------------------------ */
typedef void (__fastcall *__GLxfProc)(__GLcoord *res, const __GLfloat *v, const __GLmatrixICD *m);
/* __GLtransform's SI members in the ICD's stack entry */
#define SEQUENCE(tr)        ((tr)->sequence)
typedef void (__fastcall *__GLmatProcs)(__GLcontext *gc, __GLmatrixICD *m);
#define UPDATE_INVERSE(tr)  ((tr)->updateInverse)
typedef void (__fastcall *__GLtrProc)(__GLcontext *gc, __GLtransformICD *tr);
/* ---- s_xform.c: __glDoMultMatrix ------------------------------------------------------------- */
typedef void (__fastcall *__GLpickMatrix)(__GLcontext *gc, __GLmatrixICD *m);
typedef void (__fastcall *__GLmatMult)(__GLmatrixICD *r, __GLmatrixICD *a, __GLmatrixICD *b);
#define __glDegreesToRadians    0.017453292f
struct __glTranslateRec { __GLfloat x, y, z; };
extern void __fastcall __glInvalidateSequenceNumbers(__GLcontext *gc);
extern void __fastcall __glScaleMatrix(__GLcontext *gc, __GLmatrixICD *m, const struct __glScaleRec *scale);
extern void __fastcall __glTranslateMatrix(__GLcontext *gc, __GLmatrixICD *m, const struct __glTranslationRec *trans);
extern void __fastcall __glXForm2(__GLcoord *res, const __GLfloat v[2], const __GLmatrixICD *m);
extern void __fastcall __glXForm4(__GLcoord *res, const __GLfloat v[4], const __GLmatrixICD *m);

/* ---- s_xform.c: __glPickMatrixType ------------------------------------------------------------ */

/* 6907fd50 -- __glPickMatrixType: dst = a * b, compute dst's matrix type */
void __fastcall __glPickMatrixType(__GLmatrixICD *dst, __GLmatrixICD *a, __GLmatrixICD *b)
{
    switch (a->matrixType) {
      case __GL_MT_GENERAL:
	dst->matrixType = a->matrixType;
	break;
      case __GL_MT_W0001:
	if (b->matrixType == __GL_MT_GENERAL) {
	    dst->matrixType = b->matrixType;
	} else {
	    dst->matrixType = a->matrixType;
	}
	break;
      case __GL_MT_IS2D:
	if (b->matrixType < __GL_MT_IS2D) {
	    dst->matrixType = b->matrixType;
	} else {
	    dst->matrixType = a->matrixType;
	}
	break;
      case __GL_MT_IS2DNR:
	if (b->matrixType < __GL_MT_IS2DNR) {
	    dst->matrixType = b->matrixType;
	} else {
	    dst->matrixType = a->matrixType;
	}
	break;
      case __GL_MT_IDENTITY:
	if (b->matrixType == __GL_MT_IS2DNRSC) {
	    dst->width = b->width;
	    dst->height = b->height;
	}
	dst->matrixType = b->matrixType;
	break;
      case __GL_MT_IS2DNRSC:
	if (b->matrixType == __GL_MT_IDENTITY) {
	    dst->matrixType = __GL_MT_IS2DNRSC;
	    dst->width = a->width;
	    dst->height = a->height;
	} else if (b->matrixType < __GL_MT_IS2DNR) {
	    dst->matrixType = b->matrixType;
	} else {
	    dst->matrixType = __GL_MT_IS2DNR;
	}
	break;
    }
}

/* 6907fe90 -- SetDepthRange (s_xform.c): apply glDepthRange(zNear, zFar) */
void __fastcall SetDepthRange(__GLcontext *gc, GLuint edx_unused, GLdouble zNear, GLdouble zFar)
{
    __GLviewport *vp = &gc->state.viewport;
    GLdouble scale;

    if (zNear < 0.0) zNear = 0.0;
    else if (zNear > 1.0) zNear = 1.0;
    if (zFar < 0.0) zFar = 0.0;
    else if (zFar > 1.0) zFar = 1.0;
    vp->zNear = zNear;
    vp->zFar = zFar;

    (*gc->drawablePrivate->lockDP)(gc->drawablePrivate, gc);
    /* the hardware depth range is inverted */
    if (DEPTH_PRIVATE(gc)->invertZRange) {
        zNear = 1.0 - zNear;
        zFar = 1.0 - zFar;
    }
    (*gc->drawablePrivate->unlockDP)(gc->drawablePrivate);

    vp->zNear = zNear;
    vp->zFar = zFar;

    if (zFar - zNear < 0.0 && (gc->fx.zPrecision32 || gc->fx.zPrecision16)) {
        if (!gc->fx.fullscreen) goto linear;
        gc->fx.zPrecision32 = 0;
        gc->fx.zPrecision16 = 0;
        if (gc->fx.lastDepthFunc != gc->fx.depthDirection) {
            __glsstim_DepthFunc(gc->fx.lastDepthFunc);
            ((void (__fastcall *)(__GLcontext *))hwFlushGrState)(gc);
            scale = (GLdouble)(gc->constants.half) * gc->constants.depthScale;
            gc->state.polygon.factor = gc->fx.savedPolygonFactor;
            gc->state.polygon.units = gc->fx.savedPolygonUnits;
            vp->scale.z = (zFar - zNear) * scale;
            vp->center.z = (zNear + zFar) * scale;
        }
    }
    if (gc->fx.fullscreen &&
            ((gc->modes.depthBits > 16 && gc->fx.zPrecision32) ||
             (gc->modes.depthBits == 16 && gc->fx.zPrecision16))) {
        scale = ((__GLfloat)(gc->constants.half) * gc->constants.depthScale) * gc->constants.half;
        if (gc->fx.depthDirection == GL_LEQUAL) {
            vp->scale.z =
                (vp->zFar - vp->zNear) * scale;
            vp->center.z =
                ((GLdouble)(vp->zNear) + vp->zFar) * scale;
        } else if (gc->fx.depthDirection == GL_GEQUAL) {
            vp->scale.z =
                -((vp->zFar - vp->zNear) * scale);
            vp->center.z = gc->constants.depthScale -
                ((GLdouble)(vp->zNear) + vp->zFar) * scale;
        }
        return;
    }
linear:
    scale = (__GLfloat)(gc->constants.half) * gc->constants.depthScale;
    vp->scale.z = (zFar - zNear) * scale;
    vp->center.z = (zNear + zFar) * scale;
}

/* 69080170 -- __glInitTransformState (the ICD rounds the calloc'd stacks up to 16 bytes and keeps the
** raw pointers; no colour matrix stack).  The stacks are written through gc->transform directly (a separate
** transform pointer changes the schedule). */
void __fastcall __glInitTransformState(__GLcontext *gc)
{
    GLint i, numClipPlanes, numClipTemp;
    GLuint p;
    __GLtransformICD *tr;
    __GLvertexICD *vx;

    numClipPlanes = gc->constants.numberOfClipPlanes;

    gc->transform.modelViewStackAlloc = (GLuint)
	(*(__GLcallocProc)gc->imports[1])(gc, (size_t)gc->constants.maxModelViewStackDepth,
			      __GL_TRANSFORM_ALLOC);
    gc->transform.modelViewStack = (struct __GLtransformICDRec *) (__GLtransformICD *)((gc->transform.modelViewStackAlloc + 15) & ~15);
    gc->transform.projectionStackAlloc = (GLuint)
	(*(__GLcallocProc)gc->imports[1])(gc, (size_t)gc->constants.maxProjectionStackDepth,
			      __GL_TRANSFORM_ALLOC);
    gc->transform.projectionStack = (struct __GLtransformICDRec *) (__GLtransformICD *)((gc->transform.projectionStackAlloc + 15) & ~15);
    for (i = 0; i < __GL_NUM_TEXTURE_UNITS; ++i) {
	p = (GLuint) (*(__GLcallocProc)gc->imports[1])(gc, (size_t)gc->constants.maxTextureStackDepth,
				  __GL_TRANSFORM_ALLOC);
	gc->transform.textureStackAlloc[i] = p;
	gc->transform.textureStack[i] = (struct __GLtransformICDRec *)((p + 15) & ~15);
    }

    numClipTemp = 2 * (6 + numClipPlanes);
    gc->transform.clipTemp =
	(*(__GLcallocProc)gc->imports[1])(gc, (size_t)numClipTemp, sizeof(__GLvertexICD));

    gc->state.transform.matrixMode = GL_MODELVIEW;
    ((void (__fastcall *)(__GLcontext *, GLdouble, GLdouble))SetDepthRange)(gc, 0.0, 1.0);

    gc->transform.modelView = (struct __GLtransformICDRec *)(tr = &((__GLtransformICD *)gc->transform.modelViewStack)[0]);
    (*gc->procs.matrix[2])(&tr->matrix);
    (*gc->procs.matrix[2])(&tr->inverseTranspose);
    (*gc->procs.matrix[2])(&tr->mvp);
    (*gc->procs.pickMatrixProcs)(gc, &tr->matrix);
    (*gc->procs.pickInvTransposeProcs)(gc, &tr->inverseTranspose);
    tr->updateInverse = GL_FALSE;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_MODELVIEW);

    gc->transform.projection = (struct __GLtransformICDRec *)(tr = &((__GLtransformICD *)gc->transform.projectionStack)[0]);
    (*gc->procs.matrix[2])(&tr->matrix);
    (*gc->procs.pickMatrixProcs)(gc, &tr->matrix);
    (*gc->procs.pickMvpMatrixProcs)(gc, &((__GLtransformICD *)gc->transform.modelView)->mvp);

    for (i = 0; i < __GL_NUM_TEXTURE_UNITS; ++i) {
	gc->transform.texture[i] = (struct __GLtransformICDRec *)(tr = &((__GLtransformICD *)gc->transform.textureStack[i])[0]);
	(*gc->procs.matrix[2])(&tr->matrix);
	(*gc->procs.pickMatrixProcs)(gc, &tr->matrix);
    }

    gc->state.transform.eyePos.z = __GL_ONE_F;
    gc->state.transform.eyePosObj.z = __GL_ONE_F;
    gc->state.transform.objEyeSpecified = GL_FALSE;

    vx = &gc->transform.clipTemp[0];
    for (i = 0; i < numClipTemp; i++, vx++) {
	vx->color = &vx->colors[__GL_FRONTFACE];
	vx->flagBits = (vx->flagBits & ~__GL_VERTEX_SIZE_MASK) | __GL_VERTEX_4D_SIZE;
    }
    gc->state.current.normal.z = __GL_ONE_F;
}

/* 690805e0 -- __glim_MultMatrixf */
void __stdcall __glim_MultMatrixf(const GLfloat m[16])
{
    __GLmatrixICD m1;
    __GL_SETUP_NOT_IN_BEGIN();

    m1.matrix[0][0] = m[0];
    m1.matrix[0][1] = m[1];
    m1.matrix[0][2] = m[2];
    m1.matrix[0][3] = m[3];
    m1.matrix[1][0] = m[4];
    m1.matrix[1][1] = m[5];
    m1.matrix[1][2] = m[6];
    m1.matrix[1][3] = m[7];
    m1.matrix[2][0] = m[8];
    m1.matrix[2][1] = m[9];
    m1.matrix[2][2] = m[10];
    m1.matrix[2][3] = m[11];
    m1.matrix[3][0] = m[12];
    m1.matrix[3][1] = m[13];
    m1.matrix[3][2] = m[14];
    m1.matrix[3][3] = m[15];
    m1.matrixType = __GL_MT_GENERAL;
    __glDoMultMatrix(gc, &m1, __glMultiplyMatrix);
}

/* 690806a0 -- __glim_MultMatrixd */
void __stdcall __glim_MultMatrixd(const GLdouble m[16])
{
    __GLmatrixICD m1;
    __GL_SETUP_NOT_IN_BEGIN();

    m1.matrixType = __GL_MT_GENERAL;
    m1.matrix[0][0] = (__GLfloat)m[0];
    m1.matrix[0][1] = (__GLfloat)m[1];
    m1.matrix[0][2] = (__GLfloat)m[2];
    m1.matrix[0][3] = (__GLfloat)m[3];
    m1.matrix[1][0] = (__GLfloat)m[4];
    m1.matrix[1][1] = (__GLfloat)m[5];
    m1.matrix[1][2] = (__GLfloat)m[6];
    m1.matrix[1][3] = (__GLfloat)m[7];
    m1.matrix[2][0] = (__GLfloat)m[8];
    m1.matrix[2][1] = (__GLfloat)m[9];
    m1.matrix[2][2] = (__GLfloat)m[10];
    m1.matrix[2][3] = (__GLfloat)m[11];
    m1.matrix[3][0] = (__GLfloat)m[12];
    m1.matrix[3][1] = (__GLfloat)m[13];
    m1.matrix[3][2] = (__GLfloat)m[14];
    m1.matrix[3][3] = (__GLfloat)m[15];
    __glDoMultMatrix(gc, &m1, __glMultiplyMatrix);
}

extern void __fastcall __glDoRotate(__GLcontext *gc, GLuint unused, __GLfloat angle, __GLfloat ax, __GLfloat ay, __GLfloat az);
/* 690807c0 -- __glim_Rotated */
void __stdcall __glim_Rotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    __GL_SETUP_NOT_IN_BEGIN();
    ((__GLdoRotate)__glDoRotate)(gc, (GLfloat)angle, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}

/* 69080870 -- __glim_Scaled */
void __stdcall __glim_Scaled(GLdouble x, GLdouble y, GLdouble z)
{
    __GL_SETUP_NOT_IN_BEGIN();
    ((__GLdoXform)__glDoScale)(gc, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}

/* 69080920 -- __glim_Translated */
void __stdcall __glim_Translated(GLdouble x, GLdouble y, GLdouble z)
{
    __GL_SETUP_NOT_IN_BEGIN();
    ((__GLdoXform)__glDoTranslate)(gc, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}

/* ---- s_xform.c: glFrustum (the ICD arms a depth clear while the z-precision mode is on) -------- */

/* 69080a00 -- __glim_Frustum */
void __stdcall __glim_Frustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top,
                             GLdouble zNear, GLdouble zFar)
{
    __GLmatrixICD m;
    __GLfloat deltaX, deltaY, deltaZ;
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->fx.fullscreen &&
            ((gc->modes.depthBits > 16 && gc->fx.zPrecision32) ||
             (gc->modes.depthBits == 16 && gc->fx.zPrecision16)) &&
            gc->fx.depthDirection == GL_LEQUAL) {
        gc->fx.depthClearPending = 1;
    }

    deltaX = right - left;
    deltaY = top - bottom;
    deltaZ = zFar - zNear;
    if ((zNear <= 0.0) || (zFar <= 0.0) || (deltaX == __GL_ZERO_F) ||
            (deltaY == __GL_ZERO_F) || (deltaZ == __GL_ZERO_F)) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }

    (*gc->procs.matrix[2])(&m);
    m.matrix[0][0] = zNear * 2.0 / deltaX;
    m.matrix[1][1] = zNear * 2.0 / deltaY;
    m.matrix[2][0] = (right + left) / deltaX;
    m.matrix[2][1] = (top + bottom) / deltaY;
    m.matrix[2][2] = -(zFar + zNear) / deltaZ;
    m.matrix[2][3] = -1.0f;
    m.matrix[3][2] = ((GLdouble) -2.0) * zNear * zFar / deltaZ;
    m.matrix[3][3] = __GL_ZERO_F;
    m.matrixType = __GL_MT_GENERAL;
    __glDoMultMatrix(gc, &m, __glMultiplyMatrix);
}

/* 69080fe0 -- __glim_DepthRange */
void __stdcall __glim_DepthRange(GLdouble zNear, GLdouble zFar)
{
    __GL_SETUP_NOT_IN_BEGIN();
    ((__GLsetDepthRange)SetDepthRange)(gc, zNear, zFar);
    __gl_beginMode = __GL_NEED_VALIDATE;
    gc->dirtyMask |= __GL_DIRTY_GENERIC | __GL_DIRTY_DEPTH;
}

/* 69081140 -- __glim_ClipPlane */
void __stdcall __glim_ClipPlane(GLenum pi, const GLdouble pv[])
{
    __GLfloat p[4];
    __GLtransformICD *tr;
    __GL_SETUP_NOT_IN_BEGIN();

    if (pi < GL_CLIP_PLANE0) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    pi -= GL_CLIP_PLANE0;
    if (pi >= (GLuint)gc->constants.numberOfClipPlanes) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    p[0] = pv[0];
    p[1] = pv[1];
    p[2] = pv[2];
    p[3] = pv[3];

    tr = gc->transform.modelView;
    if (tr->updateInverse) {
        (*gc->procs.computeInverseTranspose)(gc, tr);
    }
    (*(__GLxfProc)tr->inverseTranspose.xf4)(&gc->state.transform.eyeClipPlanes[pi], p,
                                            &tr->inverseTranspose);
    __GL_DELAY_VALIDATE(gc);
}

/* 69081290 -- __glPopModelViewMatrix (s_xform.c) */
void __fastcall __glPopModelViewMatrix(__GLcontext *gc)
{
    __GLtransformICD **trp, *tr, *stack, *mvtr, *ptr;

    trp = &gc->transform.modelView;
    stack = gc->transform.modelViewStack;
    tr = *trp;
    if (tr > &stack[0]) {
	*trp = tr - 1;

	/*
	** See if sequence number of modelView matrix is the same as the
	** sequence number of the projection matrix.  If not, then
	** recompute the mvp matrix.
	*/
	mvtr = gc->transform.modelView;
	ptr = gc->transform.projection;
	if (SEQUENCE(mvtr) != SEQUENCE(ptr)) {
	    SEQUENCE(mvtr) = SEQUENCE(ptr);
	    (*__GL_MATRIX_MULT(gc))(&mvtr->mvp, &mvtr->matrix, &ptr->matrix);
	}
	(*(__GLmatProcs)gc->procs.pickMvpMatrixProcs)(gc, &mvtr->mvp);
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_MODELVIEW);
    } else {
	__glSetError(GL_STACK_UNDERFLOW);
	return;
    }
}

/* 690813f0 -- __glValidateCullVertexFromModelView (s_xform.c) */
void __fastcall __glValidateCullVertexFromModelView(__GLcontext *gc)
{
    if (gc->state.enables.general & __GL_CULL_VERTEX_ENABLE) {
	/* 
	** If the user has not specified a culling eye position in
	** object space, calculate object space eye direction or location.
	*/
	if (gc->state.transform.objEyeSpecified) {
	    gc->cullEyeObj = gc->state.transform.eyePosObj;
	} else {
	    __GLtransformICD *tr;

	    tr = gc->transform.modelView;

	    if (UPDATE_INVERSE(tr)) {
		(*(__GLtrProc)gc->procs.computeInverseTranspose)(gc, tr);
	    }

	    if (gc->state.transform.eyePos.w != 0.0f) {
		__glXForm4_Pre(&gc->cullEyeObj,
			       &gc->state.transform.eyePos.x,
			       &tr->inverseTranspose);
	    } else {
		__glXForm4_Pre(&gc->cullEyeObj,
			       &gc->state.transform.eyePos.x,
			       &tr->matrix);
		gc->cullEyeObj.w = 0.0f;
	    }
	}
    }
}

/* 69081500 -- __glPopProjectionMatrix */
void __fastcall __glPopProjectionMatrix(__GLcontext *gc)
{
    __GLtransformICD **trp, *tr, *stack, *mvtr, *ptr;

    trp = &gc->transform.projection;
    stack = gc->transform.projectionStack;
    tr = *trp;
    if (tr > &stack[0]) {
        *trp = tr - 1;

        mvtr = gc->transform.modelView;
        ptr = gc->transform.projection;
        if (mvtr->sequence != ptr->sequence) {
            mvtr->sequence = ptr->sequence;
            (*gc->procs.matrix[3])(&mvtr->mvp, &mvtr->matrix, &ptr->matrix);
        }
        (*gc->procs.pickMvpMatrixProcs)(gc, &mvtr->mvp);
    } else {
        __glSetError(GL_STACK_UNDERFLOW);
        return;
    }
}

/* 69081640 -- __glPopTextureMatrix (the active unit's stack) */
void __fastcall __glPopTextureMatrix(__GLcontext *gc)
{
    GLint unit = gc->fx.activeTexture;
    __GLtransformICD *tr = (__GLtransformICD *)gc->transform.texture[unit];

    if (tr > (__GLtransformICD *)gc->transform.textureStack[unit]) {
	gc->transform.texture[unit] = (struct __GLtransformICDRec *)(tr - 1);
    } else {
	__glSetError(GL_STACK_UNDERFLOW);
    }
}

/* 690816d0 -- __glDoLoadMatrix (s_xform.c): the texture matrix of the 3dfx active unit, no colour
** matrix */
void __fastcall __glDoLoadMatrix(__GLcontext *gc, const __GLmatrixICD *m)
{
    __GLtransformICD *tr, *otr;

    switch (gc->state.transform.matrixMode) {
      case GL_MODELVIEW:
	tr = gc->transform.modelView;
	(*__GL_MATRIX_COPY(gc))(&tr->matrix, m);
	(*(__GLmatProcs)gc->procs.pickMatrixProcs)(gc, &tr->matrix);
	UPDATE_INVERSE(tr) = GL_TRUE;
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_MODELVIEW);

	/* Update mvp matrix */
	otr = gc->transform.projection;
	SEQUENCE(tr) = SEQUENCE(otr);
	(*__GL_MATRIX_MULT(gc))(&tr->mvp, &tr->matrix, &otr->matrix);
	(*(__GLmatProcs)gc->procs.pickMvpMatrixProcs)(gc, &tr->mvp);
	break;

      case GL_PROJECTION:
	tr = gc->transform.projection;
	(*__GL_MATRIX_COPY(gc))(&tr->matrix, m);
	(*(__GLmatProcs)gc->procs.pickMatrixProcs)(gc, &tr->matrix);
	if (++gc->transform.projectionSequence == 0) {
	    __glInvalidateSequenceNumbers(gc);
	} else {
	    SEQUENCE(tr) = gc->transform.projectionSequence;
	}

	/* Update mvp matrix */
	otr = gc->transform.modelView;
	SEQUENCE(otr) = SEQUENCE(tr);
	(*__GL_MATRIX_MULT(gc))(&otr->mvp, &otr->matrix, &tr->matrix);
	(*(__GLmatProcs)gc->procs.pickMvpMatrixProcs)(gc, &otr->mvp);
	break;

      case GL_TEXTURE:
	tr = gc->transform.texture[gc->fx.activeTexture];
	(*__GL_MATRIX_COPY(gc))(&tr->matrix, m);
	(*(__GLmatProcs)gc->procs.pickMatrixProcs)(gc, &tr->matrix);
	__GL_DELAY_VALIDATE(gc);
	break;
    }
}

/* 69081810 -- __glDoMultMatrix (the ICD has no GL_COLOR matrix) */
void __fastcall __glDoMultMatrix(__GLcontext *gc, void *data, __GLmultiplyMatrixProc multiply)
{
    __GLtransformICD *tr, *otr;

    switch (gc->state.transform.matrixMode) {
      case GL_MODELVIEW:
	tr = (__GLtransformICD *)gc->transform.modelView;
	(*multiply)(gc, &tr->matrix, data);
	(*(__GLpickMatrix)gc->procs.pickMatrixProcs)(gc, &tr->matrix);
	tr->updateInverse = GL_TRUE;
	__gl_beginMode = __GL_NEED_VALIDATE;
	gc->dirtyMask |= __GL_DIRTY_MODELVIEW;

	/* Update mvp matrix */
	(*multiply)(gc, &tr->mvp, data);
	(*(__GLpickMatrix)gc->procs.pickMvpMatrixProcs)(gc, &tr->mvp);
	break;

      case GL_PROJECTION:
	tr = (__GLtransformICD *)gc->transform.projection;
	(*multiply)(gc, &tr->matrix, data);
	(*(__GLpickMatrix)gc->procs.pickMatrixProcs)(gc, &tr->matrix);
	if (++gc->transform.projectionSequence == 0) {
	    __glInvalidateSequenceNumbers(gc);
	} else {
	    tr->sequence = gc->transform.projectionSequence;
	}

	/* Update mvp matrix */
	otr = (__GLtransformICD *)gc->transform.modelView;
	otr->sequence = tr->sequence;
	(*(__GLmatMult)gc->procs.matrix[3])(&otr->mvp, &otr->matrix, &tr->matrix);
	(*(__GLpickMatrix)gc->procs.pickMvpMatrixProcs)(gc, &otr->mvp);
	break;

      case GL_TEXTURE:
	tr = (__GLtransformICD *)gc->transform.texture[gc->fx.activeTexture];
	(*multiply)(gc, &tr->matrix, data);
	(*(__GLpickMatrix)gc->procs.pickMatrixProcs)(gc, &tr->matrix);
	__gl_beginMode = __GL_NEED_VALIDATE;
	gc->dirtyMask |= __GL_DIRTY_GENERIC;
	break;
    }
}

/* __glDoMultMatrix (69081810) applies a multiply proc (gc, matrix, data) to the current matrix: si_names.h */

/* 69081950 -- __glDoRotate (s_xform.c) */
void __fastcall __glDoRotate(__GLcontext *gc, GLuint unused, __GLfloat angle, __GLfloat ax, __GLfloat ay, __GLfloat az)
{
    __GLmatrixICD m;
    __GLfloat radians, sine, cosine, ab, bc, ca, t, t2;
    __GLfloat av[4], axis[4];

    av[0] = ax;
    av[1] = ay;
    av[2] = az;
    av[3] = 0;
    (*gc->procs.normalize)(axis, av);

    radians = angle * __glDegreesToRadians;
    sine = (__GLfloat) sin(radians);
    cosine = (__GLfloat) cos(radians);
    ab = (1 - cosine) * axis[0] * axis[1];
    t2 = (1 - cosine) * axis[2];
    bc = t2 * axis[1];
    ca = t2 * axis[0];

    (*gc->procs.matrix[2])(&m);
    t = axis[0] * axis[0];
    m.matrix[0][0] = t + cosine * (1 - t);
    m.matrix[2][1] = bc - axis[0] * sine;
    m.matrix[1][2] = bc + axis[0] * sine;

    t = axis[1] * axis[1];
    m.matrix[1][1] = t + cosine * (1 - t);
    m.matrix[2][0] = ca + axis[1] * sine;
    m.matrix[0][2] = ca - axis[1] * sine;

    t = axis[2] * axis[2];
    m.matrix[2][2] = t + cosine * (1 - t);
    m.matrix[1][0] = ab - axis[2] * sine;
    m.matrix[0][1] = ab + axis[2] * sine;
    if (ax == __GL_ZERO_F && ay == __GL_ZERO_F) {
	m.matrixType = __GL_MT_IS2D;
    } else {
	m.matrixType = __GL_MT_W0001;
    }
    __glDoMultMatrix(gc, &m, __glMultiplyMatrix);
}

/* 69081b80 -- __glDoScale */
void __fastcall __glDoScale(__GLcontext *gc, GLuint edx_unused, __GLfloat x, __GLfloat y, __GLfloat z)
{
    struct __glScaleRec scale;

    scale.x = x;
    scale.y = y;
    scale.z = z;
    __glDoMultMatrix(gc, &scale, __glScaleMatrix);
}

/* 69081c40 -- __glDoTranslate */
void __fastcall __glDoTranslate(__GLcontext *gc, GLuint edx_unused, __GLfloat x, __GLfloat y, __GLfloat z)
{
    struct __glTranslateRec translate;

    translate.x = x;
    translate.y = y;
    translate.z = z;
    __glDoMultMatrix(gc, &translate, __glTranslateMatrix);
}

/* 69082680 -- __glGenericPickMatrixProcs (s_xform.c): pick a matrix's xform procs by its type, then the
** inverse-transpose procs */
void __fastcall __glGenericPickMatrixProcs(__GLcontext *gc, __GLmatrixICD *m)
{
    switch (m->matrixType) {
      case 0:
        m->xf2 = (__GLproc)__glXForm2;
        m->xf3 = (__GLproc)__glI386XForm3;
        m->xf4 = (__GLproc)__glI386XForm4;
        break;
      case 1:
        m->xf2 = (__GLproc)__glXForm2_W;
        m->xf3 = (__GLproc)__glI386XForm3_W;
        m->xf4 = (__GLproc)__glI386XForm4_W;
        break;
      case 2:
        m->xf2 = (__GLproc)__glXForm2_2DW;
        m->xf3 = (__GLproc)__glXForm3_2DW;
        m->xf4 = (__GLproc)__glXForm4_2DW;
        break;
      case 3:
      case 4:
      case 5:
        m->xf2 = (__GLproc)__glXForm2_2DNRW;
        m->xf3 = (__GLproc)__glXForm3_2DNRW;
        m->xf4 = (__GLproc)__glXForm4_2DNRW;
        break;
    }
    (*gc->procs.classifyMatrix)(gc, m);
}

/* 69082710 -- __glGenericPickInvTransposeProcs (s_xform.c): inverse-transpose matrix procs (xf3 by type, xf4
** always general) */
void __fastcall __glGenericPickInvTransposeProcs(__GLcontext *gc, __GLmatrixICD *m)
{
    m->xf4 = (__GLproc)__glXForm4;
    switch (m->matrixType) {
      case 0:
        m->xf3 = (__GLproc)__glXForm4;
        break;
      case 1:
        m->xf3 = (__GLproc)__glXForm3_W;
        break;
      case 2:
        m->xf3 = (__GLproc)__glXForm3_2DW;
        break;
      case 3:
      case 4:
      case 5:
        m->xf3 = (__GLproc)__glXForm3_2DNRW;
        break;
    }
}

#pragma optimize("y", off)
/* 69080ba0 -- __glim_Ortho (s_xform.c) without the screen-coordinates check: always IS2DNR.
** The original keeps a frame pointer here. */
void __stdcall __glim_Ortho(GLdouble left, GLdouble right,
			     GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    __GLmatrixICD m;
    GLdouble deltax, deltay, deltaz;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    deltax = right - left;
    deltay = top - bottom;
    deltaz = zFar - zNear;
    if ((deltax == 0.0) || (deltay == 0.0) || (deltaz == 0.0)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    (*__GL_MATRIX_MAKE_IDENTITY(gc))(&m);
    m.matrix[0][0] = 2.0 / deltax;
    m.matrix[3][0] = -(right + left) / deltax;
    m.matrix[1][1] = 2.0 / deltay;
    m.matrix[3][1] = -(top + bottom) / deltay;
    m.matrix[2][2] = -2.0 / deltaz;
    m.matrix[3][2] = -(zFar + zNear) / deltaz;
    m.matrixType = __GL_MT_IS2DNR;

    __glDoMultMatrix(gc, &m, __glMultiplyMatrix);
}
#pragma optimize("", on)

#pragma optimize("y", off)
/* 69080e90 -- __glim_Viewport (s_xform.c) with the ICD's clamps: to the maximum viewport size, and
** full-screen without the guard band to the desktop.  The original keeps a frame pointer here. */
void __stdcall __glim_Viewport(GLint x, GLint y,
			     GLsizei width, GLsizei height)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((width < 0) || (height < 0)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    if (height > gc->constants.maxViewportHeight) height = gc->constants.maxViewportHeight;
    if (width > gc->constants.maxViewportWidth) width = gc->constants.maxViewportWidth;
    if (gc->fx.fullscreen && !gc->fx.guardBandOk) {
	GLuint desktopWidth, desktopHeight;     /* block scope: the original's frame slots */
	wglGetDesktopSize((LONG *)&desktopWidth, (LONG *)&desktopHeight);
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if ((GLuint)x > desktopWidth) x = desktopWidth;
	if ((GLuint)y > desktopHeight) y = desktopHeight;
	if ((GLuint)(width + x) > desktopWidth) width = desktopWidth - x;
	if ((GLuint)(y + height) > desktopHeight) height = desktopHeight - y;
    }
    gc->state.viewport.x = x;
    gc->state.viewport.y = y;
    gc->state.viewport.width = width;
    gc->state.viewport.height = height;
    (*gc->procs.applyViewport)(gc);
    __GL_DELAY_VALIDATE(gc);
}
#pragma optimize("", on)

/* ---- CPU feature detection ---- */
GLuint __glCpuCaps;             /* 693c3c88: 1 MMX, 2 3DNow!, 4 KNI (display driver), 8 K6, 0x10 K6-2+ */
GLuint __glCpuCapsChecked;      /* 693c3c8c */

/*
** 6908278f -- __glDetectCPU: detect the CPU once (CPUID: MMX, AMD extended 3DNow!, the AMD K6 family by model),
** ask the 3dfx display driver (ExtEscape 0x3df4, else 0xfd4) whether KNI may be used, apply the
** __GL_FORCE_MMX / __GL_FORCE_K3D / __GL_FORCE_KNI overrides and OR the result into
** gc->constants.cpuCaps.  Unoptimised in the original; the CPUID probe is inline assembly.
*/
#pragma optimize("", off)
void __fastcall __glDetectCPU(__GLcontext *gc)
{
    GLuint feat;                    /* extended features; the /Od frame slots follow the names */
    GLint amdFamily;
    GLuint features;
    GLuint inBuf;
    HDC hdc;
    GLuint outData;
    char *envString;

    if (!__glCpuCapsChecked) {
        features = 0;
        feat = 0;
        amdFamily = 0;
        __glCpuCapsChecked = 1;
        __asm {
            push    eax
            push    ebx
            push    ecx
            push    edx
            pushfd
            pop     eax
            mov     ebx, eax
            xor     eax, 0x200000
            push    eax
            popfd
            pushfd
            pop     eax
            xor     eax, ebx
            je      done
            push    ebx
            popfd
            mov     eax, 0x80000000
            cpuid
            cmp     eax, 0x80000000
            jle     noExtended
            mov     eax, 0x80000001
            cpuid
            mov     feat, edx
        noExtended:
            mov     eax, 0
            cpuid
            cmp     ecx, 0x444d4163         /* "cAMD" of AuthenticAMD */
            jne     generic
            mov     eax, 1
            cpuid
            mov     features, edx
            and     eax, 0xff0
            cmp     eax, 0x560
            jl      generic
            cmp     eax, 0x600
            jl      k6
            mov     eax, 7
            mov     amdFamily, eax
            jmp     done
        k6:
            mov     eax, 6
            mov     amdFamily, eax
            jmp     done
        generic:
            mov     eax, 1
            cpuid
            mov     features, edx
        done:
            pop     edx
            pop     ecx
            pop     ebx
            pop     eax
        }
        if (amdFamily == 6) {
            __glCpuCaps |= 8;
        } else if (amdFamily == 7) {
            __glCpuCaps |= 0x10;
        }
        if (features & 0x800000) {
            __glCpuCaps |= 1;
        }
        if (feat & 0x80000000) {
            __glCpuCaps |= 2;
        }
        hdc = GetDC(NULL);
        inBuf = 0;
        outData = 0;
        if (ExtEscape(hdc, 0x3df4, 4, (LPCSTR)&inBuf, 4, (LPSTR)&outData)) {
            if (outData & 0x80000000) {
                __glCpuCaps |= 4;
            }
        } else {
            if (ExtEscape(hdc, 0xfd4, 4, (LPCSTR)&inBuf, 4, (LPSTR)&outData) && (outData & 0x80000000)) {
                __glCpuCaps |= 4;
            }
        }
        ReleaseDC(NULL, hdc);
        envString = getenv("__GL_FORCE_MMX");
        if (envString) {
            __glCpuCaps = atoi(envString) ? __glCpuCaps | 1 : __glCpuCaps & ~1;
        }
        envString = getenv("__GL_FORCE_K3D");
        if (envString) {
            __glCpuCaps = atoi(envString) ? __glCpuCaps | 2 : __glCpuCaps & ~2;
        }
        envString = getenv("__GL_FORCE_KNI");
        if (envString) {
            __glCpuCaps = atoi(envString) ? __glCpuCaps | 4 : __glCpuCaps & ~4;
        }
    }
    gc->constants.cpuCaps |= __glCpuCaps;
}
#pragma optimize("", on)

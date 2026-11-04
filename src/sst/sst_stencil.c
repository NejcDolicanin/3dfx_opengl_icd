/*
** sst_stencil.c -- 3dfx back end, original VA range 6901d000..69020000: hardware-path versions of glFrontFace,
** glStencilMask, glStencilOp and glPointParameterfEXT, the GL -> Glide stencil op map and the Glide
** stencil state update.
** Then glStencilFunc and the 32-bit depth / stencil buffer's fetch / store, validation and op tables, in
** address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include <g3ext.h>
#include "glsi.h"
#include "icdprotos.h"
#include "glshade.h"
#include "si_names.h"
#include "icdglob.h"
#include "og.h"
#include "glsitypes.h"

#define GL_INCR_WRAP_EXT_W          0x8507
#define GL_DECR_WRAP_EXT_W          0x8508
#define GL_POINT_SIZE_MIN_EXT_W     0x8126
#define GL_POINT_SIZE_MAX_EXT_W     0x8127
#define GL_POINT_FADE_THRESHOLD_SIZE_EXT_W 0x8128

/* ---- forward declarations ---- */
void __stdcall __glsstim_FrontFace(GLenum dir);
void __stdcall __glsstim_StencilMask(GLuint sm);
void __stdcall __glsstim_StencilOp(GLenum fail, GLenum depthFail, GLenum depthPass);
GLint __fastcall hwStencilOpToGlide(GLenum op);
void __stdcall __glsstim_PointParameterfEXT(GLenum pname, GLfloat param);

/* 6901d850 -- __glsstim_FrontFace: __glim_FrontFace (s_attrib.c), hardware path: also resets the Glide buffer
** modes */
void __stdcall __glsstim_FrontFace(GLenum dir)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (dir >= GL_CW && dir <= GL_CCW) {
        gc->state.polygon.frontFaceDirection = dir;
        wglGlideSetupBufferModes(gc);
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
        return;
    }
    __glSetError(GL_INVALID_ENUM);
}

/* 6901d8f0 -- __glsstim_StencilMask: __glim_StencilMask (s_attrib.c), hardware path: also the Glide stencil write
** mask */
void __stdcall __glsstim_StencilMask(GLuint sm)
{
    __GL_SETUP_NOT_IN_BEGIN();

    gc->state.stencil.writeMask = (GLshort)(((1 << gc->modes.stencilBits) - 1) & sm);
    __GL_DELAY_VALIDATE(gc);
    gc->validateMask |= 2;                      /* __GL_VALIDATE_STENCIL_FUNC */
    if (gc->hwcx.has32bpp) {
        GRS_SETB(__GL_GR_STENCIL_MASK, 0, (char)gc->state.stencil.writeMask);
    }
}

/* 6901db00 -- __glsstim_StencilOp: __glim_StencilOp (s_attrib.c), hardware path: the ops are checked through the
** Glide map (so the wrap ops are accepted) and also go to the Glide stencil op record */
void __stdcall __glsstim_StencilOp(GLenum fail, GLenum depthFail, GLenum depthPass)
{
    GLint grFail, grDepthFail, grDepthPass;
    __GL_SETUP_NOT_IN_BEGIN();

    grFail = hwStencilOpToGlide(fail);
    if (grFail == -1) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    grDepthFail = hwStencilOpToGlide(depthFail);
    if (grDepthFail == -1) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    grDepthPass = hwStencilOpToGlide(depthPass);
    if (grDepthPass == -1) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->state.stencil.fail = fail;
    gc->state.stencil.depthFail = depthFail;
    gc->state.stencil.depthPass = depthPass;
    __GL_DELAY_VALIDATE(gc);
    gc->validateMask |= 4;                      /* __GL_VALIDATE_STENCIL_OP */
    if (gc->hwcx.has32bpp) {
        GRS_SET(__GL_GR_STENCIL_OP, 0, grFail);
        GRS_SET(__GL_GR_STENCIL_OP, 1, grDepthFail);
        GRS_SET(__GL_GR_STENCIL_OP, 2, grDepthPass);
    }
}

/* 6901dc70 -- hwStencilOpToGlide: GL stencil op -> GrStencilOp_t, -1 for an invalid op */
GLint __fastcall hwStencilOpToGlide(GLenum op)
{
    switch (op) {
      case GL_INCR:
        return GR_STENCILOP_INCR_CLAMP;
      case GL_REPLACE:
        return GR_STENCILOP_REPLACE;
      case GL_KEEP:
        return GR_STENCILOP_KEEP;
      case GL_ZERO:
        return GR_STENCILOP_ZERO;
      case GL_INVERT:
        return GR_STENCILOP_INVERT;
      case GL_DECR:
        return GR_STENCILOP_DECR_CLAMP;
      case GL_INCR_WRAP_EXT_W:
        return GR_STENCILOP_INCR_WRAP;
      case GL_DECR_WRAP_EXT_W:
        return GR_STENCILOP_DECR_WRAP;
    }
    return -1;
}

/* 6901f4a0 -- hwArrayPointerChanged: a vertex array pointer changed: the array cache is stale */
void __fastcall hwArrayPointerChanged(__GLcontext *gc)
{
    if (!(gc->vertexArray.flags & __GL_VERTARRAY_LOCKED)) {
        gc->vertexArray.flags |= __GL_VERTARRAY_CACHE_STALE;
    }
    gc->vertexArray.flags |= __GL_VERTARRAY_CHANGED;
}

/* 6901fd30 -- hwValidateStencil: load the GL stencil state into the Glide stencil records (32 bpp only) and switch
** Glide's stencil test with GL_STENCIL_TEST */
void __fastcall hwValidateStencil(__GLcontext *gc)
{
    GLint grFail, grDepthFail, grDepthPass;
    GLuint func;

    if (gc->hwcx.has32bpp) {
        grFail = hwStencilOpToGlide(gc->state.stencil.fail);
        grDepthFail = hwStencilOpToGlide(gc->state.stencil.depthFail);
        grDepthPass = hwStencilOpToGlide(gc->state.stencil.depthPass);
        GRS_SET(__GL_GR_STENCIL_OP, 0, grFail);
        GRS_SET(__GL_GR_STENCIL_OP, 1, grDepthFail);
        GRS_SET(__GL_GR_STENCIL_OP, 2, grDepthPass);
        func = gc->state.stencil.testFunc & 7;              /* GL_NEVER.. -> GR_CMP_NEVER.. */
        GRS_SET(__GL_GR_STENCIL_FUNC, 0, (GLint)func);
        GRS_SETB(__GL_GR_STENCIL_FUNC, 4, (char)gc->state.stencil.reference);
        GRS_SETB(__GL_GR_STENCIL_FUNC, 5, (char)gc->state.stencil.mask);
        GRS_SETB(__GL_GR_STENCIL_MASK, 0, (char)gc->state.stencil.writeMask);
        if (gc->state.enables.general & __GL_STENCIL_TEST_ENABLE) {
            grEnable(GR_STENCIL_MODE_EXT);
            return;
        }
        grDisable(GR_STENCIL_MODE_EXT);
    }
}

/* 6901fe70 -- __glsstim_PointParameterfEXT: __glim_PointParameterfEXT, hardware path (no attenuation pname here) */
void __stdcall __glsstim_PointParameterfEXT(GLenum pname, GLfloat param)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (pname >= GL_POINT_SIZE_MIN_EXT_W && pname <= GL_POINT_FADE_THRESHOLD_SIZE_EXT_W) {
        switch (pname) {
          case GL_POINT_FADE_THRESHOLD_SIZE_EXT_W:
            gc->state.point.ext[2] = param;
            break;
          case GL_POINT_SIZE_MAX_EXT_W:
            gc->state.point.ext[1] = param;
            break;
          case GL_POINT_SIZE_MIN_EXT_W:
            gc->state.point.ext[0] = param;
            break;
        }
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POINT);
        return;
    }
    __glSetError(GL_INVALID_ENUM);
}

/* the Glide stencil record grState[__GL_GR_STENCIL_FUNC] (grStencilFuncExt) */
typedef struct __GLGrStencilFuncRec {
    GLuint      func;                   /* +0x00  GR_CMP_* (= GL func & 7) */
    GLubyte     ref;                    /* +0x04 */
    GLubyte     mask;                   /* +0x05 */
} __GLGrStencilFunc;
/* ---- stencil fetch from the 32-bit depth / stencil buffer ---- */
#define DEPTH_ADDR(gc, buf, x, y) \
    ((GLubyte *)((((y) - (gc)->constants.viewportYAdjust) * (buf)->outerWidth \
		  - (gc)->constants.viewportXAdjust + (x)) * (buf)->elementSize + (GLint)(buf)->base))
#define MALLOC(gc, n) \
    ((void *(__fastcall *)(__GLcontext *, size_t)) (gc)->imports[0])(gc, n)
#define GL_INCR_WRAP_EXT    0x8507
#define GL_DECR_WRAP_EXT    0x8508
extern void __stdcall __glsstim_StencilFunc(GLenum func, GLint ref, GLuint mask);
extern void __fastcall buildStencilTables(__GLcontext *gc);

/* 6901d9b0 -- __glsstim_StencilFunc of the hardware path: the SI state (the reference clamped to and the mask cut
** to the stencil bits) and, with the PIXEXT stencil (32 bpp), the Glide stencil record */
void __stdcall __glsstim_StencilFunc(GLenum func, GLint ref, GLuint mask)
{
    __GLGrStencilFunc *gr;
    __GL_SETUP_NOT_IN_BEGIN();

    if (func < GL_NEVER || func > GL_ALWAYS) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    if (ref < 0) {
        ref = 0;
    }
    if (ref > (1 << gc->modes.stencilBits) - 1) {
        ref = (1 << gc->modes.stencilBits) - 1;
    }
    gc->state.stencil.testFunc = func;
    gc->state.stencil.reference = (GLshort)ref;
    gc->state.stencil.mask = (GLshort)(((1 << gc->modes.stencilBits) - 1) & mask);
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_GENERIC);
    gc->validateMask |= 2;
    if (gc->hwcx.has32bpp != 0) {
        if (*gc->hwcx.grState[__GL_GR_STENCIL_FUNC] != (GLint)(func & 7)) {
            *gc->hwcx.grState[__GL_GR_STENCIL_FUNC] = func & 7;
            gc->hwcx.grDirty |= 1 << __GL_GR_STENCIL_FUNC;
        }
        gr = (__GLGrStencilFunc *)gc->hwcx.grState[__GL_GR_STENCIL_FUNC];
        if (gr->ref != (GLubyte)ref) {
            gr->ref = (GLubyte)ref;
            gc->hwcx.grDirty |= 1 << __GL_GR_STENCIL_FUNC;
        }
        gr = (__GLGrStencilFunc *)gc->hwcx.grState[__GL_GR_STENCIL_FUNC];
        if (gr->mask != (GLubyte)mask) {
            gr->mask = (GLubyte)mask;
            gc->hwcx.grDirty |= 1 << __GL_GR_STENCIL_FUNC;
        }
    }
}

/* 6904a600 -- Stencil32_Fetch: stencil fetch: the top byte of the 32-bit depth / stencil word (the depth fetch
** 690c5480 with the shift) */
GLuint __fastcall Stencil32_Fetch(__GLstencilBufferICD *sfb, GLint x, GLint y)
{
    __GLcontext *gc = sfb->buf.gc;
    __WGLbuffer *buf = sfb->buf.wglBuf;
    return *(GLuint *)DEPTH_ADDR(gc, buf, x, y) >> 24;
}

/* 6904a640 -- Stencil32_Store: stencil store into the top byte of the 32-bit depth / stencil word, through the
** stencil write mask */
void __fastcall Stencil32_Store(__GLstencilBufferICD *sfb, GLint x, GLint y, GLint v)
{
    __GLcontext *gc = sfb->buf.gc;
    __WGLbuffer *buf = sfb->buf.wglBuf;
    GLuint *p = (GLuint *)DEPTH_ADDR(gc, buf, x, y);
    GLint writeMask = gc->state.stencil.writeMask;

    *p = (*p & ((~writeMask << 24) | 0xffffff)) | ((writeMask & v) << 24);
}

extern void __fastcall sstSten_buildOpTable(__GLcontext *gc, GLubyte *tp, GLenum op, GLubyte reference, GLubyte writeMask);
/* 6904a880 -- buildStencilTables: the stencil tables (1 << stencilBits entries) */
void __fastcall buildStencilTables(__GLcontext *gc)
{
    GLint i;
    GLubyte reference, maskedRef, mask, writeMask;
    GLenum testFunc;
    GLboolean *tp;

    mask = (GLubyte) gc->state.stencil.mask;
    reference = (GLubyte) gc->state.stencil.reference;
    maskedRef = ((GLubyte) gc->state.stencil.reference) & mask;
    testFunc = gc->state.stencil.testFunc;

    tp = &gc->stencilBuffer.testFuncTable[0];
    if( tp == NULL ) {
        gc->stencilBuffer.testFuncTable = tp = (GLboolean *)
            MALLOC(gc, sizeof(GLboolean) * (1 << gc->modes.stencilBits));
        gc->stencilBuffer.failOpTable = (GLubyte *)
            MALLOC(gc, sizeof(GLubyte) * (1 << gc->modes.stencilBits));
        gc->stencilBuffer.depthFailOpTable = (GLubyte *)
            MALLOC(gc, sizeof(GLubyte) * (1 << gc->modes.stencilBits));
        gc->stencilBuffer.depthPassOpTable = (GLubyte *)
            MALLOC(gc, sizeof(GLubyte) * (1 << gc->modes.stencilBits));
    }

    for (i = 0; i < (1 << gc->modes.stencilBits); i++) {
        switch (testFunc) {
          case GL_NEVER:        *tp++ = GL_FALSE; break;
          case GL_LESS:         *tp++ = maskedRef < (i & mask); break;
          case GL_EQUAL:        *tp++ = maskedRef == (i & mask); break;
          case GL_LEQUAL:       *tp++ = maskedRef <= (i & mask); break;
          case GL_GREATER:      *tp++ = maskedRef > (i & mask); break;
          case GL_NOTEQUAL:     *tp++ = maskedRef != (i & mask); break;
          case GL_GEQUAL:       *tp++ = maskedRef >= (i & mask); break;
          case GL_ALWAYS:       *tp++ = GL_TRUE; break;
        }
    }

    writeMask = (GLubyte) gc->state.stencil.writeMask;
    sstSten_buildOpTable(gc, &gc->stencilBuffer.failOpTable[0],
                 gc->state.stencil.fail, reference, writeMask);
    sstSten_buildOpTable(gc, &gc->stencilBuffer.depthFailOpTable[0],
                 gc->state.stencil.depthFail, reference, writeMask);
    sstSten_buildOpTable(gc, &gc->stencilBuffer.depthPassOpTable[0],
                 gc->state.stencil.depthPass, reference, writeMask);
}

/* 6904aa80 -- sstSten_buildOpTable: buildOpTable (copy of 6909a7b0), with the range from the visual's stencil bits
** and the wrapping ops */
void __fastcall sstSten_buildOpTable(__GLcontext *gc, GLubyte *tp, GLenum op,
                             GLubyte reference, GLubyte writeMask)
{
    GLuint i;
    GLubyte newValue;
    GLubyte notWriteMask = ~writeMask;

    for (i = 0; i < (GLuint)(1 << gc->modes.stencilBits); i++) {
        switch (op) {
          case GL_KEEP:         newValue = i; break;
          case GL_ZERO:         newValue = 0; break;
          case GL_REPLACE:      newValue = reference; break;
          case GL_INVERT:       newValue = ~i; break;
          case GL_INCR:
            /* Clamp so no overflow occurs */
            if (i == (GLuint)((1 << gc->modes.stencilBits) - 1)) {
                newValue = i;
            } else {
                newValue = i + 1;
            }
            break;
          case GL_DECR:
            /* Clamp so no underflow occurs */
            if (i == 0) {
                newValue = 0;
            } else {
                newValue = i - 1;
            }
            break;
          case GL_INCR_WRAP_EXT:
            newValue = (i + 1) % (GLuint)(1 << gc->modes.stencilBits);
            break;
          case GL_DECR_WRAP_EXT:
            if (i == 0) {
                newValue = (1 << gc->modes.stencilBits) - 1;
            } else {
                newValue = i - 1;
            }
            break;
        }
        *tp++ = (i & notWriteMask) | (newValue & writeMask);
    }
}

/* 6904a870 -- __glSSTValidateStencil: __glValidateStencil: tail-jumps to the body (6904a880, its own function in
** the original) when validateMask has bit 1 or 2 set */
void __fastcall __glSSTValidateStencil(__GLcontext *gc)
{
    if (gc->validateMask & 6) buildStencilTables(gc);
}

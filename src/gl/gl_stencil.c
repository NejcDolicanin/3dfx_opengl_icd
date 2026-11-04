/*
** gl_stencil.c -- original VA range 69098000..6909c000: s_sten.c, the 8-bit stencil buffer procs
** (init, test, the three ops, free, pick), and the empty accumulation-buffer pick (s_accum.c).
** The stencil cell is the top byte of the WGL buffer element (see sten_Fetch).
** Then the rest of s_sten.c, in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "icdprotos.h"
#include <glide.h>
#include "glsi.h"
#include "glsitypes.h"
#include "glshade.h"
#include "si_names.h"

#define __GL_VALIDATE_STENCIL_FUNC  0x00000002
#define __GL_VALIDATE_STENCIL_OP    0x00000004

/* the stencil cell of (x, y): the last byte of the WGL buffer element */
#define __GL_STENCIL_ADDR_980(gc, db, x, y) \
    ((GLubyte *)(db)->base \
     + (((y) - (gc)->constants.viewportYAdjust) * (db)->outerWidth \
        - (gc)->constants.viewportXAdjust + 1 + (x)) * (db)->elementSize - 1)

typedef void (__fastcall *__GLfreeProc980)(__GLcontext *gc, void *p);

extern void __fastcall __glValidateStencil(__GLcontext *gc);
extern GLuint __fastcall sten_Fetch(__GLstencilBufferICD *sfb, GLint x, GLint y);           /* Fetch */
extern void __fastcall sten_Store(__GLstencilBufferICD *sfb, GLint x, GLint y, GLubyte v); /* Store */
extern void __fastcall sten_Clear(__GLstencilBufferICD *sfb);         /* Clear */

GLboolean __fastcall TestFunc(__GLstencilBufferICD *sfb, GLint x, GLint y);
void __fastcall FailOp(__GLstencilBufferICD *sfb, GLint x, GLint y);
void __fastcall PassDepthFailOp(__GLstencilBufferICD *sfb, GLint x, GLint y);
void __fastcall DepthPassOp(__GLstencilBufferICD *sfb, GLint x, GLint y);
void __fastcall FreeStencilBuf(__GLstencilBufferICD *sfb, __GLcontext *gc);
void __fastcall sten_Pick(__GLcontext *gc);

/* 6909a8a0 -- __glInitStencil8 (s_sten.c): gc is not passed on (it stays in EDX) */
void __fastcall __glInitStencil8(__GLstencilBufferICD *sfb)
{
    __glInitBuffer(&sfb->buf);

    sfb->buf.freeBuf = (__GLproc)FreeStencilBuf;

    sfb->pick = (__GLproc)sten_Pick;
    sfb->store = (__GLproc)sten_Store;
    sfb->fetch = (__GLproc)sten_Fetch;
    sfb->testFunc = TestFunc;
    sfb->failOp = (__GLproc)FailOp;
    sfb->passDepthFailOp = (__GLproc)PassDepthFailOp;
    sfb->depthPassOp = (__GLproc)DepthPassOp;
    sfb->clear = (__GLproc)sten_Clear;
}

/* 6909a980 -- TestFunc (s_sten.c): the stencil test of one pixel through testFuncTable */
GLboolean __fastcall TestFunc(__GLstencilBufferICD *sfb, GLint x, GLint y)
{
    __GLcontext *gc = sfb->buf.gc;
    __WGLbuffer *db = sfb->buf.wglBuf;
    GLubyte *fb;

    fb = __GL_STENCIL_ADDR_980(gc, db, x, y);
    return sfb->testFuncTable[fb[0] & gc->state.stencil.mask];
}

/* 6909a9d0 -- FailOp (s_sten.c): replace the stencil byte at (x, y) by its failOpTable entry */
void __fastcall FailOp(__GLstencilBufferICD *sfb, GLint x, GLint y)
{
    __WGLbuffer *db = sfb->buf.wglBuf;
    GLubyte *fb;

    fb = __GL_STENCIL_ADDR_980(sfb->buf.gc, db, x, y);
    fb[0] = sfb->failOpTable[fb[0]];
}

/* 6909aa20 -- PassDepthFailOp (s_sten.c): replace the stencil byte at (x, y) by its depthFailOpTable entry */
void __fastcall PassDepthFailOp(__GLstencilBufferICD *sfb, GLint x, GLint y)
{
    __WGLbuffer *db = sfb->buf.wglBuf;
    GLubyte *fb;

    fb = __GL_STENCIL_ADDR_980(sfb->buf.gc, db, x, y);
    fb[0] = sfb->depthFailOpTable[fb[0]];
}

/* 6909aa70 -- DepthPassOp (s_sten.c): replace the stencil byte at (x, y) by its depthPassOpTable entry */
void __fastcall DepthPassOp(__GLstencilBufferICD *sfb, GLint x, GLint y)
{
    __WGLbuffer *db = sfb->buf.wglBuf;
    GLubyte *fb;

    fb = __GL_STENCIL_ADDR_980(sfb->buf.gc, db, x, y);
    fb[0] = sfb->depthPassOpTable[fb[0]];
}

/* 6909acd0 -- FreeStencilBuf (s_sten.c): free the four lookup tables (private to the context) */
void __fastcall FreeStencilBuf(__GLstencilBufferICD *sfb, __GLcontext *gc)
{
    (*(__GLfreeProc980)gc->imports[3])(gc, sfb->testFuncTable);
    (*(__GLfreeProc980)gc->imports[3])(gc, sfb->failOpTable);
    (*(__GLfreeProc980)gc->imports[3])(gc, sfb->depthFailOpTable);
    (*(__GLfreeProc980)gc->imports[3])(gc, sfb->depthPassOpTable);

    sfb->testFuncTable = NULL;
    sfb->failOpTable = NULL;
    sfb->depthFailOpTable = NULL;
    sfb->depthPassOpTable = NULL;
}

/* 6909ad10 -- sten_Pick: Pick (s_sten.c, stencil buffer), revalidate the tables after glStencilFunc / Op */
void __fastcall sten_Pick(__GLcontext *gc)
{
    if (gc->validateMask & (__GL_VALIDATE_STENCIL_FUNC | __GL_VALIDATE_STENCIL_OP)) {
        __glValidateStencil(gc);
    }
}

/* 6909bb50 -- accum_Pick: Pick (s_accum.c, accumulation buffer), nothing to do */
void __fastcall accum_Pick()
{
}

#define MALLOC(gc, n) \
    ((void *(__fastcall *)(__GLcontext *, size_t)) (gc)->imports[0])(gc, n)
/************************************************************************/
/* s_sten.c */
#define GL_INCR_WRAP_EXT    0x8507
#define GL_DECR_WRAP_EXT    0x8508

extern void __fastcall buildOpTable(__GLcontext *gc, GLubyte *tp, GLenum op, GLubyte reference, GLubyte writeMask);
/* 6909a5b0 -- __glValidateStencil (tables of 1 << stencilBits entries) */
void __fastcall __glValidateStencil(__GLcontext *gc)
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
    buildOpTable(gc, &gc->stencilBuffer.failOpTable[0],
                 gc->state.stencil.fail, reference, writeMask);
    buildOpTable(gc, &gc->stencilBuffer.depthFailOpTable[0],
                 gc->state.stencil.depthFail, reference, writeMask);
    buildOpTable(gc, &gc->stencilBuffer.depthPassOpTable[0],
                 gc->state.stencil.depthPass, reference, writeMask);
}

/* 6909a7b0 -- buildOpTable, with the range from the visual's stencil bits and the wrapping ops */
void __fastcall buildOpTable(__GLcontext *gc, GLubyte *tp, GLenum op,
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

/* ---- s_stencil.c -------------------------------------------------------------------------- */
/* 6909a8f0 -- sten_Fetch: Fetch (s_sten.c), the stencil cell is the top byte of the WGL buffer element */
GLuint __fastcall sten_Fetch(__GLstencilBufferICD *sfb, GLint x, GLint y)
{
    __GLcontext *gc = sfb->buf.gc;
    __WGLbuffer *db = sfb->buf.wglBuf;
    GLubyte *fp;

    fp = (GLubyte *)db->base
	+ ((y - gc->constants.viewportYAdjust) * db->outerWidth
	   - gc->constants.viewportXAdjust + x + 1) * db->elementSize - 1;
    return fp[0];
}

/************************************************************************/
/* s_stenc.c */
/* 6909a930 -- sten_Store: Store (s_sten.c, stencil buffer), the masked write of one cell */
void __fastcall sten_Store(__GLstencilBufferICD *sfb, GLint x, GLint y, GLubyte v)
{
    __GLcontext *gc = sfb->buf.gc;
    __WGLbuffer *db = sfb->buf.wglBuf;
    GLubyte *fb;
    GLubyte wm;

    fb = (GLubyte *)db->base
    + ((y - gc->constants.viewportYAdjust) * db->outerWidth
       - gc->constants.viewportXAdjust + x + 1) * db->elementSize - 1;
    wm = (GLubyte)gc->state.stencil.writeMask;
    *fb = (~wm & *fb) | (wm & v);
}

/************************************************************************/
typedef GLubyte __GLstencilCell;
#define LOCK_BUFFERS(gc, m)   (((void (__fastcall *)(__GLcontext *, GLuint))(gc)->buffers.lockBuffers)(gc, m))
#define UNLOCK_BUFFERS(gc, m) (((void (__fastcall *)(__GLcontext *, GLuint))(gc)->buffers.unlockBuffers)(gc, m))

/* 6909aac0 -- sten_Clear: Clear (s_sten.c) on the WGL stencil buffer (the top byte of each element) */
void __fastcall sten_Clear(__GLstencilBufferICD *sfb)
{
    __GLcontext *gc = sfb->buf.gc;
    __WGLbuffer *db;
    __GLstencilCell *fb;
    GLint x, y, x1, y1, skip, width, w8, w1;
    __GLstencilCell sten;   /* (the tail loop below is the SI byte loop: VC6 turns it into rep stos itself) */

    x = gc->transform.clipX0;
    y = gc->transform.clipY0;
    x1 = gc->transform.clipX1;
    y1 = gc->transform.clipY1;
    sten = (__GLstencilCell)gc->state.stencil.clear;
    if (((width = x1 - x) == 0) || (y1 - y == 0)) {
	return;
    }

    LOCK_BUFFERS(gc, __GL_STENCIL_BUFFER_MASK);

    db = sfb->buf.wglBuf;

    fb = (__GLstencilCell *)((GLubyte *)db->base
	+ ((y - gc->constants.viewportYAdjust) * db->outerWidth
	   - gc->constants.viewportXAdjust + x + 1) * db->elementSize - 1);

    skip = (sfb->buf.wglBuf)->outerWidth - width;
    w8 = width >> 3;
    w1 = width & 7;
    if (gc->state.stencil.writeMask == (1 << gc->modes.stencilBits) - 1) {
	for (; y < y1; y++) {
	    width = w8;
	    while (--width >= 0) {
		fb[0] = sten; fb[1] = sten; fb[2] = sten; fb[3] = sten;
		fb[4] = sten; fb[5] = sten; fb[6] = sten; fb[7] = sten;
		fb += 8;
	    }
	    width = w1;
	    while (--width >= 0) {
		*fb++ = sten;
	    }
	    fb += skip;
	}
    } else {
	GLint mask;

	mask = gc->state.stencil.writeMask;
	sten = sten & mask;
	mask = ~mask;

	for (; y < y1; y++) {
	    width = w8;
	    while (--width >= 0) {
		fb[0] = (fb[0] & mask) | (sten);
		fb[1] = (fb[1] & mask) | (sten);
		fb[2] = (fb[2] & mask) | (sten);
		fb[3] = (fb[3] & mask) | (sten);
		fb[4] = (fb[4] & mask) | (sten);
		fb[5] = (fb[5] & mask) | (sten);
		fb[6] = (fb[6] & mask) | (sten);
		fb[7] = (fb[7] & mask) | (sten);
		fb += 8;
	    }
	    width = w1;
	    while (--width >= 0) {
		fb[0] = (fb[0] & mask) | (sten);
		fb++;
	    }
	    fb += skip;
	}
    }

    UNLOCK_BUFFERS(gc, __GL_STENCIL_BUFFER_MASK);
}

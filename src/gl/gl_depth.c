/*
** gl_depth.c -- original VA range 690c4000..690c8000: s_depth.c, the trivial depth store procs
** (ALWAYS without write, NEVER), the depth buffer Resize and __glInitDepth16 / 24 / 32.
** The store procs of each depth come in the table at 690f5264 (16 per depth: NEVER .. ALWAYS
** writing, then NEVER .. ALWAYS not writing).
** Then the rest of s_depth.c, in address order: the 16- / 32-bit, masked and windowed store families, fetch,
** pick, clear, make current.
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
#include "og.h"

extern GLboolean __fastcall StoreALWAYS16(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z);
extern GLboolean __fastcall StoreALWAYS24(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z);  /* StoreALWAYS24 */
extern GLboolean __fastcall StoreALWAYS32(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z);  /* StoreALWAYS32 */
extern __GLzValue __fastcall depth_Fetch16(__GLdepthBufferICD *fb, GLint x, GLint y);   /* Fetch16 */
extern void __fastcall depth_Clear(__GLdepthBufferICD *fb);                         /* Clear */
extern void __fastcall depth_Pick(__GLcontext *gc, __GLdepthBufferICD *fb, GLint depthIndex);   /* Pick */
extern void __fastcall depth_MakeCurrent(__GLbufferICD *buf, __WGLbuffer *drawableBuf, __GLcontext *gc);   /* s_depth.c MakeCurrent (gl_depth.c) */
extern __GLzValue __fastcall FetchFastClear(__GLdepthBufferICD *fb, GLint x, GLint y);
extern void __fastcall Fix_zbuffer(__GLcontext *gc, __GLdepthBufferICD *dfb, GLuint destRange);
extern __GLzValue __fastcall depth_Fetch(__GLdepthBufferICD *fb, GLint x, GLint y);   /* Fetch */
/* 3dfx declared the 16-bit fill with a GLushort value here; its definition (gl_memclr.c) takes a GLuint whose
** low half is the pixel, and these callers pass the value unmasked: kept local so both sides compile as the original */
extern void __fastcall __glFillRect16(__GLcontext *gc, GLushort *dst, GLint w, GLint h, GLint skip, GLushort value);

GLboolean __fastcall depth_Resize(__GLbufferICD *fb, GLint x, GLint y, GLint w, GLint h, GLuint bufferMask);

/* 690c4200 -- StoreALWAYS16_W (s_depth.c): depth writes off, the test always passes */
GLboolean __fastcall StoreALWAYS16_W(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z)
{
    return GL_TRUE;
}

/* 690c4700 -- StoreALWAYS24_W (the ICD's 24-bit set) */
GLboolean __fastcall StoreALWAYS24_W(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z)
{
    return GL_TRUE;
}

/* 690c4710 -- StoreNEVER32 (s_depth.c; the 32-bit set's NEVER and NEVER_W) */
GLboolean __fastcall StoreNEVER32(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z)
{
    return GL_FALSE;
}

/* 690c4b90 -- StoreALWAYS_W (s_depth.c): the 32-bit set's ALWAYS with depth writes off: passes, stores nothing */
GLboolean __fastcall StoreALWAYS_W(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z)
{
    return GL_TRUE;
}

/* 690c4d10 -- __glInitDepth16 (s_depth.c).  The ICD's minResolution is 2^24 (SI: 0x1000), and gc
** is not passed to __glInitBuffer.
*/
void __fastcall __glInitDepth16(__GLdepthBufferICD *fb)
{
    __glInitBuffer(&fb->buf);

    fb->buf.resize = (__GLproc)depth_Resize;
    fb->buf.makeCurrent = (__GLproc)depth_MakeCurrent;

    fb->pick = (__GLproc)depth_Pick;
    fb->clear = (__GLproc)depth_Clear;
    fb->store2 = (__GLproc)StoreALWAYS16;
    fb->fetch = (__GLproc)depth_Fetch16;

    fb->scale = 0xffff0000;
    fb->testFunc = GL_LESS;
    fb->fixZBuffer = (__GLproc)NopSpanRender;
    fb->writeMask = ~0;
    fb->minResolution = 16777216.0f;
}

/* 690c50f0 -- depth_Resize: Resize (s_depth.c, depth buffer), the next clear must clear the whole buffer */
GLboolean __fastcall depth_Resize(__GLbufferICD *fb, GLint x, GLint y, GLint w, GLint h, GLuint bufferMask)
{
    (*fb->gc->drawablePrivate->lockDP)(fb->gc->drawablePrivate, fb->gc);
    ((__WGLdepthPrivate *)(fb->wglBuf)->depthPrivate)->fullZClear = GL_TRUE;
    (*fb->gc->drawablePrivate->unlockDP)(fb->gc->drawablePrivate);

    return GL_TRUE;
}

/* 690c5210 -- __glInitDepth24 (s_depth.c): fast-clear fetch and Fix_zbuffer */
void __fastcall __glInitDepth24(__GLdepthBufferICD *fb)
{
    __glInitBuffer(&fb->buf);

    fb->buf.resize = (__GLproc)depth_Resize;
    fb->buf.makeCurrent = (__GLproc)depth_MakeCurrent;

    fb->pick = (__GLproc)depth_Pick;
    fb->clear = (__GLproc)depth_Clear;
    fb->store2 = (__GLproc)StoreALWAYS24;
    fb->fetch = (__GLproc)FetchFastClear;

    fb->scale = 0x7fffff80;
    fb->testFunc = GL_LESS;
    fb->fixZBuffer = (__GLproc)Fix_zbuffer;
    fb->writeMask = ~0;
    fb->minResolution = 16777216.0f;
}

/* 690c5420 -- __glInitDepth32 (s_depth.c): the ICD's scale is the full 32 bits (SI: 0x7fffff80) */
void __fastcall __glInitDepth32(__GLdepthBufferICD *fb)
{
    __glInitBuffer(&fb->buf);

    fb->buf.resize = (__GLproc)depth_Resize;
    fb->buf.makeCurrent = (__GLproc)depth_MakeCurrent;

    fb->pick = (__GLproc)depth_Pick;
    fb->clear = (__GLproc)depth_Clear;
    fb->store2 = (__GLproc)StoreALWAYS32;
    fb->fetch = (__GLproc)depth_Fetch;

    fb->scale = 0xffffffff;
    fb->testFunc = GL_LESS;
    fb->fixZBuffer = (__GLproc)NopSpanRender;
    fb->writeMask = ~0;
    fb->minResolution = 16777216.0f;
}

/* 690c54b0 -- __glInitDepth (s_depth.c): pick the 16, 24 or 32-bit depth procs by modes.depthBits */
void __fastcall __glInitDepth(__GLdepthBufferICD *fb, __GLcontext *gc)
{
    if (gc->modes.depthBits <= 16) {
        __glInitDepth16(fb);
    } else if (gc->modes.depthBits <= 24) {
        __glInitDepth24(fb);
    } else {
        __glInitDepth32(fb);
    }
}

/************************************************************************/
/* s_depth.c */

/* the WGL depth buffer's private block: __WGLdepthPrivate (wgldraw.h) */
typedef GLushort __GLzValue16;
#define DEPTH_ADDR(gc, db, cast, x, y) \
    (cast ((GLubyte *)(db)->base \
	+ (((y) - (gc)->constants.viewportYAdjust) * (db)->outerWidth \
	   - (gc)->constants.viewportXAdjust + (x)) * (db)->elementSize))
/* ---- 16-bit, write enabled ---- */
#define STORE16(name, op) \
GLboolean __fastcall name(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z) \
{ \
    __GLzValue16 *fp; \
    __GLcontext *gc = fb->buf.gc; \
    __WGLbuffer *drawableBuf = fb->buf.wglBuf; \
    __WGLdepthPrivate *dbDepth = (__WGLdepthPrivate *)drawableBuf->depthPrivate; \
    __GLzValue16 Z = (z >> dbDepth->numFracBits) + dbDepth->cnt; \
 \
    fp = DEPTH_ADDR(gc, drawableBuf, (__GLzValue16 *), x, y); \
    if (Z op fp[0]) { \
	fp[0] = Z; \
	return GL_TRUE; \
    } \
    return GL_FALSE; \
}
/************************************************************************/
/* WGL depth buffer: StoreALWAYS24 procs */

/* the WGL depth buffer's private word (buf.other + 0x48): the ICD's depth-clear state */
#define __GL_DEPTH_BYTE_ADDR(gc, buf, x, y) \
    ((GLubyte *)((((y) - (gc)->constants.viewportYAdjust) * (buf)->outerWidth \
		  + (x) - (gc)->constants.viewportXAdjust) * (buf)->elementSize + (GLint)(buf)->base))
/* ---- 16-bit, write disabled ---- */
#define STORE16_W(name, op) \
GLboolean __fastcall name(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z) \
{ \
    __GLzValue16 *fp; \
    __GLcontext *gc = fb->buf.gc; \
    __WGLbuffer *drawableBuf = fb->buf.wglBuf; \
    __WGLdepthPrivate *dbDepth = (__WGLdepthPrivate *)drawableBuf->depthPrivate; \
    __GLzValue16 Z = (z >> dbDepth->numFracBits) + dbDepth->cnt; \
 \
    fp = DEPTH_ADDR(gc, drawableBuf, (__GLzValue16 *), x, y); \
    return (Z op fp[0]); \
}
/* ---- 32-bit masked (depth shares the word with stencil), write enabled ---- */
#define STOREM(name, op) \
GLboolean __fastcall name(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z) \
{ \
    __GLzValue *fp; \
    __GLcontext *gc = fb->buf.gc; \
    __WGLbuffer *drawableBuf = fb->buf.wglBuf; \
    __WGLdepthPrivate *dbDepth = (__WGLdepthPrivate *)drawableBuf->depthPrivate; \
    __GLzValue Z = (z >> dbDepth->numFracBits) + dbDepth->cnt; \
 \
    fp = DEPTH_ADDR(gc, drawableBuf, (__GLzValue *), x, y); \
    if (Z op (fp[0] & dbDepth->zDepthMask)) { \
	fp[0] = (fp[0] & ~dbDepth->zDepthMask) | (Z & dbDepth->zDepthMask); \
	return GL_TRUE; \
    } \
    return GL_FALSE; \
}
/* ---- 32-bit masked, write disabled ---- */
#define STOREM_W(name, op) \
GLboolean __fastcall name(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z) \
{ \
    __GLzValue *fp; \
    __GLcontext *gc = fb->buf.gc; \
    __WGLbuffer *drawableBuf = fb->buf.wglBuf; \
    __WGLdepthPrivate *dbDepth = (__WGLdepthPrivate *)drawableBuf->depthPrivate; \
 \
    fp = DEPTH_ADDR(gc, drawableBuf, (__GLzValue *), x, y); \
    return (((z >> dbDepth->numFracBits) + dbDepth->cnt) op (fp[0] & dbDepth->zDepthMask)); \
}
/* ---- 32-bit, write enabled ---- */
#define STORE(name, op) \
GLboolean __fastcall name(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z) \
{ \
    __GLzValue *fp; \
    __GLcontext *gc = fb->buf.gc; \
    __WGLbuffer *drawableBuf = fb->buf.wglBuf; \
    __WGLdepthPrivate *dbDepth = (__WGLdepthPrivate *)drawableBuf->depthPrivate; \
    __GLzValue Z = (z >> dbDepth->numFracBits) + dbDepth->cnt; \
 \
    fp = DEPTH_ADDR(gc, drawableBuf, (__GLzValue *), x, y); \
    if ((GLuint)Z op (GLuint)fp[0]) { \
	fp[0] = Z; \
	return GL_TRUE; \
    } \
    return GL_FALSE; \
}
#define __GL_DEPTH_TEST(name, op)                                          \
GLboolean __fastcall name(__GLdepthBufferICD *dfb, GLint x, GLint y, __GLzValue z) \
{                                                                              \
    __GLcontext *gc = dfb->buf.gc;                                             \
    __WGLbuffer *buf = dfb->buf.wglBuf;                          \
    __WGLdepthPrivate *dc = (__WGLdepthPrivate *)buf->depthPrivate;               \
    GLuint *zp = (GLuint *)__GL_DEPTH_BYTE_ADDR(gc, buf, x, y);                 \
    return (z >> dc->numFracBits) + dc->cnt op *zp;                                  \
}
/* ---- s_depth.c: the depth clear by range flip (__GLdrawableDepthBuffer at WGL buffer +0x48) ---- */
#define ZTRICK(fb)  ((__WGLdepthPrivate *)(fb)->buf.wglBuf->depthPrivate)
typedef void (__fastcall *__GLlockBuffersProc)(__GLcontext *gc, GLuint mask);
static __GLproc g_depthStoreProcs[49];     /* the depth store procs by depthIndex (690f5264; at the end of the file) */
extern GLuint __fastcall DepthClearVal(__GLdepthBufferICD *dfb);
extern void __fastcall depth_FreePrivate(__WGLbuffer *drawableBuf, __WGLdrawable *dp);

/* 69079300 -- LoseCurrent (3dfx S_EXPORT.C): illegal in selection / feedback mode and inside
** Begin/End; hands the begin mode back to the context and clears the thread's current gc */
GLboolean __fastcall LoseCurrent(__GLcontext *gc)
{
    if (gc->renderMode == GL_RENDER && __gl_beginMode != __GL_IN_BEGIN) {
	__glLoseCurrentBuffers(gc);
	gc->beginMode = __gl_beginMode;
	__glSetCurrentGC(0);
	return GL_TRUE;
    }
    return GL_FALSE;
}

STORE16(StoreLESS16, <)

STORE16(StoreEQUAL16, ==)

STORE16(StoreLEQUAL16, <=)

STORE16(StoreGREATER16, >)

STORE16(StoreNOTEQUAL16, !=)

STORE16(StoreGEQUAL16, >=)

/* 690c3fd0 -- StoreALWAYS16: GL_ALWAYS with depth writes on, 16-bit buffer: store (z >> numFracBits) + the
** clear count cnt at (x, y) unconditionally and pass */
GLboolean __fastcall StoreALWAYS16(__GLdepthBufferICD *dfb, GLint x, GLint y, __GLzValue z)
{
    __GLcontext *gc = dfb->buf.gc;
    __WGLbuffer *buf = dfb->buf.wglBuf;
    __WGLdepthPrivate *dc = (__WGLdepthPrivate *)buf->depthPrivate;
    GLushort *zp = (GLushort *)__GL_DEPTH_BYTE_ADDR(gc, buf, x, y);
    *zp = (GLushort)((z >> dc->numFracBits) + dc->cnt);
    return GL_TRUE;
}

STORE16_W(StoreLESS16_W, <)

STORE16_W(StoreEQUAL16_W, ==)

STORE16_W(StoreLEQUAL16_W, <=)

STORE16_W(StoreGREATER16_W, >)

STORE16_W(StoreNOTEQUAL16_W, !=)

STORE16_W(StoreGEQUAL16_W, >=)

STOREM(StoreLESS24, <)     /* StoreLESS (masked) */

STOREM(StoreEQUAL24, ==)    /* StoreEQUAL (masked) */

STOREM(StoreLEQUAL24, <=)    /* StoreLEQUAL (masked) */

STOREM(StoreGREATER24, >)     /* StoreGREATER (masked) */

STOREM(StoreNOTEQUAL24, !=)    /* StoreNOTEQUAL (masked) */

STOREM(StoreGEQUAL24, >=)    /* StoreGEQUAL (masked) */

/* 690c44c0 -- StoreALWAYS24 (masked): GL_ALWAYS with depth writes on, 24-bit depth in a 32-bit word shared with
** stencil: replace only the zDepthMask bits at (x, y) with (z >> numFracBits) + cnt and pass */
GLboolean __fastcall StoreALWAYS24(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z)
{
    __GLzValue *fp;
    __GLcontext *gc = fb->buf.gc;
    __WGLbuffer *drawableBuf = fb->buf.wglBuf;
    __WGLdepthPrivate *dbDepth = (__WGLdepthPrivate *)drawableBuf->depthPrivate;

    fp = DEPTH_ADDR(gc, drawableBuf, (__GLzValue *), x, y);
    fp[0] = (((z >> dbDepth->numFracBits) + dbDepth->cnt) & dbDepth->zDepthMask)
	| (~dbDepth->zDepthMask & fp[0]);
    return GL_TRUE;
}

STOREM_W(StoreLESS24_W, <)     /* StoreLESS_W (masked) */

STOREM_W(StoreEQUAL24_W, ==)    /* StoreEQUAL_W (masked) */

STOREM_W(StoreLEQUAL24_W, <=)    /* StoreLEQUAL_W (masked) */

STOREM_W(StoreGREATER24_W, >)     /* StoreGREATER_W (masked) */

STOREM_W(StoreNOTEQUAL24_W, !=)    /* StoreNOTEQUAL_W (masked) */

STOREM_W(StoreGEQUAL24_W, >=)    /* StoreGEQUAL_W (masked) */

STORE(StoreLESS32, <)      /* StoreLESS */

STORE(StoreEQUAL32, ==)     /* StoreEQUAL */

STORE(StoreLEQUAL32, <=)     /* StoreLEQUAL */

STORE(StoreGREATER32, >)      /* StoreGREATER */

STORE(StoreNOTEQUAL32, !=)     /* StoreNOTEQUAL */

STORE(StoreGEQUAL32, >=)     /* StoreGEQUAL */

/* 690c4960 -- StoreALWAYS32: GL_ALWAYS with depth writes on, 32-bit buffer: store (z >> numFracBits) + cnt at
** (x, y) unconditionally (whole word, no mask) and pass */
GLboolean __fastcall StoreALWAYS32(__GLdepthBufferICD *dfb, GLint x, GLint y, __GLzValue z)
{
    __GLcontext *gc = dfb->buf.gc;
    __WGLbuffer *buf = dfb->buf.wglBuf;
    __WGLdepthPrivate *dc = (__WGLdepthPrivate *)buf->depthPrivate;
    GLuint *zp = (GLuint *)__GL_DEPTH_BYTE_ADDR(gc, buf, x, y);
    *zp = (z >> dc->numFracBits) + dc->cnt;
    return GL_TRUE;
}

/* 690c49b0..690c4b40 -- DepthTest32_LESS: 32-bit depth test procs (z shifted down, plus the clear count) */
__GL_DEPTH_TEST(DepthTest32_LESS, <)

__GL_DEPTH_TEST(DepthTest32_EQUAL, ==)

__GL_DEPTH_TEST(DepthTest32_LEQUAL, <=)

__GL_DEPTH_TEST(DepthTest32_GREATER, >)

__GL_DEPTH_TEST(DepthTest32_NOTEQUAL, !=)

__GL_DEPTH_TEST(DepthTest32_GEQUAL, >=)

/* 690c4ba0 -- __glValidateZCount (s_depth.c, depthBuffer.fixZBuffer): flip the depth range (count
** and clear increment) when the counter reached its end, inverting the depth function to match.
** 6907fe60 is __glUpdateDepthRange.
*/
void __fastcall __glValidateZCount(__GLdepthBufferICD *fb)
{
    __GLcontext *gc = fb->buf.gc;
    __WGLdepthPrivate *zt = ZTRICK(fb);

    if (zt->zClearInc == 0xff000000) {
        if (zt->cnt != 0xff000000) return;
        zt->cnt = 0x80000000;
        zt->zClearInc = 0x01000000;
        switch (gc->state.depth.testFunc) {
        case GL_LESS:
            fb->testFunc = GL_GREATER;
            zt->invertZRange = GL_TRUE;
            break;
        case GL_LEQUAL:
            fb->testFunc = GL_GEQUAL;
            zt->invertZRange = GL_TRUE;
            break;
        case GL_GREATER:
            fb->testFunc = GL_GREATER;
            zt->invertZRange = GL_FALSE;
            break;
        case GL_GEQUAL:
            fb->testFunc = GL_GEQUAL;
            zt->invertZRange = GL_FALSE;
            break;
        default:
            fb->testFunc = gc->state.depth.testFunc;
            zt->invertZRange = GL_FALSE;
            break;
        }
        __glUpdateDepthRange(gc);
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
    } else if (zt->zClearInc == 0x01000000) {
        if (zt->cnt != 0) return;
        zt->cnt = 0x7f000000;
        zt->zClearInc = 0xff000000;
        switch (gc->state.depth.testFunc) {
        case GL_LESS:
            fb->testFunc = GL_LESS;
            zt->invertZRange = GL_FALSE;
            break;
        case GL_LEQUAL:
            fb->testFunc = GL_LEQUAL;
            zt->invertZRange = GL_FALSE;
            break;
        case GL_GREATER:
            fb->testFunc = GL_LESS;
            zt->invertZRange = GL_TRUE;
            break;
        case GL_GEQUAL:
            fb->testFunc = GL_LEQUAL;
            zt->invertZRange = GL_TRUE;
            break;
        default:
            fb->testFunc = gc->state.depth.testFunc;
            zt->invertZRange = GL_FALSE;
            break;
        }
        __glUpdateDepthRange(gc);
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
    } else {
        fb->testFunc = gc->state.depth.testFunc;
        zt->invertZRange = GL_FALSE;
    }
}

/* 690c4d70 -- depth_Fetch16: Fetch16 (3dfx S_DEPTH.C), 16-bit depth fetch */
__GLzValue __fastcall depth_Fetch16(__GLdepthBufferICD *dfb, GLint x, GLint y)
{
    __GLcontext *gc = dfb->buf.gc;
    __WGLbuffer *buf = dfb->buf.wglBuf;
    return *(GLushort *)__GL_DEPTH_BYTE_ADDR(gc, buf, x, y);
}

/* 690c4db0 -- depth_Clear: Clear (s_depth.c), depth buffer clear (a band of the buffer per frame when the
** count trick is on).
** The partial branch in 3dfx GLCORE/S_DEPTH.C Clear's shape: dx / dy recomputed (CSE'd with w / h,
** which is what lets h be computed in y1's register) and h reassigned to the band height. */
void __fastcall depth_Clear(__GLdepthBufferICD *dfb)
{
    __GLcontext *gc = dfb->buf.gc;
    __WGLbuffer *buf;
    __WGLdepthPrivate *dc;
    GLint x0, y0, x1, y1, w, h, ya, yb;
    GLuint val;
    GLubyte *fp;
    GLint skip;
    GLint dx, dy, np, ndp, indx, start_pix, end_pix;

    if (!gc->state.depth.writeEnable) {
	return;
    }
    (*gc->drawablePrivate->lockDP)(gc->drawablePrivate, gc);
    x0 = gc->transform.clipX0;
    y0 = gc->transform.clipY0;
    x1 = gc->transform.clipX1;
    y1 = gc->transform.clipY1;
    if ((w = x1 - x0) == 0 || (h = y1 - y0) == 0) {
	(*gc->drawablePrivate->unlockDP)(gc->drawablePrivate);
	return;
    }
    buf = dfb->buf.wglBuf;
    dc = (__WGLdepthPrivate *)buf->depthPrivate;
    if (!dc->fullZClear && dc->allowPartialZClear) {
	dc->cnt += dc->zClearInc;
	val = DepthClearVal(dfb);
	dx = x1 - x0;
	dy = y1 - y0;
	np = dx * dy;
	ndp = np >> 7;
	indx = (dc->cnt >> 24) & 0x7f;
	start_pix = ndp * indx;
	end_pix = start_pix + ndp;
	ya = y0 + start_pix / dx;
	yb = y0 + end_pix / dx;
	if (ya >= y1) ya = y1 - 1;
	if (yb >= y1) yb = y1 - 1;
	if (ya == yb) {
	    return;
	}
	(*(__GLlockBuffersProc)gc->buffers.lockBuffers)(gc, 0x20);
	skip = buf->outerWidth - w;
	h = (yb - ya) + 1;
	fillRectDwords(gc, (GLuint *)((GLubyte *)buf->base
		     + ((ya - gc->constants.viewportYAdjust) * buf->outerWidth
			- gc->constants.viewportXAdjust + x0) * buf->elementSize),
		     w, h, skip * 4, val);
	(*(__GLlockBuffersProc)gc->buffers.unlockBuffers)(gc, 0x20);
	(*gc->drawablePrivate->unlockDP)(gc->drawablePrivate);
	return;
    }
    if (dc->zClearInc) {
	dc->cnt = ~dc->zDepthMask;
    }
    val = DepthClearVal(dfb);
    dc->fullZClear = GL_FALSE;
    (*(__GLlockBuffersProc)gc->buffers.lockBuffers)(gc, 0x20);
    fp = (GLubyte *)buf->base
	+ ((y0 - gc->constants.viewportYAdjust) * buf->outerWidth
	   - gc->constants.viewportXAdjust + x0) * buf->elementSize;
    skip = buf->outerWidth - w;
    if (buf->elementSize == 2) {
	__glFillRect16(gc, (GLushort *)fp, w, h, skip * 2, (GLushort)val);
    } else {
	fillRectDwords(gc, (GLuint *)fp, w, h, skip * 4, val);
    }
    (*(__GLlockBuffersProc)gc->buffers.unlockBuffers)(gc, 0x20);
    (*gc->drawablePrivate->unlockDP)(gc->drawablePrivate);
}

/* 690c4fe0 -- DepthClearVal (3dfx S_DEPTH.C): the depth clear value in buffer units */
GLuint __fastcall DepthClearVal(__GLdepthBufferICD *dfb)
{
    __GLcontext *gc = dfb->buf.gc;
    __WGLdepthPrivate *dc = (__WGLdepthPrivate *)(dfb->buf.wglBuf)->depthPrivate;
    GLuint z;

    __glValidateZCount(dfb);
    if (dc->invertZRange) {
	z = (GLuint)((1.0 - gc->state.depth.clear) * dfb->scale);
    } else {
	z = (GLuint)(dfb->scale * gc->state.depth.clear);
    }
    return (z >> dc->numFracBits) + dc->cnt;
}

/* 690c5120 -- depth_MakeCurrent: s_depth.c MakeCurrent: the ancestor's persistent block is the WGL buffer's
** depthPrivate, allocated (malloc, not calloc) the first time and set up per depth size: 16 bit z >> 16, 24 bit with
** the clear-count trick (cnt / zClearInc 0xff000000), 32 bit plain.  allowPartialZClear is set by Pick. */
void __fastcall depth_MakeCurrent(__GLbufferICD *buf, __WGLbuffer *drawableBuf, __GLcontext *gc)
{
    __WGLdepthPrivate *persistent = (__WGLdepthPrivate *)drawableBuf->depthPrivate;
    GLint depth = drawableBuf->depth;

    __glMakeCurrentBuffer(buf, drawableBuf, gc);
    if (!persistent) {
	persistent = (__WGLdepthPrivate *)(*gc->drawablePrivate->malloc)(sizeof(__WGLdepthPrivate));
	drawableBuf->depthPrivate = persistent;
	drawableBuf->freePrivate = depth_FreePrivate;
	if (depth <= 16) {
	    persistent->cnt = 0;
	    persistent->numFracBits = 16;
	    persistent->zDepthMask = 0xffff;
	    persistent->zClearInc = 0;
	    persistent->fullZClear = GL_TRUE;
	} else if (depth <= 24) {
	    persistent->cnt = 0xff000000;
	    persistent->numFracBits = 7;
	    persistent->zDepthMask = 0xffffff;
	    persistent->zClearInc = 0xff000000;
	    persistent->fullZClear = GL_FALSE;
	} else {
	    persistent->zDepthMask = 0xffffffff;
	    persistent->cnt = 0;
	    persistent->numFracBits = 0;
	    persistent->zClearInc = 0;
	    persistent->fullZClear = GL_TRUE;
	}
	persistent->invertZRange = GL_FALSE;
    }

    /* reset lastDepth, scissorRegion, etc */
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH | __GL_DIRTY_SCISSOR);
}

/* 690c51f0 -- depth_FreePrivate: the WGL depth buffer's freePrivate (3dfx S_DEPTH.C LoseCurrent's job): free
** the z-offset clear state (690c5120) */
void __fastcall depth_FreePrivate(__WGLbuffer *drawableBuf, __WGLdrawable *dp)
{
    if (drawableBuf->depthPrivate) {
	(*dp->free)(drawableBuf->depthPrivate);
    }
    drawableBuf->depthPrivate = NULL;
}

/* 690c5270 -- FetchFastClear (3dfx S_DEPTH.C): 24-bit depth fetch (the stencil/count bits masked off) */
__GLzValue __fastcall FetchFastClear(__GLdepthBufferICD *dfb, GLint x, GLint y)
{
    __GLcontext *gc = dfb->buf.gc;
    __WGLbuffer *buf = dfb->buf.wglBuf;
    return *(GLuint *)__GL_DEPTH_BYTE_ADDR(gc, buf, x, y)
	& ((__WGLdepthPrivate *)buf->depthPrivate)->zDepthMask;
}

/* 690c52b0 -- Fix_zbuffer (s_depth.c): move the depth words of the clip box into the range the Z
** trick clears to next (keeping the stencil bits), when the GL clear depth is that end.
*/
void __fastcall Fix_zbuffer(__GLcontext *gc, __GLdepthBufferICD *dfb, GLuint destRange)
{
    __WGLbuffer *drawableBuf;
    __WGLdepthPrivate *dbDepth;
    GLuint cnt;
    GLuint zDepthMask;
    GLuint zClearInc;
    GLuint clearVal;
    __GLzValue *p;
    GLint x, y, x1, y1;
    GLint w, skip, i;

    x = gc->transform.clipX0;
    y = gc->transform.clipY0;
    x1 = gc->transform.clipX1;
    y1 = gc->transform.clipY1;
    if (((w = x1-x) == 0) || (y1-y == 0)) {
        return;
    }

    (*gc->drawablePrivate->lockDP)(gc->drawablePrivate, gc);
    drawableBuf = dfb->buf.wglBuf;
    dbDepth = (__WGLdepthPrivate *) drawableBuf->depthPrivate;
    cnt = dbDepth->cnt;
    zDepthMask = dbDepth->zDepthMask;
    zClearInc = dbDepth->zClearInc;

    if ((zClearInc == 0xff000000) && (gc->state.depth.clear == 1.0)) {
        clearVal = destRange | zDepthMask;
    } else if ((zClearInc == 0x01000000) && (gc->state.depth.clear == 0.0)) {
        clearVal = destRange;
    } else {
        (*gc->drawablePrivate->unlockDP)(gc->drawablePrivate);
        return;
    }

    (*gc->buffers.lockBuffers)(gc, __GL_DEPTH_BUFFER_MASK);

    p = (__GLzValue *)((GLubyte *) drawableBuf->base +
        ((y - gc->constants.viewportYAdjust) * drawableBuf->outerWidth + x -
         gc->constants.viewportXAdjust) * drawableBuf->elementSize);
    skip = drawableBuf->outerWidth - w;

    for (; y < y1; y++) {
        for (i=w; i; i--, p++) {
            if ((p[0] & ~zDepthMask) == cnt) {
                p[0] = (p[0] & zDepthMask) | destRange;
            } else {
                p[0] = clearVal;
            }
        }
        p += skip;
    }

    (*gc->buffers.unlockBuffers)(gc, __GL_DEPTH_BUFFER_MASK);
    (*gc->drawablePrivate->unlockDP)(gc->drawablePrivate);
}

/* 690c5480 -- depth_Fetch: Fetch (s_depth.c), 32-bit depth fetch */
__GLzValue __fastcall depth_Fetch(__GLdepthBufferICD *dfb, GLint x, GLint y)
{
    __GLcontext *gc = dfb->buf.gc;
    __WGLbuffer *buf = dfb->buf.wglBuf;
    return *(GLuint *)__GL_DEPTH_BYTE_ADDR(gc, buf, x, y);
}

/* 690c5050 -- depth_Pick: Pick (s_depth.c): the per-frame band clear only for a 1.0 clear with LESS/LEQUAL or a
** 0.0 clear with GREATER/GEQUAL; the store proc comes from the ICD's table */
void __fastcall depth_Pick(__GLcontext *gc, __GLdepthBufferICD *dfb, GLint depthIndex)
{
    GLenum testFunc = gc->state.depth.testFunc;
    __WGLdepthPrivate *dc;

    (*gc->drawablePrivate->lockDP)(gc->drawablePrivate, gc);
    dc = (__WGLdepthPrivate *)(dfb->buf.wglBuf)->depthPrivate;
    dc->allowPartialZClear = GL_FALSE;
    if (dc->zClearInc) {
	if (testFunc == GL_LESS || testFunc == GL_LEQUAL) {
	    if (gc->state.depth.clear == 1.0) {
		dc->allowPartialZClear = GL_TRUE;
	    }
	} else if (testFunc == GL_GREATER || testFunc == GL_GEQUAL) {
	    if (gc->state.depth.clear == 0.0) {
		dc->allowPartialZClear = GL_TRUE;
	    }
	}
    }
    (*gc->drawablePrivate->unlockDP)(gc->drawablePrivate);
    dfb->store = g_depthStoreProcs[depthIndex];
}

/* the procs below that are defined further down or in other files */
extern GLboolean __fastcall StoreNEVER16(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z);
/* 690f5264 -- the depth store procs by depthIndex: 16-, 24-, 32-bit, each without and with the depth write mask */
static __GLproc g_depthStoreProcs[49] = {
    (__GLproc)StoreNEVER16,
    (__GLproc)StoreLESS16,
    (__GLproc)StoreEQUAL16,
    (__GLproc)StoreLEQUAL16,
    (__GLproc)StoreGREATER16,
    (__GLproc)StoreNOTEQUAL16,
    (__GLproc)StoreGEQUAL16,
    (__GLproc)StoreALWAYS16,
    (__GLproc)StoreNEVER16,
    (__GLproc)StoreLESS16_W,
    (__GLproc)StoreEQUAL16_W,
    (__GLproc)StoreLEQUAL16_W,
    (__GLproc)StoreGREATER16_W,
    (__GLproc)StoreNOTEQUAL16_W,
    (__GLproc)StoreGEQUAL16_W,
    (__GLproc)StoreALWAYS16_W,
    (__GLproc)StoreNEVER24,
    (__GLproc)StoreLESS24,
    (__GLproc)StoreEQUAL24,
    (__GLproc)StoreLEQUAL24,
    (__GLproc)StoreGREATER24,
    (__GLproc)StoreNOTEQUAL24,
    (__GLproc)StoreGEQUAL24,
    (__GLproc)StoreALWAYS24,
    (__GLproc)StoreNEVER24,
    (__GLproc)StoreLESS24_W,
    (__GLproc)StoreEQUAL24_W,
    (__GLproc)StoreLEQUAL24_W,
    (__GLproc)StoreGREATER24_W,
    (__GLproc)StoreNOTEQUAL24_W,
    (__GLproc)StoreGEQUAL24_W,
    (__GLproc)StoreALWAYS24_W,
    (__GLproc)StoreNEVER32,
    (__GLproc)StoreLESS32,
    (__GLproc)StoreEQUAL32,
    (__GLproc)StoreLEQUAL32,
    (__GLproc)StoreGREATER32,
    (__GLproc)StoreNOTEQUAL32,
    (__GLproc)StoreGEQUAL32,
    (__GLproc)StoreALWAYS32,
    (__GLproc)StoreNEVER32,
    (__GLproc)DepthTest32_LESS,
    (__GLproc)DepthTest32_EQUAL,
    (__GLproc)DepthTest32_LEQUAL,
    (__GLproc)DepthTest32_GREATER,
    (__GLproc)DepthTest32_NOTEQUAL,
    (__GLproc)DepthTest32_GEQUAL,
    (__GLproc)StoreALWAYS_W,
    NULL,
};

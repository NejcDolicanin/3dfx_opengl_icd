/*
** gl_zfuncs.c -- original VA range 690c8000..690d8000: px_upack.c __glElementsPerGroup, s_rgb.c
** Fetch_32A, the "no context" return-0 stub, s_zfuncs.c depth testers (32- and 16-bit, writing
** and masked), s_ptdraw.c __glRenderFlatFogPoint and s_ptaa.c __glBuildAntiAliasIndex.
** Then the rest of s_zfuncs.c, in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "si_names.h"
#include "glsitypes.h"

typedef GLushort __GLzValue16;

/* EXT_packed_pixels / BGRA / ABGR (the kit's GL.H predates them) */
#define GL_UNSIGNED_BYTE_3_3_2_EXT      0x8032
#define GL_UNSIGNED_INT_10_10_10_2_EXT  0x8036
#ifndef GL_BGR_EXT
#define GL_BGR_EXT                      0x80e0
#endif
#ifndef GL_BGRA_EXT
#define GL_BGRA_EXT                     0x80e1
#endif
#define __GL_RED_ALPHA                  1       /* pixel.h */
#define __GL_CI_ANTI_ALIAS_DIVISOR      16

typedef void (__fastcall *__GLfogColorProcC80)(__GLcontext *gc, __GLcolor *out, __GLcolor *in, __GLfloat fog);

/* ---- forward declarations ---- */
GLboolean __fastcall __glDT_NEVER(void);
GLboolean __fastcall __glDT_NEVER16(void);

/* 690c8a60 -- __glElementsPerGroup (px_upack.c): 1 for the packed types (EXT_packed_pixels and the
** ICD's internal types 0..5), else by format.
*/
GLint __fastcall __glElementsPerGroup(GLenum format, GLenum type)
{
    if (type <= GL_UNSIGNED_INT_10_10_10_2_EXT &&
        (type >= GL_UNSIGNED_BYTE_3_3_2_EXT || type <= 5)) {
        return 1;
    }

    switch (format) {
      case GL_RGB:
      case GL_BGR_EXT:
        return 3;
      case GL_LUMINANCE_ALPHA:
        return 2;
      case GL_RGBA:
      case GL_ABGR_EXT:
      case GL_BGRA_EXT:
        return 4;
      case __GL_RED_ALPHA:
        return 2;
      default:
        return 1;
    }
}

/* 690cc120 -- Fetch_32A (s_rgb.c Fetch_* for a 32-bit buffer with alpha; no ownership test) */
void __fastcall Fetch_32A(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result)
{
    __GLcontext *gc = cfb->buf.gc;
    __WGLbuffer *db = cfb->buf.wglBuf;
    GLuint value;

    value = *(GLuint *)((GLubyte *)db->base
                        + (((y - gc->constants.viewportYAdjust) * db->outerWidth
                            - gc->constants.viewportXAdjust) + x) * db->elementSize);
    result->r = (__GLfloat)((value & gc->modes.redMask) >> cfb->redShift);
    result->g = (__GLfloat)((value & gc->modes.greenMask) >> cfb->greenShift);
    result->b = (__GLfloat)((value & gc->modes.blueMask) >> cfb->blueShift);
    result->a = (__GLfloat)((value & gc->modes.alphaMask) >> cfb->alphaShift);
}

/* 690cdc20 -- __glDT_NEVER (s_zfuncs.c), z and zfb unused */
GLboolean __fastcall __glDT_NEVER(void)
{
    return GL_FALSE;
}

/* 690cdc30 -- __glDT_LEQUAL (s_zfuncs.c) */
GLboolean __fastcall __glDT_LEQUAL(__GLzValue z, __GLzValue *zfb)
{
    __GLzValue zbv = *zfb;

    if ((GLuint)z <= (GLuint)zbv) {
        zfb[0] = z;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/* 690cdc40 -- __glDT_LESS (s_zfuncs.c) */
GLboolean __fastcall __glDT_LESS(__GLzValue z, __GLzValue *zfb)
{
    __GLzValue zbv = *zfb;

    if ((GLuint)z < (GLuint)zbv) {
        zfb[0] = z;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/* 690cdc50 -- __glDT_EQUAL (s_zfuncs.c) */
GLboolean __fastcall __glDT_EQUAL(__GLzValue z, __GLzValue *zfb)
{
    __GLzValue zbv = *zfb;

    if (z == zbv) {
        zfb[0] = z;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/* 690cdc70 -- __glDT_NOTEQUAL (s_zfuncs.c) */
GLboolean __fastcall __glDT_NOTEQUAL(__GLzValue z, __GLzValue *zfb)
{
    __GLzValue zbv = *zfb;

    if (z != zbv) {
        zfb[0] = z;
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/* 690cdc90 -- __glDT_ALWAYS (s_zfuncs.c) */
GLboolean __fastcall __glDT_ALWAYS(__GLzValue z, __GLzValue *zfb)
{
    zfb[0] = z;
    return GL_TRUE;
}

/* 690cdca0 -- __glDT_LEQUAL_M (s_zfuncs.c) */
GLboolean __fastcall __glDT_LEQUAL_M(__GLzValue z, __GLzValue *zfb)
{
    __GLzValue zbv = *zfb;

    return ((GLuint)z <= (GLuint)zbv);
}

/* 690cdcb0 -- __glDT_LESS_M (s_zfuncs.c) */
GLboolean __fastcall __glDT_LESS_M(__GLzValue z, __GLzValue *zfb)
{
    __GLzValue zbv = *zfb;

    return ((GLuint)z < (GLuint)zbv);
}

/* 690cdcc0 -- __glDT_EQUAL_M (s_zfuncs.c) */
GLboolean __fastcall __glDT_EQUAL_M(__GLzValue z, __GLzValue *zfb)
{
    __GLzValue zbv = *zfb;

    return (z == zbv);
}

/* 690cdcd0 -- __glDT_GREATER_M (s_zfuncs.c) */
GLboolean __fastcall __glDT_GREATER_M(__GLzValue z, __GLzValue *zfb)
{
    __GLzValue zbv = *zfb;

    return ((GLuint)z > (GLuint)zbv);
}

/* 690cdce0 -- __glDT_NOTEQUAL_M (s_zfuncs.c) */
GLboolean __fastcall __glDT_NOTEQUAL_M(__GLzValue z, __GLzValue *zfb)
{
    __GLzValue zbv = *zfb;

    return (z != zbv);
}

/* 690cdcf0 -- __glDT_GEQUAL_M (s_zfuncs.c) */
GLboolean __fastcall __glDT_GEQUAL_M(__GLzValue z, __GLzValue *zfb)
{
    __GLzValue zbv = *zfb;

    return ((GLuint)z >= (GLuint)zbv);
}

/* 690cdd00 -- __glDT_ALWAYS_M (s_zfuncs.c), z and zfb unused */
GLboolean __fastcall __glDT_ALWAYS_M(void)
{
    return GL_TRUE;
}

/* 690cdd10 -- __glDT_NEVER16 (s_zfuncs.c), z and zfb unused */
GLboolean __fastcall __glDT_NEVER16(void)
{
    return GL_FALSE;
}

/* 690cdde0 -- __glDT_ALWAYS16 (s_zfuncs.c) */
GLboolean __fastcall __glDT_ALWAYS16(__GLzValue16 z, __GLzValue16 *zfb16)
{
    zfb16[0] = z;
    return GL_TRUE;
}

/* 690cddf0 -- __glDT_LEQUAL16_M (s_zfuncs.c) */
GLboolean __fastcall __glDT_LEQUAL16_M(__GLzValue z, __GLzValue16 *zfb16)
{
    __GLzValue16 zbv = *zfb16;

    return ((GLuint)z <= (GLuint)zbv);
}

/* 690cde00 -- __glDT_LESS16_M (s_zfuncs.c) */
GLboolean __fastcall __glDT_LESS16_M(__GLzValue z, __GLzValue16 *zfb16)
{
    __GLzValue16 zbv = *zfb16;

    return ((GLuint)z < (GLuint)zbv);
}

/* 690cde20 -- __glDT_GREATER16_M (s_zfuncs.c) */
GLboolean __fastcall __glDT_GREATER16_M(__GLzValue z, __GLzValue16 *zfb16)
{
    __GLzValue16 zbv = *zfb16;

    return ((GLuint)z > (GLuint)zbv);
}

/* 690cde40 -- __glDT_GEQUAL16_M (s_zfuncs.c) */
GLboolean __fastcall __glDT_GEQUAL16_M(__GLzValue z, __GLzValue16 *zfb16)
{
    __GLzValue16 zbv = *zfb16;

    return ((GLuint)z >= (GLuint)zbv);
}

/* 690cde50 -- __glDT_ALWAYS16_M (s_zfuncs.c), z and zfb unused */
GLboolean __fastcall __glDT_ALWAYS16_M(void)
{
    return GL_TRUE;
}

/* 690d16d0 -- __glRenderFlatFogPoint (3dfx S_PTDRAW.C): fog the colour, then renderPoint2 */
void __fastcall __glRenderFlatFogPoint(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLcolor *vxocp;
    __GLcolor vxcol;

    (*(__GLfogColorProcC80)gc->procs.fogColor)(gc, &vxcol, vx->color, vx->fog);
    vxocp = vx->color;
    vx->color = &vxcol;

    (*gc->procs.renderPoint2)(gc, vx);

    vx->color = vxocp;
}

/* 690d1a10 -- __glBuildAntiAliasIndex (s_ptaa.c): both arguments are floats on the stack (the two
** register slots are unused).
*/
__GLfloat __fastcall __glBuildAntiAliasIndex(__GLfloat index, __GLfloat antiAliasPercent)
{
    GLint newlowbits;

    newlowbits = (GLint)((__GL_CI_ANTI_ALIAS_DIVISOR - 1) * antiAliasPercent + (__GLfloat) 0.5);
    return (__GLfloat)((((int) index) & ~(__GL_CI_ANTI_ALIAS_DIVISOR - 1)) | newlowbits);
}

/* 690cdc60 -- __glDT_GREATER (s_zfuncs.c, 3dfx 32-bit z) */
GLboolean __fastcall __glDT_GREATER(__GLzValue z, __GLzValue *zfb)
{
    if ((GLuint) z > (GLuint) *zfb) {
	zfb[0] = z;
	return GL_TRUE;
    } else {
	return GL_FALSE;
    }
}

/* 690cdc80 -- __glDT_GEQUAL (s_zfuncs.c) */
GLboolean __fastcall __glDT_GEQUAL(__GLzValue z, __GLzValue *zfb)
{
    if ((GLuint) z >= (GLuint) *zfb) {
	zfb[0] = z;
	return GL_TRUE;
    } else {
	return GL_FALSE;
    }
}

/* 690cdd20 -- __glDT_LEQUAL16 (s_zfuncs.c) */
GLboolean __fastcall __glDT_LEQUAL16(__GLzValue z, __GLzValue16 *zfb)
{
    if ((GLuint) z <= (GLuint) *zfb) {
	zfb[0] = (__GLzValue16) z;
	return GL_TRUE;
    } else {
	return GL_FALSE;
    }
}

/* 690cdd40 -- __glDT_LESS16 (s_zfuncs.c) */
GLboolean __fastcall __glDT_LESS16(__GLzValue z, __GLzValue16 *zfb)
{
    if ((GLuint) z < (GLuint) *zfb) {
	zfb[0] = (__GLzValue16) z;
	return GL_TRUE;
    } else {
	return GL_FALSE;
    }
}

/* 690cdd60 -- __glDT_EQUAL16 (s_zfuncs.c) */
GLboolean __fastcall __glDT_EQUAL16(__GLzValue z, __GLzValue16 *zfb)
{
    if ((GLuint) z == (GLuint) *zfb) {
	zfb[0] = (__GLzValue16) z;
	return GL_TRUE;
    } else {
	return GL_FALSE;
    }
}

/* 690cdd80 -- __glDT_GREATER16 (s_zfuncs.c) */
GLboolean __fastcall __glDT_GREATER16(__GLzValue z, __GLzValue16 *zfb)
{
    if ((GLuint) z > (GLuint) *zfb) {
	zfb[0] = (__GLzValue16) z;
	return GL_TRUE;
    } else {
	return GL_FALSE;
    }
}

/* 690cdda0 -- __glDT_NOTEQUAL16 (s_zfuncs.c) */
GLboolean __fastcall __glDT_NOTEQUAL16(__GLzValue z, __GLzValue16 *zfb)
{
    if ((GLuint) z != (GLuint) *zfb) {
	zfb[0] = (__GLzValue16) z;
	return GL_TRUE;
    } else {
	return GL_FALSE;
    }
}

/* 690cddc0 -- __glDT_GEQUAL16 (s_zfuncs.c) */
GLboolean __fastcall __glDT_GEQUAL16(__GLzValue z, __GLzValue16 *zfb)
{
    if ((GLuint) z >= (GLuint) *zfb) {
	zfb[0] = (__GLzValue16) z;
	return GL_TRUE;
    } else {
	return GL_FALSE;
    }
}

/* 690cde10 -- __glDT_EQUAL16_M (s_zfuncs.c): no store */
GLboolean __fastcall __glDT_EQUAL16_M(__GLzValue z, __GLzValue16 *zfb)
{
    return (GLuint) z == (GLuint) *zfb;
}

/* 690cde30 -- __glDT_NOTEQUAL16_M (s_zfuncs.c): no store */
GLboolean __fastcall __glDT_NOTEQUAL16_M(__GLzValue z, __GLzValue16 *zfb)
{
    return (GLuint) z != (GLuint) *zfb;
}

/* 690f3f50 -- s_pick.c's depth test routines for C, by function (GL_NEVER .. GL_ALWAYS) (icdglob.h) */
typedef GLboolean (__fastcall *__GLdepthTestPixel)(GLuint z, GLuint *zfb);   /* as icdglob.h */
__GLdepthTestPixel __glCDTPixel[32] = {
    /* unsigned 16-bit, no mask */
    (__GLdepthTestPixel)__glDT_NEVER16,
    (__GLdepthTestPixel)__glDT_LESS16,
    (__GLdepthTestPixel)__glDT_EQUAL16,
    (__GLdepthTestPixel)__glDT_LEQUAL16,
    (__GLdepthTestPixel)__glDT_GREATER16,
    (__GLdepthTestPixel)__glDT_NOTEQUAL16,
    (__GLdepthTestPixel)__glDT_GEQUAL16,
    (__GLdepthTestPixel)__glDT_ALWAYS16,
    /* 16-bit, mask */
    (__GLdepthTestPixel)__glDT_NEVER16,
    (__GLdepthTestPixel)__glDT_LESS16_M,
    (__GLdepthTestPixel)__glDT_EQUAL16_M,
    (__GLdepthTestPixel)__glDT_LEQUAL16_M,
    (__GLdepthTestPixel)__glDT_GREATER16_M,
    (__GLdepthTestPixel)__glDT_NOTEQUAL16_M,
    (__GLdepthTestPixel)__glDT_GEQUAL16_M,
    (__GLdepthTestPixel)__glDT_ALWAYS16_M,
    /* 32-bit, no mask */
    (__GLdepthTestPixel)__glDT_NEVER,
    (__GLdepthTestPixel)__glDT_LESS,
    (__GLdepthTestPixel)__glDT_EQUAL,
    (__GLdepthTestPixel)__glDT_LEQUAL,
    (__GLdepthTestPixel)__glDT_GREATER,
    (__GLdepthTestPixel)__glDT_NOTEQUAL,
    (__GLdepthTestPixel)__glDT_GEQUAL,
    (__GLdepthTestPixel)__glDT_ALWAYS,
    /* 32-bit, mask */
    (__GLdepthTestPixel)__glDT_NEVER,
    (__GLdepthTestPixel)__glDT_LESS_M,
    (__GLdepthTestPixel)__glDT_EQUAL_M,
    (__GLdepthTestPixel)__glDT_LEQUAL_M,
    (__GLdepthTestPixel)__glDT_GREATER_M,
    (__GLdepthTestPixel)__glDT_NOTEQUAL_M,
    (__GLdepthTestPixel)__glDT_GEQUAL_M,
    (__GLdepthTestPixel)__glDT_ALWAYS_M,
};

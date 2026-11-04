/*
** sst_clip.c -- the clippers of the hardware path: the clip interpolation and the recursive line / triangle
** clippers.
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

/* 690efdc4 -- the clip-plane sign by plane & 1 */
GLfloat g_clipPlaneSign[2] = { 1.0f, -1.0f };
/* 69003a78 -- a second copy of the clip subdivision table (gdata.c g_clipSubdivTable, sst_tribuild.c), in front of
** g_floorLog2Byte in the original's .text; ToDo UNUSED CODE: nothing in the driver reads it */
GLubyte g_clipSubdivTableUnused[6][10] = {
    { 0, 2, 1, 2, 0, 1, 4, 0, 4, 3 },
    { 0, 1, 2, 1, 0, 3, 4, 0, 4, 2 },
    { 0, 1, 0, 2, 0, 0, 0, 0, 3, 4 },
    { 1, 0, 2, 0, 1, 2, 4, 1, 4, 3 },
    { 1, 0, 1, 2, 0, 0, 0, 1, 4, 3 },
    { 2, 0, 2, 1, 0, 0, 0, 2, 3, 4 }
};
/* 69003ab4 -- g_floorLog2Byte (icdglob.h): floor(log2(i)) for i < 64, 0 for 0 and 1 */
GLubyte g_floorLog2Byte[64] = {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5
};
typedef void (__fastcall *__GLinterpFn)(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b,
                                       __GLvertexICD *out, GLuint plane);
typedef void (__fastcall *__GLdrawTriFn)(__GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
extern void __fastcall hwClipTriangle(__GLcontext *gc, GLuint plane, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);

/* 69026610 (size 861) -- hwClipInterpolate: interpolate the clipped vertex between a and b where they cross the
** frustum plane `plane` (0x10 + 2k + sign: k = x / y / z), then recompute its outcode for the planes not yet done and
** project it into window coordinates.  VC6 orders the x87 operands of these products itself; the casts only
** steer that order to the original's.  The order is function-wide: the y / z casts make the outcode switch load w
** first (as the original), and the parentheses in texture.x and window.y (no change in C meaning) undo what that
** cast moved elsewhere.  A dead parameter's slot goes to the first local defined after the
** parameter's first use; s defined before out's first use leaves out's slot to i. */
void __fastcall hwClipInterpolate(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *out,
                             GLuint plane)
{
    __GLfloat s, t, wa, wb, d, sgn;
    GLint i;

    sgn = g_clipPlaneSign[plane & 1];
    i = ((plane - 0x10) >> 1) * 4;
    d = sgn * *(GLfloat *)((GLubyte *)&a->clip + i) + a->clip.w;
    t = d / (d - (sgn * *(GLfloat *)((GLubyte *)&b->clip + i) + b->clip.w));
    s = 1.0f - t;                   /* before the first use of out: out's dead parameter slot then
                                    ** goes to the loop index i, s gets a frame slot (as the original) */
    out->window.z = t;
    wa = (a->flagBits & __GL_CLIP_NEAR) ? 1.0f : a->clip.w;    /* s, t, q were divided by w unless behind */
    wb = (b->flagBits & __GL_CLIP_NEAR) ? 1.0f : b->clip.w;
    out->clip.x = (GLdouble)(t * b->clip.x) + a->clip.x * s;
    out->clip.y = (__GLfloat)(s * a->clip.y) + t * b->clip.y;
    out->clip.z = (__GLfloat)(s * a->clip.z) + t * b->clip.z;
    out->clip.w = (__GLfloat)(s * a->clip.w) + t * b->clip.w;
    for (i = 0; i < gc->fx.numTMUs; i++) {
        if (gc->texture.textureEnabled[i]) {
            out->texture[i].x = (wa * a->texture[i].x) * s + wb * t * b->texture[i].x;
            out->texture[i].y = wa * a->texture[i].y * s + wb * b->texture[i].y * t;
            out->texture[i].w = wb * b->texture[i].w * t + wa * s * a->texture[i].w;
        }
    }
    if ((*gc->polygon.shader.getModeFlags)(gc) & __GL_SHADE_SMOOTH_LIGHT) {
        out->colors[0].r = s * a->colors[0].r + t * b->colors[0].r;
        out->colors[0].g = s * a->colors[0].g + t * b->colors[0].g;
        out->colors[0].b = s * a->colors[0].b + t * b->colors[0].b;
        out->colors[0].a = s * a->colors[0].a + t * b->colors[0].a;
        hwPackVertexColor(gc, out);
    } else {
        out->packedColor = a->packedColor;
    }
    i = 0;
    out->flagBits = 0;
    /* the new vertex's outcodes of the planes still to clip against (below `plane`): the sign bit of each
    ** plane distance moved to the plane's bit */
    switch (plane) {
      case 0x15:
        d = out->clip.w + out->clip.z;
        out->flagBits = (*(GLuint *)&d >> 11) & __GL_CLIP_NEAR;
      case 0x14:
        d = out->clip.w - out->clip.y;
        out->flagBits |= (*(GLuint *)&d >> 12) & __GL_CLIP_TOP;
      case 0x13:
        d = out->clip.w + out->clip.y;
        out->flagBits |= (*(GLuint *)&d >> 13) & __GL_CLIP_BOTTOM;
      case 0x12:
        d = out->clip.w - out->clip.x;
        out->flagBits |= (*(GLuint *)&d >> 14) & __GL_CLIP_RIGHT;
      case 0x11:
        d = out->clip.w + out->clip.x;
        out->flagBits |= (*(GLuint *)&d >> 15) & __GL_CLIP_LEFT;
    }
    if (!(out->flagBits & __GL_CLIP_NEAR)) {
        out->window.w = 1.0f / out->clip.w;
        out->window.x = out->window.w * gc->state.viewport.scale.x * out->clip.x + gc->state.viewport.center.x;
        out->window.y = (gc->state.viewport.scale.y * out->clip.y) * out->window.w + gc->state.viewport.center.y;
        out->window.z = out->window.w * gc->state.viewport.scale.z * out->clip.z + gc->state.viewport.center.z;
        for (; i < gc->fx.numTMUs; i++) {
            if (gc->texture.textureEnabled[i]) {
                out->texture[i].x = (__GLfloat)(out->texture[i].x) * out->window.w;
                out->texture[i].y = (__GLfloat)(out->texture[i].y) * out->window.w;
                out->texture[i].w = (__GLfloat)(out->texture[i].w) * out->window.w;
            }
        }
    } else {
        out->window.w = 1.0f;
    }
}

/* 69026990 (size 242) -- __glSSTClipAndDrawLine: recursive line clipper: planes plane..0 (outcode bit 16 + plane);
** a line with no vertex outside any plane goes to grDrawLine.  The new vertex lives in a 32-byte aligned stack slot. */
void __fastcall __glSSTClipAndDrawLine(__GLcontext *gc, GLuint plane, __GLvertexICD *a, __GLvertexICD *b)
{
    GLubyte buf[0xe0];
    __GLvertexICD *v;
    GLuint bit, code;

    bit = 1 << plane;
    if (((a->flagBits | b->flagBits) & 0xffff0000) == 0) {
        grDrawLine(a, b);
        return;
    }
    code = ((a->flagBits & bit) << 1 | (b->flagBits & bit)) >> plane;
    v = (__GLvertexICD *)(((GLuint)buf + 31) & ~31);
    switch (code) {
      case 0:
        __glSSTClipAndDrawLine(gc, plane - 1, a, b);
        break;
      case 1:
        (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, a, b, v, plane);
        if (gc->state.light.shadingModel == GL_SMOOTH)
            hwPackVertexColor(gc, v);
        __glSSTClipAndDrawLine(gc, plane - 1, a, v);
        break;
      case 2:
        (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, a, b, v, plane);
        if (gc->state.light.shadingModel == GL_SMOOTH)
            hwPackVertexColor(gc, v);
        __glSSTClipAndDrawLine(gc, plane - 1, b, v);
        break;
      case 3:
        return;
    }
}

/* 69026aa0 (size 79) -- hwClippedTriangleCulled: back-face test of a clipped triangle in window coordinates:
** nonzero when the face given by the winding is the culled one */
int __fastcall hwClippedTriangleCulled(__GLcontext *gc, GLfloat *a, GLfloat *b, GLfloat *c)
{
    __GLfloat area;

    area = (a[0] - c[0]) * (b[1] - c[1]) - (b[0] - c[0]) * (a[1] - c[1]);
    return gc->polygon.face[!(*(GLint *)&area & (GLint)0x80000000)] == gc->polygon.cullFace;
}

/* 69026af0 (size 727) -- hwClipTriangle: recursive triangle clipper.  Outcode bit 16 + plane of flagBits = outside
** that plane; from `plane` down, the first plane with a vertex outside splits the triangle (the interpolator
** ogLazyProcs[0] makes the new vertices, re-read at every call: it starts as the compile-on-first-use stub), the
** pieces recurse with plane - 1; an unclipped triangle goes through the cull test hwClippedTriangleCulled to hwcx.drawTri. */
void __fastcall hwClipTriangle(__GLcontext *gc, GLuint plane, __GLvertexICD *a, __GLvertexICD *b,
                             __GLvertexICD *c)
{
    __GLvertexICD vA, vB;
    GLuint any, bit, code;

    for (;;) {
        if ((any = (c->flagBits | b->flagBits | a->flagBits) & 0xffff0000) == 0) {
            if (!hwClippedTriangleCulled(gc, (GLfloat *)a, (GLfloat *)b, (GLfloat *)c))
                (*(__GLdrawTriFn)gc->hwcx.drawTri)(a, b, c);
            return;
        }
        bit = 1 << plane;
        if ((c->flagBits & b->flagBits & a->flagBits) & 0xffff0000)
            return;
        while ((any & bit) == 0) {
            bit >>= 1;
            plane--;
        }
        code = ((((a->flagBits & bit) << 1) | (b->flagBits & bit)) << 1 | (c->flagBits & bit)) >> plane;
        switch (code) {
          case 0:
            plane--;
            continue;
          case 1:
            (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, a, c, &vA, plane);
            (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, b, c, &vB, plane);
            plane--;
            hwClipTriangle(gc, plane, a, b, &vB);
            hwClipTriangle(gc, plane, a, &vB, &vA);
            return;
          case 2:
            (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, a, b, &vA, plane);
            (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, c, b, &vB, plane);
            plane--;
            hwClipTriangle(gc, plane, a, &vA, &vB);
            hwClipTriangle(gc, plane, a, &vB, c);
            return;
          case 3:
            (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, a, b, &vB, plane);
            (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, a, c, &vA, plane);
            hwClipTriangle(gc, plane - 1, a, &vB, &vA);
            break;
          case 4:
            (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, b, a, &vA, plane);
            (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, c, a, &vB, plane);
            plane--;
            hwClipTriangle(gc, plane, b, c, &vB);
            hwClipTriangle(gc, plane, b, &vB, &vA);
            return;
          case 5:
            (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, b, a, &vA, plane);
            (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, b, c, &vB, plane);
            hwClipTriangle(gc, plane - 1, b, &vB, &vA);
            break;
          case 6:
            (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, c, a, &vB, plane);
            (*(__GLinterpFn)gc->hwcx.ogLazyProcs[0])(gc, c, b, &vA, plane);
            hwClipTriangle(gc, plane - 1, c, &vB, &vA);
            break;
          case 7:
            return;                     /* its own table entry, as in the original */
        }
        return;
    }
}

/* 69026df0 -- __glSSTClipAndDrawTriangle: clips a triangle (a, b, c on the stack): the recursive clipper (69026af0)
** from the highest outcode bit of the three vertices (flagBits bits 16..21, through the floor(log2) byte table) */
void __fastcall __glSSTClipAndDrawTriangle(__GLcontext *gc, GLuint edx_unused, __GLvertexICD *a, __GLvertexICD *b,
                             __GLvertexICD *c)
{
    hwClipTriangle(gc, g_floorLog2Byte[(a->flagBits | b->flagBits | c->flagBits) >> 16 & 0x3f] + 0x10, a, b, c);
}

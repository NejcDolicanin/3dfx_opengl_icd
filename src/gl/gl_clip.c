/*
** gl_clip.c -- s_clip.c: the frustum / user clip checks, the clip interpolators and the line / polygon
** clippers (SI port: si_s_clip.c).
*/
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "icdprotos.h"
#include "icdglob.h"

/* ---- s_clip.c: the parameter clip procs (gc->procs.lineClipParam / polyClipParam) -----------------
** dst = b + t * (a - b) for the clip coordinates and whatever parameters the variant carries
** (I = CI front index, C = RGBA front colour, B = the back colour too, F = fog, T = texture).
** The ICD differs from the SI: window.w = 1 / clip.w without the clip.w == 0 check, the fog
** variants also interpolate fogCoord (EXT_fog_coord), and the texture variants do two units.
** The projected-texture variants (PT) follow the others. */
#define __GL_CLIP_LERP(field) \
    dst->field = (a->field - b->field) * t + b->field
#define __GL_CLIP_POS \
    w = (a->clip.w - b->clip.w) * t + b->clip.w; \
    dst->clip.w = w; \
    dst->window.w = 1.0f / w; \
    __GL_CLIP_LERP(clip.x); \
    __GL_CLIP_LERP(clip.y); \
    __GL_CLIP_LERP(clip.z); \
    dst->flagBits = __GL_CLIP_MASK
#define __GL_CLIP_COLOR \
    __GL_CLIP_LERP(colors[__GL_FRONTFACE].r); \
    __GL_CLIP_LERP(colors[__GL_FRONTFACE].g); \
    __GL_CLIP_LERP(colors[__GL_FRONTFACE].b); \
    __GL_CLIP_LERP(colors[__GL_FRONTFACE].a)
#define __GL_CLIP_INDEX \
    __GL_CLIP_LERP(colors[__GL_FRONTFACE].r)
#define __GL_CLIP_BACKCOLOR \
    __GL_CLIP_LERP(colors[__GL_BACKFACE].r); \
    __GL_CLIP_LERP(colors[__GL_BACKFACE].g); \
    __GL_CLIP_LERP(colors[__GL_BACKFACE].b); \
    __GL_CLIP_LERP(colors[__GL_BACKFACE].a)
#define __GL_CLIP_BACKINDEX \
    __GL_CLIP_LERP(colors[__GL_BACKFACE].r)
/* the SI loops over __GL_NUM_TEXTURE_UNITS; the ICD has two units, unrolled */
#define __GL_CLIP_TEXTURE \
    __GL_CLIP_LERP(texture[0].x); \
    __GL_CLIP_LERP(texture[0].y); \
    __GL_CLIP_LERP(texture[0].z); \
    __GL_CLIP_LERP(texture[0].w); \
    __GL_CLIP_LERP(texture[1].x); \
    __GL_CLIP_LERP(texture[1].y); \
    __GL_CLIP_LERP(texture[1].z); \
    __GL_CLIP_LERP(texture[1].w)
/* ---- clip interpolation: dst = b + t * (a - b) on clip coordinates (window.w = 1/clip.w) and the
** two texture coordinate sets, homogenised by clip.w unless the vertex is flagged 0x100000. */
#define TEX_PROJECTED        0x00100000
/* slow fog clips eye.z, cheap fog the fog value */
#define __GL_CLIP_FOG \
    if (a->flagBits & __GL_HAS_FOG) { \
	__GL_CLIP_LERP(fog); \
    } else { \
	__GL_CLIP_LERP(eye.z); \
    } \
    __GL_CLIP_LERP(fogCoord)
/************************************************************************/
/* the clip interpolators with fog and two texture units: dst = b + t * (a - b); texture coordinates
** are interpolated homogeneous (times clip.w) unless the vertex has flag 0x100000 */
#define LERP(f)     dst->f = (a->f - b->f) * t + b->f
#define CLIP_INTERP(name, COLORS)                                                       \
void __fastcall name(__GLvertexICD *dst, __GLvertexICD *a, __GLvertexICD *b, __GLfloat t) \
{                                                                                           \
    __GLfloat w;                                                                            \
    __GLfloat a0, a1, a2, a3, a4, a5, a6, a7;                                               \
    __GLfloat b0, b1, b2, b3, b4, b5, b6, b7;                                               \
                                                                                            \
    w = (a->clip.w - b->clip.w) * t + b->clip.w;                                            \
    dst->clip.w = w;                                                                        \
    dst->window.w = __GL_ONE_F / w;                                                         \
    LERP(clip.x);                                                                       \
    LERP(clip.y);                                                                       \
    LERP(clip.z);                                                                       \
    dst->flagBits = 0xffff0000;                                                             \
    COLORS                                                                                  \
    if (a->flagBits & 0x40) {                                                               \
        LERP(fog);                                                                      \
    } else {                                                                                \
        LERP(eye.z);                                                                    \
    }                                                                                       \
    LERP(fogCoord);                                                                     \
                                                                                            \
    if (!(a->flagBits & 0x100000)) {                                                        \
        a0 = a->texture[0].x * a->clip.w;                                                 \
        a1 = a->texture[0].y * a->clip.w;                                                   \
        a2 = a->texture[0].z * a->clip.w;                                                   \
        a3 = a->texture[0].w * a->clip.w;                                                   \
        a4 = a->texture[1].x * a->clip.w;                                                   \
        a5 = a->texture[1].y * a->clip.w;                                                   \
        a6 = a->texture[1].z * a->clip.w;                                                   \
        a7 = a->texture[1].w * a->clip.w;                                                   \
    } else {                                                                                \
        a0 = a->texture[0].x;                                                               \
        a1 = a->texture[0].y;                                                               \
        a2 = a->texture[0].z;                                                               \
        a3 = a->texture[0].w;                                                               \
        a4 = a->texture[1].x;                                                               \
        a5 = a->texture[1].y;                                                               \
        a6 = a->texture[1].z;                                                               \
        a7 = a->texture[1].w;                                                               \
    }                                                                                       \
    if (!(b->flagBits & 0x100000)) {                                                        \
        b0 = b->texture[0].x * b->clip.w;                                                 \
        b1 = b->texture[0].y * b->clip.w;                                                   \
        b2 = b->texture[0].z * b->clip.w;                                                   \
        b3 = b->texture[0].w * b->clip.w;                                                   \
        b4 = b->texture[1].x * b->clip.w;                                                   \
        b5 = b->texture[1].y * b->clip.w;                                                   \
        b6 = b->texture[1].z * b->clip.w;                                                   \
        b7 = b->texture[1].w * b->clip.w;                                                   \
    } else {                                                                                \
        b0 = b->texture[0].x;                                                               \
        b1 = b->texture[0].y;                                                               \
        b2 = b->texture[0].z;                                                               \
        b3 = b->texture[0].w;                                                               \
        b4 = b->texture[1].x;                                                               \
        b5 = b->texture[1].y;                                                               \
        b6 = b->texture[1].z;                                                               \
        b7 = b->texture[1].w;                                                               \
    }                                                                                       \
    dst->texture[0].x = (a0 - b0) * t + b0;                                                 \
    dst->texture[0].y = (a1 - b1) * t + b1;                                                 \
    dst->texture[0].z = (a2 - b2) * t + b2;                                                 \
    dst->texture[0].w = (a3 - b3) * t + b3;                                                 \
    dst->texture[1].x = (a4 - b4) * t + b4;                                                 \
    dst->texture[1].y = (a5 - b5) * t + b5;                                                 \
    dst->texture[1].z = (a6 - b6) * t + b6;                                                 \
    dst->texture[1].w = (a7 - b7) * t + b7;                                                 \
}
/* ---- s_clip.c: __glGenericPickParameterClipProcs ------------------------------------------------- */
static __GLproc clipProcs[30];         /* s_clip.c (690f44d0; at the end of the file) */
#define __GL_CLIP_PROCS     clipProcs
typedef __GLvertexICD __GLvertex;

/* 6908d040 -- clip_Clip: Clip (s_clip.c), position only */
void __fastcall clip_Clip(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
}

/* 6908d0a0 -- ClipC (s_clip.c): interpolate clip position and the front RGBA colour */
void __fastcall ClipC(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_COLOR;
}

/* 6908d140 -- ClipI (s_clip.c): interpolate clip position and the front CI colour */
void __fastcall ClipI(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_INDEX;
}

/* 6908d1b0 -- ClipBC (s_clip.c): interpolate clip position and the front and back RGBA colours */
void __fastcall ClipBC(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_COLOR;
    __GL_CLIP_BACKCOLOR;
}

/* 6908d290 -- ClipBI (s_clip.c): interpolate clip position and the front and back CI colours */
void __fastcall ClipBI(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_INDEX;
    __GL_CLIP_BACKINDEX;
}

/* 6908d310 -- ClipT (s_clip.c): interpolate clip position and both texture units */
void __fastcall ClipT(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_TEXTURE;
}

/* 6908d3f0 -- ClipIT (s_clip.c): interpolate clip position, the front CI colour and both texture units */
void __fastcall ClipIT(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_INDEX;
    __GL_CLIP_TEXTURE;
}

/* 6908d4e0 -- ClipBIT (s_clip.c): interpolate clip position, front and back CI colours and both texture units */
void __fastcall ClipBIT(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_INDEX;
    __GL_CLIP_BACKINDEX;
    __GL_CLIP_TEXTURE;
}

/* 6908d5e0 -- ClipCT (s_clip.c): interpolate clip position, the front RGBA colour and both texture units */
void __fastcall ClipCT(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_COLOR;
    __GL_CLIP_TEXTURE;
}

/* 6908d700 -- ClipBCT (s_clip.c): interpolate clip position, front and back RGBA colours and both texture units */
void __fastcall ClipBCT(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_COLOR;
    __GL_CLIP_BACKCOLOR;
    __GL_CLIP_TEXTURE;
}

/* 6908d860 -- ClipT2: interpolate clip position and both texture units (clipProcs[20], ICD-only) */
void __fastcall ClipT2(__GLvertexICD *dst, __GLvertexICD *a, __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;
    __GLfloat as0, at0, ar0, aq0, as1, at1, ar1, aq1;
    __GLfloat bs0, bt0, br0, bq0, bs1, bt1, br1, bq1;

    w = (a->clip.w - b->clip.w) * t + b->clip.w;
    dst->clip.w = w;
    dst->window.w = 1.0f / w;
    dst->clip.x = (a->clip.x - b->clip.x) * t + b->clip.x;
    dst->clip.y = (a->clip.y - b->clip.y) * t + b->clip.y;
    dst->clip.z = (a->clip.z - b->clip.z) * t + b->clip.z;
    dst->flagBits = 0xffff0000;

    if (!(a->flagBits & TEX_PROJECTED)) {
        as0 = a->texture[0].x * a->clip.w;
        at0 = a->texture[0].y * a->clip.w;
        ar0 = a->texture[0].z * a->clip.w;
        aq0 = a->texture[0].w * a->clip.w;
        as1 = a->texture[1].x * a->clip.w;
        at1 = a->texture[1].y * a->clip.w;
        ar1 = a->texture[1].z * a->clip.w;
        aq1 = a->texture[1].w * a->clip.w;
    } else {
        as0 = a->texture[0].x;
        at0 = a->texture[0].y;
        ar0 = a->texture[0].z;
        aq0 = a->texture[0].w;
        as1 = a->texture[1].x;
        at1 = a->texture[1].y;
        ar1 = a->texture[1].z;
        aq1 = a->texture[1].w;
    }
    if (!(b->flagBits & TEX_PROJECTED)) {
        bs0 = b->texture[0].x * b->clip.w;
        bt0 = b->texture[0].y * b->clip.w;
        br0 = b->texture[0].z * b->clip.w;
        bq0 = b->texture[0].w * b->clip.w;
        bs1 = b->texture[1].x * b->clip.w;
        bt1 = b->texture[1].y * b->clip.w;
        br1 = b->texture[1].z * b->clip.w;
        bq1 = b->texture[1].w * b->clip.w;
    } else {
        bs0 = b->texture[0].x;
        bt0 = b->texture[0].y;
        br0 = b->texture[0].z;
        bq0 = b->texture[0].w;
        bs1 = b->texture[1].x;
        bt1 = b->texture[1].y;
        br1 = b->texture[1].z;
        bq1 = b->texture[1].w;
    }
    dst->texture[0].x = (as0 - bs0) * t + bs0;
    dst->texture[0].y = (at0 - bt0) * t + bt0;
    dst->texture[0].z = (ar0 - br0) * t + br0;
    dst->texture[0].w = (aq0 - bq0) * t + bq0;
    dst->texture[1].x = (as1 - bs1) * t + bs1;
    dst->texture[1].y = (at1 - bt1) * t + bt1;
    dst->texture[1].z = (ar1 - br1) * t + br1;
    dst->texture[1].w = (aq1 - bq1) * t + bq1;
}

/* 6908da70 -- ClipIT2: interpolate clip, CI colour and both texture units (clipProcs[21]) */
void __fastcall ClipIT2(__GLvertexICD *dst, __GLvertexICD *a, __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;
    __GLfloat as0, at0, ar0, aq0, as1, at1, ar1, aq1;
    __GLfloat bs0, bt0, br0, bq0, bs1, bt1, br1, bq1;

    w = (a->clip.w - b->clip.w) * t + b->clip.w;
    dst->clip.w = w;
    dst->window.w = 1.0f / w;
    dst->clip.x = (a->clip.x - b->clip.x) * t + b->clip.x;
    dst->clip.y = (a->clip.y - b->clip.y) * t + b->clip.y;
    dst->clip.z = (a->clip.z - b->clip.z) * t + b->clip.z;
    dst->flagBits = 0xffff0000;
    dst->colors[0].r = (a->colors[0].r - b->colors[0].r) * t + b->colors[0].r;

    if (!(a->flagBits & TEX_PROJECTED)) {
        as0 = a->texture[0].x * a->clip.w;
        at0 = a->texture[0].y * a->clip.w;
        ar0 = a->texture[0].z * a->clip.w;
        aq0 = a->texture[0].w * a->clip.w;
        as1 = a->texture[1].x * a->clip.w;
        at1 = a->texture[1].y * a->clip.w;
        ar1 = a->texture[1].z * a->clip.w;
        aq1 = a->texture[1].w * a->clip.w;
    } else {
        as0 = a->texture[0].x;
        at0 = a->texture[0].y;
        ar0 = a->texture[0].z;
        aq0 = a->texture[0].w;
        as1 = a->texture[1].x;
        at1 = a->texture[1].y;
        ar1 = a->texture[1].z;
        aq1 = a->texture[1].w;
    }
    if (!(b->flagBits & TEX_PROJECTED)) {
        bs0 = b->texture[0].x * b->clip.w;
        bt0 = b->texture[0].y * b->clip.w;
        br0 = b->texture[0].z * b->clip.w;
        bq0 = b->texture[0].w * b->clip.w;
        bs1 = b->texture[1].x * b->clip.w;
        bt1 = b->texture[1].y * b->clip.w;
        br1 = b->texture[1].z * b->clip.w;
        bq1 = b->texture[1].w * b->clip.w;
    } else {
        bs0 = b->texture[0].x;
        bt0 = b->texture[0].y;
        br0 = b->texture[0].z;
        bq0 = b->texture[0].w;
        bs1 = b->texture[1].x;
        bt1 = b->texture[1].y;
        br1 = b->texture[1].z;
        bq1 = b->texture[1].w;
    }
    dst->texture[0].x = (as0 - bs0) * t + bs0;
    dst->texture[0].y = (at0 - bt0) * t + bt0;
    dst->texture[0].z = (ar0 - br0) * t + br0;
    dst->texture[0].w = (aq0 - bq0) * t + bq0;
    dst->texture[1].x = (as1 - bs1) * t + bs1;
    dst->texture[1].y = (at1 - bt1) * t + bt1;
    dst->texture[1].z = (ar1 - br1) * t + br1;
    dst->texture[1].w = (aq1 - bq1) * t + bq1;
}

/* 6908dc90 -- ClipBIT2: interpolate clip, front and back CI colours and both texture units (clipProcs[23]) */
void __fastcall ClipBIT2(__GLvertexICD *dst, __GLvertexICD *a, __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;
    __GLfloat as0, at0, ar0, aq0, as1, at1, ar1, aq1;
    __GLfloat bs0, bt0, br0, bq0, bs1, bt1, br1, bq1;

    w = (a->clip.w - b->clip.w) * t + b->clip.w;
    dst->clip.w = w;
    dst->window.w = 1.0f / w;
    dst->clip.x = (a->clip.x - b->clip.x) * t + b->clip.x;
    dst->clip.y = (a->clip.y - b->clip.y) * t + b->clip.y;
    dst->clip.z = (a->clip.z - b->clip.z) * t + b->clip.z;
    dst->flagBits = 0xffff0000;
    dst->colors[0].r = (a->colors[0].r - b->colors[0].r) * t + b->colors[0].r;
    dst->colors[1].r = (a->colors[1].r - b->colors[1].r) * t + b->colors[1].r;

    if (!(a->flagBits & TEX_PROJECTED)) {
        as0 = a->texture[0].x * a->clip.w;
        at0 = a->texture[0].y * a->clip.w;
        ar0 = a->texture[0].z * a->clip.w;
        aq0 = a->texture[0].w * a->clip.w;
        as1 = a->texture[1].x * a->clip.w;
        at1 = a->texture[1].y * a->clip.w;
        ar1 = a->texture[1].z * a->clip.w;
        aq1 = a->texture[1].w * a->clip.w;
    } else {
        as0 = a->texture[0].x;
        at0 = a->texture[0].y;
        ar0 = a->texture[0].z;
        aq0 = a->texture[0].w;
        as1 = a->texture[1].x;
        at1 = a->texture[1].y;
        ar1 = a->texture[1].z;
        aq1 = a->texture[1].w;
    }
    if (!(b->flagBits & TEX_PROJECTED)) {
        bs0 = b->texture[0].x * b->clip.w;
        bt0 = b->texture[0].y * b->clip.w;
        br0 = b->texture[0].z * b->clip.w;
        bq0 = b->texture[0].w * b->clip.w;
        bs1 = b->texture[1].x * b->clip.w;
        bt1 = b->texture[1].y * b->clip.w;
        br1 = b->texture[1].z * b->clip.w;
        bq1 = b->texture[1].w * b->clip.w;
    } else {
        bs0 = b->texture[0].x;
        bt0 = b->texture[0].y;
        br0 = b->texture[0].z;
        bq0 = b->texture[0].w;
        bs1 = b->texture[1].x;
        bt1 = b->texture[1].y;
        br1 = b->texture[1].z;
        bq1 = b->texture[1].w;
    }
    dst->texture[0].x = (as0 - bs0) * t + bs0;
    dst->texture[0].y = (at0 - bt0) * t + bt0;
    dst->texture[0].z = (ar0 - br0) * t + br0;
    dst->texture[0].w = (aq0 - bq0) * t + bq0;
    dst->texture[1].x = (as1 - bs1) * t + bs1;
    dst->texture[1].y = (at1 - bt1) * t + bt1;
    dst->texture[1].z = (ar1 - br1) * t + br1;
    dst->texture[1].w = (aq1 - bq1) * t + bq1;
}

/* 6908dec0 -- ClipCT2: interpolate clip, RGBA colour and both texture units (clipProcs[22]) */
void __fastcall ClipCT2(__GLvertexICD *dst, __GLvertexICD *a, __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;
    __GLfloat as0, at0, ar0, aq0, as1, at1, ar1, aq1;
    __GLfloat bs0, bt0, br0, bq0, bs1, bt1, br1, bq1;

    w = (a->clip.w - b->clip.w) * t + b->clip.w;
    dst->clip.w = w;
    dst->window.w = 1.0f / w;
    dst->clip.x = (a->clip.x - b->clip.x) * t + b->clip.x;
    dst->clip.y = (a->clip.y - b->clip.y) * t + b->clip.y;
    dst->clip.z = (a->clip.z - b->clip.z) * t + b->clip.z;
    dst->flagBits = 0xffff0000;
    dst->colors[0].r = (a->colors[0].r - b->colors[0].r) * t + b->colors[0].r;
    dst->colors[0].g = (a->colors[0].g - b->colors[0].g) * t + b->colors[0].g;
    dst->colors[0].b = (a->colors[0].b - b->colors[0].b) * t + b->colors[0].b;
    dst->colors[0].a = (a->colors[0].a - b->colors[0].a) * t + b->colors[0].a;

    if (!(a->flagBits & TEX_PROJECTED)) {
        as0 = a->texture[0].x * a->clip.w;
        at0 = a->texture[0].y * a->clip.w;
        ar0 = a->texture[0].z * a->clip.w;
        aq0 = a->texture[0].w * a->clip.w;
        as1 = a->texture[1].x * a->clip.w;
        at1 = a->texture[1].y * a->clip.w;
        ar1 = a->texture[1].z * a->clip.w;
        aq1 = a->texture[1].w * a->clip.w;
    } else {
        as0 = a->texture[0].x;
        at0 = a->texture[0].y;
        ar0 = a->texture[0].z;
        aq0 = a->texture[0].w;
        as1 = a->texture[1].x;
        at1 = a->texture[1].y;
        ar1 = a->texture[1].z;
        aq1 = a->texture[1].w;
    }
    if (!(b->flagBits & TEX_PROJECTED)) {
        bs0 = b->texture[0].x * b->clip.w;
        bt0 = b->texture[0].y * b->clip.w;
        br0 = b->texture[0].z * b->clip.w;
        bq0 = b->texture[0].w * b->clip.w;
        bs1 = b->texture[1].x * b->clip.w;
        bt1 = b->texture[1].y * b->clip.w;
        br1 = b->texture[1].z * b->clip.w;
        bq1 = b->texture[1].w * b->clip.w;
    } else {
        bs0 = b->texture[0].x;
        bt0 = b->texture[0].y;
        br0 = b->texture[0].z;
        bq0 = b->texture[0].w;
        bs1 = b->texture[1].x;
        bt1 = b->texture[1].y;
        br1 = b->texture[1].z;
        bq1 = b->texture[1].w;
    }
    dst->texture[0].x = (as0 - bs0) * t + bs0;
    dst->texture[0].y = (at0 - bt0) * t + bt0;
    dst->texture[0].z = (ar0 - br0) * t + br0;
    dst->texture[0].w = (aq0 - bq0) * t + bq0;
    dst->texture[1].x = (as1 - bs1) * t + bs1;
    dst->texture[1].y = (at1 - bt1) * t + bt1;
    dst->texture[1].z = (ar1 - br1) * t + br1;
    dst->texture[1].w = (aq1 - bq1) * t + bq1;
}

/* 6908e110 -- ClipBCT2: interpolate clip, front and back RGBA colours and both texture units (clipProcs[24]) */
void __fastcall ClipBCT2(__GLvertexICD *dst, __GLvertexICD *a, __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;
    __GLfloat as0, at0, ar0, aq0, as1, at1, ar1, aq1;
    __GLfloat bs0, bt0, br0, bq0, bs1, bt1, br1, bq1;

    w = (a->clip.w - b->clip.w) * t + b->clip.w;
    dst->clip.w = w;
    dst->window.w = 1.0f / w;
    dst->clip.x = (a->clip.x - b->clip.x) * t + b->clip.x;
    dst->clip.y = (a->clip.y - b->clip.y) * t + b->clip.y;
    dst->clip.z = (a->clip.z - b->clip.z) * t + b->clip.z;
    dst->flagBits = 0xffff0000;
    dst->colors[0].r = (a->colors[0].r - b->colors[0].r) * t + b->colors[0].r;
    dst->colors[0].g = (a->colors[0].g - b->colors[0].g) * t + b->colors[0].g;
    dst->colors[0].b = (a->colors[0].b - b->colors[0].b) * t + b->colors[0].b;
    dst->colors[0].a = (a->colors[0].a - b->colors[0].a) * t + b->colors[0].a;
    dst->colors[1].r = (a->colors[1].r - b->colors[1].r) * t + b->colors[1].r;
    dst->colors[1].g = (a->colors[1].g - b->colors[1].g) * t + b->colors[1].g;
    dst->colors[1].b = (a->colors[1].b - b->colors[1].b) * t + b->colors[1].b;
    dst->colors[1].a = (a->colors[1].a - b->colors[1].a) * t + b->colors[1].a;

    if (!(a->flagBits & TEX_PROJECTED)) {
        as0 = a->texture[0].x * a->clip.w;
        at0 = a->texture[0].y * a->clip.w;
        ar0 = a->texture[0].z * a->clip.w;
        aq0 = a->texture[0].w * a->clip.w;
        as1 = a->texture[1].x * a->clip.w;
        at1 = a->texture[1].y * a->clip.w;
        ar1 = a->texture[1].z * a->clip.w;
        aq1 = a->texture[1].w * a->clip.w;
    } else {
        as0 = a->texture[0].x;
        at0 = a->texture[0].y;
        ar0 = a->texture[0].z;
        aq0 = a->texture[0].w;
        as1 = a->texture[1].x;
        at1 = a->texture[1].y;
        ar1 = a->texture[1].z;
        aq1 = a->texture[1].w;
    }
    if (!(b->flagBits & TEX_PROJECTED)) {
        bs0 = b->texture[0].x * b->clip.w;
        bt0 = b->texture[0].y * b->clip.w;
        br0 = b->texture[0].z * b->clip.w;
        bq0 = b->texture[0].w * b->clip.w;
        bs1 = b->texture[1].x * b->clip.w;
        bt1 = b->texture[1].y * b->clip.w;
        br1 = b->texture[1].z * b->clip.w;
        bq1 = b->texture[1].w * b->clip.w;
    } else {
        bs0 = b->texture[0].x;
        bt0 = b->texture[0].y;
        br0 = b->texture[0].z;
        bq0 = b->texture[0].w;
        bs1 = b->texture[1].x;
        bt1 = b->texture[1].y;
        br1 = b->texture[1].z;
        bq1 = b->texture[1].w;
    }
    dst->texture[0].x = (as0 - bs0) * t + bs0;
    dst->texture[0].y = (at0 - bt0) * t + bt0;
    dst->texture[0].z = (ar0 - br0) * t + br0;
    dst->texture[0].w = (aq0 - bq0) * t + bq0;
    dst->texture[1].x = (as1 - bs1) * t + bs1;
    dst->texture[1].y = (at1 - bt1) * t + bt1;
    dst->texture[1].z = (ar1 - br1) * t + br1;
    dst->texture[1].w = (aq1 - bq1) * t + bq1;
}

/* 6908e3a0 -- ClipF (s_clip.c): interpolate clip position and fog */
void __fastcall ClipF(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_FOG;
}

/* 6908e440 -- ClipIF (s_clip.c): interpolate clip position, the front CI colour and fog */
void __fastcall ClipIF(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_INDEX;
    __GL_CLIP_FOG;
}

/* 6908e4f0 -- ClipBIF (s_clip.c): interpolate clip position, front and back CI colours and fog */
void __fastcall ClipBIF(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_INDEX;
    __GL_CLIP_BACKINDEX;
    __GL_CLIP_FOG;
}

/* 6908e5b0 -- ClipCF (s_clip.c): interpolate clip position, the front RGBA colour and fog */
void __fastcall ClipCF(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_COLOR;
    __GL_CLIP_FOG;
}

/* 6908e690 -- ClipBCF (s_clip.c): interpolate clip position, front and back RGBA colours and fog */
void __fastcall ClipBCF(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_COLOR;
    __GL_CLIP_BACKCOLOR;
    __GL_CLIP_FOG;
}

/* 6908e7b0 -- ClipFT (s_clip.c): interpolate clip position, fog and both texture units */
void __fastcall ClipFT(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_FOG;
    __GL_CLIP_TEXTURE;
}

/* 6908e8d0 -- ClipIFT (s_clip.c): interpolate clip position, the front CI colour, fog and both texture units */
void __fastcall ClipIFT(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_INDEX;
    __GL_CLIP_FOG;
    __GL_CLIP_TEXTURE;
}

/* 6908ea00 -- ClipBIFT (s_clip.c): interpolate clip position, front and back CI colours, fog and both texture
** units */
void __fastcall ClipBIFT(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_INDEX;
    __GL_CLIP_BACKINDEX;
    __GL_CLIP_FOG;
    __GL_CLIP_TEXTURE;
}

/* 6908eb40 -- ClipCFT (s_clip.c): interpolate clip position, the front RGBA colour, fog and both texture units */
void __fastcall ClipCFT(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_COLOR;
    __GL_CLIP_FOG;
    __GL_CLIP_TEXTURE;
}

/* 6908eca0 -- ClipBCFT (s_clip.c): interpolate clip position, front and back RGBA colours, fog and both
** texture units */
void __fastcall ClipBCFT(__GLvertexICD *dst, const __GLvertexICD *a, const __GLvertexICD *b, __GLfloat t)
{
    __GLfloat w;

    __GL_CLIP_POS;
    __GL_CLIP_COLOR;
    __GL_CLIP_BACKCOLOR;
    __GL_CLIP_FOG;
    __GL_CLIP_TEXTURE;
}

/* clipProcs[25..29] (ICD-only, fog + both texture units): no colour, CI front, CI front + back,
** RGBA front, RGBA front + back */
/* 6908ee40 -- ClipFT2: clip position, fog and both texture units (clipProcs[25]) */
CLIP_INTERP(ClipFT2, ;)

/* 6908f090 -- ClipIFT2: ClipFT2 + the front CI colour (clipProcs[26]) */
CLIP_INTERP(ClipIFT2, LERP(colors[0].r);)

/* 6908f2f0 -- ClipBIFT2: ClipFT2 + front and back CI colours (clipProcs[28]) */
CLIP_INTERP(ClipBIFT2, LERP(colors[0].r); LERP(colors[1].r);)

/* 6908f560 -- ClipCFT2: ClipFT2 + the front RGBA colour (clipProcs[27]) */
CLIP_INTERP(ClipCFT2, LERP(colors[0].r); LERP(colors[0].g); LERP(colors[0].b); LERP(colors[0].a);)

/* 6908f7f0 -- ClipBCFT2: ClipFT2 + front and back RGBA colours (clipProcs[29]) */
CLIP_INTERP(ClipBCFT2, LERP(colors[0].r); LERP(colors[0].g); LERP(colors[0].b); LERP(colors[0].a);
                              LERP(colors[1].r); LERP(colors[1].g); LERP(colors[1].b); LERP(colors[1].a);)

/* 6908fac0 -- __glGenericPickParameterClipProcs: the ICD also takes the cheap fog path's
** parameter when the fog coordinate comes from glFogCoord */
void __fastcall __glGenericPickParameterClipProcs(__GLcontext *gc)
{
    GLint line = 0, poly = 0;
    GLboolean twoSided = (gc->state.enables.general & __GL_LIGHTING_ENABLE)
	&& gc->state.light.model.twoSided;
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);

    if (gc->modes.rgbMode) {
	if (gc->state.light.shadingModel != GL_FLAT) {
	    line = 2;
	    if (twoSided) {
		poly = 4;
	    } else {
		poly = 2;
	    }
	}
    } else {
	if (gc->state.light.shadingModel != GL_FLAT) {
	    line = 1;
	    if (twoSided) {
		poly = 3;
	    } else {
		poly = 1;
	    }
	}
    }
    if ((modeFlags & __GL_SHADE_SLOW_FOG) ||
	    ((modeFlags & __GL_SHADE_CHEAP_FOG) &&
	    (!(modeFlags & __GL_SHADE_SMOOTH_LIGHT) ||
	     gc->state.fog.coordSource == GL_FOG_COORDINATE_EXT))) {
	line += 5;
	poly += 5;
    }
    if (gc->texture.textureEnabled[0]) {
	if (modeFlags & __GL_SHADE_PROJSCALED_TEXTURE) {
	    line += 20;
	    poly += 20;
	} else {
	    line += 10;
	    poly += 10;
	}
    }
    gc->procs.lineClipParam = (__GLproc)__GL_CLIP_PROCS[line];
    gc->procs.polyClipParam = (__GLproc)__GL_CLIP_PROCS[poly];
}

#pragma optimize("", off)
/* 690c3310 -- __glClipCheckFast: clipCheck2/3/4 (when the ICD's fast path is chosen): clip code of v, and for an
** unclipped vertex its window coordinates.  Unoptimised C; the w > 0 path is an __asm block. */
GLuint __fastcall __glClipCheckFast(__GLcontext *gc, __GLvertexICD *v)
{
    __GLfloat wz, winY, winX;              /* the /Od frame slots follow the names */
    GLuint clipcode;
    __GLfloat negW, z, y, x, w;
    __GLfloat invW;
    __GLviewport *vport;
    GLuint result;

    vport = &gc->state.viewport;
    if (-v->clip.w < 0.0f) {
        __asm {
            mov     edi, v
            mov     eax, [edi+0x4c]
            mov     esi, 0
            cmp     eax, 0x3f800000
            jne     notone
            fld     dword ptr [edi+0x4c]
            jmp     gotinv
        notone:
            fld1
            fdiv    dword ptr [edi+0x4c]
        gotinv:
            mov     ebx, [edi+0x40]
            mov     ecx, ebx
            and     ebx, 0x7fffffff
            sar     ecx, 0x1f
            cmp     eax, ebx
            sbb     ebx, ebx
            xor     ecx, 0x20000
            and     ecx, 0x30000
            and     ebx, ecx
            or      esi, ebx
            mov     ebx, [edi+0x44]
            mov     ecx, ebx
            and     ebx, 0x7fffffff
            sar     ecx, 0x1f
            cmp     eax, ebx
            sbb     ebx, ebx
            xor     ecx, 0x80000
            and     ecx, 0xc0000
            and     ebx, ecx
            or      esi, ebx
            mov     ebx, [edi+0x48]
            mov     ecx, ebx
            and     ebx, 0x7fffffff
            sar     ecx, 0x1f
            cmp     eax, ebx
            sbb     ebx, ebx
            xor     ecx, 0x200000
            and     ecx, 0x300000
            and     ebx, ecx
            or      esi, ebx
            mov     result, esi
            fstp    dword ptr [edi+0xc]
            or      dword ptr [edi+0x10], 0x20
            cmp     esi, 0
            jne     done
            mov     esi, vport
            fld     dword ptr [edi+0x40]
            fmul    dword ptr [esi+0x20]
            fld     dword ptr [edi+0x44]
            fmul    dword ptr [esi+0x24]
            fld     dword ptr [edi+0x48]
            fmul    dword ptr [esi+0x28]
            fxch    st(2)
            fmul    dword ptr [edi+0xc]
            fxch    st(1)
            fmul    dword ptr [edi+0xc]
            fxch    st(2)
            fmul    dword ptr [edi+0xc]
            fxch    st(1)
            fadd    dword ptr [esi+0x30]
            fxch    st(2)
            fadd    dword ptr [esi+0x34]
            fxch    st(1)
            fadd    dword ptr [esi+0x38]
            fxch    st(2)
            fstp    dword ptr [edi]
            fstp    dword ptr [edi+0x4]
            fstp    dword ptr [edi+0x8]
        }
    done:
        return result;
    } else {
        w = v->clip.w;
        x = v->clip.x;
        y = v->clip.y;
        z = v->clip.z;
        negW = -w;
        invW = (*(GLuint *)&w == 0x3f800000) ? 1.0f : 1.0f / w;
        clipcode = 0;
        if (w > 0.0f) {
            if (x < negW) clipcode |= 0x10000;
            else if (x > w) clipcode |= 0x20000;
            if (y < negW) clipcode |= 0x40000;
            else if (y > w) clipcode |= 0x80000;
            if (z < negW) clipcode |= 0x100000;
            else if (z > w) clipcode |= 0x200000;
        } else {
            if (x < negW) clipcode |= 0x10000;
            if (x > w) clipcode |= 0x20000;
            if (y < negW) clipcode |= 0x40000;
            if (y > w) clipcode |= 0x80000;
            if (z < negW) clipcode |= 0x100000;
            if (z > w) clipcode |= 0x200000;
        }
        v->flagBits |= 0x20;
        if (clipcode) {
            return clipcode;
        }
        winX = x * vport->scale.x * invW + vport->center.x;
        winY = y * vport->scale.y * invW + vport->center.y;
        wz = z * vport->scale.z * invW + vport->center.z;
        v->window.w = invW;
        v->window.x = winX;
        v->window.y = winY;
        v->window.z = wz;
    }
}
#pragma optimize("", on)

/* 690f44d0 -- s_clip.c clipProcs: the clip interpolators by what the vertices carry */
static __GLproc clipProcs[30] =
{
    (__GLproc)clip_Clip, (__GLproc)ClipI, (__GLproc)ClipC, (__GLproc)ClipBI, (__GLproc)ClipBC,
    (__GLproc)ClipF, (__GLproc)ClipIF, (__GLproc)ClipCF, (__GLproc)ClipBIF, (__GLproc)ClipBCF,
    (__GLproc)ClipT, (__GLproc)ClipIT, (__GLproc)ClipCT, (__GLproc)ClipBIT, (__GLproc)ClipBCT,
    (__GLproc)ClipFT, (__GLproc)ClipIFT, (__GLproc)ClipCFT, (__GLproc)ClipBIFT, (__GLproc)ClipBCFT,
    (__GLproc)ClipT2, (__GLproc)ClipIT2, (__GLproc)ClipCT2, (__GLproc)ClipBIT2, (__GLproc)ClipBCT2,
    (__GLproc)ClipFT2, (__GLproc)ClipIFT2, (__GLproc)ClipCFT2, (__GLproc)ClipBIFT2, (__GLproc)ClipBCFT2,
};

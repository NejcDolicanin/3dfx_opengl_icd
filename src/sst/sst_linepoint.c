/*
** sst_linepoint.c -- 3dfx back end, original VA range 6905c000..69064000: an og emitter of the clip interpolation,
** the line / point draw procs (antialiased lines as Glide AA triangles) and two og add / sub emitters.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "icdprotos.h"
#include <glide.h>
#include "og.h"            /* the og records, emitters and R() / IMM() */

/* ---- forward declarations ---- */
GLubyte *__fastcall og_lerpFloat(__GLcontext *gc, GLubyte *pc, OgOperand *disp);
double __fastcall dotProduct3d(const __GLcoord *a, const __GLcoord *b);
GLubyte *__fastcall og_addEdx3(__GLcontext *gc, GLubyte *pc, OgOperand *disp);
GLubyte *__fastcall og_subEdx3(__GLcontext *gc, GLubyte *pc, OgOperand *disp);

/* 6905c220 -- og_lerpFloat: emit the interpolation of one float at disp: [edi + disp] = [eax + disp] * t1 + [ebx +
** disp] * t0, with the weights in xmm1 / xmm0 (SSE, cpuCaps & 4), mm1 / mm0 (3DNow, & 2) or st(1) / st(2) (x87) */
#pragma optimize("", off)       /* compiled without optimisation (/Od) in the original */
GLubyte *__fastcall og_lerpFloat(__GLcontext *gc, GLubyte *pc, OgOperand *disp)
{
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        pc = og_encode(gc, pc, og_movss(gc, R(mm3), og_mem(gc, disp, &og_eax, 0, 1)));
        pc = og_encode(gc, pc, og_mulss(gc, &og_mm3, &og_mm1));
        pc = og_encode(gc, pc, og_movss(gc, R(mm4), og_mem(gc, disp, &og_ebx, 0, 1)));
        pc = og_encode(gc, pc, og_mulss(gc, &og_mm4, &og_mm0));
        pc = og_encode(gc, pc, og_addss(gc, &og_mm3, &og_mm4));
        pc = og_encode(gc, pc, og_movss(gc, og_mem(gc, disp, &og_edi, 0, 1), R(mm3)));
    } else if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
        pc = og_encode(gc, pc, og_movd(gc, R(mm3), og_mem(gc, disp, &og_eax, 0, 1)));
        pc = og_encode(gc, pc, og_pfmul(gc, &og_mm3, &og_mm1));
        pc = og_encode(gc, pc, og_movd(gc, R(mm4), og_mem(gc, disp, &og_ebx, 0, 1)));
        pc = og_encode(gc, pc, og_pfmul(gc, &og_mm4, &og_mm0));
        pc = og_encode(gc, pc, og_pfadd(gc, &og_mm3, &og_mm4));
        pc = og_encode(gc, pc, og_movd(gc, og_mem(gc, disp, &og_edi, 0, 1), R(mm3)));
    } else {
        pc = og_encode(gc, pc, og_fld(gc, og_mem(gc, disp, &og_eax, 0, 1)));
        pc = og_encode(gc, pc, og_fmul(gc, &og_st1));
        pc = og_encode(gc, pc, og_fld(gc, og_mem(gc, disp, &og_ebx, 0, 1)));
        pc = og_encode(gc, pc, og_fmul(gc, &og_st3));
        pc = og_encode(gc, pc, og_faddp(gc, &og_st1, &og_st0));
        pc = og_encode(gc, pc, og_fstp(gc, og_mem(gc, disp, &og_edi, 0, 1)));
    }
    return pc;
}
#pragma optimize("", on)

/* 6905c540 -- hwInitClipInterp: install the lazy builder of the generated clip interpolation proc */
void __fastcall hwInitClipInterp(__GLcontext *gc)
{
    gc->hwcx.ogLazyProcs[1] = (__GLproc)hwCalcColorFront;
}

/* 6905f180 -- dotProduct3d: the dot product of two 3-vectors, in double */
double __fastcall dotProduct3d(const __GLcoord *a, const __GLcoord *b)
{
    return (double)a->x * (double)b->x + (double)a->y * (double)b->y + (double)a->z * (double)b->z;
}

/* 6905f590 -- __glSSTRenderLine: a smooth-shaded line, grDrawLine(a, b) (SSTLINE.C __glSSTRenderLine) */
void __fastcall __glSSTRenderLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b)
{
    grDrawLine(a, b);
}

/* 6905f5c0 -- __glSSTRenderAntiAliasedFlatLine: draw an antialiased flat-shaded line: the quad of width smoothWidth
** around it as two Glide AA triangles (only the outer edges antialiased); a takes b's colour meanwhile, its position
** is restored */
void __fastcall __glSSTRenderAntiAliasedFlatLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b)
{
    __GLfloat color;
    __GLfloat half, dx, dy, len, ox, oy;
    __GLvertexICD a2, b2;

    half = gc->state.line.smoothWidth * 0.5f;
    dx = b->window.x - a->window.x;
    dy = b->window.y - a->window.y;
    color = *(__GLfloat *)&a->packedColor;
    *(__GLfloat *)&a->packedColor = *(__GLfloat *)&b->packedColor;
    a2 = *a;
    len = (__GLfloat)sqrt(dx * dx + dy * dy);
    b2 = *b;
    ox = -((dy / len) * half);
    oy = -((dx / len) * half);
    a->window.x = ox + a->window.x;
    a->window.y = a->window.y - oy;
    a2.window.x = a2.window.x - ox;
    a2.window.y = a2.window.y + oy;
    b->window.x = ox + b->window.x;
    b->window.y = b->window.y - oy;
    b2.window.x = b2.window.x - ox;
    b2.window.y = b2.window.y + oy;
    grAADrawTriangle(&a2, a, b, FXTRUE, FXTRUE, FXFALSE);
    grAADrawTriangle(&a2, b, &b2, FXFALSE, FXTRUE, FXTRUE);
    *(__GLfloat *)&a->packedColor = color;
    a->window.x = a->window.x - ox;
    a->window.y = oy + a->window.y;
    b->window.x = b->window.x - ox;
    b->window.y = oy + b->window.y;
}

/* 6905f710 -- __glSSTRenderAntiAliasedLine: draw an antialiased smooth-shaded line (as 6905f5c0 without the colour
** swap) */
void __fastcall __glSSTRenderAntiAliasedLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b)
{
    __GLfloat half, dx, dy, len, ox, oy;
    __GLvertexICD a2, b2;

    half = gc->state.line.smoothWidth * 0.5f;
    dx = b->window.x - a->window.x;
    dy = b->window.y - a->window.y;
    a2 = *a;
    len = (__GLfloat)sqrt(dx * dx + dy * dy);
    b2 = *b;
    ox = -((dy / len) * half);
    oy = -((dx / len) * half);
    a->window.x = ox + a->window.x;
    a->window.y = a->window.y - oy;
    a2.window.x = a2.window.x - ox;
    a2.window.y = a2.window.y + oy;
    b->window.x = ox + b->window.x;
    b->window.y = b->window.y - oy;
    b2.window.x = b2.window.x - ox;
    b2.window.y = b2.window.y + oy;
    grAADrawTriangle(&a2, a, b, FXTRUE, FXTRUE, FXFALSE);
    grAADrawTriangle(&a2, b, &b2, FXFALSE, FXTRUE, FXTRUE);
    a->window.x = a->window.x - ox;
    a->window.y = oy + a->window.y;
    b->window.x = b->window.x - ox;
    b->window.y = oy + b->window.y;
}

/* 6905ff30 -- __glSSTRenderPoint: a size-1 point, grDrawPoint(v) (SSTPOINT.C __glSSTRenderPoint) */
void __fastcall __glSSTRenderPoint(__GLcontext *gc, __GLvertexICD *v)
{
    grDrawPoint(v);
}

/* 690621d0 -- og_addEdx3: emit `add [eax + disp], edx; add [ebx + disp], edx; add [ecx + disp], edx` (the three
** vertices' field at disp) */
GLubyte *__fastcall og_addEdx3(__GLcontext *gc, GLubyte *pc, OgOperand *disp)
{
    pc = og_encode(gc, pc, og_add(gc, og_mem(gc, disp, &og_eax, 0, 1), &og_edx));
    pc = og_encode(gc, pc, og_add(gc, og_mem(gc, disp, &og_ebx, 0, 1), &og_edx));
    return og_encode(gc, pc, og_add(gc, og_mem(gc, disp, &og_ecx, 0, 1), &og_edx));
}

/* 69062270 -- og_subEdx3: emit `sub [eax + disp], edx; sub [ebx + disp], edx; sub [ecx + disp], edx` */
GLubyte *__fastcall og_subEdx3(__GLcontext *gc, GLubyte *pc, OgOperand *disp)
{
    pc = og_encode(gc, pc, og_sub(gc, og_mem(gc, disp, &og_eax, 0, 1), &og_edx));
    pc = og_encode(gc, pc, og_sub(gc, og_mem(gc, disp, &og_ebx, 0, 1), &og_edx));
    return og_encode(gc, pc, og_sub(gc, og_mem(gc, disp, &og_ecx, 0, 1), &og_edx));
}

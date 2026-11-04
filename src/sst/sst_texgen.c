/*
** sst_texgen.c -- og_windowTransform: the og_* generator of the window transform at the end of the
** immediate-mode vertex proc: the clip coordinates times 1/w, the texture coordinates times 1/w,
** then the viewport scale and center (3DNow, SSE and x87 versions).
*/
#include <stddef.h>
#include "wglpriv.h"
#include "og.h"
#include <glide.h>

/* ---- og_* code generator (og.h) ---- */


/* the generated code's registers: ebp = the __GLvertexICD, esi = its flagBits, ecx = &gc->state.viewport */
#define VX(f)           offsetof(__GLvertexICD, f)
#define VP(f)           offsetof(__GLviewport, f)
#define CLIP_NEAR       0x100000        /* flagBits: behind the near plane (SI __GL_CLIP_NEAR): the
                                           texture coordinates are not divided by w (as hwTexCoords) */

/* 690225f0 -- og_windowTransform: window = clip * window.w * viewport.scale + viewport.center for the vertex in
   ebp; unless CLIP_NEAR the enabled units' s, t (and q when hwcx.nonUnityQ) are multiplied by window.w too.
   x87: 1/w comes in st0 and stays there for the caller's fstp to window.w; SSE: in xmm1, stored here;
   3DNow: read from window.w.  Returns the new pc. */
GLubyte *__fastcall og_windowTransform(__GLcontext *gc, GLubyte *pc)
{
    OgLabel *texDiv, *toWindow, *toWindow3dn;

    texDiv = og_newFixup(gc);
    toWindow = og_newFixup(gc);
    toWindow3dn = og_newFixup(gc);

    if (gc->constants.cpuCaps & __GL_CPU_3DNOW) {
        /* 3DNow: mm0 = clip x, y * 1/w, mm1 = clip z * 1/w */
        EMIT(og_femms(gc));
        EMIT(og_movq(gc, R(mm0), MEMR(VX(clip.x), ebp)));
        EMIT(og_movd(gc, R(mm1), MEMR(VX(clip.z), ebp)));
        EMIT(og_movd(gc, R(mm2), MEMR(VX(window.w), ebp)));
        EMIT(og_punpckldq(gc, R(mm2), R(mm2)));
        EMIT(og_mov(gc, R(ecx), IMM(&gc->state.viewport)));
        EMIT(og_pfmul(gc, R(mm0), R(mm2)));
        EMIT(og_pfmul(gc, R(mm1), R(mm2)));
        EMIT(og_test(gc, R(esi), IMM(CLIP_NEAR)));
        EMIT(og_jne(gc, toWindow3dn));
        if (gc->texture.textureEnabled[0]) {
            EMIT(og_movq(gc, R(mm3), MEMR(VX(texture[0].x), ebp)));
            EMIT(og_pfmul(gc, R(mm3), R(mm2)));
            EMIT(og_movq(gc, MEMR(VX(texture[0].x), ebp), R(mm3)));
            if (gc->hwcx.nonUnityQ) {
                EMIT(og_movd(gc, R(mm4), MEMR(VX(texture[0].w), ebp)));
                EMIT(og_pfmul(gc, R(mm4), R(mm2)));
                EMIT(og_movd(gc, MEMR(VX(texture[0].w), ebp), R(mm4)));
            }
        }
        if (gc->texture.textureEnabled[1]) {
            EMIT(og_movq(gc, R(mm3), MEMR(VX(texture[1].x), ebp)));
            EMIT(og_pfmul(gc, R(mm3), R(mm2)));
            EMIT(og_movq(gc, MEMR(VX(texture[1].x), ebp), R(mm3)));
            if (gc->hwcx.nonUnityQ) {
                EMIT(og_movd(gc, R(mm4), MEMR(VX(texture[1].w), ebp)));
                EMIT(og_pfmul(gc, R(mm4), R(mm2)));
                EMIT(og_movd(gc, MEMR(VX(texture[1].w), ebp), R(mm4)));
            }
        }
        LABEL(toWindow3dn);
        EMIT(og_movq(gc, R(mm4), MEMR(VP(scale.x), ecx)));
        EMIT(og_movq(gc, R(mm3), MEMR(VP(center.x), ecx)));
        EMIT(og_pfmul(gc, R(mm0), R(mm4)));
        EMIT(og_pfadd(gc, R(mm0), R(mm3)));
        EMIT(og_movq(gc, MEMR(VX(window.x), ebp), R(mm0)));
        EMIT(og_movd(gc, R(mm4), MEMR(VP(scale.z), ecx)));
        EMIT(og_movd(gc, R(mm3), MEMR(VP(center.z), ecx)));
        EMIT(og_pfmul(gc, R(mm1), R(mm4)));
        EMIT(og_pfadd(gc, R(mm1), R(mm3)));
        EMIT(og_movd(gc, MEMR(VX(window.z), ebp), R(mm1)));
        return og_encode(gc, pc, og_femms(gc));
    }
    if (gc->constants.cpuCaps & __GL_CPU_SSE) {
        /* SSE: xmm1 = 1/w broadcast, xmm0 = clip * 1/w; s, t, r, q of the enabled units (q regardless
           of nonUnityQ) */
        EMIT(og_shufps(gc, R(mm1), R(mm1), IMM(0)));
        EMIT(og_mov(gc, R(ecx), IMM(&gc->state.viewport)));
        EMIT(og_movaps(gc, R(mm0), MEMR(VX(clip), ebp)));
        EMIT(og_movups(gc, R(mm4), MEMR(VP(scale), ecx)));
        EMIT(og_mulps(gc, R(mm0), R(mm1)));
        EMIT(og_test(gc, R(esi), IMM(CLIP_NEAR)));
        EMIT(og_jne(gc, toWindow));
        if (gc->texture.textureEnabled[0]) {
            EMIT(og_movaps(gc, R(mm2), MEMR(VX(texture[0]), ebp)));
            EMIT(og_mulps(gc, R(mm2), R(mm1)));
        }
        if (gc->texture.textureEnabled[1]) {
            EMIT(og_movaps(gc, R(mm3), MEMR(VX(texture[1]), ebp)));
            EMIT(og_mulps(gc, R(mm3), R(mm1)));
        }
        if (gc->texture.textureEnabled[0])
            EMIT(og_movaps(gc, MEMR(VX(texture[0]), ebp), R(mm2)));
        if (gc->texture.textureEnabled[1])
            EMIT(og_movaps(gc, MEMR(VX(texture[1]), ebp), R(mm3)));
        LABEL(toWindow);
        EMIT(og_mulps(gc, R(mm0), R(mm4)));
        EMIT(og_movups(gc, R(mm5), MEMR(VP(center), ecx)));
        EMIT(og_addps(gc, R(mm0), R(mm5)));
        EMIT(og_movaps(gc, MEMR(VX(window), ebp), R(mm0)));                     /* w: clip.w * 1/w * scale.w + center.w, */
        return og_encode(gc, pc, og_movss(gc, MEMR(VX(window.w), ebp), R(mm1)));  /* overwritten with 1/w */
    }

    /* x87, st0 = 1/w.  CLIP_NEAR: clip y, z, x * 1/w.  The last fmulp overwrites 1/w with x, so this path
       leaves three values instead of four and the caller's fstp to window.w pops an empty register
       (original bug: window.w of a near-clipped vertex gets the x87 indefinite) */
    EMIT(og_test(gc, R(esi), IMM(CLIP_NEAR)));
    EMIT(og_je(gc, texDiv));
    EMIT(og_fld(gc, MEMR(VX(clip.y), ebp)));
    EMIT(og_fmul(gc, R(st1)));
    EMIT(og_fld(gc, MEMR(VX(clip.z), ebp)));
    EMIT(og_fmul(gc, R(st2)));
    EMIT(og_fld(gc, MEMR(VX(clip.x), ebp)));
    EMIT(og_fmulp(gc, R(st3), R(st0)));
    EMIT(og_jmp(gc, toWindow));

    /* the texture coordinates and clip z, y, x times 1/w, each texture value stored as soon as the
       stack order allows; every case ends with st0..st3 = z, y, x, 1/w */
    LABEL(texDiv);
    if (gc->texture.textureEnabled[0]) {
        if (gc->texture.textureEnabled[1]) {
            if (gc->hwcx.nonUnityQ) {
                EMIT(og_fld(gc, MEMR(VX(texture[0].x), ebp)));
                EMIT(og_fmul(gc, R(st1)));
                EMIT(og_fld(gc, MEMR(VX(texture[0].y), ebp)));
                EMIT(og_fmul(gc, R(st2)));
                EMIT(og_fld(gc, MEMR(VX(texture[0].w), ebp)));
                EMIT(og_fmul(gc, R(st3)));
                EMIT(og_fld(gc, MEMR(VX(clip.z), ebp)));
                EMIT(og_fmul(gc, R(st4)));
                EMIT(og_fld(gc, MEMR(VX(clip.y), ebp)));
                EMIT(og_fmul(gc, R(st5)));
                EMIT(og_fld(gc, MEMR(VX(clip.x), ebp)));
                EMIT(og_fmul(gc, R(st6)));
                EMIT(og_fxch(gc, R(st5)));
                EMIT(og_fstp(gc, MEMR(VX(texture[0].x), ebp)));
                EMIT(og_fxch(gc, R(st3)));
                EMIT(og_fstp(gc, MEMR(VX(texture[0].y), ebp)));
                EMIT(og_fxch(gc, R(st1)));
                EMIT(og_fstp(gc, MEMR(VX(texture[0].w), ebp)));
                EMIT(og_fld(gc, MEMR(VX(texture[1].x), ebp)));
                EMIT(og_fmul(gc, R(st4)));
                EMIT(og_fld(gc, MEMR(VX(texture[1].y), ebp)));
                EMIT(og_fmul(gc, R(st5)));
                EMIT(og_fld(gc, MEMR(VX(texture[1].w), ebp)));
                EMIT(og_fmul(gc, R(st6)));
                EMIT(og_fxch(gc, R(st2)));
                EMIT(og_fstp(gc, MEMR(VX(texture[1].x), ebp)));
                EMIT(og_fstp(gc, MEMR(VX(texture[1].y), ebp)));
                EMIT(og_fstp(gc, MEMR(VX(texture[1].w), ebp)));
            } else {
                EMIT(og_fld(gc, MEMR(VX(texture[0].x), ebp)));
                EMIT(og_fmul(gc, R(st1)));
                EMIT(og_fld(gc, MEMR(VX(texture[0].y), ebp)));
                EMIT(og_fmul(gc, R(st2)));
                EMIT(og_fld(gc, MEMR(VX(clip.z), ebp)));
                EMIT(og_fmul(gc, R(st3)));
                EMIT(og_fld(gc, MEMR(VX(clip.y), ebp)));
                EMIT(og_fmul(gc, R(st4)));
                EMIT(og_fld(gc, MEMR(VX(clip.x), ebp)));
                EMIT(og_fmul(gc, R(st5)));
                EMIT(og_fxch(gc, R(st4)));
                EMIT(og_fstp(gc, MEMR(VX(texture[0].x), ebp)));
                EMIT(og_fxch(gc, R(st2)));
                EMIT(og_fstp(gc, MEMR(VX(texture[0].y), ebp)));
                EMIT(og_fld(gc, MEMR(VX(texture[1].x), ebp)));
                EMIT(og_fmul(gc, R(st4)));
                EMIT(og_fld(gc, MEMR(VX(texture[1].y), ebp)));
                EMIT(og_fmul(gc, R(st5)));
                EMIT(og_fxch(gc, R(st1)));
                EMIT(og_fstp(gc, MEMR(VX(texture[1].x), ebp)));
                EMIT(og_fstp(gc, MEMR(VX(texture[1].y), ebp)));
            }
        } else {
            if (gc->hwcx.nonUnityQ) {
                EMIT(og_fld(gc, MEMR(VX(texture[0].x), ebp)));
                EMIT(og_fmul(gc, R(st1)));
                EMIT(og_fld(gc, MEMR(VX(texture[0].y), ebp)));
                EMIT(og_fmul(gc, R(st2)));
                EMIT(og_fld(gc, MEMR(VX(texture[0].w), ebp)));
                EMIT(og_fmul(gc, R(st3)));
                EMIT(og_fld(gc, MEMR(VX(clip.z), ebp)));
                EMIT(og_fmul(gc, R(st4)));
                EMIT(og_fld(gc, MEMR(VX(clip.y), ebp)));
                EMIT(og_fmul(gc, R(st5)));
                EMIT(og_fld(gc, MEMR(VX(clip.x), ebp)));
                EMIT(og_fmul(gc, R(st6)));
                EMIT(og_fxch(gc, R(st5)));
                EMIT(og_fstp(gc, MEMR(VX(texture[0].x), ebp)));
                EMIT(og_fxch(gc, R(st3)));
                EMIT(og_fstp(gc, MEMR(VX(texture[0].y), ebp)));
                EMIT(og_fxch(gc, R(st1)));
                EMIT(og_fstp(gc, MEMR(VX(texture[0].w), ebp)));
            } else {
                EMIT(og_fld(gc, MEMR(VX(texture[0].x), ebp)));
                EMIT(og_fmul(gc, R(st1)));
                EMIT(og_fld(gc, MEMR(VX(texture[0].y), ebp)));
                EMIT(og_fmul(gc, R(st2)));
                EMIT(og_fld(gc, MEMR(VX(clip.z), ebp)));
                EMIT(og_fmul(gc, R(st3)));
                EMIT(og_fld(gc, MEMR(VX(clip.y), ebp)));
                EMIT(og_fmul(gc, R(st4)));
                EMIT(og_fld(gc, MEMR(VX(clip.x), ebp)));
                EMIT(og_fmul(gc, R(st5)));
                EMIT(og_fxch(gc, R(st4)));
                EMIT(og_fstp(gc, MEMR(VX(texture[0].x), ebp)));
                EMIT(og_fxch(gc, R(st2)));
                EMIT(og_fstp(gc, MEMR(VX(texture[0].y), ebp)));
            }
        }
    } else {
        if (gc->texture.textureEnabled[1]) {
            /* unit 1 alone: q is divided whether or not nonUnityQ */
            EMIT(og_fld(gc, MEMR(VX(texture[1].x), ebp)));
            EMIT(og_fmul(gc, R(st1)));
            EMIT(og_fld(gc, MEMR(VX(texture[1].y), ebp)));
            EMIT(og_fmul(gc, R(st2)));
            EMIT(og_fld(gc, MEMR(VX(texture[1].w), ebp)));
            EMIT(og_fmul(gc, R(st3)));
            EMIT(og_fld(gc, MEMR(VX(clip.z), ebp)));
            EMIT(og_fmul(gc, R(st4)));
            EMIT(og_fld(gc, MEMR(VX(clip.y), ebp)));
            EMIT(og_fmul(gc, R(st5)));
            EMIT(og_fld(gc, MEMR(VX(clip.x), ebp)));
            EMIT(og_fmul(gc, R(st6)));
            EMIT(og_fxch(gc, R(st5)));
            EMIT(og_fstp(gc, MEMR(VX(texture[1].x), ebp)));
            EMIT(og_fxch(gc, R(st3)));
            EMIT(og_fstp(gc, MEMR(VX(texture[1].y), ebp)));
            EMIT(og_fxch(gc, R(st1)));
            EMIT(og_fstp(gc, MEMR(VX(texture[1].w), ebp)));
        } else {
            EMIT(og_fld(gc, MEMR(VX(clip.z), ebp)));
            EMIT(og_fmul(gc, R(st1)));
            EMIT(og_fld(gc, MEMR(VX(clip.y), ebp)));
            EMIT(og_fmul(gc, R(st2)));
            EMIT(og_fld(gc, MEMR(VX(clip.x), ebp)));
            EMIT(og_fmul(gc, R(st3)));
            EMIT(og_fxch(gc, R(st2)));
        }
    }

    /* st0..st2 = z, y, x: times the scale, plus the center, stored to window z, x, y */
    LABEL(toWindow);
    EMIT(og_mov(gc, R(ecx), IMM(&gc->state.viewport)));
    EMIT(og_fld(gc, MEMR(VP(scale.z), ecx)));
    EMIT(og_fmulp(gc, R(st1), R(st0)));
    EMIT(og_fld(gc, MEMR(VP(scale.y), ecx)));
    EMIT(og_fmulp(gc, R(st2), R(st0)));
    EMIT(og_fld(gc, MEMR(VP(scale.x), ecx)));
    EMIT(og_fmulp(gc, R(st3), R(st0)));
    EMIT(og_fadd(gc, MEMR(VP(center.z), ecx)));
    EMIT(og_fxch(gc, R(st2)));
    EMIT(og_fadd(gc, MEMR(VP(center.x), ecx)));
    EMIT(og_fxch(gc, R(st1)));
    EMIT(og_fadd(gc, MEMR(VP(center.y), ecx)));
    EMIT(og_fxch(gc, R(st2)));
    EMIT(og_fstp(gc, MEMR(VX(window.z), ebp)));
    EMIT(og_fstp(gc, MEMR(VX(window.x), ebp)));
    return og_encode(gc, pc, og_fstp(gc, MEMR(VX(window.y), ebp)));
}

/*
** gl_memclr.c -- fmemclr.c (3dfx GLIDE3X/GLCORE/FMEMCLR.C): the fast rectangle fills of the clear procs.
*/
#include <stdlib.h>
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "glshade.h"
#include "icdprotos.h"
#include <math.h>

extern void __fastcall fillRectDwords(__GLcontext *gc, GLuint *dst, GLuint w, GLint h, GLint skip, GLuint value);

/* 690e5370 -- __glFillRect16: fill a w x h rectangle of 16-bit pixels (rows `skip` bytes apart, 0 = one run of
** w * h) with `value`: an unaligned head column / odd tail column by hand, the dword-aligned middle
** through the dword fill 690e51f0 (value: the 16-bit pixel in the low half of its argument slot) */
void __fastcall __glFillRect16(__GLcontext *gc, GLushort *dst, GLint w, GLint h, GLint skip, GLuint value)
{
    GLint n, rows;
    GLushort *p;

    if (skip == 0) {
	n = w * h;
	if ((GLuint)dst & 2) {
	    *dst++ = (GLushort)value;
	    n--;
	}
	if (n & 1) {
	    dst[n - 1] = (GLushort)value;
	    n--;
	}
	if (n > 0) {
	    fillRectDwords(gc, (GLuint *)dst, n >> 1, 1, 0, (value << 16) | (value & 0xffff));
	}
    } else {
	if ((GLuint)dst & 2) {
	    p = dst;
	    rows = h;
	    do {
		*p = (GLushort)value;
		p = (GLushort *)((GLubyte *)p + skip + w * 2);
	    } while (--rows);
	    dst++;
	    skip += 2;
	    w--;
	}
	if (w & 1) {
	    p = dst + (w - 1);
	    rows = h;
	    do {
		*p = (GLushort)value;
		p = (GLushort *)((GLubyte *)p + skip + w * 2);
	    } while (--rows);
	    skip += 2;
	    w--;
	}
	if (w > 0) {
	    fillRectDwords(gc, (GLuint *)dst, w >> 1, h, skip, (value << 16) | (value & 0xffff));
	}
    }
}

/* byte masks for a partial dword: keep the low i bytes (s_clear.c-style) */
/* 690f5a90 -- the bytes a partial dword keeps, by the byte count */
static GLuint __glByteKeepMask[4] = { 0, 0xff, 0xffff, 0xffffff };

/*
** 690e51f0 -- fillRectDwords: fill a w x h rectangle of dwords (rows `skip` bytes apart, 0 = one contiguous run of
** w * h) with `value`.  When value is a well-behaved double half (0, or an exponent field that is
** neither denormal nor NaN when the dword pair is loaded as a double) the runs are written with
** 8-byte FST stores after a 4-byte head, else with eight dword stores per step.  Unoptimised in the
** original; the loops are inline assembly.  `unused` (ECX) is the caller's context word.
*/
#pragma optimize("", off)
void __fastcall fillRectDwords(__GLcontext *gc, GLuint *dst, GLuint w, GLint h, GLint skip, GLuint value)
{
    GLuint count;                   /* the name decides its /Od frame slot (VC6's symbol hash): `n` swaps it with n8 */
    GLuint n8;

    if (value == 0 || ((value & 0x7ff00000) <= 0x7fc00000 && (value & 0x7ff00000) >= 0x100000)) {
        if (skip == 0) {
            count = w * h;
            if (count == 0) {
                return;
            }
            __asm {
                mov     eax, value
                mov     edi, dst
                mov     ecx, count
                test    edi, 4
                push    eax
                push    eax
                je      aligned1
                mov     [edi], eax
                add     edi, 4
                dec     ecx
            aligned1:
                mov     edx, ecx
                fld     qword ptr [esp]
                shr     ecx, 1
                je      tail1
            loop1:
                fst     qword ptr [edi]
                add     edi, 8
                dec     ecx
                jne     loop1
            tail1:
                test    edx, 1
                je      done1
                mov     [edi], eax
            done1:
                fstp    qword ptr [esp]
                pop     eax
                pop     eax
            }
        } else {
            __asm {
                mov     eax, value
                mov     edi, dst
                mov     esi, skip
                mov     edx, h
                push    eax
                push    eax
                fld     qword ptr [esp]
            row2:
                mov     ecx, w
                test    edi, 4
                je      aligned2
                mov     [edi], eax
                dec     ecx
                add     edi, 4
            aligned2:
                mov     ebx, ecx
                shr     ecx, 1
                je      tail2
            loop2:
                fst     qword ptr [edi]
                add     edi, 8
                dec     ecx
                jne     loop2
            tail2:
                test    ebx, 1
                je      next2
                mov     [edi], eax
                add     edi, 4
            next2:
                add     edi, esi
                dec     edx
                jne     row2
                fstp    qword ptr [esp]
                pop     eax
                pop     eax
            }
        }
    } else {
        if (skip == 0) {
            n8 = w * h;
            __asm {
                mov     edi, dst
                mov     eax, value
                mov     ecx, n8
                and     ecx, 7
                je      blocks3
            single3:
                mov     [edi], eax
                add     edi, 4
                dec     ecx
                jne     single3
            blocks3:
                mov     ecx, n8
                shr     ecx, 3
                je      done3
            block3:
                mov     [edi+4], eax
                mov     [edi], eax
                mov     [edi+8], eax
                mov     [edi+0xc], eax
                mov     [edi+0x10], eax
                mov     [edi+0x14], eax
                mov     [edi+0x18], eax
                mov     [edi+0x1c], eax
                add     edi, 0x20
                dec     ecx
                jne     block3
            done3:
            }
        } else {
            __asm {
                mov     eax, value
                mov     edi, dst
                mov     esi, skip
                mov     edx, h
            row4:
                mov     ecx, w
                and     ecx, 7
                je      blocks4
            single4:
                mov     [edi], eax
                add     edi, 4
                dec     ecx
                jne     single4
            blocks4:
                mov     ecx, w
                shr     ecx, 3
                je      next4
            block4:
                mov     [edi+4], eax
                mov     [edi], eax
                mov     [edi+8], eax
                mov     [edi+0xc], eax
                mov     [edi+0x10], eax
                mov     [edi+0x14], eax
                mov     [edi+0x18], eax
                mov     [edi+0x1c], eax
                add     edi, 0x20
                dec     ecx
                jne     block4
            next4:
                add     edi, esi
                dec     edx
                jne     row4
            }
        }
    }
}
#pragma optimize("", on)

/*
** 690e5440 -- FastMemClear1 (3dfx GLIDE3X/GLCORE/FMEMCLR.C): fill a w x h rectangle of bytes
** (rows `skip` bytes apart, 0 = one run of w * h) with the low byte of `val` (the CI8 clearRect
** of 690e3490): the unaligned head and tail are merged into their dwords through the mask table
** (arrayMask = __glByteKeepMask), the aligned middle goes to FastMemClear4 (690e51f0).
*/
void __fastcall FastMemClear1(__GLcontext *gc, GLubyte *cptr, GLint w, GLint h, GLint skip, GLuint val)
{
    unsigned long val4 = (val&0xff) | ((val&0xff)<<8) |
                         ((val&0xff) << 16) | ((val&0xff) << 24);

    if(skip == 0) {
        /* Contiguous array */
        int count = w*h;
        int tmpcnt;

        if(count > 3) {
            /* Leading, trailing masks cannot intersect */

            /* Check to see if we need to trim leading bytes. */
            if(tmpcnt = (((unsigned long) cptr)&3)) {
                unsigned long arrayM = __glByteKeepMask[tmpcnt];
                unsigned long *ptr = (unsigned long *)(((unsigned int) cptr) & (~3));

                *ptr = ((*ptr)&arrayM) | ((~arrayM) & val4);
                tmpcnt = 4 - tmpcnt;
                cptr += tmpcnt;
                count -= tmpcnt;
            }

            /* Check to see if we need to trim trailing bytes. */
            if(tmpcnt = (count&3)) {
                unsigned long arrayM = ~__glByteKeepMask[tmpcnt];
                unsigned long *ptr = (unsigned long *)(cptr + count - tmpcnt);

                *ptr = ((*ptr)&arrayM) | ((~arrayM) & val4);
                count -= tmpcnt;
            }

            if(count > 0) {
                fillRectDwords(gc, (GLuint *)cptr, count>>2, 1, 0, val4);
            }
        } else {
            /* Leading, trailing masks might intersect */

            /* Just do simple loop */
            do {
                *cptr++ = (unsigned char)val;
            } while(--count);
        }

    } else {
        /* Non-contiguous array */
        unsigned int tmpcnt;

        if(w > 3) {
            /* Leading, trailing masks cannot intersect */

            /* Check to see if we need to trim off leading columns */
            if(tmpcnt = (((unsigned int) cptr)&3)) {
                unsigned long arrayM = __glByteKeepMask[tmpcnt];
                unsigned long mval = ((~arrayM) & val4);
                unsigned long *ptr = (unsigned long *)(((unsigned int) cptr) & (~3));
                int count = h;
                unsigned int pitch = w + skip;

                do {
                    *ptr = ((*ptr)&arrayM) | mval;
                    ((unsigned char *)ptr) += pitch;
                } while(--count);

                tmpcnt = 4 - tmpcnt;
                cptr += tmpcnt;
                w -= tmpcnt;
                skip += tmpcnt;
            }

            /* Check to see if we need to trim off trailing columns */
            if(tmpcnt = (w & 3)) {
                unsigned long arrayM = ~__glByteKeepMask[tmpcnt];
                unsigned long mval = ((~arrayM) & val4);
                unsigned long *ptr = (unsigned long *)(cptr + w - tmpcnt);
                int count = h;
                unsigned int pitch = w + skip;

                w -= tmpcnt;
                skip += tmpcnt;
                do {
                    *ptr = ((*ptr)&arrayM) | mval;
                    ((unsigned char *)ptr) += pitch;
                } while(--count);
            }

            if(w > 0) {
                fillRectDwords(gc, (GLuint *)cptr, w>>2, h, skip, val4);
            }
        } else {
            /* Leading, trailing masks might intersect */
            int count;

            /* Just do loop */
            do {
                count = w;
                do {
                    *cptr++ = (unsigned char)val;
                } while(--count);
                cptr += skip;
            } while(--h);
        }
    }
}

/* ---- s_light.c: the ICD's light source machine (0x100 bytes, linked through next) ---- */
#ifndef __GL_SPOT_LOOKUP_TABLE_SIZE
#define __GL_SPOT_LOOKUP_TABLE_SIZE     256
#endif
/* ICD words of the material machine / state used by colour-index lighting */
#define cmapdMinusCmapa     cmapd_minus_cmapa   /* msm +0x58 */
#define cmapsMinusCmapa     cmaps_minus_cmapa   /* msm +0x5c */
#define MSM_F(m, f)      (*(__GLfloat *)&(m)->f)
#define INDEX_SHIFT(ms)  ((ms)->indexShift)   /* ms +0x54 */
#define NORMALIZE_3ARG( X, Y, Z )                     \
{                                                       \
    __GLfloat len = (X)*(X) + (Y)*(Y) + (Z)*(Z);        \
    if (len <= zero) {                                  \
        X = zero;                                       \
        Y = zero;                                       \
        Z = zero;                                       \
    }                                                   \
    else if (len != 1.0F) {                             \
        union { unsigned int i; float f; } seed;        \
        float xy, subexp;                               \
        seed.f = len;                                   \
        seed.i = 0x5f375a00u - (seed.i >> 1);           \
        xy = len * seed.f * seed.f;                     \
        subexp = 3.f - xy;                              \
        len = .0625f * seed.f * subexp * (12.f - xy * subexp * subexp); \
        X *= len;                                       \
        Y *= len;                                       \
        Z *= len;                                       \
    }                                                   \
}
#define NORMALIZE_6ARG( NX, NY, NZ, X, Y, Z )         \
{                                                       \
    __GLfloat len = (X)*(X) + (Y)*(Y) + (Z)*(Z);        \
    if (len <= zero) {                                  \
        NX = zero;                                      \
        NY = zero;                                      \
        NZ = zero;                                      \
    }                                                   \
    else if (len == 1.0F) {                             \
        NX = X;                                         \
        NY = Y;                                         \
        NZ = Z;                                         \
    }                                                   \
    else {                                              \
        union { unsigned int i; float f; } seed;        \
        float xy, subexp;                               \
        seed.f = len;                                   \
        seed.i = 0x5f375a00u - (seed.i >> 1);           \
        xy = len * seed.f * seed.f;                     \
        subexp = 3.f - xy;                              \
        len = .0625f * seed.f * subexp * (12.f - xy * subexp * subexp); \
        NX = X * len;                                   \
        NY = Y * len;                                   \
        NZ = Z * len;                                   \
    }                                                   \
}

/* s_light.c's colour-index calcColor proc lives here, out of its module: VC6 compiles it to the original's
** instruction order only directly after FastMemClear1 (code-generation state carries over from one function to
** the next; gl_light.c, or any other predecessor tried, reorders its x87 code). */
/*
** 69090fe0 -- __glCalcCIColor (s_light.c; gc->procs.calcColor in colour-index mode).  The SI routine
** with the light loop's fast path first, no zero test on the attenuation denominator, and an ICD
** epilogue: when the material's index shift (ms +0x54) or the colour index offset (*msm->indexOffsetPtr)
** is set, the lit index is shifted by it in 8.8 fixed point and the offset added.
*/
void __fastcall __glCalcCIColor(__GLcontext *gc, GLint face, __GLvertexICD *vx)
{
    __GLlightSourceMachineICD *lsm;
    __GLmaterialState *ms;
    __GLmaterialMachineICD *msm;
    __GLcolor *c;
    __GLfloat s, d, zero, nx, ny, nz;
    GLboolean eyeWIsZero, localViewer;

    /* Pick material to use */
    if (face == __GL_FRONTFACE) {
        c = &vx->colors[__GL_FRONTFACE];
        ms = &gc->state.light.front;
        msm = &gc->light.front;
        nx = vx->normal.x;
        ny = vx->normal.y;
        nz = vx->normal.z;
    } else {
        c = &vx->colors[__GL_BACKFACE];
        ms = &gc->state.light.back;
        msm = &gc->light.back;
        nx = -vx->normal.x;
        ny = -vx->normal.y;
        nz = -vx->normal.z;
    }

    zero = __GL_ZERO_F;
    s = zero;
    d = zero;
    lsm = (__GLlightSourceMachineICD *)gc->light.sources;
    eyeWIsZero = vx->eye.w == zero;
    localViewer = gc->state.light.model.localViewer;
    while (lsm) {
        if (!lsm->slowPath && !eyeWIsZero) {
            /* Fast lighting calculation */
            __GLfloat n1;

            n1 = nx * lsm->unitVPpli.x + ny * lsm->unitVPpli.y + nz * lsm->unitVPpli.z;
            if (n1 > zero) {
                __GLfloat n2;

                d += n1 * lsm->dli;
                n2 = nx * lsm->hHat.x + ny * lsm->hHat.y + nz * lsm->hHat.z;
                n2 -= msm->threshold;
                if (n2 >= zero) {
                    GLuint ix = (GLuint)(n2 * msm->scale + __glHalf);
                    if (ix < __GL_SPEC_LOOKUP_TABLE_SIZE) {
                        n2 = msm->specTable[ix];
                        s += n2 * lsm->sli;
                    } else {
                        s += lsm->sli;
                    }
                }
            }
        } else {
            __GLfloat att, attSpot;
            __GLcoord vPliHat, vPli, hHat;
            __GLfloat hv_x, hv_y, hv_z;

            Vvec(&vPli, &vx->eye, &lsm->position);
            NORMALIZE_6ARG( vPliHat.x, vPliHat.y, vPliHat.z, vPli.x, vPli.y, vPli.z );
            if (localViewer) {
                __GLfloat vPeHat_x, vPeHat_y, vPeHat_z;
                vPeHat_x = -vx->eye.x;
                vPeHat_y = -vx->eye.y;
                vPeHat_z = -vx->eye.z;
                NORMALIZE_3ARG( vPeHat_x, vPeHat_y, vPeHat_z );
                hv_x = vPliHat.x + vPeHat_x;
                hv_y = vPliHat.y + vPeHat_y;
                hv_z = vPliHat.z + vPeHat_z;
            } else {
                hv_x = vPliHat.x;
                hv_y = vPliHat.y;
                hv_z = vPliHat.z + __GL_ONE_F;
            }
            NORMALIZE_6ARG( hHat.x, hHat.y, hHat.z, hv_x, hv_y, hv_z );

            /* Compute attenuation */
            if (lsm->position.w != zero) {
                __GLfloat k0, k1, k2, dist;

                k0 = lsm->constantAttenuation;
                k1 = lsm->linearAttenuation;
                k2 = lsm->quadraticAttenuation;
                if ((k1 == zero) && (k2 == zero)) {
                    att = lsm->attenuation;
                } else {
                    dist = (__GLfloat)sqrt(vPli.x*vPli.x + vPli.y*vPli.y + vPli.z*vPli.z);
                    att = __GL_ONE_F / (k0 + (k1 + k2 * dist) * dist);
                }
            } else {
                att = __GL_ONE_F;
            }

            /* Compute spot effect if light is a spot light */
            attSpot = att;
            if (lsm->isSpot) {
                __GLfloat dot, px, py, pz;

                px = -vPliHat.x;
                py = -vPliHat.y;
                pz = -vPliHat.z;
                dot = px * lsm->direction.x + py * lsm->direction.y + pz * lsm->direction.z;
                if ((dot >= lsm->threshold) && (dot >= lsm->cosCutOffAngle)) {
                    GLuint ix = (GLuint)((dot - lsm->threshold) * lsm->scale + __glHalf);
                    if (ix < __GL_SPOT_LOOKUP_TABLE_SIZE) {
                        attSpot = att * lsm->spotTable[ix];
                    }
                } else {
                    attSpot = zero;
                }
            }

            /* Add in remaining effect of light, if any */
            if (attSpot) {
                __GLfloat n1;
                n1 = nx * vPliHat.x + ny * vPliHat.y + nz * vPliHat.z;
                if (n1 > zero) {
                    __GLfloat n2;

                    d += attSpot * n1 * lsm->dli;
                    n2 = nx * hHat.x + ny * hHat.y + nz * hHat.z;
                    n2 -= msm->threshold;
                    if (n2 >= zero) {
                        GLuint ix = (GLuint)(n2 * msm->scale + __glHalf);
                        if (ix < __GL_SPEC_LOOKUP_TABLE_SIZE) {
                            n2 = msm->specTable[ix];
                            s += attSpot * n2 * lsm->sli;
                        } else {
                            s += attSpot * lsm->sli;
                        }
                    }
                }
            }
        }
        lsm = lsm->next;
    }

    /* Compute final color */
    if (s >= __GL_ONE_F) {
        c->r = ms->cmaps;
    } else {
        __GLfloat ci;
        ci = ms->cmapa + (__GL_ONE_F - s) * d * MSM_F(msm, cmapdMinusCmapa) + s * *(__GLfloat *)&msm->cmapsMinusCmapa;
        if (ci > ms->cmaps) {
            ci = ms->cmaps;
        }
        c->r = ci;
    }
    if (INDEX_SHIFT(ms) != zero || *msm->indexOffsetPtr != zero) {
        GLint ci, shift, offset;
        ci = (GLint)(c->r * 256.0f + 0.5f);
        shift = (GLint)INDEX_SHIFT(ms);
        offset = (GLint)*msm->indexOffsetPtr;
        if (shift > 0) {
            ci <<= shift;
        } else {
            ci >>= -shift;
        }
        c->r = (__GLfloat)((offset << 8) + ci) * (1.0f / 256.0f);
    }
}

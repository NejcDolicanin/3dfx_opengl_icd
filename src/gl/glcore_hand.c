/*
** glcore_hand.c -- glcore functions that the WGL layer calls with hand-written prototypes
** (see wglpriv.h): the null-context allocator and the per-thread dispatch helpers.
*/
#include <string.h>
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"            /* __GLtexel */
#include "icdprotos.h"

extern __GLdispatchTable __glNopDispatchState;      /* gl_disp.c: the "no context" dispatch table */

/* 69078f50 -- __glCreateContext for the null context: calloc a core-only gc (0x35150 bytes, no
** WGL / hardware part) through the imports, copy the imports and the modes in and set the destroy
** export.
*/
void *__fastcall __glCreateContext(void *imports, const __WGLmodes *m)
{
    /* imports calloc(gc, numElements, elementSize): no gc yet */
    __GLcontext *gc = (__GLcontext *)((void *(__fastcall *)(void *, size_t, size_t))((void **)imports)[1])(NULL, 1, 0x35150);
    if (gc == NULL) return NULL;
    memcpy(gc->imports, imports, sizeof(gc->imports));
    memcpy(&gc->modes, m, sizeof(gc->modes));
    gc->exports[__GL_EXP_DESTROY_CONTEXT] = (GLuint)null_DestroyContext;      /* destroyContext */
    return gc;
}

/* 69079a50 -- __glInstallNullDispatch: install the "no context" dispatch table in the current thread's gc */
void __glInstallNullDispatch(void)
{
    memcpy(&__glCurrentGC()->currentDispatchState, &__glNopDispatchState, sizeof(__GLdispatchTable));
}

/* 69079ad0 -- __glGetCurrentDispatch: the current gc's dispatch table (what DrvSetContext hands to opengl32) */
void *__glGetCurrentDispatch(void)
{
    return &__glCurrentGC()->currentDispatchState;
}

/* 690cdac0 -- finishReturnAddrThunk (8 bytes): hands its own return address (the caller) to the checker at 690cda29,
** which compares it with opengl32!wglSwapBuffers (same shape as 69067da0 in ssthand.c). */
__declspec(naked) int __fastcall finishReturnAddrThunk(int unused1 /* decoration @4 as the callers expect */)
{
    __asm mov ecx, [esp]
    __asm jmp __glim_Finish
}

/* 690e9c00 -- __glFitPixelFormat: fit a pixel format descriptor to the display: PFD_DRAW_TO_WINDOW when its
** colour depth is the display's (bpp), GDI / bitmap support only when single-buffered, palette
** needs for 8-bit RGBA; 16 / 32-bit formats get their channel sizes and shifts from the masks
** (__glMaskToBitsShift: mask -> bits, shift).  Returns 0 when the format cannot be used. */
int __fastcall __glFitPixelFormat(PIXELFORMATDESCRIPTOR *pfd, int bpp, GLuint rMask, GLuint gMask,
                            GLuint bMask, GLuint aMask)
{
    int rBits, rShift, gBits, gShift, bBits, bShift, aBits, aShift;

    if (pfd->cColorBits == bpp)
        pfd->dwFlags |= PFD_DRAW_TO_WINDOW;
    else
        pfd->dwFlags &= ~PFD_DRAW_TO_WINDOW;
    if (!(pfd->dwFlags & PFD_DOUBLEBUFFER)) {
        pfd->dwFlags |= PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI;
    } else {
        pfd->dwFlags &= ~(PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI);
        if (!(pfd->dwFlags & PFD_DRAW_TO_WINDOW))
            return 0;
    }
    if (pfd->cColorBits == 8 && pfd->iPixelType == PFD_TYPE_RGBA)
        pfd->dwFlags |= PFD_NEED_PALETTE | PFD_NEED_SYSTEM_PALETTE;
    else
        pfd->dwFlags &= ~(PFD_NEED_PALETTE | PFD_NEED_SYSTEM_PALETTE);
    if (pfd->cColorBits == 16 || pfd->cColorBits == 32) {
        __glMaskToBitsShift(rMask, &rBits, &rShift);
        __glMaskToBitsShift(gMask, &gBits, &gShift);
        __glMaskToBitsShift(bMask, &bBits, &bShift);
        __glMaskToBitsShift(aMask, &aBits, &aShift);
        pfd->cRedBits = (BYTE)rBits;
        pfd->cRedShift = (BYTE)rShift;
        pfd->cGreenBits = (BYTE)gBits;
        pfd->cGreenShift = (BYTE)gShift;
        pfd->cBlueBits = (BYTE)bBits;
        pfd->cBlueShift = (BYTE)bShift;
        pfd->cAlphaBits = (BYTE)aBits;
        pfd->cAlphaShift = (BYTE)aShift;
    }
    if (bpp > 8 && pfd->iPixelType == PFD_TYPE_COLORINDEX) {
        pfd->dwFlags &= ~PFD_DRAW_TO_WINDOW;
        if (!(pfd->dwFlags & (PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP)))
            return 0;
    }
    return 1;
}
/* 690e9d20 -- __glBuildPixelFormatList: build the list of usable pixel formats.  `list` holds n descriptors;
** __glScreenPixelLayout returns the display depth and fills its channel masks; pass 1 keeps the formats of
** that depth that __glFitPixelFormat accepts, pass 2 the other depths with per-depth default masks (byte
** switch table 690e9ee4: 3:3:2, 5:5:5, 8:8:8 or zeros).  Writes the accepted formats to out[] and
** the count to *outCount (no return value: EAX is left holding the count). */
void __fastcall __glBuildPixelFormatList(HDC hdc, PIXELFORMATDESCRIPTOR *list, int n, PIXELFORMATDESCRIPTOR **out,
                             int *outCount)
{
    GLuint rMask, gMask, bMask, aMask;
    int kind;
    int count = 0;
    int i;
    PIXELFORMATDESCRIPTOR *e;

    kind = __glScreenPixelLayout(hdc, &rMask, &gMask, &bMask, &aMask);
    for (i = 0; i < n; i++) {
        e = &list[i];
        if (e->cColorBits == kind && __glFitPixelFormat(e, kind, rMask, gMask, bMask, aMask) != 0)
            out[count++] = e;
    }
    for (i = 0; i < n; i++) {
        e = &list[i];
        if (e->cColorBits == kind)
            continue;
        if (e->iPixelType == PFD_TYPE_RGBA) {
            switch (e->cColorBits) {
            case 8:  rMask = 0xe0;     gMask = 0x1c;   bMask = 3;    aMask = 0; break;   /* 3:3:2 */
            case 16: rMask = 0x7c00;   gMask = 0x3e0;  bMask = 0x1f; aMask = 0; break;   /* 5:5:5 */
            /* 24 and 32 as two bodies: the compiler merges them, but the extra mask stores weigh in its
            ** spill-slot order (one shared body puts count at the lowest slot, not the original's) */
            case 24: rMask = 0xff0000; gMask = 0xff00; bMask = 0xff; aMask = 0; break;   /* 8:8:8 */
            case 32: rMask = 0xff0000; gMask = 0xff00; bMask = 0xff; aMask = 0; break;   /* 8:8:8 */
            default: rMask = gMask = bMask = aMask = 0; break;
            }
        } else {
            rMask = gMask = bMask = aMask = 0;
        }
        if (__glFitPixelFormat(e, kind, rMask, gMask, bMask, aMask) != 0)
            out[count++] = e;
    }
    *outCount = count;
}

#define __GL_FLOORF_LF2(f)      ((GLfloat) floor((double) (f)))
#define __GL_FRAC_LF2(f)        ((f) - __GL_FLOORF_LF2(f))  /* s_texel.c __GL_FRAC: the outer parentheses place the fsubr */
typedef void (__fastcall *__GLextractTexelProcICD)(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                                                   GLint img, GLint row, GLint col, __GLtexel *result);
/* 690a39b0 (size 1478): s_texel.c __glLinearFilter2, the ICD's form (tex, lp, s, t, result): no
** GL_CLAMP_TO_EDGE, and the texel extract proc is the level's (lp->extract at +0x70). */
void __fastcall __glLinearFilter2(__GLtextureICD *tex, __GLmipMapLevelICD *lp, GLfloat s, GLfloat t,
                             __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLfloat u, v, alpha, beta, half;
    GLint col0, row0, col1, row1, w2f, h2f;
    __GLtexel t00, t01, t10, t11;
    GLfloat omalpha, ombeta, m00, m01, m10, m11;

    /* Find col0, col1 */
    w2f = (GLint)lp->width2f;
    u = s;
    half = (GLfloat)(gc->constants.half);
    if (tex->params.sWrapMode == GL_REPEAT) {
        GLint w2mask = lp->width2 - 1;
        u -= half;
        col0 = ((GLint) __GL_FLOORF_LF2(u)) & w2mask;
        col1 = (col0 + 1) & w2mask;
    } else {
        if (u < 0.0f) u = 0.0f;
        else if (u > w2f) u = w2f;
        u -= half;
        col0 = (GLint) __GL_FLOORF_LF2(u);
        col1 = col0 + 1;
    }

    /* Find row0, row1 */
    h2f = (GLint)lp->height2f;
    v = t;
    if (tex->params.tWrapMode == GL_REPEAT) {
        GLint h2mask = lp->height2 - 1;
        v -= half;
        row0 = ((GLint) __GL_FLOORF_LF2(v)) & h2mask;
        row1 = (row0 + 1) & h2mask;
    } else {
        if (v < 0.0f) v = 0.0f;
        else if (v > h2f) v = h2f;
        v -= half;
        row0 = (GLint) __GL_FLOORF_LF2(v);
        row1 = row0 + 1;
    }

    /* Compute alpha and beta */
    alpha = __GL_FRAC_LF2(u);
    beta = __GL_FRAC_LF2(v);

    /* Calculate the final texel value as a combination of the square chosen */
    (*(__GLextractTexelProcICD)lp->extract)(lp, tex, 0, row0, col0, &t00);
    (*(__GLextractTexelProcICD)lp->extract)(lp, tex, 0, row0, col1, &t10);
    (*(__GLextractTexelProcICD)lp->extract)(lp, tex, 0, row1, col0, &t01);
    (*(__GLextractTexelProcICD)lp->extract)(lp, tex, 0, row1, col1, &t11);

    omalpha = 1.0f - alpha;
    ombeta = 1.0f - beta;

    m00 = omalpha * ombeta;
    m10 = alpha * ombeta;
    m01 = omalpha * beta;
    m11 = alpha * beta;

    /* the sums in SGI's term order (the reversed order puts the texels and weights in other stack slots) */
    switch (tex->baseFormat) {
      case GL_LUMINANCE_ALPHA:
        result->a = m00*t00.a + m10*t10.a + m01*t01.a + m11*t11.a;
        /* FALLTHROUGH */
      case GL_LUMINANCE:
        result->r = m00*t00.r + m10*t10.r + m01*t01.r + m11*t11.r;
        break;
      case GL_RGBA:
        result->a = m00*t00.a + m10*t10.a + m01*t01.a + m11*t11.a;
        /* FALLTHROUGH */
      case GL_RGB:
        result->r = m00*t00.r + m10*t10.r + m01*t01.r + m11*t11.r;
        result->g = m00*t00.g + m10*t10.g + m01*t01.g + m11*t11.g;
        result->b = m00*t00.b + m10*t10.b + m01*t01.b + m11*t11.b;
        break;
      case GL_ALPHA:
        result->a = m00*t00.a + m10*t10.a + m01*t01.a + m11*t11.a;
        break;
      case GL_INTENSITY:
        result->r = m00*t00.r + m10*t10.r + m01*t01.r + m11*t11.r;
        break;
    }
}

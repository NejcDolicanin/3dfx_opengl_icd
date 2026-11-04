/*
** si_s_blend.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_blend.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 69093870 -- __glBlendSpan */
GLboolean __fastcall __glBlendSpan(__GLcontext *gc)
{
    __GLcolor *cp, *fcp, temp;
    GLint w;

    w = gc->polygon.shader.length;

    cp = gc->polygon.shader.colors;
    fcp = gc->polygon.shader.fbcolors;
    while (--w >= 0) {
        (*gc->procs.blendColor)(gc, cp, fcp, &temp);
        *cp = temp;
        cp++;
        fcp++;
    }

    return GL_FALSE;
}

/* 690938e0 -- __glBlendSpan_SA_MSA */
GLboolean __fastcall __glBlendSpan_SA_MSA(__GLcontext *gc)
{
    __GLfloat a, msa, rr, rg, rb, ra, oneOverAlpha;
    __GLfloat one = __GL_ONE_F;
    __GLcolor *cp, *fcp;
    GLint w;

    w = gc->polygon.shader.length;
    cp = gc->polygon.shader.colors;
    fcp = gc->polygon.shader.fbcolors;
    oneOverAlpha = gc->frontBuffer.oneOverAlphaScale;
    while (--w >= 0) {
        a = cp->a * oneOverAlpha;
        msa = one - a;

        rr = cp->r * a + fcp->r * msa;
        rg = cp->g * a + fcp->g * msa;
        rb = cp->b * a + fcp->b * msa;
        ra = cp->a * a + fcp->a * msa;

        cp->r = rr;
        cp->g = rg;
        cp->b = rb;
        cp->a = ra;
        cp++;
        fcp++;
    }

    return GL_FALSE;
}

/* 69093980 -- __glBlendSpan_MSA_SA */
GLboolean __fastcall __glBlendSpan_MSA_SA(__GLcontext *gc)
{
    __GLfloat a, msa, rr, rg, rb, ra, oneOverAlpha;
    __GLcolor *cp, *fcp;
    GLint w;

    w = gc->polygon.shader.length;
    cp = gc->polygon.shader.colors;
    fcp = gc->polygon.shader.fbcolors;
    oneOverAlpha = gc->frontBuffer.oneOverAlphaScale;
    while (--w >= 0) {
        a = cp->a * oneOverAlpha;
        msa = __GL_ONE_F - a;
        rr = cp->r * msa + fcp->r * a;
        rg = cp->g * msa + fcp->g * a;
        rb = cp->b * msa + fcp->b * a;
        ra = cp->a * msa + fcp->a * a;

        cp->r = rr;
        cp->g = rg;
        cp->b = rb;
        cp->a = ra;
        cp++;
        fcp++;
    }

    return GL_FALSE;
}

/* 69093a20 -- __glBlendSpan_SA_ZERO */
GLboolean __fastcall __glBlendSpan_SA_ZERO(__GLcontext *gc)
{
    __GLfloat a, rr, rg, rb, ra, oneOverAlpha;
    __GLcolor *cp;
    GLint w;

    w = gc->polygon.shader.length;
    cp = gc->polygon.shader.colors;
    oneOverAlpha = gc->frontBuffer.oneOverAlphaScale;
    while (--w >= 0) {
        a = cp->a * oneOverAlpha;
        rr = cp->r * a;
        rg = cp->g * a;
        rb = cp->b * a;
        ra = cp->a * a;

        cp->r = rr;
        cp->g = rg;
        cp->b = rb;
        cp->a = ra;
        cp++;
    }

    return GL_FALSE;
}

/* 69093a70 -- __glBlendSpan_SA_ONE */
GLboolean __fastcall __glBlendSpan_SA_ONE(__GLcontext *gc)
{
    __GLfloat a, rr, rg, rb, ra;
    __GLfloat rs, gs, bs, as;
    __GLcolor *cp, *fcp;
    GLint w;

    w = gc->polygon.shader.length;
    cp = gc->polygon.shader.colors;
    fcp = gc->polygon.shader.fbcolors;
    rs = gc->frontBuffer.redScale;
    gs = gc->frontBuffer.greenScale;
    bs = gc->frontBuffer.blueScale;
    as = gc->frontBuffer.alphaScale;
    while (--w >= 0) {
        a = cp->a * gc->frontBuffer.oneOverAlphaScale;
        rr = fcp->r + cp->r * a;
        rg = fcp->g + cp->g * a;
        if (rr > rs) {
            rr = rs;
        }
        rb = fcp->b + cp->b * a;
        if (rg > gs) {
            rg = gs;
        }
        ra = fcp->a + cp->a * a;
        if (rb > bs) {
            rb = bs;
        }

        cp->r = rr;
        if (ra > as) {
            ra = as;
        }
        cp->g = rg;
        cp->b = rb;
        cp->a = ra;
        cp++;
        fcp++;
    }

    return GL_FALSE;
}

/* 69092f50 -- DoBlendSourceSA */
void __fastcall DoBlendSourceSA(__GLcontext *gc, const __GLcolor *source,
                            const __GLcolor *dest, __GLcolor *result)
{
    /* Compiler hints */
    __GLfloat r, g, b, a;

#if defined(__GL_LINT)
    dest = dest;
#endif /* __GL_LINT */
    a = source->a * gc->frontBuffer.oneOverAlphaScale;

    r = a * source->r;
    g = a * source->g;
    b = a * source->b;
    a = a * source->a;

    result->r = r;
    result->g = g;
    result->b = b;
    result->a = a;
}

/* 69092fa0 -- DoBlendDestSA */
void __fastcall DoBlendDestSA(__GLcontext *gc, const __GLcolor *source,
                          const __GLcolor *dest, __GLcolor *result)
{
    __GLcolor tmpc;
    __GLfloat a;

    a = source->a * gc->frontBuffer.oneOverAlphaScale;

    tmpc.r = a * dest->r;
    tmpc.g = a * dest->g;
    tmpc.b = a * dest->b;
    tmpc.a = a * dest->a;

    gc->procs.blendEquation(gc, source, &tmpc, result);
}

/* 69092ff0 -- DoBlendSourceMSA */
void __fastcall DoBlendSourceMSA(__GLcontext *gc, const __GLcolor *source,
                             const __GLcolor *dest, __GLcolor *result)
{
    /* Compiler hints */
    __GLfloat r, g, b, a;
    __GLfloat msa = __GL_ONE_F - source->a * gc->frontBuffer.oneOverAlphaScale;

#if defined(__GL_LINT)
    dest = dest;
#endif /* __GL_LINT */
    r = msa * source->r;
    g = msa * source->g;
    b = msa * source->b;
    a = msa * source->a;

    result->r = r;
    result->g = g;
    result->b = b;
    result->a = a;
}

/* 69093040 -- DoBlendDestMSA */
void __fastcall DoBlendDestMSA(__GLcontext *gc, const __GLcolor *source,
                           const __GLcolor *dest, __GLcolor *result)
{
    __GLcolor tmpc;
    __GLfloat msa =
            __GL_ONE_F - source->a * gc->frontBuffer.oneOverAlphaScale;

    tmpc.r = msa * dest->r;
    tmpc.g = msa * dest->g;
    tmpc.b = msa * dest->b;
    tmpc.a = msa * dest->a;

    gc->procs.blendEquation(gc, source, &tmpc, result);
}

/* 69093090 -- DoBlendSourceDA */
void __fastcall DoBlendSourceDA(__GLcontext *gc, const __GLcolor *source,
                            const __GLcolor *dest, __GLcolor *result)
{
    /* Compiler hints */
    __GLfloat r, g, b;
    __GLfloat a = dest->a * gc->frontBuffer.oneOverAlphaScale;

    r = a * source->r;
    g = a * source->g;
    b = a * source->b;
    a = a * source->a;

    result->r = r;
    result->g = g;
    result->b = b;
    result->a = a;
}

/* 690930e0 -- DoBlendDestDA */
void __fastcall DoBlendDestDA(__GLcontext *gc, const __GLcolor *source,
                          const __GLcolor *dest, __GLcolor *result)
{
    __GLcolor tmpc;
    __GLfloat a = dest->a * gc->frontBuffer.oneOverAlphaScale;

    tmpc.r = a * dest->r;
    tmpc.g = a * dest->g;
    tmpc.b = a * dest->b;
    tmpc.a = a * dest->a;

    gc->procs.blendEquation(gc, source, &tmpc, result);
}

/* 69093130 -- DoBlendSourceMDA */
void __fastcall DoBlendSourceMDA(__GLcontext *gc, const __GLcolor *source,
                           const __GLcolor *dest, __GLcolor *result)
{
    /* Compiler hints */
    __GLfloat r, g, b, a;
    __GLfloat mda;

    mda = __GL_ONE_F - dest->a * gc->frontBuffer.oneOverAlphaScale;

    r = mda * source->r;
    g = mda * source->g;
    b = mda * source->b;
    a = mda * source->a;

    result->r = r;
    result->g = g;
    result->b = b;
    result->a = a;
}

/* 69093180 -- DoBlendDestMDA */
void __fastcall DoBlendDestMDA(__GLcontext *gc, const __GLcolor *source,
                           const __GLcolor *dest, __GLcolor *result)
{
    __GLcolor tmpc;
    __GLfloat mda;

    mda = __GL_ONE_F - dest->a * gc->frontBuffer.oneOverAlphaScale;

    tmpc.r = mda * dest->r;
    tmpc.g = mda * dest->g;
    tmpc.b = mda * dest->b;
    tmpc.a = mda * dest->a;

    gc->procs.blendEquation(gc, source, &tmpc, result);
}

/* 690931d0 -- DoBlendSourceSAT */
void __fastcall DoBlendSourceSAT(__GLcontext *gc, const __GLcolor *source,
                             const __GLcolor *dest, __GLcolor *result)
{
    /* Compiler hints */
    __GLfloat r, g, b;
    __GLfloat sa, mda;

    mda = __GL_ONE_F - dest->a * gc->frontBuffer.oneOverAlphaScale;
    sa = source->a * gc->frontBuffer.oneOverAlphaScale;
    if (sa < mda) {
        r = sa * source->r;
        g = sa * source->g;
        b = sa * source->b;
    } else {
        r = mda * source->r;
        g = mda * source->g;
        b = mda * source->b;
    }
    result->a = source->a;
    result->r = r;
    result->g = g;
    result->b = b;
}

/* 69093260 -- DoBlend */
void __fastcall DoBlend(__GLcontext *gc, const __GLcolor *source,
                     const __GLcolor *dest, __GLcolor *result)
{
    (*gc->procs.blendSrc)(gc, source, dest, result);
    (*gc->procs.blendDst)(gc, source, dest, result);

    if (result->r > gc->frontBuffer.redScale) {
        result->r = gc->frontBuffer.redScale;
    }
    if (result->g > gc->frontBuffer.greenScale) {
        result->g = gc->frontBuffer.greenScale;
    }
    if (result->b > gc->frontBuffer.blueScale) {
        result->b = gc->frontBuffer.blueScale;
    }
    if (result->a > gc->frontBuffer.alphaScale) {
        result->a = gc->frontBuffer.alphaScale;
    }
}

/* 690932f0 -- DoBlendNoClamp */
void __fastcall DoBlendNoClamp(__GLcontext *gc, const __GLcolor *source,
                           const __GLcolor *dest, __GLcolor *result)
{
    (*gc->procs.blendSrc)(gc, source, dest, result);
    (*gc->procs.blendDst)(gc, source, dest, result);
}

/* 69093320 -- DoBlend_SA_ZERO */
void __fastcall DoBlend_SA_ZERO(__GLcontext *gc, const __GLcolor *source,
                          const __GLcolor *dest, __GLcolor *result)
{
    __GLfloat a;

#if defined(__GL_LINT)
    dest = dest;
#endif /* __GL_LINT */
    a = source->a * gc->frontBuffer.oneOverAlphaScale;
    result->r = source->r * a;
    result->g = source->g * a;
    result->b = source->b * a;
    result->a = source->a * a;
}

/* 69093350 -- DoBlend_SA_ONE */
void __fastcall DoBlend_SA_ONE(__GLcontext *gc, const __GLcolor *source,
                           const __GLcolor *dest, __GLcolor *result)
{
    __GLfloat a, ra, rr, rg, rb;

    a = source->a * gc->frontBuffer.oneOverAlphaScale;
    rr = dest->r + source->r * a;
    if (rr > gc->frontBuffer.redScale) {
        rr = gc->frontBuffer.redScale;
    }
    rg = dest->g + source->g * a;
    if (rg > gc->frontBuffer.greenScale) {
        rg = gc->frontBuffer.greenScale;
    }
    rb = dest->b + source->b * a;
    if (rb > gc->frontBuffer.blueScale) {
        rb = gc->frontBuffer.blueScale;
    }
    ra = dest->a + source->a * a;
    if (ra > gc->frontBuffer.alphaScale) {
        ra = gc->frontBuffer.alphaScale;
    }

    result->r = rr;
    result->g = rg;
    result->b = rb;
    result->a = ra;
}

/* 69093410 -- DoBlend_SA_MSA */
void __fastcall DoBlend_SA_MSA(__GLcontext *gc, const __GLcolor *source,
                           const __GLcolor *dest, __GLcolor *result)
{
    __GLfloat a, msa, rr, rg, rb, ra;

    a = source->a * gc->frontBuffer.oneOverAlphaScale;
    msa = __GL_ONE_F - a;
    rr = source->r * a + dest->r * msa;
    rg = source->g * a + dest->g * msa;
    rb = source->b * a + dest->b * msa;
    ra = source->a * a + dest->a * msa;

    result->r = rr;
    result->g = rg;
    result->b = rb;
    result->a = ra;
}

/* 69093490 -- DoBlend_MSA_SA */
void __fastcall DoBlend_MSA_SA(__GLcontext *gc, const __GLcolor *source,
                         const __GLcolor *dest, __GLcolor *result)
{
    __GLfloat a, msa, rr, rg, rb, ra;

    a = source->a * gc->frontBuffer.oneOverAlphaScale;
    msa = __GL_ONE_F - a;
    rr = source->r * msa + dest->r * a;
    rg = source->g * msa + dest->g * a;
    rb = source->b * msa + dest->b * a;
    ra = source->a * msa + dest->a * a;

    result->r = rr;
    result->g = rg;
    result->b = rb;
    result->a = ra;
}

/* 69092cf0 -- DoBlendSourceZERO */
void __fastcall DoBlendSourceZERO(__GLcontext *gc, const __GLcolor *source,
                              const __GLcolor *dest, __GLcolor *result)
{
    __GLfloat zero = __GL_ZERO_F;

#if defined(__GL_LINT)
    gc = gc;
    source = source;
    dest = dest;
#endif /* __GL_LINT */
    result->r = zero;
    result->g = zero;
    result->b = zero;
    result->a = zero;
}

/* 69092d10 -- DoBlendDestZERO */
void __fastcall DoBlendDestZERO(__GLcontext *gc, const __GLcolor *source,
                            const __GLcolor *dest, __GLcolor *result)
{
    __GLfloat zero = __GL_ZERO_F;
    __GLcolor tmpc;

    tmpc.r = zero;
    tmpc.g = zero;
    tmpc.b = zero;
    tmpc.a = zero;

    gc->procs.blendEquation(gc, source, &tmpc, result);
}

/* 69092d50 -- DoBlendSourceONE */
void __fastcall DoBlendSourceONE(__GLcontext *gc, const __GLcolor *source,
                             const __GLcolor *dest, __GLcolor *result)
{
    /* Compiler hints */
    __GLfloat r, g, b, a;

#if defined(__GL_LINT)
    gc = gc;
    dest = dest;
#endif /* __GL_LINT */
    r = source->r;
    g = source->g;
    b = source->b;
    a = source->a;

    result->r = r;
    result->g = g;
    result->b = b;
    result->a = a;
}

/* 69092d80 -- DoBlendDestONE */
void __fastcall DoBlendDestONE(__GLcontext *gc, const __GLcolor *source,
                           const __GLcolor *dest, __GLcolor *result)
{
#if defined(__GL_LINT)
    gc = gc;
    source = source;
#endif /* __GL_LINT */

    gc->procs.blendEquation(gc, source, dest,  result);
}

/* 69092da0 -- DoBlendDestSC */
void __fastcall DoBlendDestSC(__GLcontext *gc, const __GLcolor *source,
                          const __GLcolor *dest, __GLcolor *result)
{
    __GLcolor tmpc;

    tmpc.r = dest->r * source->r * gc->frontBuffer.oneOverRedScale;
    tmpc.g = dest->g * source->g * gc->frontBuffer.oneOverGreenScale;
    tmpc.b = dest->b * source->b * gc->frontBuffer.oneOverBlueScale;
    tmpc.a = dest->a * source->a * gc->frontBuffer.oneOverAlphaScale;

    gc->procs.blendEquation(gc, source, &tmpc, result);
}

/* 69092e00 -- DoBlendDestMSC */
void __fastcall DoBlendDestMSC(__GLcontext *gc, const __GLcolor *source,
                           const __GLcolor *dest, __GLcolor *result)
{
    __GLcolor tmpc;
    __GLfloat one = __GL_ONE_F;

    tmpc.r = dest->r * (one - source->r * gc->frontBuffer.oneOverRedScale);
    tmpc.g = dest->g * (one - source->g * gc->frontBuffer.oneOverGreenScale);
    tmpc.b = dest->b * (one - source->b * gc->frontBuffer.oneOverBlueScale);
    tmpc.a = dest->a * (one - source->a * gc->frontBuffer.oneOverAlphaScale);

    gc->procs.blendEquation(gc, source, &tmpc, result);
}

/* 69092e80 -- DoBlendSourceDC */
void __fastcall DoBlendSourceDC(__GLcontext *gc, const __GLcolor *source,
                            const __GLcolor *dest, __GLcolor *result)
{
    /* Compiler hints */
    __GLfloat r, g, b, a;

    r = source->r * dest->r * gc->frontBuffer.oneOverRedScale;
    g = source->g * dest->g * gc->frontBuffer.oneOverGreenScale;
    b = source->b * dest->b * gc->frontBuffer.oneOverBlueScale;
    a = source->a * dest->a * gc->frontBuffer.oneOverAlphaScale;

    result->r = r;
    result->g = g;
    result->b = b;
    result->a = a;
}

/* 69092ee0 -- DoBlendSourceMDC */
void __fastcall DoBlendSourceMDC(__GLcontext *gc, const __GLcolor *source,
                             const __GLcolor *dest, __GLcolor *result)
{
    /* Compiler hints */
    __GLfloat r, g, b, a;
    __GLfloat one = __GL_ONE_F;

    r = source->r * (one - dest->r * gc->frontBuffer.oneOverRedScale);
    g = source->g * (one - dest->g * gc->frontBuffer.oneOverGreenScale);
    b = source->b * (one - dest->b * gc->frontBuffer.oneOverBlueScale);
    a = source->a * (one - dest->a * gc->frontBuffer.oneOverAlphaScale);

    result->r = r;
    result->g = g;
    result->b = b;
    result->a = a;
}

/* 69093800 -- Nop (s_blend.c) */
void __fastcall Nop(__GLcontext *gc, const __GLcolor *source,
                const __GLcolor *dest, __GLcolor *result)
{
#if defined(__GL_LINT)
    gc = gc;
    source = source;
    dest = dest;
    result = result;
#endif /* __GL_LINT */
}

/* 690937d0 -- BlendAdd */
void __fastcall BlendAdd(__GLcontext *gc, const __GLcolor *source,
                     const __GLcolor *dest, __GLcolor *result)
{
    result->r += dest->r;
    result->g += dest->g;
    result->b += dest->b;
    result->a += dest->a;
}

/* 69093810 -- NoFetchBlend */
void __fastcall NoFetchBlend(__GLcontext *gc, __GLcolorBufferICD *cfb,
                         const __GLfragmentICD *frag, __GLcolor *result)
{
#if defined(__GL_LINT)
    cfb = cfb;
#endif /* __GL_LINT */
    (*gc->procs.blendColor)(gc, &(frag->color),
                            NULL, result);
}

/* 69093830 -- FetchBlend */
void __fastcall FetchBlend(__GLcontext *gc, __GLcolorBufferICD *cfb,
                       const __GLfragmentICD *frag, __GLcolor *result)
{
    __GLcolor dest;

    (*cfb->fetch)(cfb, frag->x, frag->y, &dest);

    (*gc->procs.blendColor)(gc, &(frag->color),
                            &dest, result);
}

/* 69093b70 -- __glBlendStippledSpan */
GLboolean __fastcall __glBlendStippledSpan(__GLcontext *gc)
{
    __GLstippleWord *sp, bit, inMask;
    __GLcolor *cp, *fcp, temp;
    GLint count;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    cp = gc->polygon.shader.colors;
    fcp = gc->polygon.shader.fbcolors;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp++;
        bit = __GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (inMask & bit) {
                (*gc->procs.blendColor)(gc, cp, fcp, &temp);
                *cp = temp;
            }
            cp++;
            fcp++;
#if defined(__GL_STIPPLE_MSB)
            bit >>= 1;
#else  /* __GL_STIPPLE_MSB */
            bit <<= 1;
#endif /* __GL_STIPPLE_MSB */
        }
    }

    return GL_FALSE;
}

/*
** si_s_pgspan.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_pgspan.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690cde60 -- __glProcessSpan */
GLboolean __fastcall __glProcessSpan(__GLcontext *gc)
{
    GLint m, i;

    m = gc->procs.span.m;

    gc->polygon.shader.done = GL_FALSE;
    for (i = 0; i < m; i++) {
        if ((*(GLboolean (__fastcall *)(__GLcontext *))gc->procs.span.spanFuncs[i])(gc)     /* the span procs return GLboolean (test al) */) {
            i++;
            break;
        }
    }

    if (i != m && !gc->polygon.shader.done) {
        for (; i<m; i++) {
            if ((*(GLboolean (__fastcall *)(__GLcontext *))gc->procs.span.stippledSpanFuncs[i])(gc)) {
                break;
            }
        }
    }

    return GL_FALSE;
}

/* 690d0920 -- __glRoundCISpan */
GLboolean __fastcall __glRoundCISpan(__GLcontext *gc)
{
    __GLcolor *cp;
    GLint x, r, maxR;
    GLint w;

    w = gc->polygon.shader.length;
    x = gc->polygon.shader.frag.x;
    maxR = (1 << gc->modes.indexBits) - 1;
    cp = gc->polygon.shader.colors;
    while (--w >= 0) {
        r = (GLint)(cp->r + __glHalf);
        if (r > maxR) { r = maxR; }
        cp->r = r;
        cp++;
        x++;
    }

    return GL_FALSE;
}

/* 690d0a40 -- __glRoundRGBASpan */
GLboolean __fastcall __glRoundRGBASpan(__GLcontext *gc)
{
    __GLcolor *cp;
    GLint x, r, g, b, a;
    GLint maxR, maxG, maxB, maxA;
    GLint w;

    w = gc->polygon.shader.length;

    x = gc->polygon.shader.frag.x;
    maxR = gc->frontBuffer.iRedScale;
    maxG = gc->frontBuffer.iGreenScale;
    maxB = gc->frontBuffer.iBlueScale;
    maxA = gc->frontBuffer.iAlphaScale;
    cp = gc->polygon.shader.colors;
    while (--w >= 0) {
        r = (GLint) (cp->r + __glHalf);
        if (r > maxR) r = maxR;
        cp->r = r;
        g = (GLint) (cp->g + __glHalf);
        if (g > maxG) g = maxG;
        cp->g = g;
        b = (GLint) (cp->b + __glHalf);
        if (b > maxB) b = maxB;
        cp->b = b;
        a = (GLint) (cp->a + __glHalf);
        if (a > maxA) a = maxA;
        cp->a = a;
        cp++;
        x++;
    }

    return GL_FALSE;
}

/* 690d0c90 -- __glLogicOpSpan */
GLboolean __fastcall __glLogicOpSpan(__GLcontext *gc)
{
    __GLcolor *cp, *fcp;
    GLint x, color, fbcolor;
    GLint w;

    w = gc->polygon.shader.length;

    cp = gc->polygon.shader.colors;
    fcp = gc->polygon.shader.fbcolors;
    x = gc->polygon.shader.frag.x;
    while (--w >= 0) {
        color = (GLint) cp->r;
        fbcolor = (GLint) fcp->r;
        switch (gc->state.raster.logicOp) {
          case GL_CLEAR:         color = 0; break;
          case GL_AND:           color = color & fbcolor; break;
          case GL_AND_REVERSE:   color = color & (~fbcolor); break;
          case GL_COPY:          color = color; break;
          case GL_AND_INVERTED:  color = (~color) & fbcolor; break;
          case GL_NOOP:          color = fbcolor; break;
          case GL_XOR:           color = color ^ fbcolor; break;
          case GL_OR:            color = color | fbcolor; break;
          case GL_NOR:           color = ~(color | fbcolor); break;
          case GL_EQUIV:         color = ~(color ^ fbcolor); break;
          case GL_INVERT:        color = ~fbcolor; break;
          case GL_OR_REVERSE:    color = color | (~fbcolor); break;
          case GL_COPY_INVERTED: color = ~color; break;
          case GL_OR_INVERTED:   color = (~color) | fbcolor; break;
          case GL_NAND:          color = ~(color & fbcolor); break;
          case GL_SET:           color = ~0; break;
        }
        cp->r = color;
        cp++;
        fcp++;
        x++;
    }

    return GL_FALSE;
}

/* 690ce2a0 -- __glStippleStippledSpan */
GLboolean __fastcall __glStippleStippledSpan(__GLcontext *gc)
{
    __GLstippleWord stipple;
    __GLstippleWord *sp;
    GLint count;
    GLint shift;
    GLint w;

    w = gc->polygon.shader.length;

    if (gc->constants.yInverted) {
        stipple = gc->polygon.stipple[(gc->constants.height - 
                (gc->polygon.shader.frag.y - gc->constants.viewportYAdjust)-1) 
                & (__GL_STIPPLE_BITS-1)];
    } else {
        stipple = gc->polygon.stipple[gc->polygon.shader.frag.y & 
                (__GL_STIPPLE_BITS-1)];
    }
    shift = gc->polygon.shader.frag.x & (__GL_STIPPLE_BITS - 1);
#ifdef __GL_STIPPLE_MSB
    stipple = (stipple << shift) | (stipple >> (__GL_STIPPLE_BITS - shift));
#else
    stipple = (stipple >> shift) | (stipple << (__GL_STIPPLE_BITS - shift));
#endif
    if (stipple == 0) {
        /* No point in continuing */
        gc->polygon.shader.done = GL_TRUE;
        return GL_TRUE;
    }

    /* Replicate stipple word */
    count = w;
    sp = gc->polygon.shader.stipplePat;
    while (count > 0) {
        *sp++ &= stipple;
        count -= __GL_STIPPLE_BITS;
    }

    return GL_FALSE;
}

/* 690d01d0 -- __glFlatCISpan */
GLboolean __fastcall __glFlatCISpan(__GLcontext *gc)
{
    __GLcolor *cp;
    __GLfloat r;
    GLint w;

    w = gc->polygon.shader.length;

    r = gc->polygon.shader.frag.color.r;
    cp = gc->polygon.shader.colors;
    while (--w >= 0) {
        cp->r = r;
        cp++;
    }

    return GL_FALSE;
}

/* 690d0200 -- __glFlatRGBASpan */
GLboolean __fastcall __glFlatRGBASpan(__GLcontext *gc)
{
    __GLcolor *cp;
    __GLfloat r, g, b, a;
    GLint w;

    w = gc->polygon.shader.length;

    r = gc->polygon.shader.frag.color.r;
    g = gc->polygon.shader.frag.color.g;
    b = gc->polygon.shader.frag.color.b;
    a = gc->polygon.shader.frag.color.a;
    cp = gc->polygon.shader.colors;
    while (--w >= 0) {
        cp->r = r;
        cp->g = g;
        cp->b = b;
        cp->a = a;
        cp++;
    }

    return GL_FALSE;
}

/* 690ce210 -- __glStippleSpan */
GLboolean __fastcall __glStippleSpan(__GLcontext *gc)
{
    __GLstippleWord stipple;
    __GLstippleWord *sp;
    GLint count;
    GLint shift;
    GLint w;

    w = gc->polygon.shader.length;

    if (gc->constants.yInverted) {
        stipple = gc->polygon.stipple[(gc->constants.height - 
                (gc->polygon.shader.frag.y - gc->constants.viewportYAdjust)-1) 
                & (__GL_STIPPLE_BITS-1)];
    } else {
        stipple = gc->polygon.stipple[gc->polygon.shader.frag.y & 
                (__GL_STIPPLE_BITS-1)];
    }
    shift = gc->polygon.shader.frag.x & (__GL_STIPPLE_BITS - 1);
#ifdef __GL_STIPPLE_MSB
    stipple = (stipple << shift) | (stipple >> (__GL_STIPPLE_BITS - shift));
#else
    stipple = (stipple >> shift) | (stipple << (__GL_STIPPLE_BITS - shift));
#endif
    if (stipple == 0) {
        /* No point in continuing */
        gc->polygon.shader.done = GL_TRUE;
        return GL_TRUE;
    }

    /* Replicate stipple word */
    count = w;
    sp = gc->polygon.shader.stipplePat;
    while (count > 0) {
        *sp++ = stipple;
        count -= __GL_STIPPLE_BITS;
    }

    return GL_TRUE;
}

/* 690ce150 -- __glClipSpan */
GLboolean __fastcall __glClipSpan(__GLcontext *gc)
{
    GLint clipX0, clipX1, delta;
    GLint x, xr;
    GLint w, w2;
    GLboolean stippled;

    w = gc->polygon.shader.length;

    x = gc->polygon.shader.frag.x;
    stippled = GL_FALSE;
    clipX0 = gc->transform.clipX0;
    clipX1 = gc->transform.clipX1;
    xr = x + w;
    if ((x < clipX0) || (xr > clipX1)) {
        /*
        ** Span needs to be scissored in some fashion
        */
        if ((xr <= clipX0) || (x >= clipX1)) {
            /* Scissor out the entire span */
            gc->polygon.shader.done = GL_TRUE;
            return GL_TRUE;
        }
        if (xr > clipX1) {
            /*
            ** Span is clipped by the right edge of the scissor.  This is 
            ** easy, we will simply reduce the width of this span!
            */
            w = clipX1 - x;
        }
        if (x < clipX0) {
            __GLstippleWord bit, outMask, *osp;
            GLint count;

            /*
            ** Span is clipped by the left edge of the scissor.  This is hard.
            ** We have two choices.
            **
            ** 1) We can stipple the first half of the span.
            ** 2) We can bump all of the iterator values.
            **
            ** The problem with approach number 2 is that the routine 
            ** which originally asks to have a span processed has assumed 
            ** that the iterator values will not be munged.  So, if we 
            ** wanted to implement 2 (which would make this case faster),
            ** we would need to change that assumption, and make the higher
            ** routine shadow all of the iterator values, which would slow
            ** down all paths.  This is probably not a good trade to speed
            ** this path up, since this path will only occur when the scissor
            ** region (or window) is smaller than the viewport, and this span
            ** happens to hit the left edge of the scissor region (or window).
            **
            ** Therefore, we choose number 1.
            */
            delta = clipX0 - x;

            osp = gc->polygon.shader.stipplePat;
            w2 = w;
            while (w2) {
                count = w2;
                if (count > __GL_STIPPLE_BITS) {
                    count = __GL_STIPPLE_BITS;
                }
                w2 -= count;

                outMask = ~0;
                bit = __GL_STIPPLE_SHIFT(0);
                while (--count >= 0) {
                    if (delta > 0) {
                        delta--;
                        outMask &= ~bit;
                    }
#ifdef __GL_STIPPLE_MSB
                    bit >>= 1;
#else
                    bit <<= 1;
#endif
                }

                *osp++ = outMask;
            }

            stippled = GL_TRUE;
        }
    }
    assert(w <= __GL_MAX_MAX_VIEWPORT);

    gc->polygon.shader.length = w;

    return stippled;
}

/* 690ce340 -- __glAlphaTestSpan */
GLboolean __fastcall __glAlphaTestSpan(__GLcontext *gc)
{
    GLubyte *atft;
    GLint failed, count, ia;
    __GLstippleWord bit, outMask, *osp;
    __GLcolor *cp;
    GLint maxAlpha;
    GLint w;

    w = gc->polygon.shader.length;

    atft = gc->frontBuffer.alphaTestFuncTable;      /* ICD: the colour buffer's table (690ce34c) */
    cp = gc->polygon.shader.colors;
    maxAlpha = gc->constants.alphaTestSize - 1;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        outMask = ~0;
        bit = __GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            ia = (GLint)(gc->constants.alphaTableConv * cp->a);
            if (ia < 0) ia = 0;
            if (ia > maxAlpha) ia = maxAlpha;
            if (!atft[ia]) {
                /* Test failed */
                outMask &= ~bit;
                failed++;
            }
            cp++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
        *osp++ = outMask;
    }

    if (failed == 0) {
        /* Call next span proc */
        return GL_FALSE;
    } else {
        if (failed != gc->polygon.shader.length) {
            /* Call next stippled span proc */
            return GL_TRUE;
        }
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

/* 690ce440 -- __glAlphaTestStippledSpan */
GLboolean __fastcall __glAlphaTestStippledSpan(__GLcontext *gc)
{
    GLubyte *atft;
    GLint count, ia, failed;
    __GLstippleWord bit, inMask, outMask, *isp;
    __GLcolor *cp;
    GLint maxAlpha;
    GLint w;

    w = gc->polygon.shader.length;
    isp = gc->polygon.shader.stipplePat;

    atft = gc->frontBuffer.alphaTestFuncTable;      /* ICD: the colour buffer's table */
    cp = gc->polygon.shader.colors;
    maxAlpha = gc->constants.alphaTestSize - 1;
    failed = 0;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *isp;
        outMask = ~0;
        bit = __GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (inMask & bit) {
                ia = (GLint)(gc->constants.alphaTableConv * cp->a);
                if (ia < 0) ia = 0;
                if (ia > maxAlpha) ia = maxAlpha;
                if (!atft[ia]) {
                    /* Test failed */
                    outMask &= ~bit;
                    failed++;
                }
            } else failed++;
            cp++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
        *isp++ = outMask & inMask;
    }

    if (failed != gc->polygon.shader.length) {
        /* Call next stippled span proc */
        return GL_FALSE;
    }
    return GL_TRUE;
}

/* 690d0b30 -- __glRoundRGBAStippledSpan */
GLboolean __fastcall __glRoundRGBAStippledSpan(__GLcontext *gc)
{
    __GLstippleWord bit, inMask, *sp;
    __GLcolor *cp;
    GLint count, x, r, g, b, a;
    GLint maxR, maxG, maxB, maxA;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    x = gc->polygon.shader.frag.x;
    maxR = gc->frontBuffer.iRedScale;
    maxG = gc->frontBuffer.iGreenScale;
    maxB = gc->frontBuffer.iBlueScale;
    maxA = gc->frontBuffer.iAlphaScale;
    cp = gc->polygon.shader.colors;
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
                r = (GLint) (cp->r + __glHalf);
                if (r > maxR) r = maxR;
                cp->r = r;
                g = (GLint) (cp->g + __glHalf);
                if (g > maxG) g = maxG;
                cp->g = g;
                b = (GLint) (cp->b + __glHalf);
                if (b > maxB) b = maxB;
                cp->b = b;
                a = (GLint) (cp->a + __glHalf);
                if (a > maxA) a = maxA;
                cp->a = a;
            }
            cp++;
            x++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
    }

    return GL_FALSE;
}

/* 690d0dc0 -- __glLogicOpStippledSpan */
GLboolean __fastcall __glLogicOpStippledSpan(__GLcontext *gc)
{
    __GLstippleWord bit, inMask, *sp;
    __GLcolor *cp, *fcp;
    GLint count, x, color, fbcolor;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    cp = gc->polygon.shader.colors;
    fcp = gc->polygon.shader.fbcolors;
    x = gc->polygon.shader.frag.x;
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
                color = (GLint) cp->r;
                fbcolor = (GLint) fcp->r;
                switch (gc->state.raster.logicOp) {
                  case GL_CLEAR:         color = 0; break;
                  case GL_AND:           color = color & fbcolor; break;
                  case GL_AND_REVERSE:   color = color & (~fbcolor); break;
                  case GL_COPY:          color = color; break;
                  case GL_AND_INVERTED:  color = (~color) & fbcolor; break;
                  case GL_NOOP:          color = fbcolor; break;
                  case GL_XOR:           color = color ^ fbcolor; break;
                  case GL_OR:            color = color | fbcolor; break;
                  case GL_NOR:           color = ~(color | fbcolor); break;
                  case GL_EQUIV:         color = ~(color ^ fbcolor); break;
                  case GL_INVERT:        color = ~fbcolor; break;
                  case GL_OR_REVERSE:    color = color | (~fbcolor); break;
                  case GL_COPY_INVERTED: color = ~color; break;
                  case GL_OR_INVERTED:   color = (~color) | fbcolor; break;
                  case GL_NAND:          color = ~(color & fbcolor); break;
                  case GL_SET:           color = ~0; break;
                }
                cp->r = color;
            }
            cp++;
            fcp++;
            x++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
    }

    return GL_FALSE;
}

/* 690cded0 -- __glProcessReplicateSpan */
GLboolean __fastcall __glProcessReplicateSpan(__GLcontext *gc)
{
    GLint n, m, i;
    __GLcolor colors[__GL_MAX_MAX_VIEWPORT], *fcp, *tcp;
    GLint w;

    w = gc->polygon.shader.length;
    n = gc->procs.span.n;
    m = gc->procs.span.m; 

    gc->polygon.shader.done = GL_FALSE;
    for (i = 0; i < n; i++) {
        if ((*(GLboolean (__fastcall *)(__GLcontext *))gc->procs.span.spanFuncs[i])(gc)     /* GLboolean procs */) {
            i++;
            goto earlyStipple;
        }
    }

    fcp = gc->polygon.shader.colors;
    tcp = colors;
    if (gc->modes.rgbMode) {
        for (i = 0; i < w; i++) {
            *tcp++ = *fcp++;
        }
    } else {
        for (i = 0; i < w; i++) {
            tcp->r = fcp->r;
            fcp++;
            tcp++;
        }
    }
    gc->polygon.shader.cfb = &gc->frontBuffer;
    for (i = n; i < m; i++) {
        (*(GLboolean (__fastcall *)(__GLcontext *))gc->procs.span.spanFuncs[i])(gc)     /* GLboolean procs */;
    }
    fcp = colors;
    tcp = gc->polygon.shader.colors;
    if (gc->modes.rgbMode) {
        for (i = 0; i < w; i++) {
            *tcp++ = *fcp++;
        }
    } else {
        for (i = 0; i < w; i++) {
            tcp->r = fcp->r;
            fcp++;
            tcp++;
        }
    }
    gc->polygon.shader.cfb = &gc->backBuffer;
    for (i = n; i < m; i++) {
        (*(GLboolean (__fastcall *)(__GLcontext *))gc->procs.span.spanFuncs[i])(gc)     /* GLboolean procs */;
    }
    return GL_FALSE;

earlyStipple:
    if (gc->polygon.shader.done) return GL_FALSE;

    for (; i < n; i++) {
        if ((*(GLboolean (__fastcall *)(__GLcontext *))gc->procs.span.stippledSpanFuncs[i])(gc)) {
            return GL_FALSE;
        }
    }

    fcp = gc->polygon.shader.colors;
    tcp = colors;
    if (gc->modes.rgbMode) {
        for (i = 0; i < w; i++) {
            *tcp++ = *fcp++;
        }
    } else {
        for (i = 0; i < w; i++) {
            tcp->r = fcp->r;
            fcp++;
            tcp++;
        }
    }
    gc->polygon.shader.cfb = &gc->frontBuffer;
    for (i = n; i < m; i++) {
        (*(GLboolean (__fastcall *)(__GLcontext *))gc->procs.span.stippledSpanFuncs[i])(gc);
    }
    fcp = colors;
    tcp = gc->polygon.shader.colors;
    if (gc->modes.rgbMode) {
        for (i = 0; i < w; i++) {
            *tcp++ = *fcp++;
        }
    } else {
        for (i = 0; i < w; i++) {
            tcp->r = fcp->r;
            fcp++;
            tcp++;
        }
    }
    gc->polygon.shader.cfb = &gc->backBuffer;
    for (i = n; i < m; i++) {
        (*(GLboolean (__fastcall *)(__GLcontext *))gc->procs.span.stippledSpanFuncs[i])(gc);
    }
    return GL_FALSE;
}

/* 690d0980 -- __glRoundCIStippledSpan */
GLboolean __fastcall __glRoundCIStippledSpan(__GLcontext *gc)
{
    __GLstippleWord bit, inMask, *sp;
    __GLcolor *cp;
    GLint count, x, r, maxR;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;
    x = gc->polygon.shader.frag.x;
    maxR = (1 << gc->modes.indexBits) - 1;
    cp = gc->polygon.shader.colors;
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
                r = (GLint)(cp->r + __glHalf);
                if (r > maxR) { r = maxR; }
                cp->r = r;
            }
            cp++;
            x++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
    }

    return GL_FALSE;
}

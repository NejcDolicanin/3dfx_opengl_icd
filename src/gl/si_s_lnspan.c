/*
** si_s_lnspan.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_lnspan.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690d4940 -- __glStippleLine */
GLboolean __fastcall __glStippleLine(__GLcontext *gc)
{
    GLint failed, count, stippleRepeat;
    GLint stipple, currentBit, stipplePos, repeat;
    __GLstippleWord bit, outMask, *osp;
    __GLlineState *ls = &gc->state.line;
    GLint w;

    w = gc->polygon.shader.length;
    osp = gc->polygon.shader.stipplePat;
    repeat = gc->line.repeat;
    stippleRepeat = ls->stippleRepeat;
    stipplePos = gc->line.stipplePosition;
    currentBit = 1 << stipplePos;
    stipple = ls->stipple;
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
            if ((stipple & currentBit) == 0) {
                /* Stippled fragment away */
                outMask &= ~bit;
                failed++;
            }

            if (++repeat >= stippleRepeat) {
                stipplePos = (stipplePos + 1) & 0xf;
                currentBit = 1 << stipplePos;
                repeat = 0;
            }

#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
        *osp++ = outMask;
    }

    gc->line.repeat = repeat;
    gc->line.stipplePosition = stipplePos;

    if (failed == 0) {
        return GL_FALSE;
    } else if (failed != gc->polygon.shader.length) {
        return GL_TRUE;
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

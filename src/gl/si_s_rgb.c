/*
** si_s_rgb.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_rgb.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690cc3c0 -- rgb_StoreSpan: rgb_StoreSpan */
GLboolean __fastcall rgb_StoreSpan(__GLcontext *gc)
{
    int x, x1;
    int i;
    __GLfragmentICD frag;
    __GLcolor *cp;
    __GLcolorBufferICD *cfb;
    GLint w;

    w = gc->polygon.shader.length;

    frag.y = gc->polygon.shader.frag.y;
    x = gc->polygon.shader.frag.x;
    x1 = gc->polygon.shader.frag.x + w;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

    for (i = x; i < x1; i++) {
        frag.x = i;
        frag.color = *cp++;

        (*cfb->store)(cfb, &frag);
    }

    return GL_FALSE;
}

/* 690cc430 -- rgb_StoreStippledSpan: rgb_StoreStippledSpan */
GLboolean __fastcall rgb_StoreStippledSpan(__GLcontext *gc)
{
    int x;
    __GLfragmentICD frag;
    __GLcolor *cp;
    __GLcolorBufferICD *cfb;
    __GLstippleWord inMask, bit, *sp;
    GLint count;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    frag.y = gc->polygon.shader.frag.y;
    x = gc->polygon.shader.frag.x;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;

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
                frag.x = x;
                frag.color = *cp;

                (*cfb->store)(cfb, &frag);
            }
            x++;
            cp++;
#if defined(__GL_STIPPLE_MSB)
            bit >>= 1;
#else /* __GL_STIPPLE_MSB */
            bit <<= 1;
#endif /* __GL_STIPPLE_MSB */
        }
    }

    return GL_FALSE;
}

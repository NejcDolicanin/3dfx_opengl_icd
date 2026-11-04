/*
** si_s_span.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_span.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690c54d0 -- __glReadSpan */
GLboolean __fastcall __glReadSpan(__GLcolorBufferICD *cfb, GLint x, GLint y,
                       __GLcolor *results, GLint w)
{
    __GLcontext *gc = cfb->buf.gc;

    while (--w >= 0) {
        (*cfb->readColor)(cfb, x, y, results);
        x++;
        results++;
    }

    return GL_FALSE;
}

/* 690c5670 -- __glFetchSpan */
GLboolean __fastcall __glFetchSpan(__GLcontext *gc)
{
    __GLcolor *fcp;
    __GLcolorBufferICD *cfb;
    GLint x, y;
    GLint w;

    w = gc->polygon.shader.length;

    fcp = gc->polygon.shader.fbcolors;
    cfb = gc->polygon.shader.cfb;
    x = gc->polygon.shader.frag.x;
    y = gc->polygon.shader.frag.y;
    (*cfb->readSpan)(cfb, x, y, fcp, w);

    return GL_FALSE;
}

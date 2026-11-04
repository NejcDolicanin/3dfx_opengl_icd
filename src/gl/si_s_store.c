/*
** si_s_store.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_store.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 6908afd0 -- __glDoStore_SD */
void __fastcall __glDoStore_SD(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GLcontext *gc;
    GLint x, y;

    gc = cfb->buf.gc;

    x = frag->x;
    y = frag->y;

    /* Pixel ownership, scissor */
    if (x < gc->transform.clipX0 || y < gc->transform.clipY0 ||
            x >= gc->transform.clipX1 || y >= gc->transform.clipY1) {
        return;
    }

    if (!(*gc->stencilBuffer.testFunc)(&gc->stencilBuffer, x, y)) {
        /* stencil test failed */
        (*gc->stencilBuffer.failOp)(&gc->stencilBuffer, x, y);
        return;
    }
    if (!(*gc->depthBuffer.store)(&gc->depthBuffer, x, y, frag->z)) {
        /* depth buffer test failed */
        (*gc->stencilBuffer.passDepthFailOp)(&gc->stencilBuffer, x, y);
        return;
    }
    (*gc->stencilBuffer.depthPassOp)(&gc->stencilBuffer, x, y);

    (*gc->procs.cfbStore)( cfb, frag );
}

/* 6908b140 -- __glDoStore_S */
void __fastcall __glDoStore_S(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GLcontext *gc;
    GLint x, y;

    gc = cfb->buf.gc;

    x = frag->x;
    y = frag->y;

    /* Pixel ownership, scissor */
    if (x < gc->transform.clipX0 || y < gc->transform.clipY0 ||
            x >= gc->transform.clipX1 || y >= gc->transform.clipY1) {
        return;
    }

    if (!(*gc->stencilBuffer.testFunc)(&gc->stencilBuffer, x, y)) {
        /* stencil test failed */
        (*gc->stencilBuffer.failOp)(&gc->stencilBuffer, x, y);
        return;
    }
    (*gc->stencilBuffer.depthPassOp)(&gc->stencilBuffer, x, y);

    (*gc->procs.cfbStore)( cfb, frag );
}

/* 6908b1c0 -- __glDoStore_D */
void __fastcall __glDoStore_D(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GLcontext *gc;
    GLint x, y;

    gc = cfb->buf.gc;

    x = frag->x;
    y = frag->y;

    /* Pixel ownership, scissor */
    if (x < gc->transform.clipX0 || y < gc->transform.clipY0 ||
            x >= gc->transform.clipX1 || y >= gc->transform.clipY1) {
        return;
    }

    if (!(*gc->depthBuffer.store)(&gc->depthBuffer, x, y, frag->z)) {
        /* depth buffer test failed */
        return;
    }

    (*gc->procs.cfbStore)( cfb, frag );
}

/* 6908b220 -- __glDoStore */
void __fastcall __glDoStore(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GLcontext *gc;
    GLint x, y;

    gc = cfb->buf.gc;

    x = frag->x;
    y = frag->y;

    /* Pixel ownership, scissor */
    if (x < gc->transform.clipX0 || y < gc->transform.clipY0 ||
            x >= gc->transform.clipX1 || y >= gc->transform.clipY1) {
        return;
    }

    (*gc->procs.cfbStore)( cfb, frag );
}

/* 6908b270 -- __glDoDoubleStore */
void __fastcall __glDoDoubleStore(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GLcontext * gc = cfb->buf.gc;
    cfb = gc->front;
    cfb->store( cfb, frag );
    cfb = gc->back;
    cfb->store( cfb, frag );
}

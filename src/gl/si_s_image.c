/*
** si_s_image.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_image.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 69095c30 -- __glConvertStipple */
void __fastcall __glConvertStipple(__GLcontext *gc)
{
    GLubyte b0, b1, b2, b3, *stipple;
    GLuint *dst;
    GLint i;

    stipple = &gc->state.polygonStipple.stipple[0];
    dst = &gc->polygon.stipple[0];
#ifdef __GL_STIPPLE_MSB
    /*
    ** Convert input stipple bytes which are in little endian MSB format
    ** into a single long word, whose high bit represents the left most
    ** X coordinate of a 32 bit span.
    */
    for (i = 0; i < 32; i++) {
        b0 = *stipple++;
        b1 = *stipple++;
        b2 = *stipple++;
        b3 = *stipple++;
        *dst++ = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }
#else
    /*
    ** Make 32bit form of the stipple for easier rendering.
    */
    for (i = 0; i < 32; i++) {
        b0 = __glMsbToLsbTable[*stipple++];
        b1 = __glMsbToLsbTable[*stipple++];
        b2 = __glMsbToLsbTable[*stipple++];
        b3 = __glMsbToLsbTable[*stipple++];
        *dst++ = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }
#endif
}

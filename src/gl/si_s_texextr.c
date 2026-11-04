/*
** si_s_texextr.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_texextr.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690a2ed0 -- __glExtractTexelXRGB1555_B */
void __fastcall __glExtractTexelXRGB1555_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                        GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLushort *image, texel;

    row++;
    col++;
    image = (GLushort *)level->buffer + (row * level->width) + col;
    texel = image[0];
    result->r = (GLubyte) ((texel >> 10) & 0x1F) * (255.0F/31.0F);
    result->g = (GLubyte) ((texel >>  5) & 0x1F) * (255.0F/31.0F);
    result->b = (GLubyte) ((texel      ) & 0x1F) * (255.0F/31.0F);
}

/* 690a2f60 -- __glExtractTexelRGB332_B */
void __fastcall __glExtractTexelRGB332_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                        GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image, texel;

    row++;
    col++;
    image = (GLubyte *)level->buffer + (row * level->width) + col;
    texel = image[0];
    result->r = (GLubyte) ((texel >> 5) & 0x7) * (255.0F/7.0F);
    result->g = (GLubyte) ((texel >> 2) & 0x7) * (255.0F/7.0F);
    result->b = (GLubyte) ((texel     ) & 0x3) * (255.0F/3.0F);
}

/* 690a2ff0 -- __glExtractTexelRGB565_B */
void __fastcall __glExtractTexelRGB565_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                        GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLushort *image, texel;

    row++;
    col++;
    image = (GLushort *)level->buffer + (row * level->width) + col;
    texel = image[0];
    result->r = (GLubyte) ((texel >> 11) & 0x1F) * (255.0F/31.0F);
    result->g = (GLubyte) ((texel >>  5) & 0x3F) * (255.0F/63.0F);
    result->b = (GLubyte) ((texel      ) & 0x1F) * (255.0F/31.0F);
}

/* 690a3080 -- __glExtractTexelRGBA4_B */
void __fastcall __glExtractTexelRGBA4_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                        GLint img, GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLushort *image, texel;

    row++;
    col++;
    image = (GLushort *)level->buffer + (row * level->width) + col;
    texel = image[0];
    result->r = (GLubyte) ((texel >> 12) & 0x0F) * (255.0F/15.0F);
    result->g = (GLubyte) ((texel >>  8) & 0x0F) * (255.0F/15.0F);
    result->b = (GLubyte) ((texel >>  4) & 0x0F) * (255.0F/15.0F);
    result->a = (GLubyte) ((texel >>  0) & 0x0F) * (255.0F/15.0F);
}

/* 690a3130 -- __glExtractTexelARGB4_B */
void __fastcall __glExtractTexelARGB4_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                        GLint img, GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLushort *image, texel;

    row++;
    col++;
    image = (GLushort *)level->buffer + (row * level->width) + col;
    texel = image[0];
    result->r = (GLubyte) ((texel >>  8) & 0x0F) * (255.0F/15.0F);
    result->g = (GLubyte) ((texel >>  4) & 0x0F) * (255.0F/15.0F);
    result->b = (GLubyte) ((texel      ) & 0x0F) * (255.0F/15.0F);
    result->a = (GLubyte) ((texel >> 12) & 0x0F) * (255.0F/15.0F);
}

/* 690a31e0 -- __glExtractTexelRGBA5551_B */
void __fastcall __glExtractTexelRGBA5551_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                        GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLushort *image, texel;

    row++;
    col++;
    image = (GLushort *)level->buffer + (row * level->width) + col;
    texel = image[0];
    result->r = (GLubyte) ((texel >> 11) & 0x1F) * (255.0F/31.0F);
    result->g = (GLubyte) ((texel >>  6) & 0x1F) * (255.0F/31.0F);
    result->b = (GLubyte) ((texel >>  1) & 0x1F) * (255.0F/31.0F);
    result->a = (GLubyte) ((texel >>  0) & 0x01) * 255.0F;
}

/* 690a3290 -- __glExtractTexelARGB1555_B */
void __fastcall __glExtractTexelARGB1555_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                        GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLushort *image, texel;

    row++;
    col++;
    image = (GLushort *)level->buffer + (row * level->width) + col;
    texel = image[0];
    result->r = (GLubyte) ((texel >> 10) & 0x1F) * (255.0F/31.0F);
    result->g = (GLubyte) ((texel >>  5) & 0x1F) * (255.0F/31.0F);
    result->b = (GLubyte) ((texel      ) & 0x1F) * (255.0F/31.0F);
    result->a = (GLubyte) ((texel >> 15) & 0x01) * 255.0F;
}

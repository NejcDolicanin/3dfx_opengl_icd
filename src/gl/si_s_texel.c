/*
** si_s_texel.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_texel.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690a46f0 -- __glTextureModulateL */
void __fastcall __glTextureModulateL(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat luminance = __GL_UB_TO_FLOAT(texel->r);

    color->r = luminance * color->r;
    color->g = luminance * color->g;
    color->b = luminance * color->b;
}

/* 690a4720 -- __glTextureModulateLA */
void __fastcall __glTextureModulateLA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat luminance = __GL_UB_TO_FLOAT(texel->r);
    __GLfloat alpha = __GL_UB_TO_FLOAT(texel->a);

    color->r = luminance * color->r;
    color->g = luminance * color->g;
    color->b = luminance * color->b;
    color->a = alpha * color->a;
}

/* 690a4770 -- __glTextureModulateRGB */
void __fastcall __glTextureModulateRGB(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->r = __GL_UB_TO_FLOAT(texel->r) * color->r;
    color->g = __GL_UB_TO_FLOAT(texel->g) * color->g;
    color->b = __GL_UB_TO_FLOAT(texel->b) * color->b;
}

/* 690a47b0 -- __glTextureModulateRGBA */
void __fastcall __glTextureModulateRGBA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->r = __GL_UB_TO_FLOAT(texel->r) * color->r;
    color->g = __GL_UB_TO_FLOAT(texel->g) * color->g;
    color->b = __GL_UB_TO_FLOAT(texel->b) * color->b;
    color->a = __GL_UB_TO_FLOAT(texel->a) * color->a;
}

/* 690a4800 -- __glTextureModulateA */
void __fastcall __glTextureModulateA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->a = __GL_UB_TO_FLOAT(texel->a) * color->a;
}

/* 690a4820 -- __glTextureModulateI */
void __fastcall __glTextureModulateI(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat intensity = __GL_UB_TO_FLOAT(texel->r);

    color->r = intensity * color->r;
    color->g = intensity * color->g;
    color->b = intensity * color->b;
    color->a = intensity * color->a;
}

/* 690a4850 -- __glTextureModulateCI */
void __fastcall __glTextureModulateCI(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat index = __GL_UB_TO_FLOAT(texel->r);

    color->r = index * color->r;
}

/* 690a4870 -- __glTextureDecalRGB */
void __fastcall __glTextureDecalRGB(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->r = __GL_UB_TO_FLOAT(texel->r) * gc->frontBuffer.redScale;
    color->g = __GL_UB_TO_FLOAT(texel->g) * gc->frontBuffer.greenScale;
    color->b = __GL_UB_TO_FLOAT(texel->b) * gc->frontBuffer.blueScale;
}

/* 690a48c0 -- __glTextureDecalRGBA */
void __fastcall __glTextureDecalRGBA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat a = __GL_UB_TO_FLOAT(texel->a);
    __GLfloat oma = __GL_ONE_F - a;

    color->r = oma * color->r
        + a * __GL_UB_TO_FLOAT(texel->r) * gc->frontBuffer.redScale;
    color->g = oma * color->g
        + a * __GL_UB_TO_FLOAT(texel->g) * gc->frontBuffer.greenScale;
    color->b = oma * color->b
        + a * __GL_UB_TO_FLOAT(texel->b) * gc->frontBuffer.blueScale;
}

/* 690a4b30 -- __glTextureBlendA */
void __fastcall __glTextureBlendA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->a = __GL_UB_TO_FLOAT(texel->a) * color->a;
}

/* 690a4bb0 -- __glTextureReplaceL */
void __fastcall __glTextureReplaceL(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat luminance = __GL_UB_TO_FLOAT(texel->r);

    color->r = luminance * gc->frontBuffer.redScale;
    color->g = luminance * gc->frontBuffer.greenScale;
    color->b = luminance * gc->frontBuffer.blueScale;
}

/* 690a4bf0 -- __glTextureReplaceLA */
void __fastcall __glTextureReplaceLA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat luminance = __GL_UB_TO_FLOAT(texel->r);
    __GLfloat alpha = __GL_UB_TO_FLOAT(texel->a);

    color->r = luminance * gc->frontBuffer.redScale;
    color->g = luminance * gc->frontBuffer.greenScale;
    color->b = luminance * gc->frontBuffer.blueScale;
    color->a = alpha * gc->frontBuffer.alphaScale;
}

/* 690a4c40 -- __glTextureReplaceRGB */
void __fastcall __glTextureReplaceRGB(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->r = __GL_UB_TO_FLOAT(texel->r) * gc->frontBuffer.redScale;
    color->g = __GL_UB_TO_FLOAT(texel->g) * gc->frontBuffer.greenScale;
    color->b = __GL_UB_TO_FLOAT(texel->b) * gc->frontBuffer.blueScale;
}

/* 690a4c90 -- __glTextureReplaceRGBA */
void __fastcall __glTextureReplaceRGBA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->r = __GL_UB_TO_FLOAT(texel->r) * gc->frontBuffer.redScale;
    color->g = __GL_UB_TO_FLOAT(texel->g) * gc->frontBuffer.greenScale;
    color->b = __GL_UB_TO_FLOAT(texel->b) * gc->frontBuffer.blueScale;
    color->a = __GL_UB_TO_FLOAT(texel->a) * gc->frontBuffer.alphaScale;
}

/* 690a4cf0 -- __glTextureReplaceA */
void __fastcall __glTextureReplaceA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->a = __GL_UB_TO_FLOAT(texel->a) * gc->frontBuffer.alphaScale;
}

/* 690a4d10 -- __glTextureReplaceI */
void __fastcall __glTextureReplaceI(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat intensity = __GL_UB_TO_FLOAT(texel->r);

    color->r = intensity * gc->frontBuffer.redScale;
    color->g = intensity * gc->frontBuffer.greenScale;
    color->b = intensity * gc->frontBuffer.blueScale;
    color->a = intensity * gc->frontBuffer.alphaScale;
}

/* 690a4d50 -- __glTextureReplaceCI */
void __fastcall __glTextureReplaceCI(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->r = __GL_UB_TO_FLOAT(texel->r);
}

/* 690a4d70 -- __glTextureAddCI */
void __fastcall __glTextureAddCI(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->r = __GL_UB_TO_FLOAT(texel->r) + color->r;
}

/* 690a4940 -- __glTextureBlendL */
void __fastcall __glTextureBlendL(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat l = __GL_UB_TO_FLOAT(texel->r);
    __GLfloat oml = __GL_ONE_F - l;
    __GLcolor *cc = &gc->state.texture.unit[0].env[0].color;

    color->r = oml * color->r + l * cc->r;
    color->g = oml * color->g + l * cc->g;
    color->b = oml * color->b + l * cc->b;
}

/* 690a4990 -- __glTextureBlendLA */
void __fastcall __glTextureBlendLA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat l = __GL_UB_TO_FLOAT(texel->r);
    __GLfloat oml = __GL_ONE_F - l;
    __GLcolor *cc = &gc->state.texture.unit[0].env[0].color;

    color->r = oml * color->r + l * cc->r;
    color->g = oml * color->g + l * cc->g;
    color->b = oml * color->b + l * cc->b;
    color->a = __GL_UB_TO_FLOAT(texel->a) * color->a;
}

/* 690a4a00 -- __glTextureBlendRGB */
void __fastcall __glTextureBlendRGB(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat r = __GL_UB_TO_FLOAT(texel->r);
    __GLfloat g = __GL_UB_TO_FLOAT(texel->g);
    __GLfloat b = __GL_UB_TO_FLOAT(texel->b);
    __GLcolor *cc = &gc->state.texture.unit[0].env[0].color;

    color->r = (__GL_ONE_F - r) * color->r + r * cc->r;
    color->g = (__GL_ONE_F - g) * color->g + g * cc->g;
    color->b = (__GL_ONE_F - b) * color->b + b * cc->b;
}

/* 690a4a90 -- __glTextureBlendRGBA */
void __fastcall __glTextureBlendRGBA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat r = __GL_UB_TO_FLOAT(texel->r);
    __GLfloat g = __GL_UB_TO_FLOAT(texel->g);
    __GLfloat b = __GL_UB_TO_FLOAT(texel->b);
    __GLcolor *cc = &gc->state.texture.unit[0].env[0].color;

    color->r = (__GL_ONE_F - r) * color->r + r * cc->r;
    color->g = (__GL_ONE_F - g) * color->g + g * cc->g;
    color->b = (__GL_ONE_F - b) * color->b + b * cc->b;
    color->a = __GL_UB_TO_FLOAT(texel->a) * color->a;
}

/* 690a4b50 -- __glTextureBlendI */
void __fastcall __glTextureBlendI(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat i = __GL_UB_TO_FLOAT(texel->r);
    __GLfloat omi = __GL_ONE_F - i;
    __GLcolor *cc = &gc->state.texture.unit[0].env[0].color;

    color->r = omi * color->r + i * cc->r;
    color->g = omi * color->g + i * cc->g;
    color->b = omi * color->b + i * cc->b;
    color->a = omi * color->a + i * cc->a;
}

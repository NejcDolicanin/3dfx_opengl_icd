/*
** sst_color.c -- glColor* of the hardware path (__glsstim_Color*) and the colour packers.
*/
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "glshade.h"
#include "si_names.h"
#include "icdglob.h"
#include "icdprotos.h"
#include <glide.h>
#include "og.h"
#include <string.h>
#include <stdlib.h>

#define CLAMP_255(x)                                \
    x *= 255.0f;                                        \
    if (x > 255.0f) x = 255.0f;                         \
    else if (x < __GL_ZERO_F) x = __GL_ZERO_F;
/* ---- current colour packing of the hardware path (ARGB 8888) ---- */
/* float [0, 256) -> integer part, through the bits of f + 1.5 * 2^15 (8 fraction bits) */
#define UB_MAGIC        ((__GLfloat)49152.0)
#define FTOUB(u, v)     ((u).f = (v) + UB_MAGIC, (((u).i & 0x7fffff) - 0x400000) >> 8)

/* 69048f80 -- __glsstim_Color3ub: __glim_Color3ub (copy of 690a5bd0) */
void __stdcall __glsstim_Color3ub(GLubyte r, GLubyte g, GLubyte b)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_UB_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_UB_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_UB_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 69048ff0 -- __glsstim_Color3ubv */
void __stdcall __glsstim_Color3ubv(const GLubyte *v)
{
  __GL_SETUP();

  gc->state.current.userColor.r = (__GLfloat)(gc->constants.uByteToFloat[v[0]]);
  gc->state.current.userColor.g = (__GLfloat)(gc->constants.uByteToFloat[v[1]]);
  gc->state.current.userColor.b = (__GLfloat)(gc->constants.uByteToFloat[v[2]]);
  gc->state.current.userColor.a = __GL_ONE_F;
  gc->colorSet = 1;
  (*gc->procs.applyColor)(gc);
}

/* 69049060 -- __glsstim_Color3b (the SI __glim_ body, as its gl twin 690a5e20) */
void __stdcall __glsstim_Color3b(GLbyte r, GLbyte g, GLbyte b)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_B_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_B_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_B_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690490e0 -- __glsstim_Color3bv */
void __stdcall __glsstim_Color3bv(const GLbyte *v)
{
  __GL_SETUP();

  gc->state.current.userColor.r = (float)(v[0] * 2 + 1) * gc->constants.oneOver255;
  gc->state.current.userColor.g = (float)(v[1] * 2 + 1) * gc->constants.oneOver255;
  gc->state.current.userColor.b = (float)(v[2] * 2 + 1) * gc->constants.oneOver255;
  gc->state.current.userColor.a = __GL_ONE_F;
  gc->colorSet = 1;
  (*gc->procs.applyColor)(gc);
}

/* 69049160 -- __glsstim_Color3d: glColor3d, hardware path */
void __stdcall __glsstim_Color3d(GLdouble red, GLdouble green, GLdouble blue)
{
    __GL_SETUP();

    gc->state.current.userColor.a = __GL_ONE_F;
    gc->state.current.userColor.r = (__GLfloat)red;
    gc->colorSet = GL_TRUE;
    gc->state.current.userColor.g = (__GLfloat)green;
    gc->state.current.userColor.b = (__GLfloat)blue;
    (*gc->procs.applyColor)(gc);
}

/* 69049200 -- __glsstim_Color3f (packed-colour path): userColor, color = 0..255 clamped, packedColor.
** color is first set to the input and scaled in place (VC6 forwards the stored values; the unscaled
** g / b stores stay, as in the original) */
void __stdcall __glsstim_Color3f(GLfloat r, GLfloat g, GLfloat b)
{
  __GL_SETUP();

  gc->state.current.userColor.r = r;
  gc->state.current.userColor.g = g;
  gc->state.current.userColor.b = b;
  gc->state.current.userColor.a = __GL_ONE_F;
  gc->state.current.color.r = r;
  gc->state.current.color.g = g;
  gc->state.current.color.b = b;
  gc->state.current.color.a = 255.0f;
  gc->state.current.color.r *= 255.0f;
  if (!(gc->state.current.color.r <= 255.0f)) {
    gc->state.current.color.r = 255.0f;
  }
  else if (gc->state.current.color.r < 0.0f) {
    gc->state.current.color.r = 0.0f;
  }
  gc->state.current.color.g *= 255.0f;
  if (!(gc->state.current.color.g <= 255.0f)) {
    gc->state.current.color.g = 255.0f;
  }
  else if (gc->state.current.color.g < 0.0f) {
    gc->state.current.color.g = 0.0f;
  }
  gc->state.current.color.b *= 255.0f;
  if (!(gc->state.current.color.b <= 255.0f)) {
    gc->state.current.color.b = 255.0f;
  }
  else if (gc->state.current.color.b < 0.0f) {
    gc->state.current.color.b = 0.0f;
  }
  gc->state.current.packedColor = packColor255(gc, &gc->state.current.color);
}

/* 69049330 -- __glsstim_Color3fv: glColor3fv of the hardware path (alpha 1) */
void __stdcall __glsstim_Color3fv(const GLfloat *v)
{
    __GLcontext *gc = __glCurrentGC();

    gc->state.current.color.r = v[0];
    gc->state.current.userColor.r = v[0];
    gc->state.current.color.g = v[1];
    gc->state.current.userColor.g = v[1];
    gc->state.current.color.b = v[2];
    gc->state.current.userColor.b = v[2];
    gc->state.current.color.a = 255.0f;
    gc->state.current.userColor.a = __GL_ONE_F;
    CLAMP_255(gc->state.current.color.r)
    CLAMP_255(gc->state.current.color.g)
    CLAMP_255(gc->state.current.color.b)
    gc->state.current.packedColor = packColor255(gc, &gc->state.current.color);
}

/* 690494f0 -- __glsstim_Color3iv */
void __stdcall __glsstim_Color3iv(const GLint *v)
{
  __GL_SETUP();

  gc->state.current.userColor.r = ((float)v[0] + (float)v[0] + 1.0f) * gc->constants.oneOver4294965000;
  gc->state.current.userColor.g = ((float)v[1] + (float)v[1] + 1.0f) * gc->constants.oneOver4294965000;
  gc->state.current.userColor.b = ((float)v[2] + (float)v[2] + 1.0f) * gc->constants.oneOver4294965000;
  gc->state.current.userColor.a = __GL_ONE_F;
  gc->colorSet = 1;
  (*gc->procs.applyColor)(gc);
}

/* 690495f0 -- __glsstim_Color3uiv */
void __stdcall __glsstim_Color3uiv(const GLuint *v)
{
  __GL_SETUP();

  gc->state.current.userColor.r = (float)v[0] * gc->constants.oneOver4294965000;
  gc->state.current.userColor.g = (float)v[1] * gc->constants.oneOver4294965000;
  gc->state.current.userColor.b = (float)v[2] * gc->constants.oneOver4294965000;
  gc->state.current.userColor.a = __GL_ONE_F;
  gc->colorSet = 1;
  (*gc->procs.applyColor)(gc);
}

/* 69049680 -- __glsstim_Color3s (the SI __glim_ body, as its gl twin 690a6270) */
void __stdcall __glsstim_Color3s(GLshort r, GLshort g, GLshort b)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_S_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_S_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_S_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 69049700 -- __glsstim_Color3sv */
void __stdcall __glsstim_Color3sv(const GLshort *v)
{
  __GL_SETUP();

  gc->state.current.userColor.r = (float)(v[0] * 2 + 1) * gc->constants.oneOver65535;
  gc->state.current.userColor.g = (float)(v[1] * 2 + 1) * gc->constants.oneOver65535;
  gc->state.current.userColor.b = (float)(v[2] * 2 + 1) * gc->constants.oneOver65535;
  gc->state.current.userColor.a = __GL_ONE_F;
  gc->colorSet = 1;
  (*gc->procs.applyColor)(gc);
}

/* 69049780 -- __glsstim_Color3us */
void __stdcall __glsstim_Color3us(GLushort r, GLushort g, GLushort b)
{
  __GL_SETUP();

  gc->state.current.userColor.r = (float)r * gc->constants.oneOver65535;
  gc->state.current.userColor.g = (float)g * gc->constants.oneOver65535;
  gc->state.current.userColor.b = (float)b * gc->constants.oneOver65535;
  gc->state.current.userColor.a = __GL_ONE_F;
  gc->colorSet = 1;
  (*gc->procs.applyColor)(gc);
}

/* 69049800 -- __glsstim_Color3usv */
void __stdcall __glsstim_Color3usv(const GLushort *v)
{
  __GL_SETUP();

  gc->state.current.userColor.r = (float)v[0] * gc->constants.oneOver65535;
  gc->state.current.userColor.g = (float)v[1] * gc->constants.oneOver65535;
  gc->state.current.userColor.b = (float)v[2] * gc->constants.oneOver65535;
  gc->state.current.userColor.a = __GL_ONE_F;
  gc->colorSet = 1;
  (*gc->procs.applyColor)(gc);
}

/* ---- the glcore twin in src/gl/si_s_capi.c, copied (the hardware module has its own copy of the code) ---- */
/* 69049880 -- __glsstim_Color4ub: __glim_Color4ub (copy of 690a6470) */
void __stdcall __glsstim_Color4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_UB_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_UB_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_UB_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_UB_TO_FLOAT(a);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 69049900 -- __glsstim_Color4ubv: glColor4ubv of the hardware path: the user colour from the table, the RGBA bytes
** swizzled to ARGB, the current colour 0..255 */
void __stdcall __glsstim_Color4ubv(const GLubyte *v)
{
    GLuint w;
    __GL_SETUP();

    gc->state.current.userColor.r = __GL_UB_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_UB_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_UB_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_UB_TO_FLOAT(v[3]);
    w = *(const GLuint *)v;
    gc->state.current.packedColor = ((w >> 16) & 0xff) | ((w & 0xff) << 16) | (w & 0xff00ff00);
    gc->state.current.color.r = (__GLfloat)(GLint)v[0];
    gc->state.current.color.g = (__GLfloat)(GLint)v[1];
    gc->state.current.color.b = (__GLfloat)(GLint)v[2];
    gc->state.current.color.a = (__GLfloat)(GLint)v[3];
}

/* 690499d0 -- __glsstim_Color4b (the SI __glim_ body, as its gl twin 690a6560) */
void __stdcall __glsstim_Color4b(GLbyte r, GLbyte g, GLbyte b, GLbyte a)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_B_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_B_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_B_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_B_TO_FLOAT(a);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 69049a60 -- __glsstim_Color4bv */
void __stdcall __glsstim_Color4bv(const GLbyte *v)
{
  __GL_SETUP();

  gc->state.current.userColor.r = (float)(v[0] * 2 + 1) * gc->constants.oneOver255;
  gc->state.current.userColor.g = (float)(v[1] * 2 + 1) * gc->constants.oneOver255;
  gc->state.current.userColor.b = (float)(v[2] * 2 + 1) * gc->constants.oneOver255;
  gc->state.current.userColor.a = (float)(v[3] * 2 + 1) * gc->constants.oneOver255;
  gc->colorSet = 1;
  (*gc->procs.applyColor)(gc);
}

/* 69049af0 -- __glsstim_Color4d: glColor4d, hardware path */
void __stdcall __glsstim_Color4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    __GL_SETUP();

    gc->colorSet = GL_TRUE;
    gc->state.current.userColor.r = (__GLfloat)red;
    gc->state.current.userColor.g = (__GLfloat)green;
    gc->state.current.userColor.b = (__GLfloat)blue;
    gc->state.current.userColor.a = (__GLfloat)alpha;
    (*gc->procs.applyColor)(gc);
}

/* 69049e00 -- __glsstim_Color4fv: glColor4fv of the hardware path: user and current colour, the current one scaled to
** 0..255, clamped and packed */
void __stdcall __glsstim_Color4fv(const GLfloat *v)
{
    __GLcontext *gc = __glCurrentGC();

    gc->state.current.color.r = v[0];
    gc->state.current.userColor.r = v[0];
    gc->state.current.color.g = v[1];
    gc->state.current.userColor.g = v[1];
    gc->state.current.color.b = v[2];
    gc->state.current.userColor.b = v[2];
    gc->state.current.color.a = v[3];
    gc->state.current.userColor.a = v[3];
    CLAMP_255(gc->state.current.color.r)
    CLAMP_255(gc->state.current.color.g)
    CLAMP_255(gc->state.current.color.b)
    CLAMP_255(gc->state.current.color.a)
    gc->state.current.packedColor = packColor255(gc, &gc->state.current.color);
}

/* 6904a120 -- __glsstim_Color4uiv: __glim_Color4uiv (copy of 690a6960) */
void __stdcall __glsstim_Color4uiv(const GLuint v[4])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_UI_TO_FLOAT(v[0]);
    gc->state.current.userColor.g = __GL_UI_TO_FLOAT(v[1]);
    gc->state.current.userColor.b = __GL_UI_TO_FLOAT(v[2]);
    gc->state.current.userColor.a = __GL_UI_TO_FLOAT(v[3]);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 6904a1b0 -- __glsstim_Color4s (the SI __glim_ body, as its gl twin 690a69f0) */
void __stdcall __glsstim_Color4s(GLshort r, GLshort g, GLshort b, GLshort a)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_S_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_S_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_S_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_S_TO_FLOAT(a);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 6904a240 -- __glsstim_Color4sv */
void __stdcall __glsstim_Color4sv(const GLshort *v)
{
  __GL_SETUP();

  gc->state.current.userColor.r = (float)(v[0] * 2 + 1) * gc->constants.oneOver65535;
  gc->state.current.userColor.g = (float)(v[1] * 2 + 1) * gc->constants.oneOver65535;
  gc->state.current.userColor.b = (float)(v[2] * 2 + 1) * gc->constants.oneOver65535;
  gc->state.current.userColor.a = (float)(v[3] * 2 + 1) * gc->constants.oneOver65535;
  gc->colorSet = 1;
  (*gc->procs.applyColor)(gc);
}

/* 6904a2d0 -- __glsstim_Color4us */
void __stdcall __glsstim_Color4us(GLushort r, GLushort g, GLushort b, GLushort a)
{
  __GL_SETUP();

  gc->state.current.userColor.r = (float)r * gc->constants.oneOver65535;
  gc->state.current.userColor.g = (float)g * gc->constants.oneOver65535;
  gc->state.current.userColor.b = (float)b * gc->constants.oneOver65535;
  gc->state.current.userColor.a = (float)a * gc->constants.oneOver65535;
  gc->colorSet = 1;
  (*gc->procs.applyColor)(gc);
}

/* 6904a360 -- __glsstim_Color4usv */
void __stdcall __glsstim_Color4usv(const GLushort *v)
{
  __GL_SETUP();

  gc->state.current.userColor.r = (float)v[0] * gc->constants.oneOver65535;
  gc->state.current.userColor.g = (float)v[1] * gc->constants.oneOver65535;
  gc->state.current.userColor.b = (float)v[2] * gc->constants.oneOver65535;
  gc->state.current.userColor.a = (float)v[3] * gc->constants.oneOver65535;
  gc->colorSet = 1;
  (*gc->procs.applyColor)(gc);
}

/* 6904a3f0 -- hwPackColor: packColor of a vertex: *vx->color (4 floats 0..255) as ARGB into packedColor:
** x + 49152.0f leaves x * 256 + 0x400000 in the mantissa */
void __fastcall hwPackColor(__GLcontext *gc, __GLvertexICD *vx)
{
  const __GLfloat *c;
  union { float f; GLint i; } r, g, b, a;

  c = &vx->color->r;
  a.f = c[3] + 49152.0f;
  r.f = c[0] + 49152.0f;
  g.f = c[1] + 49152.0f;
  b.f = (__GLfloat)(c[2]) + 49152.0f;   /* cast: the original's fadd scheduling */
  vx->packedColor =
       ((((((r.i & 0x7fffff) + 0xc00000) << 8) & 0xffff00ff) | ((g.i & 0x7fffff) - 0x400000)) & 0xffffff00) |
       (((b.i & 0x7fffff) - 0x400000) >> 8) | ((a.i & 0xffffff00) << 16);
}

/* 6904a480 -- hwPackVertexColor: as 6904a3f0 from the front colour colors[0] */
void __fastcall hwPackVertexColor(__GLcontext *gc, __GLvertexICD *vx)
{
  union { float f; GLint i; } r, g, b, a;

  a.f = vx->colors[0].a + 49152.0f;
  r.f = vx->colors[0].r + 49152.0f;
  g.f = vx->colors[0].g + 49152.0f;
  b.f = (__GLfloat)(vx->colors[0].b) + 49152.0f;   /* cast: the original's fadd scheduling */
  vx->packedColor =
       ((((((r.i & 0x7fffff) + 0xc00000) << 8) & 0xffff00ff) | ((g.i & 0x7fffff) - 0x400000)) & 0xffffff00) |
       (((b.i & 0x7fffff) - 0x400000) >> 8) | ((a.i & 0xffffff00) << 16);
}

/* 6904a510 -- packColor255: pack a 0..255 colour into ARGB */
GLuint __fastcall packColor255(__GLcontext *gc, const __GLcolor *c)
{
    union { __GLfloat f; GLint i; } ur, ug, ub, ua;
    GLint r, g, b, a;

    a = FTOUB(ua, c->a);
    r = FTOUB(ur, c->r);
    g = FTOUB(ug, c->g);
    b = FTOUB(ub, c->b);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

/*
** 69049b90 -- __glsstim_Color4f: glColor4f of the hardware path: the user colour, and the current colour scaled to
** 0..255 and packed to ARGB at once when every component is in range; otherwise the scaled
** components are clamped and packed by 6904a510.
*/
void __stdcall __glsstim_Color4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    union { __GLfloat f; GLint i; } u;
    GLint ir, ig, ib, ia;
    __GLcontext *gc = __glCurrentGC();

    gc->state.current.userColor.r = r;
    gc->state.current.userColor.g = g;
    gc->state.current.userColor.b = b;
    gc->state.current.userColor.a = a;
    r *= 255.0;
    g *= 255.0;
    b *= 255.0;
    a *= 255.0;
    ir = FTOUB(u, r);
    ig = FTOUB(u, g);
    ib = FTOUB(u, b);
    ia = FTOUB(u, a);
    if (((ia | ib | ig | ir) & 0xffffff00) == 0) {
        gc->state.current.color.r = r;
        gc->state.current.color.g = g;
        gc->state.current.color.b = b;
        gc->state.current.color.a = a;
        gc->state.current.packedColor = (((ia << 8 | ir) << 8 | ig) << 8) | ib;
        return;
    }
    gc->state.current.color = gc->state.current.userColor;
    CLAMP_255(gc->state.current.color.r)
    CLAMP_255(gc->state.current.color.g)
    CLAMP_255(gc->state.current.color.b)
    CLAMP_255(gc->state.current.color.a)
    gc->state.current.packedColor = packColor255(gc, &gc->state.current.color);
}

#pragma optimize("w", on)
/* 6904a000 -- __glsstim_Color4iv -- compiled assuming no aliasing in the original */
void __stdcall __glsstim_Color4iv(const GLint *v)
{
  __GL_SETUP();

  gc->state.current.userColor.r = ((float)v[0] + (float)v[0] + 1.0f) * gc->constants.oneOver4294965000;
  gc->state.current.userColor.g = ((float)v[1] + (float)v[1] + 1.0f) * gc->constants.oneOver4294965000;
  gc->state.current.userColor.b = ((float)v[2] + (float)v[2] + 1.0f) * gc->constants.oneOver4294965000;
  gc->state.current.userColor.a = ((float)v[3] + (float)v[3] + 1.0f) * gc->constants.oneOver4294965000;
  gc->colorSet = 1;
  (*gc->procs.applyColor)(gc);
}
#pragma optimize("", on)

/*
** si_px_api.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/px_api.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"
#include <stdlib.h>

/* ---- OpenGL 1.2 packed pixel types (FX_GL_VERSION 1.2; not in the original) ---------------------------------------
** The original takes the five EXT_packed_pixels types.  1.2 adds GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_BYTE_2_3_3_REV
** and the _REV forms.  They are converted on the CPU, outside the span pipeline: an image coming in (glTexImage,
** glTexSubImage, glDrawPixels) is first rewritten as tightly packed GL_UNSIGNED_BYTE components of the same format,
** an image going out (glReadPixels, glGetTexImage) is fetched as GL_UNSIGNED_BYTE and packed into the caller's
** buffer; the glPixelStore modes are applied here and set to their defaults around the inner call.  Not handled:
** these types inside display lists. */
#define GL_UNSIGNED_BYTE_2_3_3_REV_12       0x8362
#define GL_UNSIGNED_SHORT_5_6_5_12          0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV_12      0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV_12    0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV_12    0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV_12      0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV_12   0x8368

/* a 1.2 packed type, while 1.2 is offered: 3 or 4 components and its element size in *esize, else 0 */
GLint __fastcall __glPacked12Type(GLenum type, GLint *esize)
{
    GLint dummy;

    if (esize == NULL) esize = &dummy;
    if (g_glVersion < 12) return 0;
    switch (type) {
      case GL_UNSIGNED_BYTE_2_3_3_REV_12:     *esize = 1; return 3;
      case GL_UNSIGNED_SHORT_5_6_5_12:
      case GL_UNSIGNED_SHORT_5_6_5_REV_12:    *esize = 2; return 3;
      case GL_UNSIGNED_SHORT_4_4_4_4_REV_12:
      case GL_UNSIGNED_SHORT_1_5_5_5_REV_12:  *esize = 2; return 4;
      case GL_UNSIGNED_INT_8_8_8_8_REV_12:
      case GL_UNSIGNED_INT_2_10_10_10_REV_12: *esize = 4; return 4;
    }
    return 0;
}

/* the format must have as many components as the type has fields (GL_INVALID_OPERATION otherwise) */
GLboolean __fastcall __glPacked12FormatOK(GLenum format, GLenum type)
{
    GLboolean ok;

    if (__glPacked12Type(type, NULL) == 3)
        ok = (GLboolean)(format == GL_RGB);
    else
        ok = (GLboolean)(format == GL_RGBA || format == 0x80E1 /* GL_BGRA */ || format == 0x8000 /* GL_ABGR_EXT */);
    if (!ok)
        __glSetError(GL_INVALID_OPERATION);
    return ok;
}

#define P12_UP(v, max)      ((GLubyte)(((v) * 255u + (max) / 2u) / (max)))      /* field -> byte */
#define P12_DOWN(c, max)    (((GLuint)(c) * (max) + 127u) / 255u)               /* byte -> field */

/* the fields of one element, first component first, as bytes */
static void packed12Split(GLenum type, GLuint v, GLubyte *c)
{
    switch (type) {
      case GL_UNSIGNED_BYTE_2_3_3_REV_12:
        c[0] = P12_UP(v & 7, 7); c[1] = P12_UP((v >> 3) & 7, 7); c[2] = P12_UP((v >> 6) & 3, 3);
        break;
      case GL_UNSIGNED_SHORT_5_6_5_12:
        c[0] = P12_UP((v >> 11) & 31, 31); c[1] = P12_UP((v >> 5) & 63, 63); c[2] = P12_UP(v & 31, 31);
        break;
      case GL_UNSIGNED_SHORT_5_6_5_REV_12:
        c[0] = P12_UP(v & 31, 31); c[1] = P12_UP((v >> 5) & 63, 63); c[2] = P12_UP((v >> 11) & 31, 31);
        break;
      case GL_UNSIGNED_SHORT_4_4_4_4_REV_12:
        c[0] = P12_UP(v & 15, 15); c[1] = P12_UP((v >> 4) & 15, 15);
        c[2] = P12_UP((v >> 8) & 15, 15); c[3] = P12_UP((v >> 12) & 15, 15);
        break;
      case GL_UNSIGNED_SHORT_1_5_5_5_REV_12:
        c[0] = P12_UP(v & 31, 31); c[1] = P12_UP((v >> 5) & 31, 31);
        c[2] = P12_UP((v >> 10) & 31, 31); c[3] = (GLubyte)((v & 0x8000) ? 255 : 0);
        break;
      case GL_UNSIGNED_INT_8_8_8_8_REV_12:
        c[0] = (GLubyte)v; c[1] = (GLubyte)(v >> 8); c[2] = (GLubyte)(v >> 16); c[3] = (GLubyte)(v >> 24);
        break;
      case GL_UNSIGNED_INT_2_10_10_10_REV_12:
        c[0] = P12_UP(v & 1023, 1023); c[1] = P12_UP((v >> 10) & 1023, 1023);
        c[2] = P12_UP((v >> 20) & 1023, 1023); c[3] = P12_UP(v >> 30, 3);
        break;
    }
}

static GLuint packed12Join(GLenum type, const GLubyte *c)
{
    switch (type) {
      case GL_UNSIGNED_BYTE_2_3_3_REV_12:
        return P12_DOWN(c[0], 7) | (P12_DOWN(c[1], 7) << 3) | (P12_DOWN(c[2], 3) << 6);
      case GL_UNSIGNED_SHORT_5_6_5_12:
        return (P12_DOWN(c[0], 31) << 11) | (P12_DOWN(c[1], 63) << 5) | P12_DOWN(c[2], 31);
      case GL_UNSIGNED_SHORT_5_6_5_REV_12:
        return P12_DOWN(c[0], 31) | (P12_DOWN(c[1], 63) << 5) | (P12_DOWN(c[2], 31) << 11);
      case GL_UNSIGNED_SHORT_4_4_4_4_REV_12:
        return P12_DOWN(c[0], 15) | (P12_DOWN(c[1], 15) << 4) | (P12_DOWN(c[2], 15) << 8) | (P12_DOWN(c[3], 15) << 12);
      case GL_UNSIGNED_SHORT_1_5_5_5_REV_12:
        return P12_DOWN(c[0], 31) | (P12_DOWN(c[1], 31) << 5) | (P12_DOWN(c[2], 31) << 10) | (c[3] >= 128 ? 0x8000u : 0);
      case GL_UNSIGNED_INT_8_8_8_8_REV_12:
        return (GLuint)c[0] | ((GLuint)c[1] << 8) | ((GLuint)c[2] << 16) | ((GLuint)c[3] << 24);
      case GL_UNSIGNED_INT_2_10_10_10_REV_12:
        return P12_DOWN(c[0], 1023) | (P12_DOWN(c[1], 1023) << 10) | (P12_DOWN(c[2], 1023) << 20) | (P12_DOWN(c[3], 3) << 30);
    }
    return 0;
}

/* where the caller's image starts and how far its rows are apart, by the glPixelStore modes */
static GLubyte *packed12Rows(const __GLpixelPackMode *m, GLsizei w, GLint esize, const GLvoid *pixels, GLint *stride)
{
    GLint groups = m->lineLength ? (GLint)m->lineLength : w;
    GLint bytes = groups * esize;
    GLint a = (GLint)m->alignment;

    if (esize < a)
        bytes = (bytes + a - 1) / a * a;
    *stride = bytes;
    return (GLubyte *)pixels + m->skipLines * bytes + m->skipPixels * esize;
}

static const __GLpixelPackMode s_packed12Tight = { GL_FALSE, GL_FALSE, { 0, 0 }, 0, 0, 0, 1 };

/* an incoming image of a 1.2 packed type as a new, tightly packed GL_UNSIGNED_BYTE image; the unpack modes are set
** to their defaults, the caller's kept in *saved.  NULL: out of memory (error set).  __glPacked12Done frees it */
GLubyte *__fastcall __glPacked12Unpack(__GLcontext *gc, GLsizei w, GLsizei h, GLenum type, const GLvoid *pixels,
                                       __GLpixelPackMode *saved)
{
    GLint esize, n = __glPacked12Type(type, &esize), stride, x, y;
    GLubyte *tmp, *o;
    const GLubyte *row, *e;
    GLuint v;

    tmp = (GLubyte *)malloc((size_t)w * h * n);
    if (tmp == NULL) {
        __glSetError(GL_OUT_OF_MEMORY);
        return NULL;
    }
    *saved = gc->state.pixel.unpackModes;
    row = packed12Rows(saved, w, esize, pixels, &stride);
    o = tmp;
    for (y = 0; y < h; y++, row += stride) {
        for (x = 0, e = row; x < w; x++, e += esize, o += n) {
            if (esize == 1) {
                v = e[0];
            } else if (esize == 2) {
                v = saved->swapEndian ? (((GLuint)e[0] << 8) | e[1]) : (((GLuint)e[1] << 8) | e[0]);
            } else if (saved->swapEndian) {
                v = ((GLuint)e[0] << 24) | ((GLuint)e[1] << 16) | ((GLuint)e[2] << 8) | e[3];
            } else {
                v = ((GLuint)e[3] << 24) | ((GLuint)e[2] << 16) | ((GLuint)e[1] << 8) | e[0];
            }
            packed12Split(type, v, o);
        }
    }
    gc->state.pixel.unpackModes = s_packed12Tight;
    return tmp;
}

void __fastcall __glPacked12Done(__GLcontext *gc, GLubyte *tmp, const __GLpixelPackMode *saved)
{
    gc->state.pixel.unpackModes = *saved;
    free(tmp);
}

/* an outgoing image: a buffer for w x h tightly packed GL_UNSIGNED_BYTE groups, the pack modes set to their defaults
** (*saved keeps the caller's).  __glPacked12Store packs it into the caller's buffer and frees it */
GLubyte *__fastcall __glPacked12Fetch(__GLcontext *gc, GLsizei w, GLsizei h, GLenum type, __GLpixelPackMode *saved)
{
    GLubyte *tmp = (GLubyte *)malloc((size_t)w * h * __glPacked12Type(type, NULL));

    if (tmp == NULL) {
        __glSetError(GL_OUT_OF_MEMORY);
        return NULL;
    }
    *saved = gc->state.pixel.packModes;
    gc->state.pixel.packModes = s_packed12Tight;
    return tmp;
}

void __fastcall __glPacked12Store(__GLcontext *gc, GLsizei w, GLsizei h, GLenum type, GLubyte *tmp, GLvoid *pixels,
                                  const __GLpixelPackMode *saved)
{
    GLint esize, n = __glPacked12Type(type, &esize), stride, x, y;
    const GLubyte *i = tmp;
    GLubyte *row, *e;
    GLuint v;

    gc->state.pixel.packModes = *saved;
    row = packed12Rows(saved, w, esize, pixels, &stride);
    for (y = 0; y < h; y++, row += stride) {
        for (x = 0, e = row; x < w; x++, e += esize, i += n) {
            v = packed12Join(type, i);
            if (esize == 1) {
                e[0] = (GLubyte)v;
            } else if (esize == 2) {
                if (saved->swapEndian) { e[0] = (GLubyte)(v >> 8); e[1] = (GLubyte)v; }
                else                   { e[1] = (GLubyte)(v >> 8); e[0] = (GLubyte)v; }
            } else if (saved->swapEndian) {
                e[0] = (GLubyte)(v >> 24); e[1] = (GLubyte)(v >> 16); e[2] = (GLubyte)(v >> 8); e[3] = (GLubyte)v;
            } else {
                e[3] = (GLubyte)(v >> 24); e[2] = (GLubyte)(v >> 16); e[1] = (GLubyte)(v >> 8); e[0] = (GLubyte)v;
            }
        }
    }
    free(tmp);
}

/* 690c04a0 -- __glim_ReadBuffer */
void __stdcall __glim_ReadBuffer(GLenum mode)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    switch (mode) {
      case GL_FRONT:
      case GL_LEFT:
      case GL_FRONT_LEFT:
        gc->state.pixel.readBuffer = GL_FRONT;
        break;
      case GL_BACK:
      case GL_BACK_LEFT:
        if (!gc->modes.doubleBufferMode) {
            goto invalid_buffer;
        }
        gc->state.pixel.readBuffer = GL_BACK;
        break;
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
aux_4_through_246:
        {
            GLint i = mode - GL_AUX0;
            if (i >= gc->modes.numAuxBuffers) {
                goto invalid_buffer;
            }
            gc->state.pixel.readBuffer = mode;
        }
        break;
      case GL_FRONT_RIGHT:
      case GL_BACK_RIGHT:
      case GL_RIGHT:
invalid_buffer:
        __glSetError(GL_INVALID_OPERATION);
        return;
      default:
        if (mode > GL_AUX3 && mode < GL_AUX0+247) {
            goto aux_4_through_246;
        }
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->state.pixel.readBufferReturn = mode;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_GENERIC | __GL_DIRTY_PIXEL);
}

/* 690c0850 -- __glim_DrawPixels */
static void __stdcall __glim_DrawPixels_base(GLsizei width, GLsizei height, GLenum format,
                       GLenum type, const GLvoid *pixels)
{
    GLuint beginMode;
    __GL_SETUP();

    beginMode = __gl_beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
        if (beginMode == __GL_NEED_VALIDATE) {
            (*gc->procs.validate)(gc);
            __gl_beginMode = __GL_NOT_IN_BEGIN;
            glDrawPixels(width,height,format,type,pixels);
            return;
        } else {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
    }

    if (!__glCheckDrawPixelArgs(gc, width, height, format, type)) return;
    if (!gc->state.current.validRasterPos) {
        return;
    }

    if (gc->renderMode == GL_FEEDBACK) {
        __glFeedbackDrawPixels(gc, &gc->state.current.rasterPos);
        return;
    }

    if (gc->renderMode != GL_RENDER) return;

    (*gc->procs.revalidate)(gc);
    (*gc->procs.waitIdle)(gc);    /* ICD: the pixel-op sync (690c091d) */
    (*gc->procs.drawPixels)(gc, width, height, format, type, pixels, GL_FALSE);
}

/* 690c0950 -- __glim_ReadPixels */
static void __stdcall __glim_ReadPixels_base(GLint x, GLint y, GLsizei width, GLsizei height, 
                       GLenum format, GLenum type, GLvoid *buf)
{
    GLuint beginMode;
    __GL_SETUP();

    beginMode = __gl_beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
        if (beginMode == __GL_NEED_VALIDATE) {
            (*gc->procs.validate)(gc);
            __gl_beginMode = __GL_NOT_IN_BEGIN;
            glReadPixels(x,y,width,height,format,type,buf);
            return;
        } else {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
    }

    if (!__glCheckReadPixelArgs(gc, width, height, format, type)) return;

    (*gc->procs.revalidate)(gc);
    (*gc->procs.waitIdle)(gc);    /* ICD: the pixel-op sync */
    (*gc->procs.readPixels)(gc, x, y, width, height, format, type, buf);
}

/* 690bf5b0 -- __glInitDefaultPixelMap */
void __fastcall __glInitDefaultPixelMap(__GLcontext *gc, GLenum map)
{
    __GLpixelState *ps = &gc->state.pixel;
    __GLpixelMapHead *pMap = ((__GLpixelMapHead *)gc->state.pixel.map);
    GLint index = map - GL_PIXEL_MAP_I_TO_I;
    
    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
        /*
        ** Allocate single-entry map for index type.
        */
        if (!(pMap[index].base.mapI = (GLint*)
              ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, sizeof(GLint)))) {
            return;
        } else {
            pMap[index].base.mapI[0] = 0;
            pMap[index].size = 1;
        }
        break;
      case GL_PIXEL_MAP_I_TO_R: case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B: case GL_PIXEL_MAP_I_TO_A:
      case GL_PIXEL_MAP_R_TO_R: case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B: case GL_PIXEL_MAP_A_TO_A:
        /*
        ** Allocate single-entry map for component type.
        */
        if (!(pMap[index].base.mapF = (__GLfloat*)
              ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, sizeof(__GLfloat)))) {
            return;
        } else {
            pMap[index].base.mapF[0] = __GL_ZERO_F;
            pMap[index].size = 1;
        }
        break;
    default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690bf640 -- __glPixelSetColorScales */
void __fastcall __glPixelSetColorScales(__GLcontext *gc)
{
    __GLpixelMachineICD *pm = &gc->pixel;
    GLfloat redScale, greenScale, blueScale, alphaScale;
    int i;
    GLint mask;

    if (pm->redMap == NULL) {
        /* First time allocation of these maps */

        /* 
        ** These lookup tables are for type UNSIGNED_BYTE, so they are sized
        ** to 256 entries.  They map from UNSIGNED_BYTE to internal scaled 
        ** floating point colors.
        */
        pm->redMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->greenMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->blueMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->alphaMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->iMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
    }

    redScale = gc->frontBuffer.redScale / 255;
    greenScale = gc->frontBuffer.greenScale / 255;
    blueScale = gc->frontBuffer.blueScale / 255;
    alphaScale = gc->frontBuffer.alphaScale / 255;
    mask = gc->frontBuffer.redMax;
    for (i=0; i<256; i++) {
        pm->redMap[i] = i * redScale;
        pm->greenMap[i] = i * greenScale;
        pm->blueMap[i] = i * blueScale;
        pm->alphaMap[i] = i * alphaScale;
        pm->iMap[i] = (GLfloat) (i & mask);
    }

    /*
    ** Invalidate the RGBA modify tables so that they will be
    ** recomputed using the current color buffer scales.
    */
    pm->rgbaCurrent = GL_FALSE;
}

/* 690bfee0 -- __glim_PixelMapfv */
void __stdcall __glim_PixelMapfv(GLenum map, GLint mapSize,
                       const GLfloat values[])
{
    __GLpixelState *ps;
    __GLpixelMapHead *pMap;
    GLint index = map - GL_PIXEL_MAP_I_TO_I;
    GLfloat value;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    ps = &gc->state.pixel;
    pMap = ((__GLpixelMapHead *)gc->state.pixel.map);

    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapI) {
            ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, pMap[index].base.mapI);
            pMap[index].base.mapI = 0;
        }
        pMap[index].base.mapI = (GLint*)
            ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, (size_t) (mapSize * sizeof(GLint)));
        if (!pMap[index].base.mapI) {
            pMap[index].size = 0;
            return;
        }
        pMap[index].size = mapSize;
        while (--mapSize >= 0) {
            value = values[mapSize];
            if (value > 0) {                /* round! */
                pMap[index].base.mapI[mapSize] =
                    (GLint)(value + __glHalf);
            } else {
                pMap[index].base.mapI[mapSize] =
                    (GLint)(value - __glHalf);
            }
        }
        break;
      case GL_PIXEL_MAP_I_TO_R:
      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:
      case GL_PIXEL_MAP_I_TO_A:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
      case GL_PIXEL_MAP_R_TO_R:
      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:
      case GL_PIXEL_MAP_A_TO_A:
        if (mapSize < 0) {
            /*
            ** Maps indexed by color component must not have negative size.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapF) {
            ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, pMap[index].base.mapF);
            pMap[index].base.mapF = 0;
        }
        if (mapSize == 0) {
            __glInitDefaultPixelMap(gc, map);
        } else {
            pMap[index].base.mapF = (__GLfloat*)
                ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc,
                                      (size_t) (mapSize * sizeof(__GLfloat)));
            if (!pMap[index].base.mapF) {
                pMap[index].size = 0;
                return;
            }
            pMap[index].size = mapSize;
            while (--mapSize >= 0) {
                value = values[mapSize];
                if (value < __GL_ZERO_F) value = __GL_ZERO_F;
                else if (value > __GL_ONE_F) value = __GL_ONE_F;
                pMap[index].base.mapF[mapSize] = value;
            }
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690c00f0 -- __glim_PixelMapuiv */
void __stdcall __glim_PixelMapuiv(GLenum map, GLint mapSize,
                        const GLuint values[])
{
    __GLpixelState *ps;
    __GLpixelMapHead *pMap;
    GLint index = map - GL_PIXEL_MAP_I_TO_I;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    ps = &gc->state.pixel;
    pMap = ((__GLpixelMapHead *)gc->state.pixel.map);

    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapI) {
            ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, pMap[index].base.mapI);
            pMap[index].base.mapI = 0;
        }
        pMap[index].base.mapI = (GLint*)
            ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, (size_t) (mapSize * sizeof(GLint)));
        if (!pMap[index].base.mapI) {
            pMap[index].size = 0;
            return;
        }
        pMap[index].size = mapSize;
        while (--mapSize >= 0) {
            pMap[index].base.mapI[mapSize] = values[mapSize];
        }
        break;
      case GL_PIXEL_MAP_I_TO_R:
      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:
      case GL_PIXEL_MAP_I_TO_A:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
      case GL_PIXEL_MAP_R_TO_R:
      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:
      case GL_PIXEL_MAP_A_TO_A:
        if (mapSize < 0) {
            /*
            ** Maps indexed by color component must not have negative size.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapF) {
            ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, pMap[index].base.mapF);
            pMap[index].base.mapF = 0;
        }
        if (mapSize == 0) {
            __glInitDefaultPixelMap(gc, map);
        } else {
            pMap[index].base.mapF = (__GLfloat*)
                ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, (size_t) (mapSize * sizeof(GLfloat)));
            if (!pMap[index].base.mapF) {
                pMap[index].size = 0;
                return;
            }
            pMap[index].size = mapSize;
            while (--mapSize >= 0) {
                pMap[index].base.mapF[mapSize] = 
                        __GL_UI_TO_FLOAT(values[mapSize]);
            }
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690c02d0 -- __glim_PixelMapusv */
void __stdcall __glim_PixelMapusv(GLenum map, GLint mapSize,
                        const GLushort values[])
{
    __GLpixelState *ps;
    __GLpixelMapHead *pMap;
    GLint index = map - GL_PIXEL_MAP_I_TO_I;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    ps = &gc->state.pixel;
    pMap = ((__GLpixelMapHead *)gc->state.pixel.map);

    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapI) {
            ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, pMap[index].base.mapI);
            pMap[index].base.mapI = 0;
        }
        pMap[index].base.mapI = (GLint*)
            ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, (size_t) (mapSize * sizeof(GLint)));
        if (!pMap[index].base.mapI) {
            pMap[index].size = 0;
            return;
        }
        pMap[index].size = mapSize;
        while (--mapSize >= 0) {
            pMap[index].base.mapI[mapSize] = values[mapSize];
        }
        break;
      case GL_PIXEL_MAP_I_TO_R:
      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:
      case GL_PIXEL_MAP_I_TO_A:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
      case GL_PIXEL_MAP_R_TO_R:
      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:
      case GL_PIXEL_MAP_A_TO_A:
        if (mapSize < 0) {
            /*
            ** Maps indexed by color component must not have negative size.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapF) {
            ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, pMap[index].base.mapF);
            pMap[index].base.mapF = 0;
        }
        if (mapSize == 0) {
            __glInitDefaultPixelMap(gc, map);
        } else {
            pMap[index].base.mapF = (__GLfloat*)
                ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, (size_t) (mapSize * sizeof(GLfloat)));
            if (!pMap[index].base.mapF) {
                pMap[index].size = 0;
                return;
            }
            pMap[index].size = mapSize;
            while (--mapSize >= 0) {
                pMap[index].base.mapF[mapSize] = 
                        __GL_US_TO_FLOAT(values[mapSize]);
            }
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* glDrawPixels / glReadPixels: the 1.2 packed types are converted here (__glPacked12*), the rest goes on as it was */
void __stdcall __glim_DrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
    __GLcontext *gc = __glCurrentGC();
    __GLpixelPackMode saved;
    GLubyte *tmp;

    if (!__glPacked12Type(type, NULL)) {
        __glim_DrawPixels_base(width, height, format, type, pixels);
        return;
    }
    if (!__glPacked12FormatOK(format, type)) return;
    if (pixels == NULL || width <= 0 || height <= 0) {
        __glim_DrawPixels_base(width, height, format, GL_UNSIGNED_BYTE, pixels);
        return;
    }
    tmp = __glPacked12Unpack(gc, width, height, type, pixels, &saved);
    if (tmp == NULL) return;
    __glim_DrawPixels_base(width, height, format, GL_UNSIGNED_BYTE, tmp);
    __glPacked12Done(gc, tmp, &saved);
}

void __stdcall __glim_ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                                 GLvoid *buf)
{
    __GLcontext *gc = __glCurrentGC();
    __GLpixelPackMode saved;
    GLubyte *tmp;

    if (!__glPacked12Type(type, NULL)) {
        __glim_ReadPixels_base(x, y, width, height, format, type, buf);
        return;
    }
    if (!__glPacked12FormatOK(format, type)) return;
    if (buf == NULL || width <= 0 || height <= 0) {
        __glim_ReadPixels_base(x, y, width, height, format, GL_UNSIGNED_BYTE, buf);
        return;
    }
    tmp = __glPacked12Fetch(gc, width, height, type, &saved);
    if (tmp == NULL) return;
    memset(tmp, 0, (size_t)width * height * __glPacked12Type(type, NULL));
    __glim_ReadPixels_base(x, y, width, height, format, GL_UNSIGNED_BYTE, tmp);
    __glPacked12Store(gc, width, height, type, tmp, buf, &saved);
}

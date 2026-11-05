/*
** sst_ctable.c -- colour tables of the hardware path (EXT_paletted_texture): the argument checks, the table
** store and the Glide palette.
*/
#include <string.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "siprotos.h"
#include "icdglob.h"
#include "icdprotos.h"
#include "glshade.h"
#include "si_names.h"
#include <math.h>
#include <stdlib.h>
#include "og.h"

#pragma warning(disable: 4273)
/* ---- paletted textures (EXT_paletted_texture / EXT_shared_texture_palette): a texture's t.CT, or the
** shared hwcx.sharedPalette (glctx.h __GLcolorTableICD) ---------------------------------------------- */
/* ---- glMultiTexCoord, glColorSubTableEXT ---- */
extern void __fastcall AllocColorTable(__GLcolorTableICD *ct, GLenum target, GLenum internalFormat, GLsizei width);
extern void __fastcall hwColorSubTable(__GLcontext *gc, GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data, GLubyte packed);
extern void __fastcall hwColorTableToPalette(__GLcolorTableICD *ct, void *palette);
extern GLenum __fastcall __glSSTCheckColorTableArgs(__GLcontext *gc, GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type);

/* 69067220 (size 383) -- __glSSTCheckColorTableArgs: __glCheckColorTableArgs (texture targets + the shared
** palette; 32 KB table limit) */
GLenum __fastcall __glSSTCheckColorTableArgs(__GLcontext *gc, GLenum target, GLenum internalFormat, GLsizei width,
                               GLenum format, GLenum type)
{
    GLenum baseFormat;

    switch (target) {
      case GL_TEXTURE_1D:
      case GL_TEXTURE_2D:
      case GL_PROXY_TEXTURE_1D:
      case GL_PROXY_TEXTURE_2D:
      case 0x81fb:
        break;
      default:
        return GL_INVALID_ENUM;
    }

    switch (internalFormat) {
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
      case 0x84eb:
        baseFormat = GL_LUMINANCE_ALPHA;
        break;
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
      case 0x84ea:
        baseFormat = GL_LUMINANCE;
        break;
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
      case 0x84e9:
        baseFormat = GL_ALPHA;
        break;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
      case 0x84ec:
        baseFormat = GL_INTENSITY;
        break;
      default:
        return GL_INVALID_ENUM;
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
      case 0x83a2:                          /* GL_RGBA_S3TC, GL_RGBA4_S3TC */
      case 0x83a3:
      case 0x86b1:
        baseFormat = GL_RGBA;
        break;
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
      case 0x83a0:                          /* GL_RGB_S3TC, GL_RGB4_S3TC */
      case 0x83a1:
      case 0x86b0:
        baseFormat = GL_RGB;
        break;
    }

    /* width must be a positive power of two */
    if (width < 0 || (width & (width - 1))) {
        return GL_INVALID_VALUE;
    }
    if ((GLuint)width > 0x8000u / (GLuint)(__glElementsPerGroup(baseFormat, type) * __glBytesPerElement(type))) {
        return 0x8031;      /* GL_TABLE_TOO_LARGE */
    }

    switch (format) {
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_ABGR_EXT:
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
        break;
      default:
        return GL_INVALID_ENUM;
    }

    switch (type) {
      case GL_UNSIGNED_BYTE:
      case GL_BYTE:
      case GL_UNSIGNED_SHORT:
      case GL_SHORT:
      case GL_UNSIGNED_INT:
      case GL_INT:
      case GL_FLOAT:
        break;
      default:
        return GL_INVALID_ENUM;
    }
    return 0;
}

/* 69067400 (size 427): AllocColorTable's format part -- the table is always 8-bit components */
void __fastcall AllocColorTable(__GLcolorTableICD *ct, GLenum target, GLenum internalFormat, GLsizei width)
{
    ct->target = target;
    ct->internalFormat = internalFormat;
    ct->width = width;
    switch (internalFormat) {
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
      case 0x84eb:
        ct->redSize = 0;
        ct->greenSize = 0;
        ct->blueSize = 0;
        ct->alphaSize = 8;
        ct->luminanceSize = 8;
        ct->intensitySize = 0;
        ct->baseFormat = GL_LUMINANCE_ALPHA;
        ct->components = 2;
        ct->type = GL_UNSIGNED_BYTE;
        break;
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
      case 0x84ea:
        ct->redSize = 0;
        ct->greenSize = 0;
        ct->blueSize = 0;
        ct->alphaSize = 0;
        ct->luminanceSize = 8;
        ct->intensitySize = 0;
        ct->baseFormat = GL_LUMINANCE;
        ct->components = 1;
        ct->type = GL_UNSIGNED_BYTE;
        break;
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
      case 0x84e9:
        ct->redSize = 0;
        ct->greenSize = 0;
        ct->blueSize = 0;
        ct->alphaSize = 8;
        ct->luminanceSize = 0;
        ct->intensitySize = 0;
        ct->baseFormat = GL_ALPHA;
        ct->components = 1;
        ct->type = GL_UNSIGNED_BYTE;
        break;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
      case 0x84ec:
        ct->redSize = 0;
        ct->greenSize = 0;
        ct->blueSize = 0;
        ct->alphaSize = 0;
        ct->luminanceSize = 0;
        ct->intensitySize = 8;
        ct->baseFormat = GL_INTENSITY;
        ct->components = 1;
        ct->type = GL_UNSIGNED_BYTE;
        break;
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
      case 0x83a2:                          /* GL_RGBA_S3TC, GL_RGBA4_S3TC */
      case 0x83a3:
      case 0x86b1:
        ct->redSize = 8;
        ct->greenSize = 8;
        ct->blueSize = 8;
        ct->alphaSize = 8;
        ct->luminanceSize = 0;
        ct->intensitySize = 0;
        ct->baseFormat = GL_RGBA;
        ct->components = 4;
        ct->type = GL_UNSIGNED_BYTE;
        break;
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
      case 0x83a0:                          /* GL_RGB_S3TC, GL_RGB4_S3TC */
      case 0x83a1:
      case 0x86b0:
        ct->redSize = 8;
        ct->greenSize = 8;
        ct->blueSize = 8;
        ct->alphaSize = 0;
        ct->luminanceSize = 0;
        ct->intensitySize = 0;
        ct->baseFormat = GL_RGB;
        ct->components = 3;
        ct->type = GL_UNSIGNED_BYTE;
        break;
    }
}

/* 69067600 (size 245) -- hwColorSubTable: glColorSubTableEXT, after the argument checks */
void __fastcall hwColorSubTable(__GLcontext *gc, GLenum target, GLsizei start, GLsizei count, GLenum format,
                             GLenum type, const GLvoid *data, GLubyte packed)
{
    __GLpixelSpanInfoICD spanInfo;
    __GLcolorTableICD *ct;
    __GLtextureFxICD *tex;

    switch (target) {
      case 0x81fb:      /* GL_SHARED_TEXTURE_PALETTE_EXT */
        ct = &gc->hwcx.sharedPalette;
        gc->hwcx.sharedPaletteLoaded = GL_FALSE;
        break;
      case GL_TEXTURE_1D:
      case GL_TEXTURE_2D:
        tex = (__GLtextureFxICD *)__glLookUpTexture(gc, target, gc->fx.activeTexture);
        ct = &tex->t.CT;
        tex->paletteLoaded = GL_FALSE;
        break;
    }
    if (start + count > ct->width) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    __glInitMemUnpack(gc, &spanInfo, count, 1, 0, format, type, data, packed);
    __glSSTInitColorTableStore(ct, &spanInfo);
    spanInfo.dstSkipPixels = start;
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);
    spanInfo.applyFlags[2] = GL_FALSE;          /* +0x16a: the third of __glInitUnpacker's flags (0, 1, 1, 0)
                                                    cleared */
    ((void (__fastcall *)(__GLcontext *, void *, GLint))gc->procs.copyImage)(gc, &spanInfo, 0);
}

/* 69067740 -- __glsstim_ColorSubTableEXT of the hardware path: the argument check 690671a0 (target; start / count
** not negative; format / type), the update 69067600 (unpacked data), then revalidation (0x400011) */
void __stdcall __glsstim_ColorSubTableEXT(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type,
                            const GLvoid *data)
{
    GLenum error;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    error = checkColorTableEXTArgs(gc, target, start, count, format, type);
    if (error != 0) {
        __glSetError(error);
        return;
    }
    hwColorSubTable(gc, target, start, count, format, type, data, GL_FALSE);
    __GL_DELAY_VALIDATE_MASK(gc, 0x400011);
}

/* 69067820 (size 503) -- hwColorTable: glColorTableEXT, after the argument checks */
void __fastcall hwColorTable(__GLcontext *gc, GLenum target, GLenum internalFormat, GLsizei width,
                             GLenum format, GLenum type, const GLvoid *table, GLubyte packed)
{
    __GLpixelSpanInfoICD spanInfo;
    __GLcolorTableICD *ct;
    GLuint *palette;
    GLboolean isProxy;
    __GLtextureFxICD *tex;
    GLint oldSize;

    isProxy = GL_FALSE;
    switch (target) {
      case GL_PROXY_TEXTURE_1D:
      case GL_PROXY_TEXTURE_2D:
        isProxy = GL_TRUE;
        /* FALLTHROUGH */
      case GL_TEXTURE_1D:
      case GL_TEXTURE_2D:
        tex = (__GLtextureFxICD *)__glLookUpTexture(gc, target, gc->fx.activeTexture);
        ct = &tex->t.CT;
        tex->paletteLoaded = GL_FALSE;
        if (!tex->palette) {
            tex->palette = (GLuint *)((void *(__fastcall *)(__GLcontext *, GLsizei))gc->imports[0])(gc, 0x400);
        }
        palette = tex->palette;
        gc->state.enables.general &= ~__GL_SHARED_TEXTURE_PALETTE_EXT_ENABLE;
        break;
      case 0x81fb:      /* GL_SHARED_TEXTURE_PALETTE_EXT */
        ct = &gc->hwcx.sharedPalette;
        gc->hwcx.sharedPaletteLoaded = GL_FALSE;
        palette = gc->hwcx.palette;
        gc->state.enables.general |= __GL_SHARED_TEXTURE_PALETTE_EXT_ENABLE;
        break;
    }

    if (!internalFormat) {
        ct->internalFormat = 0;
        ct->width = 0;
        ct->type = 0;
        ct->redSize = 0;
        ct->greenSize = 0;
        ct->blueSize = 0;
        ct->alphaSize = 0;
        ct->luminanceSize = 0;
        ct->intensitySize = 0;
        return;
    }
    oldSize = __glElementsPerGroup(ct->baseFormat, ct->type) * ct->width;
    AllocColorTable(ct, target, internalFormat, width);
    if (isProxy) return;
    if (oldSize < __glElementsPerGroup(ct->baseFormat, ct->type) * width) {
        ct->table = ((void *(__fastcall *)(__GLcontext *, void *, GLsizei))gc->imports[2])
            (gc, ct->table, __glElementsPerGroup(ct->baseFormat, ct->type) * __glBytesPerElement(ct->type) * width);
    }
    __glInitMemUnpack(gc, &spanInfo, width, 1, 0, format, type, table, packed);
    __glSSTInitColorTableStore(ct, &spanInfo);
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);
    spanInfo.applyFlags[2] = GL_FALSE;          /* +0x16a: the third of __glInitUnpacker's flags (0, 1, 1, 0)
                                                    cleared */
    ((void (__fastcall *)(__GLcontext *, void *, GLint))gc->procs.copyImage)(gc, &spanInfo, 0);
    hwColorTableToPalette(ct, palette);
}

/* 69067a20 (size 116) -- hwColorTableToPalette: colour table (RGB / RGBA bytes) -> Glide palette entries (B, G, R,
** A bytes) */
void __fastcall hwColorTableToPalette(__GLcolorTableICD *ct, void *palette)
{
    GLubyte *src = (GLubyte *)ct->table;
    GLubyte *dst = (GLubyte *)palette;
    GLint i;

    if (ct->baseFormat == GL_RGB) {
        for (i = 0; i < ct->width; i++) {
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            src += 3;
            dst += 4;
        }
    } else {
        for (i = 0; i < ct->width; i++) {
            dst[0] = src[2];
            dst[1] = src[1];
            dst[2] = src[0];
            dst[3] = src[3];
            src += 4;
            dst += 4;
        }
    }
}

#pragma optimize("y", off)      /* the original keeps the ebp frame here (no frame-pointer omission) */
/* 69067aa0 (size 240) -- __glsstim_ColorTable: glColorTableEXT (the s_ctable.c __glim_ColorTable shape; frame
** pointer kept) */
void __stdcall __glsstim_ColorTable(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
    GLenum rv;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    /* check the arguments; compute internalformat numbers */
    if ((rv = __glSSTCheckColorTableArgs(gc, target, internalFormat, width, format, type))) {
        switch (target) {
        case GL_PROXY_TEXTURE_1D:
        case GL_PROXY_TEXTURE_2D:
            if (rv == 0x8031) {             /* GL_TABLE_TOO_LARGE */
                internalFormat = (GLenum)0;     /* proceed with format == 0 */
            } else {
                __glSetError(rv);
                return;
            }
            break;
        default:                        /* normal error behavior */
            __glSetError(rv);
            return;
        }
    }
    hwColorTable(gc, target, internalFormat, width, format, type, table, GL_FALSE);
    __GL_DELAY_VALIDATE_MASK(gc, 0x400011);
}
#pragma optimize("", on)

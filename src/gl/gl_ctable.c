/*
** gl_ctable.c -- s_ctable.c: colour tables (EXT_paletted_texture / SGI_color_table).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "siprotos.h"
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "icdglob.h"
#include "icdprotos.h"
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <string.h>

/* ---- EXT_paletted_texture: glColorTableEXT argument check (returns the GL error, 0 if valid) --- */
#ifndef GL_TABLE_TOO_LARGE
#define GL_TABLE_TOO_LARGE              0x8031
#endif
typedef __GLpixelSpanInfoICD __GLpixelSpanInfo;
#define __glLookupActiveTexture(gc, target)  __glLookUpTexture(gc, target, (gc)->fx.activeTexture)
typedef void *(__fastcall *__GLreallocProc)(__GLcontext *gc, void *p, size_t n);
typedef void (__fastcall *__GLcopyImageProc)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, GLboolean applyPixelTransfer);
/************************************************************************/
/* s_ctable.c: the ICD's colour sub-table back end */
typedef void (__fastcall *__GLcopyImageProcICD)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLboolean applyPixelTransfer);
/* ---- s_ctable.c: glColorSubTableEXT (the argument check and the table store are procs) ---------- */
typedef void (__fastcall *__GLcolorSubTableProc)(__GLcontext *gc, GLenum target, GLsizei start, GLsizei count,
                                                     GLenum format, GLenum type, const void *table, GLboolean dlist);
/* ---- s_ctable.c: the address of a texture color table parameter ------------------------------ */
#define __GL_TEX_COLOR_TABLE(tex)   (&((__GLtextureICD *)(tex))->CT)
typedef void (__fastcall *__GLcolorTableProc)(__GLcontext *gc, GLenum target, GLenum internalFormat,
					   GLsizei width, GLenum format, GLenum type,
					   const GLvoid *table, GLint swap);
extern void __fastcall __glLoadColorTableParams(__GLcolorTableICD *ct, GLenum target, GLenum internalFormat, GLsizei width);

/* 69082a30 -- __glCheckColorTableArgs (s_ctable.c): check glColorTableEXT(target, internalFormat, width,
** format, type) */
GLenum __fastcall __glCheckColorTableArgs(__GLcontext *gc_unused, GLenum target, GLenum internalFormat,
                               GLsizei width, GLenum format, GLenum type)
{
    GLenum baseFormat;

    switch (target) {
      case GL_TEXTURE_1D:
      case GL_TEXTURE_2D:
      case GL_PROXY_TEXTURE_1D:
      case GL_PROXY_TEXTURE_2D:
        break;
      default:
        return GL_INVALID_ENUM;
    }

    switch (internalFormat) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
        baseFormat = GL_ALPHA;
        break;
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
        baseFormat = GL_LUMINANCE;
        break;
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
        baseFormat = GL_LUMINANCE_ALPHA;
        break;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
        baseFormat = GL_INTENSITY;
        break;
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
        baseFormat = GL_RGB;
        break;
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
        baseFormat = GL_RGBA;
        break;
      default:
        return GL_INVALID_ENUM;
    }

    if ((width < 0) || (width & (width - 1))) {
        return GL_INVALID_VALUE;
    }
    if ((GLuint)width > (GLuint)__GL_MAX_SPAN_SIZE /
            (GLuint)(__glElementsPerGroup(baseFormat, type) * __glBytesPerElement(type))) {
        return GL_TABLE_TOO_LARGE;
    }

    switch (format) {
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_ABGR_EXT:
      case GL_BGR:
      case GL_BGRA:
        break;
      default:
        return GL_INVALID_ENUM;
    }

    switch (type) {
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
        break;
      default:
        return GL_INVALID_ENUM;
    }
    return GL_NO_ERROR;
}

/* 69082c20 -- __glLoadColorTableParams (s_ctable.c): set up a color table's format: component sizes and base
** format from the internal
** format (always GL_UNSIGNED_BYTE entries).  GL_INTENSITY16 shares the intensity case and GL_RGB2_EXT goes to the
** default, as in the original (fixed 2026-10-30: the two labels were swapped). */
void __fastcall __glLoadColorTableParams(__GLcolorTableICD *ct, GLenum target, GLenum internalFormat, GLsizei width)
{
    GLuint *ctw = (GLuint *)ct;     /* +0x00 target, +0x08 internalFormat, +0x0c type, +0x10 width,
				       +0x2c baseFormat, +0x30 components */
    ctw[0] = target;
    ctw[2] = internalFormat;
    ctw[4] = width;
    switch (internalFormat) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
	ct->alphaSize = 8;
	ct->redSize = 0;
	ct->greenSize = 0;
	ct->blueSize = 0;
	ct->luminanceSize = 0;
	ct->intensitySize = 0;
	ctw[11] = GL_ALPHA;
	ctw[12] = 1;
	ctw[3] = GL_UNSIGNED_BYTE;
	break;
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
	ct->luminanceSize = 8;
	ct->redSize = 0;
	ct->greenSize = 0;
	ct->blueSize = 0;
	ct->alphaSize = 0;
	ct->intensitySize = 0;
	ctw[11] = GL_LUMINANCE;
	ctw[12] = 1;
	ctw[3] = GL_UNSIGNED_BYTE;
	break;
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
	ct->redSize = 0;
	ct->greenSize = 0;
	ct->blueSize = 0;
	ct->alphaSize = 8;
	ct->luminanceSize = 8;
	ct->intensitySize = 0;
	ctw[11] = GL_LUMINANCE_ALPHA;
	ctw[12] = 2;
	ctw[3] = GL_UNSIGNED_BYTE;
	break;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
	ct->intensitySize = 8;
	ct->redSize = 0;
	ct->greenSize = 0;
	ct->blueSize = 0;
	ct->alphaSize = 0;
	ct->luminanceSize = 0;
	ctw[11] = GL_INTENSITY;
	ctw[12] = 1;
	ctw[3] = GL_UNSIGNED_BYTE;
	break;
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
	ct->redSize = 8;
	ct->greenSize = 8;
	ct->blueSize = 8;
	ct->alphaSize = 0;
	ct->luminanceSize = 0;
	ct->intensitySize = 0;
	ctw[11] = GL_RGB;
	ctw[12] = 3;
	ctw[3] = GL_UNSIGNED_BYTE;
	break;
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
	ct->redSize = 8;
	ct->greenSize = 8;
	ct->blueSize = 8;
	ct->alphaSize = 8;
	ct->luminanceSize = 0;
	ct->intensitySize = 0;
	ctw[11] = GL_RGBA;
	ctw[12] = 4;
	ctw[3] = GL_UNSIGNED_BYTE;
	break;
    }
}

/* 69082df0 -- __glColorTableEXT (s_ctable.c): procs.colorTable, glColorTableEXT on a texture palette */
void __fastcall __glColorTableEXT(__GLcontext *gc, GLenum target, GLenum internalFormat, GLsizei width,
                             GLenum format, GLenum type, const GLvoid *table, GLboolean dlist)
{
    __GLcolorTableICD *ct;
    GLboolean isProxy;
    GLint oldSize;
    __GLpixelSpanInfo spanInfo;

    isProxy = GL_FALSE;
    switch (target) {
    case GL_PROXY_TEXTURE_1D:
    case GL_PROXY_TEXTURE_2D:
        isProxy = GL_TRUE;
        /* fall through */
    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
        ct = &__glLookupActiveTexture(gc, target)->CT;
        break;
    }

    if (internalFormat == 0) {
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
    __glLoadColorTableParams(ct, target, internalFormat, width);
    if (isProxy) {
        return;
    }
    if (oldSize < __glElementsPerGroup(ct->baseFormat, ct->type) * width) {
        ct->table = (*(__GLreallocProc)gc->imports[2])(gc, ct->table,
            __glElementsPerGroup(ct->baseFormat, ct->type) * __glBytesPerElement(ct->type) * width);
    }
    __glInitMemUnpack(gc, &spanInfo, width, 1, 0, format, type, table, dlist);
    __glInitColorTableStore(ct, &spanInfo);
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);
    spanInfo.applyFlags[2] = GL_FALSE;
    (*(__GLcopyImageProc)gc->procs.copyImage)(gc, &spanInfo, GL_FALSE);
}

/* 69082fb0 -- __glColorSubTableEXT (s_ctable.c): procs.colorSubTable, glColorSubTableEXT on a texture palette
** (__glim_ColorSubTable
** without the argument checks); ct is left unset for a non-texture target (original) */
void __fastcall __glColorSubTableEXT(__GLcontext *gc, GLenum target, GLsizei start, GLsizei count,
			     GLenum format, GLenum type, const GLvoid *table, GLboolean dlist)
{
    __GLcolorTableICD *ct;
    __GLpixelSpanInfoICD spanInfo;

    if (target >= GL_TEXTURE_1D && target <= GL_TEXTURE_2D) {
	ct = &((__GLtextureICD *)__glLookUpTexture(gc, target, gc->fx.activeTexture))->CT;
    }
    if (start + count > ct->width) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    __glInitMemUnpack(gc, &spanInfo, count, 1, 0, format, type, table, dlist);
    __glInitColorTableStore(ct, &spanInfo);
    spanInfo.dstSkipPixels = start;

    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);

    spanInfo.applyFlags[2] = GL_FALSE;
    (*(__GLcopyImageProcICD)gc->procs.copyImage)(gc, &spanInfo, GL_FALSE);
}

/* 69083090 -- __glim_ColorSubTable */
void __stdcall __glim_ColorSubTable(GLenum target, GLsizei start, GLsizei count,
                             GLenum format, GLenum type, const void *table)
{
    GLenum rv;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    if ((rv = __glCheckColorSubTableArgs(gc, target, start, count, format, type))) {
	__glSetError(rv);
	return;
    }
    (*(__GLcolorSubTableProc)gc->procs.colorSubTable)(gc, target, start, count, format, type, table, GL_FALSE);
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_PIXEL | __GL_DIRTY_GENERIC);
}

#pragma optimize("y", off)
/* 69083260 -- __glim_GetColorTable (s_ctable.c): reduced to the argument checks in the ICD, every
** valid texture target ends in GL_INVALID_ENUM, any other target is ignored.  Frame pointer
** kept; the span info is never used. */
void __stdcall __glim_GetColorTable(GLenum target, GLenum format,
			     GLenum type, void *table)
{
    __GLpixelSpanInfoICD spanInfo;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_GET();

    if (__glCheckColorTableArgs(gc, target, GL_RGB, 0, format, type)) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    switch (target) {
      case GL_PROXY_TEXTURE_1D:
      case GL_PROXY_TEXTURE_2D:
	__glSetError(GL_INVALID_ENUM);
	return;
      case GL_TEXTURE_1D:
      case GL_TEXTURE_2D:
	__glLookUpTexture(gc, target, gc->fx.activeTexture);
	__glSetError(GL_INVALID_ENUM);
	return;
      default:
	return;
    }
    /* not reached: the SI's read-back, left in the source */
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);
}
#pragma optimize("", on)

/* 69083330 -- __glColorTableModifiers (s_ctable.c): &parameter of the texture color table of target
** (glGetColorTableParameter*EXT) */
GLint *__fastcall __glColorTableModifiers(__GLcontext *gc, GLenum target, GLenum pname)
{
    __GLcolorTableICD *ct;

    switch (target) {
      case GL_TEXTURE_1D:
      case GL_TEXTURE_2D:
      case GL_PROXY_TEXTURE_1D:
      case GL_PROXY_TEXTURE_2D:
	ct = __GL_TEX_COLOR_TABLE(__glLookUpTexture(gc, target, gc->fx.activeTexture));
	switch (pname) {
	  case GL_COLOR_TABLE_FORMAT_EXT:
	    return &ct->internalFormat;
	  case GL_COLOR_TABLE_WIDTH_EXT:
	    return &ct->width;
	  case GL_COLOR_TABLE_RED_SIZE_EXT:
	    return &ct->redSize;
	  case GL_COLOR_TABLE_GREEN_SIZE_EXT:
	    return &ct->greenSize;
	  case GL_COLOR_TABLE_BLUE_SIZE_EXT:
	    return &ct->blueSize;
	  case GL_COLOR_TABLE_ALPHA_SIZE_EXT:
	    return &ct->alphaSize;
	  case GL_COLOR_TABLE_LUMINANCE_SIZE_EXT:
	    return &ct->luminanceSize;
	  case GL_COLOR_TABLE_INTENSITY_SIZE_EXT:
	    return &ct->intensitySize;
	}
	break;
    }
    __glSetError(GL_INVALID_ENUM);
    return 0;
}
/* 69083470 -- __glim_CopyColorTableEXT (3dfx GLIDE3X/GLCORE/S_CTABLE.C; the ICD hands the texture
** to procs.readImage, and ct stays unset for a non-texture target as there) */
void __stdcall __glim_CopyColorTableEXT(GLenum target,
                             GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    __GLcolorTableICD *ct;
    GLenum rvalue;
    __GLpixelSpanInfoICD spanInfo;
    __GLtextureICD *tex = NULL;
    __GL_SETUP();

    ((void (__fastcall *)(__GLcontext *))gc->procs.revalidate)(gc);
    (*gc->procs.waitIdle)(gc);

    /* reuse function by filling in missing args with legal values */
    if ((rvalue = __glCheckColorTableArgs(gc, target, internalformat, width,
                               GL_RGBA, GL_FLOAT))) {
	__glSetError(rvalue);
    }
    switch (target) { /* proxy tables aren't valid targets for copycolortable */
    case GL_PROXY_TEXTURE_1D:
    case GL_PROXY_TEXTURE_2D:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    /* load the table */
    switch (target) {
    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
	{
	    tex = (__GLtextureICD *)__glLookUpTexture(gc, target, gc->fx.activeTexture);
	    ct = &tex->CT;
	}
	break;
    }

    {
	int oldWidth = ct->width * __glElementsPerGroup(ct->baseFormat, ct->type);

	__glLoadColorTableParams(ct, target, internalformat, width);

	/* malloc table if it needs it */
	if (oldWidth < width * __glElementsPerGroup(ct->baseFormat, ct->type)) {
	    ct->table = ((void *(__fastcall *)(__GLcontext *, void *, size_t))gc->imports[2])(gc, ct->table, width *
			 __glElementsPerGroup(ct->baseFormat, ct->type) *
			 __glBytesPerElement(ct->type));
	}
    }

    __glInitReadImageSrcInfo(gc, &spanInfo, x, y, width, 1);
    __glInitColorTableStore(ct, &spanInfo);

    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);

    if (!__glClipReadPixels(gc, &spanInfo)) return;
    (*gc->procs.readImage)(gc, tex, &spanInfo);

    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_PIXEL);
}

#pragma optimize("y", off)
/* 69083170 -- __glim_ColorTable (s_ctable.c): the ICD hands the table to procs.colorTable; a proxy
** texture target that is too large is passed on with internal format 0.  Frame pointer kept.
** The argument check is 3dfx's __glim_ColorTableEXT (GLIDE3X/GLCORE/S_CTABLE.C) switch. */
void __stdcall __glim_ColorTable(GLenum target,
			     GLenum internalFormat, GLsizei width, GLenum format, GLenum type,
			     const GLvoid *table)
{
    GLenum rv;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    /* check the arguments; compute internalformat numbers */
    if ((rv = __glCheckColorTableArgs(gc, target, internalFormat, width, format, type))) {
	switch (target) {
	case GL_PROXY_TEXTURE_1D:
	case GL_PROXY_TEXTURE_2D:
	    if (rv == GL_TABLE_TOO_LARGE) {
		internalFormat = (GLenum)0;	/* proceed with format == 0 */
	    } else {
		__glSetError(rv);
		return;
	    }
	    break;
	default:			/* normal error behavior */
	    __glSetError(rv);
	    return;
	}
    }
    (*(__GLcolorTableProc)gc->procs.colorTable)(gc, target, internalFormat, width, format, type,
					     table, 0);
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_PIXEL | __GL_DIRTY_GENERIC);
}
#pragma optimize("", on)

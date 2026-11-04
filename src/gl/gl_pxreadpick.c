/*
** gl_pxreadpick.c -- original VA range 69088000..6908c000: px_paths.c isPackedType, the ReadPixels /
** CopyPixels pickers and __glInitMemUnpack / __glInitMemPack (SGI Sample Implementation glcore,
** MS ICD kit flavour).
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "siprotos.h"
#include "si_names.h"
#include "icdprotos.h"

/* buffers.h: lock masks of the pixel paths */

/* EXT_packed_pixels types (the kit's GL.H predates them) */
#define GL_UNSIGNED_BYTE_3_3_2_EXT      0x8032
#define GL_UNSIGNED_INT_10_10_10_2_EXT  0x8036
/* the ICD's internal packed texel types are 0 .. 5 (see gl_pxpick.c) */
#define __GL_LAST_INTERNAL_PACKED_TYPE  5

/* 69088700 -- isPackedType (px_paths.c): the EXT_packed_pixels types and the ICD's internal packed
** types 0 .. 5.
*/
GLboolean __fastcall isPackedType(GLenum type)
{
    if (type <= GL_UNSIGNED_INT_10_10_10_2_EXT &&
        (type >= GL_UNSIGNED_BYTE_3_3_2_EXT || type <= __GL_LAST_INTERNAL_PACKED_TYPE)) {
        return GL_TRUE;
    }
    return GL_FALSE;
}

/* 69088c80 -- __glSlowPickReadPixels (px_paths.c), with __GL_(UN)LOCK_PXL_READ_BUFFER */
void __fastcall __glSlowPickReadPixels(__GLcontext *gc, GLint x, GLint y, GLsizei width, GLsizei height,
                             GLenum format, GLenum type, GLvoid *pixels)
{
    __GLpixelSpanInfoICD spanInfo;
    GLuint mask;

    __glInitReadPixelsInfo(gc, &spanInfo, x, y, width, height, format, type, pixels);
    if (!__glClipReadPixels(gc, &spanInfo)) return;

    __glInitPacker(gc, &spanInfo);

    mask = gc->buffers.readBufferMask;
    if (format == GL_DEPTH_COMPONENT) mask = __GL_DEPTH_BUFFER_MASK;
    else if (format == GL_STENCIL_INDEX) mask = __GL_STENCIL_BUFFER_MASK;
    (*gc->buffers.lockBuffers)(gc, mask);

    __glGenericPickReadPixels(gc, &spanInfo);

    mask = gc->buffers.readBufferMask;
    if (format == GL_DEPTH_COMPONENT) mask = __GL_DEPTH_BUFFER_MASK;
    else if (format == GL_STENCIL_INDEX) mask = __GL_STENCIL_BUFFER_MASK;
    (*gc->buffers.unlockBuffers)(gc, mask);
}

/* 6908a230 -- __glSlowPickCopyPixels (px_paths.c), with __GL_(UN)LOCK_PXL_COPY_BUFFERS */
void __fastcall __glSlowPickCopyPixels(__GLcontext *gc, GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    __GLpixelSpanInfoICD spanInfo;
    GLuint mask;

    __glInitCopyPixelsInfo(gc, &spanInfo, x, y, width, height, type);
    if (!__glClipCopyPixels(gc, &spanInfo)) return;

    mask = gc->buffers.renderBufferMask | gc->buffers.drawBufferMask;
    if (type == GL_DEPTH_COMPONENT) mask |= __GL_DEPTH_BUFFER_MASK;
    else if (type == GL_STENCIL_INDEX) mask = __GL_STENCIL_BUFFER_MASK;
    (*gc->buffers.lockBuffers)(gc, mask);

    __glGenericPickCopyPixels(gc, &spanInfo);

    mask = gc->buffers.renderBufferMask | gc->buffers.drawBufferMask;
    if (type == GL_DEPTH_COMPONENT) mask |= __GL_DEPTH_BUFFER_MASK;
    else if (type == GL_STENCIL_INDEX) mask = __GL_STENCIL_BUFFER_MASK;
    (*gc->buffers.unlockBuffers)(gc, mask);
}

/* 6908ab50 -- __glInitMemUnpack (px_paths.c): a host-memory source (depth: SI 1.2's field, kept).
*/
void __fastcall __glInitMemUnpack(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLsizei width, GLsizei height,
                             GLsizei depth, GLenum format, GLenum type, const GLvoid *buf, GLboolean dltex)
{
    spanInfo->x = __GL_ZERO_F;
    spanInfo->width = width;
    spanInfo->height = height;
    spanInfo->depth = depth;
    spanInfo->srcFormat = format;
    spanInfo->srcType = type;
    spanInfo->srcImage = buf;
    spanInfo->zoomx = __GL_ONE_F;

    /* the remaining source modes from the pixel state */
    __glLoadUnpackModes(gc, spanInfo, dltex);
}

/* 6908abb0 -- __glInitMemPack (px_paths.c): a host-memory destination */
void __fastcall __glInitMemPack(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLsizei width, GLsizei height,
                             GLsizei depth, GLenum format, GLenum type, GLvoid *buf)
{
    spanInfo->x = __GL_ZERO_F;
    spanInfo->width = width;
    spanInfo->height = height;
    spanInfo->depth = depth;
    spanInfo->dstFormat = format;
    spanInfo->dstType = type;
    spanInfo->zoomx = __GL_ONE_F;
    spanInfo->dstImage = buf;

    /* the remaining destination modes from the pixel state */
    __glLoadPackModes(gc, spanInfo);
}

/* 6908b260 -- __glDoNullStore (s_store.c): store nothing */
void __fastcall __glDoNullStore()
{
}

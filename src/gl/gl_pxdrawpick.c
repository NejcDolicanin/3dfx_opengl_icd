/*
** gl_pxdrawpick.c -- original VA range 69084000..69088000: px_paths.c __glLoadUnpackModes and the
** DrawPixels picker (SGI Sample Implementation glcore, MS ICD kit flavour).
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

/* buffers.h: lock masks of the pixel paths */

/* 69086310 -- __glLoadUnpackModes (px_paths.c): source unpack modes of spanInfo, from the display
** list (packed) or from glPixelStore state.  No skipImages / imageHeight in this kit.
*/
void __fastcall __glLoadUnpackModes(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLboolean packed)
{
    GLint lineLength;

    if (packed) {
        /* data came from a display list */
        spanInfo->srcSkipPixels = 0;
        spanInfo->srcSkipLines = 0;
        spanInfo->srcLsbFirst = GL_FALSE;
        spanInfo->srcSwapBytes = GL_FALSE;
        spanInfo->srcAlignment = 1;
        spanInfo->srcLineLength = spanInfo->width;
        return;
    }
    /* data came straight from the application */
    lineLength = (GLint)gc->state.pixel.unpackModes.lineLength;
    spanInfo->srcAlignment = gc->state.pixel.unpackModes.alignment;
    spanInfo->srcSkipPixels = gc->state.pixel.unpackModes.skipPixels;
    spanInfo->srcSkipLines = gc->state.pixel.unpackModes.skipLines;
    spanInfo->srcLsbFirst = gc->state.pixel.unpackModes.lsbFirst;
    spanInfo->srcSwapBytes = gc->state.pixel.unpackModes.swapEndian;
    if (lineLength <= 0) lineLength = spanInfo->width;
    spanInfo->srcLineLength = lineLength;
}

/* 690869c0 -- __glSlowPickDrawPixels (px_paths.c): fill a span info, clip, init the unpacker and run
** the generic DrawPixels picker inside __GL_LOCK_PXL_DRAW_BUFFER / __GL_UNLOCK_PXL_DRAW_BUFFER.
*/
void __fastcall __glSlowPickDrawPixels(__GLcontext *gc, GLint width, GLint height, GLenum format, GLenum type,
                             const GLvoid *pixels, GLboolean packed)
{
    __GLpixelSpanInfoICD spanInfo;
    GLuint mask;

    __glInitDrawPixelsInfo(gc, &spanInfo, width, height, format, type, pixels);
    __glLoadUnpackModes(gc, &spanInfo, packed);
    if (!__glClipDrawPixels(gc, &spanInfo)) return;

    __glInitUnpacker(gc, &spanInfo);

    mask = gc->buffers.renderBufferMask;
    if (format == GL_DEPTH_COMPONENT) mask |= __GL_DEPTH_BUFFER_MASK;
    else if (format == GL_STENCIL_INDEX) mask = __GL_STENCIL_BUFFER_MASK;
    (*gc->buffers.lockBuffers)(gc, mask);

    __glGenericPickDrawPixels(gc, &spanInfo);

    mask = gc->buffers.renderBufferMask;
    if (format == GL_DEPTH_COMPONENT) mask |= __GL_DEPTH_BUFFER_MASK;
    else if (format == GL_STENCIL_INDEX) mask = __GL_STENCIL_BUFFER_MASK;
    (*gc->buffers.unlockBuffers)(gc, mask);
}

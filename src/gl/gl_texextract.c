/*
** gl_texextract.c -- original VA range 690a0000..690a4000: glCopyTexSubImage2D, __glShareDlist, the
** index-test table free, the polygon-mode triangle renderers, the texel extract procs and the UV-scaled
** texture filters (SGI Sample Implementation glcore, MS ICD kit flavour).
** Then the rest of s_texextr.c, in address order.
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
#include "glsitypes.h"
#include "glshade.h"

typedef void (__fastcall *__GLcopyTexImageProcA00)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                                   __GLtextureICD *tex, GLint lod);
typedef void (__fastcall *__GLtexFilterProcA00)(__GLtextureICD *tex, __GLmipMapLevelICD *lp,
                                                __GLfloat s, __GLfloat t, __GLtexel *result);
typedef void (__fastcall *__GLfreeProcA00)(__GLcontext *gc, void *p);

/* dl_init.c: the shared display-list arrays (only the reference counts are touched here) */

/* 690a0a30 -- __glim_CopyTexSubImage2D (s_tex.c).  The ICD has no pixel-path width adjustment and
** tests the texture's own reference count.
*/
void __stdcall __glim_CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                            GLint x, GLint y, GLsizei width, GLsizei height)
{
    __GLtextureICD *tex;
    __GLpixelSpanInfoICD spanInfo;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_PIXEL_OP();

    (*gc->procs.revalidate)(gc);
    (*gc->procs.waitIdle)(gc);

    /* Check arguments and get the right texture level being changed */
    tex = __glCheckCopyTexSubImageArgs(gc, target, level, xoffset, yoffset, 0, x, y, width, height, 2);
    if (!tex) {
        return;
    }
    if (tex->refCount > 2) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }

    /* Copy sub-image data */
    __glInitReadImageSrcInfo(gc, &spanInfo, x, y, width, height);
    __glInitTexSubImageStore(gc, &spanInfo, tex, level, xoffset, yoffset, 0);
    __glInitUnpacker(gc, &spanInfo);
    __glInitPacker(gc, &spanInfo);

    if (!__glClipReadPixels(gc, &spanInfo)) return;
    (*(__GLcopyTexImageProcA00)tex->readTexImage)(gc, &spanInfo, tex, level);

    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);     /* sic (SI) */
}

/* 690a0e20 -- __glShareDlist (dl_init.c): drop our display lists and share src's arrays */
void __fastcall __glShareDlist(__GLcontext *dst, __GLcontext *src)
{
    /* first get rid of our private display list state */
    __glFreeDlistState(dst);

    dst->dlist.dlistArray = src->dlist.dlistArray;
    dst->dlist.dlistArray->refcount++;
    dst->dlist.namesArray = src->dlist.namesArray;
    dst->dlist.namesArray->refcount++;
}

/* 690a1020 -- __glFreeIndexTestTable: free the front buffer's colour-index test table (glIndexFuncSGI; ICD). 
** __glDestroyContext
** calls it after __glFreeEvaluatorState; the SI's __glFreePixelState is 690bf740.
*/
void __fastcall __glFreeIndexTestTable(__GLcontext *gc)
{
    if (gc->frontBuffer.indexTestFuncTable) {
        (*(__GLfreeProcA00)gc->imports[3])(gc, gc->frontBuffer.indexTestFuncTable);
    }
}

/* 690a13b0 -- __glRenderTriangleLine (s_pgmode.c): GL_LINE polygon mode, edges with the edge flag */
void __fastcall __glRenderTriangleLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    if (a->flagBits & __GL_VERTEX_EDGE_FLAG) (*gc->procs.renderLine)(gc, a, b);
    if (b->flagBits & __GL_VERTEX_EDGE_FLAG) (*gc->procs.renderLine)(gc, b, c);
    if (c->flagBits & __GL_VERTEX_EDGE_FLAG) (*gc->procs.renderLine)(gc, c, a);
}

/* 690a1490 -- __glRenderTrianglePoint (s_pgmode.c): GL_POINT polygon mode */
void __fastcall __glRenderTrianglePoint(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    if (a->flagBits & __GL_VERTEX_EDGE_FLAG) (*gc->procs.renderPoint)(gc, a);
    if (b->flagBits & __GL_VERTEX_EDGE_FLAG) (*gc->procs.renderPoint)(gc, b);
    if (c->flagBits & __GL_VERTEX_EDGE_FLAG) (*gc->procs.renderPoint)(gc, c);
}

/************************************************************************/
/* s_texextr.c texel extract procs (level, tex, img, row, col, result): the 2D formats outside the
** level read the border colour, the _B forms address a level stored with its border.  The ICD tests
** row and col against 0 before the sizes; img is unused. */

/* 690a2100 -- __glExtractTexelL8 */
void __fastcall __glExtractTexelL8(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;
    GLubyte luminance;

    if ((row < 0) || (col < 0) ||
        (row >= level->height2) || (col >= level->width2)) {
        /*
        ** Use border color when the texture supplies no border.
        */
        luminance = (GLubyte) (255.0F * tex->params.borderColor.r);
    } else {
        image = (GLubyte *)level->buffer + (row << level->widthLog2) + col;
        luminance = image[0];
    }
    result->r = luminance;
    result->g = luminance;
    result->b = luminance;
}

/* 690a21d0 -- __glExtractTexelRGB8 */
void __fastcall __glExtractTexelRGB8(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;

    if ((row < 0) || (col < 0) ||
        (row >= level->height2) || (col >= level->width2)) {
        /*
        ** Use border color when the texture supplies no border.
        */
        result->r = (GLubyte) (255.0F * tex->params.borderColor.r);
        result->g = (GLubyte) (255.0F * tex->params.borderColor.g);
        result->b = (GLubyte) (255.0F * tex->params.borderColor.b);
    } else {
        image = (GLubyte *)level->buffer + ((row << level->widthLog2) + col) * 3;
        result->r = image[0];
        result->g = image[1];
        result->b = image[2];
    }
}

/* 690a2300 -- __glExtractTexelA8 */
void __fastcall __glExtractTexelA8(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;

    if ((row < 0) || (col < 0) ||
        (row >= level->height2) || (col >= level->width2)) {
        /*
        ** Use border color when the texture supplies no border.
        */
        result->a = (GLubyte) (255.0F * tex->params.borderColor.a);
    } else {
        image = (GLubyte *)level->buffer + (row << level->widthLog2) + col;
        result->a = image[0];
    }
}

/* 690a2350 -- __glExtractTexelI8 */
void __fastcall __glExtractTexelI8(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;
    GLubyte intensity;

    if ((row < 0) || (col < 0) ||
        (row >= level->height2) || (col >= level->width2)) {
        /*
        ** Use border color when the texture supplies no border.
        */
        intensity = (GLubyte) (255.0F * tex->params.borderColor.r);
    } else {
        image = (GLubyte *)level->buffer + (row << level->widthLog2) + col;
        intensity = image[0];
    }
    result->r = intensity;
    result->g = intensity;
    result->b = intensity;
    result->a = intensity;
}

/* 690a23b0 -- __glExtractTexelXRGB1555 */
void __fastcall __glExtractTexelXRGB1555(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLushort *image, texel;

    if ((row < 0) || (col < 0) ||
        (row >= level->height2) || (col >= level->width2)) {
        /*
        ** Use border color when the texture supplies no border.
        */
        result->r = (GLubyte) (255.0F * tex->params.borderColor.r);
        result->g = (GLubyte) (255.0F * tex->params.borderColor.g);
        result->b = (GLubyte) (255.0F * tex->params.borderColor.b);
    } else {
        image = (GLushort *)level->buffer + (row << level->widthLog2) + col;

        texel = image[0];
        result->r = (GLubyte) ((texel >> 10) & 0x1F) * (255.0F/31.0F);
        result->g = (GLubyte) ((texel >>  5) & 0x1F) * (255.0F/31.0F);
        result->b = (GLubyte) ((texel      ) & 0x1F) * (255.0F/31.0F);
    }
}

/* 690a24a0 -- __glExtractTexelRGB332 */
void __fastcall __glExtractTexelRGB332(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image, texel;

    if ((row < 0) || (col < 0) ||
        (row >= level->height2) || (col >= level->width2)) {
        /*
        ** Use border color when the texture supplies no border.
        */
        result->r = (GLubyte) (255.0F * tex->params.borderColor.r);
        result->g = (GLubyte) (255.0F * tex->params.borderColor.g);
        result->b = (GLubyte) (255.0F * tex->params.borderColor.b);
    } else {
        image = (GLubyte *)level->buffer + (row << level->widthLog2) + col;

        texel = image[0];
        result->r = (GLubyte) ((texel >> 5) & 0x7) * (255.0F/7.0F);
        result->g = (GLubyte) ((texel >> 2) & 0x7) * (255.0F/7.0F);
        result->b = (GLubyte) ((texel     ) & 0x3) * (255.0F/3.0F);
    }
}

/* 690a28b0 -- __glExtractTexelRGBA5551 */
void __fastcall __glExtractTexelRGBA5551(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLushort *image, texel;

    if ((row < 0) || (col < 0) ||
        (row >= level->height2) || (col >= level->width2)) {
        /*
        ** Use border color when the texture supplies no border.
        */
        result->r = (GLubyte) (255.0F * tex->params.borderColor.r);
        result->g = (GLubyte) (255.0F * tex->params.borderColor.g);
        result->b = (GLubyte) (255.0F * tex->params.borderColor.b);
        result->a = (GLubyte) (255.0F * tex->params.borderColor.a);
    } else {
        image = (GLushort *)level->buffer + (row << level->widthLog2) + col;
        texel = image[0];
        result->r = (__GLfloat) ((texel >> 11) & 0x1F) * (255.0F/31.0F);
        result->g = (__GLfloat) ((texel >>  6) & 0x1F) * (255.0F/31.0F);
        result->b = (__GLfloat) ((texel >>  1) & 0x1F) * (255.0F/31.0F);
        result->a = (__GLfloat) ((texel >>  0) & 0x01) * 255.0F;
    }
}

/* 690a29c0 -- __glExtractTexelARGB1555 */
void __fastcall __glExtractTexelARGB1555(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLushort *image, texel;

    if ((row < 0) || (col < 0) ||
        (row >= level->height2) || (col >= level->width2)) {
        /*
        ** Use border color when the texture supplies no border.
        */
        result->r = (GLubyte) (255.0F * tex->params.borderColor.r);
        result->g = (GLubyte) (255.0F * tex->params.borderColor.g);
        result->b = (GLubyte) (255.0F * tex->params.borderColor.b);
        result->a = (GLubyte) (255.0F * tex->params.borderColor.a);
    } else {
        image = (GLushort *)level->buffer + (row << level->widthLog2) + col;
        texel = image[0];
        result->r = (__GLfloat) ((texel >> 10) & 0x1F) * (255.0F/31.0F);
        result->g = (__GLfloat) ((texel >>  5) & 0x1F) * (255.0F/31.0F);
        result->b = (__GLfloat) ((texel      ) & 0x1F) * (255.0F/31.0F);
        result->a = (__GLfloat) ((texel >> 15) & 0x01) * 255.0F;
    }
}

/* 690a2d90 -- __glExtractTexelL8_B */
void __fastcall __glExtractTexelL8_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;
    GLubyte luminance;

    row++;
    col++;
    image = (GLubyte *)level->buffer + (row * level->width) + col;
    luminance = image[0];
    result->r = luminance;
    result->g = luminance;
    result->b = luminance;
}

/* 690a2df0 -- __glExtractTexelRGB8_B */
void __fastcall __glExtractTexelRGB8_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;

    row++;
    col++;
    image = (GLubyte *)level->buffer + (row * level->width + col) * 3;
    result->r = image[0];
    result->g = image[1];
    result->b = image[2];
}

/* 690a2e30 -- __glExtractTexelRGBA8_B */
void __fastcall __glExtractTexelRGBA8_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;

    row++;
    col++;
    image = (GLubyte *)level->buffer + (row * level->width + col) * 4;
    result->r = image[0];
    result->g = image[1];
    result->b = image[2];
    result->a = image[3];
}

/* 690a2e70 -- __glExtractTexelA8_B (row << widthLog2 as in the SI, not row * width: the level width
** includes the border) */
void __fastcall __glExtractTexelA8_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;

    row++;
    col++;
    image = (GLubyte *)level->buffer + (row << level->widthLog2) + col;
    result->a = image[0];
}

/* 690a2ea0 -- __glExtractTexelI8_B (row << widthLog2, as A8_B) */
void __fastcall __glExtractTexelI8_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;
    GLubyte intensity;

    row++;
    col++;
    image = (GLubyte *)level->buffer + (row << level->widthLog2) + col;
    intensity = image[0];
    result->r = intensity;
    result->g = intensity;
    result->b = intensity;
    result->a = intensity;
}

/* 690a3f90 -- __glLinearFilterUVScaled (s_texel.c __glLinearFilter without the width2f / height2f
** scaling: s and t come in texel units); lod is unused, the base level is level[0].
*/
void __fastcall __glLinearFilterUVScaled(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod, __GLfloat s, __GLfloat t)
{
    (*(__GLtexFilterProcA00)tex->linear)(tex, tex->level[0], s, t, result);
}

/* 690a3fb0 -- __glNearestFilterUVScaled (s_texel.c __glNearestFilter, s and t in texel units) */
void __fastcall __glNearestFilterUVScaled(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod, __GLfloat s, __GLfloat t)
{
    (*(__GLtexFilterProcA00)tex->nearest)(tex, tex->level[0], s, t, result);
}

/************************************************************************/
/* s_texextr.c / s_texel.c */
#define LP_WIDTH_LOG2(lp)   ((lp)->widthLog2)
/* ---- s_texextr.c -------------------------------------------------------------------------- */
#define WIDTHLOG2(level)    (level)->widthLog2
/************************************************************************/
/* s_texextr.c: the palette texel fetches */

/* the texture's colour table as the extract procs address it: a view from tex->CT.table (+0x60), so
** the table loads come out as the original's [index+table] (glctx.h __GLcolorTableICD starts at +0x5c) */
typedef struct __GLcolorTableW10Rec {
    GLubyte    *table;                      /* tex +0x60  (CT.table) */
    GLuint      unk_04[9];
    GLenum      baseFormat;                 /* tex +0x88  (CT.baseFormat) */
} __GLcolorTable;
#define TEX_CT(tex)         ((__GLcolorTable *)&(tex)->CT.table)
#define EXTRACT_CI_B(name, TYPE)                                                        \
void __fastcall name(__GLmipMapLevelICD *level, __GLtextureICD *tex,                        \
                     GLint img, GLint row, GLint col, __GLtexel *result)                    \
{                                                                                           \
    TYPE *image;                                                                            \
    GLuint index;                                                                           \
    __GLcolorTable *ct = TEX_CT(tex);                                                \
    GLubyte luminance, alpha, intensity;                                                    \
                                                                                            \
    row++;                                                                                  \
    col++;                                                                                  \
    image = (TYPE *)level->buffer + (row << WIDTHLOG2(level)) + col;                    \
    index = image[0];                                                                       \
                                                                                            \
    if (tex->baseFormat == GL_COLOR_INDEX) {                                                \
        result->r = index;                                                                  \
    } else {                                                                                \
        switch (ct->baseFormat) {                                                           \
        case GL_LUMINANCE:                                                                  \
            luminance = ct->table[index];                                                   \
            result->r = luminance;                                                          \
            result->g = luminance;                                                          \
            result->b = luminance;                                                          \
            break;                                                                          \
        case GL_LUMINANCE_ALPHA:                                                            \
            index <<= 1;                                                                    \
            luminance = ct->table[index];                                                   \
            alpha = ct->table[index+1];                                                     \
            result->r = luminance;                                                          \
            result->g = luminance;                                                          \
            result->b = luminance;                                                          \
            result->a = alpha;                                                              \
            break;                                                                          \
        case GL_RGB:                                                                        \
            index += index<<1;                                                              \
            result->r = ct->table[index];                                                   \
            result->g = ct->table[index+1];                                                 \
            result->b = ct->table[index+2];                                                 \
            break;                                                                          \
        case GL_RGBA:                                                                       \
            index <<= 2;                                                                    \
            result->r = ct->table[index];                                                   \
            result->g = ct->table[index+1];                                                 \
            result->b = ct->table[index+2];                                                 \
            result->a = ct->table[index+3];                                                 \
            break;                                                                          \
        case GL_ALPHA:                                                                      \
            result->a = ct->table[index];                                                   \
            break;                                                                          \
        case GL_INTENSITY:                                                                  \
            intensity = ct->table[index];                                                   \
            result->r = intensity;                                                          \
            result->g = intensity;                                                          \
            result->b = intensity;                                                          \
            result->a = intensity;                                                          \
            break;                                                                          \
        }                                                                                   \
    }                                                                                       \
}
#define TEX_CT_TABLE(tex)   ((tex)->CT.table)

/* 690a2160 -- __glExtractTexelLA8 (border colour outside, RGBA bytes out) */
void __fastcall __glExtractTexelLA8(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLint idx;
    GLubyte luminance, alpha;

    if ((row < 0) || (col < 0) ||
        (row >= level->height2) || (col >= level->width2)) {
        luminance = (GLubyte) (tex->params.borderColor.r * 255.0F);
        alpha = (GLubyte) (tex->params.borderColor.a * 255.0F);
    } else {
        idx = (row << LP_WIDTH_LOG2(level)) + col;
        luminance = ((GLubyte *)level->buffer)[idx * 2];
        alpha = ((GLubyte *)level->buffer)[idx * 2 + 1];
    }
    result->r = luminance;
    result->g = luminance;
    result->b = luminance;
    result->a = alpha;
}

/* 690a2260 -- s_texextr.c __glExtractTexelRGBA8 */
void __fastcall __glExtractTexelRGBA8(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;

    if ((row < 0) || (col < 0) ||
	(row >= level->height2) || (col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = (GLubyte) (255.0F * tex->params.borderColor.r);
	result->g = (GLubyte) (255.0F * tex->params.borderColor.g);
	result->b = (GLubyte) (255.0F * tex->params.borderColor.b);
	result->a = (GLubyte) (255.0F * tex->params.borderColor.a);
    } else {
	image = (GLubyte *)level->buffer + ((row << WIDTHLOG2(level)) + col) * 4;
	result->r = image[0];
	result->g = image[1];
	result->b = image[2];
	result->a = image[3];
    }
}

/* 690a2670 -- s_texextr.c __glExtractTexelRGBA4 */
void __fastcall __glExtractTexelRGBA4(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLushort *image, texel;

    if ((row < 0) || (col < 0) ||
	(row >= level->height2) || (col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = (GLubyte) (255.0F * tex->params.borderColor.r);
	result->g = (GLubyte) (255.0F * tex->params.borderColor.g);
	result->b = (GLubyte) (255.0F * tex->params.borderColor.b);
	result->a = (GLubyte) (255.0F * tex->params.borderColor.a);
    } else {
	image = (GLushort *)level->buffer + (row << WIDTHLOG2(level)) + col;
	texel = image[0];
	result->r = (GLubyte) ((texel >> 12) & 0x0F) * (255.0F/15.0F);
	result->g = (GLubyte) ((texel >>  8) & 0x0F) * (255.0F/15.0F);
	result->b = (GLubyte) ((texel >>  4) & 0x0F) * (255.0F/15.0F);
	result->a = (GLubyte) ((texel >>  0) & 0x0F) * (255.0F/15.0F);
    }
}

/* 690a2790 -- s_texextr.c __glExtractTexelARGB4 */
void __fastcall __glExtractTexelARGB4(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLushort *image, texel;

    if ((row < 0) || (col < 0) ||
	(row >= level->height2) || (col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = (GLubyte) (255.0F * tex->params.borderColor.r);
	result->g = (GLubyte) (255.0F * tex->params.borderColor.g);
	result->b = (GLubyte) (255.0F * tex->params.borderColor.b);
	result->a = (GLubyte) (255.0F * tex->params.borderColor.a);
    } else {
	image = (GLushort *)level->buffer + (row << WIDTHLOG2(level)) + col;
	texel = image[0];
	result->r = (GLubyte) ((texel >>  8) & 0x0F) * (255.0F/15.0F);
	result->g = (GLubyte) ((texel >>  4) & 0x0F) * (255.0F/15.0F);
	result->b = (GLubyte) ((texel	   ) & 0x0F) * (255.0F/15.0F);
	result->a = (GLubyte) ((texel >> 12) & 0x0F) * (255.0F/15.0F);
    }
}

/* 690a2dc0 -- s_texextr.c __glExtractTexelLA8_B */
void __fastcall __glExtractTexelLA8_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;
    GLubyte luminance, alpha;

    row++;
    col++;
    image = (GLubyte *)level->buffer + (row * level->width + col) * 2;
    luminance = image[0];
    alpha = image[1];
    result->r = luminance;
    result->g = luminance;
    result->b = luminance;
    result->a = alpha;
}

/* 690a3340 -- __glExtractTexelCI8_B (s_texextr.c) */
EXTRACT_CI_B(__glExtractTexelCI8_B, GLubyte)

/* 690a3470 -- __glExtractTexelCI16_B (s_texextr.c) */
EXTRACT_CI_B(__glExtractTexelCI16_B, GLushort)

/* 690a2ad0 -- __glExtractTexelCI8 (border colour outside the level).  The RGB / RGBA cases read the
** table through the texture pointer (TEX_CT_TABLE): VC6 then emits [index+table] as the original */
void __fastcall __glExtractTexelCI8(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;
    GLuint index;
    __GLcolorTable *ct = TEX_CT(tex);
    GLubyte luminance, alpha, intensity;

    if ((row < 0) || (col < 0) ||
        (row >= level->height2) || (col >= level->width2)) {
        /*
        ** Use border color when the texture supplies no border.
        */
        index = (GLubyte) (255.0F * tex->params.borderColor.r);
    } else {
        image = (GLubyte *)level->buffer + (row << WIDTHLOG2(level)) + col;
        index = image[0];
    }

    if (tex->baseFormat == GL_COLOR_INDEX) {
        result->r = index;
    } else {
        switch (ct->baseFormat) {
        case GL_LUMINANCE:
            luminance = ct->table[index];
            result->r = luminance;
            result->g = luminance;
            result->b = luminance;
            break;
        case GL_LUMINANCE_ALPHA:
            index <<= 1;
            luminance = ct->table[index];
            alpha = ct->table[index+1];
            result->r = luminance;
            result->g = luminance;
            result->b = luminance;
            result->a = alpha;
            break;
        case GL_RGB:
            index += index<<1;
            result->r = TEX_CT_TABLE(tex)[index];
            result->g = TEX_CT_TABLE(tex)[index+1];
            result->b = TEX_CT_TABLE(tex)[index+2];
            break;
        case GL_RGBA:
            index <<= 2;
            result->r = TEX_CT_TABLE(tex)[index];
            result->g = TEX_CT_TABLE(tex)[index+1];
            result->b = TEX_CT_TABLE(tex)[index+2];
            result->a = TEX_CT_TABLE(tex)[index+3];
            break;
        case GL_ALPHA:
            result->a = ct->table[index];
            break;
        case GL_INTENSITY:
            intensity = ct->table[index];
            result->r = intensity;
            result->g = intensity;
            result->b = intensity;
            result->a = intensity;
            break;
        }
    }
}

/* 690a2c30 -- __glExtractTexelCI16 (s_texextr.c): the ICD reads tex->CT.table at every use, through a pointer to
** it (as the SI's ct): VC6 then emits [table+index] as the original */
void __fastcall __glExtractTexelCI16(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLushort *image;
    GLuint index;
    GLubyte luminance, alpha, intensity;
    GLubyte **ctTable = &tex->CT.table;

    if ((row < 0) || (col < 0) ||
        (row >= level->height2) || (col >= level->width2)) {
        index = (GLubyte) (tex->params.borderColor.r * 255.0F);
    } else {
        image = (GLushort *)level->buffer + (row << level->widthLog2) + col;
        index = image[0];
    }

    if (tex->baseFormat == GL_COLOR_INDEX) {
        result->r = index;
    } else {
        switch (tex->CT.baseFormat) {
          case GL_LUMINANCE:
            luminance = (*ctTable)[index];
            result->r = luminance;
            result->g = luminance;
            result->b = luminance;
            break;
          case GL_LUMINANCE_ALPHA:
            index <<= 1;
            luminance = (*ctTable)[index];
            alpha = (*ctTable)[index+1];
            result->r = luminance;
            result->g = luminance;
            result->b = luminance;
            result->a = alpha;
            break;
          case GL_RGB:
            index += index<<1;
            result->r = (*ctTable)[index];
            result->g = (*ctTable)[index+1];
            result->b = (*ctTable)[index+2];
            break;
          case GL_RGBA:
            index <<= 2;
            result->r = (*ctTable)[index];
            result->g = (*ctTable)[index+1];
            result->b = (*ctTable)[index+2];
            result->a = (*ctTable)[index+3];
            break;
          case GL_ALPHA:
            result->a = (*ctTable)[index];
            break;
          case GL_INTENSITY:
            intensity = (*ctTable)[index];
            result->r = intensity;
            result->g = intensity;
            result->b = intensity;
            result->a = intensity;
            break;
        }
    }
}

/*****************************************************************/
/*		    Texture internal formats (s_texfmt.c)	 */
/*****************************************************************/
/* the pixel-span format and types of the stored texels (pixel.h) */
#define __GL_RED_ALPHA                  1
#define __GL_UNSIGNED_SHORT_5_6_5       0
#define __GL_UNSIGNED_SHORT_X_5_5_5     1
#define __GL_UNSIGNED_SHORT_1_5_5_5     2
#define __GL_UNSIGNED_SHORT_4_4_4_4_ARGB 3
#define GL_UNSIGNED_BYTE_3_3_2          0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4       0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1       0x8034
/* the extract procs defined elsewhere: gl_tex.c, si_s_texextr.c */
extern void __fastcall __glExtractTexelRGB565(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelRGB332_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelXRGB1555_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelRGB565_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelRGBA4_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelARGB4_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelRGBA5551_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);
extern void __fastcall __glExtractTexelARGB1555_B(__GLmipMapLevelICD *, __GLtextureICD *, GLint, GLint, GLint, __GLtexel *);

/* 69005b20 */
const __GLtextureFormatSI __glTexFormatLuminance8 = {
    __GL_FORMAT_LUMINANCE8,	/* internalFormat */
    0,				/* indexSize */
    0,				/* redSize */
    0,				/* greenSize */
    0,				/* blueSize */
    0,				/* alphaSize */
    8,				/* luminanceSize */
    0,				/* intensitySize */
    8,				/* bitsPerTexel */
    GL_RED,			/* pxFormat */
    GL_UNSIGNED_BYTE,		/* pxType */
    1,				/* pxAlignment */
    __glExtractTexelL8,		/* extractTexel */
    __glExtractTexelL8_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005b68 */
const __GLtextureFormatSI __glTexFormatLuminanceAlpha8 = {
    __GL_FORMAT_LUMINANCE_ALPHA8,	/* internalFormat */
    0,				/* indexSize */
    0,				/* redSize */
    0,				/* greenSize */
    0,				/* blueSize */
    8,				/* alphaSize */
    8,				/* luminanceSize */
    0,				/* intensitySize */
    16,				/* bitsPerTexel */
    __GL_RED_ALPHA,		/* pxFormat */
    GL_UNSIGNED_BYTE,		/* pxType */
    1,				/* pxAlignment */
    __glExtractTexelLA8,	/* extractTexel */
    __glExtractTexelLA8_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005bb0 */
const __GLtextureFormatSI __glTexFormatRGB8 = {
    __GL_FORMAT_RGB8,		/* internalFormat */
    0,				/* indexSize */
    8,				/* redSize */
    8,				/* greenSize */
    8,				/* blueSize */
    0,				/* alphaSize */
    0,				/* luminanceSize */
    0,				/* intensitySize */
    24,				/* bitsPerTexel */
    GL_RGB,			/* pxFormat */
    GL_UNSIGNED_BYTE,		/* pxType */
    1,				/* pxAlignment */
    __glExtractTexelRGB8,	/* extractTexel */
    __glExtractTexelRGB8_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005bf8 */
const __GLtextureFormatSI __glTexFormatRGB332 = {
    __GL_FORMAT_RGB332,		/* internalFormat */
    0,				/* indexSize */
    3,				/* redSize */
    3,				/* greenSize */
    2,				/* blueSize */
    0,				/* alphaSize */
    0,				/* luminanceSize */
    0,				/* intensitySize */
    8,				/* bitsPerTexel */
    GL_RGB,			/* pxFormat */
    GL_UNSIGNED_BYTE_3_3_2,	/* pxType */
    1,				/* pxAlignment */
    __glExtractTexelRGB332,	/* extractTexel */
    __glExtractTexelRGB332_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005c40 */
const __GLtextureFormatSI __glTexFormatXRGB1555 = {
    __GL_FORMAT_XRGB1555,	/* internalFormat */
    0,				/* indexSize */
    5,				/* redSize */
    5,				/* greenSize */
    5,				/* blueSize */
    0,				/* alphaSize */
    0,				/* luminanceSize */
    0,				/* intensitySize */
    16,				/* bitsPerTexel */
    GL_RGB,			/* pxFormat */
    __GL_UNSIGNED_SHORT_X_5_5_5,	/* pxType */
    2,				/* pxAlignment */
    __glExtractTexelXRGB1555,	/* extractTexel */
    __glExtractTexelXRGB1555_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005c88 */
const __GLtextureFormatSI __glTexFormatRGB565 = {
    __GL_FORMAT_RGB565,		/* internalFormat */
    0,				/* indexSize */
    5,				/* redSize */
    6,				/* greenSize */
    5,				/* blueSize */
    0,				/* alphaSize */
    0,				/* luminanceSize */
    0,				/* intensitySize */
    16,				/* bitsPerTexel */
    GL_RGB,			/* pxFormat */
    __GL_UNSIGNED_SHORT_5_6_5,	/* pxType */
    2,				/* pxAlignment */
    __glExtractTexelRGB565,	/* extractTexel */
    __glExtractTexelRGB565_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005cd0 */
const __GLtextureFormatSI __glTexFormatRGBA8 = {
    __GL_FORMAT_RGBA8,		/* internalFormat */
    0,				/* indexSize */
    8,				/* redSize */
    8,				/* greenSize */
    8,				/* blueSize */
    8,				/* alphaSize */
    0,				/* luminanceSize */
    0,				/* intensitySize */
    32,				/* bitsPerTexel */
    GL_RGBA,			/* pxFormat */
    GL_UNSIGNED_BYTE,		/* pxType */
    1,				/* pxAlignment */
    __glExtractTexelRGBA8,	/* extractTexel */
    __glExtractTexelRGBA8_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005d18 */
const __GLtextureFormatSI __glTexFormatRGBA4 = {
    __GL_FORMAT_RGBA4,		/* internalFormat */
    0,				/* indexSize */
    4,				/* redSize */
    4,				/* greenSize */
    4,				/* blueSize */
    4,				/* alphaSize */
    0,				/* luminanceSize */
    0,				/* intensitySize */
    16,				/* bitsPerTexel */
    GL_RGBA,			/* pxFormat */
    GL_UNSIGNED_SHORT_4_4_4_4,	/* pxType */
    2,				/* pxAlignment */
    __glExtractTexelRGBA4,	/* extractTexel */
    __glExtractTexelRGBA4_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005d60 */
const __GLtextureFormatSI __glTexFormatARGB4 = {
    __GL_FORMAT_ARGB4,		/* internalFormat */
    0,				/* indexSize */
    4,				/* redSize */
    4,				/* greenSize */
    4,				/* blueSize */
    4,				/* alphaSize */
    0,				/* luminanceSize */
    0,				/* intensitySize */
    16,				/* bitsPerTexel */
    GL_RGBA,			/* pxFormat */
    __GL_UNSIGNED_SHORT_4_4_4_4_ARGB,	/* pxType */
    2,				/* pxAlignment */
    __glExtractTexelARGB4,	/* extractTexel */
    __glExtractTexelARGB4_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005da8 */
const __GLtextureFormatSI __glTexFormatRGBA5551 = {
    __GL_FORMAT_RGBA5551,	/* internalFormat */
    0,				/* indexSize */
    5,				/* redSize */
    5,				/* greenSize */
    5,				/* blueSize */
    1,				/* alphaSize */
    0,				/* luminanceSize */
    0,				/* intensitySize */
    16,				/* bitsPerTexel */
    GL_RGBA,			/* pxFormat */
    GL_UNSIGNED_SHORT_5_5_5_1,	/* pxType */
    1,				/* pxAlignment */
    __glExtractTexelRGBA5551,	/* extractTexel */
    __glExtractTexelRGBA5551_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005df0 */
const __GLtextureFormatSI __glTexFormatARGB1555 = {
    __GL_FORMAT_ARGB1555,	/* internalFormat */
    0,				/* indexSize */
    5,				/* redSize */
    5,				/* greenSize */
    5,				/* blueSize */
    1,				/* alphaSize */
    0,				/* luminanceSize */
    0,				/* intensitySize */
    16,				/* bitsPerTexel */
    GL_RGBA,			/* pxFormat */
    __GL_UNSIGNED_SHORT_1_5_5_5,	/* pxType */
    2,				/* pxAlignment */
    __glExtractTexelARGB1555,	/* extractTexel */
    __glExtractTexelARGB1555_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005e38 */
const __GLtextureFormatSI __glTexFormatAlpha8 = {
    __GL_FORMAT_ALPHA8,		/* internalFormat */
    0,				/* indexSize */
    0,				/* redSize */
    0,				/* greenSize */
    0,				/* blueSize */
    8,				/* alphaSize */
    0,				/* luminanceSize */
    0,				/* intensitySize */
    8,				/* bitsPerTexel */
    GL_ALPHA,			/* pxFormat */
    GL_UNSIGNED_BYTE,		/* pxType */
    1,				/* pxAlignment */
    __glExtractTexelA8,		/* extractTexel */
    __glExtractTexelA8_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005e80 */
const __GLtextureFormatSI __glTexFormatIntensity8 = {
    __GL_FORMAT_INTENSITY8,	/* internalFormat */
    0,				/* indexSize */
    0,				/* redSize */
    0,				/* greenSize */
    0,				/* blueSize */
    0,				/* alphaSize */
    0,				/* luminanceSize */
    8,				/* intensitySize */
    8,				/* bitsPerTexel */
    GL_RED,			/* pxFormat */
    GL_UNSIGNED_BYTE,		/* pxType */
    1,				/* pxAlignment */
    __glExtractTexelI8,		/* extractTexel */
    __glExtractTexelI8_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005ec8 */
const __GLtextureFormatSI __glTexFormatColorIndex8 = {
    __GL_FORMAT_COLOR_INDEX8,	/* internalFormat */
    0,				/* indexSize */
    0,				/* redSize */
    0,				/* greenSize */
    0,				/* blueSize */
    0,				/* alphaSize */
    0,				/* luminanceSize */
    0,				/* intensitySize */
    8,				/* bitsPerTexel */
    GL_COLOR_INDEX,		/* pxFormat */
    GL_UNSIGNED_BYTE,		/* pxType */
    1,				/* pxAlignment */
    __glExtractTexelCI8,	/* extractTexel */
    __glExtractTexelCI8_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

/* 69005f10 */
const __GLtextureFormatSI __glTexFormatColorIndex16 = {
    __GL_FORMAT_COLOR_INDEX16,	/* internalFormat */
    0,				/* indexSize */
    0,				/* redSize */
    0,				/* greenSize */
    0,				/* blueSize */
    0,				/* alphaSize */
    0,				/* luminanceSize */
    0,				/* intensitySize */
    16,				/* bitsPerTexel */
    GL_COLOR_INDEX,		/* pxFormat */
    GL_UNSIGNED_SHORT,		/* pxType */
    2,				/* pxAlignment */
    __glExtractTexelCI16,	/* extractTexel */
    __glExtractTexelCI16_B,	/* extractTexelBorder */
    NULL,			/* store */
    NULL,			/* fetch */
    NULL,			/* storeRect */
    NULL,			/* fetchRect */
};

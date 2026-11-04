/*
** si_px_rend.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/px_rend.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 69083fe0 -- __glSpanRenderStencilUshort */
void __fastcall __glSpanRenderStencilUshort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                 const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLushort *spanData;
    void (*store)(__GLstencilBufferICD *sfb, GLint x, GLint y, GLint value);
    __GLstencilBufferICD *sb = &gc->stencilBuffer;
    GLint rows;
    GLint startCol;
    GLint value;
    GLshort *pixelArray;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;

    store = sb->store;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = startCol;
        pixelArray = spanInfo->pixelArray;
        spanData = (const GLushort*) span;

        for (i=0; i<width; i++) {
            iright = column + *pixelArray++;
            value = *spanData++;
            do {
                (*store)(sb, column, row, value);
                column += coladd;
            } while (column != iright);
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 690840f0 -- __glSpanRenderStencilUshort2 */
void __fastcall __glSpanRenderStencilUshort2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                  const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    const GLushort *spanData;
    void (*store)(__GLstencilBufferICD *sfb, GLint x, GLint y, GLint value);
    __GLstencilBufferICD *sb = &gc->stencilBuffer;
    GLint endCol, startCol;
    GLint rows;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);

    store = sb->store;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        spanData = (const GLushort*) span;
        i = startCol;
        do {
            (*store)(sb, i, row, *spanData++);
            i += coladd;
        } while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 690841e0 -- __glSpanRenderStencilUbyte */
void __fastcall __glSpanRenderStencilUbyte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLubyte *spanData;
    void (*store)(__GLstencilBufferICD *sfb, GLint x, GLint y, GLint value);
    __GLstencilBufferICD *sb = &gc->stencilBuffer;
    GLint rows;
    GLint startCol;
    GLint value;
    GLshort *pixelArray;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;

    store = sb->store;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = startCol;
        pixelArray = spanInfo->pixelArray;
        spanData = (const GLubyte*) span;

        for (i=0; i<width; i++) {
            iright = column + *pixelArray++;
            value = *spanData++;
            do {
                (*store)(sb, column, row, value);
                column += coladd;
            } while (column != iright);
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 690842f0 -- __glSpanRenderStencilUbyte2 */
void __fastcall __glSpanRenderStencilUbyte2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                 const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    const GLubyte *spanData;
    void (*store)(__GLstencilBufferICD *sfb, GLint x, GLint y, GLint value);
    __GLstencilBufferICD *sb = &gc->stencilBuffer;
    GLint endCol, startCol;
    GLint rows;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);

    store = sb->store;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        spanData = (const GLubyte*) span;
        i = startCol;
        do {
            (*store)(sb, i, row, *spanData++);
            i += coladd;
        } while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69085560 -- __glSpanRenderStencil */
void __fastcall __glSpanRenderStencil(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                           const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLfloat *spanData;
    GLint index;
    __GLstencilBufferICD *sb;
    GLint rows;
    GLshort *pixelArray;
    GLint mask;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;
    mask = (1<<gc->modes.stencilBits)-1;

    sb = &gc->stencilBuffer;

    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = spanInfo->startCol;
        pixelArray = spanInfo->pixelArray;
        spanData = (const GLfloat*) span;
        for (i=0; i<width; i++) {
            iright = column + *pixelArray++;
            index = (GLint) (*spanData++) & mask;
            do {
                (*sb->store)(sb, column, row, index);
                column += coladd;
            } while (column != iright);
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69085690 -- __glSpanRenderStencil2 */
void __fastcall __glSpanRenderStencil2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                            const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLfloat *spanData;
    GLint index;
    __GLstencilBufferICD *sb;
    GLint rows;
    GLint mask;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;
    mask = (1<<gc->modes.stencilBits)-1;

    sb = &gc->stencilBuffer;

    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = spanInfo->startCol;
        spanData = (const GLfloat*) span;
        for (i=0; i<width; i++) {
            index = (GLint) (*spanData++) & mask;
            (*sb->store)(sb, column, row, index);
            column += coladd;
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 690836d0 -- __glSpanRenderRGBubyte */
void __fastcall __glSpanRenderRGBubyte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                            const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLubyte *spanData;
    GLfloat *redMap, *greenMap, *blueMap;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLint rows;
    GLint startCol;
    GLshort *pixelArray;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;

    redMap = (GLfloat*) gc->pixel.redCurMap;
    greenMap = (GLfloat*) gc->pixel.greenCurMap;
    blueMap = (GLfloat*) gc->pixel.blueCurMap;
    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    frag.color.a =
        ((GLfloat *) (gc->pixel.alphaCurMap))[255];
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = startCol;
        pixelArray = spanInfo->pixelArray;
        spanData = (const GLubyte*) span;
        frag.y = row;

        for (i=0; i<width; i++) {
            iright = column + *pixelArray++;
            frag.color.r = redMap[*spanData++];
            frag.color.g = greenMap[*spanData++];
            frag.color.b = blueMap[*spanData++];
            do {
                frag.x = column;

                /* This procedure will do the rest */
                (*store)(gc->drawBuffer, &frag);
                column += coladd;
            } while (column != iright);
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69083850 -- __glSpanRenderRGBubyte2 */
void __fastcall __glSpanRenderRGBubyte2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    const GLubyte *spanData;
    GLfloat *redMap, *greenMap, *blueMap;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLint endCol, startCol;
    GLint rows;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);

    frag.color.a =
        ((GLfloat *) (gc->pixel.alphaCurMap))[255];
    frag.z = spanInfo->fragz;
    redMap = (GLfloat*) gc->pixel.redCurMap;
    greenMap = (GLfloat*) gc->pixel.greenCurMap;
    blueMap = (GLfloat*) gc->pixel.blueCurMap;
    store = gc->procs.pxStore;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        spanData = (const GLubyte*) span;
        frag.y = row;
        i = startCol;
        do {
            frag.color.r = redMap[*spanData++];
            frag.color.g = greenMap[*spanData++];
            frag.color.b = blueMap[*spanData++];
            frag.x = i;

            /* This procedure will do the rest */
            (*store)(gc->drawBuffer, &frag);
            i += coladd;
        } while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 690839c0 -- __glSpanRenderRGBAubyte */
void __fastcall __glSpanRenderRGBAubyte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLubyte *spanData;
    GLfloat *redMap, *greenMap, *blueMap, *alphaMap;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLint rows;
    GLint startCol;
    GLshort *pixelArray;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;

    redMap = (GLfloat*) gc->pixel.redCurMap;
    greenMap = (GLfloat*) gc->pixel.greenCurMap;
    blueMap = (GLfloat*) gc->pixel.blueCurMap;
    alphaMap = (GLfloat*) gc->pixel.alphaCurMap;
    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = startCol;
        pixelArray = spanInfo->pixelArray;
        spanData = (const GLubyte*) span;
        frag.y = row;

        for (i=0; i<width; i++) {
            iright = column + *pixelArray++;
            frag.color.r = redMap[*spanData++];
            frag.color.g = greenMap[*spanData++];
            frag.color.b = blueMap[*spanData++];
            frag.color.a = alphaMap[*spanData++];
            do {
                frag.x = column;

                /* This procedure will do the rest */
                (*store)(gc->drawBuffer, &frag);
                column += coladd;
            } while (column != iright);
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69083b50 -- __glSpanRenderRGBAubyte2 */
void __fastcall __glSpanRenderRGBAubyte2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                              const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    const GLubyte *spanData;
    GLfloat *redMap, *greenMap, *blueMap, *alphaMap;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLint endCol, startCol;
    GLint rows;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);

    frag.z = spanInfo->fragz;
    redMap = (GLfloat*) gc->pixel.redCurMap;
    greenMap = (GLfloat*) gc->pixel.greenCurMap;
    blueMap = (GLfloat*) gc->pixel.blueCurMap;
    alphaMap = (GLfloat*) gc->pixel.alphaCurMap;
    store = gc->procs.pxStore;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        spanData = (const GLubyte*) span;
        frag.y = row;
        i = startCol;
        do {
            frag.color.r = redMap[*spanData++];
            frag.color.g = greenMap[*spanData++];
            frag.color.b = blueMap[*spanData++];
            frag.color.a = alphaMap[*spanData++];
            frag.x = i;

            /* This procedure will do the rest */
            (*store)(gc->drawBuffer, &frag);
            i += coladd;
        } while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 690843e0 -- __glSpanRenderCIuint */
void __fastcall __glSpanRenderCIuint(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                          const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLuint *spanData;
    GLint rows;
    GLint startCol;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLushort *pixelArray;
    GLint mask;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;
    mask = gc->frontBuffer.redMax;

    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = startCol;
        pixelArray = (GLushort *) spanInfo->pixelArray;
        spanData = (const GLuint*) span;
        frag.y = row;

        for (i=0; i<width; i++) {
            iright = column + *pixelArray++;
            frag.color.r =
                (__GLfloat) (*spanData++ & mask);
            do {
                frag.x = column;
                (*store)(gc->drawBuffer, &frag);
                column += coladd;
            } while (column != iright);
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69084520 -- __glSpanRenderCIuint2 */
void __fastcall __glSpanRenderCIuint2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                           const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    const GLuint *spanData;
    GLint endCol, startCol;
    GLint rows;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLint mask;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    mask = gc->frontBuffer.redMax;

    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        spanData = (const GLuint*) span;
        frag.y = row;
        i = startCol;
        do {
            frag.x = i;
            frag.color.r =
                (__GLfloat) (*spanData++ & mask);
            (*store)(gc->drawBuffer, &frag);
            i += coladd;
        } while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69084650 -- __glSpanRenderCIushort */
void __fastcall __glSpanRenderCIushort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                            const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLushort *spanData;
    GLint rows;
    GLint startCol;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLshort *pixelArray;
    GLint mask;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;
    mask = gc->frontBuffer.redMax;

    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = startCol;
        pixelArray = spanInfo->pixelArray;
        spanData = (const GLushort*) span;
        frag.y = row;

        for (i=0; i<width; i++) {
            iright = column + *pixelArray++;
            frag.color.r =
                (__GLfloat) (*spanData++ & mask);
            do {
                frag.x = column;
                (*store)(gc->drawBuffer, &frag);
                column += coladd;
            } while (column != iright);
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69084790 -- __glSpanRenderCIushort2 */
void __fastcall __glSpanRenderCIushort2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    const GLushort *spanData;
    GLint endCol, startCol;
    GLint rows;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLint mask;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    mask = gc->frontBuffer.redMax;

    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        spanData = (const GLushort*) span;
        frag.y = row;
        i = startCol;
        do {
            frag.x = i;
            frag.color.r =
                (__GLfloat) (*spanData++ & mask);
            (*store)(gc->drawBuffer, &frag);
            i += coladd;
        } while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 690848c0 -- __glSpanRenderCIubyte */
void __fastcall __glSpanRenderCIubyte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                           const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLubyte *spanData;
    GLint rows;
    GLint startCol;
    GLfloat *indexMap;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLshort *pixelArray;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;

    indexMap = (GLfloat*) gc->pixel.iCurMap;
    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = startCol;
        pixelArray = spanInfo->pixelArray;
        spanData = (const GLubyte*) span;
        frag.y = row;

        for (i=0; i<width; i++) {
            iright = column + *pixelArray++;
            frag.color.r = indexMap[*spanData++];
            do {
                frag.x = column;
                (*store)(gc->drawBuffer, &frag);
                column += coladd;
            } while (column != iright);
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 690849f0 -- __glSpanRenderCIubyte2 */
void __fastcall __glSpanRenderCIubyte2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                            const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    const GLubyte *spanData;
    GLint endCol, startCol;
    GLint rows;
    GLfloat *indexMap;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);

    indexMap = (GLfloat*) gc->pixel.iCurMap;
    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        spanData = (const GLubyte*) span;
        frag.y = row;
        i = startCol;
        do {
            frag.x = i;
            frag.color.r = indexMap[*spanData++];
            (*store)(gc->drawBuffer, &frag);
            i += coladd;
        } while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69084b10 -- __glSpanRenderCIubyte3 */
void __fastcall __glSpanRenderCIubyte3(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                            const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLubyte *spanData;
    GLint rows;
    GLint startCol;
    GLfloat *redMap, *greenMap, *blueMap, *alphaMap;
    __GLfragmentICD frag;
    GLubyte value;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLshort *pixelArray;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;

    redMap = (GLfloat*) gc->pixel.redCurMap;
    greenMap = (GLfloat*) gc->pixel.greenCurMap;
    blueMap = (GLfloat*) gc->pixel.blueCurMap;
    alphaMap = (GLfloat*) gc->pixel.alphaCurMap;
    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    rows = spanInfo->rows;
    startCol = spanInfo->startCol;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = startCol;
        pixelArray = spanInfo->pixelArray;
        spanData = (const GLubyte*) span;
        frag.y = row;

        for (i=0; i<width; i++) {
            iright = column + *pixelArray++;
            value = *spanData++;
            frag.color.r = redMap[value];
            frag.color.g = greenMap[value];
            frag.color.b = blueMap[value];
            frag.color.a = alphaMap[value];
            do {
                frag.x = column;
                (*store)(gc->drawBuffer, &frag);
                column += coladd;
            } while (column != iright);
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69084ca0 -- __glSpanRenderCIubyte4 */
void __fastcall __glSpanRenderCIubyte4(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                            const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row;
    GLint i;
    GLint rowadd, coladd;
    const GLubyte *spanData;
    GLint endCol, startCol;
    GLint rows;
    GLubyte value;
    GLfloat *redMap, *greenMap, *blueMap, *alphaMap;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);

    redMap = (GLfloat*) gc->pixel.redCurMap;
    greenMap = (GLfloat*) gc->pixel.greenCurMap;
    blueMap = (GLfloat*) gc->pixel.blueCurMap;
    alphaMap = (GLfloat*) gc->pixel.alphaCurMap;
    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;
    startCol = spanInfo->startCol;
    endCol = spanInfo->endCol;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        spanData = (const GLubyte*) span;
        frag.y = row;
        i = startCol;
        do {
            frag.x = i;
            value = *spanData++;
            frag.color.r = redMap[value];
            frag.color.g = greenMap[value];
            frag.color.b = blueMap[value];
            frag.color.a = alphaMap[value];
            (*store)(gc->drawBuffer, &frag);
            i += coladd;
        } while (i != endCol);
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69084e10 -- __glSpanRenderRGBA */
void __fastcall __glSpanRenderRGBA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                        const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLfloat *spanData;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLint rows;
    GLshort *pixelArray;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;

    store = gc->procs.pxStore;

    rows = spanInfo->rows;
    frag.z = spanInfo->fragz;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = spanInfo->startCol;
        pixelArray = spanInfo->pixelArray;
        spanData = (const GLfloat*) span;
        frag.y = row;
        for (i=0; i<width; i++) {
            iright = column + *pixelArray++;
            frag.color.r = *spanData++;
            frag.color.g = *spanData++;
            frag.color.b = *spanData++;
            frag.color.a = *spanData++;
            do {
                frag.x = column;

                /* This procedure will do the rest */
                (*store)(gc->drawBuffer, &frag);
                column += coladd;
            } while (column != iright);
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69084f50 -- __glSpanRenderRGBA2 */
void __fastcall __glSpanRenderRGBA2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLfloat *spanData;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLint rows;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;

    store = gc->procs.pxStore;

    frag.z = spanInfo->fragz;
    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = spanInfo->startCol;
        spanData = (const GLfloat*) span;
        frag.y = row;
        for (i=0; i<width; i++) {
            frag.color.r = *spanData++;
            frag.color.g = *spanData++;
            frag.color.b = *spanData++;
            frag.color.a = *spanData++;
            frag.x = column;

            /* This procedure will do the rest */
            (*store)(gc->drawBuffer, &frag);
            column += coladd;
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69085050 -- __glSpanRenderDepth */
void __fastcall __glSpanRenderDepth(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLfloat *spanData;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLint rows;
    GLshort *pixelArray;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;

    store = gc->procs.pxStore;
    frag.color.r =
        gc->state.current.rasterPos.colors[__GL_FRONTFACE].r *
        gc->constants.redRescale;
    frag.color.g =
        gc->state.current.rasterPos.colors[__GL_FRONTFACE].g *
        gc->constants.greenRescale;
    frag.color.b =
        gc->state.current.rasterPos.colors[__GL_FRONTFACE].b *
        gc->constants.blueRescale;
    frag.color.a =
        gc->state.current.rasterPos.colors[__GL_FRONTFACE].a *
        gc->constants.alphaRescale;

    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = spanInfo->startCol;
        pixelArray = spanInfo->pixelArray;
        spanData = (const GLfloat*) span;
        frag.y = row;
        for (i=0; i<width; i++) {
            iright = column + *pixelArray++;

            frag.z = (__GLzValue) (*spanData++ * gc->depthBuffer.scale);

            do {
                frag.x = column;

                /* This procedure will do the rest */
                (*store)(gc->drawBuffer, &frag);
                column += coladd;
            } while (column != iright);
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 690851c0 -- __glSpanRenderDepth2 */
void __fastcall __glSpanRenderDepth2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                          const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLfloat *spanData;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLint rows;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;

    store = gc->procs.pxStore;
    frag.color.r =
        gc->state.current.rasterPos.colors[__GL_FRONTFACE].r *
        gc->constants.redRescale;
    frag.color.g =
        gc->state.current.rasterPos.colors[__GL_FRONTFACE].g *
        gc->constants.greenRescale;
    frag.color.b =
        gc->state.current.rasterPos.colors[__GL_FRONTFACE].b *
        gc->constants.blueRescale;
    frag.color.a =
        gc->state.current.rasterPos.colors[__GL_FRONTFACE].a *
        gc->constants.alphaRescale;

    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = spanInfo->startCol;
        spanData = (const GLfloat*) span;
        frag.y = row;
        for (i=0; i<width; i++) {
            frag.x = column;
            frag.z = (__GLzValue) (*spanData++ * gc->depthBuffer.scale);

            /* This procedure will do the rest */
            (*store)(gc->drawBuffer, &frag);
            column += coladd;
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69085310 -- __glSpanRenderCI */
void __fastcall __glSpanRenderCI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                      const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom, iright;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLfloat *spanData;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLint rows;
    GLshort *pixelArray;
    GLint mask;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;
    mask = gc->frontBuffer.redMax;

    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;

    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = spanInfo->startCol;
        pixelArray = spanInfo->pixelArray;
        spanData = (const GLfloat*) span;
        frag.y = row;
        for (i=0; i<width; i++) {
            iright = column + *pixelArray++;
            frag.color.r =
                (__GLfloat) ((GLint) (*spanData++) & mask);
            do {
                frag.x = column;

                /* This procedure will do the rest */
                (*store)(gc->drawBuffer, &frag);
                column += coladd;
            } while (column != iright);
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

/* 69085450 -- __glSpanRenderCI2 */
void __fastcall __glSpanRenderCI2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                       const GLvoid *span)
{
    __GLfloat zoomy;
    GLint itop, ibottom;
    GLint row, column;
    GLint i, width;
    GLint coladd, rowadd;
    const GLfloat *spanData;
    __GLfragmentICD frag;
    void (*store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
    GLint rows;
    GLint mask;

    zoomy = spanInfo->zoomy;
    rowadd = spanInfo->rowadd;
    coladd = spanInfo->coladd;
    ibottom = spanInfo->startRow;
    itop = (GLint) (spanInfo->y + zoomy);
    width = spanInfo->width;
    mask = gc->frontBuffer.redMax;

    store = gc->procs.pxStore;
    frag.z = spanInfo->fragz;

    rows = spanInfo->rows;
    for (row = ibottom; row != itop; row += rowadd) {
        if (rows == 0) break;
        rows--;
        column = spanInfo->startCol;
        spanData = (const GLfloat*) span;
        frag.y = row;
        for (i=0; i<width; i++) {
            frag.color.r =
                (__GLfloat) ((GLint) (*spanData++) & mask);
            frag.x = column;

            /* This procedure will do the rest */
            (*store)(gc->drawBuffer, &frag);
            column += coladd;
        }
    }
    spanInfo->rows = rows;
    spanInfo->startRow = itop;
}

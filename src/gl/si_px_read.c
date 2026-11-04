/*
** si_px_read.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/px_read.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 69085be0 -- __glSpanReadStencil */
void __fastcall __glSpanReadStencil(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                         GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;
    GLshort *pixelArray;
    GLint skipCount;

    width = spanInfo->width;
    spanData = (GLfloat*) span;
    pixelArray = spanInfo->pixelArray;

    readY = (GLint) spanInfo->readY;
    readX = (GLint) spanInfo->readX;

    for (i=0; i<width; i++) {
        *spanData++ = (GLfloat)
            ((*gc->stencilBuffer.fetch)(&(gc->stencilBuffer), readX, readY));
        skipCount = *pixelArray++;
        readX += skipCount;
    }
}

/* 69085c70 -- __glSpanReadStencil2 */
void __fastcall __glSpanReadStencil2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                          GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;

    width = spanInfo->width;
    spanData = (GLfloat*) span;

    readY = (GLint) spanInfo->readY;
    readX = (GLint) spanInfo->readX;

    for (i=0; i<width; i++) {
        *spanData++ =  (GLfloat)
            ((*gc->stencilBuffer.fetch)(&(gc->stencilBuffer), readX, readY));
        readX++;
    }
}

/* 69085780 -- __glSpanReadRGBA */
void __fastcall __glSpanReadRGBA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                      GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;
    GLshort *pixelArray;
    GLint skipCount;

    width = spanInfo->width;
    spanData = (GLfloat*) span;
    pixelArray = spanInfo->pixelArray;

    readY = (GLint) spanInfo->readY;
    readX = (GLint) spanInfo->readX;

    for (i=0; i<width; i++) {
        (*gc->readBuffer->readColor)(gc->readBuffer, readX, readY, 
                (__GLcolor *) spanData);

        spanData += 4;
        skipCount = *pixelArray++;
        readX += skipCount;
    }
}

/* 69085800 -- __glSpanReadRGBA2 */
void __fastcall __glSpanReadRGBA2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                       GLvoid *span)
{
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;

    width = spanInfo->width;
    spanData = (GLfloat*) span;

    readY = (GLint) spanInfo->readY;
    readX = (GLint) spanInfo->readX;

    (*gc->readBuffer->readSpan)(gc->readBuffer, readX, readY, 
            (__GLcolor *) spanData, width);
}

/* 69085840 -- __glSpanReadCI */
void __fastcall __glSpanReadCI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                    GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;
    GLshort *pixelArray;
    GLint skipCount;

    width = spanInfo->width;
    spanData = (GLfloat*) span;
    pixelArray = spanInfo->pixelArray;

    readY = (GLint) spanInfo->readY;
    readX = (GLint) spanInfo->readX;

    for (i=0; i<width; i++) {
        (*gc->readBuffer->readColor)(gc->readBuffer, readX, readY, 
                (__GLcolor *) spanData);
        spanData++;
        skipCount = *pixelArray++;
        readX += skipCount;
    }
}

/* 690858c0 -- __glSpanReadCI2 */
void __fastcall __glSpanReadCI2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                     GLvoid *span)
{
    GLint i;
    GLint width;
    GLfloat *spanData;
    GLint readY, readX;

    width = spanInfo->width;
    spanData = (GLfloat*) span;

    readY = (GLint) spanInfo->readY;
    readX = (GLint) spanInfo->readX;

    for (i=0; i<width; i++) {
        (*gc->readBuffer->readColor)(gc->readBuffer, readX, readY, 
                (__GLcolor *) spanData);
        spanData++;
        readX++;
    }
}

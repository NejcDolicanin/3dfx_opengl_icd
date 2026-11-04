/*
** si_px_upack.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/px_upack.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690c8cb0 -- __glSpanUnpackBitmap */
void __fastcall __glSpanUnpackBitmap(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                          const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width;
    const GLvoid *userData;
    GLfloat *spanData;
    GLint lsbFirst;
    GLint startBit;
    GLint bit;
    GLubyte ubyte;
    GLshort *pixelArray;
    GLint skipCount;
    __GLfloat zero, one;

    userData = inspan;
    spanData = (GLfloat *) outspan;
    pixelArray = spanInfo->pixelArray;

    width = spanInfo->width;
    lsbFirst = spanInfo->srcLsbFirst;
    startBit = spanInfo->srcStartBit;

    i = width;
    bit = startBit;
    ubyte = *(const GLubyte *) userData;

    zero = __GL_ZERO_F;
    one = __GL_ONE_F;

    skipCount = 1;
    if (lsbFirst) {
        switch(bit) {
          case 1:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x02) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) break;
          case 2:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x04) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) break;
          case 3:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x08) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) break;
          case 4:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x10) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) break;
          case 5:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x20) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) break;
          case 6:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x40) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) break;
          case 7:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x80) *spanData++ = one;
                else *spanData++ = zero;
            }
            i--;
            userData = (const GLvoid *) ((const GLubyte *) userData + 1);
          case 0:
            break;
        }
        while (i >= 8) {
            ubyte = *(const GLubyte *) userData;
            userData = (const GLvoid *) ((const GLubyte *) userData + 1);
            i -= 8;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x01) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x02) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x04) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x08) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x10) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x20) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x40) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x80) *spanData++ = one;
                else *spanData++ = zero;
            }
        }
        if (i) {
            ubyte = *(const GLubyte *) userData;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x01) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) return;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x02) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) return;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x04) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) return;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x08) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) return;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x10) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) return;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x20) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) return;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x40) *spanData++ = one;
                else *spanData++ = zero;
            }
        }
    } else {
        switch(bit) {
          case 1:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x40) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) break;
          case 2:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x20) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) break;
          case 3:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x10) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) break;
          case 4:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x08) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) break;
          case 5:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x04) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) break;
          case 6:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x02) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) break;
          case 7:
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x01) *spanData++ = one;
                else *spanData++ = zero;
            }
            i--;
            userData = (const GLvoid *) ((const GLubyte *) userData + 1);
          case 0:
            break;
        }
        while (i >= 8) {
            ubyte = *(const GLubyte *) userData;
            userData = (const GLvoid *) ((const GLubyte *) userData + 1);
            i -= 8;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x80) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x40) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x20) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x10) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x08) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x04) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x02) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x01) *spanData++ = one;
                else *spanData++ = zero;
            }
        }
        if (i) {
            ubyte = *(const GLubyte *) userData;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x80) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) return;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x40) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) return;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x20) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) return;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x10) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) return;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x08) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) return;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x04) *spanData++ = one;
                else *spanData++ = zero;
            }
            if (--i == 0) return;
            if (--skipCount == 0) {
                skipCount = *pixelArray++;
                if (ubyte & 0x02) *spanData++ = one;
                else *spanData++ = zero;
            }
        }
    }
}

/* 690c9420 -- __glSpanUnpackBitmap2 */
void __fastcall __glSpanUnpackBitmap2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                           const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width;
    const GLvoid *userData;
    GLfloat *spanData;
    GLint lsbFirst;
    GLint startBit;
    GLint bit;
    GLubyte ubyte;

    width = spanInfo->width;
    userData = inspan;
    spanData = (GLfloat *) outspan;

    lsbFirst = spanInfo->srcLsbFirst;
    startBit = spanInfo->srcStartBit;

    i = width;
    bit = startBit;
    ubyte = *(const GLubyte *) userData;

    if (lsbFirst) {
        switch(bit) {
          case 1:
            if (ubyte & 0x02) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) break;
          case 2:
            if (ubyte & 0x04) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) break;
          case 3:
            if (ubyte & 0x08) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) break;
          case 4:
            if (ubyte & 0x10) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) break;
          case 5:
            if (ubyte & 0x20) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) break;
          case 6:
            if (ubyte & 0x40) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) break;
          case 7:
            if (ubyte & 0x80) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            i--;
            userData = (const GLvoid *) ((const GLubyte *) userData + 1);
          case 0:
            break;
        }
        while (i >= 8) {
            ubyte = *(const GLubyte *) userData;
            userData = (const GLvoid *) ((const GLubyte *) userData + 1);
            i -= 8;
            if (ubyte & 0x01) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x02) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x04) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x08) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x10) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x20) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x40) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x80) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
        }
        if (i) {
            ubyte = *(const GLubyte *) userData;
            if (ubyte & 0x01) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) return;
            if (ubyte & 0x02) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) return;
            if (ubyte & 0x04) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) return;
            if (ubyte & 0x08) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) return;
            if (ubyte & 0x10) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) return;
            if (ubyte & 0x20) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) return;
            if (ubyte & 0x40) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
        }
    } else {
        switch(bit) {
          case 1:
            if (ubyte & 0x40) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) break;
          case 2:
            if (ubyte & 0x20) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) break;
          case 3:
            if (ubyte & 0x10) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) break;
          case 4:
            if (ubyte & 0x08) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) break;
          case 5:
            if (ubyte & 0x04) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) break;
          case 6:
            if (ubyte & 0x02) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) break;
          case 7:
            if (ubyte & 0x01) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            i--;
            userData = (const GLvoid *) ((const GLubyte *) userData + 1);
          case 0:
            break;
        }
        while (i >= 8) {
            ubyte = *(const GLubyte *) userData;
            userData = (const GLvoid *) ((const GLubyte *) userData + 1);
            i -= 8;
            if (ubyte & 0x80) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x40) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x20) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x10) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x08) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x04) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x02) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (ubyte & 0x01) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
        }
        if (i) {
            ubyte = *(const GLubyte *) userData;
            if (ubyte & 0x80) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) return;
            if (ubyte & 0x40) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) return;
            if (ubyte & 0x20) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) return;
            if (ubyte & 0x10) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) return;
            if (ubyte & 0x08) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) return;
            if (ubyte & 0x04) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
            if (--i == 0) return;
            if (ubyte & 0x02) *spanData++ = __GL_ONE_F;
            else *spanData++ = __GL_ZERO_F;
        }
    }
}

/* 690c99d0 -- __glSpanUnpackRGBubyte */
void __fastcall __glSpanUnpackRGBubyte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    const GLubyte *userData;
    GLubyte *spanData;
    GLint width;
    GLshort *pixelArray;
    GLint skipCount;

    width = spanInfo->width;
    userData = (const GLubyte *) inspan;
    spanData = (GLubyte *) outspan;

    pixelArray = spanInfo->pixelArray;

    i = 0;
    do {
        spanData[0] = userData[0];
        spanData[1] = userData[1];
        spanData[2] = userData[2];
        spanData += 3;

        skipCount = *pixelArray++;
        userData = (const GLubyte *) ((const GLubyte *) userData + 3 * skipCount);
        i++;
    } while (i<width);
}

/* 690c9a20 -- __glSpanUnpackIndexUbyte */
void __fastcall __glSpanUnpackIndexUbyte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                              const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    const GLubyte *userData;
    GLubyte *spanData;
    GLint width;
    GLshort *pixelArray;
    GLint skipCount;

    width = spanInfo->width;
    userData = (const GLubyte *) inspan;
    spanData = (GLubyte *) outspan;
    
    pixelArray = spanInfo->pixelArray;

    i = 0;
    do {
        *spanData = *userData;
        spanData++;

        skipCount = *pixelArray++;
        userData = (const GLubyte *) ((const GLubyte *) userData + skipCount);
        i++;
    } while (i<width);
}

/* 690c9a60 -- __glSpanUnpackRGBAubyte */
void __fastcall __glSpanUnpackRGBAubyte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    const GLubyte *userData;
    GLubyte *spanData;
    GLint width;
    GLshort *pixelArray;
    GLint skipCount;

    width = spanInfo->width;
    userData = (const GLubyte *) inspan;
    spanData = (GLubyte *) outspan;

    pixelArray = spanInfo->pixelArray;

    i = 0;
    do {
        spanData[0] = userData[0];
        spanData[1] = userData[1];
        spanData[2] = userData[2];
        spanData[3] = userData[3];
        spanData += 4;

        skipCount = *pixelArray++;
        userData = (const GLubyte *) ((const GLubyte *) userData + (skipCount << 2));
        i += skipCount;    /* ICD: counts the skipped pixels (690c9a9c) */
    } while (i<width);
}

/* 690c9ab0 -- __glSpanSwapBytes2 */
void __fastcall __glSpanSwapBytes2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                        const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        a = inData[0];
        b = inData[1];
        outData[0] = b;
        outData[1] = a;
        outData += 2;
        inData += 2;
    }
}

/* 690c9af0 -- __glSpanSwapBytes2Dst */
void __fastcall __glSpanSwapBytes2Dst(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                           const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        a = inData[0];
        b = inData[1];
        outData[0] = b;
        outData[1] = a;
        outData += 2;
        inData += 2;
    }
}

/* 690c9b30 -- __glSpanSwapAndSkipBytes2 */
void __fastcall __glSpanSwapAndSkipBytes2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                               const GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

    for (i=0; i<width; i++) {
        for (j=0; j<components; j++) {
            a = inData[0];
            b = inData[1];
            outData[0] = b;
            outData[1] = a;
            outData += 2;
            inData += 2;
        }

        skipCount = (*pixelArray++) - 1;
        inData = (const GLubyte *) ((const GLubyte *) inData + (skipCount * groupInc));
    }
}

/* 690c9ba0 -- __glSpanSwapBytes4 */
void __fastcall __glSpanSwapBytes4(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                        const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b,c,d;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        c = inData[2];
        d = inData[3];
        a = inData[0];
        b = inData[1];
        outData[0] = d;
        outData[1] = c;
        outData[2] = b;
        outData[3] = a;
        outData += 4;
        inData += 4;
    }
}

/* 690c9c00 -- __glSpanSwapBytes4Dst */
void __fastcall __glSpanSwapBytes4Dst(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                           const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b,c,d;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        c = inData[2];
        d = inData[3];
        a = inData[0];
        b = inData[1];
        outData[0] = d;
        outData[1] = c;
        outData[2] = b;
        outData[3] = a;
        outData += 4;
        inData += 4;
    }
}

/* 690c9c60 -- __glSpanSwapAndSkipBytes4 */
void __fastcall __glSpanSwapAndSkipBytes4(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                               const GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b,c,d;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

    for (i=0; i<width; i++) {
        for (j=0; j<components; j++) {
            c = inData[2];
            d = inData[3];
            outData[0] = d;
            outData[1] = c;
            a = inData[0];
            b = inData[1];
            outData[2] = b;
            outData[3] = a;
            outData += 4;
            inData += 4;
        }

        skipCount = (*pixelArray++) - 1;
        inData = (const GLubyte *) ((const GLubyte *) inData + (skipCount * groupInc));
    }
}

/* 690c9ce0 -- __glSpanSkipPixels1 */
void __fastcall __glSpanSkipPixels1(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

    for (i=0; i<width; i++) {
        for (j=0; j<components; j++) {
            *outData++ = *inData++;
        }

        skipCount = (*pixelArray++) - 1;
        inData = (const GLubyte *) ((const GLubyte *) inData + (skipCount * groupInc));
    }
}

/* 690c9d30 -- __glSpanSkipPixels2 */
void __fastcall __glSpanSkipPixels2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->width;
    const GLushort *inData = (const GLushort *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

    for (i=0; i<width; i++) {
        for (j=0; j<components; j++) {
            *outData++ = *inData++;
        }

        skipCount = (*pixelArray++) - 1;
        inData = (const GLushort *) ((const GLubyte *) inData + (skipCount * groupInc));
    }
}

/* 690c9d90 -- __glSpanSkipPixels4 */
void __fastcall __glSpanSkipPixels4(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

    for (i=0; i<width; i++) {
        for (j=0; j<components; j++) {
            *outData++ = *inData++;
        }

        skipCount = (*pixelArray++) - 1;
        inData = (const GLuint *) ((const GLubyte *) inData + (skipCount * groupInc));
    }
}

/* 690c9df0 -- __glSpanSlowSkipPixels2 */
void __fastcall __glSpanSlowSkipPixels2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

    for (i=0; i<width; i++) {
        for (j=0; j<components; j++) {
            a = inData[0];
            b = inData[1];
            outData[0] = a;
            outData[1] = b;
            outData += 2;
            inData += 2;
        }

        skipCount = (*pixelArray++) - 1;
        inData = ((const GLubyte *) inData + (skipCount * groupInc));
    }
}

/* 690c9e60 -- __glSpanSlowSkipPixels4 */
void __fastcall __glSpanSlowSkipPixels4(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b,c,d;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

    for (i=0; i<width; i++) {
        for (j=0; j<components; j++) {
            a = inData[0];
            b = inData[1];
            c = inData[2];
            d = inData[3];
            outData[0] = a;
            outData[1] = b;
            outData[2] = c;
            outData[3] = d;
            outData += 4;
            inData += 4;
        }

        skipCount = (*pixelArray++) - 1;
        inData = ((const GLubyte *) inData + (skipCount * groupInc));
    }
}

/* 690c9ef0 -- __glSpanAlignPixels2 */
void __fastcall __glSpanAlignPixels2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                          const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;


    for (i=0; i<totalSize; i++) {
        a = inData[0];
        b = inData[1];
        outData[0] = a;
        outData[1] = b;
        outData += 2;
        inData += 2;
    }
}

/* 690c9f30 -- __glSpanAlignPixels2Dst */
void __fastcall __glSpanAlignPixels2Dst(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;


    for (i=0; i<totalSize; i++) {
        a = inData[0];
        b = inData[1];
        outData[0] = a;
        outData[1] = b;
        outData += 2;
        inData += 2;
    }
}

/* 690c9f70 -- __glSpanAlignPixels4 */
void __fastcall __glSpanAlignPixels4(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                          const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b,c,d;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;


    for (i=0; i<totalSize; i++) {
        a = inData[0];
        b = inData[1];
        c = inData[2];
        d = inData[3];
        outData[0] = a;
        outData[1] = b;
        outData[2] = c;
        outData[3] = d;
        outData += 4;
        inData += 4;
    }
}

/* 690c9fd0 -- __glSpanAlignPixels4Dst */
void __fastcall __glSpanAlignPixels4Dst(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b,c,d;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;


    for (i=0; i<totalSize; i++) {
        a = inData[0];
        b = inData[1];
        c = inData[2];
        d = inData[3];
        outData[0] = a;
        outData[1] = b;
        outData[2] = c;
        outData[3] = d;
        outData += 4;
        inData += 4;
    }
}

/* 690ca030 -- __glSpanUnpackUbyte */
void __fastcall __glSpanUnpackUbyte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = __GL_UB_TO_FLOAT(*inData++);
    }
}

/* 690ca070 -- __glSpanUnpackByte */
void __fastcall __glSpanUnpackByte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                        const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLbyte *inData = (const GLbyte *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = __GL_B_TO_FLOAT(*inData++);
    }
}

/* 690ca0b0 -- __glSpanUnpackUshort */
void __fastcall __glSpanUnpackUshort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                          const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLushort *inData = (const GLushort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = __GL_US_TO_FLOAT(*inData++);
    }
}

/* 690ca0f0 -- __glSpanUnpackShort */
void __fastcall __glSpanUnpackShort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLshort *inData = (const GLshort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = __GL_S_TO_FLOAT(*inData++);
    }
}

/* 690ca130 -- __glSpanUnpackUint */
void __fastcall __glSpanUnpackUint(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                        const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = __GL_UI_TO_FLOAT(*inData++);
    }
}

/* 690ca180 -- __glSpanUnpackInt */
void __fastcall __glSpanUnpackInt(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLint *inData = (const GLint *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = __GL_I_TO_FLOAT(*inData++);
    }
}

/* 690ca1c0 -- __glSpanUnpack332Ubyte */
void __fastcall __glSpanUnpack332Ubyte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per byte */
    GLubyte inbyte;

    for (i=0; i<totalSize; i++) {
        inbyte = (*inData++);
        *outData++ = (float)((inbyte & 0xE0) >> 5) / 7.0;
        *outData++ = (float)((inbyte & 0x1C) >> 2) / 7.0;         /* 7 = 2^3-1 */
        *outData++ = (float)(inbyte & 0x03) / 3.0;                 /* 3 = 2^2-1 */
    }
}

/* 690ca240 -- __glSpanUnpack4444Ushort */
void __fastcall __glSpanUnpack4444Ushort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                              const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLushort *inData = (const GLushort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLushort invalue;

    for (i=0; i<totalSize; i++) {
        invalue = (*inData++);
        *outData++ = (float)((invalue & 0xF000) >> 12) / 15.0;  /* 15 = 2^4-1 */
        *outData++ = (float)((invalue & 0x0F00) >> 8) / 15.0;
        *outData++ = (float)((invalue & 0x00F0) >> 4) / 15.0;
        *outData++ = (float)(invalue & 0x000F) / 15.0;
    }
}

/* 690ca2e0 -- __glSpanUnpackARGB4444Ushort */
void __fastcall __glSpanUnpackARGB4444Ushort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                  const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLushort *inData = (const GLushort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLushort invalue;

    for (i=0; i<totalSize; i++) {
        invalue = (*inData++);
        *outData++ = (float)((invalue & 0x0F00) >> 8) / 15.0;
        *outData++ = (float)((invalue & 0x00F0) >> 4) / 15.0;
        *outData++ = (float)(invalue & 0x000F) / 15.0;
        *outData++ = (float)((invalue & 0xF000) >> 12) / 15.0;  /* 15 = 2^4-1 */
    }
}

/* 690ca370 -- __glSpanUnpack5551Ushort */
void __fastcall __glSpanUnpack5551Ushort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                              const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLushort *inData = (const GLushort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLushort invalue;

    for (i=0; i<totalSize; i++) {
        invalue = (*inData++);
        *outData++ = (float)((invalue & 0xF800) >> 11) / 31.0;  /* 31 = 2^5-1 */
        *outData++ = (float)((invalue & 0x07C0) >> 6) / 31.0;
        *outData++ = (float)((invalue & 0x003E) >> 1) / 31.0;
        *outData++ = (float)(invalue & 0x0001);
    }
}

/* 690ca400 -- __glSpanUnpack8888Uint */
void __fastcall __glSpanUnpack8888Uint(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLuint invalue;

    for (i=0; i<totalSize; i++) {
        invalue = (*inData++); 
                                                        /* 255 = 2^8-1 */
        *outData++ = (float)((invalue & 0xFF000000) >> 24) / 255.0;
        *outData++ = (float)((invalue & 0x00FF0000) >> 16) / 255.0;
        *outData++ = (float)((invalue & 0x0000FF00) >> 8) / 255.0;
        *outData++ = (float)(invalue & 0x000000FF) / 255.0;
    }
}

/* 690ca4c0 -- __glSpanUnpack8888RevUint */
void __fastcall __glSpanUnpack8888RevUint(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                               const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLuint invalue;

    for (i=0; i<totalSize; i++) {
        invalue = (*inData++); 
                                                        /* 255 = 2^8-1 */
        *outData++ = (float)((invalue & 0x00FF0000) >> 16) / 255.0;    /* ICD: BGRA order, red from bits 16..23 (690ca4ea) */
        *outData++ = (float)((invalue & 0x0000FF00) >>  8) / 255.0;
        *outData++ = (float)((invalue & 0x000000FF)      ) / 255.0;
        *outData++ = (float)((invalue & 0xFF000000) >> 24) / 255.0;
    }
}

/* 690ca580 -- __glSpanUnpack_10_10_10_2_Uint */
void __fastcall __glSpanUnpack_10_10_10_2_Uint(__GLcontext *gc, 
                                    __GLpixelSpanInfoICD *spanInfo, 
                                    const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLuint invalue;

    for (i=0; i<totalSize; i++) {
        invalue = (*inData++); 
                                                        /* 1023 = 2^10-1 */
        *outData++ = (float)((invalue & 0xFFC00000) >> 22) / 1023.0;
        *outData++ = (float)((invalue & 0x003FF000) >> 12) / 1023.0;
        *outData++ = (float)((invalue & 0x00000FFC) >> 2) / 1023.0;
        *outData++ = (float)(invalue & 0x00000003) / 3.0;
    }
}

/* 690ca640 -- __glSpanUnpackX555Ushort */
void __fastcall __glSpanUnpackX555Ushort(__GLcontext *gc, 
                              __GLpixelSpanInfoICD *spanInfo, 
                              const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLushort *inData = (const GLushort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLushort invalue;

    for (i=0; i<totalSize; i++) {
        invalue = (*inData++); 
                                                        /* 31 = 2^5-1 */
        *outData++ = (float)((invalue & 0x7C00) >> 10) / 31.0;
        *outData++ = (float)((invalue & 0x03E0) >> 5) / 31.0;
        *outData++ = (float)(invalue & 0x001F) / 31.0;
    }
}

/* 690ca6c0 -- __glSpanUnpack1555Ushort */
void __fastcall __glSpanUnpack1555Ushort(__GLcontext *gc, 
                              __GLpixelSpanInfoICD *spanInfo, 
                              const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLushort *inData = (const GLushort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLushort invalue;

    for (i=0; i<totalSize; i++) {
        invalue = (*inData++); 
                                                        /* 31 = 2^5-1 */
        *outData++ = (float)((invalue & 0x7C00) >> 10) / 31.0;
        *outData++ = (float)((invalue & 0x03E0) >> 5) / 31.0;
        *outData++ = (float)(invalue & 0x001F) / 31.0;
        *outData++ = (float)((invalue & 0x8000) >> 15);
    }
}

/* 690ca750 -- __glSpanUnpack565Ushort */
void __fastcall __glSpanUnpack565Ushort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLushort *inData = (const GLushort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per unit */
    GLushort invalue;

    for (i=0; i<totalSize; i++) {
        invalue = (*inData++);
        *outData++ = (float)((invalue & 0xF800) >> 11) / 31.0; /* 31 = 2^5-1 */
        *outData++ = (float)((invalue & 0x07E0) >> 5) / 63.0; /* 63 = 2^6-1 */
        *outData++ = (float)(invalue & 0x001F) / 31.0;                 
    }
}

/* 690ca7d0 -- __glSpanUnpackUbyteI */
void __fastcall __glSpanUnpackUbyteI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                          const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLfloat *outData = (GLfloat *) outspan;

    for (i=0; i<totalSize; i++) {
        *outData++ = *inData++;
    }
}

/* 690ca800 -- __glSpanUnpackByteI */
void __fastcall __glSpanUnpackByteI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->width;
    const GLbyte *inData = (const GLbyte *) inspan;
    GLfloat *outData = (GLfloat *) outspan;

    for (i=0; i<totalSize; i++) {
        *outData++ = *inData++;
    }
}

/* 690ca830 -- __glSpanUnpackUshortI */
void __fastcall __glSpanUnpackUshortI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                           const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->width;
    const GLushort *inData = (const GLushort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;

    for (i=0; i<totalSize; i++) {
        *outData++ = *inData++;
    }
}

/* 690ca860 -- __glSpanUnpackShortI */
void __fastcall __glSpanUnpackShortI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                          const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->width;
    const GLshort *inData = (const GLshort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;

    for (i=0; i<totalSize; i++) {
        *outData++ = *inData++;
    }
}

/* 690ca890 -- __glSpanUnpackUintI */
void __fastcall __glSpanUnpackUintI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLfloat *outData = (GLfloat *) outspan;

    for (i=0; i<totalSize; i++) {
        *outData++ = *inData++;
    }
}

/* 690ca8d0 -- __glSpanUnpackIntI */
void __fastcall __glSpanUnpackIntI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                        const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->width;
    const GLint *inData = (const GLint *) inspan;
    GLfloat *outData = (GLfloat *) outspan;

    for (i=0; i<totalSize; i++) {
        *outData++ = *inData++;
    }
}

/* 690ca900 -- __glSpanUnpackNonCompUint */
void __fastcall __glSpanUnpackNonCompUint(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                               const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLuint *inData = (const GLuint *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = (GLfloat)(*inData++);
    }
}

/* 690ca940 -- __glSpanClampFloat */
void __fastcall __glSpanClampFloat(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                           const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat r, one, zero;

    one = __GL_ONE_F;
    zero = __GL_ZERO_F;
    for (i=0; i<totalSize; i++) {
        r = *inData++;
        if (r > one) r = one;
        else if (r < zero) r = zero;
        *outData++ = r;
    }
}

/* 690ca9a0 -- __glSpanClampFloatDst */
void __fastcall __glSpanClampFloatDst(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                           const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat r, one, zero;

    one = __GL_ONE_F;
    zero = __GL_ZERO_F;
    for (i=0; i<totalSize; i++) {
        r = *inData++;
        if (r > one) r = one;
        else if (r < zero) r = zero;
        *outData++ = r;
    }
}

/* 690caa00 -- __glSpanClampRed */
void __fastcall __glSpanClampRed(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                      const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat r, rs, zero;

    rs = gc->frontBuffer.redScale;
    zero = __GL_ZERO_F;
    for (i=0; i<width; i++) {
        r = *inData++;
        if (r > rs) r = rs; /* assumes red scale is positive */
        else if (r < zero) r = zero;
        *outData++ = r;
        *outData++ = *inData++;
        *outData++ = *inData++;
        *outData++ = *inData++;
    }
}

/* 690caa80 -- __glSpanClampGreen */
void __fastcall __glSpanClampGreen(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                        const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat g, gs, zero;

    gs = gc->frontBuffer.greenScale;
    zero = __GL_ZERO_F;
    for (i=0; i<width; i++) {
        *outData++ = *inData++;
        g = *inData++;
        if (g > gs) g = gs; /* assumes green scale is positive */
        else if (g < zero) g = zero;
        *outData++ = g;
        *outData++ = *inData++;
        *outData++ = *inData++;
    }
}

/* 690cab00 -- __glSpanClampBlue */
void __fastcall __glSpanClampBlue(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat b, bs, zero;

    bs = gc->frontBuffer.blueScale;
    zero = __GL_ZERO_F;
    for (i=0; i<width; i++) {
        *outData++ = *inData++;
        *outData++ = *inData++;
        b = *inData++;
        if (b > bs) b = bs; /* assumes blue scale is positive */
        else if (b < zero) b = zero;
        *outData++ = b;
        *outData++ = *inData++;
    }
}

/* 690cab80 -- __glSpanClampAlpha */
void __fastcall __glSpanClampAlpha(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                        const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat a, as, zero;

    as = gc->frontBuffer.alphaScale;
    zero = __GL_ZERO_F;
    for (i=0; i<width; i++) {
        *outData++ = *inData++;
        *outData++ = *inData++;
        *outData++ = *inData++;
        a = *inData++;
        if (a > as) a = as; /* assumes alpha scale is positive */
        else if (a < zero) a = zero;
        *outData++ = a;
    }
}

/* 690cac00 -- __glSpanClampRGB */
void __fastcall __glSpanClampRGB(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                      const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat r, g, b, rs, gs, bs, zero;

    rs = gc->frontBuffer.redScale;
    gs = gc->frontBuffer.greenScale;
    bs = gc->frontBuffer.blueScale;

    zero = __GL_ZERO_F;
    for (i=0; i<width; i++) {
        r = *inData++;
        if (r > rs) r = rs; /* assumes red scale is positive */
        else if (r < zero) r = zero;
        *outData++ = r;

        g = *inData++;
        if (g > gs) g = gs; /* assumes green scale is positive */
        else if (g < zero) g = zero;
        *outData++ = g;

        b = *inData++;
        if (b > bs) b = bs; /* assumes blue scale is positive */
        else if (b < zero) b = zero;
        *outData++ = b;
        *outData++ = *inData++;
    }
}

/* 690cacf0 -- __glSpanClampRGBA */
void __fastcall __glSpanClampRGBA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData;
    GLfloat *outData;
    GLfloat r, g, b, a, rs, gs, bs, as, zero;

    inData = (const GLfloat *) inspan;
    outData = (GLfloat *) outspan;

    rs = gc->frontBuffer.redScale;
    gs = gc->frontBuffer.greenScale;
    bs = gc->frontBuffer.blueScale;
    as = gc->frontBuffer.alphaScale;

    zero = __GL_ZERO_F;
    for (i=0; i<width; i++) {
        r = *inData++;
        if (r > rs) r = rs; /* assumes red scale is positive */
        else if (r < zero) r = zero;
        *outData++ = r;

        g = *inData++;
        if (g > gs) g = gs; /* assumes green scale is positive */
        else if (g < zero) g = zero;
        *outData++ = g;

        b = *inData++;
        if (b > bs) b = bs; /* assumes blue scale is positive */
        else if (b < zero) b = zero;
        *outData++ = b;

        a = *inData++;
        if (a > as) a = as; /* assumes alpha scale is positive */
        else if (a < zero) a = zero;
        *outData++ = a;
    }
}

/* 690cae10 -- __glSpanClampSignedDst */
void __fastcall __glSpanClampSignedDst(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat r, zero;

    zero = __GL_ZERO_F;
    for (i=0; i<totalSize; i++) {
        r = *inData++;
        if (r < zero) r = zero;
        *outData++ = r;
    }
}

/* 690cae50 -- __glSpanExpandRed */
void __fastcall __glSpanExpandRed(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat zero = __GL_ZERO_F;
    GLfloat as = gc->frontBuffer.alphaScale;
    GLfloat rs = gc->frontBuffer.redScale;

    for (i=0; i<width; i++) {
        *outData++ = *inData++ * rs;        /* Red */
        *outData++ = zero;                /* Green */
        *outData++ = zero;                /* Blue */
        *outData++ = as;                /* Alpha */
    }
}

/* 690caeb0 -- __glSpanExpandGreen */
void __fastcall __glSpanExpandGreen(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat zero = __GL_ZERO_F;
    GLfloat gs = gc->frontBuffer.greenScale;
    GLfloat as = gc->frontBuffer.alphaScale;

    for (i=0; i<width; i++) {
        *outData++ = zero;                /* Red */
        *outData++ = *inData++ * gs;        /* Green */
        *outData++ = zero;                /* Blue */
        *outData++ = as;                /* Alpha */
    }
}

/* 690caf10 -- __glSpanExpandBlue */
void __fastcall __glSpanExpandBlue(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                        const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat zero = __GL_ZERO_F;
    GLfloat bs = gc->frontBuffer.blueScale;
    GLfloat as = gc->frontBuffer.alphaScale;

    for (i=0; i<width; i++) {
        *outData++ = zero;                /* Red */
        *outData++ = zero;                /* Green */
        *outData++ = *inData++ * bs;        /* Blue */
        *outData++ = as;                /* Alpha */
    }
}

/* 690caf70 -- __glSpanExpandAlpha */
void __fastcall __glSpanExpandAlpha(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat zero = __GL_ZERO_F;
    GLfloat as = gc->frontBuffer.alphaScale;

    for (i=0; i<width; i++) {
        *outData++ = zero;                /* Red */
        *outData++ = zero;                /* Green */
        *outData++ = zero;                /* Blue */
        *outData++ = *inData++ * as;        /* Alpha */
    }
}

/* 690cafc0 -- __glSpanExpandRGB */
void __fastcall __glSpanExpandRGB(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat r,g,b;
    GLfloat rs,gs,bs;
    GLfloat as = gc->frontBuffer.alphaScale;

    rs = gc->frontBuffer.redScale;
    gs = gc->frontBuffer.greenScale;
    bs = gc->frontBuffer.blueScale;

    for (i=0; i<width; i++) {
        r = *inData++ * rs;
        g = *inData++ * gs;
        b = *inData++ * bs;
        *outData++ = r;
        *outData++ = g;
        *outData++ = b;
        *outData++ = as;                /* Alpha */
    }
}

/* 690cb060 -- __glSpanExpandBGR */
void __fastcall __glSpanExpandBGR(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat r,g,b;
    GLfloat rs,gs,bs;
    GLfloat as = gc->frontBuffer.alphaScale;

    rs = gc->frontBuffer.redScale;
    gs = gc->frontBuffer.greenScale;
    bs = gc->frontBuffer.blueScale;

    for (i=0; i<width; i++) {
        b = *inData++ * bs;
        g = *inData++ * gs;
        r = *inData++ * rs;
        *outData++ = r;
        *outData++ = g;
        *outData++ = b;
        *outData++ = as;                /* Alpha */
    }
}

/* 690cb0f0 -- __glSpanExpandLuminance */
void __fastcall __glSpanExpandLuminance(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat comp;
    GLfloat rs,gs,bs;
    GLfloat as = gc->frontBuffer.alphaScale;

    rs = gc->frontBuffer.redScale;
    gs = gc->frontBuffer.greenScale;
    bs = gc->frontBuffer.blueScale;

    for (i=0; i<width; i++) {
        comp = *inData++;
        *outData++ = comp * rs;                /* Red */
        *outData++ = comp * gs;                /* Green */
        *outData++ = comp * bs;                /* Blue */
        *outData++ = as;                /* Alpha */
    }
}

/* 690cb170 -- __glSpanExpandLuminanceAlpha */
void __fastcall __glSpanExpandLuminanceAlpha(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                  const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat comp;
    GLfloat rs,gs,bs;
    GLfloat as = gc->frontBuffer.alphaScale;

    rs = gc->frontBuffer.redScale;
    gs = gc->frontBuffer.greenScale;
    bs = gc->frontBuffer.blueScale;

    for (i=0; i<width; i++) {
        comp = *inData++;
        *outData++ = comp * rs;                /* Red */
        *outData++ = comp * gs;                /* Green */
        *outData++ = comp * bs;                /* Blue */
        *outData++ = *inData++ * as;        /* Alpha */
    }
}

/* 690cb200 -- __glSpanExpandRedAlpha */
void __fastcall __glSpanExpandRedAlpha(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat comp;
    GLfloat zero;
    GLfloat rs;
    GLfloat as = gc->frontBuffer.alphaScale;

    rs = gc->frontBuffer.redScale;
    zero = __GL_ZERO_F;

    for (i=0; i<width; i++) {
        comp = *inData++;
        *outData++ = comp * rs;                /* Red */
        *outData++ = zero;
        *outData++ = zero;
        *outData++ = *inData++ * as;        /* Alpha */
    }
}

/* 690cb270 -- __glSpanExpandIntensity */
void __fastcall __glSpanExpandIntensity(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat comp;
    GLfloat rs,gs,bs;
    GLfloat as = gc->frontBuffer.alphaScale;

    rs = gc->frontBuffer.redScale;
    gs = gc->frontBuffer.greenScale;
    bs = gc->frontBuffer.blueScale;

    for (i=0; i<width; i++) {
        comp = *inData++;
        *outData++ = comp * rs;                /* Red */
        *outData++ = comp * gs;                /* Green */
        *outData++ = comp * bs;                /* Blue */
        *outData++ = comp * as;                /* Alpha */
    }
}

/* 690cb2f0 -- __glSpanExpandRedNS */
void __fastcall __glSpanExpandRedNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat zero = __GL_ZERO_F;

    for (i=0; i<width; i++) {
        *outData++ = *inData++;                /* Red */
        *outData++ = zero;                /* Green */
        *outData++ = zero;                /* Blue */
        *outData++ = 1.0;                /* Alpha */
    }
}

/* 690cb330 -- __glSpanExpandGreenNS */
void __fastcall __glSpanExpandGreenNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                           const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat zero = __GL_ZERO_F;

    for (i=0; i<width; i++) {
        *outData++ = zero;                /* Red */
        *outData++ = *inData++;                /* Green */
        *outData++ = zero;                /* Blue */
        *outData++ = 1.0;                /* Alpha */
    }
}

/* 690cb370 -- __glSpanExpandBlueNS */
void __fastcall __glSpanExpandBlueNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                          const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat zero = __GL_ZERO_F;

    for (i=0; i<width; i++) {
        *outData++ = zero;                /* Red */
        *outData++ = zero;                /* Green */
        *outData++ = *inData++;                /* Blue */
        *outData++ = 1.0;                /* Alpha */
    }
}

/* 690cb3b0 -- __glSpanExpandAlphaNS */
void __fastcall __glSpanExpandAlphaNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                           const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat zero = __GL_ZERO_F;

    for (i=0; i<width; i++) {
        *outData++ = zero;                /* Red */
        *outData++ = zero;                /* Green */
        *outData++ = zero;                /* Blue */
        *outData++ = *inData++;                /* Alpha */
    }
}

/* 690cb3f0 -- __glSpanExpandRGBNS */
void __fastcall __glSpanExpandRGBNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;

    for (i=0; i<width; i++) {
        *outData++ = *inData++;
        *outData++ = *inData++;
        *outData++ = *inData++;
        *outData++ = 1.0;                /* Alpha */
    }
}

/* 690cb440 -- __glSpanExpandBGRNS */
void __fastcall __glSpanExpandBGRNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat r,g,b;

    for (i=0; i<width; i++) {
        b = *inData++;
        g = *inData++;
        r = *inData++;
        *outData++ = r;
        *outData++ = g;
        *outData++ = b;
        *outData++ = 1.0;                /* Alpha */
    }
}

/* 690cb490 -- __glSpanExpandLuminanceNS */
void __fastcall __glSpanExpandLuminanceNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                               const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat comp;

    for (i=0; i<width; i++) {
        comp = *inData++;
        *outData++ = comp;                /* Red */
        *outData++ = comp;                /* Green */
        *outData++ = comp;                /* Blue */
        *outData++ = 1.0;                /* Alpha */
    }
}

/* 690cb4d0 -- __glSpanExpandLuminanceAlphaNS */
void __fastcall __glSpanExpandLuminanceAlphaNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                    const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat comp;

    for (i=0; i<width; i++) {
        comp = *inData++;
        *outData++ = comp;                /* Red */
        *outData++ = comp;                /* Green */
        *outData++ = comp;                /* Blue */
        *outData++ = *inData++;                /* Alpha */
    }
}

/* 690cb510 -- __glSpanExpandRedAlphaNS */
void __fastcall __glSpanExpandRedAlphaNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                              const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat comp;
    GLfloat zero;

    zero = __GL_ZERO_F;

    for (i=0; i<width; i++) {
        comp = *inData++;
        *outData++ = comp;                /* Red */
        *outData++ = zero;
        *outData++ = zero;
        *outData++ = *inData++;                /* Alpha */
    }
}

/* 690cb560 -- __glSpanExpandIntensityNS */
void __fastcall __glSpanExpandIntensityNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                               const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat comp;

    for (i=0; i<width; i++) {
        comp = *inData++;
        *outData++ = comp;                /* Red */
        *outData++ = comp;                /* Green */
        *outData++ = comp;                /* Blue */
        *outData++ = comp;                /* Alpha */
    }
}

/* 690cb5f0 -- __glSpanScaleRGBA */
void __fastcall __glSpanScaleRGBA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i, width;
    GLfloat rscale, gscale, bscale, ascale;
    GLfloat red, green, blue, alpha;
    const GLfloat *inData;
    GLfloat *outData;

    width = spanInfo->width;
    inData = (const GLfloat *) inspan;
    outData = (GLfloat *) outspan;

    rscale = gc->frontBuffer.redScale;
    gscale = gc->frontBuffer.greenScale;
    bscale = gc->frontBuffer.blueScale;
    ascale = gc->frontBuffer.alphaScale;
    for (i=0; i<width; i++) {
        red = *inData++ * rscale;
        green = *inData++ * gscale;
        *outData++ = red;
        *outData++ = green;
        blue = *inData++ * bscale;
        alpha = *inData++ * ascale;
        *outData++ = blue;
        *outData++ = alpha;
    }
}

/* 690cb690 -- __glSpanScaleABGR */
void __fastcall __glSpanScaleABGR(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i, width;
    GLfloat rscale, gscale, bscale, ascale;
    GLfloat red, green, blue, alpha;
    const GLfloat *inData;
    GLfloat *outData;

    width = spanInfo->width;
    inData = (const GLfloat *) inspan;
    outData = (GLfloat *) outspan;

    rscale = gc->frontBuffer.redScale;
    gscale = gc->frontBuffer.greenScale;
    bscale = gc->frontBuffer.blueScale;
    ascale = gc->frontBuffer.alphaScale;
    for (i=0; i<width; i++) {
        alpha = *inData++ * ascale;
        blue = *inData++ * bscale;
        green = *inData++ * gscale;
        red = *inData++ * rscale;
        *outData++ = red;
        *outData++ = green;
        *outData++ = blue;
        *outData++ = alpha;
    }
}

/* 690cb720 -- __glSpanScaleBGRA */
void __fastcall __glSpanScaleBGRA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i, width;
    GLfloat rscale, gscale, bscale, ascale;
    GLfloat red, green, blue, alpha;
    const GLfloat *inData;
    GLfloat *outData;

    width = spanInfo->width;
    inData = (const GLfloat *) inspan;
    outData = (GLfloat *) outspan;

    rscale = gc->frontBuffer.redScale;
    gscale = gc->frontBuffer.greenScale;
    bscale = gc->frontBuffer.blueScale;
    ascale = gc->frontBuffer.alphaScale;
    for (i=0; i<width; i++) {
        blue = *inData++ * bscale;
        green = *inData++ * gscale;
        red = *inData++ * rscale;
        alpha = *inData++ * ascale;
        *outData++ = red;
        *outData++ = green;
        *outData++ = blue;
        *outData++ = alpha;
    }
}

/* 690cb7b0 -- __glSpanPreReorderABGR */
void __fastcall __glSpanPreReorderABGR(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat red, green, blue, alpha;

    for (i=0; i<width; i++) {
        alpha = *inData++;
        blue = *inData++;
        green = *inData++;
        red = *inData++;
        *outData++ = red;
        *outData++ = green;
        *outData++ = blue;
        *outData++ = alpha;
    }
}

/* 690cb800 -- __glSpanPreReorderBGRA */
void __fastcall __glSpanPreReorderBGRA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat red, green, blue, alpha;

    for (i=0; i<width; i++) {
        blue = *inData++;
        green = *inData++;
        red = *inData++;
        alpha = *inData++;
        *outData++ = red;
        *outData++ = green;
        *outData++ = blue;
        *outData++ = alpha;
    }
}

/* 690cb850 -- __glSpanPreUnscaleRGBA */
void __fastcall __glSpanPreUnscaleRGBA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i, width;
    GLfloat rscale, gscale, bscale, ascale;
    GLfloat red, green, blue, alpha;
    const GLfloat *inData;
    GLfloat *outData;

    width = spanInfo->width;
    inData = (const GLfloat *) inspan;
    outData = (GLfloat *) outspan;

    rscale = gc->frontBuffer.oneOverRedScale;
    gscale = gc->frontBuffer.oneOverGreenScale;
    bscale = gc->frontBuffer.oneOverBlueScale;
    ascale = gc->frontBuffer.oneOverAlphaScale;
    for (i=0; i<width; i++) {
        red = *inData++ * rscale;
        green = *inData++ * gscale;
        *outData++ = red;
        *outData++ = green;
        blue = *inData++ * bscale;
        alpha = *inData++ * ascale;
        *outData++ = blue;
        *outData++ = alpha;
    }
}

/* 690cb8f0 -- __glSpanPostUnscaleRGBA */
void __fastcall __glSpanPostUnscaleRGBA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i, width;
    GLfloat rscale, gscale, bscale, ascale;
    GLfloat red, green, blue, alpha;
    const GLfloat *inData;
    GLfloat *outData;

    width = spanInfo->width;
    inData = (const GLfloat *) inspan;
    outData = (GLfloat *) outspan;

    rscale = gc->frontBuffer.oneOverRedScale;
    gscale = gc->frontBuffer.oneOverGreenScale;
    bscale = gc->frontBuffer.oneOverBlueScale;
    ascale = gc->frontBuffer.oneOverAlphaScale;
    for (i=0; i<width; i++) {
        red = *inData++ * rscale;
        green = *inData++ * gscale;
        *outData++ = red;
        *outData++ = green;
        blue = *inData++ * bscale;
        alpha = *inData++ * ascale;
        *outData++ = blue;
        *outData++ = alpha;
    }
}

/* 690cb990 -- __glSpanUnscaleABGR */
void __fastcall __glSpanUnscaleABGR(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i, width;
    GLfloat rscale, gscale, bscale, ascale;
    GLfloat red, green, blue, alpha;
    const GLfloat *inData;
    GLfloat *outData;

    width = spanInfo->width;
    inData = (const GLfloat *) inspan;
    outData = (GLfloat *) outspan;

    rscale = gc->frontBuffer.oneOverRedScale;
    gscale = gc->frontBuffer.oneOverGreenScale;
    bscale = gc->frontBuffer.oneOverBlueScale;
    ascale = gc->frontBuffer.oneOverAlphaScale;
    for (i=0; i<width; i++) {
        red = *inData++ * rscale;
        green = *inData++ * gscale;
        blue = *inData++ * bscale;
        alpha = *inData++ * ascale;
        *outData++ = alpha;
        *outData++ = blue;
        *outData++ = green;
        *outData++ = red;
    }
}

/* 690cba20 -- __glSpanUnscaleBGRA */
void __fastcall __glSpanUnscaleBGRA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i, width;
    GLfloat rscale, gscale, bscale, ascale;
    GLfloat red, green, blue, alpha;
    const GLfloat *inData;
    GLfloat *outData;

    width = spanInfo->width;
    inData = (const GLfloat *) inspan;
    outData = (GLfloat *) outspan;

    rscale = gc->frontBuffer.oneOverRedScale;
    gscale = gc->frontBuffer.oneOverGreenScale;
    bscale = gc->frontBuffer.oneOverBlueScale;
    ascale = gc->frontBuffer.oneOverAlphaScale;
    for (i=0; i<width; i++) {
        red = *inData++ * rscale;
        green = *inData++ * gscale;
        blue = *inData++ * bscale;
        alpha = *inData++ * ascale;
        *outData++ = blue;
        *outData++ = green;
        *outData++ = red;
        *outData++ = alpha;
    }
}

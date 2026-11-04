/*
** si_px_pack.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/px_pack.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690c6cf0 -- __glSpanReduceRed */
void __fastcall __glSpanReduceRed(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;

    for (i=0; i<width; i++) {
        *outData++ = *inData * rs;
        inData += 4;
    }
}

/* 690c6d30 -- __glSpanReduceGreen */
void __fastcall __glSpanReduceGreen(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat gs = gc->frontBuffer.oneOverGreenScale;

    inData++;        /* Skip first red */
    for (i=0; i<width; i++) {
        *outData++ = *inData * gs;
        inData += 4;
    }
}

/* 690c6d70 -- __glSpanReduceBlue */
void __fastcall __glSpanReduceBlue(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                        const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat bs = gc->frontBuffer.oneOverBlueScale;

    inData += 2;        /* Skip first red, green */
    for (i=0; i<width; i++) {
        *outData++ = *inData * bs;
        inData += 4;
    }
}

/* 690c6db0 -- __glSpanReduceAlpha */
void __fastcall __glSpanReduceAlpha(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat as = gc->frontBuffer.oneOverAlphaScale;

    inData += 3;        /* Skip first red, green, blue */
    for (i=0; i<width; i++) {
        *outData++ = *inData * as;
        inData += 4;
    }
}

/* 690c6df0 -- __glSpanReduceRGB */
void __fastcall __glSpanReduceRGB(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;
    GLfloat bs = gc->frontBuffer.oneOverBlueScale;
    GLfloat gs = gc->frontBuffer.oneOverGreenScale;
    GLfloat red, green, blue;

    for (i=0; i<width; i++) {
        red = *inData++ * rs;
        green = *inData++ * gs;
        blue = *inData++ * bs;
        *outData++ = red;
        *outData++ = green;
        *outData++ = blue;
        inData++;
    }
}

/* 690c6e70 -- __glSpanReduceBGR */
void __fastcall __glSpanReduceBGR(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;
    GLfloat bs = gc->frontBuffer.oneOverBlueScale;
    GLfloat gs = gc->frontBuffer.oneOverGreenScale;
    GLfloat red, green, blue;

    for (i=0; i<width; i++) {
        red = *inData++ * rs;
        green = *inData++ * gs;
        blue = *inData++ * bs;
        *outData++ = blue;
        *outData++ = green;
        *outData++ = red;
        inData++;
    }
}

/* 690c6ee0 -- __glSpanReorderABGR */
void __fastcall __glSpanReorderABGR(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;
    GLfloat bs = gc->frontBuffer.oneOverBlueScale;
    GLfloat gs = gc->frontBuffer.oneOverGreenScale;
    GLfloat as = gc->frontBuffer.oneOverAlphaScale;
    GLfloat red, green, blue, alpha;

    for (i=0; i<width; i++) {
        red = *inData++ * rs;
        green = *inData++ * gs;
        blue = *inData++ * bs;
        alpha = *inData++ * as;
        *outData++ = alpha;
        *outData++ = blue;
        *outData++ = green;
        *outData++ = red;
    }
}

/* 690c6f70 -- __glSpanReorderBGRA */
void __fastcall __glSpanReorderBGRA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;
    GLfloat bs = gc->frontBuffer.oneOverBlueScale;
    GLfloat gs = gc->frontBuffer.oneOverGreenScale;
    GLfloat as = gc->frontBuffer.oneOverAlphaScale;
    GLfloat red, green, blue, alpha;

    for (i=0; i<width; i++) {
        red = *inData++ * rs;
        green = *inData++ * gs;
        blue = *inData++ * bs;
        alpha = *inData++ * as;
        *outData++ = blue;
        *outData++ = green;
        *outData++ = red;
        *outData++ = alpha;
    }
}

/* 690c7000 -- __glSpanReduceLuminance */
void __fastcall __glSpanReduceLuminance(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat l, one;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;
    GLfloat bs = gc->frontBuffer.oneOverBlueScale;
    GLfloat gs = gc->frontBuffer.oneOverGreenScale;

    one = __GL_ONE_F;

    for (i=0; i<width; i++) {
        l = inData[0] * rs + inData[1] * gs + inData[2] * bs;
        if (l > one) l = one;
        *outData++ = l;
        inData += 4;
    }
}

/* 690c7080 -- __glSpanReduceLuminanceAlpha */
void __fastcall __glSpanReduceLuminanceAlpha(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                  const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat l, one;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;
    GLfloat bs = gc->frontBuffer.oneOverBlueScale;
    GLfloat gs = gc->frontBuffer.oneOverGreenScale;
    GLfloat as = gc->frontBuffer.oneOverAlphaScale;

    one = __GL_ONE_F;

    for (i=0; i<width; i++) {
        l = inData[0] * rs + inData[1] * gs + inData[2] * bs;
        if (l > one) l = one;
        *outData++ = l; 
        inData += 3;
        *outData++ = *inData++ * as;
    }
}

/* 690c7120 -- __glSpanReduceRedAlpha */
void __fastcall __glSpanReduceRedAlpha(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;
    GLfloat as = gc->frontBuffer.oneOverAlphaScale;

    for (i=0; i<width; i++) {
        *outData++ = *inData++ * rs;
        inData += 2;
        *outData++ = *inData++ * as;
    }
}

/* 690c7170 -- __glSpanReduceRedNS */
void __fastcall __glSpanReduceRedNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;

    for (i=0; i<width; i++) {
        *outData++ = *inData;
        inData += 4;
    }
}

/* 690c71a0 -- __glSpanReduceGreenNS */
void __fastcall __glSpanReduceGreenNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                           const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;

    inData++;        /* Skip first red */
    for (i=0; i<width; i++) {
        *outData++ = *inData;
        inData += 4;
    }
}

/* 690c71d0 -- __glSpanReduceBlueNS */
void __fastcall __glSpanReduceBlueNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                          const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;

    inData += 2;        /* Skip first red, green */
    for (i=0; i<width; i++) {
        *outData++ = *inData;
        inData += 4;
    }
}

/* 690c7200 -- __glSpanReduceAlphaNS */
void __fastcall __glSpanReduceAlphaNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                           const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;

    inData += 3;        /* Skip first red, green, blue */
    for (i=0; i<width; i++) {
        *outData++ = *inData;
        inData += 4;
    }
}

/* 690c7230 -- __glSpanReduceRGBNS */
void __fastcall __glSpanReduceRGBNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat red, green, blue;

    for (i=0; i<width; i++) {
        red = *inData++;
        green = *inData++;
        blue = *inData++;
        *outData++ = red;
        *outData++ = green;
        *outData++ = blue;
        inData++;
    }
}

/* 690c7280 -- __glSpanReduceBGRNS */
void __fastcall __glSpanReduceBGRNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat red, green, blue;

    for (i=0; i<width; i++) {
        red = *inData++;
        green = *inData++;
        blue = *inData++;
        *outData++ = blue;
        *outData++ = green;
        *outData++ = red;
        inData++;
    }
}

/* 690c72c0 -- __glSpanReorderABGRNS */
void __fastcall __glSpanReorderABGRNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                           const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat red, green, blue, alpha;

    for (i=0; i<width; i++) {
        red = *inData++;
        green = *inData++;
        blue = *inData++;
        alpha = *inData++;
        *outData++ = alpha;
        *outData++ = blue;
        *outData++ = green;
        *outData++ = red;
    }
}

/* 690c7310 -- __glSpanReorderBGRANS */
void __fastcall __glSpanReorderBGRANS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                           const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLfloat red, green, blue, alpha;

    for (i=0; i<width; i++) {
        red = *inData++;
        green = *inData++;
        blue = *inData++;
        alpha = *inData++;
        *outData++ = blue;
        *outData++ = green;
        *outData++ = red;
        *outData++ = alpha;
    }
}

/* 690c7360 -- __glSpanReduceRedAlphaNS */
void __fastcall __glSpanReduceRedAlphaNS(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                              const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    GLfloat *outData = (GLfloat *) outspan;
    const GLfloat *inData = (const GLfloat *) inspan;

    for (i=0; i<width; i++) {
        *outData++ = *inData++;
        inData += 2;
        *outData++ = *inData++;
    }
}

/* 690c73e0 -- __glSpanPackUbyte */
void __fastcall __glSpanPackUbyte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        /* ICD: the older SI macro, `* __glHalf` where `+` was meant (original bug kept: halves the value) */
        *outData++ = (GLubyte) __GL_FLOORF((*inData++ * gc->constants.val255) * __glHalf);
    }
}

/* 690c75d0 -- __glSpanPack332Ubyte */
void __fastcall __glSpanPack332Ubyte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                          const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per byte */
    GLfloat invalue;

    for (i=0; i<totalSize; i++) {
        invalue = *inData++;
        *outData = ((GLubyte)(invalue * 7) <<5) & 0xE0;
        invalue = *inData++;
        *outData |= ((GLubyte)(invalue * 7) <<2) & 0x1C;
        invalue = *inData++;
        *outData |= ((GLubyte)(invalue * 3)) & 0x03;
        outData++;
    }
}

/* 690c7640 -- __glSpanPack4444Ushort */
void __fastcall __glSpanPack4444Ushort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLfloat invalue;

    for (i=0; i<totalSize; i++) {
        invalue = *inData++;
        *outData = ((GLushort)(invalue * 15) <<12) & 0xF000;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 15) <<8) & 0x0F00;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 15) <<4) & 0x00F0;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 15)) & 0x000F;
        outData++;
    }
}

/* 690c76d0 -- __glSpanPackARGB4444Ushort */
void __fastcall __glSpanPackARGB4444Ushort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                                const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLfloat invalue;

    for (i=0; i<totalSize; i++) {
        invalue = *inData++;
        *outData = ((GLushort)(invalue * 15) <<8) & 0x0F00;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 15) <<4) & 0x00F0;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 15)) & 0x000F;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 15) <<12) & 0xF000;
        outData++;
    }
}

/* 690c7760 -- __glSpanPack5551Ushort */
void __fastcall __glSpanPack5551Ushort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLfloat invalue;

    for (i=0; i<totalSize; i++) {
        invalue = *inData++;
        *outData = ((GLushort)(invalue * 31) <<11) & 0xF800;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 31) <<6) & 0x07C0;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 31) <<1) & 0x003E;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue)) & 0x0001;
        outData++;
    }
}

/* 690c77e0 -- __glSpanPack8888Uint */
void __fastcall __glSpanPack8888Uint(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                          const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLuint *outData = (GLuint *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLfloat invalue;

    for (i=0; i<totalSize; i++) {
        invalue = *inData++;
        *outData = ((GLuint)(invalue * 255) <<24) & 0xFF000000;
        invalue = *inData++;
        *outData |= ((GLuint)(invalue * 255) <<16) & 0x00FF0000;
        invalue = *inData++;
        *outData |= ((GLuint)(invalue * 255) <<8) & 0x0000FF00;
        invalue = *inData++;
        *outData |= ((GLuint)(invalue * 255)) & 0x000000FF;
        outData++;
    }
}

/* 690c7870 -- __glSpanPack8888RevUint */
void __fastcall __glSpanPack8888RevUint(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLuint *outData = (GLuint *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLfloat invalue;

    for (i=0; i<totalSize; i++) {
        invalue = *inData++;
        *outData = ((GLuint)(invalue * 255) << 16) & 0x00FF0000;    /* ICD: BGRA order (690c789d) */
        invalue = *inData++;
        *outData |= ((GLuint)(invalue * 255) <<  8) & 0x0000FF00;
        invalue = *inData++;
        *outData |= ((GLuint)(invalue * 255)       ) & 0x000000FF;
        invalue = *inData++;
        *outData |= ((GLuint)(invalue * 255) << 24) & 0xFF000000;
        outData++;
    }
}

/* 690c7980 -- __glSpanPack_10_10_10_2_Uint */
void __fastcall __glSpanPack_10_10_10_2_Uint(__GLcontext *gc, 
                                  __GLpixelSpanInfoICD *spanInfo, 
                                  const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLuint *outData = (GLuint *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLfloat invalue;

    for (i=0; i<totalSize; i++) {
        invalue = *inData++;
        *outData = ((GLuint)(invalue * 1023) <<22) & 0xFFC00000;
        invalue = *inData++;
        *outData |= ((GLuint)(invalue * 1023) <<12) & 0x003FF000;
        invalue = *inData++;
        *outData |= ((GLuint)(invalue * 1023) <<2) & 0x00000FFC;
        invalue = *inData++;
        *outData |= ((GLuint)(invalue * 3)) & 0x00000003;
        outData++;
    }
}

/* 690c7a10 -- __glSpanPackX555Ushort */
void __fastcall __glSpanPackX555Ushort(__GLcontext *gc, 
                            __GLpixelSpanInfoICD *spanInfo, 
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLfloat invalue;

    for (i=0; i<totalSize; i++) {
        *outData = 0x8000;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 31) <<10) & 0x7C00;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 31) <<5) & 0x03E0;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 31)) & 0x001F;
        outData++;
    }
}

/* 690c7a90 -- __glSpanPack1555Ushort */
void __fastcall __glSpanPack1555Ushort(__GLcontext *gc, 
                            __GLpixelSpanInfoICD *spanInfo, 
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per type unit */
    GLfloat invalue;

    for (i=0; i<totalSize; i++) {
        invalue = *inData++;
        *outData = ((GLushort)(invalue * 31) <<10) & 0x7C00;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 31) <<5) & 0x03E0;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 31)) & 0x001F;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue) <<15) & 0x8000;
        outData++;
    }
}

/* 690c7b80 -- __glSpanPackUbyteI */
void __fastcall __glSpanPackUbyteI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                        const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLubyte *outData = (GLubyte *) outspan;

    for (i=0; i<totalSize; i++) {
        *outData++ = ((GLint) *inData++) & 0xff;
    }
}

/* 690c7bb0 -- __glSpanPackByteI */
void __fastcall __glSpanPackByteI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLbyte *outData = (GLbyte *) outspan;

    for (i=0; i<totalSize; i++) {
        *outData++ = ((GLint) *inData++) & 0x7f;
    }
}

/* 690c7be0 -- __glSpanPackUshortI */
void __fastcall __glSpanPackUshortI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                         const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLushort *outData = (GLushort *) outspan;

    for (i=0; i<totalSize; i++) {
        *outData++ = ((GLint) *inData++) & 0xffff;
    }
}

/* 690c7c10 -- __glSpanPackShortI */
void __fastcall __glSpanPackShortI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                        const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLshort *outData = (GLshort *) outspan;

    for (i=0; i<totalSize; i++) {
        *outData++ = ((GLint) *inData++) & 0x7fff;
    }
}

/* 690c7c50 -- __glSpanPackUintI */
void __fastcall __glSpanPackUintI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLuint *outData = (GLuint *) outspan;

    for (i=0; i<totalSize; i++) {
        *outData++ = (GLuint) *inData++;
    }
}

/* 690c7c80 -- __glSpanPackIntI */
void __fastcall __glSpanPackIntI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                      const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLint *outData = (GLint *) outspan;

    for (i=0; i<totalSize; i++) {
        *outData++ = ((GLint) *inData++) & 0x7fffffff;
    }
}

/* 690c7cc0 -- __glSpanPackNonCompUbyte */
void __fastcall __glSpanPackNonCompUbyte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                              const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = (GLubyte)(*inData++);
    }
}

/* 690c7d00 -- __glSpanPackNonCompByte */
void __fastcall __glSpanPackNonCompByte(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLbyte *outData = (GLbyte *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = (GLbyte)(*inData++);
    }
}

/* 690c7d40 -- __glSpanPackNonCompUshort */
void __fastcall __glSpanPackNonCompUshort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                               const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = (GLushort)(*inData++);
    }
}

/* 690c7d80 -- __glSpanPackNonCompShort */
void __fastcall __glSpanPackNonCompShort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                              const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLshort *outData = (GLshort *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = (GLshort)(*inData++);
    }
}

/* 690c7dc0 -- __glSpanPackNonCompUint */
void __fastcall __glSpanPackNonCompUint(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                             const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLuint *outData = (GLuint *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = (GLuint)(*inData++);
    }
}

/* 690c7e00 -- __glSpanPackNonCompInt */
void __fastcall __glSpanPackNonCompInt(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLint *outData = (GLint *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = (GLint)(*inData++);
    }
}

/* 690c7e70 -- __glSpanPackBitmap */
void __fastcall __glSpanPackBitmap(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                        const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width;
    GLvoid *userData;
    const GLfloat *spanData;
    GLint lsbFirst;
    GLint startBit;
    GLint bit;
    GLubyte ubyte;


    width = spanInfo->width;
    userData = outspan;
    spanData = (const GLfloat *) inspan;

    lsbFirst = spanInfo->dstLsbFirst;
    startBit = spanInfo->dstStartBit;

    i = width;
    bit = startBit;
    ubyte = *(GLubyte *) userData;

    if (lsbFirst) {
        if (bit) {
            switch(bit) {
              case 1:
                if (((GLint) *spanData++) & 1) ubyte |= 0x02;
                else ubyte &= ~0x02;
                if (--i == 0) break;
              case 2:
                if (((GLint) *spanData++) & 1) ubyte |= 0x04;
                else ubyte &= ~0x04;
                if (--i == 0) break;
              case 3:
                if (((GLint) *spanData++) & 1) ubyte |= 0x08;
                else ubyte &= ~0x08;
                if (--i == 0) break;
              case 4:
                if (((GLint) *spanData++) & 1) ubyte |= 0x10;
                else ubyte &= ~0x10;
                if (--i == 0) break;
              case 5:
                if (((GLint) *spanData++) & 1) ubyte |= 0x20;
                else ubyte &= ~0x20;
                if (--i == 0) break;
              case 6:
                if (((GLint) *spanData++) & 1) ubyte |= 0x40;
                else ubyte &= ~0x40;
                if (--i == 0) break;
              case 7:
                if (((GLint) *spanData++) & 1) ubyte |= 0x80;
                else ubyte &= ~0x80;
                i--;
            }
            *(GLubyte *) userData = ubyte;
            userData = (GLvoid *) ((GLubyte *) userData + 1);
        }
        while (i >= 8) {
            ubyte = 0;
            i -= 8;
            if (((GLint) *spanData++) & 1) ubyte |= 0x01;
            if (((GLint) *spanData++) & 1) ubyte |= 0x02;
            if (((GLint) *spanData++) & 1) ubyte |= 0x04;
            if (((GLint) *spanData++) & 1) ubyte |= 0x08;
            if (((GLint) *spanData++) & 1) ubyte |= 0x10;
            if (((GLint) *spanData++) & 1) ubyte |= 0x20;
            if (((GLint) *spanData++) & 1) ubyte |= 0x40;
            if (((GLint) *spanData++) & 1) ubyte |= 0x80;
            *(GLubyte *) userData = ubyte;
            userData = (GLvoid *) ((GLubyte *) userData + 1);
        }
        if (i) {
            ubyte = *(GLubyte *) userData;
            switch(i) {
              case 7:
                if (((GLint) *spanData++) & 1) ubyte |= 0x01;
                else ubyte &= ~0x01;
              case 6:
                if (((GLint) *spanData++) & 1) ubyte |= 0x02;
                else ubyte &= ~0x02;
              case 5:
                if (((GLint) *spanData++) & 1) ubyte |= 0x04;
                else ubyte &= ~0x04;
              case 4:
                if (((GLint) *spanData++) & 1) ubyte |= 0x08;
                else ubyte &= ~0x08;
              case 3:
                if (((GLint) *spanData++) & 1) ubyte |= 0x10;
                else ubyte &= ~0x10;
              case 2:
                if (((GLint) *spanData++) & 1) ubyte |= 0x20;
                else ubyte &= ~0x20;
              case 1:
                if (((GLint) *spanData++) & 1) ubyte |= 0x40;
                else ubyte &= ~0x40;
                break;
            }
            *(GLubyte *) userData = ubyte;
        }
    } else {
        if (bit) {
            switch(bit) {
              case 1:
                if (((GLint) *spanData++) & 1) ubyte |= 0x40;
                else ubyte &= ~0x40;
                if (--i == 0) break;
              case 2:
                if (((GLint) *spanData++) & 1) ubyte |= 0x20;
                else ubyte &= ~0x20;
                if (--i == 0) break;
              case 3:
                if (((GLint) *spanData++) & 1) ubyte |= 0x10;
                else ubyte &= ~0x10;
                if (--i == 0) break;
              case 4:
                if (((GLint) *spanData++) & 1) ubyte |= 0x08;
                else ubyte &= ~0x08;
                if (--i == 0) break;
              case 5:
                if (((GLint) *spanData++) & 1) ubyte |= 0x04;
                else ubyte &= ~0x04;
                if (--i == 0) break;
              case 6:
                if (((GLint) *spanData++) & 1) ubyte |= 0x02;
                else ubyte &= ~0x02;
                if (--i == 0) break;
              case 7:
                if (((GLint) *spanData++) & 1) ubyte |= 0x01;
                else ubyte &= ~0x01;
                i--;
            }
            *(GLubyte *) userData = ubyte;
            userData = (GLvoid *) ((GLubyte *) userData + 1);
        }
        while (i >= 8) {
            ubyte = 0;
            i -= 8;
            if (((GLint) *spanData++) & 1) ubyte |= 0x80;
            if (((GLint) *spanData++) & 1) ubyte |= 0x40;
            if (((GLint) *spanData++) & 1) ubyte |= 0x20;
            if (((GLint) *spanData++) & 1) ubyte |= 0x10;
            if (((GLint) *spanData++) & 1) ubyte |= 0x08;
            if (((GLint) *spanData++) & 1) ubyte |= 0x04;
            if (((GLint) *spanData++) & 1) ubyte |= 0x02;
            if (((GLint) *spanData++) & 1) ubyte |= 0x01;
            *(GLubyte *) userData = ubyte;
            userData = (GLvoid *) ((GLubyte *) userData + 1);
        }
        if (i) {
            ubyte = *(GLubyte *) userData;
            switch(i) {
              case 7:
                if (((GLint) *spanData++) & 1) ubyte |= 0x80;
                else ubyte &= ~0x80;
              case 6:
                if (((GLint) *spanData++) & 1) ubyte |= 0x40;
                else ubyte &= ~0x40;
              case 5:
                if (((GLint) *spanData++) & 1) ubyte |= 0x20;
                else ubyte &= ~0x20;
              case 4:
                if (((GLint) *spanData++) & 1) ubyte |= 0x10;
                else ubyte &= ~0x10;
              case 3:
                if (((GLint) *spanData++) & 1) ubyte |= 0x08;
                else ubyte &= ~0x08;
              case 2:
                if (((GLint) *spanData++) & 1) ubyte |= 0x04;
                else ubyte &= ~0x04;
              case 1:
                if (((GLint) *spanData++) & 1) ubyte |= 0x02;
                else ubyte &= ~0x02;
                break;
            }
            *(GLubyte *) userData = ubyte;
        }
    }
}

/* 690c8470 -- __glSpanPackL8ToXRGB1555 */
void __fastcall __glSpanPackL8ToXRGB1555(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                              const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLuint invalue, outvalue;

    for (i=0; i<width; i++, inData++, outData++) {
        invalue = (*inData) >> 3;
        outvalue = 0x8000 | (invalue<<10) | (invalue<<5) | invalue;
        *outData = (GLushort) outvalue;
    }
}

/* 690c84b0 -- __glSpanPackRGB8ToXRGB1555 */
void __fastcall __glSpanPackRGB8ToXRGB1555(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                                const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLuint invalue, outvalue;

    for (i=0; i<width; i++, inData+=3, outData++) {
        invalue = inData[0];
        outvalue = 0x8000 | ((invalue>>3)<<10);
        invalue = inData[1];
        outvalue |= ((invalue>>3)<<5);
        invalue = inData[2];
        outvalue |= invalue>>3;
        *outData = (GLushort) outvalue;
    }
}

/* 690c8510 -- __glSpanPackRGBA8ToXRGB1555 */
void __fastcall __glSpanPackRGBA8ToXRGB1555(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                                 const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLuint invalue, outvalue;

    for (i=0; i<width; i++, inData+=4, outData++) {
        invalue = inData[0];
        outvalue = 0x8000 | ((invalue>>3)<<10);
        invalue = inData[1];
        outvalue |= ((invalue>>3)<<5);
        invalue = inData[2];
        outvalue |= invalue>>3;
        *outData = (GLushort) outvalue;
    }
}

/* 690c8570 -- __glSpanPackL8ToRGB565 */
void __fastcall __glSpanPackL8ToRGB565(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLuint invalue, outvalue;

    for (i=0; i<width; i++, inData++, outData++) {
        invalue = *inData;
        outvalue = ((invalue>>3)<<11) | ((invalue>>2)<<5) | (invalue>>3);
        *outData = (GLushort) outvalue;
    }
}

/* 690c85c0 -- __glSpanPackRGB8ToRGB565 */
void __fastcall __glSpanPackRGB8ToRGB565(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                              const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLuint invalue, outvalue;

    for (i=0; i<width; i++, inData+=3, outData++) {
        invalue = inData[0];
        outvalue = ((invalue>>3)<<11);
        invalue = inData[1];
        outvalue |= ((invalue>>2)<<5);
        invalue = inData[2];
        outvalue |= invalue>>3;
        *outData = (GLushort) outvalue;
    }
}

/* 690c86f0 -- __glSpanPackLA8ToARGB1555 */
void __fastcall __glSpanPackLA8ToARGB1555(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                               const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLuint invalue, outvalue;

    for (i=0; i<width; i++, inData+=2, outData++) {
        invalue = inData[0] >> 3;
        outvalue = (invalue<<10) | (invalue<<5) | invalue;
        invalue = inData[1];
        outvalue |= (invalue >> 7) << 15;
        *outData = (GLushort) outvalue;
    }
}

/* 690c8740 -- __glSpanPackRGBA8ToARGB1555 */
void __fastcall __glSpanPackRGBA8ToARGB1555(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                                 const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLuint invalue, outvalue;

    for (i=0; i<width; i++, inData+=4, outData++) {
        invalue = inData[0];
        outvalue = ((invalue>>3)<<10);
        invalue = inData[1];
        outvalue |= ((invalue>>3)<<5);
        invalue = inData[2];
        outvalue |= invalue>>3;
        invalue = inData[3];
        outvalue |= (invalue >> 7) << 15;
        *outData = (GLushort) outvalue;
    }
}

/* 690c87a0 -- __glSpanPackLA8ToARGB4 */
void __fastcall __glSpanPackLA8ToARGB4(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                            const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLuint invalue, outvalue;

    for (i=0; i<width; i++, inData+=2, outData++) {
        invalue = inData[0] >> 4;
        outvalue = (invalue<<8) | (invalue<<4) | invalue;
        invalue = inData[1];
        outvalue |= (invalue >> 4) << 12;
        *outData = (GLushort) outvalue;
    }
}

/* 690c87f0 -- __glSpanPackRGBA8ToARGB4 */
void __fastcall __glSpanPackRGBA8ToARGB4(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                              const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLubyte *inData = (const GLubyte *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLuint invalue, outvalue;

    for (i=0; i<width; i++, inData+=4, outData++) {
        invalue = inData[0];
        outvalue = ((invalue>>4)<<8);
        invalue = inData[1];
        outvalue |= ((invalue>>4)<<4);
        invalue = inData[2];
        outvalue |= invalue>>4;
        invalue = inData[3];
        outvalue |= (invalue >> 4) << 12;
        *outData = (GLushort) outvalue;
    }
}

/* 690c7450 -- __glSpanPackUshort */
void __fastcall __glSpanPackUshort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                        const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = __GL_FLOAT_TO_US(*inData++);
    }
}

/* 690c7510 -- __glSpanPackUint */
void __fastcall __glSpanPackUint(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                      const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLuint *outData = (GLuint *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = __GL_FLOAT_TO_UI(*inData++);
    }
}

/* 690c7b10 -- __glSpanPack565Ushort */
void __fastcall __glSpanPack565Ushort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                           const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLint totalSize = width; /* one group (i.e. pixel) per unit */
    GLfloat invalue;

    for (i=0; i<totalSize; i++) {
        invalue = *inData++;
        *outData = ((GLushort)(invalue * 31) <<11) & 0xF800;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 63) <<5) & 0x07E0;
        invalue = *inData++;
        *outData |= ((GLushort)(invalue * 31)) & 0x001F;
        outData++;
    }
}

/* 690c74a0 -- __glSpanPackShort */
void __fastcall __glSpanPackShort(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                       const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLshort *outData = (GLshort *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = __GL_FLOAT_TO_S(*inData++);
    }
}

/* 690c7560 -- __glSpanPackInt */
void __fastcall __glSpanPackInt(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                     const GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->width;
    const GLfloat *inData = (const GLfloat *) inspan;
    GLint *outData = (GLint *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    for (i=0; i<totalSize; i++) {
        *outData++ = __GL_FLOAT_TO_I(*inData++);
    }
}

/* 690c7e40 -- __glSpanCopy */
void __fastcall __glSpanCopy(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                  const GLvoid *inspan, GLvoid *outspan)
{
    GLint totalSize = spanInfo->width * spanInfo->srcComponents *
        spanInfo->srcElementSize;

    __GL_MEMCOPY(outspan, inspan, totalSize);
}

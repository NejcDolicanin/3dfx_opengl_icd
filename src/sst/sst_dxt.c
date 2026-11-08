/*
** sst_dxt.c -- the DXT1 / DXT3 / DXT5 codec of the hardware path.  S3TC internal formats are stored as DXT blocks
** for Napalm's native compressed texel formats: uploads of compressed data are copied as they are; uncompressed
** input is encoded here.  The encoder is libtxc_dxtn's (Roland Scheidegger, MIT licence below; Mesa's
** texcompress_s3tc_tmp.h since 2018), the same code MesaFX reaches through dxtn.dll, so the blocks are the ones that
** driver would produce; it costs a few hundred operations per 4x4 block against the FXT1 vector quantiser the
** original ran for every S3TC texture.  The decoder feeds the uncompressed copy the software paths and glGetTexImage
** read.  Texels are ARGB8888 words; blocks are the D3D layout, little-endian.
**
** libtxc_dxtn, Version 1.0, Copyright (C) 2004 Roland Scheidegger  All Rights Reserved.
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
** to permit persons to whom the Software is furnished to do so, subject to the following conditions:
** The above copyright notice and this permission notice shall be included in all copies or substantial portions of
** the Software.
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
** THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
** BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
** OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <stdlib.h>
#include "wglpriv.h"
#include "texcmp.h"

/* the block's texels as r, g, b, a bytes [row][col], edge-replicated past the level's size (1x1 .. 2x2 levels) */
static void dxtFetchBlock(GLubyte blk[4][4][4], const GLuint *src, GLint w, GLint h, GLint x0, GLint y0)
{
    GLint i, j, x, y;
    GLuint c;
    for (i = 0; i < 4; i++) {
        y = y0 + i;
        if (y >= h) y = h - 1;
        for (j = 0; j < 4; j++) {
            x = x0 + j;
            if (x >= w) x = w - 1;
            c = src[y * w + x];
            blk[i][j][0] = (GLubyte)(c >> 16);
            blk[i][j][1] = (GLubyte)(c >> 8);
            blk[i][j][2] = (GLubyte)c;
            blk[i][j][3] = (GLubyte)(c >> 24);
        }
    }
}

/* ---- libtxc_dxtn's encoder (texcompress_s3tc_tmp.h), with the ICD's types; the "type" is the output kind */
#define DXT_TYPE_RGB1   0           /* DXT1 without alpha */
#define DXT_TYPE_RGBA1  1           /* DXT1 with 1-bit alpha (alpha <= ALPHACUT is transparent) */
#define DXT_TYPE_RGBA   2           /* DXT3 / DXT5: the four-colour encoding always */

/* weights used for error function, basically weights (unsquared 2/4/1) according to rgb->luminance conversion */
#define REDWEIGHT 4
#define GREENWEIGHT 16
#define BLUEWEIGHT 1
#define ALPHACUT 127

/* one refinement of the two base colours: move each by the mean residual of the texels it (partly) encodes */
static void fancybasecolorsearch(GLubyte srccolors[4][4][4], GLubyte *bestcolor[2])
{
    GLint i, j, colors, z;
    GLuint pixerror, pixerrorred, pixerrorgreen, pixerrorblue, pixerrorbest;
    GLint colordist, blockerrlin[2][3];
    GLubyte nrcolor[2];
    GLint pixerrorcolorbest[3];
    GLubyte enc = 0;
    GLubyte cv[4][4];
    GLubyte testcolor[2][3];

    if (((bestcolor[0][0] & 0xf8) << 8 | (bestcolor[0][1] & 0xfc) << 3 | bestcolor[0][2] >> 3) <
        ((bestcolor[1][0] & 0xf8) << 8 | (bestcolor[1][1] & 0xfc) << 3 | bestcolor[1][2] >> 3)) {
        for (i = 0; i < 3; i++) { testcolor[0][i] = bestcolor[0][i]; testcolor[1][i] = bestcolor[1][i]; }
    } else {
        for (i = 0; i < 3; i++) { testcolor[1][i] = bestcolor[0][i]; testcolor[0][i] = bestcolor[1][i]; }
    }
    for (i = 0; i < 3; i++) {
        cv[0][i] = testcolor[0][i];
        cv[1][i] = testcolor[1][i];
        cv[2][i] = (testcolor[0][i] * 2 + testcolor[1][i]) / 3;
        cv[3][i] = (testcolor[0][i] + testcolor[1][i] * 2) / 3;
    }
    for (j = 0; j < 2; j++) for (z = 0; z < 3; z++) blockerrlin[j][z] = 0;
    nrcolor[0] = nrcolor[1] = 0;
    for (j = 0; j < 4; j++) {
        for (i = 0; i < 4; i++) {
            pixerrorbest = 0xffffffff;
            for (colors = 0; colors < 4; colors++) {
                colordist = srccolors[j][i][0] - cv[colors][0];
                pixerror = colordist * colordist * REDWEIGHT;
                pixerrorred = colordist;
                colordist = srccolors[j][i][1] - cv[colors][1];
                pixerror += colordist * colordist * GREENWEIGHT;
                pixerrorgreen = colordist;
                colordist = srccolors[j][i][2] - cv[colors][2];
                pixerror += colordist * colordist * BLUEWEIGHT;
                pixerrorblue = colordist;
                if (pixerror < pixerrorbest) {
                    enc = (GLubyte)colors;
                    pixerrorbest = pixerror;
                    pixerrorcolorbest[0] = pixerrorred;
                    pixerrorcolorbest[1] = pixerrorgreen;
                    pixerrorcolorbest[2] = pixerrorblue;
                }
            }
            if (enc == 0) {
                for (z = 0; z < 3; z++) blockerrlin[0][z] += 3 * pixerrorcolorbest[z];
                nrcolor[0] += 3;
            } else if (enc == 2) {
                for (z = 0; z < 3; z++) { blockerrlin[0][z] += 2 * pixerrorcolorbest[z]; blockerrlin[1][z] += pixerrorcolorbest[z]; }
                nrcolor[0] += 2;
                nrcolor[1] += 1;
            } else if (enc == 3) {
                for (z = 0; z < 3; z++) { blockerrlin[0][z] += pixerrorcolorbest[z]; blockerrlin[1][z] += 2 * pixerrorcolorbest[z]; }
                nrcolor[0] += 1;
                nrcolor[1] += 2;
            } else {
                for (z = 0; z < 3; z++) blockerrlin[1][z] += 3 * pixerrorcolorbest[z];
                nrcolor[1] += 3;
            }
        }
    }
    if (nrcolor[0] == 0) nrcolor[0] = 1;
    if (nrcolor[1] == 0) nrcolor[1] = 1;
    for (j = 0; j < 2; j++) {
        for (i = 0; i < 3; i++) {
            GLint newvalue = testcolor[j][i] + blockerrlin[j][i] / nrcolor[j];
            if (newvalue <= 0) testcolor[j][i] = 0;
            else if (newvalue >= 255) testcolor[j][i] = 255;
            else testcolor[j][i] = (GLubyte)newvalue;
        }
    }
    if (abs(testcolor[0][0] - testcolor[1][0]) < 8 && abs(testcolor[0][1] - testcolor[1][1]) < 4 &&
        abs(testcolor[0][2] - testcolor[1][2]) < 8) {
        /* both colors are so close they might get encoded as the same 16bit values */
        GLubyte coldiffred, coldiffgreen, coldiffblue, coldiffmax, factor, ind0, ind1;
        coldiffred = (GLubyte)abs(testcolor[0][0] - testcolor[1][0]);
        coldiffgreen = (GLubyte)(2 * abs(testcolor[0][1] - testcolor[1][1]));
        coldiffblue = (GLubyte)abs(testcolor[0][2] - testcolor[1][2]);
        coldiffmax = coldiffred;
        if (coldiffmax < coldiffgreen) coldiffmax = coldiffgreen;
        if (coldiffmax < coldiffblue) coldiffmax = coldiffblue;
        if (coldiffmax > 0) {
            if (coldiffmax > 4) factor = 2;
            else if (coldiffmax > 2) factor = 3;
            else factor = 4;
            if (testcolor[1][1] >= testcolor[0][1]) { ind1 = 1; ind0 = 0; }
            else { ind1 = 0; ind0 = 1; }
            if (testcolor[ind1][1] + factor * coldiffgreen <= 255) testcolor[ind1][1] += factor * coldiffgreen;
            else testcolor[ind1][1] = 255;
            if (testcolor[ind1][0] - testcolor[ind0][1] > 0) {
                if (testcolor[ind1][0] + factor * coldiffred <= 255) testcolor[ind1][0] += factor * coldiffred;
                else testcolor[ind1][0] = 255;
            } else {
                if (testcolor[ind0][0] + factor * coldiffred <= 255) testcolor[ind0][0] += factor * coldiffred;
                else testcolor[ind0][0] = 255;
            }
            if (testcolor[ind1][2] - testcolor[ind0][2] > 0) {
                if (testcolor[ind1][2] + factor * coldiffblue <= 255) testcolor[ind1][2] += factor * coldiffblue;
                else testcolor[ind1][2] = 255;
            } else {
                if (testcolor[ind0][2] + factor * coldiffblue <= 255) testcolor[ind0][2] += factor * coldiffblue;
                else testcolor[ind0][2] = 255;
            }
        }
    }
    if (((testcolor[0][0] & 0xf8) << 8 | (testcolor[0][1] & 0xfc) << 3 | testcolor[0][2] >> 3) <
        ((testcolor[1][0] & 0xf8) << 8 | (testcolor[1][1] & 0xfc) << 3 | testcolor[1][2]) >> 3) {
        for (i = 0; i < 3; i++) { bestcolor[0][i] = testcolor[0][i]; bestcolor[1][i] = testcolor[1][i]; }
    } else {
        for (i = 0; i < 3; i++) { bestcolor[0][i] = testcolor[1][i]; bestcolor[1][i] = testcolor[0][i]; }
    }
}

/* the colour half of a block (8 bytes) from the two base colours: the better of the four-colour and (DXT1 only)
** the three-colour encoding; a DXT1 block with transparent texels always uses the three-colour one */
static void storedxtencodedblock(GLubyte *blkaddr, GLubyte srccolors[4][4][4], GLubyte *bestcolor[2], GLint type,
                                 GLboolean haveAlpha)
{
    GLint i, j, colors;
    GLuint testerror, testerror2, pixerror, pixerrorbest;
    GLint colordist;
    GLushort color0, color1, tempcolor;
    GLuint bits = 0, bits2 = 0;
    GLubyte *colorptr;
    GLubyte enc = 0;
    GLubyte cv[4][4];

    bestcolor[0][0] &= 0xf8; bestcolor[0][1] &= 0xfc; bestcolor[0][2] &= 0xf8;
    bestcolor[1][0] &= 0xf8; bestcolor[1][1] &= 0xfc; bestcolor[1][2] &= 0xf8;
    color0 = (GLushort)(bestcolor[0][0] << 8 | bestcolor[0][1] << 3 | bestcolor[0][2] >> 3);
    color1 = (GLushort)(bestcolor[1][0] << 8 | bestcolor[1][1] << 3 | bestcolor[1][2] >> 3);
    if (color0 < color1) {
        tempcolor = color0; color0 = color1; color1 = tempcolor;
        colorptr = bestcolor[0]; bestcolor[0] = bestcolor[1]; bestcolor[1] = colorptr;
    }
    for (i = 0; i < 3; i++) {
        cv[0][i] = bestcolor[0][i];
        cv[1][i] = bestcolor[1][i];
        cv[2][i] = (bestcolor[0][i] * 2 + bestcolor[1][i]) / 3;
        cv[3][i] = (bestcolor[0][i] + bestcolor[1][i] * 2) / 3;
    }
    testerror = 0;
    for (j = 0; j < 4; j++) {
        for (i = 0; i < 4; i++) {
            pixerrorbest = 0xffffffff;
            for (colors = 0; colors < 4; colors++) {
                colordist = srccolors[j][i][0] - cv[colors][0];
                pixerror = colordist * colordist * REDWEIGHT;
                colordist = srccolors[j][i][1] - cv[colors][1];
                pixerror += colordist * colordist * GREENWEIGHT;
                colordist = srccolors[j][i][2] - cv[colors][2];
                pixerror += colordist * colordist * BLUEWEIGHT;
                if (pixerror < pixerrorbest) { pixerrorbest = pixerror; enc = (GLubyte)colors; }
            }
            testerror += pixerrorbest;
            bits |= (GLuint)enc << (2 * (j * 4 + i));
        }
    }
    if (type != DXT_TYPE_RGBA) {                        /* DXT1: try the three-colour encoding too */
        for (i = 0; i < 3; i++) {
            cv[2][i] = (bestcolor[0][i] + bestcolor[1][i]) / 2;
            cv[3][i] = 0;
        }
        testerror2 = 0;
        for (j = 0; j < 4; j++) {
            for (i = 0; i < 4; i++) {
                pixerrorbest = 0xffffffff;
                if (type == DXT_TYPE_RGBA1 && srccolors[j][i][3] <= ALPHACUT) {
                    enc = 3;
                    pixerrorbest = 0;
                } else {
                    for (colors = 0; colors < 3; colors++) {
                        colordist = srccolors[j][i][0] - cv[colors][0];
                        pixerror = colordist * colordist * REDWEIGHT;
                        colordist = srccolors[j][i][1] - cv[colors][1];
                        pixerror += colordist * colordist * GREENWEIGHT;
                        colordist = srccolors[j][i][2] - cv[colors][2];
                        pixerror += colordist * colordist * BLUEWEIGHT;
                        if (pixerror < pixerrorbest) {
                            pixerrorbest = pixerror;
                            enc = (GLubyte)(colors > 1 ? colors : colors ^ 1);  /* the colours swap below */
                        }
                    }
                }
                testerror2 += pixerrorbest;
                bits2 |= (GLuint)enc << (2 * (j * 4 + i));
            }
        }
    } else {
        testerror2 = 0xffffffff;
    }
    if (testerror > testerror2 || haveAlpha) {
        *blkaddr++ = (GLubyte)color1; *blkaddr++ = (GLubyte)(color1 >> 8);
        *blkaddr++ = (GLubyte)color0; *blkaddr++ = (GLubyte)(color0 >> 8);
        *blkaddr++ = (GLubyte)bits2; *blkaddr++ = (GLubyte)(bits2 >> 8); *blkaddr++ = (GLubyte)(bits2 >> 16); *blkaddr = (GLubyte)(bits2 >> 24);
    } else {
        *blkaddr++ = (GLubyte)color0; *blkaddr++ = (GLubyte)(color0 >> 8);
        *blkaddr++ = (GLubyte)color1; *blkaddr++ = (GLubyte)(color1 >> 8);
        *blkaddr++ = (GLubyte)bits; *blkaddr++ = (GLubyte)(bits >> 8); *blkaddr++ = (GLubyte)(bits >> 16); *blkaddr = (GLubyte)(bits >> 24);
    }
}

/* the colour half of a block: base colours = the texels of lowest and highest weighted magnitude, refined once */
static void encodedxtcolorblockfaster(GLubyte *blkaddr, GLubyte srccolors[4][4][4], GLint type)
{
    GLubyte *bestcolor[2];
    GLubyte basecolors[2][3];
    GLint i, j;
    GLuint lowcv, highcv, testcv;
    GLboolean haveAlpha = GL_FALSE;

    lowcv = highcv = srccolors[0][0][0] * srccolors[0][0][0] * REDWEIGHT +
                     srccolors[0][0][1] * srccolors[0][0][1] * GREENWEIGHT +
                     srccolors[0][0][2] * srccolors[0][0][2] * BLUEWEIGHT;
    bestcolor[0] = bestcolor[1] = srccolors[0][0];
    for (j = 0; j < 4; j++) {
        for (i = 0; i < 4; i++) {
            /* don't use this as a base color if the pixel will get black/transparent anyway */
            if (type != DXT_TYPE_RGBA1 || srccolors[j][i][3] > ALPHACUT) {
                testcv = srccolors[j][i][0] * srccolors[j][i][0] * REDWEIGHT +
                         srccolors[j][i][1] * srccolors[j][i][1] * GREENWEIGHT +
                         srccolors[j][i][2] * srccolors[j][i][2] * BLUEWEIGHT;
                if (testcv > highcv) { highcv = testcv; bestcolor[1] = srccolors[j][i]; }
                else if (testcv < lowcv) { lowcv = testcv; bestcolor[0] = srccolors[j][i]; }
            } else {
                haveAlpha = GL_TRUE;
            }
        }
    }
    for (j = 0; j < 2; j++) for (i = 0; i < 3; i++) basecolors[j][i] = bestcolor[j][i];
    bestcolor[0] = basecolors[0];
    bestcolor[1] = basecolors[1];
    fancybasecolorsearch(srccolors, bestcolor);
    storedxtencodedblock(blkaddr, srccolors, bestcolor, type, haveAlpha);
}

static void writedxt5encodedalphablock(GLubyte *blkaddr, GLubyte alphabase1, GLubyte alphabase2, GLubyte alphaenc[16])
{
    *blkaddr++ = alphabase1;
    *blkaddr++ = alphabase2;
    *blkaddr++ = (GLubyte)(alphaenc[0] | (alphaenc[1] << 3) | ((alphaenc[2] & 3) << 6));
    *blkaddr++ = (GLubyte)((alphaenc[2] >> 2) | (alphaenc[3] << 1) | (alphaenc[4] << 4) | ((alphaenc[5] & 1) << 7));
    *blkaddr++ = (GLubyte)((alphaenc[5] >> 1) | (alphaenc[6] << 2) | (alphaenc[7] << 5));
    *blkaddr++ = (GLubyte)(alphaenc[8] | (alphaenc[9] << 3) | ((alphaenc[10] & 3) << 6));
    *blkaddr++ = (GLubyte)((alphaenc[10] >> 2) | (alphaenc[11] << 1) | (alphaenc[12] << 4) | ((alphaenc[13] & 1) << 7));
    *blkaddr++ = (GLubyte)((alphaenc[13] >> 1) | (alphaenc[14] << 2) | (alphaenc[15] << 5));
}

/* DXT5's alpha half: the eight-value encoding over the block's range, or the six-value one (with 0 and 255 fixed)
** from the range of the in-between alphas or from refined base values; the one with the least error wins */
static void encodedxt5alpha(GLubyte *blkaddr, GLubyte srccolors[4][4][4])
{
    GLubyte alphabase[2], alphause[2];
    GLshort alphatest[2];
    GLuint alphablockerror1, alphablockerror2, alphablockerror3;
    GLint i, j, aindex;
    GLubyte acutValues[7];
    GLubyte alphaenc1[16], alphaenc2[16], alphaenc3[16];
    GLboolean alphaabsmin = GL_FALSE;
    GLboolean alphaabsmax = GL_FALSE;
    GLshort alphadist;

    alphatest[0] = alphatest[1] = 0;
    alphabase[0] = 0xff; alphabase[1] = 0x0;
    for (j = 0; j < 4; j++) {
        for (i = 0; i < 4; i++) {
            if (srccolors[j][i][3] == 0) alphaabsmin = GL_TRUE;
            else if (srccolors[j][i][3] == 255) alphaabsmax = GL_TRUE;
            else {
                if (srccolors[j][i][3] > alphabase[1]) alphabase[1] = srccolors[j][i][3];
                if (srccolors[j][i][3] < alphabase[0]) alphabase[0] = srccolors[j][i][3];
            }
        }
    }
    if (alphabase[0] > alphabase[1] && !(alphaabsmin && alphaabsmax)) {    /* one alpha, either max or min */
        *blkaddr++ = srccolors[0][0][3];
        blkaddr++;
        for (i = 0; i < 6; i++) *blkaddr++ = 0;
        return;
    }
    alphablockerror1 = 0x0;
    alphablockerror2 = 0xffffffff;
    alphablockerror3 = 0xffffffff;
    alphause[0] = alphaabsmin ? 0 : alphabase[0];
    alphause[1] = alphaabsmax ? 255 : alphabase[1];
    for (aindex = 0; aindex < 7; aindex++)
        acutValues[aindex] = (GLubyte)((alphause[0] * (2 * aindex + 1) + alphause[1] * (14 - (2 * aindex + 1))) / 14);
    for (j = 0; j < 4; j++) {
        for (i = 0; i < 4; i++) {
            GLubyte a = srccolors[j][i][3];
            if (a > acutValues[0])      { alphaenc1[4 * j + i] = 0; alphadist = a - alphause[1]; }
            else if (a > acutValues[1]) { alphaenc1[4 * j + i] = 2; alphadist = a - (alphause[1] * 6 + alphause[0] * 1) / 7; }
            else if (a > acutValues[2]) { alphaenc1[4 * j + i] = 3; alphadist = a - (alphause[1] * 5 + alphause[0] * 2) / 7; }
            else if (a > acutValues[3]) { alphaenc1[4 * j + i] = 4; alphadist = a - (alphause[1] * 4 + alphause[0] * 3) / 7; }
            else if (a > acutValues[4]) { alphaenc1[4 * j + i] = 5; alphadist = a - (alphause[1] * 3 + alphause[0] * 4) / 7; }
            else if (a > acutValues[5]) { alphaenc1[4 * j + i] = 6; alphadist = a - (alphause[1] * 2 + alphause[0] * 5) / 7; }
            else if (a > acutValues[6]) { alphaenc1[4 * j + i] = 7; alphadist = a - (alphause[1] * 1 + alphause[0] * 6) / 7; }
            else                        { alphaenc1[4 * j + i] = 1; alphadist = a - alphause[0]; }
            alphablockerror1 += alphadist * alphadist;
        }
    }
    if (alphablockerror1 >= 32) {
        alphablockerror2 = 0;
        for (aindex = 0; aindex < 5; aindex++)
            acutValues[aindex] = (GLubyte)((alphabase[0] * (10 - (2 * aindex + 1)) + alphabase[1] * (2 * aindex + 1)) / 10);
        for (j = 0; j < 4; j++) {
            for (i = 0; i < 4; i++) {
                GLubyte a = srccolors[j][i][3];
                if (a == 0)                  { alphaenc2[4 * j + i] = 6; alphadist = 0; }
                else if (a == 255)           { alphaenc2[4 * j + i] = 7; alphadist = 0; }
                else if (a <= acutValues[0]) { alphaenc2[4 * j + i] = 0; alphadist = a - alphabase[0]; }
                else if (a <= acutValues[1]) { alphaenc2[4 * j + i] = 2; alphadist = a - (alphabase[0] * 4 + alphabase[1] * 1) / 5; }
                else if (a <= acutValues[2]) { alphaenc2[4 * j + i] = 3; alphadist = a - (alphabase[0] * 3 + alphabase[1] * 2) / 5; }
                else if (a <= acutValues[3]) { alphaenc2[4 * j + i] = 4; alphadist = a - (alphabase[0] * 2 + alphabase[1] * 3) / 5; }
                else if (a <= acutValues[4]) { alphaenc2[4 * j + i] = 5; alphadist = a - (alphabase[0] * 1 + alphabase[1] * 4) / 5; }
                else                         { alphaenc2[4 * j + i] = 1; alphadist = a - alphabase[1]; }
                alphablockerror2 += alphadist * alphadist;
            }
        }
        if (alphablockerror2 > 96 && alphablockerror1 > 96) {
            GLshort blockerrlin1 = 0;
            GLshort blockerrlin2 = 0;
            GLubyte nralphainrangelow = 0;
            GLubyte nralphainrangehigh = 0;
            alphatest[0] = 0xff;
            alphatest[1] = 0x0;
            /* if we have large range it's likely there are values close to 0/255, try to map them to 0/255 */
            for (j = 0; j < 4; j++) {
                for (i = 0; i < 4; i++) {
                    GLubyte a = srccolors[j][i][3];
                    if (a > alphatest[1] && a < 255 - (alphabase[1] - alphabase[0]) / 28) alphatest[1] = a;
                    if (a < alphatest[0] && a > (alphabase[1] - alphabase[0]) / 28) alphatest[0] = a;
                }
            }
            if (alphatest[1] <= alphatest[0]) { alphatest[0] = 1; alphatest[1] = 254; }
            for (aindex = 0; aindex < 5; aindex++)
                acutValues[aindex] = (GLubyte)((alphatest[0] * (10 - (2 * aindex + 1)) + alphatest[1] * (2 * aindex + 1)) / 10);
            for (j = 0; j < 4; j++) {
                for (i = 0; i < 4; i++) {
                    GLubyte a = srccolors[j][i][3];
                    if (a <= alphatest[0] / 2) {
                    } else if (a > (255 + alphatest[1]) / 2) {
                    } else if (a <= acutValues[0]) {
                        blockerrlin1 += (GLshort)(a - alphatest[0]);
                        nralphainrangelow += 1;
                    } else if (a <= acutValues[1]) {
                        blockerrlin1 += (GLshort)(a - (alphatest[0] * 4 + alphatest[1] * 1) / 5);
                        blockerrlin2 += (GLshort)(a - (alphatest[0] * 4 + alphatest[1] * 1) / 5);
                        nralphainrangelow += 1; nralphainrangehigh += 1;
                    } else if (a <= acutValues[2]) {
                        blockerrlin1 += (GLshort)(a - (alphatest[0] * 3 + alphatest[1] * 2) / 5);
                        blockerrlin2 += (GLshort)(a - (alphatest[0] * 3 + alphatest[1] * 2) / 5);
                        nralphainrangelow += 1; nralphainrangehigh += 1;
                    } else if (a <= acutValues[3]) {
                        blockerrlin1 += (GLshort)(a - (alphatest[0] * 2 + alphatest[1] * 3) / 5);
                        blockerrlin2 += (GLshort)(a - (alphatest[0] * 2 + alphatest[1] * 3) / 5);
                        nralphainrangelow += 1; nralphainrangehigh += 1;
                    } else if (a <= acutValues[4]) {
                        blockerrlin1 += (GLshort)(a - (alphatest[0] * 1 + alphatest[1] * 4) / 5);
                        blockerrlin2 += (GLshort)(a - (alphatest[0] * 1 + alphatest[1] * 4) / 5);
                        nralphainrangelow += 1; nralphainrangehigh += 1;
                    } else {
                        blockerrlin2 += (GLshort)(a - alphatest[1]);
                        nralphainrangehigh += 1;
                    }
                }
            }
            if (nralphainrangelow == 0) nralphainrangelow = 1;
            if (nralphainrangehigh == 0) nralphainrangehigh = 1;
            alphatest[0] = (GLshort)(alphatest[0] + blockerrlin1 / nralphainrangelow);
            if (alphatest[0] < 0) alphatest[0] = 0;
            alphatest[1] = (GLshort)(alphatest[1] + blockerrlin2 / nralphainrangehigh);
            if (alphatest[1] > 255) alphatest[1] = 255;
            alphablockerror3 = 0;
            for (aindex = 0; aindex < 5; aindex++)
                acutValues[aindex] = (GLubyte)((alphatest[0] * (10 - (2 * aindex + 1)) + alphatest[1] * (2 * aindex + 1)) / 10);
            for (j = 0; j < 4; j++) {
                for (i = 0; i < 4; i++) {
                    GLubyte a = srccolors[j][i][3];
                    if (a <= alphatest[0] / 2)            { alphaenc3[4 * j + i] = 6; alphadist = a; }
                    else if (a > (255 + alphatest[1]) / 2) { alphaenc3[4 * j + i] = 7; alphadist = 255 - a; }
                    else if (a <= acutValues[0])          { alphaenc3[4 * j + i] = 0; alphadist = a - alphatest[0]; }
                    else if (a <= acutValues[1])          { alphaenc3[4 * j + i] = 2; alphadist = a - (alphatest[0] * 4 + alphatest[1] * 1) / 5; }
                    else if (a <= acutValues[2])          { alphaenc3[4 * j + i] = 3; alphadist = a - (alphatest[0] * 3 + alphatest[1] * 2) / 5; }
                    else if (a <= acutValues[3])          { alphaenc3[4 * j + i] = 4; alphadist = a - (alphatest[0] * 2 + alphatest[1] * 3) / 5; }
                    else if (a <= acutValues[4])          { alphaenc3[4 * j + i] = 5; alphadist = a - (alphatest[0] * 1 + alphatest[1] * 4) / 5; }
                    else                                  { alphaenc3[4 * j + i] = 1; alphadist = a - alphatest[1]; }
                    alphablockerror3 += alphadist * alphadist;
                }
            }
        }
    }
    if (alphablockerror1 <= alphablockerror2 && alphablockerror1 <= alphablockerror3)
        writedxt5encodedalphablock(blkaddr, alphause[1], alphause[0], alphaenc1);
    else if (alphablockerror2 <= alphablockerror3)
        writedxt5encodedalphablock(blkaddr, alphabase[0], alphabase[1], alphaenc2);
    else
        writedxt5encodedalphablock(blkaddr, (GLubyte)alphatest[0], (GLubyte)alphatest[1], alphaenc3);
}

void __fastcall hwDxtEncode(void *dst, const GLuint *src, GLint w, GLint h, GLint grFormat, GLboolean punch)
{
    GLubyte *out = (GLubyte *)dst;
    GLubyte blk[4][4][4];
    GLint x, y, j;

    for (y = 0; y < h; y += 4) {
        for (x = 0; x < w; x += 4) {
            dxtFetchBlock(blk, src, w, h, x, y);
            switch (grFormat) {
              case TEXCMP_GR_DXT3: {                    /* 4-bit alpha, rounded (libtxc_dxtn truncates) */
                GLuint a0, a1;
                for (j = 0; j < 8; j++) {
                    a0 = (blk[j >> 1][(j & 1) * 2][3] * 15u + 127u) / 255u;
                    a1 = (blk[j >> 1][(j & 1) * 2 + 1][3] * 15u + 127u) / 255u;
                    *out++ = (GLubyte)(a0 | (a1 << 4));
                }
              }
                encodedxtcolorblockfaster(out, blk, DXT_TYPE_RGBA);
                out += 8;
                break;
              case TEXCMP_GR_DXT5:
                encodedxt5alpha(out, blk);
                encodedxtcolorblockfaster(out + 8, blk, DXT_TYPE_RGBA);
                out += 16;
                break;
              default:
                encodedxtcolorblockfaster(out, blk, punch ? DXT_TYPE_RGBA1 : DXT_TYPE_RGB1);
                out += 8;
                break;
            }
        }
    }
}

/* ---- the decoder */

/* 565 -> the 8-bit channels the hardware expands to */
static void dxtFrom565(GLuint c, GLint rgb[3])
{
    GLint r = (c >> 11) & 0x1f, g = (c >> 5) & 0x3f, b = c & 0x1f;
    rgb[0] = (r << 3) | (r >> 2);
    rgb[1] = (g << 2) | (g >> 4);
    rgb[2] = (b << 3) | (b >> 2);
}

/* the colour half of a block -> 16 ARGB words with alpha 255 (or 0 for DXT1's transparent index) */
static void dxtDecodeColor(GLuint texel[16], const GLubyte *src, GLboolean dxt1)
{
    GLuint c0 = src[0] | (src[1] << 8), c1 = src[2] | (src[3] << 8);
    GLuint bits = src[4] | (src[5] << 8) | ((GLuint)src[6] << 16) | ((GLuint)src[7] << 24);
    GLint pal[4][3], c, i;
    GLuint pix[4];

    dxtFrom565(c0, pal[0]);
    dxtFrom565(c1, pal[1]);
    for (c = 0; c < 3; c++) {
        if (c0 > c1 || !dxt1) {
            pal[2][c] = (2 * pal[0][c] + pal[1][c] + 1) / 3;
            pal[3][c] = (pal[0][c] + 2 * pal[1][c] + 1) / 3;
        } else {
            pal[2][c] = (pal[0][c] + pal[1][c]) >> 1;
            pal[3][c] = 0;
        }
    }
    for (i = 0; i < 4; i++)
        pix[i] = 0xff000000u | ((GLuint)pal[i][0] << 16) | ((GLuint)pal[i][1] << 8) | (GLuint)pal[i][2];
    if (dxt1 && c0 <= c1) pix[3] = 0;                   /* transparent black */
    for (i = 0; i < 16; i++) texel[i] = pix[(bits >> (2 * i)) & 3];
}

void __fastcall hwDxtDecode(GLuint *dst, const void *src, GLint w, GLint h, GLint grFormat)
{
    const GLubyte *in = (const GLubyte *)src;
    GLuint texel[16];
    GLint x, y, i, j;

    for (y = 0; y < h; y += 4) {
        for (x = 0; x < w; x += 4) {
            switch (grFormat) {
              case TEXCMP_GR_DXT3:
                dxtDecodeColor(texel, in + 8, GL_FALSE);
                for (i = 0; i < 16; i++) {
                    GLuint a = (in[i >> 1] >> ((i & 1) * 4)) & 0xf;
                    texel[i] = (texel[i] & 0xffffff) | ((a * 17) << 24);
                }
                in += 16;
                break;
              case TEXCMP_GR_DXT5: {
                GLint pal[8], k;
                GLuint lo = in[2] | (in[3] << 8) | ((GLuint)in[4] << 16), hi = in[5] | (in[6] << 8) | ((GLuint)in[7] << 16);
                pal[0] = in[0];
                pal[1] = in[1];
                if (pal[0] > pal[1]) {
                    for (k = 1; k <= 6; k++) pal[k + 1] = ((7 - k) * pal[0] + k * pal[1] + 3) / 7;
                } else {
                    for (k = 1; k <= 4; k++) pal[k + 1] = ((5 - k) * pal[0] + k * pal[1] + 2) / 5;
                    pal[6] = 0;
                    pal[7] = 255;
                }
                dxtDecodeColor(texel, in + 8, GL_FALSE);
                for (i = 0; i < 16; i++) {
                    GLuint idx = i < 8 ? (lo >> (3 * i)) & 7 : (hi >> (3 * (i - 8))) & 7;
                    texel[i] = (texel[i] & 0xffffff) | ((GLuint)pal[idx] << 24);
                }
                in += 16;
                break;
              }
              default:
                dxtDecodeColor(texel, in, GL_TRUE);
                in += 8;
                break;
            }
            for (i = 0; i < 4 && y + i < h; i++)
                for (j = 0; j < 4 && x + j < w; j++)
                    dst[(y + i) * w + x + j] = texel[i * 4 + j];
        }
    }
}

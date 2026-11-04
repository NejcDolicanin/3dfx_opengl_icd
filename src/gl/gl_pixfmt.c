/*
** gl_pixfmt.c -- the pixel-format list: the display layout and the format description (wglDescribePixelFormat
** back end).
*/
#include <string.h>
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "glshade.h"
#include "icdglob.h"
#include "icdprotos.h"
#include "glsitypes.h"
#include <stdlib.h>

/* 690f5dc8 -- the 32 pixel-format templates __glBuildPixelFormatList filters into g_pfdList.  Per row: nSize, nVersion,
** dwFlags; iPixelType, cColorBits, cRed/Green/Blue/AlphaBits and -Shift; cAccumBits and its four; cDepthBits,
** cStencilBits, cAuxBuffers; iLayerType, bReserved, the three masks */
static PIXELFORMATDESCRIPTOR g_pfdTemplates[32] = {
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0,  64, 16, 16, 16, 16,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0,  64, 16, 16, 16, 16,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0,  64, 16, 16, 16, 16,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0,  64, 16, 16, 16, 16,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 32, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 32, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 32, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 32, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 24, 8, 16, 8, 8, 8, 0, 0, 0,  64, 16, 16, 16, 16,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 24, 8, 16, 8, 8, 8, 0, 0, 0,  64, 16, 16, 16, 16,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 24, 8, 16, 8, 8, 8, 0, 0, 0,  64, 16, 16, 16, 16,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 24, 8, 16, 8, 8, 8, 0, 0, 0,  64, 16, 16, 16, 16,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 24, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 24, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 24, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 24, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 16, 0, 0, 0, 0, 0, 0, 0, 0,  64, 16, 16, 16, 16,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 16, 0, 0, 0, 0, 0, 0, 0, 0,  64, 16, 16, 16, 16,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 16, 0, 0, 0, 0, 0, 0, 0, 0,  64, 16, 16, 16, 16,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 16, 0, 0, 0, 0, 0, 0, 0, 0,  64, 16, 16, 16, 16,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 16, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 16, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 16, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 16, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 8, 3, 5, 3, 2, 2, 0, 0, 0,  64, 16, 16, 16, 16,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 8, 3, 5, 3, 2, 2, 0, 0, 0,  64, 16, 16, 16, 16,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 8, 3, 5, 3, 2, 2, 0, 0, 0,  64, 16, 16, 16, 16,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_RGBA, 8, 3, 5, 3, 2, 2, 0, 0, 0,  64, 16, 16, 16, 16,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 8, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DRAW_TO_WINDOW | PFD_DRAW_TO_BITMAP | PFD_SUPPORT_GDI | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 8, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 8, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  32, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
    { 40, 1, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
      PFD_TYPE_COLORINDEX, 8, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0,  16, 8, 0, PFD_MAIN_PLANE, 0, 0, 0, 0 },
};

/* 690f62c8 -- 1: rebuild the pixel-format list; 693c3cc4, 693c3cc8 -- the list (wglMalloc'd [32]) */
GLint g_pfdListStale = 1;
GLint g_pfdCount;
PIXELFORMATDESCRIPTOR **g_pfdList;

/* ---- the glcore's pixel formats: 32 PIXELFORMATDESCRIPTOR templates, the usable ones listed lazily by
** __glBuildPixelFormatList (glcore_hand.c); og_util.c __glResetPixelFormats drops the list ---- */

/* 690e9f00 -- __glScreenPixelLayout: the screen's pixel layout through a 1x1 compatible bitmap: returns the bit count
** and the red / green / blue / alpha masks (BI_BITFIELDS masks when the driver reports them) */
GLuint __fastcall __glScreenPixelLayout(HDC hdc, DWORD *redMask, DWORD *greenMask, DWORD *blueMask, DWORD *alphaMask)
{
    HBITMAP hbm;
    BITMAPINFO *bmi;
    GLuint bpp = 0;

    hbm = CreateCompatibleBitmap(hdc, 1, 1);
    if (hbm) {
        bmi = (BITMAPINFO *)wglCalloc(1, 0x438);
        if (bmi) {
            bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            GetDIBits(hdc, hbm, 0, 1, NULL, bmi, DIB_RGB_COLORS);
            GetDIBits(hdc, hbm, 0, 1, NULL, bmi, DIB_RGB_COLORS);
            bpp = bmi->bmiHeader.biBitCount;
            switch (bpp) {
              case 1:
              case 4:
              case 8:
                *alphaMask = 0;
                *blueMask = 0;
                *greenMask = 0;
                *redMask = 0;
                break;
              case 16:
                if (bmi->bmiHeader.biCompression == BI_BITFIELDS) {
                    *redMask = ((DWORD *)bmi->bmiColors)[0];
                    *greenMask = ((DWORD *)bmi->bmiColors)[1];
                    *blueMask = ((DWORD *)bmi->bmiColors)[2];
                    *alphaMask = 0;
                } else {
                    *redMask = 0x7c00;
                    *greenMask = 0x3e0;
                    *blueMask = 0x1f;
                    *alphaMask = 0;
                }
                break;
              case 24:
                *redMask = 0xff0000;
                *greenMask = 0xff00;
                *blueMask = 0xff;
                *alphaMask = 0;
                break;
              case 32:
                if (bmi->bmiHeader.biCompression == BI_BITFIELDS) {
                    *redMask = ((DWORD *)bmi->bmiColors)[0];
                    *greenMask = ((DWORD *)bmi->bmiColors)[1];
                    *blueMask = ((DWORD *)bmi->bmiColors)[2];
                    *alphaMask = 0;
                } else {
                    *redMask = 0xff0000;
                    *greenMask = 0xff00;
                    *blueMask = 0xff;
                    *alphaMask = 0;
                }
                break;
            }
        }
        DeleteObject(hbm);
        if (bmi) {
            wglFree(bmi);
        }
    }
    return bpp;
}

/* 690ea0d0 -- __glDescribePixelFormat: the glcore's describe-pixel-format callback (unreferenced; the device
** table uses wglGlideDescribePixelFormatCB): copy format i (0-based) of the usable list into *out; returns
** the format count, 0 for a bad index (the count when out is NULL).  Original bug (dead code): out of memory
** returns 0 with wglLock still held. */
GLint __fastcall __glDescribePixelFormat(HDC hdc, PIXELFORMATDESCRIPTOR *out, int i)
{
    wglLock();
    if (g_pfdListStale) {
	g_pfdList = (PIXELFORMATDESCRIPTOR **) wglMalloc(32 * sizeof(PIXELFORMATDESCRIPTOR *));
	if (g_pfdList == NULL)
	    return 0;
	__glBuildPixelFormatList(hdc, g_pfdTemplates, 32, g_pfdList, &g_pfdCount);
	g_pfdListStale = 0;
    }
    wglUnlock();
    if (i >= 0 && i < g_pfdCount) {
	*out = *g_pfdList[i];
	return g_pfdCount;
    }
    return out ? 0 : g_pfdCount;
}

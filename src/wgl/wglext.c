/*
** wglext.c -- WGL_EXT_extensions_string, WGL_ARB_extensions_string, WGL_EXT_swap_control
** and WGL_ARB_pixel_format entry points (handed out through wglGetProcAddress).
** All are __stdcall (called by the application).
*/
#include <string.h>
#include "wglpriv.h"
#include "icdprotos.h"

#define CUR_GC()  __glCurrentGC()          /* the glcore's inline TEB fetch: these entry points are built like the gl core */

/* 690136b0 -- wglGetExtensionsStringEXT: the current gc's extension string; NULL (INVALID_OPERATION) in glBegin */
const char *WINAPI wglGetExtensionsStringEXT(void)
{
    __GLcontext *gc = CUR_GC();
    if (__glCurrentGC()->beginMode == 1) {     /* __GL_SETUP_NOT_IN_BEGIN (a second TEB fetch) */ __glSetError(GL_INVALID_OPERATION); return NULL; }
    return gc->hwcx.extString;
}

/* 690136f0 -- wglGetExtensionsStringARB(hdc): only for the current context's DC */
const char *WINAPI wglGetExtensionsStringARB(HDC hdc)
{
    __GLcontext *gc = CUR_GC();
    __WGLrc *rc;
    if (__glCurrentGC()->beginMode == 1) {     /* __GL_SETUP_NOT_IN_BEGIN (a second TEB fetch) */ __glSetError(GL_INVALID_OPERATION); return NULL; }
    rc = wglRcFromGC(gc);
    if (rc != NULL && hdc == rc->hdc) return gc->hwcx.extString;
    __glSetError(ERROR_DC_NOT_FOUND);   /* 0x591: a Win32 code through __glSetError (original): glGetError returns it */
    return NULL;
}

/* 69013760 -- wglSwapIntervalEXT (WGL_EXT_swap_control): the interval grBufferSwap waits */
GLboolean WINAPI wglSwapIntervalEXT(int interval)
{
    __GLcontext *gc = CUR_GC();
    if (__glCurrentGC()->beginMode == 1) {     /* __GL_SETUP_NOT_IN_BEGIN (a second TEB fetch) */ __glSetError(GL_INVALID_OPERATION); return FALSE; }
    gc->hwcx.swapInterval = interval;
    return TRUE;
}

/* 690137b0 -- wglGetSwapIntervalEXT: the current context's swap interval; 0 (GL_INVALID_OPERATION) inside glBegin */
int WINAPI wglGetSwapIntervalEXT(void)
{
    __GLcontext *gc = CUR_GC();
    if (__glCurrentGC()->beginMode == 1) {     /* __GL_SETUP_NOT_IN_BEGIN (a second TEB fetch) */ __glSetError(GL_INVALID_OPERATION); return 0; }
    return gc->hwcx.swapInterval;
}

/* WGL_ARB_pixel_format attribute names */
#define WGL_NUMBER_PIXEL_FORMATS_ARB  0x2000
#define WGL_DRAW_TO_WINDOW_ARB        0x2001
#define WGL_DRAW_TO_BITMAP_ARB        0x2002
#define WGL_ACCELERATION_ARB          0x2003
#define WGL_NEED_PALETTE_ARB          0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB   0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB    0x2006
#define WGL_SWAP_METHOD_ARB           0x2007
#define WGL_NUMBER_OVERLAYS_ARB       0x2008
#define WGL_NUMBER_UNDERLAYS_ARB      0x2009
#define WGL_TRANSPARENT_ARB           0x200A
#define WGL_SHARE_DEPTH_ARB           0x200C
#define WGL_SHARE_STENCIL_ARB         0x200D
#define WGL_SHARE_ACCUM_ARB           0x200E
#define WGL_SUPPORT_GDI_ARB           0x200F
#define WGL_SUPPORT_OPENGL_ARB        0x2010
#define WGL_DOUBLE_BUFFER_ARB         0x2011
#define WGL_STEREO_ARB                0x2012
#define WGL_PIXEL_TYPE_ARB            0x2013
#define WGL_COLOR_BITS_ARB            0x2014
#define WGL_RED_BITS_ARB              0x2015
#define WGL_RED_SHIFT_ARB             0x2016
#define WGL_GREEN_BITS_ARB            0x2017
#define WGL_GREEN_SHIFT_ARB           0x2018
#define WGL_BLUE_BITS_ARB             0x2019
#define WGL_BLUE_SHIFT_ARB            0x201A
#define WGL_ALPHA_BITS_ARB            0x201B
#define WGL_ALPHA_SHIFT_ARB           0x201C
#define WGL_ACCUM_BITS_ARB            0x201D
#define WGL_ACCUM_RED_BITS_ARB        0x201E
#define WGL_ACCUM_GREEN_BITS_ARB      0x201F
#define WGL_ACCUM_BLUE_BITS_ARB       0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB      0x2021
#define WGL_DEPTH_BITS_ARB            0x2022
#define WGL_STENCIL_BITS_ARB          0x2023
#define WGL_AUX_BUFFERS_ARB           0x2024
#define WGL_NO_ACCELERATION_ARB       0x2025
#define WGL_GENERIC_ACCELERATION_ARB  0x2026
#define WGL_FULL_ACCELERATION_ARB     0x2027
#define WGL_SWAP_EXCHANGE_ARB         0x2028
#define WGL_SWAP_COPY_ARB             0x2029
#define WGL_SWAP_UNDEFINED_ARB        0x202A
#define WGL_TYPE_RGBA_ARB             0x202B
#define WGL_TRANSPARENT_RED_VALUE_ARB 0x2037
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203B
#define WGL_SAMPLE_BUFFERS_3DFX       0x2060
#define WGL_SAMPLES_3DFX              0x2061

/* 690137f0 -- wglPixelFormatAttribs: shared body of wglGetPixelFormatAttribivARB / fvARB (__stdcall, eight
** arguments).  'stride' is 4 for both; 'asFloat' selects the float conversion.  Formats below the device's count
** come from the 3dfx table, the rest from GDI (original quirk: `iPixelFormat < devCount`, so the last 3dfx format,
** number devCount, is described by GDI's DescribePixelFormat; wglChoosePixelFormatARB uses `<=`).  The 3dfx PFD
** table rows carry extra dwords after the PIXELFORMATDESCRIPTOR: the multisample count is the first.
*/
static BOOL WINAPI wglPixelFormatAttribs(HDC hdc, int iPixelFormat, int iLayerPlane, UINT n,
                                             const int *attrs, float *fo, int stride, int asFloat)
{
    __GLcontext *gc;
    __WGLrc *rc;
    struct { PIXELFORMATDESCRIPTOR pfd; GLuint samples; } fmt;     /* 0x2c bytes: the original zeroes 11 dwords */
    int numFormats, ok, bad, devCount;
    (void)iLayerPlane;

    bad = 0;
    ok = 1;
    gc = CUR_GC();

    if (__glCurrentGC()->beginMode == 1) {     /* __GL_SETUP_NOT_IN_BEGIN (a second TEB fetch) */ __glSetError(GL_INVALID_OPERATION); return FALSE; }
    rc = wglRcFromGC(gc);
    if (rc == NULL || hdc != rc->hdc) { __glSetError(ERROR_DC_NOT_FOUND); return FALSE; }     /* as above */
    memset(&fmt, 0, sizeof(fmt));
    fmt.pfd.nSize = sizeof(fmt);        /* 0x2c (sic: the PFD and the sample count) */
    fmt.pfd.nVersion = 1;
    numFormats = DescribePixelFormat(hdc, -1, sizeof(PIXELFORMATDESCRIPTOR), NULL);
    devCount = wglGlideDescribePixelFormat(hdc, NULL, -1);
    if (iPixelFormat < devCount) {
        if (wglGlideDescribePixelFormat(hdc, &fmt.pfd, iPixelFormat - 1) == 0) ok = 0;
    } else {
        if (DescribePixelFormat(hdc, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &fmt.pfd) == 0) ok = 0;
    }
    for (; n > 0; n--, attrs++, fo = (float *)((BYTE *)fo + stride)) {
/* each attribute stores twice over: the float path converts the int value, the int path stores it */
#define PUT(x) do { if (asFloat) *fo = (float)(int)(x); else *(int *)fo = (x); } while (0)
        if (*attrs == WGL_NUMBER_PIXEL_FORMATS_ARB) { PUT(numFormats); continue; }
        if (!ok) { bad = 1; continue; }
        switch (*attrs) {
        case WGL_NUMBER_OVERLAYS_ARB:     PUT(fmt.pfd.bReserved & 0xf); break;
        case WGL_NUMBER_UNDERLAYS_ARB:    PUT(fmt.pfd.bReserved >> 4); break;
        case WGL_DRAW_TO_WINDOW_ARB:      PUT((fmt.pfd.dwFlags >> 2) & 1); break;
        case WGL_DRAW_TO_BITMAP_ARB:      PUT((fmt.pfd.dwFlags >> 3) & 1); break;
        case WGL_NEED_PALETTE_ARB:        PUT((fmt.pfd.dwFlags >> 7) & 1); break;
        case WGL_NEED_SYSTEM_PALETTE_ARB: PUT((fmt.pfd.dwFlags >> 8) & 1); break;
        case WGL_SWAP_LAYER_BUFFERS_ARB:  PUT((fmt.pfd.dwFlags >> 11) & 1); break;
        case WGL_SWAP_METHOD_ARB:
            PUT((fmt.pfd.dwFlags & PFD_SWAP_EXCHANGE) ? WGL_SWAP_EXCHANGE_ARB
                : (fmt.pfd.dwFlags & PFD_SWAP_COPY) ? WGL_SWAP_COPY_ARB : WGL_SWAP_UNDEFINED_ARB);
            break;
        case WGL_ACCELERATION_ARB:
            PUT((fmt.pfd.dwFlags & PFD_GENERIC_FORMAT)
                ? ((fmt.pfd.dwFlags & PFD_GENERIC_ACCELERATED) ? WGL_GENERIC_ACCELERATION_ARB : WGL_NO_ACCELERATION_ARB)
                : WGL_FULL_ACCELERATION_ARB);
            break;
        case WGL_TRANSPARENT_ARB: case 0x2037: case 0x2038: case 0x2039: case 0x203a: case WGL_TRANSPARENT_ALPHA_VALUE_ARB:
            PUT(0); break;
        case WGL_SHARE_DEPTH_ARB: case WGL_SHARE_STENCIL_ARB: case WGL_SHARE_ACCUM_ARB:
            PUT(1); break;
        case WGL_SUPPORT_GDI_ARB:         PUT((fmt.pfd.dwFlags >> 4) & 1); break;
        case WGL_SUPPORT_OPENGL_ARB:      PUT((fmt.pfd.dwFlags >> 5) & 1); break;
        case WGL_DOUBLE_BUFFER_ARB:       PUT(fmt.pfd.dwFlags & 1); break;
        case WGL_STEREO_ARB:              PUT((fmt.pfd.dwFlags >> 1) & 1); break;
        case WGL_PIXEL_TYPE_ARB:          PUT((fmt.pfd.iPixelType != 0) + WGL_TYPE_RGBA_ARB); break;
        case WGL_COLOR_BITS_ARB:          PUT(fmt.pfd.cColorBits); break;
        case WGL_RED_BITS_ARB:            PUT(fmt.pfd.cRedBits); break;
        case WGL_RED_SHIFT_ARB:           PUT(fmt.pfd.cRedShift); break;
        case WGL_GREEN_BITS_ARB:          PUT(fmt.pfd.cGreenBits); break;
        case WGL_GREEN_SHIFT_ARB:         PUT(fmt.pfd.cGreenShift); break;
        case WGL_BLUE_BITS_ARB:           PUT(fmt.pfd.cBlueBits); break;
        case WGL_BLUE_SHIFT_ARB:          PUT(fmt.pfd.cBlueShift); break;
        case WGL_ALPHA_BITS_ARB:          PUT(fmt.pfd.cAlphaBits); break;
        case WGL_ALPHA_SHIFT_ARB:         PUT(fmt.pfd.cAlphaShift); break;
        case WGL_ACCUM_BITS_ARB:          PUT(fmt.pfd.cAccumBits); break;
        case WGL_ACCUM_RED_BITS_ARB:      PUT(fmt.pfd.cAccumRedBits); break;
        case WGL_ACCUM_GREEN_BITS_ARB:    PUT(fmt.pfd.cAccumGreenBits); break;
        case WGL_ACCUM_BLUE_BITS_ARB:     PUT(fmt.pfd.cAccumBlueBits); break;
        case WGL_ACCUM_ALPHA_BITS_ARB:    PUT(fmt.pfd.cAccumAlphaBits); break;
        case WGL_DEPTH_BITS_ARB:          PUT(fmt.pfd.cDepthBits); break;
        case WGL_STENCIL_BITS_ARB:        PUT(fmt.pfd.cStencilBits); break;
        case WGL_AUX_BUFFERS_ARB:         PUT(fmt.pfd.cAuxBuffers); break;
        case WGL_SAMPLE_BUFFERS_3DFX:     PUT(fmt.samples > 0); break;
        case WGL_SAMPLES_3DFX:            if (asFloat) *fo = (float)fmt.samples; else *(GLuint *)fo = fmt.samples; break;
        default: bad = 1; break;
        }
#undef PUT
    }
    if (bad) { __glSetError(GL_INVALID_ENUM); return FALSE; }
    return TRUE;
}

/* 69014060 -- wglGetPixelFormatAttribivARB: wglPixelFormatAttribs with integer results */
BOOL WINAPI wglGetPixelFormatAttribivARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT n, const int *attrs, int *values)
{ return wglPixelFormatAttribs(hdc, iPixelFormat, iLayerPlane, n, attrs, (float *)values, 4, 0); }
/* 69014090 -- wglGetPixelFormatAttribfvARB: wglPixelFormatAttribs with float results */
BOOL WINAPI wglGetPixelFormatAttribfvARB(HDC hdc, int iPixelFormat, int iLayerPlane, UINT n, const int *attrs, FLOAT *values)
{ return wglPixelFormatAttribs(hdc, iPixelFormat, iLayerPlane, n, attrs, values, 4, 1); }

/* 690140c0 -- wglChoosePixelFormatARB.  Walks every format and keeps those matching all
** integer attributes then all float attributes (two passes over the same table).  The bit counts are
** minimums, compared unsigned as the original (jb): a negative request matches no format.
*/
BOOL WINAPI wglChoosePixelFormatARB(HDC hdc, const int *iattrs, const FLOAT *fattrs, UINT maxFormats, int *formats, UINT *numFormats)
{
    struct ListW29 { const void *list; int stride; int isFloat; } lists[2], *lp;
    struct { PIXELFORMATDESCRIPTOR pfd; GLuint samples; } fmt;
    int total, devCount, i, found;
    unsigned l;

    lists[0].list = iattrs; lists[0].stride = 4; lists[0].isFloat = 0;
    lists[1].list = fattrs; lists[1].stride = 4; lists[1].isFloat = 1;
    total    = DescribePixelFormat(hdc, -1, sizeof(PIXELFORMATDESCRIPTOR), NULL);
    devCount = wglGlideDescribePixelFormat(hdc, NULL, -1);
    i = 1;
    found = 0;
    /* (no check against maxFormats in the loop: the original writes every matching format) */
    for (; maxFormats > 0 && i <= total; i++) {
        int match;
        memset(&fmt, 0, sizeof(fmt));
        fmt.pfd.nSize = sizeof(fmt); fmt.pfd.nVersion = 1;     /* 0x2c, as above */
        if (i <= devCount) match = wglGlideDescribePixelFormat(hdc, &fmt.pfd, i - 1) != 0;
        else               match = DescribePixelFormat(hdc, i, sizeof(PIXELFORMATDESCRIPTOR), &fmt.pfd) != 0;
        if (!match) continue;
        for (lp = lists, l = 0; match && l < sizeof(lists) / sizeof(lists[0]); lp++, l++) {
            const BYTE *p = (const BYTE *)lp->list;
            int attr, val;

            while (match && p != NULL &&
                   (attr = lp->isFloat ? (int)*(const float *)p : *(const int *)p) != 0) {
                p += lp->stride;
                val = lp->isFloat ? (int)*(const float *)p : *(const int *)p;
                p += lp->stride;

                switch (attr) {
                case WGL_DRAW_TO_WINDOW_ARB:      match = (fmt.pfd.dwFlags & PFD_DRAW_TO_WINDOW) ? val != 0 : val == 0; break;
                case WGL_DRAW_TO_BITMAP_ARB:      match = (fmt.pfd.dwFlags & PFD_DRAW_TO_BITMAP) ? val != 0 : val == 0; break;
                case WGL_NEED_PALETTE_ARB:        match = (fmt.pfd.dwFlags & PFD_NEED_PALETTE) ? val != 0 : val == 0; break;
                case WGL_NEED_SYSTEM_PALETTE_ARB: match = (fmt.pfd.dwFlags & PFD_NEED_SYSTEM_PALETTE) ? val != 0 : val == 0; break;
                case WGL_SWAP_LAYER_BUFFERS_ARB:  match = (fmt.pfd.dwFlags & PFD_SWAP_LAYER_BUFFERS) ? val != 0 : val == 0; break;
                case WGL_SUPPORT_GDI_ARB:         match = (fmt.pfd.dwFlags & PFD_SUPPORT_GDI) ? val != 0 : val == 0; break;
                case WGL_SUPPORT_OPENGL_ARB:      match = (fmt.pfd.dwFlags & PFD_SUPPORT_OPENGL) ? val != 0 : val == 0; break;
                case WGL_DOUBLE_BUFFER_ARB:       match = (fmt.pfd.dwFlags & PFD_DOUBLEBUFFER) ? val != 0 : val == 0; break;
                case WGL_STEREO_ARB:              match = (fmt.pfd.dwFlags & PFD_STEREO) ? val != 0 : val == 0; break;
                case WGL_ACCELERATION_ARB:
                    switch (val) {
                    case WGL_NO_ACCELERATION_ARB:
                        if ((fmt.pfd.dwFlags & PFD_GENERIC_FORMAT) && !(fmt.pfd.dwFlags & PFD_GENERIC_ACCELERATED)) match = 1;
                        else match = 0;
                        break;
                    case WGL_GENERIC_ACCELERATION_ARB:
                        if ((fmt.pfd.dwFlags & PFD_GENERIC_FORMAT) && (fmt.pfd.dwFlags & PFD_GENERIC_ACCELERATED)) match = 1;
                        else match = 0;
                        break;
                    case WGL_FULL_ACCELERATION_ARB:
                        match = !(fmt.pfd.dwFlags & PFD_GENERIC_FORMAT);
                        break;
                    default:
                        match = 0;
                        break;
                    }
                    break;
                case WGL_SWAP_METHOD_ARB:
                    switch (val) {
                    case WGL_SWAP_EXCHANGE_ARB:  match = fmt.pfd.dwFlags & PFD_SWAP_EXCHANGE; break;
                    case WGL_SWAP_COPY_ARB:      match = fmt.pfd.dwFlags & PFD_SWAP_COPY; break;
                    case WGL_SWAP_UNDEFINED_ARB: break;
                    default:                     match = 0; break;
                    }
                    break;
                case WGL_PIXEL_TYPE_ARB:          match = val == (fmt.pfd.iPixelType != 0) + WGL_TYPE_RGBA_ARB; break;
                case WGL_NUMBER_OVERLAYS_ARB:     match = (GLuint)(fmt.pfd.bReserved & 0xf) >= (GLuint)val; break;
                case WGL_NUMBER_UNDERLAYS_ARB:    match = (GLuint)(fmt.pfd.bReserved >> 4) >= (GLuint)val; break;
                case WGL_COLOR_BITS_ARB:          match = (GLuint)fmt.pfd.cColorBits >= (GLuint)val; break;
                case WGL_RED_BITS_ARB:            match = (GLuint)fmt.pfd.cRedBits >= (GLuint)val; break;
                case WGL_GREEN_BITS_ARB:          match = (GLuint)fmt.pfd.cGreenBits >= (GLuint)val; break;
                case WGL_BLUE_BITS_ARB:           match = (GLuint)fmt.pfd.cBlueBits >= (GLuint)val; break;
                case WGL_ALPHA_BITS_ARB:          match = (GLuint)fmt.pfd.cAlphaBits >= (GLuint)val; break;
                case WGL_ACCUM_BITS_ARB:          match = (GLuint)fmt.pfd.cAccumBits >= (GLuint)val; break;
                case WGL_ACCUM_RED_BITS_ARB:      match = (GLuint)fmt.pfd.cAccumRedBits >= (GLuint)val; break;
                case WGL_ACCUM_GREEN_BITS_ARB:    match = (GLuint)fmt.pfd.cAccumGreenBits >= (GLuint)val; break;
                case WGL_ACCUM_BLUE_BITS_ARB:     match = (GLuint)fmt.pfd.cAccumBlueBits >= (GLuint)val; break;
                case WGL_ACCUM_ALPHA_BITS_ARB:    match = (GLuint)fmt.pfd.cAccumAlphaBits >= (GLuint)val; break;
                case WGL_DEPTH_BITS_ARB:          match = (GLuint)fmt.pfd.cDepthBits >= (GLuint)val; break;
                case WGL_STENCIL_BITS_ARB:        match = (GLuint)fmt.pfd.cStencilBits >= (GLuint)val; break;
                case WGL_AUX_BUFFERS_ARB:         match = (GLuint)fmt.pfd.cAuxBuffers >= (GLuint)val; break;
                case WGL_SAMPLE_BUFFERS_3DFX:     match = 1; break;
                case WGL_SAMPLES_3DFX:            match = fmt.samples >= (GLuint)val; break;
                }
            }
        }
        if (match) {
            found++;
            *formats++ = i;
        }
    }
    *numFormats = found;
    return TRUE;
}

/*
** wglext3dfx.c -- WGL_3DFX_multisample, GL_3DFX_multisample (glTBufferMask3DFX) and
** WGL_3DFX_gamma_control.  __stdcall entry points.
*/
#include <string.h>
#include "wglpriv.h"
#include "icdprotos.h"
#include <glide.h>

/* a real (hardware) gc is current: not NULL and not the null context, which has no hwcx / fx fields (fix) */
static __inline BOOL wglHasHwContext(__GLcontext *gc)
{
    return gc != NULL && gc != g_nullGC;
}

/* 69014590 -- wglQueryMultisample3DFX(samples): supported? (needs T-buffer and fullscreen) */
BOOL WINAPI wglQueryMultisample3DFX(int samples)
{
    __GLcontext *gc = __glCurrentGC();
    if (!wglHasHwContext(gc)) return FALSE;
    if (__glCurrentGC()->beginMode == 1) {      /* __GL_SETUP_NOT_IN_BEGIN: a second fetch */ __glSetError(GL_INVALID_OPERATION); return FALSE; }
    if (gc->hwcx.hasTBuffer && !gc->hwcx.windowed) {
        if (samples == 1) return TRUE;
        if (samples == 2 && grGetInteger(GR_NUM_FB) > 1) return TRUE;
    }
    return FALSE;
}

/* 69014610 -- wglSetMultisample3DFX(samples): 1 -> 2x, 2 -> 4x; reopens the Glide screen */
BOOL WINAPI wglSetMultisample3DFX(int samples)
{
    __GLcontext *gc = __glCurrentGC();
    GLuint n;                                   /* fx.numSamples is unsigned */
    HWND hwnd;
    if (!wglHasHwContext(gc)) return FALSE;
    if (__glCurrentGC()->beginMode == 1) {      /* __GL_SETUP_NOT_IN_BEGIN: a second fetch */ __glSetError(GL_INVALID_OPERATION); return FALSE; }
    if (!gc->hwcx.hasTBuffer || gc->hwcx.windowed) return FALSE;
    if (samples == 1) {
        n = 2;
        if (gc->fx.numSamples == n) goto done;
    } else {
        if (samples != 2) return FALSE;
        if (grGetInteger(GR_NUM_FB) <= 1) return FALSE;
        n = 4;
        if (gc->fx.numSamples == n) goto done;
    }
    gc->fx.numSamples = n;
    hwnd = GC_imports_wscx(gc)->window->hwnd;
    wglGlideLose(hwnd);
    wglGlideRestore(hwnd);
done:
    return TRUE;
}

/* 690146d0 -- glTBufferMask3DFX */
void APIENTRY glTBufferMask3DFX_impl(GLuint mask)
{
    __GLcontext *gc = __glCurrentGC();
    if (__glCurrentGC()->beginMode == 1) {      /* __GL_SETUP_NOT_IN_BEGIN: a second fetch */ __glSetError(GL_INVALID_OPERATION); return; }
    if (gc->hwcx.hasTBuffer && !gc->hwcx.windowed && gc->fx.numSamples != 0)
        ((GCfn_tbufmask)gc->hwcx.grTBufferWriteMaskExt)(mask);
}

/* 69014840 -- is any entry of a 3x256 WORD ramp non-zero? */
static BOOL __fastcall wglRampNonZero(__GLcontext *gc, const WORD *ramp)
{
    int i;
    (void)gc;
    for (i = 0; i < 0x300; i++) if (*ramp++ != 0) return TRUE;
    return FALSE;
}

/* 69014740 -- wglSetDeviceGammaRamp3DFX(hdc, WORD ramp[3][256]) (the hdc is not used) */
BOOL WINAPI wglSetDeviceGammaRamp3DFX(HDC hdc, LPVOID lpRamp)
{
    __GLcontext *gc = __glCurrentGC();
    const WORD *ramp = (const WORD *)lpRamp, *gp, *bp;
    GLuint r[256], g[256], b[256];
    int n, i, step;
    (void)hdc;
    if (!wglHasHwContext(gc)) return FALSE;
    if (__glCurrentGC()->beginMode == 1) {      /* __GL_SETUP_NOT_IN_BEGIN: a second fetch */ __glSetError(GL_INVALID_OPERATION); return FALSE; }
    if (wglRampNonZero(gc, ramp)) {
        memcpy(&gc->hwcx.gammaRamp[0][0], ramp, 0x600);
        n = grGetInteger(GR_GAMMA_TABLE_ENTRIES);
        step = 256 / n;
        gp = ramp + 256;
        bp = ramp + 512;
        for (i = 0; i < n; i++) {
            r[i] = ramp[i * step] >> 8;
            g[i] = gp[i * step] >> 8;
            b[i] = bp[i * step] >> 8;
        }
        grLoadGammaTable(n, r, g, b);
    }
    return TRUE;
}

/* 69014860 -- wglGetDeviceGammaRamp3DFX(hdc, WORD ramp[3][256]) (the hdc is not used) */
BOOL WINAPI wglGetDeviceGammaRamp3DFX(HDC hdc, LPVOID lpRamp)
{
    __GLcontext *gc = __glCurrentGC();
    WORD *ramp, *gp, *bp;
    GLuint r[256], g[256], b[256];
    int n, i, step;
    (void)hdc;
    if (!wglHasHwContext(gc)) return FALSE;
    if (__glCurrentGC()->beginMode == 1) {      /* __GL_SETUP_NOT_IN_BEGIN: a second fetch */ __glSetError(GL_INVALID_OPERATION); return FALSE; }
    if (gc->hwcx.hasGetGamma) {
        n = grGetInteger(GR_GAMMA_TABLE_ENTRIES);
        ((GCfn_getgamma)gc->hwcx.grGetGammaTableExt)(n, r, g, b);
        ramp = (WORD *)lpRamp;
        step = 256 / n;
        gp = ramp + 256;
        bp = ramp + 512;
        for (i = 0; i < n; i++) {
            ramp[i * step] = (WORD)(r[i] << 8);
            gp[i * step]   = (WORD)(g[i] << 8);
            bp[i * step]   = (WORD)(b[i] << 8);
        }
    } else {
        memcpy(lpRamp, &gc->hwcx.gammaRamp[0][0], 0x600);
    }
    return TRUE;
}

/* 69014960 -- re-apply gc's stored ramp after its Glide screen was reopened (fix: gc passed in, not the current one) */
void __fastcall wglReloadGammaRamp(__GLcontext *gc)
{
    const WORD *ramp = &gc->hwcx.gammaRamp[0][0], *gp, *bp;
    GLuint r[256], g[256], b[256];
    int n, i, step;
    if (!wglRampNonZero(gc, ramp)) return;
    n = grGetInteger(GR_GAMMA_TABLE_ENTRIES);
    step = 256 / n;
    gp = ramp + 256;
    bp = ramp + 512;
    for (i = 0; i < n; i++) {
        r[i] = ramp[i * step] >> 8;
        g[i] = gp[i * step] >> 8;
        b[i] = bp[i * step] >> 8;
    }
    grLoadGammaTable(n, r, g, b);
}

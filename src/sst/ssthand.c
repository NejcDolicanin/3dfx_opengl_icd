/*
** ssthand.c -- sst functions with indirect fastcall callbacks, COM vtable calls and
** partial-register results.
*/
#include <string.h>
#include "wglpriv.h"
#include "icdprotos.h"
#include <glide.h>
#include "icdglob.h"

void __fastcall winReleaseDDraw(void);

/* 6901c1f0 (size 64) -- winFreeDrawableBuffers: free the front, back and depth buffers of a window's drawable (each
** buffer's free proc, when set), then 6901c230 (the DirectDraw objects). */
void __fastcall winFreeDrawableBuffers(__WGLwindow *w)
{
    __WGLdrawable *dp = &w->dp;
    void (__fastcall *fn)(__WGLbuffer *buf, __WGLdrawable *dp);

    if ((fn = dp->frontBuffer.free) != 0) fn(&dp->frontBuffer, dp);
    if ((fn = dp->backBuffer.free) != 0) fn(&dp->backBuffer, dp);
    if ((fn = dp->depthBuffer.free) != 0) fn(&dp->depthBuffer, dp);
    winReleaseDDraw();
}

/* 6901c230 -- winReleaseDDraw: (size 147, its own function: 6901c1f0 tail-jumps into it): on the last user release
** the DirectDraw objects (and detach the Glide rendering surface), then drop the reference count */
void __fastcall winReleaseDDraw(void)
{
    if (g_winUsers == 1) {
        __GLcontext *gc = __glCurrentGC();
        IDirectDrawClipper_Release(g_winClipper);
        if (gc->hwcx.windowed != 0) {
            if ((gc->hwcx.glideVerFlags & 0xfffffffe) != 0)
                grSurfaceSetRenderingSurfaceExt2(0, 0);
            else
                grSurfaceSetRenderingSurfaceExt1(0);
        }
        IDirectDrawSurface2_Release(g_winPrimary);
        IDirectDraw2_Release(g_winDDraw);
        g_winClipper = NULL;
        g_winPrimary = NULL;
        g_winDDraw = NULL;
    }
    if (g_winUsers > 0)
        g_winUsers--;
}

/* 69065320 (size 127) -- hwPixelFormatCaps: capability bits for the pixel-format description.  bit0 always set
** ((x | 0x200) >> 9), bit1 = modeFlags 0x400 (__GL_SHADE_TWOSIDED), bit3 = enables.general 0x1000
** (__GL_CULL_FACE_ENABLE), 0x40 = a polygon offset enabled (0x1c0000, __GL_POLYGON_OFFSET_ANY_ENABLE).  The file
** includes neither glsi.h nor glshade.h, so the masks stay numbers. */
void __fastcall hwPixelFormatCaps(__GLcontext *gc, GLuint *out)
{
    GLuint extra = (gc->fx.fullscreen != 0 && gc->fx.guardBand != 0) ? 0x20 : 0;

    /* enables.general read twice in the expression (no local): VC6 still loads it once, before the call */
    *out = ((((*gc->polygon.shader.getModeFlags)(gc) & 0x400) | (gc->state.enables.general & 0x1000) | 0x200) >> 9)
         | (gc->hwcx.cmdTransportInfo != 0 ? 4 : 0)
         | ((gc->state.enables.general & 0x1c0000) != 0 ? 0x40 : 0)
         | (*(int *)&gc->state.enables.clipPlanes != 0 ? 0x10 : 0)
         | extra;
}

/* 69067da0 -- swapReturnAddrThunk (8 bytes): DrvSwapBuffers helper -- hands its own return address (the caller) to the
** checker at 69067cc9, which compares it with opengl32!wglSwapBuffers. */
__declspec(naked) int __fastcall swapReturnAddrThunk(int unused1, int unused2 /* decoration @8 as the callers expect */)
{
    __asm mov ecx, [esp]
    __asm jmp hwFinishFromCaller
}


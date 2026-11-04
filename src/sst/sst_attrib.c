/*
** sst_attrib.c -- state entries of the hardware path (__glsstim_*): alpha function, blend function and equation,
** depth function, draw / read buffer, fog, point parameters, glPopAttrib.
*/
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "icdglob.h"
#include "icdprotos.h"
#include "si_names.h"
#include <string.h>

#pragma warning(disable: 4273)
/************************************************************************/

/* set field i of Glide state record n, marking it dirty */
#define GRSET(n, i, v, bit)                                     \
    if (gc->hwcx.grState[n][i] != (v)) {                            \
        gc->hwcx.grState[n][i] = (v);                               \
        gc->hwcx.grDirty |= (bit);                                  \
    }
#define GR_ALPHATEST    16
#define GR_ALPHAREF     17
#define GR_DEPTHFUNC    1
typedef int (__fastcall *__GLGcFn)(__GLcontext *gc);
#define CALL1(f, gc)    ((__GLGcFn)(f))(gc)
/************************************************************************/
#define GR_RENDERBUFFER 14
/* the Glide fog records: grState[__GL_GR_FOG_COLOR] = the fog colour (packed ARGB), grState[__GL_GR_FOG] = {mode, density, start, end} */
#define GR_FOGCOLOR     24
#define GR_FOG          25
/************************************************************************/
typedef struct { GLfloat v[3]; } __GLVec3;
/************************************************************************/
/* glPopAttrib of the hardware path: the software one (gl_attrib.c, 690b7ef0) plus the Glide re-picks */

/* the attribute block glPushAttrib stacks is glctx.h's __GLattribute (gc->state, gc 0x58 .. 0x8a8,
** 0x850 bytes) with its __GLcurrentState (gc 0x60 .. 0x1a8) */

/* the gc->state field at the same place as `p` in the attribute block `sp` */
#define __GL_STATE_FIELD(gc, sp, p) \
    ((void *) (((GLubyte *) (p) - (GLubyte *) (sp)) + (GLubyte *) &(gc)->state))
#define __GL_LIGHT_SOURCE_SIZE  0x74            /* sizeof(__GLlightSourceState) in the ICD */
#define __GL_TEXENV_SIZE        0x54            /* sizeof the per-unit texture env block */
/* 690a0130: __glBindTexture(gc, unit/target, name) */

/* attrib.h enable groups (the ICD's bits) */
#define __GL_COLOR_BUFFER_ENABLES_ICD   0x00a0000f
#define __GL_LIGHTING_ENABLES_ICD       0x004000c0
#define __GL_LINE_ENABLES_ICD           0x00000300
#define __GL_POLYGON_ENABLES_ICD        0x001c3800
#define __GL_AUTO_NORMAL_ENABLE_ICD     0x00020000
#define __GL_NORMALIZE_ENABLE_ICD       0x00010000
/* ---- glReadBuffer (SI px_api.c; GL_FRONT on a windowed surface makes the front buffer readable) - */
extern GLenum __fastcall hwMapDepthFunc(GLenum func, GLenum dir, GLenum last);
extern void __stdcall __glsstim_Fogfv(GLenum p, const GLfloat pv[]);
extern void __stdcall __glsstim_Fogiv(GLenum p, const GLint pv[]);

#pragma optimize("y", off)      /* the original keeps the ebp frame here (no frame-pointer omission) */
/* 6901c4d0 (size 358) -- __glsstim_AlphaFunc: glAlphaFunc of the hardware path: the SI's state plus the Glide
** alpha test records grState[__GL_GR_ALPHA_TEST_FUNC] (function, GR_CMP_ALWAYS when disabled) and [17] (reference
** 0..255) */
void __stdcall __glsstim_AlphaFunc(GLenum func, GLfloat ref)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((func < GL_NEVER) || (func > GL_ALWAYS)) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    if (ref < __GL_ZERO_F) {
        ref = __GL_ZERO_F;
    } else if (ref > __GL_ONE_F) {
        ref = __GL_ONE_F;
    }
    gc->state.raster.alphaReference = ref;
    gc->state.raster.alphaFunction = func;
    if ((__GLfloat) *(GLuint *) gc->hwcx.grState[GR_ALPHAREF] != ref * 255.0f) {
        *(GLuint *) gc->hwcx.grState[GR_ALPHAREF] = (GLuint) (ref * 255.0f);
        gc->hwcx.grDirty |= 1 << GR_ALPHAREF;
    }
    if (gc->state.enables.general & __GL_ALPHA_TEST_ENABLE) {
        GRSET(GR_ALPHATEST, 0, (GLint)(func - GL_NEVER), 1 << GR_ALPHATEST);
    } else {
        GRSET(GR_ALPHATEST, 0, GR_CMP_ALWAYS, 1 << GR_ALPHATEST);
    }
    __GL_DELAY_VALIDATE(gc);
    gc->validateMask |= 1;
}
#pragma optimize("", on)

/************************************************************************/
#pragma optimize("y", off)
/* 6901c640 (size 495) -- __glsstim_BlendFunc: glBlendFunc of the hardware path: the factors and their Glide
** equivalents (no destination alpha: DST_ALPHA -> ONE, ONE_MINUS_DST_ALPHA / SRC_ALPHA_SATURATE -> ZERO) */
void __stdcall __glsstim_BlendFunc(GLenum sfactor, GLenum dfactor)
{
    GLuint src, dst;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (sfactor) {
      case GL_ONE: src = GR_BLEND_ONE; break;
      case GL_ZERO: src = GR_BLEND_ZERO; break;
      case GL_SRC_ALPHA: src = GR_BLEND_SRC_ALPHA; break;
      case GL_ONE_MINUS_SRC_ALPHA: src = GR_BLEND_ONE_MINUS_SRC_ALPHA; break;
      case GL_DST_ALPHA: src = gc->modes.alphaBits ? GR_BLEND_DST_ALPHA : GR_BLEND_ONE; break;
      case GL_DST_COLOR: src = GR_BLEND_DST_COLOR; break;
      case GL_ONE_MINUS_DST_COLOR: src = GR_BLEND_ONE_MINUS_DST_COLOR; break;
      case GL_ONE_MINUS_DST_ALPHA: src = gc->modes.alphaBits ? GR_BLEND_ONE_MINUS_DST_ALPHA : GR_BLEND_ZERO; break;
      case GL_SRC_ALPHA_SATURATE: src = gc->modes.alphaBits ? GR_BLEND_ALPHA_SATURATE : GR_BLEND_ZERO; break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    switch (dfactor) {
      case GL_ONE: dst = GR_BLEND_ONE; break;
      case GL_ZERO: dst = GR_BLEND_ZERO; break;
      case GL_SRC_COLOR: dst = GR_BLEND_SRC_COLOR; break;
      case GL_ONE_MINUS_SRC_COLOR: dst = GR_BLEND_ONE_MINUS_SRC_COLOR; break;
      case GL_SRC_ALPHA: dst = GR_BLEND_SRC_ALPHA; break;
      case GL_ONE_MINUS_SRC_ALPHA: dst = GR_BLEND_ONE_MINUS_SRC_ALPHA; break;
      case GL_DST_ALPHA: dst = gc->modes.alphaBits ? GR_BLEND_DST_ALPHA : GR_BLEND_ONE; break;
      case GL_ONE_MINUS_DST_ALPHA: dst = gc->modes.alphaBits ? GR_BLEND_ONE_MINUS_DST_ALPHA : GR_BLEND_ZERO; break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->state.raster.blendSrc = sfactor;
    gc->state.raster.blendDst = dfactor;
    gc->hwcx.blendSrcHw = src;
    gc->hwcx.blendDstHw = dst;
    if (gc->state.enables.general & __GL_BLEND_ENABLE) {
        GRSET(0, 0, src, 1);
        GRSET(0, 1, dst, 1);
    }
    __GL_DELAY_VALIDATE(gc);
}
#pragma optimize("", on)

#pragma optimize("y", off)
/* 6901c840 (size 188) -- __glsstim_BlendEquationEXT: glBlendEquationEXT of the hardware path */
void __stdcall __glsstim_BlendEquationEXT(GLenum mode)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (mode) {
      case 0x8006: gc->hwcx.blendEquationHw = 0; break;     /* GL_FUNC_ADD_EXT */
      case 0x800a: gc->hwcx.blendEquationHw = 1; break;     /* GL_FUNC_SUBTRACT_EXT */
      case 0x800b: gc->hwcx.blendEquationHw = 2; break;     /* GL_FUNC_REVERSE_SUBTRACT_EXT */
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->state.raster.blendEquation = mode;
    if (gc->state.enables.general & __GL_BLEND_ENABLE) {
        GRSET(0, 2, gc->hwcx.blendEquationHw, 1);
    }
    (*gc->procs.pickBlendProcs)(gc);
}
#pragma optimize("", on)

#pragma optimize("y", off)
/* 6901cca0 (size 302) -- __glsstim_DepthFunc: glDepthFunc of the hardware path: the Glide depth function record
** grState[__GL_GR_DEPTH_FUNC], mapped through 6901cb40 while the z-precision mode inverts the depth range,
** GR_CMP_ALWAYS in FX_GL_SUPER mode 1 */
void __stdcall __glsstim_DepthFunc(GLenum func)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((func < GL_NEVER) || (func > GL_ALWAYS)) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->state.depth.testFunc = func;
    if (gc->fx.fullscreen
        && ((gc->modes.depthBits > 16 && gc->fx.zPrecision32)
            || (gc->modes.depthBits == 16 && gc->fx.zPrecision16))) {
        GLint f = hwMapDepthFunc(func, gc->fx.depthDirection, gc->fx.lastDepthFunc);
        GRSET(GR_DEPTHFUNC, 0, f - GL_NEVER, 1 << GR_DEPTHFUNC);
    } else if (gc->fx.superMode == 1) {
        GRSET(GR_DEPTHFUNC, 0, GR_CMP_ALWAYS, 1 << GR_DEPTHFUNC);
    } else {
        GRSET(GR_DEPTHFUNC, 0, (GLint)(func - GL_NEVER), 1 << GR_DEPTHFUNC);
    }
    gc->fx.lastDepthFunc = func;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
}
#pragma optimize("", on)

#pragma optimize("y", off)
/* 6901ce60 (size 528) -- __glsstim_DrawBuffer: glDrawBuffer of the hardware path (the software one is 690b4fa0,
** gl_attrib.c): the Glide render buffer record, or the window surface's draw buffer when windowed */
void __stdcall __glsstim_DrawBuffer(GLenum mode)
{
    GLubyte *draw;
    __GL_SETUP_NOT_IN_BEGIN();

    draw = *(GLubyte **) gc->drawablePrivate;
    switch (mode) {
      case GL_NONE:
        gc->state.raster.drawBuffer = GL_NONE;
        break;
      case GL_FRONT:
      case GL_FRONT_LEFT:
        gc->state.raster.drawBuffer = GL_FRONT;
        if (gc->hwcx.windowed) {
            if (!draw[0x55]) {
                draw[0x55] = 1;
                winReattachFrontBuffer(gc);
            }
            CALL1(winSetFrontSurface, gc);
            gc->hwcx.depthFlag = 1;
        } else {
            GRSET(GR_RENDERBUFFER, 0, GR_BUFFER_FRONTBUFFER, 1 << GR_RENDERBUFFER);
        }
        break;
      case GL_FRONT_AND_BACK:
      case GL_LEFT:
        if (!gc->modes.doubleBufferMode) {
            gc->state.raster.drawBuffer = GL_FRONT;
        } else {
            gc->state.raster.drawBuffer = GL_FRONT_AND_BACK;
        }
        if (gc->hwcx.windowed) {
            if (!draw[0x55]) {
                draw[0x55] = 1;
                winReattachFrontBuffer(gc);
            }
            gc->hwcx.depthFlag = 1;
        }
        break;
      case GL_BACK:
      case GL_BACK_LEFT:
        if (!gc->modes.doubleBufferMode) {
            goto not_supported_in_this_implementation;
        }
        gc->state.raster.drawBuffer = GL_BACK;
        if (gc->hwcx.windowed) {
            CALL1(winSetBackSurface, gc);
        } else {
            GRSET(GR_RENDERBUFFER, 0, GR_BUFFER_BACKBUFFER, 1 << GR_RENDERBUFFER);
        }
        break;
      case GL_FRONT_RIGHT:
      case GL_BACK_RIGHT:
      case GL_RIGHT:
      not_supported_in_this_implementation:
        __glSetError(GL_INVALID_OPERATION);
        return;
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
        {
            GLint i = mode - GL_AUX0;
            if (i >= gc->modes.numAuxBuffers) {
                goto not_supported_in_this_implementation;
            }
            gc->state.raster.drawBuffer = mode;
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->state.raster.drawBufferReturn = mode;
    __GL_DELAY_VALIDATE(gc);
}
#pragma optimize("", on)

#pragma optimize("y", off)
/* 6901d090 (size 923) -- __glsstim_Fogfv: glFogfv of the hardware path (the software one is 690b5140, si_s_attrib.c):
** the SI's __glim_Fogfv plus the Glide fog records */
void __stdcall __glsstim_Fogfv(GLenum p, const GLfloat pv[])
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    if ((gc->fx.zPrecision32 || gc->fx.zPrecision16) && gc->fx.fullscreen) {
        gc->fx.zPrecision32 = 1;
        gc->fx.zPrecision16 = 1;
    }

    switch (p) {
      case GL_FOG_COLOR:
        {
            union { GLuint u; GLubyte b[4]; } fc;

            __glClampAndScaleColorf(gc, &gc->state.fog.color, pv);

            gc->state.fog.bufColor.r =
                gc->state.fog.color.r * gc->constants.redRescale;
            gc->state.fog.bufColor.g =
                gc->state.fog.color.g * gc->constants.greenRescale;
            gc->state.fog.bufColor.b =
                gc->state.fog.color.b * gc->constants.blueRescale;

            gc->state.fog.r =
                gc->state.fog.color.r * 255.0f * gc->constants.oneOverRedScale;
            gc->state.fog.g =
                gc->state.fog.color.g * 255.0f * gc->constants.oneOverGreenScale;
            gc->state.fog.b =
                gc->state.fog.color.b * 255.0f * gc->constants.oneOverBlueScale;

            fc.b[3] = (GLubyte) gc->state.fog.color.a;
            fc.b[2] = (GLubyte) gc->state.fog.color.r;
            fc.b[1] = (GLubyte) gc->state.fog.color.g;
            fc.b[0] = (GLubyte) gc->state.fog.color.b;
            if (*(GLuint *) gc->hwcx.grState[GR_FOGCOLOR] != fc.u) {
                *(GLuint *) gc->hwcx.grState[GR_FOGCOLOR] = fc.u;
                gc->hwcx.grDirty |= 1 << GR_FOGCOLOR;
            }
        }
        break;
      case GL_FOG_DENSITY:
        if (pv[0] < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        gc->state.fog.density = pv[0];
        if (((GLfloat *) gc->hwcx.grState[GR_FOG])[1] != pv[0]) {
            ((GLfloat *) gc->hwcx.grState[GR_FOG])[1] = pv[0];
            gc->hwcx.grDirty |= 1 << GR_FOG;
        }
        break;
      case GL_FOG_END:
        gc->state.fog.end = pv[0];
        if (((GLfloat *) gc->hwcx.grState[GR_FOG])[3] != pv[0]) {
            ((GLfloat *) gc->hwcx.grState[GR_FOG])[3] = pv[0];
            gc->hwcx.grDirty |= 1 << GR_FOG;
        }
        break;
      case GL_FOG_START:
        gc->state.fog.start = pv[0];
        if (((GLfloat *) gc->hwcx.grState[GR_FOG])[2] != pv[0]) {
            ((GLfloat *) gc->hwcx.grState[GR_FOG])[2] = pv[0];
            gc->hwcx.grDirty |= 1 << GR_FOG;
        }
        break;
      case GL_FOG_INDEX:
        {
            GLint fogIndex, indexMask;

            indexMask = (1 << gc->modes.indexBits) - 1;
            fogIndex = (GLint)pv[0];
            gc->state.fog.index = (__GLfloat)(fogIndex & indexMask);
            gc->state.fog.i = fogIndex & indexMask;
        }
        break;
      case GL_FOG_MODE:
        switch ((GLenum) pv[0]) {
          case GL_EXP:
          case GL_EXP2:
          case GL_LINEAR:
            gc->state.fog.mode = (GLenum) pv[0];
            if ((__GLfloat) *(GLuint *) gc->hwcx.grState[GR_FOG] != pv[0]) {
                *(GLuint *) gc->hwcx.grState[GR_FOG] = (GLuint) pv[0];
                gc->hwcx.grDirty |= 1 << GR_FOG;
            }
            (*gc->procs.pickFogProcs)(gc);
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case GL_FOG_COORDINATE_SOURCE_EXT:
        switch ((GLenum) pv[0]) {
          case GL_FOG_COORDINATE_EXT:
          case GL_FRAGMENT_DEPTH_EXT:
            gc->state.fog.coordSource = (GLenum) pv[0];
            gc->hwcx.grDirty |= 1 << GR_FOG;
            __GL_DELAY_VALIDATE(gc);
            (*gc->procs.pickFogProcs)(gc);
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    /*
    ** Recompute cached 1/(end - start) value for linear fogging.
    */
    if (gc->state.fog.mode == GL_LINEAR) {
        if (gc->state.fog.start != gc->state.fog.end) {
            gc->state.fog.oneOverEMinusS =
                __GL_ONE_F / (gc->state.fog.end - gc->state.fog.start);
        } else {
            gc->state.fog.oneOverEMinusS = __GL_ZERO_F;
        }
    }
}
#pragma optimize("", on)

/* 6901d450 -- __glsstim_Fogf: glFogf (hw) */
void __stdcall __glsstim_Fogf(GLenum p, GLfloat f)
{
    switch (p) {
      case GL_FOG_DENSITY:
      case GL_FOG_END:
      case GL_FOG_START:
      case GL_FOG_INDEX:
      case GL_FOG_MODE:
      case GL_FOG_COORDINATE_SOURCE_EXT:
        __glsstim_Fogfv(p, &f);
        return;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/************************************************************************/
#pragma optimize("y", off)      /* the original keeps the ebp frame here (no frame-pointer omission) */
/* 6901d490 (size 863) -- __glsstim_Fogiv: glFogiv of the hardware path (the software one is 690b53f0, si_s_attrib.c):
** the SI's __glim_Fogiv plus the Glide fog records */
void __stdcall __glsstim_Fogiv(GLenum p, const GLint pv[])
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    if ((gc->fx.zPrecision32 || gc->fx.zPrecision16) && gc->fx.fullscreen) {
        gc->fx.zPrecision32 = 1;
        gc->fx.zPrecision16 = 1;
    }

    switch (p) {
      case GL_FOG_COLOR:
        {
            union { GLuint u; GLubyte b[4]; } fc;

            __glClampAndScaleColori(gc, &gc->state.fog.color, pv);

            gc->state.fog.bufColor.r =
                gc->state.fog.color.r * gc->constants.redRescale;
            gc->state.fog.bufColor.g =
                gc->state.fog.color.g * gc->constants.greenRescale;
            gc->state.fog.bufColor.b =
                gc->state.fog.color.b * gc->constants.blueRescale;

            gc->state.fog.r =
                gc->state.fog.color.r * 255.0f * gc->constants.oneOverRedScale;
            gc->state.fog.g =
                gc->state.fog.color.g * 255.0f * gc->constants.oneOverGreenScale;
            gc->state.fog.b =
                gc->state.fog.color.b * 255.0f * gc->constants.oneOverBlueScale;

            fc.b[3] = (GLubyte) gc->state.fog.color.a;
            fc.b[2] = (GLubyte) gc->state.fog.color.r;
            fc.b[1] = (GLubyte) gc->state.fog.color.g;
            fc.b[0] = (GLubyte) gc->state.fog.color.b;
            if (*(GLuint *) gc->hwcx.grState[GR_FOGCOLOR] != fc.u) {
                *(GLuint *) gc->hwcx.grState[GR_FOGCOLOR] = fc.u;
                gc->hwcx.grDirty |= 1 << GR_FOGCOLOR;
            }
        }
        break;
      case GL_FOG_DENSITY:
        if (pv[0] < 0) {
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        gc->state.fog.density = pv[0];
        if (((GLfloat *) gc->hwcx.grState[GR_FOG])[1] != pv[0]) {
            ((GLfloat *) gc->hwcx.grState[GR_FOG])[1] = pv[0];
            gc->hwcx.grDirty |= 1 << GR_FOG;
        }
        break;
      case GL_FOG_END:
        gc->state.fog.end = pv[0];
        if (((GLfloat *) gc->hwcx.grState[GR_FOG])[3] != pv[0]) {
            ((GLfloat *) gc->hwcx.grState[GR_FOG])[3] = pv[0];
            gc->hwcx.grDirty |= 1 << GR_FOG;
        }
        break;
      case GL_FOG_START:
        gc->state.fog.start = pv[0];
        if (((GLfloat *) gc->hwcx.grState[GR_FOG])[2] != pv[0]) {
            ((GLfloat *) gc->hwcx.grState[GR_FOG])[2] = pv[0];
            gc->hwcx.grDirty |= 1 << GR_FOG;
        }
        break;
      case GL_FOG_INDEX:
        {
            GLint fogIndex, indexMask;

            indexMask = (1 << gc->modes.indexBits) - 1;
            fogIndex = (GLint)pv[0];
            gc->state.fog.index = (__GLfloat)(fogIndex & indexMask);
            gc->state.fog.i = fogIndex & indexMask;
        }
        break;
      case GL_FOG_MODE:
        switch ((GLenum) pv[0]) {
          case GL_EXP:
          case GL_EXP2:
          case GL_LINEAR:
            gc->state.fog.mode = (GLenum) pv[0];
            if (*(GLint *) gc->hwcx.grState[GR_FOG] != pv[0]) {
                *(GLint *) gc->hwcx.grState[GR_FOG] = pv[0];
                gc->hwcx.grDirty |= 1 << GR_FOG;
            }
            (*gc->procs.pickFogProcs)(gc);
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      case GL_FOG_COORDINATE_SOURCE_EXT:
        switch ((GLenum) pv[0]) {
          case GL_FOG_COORDINATE_EXT:
          case GL_FRAGMENT_DEPTH_EXT:
            gc->state.fog.coordSource = (GLenum) pv[0];
            gc->hwcx.grDirty |= 1 << GR_FOG;
            __GL_DELAY_VALIDATE(gc);
            (*gc->procs.pickFogProcs)(gc);
            break;
          default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    /*
    ** Recompute cached 1/(end - start) value for linear fogging.
    */
    if (gc->state.fog.mode == GL_LINEAR) {
        if (gc->state.fog.start != gc->state.fog.end) {
            gc->state.fog.oneOverEMinusS =
                __GL_ONE_F / (gc->state.fog.end - gc->state.fog.start);
        } else {
            gc->state.fog.oneOverEMinusS = __GL_ZERO_F;
        }
    }
}
#pragma optimize("", on)

/* 6901d810 -- __glsstim_Fogi: glFogi (hw) */
void __stdcall __glsstim_Fogi(GLenum p, GLint i)
{
    switch (p) {
      case GL_FOG_DENSITY:
      case GL_FOG_END:
      case GL_FOG_START:
      case GL_FOG_INDEX:
      case GL_FOG_MODE:
      case GL_FOG_COORDINATE_SOURCE_EXT:
        __glsstim_Fogiv(p, &i);
        return;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

#pragma optimize("y", off)      /* the original keeps the ebp frame here (no frame-pointer omission) */
/* 6901ff20 (size 216) -- __glsstim_PointParameterfvEXT: glPointParameterfvEXT of the hardware path */
void __stdcall __glsstim_PointParameterfvEXT(GLenum pname, const GLfloat *pv)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (pname) {
      case 0x8126:      /* GL_POINT_SIZE_MIN_EXT */
      case 0x8127:      /* GL_POINT_SIZE_MAX_EXT */
      case 0x8128:      /* GL_POINT_FADE_THRESHOLD_SIZE_EXT */
      case 0x8129:      /* GL_DISTANCE_ATTENUATION_EXT */
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    switch (pname) {
      case 0x8126: gc->state.point.ext[0] = pv[0]; break;
      case 0x8127: gc->state.point.ext[1] = pv[0]; break;
      case 0x8128: gc->state.point.ext[2] = pv[0]; break;
      case 0x8129:
        *(__GLVec3 *)&gc->state.point.ext[3] = *(const __GLVec3 *)pv;
        break;
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POINT);
}
#pragma optimize("", on)

/* 690200d0 (size 2533) -- __glsstim_PopAttrib: glPopAttrib of the hardware path (the software one is 690b7ef0,
** gl_attrib.c) */
void __fastcall __glsstim_PopAttrib(void)
{
    __GLattribute **spp;
    __GLattribute *sp;
    GLuint mask;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    spp = gc->attributes.stackPointer;
    if (spp > gc->attributes.stack) {
	--spp;
	sp = *spp;
	assert(sp != 0);
	mask = sp->mask;
	gc->attributes.stackPointer = spp;
	if (mask & GL_ACCUM_BUFFER_BIT) {
	    gc->state.accum = sp->accum;
	}
	if (mask & GL_COLOR_BUFFER_BIT) {
	    gc->state.raster = sp->raster;
	    gc->state.enables.general &= ~__GL_COLOR_BUFFER_ENABLES_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_COLOR_BUFFER_ENABLES_ICD;
	    gc->validateMask |= 0x9;
	}
	if (mask & GL_CURRENT_BIT) {
	    gc->state.current = sp->current;
	}
	if (mask & GL_DEPTH_BUFFER_BIT) {
	    gc->state.depth = sp->depth;
	    gc->state.enables.general &= ~__GL_DEPTH_TEST_ENABLE;
	    gc->dirtyMask |= __GL_DIRTY_HWDEPTH_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_DEPTH_TEST_ENABLE;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
	    (*gc->procs.pickAllProcsPhase1)(gc);
	    (*gc->procs.pickBufferProcs)(gc);
	}
	if (mask & GL_ENABLE_BIT) {
	    gc->state.enables = sp->enables;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE | __GL_DIRTY_POLYGON |
		    __GL_DIRTY_POINT | __GL_DIRTY_PIXEL | __GL_DIRTY_LIGHTING);
	    (*gc->procs.pickAllProcsPhase1)(gc);
	    (*gc->procs.pickColorMaterialProcs)(gc);
	    (*gc->procs.applyColor)(gc);
	    (*gc->procs.computeClipBox)(gc);
	    (*gc->procs.applyScissor)(gc);
	    wglGlideSetupBufferModes(gc);
	}
	if (mask & GL_EVAL_BIT) {
	    gc->state.evaluator = sp->evaluator;
	    gc->state.enables.general &= ~__GL_AUTO_NORMAL_ENABLE_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_AUTO_NORMAL_ENABLE_ICD;
	    gc->state.enables.eval1 = sp->enables.eval1;
	    gc->state.enables.eval2 = sp->enables.eval2;
	}
	if (mask & GL_FOG_BIT) {
	    gc->state.fog = sp->fog;
	    gc->state.enables.general &= ~__GL_FOG_ENABLE;
	    gc->dirtyMask |= __GL_DIRTY_HWFOG_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_FOG_ENABLE;
	    (*gc->procs.pickFogProcs)(gc);
	}
	if (mask & GL_HINT_BIT) {
	    gc->state.hints = sp->hints;
	}
	if (mask & GL_LIGHTING_BIT) {
	    gc->state.light.colorMaterialFace = sp->light.colorMaterialFace;
	    gc->state.light.colorMaterialParam = sp->light.colorMaterialParam;
	    gc->state.light.indexMaterialFace = sp->light.indexMaterialFace;
	    gc->state.light.indexMaterialParam = sp->light.indexMaterialParam;
	    if (gc->state.light.shadingModel != sp->light.shadingModel) {
		gc->state.light.shadingModel = sp->light.shadingModel;
		gc->dirtyMask |= __GL_DIRTY_SHADEMODEL_ICD;
	    }
	    gc->state.light.model = sp->light.model;
	    gc->state.light.front = sp->light.front;
	    gc->state.light.back = sp->light.back;
	    __GL_MEMCOPY(gc->state.light.source, sp->light.source,
			 gc->constants.numberOfLights
			     * __GL_LIGHT_SOURCE_SIZE);
	    __GL_FREE(gc, sp->light.source);
	    sp->light.source = NULL;
	    gc->state.enables.general &= ~__GL_LIGHTING_ENABLES_ICD;
	    gc->dirtyMask |= __GL_DIRTY_HWLIGHT_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_LIGHTING_ENABLES_ICD;
	    gc->state.enables.lights = sp->enables.lights;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
	}
	if (mask & GL_LINE_BIT) {
	    gc->state.line = sp->line;
	    gc->state.enables.general &= ~__GL_LINE_ENABLES_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_LINE_ENABLES_ICD;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
	}
	if (mask & GL_LIST_BIT) {
	    gc->state.list = sp->list;
	}
	if (mask & GL_PIXEL_MODE_BIT) {
	    gc->state.pixel.transferMode = sp->pixel.transferMode;
	    gc->state.pixel.readBufferReturn = sp->pixel.readBufferReturn;
	    gc->state.pixel.readBuffer = sp->pixel.readBuffer;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_PIXEL);
	}
	if (mask & GL_POINT_BIT) {
	    gc->state.point = sp->point;
	    gc->state.enables.general &= ~__GL_POINT_SMOOTH_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_POINT_SMOOTH_ENABLE;
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POINT);
	}
	if (mask & GL_POLYGON_BIT) {
	    gc->state.polygon = sp->polygon;
	    gc->state.enables.general &= ~__GL_POLYGON_ENABLES_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_POLYGON_ENABLES_ICD;
	    wglGlideSetupBufferModes(gc);
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
	}
	if (mask & GL_POLYGON_STIPPLE_BIT) {
	    gc->state.polygonStipple = sp->polygonStipple;
	    gc->state.enables.general &= ~__GL_POLYGON_STIPPLE_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_POLYGON_STIPPLE_ENABLE;
	    (*gc->procs.convertPolygonStipple)(gc);
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
	}
	if (mask & GL_SCISSOR_BIT) {
	    gc->state.scissor = sp->scissor;
	    gc->state.enables.general &= ~__GL_SCISSOR_TEST_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_SCISSOR_TEST_ENABLE;
	    (*gc->procs.computeClipBox)(gc);
	    (*gc->procs.applyScissor)(gc);
	}
	if (mask & GL_STENCIL_BUFFER_BIT) {
	    gc->state.stencil = sp->stencil;
	    gc->state.enables.general &= ~__GL_STENCIL_TEST_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_STENCIL_TEST_ENABLE;
	    gc->validateMask |= 0x6;
	    (*gc->procs.pickBufferProcs)(gc);
	}
	if (mask & GL_TEXTURE_BIT) {
	    GLint i;
	    GLuint j, targets = gc->constants.numberOfTextures;
	    const __GLtexParamICD *tbsSrc;

	    /* texture unit state */
	    for (i = 0; i < gc->fx.numTMUs; i++) {
		/* TexGen state */
		gc->state.texture.unit[i].s = sp->texture.unit[i].s;
		gc->state.texture.unit[i].t = sp->texture.unit[i].t;
		gc->state.texture.unit[i].r = sp->texture.unit[i].r;
		gc->state.texture.unit[i].q = sp->texture.unit[i].q;
		gc->state.texture.unit[i].scaleBias[0][0] = sp->texture.unit[i].scaleBias[0][0];
		gc->state.texture.unit[i].scaleBias[0][1] = sp->texture.unit[i].scaleBias[0][1];
		gc->state.texture.unit[i].scaleBias[0][2] = sp->texture.unit[i].scaleBias[0][2];
		gc->state.texture.unit[i].scaleBias[0][3] = sp->texture.unit[i].scaleBias[0][3];
		gc->state.texture.unit[i].scaleBias[1][0] = sp->texture.unit[i].scaleBias[1][0];
		gc->state.texture.unit[i].scaleBias[1][1] = sp->texture.unit[i].scaleBias[1][1];
		gc->state.texture.unit[i].scaleBias[1][2] = sp->texture.unit[i].scaleBias[1][2];
		gc->state.texture.unit[i].scaleBias[1][3] = sp->texture.unit[i].scaleBias[1][3];
	    }

	    /*
	    ** Texture binding state.  If the texture name is different, a new
	    ** binding is called for.
	    */
	    tbsSrc = sp->texture.unit[0].param;
	    {
		const __GLtexParamICD *pts, *spPts;

		pts = (const __GLtexParamICD *) __GL_STATE_FIELD(gc, sp, sp->texture.unit[0].param);
		spPts = sp->texture.unit[0].param;
		for (j = 0; j < targets; j++, pts++, spPts++) {
		    if (pts->name != spPts->name) {
			__glBindTexture(gc, j, spPts->name);
		    }
		}
	    }
	    __GL_MEMCOPY(gc->state.texture.unit[0].param, tbsSrc,
			 targets * sizeof(__GLtexParamICD));

	    /* TexEnv state */
	    __GL_MEMCOPY(gc->state.texture.unit[0].env, sp->texture.unit[0].env,
			 gc->constants.numberOfTextureEnvs * __GL_TEXENV_SIZE);
	    __GL_FREE(gc, sp->texture.unit[0].env);
	    sp->texture.unit[0].env = NULL;

	    /* texture enable state */
	    gc->state.enables.texture[0] &= ~(__GL_TEXTURE_1D_ENABLE | __GL_TEXTURE_2D_ENABLE | __GL_TEXTURE_GEN_ANY_ENABLE);
	    gc->state.enables.texture[0] |= sp->enables.texture[0] & (__GL_TEXTURE_1D_ENABLE | __GL_TEXTURE_2D_ENABLE | __GL_TEXTURE_GEN_ANY_ENABLE);
	}
	if (mask & GL_TRANSFORM_BIT) {
	    gc->state.transform.matrixMode = sp->transform.matrixMode;
	    __GL_MEMCOPY(gc->state.transform.eyeClipPlanes,
			 sp->transform.eyeClipPlanes,
			 gc->constants.numberOfClipPlanes * sizeof(__GLcoord));
	    __GL_FREE(gc, sp->transform.eyeClipPlanes);
	    sp->transform.eyeClipPlanes = NULL;
	    gc->state.enables.general &= ~__GL_NORMALIZE_ENABLE_ICD;
	    gc->state.enables.general |=
		sp->enables.general & __GL_NORMALIZE_ENABLE_ICD;
	    gc->state.enables.clipPlanes = sp->enables.clipPlanes;
	    (*gc->procs.pickTransformProcs)(gc);
	    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_CLIPPLANE_ICD);
	}
	if (mask & GL_VIEWPORT_BIT) {
	    gc->state.viewport = sp->viewport;
	    (*gc->procs.applyViewport)(gc);
	}

	/*
	** Clear out mask so that any memory frees done above won't get
	** re-done when the context is destroyed
	*/
	sp->mask = 0;

	__GL_DELAY_VALIDATE(gc);
	hwLoadAllState(gc);
    } else {
	__glSetError(GL_STACK_UNDERFLOW);
	return;
    }
}

/* 6905fdd0 (size 290) -- __glsstim_ReadBuffer: glReadBuffer */
void __stdcall __glsstim_ReadBuffer(GLenum mode)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (mode) {
      case GL_FRONT:
      case GL_LEFT:
      case GL_FRONT_LEFT:
        gc->state.pixel.readBuffer = GL_FRONT;
        if (gc->hwcx.windowed && !(*(GLubyte **)gc->drawablePrivate)[0x55]) {
            (*(GLubyte **)gc->drawablePrivate)[0x55] = 1;
            winReattachFrontBuffer(gc);
            winReadBack(gc);
            winPresent(gc);
        }
        break;
      case GL_BACK:
      case GL_BACK_LEFT:
        if (!gc->modes.doubleBufferMode) {
            goto invalid_buffer;
        }
        gc->state.pixel.readBuffer = GL_BACK;
        break;
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
      aux_4_through_246:
        {
            GLint i = mode - GL_AUX0;
            if (i >= gc->modes.numAuxBuffers) {
                goto invalid_buffer;
            }
            gc->state.pixel.readBuffer = mode;
        }
        break;
      case GL_FRONT_RIGHT:
      case GL_BACK_RIGHT:
      case GL_RIGHT:
      invalid_buffer:
        __glSetError(GL_INVALID_OPERATION);
        return;
      default:
        if (mode > GL_AUX3 && mode < GL_AUX0 + 247) {
            goto aux_4_through_246;
        }
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->state.pixel.readBufferReturn = mode;
    __GL_DELAY_VALIDATE_MASK(gc, 0x11);
}

/*
** 6901cb40 (size 344) -- hwMapDepthFunc: the depth function to program for a depth direction (gc->fx.depthDirection,
** GL_LEQUAL or GL_GEQUAL) and the last depth function: GL_NEVER / GL_ALWAYS / GL_EQUAL / GL_NOTEQUAL
** stay, the ordered ones keep or flip their direction.  The mix of result variable and direct returns
** reproduces the original's shared return blocks and cross-jumped tails.
*/
GLenum __fastcall hwMapDepthFunc(GLenum func, GLenum dir, GLenum last)
{
    GLenum r = func;

    if (func == GL_NEVER || func == GL_ALWAYS || func == GL_EQUAL || func == GL_NOTEQUAL)
        return func;
    if (dir == GL_LEQUAL) {
        if (func == GL_LESS) {
            r = (last == GL_GREATER || last == GL_GEQUAL) ? GL_GREATER : GL_LESS;
        } else if (func == GL_LEQUAL) {
            if (last == GL_GREATER || last == GL_GEQUAL) return GL_GEQUAL;
            return GL_LEQUAL;
        } else if (func == GL_GREATER) {
            if (last == GL_LESS || last == GL_LEQUAL) r = GL_GREATER;
            else return GL_LESS;
        } else if (func == GL_GEQUAL) {
            if (last == GL_LESS || last == GL_LEQUAL) return GL_GEQUAL;
            return GL_LEQUAL;
        }
    } else if (dir == GL_GEQUAL) {
        if (func == GL_LESS) {
            r = (last == GL_GREATER || last == GL_GEQUAL) ? GL_LESS : GL_GREATER;
        } else if (func == GL_LEQUAL) {
            if (last == GL_GREATER || last == GL_GEQUAL) return GL_LEQUAL;
            return GL_GEQUAL;
        } else if (func == GL_GREATER) {
            if (last == GL_LESS || last == GL_LEQUAL) return GL_LESS;
            return GL_GREATER;
        } else if (func == GL_GEQUAL) {
            return (last == GL_LESS || last == GL_LEQUAL) ? GL_LEQUAL : GL_GEQUAL;
        }
    }
    return r;
}

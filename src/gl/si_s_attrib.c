/*
** si_s_attrib.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_attrib.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 690b6470 -- __glErrorCheckMaterial */
GLenum __fastcall __glErrorCheckMaterial(GLenum face, GLenum p, GLfloat pv0)
{
    switch (face) {
      case GL_FRONT:
      case GL_BACK:
      case GL_FRONT_AND_BACK:
        break;
      default:
        return GL_INVALID_ENUM;
    }
    switch (p) {
      case GL_INDEX_SHIFT: case GL_INDEX_OFFSET:       /* ICD: accepted as well (690b64a2) */
      case GL_COLOR_INDEXES:
      case GL_EMISSION:
      case GL_SPECULAR:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_AMBIENT_AND_DIFFUSE:
        break;
      case GL_SHININESS:
        if (pv0 < 0 || pv0 > 128) {
            return GL_INVALID_VALUE;
        }
        break;
      default:
        return GL_INVALID_ENUM;
    }
    return GL_NO_ERROR;
}

/* 690b4780 -- __glim_BlendFunc */
void __stdcall __glim_BlendFunc(GLenum sf, GLenum df)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (sf) {
      case GL_ZERO:
      case GL_ONE:
      case GL_DST_COLOR:
      case GL_ONE_MINUS_DST_COLOR:
      case GL_SRC_ALPHA:
      case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA:
      case GL_ONE_MINUS_DST_ALPHA:
      case GL_SRC_ALPHA_SATURATE:
#if defined(__GL_SUPPORT_ARB_IMAGING_SUBSET)
      case GL_CONSTANT_COLOR:
      case GL_ONE_MINUS_CONSTANT_COLOR:
      case GL_CONSTANT_ALPHA:
      case GL_ONE_MINUS_CONSTANT_ALPHA:
#endif /* __GL_SUPPORT_ARB_IMAGING_SUBSET */        
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    switch (df) {
      case GL_ZERO:
      case GL_ONE:
      case GL_SRC_COLOR:
      case GL_ONE_MINUS_SRC_COLOR:
      case GL_SRC_ALPHA:
      case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA:
      case GL_ONE_MINUS_DST_ALPHA:
#if defined(__GL_SUPPORT_ARB_IMAGING_SUBSET)
      case GL_CONSTANT_COLOR:
      case GL_ONE_MINUS_CONSTANT_COLOR:
      case GL_CONSTANT_ALPHA:
      case GL_ONE_MINUS_CONSTANT_ALPHA:
#endif /* __GL_SUPPORT_ARB_IMAGING_SUBSET */        
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    __GL_API_STATE();

    gc->state.raster.blendSrc = sf;
    gc->state.raster.blendDst = df;
    __GL_DELAY_VALIDATE(gc);
}

/* 690b4cd0 -- __glim_ColorMask */
void __stdcall __glim_ColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    gc->state.raster.rMask = r;
    gc->state.raster.gMask = g;
    gc->state.raster.bMask = b;
    gc->state.raster.aMask = a;
    __GL_DELAY_VALIDATE(gc);
}

/* 690b4e10 -- __glim_CullFace */
void __stdcall __glim_CullFace(GLenum cfm)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (cfm) {
      case GL_FRONT:
      case GL_BACK:
      case GL_FRONT_AND_BACK:
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    __GL_API_STATE();

    gc->state.polygon.cull = cfm;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
}

/* 690b4ea0 -- __glim_DepthFunc */
void __stdcall __glim_DepthFunc(GLenum zf)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    if ((zf < GL_NEVER) || (zf > GL_ALWAYS)) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    gc->state.depth.testFunc = zf;

    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
}

/* 690b4f30 -- __glim_DepthMask */
void __stdcall __glim_DepthMask(GLboolean enabled)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    gc->state.depth.writeEnable = enabled;

    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
}

/* 690b5680 -- __glim_FrontFace */
void __stdcall __glim_FrontFace(GLenum dir)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (dir) {
      case GL_CW:
      case GL_CCW:
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    __GL_API_STATE();

    gc->state.polygon.frontFaceDirection = dir;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
}

/* 690b6200 -- __glim_LineStipple */
void __stdcall __glim_LineStipple(GLint factor, GLushort stipple)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (factor < 1) {
        factor = 1;
    }
    if (factor > 256) {
        factor = 256;
    }

    __GL_API_STATE();

    gc->state.line.stippleRepeat = (GLshort) factor;
    gc->state.line.stipple = stipple;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
}

/* 690b63e0 -- __glim_LogicOp */
void __stdcall __glim_LogicOp(GLenum op)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((op < GL_CLEAR) || (op > GL_SET)) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    __GL_API_STATE();

    gc->state.raster.logicOp = op;
    __GL_DELAY_VALIDATE(gc);
}

/* 690b6c80 -- __glim_PolygonMode */
void __stdcall __glim_PolygonMode(GLenum face, GLenum mode)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    switch (mode) {
      case GL_FILL:
        break;
      case GL_POINT:
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POINT);
        break;
      case GL_LINE:
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    switch (face) {
      case GL_FRONT:
        gc->state.polygon.frontMode = mode;
        break;
      case GL_BACK:
        gc->state.polygon.backMode = mode;
        break;
      case GL_FRONT_AND_BACK:
        gc->state.polygon.frontMode = mode;
        gc->state.polygon.backMode = mode;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
}

/* 690b4d60 -- __glim_ColorMaterial */
void __stdcall __glim_ColorMaterial(GLenum face, GLenum p)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (face) {
      case GL_FRONT:
      case GL_BACK:
      case GL_FRONT_AND_BACK:
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    switch (p) {
      case GL_EMISSION:
      case GL_SPECULAR:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_AMBIENT_AND_DIFFUSE:
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    __GL_API_STATE();

    gc->state.light.colorMaterialFace = face;
    gc->state.light.colorMaterialParam = p;

    if (gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE) {
        (*gc->procs.pickColorMaterialProcs)(gc);
        (*gc->procs.applyColor)(gc);
    }
}

/* 690b4980 -- __glim_ClearColor */
void __stdcall __glim_ClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    __GLfloat zero;
    __GLfloat one;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    zero = (__GLfloat)__GL_ZERO_F;
    one = (__GLfloat)__GL_ONE_F;
    if (r < zero) r = zero;
    if (r > one) r = one;
    if (g < zero) g = zero;
    if (g > one) g = one;
    if (b < zero) b = zero;
    if (b > one) b = one;
    if (a < zero) a = zero;
    if (a > one) a = one;

    gc->state.raster.clear.r = r;
    gc->state.raster.clear.g = g;
    gc->state.raster.clear.b = b;
    gc->state.raster.clear.a = a;
}

/* 690b4be0 -- __glim_ClearIndex */
void __stdcall __glim_ClearIndex(GLfloat val)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    val = __GL_MASK_INDEXF(gc, val);
    gc->state.raster.clearIndex = val;
}

/* 690b6370 -- ClampWidth */
__GLfloat __fastcall ClampWidth(__GLcontext *gc, __GLfloat size)
{
    __GLfloat minSize = gc->constants.lineWidthMinimum;
    __GLfloat maxSize = gc->constants.lineWidthMaximum;
    __GLfloat gran = gc->constants.lineWidthGranularity;
    GLint i;

    if (size <= minSize) return minSize;
    if (size >= maxSize) return maxSize;

    /* choose closest fence post */
    i = (GLint)(((size - minSize) / gran) + __glHalf);
    return minSize + i * gran;
}

/* 690b6c10 -- ClampSize */
__GLfloat __fastcall ClampSize(__GLcontext *gc, __GLfloat size)
{
    __GLfloat minSize = gc->constants.pointSizeMinimum;
    __GLfloat maxSize = gc->constants.pointSizeMaximum;
    __GLfloat gran = gc->constants.pointSizeGranularity;
    GLint i;

    if (size <= minSize) return minSize;
    if (size >= maxSize) return maxSize;

    /* choose closest fence post */
    i = (GLint)(((size - minSize) / gran) + __glHalf);
    return minSize + i * gran;
}

/* 690b4830 -- __glim_ClearAccum */
void __stdcall __glim_ClearAccum(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    __GLfloat minusOne;
    __GLfloat one;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    minusOne = __glMinusOne;
    one = __GL_ONE_F;
    if (r < minusOne) r = minusOne;
    if (r > one) r = one;
    if (g < minusOne) g = minusOne;
    if (g > one) g = one;
    if (b < minusOne) b = minusOne;
    if (b > one) b = one;
    if (a < minusOne) a = minusOne;
    if (a > one) a = one;

    gc->state.accum.clear.r = r;
    gc->state.accum.clear.g = g;
    gc->state.accum.clear.b = b;
    gc->state.accum.clear.a = a;

    __GL_DELAY_VALIDATE(gc);
}

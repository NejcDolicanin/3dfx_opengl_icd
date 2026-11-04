/*
** si_px_api.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/px_api.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 690c04a0 -- __glim_ReadBuffer */
void __stdcall __glim_ReadBuffer(GLenum mode)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    switch (mode) {
      case GL_FRONT:
      case GL_LEFT:
      case GL_FRONT_LEFT:
        gc->state.pixel.readBuffer = GL_FRONT;
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
        if (mode > GL_AUX3 && mode < GL_AUX0+247) {
            goto aux_4_through_246;
        }
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->state.pixel.readBufferReturn = mode;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_GENERIC | __GL_DIRTY_PIXEL);
}

/* 690c0850 -- __glim_DrawPixels */
void __stdcall __glim_DrawPixels(GLsizei width, GLsizei height, GLenum format,
                       GLenum type, const GLvoid *pixels)
{
    GLuint beginMode;
    __GL_SETUP();

    beginMode = __gl_beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
        if (beginMode == __GL_NEED_VALIDATE) {
            (*gc->procs.validate)(gc);
            __gl_beginMode = __GL_NOT_IN_BEGIN;
            glDrawPixels(width,height,format,type,pixels);
            return;
        } else {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
    }

    if (!__glCheckDrawPixelArgs(gc, width, height, format, type)) return;
    if (!gc->state.current.validRasterPos) {
        return;
    }

    if (gc->renderMode == GL_FEEDBACK) {
        __glFeedbackDrawPixels(gc, &gc->state.current.rasterPos);
        return;
    }

    if (gc->renderMode != GL_RENDER) return;

    (*gc->procs.revalidate)(gc);
    (*gc->procs.waitIdle)(gc);    /* ICD: the pixel-op sync (690c091d) */
    (*gc->procs.drawPixels)(gc, width, height, format, type, pixels, GL_FALSE);
}

/* 690c0950 -- __glim_ReadPixels */
void __stdcall __glim_ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, 
                       GLenum format, GLenum type, GLvoid *buf)
{
    GLuint beginMode;
    __GL_SETUP();

    beginMode = __gl_beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
        if (beginMode == __GL_NEED_VALIDATE) {
            (*gc->procs.validate)(gc);
            __gl_beginMode = __GL_NOT_IN_BEGIN;
            glReadPixels(x,y,width,height,format,type,buf);
            return;
        } else {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
    }

    if (!__glCheckReadPixelArgs(gc, width, height, format, type)) return;

    (*gc->procs.revalidate)(gc);
    (*gc->procs.waitIdle)(gc);    /* ICD: the pixel-op sync */
    (*gc->procs.readPixels)(gc, x, y, width, height, format, type, buf);
}

/* 690bf5b0 -- __glInitDefaultPixelMap */
void __fastcall __glInitDefaultPixelMap(__GLcontext *gc, GLenum map)
{
    __GLpixelState *ps = &gc->state.pixel;
    __GLpixelMapHead *pMap = ((__GLpixelMapHead *)gc->state.pixel.map);
    GLint index = map - GL_PIXEL_MAP_I_TO_I;
    
    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
        /*
        ** Allocate single-entry map for index type.
        */
        if (!(pMap[index].base.mapI = (GLint*)
              ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, sizeof(GLint)))) {
            return;
        } else {
            pMap[index].base.mapI[0] = 0;
            pMap[index].size = 1;
        }
        break;
      case GL_PIXEL_MAP_I_TO_R: case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B: case GL_PIXEL_MAP_I_TO_A:
      case GL_PIXEL_MAP_R_TO_R: case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B: case GL_PIXEL_MAP_A_TO_A:
        /*
        ** Allocate single-entry map for component type.
        */
        if (!(pMap[index].base.mapF = (__GLfloat*)
              ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, sizeof(__GLfloat)))) {
            return;
        } else {
            pMap[index].base.mapF[0] = __GL_ZERO_F;
            pMap[index].size = 1;
        }
        break;
    default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690bf640 -- __glPixelSetColorScales */
void __fastcall __glPixelSetColorScales(__GLcontext *gc)
{
    __GLpixelMachineICD *pm = &gc->pixel;
    GLfloat redScale, greenScale, blueScale, alphaScale;
    int i;
    GLint mask;

    if (pm->redMap == NULL) {
        /* First time allocation of these maps */

        /* 
        ** These lookup tables are for type UNSIGNED_BYTE, so they are sized
        ** to 256 entries.  They map from UNSIGNED_BYTE to internal scaled 
        ** floating point colors.
        */
        pm->redMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->greenMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->blueMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->alphaMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->iMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
    }

    redScale = gc->frontBuffer.redScale / 255;
    greenScale = gc->frontBuffer.greenScale / 255;
    blueScale = gc->frontBuffer.blueScale / 255;
    alphaScale = gc->frontBuffer.alphaScale / 255;
    mask = gc->frontBuffer.redMax;
    for (i=0; i<256; i++) {
        pm->redMap[i] = i * redScale;
        pm->greenMap[i] = i * greenScale;
        pm->blueMap[i] = i * blueScale;
        pm->alphaMap[i] = i * alphaScale;
        pm->iMap[i] = (GLfloat) (i & mask);
    }

    /*
    ** Invalidate the RGBA modify tables so that they will be
    ** recomputed using the current color buffer scales.
    */
    pm->rgbaCurrent = GL_FALSE;
}

/* 690bfee0 -- __glim_PixelMapfv */
void __stdcall __glim_PixelMapfv(GLenum map, GLint mapSize,
                       const GLfloat values[])
{
    __GLpixelState *ps;
    __GLpixelMapHead *pMap;
    GLint index = map - GL_PIXEL_MAP_I_TO_I;
    GLfloat value;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    ps = &gc->state.pixel;
    pMap = ((__GLpixelMapHead *)gc->state.pixel.map);

    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapI) {
            ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, pMap[index].base.mapI);
            pMap[index].base.mapI = 0;
        }
        pMap[index].base.mapI = (GLint*)
            ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, (size_t) (mapSize * sizeof(GLint)));
        if (!pMap[index].base.mapI) {
            pMap[index].size = 0;
            return;
        }
        pMap[index].size = mapSize;
        while (--mapSize >= 0) {
            value = values[mapSize];
            if (value > 0) {                /* round! */
                pMap[index].base.mapI[mapSize] =
                    (GLint)(value + __glHalf);
            } else {
                pMap[index].base.mapI[mapSize] =
                    (GLint)(value - __glHalf);
            }
        }
        break;
      case GL_PIXEL_MAP_I_TO_R:
      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:
      case GL_PIXEL_MAP_I_TO_A:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
      case GL_PIXEL_MAP_R_TO_R:
      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:
      case GL_PIXEL_MAP_A_TO_A:
        if (mapSize < 0) {
            /*
            ** Maps indexed by color component must not have negative size.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapF) {
            ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, pMap[index].base.mapF);
            pMap[index].base.mapF = 0;
        }
        if (mapSize == 0) {
            __glInitDefaultPixelMap(gc, map);
        } else {
            pMap[index].base.mapF = (__GLfloat*)
                ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc,
                                      (size_t) (mapSize * sizeof(__GLfloat)));
            if (!pMap[index].base.mapF) {
                pMap[index].size = 0;
                return;
            }
            pMap[index].size = mapSize;
            while (--mapSize >= 0) {
                value = values[mapSize];
                if (value < __GL_ZERO_F) value = __GL_ZERO_F;
                else if (value > __GL_ONE_F) value = __GL_ONE_F;
                pMap[index].base.mapF[mapSize] = value;
            }
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690c00f0 -- __glim_PixelMapuiv */
void __stdcall __glim_PixelMapuiv(GLenum map, GLint mapSize,
                        const GLuint values[])
{
    __GLpixelState *ps;
    __GLpixelMapHead *pMap;
    GLint index = map - GL_PIXEL_MAP_I_TO_I;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    ps = &gc->state.pixel;
    pMap = ((__GLpixelMapHead *)gc->state.pixel.map);

    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapI) {
            ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, pMap[index].base.mapI);
            pMap[index].base.mapI = 0;
        }
        pMap[index].base.mapI = (GLint*)
            ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, (size_t) (mapSize * sizeof(GLint)));
        if (!pMap[index].base.mapI) {
            pMap[index].size = 0;
            return;
        }
        pMap[index].size = mapSize;
        while (--mapSize >= 0) {
            pMap[index].base.mapI[mapSize] = values[mapSize];
        }
        break;
      case GL_PIXEL_MAP_I_TO_R:
      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:
      case GL_PIXEL_MAP_I_TO_A:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
      case GL_PIXEL_MAP_R_TO_R:
      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:
      case GL_PIXEL_MAP_A_TO_A:
        if (mapSize < 0) {
            /*
            ** Maps indexed by color component must not have negative size.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapF) {
            ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, pMap[index].base.mapF);
            pMap[index].base.mapF = 0;
        }
        if (mapSize == 0) {
            __glInitDefaultPixelMap(gc, map);
        } else {
            pMap[index].base.mapF = (__GLfloat*)
                ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, (size_t) (mapSize * sizeof(GLfloat)));
            if (!pMap[index].base.mapF) {
                pMap[index].size = 0;
                return;
            }
            pMap[index].size = mapSize;
            while (--mapSize >= 0) {
                pMap[index].base.mapF[mapSize] = 
                        __GL_UI_TO_FLOAT(values[mapSize]);
            }
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690c02d0 -- __glim_PixelMapusv */
void __stdcall __glim_PixelMapusv(GLenum map, GLint mapSize,
                        const GLushort values[])
{
    __GLpixelState *ps;
    __GLpixelMapHead *pMap;
    GLint index = map - GL_PIXEL_MAP_I_TO_I;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    ps = &gc->state.pixel;
    pMap = ((__GLpixelMapHead *)gc->state.pixel.map);

    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapI) {
            ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, pMap[index].base.mapI);
            pMap[index].base.mapI = 0;
        }
        pMap[index].base.mapI = (GLint*)
            ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, (size_t) (mapSize * sizeof(GLint)));
        if (!pMap[index].base.mapI) {
            pMap[index].size = 0;
            return;
        }
        pMap[index].size = mapSize;
        while (--mapSize >= 0) {
            pMap[index].base.mapI[mapSize] = values[mapSize];
        }
        break;
      case GL_PIXEL_MAP_I_TO_R:
      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:
      case GL_PIXEL_MAP_I_TO_A:
        if (mapSize <= 0 || (mapSize & (mapSize - 1))) {
            /*
            ** Maps indexed by color or stencil index must be sized
            ** to a power of two.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
      case GL_PIXEL_MAP_R_TO_R:
      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:
      case GL_PIXEL_MAP_A_TO_A:
        if (mapSize < 0) {
            /*
            ** Maps indexed by color component must not have negative size.
            */
            __glSetError(GL_INVALID_VALUE);
            return;
        }
        if (pMap[index].base.mapF) {
            ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])(gc, pMap[index].base.mapF);
            pMap[index].base.mapF = 0;
        }
        if (mapSize == 0) {
            __glInitDefaultPixelMap(gc, map);
        } else {
            pMap[index].base.mapF = (__GLfloat*)
                ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, (size_t) (mapSize * sizeof(GLfloat)));
            if (!pMap[index].base.mapF) {
                pMap[index].size = 0;
                return;
            }
            pMap[index].size = mapSize;
            while (--mapSize >= 0) {
                pMap[index].base.mapF[mapSize] = 
                        __GL_US_TO_FLOAT(values[mapSize]);
            }
        }
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/*
** gl_polyoffset.c -- original VA range 690b8000..690bc000: s_attrib.c glPolygonOffset (with the 3dfx
** z-precision sign flip), glPushClientAttrib and glCullParameterfvEXT.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"


/* 690ba1d0 -- __glim_PolygonOffset.  3dfx: with the z-precision mode on (fullscreen, 16- or
** 32-bit depth and FX_GL_*BPP_ZPRECISION set) the depth range may be inverted (depthDirection
** GL_GEQUAL): for a depth function that points the other way the offset is negated.  The values and
** the depth function are saved so the sign can be recomputed when the depth function changes.
*/
void __stdcall __glim_PolygonOffset(GLfloat factor, GLfloat units)
{
    GLenum func;
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->fx.fullscreen &&
        ((gc->modes.depthBits > 16 && gc->fx.zPrecision32) ||
         (gc->modes.depthBits == 16 && gc->fx.zPrecision16))) {
        func = gc->fx.lastDepthFunc;
        gc->fx.savedPolygonFactor = factor;
        gc->fx.savedPolygonUnits = units;
        gc->fx.offsetDepthFunc = func;
        if ((gc->fx.depthDirection == GL_GEQUAL && (func == GL_LESS || func == GL_LEQUAL)) ||
            (gc->fx.depthDirection == GL_LEQUAL && (func == GL_GREATER || func == GL_GEQUAL))) {
            gc->state.polygon.factor = factor * -1.0f;
            gc->state.polygon.units = units * -1.0f;
            goto done;
        }
    }
    gc->state.polygon.factor = factor;
    gc->state.polygon.units = units;
  done:
    gc->fx.flags |= 4;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
}

/* 690ba300 -- __glim_PushClientAttrib */
void __stdcall __glim_PushClientAttrib(GLbitfield mask)
{
    __GLclientAttribute **spp;
    __GLclientAttribute *sp;
    __GL_SETUP_NOT_IN_BEGIN();

    spp = gc->attributes.clientStackPointer;
    if (spp < &gc->attributes.clientStack[gc->constants.maxClientAttribStackDepth]) {
        if (!(sp = *spp)) {
            sp = (__GLclientAttribute *)__GL_CALLOC(gc, 1, sizeof(__GLclientAttribute));
            *spp = sp;
        }
        sp->mask = mask;
        gc->attributes.clientStackPointer = spp + 1;
        if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
            sp->packModes = gc->state.pixel.packModes;
            sp->unpackModes = gc->state.pixel.unpackModes;
        }
        if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
            sp->vertexArray = gc->vertexArray;
        }
    } else {
        __glSetError(GL_STACK_OVERFLOW);
        return;
    }
}

/* 690bab00 -- __glim_CullParameterfvEXT (GL_CULL_VERTEX_EYE_POSITION_EXT / _OBJECT_POSITION_EXT) */
void __stdcall __glim_CullParameterfvEXT(GLenum pname, const GLfloat *params)
{
    __GLcoord *pEye;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (pname) {
    case 0x81ab:        /* GL_CULL_VERTEX_EYE_POSITION_EXT */
        pEye = &gc->state.transform.eyePos;
        gc->state.transform.objEyeSpecified = GL_FALSE;
        break;
    case 0x81ac:        /* GL_CULL_VERTEX_OBJECT_POSITION_EXT */
        pEye = &gc->state.transform.eyePosObj;
        gc->state.transform.objEyeSpecified = GL_TRUE;
        break;
    default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    pEye->x = params[0];
    pEye->y = params[1];
    pEye->z = params[2];
    pEye->w = params[3];
    __GL_DELAY_VALIDATE(gc);
}

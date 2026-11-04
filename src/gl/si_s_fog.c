/*
** si_s_fog.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_fog.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690a2080 -- __glFogColorSlow */
void __fastcall __glFogColorSlow(__GLcontext *gc, __GLcolor *out, __GLcolor *in, 
                      __GLfloat fog)
{
    __GLcolor *fogColor;
    __GLfloat oneMinusFog;
    __GLfloat r, g, b;

    oneMinusFog = __GL_ONE_F - fog;

    /*
    ** Blend incoming color against the fog color
    */
    fogColor = &gc->state.fog.color;
    if (gc->modes.colorIndexMode) {
        out->r = in->r + oneMinusFog * gc->state.fog.index;
    } else {
        /*
        ** The following is coded like this to give the instruction scheduler
        ** a hand.
        */
        r = fog * in->r;
        g = fog * in->g;
        b = fog * in->b;
        r += oneMinusFog * fogColor->r;
        g += oneMinusFog * fogColor->g;
        b += oneMinusFog * fogColor->b;
        out->r = r;
        out->g = g;
        out->b = b;
        out->a = in->a;
    }
}

/* 690a17a0 -- __glFogSpan */
GLboolean __fastcall __glFogSpan(__GLcontext *gc)
{
    __GLcolor *cp, *fogColor;
    __GLfloat f, oneMinusFog, fog;
    GLint w;

    w = gc->polygon.shader.length;

    f = gc->polygon.shader.frag.f;
    cp = gc->polygon.shader.colors;
    fogColor = &gc->state.fog.bufColor;
    while (--w >= 0) {
        /* clamp fog value */
        fog = f;
        if (fog < __GL_ZERO_F) fog = __GL_ZERO_F;
        else if (fog > __GL_ONE_F) fog = __GL_ONE_F;
        oneMinusFog = __GL_ONE_F - fog;

        /* Blend incoming color against the fog color */
        if (gc->modes.colorIndexMode) {
            cp->r = cp->r + oneMinusFog * gc->state.fog.index;
        } else {
            cp->r = fog * cp->r + oneMinusFog * fogColor->r;
            cp->g = fog * cp->g + oneMinusFog * fogColor->g;
            cp->b = fog * cp->b + oneMinusFog * fogColor->b;
        }

        f += gc->polygon.shader.dfdx;
        cp++;
    }

    return GL_FALSE;
}

/* 690a1f10 -- __glFogVertexLinear */
__GLfloat __fastcall __glFogVertexLinear(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat eyeZ, fog, end;

    eyeZ = vx->eye.z;
    if (eyeZ < __GL_ZERO_F) eyeZ = -eyeZ;

    end = gc->state.fog.end;
    fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;

    if (fog < __GL_ZERO_F)
        fog = __GL_ZERO_F;
    else if (fog > __GL_ONE_F)
        fog = __GL_ONE_F;
    
    return fog;
}

/* 690a2030 -- __glFogVertexLinearFogCoord: __glFogVertexLinear (s_fog.c) on the vertex fog coordinate */
__GLfloat __fastcall __glFogVertexLinearFogCoord(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat eyeZ, fog, end;

    eyeZ = vx->fogCoord;     /* ICD: the vertex fog coordinate (0xa0), not eye.z */
    if (eyeZ < __GL_ZERO_F) eyeZ = -eyeZ;

    end = gc->state.fog.end;
    fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;

    if (fog < __GL_ZERO_F)
        fog = __GL_ZERO_F;
    else if (fog > __GL_ONE_F)
        fog = __GL_ONE_F;
    
    return fog;
}

/* 690a1870 -- __glFogStippledSpan */
GLboolean __fastcall __glFogStippledSpan(__GLcontext *gc)
{
    __GLstippleWord bit, inMask, *sp;
    __GLcolor *cp, *fogColor;
    __GLfloat f, oneMinusFog, fog;
    GLint count;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    f = gc->polygon.shader.frag.f;
    cp = gc->polygon.shader.colors;
    fogColor = &gc->state.fog.bufColor;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp++;
        bit = __GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (inMask & bit) {
                /* clamp fog value */
                fog = f;
                if (fog < __GL_ZERO_F) fog = __GL_ZERO_F;
                else if (fog > __GL_ONE_F) fog = __GL_ONE_F;
                oneMinusFog = __GL_ONE_F - fog;

                /* Blend incoming color against the fog color */
                if (gc->modes.colorIndexMode) {
                    cp->r = cp->r + oneMinusFog * gc->state.fog.index;
                } else {
                    cp->r = fog * cp->r + oneMinusFog * fogColor->r;
                    cp->g = fog * cp->g + oneMinusFog * fogColor->g;
                    cp->b = fog * cp->b + oneMinusFog * fogColor->b;
                }
            }
            f += gc->polygon.shader.dfdx;
            cp++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
    }

    return GL_FALSE;
}

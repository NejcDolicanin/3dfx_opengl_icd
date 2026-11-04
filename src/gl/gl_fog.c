/*
** gl_fog.c -- s_fog.c (3dfx GLCORE/S_FOG.C): the fog procs; the SI-ported part of the module is
** si_s_fog.c.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "glshade.h"
#include "si_names.h"
#include "icdprotos.h"
#include <string.h>
#include <math.h>

#define __glE       2.7182818284590452354      /* a double: the original pushes the full constant */

/* 690a1990 -- s_fog.c __glFogSpanSlow (fog is not initialised in the ICD) */
GLboolean __fastcall __glFogSpanSlow(__GLcontext *gc)
{
    __GLcolor *cp, *fogColor;
    __GLfloat f, oneMinusFog, fog, eyeZ;
    __GLfloat density, density2, end;
    GLint w;

    w = gc->polygon.shader.length;

    f = gc->polygon.shader.frag.f;
    cp = gc->polygon.shader.colors;
    fogColor = &gc->state.fog.bufColor;
    density = gc->state.fog.density;
    density2 = density * density;
    end = gc->state.fog.end;
    while (--w >= 0) {
        /* Compute fog value */
        eyeZ = f;
        if (eyeZ < __GL_ZERO_F) eyeZ = -eyeZ;
        switch (gc->state.fog.mode) {
          case GL_EXP:
            fog = (__GLfloat)pow(__glE, -density * eyeZ);
            break;
          case GL_EXP2:
            fog = (__GLfloat)pow(__glE, -(density2 * eyeZ * eyeZ));
            break;
          case GL_LINEAR:
            fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
            break;
        }

        /* clamp fog value */
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

/* 690a1b00 -- s_fog.c __glFogStippledSpanSlow */
GLboolean __fastcall __glFogStippledSpanSlow(__GLcontext *gc)
{
    GLuint bit, inMask, *sp;
    __GLcolor *cp, *fogColor;
    __GLfloat f, oneMinusFog, fog, eyeZ;
    __GLfloat density, density2, end;
    GLint count;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    f = gc->polygon.shader.frag.f;
    cp = gc->polygon.shader.colors;
    fogColor = &gc->state.fog.bufColor;
    density = gc->state.fog.density;
    density2 = density * density;
    end = gc->state.fog.end;
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
                /* Compute fog value */
                eyeZ = f;
                if (eyeZ < __GL_ZERO_F) eyeZ = -eyeZ;
                switch (gc->state.fog.mode) {
                  case GL_EXP:
                    fog = (__GLfloat)pow(__glE, -density * eyeZ);
                    break;
                  case GL_EXP2:
                    fog = (__GLfloat)pow(__glE, -(density2 * eyeZ * eyeZ));
                    break;
                  case GL_LINEAR:
                    fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
                    break;
                }

                /* Clamp resulting fog value */
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
            bit >>= 1;
        }
    }

    return GL_FALSE;
}

/************************************************************************/
/* 690a1cc0 -- __glFogPow (3dfx S_FOG.C): the fog/attenuation polynomial: 1 / (1 + c1 x + c2 x^2 + c3 x^3)^4 */
double __stdcall __glFogPow(float x)
{
    double f;

    f = x*x*x*0.0038278 + x*x*0.0292732 + x*0.2507213 + 1.0;
    f = f * f;
    return 1.0 / (f * f);
}

/* 690a1d10 -- s_fog.c __glFogFragmentSlow (fog not initialised) */
void __fastcall __glFogFragmentSlow(__GLcontext *gc, __GLfragmentICD *frag, __GLfloat eyeZ)
{
    __GLfloat fog, oneMinusFog, density, end;
    __GLcolor *fogColor;

    if (eyeZ < __GL_ZERO_F) eyeZ = -eyeZ;

    switch (gc->state.fog.mode) {
      case GL_EXP:
        density = gc->state.fog.density;
        fog = (__GLfloat)pow(__glE, -density * eyeZ);
        break;
      case GL_EXP2:
        density = gc->state.fog.density;
        fog = (__GLfloat)pow(__glE, -(density * eyeZ * density * eyeZ));
        break;
      case GL_LINEAR:
        end = gc->state.fog.end;
        fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
        break;
    }

    /*
    ** clamp fog value
    */
    if (fog < __GL_ZERO_F)
        fog = __GL_ZERO_F;
    else
    if (fog > __GL_ONE_F)
        fog = __GL_ONE_F;
    oneMinusFog = __GL_ONE_F - fog;

    /*
    ** Blend incoming color against the fog color
    */
    fogColor = &gc->state.fog.bufColor;
    if (gc->modes.colorIndexMode) {
        frag->color.r += oneMinusFog * gc->state.fog.index;
    } else {
        frag->color.r = fog * frag->color.r + oneMinusFog * fogColor->r;
        frag->color.g = fog * frag->color.g + oneMinusFog * fogColor->g;
        frag->color.b = fog * frag->color.b + oneMinusFog * fogColor->b;
    }
}

/* 690a1e40 -- s_fog.c __glFogVertex (fog left uninitialised: an unknown mode returns a stale slot) */
__GLfloat __fastcall __glFogVertex(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat eyeZ, fog, density, end;

    eyeZ = vx->eye.z;
    if (eyeZ < __GL_ZERO_F) eyeZ = -eyeZ;

    switch (gc->state.fog.mode) {
      case GL_EXP:
	density = gc->state.fog.density;
	fog = pow(__glE, -density * eyeZ);
	break;
      case GL_EXP2:
	density = gc->state.fog.density;
	fog = pow(__glE, -(density * density * eyeZ * eyeZ));
	break;
      case GL_LINEAR:
	end = gc->state.fog.end;
	fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
	break;
    }

    /*
    ** Since this routine is called when we are doing slow fog, we can
    ** safely clamp the fog value here.
    */
    if (fog < __GL_ZERO_F)
	fog = __GL_ZERO_F;
    else if (fog > __GL_ONE_F)
	fog = __GL_ONE_F;

    return fog;
}

/* 690a1f60 -- __glFogVertexFogCoord: s_fog.c __glFogVertex on the vertex fog coordinate (EXT_fog_coord) */
__GLfloat __fastcall __glFogVertexFogCoord(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat eyeZ, fog, density, end;

    eyeZ = vx->fogCoord;
    if (eyeZ < __GL_ZERO_F) eyeZ = -eyeZ;

    switch (gc->state.fog.mode) {
      case GL_EXP:
	density = gc->state.fog.density;
	fog = pow(__glE, -density * eyeZ);
	break;
      case GL_EXP2:
	density = gc->state.fog.density;
	fog = pow(__glE, -(density * density * eyeZ * eyeZ));
	break;
      case GL_LINEAR:
	end = gc->state.fog.end;
	fog = (end - eyeZ) * gc->state.fog.oneOverEMinusS;
	break;
    }

    if (fog < __GL_ZERO_F)
	fog = __GL_ZERO_F;
    else if (fog > __GL_ONE_F)
	fog = __GL_ONE_F;

    return fog;
}

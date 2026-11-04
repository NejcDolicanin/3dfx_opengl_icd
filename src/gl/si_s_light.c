/*
** si_s_light.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_light.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 6908fb90 -- __glScaleColorf */
void __fastcall __glScaleColorf(__GLcontext *gc, __GLcolor *dst, const GLfloat src[4])
{
    dst->r = src[0] * gc->constants.redScale;
    dst->g = src[1] * gc->constants.greenScale;
    dst->b = src[2] * gc->constants.blueScale;
    dst->a = src[3] * gc->constants.alphaScale;
}

/* 6908fbd0 -- __glClampAndScaleColorf */
void __fastcall __glClampAndScaleColorf(__GLcontext *gc, __GLcolor *d, const GLfloat s[4])
{
    __GLfloat zero = __GL_ZERO_F;

    d->r = s[0] * gc->constants.redScale;
    if (d->r < zero) d->r = zero;
    if (d->r > gc->constants.redScale) d->r = gc->constants.redScale;

    d->g = s[1] * gc->constants.greenScale;
    if (d->g < zero) d->g = zero;
    if (d->g > gc->constants.greenScale) d->g = gc->constants.greenScale;

    d->b = s[2] * gc->constants.blueScale;
    if (d->b < zero) d->b = zero;
    if (d->b > gc->constants.blueScale) d->b = gc->constants.blueScale;

    d->a = s[3] * gc->constants.alphaScale;
    if (d->a < zero) d->a = zero;
    if (d->a > gc->constants.alphaScale) d->a = gc->constants.alphaScale;
}

/* 6908fcc0 -- __glClampColorf */
void __fastcall __glClampColorf(__GLcontext *gc, __GLcolor *d, const GLfloat s[4])
{
    __GLfloat zero = __GL_ZERO_F;
    __GLfloat one = __GL_ONE_F;
    __GLfloat r,g,b,a;

    r = s[0];
    g = s[1];
    b = s[2];
    a = s[3];

    if (r < zero) d->r = zero;
    else if (r > one) d->r = one;
    else d->r = r;

    if (g < zero) d->g = zero;
    else if (g > one) d->g = one;
    else d->g = g;

    if (b < zero) d->b = zero;
    else if (b > one) d->b = one;
    else d->b = b;

    if (a < zero) d->a = zero;
    else if (a > one) d->a = one;
    else d->a = a;
}

/* 6908fe90 -- __glScaleColori */
void __fastcall __glScaleColori(__GLcontext *gc, __GLcolor *dst, const GLint src[4])
{
    dst->r = __GL_I_TO_FLOAT(src[0]) * gc->constants.redScale;
    dst->g = __GL_I_TO_FLOAT(src[1]) * gc->constants.greenScale;
    dst->b = __GL_I_TO_FLOAT(src[2]) * gc->constants.blueScale;
    dst->a = __GL_I_TO_FLOAT(src[3]) * gc->constants.alphaScale;
}

/* 6908ff00 -- __glClampAndScaleColori */
void __fastcall __glClampAndScaleColori(__GLcontext *gc, __GLcolor *d, const GLint s[4])
{
    __GLfloat zero = __GL_ZERO_F;

    d->r = __GL_I_TO_FLOAT(s[0]) * gc->constants.redScale;
    if (d->r < zero) d->r = zero;
    if (d->r > gc->constants.redScale) d->r = gc->constants.redScale;

    d->g = __GL_I_TO_FLOAT(s[1]) * gc->constants.greenScale;
    if (d->g < zero) d->g = zero;
    if (d->g > gc->constants.greenScale) d->g = gc->constants.greenScale;

    d->b = __GL_I_TO_FLOAT(s[2]) * gc->constants.blueScale;
    if (d->b < zero) d->b = zero;
    if (d->b > gc->constants.blueScale) d->b = gc->constants.blueScale;

    d->a = __GL_I_TO_FLOAT(s[3]) * gc->constants.alphaScale;
    if (d->a < zero) d->a = zero;
    if (d->a > gc->constants.alphaScale) d->a = gc->constants.alphaScale;
}

/* 69090020 -- __glClampColori */
void __fastcall __glClampColori(__GLcontext *gc, __GLcolor *d, const GLint s[4])
{
    __GLfloat zero = __GL_ZERO_F;
    __GLfloat one = __GL_ONE_F;
    __GLfloat r,g,b,a;

    r = __GL_I_TO_FLOAT(s[0]);
    g = __GL_I_TO_FLOAT(s[1]);
    b = __GL_I_TO_FLOAT(s[2]);
    a = __GL_I_TO_FLOAT(s[3]);

    if (r < zero) d->r = zero;
    else if (r > one) d->r = one;
    else d->r = r;

    if (g < zero) d->g = zero;
    else if (g > one) d->g = one;
    else d->g = g;

    if (b < zero) d->b = zero;
    else if (b > one) d->b = one;
    else d->b = b;

    if (a < zero) d->a = zero;
    else if (a > one) d->a = one;
    else d->a = a;
}

/* 69090160 -- __glUnScaleColorf */
void __fastcall __glUnScaleColorf(__GLcontext *gc, GLfloat dst[4], const __GLcolor* src)
{
    dst[0] = src->r * gc->constants.oneOverRedScale;
    dst[1] = src->g * gc->constants.oneOverGreenScale;
    dst[2] = src->b * gc->constants.oneOverBlueScale;
    dst[3] = src->a * gc->constants.oneOverAlphaScale;
}

/* 690901a0 -- __glUnScaleColori */
void __fastcall __glUnScaleColori(__GLcontext *gc, GLint dst[4], const __GLcolor* src)
{
    dst[0] = __GL_FLOAT_TO_I(src->r * gc->constants.oneOverRedScale);
    dst[1] = __GL_FLOAT_TO_I(src->g * gc->constants.oneOverGreenScale);
    dst[2] = __GL_FLOAT_TO_I(src->b * gc->constants.oneOverBlueScale);
    dst[3] = __GL_FLOAT_TO_I(src->a * gc->constants.oneOverAlphaScale);
}

/* 69090250 -- __glClampRGBColor */
void __fastcall __glClampRGBColor(__GLcontext *gc, __GLcolor *dst, const __GLcolor *src)
{
    __GLfloat zero = __GL_ZERO_F;
    __GLfloat r, g, b, a;
    __GLfloat rl, gl, bl, al;

    r = src->r; rl = gc->constants.redScale;
    if (r <= zero) {
        dst->r = zero;
    } else {
        if (r >= rl) {
            dst->r = rl;
        } else {
            dst->r = r;
        }
    }
    g = src->g; gl = gc->constants.greenScale;
    if (g <= zero) {
        dst->g = zero;
    } else {
        if (g >= gl) {
            dst->g = gl;
        } else {
            dst->g = g;
        }
    }
    b = src->b; bl = gc->constants.blueScale;
    if (b <= zero) {
        dst->b = zero;
    } else {
        if (b >= bl) {
            dst->b = bl;
        } else {
            dst->b = b;
        }
    }
    a = src->a; al = gc->constants.alphaScale;
    if (a <= zero) {
        dst->a = zero;
    } else {
        if (a >= al) {
            dst->a = al;
        } else {
            dst->a = a;
        }
    }
}

#ifndef __GL_SPOT_LOOKUP_TABLE_SIZE
#define __GL_SPOT_LOOKUP_TABLE_SIZE     256
#endif

/*
** s_light.c's vector normalisation macros (one Newton-Raphson step from a magic-number seed, as
** __glNormalize in s_math.c); `zero` is the calling function's local.  Same as gl_memclr.c's copy.
*/
#define NORMALIZE_3ARG( X, Y, Z )                     \
{                                                       \
    __GLfloat len = (X)*(X) + (Y)*(Y) + (Z)*(Z);        \
    if (len <= zero) {                                  \
        X = zero;                                       \
        Y = zero;                                       \
        Z = zero;                                       \
    }                                                   \
    else if (len != 1.0F) {                             \
        union { unsigned int i; float f; } seed;        \
        float xy, subexp;                               \
        seed.f = len;                                   \
        seed.i = 0x5f375a00u - (seed.i >> 1);           \
        xy = len * seed.f * seed.f;                     \
        subexp = 3.f - xy;                              \
        len = .0625f * seed.f * subexp * (12.f - xy * subexp * subexp); \
        X *= len;                                       \
        Y *= len;                                       \
        Z *= len;                                       \
    }                                                   \
}
#define NORMALIZE_6ARG( NX, NY, NZ, X, Y, Z )         \
{                                                       \
    __GLfloat len = (X)*(X) + (Y)*(Y) + (Z)*(Z);        \
    if (len <= zero) {                                  \
        NX = zero;                                      \
        NY = zero;                                      \
        NZ = zero;                                      \
    }                                                   \
    else if (len == 1.0F) {                             \
        NX = X;                                         \
        NY = Y;                                         \
        NZ = Z;                                         \
    }                                                   \
    else {                                              \
        union { unsigned int i; float f; } seed;        \
        float xy, subexp;                               \
        seed.f = len;                                   \
        seed.i = 0x5f375a00u - (seed.i >> 1);           \
        xy = len * seed.f * seed.f;                     \
        subexp = 3.f - xy;                              \
        len = .0625f * seed.f * subexp * (12.f - xy * subexp * subexp); \
        NX = X * len;                                   \
        NY = Y * len;                                   \
        NZ = Z * len;                                   \
    }                                                   \
}

/*
** 69090350 -- __glCalcRGBColorSI (s_light.c): the SI's plain-C RGB lighting of one vertex (fast path
** first, no separate specular colour, no zero test on the attenuation denominator); ToDo UNUSED CODE:
** nothing in the driver calls it (the ICD uses __glCalcRGBColor, 690e2ce0)
*/
void __fastcall __glCalcRGBColorSI(__GLcontext *gc, GLint face, __GLvertexICD *vx)
{
    __GLlightSourceMachineICD *lsm;
    __GLmaterialMachineICD *msm;
    __GLmaterialState *ms;
    __GLcolor *c;
    __GLfloat r, g, b, zero, nx, ny, nz;
    GLboolean eyeWIsZero, localViewer;

    /* Pick material to use */
    if (face == __GL_FRONTFACE) {
        c = &vx->colors[__GL_FRONTFACE];
        msm = &gc->light.front;
        ms = &gc->state.light.front;
        nx = vx->normal.x;
        ny = vx->normal.y;
        nz = vx->normal.z;
    } else {
        c = &vx->colors[__GL_BACKFACE];
        msm = &gc->light.back;
        ms = &gc->state.light.back;
        nx = -vx->normal.x;
        ny = -vx->normal.y;
        nz = -vx->normal.z;
    }

    /* Compute the color */
    zero = __GL_ZERO_F;
    r = msm->sceneColor.r;
    g = msm->sceneColor.g;
    b = msm->sceneColor.b;
    eyeWIsZero = vx->eye.w == zero;
    localViewer = gc->state.light.model.localViewer;
    lsm = gc->light.sources;
    while (lsm) {
        __GLlightSourceStateICD *lss = lsm->state;

        if (!lsm->slowPath && !eyeWIsZero) {
            /* Fast lighting */
            __GLfloat n1;

            r += lss->ambient.r * ms->ambient.r;
            g += lss->ambient.g * ms->ambient.g;
            b += lss->ambient.b * ms->ambient.b;

            /* Add in specular and diffuse effect of light, if any */
            n1 = nx * lsm->unitVPpli.x + ny * lsm->unitVPpli.y + nz * lsm->unitVPpli.z;
            if (n1 > zero) {
                __GLfloat n2;

                n2 = nx * lsm->hHat.x + ny * lsm->hHat.y + nz * lsm->hHat.z;
                n2 -= msm->threshold;
                if (n2 >= zero) {
                    GLuint ix = (GLuint)(n2 * msm->scale + __glHalf);
                    if (ix < __GL_SPEC_LOOKUP_TABLE_SIZE) {
                        n2 = msm->specTable[ix];
                        r += n2 * lss->specular.r * ms->specular.r;
                        g += n2 * lss->specular.g * ms->specular.g;
                        b += n2 * lss->specular.b * ms->specular.b;
                    } else {
                        r += lss->specular.r * ms->specular.r;
                        g += lss->specular.g * ms->specular.g;
                        b += lss->specular.b * ms->specular.b;
                    }
                }
                r += n1 * lss->diffuse.r * ms->diffuse.r;
                g += n1 * lss->diffuse.g * ms->diffuse.g;
                b += n1 * lss->diffuse.b * ms->diffuse.b;
            }
        } else {
            __GLcoord hHat, vPli, vPliHat;
            __GLfloat att, attSpot;
            __GLfloat hv_x, hv_y, hv_z;

            /* Compute unit h[i] */
            Vvec(&vPli, &vx->eye, &lsm->position);
            NORMALIZE_6ARG( vPliHat.x, vPliHat.y, vPliHat.z, vPli.x, vPli.y, vPli.z );
            if (localViewer) {
                __GLfloat vPeHat_x, vPeHat_y, vPeHat_z;

                /* vPeHat = vector from vx->eye to Pe (the eye at the origin) */
                vPeHat_x = -vx->eye.x;
                vPeHat_y = -vx->eye.y;
                vPeHat_z = -vx->eye.z;
                NORMALIZE_3ARG( vPeHat_x, vPeHat_y, vPeHat_z );
                hv_x = vPliHat.x + vPeHat_x;
                hv_y = vPliHat.y + vPeHat_y;
                hv_z = vPliHat.z + vPeHat_z;
            } else {
                hv_x = vPliHat.x;
                hv_y = vPliHat.y;
                hv_z = vPliHat.z + __GL_ONE_F;
            }
            NORMALIZE_6ARG( hHat.x, hHat.y, hHat.z, hv_x, hv_y, hv_z );

            /* Compute attenuation */
            if (lsm->position.w != zero) {
                __GLfloat k0, k1, k2, dist;

                k0 = lsm->constantAttenuation;
                k1 = lsm->linearAttenuation;
                k2 = lsm->quadraticAttenuation;
                if ((k1 == zero) && (k2 == zero)) {
                    /* Use pre-computed 1/k0 */
                    att = lsm->attenuation;
                } else {
                    dist = __GL_SQRTF(vPli.x*vPli.x + vPli.y*vPli.y + vPli.z*vPli.z);
                    att = __GL_ONE_F / (k0 + (k1 + k2 * dist) * dist);
                }
            } else {
                att = __GL_ONE_F;
            }

            /* Compute spot effect if light is a spot light */
            attSpot = att;
            if (lsm->isSpot) {
                __GLfloat dot, px, py, pz;

                px = -vPliHat.x;
                py = -vPliHat.y;
                pz = -vPliHat.z;
                dot = px * lsm->direction.x + py * lsm->direction.y + pz * lsm->direction.z;
                if ((dot >= lsm->threshold) && (dot >= lsm->cosCutOffAngle)) {
                    GLuint ix = (GLuint)((dot - lsm->threshold) * lsm->scale + __glHalf);
                    if (ix < __GL_SPOT_LOOKUP_TABLE_SIZE) {
                        attSpot = att * lsm->spotTable[ix];
                    }
                } else {
                    attSpot = zero;
                }
            }

            /* Add in remaining effect of light, if any */
            if (attSpot) {
                __GLfloat n1;
                __GLcolor sum;

                sum.r = lss->ambient.r * ms->ambient.r;
                sum.g = lss->ambient.g * ms->ambient.g;
                sum.b = lss->ambient.b * ms->ambient.b;

                n1 = nx * vPliHat.x + ny * vPliHat.y + nz * vPliHat.z;
                if (n1 > zero) {
                    __GLfloat n2;

                    n2 = nx * hHat.x + ny * hHat.y + nz * hHat.z;
                    n2 -= msm->threshold;
                    if (n2 >= zero) {
                        GLuint ix = (GLuint)(n2 * msm->scale + __glHalf);
                        if (ix < __GL_SPEC_LOOKUP_TABLE_SIZE) {
                            n2 = msm->specTable[ix];
                            sum.r += n2 * lss->specular.r * ms->specular.r;
                            sum.g += n2 * lss->specular.g * ms->specular.g;
                            sum.b += n2 * lss->specular.b * ms->specular.b;
                        } else {
                            sum.r += lss->specular.r * ms->specular.r;
                            sum.g += lss->specular.g * ms->specular.g;
                            sum.b += lss->specular.b * ms->specular.b;
                        }
                    }
                    sum.r += n1 * lss->diffuse.r * ms->diffuse.r;
                    sum.g += n1 * lss->diffuse.g * ms->diffuse.g;
                    sum.b += n1 * lss->diffuse.b * ms->diffuse.b;
                }
                r += attSpot * sum.r;
                g += attSpot * sum.g;
                b += attSpot * sum.b;
            }
        }
        lsm = lsm->next;
    }

    /* Clamp the computed color */
    {
        __GLfloat rl, gl, bl;

        rl = gc->constants.redScale;
        gl = gc->constants.greenScale;
        bl = gc->constants.blueScale;

        if (r <= zero) r = zero;
        else if (r >= rl) r = rl;

        if (g <= zero) g = zero;
        else if (g >= gl) g = gl;

        if (b <= zero) b = zero;
        else if (b >= bl) b = bl;

        c->r = r;
        c->g = g;
        c->b = b;
        c->a = msm->alpha;
    }
}

/* 69091d90 -- __glChangeVertexMaterialColor */
void __fastcall __glChangeVertexMaterialColor(__GLcontext *gc, __GLcolor *col)
{
    __GLcolor saveColor = gc->state.current.userColor;

    gc->state.current.userColor = *col;
    (*gc->procs.applyColor)(gc);
    gc->state.current.userColor = saveColor;
}

/* 6908fdc0 -- ClampScaledColorf */
void __fastcall ClampScaledColorf(__GLcontext *gc, __GLcolor *d, 
                              const GLfloat s[4])
{
    __GLfloat zero = __GL_ZERO_F;

    d->r = s[0];
    if (d->r < zero) d->r = zero;
    if (d->r > gc->constants.redScale) d->r = gc->constants.redScale;

    d->g = s[1];
    if (d->g < zero) d->g = zero;
    if (d->g > gc->constants.greenScale) d->g = gc->constants.greenScale;

    d->b = s[2];
    if (d->b < zero) d->b = zero;
    if (d->b > gc->constants.blueScale) d->b = gc->constants.blueScale;

    d->a = s[3];
    if (d->a < zero) d->a = zero;
    if (d->a > gc->constants.alphaScale) d->a = gc->constants.alphaScale;
}

/* 69090bf0 -- Vvec (s_light.c) */
void __fastcall Vvec(__GLcoord *r, const __GLcoord *p1, const __GLcoord *p2)
{
   const int i = *(const int *)&p1->w;
   const int j = *(const int *)&p2->w;
   
   /*
   * Test for +/-0.0
   */
   if (i << 1) {
      if (j << 1) {
      /*
             * Cross multiply instead of dividing, since
        * result always gets normalized.
         */
         __GLfloat w1, w2;
         w1 = p1->w;
         w2 = p2->w;
         r->x = p2->x * w1 - p1->x * w2;
         r->y = p2->y * w1 - p1->y * w2;
         r->z = p2->z * w1 - p1->z * w2;
      } else {
      /*
             * w2 == 0
         */
         r->x = p2->x;
         r->y = p2->y;
         r->z = p2->z;
      }
   } else if (j << 1) {
   /*
   * w1 == 0
             */
      r->x = -p1->x;
      r->y = -p1->y;
      r->z = -p1->z;
   } else {
   /*
   * w1 == 0 & w2 == 0
             */
      r->x = p2->x - p1->x;
      r->y = p2->y - p1->y;
      r->z = p2->z - p1->z;
   }
}

/* 69090f30 -- FogRGBColor */
void __fastcall FogRGBColor(__GLcontext *gc, GLint face, __GLvertexICD *vx)
{
    __GLfloat fog, oneMinusFog;
    __GLcolor *cp;

    /* Get the vertex fog value */
    fog = vx->fog;
    oneMinusFog = __GL_ONE_F - fog;

    /* Now whack the color */
    cp = &vx->colors[face];
    cp->r = fog * cp->r + oneMinusFog * gc->state.fog.color.r;
    cp->g = fog * cp->g + oneMinusFog * gc->state.fog.color.g;
    cp->b = fog * cp->b + oneMinusFog * gc->state.fog.color.b;
}

/* 69090f80 -- FogLitRGBColor */
void __fastcall FogLitRGBColor(__GLcontext *gc, GLint face, __GLvertexICD *vx)
{
    __GLfloat fog, oneMinusFog;
    __GLcolor *cp;

    /* First compute vertex color */
    (*gc->procs.calcColor2)(gc, face, vx);

    /* Get the vertex fog value */
    fog = vx->fog;
    oneMinusFog = __GL_ONE_F - fog;

    /* Now whack the color */
    cp = &vx->colors[face];
    cp->r = fog * cp->r + oneMinusFog * gc->state.fog.color.r;
    cp->g = fog * cp->g + oneMinusFog * gc->state.fog.color.g;
    cp->b = fog * cp->b + oneMinusFog * gc->state.fog.color.b;
}

/* 69091a80 -- FogCIColor */
void __fastcall FogCIColor(__GLcontext *gc, GLint face, __GLvertexICD *vx)
{
    __GLfloat fog, oneMinusFog, maxR;
    __GLcolor *cp;

    /* Get the vertex fog value */
    fog = vx->fog;
    oneMinusFog = __GL_ONE_F - fog;

    /* Now whack the color */
    cp = &vx->colors[face];
    cp->r = cp->r + oneMinusFog * gc->state.fog.index;
    maxR = (1 << gc->modes.indexBits) - 1;
    if (cp->r > maxR) {
        cp->r = maxR;
    }
}

/* 69091ad0 -- FogLitCIColor */
void __fastcall FogLitCIColor(__GLcontext *gc, GLint face, __GLvertexICD *vx)
{
    __GLfloat fog, oneMinusFog, maxR;
    __GLcolor *cp;

    /* First compute vertex color */
    (*gc->procs.calcColor2)(gc, face, vx);

    /* Get the vertex fog value */
    fog = vx->fog;
    oneMinusFog = __GL_ONE_F - fog;

    /* Now whack the color */
    cp = &vx->colors[face];
    cp->r = cp->r + oneMinusFog * gc->state.fog.index;
    maxR = (1 << gc->modes.indexBits) - 1;
    if (cp->r > maxR) {
        cp->r = maxR;
    }
}

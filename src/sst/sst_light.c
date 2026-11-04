/*
** sst_light.c -- lighting of the hardware path: the RGB lighting of a vertex and its vector helper.
*/
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "glshade.h"
#include "si_names.h"
#include "icdglob.h"
#include "icdprotos.h"
#include <glide.h>
#include "og.h"
/* ---- the light source machine: glctx.h __GLlightSourceMachineICD ---- */
#define SPEC_LOOKUP_TABLE_SIZE  256
#define SPOT_LOOKUP_TABLE_SIZE  256
/* T: a cast on one operand of z*z; with GLdouble VC6 computes (x*x + y*y) + z*z from z, y, x loaded in
** that order on the vertex normal, exactly as the original; every plain form is
** reassociated (z*z first).  The x87 evaluates both at the same precision. */
#define NORMALIZE_3ARG( X, Y, Z )   NORMALIZE_3ARG_T(X, Y, Z, __GLfloat)
#define NORMALIZE_3ARG_T( X, Y, Z, T )                    \
{                                                       \
    __GLfloat len = (X)*(X) + (Z)*(T)(Z) + (Y)*(Y);        \
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
#define NORMALIZE_6ARG( NX, NY, NZ, X, Y, Z )        \
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
extern void __fastcall sstLight_Vvec(__GLcoord *r, const __GLcoord *p1, const __GLcoord *p2);

/* ---- the glcore twin in src/gl/si_s_light.c, copied (the hardware module has its own copy of the code) ---- */
/* 6905cea0 -- sstLight_Vvec (s_light.c Vvec): copy of 69090bf0 */
void __fastcall sstLight_Vvec(__GLcoord *r, const __GLcoord *p1, const __GLcoord *p2)
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

/*
** 6905c550 -- hwCalcColorFront: RGB lighting of one vertex, front face (the hardware path's calcColor): the SI's
** __glCalcRGBColor without the back face and the separate specular colour, with the normal
** normalised first when GL_NORMALIZE is on; the lit colour is packed by hwPackVertexColor.
*/
void __fastcall hwCalcColorFront(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLlightSourceMachineICD *lsm;
    __GLmaterialMachineICD *msm;
    __GLmaterialState *ms;
    __GLcolor *c;
    __GLfloat r, g, b, zero, nx, ny, nz;
    GLboolean eyeWIsZero, localViewer;

    zero = 0.0f;
    if (gc->state.enables.general & __GL_NORMALIZE_ENABLE) {
        NORMALIZE_3ARG_T(vx->normal.x, vx->normal.y, vx->normal.z, GLdouble);
    }
    c = &vx->colors[0];
    msm = &gc->light.front;
    ms = &gc->state.light.front;
    nx = vx->normal.x;
    ny = vx->normal.y;
    nz = vx->normal.z;

    r = msm->sceneColor.r;
    g = msm->sceneColor.g;
    b = msm->sceneColor.b;
    eyeWIsZero = vx->eye.w == zero;
    localViewer = gc->state.light.model.localViewer;
    lsm = (__GLlightSourceMachineICD *)gc->light.sources;
    while (lsm) {
        __GLlightSourceStateICD *lss = lsm->state;

        if (!lsm->slowPath && !eyeWIsZero) {
            /* Fast lighting */
            __GLfloat n1;

            r += lss->ambient.r * ms->ambient.r;
            g += lss->ambient.g * ms->ambient.g;
            b += lss->ambient.b * ms->ambient.b;

            n1 = nx * lsm->unitVPpli.x + ny * lsm->unitVPpli.y + nz * lsm->unitVPpli.z;
            if (n1 > zero) {
                __GLfloat n2;
                n2 = nx * lsm->hHat.x + ny * lsm->hHat.y + nz * lsm->hHat.z;
                n2 -= msm->threshold;
                if (n2 >= zero) {
                    GLuint ix = (GLuint)(n2 * msm->scale + gc->constants.half);
                    if (ix < SPEC_LOOKUP_TABLE_SIZE) {
                        n2 = msm->specTable[ix];
                        r += lss->specular.r * ms->specular.r * n2;
                        g += lss->specular.g * ms->specular.g * n2;
                        b += lss->specular.b * ms->specular.b * n2;
                    } else {
                        r += lss->specular.r * ms->specular.r;
                        g += lss->specular.g * ms->specular.g;
                        b += lss->specular.b * ms->specular.b;
                    }
                }
                r += lss->diffuse.r * ms->diffuse.r * n1;
                g += lss->diffuse.g * ms->diffuse.g * n1;
                b += lss->diffuse.b * ms->diffuse.b * n1;
            }
        } else {
            __GLcoord hHat, vPli, vPliHat;
            __GLfloat att, attSpot;
            __GLfloat hv_x, hv_y, hv_z;

            /* Compute unit h[i] */
            sstLight_Vvec(&vPli, &vx->eye, &lsm->position);
            NORMALIZE_6ARG(vPliHat.x, vPliHat.y, vPliHat.z, vPli.x, vPli.y, vPli.z);
            if (localViewer) {
                __GLfloat vPeHat_x, vPeHat_y, vPeHat_z;
                vPeHat_x = -vx->eye.x;
                vPeHat_y = -vx->eye.y;
                vPeHat_z = -vx->eye.z;
                NORMALIZE_3ARG(vPeHat_x, vPeHat_y, vPeHat_z);
                hv_x = vPliHat.x + vPeHat_x;
                hv_y = vPliHat.y + vPeHat_y;
                hv_z = vPliHat.z + vPeHat_z;
            } else {
                hv_x = vPliHat.x;
                hv_y = vPliHat.y;
                hv_z = vPliHat.z + 1.0f;
            }
            NORMALIZE_6ARG(hHat.x, hHat.y, hHat.z, hv_x, hv_y, hv_z);

            /* Compute attenuation */
            if (lsm->position.w != zero) {
                __GLfloat k0, k1, k2, dist;

                k0 = lsm->constantAttenuation;
                k1 = lsm->linearAttenuation;
                k2 = lsm->quadraticAttenuation;
                if ((k1 == zero) && (k2 == zero)) {
                    att = lsm->attenuation;
                } else {
                    dist = (__GLfloat)sqrt(vPli.x*vPli.x + vPli.y*vPli.y + vPli.z*vPli.z);
                    att = 1.0f / (k0 + (k1 + k2 * dist) * dist);
                }
            } else {
                att = 1.0f;
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
                    GLuint ix = (GLuint)((dot - lsm->threshold) * lsm->scale + gc->constants.half);
                    if (ix < SPOT_LOOKUP_TABLE_SIZE) {
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
                        GLuint ix = (GLuint)(n2 * msm->scale + gc->constants.half);
                        if (ix < SPEC_LOOKUP_TABLE_SIZE) {
                            n2 = msm->specTable[ix];
                            sum.r += lss->specular.r * ms->specular.r * n2;
                            sum.g += lss->specular.g * ms->specular.g * n2;
                            sum.b += lss->specular.b * ms->specular.b * n2;
                        } else {
                            sum.r += lss->specular.r * ms->specular.r;
                            sum.g += lss->specular.g * ms->specular.g;
                            sum.b += lss->specular.b * ms->specular.b;
                        }
                    }
                    sum.r += lss->diffuse.r * ms->diffuse.r * n1;
                    sum.g += lss->diffuse.g * ms->diffuse.g * n1;
                    sum.b += lss->diffuse.b * ms->diffuse.b * n1;
                }

                r += sum.r * attSpot;
                g += sum.g * attSpot;
                b += sum.b * attSpot;
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

        if (r <= zero)  r = zero;
        else if (r >= rl) r = rl;

        if (g <= zero)  g = zero;
        else if (g >= gl) g = gl;

        if (b <= zero)  b = zero;
        else if (b >= bl) b = bl;

        c->r = r;
        c->g = g;
        c->b = b;
        c->a = msm->alpha;
    }
    hwPackVertexColor(gc, vx);
}

/*
** gl_rasterpos.c -- s_rpos.c / s_rapi.c: the raster position.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "glshade.h"
#include "si_names.h"
#include "icdprotos.h"
#include <string.h>
#include "icdglob.h"
#include <stdlib.h>

/************************************************************************/
/* s_rpos.c: the ICD swaps one calcTexture proc (procs +0x2524 <- +0x252c) */
typedef void (__fastcall *__GLxf)(__GLcoord *res, const __GLfloat *v, __GLmatrixICD *m);
typedef GLuint (__fastcall *__GLclipCheck)(__GLcontext *gc, __GLvertexICD *v);
typedef void (__fastcall *__GLvalidate)(__GLcontext *gc, __GLvertexICD *v, GLuint needs);
#define __GL_NEEDS_RASTERPOS    0x1d
#define RASTERPOS_TAIL                                                                  \
    gc->state.current.validRasterPos = GL_TRUE;                                             \
                                                                                            \
    /* Copy info into vertex structure */                                                   \
    rp->normal = gc->state.current.normal;                                                  \
    if (gc->modes.rgbMode) {                                                                \
        rp->colors[0] = gc->state.current.color;                                            \
    } else {                                                                                \
        rp->colors[0].r = gc->state.current.userColorIndex;                                 \
    }                                                                                       \
    rp->texture[0] = gc->state.current.texture[0];                                          \
                                                                                            \
    /* This may be generous, but it will work */                                            \
    needs = (*(GLuint (__fastcall *)(__GLcontext *))gc->vertex.getNeeds)(gc) | __GL_NEEDS_RASTERPOS; \
                                                                                            \
    cc = gc->procs.calcColor;                                                               \
    gc->procs.calcColor = gc->procs.calcRasterColor;                                        \
    ct = gc->procs.calcTexture;                                                             \
    gc->procs.calcTexture = gc->procs.calcTextureUnit[1];                                   \
    (*(__GLvalidate)gc->procs.validateVertex[(rp->flagBits >> 14) & 3])(gc, rp, needs);  \
    gc->procs.calcTexture = ct;                                                             \
    gc->procs.calcColor = cc;                                                               \
                                                                                            \
    if (gc->renderMode == GL_SELECT) {                                                      \
        __glSelectPoint(gc, rp);                                                            \
    }

/* 69093c30 -- __glRasterPos2 */
void __fastcall __glRasterPos2(__GLcontext *gc, const __GLfloat v[2])
{
    __GLvertexICD *rp = &gc->state.current.rasterPos;
    struct __GLtransformICDRec *tr;
    GLuint needs;
    __GLproc cc, ct;

    rp->obj.x = v[0];
    rp->obj.y = v[1];
    rp->obj.z = __GL_ZERO_F;
    rp->obj.w = __GL_ONE_F;

    tr = gc->transform.modelView;
    (*(__GLxf)tr->mvp.xf2)(&rp->clip, &rp->obj.x, &tr->mvp);

    /* clip check the raster pos */
    rp->flagBits &= __GL_CLIP_ALL_MASK;
    rp->flagBits |= 0x4000;
    if ((*(__GLclipCheck)gc->procs.clipCheck2)(gc, rp) & __GL_CLIP_ALL_MASK) {
        gc->state.current.validRasterPos = GL_FALSE;
        return;
    }
    RASTERPOS_TAIL
}

/* 69093d90 -- __glRasterPos3 */
void __fastcall __glRasterPos3(__GLcontext *gc, const __GLfloat v[3])
{
    __GLvertexICD *rp = &gc->state.current.rasterPos;
    struct __GLtransformICDRec *tr;
    GLuint needs;
    __GLproc cc, ct;

    rp->obj.x = v[0];
    rp->obj.y = v[1];
    rp->obj.z = v[2];
    rp->obj.w = __GL_ONE_F;

    tr = gc->transform.modelView;
    (*(__GLxf)tr->mvp.xf3)(&rp->clip, &rp->obj.x, &tr->mvp);

    /* clip check the raster pos */
    rp->flagBits &= __GL_CLIP_ALL_MASK;
    rp->flagBits |= 0x8000;
    if ((*(__GLclipCheck)gc->procs.clipCheck3)(gc, rp) & __GL_CLIP_ALL_MASK) {
        gc->state.current.validRasterPos = GL_FALSE;
        return;
    }
    RASTERPOS_TAIL
}

/* 69093ef0 -- __glRasterPos4 */
void __fastcall __glRasterPos4(__GLcontext *gc, const __GLfloat v[4])
{
    __GLvertexICD *rp = &gc->state.current.rasterPos;
    struct __GLtransformICDRec *tr;
    GLuint needs;
    __GLproc cc, ct;

    rp->obj.x = v[0];
    rp->obj.y = v[1];
    rp->obj.z = v[2];
    rp->obj.w = v[3];

    tr = gc->transform.modelView;
    (*(__GLxf)tr->mvp.xf4)(&rp->clip, &rp->obj.x, &tr->mvp);

    /* clip check the raster pos */
    rp->flagBits &= __GL_CLIP_ALL_MASK;
    rp->flagBits |= 0xc000;
    if ((*(__GLclipCheck)gc->procs.clipCheck4)(gc, rp) & __GL_CLIP_ALL_MASK) {
        gc->state.current.validRasterPos = GL_FALSE;
        return;
    }
    RASTERPOS_TAIL
}

/************************************************************************/
/* s_rapi.c: the double vector forms */

/* 690c20f0 -- __glim_RasterPos2dv */
void __stdcall __glim_RasterPos2dv(const GLdouble v[2])
{
    __GLfloat vv[2];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    vv[0] = v[0]; vv[1] = v[1];
    (*gc->procs.rasterPos2)(gc, vv);
}

/* 690c2570 -- __glim_RasterPos3dv */
void __stdcall __glim_RasterPos3dv(const GLdouble v[3])
{
    __GLfloat vv[3];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2];
    (*gc->procs.rasterPos3)(gc, vv);
}

/* 690c29f0 -- __glim_RasterPos4dv */
void __stdcall __glim_RasterPos4dv(const GLdouble v[4])
{
    __GLfloat vv[4];
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    __GL_API_VAPI();

    vv[0] = v[0]; vv[1] = v[1]; vv[2] = v[2]; vv[3] = v[3];
    (*gc->procs.rasterPos4)(gc, vv);
}

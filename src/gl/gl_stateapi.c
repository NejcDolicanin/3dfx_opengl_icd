/*
** gl_stateapi.c -- original VA range 690b4000..690b8000: dl_list.c glCallList and __glDlistAllocOp2,
** EXT_fog_coord, and s_attrib.c state entries (ClearStencil, IndexFuncSGI, IndexMaterialSGI,
** PolygonStipple, ShadeModel, StencilMask).
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "icdprotos.h"
#include "si_names.h"

#define __GL_MAX_STENCIL_VALUE  ((1 << gc->modes.stencilBits) - 1)

typedef void *(__fastcall *__GLarenaAllocProcB40)(void *arena, GLuint size);

/* 690b4000 -- __glim_CallList (dl_list.c) */
void __stdcall __glim_CallList(GLuint list)
{
    __GL_SETUP();

    if (list == 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    DoCallList(list);
    gc->dlist.nesting = 0;
}

/* 690b44d0 -- __glDlistAllocOp2 (dl_list.c): an op with size bytes of data from the list arena */
__GLdlistOpICD *__fastcall __glDlistAllocOp2(__GLcontext *gc, GLuint size)
{
    __GLdlistOpICD *newDlistOp;

    newDlistOp = (__GLdlistOpICD *)(*(__GLarenaAllocProcB40)gc->procs.arenaAlloc)(gc->dlist.arena,
                                                                               size + 0x10);
    if (newDlistOp == NULL) {
        __glSetError(GL_OUT_OF_MEMORY);
        return NULL;
    }

    newDlistOp->size = size;
    newDlistOp->next = NULL;
    newDlistOp->dlistFree = NULL;
    newDlistOp->aligned = GL_FALSE;

    return newDlistOp;
}

/* 690b45f0 -- __glim_FogCoordfEXT: glFogCoordfEXT: the ICD stores 1 - c */
void __stdcall __glim_FogCoordfEXT(GLfloat coord)
{
    __glCurrentGC()->state.current.fogCoord = 1.0f - coord;
}

/* 690b4650 -- __glim_FogCoordfvEXT: glFogCoordfvEXT */
void __stdcall __glim_FogCoordfvEXT(const GLfloat *coord)
{
    __glCurrentGC()->state.current.fogCoord = 1.0f - *coord;
}

/* 690b4680 -- __glim_FogCoorddvEXT: glFogCoorddEXT / glFogCoorddvEXT (the double is subtracted in double
** precision) */
void __stdcall __glim_FogCoorddvEXT(const GLdouble *coord)
{
    __glCurrentGC()->state.current.fogCoord = (GLfloat)(1.0 - *coord);
}

/* 690b4c50 -- __glim_ClearStencil (s_attrib.c) */
void __stdcall __glim_ClearStencil(GLint s)
{
    __GL_SETUP_NOT_IN_BEGIN();

    gc->state.stencil.clear = (GLshort)(s & __GL_MAX_STENCIL_VALUE);
    __GL_DELAY_VALIDATE(gc);
}

/* 690b58a0 -- __glim_IndexFuncSGI (3dfx S_ATTRIB.C) */
void __stdcall __glim_IndexFuncSGI(GLenum func, GLfloat ref)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((func < GL_NEVER) || (func > GL_ALWAYS)) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    gc->state.raster.indexFunction = func;
    gc->state.raster.indexReference = ref;
    __GL_DELAY_VALIDATE(gc);
    gc->validateMask |= __GL_VALIDATE_INDEX_FUNC;
}

/* 690b59e0 -- __glim_IndexMaterialSGI (3dfx S_ATTRIB.C): face GL_FRONT / GL_BACK /
** GL_FRONT_AND_BACK, parameter GL_INDEX_OFFSET.  No validation is scheduled.
*/
void __stdcall __glim_IndexMaterialSGI(GLenum face, GLenum p)
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
      case GL_INDEX_OFFSET:
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    gc->state.light.indexMaterialFace = face;
    gc->state.light.indexMaterialParam = p;
}

/* 690b6d80 -- __glim_PolygonStipple (s_attrib.c) */
void __stdcall __glim_PolygonStipple(const GLubyte *mask)
{
    GLubyte *stipple;
    __GL_SETUP_NOT_IN_BEGIN();

    stipple = &gc->state.polygonStipple.stipple[0];
    __glFillImage(gc, 32, 32, GL_COLOR_INDEX, GL_BITMAP, mask, stipple);
    (*gc->procs.convertPolygonStipple)(gc);
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
}

/* 690b6e10 -- __glim_ShadeModel (s_attrib.c).  ICD: a change of the model also sets dirty bit
** 0x4000 (the hardware shading state).
*/
void __stdcall __glim_ShadeModel(GLenum sm)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((sm < GL_FLAT) || (sm > GL_SMOOTH)) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    if (gc->state.light.shadingModel != sm) {
        gc->state.light.shadingModel = sm;
        gc->dirtyMask |= __GL_DIRTY_SHADEMODEL_ICD;
    }
    __GL_DELAY_VALIDATE(gc);
}

/* 690b6ec0 -- __glim_StencilMask (s_attrib.c) */
void __stdcall __glim_StencilMask(GLuint sm)
{
    __GL_SETUP_NOT_IN_BEGIN();

    gc->state.stencil.writeMask = (GLshort)(sm & __GL_MAX_STENCIL_VALUE);
    __GL_DELAY_VALIDATE(gc);
    gc->validateMask |= __GL_VALIDATE_STENCIL_FUNC;
}

/* 690b4620 -- __glim_FogCoorddEXT: glFogCoorddEXT: the ICD stores 1 - c (computed in double) */
void __stdcall __glim_FogCoorddEXT(GLdouble coord)
{
    __glCurrentGC()->state.current.fogCoord = 1.0 - coord;
}

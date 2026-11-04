/*
** si_dl_mopt.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/dl_mopt.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690dff10 -- __glle_FastMaterial */
const GLubyte * __fastcall __glle_FastMaterial(const GLubyte *PC)
{
    GLint dirtyBits, frontDirty, backDirty;
    const __GLfloat *ptr;
    __GLmaterialState *ms;
    __GLmaterialMachineICD *msm;
    GLenum face;
    __GLspecLUTEntry *lutEntry;
    __GL_SETUP();

    ptr = (const __GLfloat *) PC;
    frontDirty = backDirty = 0;

    /*
    ** If we are in the middle of contructing a primitive, we possibly
    ** need to validate the front and back colors of the vertices which 
    ** we have queued.
    */
    if (__GL_IN_BEGIN() && (*gc->vertex.getMaterialNeeds)(gc)) {
        (*gc->procs.matValidate)(gc);
    }

    while ((face = (*(const GLenum *) (ptr++))) != 0) {
        dirtyBits = (*(const GLint *) (ptr++));
        switch(face) {
          case GL_FRONT:
            ms = &gc->state.light.front;
            msm = &gc->light.front;
            frontDirty |= dirtyBits;
            break;
          case GL_BACK:
            ms = &gc->state.light.back;
            msm = &gc->light.back;
            backDirty |= dirtyBits;
            break;
        }
        if (dirtyBits & __GL_MATERIAL_AMBIENT) {
            ms->ambient.r = *ptr++;
            ms->ambient.g = *ptr++;
            ms->ambient.b = *ptr++;
            ms->ambient.a = *ptr++;
        }
        if (dirtyBits & __GL_MATERIAL_DIFFUSE) {
            ms->diffuse.r = *ptr++;
            ms->diffuse.g = *ptr++;
            ms->diffuse.b = *ptr++;
            ms->diffuse.a = *ptr++;
        }
        if (dirtyBits & __GL_MATERIAL_SPECULAR) {
            ms->specular.r = *ptr++;
            ms->specular.g = *ptr++;
            ms->specular.b = *ptr++;
            ms->specular.a = *ptr++;
        }
        if (dirtyBits & __GL_MATERIAL_EMISSIVE) {
            __GLfloat r,g,b,a;

            r = *ptr++ * gc->constants.redScale;
            g = *ptr++ * gc->constants.greenScale;
            b = *ptr++ * gc->constants.blueScale;
            a = *ptr++ * gc->constants.alphaScale;
            ms->emissive.r = r;
            ms->emissive.g = g;
            ms->emissive.b = b;
            ms->emissive.a = a;
        }
        if (dirtyBits & __GL_MATERIAL_SHININESS) {
            msm->specularExponent = ms->specularExponent = *ptr++;
            lutEntry = (*(__GLspecLUTEntry * const *) (ptr++));
            lutEntry->refcount++;
            __glFreeSpecLUT(gc, msm->cache);
            msm->cache = lutEntry;
            msm->threshold = lutEntry->threshold;
            msm->scale = lutEntry->scale;
            msm->specTable = lutEntry->table;
        }
        if (dirtyBits & __GL_MATERIAL_COLORINDEXES) {
            ms->cmapa = *ptr++;
            ms->cmaps = *ptr++;
            ms->cmapd = *ptr++;
        }
    }

    if (frontDirty | backDirty) {
        __glValidateMaterial(gc, frontDirty, backDirty);
    }

    return (const GLubyte *) ptr;
}

/*
** si_s_tex.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_tex.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 6909e910 -- IsLegalRange */
GLboolean __fastcall IsLegalRange(__GLcontext *gc, GLsizei r, GLint border)
{
#if defined(__GL_LINT)
    gc = gc;
#endif
    r -= border * 2;
    if ((r < 0) || (r & (r - 1))) {
        __glSetError(GL_INVALID_VALUE);
        return GL_FALSE;
    }
    return GL_TRUE;
}

/* 6909f1a0 -- Dot (s_tex.c) */
__GLfloat __fastcall Dot(const __GLcoord *v1, const __GLcoord *v2)
{
    return (v1->x * v2->x + v1->y * v2->y + v1->z * v2->z);
}

/* 6909f960 -- CheckTexSubImageRange */
GLboolean __fastcall CheckTexSubImageRange(__GLcontext *gc, __GLmipMapLevelICD *lp,
                                       GLint offset, GLsizei size, GLsizei max)
{
#if defined(__GL_LINT)
    gc = gc;
#endif
    if ((size < 0) || (offset < -lp->border) || (offset+size > max-lp->border))
    {
        __glSetError(GL_INVALID_VALUE);
        return GL_FALSE;
    }
    return GL_TRUE;
}

/* 6909e220 -- __glTexEnviv_size */
GLint __fastcall __glTexEnviv_size(GLenum e)
{
    return __glTexEnvfv_size(e);
}

/* 6909e200 -- __glTexEnvfv_size */
GLint __fastcall __glTexEnvfv_size(GLenum e)
{
    switch (e) {
      case GL_TEXTURE_ENV_MODE:
        return 1;
      case GL_TEXTURE_ENV_COLOR:
        return 4;
      default:
        return -1;
    }
}

/* 6909d110 -- __glTexGendv_size */
GLint __fastcall __glTexGendv_size(GLenum e)
{
    switch (e) {
      case GL_TEXTURE_GEN_MODE:
        return 1;
      case GL_OBJECT_PLANE:
      case GL_EYE_PLANE:
        return 4;
      default:
        return -1;
    }
}

/* 6909d140 -- __glTexGenfv_size */
GLint __fastcall __glTexGenfv_size(GLenum e)
{
    return __glTexGendv_size(e);
}

/* 6909d150 -- __glTexGeniv_size */
GLint __fastcall __glTexGeniv_size(GLenum e)
{
    return __glTexGendv_size(e);
}

/* 690a0530 -- IsTextureResident */
GLboolean __fastcall IsTextureResident(__GLcontext *gc, __GLtextureICD *tex)
{
    GLint i;
    int maxMipMapLevel = gc->constants.maxMipMapLevel;

    for (i=0; i < maxMipMapLevel; i++) {
        if (!tex->level[i]->buffer) {
            return GL_FALSE;
        }
        /* If not-mipmapping, return after checking the first level. */
        switch (tex->params.minFilter) {
          case GL_NEAREST:
          case GL_LINEAR:
            return GL_TRUE;
          default:
            break;
        }
        if (tex->level[i]->width == 1 && tex->level[i]->height == 1) break;
    }

    return GL_TRUE;
}

/* 6909f0d0 -- SphereGen */
void __fastcall SphereGen(__GLcontext *gc, __GLvertexICD *vx, __GLcoord *result)
{
    __GLcoord u, r;
    __GLfloat m, ndotu;

    /* Get unit vector from origin to the vertex in eye coordinates into u */
    (*gc->procs.normalize)(&u.x, &vx->eye.x);

    /* Dot the normal with the unit position u */
    ndotu = Dot(&vx->normal, &u);

    /* Compute r */
    r.x = u.x - 2 * vx->normal.x * ndotu;
    r.y = u.y - 2 * vx->normal.y * ndotu;
    r.z = u.z - 2 * vx->normal.z * ndotu;

    /* Compute m */
    m = 2 * __GL_SQRTF(r.x*r.x + r.y*r.y + (r.z + 1) * (r.z + 1));

    if (m) {
        result->x = r.x / m + __glHalf;
        result->y = r.y / m + __glHalf;
    } else {
        result->x = __glHalf;
        result->y = __glHalf;
    }
}

/* 690a0580 -- __glim_IsTexture */
GLboolean __stdcall __glim_IsTexture(GLuint texture)
{
    __GLtextureICD *tex;
    __GL_SETUP_NOT_IN_BEGIN2();
    __GL_API_GET();

    if (0 == texture) return GL_FALSE;

    tex = (__GLtextureICD *)
            __glNamesLockData(gc, gc->texture.namesArray, texture);

    if (NULL == tex) return GL_FALSE;

    __glNamesUnlockData(gc, tex);

    return GL_TRUE;
}

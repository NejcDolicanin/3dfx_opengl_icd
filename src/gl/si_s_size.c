/*
** si_s_size.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_size.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690e5080 -- __glFogiv_size */
GLint __fastcall __glFogiv_size(GLenum pname)
{
    switch (pname) {
      case GL_FOG_COLOR:        return 4;
      case GL_FOG_DENSITY:        return 1;
      case GL_FOG_END:                return 1;
      case GL_FOG_MODE:                return 1;
      case GL_FOG_INDEX:        return 1;
      case GL_FOG_START:        return 1;
      default:
        return -1;
    }
}

/* 690e50c0 -- __glFogfv_size */
GLint __fastcall __glFogfv_size(GLenum pname)
{
    return __glFogiv_size(pname);
}

/* 690e50d0 -- __glLightfv_size */
GLint __fastcall __glLightfv_size(GLenum pname)
{
    switch (pname) {
      case GL_SPOT_EXPONENT:                return 1;
      case GL_SPOT_CUTOFF:                return 1;
      case GL_AMBIENT:                        return 4;
      case GL_DIFFUSE:                        return 4;
      case GL_SPECULAR:                        return 4;
      case GL_POSITION:                        return 4;
      case GL_SPOT_DIRECTION:                return 3;
      case GL_CONSTANT_ATTENUATION:        return 1;
      case GL_LINEAR_ATTENUATION:        return 1;
      case GL_QUADRATIC_ATTENUATION:        return 1;
      default:
        return -1;
    }
}

/* 690e5160 -- __glMaterialfv_size */
GLint __fastcall __glMaterialfv_size(GLenum pname)
{
    switch (pname) {
      case GL_SHININESS:                return 1;
      case GL_EMISSION:                        return 4;
      case GL_AMBIENT:                        return 4;
      case GL_DIFFUSE:                        return 4;
      case GL_SPECULAR:                        return 4;
      case GL_AMBIENT_AND_DIFFUSE:        return 4;
      case GL_COLOR_INDEXES:                return 3;
      default:
        return -1;
    }
}

/* 690e51a0 -- __glMaterialiv_size */
GLint __fastcall __glMaterialiv_size(GLenum pname)
{
    return __glMaterialfv_size(pname);
}

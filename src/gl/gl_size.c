/*
** gl_size.c -- s_size.c: the parameter counts of the vector entries (__gl*_size) (SI port:
** si_s_size.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "glsitypes.h"
#include <stdlib.h>

#define POINT_SIZE_MIN_EXT              0x8126
#define POINT_SIZE_MAX_EXT              0x8127
#define POINT_FADE_THRESHOLD_SIZE_EXT   0x8128
#define DISTANCE_ATTENUATION_EXT        0x8129

/* ---- s_size.c-style parameter counts ---- */
/* 690e5130 -- __glLightModelfv_size (no GL_LIGHT_MODEL_COLOR_CONTROL in the ICD) */
GLint __fastcall __glLightModelfv_size(GLenum pname)
{
    switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:		return 4;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:		return 1;
      case GL_LIGHT_MODEL_TWO_SIDE:		return 1;
      default:
	return -1;
    }
}

/* 690e5150 -- __glLightModeliv_size: a jump to __glLightModelfv_size above (not __glLightiv_size) */
GLint __fastcall __glLightModeliv_size(GLenum pname)
{
    return __glLightModelfv_size(pname);
}

/* 690e51b0 -- __glPointParameterfvEXT_size (0x8126..0x8129) */
GLint __fastcall __glPointParameterfvEXT_size(GLenum pname)
{
    switch (pname) {
      case POINT_SIZE_MIN_EXT:		return 1;
      case POINT_SIZE_MAX_EXT:		return 1;
      case POINT_FADE_THRESHOLD_SIZE_EXT:	return 1;
      case DISTANCE_ATTENUATION_EXT:	return 3;
      default:
	return -1;
    }
}

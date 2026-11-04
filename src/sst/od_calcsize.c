/*
** od_calcsize.c -- 69056950, calc_size_and_scale of the 3dfx vertex-array code generator (GEOM_OG.C).
** The original compiled it without optimisation (/Od).
*/
#include "wglpriv.h"

/* FAC(N): the GL conversion factor of an N-bit integer, 1 / (2^N - 1) */
#define FAC(N)  ((GLfloat)(1.0 / ((double)(unsigned long)((1UL << (N)) - 1UL))))

/* 69056950 -- calc_size_and_scale (GEOM_OG.C): the byte size of one array component of `type` and the
** factor that maps it to [0, 1] (signed types get the unsigned factor: the caller's (2c + 1) load
** makes up the rest); no default case (the ancestor asserted) */
#pragma optimize("", off)
void __fastcall calc_size_and_scale(GLuint *isize, GLfloat *scale, GLenum type)
{
  switch (type) {
  case GL_BYTE:
    *isize = 1;
    *scale = FAC(8);
    break;
  case GL_UNSIGNED_BYTE:
    *isize = 1;
    *scale = FAC(8);
    break;
  case GL_SHORT:
    *isize = 2;
    *scale = FAC(16);
    break;
  case GL_UNSIGNED_SHORT:
    *isize = 2;
    *scale = FAC(16);
    break;
  case GL_INT:
    *isize = 4;
    *scale = FAC(32);
    break;
  case GL_UNSIGNED_INT:
    *isize = 4;
    *scale = FAC(32);
    break;
  case GL_FLOAT:
    *isize = 4;
    *scale = 1.0f;
    break;
  case GL_DOUBLE:
    *isize = 8;
    *scale = 1.0f;
  }
}
#pragma optimize("", on)

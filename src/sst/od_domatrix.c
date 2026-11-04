/*
** od_domatrix.c -- doMatrix: the og generator of a 4x4 matrix transform (the ancestor's doMatrix,
** SST_OGI.C), compiled without optimisation (/Od) in the original.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "og.h"


/* a memory operand holding the float constant v (the ancestor's fpk): a float-only callee, no EDX load */
/* the matrix element m[r][c] at [ecx + r * 16 + c * 4] (the caller loads ecx) */
#define MAT(r, c)       og_memBase(gc, IMM((r) * 16 + (c) * 4), R(ecx))

/* 69057380 -- doMatrix: emit (out x, y, z, w) = (x, y, z, w) * m for the vertex at [edx] (vertexSize 2:
** z = 0, w = 1; 3: w = 1) and the matrix at [ecx], shaped by its type: __GL_MT_IS2DNR / IDENTITY / IS2DNRSC
** scale and translate only (__glXForm*_2DNRW), anything else is the full product.  3DNow (cpuCaps & 2), SSE
** (& 4; og_movss = movss) or x87.  Unlike the ancestor (which returned w on the x87 stack) every path stores
** w to outw; mat is unused (the caller emits the load of ecx).  The 2DNR paths leave outz unwritten for a
** 2-component vertex, as the ancestor does. */
#pragma optimize("", off)
GLubyte *__fastcall doMatrix(__GLcontext *gc, GLubyte *pc, OgOperand *outx, OgOperand *outy, OgOperand *outz,
                                 OgOperand *outw, GLenum matrixType, const __GLfloat *mat, GLuint vertexSize)
{
    OgOperand *z, *y, *x, *w;       /* the vertex components (declared in this order: the original's frame) */

    x = og_memBase(gc, NULL, R(edx));
    y = og_memBase(gc, IMM(4), R(edx));
    z = og_memBase(gc, IMM(8), R(edx));
    w = og_memBase(gc, IMM(0xc), R(edx));
    /* implied z and w for smaller vertices; w == NULL means 1 */
    switch (vertexSize) {
      case 2:
        z = FPK(0.0);
      case 3:
        w = NULL;
    }
    if ((gc->constants.cpuCaps & __GL_CPU_3DNOW) != 0) {
        /* 3DNow */
        switch (matrixType) {
          case __GL_MT_IS2DNR:
          case __GL_MT_IDENTITY:
          case __GL_MT_IS2DNRSC:
            /* x' = x * m00 + w * m30, y' = y * m11 + w * m31, z' = z * m22 + w * m32 */
            EMIT(og_femms(gc));
            if (vertexSize == 2) {
                EMIT(og_movd(gc, R(mm0), x));
                EMIT(og_movd(gc, R(mm1), y));
                EMIT(og_movd(gc, R(mm2), MAT(3, 0)));
                EMIT(og_movd(gc, R(mm3), MAT(3, 1)));
                EMIT(og_pfmul(gc, R(mm0), MAT(0, 0)));
                EMIT(og_pfmul(gc, R(mm1), MAT(1, 1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm2)));
                EMIT(og_pfadd(gc, R(mm1), R(mm3)));
                EMIT(og_movd(gc, outx, R(mm0)));
                EMIT(og_movd(gc, outy, R(mm1)));
            } else if (vertexSize == 3) {
                EMIT(og_movd(gc, R(mm0), x));
                EMIT(og_movd(gc, R(mm1), y));
                EMIT(og_movd(gc, R(mm2), z));
                EMIT(og_movd(gc, R(mm3), MAT(3, 0)));
                EMIT(og_movd(gc, R(mm4), MAT(3, 1)));
                EMIT(og_movd(gc, R(mm5), MAT(3, 2)));
                EMIT(og_pfmul(gc, R(mm0), MAT(0, 0)));
                EMIT(og_pfmul(gc, R(mm1), MAT(1, 1)));
                EMIT(og_pfmul(gc, R(mm2), MAT(2, 2)));
                EMIT(og_pfadd(gc, R(mm0), R(mm3)));
                EMIT(og_pfadd(gc, R(mm1), R(mm4)));
                EMIT(og_pfadd(gc, R(mm2), R(mm5)));
                EMIT(og_movd(gc, outx, R(mm0)));
                EMIT(og_movd(gc, outy, R(mm1)));
                EMIT(og_movd(gc, outz, R(mm2)));
            } else {
                EMIT(og_movd(gc, R(mm0), x));
                EMIT(og_movd(gc, R(mm1), y));
                EMIT(og_movd(gc, R(mm2), z));
                EMIT(og_movd(gc, R(mm3), w));
                EMIT(og_pfmul(gc, R(mm0), MAT(0, 0)));
                EMIT(og_pfmul(gc, R(mm1), MAT(1, 1)));
                EMIT(og_pfmul(gc, R(mm2), MAT(2, 2)));
                EMIT(og_movq(gc, R(mm4), R(mm3)));
                EMIT(og_movq(gc, R(mm5), R(mm3)));
                EMIT(og_pfmul(gc, R(mm3), MAT(3, 0)));
                EMIT(og_pfmul(gc, R(mm4), MAT(3, 1)));
                EMIT(og_pfmul(gc, R(mm5), MAT(3, 2)));
                EMIT(og_pfadd(gc, R(mm0), R(mm3)));
                EMIT(og_pfadd(gc, R(mm1), R(mm4)));
                EMIT(og_pfadd(gc, R(mm2), R(mm5)));
                EMIT(og_movd(gc, outx, R(mm0)));
                EMIT(og_movd(gc, outy, R(mm1)));
                EMIT(og_movd(gc, outz, R(mm2)));
            }
            EMIT(og_femms(gc));
            /* w' = w (x87) */
            if (w != NULL) {
                EMIT(og_fld(gc, w));
                EMIT(og_fstp(gc, outw));
            } else {
                EMIT(og_fld1(gc));
                EMIT(og_fstp(gc, outw));
            }
            break;
          default:
            /* each output is one column: x * m0c + y * m1c + z * m2c + (w * m3c or m3c) */
            EMIT(og_femms(gc));
            if (w == NULL) {
                EMIT(og_movd(gc, R(mm0), x));
                EMIT(og_movd(gc, R(mm1), y));
                EMIT(og_movd(gc, R(mm2), z));
                EMIT(og_pfmul(gc, R(mm0), MAT(0, 0)));
                EMIT(og_pfmul(gc, R(mm1), MAT(1, 0)));
                EMIT(og_pfmul(gc, R(mm2), MAT(2, 0)));
                EMIT(og_pfadd(gc, R(mm0), R(mm1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm2)));
                EMIT(og_pfadd(gc, R(mm0), MAT(3, 0)));
                EMIT(og_movd(gc, outx, R(mm0)));
                EMIT(og_movd(gc, R(mm0), x));
                EMIT(og_movd(gc, R(mm1), y));
                EMIT(og_movd(gc, R(mm2), z));
                EMIT(og_pfmul(gc, R(mm0), MAT(0, 1)));
                EMIT(og_pfmul(gc, R(mm1), MAT(1, 1)));
                EMIT(og_pfmul(gc, R(mm2), MAT(2, 1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm2)));
                EMIT(og_pfadd(gc, R(mm0), MAT(3, 1)));
                EMIT(og_movd(gc, outy, R(mm0)));
                EMIT(og_movd(gc, R(mm0), x));
                EMIT(og_movd(gc, R(mm1), y));
                EMIT(og_movd(gc, R(mm2), z));
                EMIT(og_pfmul(gc, R(mm0), MAT(0, 2)));
                EMIT(og_pfmul(gc, R(mm1), MAT(1, 2)));
                EMIT(og_pfmul(gc, R(mm2), MAT(2, 2)));
                EMIT(og_pfadd(gc, R(mm0), R(mm1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm2)));
                EMIT(og_pfadd(gc, R(mm0), MAT(3, 2)));
                EMIT(og_movd(gc, outz, R(mm0)));
                EMIT(og_movd(gc, R(mm0), x));
                EMIT(og_movd(gc, R(mm1), y));
                EMIT(og_movd(gc, R(mm2), z));
                EMIT(og_pfmul(gc, R(mm0), MAT(0, 3)));
                EMIT(og_pfmul(gc, R(mm1), MAT(1, 3)));
                EMIT(og_pfmul(gc, R(mm2), MAT(2, 3)));
                EMIT(og_pfadd(gc, R(mm0), R(mm1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm2)));
                EMIT(og_pfadd(gc, R(mm0), MAT(3, 3)));
                EMIT(og_movd(gc, outw, R(mm0)));
            } else {
                EMIT(og_movd(gc, R(mm0), x));
                EMIT(og_movd(gc, R(mm1), y));
                EMIT(og_movd(gc, R(mm2), z));
                EMIT(og_movd(gc, R(mm3), w));
                EMIT(og_pfmul(gc, R(mm0), MAT(0, 0)));
                EMIT(og_pfmul(gc, R(mm1), MAT(1, 0)));
                EMIT(og_pfmul(gc, R(mm2), MAT(2, 0)));
                EMIT(og_pfmul(gc, R(mm3), MAT(3, 0)));
                EMIT(og_pfadd(gc, R(mm0), R(mm1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm2)));
                EMIT(og_pfadd(gc, R(mm0), R(mm3)));
                EMIT(og_movd(gc, outx, R(mm0)));
                EMIT(og_movd(gc, R(mm0), x));
                EMIT(og_movd(gc, R(mm1), y));
                EMIT(og_movd(gc, R(mm2), z));
                EMIT(og_movd(gc, R(mm3), w));
                EMIT(og_pfmul(gc, R(mm0), MAT(0, 1)));
                EMIT(og_pfmul(gc, R(mm1), MAT(1, 1)));
                EMIT(og_pfmul(gc, R(mm2), MAT(2, 1)));
                EMIT(og_pfmul(gc, R(mm3), MAT(3, 1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm2)));
                EMIT(og_pfadd(gc, R(mm0), R(mm3)));
                EMIT(og_movd(gc, outy, R(mm0)));
                EMIT(og_movd(gc, R(mm0), x));
                EMIT(og_movd(gc, R(mm1), y));
                EMIT(og_movd(gc, R(mm2), z));
                EMIT(og_movd(gc, R(mm3), w));
                EMIT(og_pfmul(gc, R(mm0), MAT(0, 2)));
                EMIT(og_pfmul(gc, R(mm1), MAT(1, 2)));
                EMIT(og_pfmul(gc, R(mm2), MAT(2, 2)));
                EMIT(og_pfmul(gc, R(mm3), MAT(3, 2)));
                EMIT(og_pfadd(gc, R(mm0), R(mm1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm2)));
                EMIT(og_pfadd(gc, R(mm0), R(mm3)));
                EMIT(og_movd(gc, outz, R(mm0)));
                EMIT(og_movd(gc, R(mm0), x));
                EMIT(og_movd(gc, R(mm1), y));
                EMIT(og_movd(gc, R(mm2), z));
                EMIT(og_movd(gc, R(mm3), w));
                EMIT(og_pfmul(gc, R(mm0), MAT(0, 3)));
                EMIT(og_pfmul(gc, R(mm1), MAT(1, 3)));
                EMIT(og_pfmul(gc, R(mm2), MAT(2, 3)));
                EMIT(og_pfmul(gc, R(mm3), MAT(3, 3)));
                EMIT(og_pfadd(gc, R(mm0), R(mm1)));
                EMIT(og_pfadd(gc, R(mm0), R(mm2)));
                EMIT(og_pfadd(gc, R(mm0), R(mm3)));
                EMIT(og_movd(gc, outw, R(mm0)));
            }
            EMIT(og_femms(gc));
        }
    } else if ((gc->constants.cpuCaps & __GL_CPU_SSE) != 0) {
        /* SSE (og_movss = movss), the same products in scalar registers */
        switch (matrixType) {
          case __GL_MT_IS2DNR:
          case __GL_MT_IDENTITY:
          case __GL_MT_IS2DNRSC:
            if (vertexSize == 2) {
                EMIT(og_movss(gc, R(mm0), x));
                EMIT(og_movss(gc, R(mm1), y));
                EMIT(og_movss(gc, R(mm2), MAT(3, 0)));
                EMIT(og_movss(gc, R(mm3), MAT(3, 1)));
                EMIT(og_movss(gc, R(mm4), MAT(0, 0)));
                EMIT(og_movss(gc, R(mm5), MAT(1, 1)));
                EMIT(og_mulss(gc, R(mm0), R(mm4)));
                EMIT(og_mulss(gc, R(mm1), R(mm5)));
                EMIT(og_addss(gc, R(mm0), R(mm2)));
                EMIT(og_addss(gc, R(mm1), R(mm3)));
                EMIT(og_movss(gc, outx, R(mm0)));
                EMIT(og_movss(gc, outy, R(mm1)));
            } else if (vertexSize == 3) {
                EMIT(og_movss(gc, R(mm0), x));
                EMIT(og_movss(gc, R(mm1), y));
                EMIT(og_movss(gc, R(mm2), z));
                EMIT(og_movss(gc, R(mm3), MAT(0, 0)));
                EMIT(og_movss(gc, R(mm4), MAT(1, 1)));
                EMIT(og_movss(gc, R(mm5), MAT(2, 2)));
                EMIT(og_mulss(gc, R(mm0), R(mm3)));
                EMIT(og_mulss(gc, R(mm1), R(mm4)));
                EMIT(og_mulss(gc, R(mm2), R(mm5)));
                EMIT(og_movss(gc, R(mm3), MAT(3, 0)));
                EMIT(og_movss(gc, R(mm4), MAT(3, 1)));
                EMIT(og_movss(gc, R(mm5), MAT(3, 2)));
                EMIT(og_addss(gc, R(mm0), R(mm3)));
                EMIT(og_addss(gc, R(mm1), R(mm4)));
                EMIT(og_addss(gc, R(mm2), R(mm5)));
                EMIT(og_movss(gc, outx, R(mm0)));
                EMIT(og_movss(gc, outy, R(mm1)));
                EMIT(og_movss(gc, outz, R(mm2)));
            } else {
                EMIT(og_movss(gc, R(mm0), x));
                EMIT(og_movss(gc, R(mm1), y));
                EMIT(og_movss(gc, R(mm2), z));
                EMIT(og_movss(gc, R(mm3), w));
                EMIT(og_movss(gc, R(mm4), MAT(0, 0)));
                EMIT(og_movss(gc, R(mm5), MAT(1, 1)));
                EMIT(og_movss(gc, R(mm6), MAT(2, 2)));
                EMIT(og_mulss(gc, R(mm0), R(mm4)));
                EMIT(og_mulss(gc, R(mm1), R(mm5)));
                EMIT(og_mulss(gc, R(mm2), R(mm6)));
                EMIT(og_movss(gc, R(mm4), MAT(3, 0)));
                EMIT(og_movss(gc, R(mm5), MAT(3, 1)));
                EMIT(og_movss(gc, R(mm6), MAT(3, 2)));
                EMIT(og_mulss(gc, R(mm4), R(mm3)));
                EMIT(og_mulss(gc, R(mm5), R(mm3)));
                EMIT(og_mulss(gc, R(mm6), R(mm3)));
                EMIT(og_addss(gc, R(mm0), R(mm4)));
                EMIT(og_addss(gc, R(mm1), R(mm5)));
                EMIT(og_addss(gc, R(mm2), R(mm6)));
                EMIT(og_movss(gc, outx, R(mm0)));
                EMIT(og_movss(gc, outy, R(mm1)));
                EMIT(og_movss(gc, outz, R(mm2)));
            }
            if (w != NULL) {
                EMIT(og_fld(gc, w));
                EMIT(og_fstp(gc, outw));
            } else {
                EMIT(og_fld1(gc));
                EMIT(og_fstp(gc, outw));
            }
            break;
          default:
            if (w == NULL) {
                EMIT(og_movss(gc, R(mm0), x));
                EMIT(og_movss(gc, R(mm1), y));
                EMIT(og_movss(gc, R(mm2), z));
                EMIT(og_movss(gc, R(mm3), MAT(0, 0)));
                EMIT(og_movss(gc, R(mm4), MAT(1, 0)));
                EMIT(og_movss(gc, R(mm5), MAT(2, 0)));
                EMIT(og_movss(gc, R(mm6), MAT(3, 0)));
                EMIT(og_mulss(gc, R(mm0), R(mm3)));
                EMIT(og_mulss(gc, R(mm1), R(mm4)));
                EMIT(og_mulss(gc, R(mm2), R(mm5)));
                EMIT(og_addss(gc, R(mm0), R(mm1)));
                EMIT(og_addss(gc, R(mm2), R(mm6)));
                EMIT(og_addss(gc, R(mm0), R(mm2)));
                EMIT(og_movss(gc, outx, R(mm0)));
                EMIT(og_movss(gc, R(mm0), x));
                EMIT(og_movss(gc, R(mm1), y));
                EMIT(og_movss(gc, R(mm2), z));
                EMIT(og_movss(gc, R(mm3), MAT(0, 1)));
                EMIT(og_movss(gc, R(mm4), MAT(1, 1)));
                EMIT(og_movss(gc, R(mm5), MAT(2, 1)));
                EMIT(og_movss(gc, R(mm6), MAT(3, 1)));
                EMIT(og_mulss(gc, R(mm0), R(mm3)));
                EMIT(og_mulss(gc, R(mm1), R(mm4)));
                EMIT(og_mulss(gc, R(mm2), R(mm5)));
                EMIT(og_addss(gc, R(mm0), R(mm1)));
                EMIT(og_addss(gc, R(mm2), R(mm6)));
                EMIT(og_addss(gc, R(mm0), R(mm2)));
                EMIT(og_movss(gc, outy, R(mm0)));
                EMIT(og_movss(gc, R(mm0), x));
                EMIT(og_movss(gc, R(mm1), y));
                EMIT(og_movss(gc, R(mm2), z));
                EMIT(og_movss(gc, R(mm3), MAT(0, 2)));
                EMIT(og_movss(gc, R(mm4), MAT(1, 2)));
                EMIT(og_movss(gc, R(mm5), MAT(2, 2)));
                EMIT(og_movss(gc, R(mm6), MAT(3, 2)));
                EMIT(og_mulss(gc, R(mm0), R(mm3)));
                EMIT(og_mulss(gc, R(mm1), R(mm4)));
                EMIT(og_mulss(gc, R(mm2), R(mm5)));
                EMIT(og_addss(gc, R(mm0), R(mm1)));
                EMIT(og_addss(gc, R(mm2), R(mm6)));
                EMIT(og_addss(gc, R(mm0), R(mm2)));
                EMIT(og_movss(gc, outz, R(mm0)));
                EMIT(og_movss(gc, R(mm0), x));
                EMIT(og_movss(gc, R(mm1), y));
                EMIT(og_movss(gc, R(mm2), z));
                EMIT(og_movss(gc, R(mm3), MAT(0, 3)));
                EMIT(og_movss(gc, R(mm4), MAT(1, 3)));
                EMIT(og_movss(gc, R(mm5), MAT(2, 3)));
                EMIT(og_movss(gc, R(mm6), MAT(3, 3)));
                EMIT(og_mulss(gc, R(mm0), R(mm3)));
                EMIT(og_mulss(gc, R(mm1), R(mm4)));
                EMIT(og_mulss(gc, R(mm2), R(mm5)));
                EMIT(og_addss(gc, R(mm0), R(mm1)));
                EMIT(og_addss(gc, R(mm2), R(mm6)));
                EMIT(og_addss(gc, R(mm0), R(mm2)));
                EMIT(og_movss(gc, outw, R(mm0)));
            } else {
                EMIT(og_movss(gc, R(mm0), x));
                EMIT(og_movss(gc, R(mm1), y));
                EMIT(og_movss(gc, R(mm2), z));
                EMIT(og_movss(gc, R(mm3), w));
                EMIT(og_movss(gc, R(mm4), MAT(0, 0)));
                EMIT(og_movss(gc, R(mm5), MAT(1, 0)));
                EMIT(og_movss(gc, R(mm6), MAT(2, 0)));
                EMIT(og_movss(gc, R(mm7), MAT(3, 0)));
                EMIT(og_mulss(gc, R(mm0), R(mm4)));
                EMIT(og_mulss(gc, R(mm1), R(mm5)));
                EMIT(og_mulss(gc, R(mm2), R(mm6)));
                EMIT(og_mulss(gc, R(mm3), R(mm7)));
                EMIT(og_addss(gc, R(mm0), R(mm1)));
                EMIT(og_addss(gc, R(mm0), R(mm2)));
                EMIT(og_addss(gc, R(mm0), R(mm3)));
                EMIT(og_movss(gc, outx, R(mm0)));
                EMIT(og_movss(gc, R(mm0), x));
                EMIT(og_movss(gc, R(mm1), y));
                EMIT(og_movss(gc, R(mm2), z));
                EMIT(og_movss(gc, R(mm3), w));
                EMIT(og_movss(gc, R(mm4), MAT(0, 1)));
                EMIT(og_movss(gc, R(mm5), MAT(1, 1)));
                EMIT(og_movss(gc, R(mm6), MAT(2, 1)));
                EMIT(og_movss(gc, R(mm7), MAT(3, 1)));
                EMIT(og_mulss(gc, R(mm0), R(mm4)));
                EMIT(og_mulss(gc, R(mm1), R(mm5)));
                EMIT(og_mulss(gc, R(mm2), R(mm6)));
                EMIT(og_mulss(gc, R(mm3), R(mm7)));
                EMIT(og_addss(gc, R(mm0), R(mm1)));
                EMIT(og_addss(gc, R(mm0), R(mm2)));
                EMIT(og_addss(gc, R(mm0), R(mm3)));
                EMIT(og_movss(gc, outy, R(mm0)));
                EMIT(og_movss(gc, R(mm0), x));
                EMIT(og_movss(gc, R(mm1), y));
                EMIT(og_movss(gc, R(mm2), z));
                EMIT(og_movss(gc, R(mm3), w));
                EMIT(og_movss(gc, R(mm4), MAT(0, 2)));
                EMIT(og_movss(gc, R(mm5), MAT(1, 2)));
                EMIT(og_movss(gc, R(mm6), MAT(2, 2)));
                EMIT(og_movss(gc, R(mm7), MAT(3, 2)));
                EMIT(og_mulss(gc, R(mm0), R(mm4)));
                EMIT(og_mulss(gc, R(mm1), R(mm5)));
                EMIT(og_mulss(gc, R(mm2), R(mm6)));
                EMIT(og_mulss(gc, R(mm3), R(mm7)));
                EMIT(og_addss(gc, R(mm0), R(mm1)));
                EMIT(og_addss(gc, R(mm0), R(mm2)));
                EMIT(og_addss(gc, R(mm0), R(mm3)));
                EMIT(og_movss(gc, outz, R(mm0)));
                EMIT(og_movss(gc, R(mm0), x));
                EMIT(og_movss(gc, R(mm1), y));
                EMIT(og_movss(gc, R(mm2), z));
                EMIT(og_movss(gc, R(mm3), w));
                EMIT(og_movss(gc, R(mm4), MAT(0, 3)));
                EMIT(og_movss(gc, R(mm5), MAT(1, 3)));
                EMIT(og_movss(gc, R(mm6), MAT(2, 3)));
                EMIT(og_movss(gc, R(mm7), MAT(3, 3)));
                EMIT(og_mulss(gc, R(mm0), R(mm4)));
                EMIT(og_mulss(gc, R(mm1), R(mm5)));
                EMIT(og_mulss(gc, R(mm2), R(mm6)));
                EMIT(og_mulss(gc, R(mm3), R(mm7)));
                EMIT(og_addss(gc, R(mm0), R(mm1)));
                EMIT(og_addss(gc, R(mm0), R(mm2)));
                EMIT(og_addss(gc, R(mm0), R(mm3)));
                EMIT(og_movss(gc, outw, R(mm0)));
            }
        }
    } else {
        /* x87: the ancestor's code; the 2DNR results are stored from the stack in its order */
        switch (matrixType) {
          case __GL_MT_IS2DNR:
          case __GL_MT_IDENTITY:
          case __GL_MT_IS2DNRSC:
            if (vertexSize == 2) {
                EMIT(og_fld(gc, x));
                EMIT(og_fmul(gc, MAT(0, 0)));
                EMIT(og_fld(gc, y));
                EMIT(og_fmul(gc, MAT(1, 1)));
                EMIT(og_fxch(gc, R(st1)));
                EMIT(og_fadd(gc, MAT(3, 0)));
                EMIT(og_fxch(gc, R(st1)));
                EMIT(og_fadd(gc, MAT(3, 1)));
                EMIT(og_fxch(gc, R(st1)));
                EMIT(og_fstp(gc, outx));
                EMIT(og_fstp(gc, outy));
            } else if (vertexSize == 3) {
                EMIT(og_fld(gc, x));
                EMIT(og_fmul(gc, MAT(0, 0)));
                EMIT(og_fld(gc, y));
                EMIT(og_fmul(gc, MAT(1, 1)));
                EMIT(og_fld(gc, z));
                EMIT(og_fmul(gc, MAT(2, 2)));
                EMIT(og_fxch(gc, R(st2)));
                EMIT(og_fadd(gc, MAT(3, 0)));
                EMIT(og_fxch(gc, R(st1)));
                EMIT(og_fadd(gc, MAT(3, 1)));
                EMIT(og_fxch(gc, R(st2)));
                EMIT(og_fadd(gc, MAT(3, 2)));
                EMIT(og_fxch(gc, R(st1)));
                EMIT(og_fstp(gc, outx));
                EMIT(og_fstp(gc, outz));
                EMIT(og_fstp(gc, outy));
            } else {
                EMIT(og_fld(gc, x));
                EMIT(og_fmul(gc, MAT(0, 0)));
                EMIT(og_fld(gc, y));
                EMIT(og_fmul(gc, MAT(1, 1)));
                EMIT(og_fld(gc, z));
                EMIT(og_fmul(gc, MAT(2, 2)));
                EMIT(og_fxch(gc, R(st2)));
                EMIT(og_fld(gc, w));
                EMIT(og_fmul(gc, MAT(3, 0)));
                EMIT(og_faddp(gc, R(st1), R(st0)));
                EMIT(og_fld(gc, w));
                EMIT(og_fmul(gc, MAT(3, 1)));
                EMIT(og_faddp(gc, R(st2), R(st0)));
                EMIT(og_fld(gc, w));
                EMIT(og_fmul(gc, MAT(3, 2)));
                EMIT(og_faddp(gc, R(st3), R(st0)));
                EMIT(og_fstp(gc, outx));
                EMIT(og_fstp(gc, outy));
                EMIT(og_fstp(gc, outz));
            }
            if (w != NULL) {
                EMIT(og_fld(gc, w));
                EMIT(og_fstp(gc, outw));
            } else {
                EMIT(og_fld1(gc));
                EMIT(og_fstp(gc, outw));
            }
            break;
          default:
            EMIT(og_fld(gc, x));
            EMIT(og_fmul(gc, MAT(0, 0)));
            EMIT(og_fld(gc, y));
            EMIT(og_fmul(gc, MAT(1, 0)));
            EMIT(og_fxch(gc, R(st1)));
            if (w == NULL) {
                EMIT(og_fadd(gc, MAT(3, 0)));
            } else {
                EMIT(og_fld(gc, w));
                EMIT(og_fmul(gc, MAT(3, 0)));
                EMIT(og_faddp(gc, R(st1), R(st0)));
            }
            EMIT(og_fld(gc, z));
            EMIT(og_fmul(gc, MAT(2, 0)));
            EMIT(og_fxch(gc, R(st1)));
            EMIT(og_faddp(gc, R(st2), R(st0)));
            EMIT(og_fld(gc, x));
            EMIT(og_fmul(gc, MAT(0, 1)));
            EMIT(og_fxch(gc, R(st1)));
            EMIT(og_faddp(gc, R(st2), R(st0)));
            EMIT(og_fld(gc, y));
            EMIT(og_fmul(gc, MAT(1, 1)));
            EMIT(og_fxch(gc, R(st1)));
            if (w == NULL) {
                EMIT(og_fadd(gc, MAT(3, 1)));
            } else {
                EMIT(og_fld(gc, w));
                EMIT(og_fmul(gc, MAT(3, 1)));
                EMIT(og_faddp(gc, R(st1), R(st0)));
            }
            EMIT(og_fld(gc, z));
            EMIT(og_fmul(gc, MAT(2, 1)));
            EMIT(og_fxch(gc, R(st1)));
            EMIT(og_faddp(gc, R(st2), R(st0)));
            EMIT(og_fxch(gc, R(st2)));
            EMIT(og_fstp(gc, outx));
            EMIT(og_faddp(gc, R(st1), R(st0)));
            EMIT(og_fld(gc, x));
            EMIT(og_fmul(gc, MAT(0, 2)));
            EMIT(og_fld(gc, y));
            EMIT(og_fmul(gc, MAT(1, 2)));
            EMIT(og_fxch(gc, R(st1)));
            if (w == NULL) {
                EMIT(og_fadd(gc, MAT(3, 2)));
            } else {
                EMIT(og_fld(gc, w));
                EMIT(og_fmul(gc, MAT(3, 2)));
                EMIT(og_faddp(gc, R(st1), R(st0)));
            }
            EMIT(og_fld(gc, z));
            EMIT(og_fmul(gc, MAT(2, 2)));
            EMIT(og_fxch(gc, R(st1)));
            EMIT(og_faddp(gc, R(st2), R(st0)));
            EMIT(og_fxch(gc, R(st2)));
            EMIT(og_fstp(gc, outy));
            EMIT(og_faddp(gc, R(st1), R(st0)));
            EMIT(og_fld(gc, x));
            EMIT(og_fmul(gc, MAT(0, 3)));
            EMIT(og_fld(gc, y));
            EMIT(og_fmul(gc, MAT(1, 3)));
            EMIT(og_fxch(gc, R(st1)));
            if (w == NULL) {
                EMIT(og_fadd(gc, MAT(3, 3)));
            } else {
                EMIT(og_fld(gc, w));
                EMIT(og_fmul(gc, MAT(3, 3)));
                EMIT(og_faddp(gc, R(st1), R(st0)));
            }
            EMIT(og_fld(gc, z));
            EMIT(og_fmul(gc, MAT(2, 3)));
            EMIT(og_fxch(gc, R(st1)));
            EMIT(og_faddp(gc, R(st2), R(st0)));
            EMIT(og_fxch(gc, R(st2)));
            EMIT(og_fstp(gc, outz));
            EMIT(og_faddp(gc, R(st1), R(st0)));
            /* the ancestor's "IS2D caller expects no w on the stack" pop, kept although w is stored next:
            ** for an IS2D matrix and a 2- or 3-component vertex this fstp pops an empty x87 stack (an
            ** original bug: outw gets the invalid-operation NaN) */
            if (w == NULL && matrixType == __GL_MT_IS2D) {
                EMIT(og_fstp(gc, R(st0)));
            }
            EMIT(og_fstp(gc, outw));
        }
    }
    return pc;
}
#pragma optimize("", on)

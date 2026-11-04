/*
** od_clear8dither.c -- 3dfx s_rgb.c ClearRect_8_Dither, compiled without optimisation (/Od) in the
** original; named after the 3dfx ancestor.
*/
#include "wglpriv.h"
#include "icdglob.h"

/* 3dfx SFloat2Int: round a float in [-2^22, 2^22) to the nearest integer through the float's bits
** (adding 3 << 22 leaves the integer + 2^22 in the 23 mantissa bits) */
#define SFloat2Int(flt, i) \
    (*(__GLfloat *)&(i) = (flt) + __glFloatToIntBias, (i) = ((i) & 0x7fffff) - 0x400000)

#define __GL_CFB_WGL_OD(cfb)    ((cfb)->buf.wglBuf)

/* 690c5850 -- ClearRect_8_Dither (3dfx s_rgb.c; SI: __glI386ClearRectDitherRGB8): clear [x0,x1) x [y,y1)
** of a 1-byte-per-pixel RGB buffer to the dithered clear colour.  A 4 x 7 table of dithered pixels is
** built first; rows with at least one QWORD (8 pixels) are then filled a dword at a time (inline-asm
** loops, or C loops when a byte prestep / fixup is needed), narrower rectangles pixel by pixel.
** No buffer lock (the ancestor's __GL_LOCK_BUFFERS is gone).  Original quirk kept: case 1 packs its
** dword with pixel x0 in the HIGH byte where cases 2..4 put it in the low byte, so on x86 its runs
** come out with the four dither values reversed.
*/
#pragma optimize("", off)
void __fastcall ClearRect_8_Dither(__GLcolorBufferICD *cfb, GLint x0, GLint y, GLint x1, GLint y1)
{
  struct {                   /* the /Od frame [ebp-0xcc, ebp): every slot at the original's ebp offset */
    GLuint gi2;              /* -0xcc  narrow path (qword_width == 0): green */
    __GLfloat ftmp2;         /* -0xc8 */
    __GLfloat inc2;          /* -0xc4 */
    GLuint ri2;              /* -0xc0 */
    GLuint bi2;              /* -0xbc */
    GLubyte result2;         /* -0xb8 */
    char od_pad_b4[3];
    __GLfloat *dither2;      /* -0xb4 */
    GLubyte *bbuf4;          /* -0xb0  case 4: prestep, QWORDs, fixup */
    GLint i4;                /* -0xac */
    GLint qww4;              /* -0xa8 */
    long quad4;              /* -0xa4 */
    GLuint *lbuf4;           /* -0xa0 */
    GLuint *lbuf3;           /* -0x9c  case 3: QWORDs, fixup */
    GLubyte *bbuf3;          /* -0x98 */
    GLint i3;                /* -0x94 */
    GLint qww3;              /* -0x90 */
    long quad3;              /* -0x8c */
    GLubyte *bbuf2;          /* -0x88  case 2: prestep, QWORDs */
    GLint i2;                /* -0x84 */
    long quad2;              /* -0x80 */
    long quad1;              /* -0x7c  case 1: QWORDs only */
    __GLfloat *dither;       /* -0x78 */
    GLubyte *fp;             /* -0x74 */
    GLint pre_step_pixels;   /* -0x70 */
    __WGLbuffer *drawableBuf; /* -0x6c */
    GLint height;            /* -0x68 */
    GLint qword_width;       /* -0x64 */
    GLint fixup_pixels;      /* -0x60 */
    GLint x;                 /* -0x5c */
    GLubyte *rfp;            /* -0x58 */
    GLuint gi;               /* -0x54 */
    __GLcontext *gc;         /* -0x50 */
    __GLfloat fg;            /* -0x4c */
    __GLfloat ftmp;          /* -0x48 */
    __GLfloat inc;           /* -0x44 */
    GLint stride;            /* -0x40 */
    GLint row;               /* -0x3c */
    GLuint ri;               /* -0x38 */
    GLuint bi;               /* -0x34 */
    GLint width;             /* -0x30 */
    __GLfloat fr;            /* -0x2c */
    __GLfloat fb;            /* -0x28 */
    GLint col;               /* -0x24 */
    GLubyte result;          /* -0x20 */
    char od_pad_1c[3];
    GLubyte dither_colors[4][7]; /* -0x1c */
  } od_f;
#define gi2             od_f.gi2
#define ftmp2           od_f.ftmp2
#define inc2            od_f.inc2
#define ri2             od_f.ri2
#define bi2             od_f.bi2
#define result2         od_f.result2
#define dither2         od_f.dither2
#define bbuf4           od_f.bbuf4
#define i4              od_f.i4
#define qww4            od_f.qww4
#define quad4           od_f.quad4
#define lbuf4           od_f.lbuf4
#define lbuf3           od_f.lbuf3
#define bbuf3           od_f.bbuf3
#define i3              od_f.i3
#define qww3            od_f.qww3
#define quad3           od_f.quad3
#define bbuf2           od_f.bbuf2
#define i2              od_f.i2
#define quad2           od_f.quad2
#define quad1           od_f.quad1
#define dither          od_f.dither
#define fp              od_f.fp
#define pre_step_pixels od_f.pre_step_pixels
#define drawableBuf     od_f.drawableBuf
#define height          od_f.height
#define qword_width     od_f.qword_width
#define fixup_pixels    od_f.fixup_pixels
#define x               od_f.x
#define rfp             od_f.rfp
#define gi              od_f.gi
#define fg              od_f.fg
#define ftmp            od_f.ftmp
#define inc             od_f.inc
#define stride          od_f.stride
#define row             od_f.row
#define ri              od_f.ri
#define bi              od_f.bi
#define width           od_f.width
#define fr              od_f.fr
#define fb              od_f.fb
#define col             od_f.col
#define result          od_f.result
#define dither_colors   od_f.dither_colors

  od_f.gc = cfb->buf.gc;      /* (before the gc #define: buf.gc would expand too) */
#define gc              od_f.gc
  drawableBuf = __GL_CFB_WGL_OD(cfb);

  fr = gc->state.raster.clear.r * gc->frontBuffer.redScale;
  fg = gc->state.raster.clear.g * gc->frontBuffer.greenScale;
  fb = gc->state.raster.clear.b * gc->frontBuffer.blueScale;

  width  = x1 - x0;
  height = y1 - y;

  /* precompute the dithered pixels: [y & 3][x & 3] (7 columns, only 0..3 are read) */
  for (row = 0; row < 4; row++) {
    dither = __glDitherTableFloat + ((row << 2) & 12);

    for (col = 0; col < 7; col++) {
      inc = dither[col & 3];
      SFloat2Int(ftmp = fr + inc, ri);
      SFloat2Int(ftmp = fg + inc, gi);
      SFloat2Int(ftmp = fb + inc, bi);

      result = (ri << cfb->redShift) | (gi << cfb->greenShift) | (bi << cfb->blueShift);

      dither_colors[row][col] = result;
    }
  }

  rfp = (GLubyte *)drawableBuf->base +
        ((y - gc->constants.viewportYAdjust) * drawableBuf->outerWidth + x0 -
         gc->constants.viewportXAdjust) * drawableBuf->elementSize;

  /* bytes up to the next QWORD boundary, whole QWORDs, bytes after the last QWORD */
  pre_step_pixels = (8 - ((GLuint)rfp & 7)) & 7;
  if (pre_step_pixels > width) {
    pre_step_pixels = width;
    fixup_pixels = 0;
    qword_width = 0;
  } else {
    fixup_pixels = (GLuint)(rfp + width) & 7;
    qword_width  = (width - pre_step_pixels - fixup_pixels) >> 3;
  }

  stride = __GL_CFB_WGL_OD(cfb)->outerWidth;

  /* CASE 1: QWORD aligned start and width (the common case, e.g. a back buffer) */
  if (!pre_step_pixels && !fixup_pixels && qword_width) {
    while (height) {
      quad1 = (dither_colors[y & 3][x0 & 3] << 24) |
              (dither_colors[y & 3][(x0 + 1) & 3] << 16) |
              (dither_colors[y & 3][(x0 + 2) & 3] << 8) |
              dither_colors[y & 3][(x0 + 3) & 3];
      __asm {
        mov edi, rfp
        mov eax, quad1
        mov ebx, quad1
        mov ecx, qword_width
      lp_690c5b2f:
        mov DWORD PTR [edi], eax
        mov DWORD PTR [edi+0x4], ebx
        add edi, 0x8
        dec ecx
        jne lp_690c5b2f
      }
      rfp += stride;
      height--;
      y++;
    }
  }
  /* CASE 2: prestep to the QWORD boundary, no fixup */
  else if (pre_step_pixels && !fixup_pixels && qword_width) {
    while (height) {
      bbuf2 = rfp;
      quad2 = (dither_colors[y & 3][(x0 + pre_step_pixels + 3) & 3] << 24) |
              (dither_colors[y & 3][(x0 + pre_step_pixels + 2) & 3] << 16) |
              (dither_colors[y & 3][(x0 + pre_step_pixels + 1) & 3] << 8) |
              dither_colors[y & 3][(x0 + pre_step_pixels) & 3];

      for (i2 = 0; i2 < pre_step_pixels; i2++) {
        *bbuf2++ = dither_colors[y & 3][(x0 + i2) & 3];
      }

      __asm {
        mov edi, rfp
        mov eax, quad2
        add edi, pre_step_pixels
        mov ebx, quad2
        mov ecx, qword_width
      lp_690c5c93:
        mov DWORD PTR [edi], eax
        mov DWORD PTR [edi+0x4], ebx
        add edi, 0x8
        dec ecx
        jne lp_690c5c93
      }
      rfp += stride;
      height--;
      y++;
    }
  }
  /* CASE 3: QWORD aligned start, fixup bytes at the end */
  else if (!pre_step_pixels && fixup_pixels && qword_width) {
    while (height) {
      qww3 = qword_width;
      lbuf3 = (GLuint *)rfp;
      quad3 = (dither_colors[y & 3][(x0 + 3) & 3] << 24) |
              (dither_colors[y & 3][(x0 + 2) & 3] << 16) |
              (dither_colors[y & 3][(x0 + 1) & 3] << 8) |
              dither_colors[y & 3][x0 & 3];

      while (qww3) {
        lbuf3[0] = quad3;
        lbuf3[1] = quad3;
        lbuf3 += 2;
        qww3--;
      }

      bbuf3 = (GLubyte *)lbuf3;
      for (i3 = 0; i3 < fixup_pixels; i3++) {
        *bbuf3++ = dither_colors[y & 3][(x0 + (qword_width << 2) + i3) & 3];
      }
      rfp += stride;
      height--;
      y++;
    }
  }
  /* CASE 4: prestep and fixup */
  else if (pre_step_pixels && fixup_pixels && qword_width) {
    while (height) {
      qww4 = qword_width;
      bbuf4 = rfp;
      quad4 = (dither_colors[y & 3][(x0 + pre_step_pixels + 3) & 3] << 24) |
              (dither_colors[y & 3][(x0 + pre_step_pixels + 2) & 3] << 16) |
              (dither_colors[y & 3][(x0 + pre_step_pixels + 1) & 3] << 8) |
              dither_colors[y & 3][(x0 + pre_step_pixels) & 3];

      for (i4 = 0; i4 < pre_step_pixels; i4++) {
        *bbuf4++ = dither_colors[y & 3][(x0 + i4) & 3];
      }

      lbuf4 = (GLuint *)bbuf4;
      while (qww4) {
        lbuf4[0] = quad4;
        lbuf4[1] = quad4;
        lbuf4 += 2;
        qww4--;
      }

      bbuf4 = (GLubyte *)lbuf4;
      for (i4 = 0; i4 < fixup_pixels; i4++) {
        *bbuf4++ = dither_colors[y & 3][(x0 + pre_step_pixels + (qword_width << 2) + i4) & 3];
      }
      rfp += stride;
      height--;
      y++;
    }
  }
  /* narrower than a QWORD: pixel by pixel */
  else if (!qword_width) {
    for (; y < y1; y++, rfp += __GL_CFB_WGL_OD(cfb)->outerWidth) {
      dither2 = __glDitherTableFloat + ((y << 2) & 12);

      for (x = x0, fp = rfp; x < x1; x++, fp++) {
        inc2 = dither2[x & 3];
        SFloat2Int(ftmp2 = fr + inc2, ri2);
        SFloat2Int(ftmp2 = fg + inc2, gi2);
        SFloat2Int(ftmp2 = fb + inc2, bi2);

        result2 = (ri2 << cfb->redShift) | (gi2 << cfb->greenShift) | (bi2 << cfb->blueShift);

        *fp = result2;
      }
    }
  }
#undef gi2
#undef ftmp2
#undef inc2
#undef ri2
#undef bi2
#undef result2
#undef dither2
#undef bbuf4
#undef i4
#undef qww4
#undef quad4
#undef lbuf4
#undef lbuf3
#undef bbuf3
#undef i3
#undef qww3
#undef quad3
#undef bbuf2
#undef i2
#undef quad2
#undef quad1
#undef dither
#undef fp
#undef pre_step_pixels
#undef drawableBuf
#undef height
#undef qword_width
#undef fixup_pixels
#undef x
#undef rfp
#undef gi
#undef gc
#undef fg
#undef ftmp
#undef inc
#undef stride
#undef row
#undef ri
#undef bi
#undef width
#undef fr
#undef fb
#undef col
#undef result
#undef dither_colors
}
#pragma optimize("", on)

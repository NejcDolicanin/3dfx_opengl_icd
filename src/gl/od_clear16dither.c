/*
** od_clear16dither.c -- 3dfx s_rgb.c ClearRect_16_Dither, compiled without optimisation (/Od) in the
** original; named after the 3dfx ancestor.
*/
#include "wglpriv.h"
#include "icdglob.h"

/* 3dfx SFloat2Int: round a float in [-2^22, 2^22) to the nearest integer through the float's bits
** (adding 3 << 22 leaves the integer + 2^22 in the 23 mantissa bits) */
#define SFloat2Int(flt, i) \
    (*(__GLfloat *)&(i) = (flt) + __glFloatToIntBias, (i) = ((i) & 0x7fffff) - 0x400000)

#define __GL_CFB_WGL_OD(cfb)    ((cfb)->buf.wglBuf)

#pragma optimize("", off)
/* 690c6200 -- ClearRect_16_Dither (3dfx s_rgb.c; SI: __glI386ClearRectDitherRGB16 / ClearRect_16_Dither):
** clear [x0,x1) x [y,y1) of a 2-byte-per-pixel RGB buffer to the dithered clear colour.  A 4 x 7 table
** of dithered pixels is built first; rows with at least one QWORD (4 pixels) are then filled two dwords
** at a time (inline-asm loops, or C loops when a 1-pixel prestep / fixup pixels are needed), narrower
** rectangles pixel by pixel.  No buffer lock (the ancestor's __GL_LOCK_BUFFERS is gone).
** Original quirks kept (dither pattern only, every pixel still gets the clear colour):
**   - every pair puts its first pixel in the HIGH word, so on x86 the two pixels of a dword swap
**     their dither values;
**   - the fixup pixels index the table with x0 + qword_width*4 + 3 - fup, right only for 3 fixup
**     pixels, and case 4 does not count its prestep pixel.
*/
void __fastcall ClearRect_16_Dither(__GLcolorBufferICD *cfb, GLint x0, GLint y, GLint x1, GLint y1)
{
  struct {                   /* the /Od frame [ebp-0xf0, ebp): every slot at the original's ebp offset */
    GLuint gi2;              /* -0xf0  narrow path (qword_width == 0): green */
    __GLfloat ftmp2;         /* -0xec */
    __GLfloat inc2;          /* -0xe8 */
    GLuint ri2;              /* -0xe4 */
    GLuint bi2;              /* -0xe0 */
    GLushort result2;        /* -0xdc */
    char od_pad_d8[2];
    __GLfloat *dither2;      /* -0xd8 */
    GLint fup4;              /* -0xd4  case 4: prestep, QWORDs, fixup */
    GLushort *sbuf4;         /* -0xd0 */
    long first_pair4;        /* -0xcc */
    GLint qww4;              /* -0xc8 */
    GLuint *buf4;            /* -0xc4 */
    long second_pair4;       /* -0xc0 */
    GLint fup3;              /* -0xbc  case 3: QWORDs, fixup */
    GLushort *sbuf3;         /* -0xb8 */
    long first_pair3;        /* -0xb4 */
    GLint qww3;              /* -0xb0 */
    GLuint *buf3;            /* -0xac */
    long second_pair3;       /* -0xa8 */
    long first_pair2;        /* -0xa4  case 2: prestep, QWORDs */
    long second_pair2;       /* -0xa0 */
    long first_pair1;        /* -0x9c  case 1: QWORDs only */
    long second_pair1;       /* -0x98 */
    __GLfloat *dither;       /* -0x94 */
    GLushort *fp;            /* -0x90 */
    GLint pre_step_pixels;   /* -0x8c */
    __WGLbuffer *drawableBuf; /* -0x88 */
    GLint height;            /* -0x84 */
    GLint qword_width;       /* -0x80 */
    GLint fixup_pixels;      /* -0x7c */
    GLint x;                 /* -0x78 */
    GLushort *rfp;           /* -0x74 */
    GLuint gi;               /* -0x70 */
    __GLcontext *gc;         /* -0x6c */
    __GLfloat fg;            /* -0x68 */
    __GLfloat ftmp;          /* -0x64 */
    __GLfloat inc;           /* -0x60 */
    GLint stride;            /* -0x5c */
    GLint row;               /* -0x58 */
    GLuint ri;               /* -0x54 */
    GLuint bi;               /* -0x50 */
    GLint width;             /* -0x4c */
    __GLfloat fr;            /* -0x48 */
    __GLfloat fb;            /* -0x44 */
    GLint col;               /* -0x40 */
    GLushort result;         /* -0x3c */
    char od_pad_38[2];
    GLushort dither_colors[4][7]; /* -0x38 */
  } od_f;
#define gi2             od_f.gi2
#define ftmp2           od_f.ftmp2
#define inc2            od_f.inc2
#define ri2             od_f.ri2
#define bi2             od_f.bi2
#define result2         od_f.result2
#define dither2         od_f.dither2
#define fup4            od_f.fup4
#define sbuf4           od_f.sbuf4
#define first_pair4     od_f.first_pair4
#define qww4            od_f.qww4
#define buf4            od_f.buf4
#define second_pair4    od_f.second_pair4
#define fup3            od_f.fup3
#define sbuf3           od_f.sbuf3
#define first_pair3     od_f.first_pair3
#define qww3            od_f.qww3
#define buf3            od_f.buf3
#define second_pair3    od_f.second_pair3
#define first_pair2     od_f.first_pair2
#define second_pair2    od_f.second_pair2
#define first_pair1     od_f.first_pair1
#define second_pair1    od_f.second_pair1
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

  rfp = (GLushort *)((GLubyte *)drawableBuf->base +
        ((y - gc->constants.viewportYAdjust) * drawableBuf->outerWidth + x0 -
         gc->constants.viewportXAdjust) * drawableBuf->elementSize);

  /* one pixel up to the DWORD boundary, whole QWORDs (4 pixels), pixels after the last QWORD */
  pre_step_pixels = ((4 - ((GLuint)rfp & 3)) & 3) >> 1;
  if (pre_step_pixels > width) {
    pre_step_pixels = width;
    qword_width = 0;
    fixup_pixels = 0;
  } else {
    qword_width  = (width - pre_step_pixels) >> 2;
    fixup_pixels = width - (qword_width * 4 + pre_step_pixels);
  }

  stride = __GL_CFB_WGL_OD(cfb)->outerWidth;

  /* CASE 1: DWORD aligned start, whole QWORDs (the common case, e.g. a back buffer) */
  if (!pre_step_pixels && !fixup_pixels && qword_width) {
    while (height) {
      first_pair1  = (dither_colors[y & 3][x0 & 3] << 16) |
                     dither_colors[y & 3][(x0 + 1) & 3];
      second_pair1 = (dither_colors[y & 3][(x0 + 2) & 3] << 16) |
                     dither_colors[y & 3][(x0 + 3) & 3];
      __asm {
        mov edi, rfp
        mov eax, first_pair1
        mov ebx, second_pair1
        mov ecx, qword_width
      lp_690c651e:
        mov DWORD PTR [edi], eax
        mov DWORD PTR [edi+0x4], ebx
        add edi, 0x8
        dec ecx
        jne lp_690c651e
      }
      rfp += stride;
      height--;
      y++;
    }
  }
  /* CASE 2: one prestep pixel, no fixup */
  else if (pre_step_pixels && !fixup_pixels && qword_width) {
    while (height) {
      if (pre_step_pixels) {
        *rfp = dither_colors[y & 3][x0 & 3];
      }
      first_pair2  = (dither_colors[y & 3][(x0 + 1) & 3] << 16) |
                     dither_colors[y & 3][(x0 + 2) & 3];
      second_pair2 = (dither_colors[y & 3][(x0 + 3) & 3] << 16) |
                     dither_colors[y & 3][(x0 + 4) & 3];
      __asm {
        mov edi, rfp
        mov eax, first_pair2
        add edi, 0x2
        mov ebx, second_pair2
        mov ecx, qword_width
      lp_690c6655:
        mov DWORD PTR [edi], eax
        mov DWORD PTR [edi+0x4], ebx
        add edi, 0x8
        dec ecx
        jne lp_690c6655
      }
      rfp += stride;
      height--;
      y++;
    }
  }
  /* CASE 3: DWORD aligned start, fixup pixels at the end */
  else if (!pre_step_pixels && fixup_pixels && qword_width) {
    while (height) {
      fup3 = fixup_pixels;
      qww3 = qword_width;
      buf3 = (GLuint *)rfp;
      first_pair3  = (dither_colors[y & 3][x0 & 3] << 16) |
                     dither_colors[y & 3][(x0 + 1) & 3];
      second_pair3 = (dither_colors[y & 3][(x0 + 2) & 3] << 16) |
                     dither_colors[y & 3][(x0 + 3) & 3];

      while (qww3) {
        buf3[0] = first_pair3;
        buf3[1] = second_pair3;
        buf3 += 2;
        qww3--;
      }

      sbuf3 = (GLushort *)buf3;
      while (fup3) {
        *sbuf3++ = dither_colors[y & 3][(x0 + qword_width * 4 + 3 - fup3) & 3];
        fup3--;
      }
      rfp += stride;
      height--;
      y++;
    }
  }
  /* CASE 4: one prestep pixel and fixup pixels */
  else if (pre_step_pixels && fixup_pixels && qword_width) {
    while (height) {
      fup4 = fixup_pixels;
      qww4 = qword_width;
      buf4 = (GLuint *)(rfp + 1);
      first_pair4  = (dither_colors[y & 3][(x0 + 1) & 3] << 16) |
                     dither_colors[y & 3][(x0 + 2) & 3];
      second_pair4 = (dither_colors[y & 3][(x0 + 3) & 3] << 16) |
                     dither_colors[y & 3][(x0 + 4) & 3];

      if (pre_step_pixels) {
        *rfp = dither_colors[y & 3][x0 & 3];
      }

      while (qww4) {
        buf4[0] = first_pair4;
        buf4[1] = second_pair4;
        buf4 += 2;
        qww4--;
      }

      sbuf4 = (GLushort *)buf4;
      while (fup4) {
        *sbuf4++ = dither_colors[y & 3][(x0 + qword_width * 4 + 3 - fup4) & 3];
        fup4--;
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
#undef fup4
#undef sbuf4
#undef first_pair4
#undef qww4
#undef buf4
#undef second_pair4
#undef fup3
#undef sbuf3
#undef first_pair3
#undef qww3
#undef buf3
#undef second_pair3
#undef first_pair2
#undef second_pair2
#undef first_pair1
#undef second_pair1
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

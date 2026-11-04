/*
** glsi.h -- the ICD's versions of SGI Sample Implementation structures that the SI-source routines
** (src/gl/si_*.c) use.  The ICD's SI is older than ref/ogl-sample (release
** 20000807): no 3D-image fields, so the layouts differ; every offset here is checked against the
** original's accesses.
*/
#ifndef GLSI_H
#define GLSI_H
#include <math.h>
#include <string.h>

typedef GLuint __GLzValue;                  /* SI: a depth value */

/* __GLpixelSpanInfo: 16 words per source / destination block (SI 1.2.1 has 18) */
typedef struct __GLpixelSpanInfoICDRec {
    GLenum      srcFormat, srcType;         /* +0x00 */
    const GLvoid *srcImage;                 /* +0x08 */
    const GLvoid *srcCurrent;               /* +0x0c */
    GLint       srcRowIncrement;            /* +0x10 */
    GLint       srcGroupIncrement;          /* +0x14 */
    GLint       srcComponents;              /* +0x18 */
    GLint       srcElementSize;             /* +0x1c */
    GLint       srcSwapBytes;               /* +0x20 */
    GLint       srcLsbFirst;                /* +0x24 */
    GLint       srcSkipPixels, srcSkipLines;   /* +0x28 */
    GLint       srcLineLength;              /* +0x30 */
    GLint       srcAlignment;               /* +0x34 */
    GLboolean   srcPackedData;              /* +0x38  set by __glInitUnpacker (690c8b70) */
    GLubyte     pad39[3];
    GLint       srcStartBit;                /* +0x3c  after applying skipPixels */
    GLenum      dstFormat, dstType;         /* +0x40 */
    GLvoid     *dstImage;                   /* +0x48 */
    GLvoid     *dstCurrent;                 /* +0x4c */
    GLint       dstRowIncrement;            /* +0x50 */
    GLint       dstGroupIncrement;          /* +0x54 */
    GLint       dstComponents;              /* +0x58 */
    GLint       dstElementSize;             /* +0x5c */
    GLint       dstSwapBytes;               /* +0x60 */
    GLint       dstLsbFirst;                /* +0x64 */
    GLint       dstSkipPixels, dstSkipLines;   /* +0x68 */
    GLint       dstLineLength;              /* +0x70 */
    GLint       dstAlignment;               /* +0x74 */
    GLboolean   dstPackedData;              /* +0x78 */
    GLubyte     pad79[3];
    GLint       dstStartBit;                /* +0x7c */
    __GLfloat   zoomx, zoomy;               /* +0x80 */
    GLint       width, height;              /* +0x88 */
    GLint       depth;                      /* +0x90  (SI 1.2's depth; the ICD has no 3D images) */
    __GLfloat   readX, readY;               /* +0x94  reading coords (CopyPixels, ReadPixels) */
    __GLfloat   x, y;                       /* +0x9c  effective raster coordinates */
    GLint       startCol, startRow;         /* +0xa4  first actual pixel goes here */
    GLint       endCol;                     /* +0xac */
    GLint       columns, rows;              /* +0xb0  taking zoomx, zoomy into account */
    GLboolean   overlap;                    /* +0xb8  CopyPixels source / destination overlap */
    GLubyte     padb9[3];
    GLint       rowsUp, rowsDown;           /* +0xbc */
    GLint       rowadd, coladd;             /* +0xc4 */
    __GLfloat   rendZoomx;                  /* +0xcc */
    __GLzValue  fragz;                      /* +0xd0 */
    __GLfloat   rpyUp, rpyDown;             /* +0xd4 */
    GLint       startUp, startDown;         /* +0xdc */
    GLint       readUp, readDown;           /* +0xe4 */
    GLvoid     *redMap, *greenMap, *blueMap, *alphaMap;   /* +0xec */
    GLvoid     *indexMap;                   /* +0xfc */
    GLshort    *pixelArray;                 /* +0x100  replication counts / pixels to skip */
    GLfloat     scaleBias[9];               /* +0x104  the SI's generic rscale, gscale, bscale, ascale [0..3] (1 for the
                                                 texture paths) and rbias .. abias [4..7] (0); [8] is its GLboolean
                                                 fbScale (+0x124) */
    GLint       numSpanMods;                /* +0x128 */
    void      (__fastcall *spanReader)(struct __GLcontextRec *gc, struct __GLpixelSpanInfoICDRec *spanInfo, GLvoid *outspan);   /* +0x12c */
    void      (__fastcall *spanModifier[13])(struct __GLcontextRec *gc, struct __GLpixelSpanInfoICDRec *spanInfo,
                                             const GLvoid *inspan, GLvoid *outspan);   /* +0x130 */
    void      (__fastcall *spanRender)(struct __GLcontextRec *gc, struct __GLpixelSpanInfoICDRec *spanInfo, const GLvoid *inspan);   /* +0x164 */
    GLboolean   applyFlags[4];              /* +0x168  __glInitUnpacker: 0, 1, 1, 0 */
    GLint       dim;                        /* +0x16c  SI 1.2's dim: 1 or 2 (__glCopyPixelsOverlapping copies 0x170 bytes) */
} __GLpixelSpanInfoICD;

/* s_memmgr.c: the arena the display-list ops are allocated from (gc->procs.newArena 690dd200,
** deleteArena 690dd290, arenaAlloc 690dd2e0: 8-byte steps, a new block of at least 256 KB when the
** last one is full).  ICD block layout: size, allocated, data, next. */
typedef struct __GLarenaBlockRec {
    GLuint      size;                       /* +0x00 */
    GLuint      allocated;                  /* +0x04 */
    GLubyte    *data;                       /* +0x08 */
    struct __GLarenaBlockRec *next;         /* +0x0c */
} __GLarenaBlock;
typedef struct __GLarenaRec {
    struct __GLcontextRec *gc;              /* +0x00 */
    __GLarenaBlock *firstBlock;             /* +0x04 */
    __GLarenaBlock *lastBlock;              /* +0x08  allocations come from this one */
} __GLarena;

/* vertex.h with the ICD's values: what a vertex has / a primitive needs (__GLvertexICD.flagBits,
** vertex.needs / faceNeeds; s_vertex.c), the vertex size bits and the clip codes */
#define __GL_HAS_FRONT_COLOR    0x0001
#define __GL_HAS_BACK_COLOR     0x0002
#define __GL_HAS_TEXTURE        0x0004
#define __GL_HAS_NORMAL         0x0008
#define __GL_HAS_EYE            0x0010
#define __GL_HAS_CLIP           0x0020
#define __GL_HAS_FOG            0x0040
#define __GL_HAS_PACKED_COLOR   0x0100      /* ICD: procs.packVertexColor has run */
#define __GL_HAS_BOTH           (__GL_HAS_FRONT_COLOR | __GL_HAS_BACK_COLOR)
#define __GL_HAS_LIGHTING       (__GL_HAS_EYE | __GL_HAS_NORMAL)
#define __GL_VERTEX_EDGE_FLAG   0x1000
#define __GL_VERTEX_2D_SIZE     0x4000      /* the size bits ((flagBits >> 14) & 3 picks procs.validateVertex[]) */
#define __GL_VERTEX_3D_SIZE     0x8000
#define __GL_VERTEX_4D_SIZE     0xc000
#define __GL_CLIP_ALL_MASK      0xffff2000  /* the clip codes in flagBits (and the 0x2000 bit) */
/* vertex.h clip codes (flagBits bits 16..31: the six frustum planes, then the user clip planes) */
#define __GL_CLIP_MASK          0xffff0000
#define __GL_CLIP_SHIFT         16
#define __GL_CLIP_LEFT          0x00010000
#define __GL_CLIP_RIGHT         0x00020000
#define __GL_CLIP_BOTTOM        0x00040000
#define __GL_CLIP_TOP           0x00080000
#define __GL_CLIP_NEAR          0x00100000
#define __GL_CLIP_FAR           0x00200000
#define __GL_CLIP_FRUSTUM_MASK  0x003f0000

/* attrib.h: gc->validateMask bits that delay a validation (ICD: INDEX_FUNC for glIndexFuncSGI) */
#define __GL_VALIDATE_ALPHA_FUNC    0x00000001
#define __GL_VALIDATE_STENCIL_FUNC  0x00000002
#define __GL_VALIDATE_STENCIL_OP    0x00000004
#define __GL_VALIDATE_INDEX_FUNC    0x00000008

/* the ICD's __glOne / __glZero are constants (stored as 0x3f800000 / 0), not gc->constants.one */
#define __GL_ONE_F      1.0f
#define __GL_ZERO_F     0.0f

/* lighting.h: incoming colour components to the 0..1 range (the ICD's gc->constants names are the SI's) */
#define __GL_B_TO_FLOAT(b)      ((((b)<<1) + 1) * gc->constants.oneOver255)
#define __GL_UB_TO_FLOAT(ub)    (gc->constants.uByteToFloat[ub])
#define __GL_S_TO_FLOAT(s)      ((((s)<<1) + 1) * gc->constants.oneOver65535)
#define __GL_US_TO_FLOAT(us)    ((us) * gc->constants.oneOver65535)
#define __GL_I_TO_FLOAT(i)      ((((__GLfloat)(i) * (__GLfloat) 2.0) + 1) * gc->constants.oneOver4294965000)
#define __GL_UI_TO_FLOAT(ui)    ((__GLfloat)(ui) * gc->constants.oneOver4294965000)

/* lighting.h: back to integer components (SI: "round towards 0") */
#define __glHalf                (gc->constants.half)
#define __GL_FLOORF(f)          ((__GLfloat) floor((double) (f)))
#define __GL_CEILF(f)          ((__GLfloat) ceil(f))
#define __GL_MEMCOPY(to, from, count)  memcpy(to, from, count)

/* eval.h / attrib.h: the nine map ranges and their enable bits (gc->state.enables.eval1 / eval2) */
#define __GL_MAX_ORDER          40
#define __GL_MAP_RANGE_COUNT    9
#define __GL_EVAL1D_INDEX(old)  ((old) - GL_MAP1_COLOR_4)
#define __GL_EVAL2D_INDEX(old)  ((old) - GL_MAP2_COLOR_4)
#define __GL_C4                 __GL_EVAL1D_INDEX(GL_MAP1_COLOR_4)
#define __GL_I                  __GL_EVAL1D_INDEX(GL_MAP1_INDEX)
#define __GL_N3                 __GL_EVAL1D_INDEX(GL_MAP1_NORMAL)
#define __GL_T1                 __GL_EVAL1D_INDEX(GL_MAP1_TEXTURE_COORD_1)
#define __GL_T2                 __GL_EVAL1D_INDEX(GL_MAP1_TEXTURE_COORD_2)
#define __GL_T3                 __GL_EVAL1D_INDEX(GL_MAP1_TEXTURE_COORD_3)
#define __GL_T4                 __GL_EVAL1D_INDEX(GL_MAP1_TEXTURE_COORD_4)
#define __GL_V3                 __GL_EVAL1D_INDEX(GL_MAP1_VERTEX_3)
#define __GL_V4                 __GL_EVAL1D_INDEX(GL_MAP1_VERTEX_4)
#define __GL_MAP1_VERTEX_3_ENABLE               (1 << __GL_V3)
#define __GL_MAP1_VERTEX_4_ENABLE               (1 << __GL_V4)
#define __GL_MAP1_COLOR_4_ENABLE                (1 << __GL_C4)
#define __GL_MAP1_INDEX_ENABLE                  (1 << __GL_I)
#define __GL_MAP1_NORMAL_ENABLE                 (1 << __GL_N3)
#define __GL_MAP1_TEXTURE_COORD_1_ENABLE        (1 << __GL_T1)
#define __GL_MAP1_TEXTURE_COORD_2_ENABLE        (1 << __GL_T2)
#define __GL_MAP1_TEXTURE_COORD_3_ENABLE        (1 << __GL_T3)
#define __GL_MAP1_TEXTURE_COORD_4_ENABLE        (1 << __GL_T4)
#define __GL_MAP2_VERTEX_3_ENABLE               (1 << __GL_V3)
#define __GL_MAP2_VERTEX_4_ENABLE               (1 << __GL_V4)
#define __GL_MAP2_COLOR_4_ENABLE                (1 << __GL_C4)
#define __GL_MAP2_INDEX_ENABLE                  (1 << __GL_I)
#define __GL_MAP2_NORMAL_ENABLE                 (1 << __GL_N3)
#define __GL_MAP2_TEXTURE_COORD_1_ENABLE        (1 << __GL_T1)
#define __GL_MAP2_TEXTURE_COORD_2_ENABLE        (1 << __GL_T2)
#define __GL_MAP2_TEXTURE_COORD_3_ENABLE        (1 << __GL_T3)
#define __GL_MAP2_TEXTURE_COORD_4_ENABLE        (1 << __GL_T4)
#define __glMinusOne            ((__GLfloat) -1.0)

/* lighting.h: the material change bits (Material ops, dl_mopt FastMaterial) and the shared
** specular lookup tables (gc->light.front/back.cache) */
#define __GL_MATERIAL_AMBIENT           0x00000001
#define __GL_MATERIAL_DIFFUSE           0x00000002
#define __GL_MATERIAL_SPECULAR          0x00000004
#define __GL_MATERIAL_EMISSIVE          0x00000008
#define __GL_MATERIAL_SHININESS         0x00000010
#define __GL_MATERIAL_COLORINDEXES      0x00000020
#define __GL_MATERIAL_ALL               0x0000003f      /* every material change bit */
#define __GL_SPEC_LOOKUP_TABLE_SIZE     256
typedef struct __GLspecLUTEntryRec {
    GLint refcount;
    __GLfloat threshold, scale, exp;
    __GLfloat table[__GL_SPEC_LOOKUP_TABLE_SIZE];
} __GLspecLUTEntry;
/* s_lcache.c: gc->light.lutCache, the SI layout (__glInitLUTCache 690dd380 mallocs 0x10 for one entry) */
typedef struct {
    __GLfloat exp;
    __GLspecLUTEntry *table;
} __GLspecLUTEntryPtr;
typedef struct __GLspecLUTCacheRec {
    GLint nentries;
    GLint allocatedSize;
    __GLspecLUTEntryPtr entries[1];
} __GLspecLUTCache;

/* s_eval.c: DoEvalCoord2 records its state changes for the mesh fill */
typedef struct StateChangeRec {
    GLint changed;
    __GLcolor color;
    __GLcoord normal;
    __GLcoord texture;
    __GLcoord vertex;
} StateChange;
#define CHANGE_COLOR    1
#define CHANGE_NORMAL   2
#define CHANGE_TEXTURE  4
#define CHANGE_VERTEX3  8
#define CHANGE_VERTEX4  16
/* s_eval.c's own constants */
#define MaxK                    4
#define MaxOrder                40
#define TYPE_COEFF_AND_DERIV    1
#define TYPE_COEFF              2
#define MAX_WIDTH               1024

/* gc->dispatchState is the Windows GLPROCTABLE: cEntries, then the GL 1.1 entries in glapi
** order (Begin = 7 -> fn[8] at +0x20, End 43, Vertex3fv 137 at +0x228): __GL_DISP_<Name> */
#include "gldisp.h"
#define __GL_FLOAT_TO_B(f)      ((GLbyte) __GL_FLOORF(((f) * gc->constants.val255) * __glHalf))
#define __GL_FLOAT_TO_UB(f)     ((GLubyte) ((f) * gc->constants.val255 + __glHalf))
#define __GL_FLOAT_TO_S(f)      ((GLshort) __GL_FLOORF(((f) * gc->constants.val65535) * __glHalf))
#define __GL_FLOAT_TO_US(f)     ((GLushort) ((f) * gc->constants.val65535 + __glHalf))
#define __GL_FLOAT_TO_I(f)      ((GLint) __GL_FLOORF(((f) * gc->constants.val4294965000) * __glHalf))
#define __GL_FLOAT_TO_UI(f)     ((GLuint) ((f) * gc->constants.val4294965000 + __glHalf))

/* small SI types the ICD shares */
typedef GLuint __GLstippleWord;
typedef struct __GLtexelRec { GLubyte r, g, b, a; } __GLtexel;   /* texture.h (texel fetch: GLubyte texel[4]) */
#define __GL_STIPPLE_BITS       32

/* global.h / osunix.h / context.h / apimacro.h: API-entry plumbing.  The ICD reads the context
** from the TEB at every use (`__glCurrentGC()->beginMode == 1` right after `gc = __glCurrentGC()`),
** which is the SI macros expanded with __gl_context = the TEB read; __glSetError is __glSetError. */
#undef assert                               /* SI asserts compile out (the release ICD has none) */
#define assert(x)
enum { __GL_NOT_IN_BEGIN = 0, __GL_IN_BEGIN = 1, __GL_NEED_VALIDATE = 2 };
#define __gl_context            (__glCurrentGC())
#define __gl_beginMode          (__gl_context->beginMode)
#define __GL_SETUP()            __GLcontext *gc = __gl_context
#define __GL_IN_BEGIN()         (__gl_beginMode == __GL_IN_BEGIN)
#define __GL_SETUP_NOT_IN_BEGIN()       __GL_SETUP(); if (__GL_IN_BEGIN()) { __glSetError(GL_INVALID_OPERATION); return; }
#define __GL_SETUP_NOT_IN_BEGIN_NOGLSETUP() if (__GL_IN_BEGIN()) { __glSetError(GL_INVALID_OPERATION); return; }
#define __GL_SETUP_NOT_IN_BEGIN2()      __GL_SETUP(); if (__GL_IN_BEGIN()) { __glSetError(GL_INVALID_OPERATION); return 0; }
#define __GL_SETUP_NOT_IN_BEGIN_VALIDATE() __GL_SETUP(); { GLint beginMode_ = __gl_beginMode; if (beginMode_ != __GL_NOT_IN_BEGIN) { \
            if (beginMode_ == __GL_NEED_VALIDATE) { (*gc->procs.validate)(gc); __gl_beginMode = __GL_NOT_IN_BEGIN; } \
            else { __glSetError(GL_INVALID_OPERATION); return; } } }
#define __GL_API_STATE()
#define __GL_API_GET()
#define __GL_API_PIXEL_OP()
#define __GL_API_VAPI()
#define __GL_API_BLAND()
/* SI 1.2's vertex-cache flush hooks: nothing in this ICD */
#define __GL_API_BGN_RENDER()
#define __GL_API_NOTBE_RENDER()
#define __GL_API_EVAL()
#define __GL_API_FLUSH()
/* buffers.h pixel-path locks: no __GL_USE_SLOCK build, no locking around pixel paths */
#define __GL_LOCK_PXL_DRAW_BUFFER(gc, format)
#define __GL_UNLOCK_PXL_DRAW_BUFFER(gc, format)
#define __GL_LOCK_PXL_READ_BUFFER(gc, format)
#define __GL_UNLOCK_PXL_READ_BUFFER(gc, format)
/* importsx.h, render.h.  Stipple words are MSB-first in the ICD (cpu.h __GL_STIPPLE_MSB: the span
** procs start at 0x80000000 and shift right, 690cc487) */
#define __GL_SQRTF(f)           ((__GLfloat) sqrt((double) (f)))
#define __GL_STIPPLE_MSB        1
#define __GL_STIPPLE_SHIFT(i)   (1 << (__GL_STIPPLE_BITS - 1 - (i)))
/* lighting.h */
#define __GL_MASK_INDEXF(gc, val) \
    (((__GLfloat) (((GLint) ((val) * 16)) & (((gc)->frontBuffer.redMax << 4) | 0xf))) / (__GLfloat)16.0)
#define __GL_DIRTY_GENERIC          0x00000001
#define __GL_DIRTY_LINE             0x00000002
#define __GL_DIRTY_POLYGON          0x00000004
#define __GL_DIRTY_POINT            0x00000008
#define __GL_DIRTY_PIXEL            0x00000010
#define __GL_DIRTY_LIGHTING         0x00000020
#define __GL_DIRTY_POLYGON_STIPPLE  0x00000040
#define __GL_DIRTY_DEPTH            0x00000080
#define __GL_DIRTY_SCISSOR          0x00000100
#define __GL_DIRTY_VERTARRAY        0x00000200
#define __GL_DIRTY_MODELVIEW        0x00000400
#define __GL_DIRTY_ALL              0x000007ff
/* the ICD's own dirty bits above the SI set (named after where glEnable / glDisable set them) */
#define __GL_DIRTY_HWTEXTURE_ICD    0x00000800      /* re-pick the sst texture procs (hwValidateTextures) */
#define __GL_DIRTY_TEXTURE_ICD      0x00001000      /* GL_TEXTURE_1D / 2D toggled */
#define __GL_DIRTY_CLIPPLANE_ICD    0x00002000      /* GL_CLIP_PLANEi toggled */
#define __GL_DIRTY_SHADEMODEL_ICD   0x00004000      /* glShadeModel changed (also context init, attribute pop) */
#define __GL_DIRTY_HWDEPTH_ICD      0x00008000      /* depth test toggled (Glide depth mode) */
#define __GL_DIRTY_FACE_ICD         0x00010000      /* polygon.face[] / cull mode changed (front face, y inversion) */
#define __GL_DIRTY_NONUNITYQ_ICD    0x00020000      /* the first vertex with q != 1 (procs.nonUnityQ) */
#define __GL_DIRTY_COLORARRAY_ICD   0x00040000      /* GL_COLOR_ARRAY toggled */
#define __GL_DIRTY_TEXENABLE_ICD    0x00080000      /* a unit's texture.textureEnabled changed */
#define __GL_DIRTY_HWFOG_ICD        0x00100000      /* fog toggled */
#define __GL_DIRTY_HWLIGHT_ICD      0x00200000      /* lighting toggled */
#define __GL_DIRTY_MVPTYPE_ICD      0x00800000      /* modelView->mvp.matrixType changed */
#define __GL_DIRTY_TEXMATRIX_ICD    0x01000000      /* a texgen unit's texture matrix type changed */
#define __GL_DIRTY_TEXGEN_S_ICD     0x02000000      /* GL_TEXTURE_GEN_S .. Q toggled: S << 0..3 */
#define __GL_DIRTY_TEXGEN_T_ICD     0x04000000
#define __GL_DIRTY_TEXGEN_R_ICD     0x08000000
#define __GL_DIRTY_TEXGEN_Q_ICD     0x10000000
#define __GL_DIRTY_HWVERTEXBUF_ICD  0x20000000      /* the hw staging vertex buffer was reallocated (hwcx.vtxBase) */
/* the ICD writes beginMode through the TEB here too */
#define __GL_DELAY_VALIDATE(gc)             __gl_beginMode = __GL_NEED_VALIDATE; (gc)->dirtyMask |= __GL_DIRTY_GENERIC
#define __GL_DELAY_VALIDATE_MASK(gc, mask)  __gl_beginMode = __GL_NEED_VALIDATE; (gc)->dirtyMask |= (mask)

/* buffers.h: the buffer mask bits (gc->buffers.*BufferMask, dp->accelBufferMask, glClear's mask) */
#define __GL_FRONT_BUFFER_MASK          0x00000001
#define __GL_BACK_BUFFER_MASK           0x00000004
#define __GL_ACCUM_BUFFER_MASK          0x00000010
#define __GL_DEPTH_BUFFER_MASK          0x00000020
#define __GL_STENCIL_BUFFER_MASK        0x00000040
/* context.h gcState bits, kept in gc->mcFlags (S_EXPORT.C MakeCurrent / wglDevMakeCurrent) */
#define __GL_INIT_HW            0x1         /* validated once on the hardware */
#define __GL_HW_MODE            0x2         /* hardware procs installed */
#define __GL_PIXMAP_MODE        0x4
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE        0x812F      /* OpenGL 1.2 (the kit's GL.H predates it) */
#endif

/* vertex.h faces */
#define __GL_FRONTFACE          0
#define __GL_BACKFACE           1
/* GL_ARB_multitexture (the kit's GL.H predates it) */
#ifndef GL_TEXTURE0_ARB
#define GL_TEXTURE0_ARB                 0x84C0
#define GL_TEXTURE1_ARB                 0x84C1
#define GL_ACTIVE_TEXTURE_ARB           0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB    0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB        0x84E2
#endif
#ifndef __GL_NUM_TEXTURE_UNITS
#define __GL_NUM_TEXTURE_UNITS          2       /* texture[2] in the vertex and current state */
#endif
/* EXT_abgr / EXT_bgra / GL 1.2 pixel formats (the kit's GL.H predates them) */
#ifndef GL_ABGR_EXT
#define GL_ABGR_EXT                     0x8000
#endif
#ifndef GL_BGR
#define GL_BGR                          0x80E0
#define GL_BGRA                         0x80E1
#endif
/* attrib.h enables.general: bits 0..21 checked against the ICD's glEnable (690b88e0, its two byte
** tables); from bit 22 on the ICD differs from the SI (see below) */
#define __GL_ALPHA_TEST_ENABLE          (1 <<  0)
#define __GL_BLEND_ENABLE               (1 <<  1)
#define __GL_INDEX_LOGIC_OP_ENABLE      (1 <<  2)
#define __GL_DITHER_ENABLE              (1 <<  3)
#define __GL_DEPTH_TEST_ENABLE          (1 <<  4)
#define __GL_FOG_ENABLE                 (1 <<  5)
#define __GL_LIGHTING_ENABLE            (1 <<  6)
#define __GL_COLOR_MATERIAL_ENABLE      (1 <<  7)
#define __GL_LINE_STIPPLE_ENABLE        (1 <<  8)
#define __GL_LINE_SMOOTH_ENABLE         (1 <<  9)
#define __GL_POINT_SMOOTH_ENABLE        (1 << 10)
#define __GL_POLYGON_SMOOTH_ENABLE      (1 << 11)
#define __GL_CULL_FACE_ENABLE           (1 << 12)
#define __GL_POLYGON_STIPPLE_ENABLE     (1 << 13)
#define __GL_SCISSOR_TEST_ENABLE        (1 << 14)
#define __GL_STENCIL_TEST_ENABLE        (1 << 15)
#define __GL_NORMALIZE_ENABLE           (1 << 16)
#define __GL_AUTO_NORMAL_ENABLE         (1 << 17)
#define __GL_POLYGON_OFFSET_POINT_ENABLE (1 << 18)
#define __GL_POLYGON_OFFSET_LINE_ENABLE (1 << 19)
#define __GL_POLYGON_OFFSET_FILL_ENABLE (1 << 20)
#define __GL_POLYGON_OFFSET_ANY_ENABLE  (__GL_POLYGON_OFFSET_POINT_ENABLE | __GL_POLYGON_OFFSET_LINE_ENABLE | \
                                         __GL_POLYGON_OFFSET_FILL_ENABLE)       /* 0x1c0000 */
#define __GL_COLOR_LOGIC_OP_ENABLE      (1 << 21)
#define __GL_INDEX_MATERIAL_EXT_ENABLE  (1 << 22)      /* ICD: GL_INDEX_MATERIAL_EXT (0x81b8) */
#define __GL_INDEX_TEST_EXT_ENABLE      (1 << 23)      /* ICD: GL_INDEX_TEST_EXT (0x81b5) */
/* bits 22, 23 and 25 are the ICD's extension caps (not in the SI); no RESCALE_NORMAL */
#define __GL_CULL_VERTEX_ENABLE         (1 << 24)
#define __GL_SHARED_TEXTURE_PALETTE_EXT_ENABLE (1 << 25) /* ICD: GL_SHARED_TEXTURE_PALETTE_EXT (0x81fb) */
/* enables.texture[unit] (gc+0x520): no 3D texture, so the SI's texgen bits sit one lower */
#define __GL_TEXTURE_1D_ENABLE          (1 << 0)
#define __GL_TEXTURE_2D_ENABLE          (1 << 1)
#define __GL_TEXTURE_GEN_S_ENABLE       (1 << 2)
#define __GL_TEXTURE_GEN_T_ENABLE       (1 << 3)
#define __GL_TEXTURE_GEN_R_ENABLE       (1 << 4)
#define __GL_TEXTURE_GEN_Q_ENABLE       (1 << 5)
#define __GL_TEXTURE_GEN_ANY_ENABLE     (__GL_TEXTURE_GEN_S_ENABLE | __GL_TEXTURE_GEN_T_ENABLE | \
                                         __GL_TEXTURE_GEN_R_ENABLE | __GL_TEXTURE_GEN_Q_ENABLE)


/* s_names.c / namesint.h: the name trees of display lists and texture objects (SI layout; the ICD's
** accesses agree: array +0xc dataInfo, dataInfo +4 empty, leaf +4 start / +8 end / +0xc dataList) */
typedef enum __GLnamesArrayTypeEnum {
    __GL_NAMES_DLIST,
    __GL_NAMES_TEXOBJ
} __GLnamesArrayType;
#define __GL_DL_EXTRA_BRANCHES          16
#define __GL_DL_EXTRA_LEAVES            3
#define __GL_DLIST_MIN_ARRAY_BLOCK      16          /* dlistint.h */
#define __GL_DLIST_MAX_ARRAY_BLOCK      1024
typedef struct __GLnamesArrayTypeInfoRec __GLnamesArrayTypeInfo;
typedef struct __GLnamesBranchRec __GLnamesBranch;
typedef struct __GLnamesLeafRec __GLnamesLeaf;
typedef struct __GLnamesArrayRec {
    __GLnamesBranch *tree;                  /* +0x00 */
    GLuint depth;                           /* +0x04 */
    GLint refcount;                         /* +0x08 */
    __GLnamesArrayTypeInfo *dataInfo;       /* +0x0c */
    GLuint nbranches, nleaves;              /* +0x10 */
    __GLnamesBranch *branches[__GL_DL_EXTRA_BRANCHES];      /* +0x18 */
    __GLnamesLeaf *leaves[__GL_DL_EXTRA_LEAVES];            /* +0x58 */
} __GLnamesArray;
struct __GLnamesArrayTypeInfoRec {
    __GLnamesArrayType type;
    void *empty;
    GLuint dataSize;
    void (*free)(__GLcontext *gc, void *memory);
    GLboolean (*alloc)(__GLcontext *gc, size_t size);
};
struct __GLnamesLeafRec {
    __GLnamesBranch *parent;
    GLuint start;
    GLuint end;
    void **dataList;
    __GLnamesArrayTypeInfo *dataInfo;
};
struct __GLnamesBranchRec {
    __GLnamesBranch *parent;
    GLuint low;
    GLuint medium;
    __GLnamesBranch *children[3];
};


/* g_lcomp.c: the list compiler's error ops __gllc_InvalidValue (6907f7c0) / __gllc_InvalidEnum (6907f7f0),
** __stdcall (gc) (defined in gl_pick.c) */
extern void __stdcall __gllc_InvalidValue(__GLcontext *gc);
extern void __stdcall __gllc_InvalidEnum(__GLcontext *gc);

/* dlist.h: a display-list operation (SI __GLdlistOp, byte for byte) */
typedef struct __GLdlistOpICDRec {
    struct __GLdlistOpICDRec *next;         /* +0x00 */
    void (__fastcall *dlistFree)(__GLcontext *gc, GLubyte *PC);   /* +0x04 */
    GLuint      size;                       /* +0x08 */
    GLshort     opcode;                     /* +0x0c */
    GLboolean   aligned;                    /* +0x0e  the data holds doubles (kept 8-byte aligned) */
    GLboolean   pad1;
    GLubyte     data[4];                    /* +0x10 */
} __GLdlistOpICD;

/* dlist.h: a compiled display list (SI __GLdlist) */
typedef struct __GLdlistICDRec {
    GLint       refcount;                   /* +0x00  created with 1 */
    void (__fastcall *free)(__GLcontext *gc, struct __GLdlistICDRec *dlist);      /* +0x04 */
    void (__fastcall *execute)(__GLcontext *gc, struct __GLdlistICDRec *dlist);   /* +0x08 */
    GLubyte    *segment;                    /* +0x0c  the first segment */
    GLubyte     local[4];                   /* +0x10  the execute function's data (free count, free functions) */
} __GLdlistICD;

/* g_lcomp.c / dl_list.c: a list-execute function (the record in, the next record out), and the
** compiler's record allocator and appender */
typedef const GLubyte *(__fastcall *__GLlistExecFunc)(const GLubyte *PC);
extern __GLdlistOpICD *__fastcall __glDlistAllocOp2(__GLcontext *gc, GLuint size);
extern void __fastcall __glDlistAppendOp(__GLcontext *gc, __GLdlistOpICD *dlop, __GLlistExecFunc func);
#define __GL_PAD(x)             (((x) + 3) & ~3)    /* records are padded to whole words */
/* dlistopt.h: what a compiled list holds (__GLcompiledDlist.genericFlags) */
#define __GL_DLFLAG_HAS_VERTEX          0x00000001
#define __GL_DLFLAG_HAS_NORMAL          0x00000002
#define __GL_DLFLAG_HAS_COLOR           0x00000004
#define __GL_DLFLAG_HAS_TEXCOORDS       0x00000008
#define __GL_DLFLAG_HAS_INDEX           0x00000010
#define __GL_DLFLAG_HAS_RASTERPOS       0x00000020
#define __GL_DLFLAG_HAS_RECT            0x00000040
#define __GL_DLFLAG_HAS_BEGIN           0x00000080
#define __GL_DLFLAG_HAS_MATERIAL        0x00000100
/* g_lcomp.c: the op codes of the list compiler's records (__GLdlistOpICD.opcode); the dl_opt.c ops
** (errors, FastMaterial) are 0x3f2 and up */
#define __glop_CallList                 0
#define __glop_CallLists                1
#define __glop_ListBase                 0x02
#define __glop_Bitmap                   0x4
#define __glop_Color3bv                 0x05
#define __glop_Color3dv                 0x06
#define __glop_Color3fv                 0x07
#define __glop_Color3iv                 0x08
#define __glop_Color3sv                 0x09
#define __glop_Color3ubv                0x0a
#define __glop_Color3uiv                0x0b
#define __glop_Color3usv                0x0c
#define __glop_Color4bv                 0x0d
#define __glop_Color4dv                 0x0e
#define __glop_Color4fv                 0x0f
#define __glop_Color4iv                 0x10
#define __glop_Color4sv                 0x11
#define __glop_Color4ubv                0x12
#define __glop_Color4uiv                0x13
#define __glop_Color4usv                0x14
#define __glop_EdgeFlagv                0x15
#define __glop_End                      0x16
#define __glop_Indexdv                  0x17
#define __glop_Indexfv                  0x18
#define __glop_Indexiv                  0x19
#define __glop_Indexsv                  0x1a
#define __glop_Vertex3d                 0x44
#define __glop_Vertex4d                 0x48
#define __glop_Fogiv                    0x52
#define __glop_Lightfv                  0x56
#define __glop_LightModelfv             0x5a
#define __glop_Materialfv               0x60
#define __glop_Materialiv               0x62
#define __glop_TexParameterfv           0x69
#define __glop_TexImage1D               0x6c
#define __glop_TexImage2D               0x6d
#define __glop_TexEnvfv                 0x6f
#define __glop_TexGendv                 0x73
#define __glop_DisableClientState       0x89
#define __glop_EnableClientState        0x8a
#define __glop_Map1d                    0x8d
#define __glop_Map1f                    0x8e
#define __glop_Map2d                    0x8f
#define __glop_Map2f                    0x90
#define __glop_MapGrid2d                0x93
#define __glop_MapGrid2f                0x94
#define __glop_EvalCoord1dv             0x95
#define __glop_EvalCoord1fv             0x96
#define __glop_EvalCoord2dv             0x97
#define __glop_EvalCoord2fv             0x98
#define __glop_EvalMesh1                0x99
#define __glop_EvalPoint1               0x9a
#define __glop_EvalMesh2                0x9b
#define __glop_EvalPoint2               0x9c
#define __glop_AlphaFunc                0x9d
#define __glop_BlendFunc                0x9e
#define __glop_LogicOp                  0x9f
#define __glop_StencilFunc              0xa0
#define __glop_StencilOp                0xa1
#define __glop_DepthFunc                0xa2
#define __glop_PixelZoom                0xa3
#define __glop_PixelTransferf           0xa4
#define __glop_PixelTransferi           0xa5
#define __glop_PixelMapusv              0xa8
#define __glop_ReadBuffer               0xa9
#define __glop_CopyPixels               0xaa
#define __glop_DrawPixels               0xab
#define __glop_DepthRange               0xac
#define __glop_Frustum                  0xad
#define __glop_LoadIdentity             0xae
#define __glop_MatrixMode               0xb1
#define __glop_Ortho                    0xb4
#define __glop_PopMatrix                0xb5
#define __glop_PushMatrix               0xb6
#define __glop_Rotated                  0xb7
#define __glop_Rotatef                  0xb8
#define __glop_Scaled                   0xb9
#define __glop_Scalef                   0xba
#define __glop_Translated               0xbb
#define __glop_Translatef               0xbc
#define __glop_Viewport                 0xbd
#define __glop_ColorTable               0xc0
#define __glop_ColorSubTable            0xc1
#define __glop_CopyColorTableEXT        0xc2
#define __glop_PolygonOffset            0xc6
#define __glop_CopyTexImage1D           0xc7
#define __glop_CopyTexImage2D           0xc8
#define __glop_CopyTexSubImage1D        0xc9
#define __glop_CopyTexSubImage2D        0xca
#define __glop_TexSubImage1D            0xcb
#define __glop_TexSubImage2D            0xcc
#define __glop_BindTexture              0xcd
#define __glop_Indexubv                 0xcf
#define __glop_IndexFuncSGI             0xd0
#define __glop_IndexMaterialSGI         0xd1
#define __glop_ActiveTextureARB         0xd2
#define __glop_ClientActiveTextureARB   0xd3
#define __glop_MultiTexCoord1dARB       0xd4
#define __glop_MultiTexCoord1dvARB      0xd5
#define __glop_MultiTexCoord1fARB       0xd6
#define __glop_MultiTexCoord1fvARB      0xd7
#define __glop_MultiTexCoord1iARB       0xd8
#define __glop_MultiTexCoord1ivARB      0xd9
#define __glop_MultiTexCoord1sARB       0xda
#define __glop_MultiTexCoord1svARB      0xdb
#define __glop_MultiTexCoord2dARB       0xdc
#define __glop_MultiTexCoord2dvARB      0xdd
#define __glop_MultiTexCoord2fARB       0xde
#define __glop_MultiTexCoord2fvARB      0xdf
#define __glop_MultiTexCoord2iARB       0xe0
#define __glop_MultiTexCoord2ivARB      0xe1
#define __glop_MultiTexCoord2sARB       0xe2
#define __glop_MultiTexCoord2svARB      0xe3
#define __glop_MultiTexCoord3dARB       0xe4
#define __glop_MultiTexCoord3dvARB      0xe5
#define __glop_MultiTexCoord3fARB       0xe6
#define __glop_MultiTexCoord3fvARB      0xe7
#define __glop_MultiTexCoord3iARB       0xe8
#define __glop_MultiTexCoord3ivARB      0xe9
#define __glop_MultiTexCoord3sARB       0xea
#define __glop_MultiTexCoord3svARB      0xeb
#define __glop_MultiTexCoord4dARB       0xec
#define __glop_MultiTexCoord4dvARB      0xed
#define __glop_MultiTexCoord4fARB       0xee
#define __glop_MultiTexCoord4fvARB      0xef
#define __glop_MultiTexCoord4iARB       0xf0
#define __glop_MultiTexCoord4ivARB      0xf1
#define __glop_MultiTexCoord4sARB       0xf2
#define __glop_MultiTexCoord4svARB      0xf3
#define __glop_PointParameterfEXT       0xf4
#define __glop_PointParameterfvEXT      0xf5
#define __glop_BlendEquationEXT         0xf6
#define __glop_CompressedTexImage1DARB  0xf7
#define __glop_CompressedTexImage2DARB  0xf8
#define __glop_CompressedTexSubImage1DARB 0xf9
#define __glop_CompressedTexSubImage2DARB 0xfa
#define __glop_TBufferMask3DFX          0xfb
#define __glop_FogCoordfEXT             0xfc
#define __glop_FogCoorddEXT             0xfd
#define __glop_FogCoordfvEXT            0xfe
#define __glop_FogCoorddvEXT            0xff
#define __glop_InvalidValue             0x3f2
#define __glop_InvalidEnum              0x3f3
#define __glop_InvalidOperation         0x3f4
#define __glop_UnimplementedExtension   0x3f5
#define __glop_FastMaterial             0x3f6
#define __glop_TableTooLarge            0x3f7

/* a transform stack entry (0x130 bytes: glGetIntegerv(GL_MODELVIEW_STACK_DEPTH) divides by it, 690bdcd8):
** the matrix, its inverse transpose and the matrix times the projection (SI __GLtransform) */
struct __GLtransformICDRec {
    __GLmatrixICD matrix;                   /* +0x000 */
    __GLmatrixICD inverseTranspose;         /* +0x060 */
    __GLmatrixICD mvp;                      /* +0x0c0 */
    GLuint      sequence;                   /* +0x120  the projection sequence mvp was built with */
    GLboolean   updateInverse;              /* +0x124  inverseTranspose is stale */
    GLubyte     pad125[3];
    GLuint      unk_128[2];                 /* +0x128 */
};
typedef struct __GLtransformICDRec __GLtransformICD;

/* xform.h matrix types, global.h */
#define __GL_MT_GENERAL         0
#define __GL_MT_W0001           1
#define __GL_MT_IS2D            2
#define __GL_MT_IS2DNR          3
#define __GL_MT_IDENTITY        4
#define __GL_MT_IS2DNRSC        5
#ifndef __glTwo
#define __glTwo                 ((__GLfloat) 2.0)
#endif
#define __GL_VERTEX_EDGE_FLAG   0x1000      /* edgeTag bit 12 (690bd9ac) */
#ifndef GL_BLEND_EQUATION
#define GL_BLEND_EQUATION       0x8009
#endif


/* pixel.h: the glPixelMap tables are the state's ten 12-byte heads (state.pixel.map, 0x2b0) */
typedef struct __GLpixelMapHeadRec {
    GLint       size;
    GLint       tableId;
    union { GLint *mapI; __GLfloat *mapF; } base;
} __GLpixelMapHead;
#define __GL_PIXEL_MAP_I_TO_I   (GL_PIXEL_MAP_I_TO_I - GL_PIXEL_MAP_I_TO_I)
#define __GL_PIXEL_MAP_S_TO_S   (GL_PIXEL_MAP_S_TO_S - GL_PIXEL_MAP_I_TO_I)
#define __GL_PIXEL_MAP_I_TO_R   (GL_PIXEL_MAP_I_TO_R - GL_PIXEL_MAP_I_TO_I)
#define __GL_PIXEL_MAP_I_TO_G   (GL_PIXEL_MAP_I_TO_G - GL_PIXEL_MAP_I_TO_I)
#define __GL_PIXEL_MAP_I_TO_B   (GL_PIXEL_MAP_I_TO_B - GL_PIXEL_MAP_I_TO_I)
#define __GL_PIXEL_MAP_I_TO_A   (GL_PIXEL_MAP_I_TO_A - GL_PIXEL_MAP_I_TO_I)
#define __GL_PIXEL_MAP_R_TO_R   (GL_PIXEL_MAP_R_TO_R - GL_PIXEL_MAP_I_TO_I)
#define __GL_PIXEL_MAP_G_TO_G   (GL_PIXEL_MAP_G_TO_G - GL_PIXEL_MAP_I_TO_I)
#define __GL_PIXEL_MAP_B_TO_B   (GL_PIXEL_MAP_B_TO_B - GL_PIXEL_MAP_I_TO_I)
#define __GL_PIXEL_MAP_A_TO_A   (GL_PIXEL_MAP_A_TO_A - GL_PIXEL_MAP_I_TO_I)


/* consts.h / render.h sizes the span and line code allocates by */
#define __GL_MAX_MAX_VIEWPORT   2048
#define __GL_STIPPLE_COUNT_BITS 5
#define __GL_MAX_STIPPLE_WORDS  ((__GL_MAX_MAX_VIEWPORT + __GL_STIPPLE_BITS - 1) / __GL_STIPPLE_BITS)
#define __GL_MAX_SPAN_WIDTH     (__GL_MAX_MAX_VIEWPORT)
#define __GL_MAX_SPAN_SIZE      (__GL_MAX_SPAN_WIDTH * 4 * sizeof(GLfloat))
#define __GL_X_MAJOR            0
#define __GL_Y_MAJOR            1

#endif

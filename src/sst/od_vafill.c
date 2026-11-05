/*
** od_vafill.c -- hwGenVertexArrayFill, the generator of the hardware path's vertex-array fill routine
** (vertexArray.fillRange / fillLockedRange and its second entry fetchVertex).  The original compiled it
** without optimisation (/Od): every frame slot sits at the original's [ebp - X] through the od_f
** frame struct.
*/
#include <stdlib.h>
#include "wglpriv.h"
#include "glsi.h"
#include "og.h"
#include "icdglob.h"
#include "icdprotos.h"

/* The data page of a bufRec[2] cache entry (valB: 4 KB, page aligned): constants, array cursors and
** scratch the generated routine addresses absolutely. */
typedef struct VaFillBlockRec {
    GLuint      raw_000[0x150 / 4];
    GLuint      negOne[4];                  /* 0x150  -1.0f (SSE: -w; 3DNow: pfmul {-1, -1}) */
    GLuint      clipLeft[4];                /* 0x160  the clip-code bits as SSE andps masks, 4 copies each */
    GLuint      clipRight[4];               /* 0x170 */
    GLuint      clipBottom[4];              /* 0x180 */
    GLuint      clipTop[4];                 /* 0x190 */
    GLuint      clipNear[4];                /* 0x1a0 */
    GLuint      clipFar[4];                 /* 0x1b0 */
    GLuint      raw_1c0[(0x1e0 - 0x1c0) / 4];
    GLuint      fpuSave;                    /* 0x1e0  (the compiled-out control-word code) */
    GLuint      tmp;                        /* 0x1e4  x87 -> integer scratch (clip-plane distance, GB_CLIP w) */
    GLint       first;                      /* 0x1e8  edx at entry: the first vertex */
    __GLcoord   clip;                       /* 0x1ec  the vertex transformed by the MVP */
    GLuint      raw_1fc;                    /* 0x1fc  only touched (loaded at entry) */
    GLuint      raw_200[2];
    GLuint      colorTmp[4];                /* 0x208  scratch of the colour fetch (CopyColor) */
    GLubyte    *normalPtr;                  /* 0x218  array cursors: the element of the current vertex, */
    GLubyte    *colorPtr;                   /* 0x21c  stepped back by the stride per vertex */
    GLubyte    *texPtr[2];                  /* 0x220 */
    GLubyte    *vertexPtr;                  /* 0x228 */
    GLubyte    *fogPtr;                     /* 0x22c */
    GLuint      raw_230[4];
    GLuint      fetchTmp;                   /* 0x240  scratch of the element fetchers */
    GLuint      raw_244[5];
    GLuint      gbClipScale;                /* 0x258  1.1f: the GB_CLIP margin */
    GLubyte    *fetchVertex;                /* 0x25c  the second entry (va->fetchVertex on a cache hit) */
    GLuint      raw_260[(0x804 - 0x260) / 4];
    __GLfloat   gb[16];                     /* 0x804  guard band: SSE gbLeft, gbRight, gbBottom, gbTop (4 copies
                                               each, stored with movups: 0x804 is not 16-byte aligned); 3DNow
                                               {gbRight, gbTop, gbLeft, gbBottom} */
    GLuint      gbBits[4];                  /* 0x844  3DNow: the right, top, left, bottom clip bits */
} VaFillBlock;

/* offsets the generated code adds to a register or to blk; the /Od code computes the indexed ones at run
** time (`mov r, index * size; add r, member`), as the original's offsetof expressions did */
#define BOFS(m)         ((GLint)&((VaFillBlock *)0)->m)         /* a field of the data page */
#define TOFS(m)         ((GLint)&((__GLtextureFxICD *)0)->m)    /* a field of the texture object (eax) */
/* the texture-matrix proc of a unit: ogLazyProcs[9 + 3 * unit .. 11 + 3 * unit] all hold it; this takes the last */
#define TEXMAT_PROC(hw, unit)   (&((__GLproc (*)[3])&(hw)->hwcx.ogLazyProcs[9])[unit][2])

typedef GLuint (__fastcall *VaNeedsFn)(__GLcontext *gc);
typedef GLubyte *(__fastcall *VaCacheLookupFn)(__GLcontext *gc, __GLhwBufRecICD *rec, const GLuint *key,
                                               VaFillBlock **blk);
/* emitters (pc in, pc out) */
                                                                /* reg *= factor (shl or imul; nothing for 1) */
                                                                /* coordinates [eax] -> [edi + dst] */
                                                                /* normal [eax] -> vertex normal */
                                                                /* fog coordinate [eax] -> vertex fogCoord */
                                                                /* colour [eax] -> vertex colour / packedColor */
                                                                /* [edx] by the matrix in ecx -> x, y, z, w */
                                                                /* call [target] (line: IMM(source line), unused) */

/* 6904e048 -- hwGenVertexArrayFill: build the vertex-array fill routine of the hardware path.  The state key (make_key) is
** looked up in the bufRec[2] code cache; a hit installs the cached routine.  On a miss the routine is
** generated into the entry's code page, with its constants and cursors in the entry's data page (blk):
**   fillRange(gc, first; first, count) (ret 8): for the vertices first + count - 1 down to first of the
**     vertex cache, fetch the enabled arrays (the current values otherwise) into the vertex, the eye
**     coordinates, normal, texture-matrix / texgen and lighting procs (ogLazyProcs), the MVP transform,
**     1/w, the guard-band codes (+0x18), the clip codes with the user clip planes, the flag bits (+0x10),
**     the window coordinates and the texture coordinates scaled for Glide;
**   fetchVertex(gc, vertex) (ret): once per vertex (flag 0x20), fetch its colour; GB_CLIP (the
**     environment variable, read while generating) adds flag 0x800 when x or y is beyond 1.1 * |w|.
** Fix: on SSE CPUs with the guard band (fullscreen) the entry stores the guard-band vectors with movups; the
** original used movaps on blk + 0x804, which is not 16-byte aligned (an alignment fault on every call). */
#pragma optimize("", off)
void __fastcall hwGenVertexArrayFill(__GLcontext *gc)
{
  struct {                   /* the /Od frame [ebp-0x110, ebp): each slot at its original ebp offset */
    OgLabel *gbClipSkip;                /* ebp-0x110 */
    __GLtextureICD *curTexB2;           /* ebp-0x10c */
    __GLtextureICD *curTexB;            /* ebp-0x108 */
    __GLtextureICD *curTexA2;           /* ebp-0x104 */
    __GLtextureICD *curTexA;            /* ebp-0x100 */
    OgLabel *texDone;                   /* ebp-0xfc */
    OgLabel *texUnprojected;            /* ebp-0xf8 */
    GLuint doneNeeds;                   /* ebp-0xf4  flag bits of what the routine computed */
    GLuint planeBit;                    /* ebp-0xf0 */
    GLuint planeMask;                   /* ebp-0xec */
    __GLcoord *plane;                   /* ebp-0xe8 */
    OgLabel *noFlagsReset;              /* ebp-0xe4  (compiled-out block) */
    __GLmatrixICD *texMatrix;           /* ebp-0xe0 */
    GLubyte **texCursor;                /* ebp-0xdc */
    GLubyte **texCursor0;               /* ebp-0xd8 */
    GLint j;                            /* ebp-0xd4 */
    GLint texFloat2;                    /* ebp-0xd0  (unused) every enabled texcoord array is 2 floats */
    GLuint key[23];                     /* ebp-0xcc .. ebp-0x70: the bufRec[2] key (keySize 0x5c) */
    GLint vtxStride;                    /* ebp-0x70 */
    GLubyte *code;                      /* ebp-0x6c */
    __GLviewport *vp;                   /* ebp-0x68 */
    GLubyte noColorArray;               /* ebp-0x64 */
    char od_pad_60[3];
    GLint unit;                         /* ebp-0x60 */
    GLubyte clipCodes;                  /* ebp-0x5c */
    char od_pad_58[3];
    GLuint needs;                       /* ebp-0x58 */
    GLuint modeFlags;                   /* ebp-0x54 */
    GLubyte depthWrites;                /* ebp-0x50  (unused) */
    char od_pad_4c[3];
    OgLabel *loopTop;                   /* ebp-0x4c */
    __GLmatrixICD *mv;                  /* ebp-0x48  modelView: matrix, inverse transpose, MVP */
    GLubyte noFogArray;                 /* ebp-0x44 */
    char od_pad_40[3];
    GLint i;                            /* ebp-0x40 */
    GLuint arrays;                      /* ebp-0x3c  vertexArray.enableFlags */
    __GLcontext *hw;                    /* ebp-0x38  the same context (the hwcx fields are read through it) */
    GLuint allNeeds;                    /* ebp-0x34 */
    OgLabel *clipDone;                  /* ebp-0x30 */
    GLubyte noTexArray[4];              /* ebp-0x2c */
    GLubyte noNormalArray;              /* ebp-0x28 */
    char od_pad_24[3];
    OgOperand *argCount;                /* ebp-0x24  [esp + 0x1c] */
    GLuint texEnables;                  /* ebp-0x20 */
    GLubyte *pc;                        /* ebp-0x1c */
    OgOperand *argFirst;                /* ebp-0x18  [esp + 0x18] */
    VaFillBlock *blk;                   /* ebp-0x14 */
    GLint normalStride;                 /* ebp-0x10 */
    GLint fogStride;                    /* ebp-0xc */
    __GLvertArrayMachine *va;           /* ebp-0x8 */
    OgLabel *fetchDone;                 /* ebp-0x4 */
  } od_f;
#define gbClipSkip od_f.gbClipSkip
#define curTexB2 od_f.curTexB2
#define curTexB od_f.curTexB
#define curTexA2 od_f.curTexA2
#define curTexA od_f.curTexA
#define texDone od_f.texDone
#define texUnprojected od_f.texUnprojected
#define doneNeeds od_f.doneNeeds
#define planeBit od_f.planeBit
#define planeMask od_f.planeMask
#define plane od_f.plane
#define noFlagsReset od_f.noFlagsReset
#define texMatrix od_f.texMatrix
#define texCursor od_f.texCursor
#define texCursor0 od_f.texCursor0
#define j od_f.j
#define texFloat2 od_f.texFloat2
#define key od_f.key
#define vtxStride od_f.vtxStride
#define code od_f.code
#define vp od_f.vp
#define noColorArray od_f.noColorArray
#define unit od_f.unit
#define clipCodes od_f.clipCodes
#define needs od_f.needs
#define modeFlags od_f.modeFlags
#define depthWrites od_f.depthWrites
#define loopTop od_f.loopTop
#define mv od_f.mv
#define noFogArray od_f.noFogArray
#define i od_f.i
#define arrays od_f.arrays
#define hw od_f.hw
#define allNeeds od_f.allNeeds
#define clipDone od_f.clipDone
#define noTexArray od_f.noTexArray
#define noNormalArray od_f.noNormalArray
#define argCount od_f.argCount
#define texEnables od_f.texEnables
#define pc od_f.pc
#define argFirst od_f.argFirst
#define blk od_f.blk
#define normalStride od_f.normalStride
#define fogStride od_f.fogStride
#define va od_f.va
#define fetchDone od_f.fetchDone

  hw = gc;
  mv = (__GLmatrixICD *)gc->transform.modelView;
  vp = &gc->state.viewport;
  modeFlags = (*(VaNeedsFn)gc->polygon.shader.getModeFlags)(gc);
  allNeeds = ((*(VaNeedsFn)gc->vertex.getFaceNeeds0)(gc) | (*(VaNeedsFn)gc->vertex.getNeeds)(gc));
  needs = what_needs(gc);
  va = &gc->vertexArray;
  arrays = va->enableFlags;

  /* ---- look the state up in the bufRec[2] code cache ---- */
  make_key(key, gc);
  code = (*(VaCacheLookupFn)hw->hwcx.bufRec[2].alloc)(gc, &hw->hwcx.bufRec[2], key, &blk);
  if (code != NULL) {
    va->fillLockedRange = (__GLproc)code;
    va->fillRange = (__GLproc)code;
    va->fetchVertex = (__GLproc)blk->fetchVertex;
  }
  else {
    code = hwPageCacheInsert(gc, &hw->hwcx.bufRec[2], key, &blk);
    pc = code;

    /* ---- the data page's constants and the state the generator reads ---- */
    i = 0;
    for (; i < 4; i = (i + 1)) {
      blk->negOne[i] = 0xbf800000;
      blk->clipLeft[i] = 0x10000;
      blk->clipRight[i] = 0x20000;
      blk->clipBottom[i] = 0x40000;
      blk->clipTop[i] = 0x80000;
      blk->clipNear[i] = 0x100000;
      blk->clipFar[i] = 0x200000;
    }
    blk->gbBits[0] = 0x20000;
    blk->gbBits[1] = 0x80000;
    blk->gbBits[2] = 0x10000;
    blk->gbBits[3] = 0x40000;
    if ((gc->constants.cpuCaps & (__GL_CPU_3DNOW | __GL_CPU_SSE)) != 0) {
      texFloat2 = 1;
      texEnables = 0;
      unit = 0;
      for (; unit < gc->fx.numTMUs; unit = (unit + 1)) {
        texEnables = (texEnables | gc->state.enables.texture[unit]);
        if (gc->state.enables.texture[unit] != 0) {
          texFloat2 = (texFloat2 & (va->texture.type[unit] == GL_FLOAT && va->texture.size[unit] == 2));
        }
      }
    }
    /* (modeFlags & 4 = __GL_SHADE_DEPTH_TEST; overwritten, unused) */
    depthWrites = ((modeFlags & 4) != 0 && (gc->state.depth.writeEnable != 0 || gc->state.depth.testFunc != GL_ALWAYS));
    depthWrites = 1;
    noNormalArray = ((arrays & __GL_VA_NORMAL_ENABLE) == 0);
    noColorArray = ((arrays & __GL_VA_COLOR_ENABLE) == 0);
    unit = 0;
    for (; unit < gc->fx.numTMUs; unit = (unit + 1)) {
      noTexArray[unit] = ((arrays & __GL_VA_TEXCOORD_ENABLE(unit)) == 0);
    }
    noFogArray = ((arrays & __GL_VA_FOGCOORD_ENABLE) == 0);
    texEnables = 0;
    j = 0;
    for (; j < gc->fx.numTMUs; j = (j + 1)) {
      texEnables = (texEnables | gc->state.enables.texture[j]);
    }
    clipCodes = (gc->state.hints.ext590 != GL_FASTEST);

    /* ---- fillRange entry: save the registers, the guard band into blk ---- */
    og_reset(gc);
    argFirst = BASE(0x18, esp);
    argCount = BASE(0x1c, esp);
    loopTop = og_newFixup(gc);
    fetchDone = og_newFixup(gc);
    clipDone = og_newFixup(gc);
    va->fillRange = (__GLproc)pc;
    va->fillLockedRange = (__GLproc)pc;
    EMIT(og_push(gc, R(esi)));
    EMIT(og_push(gc, R(edi)));
    EMIT(og_push(gc, R(ebx)));
    EMIT(og_push(gc, R(ecx)));
    EMIT(og_push(gc, R(ebp)));
    EMIT(og_mov(gc, MEM(&blk->first), R(edx)));
    if (gc->fx.fullscreen != 0) {
      if (gc->fx.guardBand != 0) {
        if ((gc->constants.cpuCaps & __GL_CPU_SSE) != 0) {
          EMIT(og_movss(gc, R(mm0), MEM(&gc->fx.gbLeft)));
          EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
          EMIT(og_movups(gc, MEM((GLuint)blk + BOFS(gb[0])), R(mm0)));
          EMIT(og_movss(gc, R(mm0), MEM(&gc->fx.gbRight)));
          EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
          EMIT(og_movups(gc, MEM((GLuint)blk + BOFS(gb[4])), R(mm0)));
          EMIT(og_movss(gc, R(mm0), MEM(&gc->fx.gbBottom)));
          EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
          EMIT(og_movups(gc, MEM((GLuint)blk + BOFS(gb[8])), R(mm0)));
          EMIT(og_movss(gc, R(mm0), MEM(&gc->fx.gbTop)));
          EMIT(og_shufps(gc, R(mm0), R(mm0), IMM(0)));
          EMIT(og_movups(gc, MEM((GLuint)blk + BOFS(gb[12])), R(mm0)));
        }
        else {
          if ((gc->constants.cpuCaps & __GL_CPU_3DNOW) != 0) {
            EMIT(og_mov(gc, R(eax), MEM(&gc->fx.gbLeft)));
            EMIT(og_mov(gc, MEM((GLuint)blk + BOFS(gb[2])), R(eax)));
            EMIT(og_mov(gc, R(eax), MEM(&gc->fx.gbTop)));
            EMIT(og_mov(gc, MEM((GLuint)blk + BOFS(gb[1])), R(eax)));
            EMIT(og_mov(gc, R(eax), MEM(&gc->fx.gbRight)));
            EMIT(og_mov(gc, MEM((GLuint)blk + BOFS(gb[0])), R(eax)));
            EMIT(og_mov(gc, R(eax), MEM(&gc->fx.gbBottom)));
            EMIT(og_mov(gc, MEM((GLuint)blk + BOFS(gb[3])), R(eax)));
          }
        }
      }
    }
    /* touch the data page */
    EMIT(og_mov(gc, R(eax), MEM(&blk->fpuSave)));
    EMIT(og_mov(gc, R(ebx), MEM(&blk->raw_1fc)));
    EMIT(og_mov(gc, R(eax), MEM(&blk->vertexPtr)));
    EMIT(og_mov(gc, R(ebx), MEM(&blk->normalPtr)));
    EMIT(og_mov(gc, R(eax), MEM(&blk->fogPtr)));
    if (0 != 0) {
      EMIT(og_fnstcw(gc, MEM(&blk->fpuSave)));
      EMIT(og_mov(gc, R(edx), MEM(&blk->fpuSave)));
      EMIT(og_and(gc, R(dh), IMM(0xfffffffc)));
      EMIT(og_mov(gc, MEM(&blk->tmp), R(edx)));
      EMIT(og_fldcw(gc, MEM(&blk->tmp)));
    }

    /* ---- the last vertex (first + count - 1) and the array cursors at it ---- */
    EMIT(og_mov(gc, R(eax), argFirst));
    EMIT(og_add(gc, R(eax), argCount));
    EMIT(og_dec(gc, R(eax)));
    EMIT(og_mov(gc, argFirst, R(eax)));
    if ((arrays & __GL_VA_VERTEX_ENABLE) != 0) {
      EMIT(og_mov(gc, R(eax), argFirst));
      pc = og_scaleby(gc, pc, R(eax), IMM(va->vertex.stride));
      EMIT(og_add(gc, R(eax), MEM(&va->vertex.pointer)));
      EMIT(og_mov(gc, MEM(&blk->vertexPtr), R(eax)));
    }
    if ((arrays & __GL_VA_NORMAL_ENABLE) != 0) {
      EMIT(og_mov(gc, R(eax), argFirst));
      pc = og_scaleby(gc, pc, R(eax), IMM(va->normal.stride));
      EMIT(og_add(gc, R(eax), MEM(&va->normal.pointer)));
      EMIT(og_mov(gc, MEM(&blk->normalPtr), R(eax)));
    }
    unit = 0;
    for (; unit < gc->fx.numTMUs; unit = (unit + 1)) {
      texCursor0 = &blk->texPtr[unit];
      if ((arrays & __GL_VA_TEXCOORD_ENABLE(unit)) != 0) {
        EMIT(og_mov(gc, R(eax), argFirst));
        pc = og_scaleby(gc, pc, R(eax), IMM(va->texture.stride[unit]));
        EMIT(og_add(gc, R(eax), MEM(&va->texture.pointer[unit])));
        EMIT(og_mov(gc, MEM(texCursor0), R(eax)));
      }
    }
    if ((arrays & (__GL_VA_COLOR_ENABLE | __GL_VA_INDEX_ENABLE)) != 0) {
      EMIT(og_mov(gc, R(eax), argFirst));
      pc = og_scaleby(gc, pc, R(eax), IMM(va->color.stride));
      EMIT(og_add(gc, R(eax), MEM(&va->color.pointer)));
      EMIT(og_mov(gc, MEM(&blk->colorPtr), R(eax)));
    }
    if ((arrays & __GL_VA_FOGCOORD_ENABLE) != 0) {
      EMIT(og_mov(gc, R(eax), argFirst));
      pc = og_scaleby(gc, pc, R(eax), IMM(va->fogCoord.stride));
      EMIT(og_add(gc, R(eax), MEM(&va->fogCoord.pointer)));
      EMIT(og_mov(gc, MEM(&blk->fogPtr), R(eax)));
    }
    /* edx = the vertex element, edi = its cache vertex (vertexCache + (first + count - 1) * 0xc0) */
    EMIT(og_mov(gc, R(edx), MEM(&blk->vertexPtr)));
    EMIT(og_mov(gc, R(edi), MEM(&va->vertexCache)));
    EMIT(og_mov(gc, R(eax), MEM(&blk->first)));
    EMIT(og_add(gc, R(eax), argCount));
    EMIT(og_dec(gc, R(eax)));
    pc = og_scaleby(gc, pc, R(eax), IMM(sizeof(__GLvertexICD)));
    EMIT(og_add(gc, R(edi), R(eax)));
    vtxStride = va->vertex.stride;
    normalStride = va->normal.stride;
    fogStride = va->fogCoord.stride;

    /* ---- the loop: one vertex ---- */
    LABEL(loopTop);
    if ((arrays & __GL_VA_VERTEX_ENABLE) != 0) {
      if (va->vertex.type != GL_FLOAT || (gc->state.enables.general & __GL_LIGHTING_ENABLE) != 0 ||
          (texEnables & __GL_TEXTURE_GEN_ANY_ENABLE) != 0 || (needs & __GL_HAS_EYE) != 0 ||
          gc->state.enables.clipPlanes != 0) {
        /* the object coordinates into the vertex (edx = obj); else edx stays on the array element */
        EMIT(og_mov(gc, R(eax), MEM(&blk->vertexPtr)));
        pc = CopyCoord(gc, pc, va->vertex.type, va->vertex.size, VOFS(obj));
        EMIT(og_sub(gc, R(eax), IMM(va->vertex.stride)));
        EMIT(og_mov(gc, MEM(&blk->vertexPtr), R(eax)));
        EMIT(og_lea(gc, R(edx), MEMR(VOFS(obj.x), edi)));
      }
    }
    /* the normal */
    if ((allNeeds & __GL_HAS_NORMAL) != 0) {
      if (noNormalArray != 0) {
        EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.normal.x)));
        EMIT(og_mov(gc, MEMR(VOFS(normal.x), edi), R(eax)));
        EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.normal.y)));
        EMIT(og_mov(gc, MEMR(VOFS(normal.y), edi), R(eax)));
        EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.normal.z)));
        EMIT(og_mov(gc, MEMR(VOFS(normal.z), edi), R(eax)));
      }
      else {
        EMIT(og_mov(gc, R(eax), MEM(&blk->normalPtr)));
        pc = CopyNormal(gc, pc, va->normal.type, IMM(&blk->fetchTmp));
        EMIT(og_sub(gc, R(eax), IMM(va->normal.stride)));
        EMIT(og_mov(gc, MEM(&blk->normalPtr), R(eax)));
      }
    }
    /* the fog coordinate */
    if ((gc->state.enables.general & __GL_FOG_ENABLE) != 0) {
      if (gc->state.fog.coordSource == GL_FOG_COORDINATE_EXT) {
        if (noFogArray != 0) {
          EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.fogCoord)));
          EMIT(og_mov(gc, MEMR(VOFS(fogCoord), edi), R(eax)));
        }
        else {
          EMIT(og_mov(gc, R(eax), MEM(&blk->fogPtr)));
          pc = CopyFogCoord(gc, pc, va->fogCoord.type, IMM(&blk->fetchTmp));
          EMIT(og_sub(gc, R(eax), IMM(va->fogCoord.stride)));
          EMIT(og_mov(gc, MEM(&blk->fogPtr), R(eax)));
        }
      }
    }
    /* the eye coordinates: ogLazyProcs[3 + size](gc, &eye, &obj) */
    if ((needs & __GL_HAS_EYE) != 0 || gc->state.enables.clipPlanes != 0) {
      EMIT(og_push(gc, R(edx)));
      EMIT(og_lea(gc, R(edx), MEMR(VOFS(obj), edi)));
      EMIT(og_push(gc, R(edx)));
      EMIT(og_lea(gc, R(edx), MEMR(VOFS(eye), edi)));
      EMIT(og_mov(gc, R(ecx), IMM(gc)));
      pc = og_callOperand(gc, pc, MEM(&hw->hwcx.ogLazyProcs[3 + va->vertex.size]), IMM(0x759));
      EMIT(og_pop(gc, R(edx)));
    }
    /* the normal transform: ogLazyProcs[8](gc, &normal, &normal) */
    if ((needs & __GL_HAS_NORMAL) != 0) {
      EMIT(og_push(gc, R(edx)));
      EMIT(og_lea(gc, R(edx), MEMR(VOFS(normal), edi)));
      EMIT(og_push(gc, R(edx)));
      EMIT(og_mov(gc, R(ecx), IMM(gc)));
      pc = og_callOperand(gc, pc, MEM(&hw->hwcx.ogLazyProcs[8]), IMM(0x762));
      EMIT(og_pop(gc, R(edx)));
    }
    /* the texture coordinates of each enabled unit, then texgen or the texture matrix */
    unit = 0;
    for (; unit < gc->fx.numTMUs; unit = (unit + 1)) {
      if (gc->texture.textureEnabled[unit] != 0) {
        if (noTexArray[unit] != 0) {
          EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.texture[unit].x)));
          EMIT(og_mov(gc, MEMR(VOFS(texture[unit].x), edi), R(eax)));
          EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.texture[unit].y)));
          EMIT(og_mov(gc, MEMR(VOFS(texture[unit].y), edi), R(eax)));
          EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.texture[unit].z)));
          EMIT(og_mov(gc, MEMR(VOFS(texture[unit].z), edi), R(eax)));
          EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.texture[unit].w)));
          EMIT(og_mov(gc, MEMR(VOFS(texture[unit].w), edi), R(eax)));
        }
        else {
          texCursor = &blk->texPtr[unit];
          EMIT(og_mov(gc, R(eax), MEM(texCursor)));
          pc = CopyCoord(gc, pc, va->texture.type[unit], va->texture.size[unit], VOFS(texture[unit].x));
          EMIT(og_sub(gc, R(eax), IMM(va->texture.stride[unit])));
          EMIT(og_mov(gc, MEM(texCursor), R(eax)));
        }
        if ((gc->state.enables.texture[unit] & __GL_TEXTURE_GEN_ANY_ENABLE) != 0) {
          /* texgen: __glSSTCalcMixedTextureUnit(gc, vertex, unit) */
          EMIT(og_push(gc, R(edx)));
          EMIT(og_mov(gc, R(eax), IMM(unit)));
          EMIT(og_push(gc, R(eax)));
          EMIT(og_mov(gc, R(edx), R(edi)));
          EMIT(og_mov(gc, R(ecx), IMM(gc)));
          pc = og_callRel32Gen(gc, pc, (GLuint)__glSSTCalcMixedTextureUnit);
          EMIT(og_pop(gc, R(edx)));
        }
        else {
          texMatrix = (__GLmatrixICD *)gc->transform.texture[unit];
          if (texMatrix->matrixType != __GL_MT_IDENTITY) {
            EMIT(og_push(gc, R(edx)));
            EMIT(og_lea(gc, R(edx), MEMR(VOFS(texture[unit].x), edi)));
            EMIT(og_push(gc, R(edx)));
            EMIT(og_mov(gc, R(ecx), IMM(gc)));
            EMIT(og_call(gc, MEM(TEXMAT_PROC(hw, unit))));
            EMIT(og_pop(gc, R(edx)));
          }
        }
      }
    }
    /* the colour: the current one, the colour array, or lighting (ogLazyProcs[1](gc, vertex)) */
    if ((gc->state.enables.general & __GL_LIGHTING_ENABLE) == 0) {
      if ((allNeeds & (__GL_HAS_PACKED_COLOR | __GL_HAS_FRONT_COLOR)) != 0) {
        if (noColorArray != 0) {
          EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.color.r)));
          EMIT(og_mov(gc, MEMR(VOFS(colors[0].r), edi), R(eax)));
          EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.color.g)));
          EMIT(og_mov(gc, MEMR(VOFS(colors[0].g), edi), R(eax)));
          EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.color.b)));
          EMIT(og_mov(gc, MEMR(VOFS(colors[0].b), edi), R(eax)));
          EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.color.a)));
          EMIT(og_mov(gc, MEMR(VOFS(colors[0].a), edi), R(eax)));
          EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.packedColor)));
          EMIT(og_mov(gc, MEMR(VOFS(packedColor), edi), R(eax)));
        }
        else {
          EMIT(og_mov(gc, R(eax), MEM(&blk->colorPtr)));
          pc = CopyColor(gc, pc, va->color.type, va->color.size, MEM(&blk->fetchTmp), MEM(&blk->colorTmp[0]),
                            MEM(&blk->colorTmp[1]), MEM(&blk->colorTmp[2]), MEM(&blk->colorTmp[3]),
                            (needs & ~__GL_HAS_FRONT_COLOR));
          EMIT(og_sub(gc, R(eax), IMM(va->color.stride)));
          EMIT(og_mov(gc, MEM(&blk->colorPtr), R(eax)));
        }
      }
    }
    else {
      EMIT(og_push(gc, R(edx)));
      EMIT(og_mov(gc, R(ecx), IMM(gc)));
      EMIT(og_mov(gc, R(edx), R(edi)));
      pc = og_callOperand(gc, pc, MEM(&hw->hwcx.ogLazyProcs[1]), IMM(0x7af));
      EMIT(og_pop(gc, R(edx)));
    }
    if (0 != 0) {
      noFlagsReset = og_newLabel(gc);
      EMIT(og_mov(gc, R(eax), MEM(&gc->vertexArray.flags)));
      EMIT(og_test(gc, R(eax), IMM(__GL_VERTARRAY_CACHE_STALE)));
      EMIT(og_jne(gc, noFlagsReset));
      EMIT(og_mov(gc, R(esi), MEMR(VOFS(flagBits), edi)));
      EMIT(og_and(gc, R(esi), IMM(0xfffff7df)));
      EMIT(og_mov(gc, MEMR(VOFS(flagBits), edi), R(esi)));
      EMIT(og_jmp(gc, clipDone));
      LABEL(noFlagsReset);
    }

    /* ---- the MVP transform (ecx = the MVP, edx = obj) into blk->clip, then 1/w into window.w ---- */
    EMIT(og_mov(gc, R(ecx), MEM(&gc->transform.modelView)));
    EMIT(og_add(gc, R(ecx), IMM(2 * sizeof(__GLmatrixICD))));
    pc = doMatrix(gc, pc, MEM(&blk->clip.x), MEM(&blk->clip.y), MEM(&blk->clip.z), MEM(&blk->clip.w),
                      mv[2].matrixType, &mv[2].matrix[0][0], va->vertex.size);
    if ((gc->constants.cpuCaps & __GL_CPU_3DNOW) != 0) {
      EMIT(og_femms(gc));
      EMIT(og_movd(gc, R(mm0), MEM(&blk->clip.w)));
      EMIT(og_pfrcp(gc, R(mm1), R(mm0)));
      EMIT(og_pfrcpit1(gc, R(mm0), R(mm1)));
      EMIT(og_pfrcpit2(gc, R(mm0), R(mm1)));
      EMIT(og_movd(gc, MEMR(VOFS(window.w), edi), R(mm0)));
      EMIT(og_femms(gc));
    }
    else {
      if ((gc->constants.cpuCaps & __GL_CPU_SSE) != 0) {
        EMIT(og_movss(gc, R(mm0), MEM(&blk->clip.w)));
        EMIT(og_rcpss(gc, R(mm7), R(mm0)));
        EMIT(og_mulss(gc, R(mm0), R(mm7)));
        EMIT(og_mulss(gc, R(mm0), R(mm7)));
        EMIT(og_addss(gc, R(mm7), R(mm7)));
        EMIT(og_subss(gc, R(mm7), R(mm0)));
        EMIT(og_movss(gc, MEMR(VOFS(window.w), edi), R(mm7)));
      }
      else {
        EMIT(og_fld(gc, MEM(&blk->clip.w)));
        EMIT(og_fdivr(gc, (OgOperand *)((GLuint (__fastcall *)(__GLcontext *, double))og_fpk)(gc, 1.0)));
        EMIT(og_fstp(gc, MEMR(VOFS(window.w), edi)));
      }
    }

    /* ---- the clip codes into esi (none when the hint is GL_FASTEST) ---- */
    if (clipCodes == 0) {
      EMIT(og_xor(gc, R(esi), R(esi)));
    }
    else {
      if (gc->fx.fullscreen != 0) {
        if (gc->fx.guardBand != 0) {
          if ((gc->state.enables.general & __GL_FOG_ENABLE) == 0) {
            /* the guard-band codes (left 0x10000, right 0x20000, bottom 0x40000, top 0x80000) into fog */
            if ((gc->constants.cpuCaps & __GL_CPU_SSE) != 0) {
              EMIT(og_movss(gc, R(mm7), MEM(&blk->clip.w)));
              EMIT(og_movss(gc, R(mm4), MEM(&blk->clip.x)));
              EMIT(og_movss(gc, R(mm5), MEM(&blk->clip.y)));
              EMIT(og_movaps(gc, R(mm3), R(mm7)));
              EMIT(og_mulss(gc, R(mm3), MEM(&blk->negOne[0])));
              EMIT(og_movaps(gc, R(mm2), R(mm3)));
              EMIT(og_mulss(gc, R(mm7), MEM((GLuint)blk + BOFS(gb[4]))));
              EMIT(og_mulss(gc, R(mm3), MEM((GLuint)blk + BOFS(gb[0]))));
              EMIT(og_movaps(gc, R(mm0), R(mm4)));
              EMIT(og_cmpps(gc, R(mm0), R(mm3), IMM(1)));               /* x < -w * gbLeft */
              EMIT(og_movss(gc, R(mm6), MEM(&blk->clipLeft[0])));
              EMIT(og_andps(gc, R(mm0), R(mm6)));
              EMIT(og_movaps(gc, R(mm1), R(mm4)));
              EMIT(og_cmpps(gc, R(mm1), R(mm7), IMM(6)));               /* x > w * gbRight */
              EMIT(og_movss(gc, R(mm6), MEM(&blk->clipRight[0])));
              EMIT(og_andps(gc, R(mm1), R(mm6)));
              EMIT(og_orps(gc, R(mm0), R(mm1)));
              EMIT(og_movss(gc, R(mm7), MEM(&blk->clip.w)));
              EMIT(og_movaps(gc, R(mm3), R(mm2)));
              EMIT(og_mulss(gc, R(mm7), MEM((GLuint)blk + BOFS(gb[12]))));
              EMIT(og_mulss(gc, R(mm3), MEM((GLuint)blk + BOFS(gb[8]))));
              EMIT(og_movaps(gc, R(mm1), R(mm5)));
              EMIT(og_cmpps(gc, R(mm1), R(mm3), IMM(1)));               /* y < -w * gbBottom */
              EMIT(og_movss(gc, R(mm6), MEM(&blk->clipBottom[0])));
              EMIT(og_andps(gc, R(mm1), R(mm6)));
              EMIT(og_orps(gc, R(mm0), R(mm1)));
              EMIT(og_movaps(gc, R(mm1), R(mm5)));
              EMIT(og_cmpps(gc, R(mm1), R(mm7), IMM(6)));               /* y > w * gbTop */
              EMIT(og_movss(gc, R(mm6), MEM(&blk->clipTop[0])));
              EMIT(og_andps(gc, R(mm1), R(mm6)));
              EMIT(og_orps(gc, R(mm0), R(mm1)));
              EMIT(og_movss(gc, MEMR(VOFS(fog), edi), R(mm0)));
            }
            else {
              if ((gc->constants.cpuCaps & __GL_CPU_3DNOW) != 0) {
                EMIT(og_femms(gc));
                EMIT(og_movd(gc, R(mm4), MEM(&blk->clip.w)));
                EMIT(og_punpckldq(gc, R(mm4), R(mm4)));
                EMIT(og_movd(gc, R(mm5), MEM(&blk->clip.w)));
                EMIT(og_punpckldq(gc, R(mm5), R(mm5)));
                EMIT(og_pfmul(gc, R(mm4), MEM((GLuint)blk + BOFS(gb[0]))));     /* w * {gbRight, gbTop} */
                EMIT(og_pfmul(gc, R(mm5), MEM((GLuint)blk + BOFS(gb[2]))));     /* w * {gbLeft, gbBottom} */
                EMIT(og_pxor(gc, R(mm3), R(mm3)));
                EMIT(og_pfmul(gc, R(mm5), MEM(&blk->negOne[0])));
                EMIT(og_movq(gc, R(mm2), MEM(&blk->clip.x)));
                EMIT(og_pfcmpgt(gc, R(mm2), R(mm4)));
                EMIT(og_pand(gc, R(mm2), MEM((GLuint)blk + BOFS(gbBits[0]))));
                EMIT(og_por(gc, R(mm3), R(mm2)));
                EMIT(og_movq(gc, R(mm2), MEM(&blk->clip.x)));
                EMIT(og_pfcmpgt(gc, R(mm5), R(mm2)));
                EMIT(og_pand(gc, R(mm5), MEM((GLuint)blk + BOFS(gbBits[2]))));
                EMIT(og_por(gc, R(mm3), R(mm5)));
                EMIT(og_movq(gc, R(mm4), R(mm3)));
                EMIT(og_punpckhdq(gc, R(mm4), R(mm4)));
                EMIT(og_por(gc, R(mm3), R(mm4)));
                EMIT(og_movd(gc, MEMR(VOFS(fog), edi), R(mm3)));
                EMIT(og_femms(gc));
              }
            }
          }
        }
      }
      /* esi = the frustum clip codes: the sign bits of w, z, y, x and the |c| > |w| compares through
      ** the three tables */
      EMIT(og_mov(gc, R(ebp), MEM(&blk->clip.w)));
      EMIT(og_add(gc, R(ebp), R(ebp)));
      EMIT(og_sbb(gc, R(esi), R(esi)));
      EMIT(og_mov(gc, R(ecx), MEM(&blk->clip.z)));
      EMIT(og_mov(gc, R(ebx), MEM(&blk->clip.y)));
      EMIT(og_mov(gc, R(eax), MEM(&blk->clip.x)));
      EMIT(og_adc(gc, R(ecx), R(ecx)));
      EMIT(og_adc(gc, R(esi), R(esi)));
      EMIT(og_add(gc, R(ebx), R(ebx)));
      EMIT(og_adc(gc, R(esi), R(esi)));
      EMIT(og_add(gc, R(eax), R(eax)));
      EMIT(og_adc(gc, R(esi), R(esi)));
      EMIT(og_cmp(gc, R(ebp), R(ecx)));
      EMIT(og_adc(gc, R(ecx), R(ecx)));
      EMIT(og_cmp(gc, R(ebp), R(ebx)));
      EMIT(og_adc(gc, R(ecx), R(ecx)));
      EMIT(og_cmp(gc, R(ebp), R(eax)));
      EMIT(og_adc(gc, R(ecx), R(ecx)));
      EMIT(og_mov(gc, R(ebx), R(esi)));
      EMIT(og_and(gc, R(ebx), IMM(0xf)));
      EMIT(og_and(gc, R(ecx), IMM(0xf)));
      EMIT(og_mov(gc, R(ecx), TAB(g_clipAxisBits, ecx)));
      EMIT(og_mov(gc, R(esi), TAB(g_clipSideBits, ebx)));
      EMIT(og_and(gc, R(esi), R(ecx)));
      EMIT(og_mov(gc, R(ecx), TAB(g_clipWNegBits, ebx)));
      EMIT(og_or(gc, R(esi), R(ecx)));
    }
    /* the user clip planes (bit 0x400000 << plane): the sign of eye . plane */
    if (gc->state.enables.clipPlanes != 0) {
      planeMask = gc->state.enables.clipPlanes;
      plane = gc->state.transform.eyeClipPlanes;
      planeBit = 0x400000;
      while (planeMask != 0) {
        if ((planeMask & 1) != 0) {
          EMIT(og_fld(gc, MEMR(VOFS(eye.x), edi)));
          EMIT(og_fmul(gc, MEM(plane)));
          EMIT(og_fld(gc, MEMR(VOFS(eye.y), edi)));
          EMIT(og_fmul(gc, MEM(&plane->y)));
          EMIT(og_fld(gc, MEMR(VOFS(eye.z), edi)));
          EMIT(og_fmul(gc, MEM(&plane->z)));
          EMIT(og_fld(gc, MEMR(VOFS(eye.w), edi)));
          EMIT(og_fmul(gc, MEM(&plane->w)));
          EMIT(og_fxch(gc, R(st3)));
          EMIT(og_faddp(gc, R(st2), R(st0)));
          EMIT(og_faddp(gc, R(st2), R(st0)));
          EMIT(og_faddp(gc, R(st1), R(st0)));
          EMIT(og_fstp(gc, MEM(&blk->tmp)));
          EMIT(og_mov(gc, R(eax), MEM(&blk->tmp)));
          EMIT(og_sar(gc, R(eax), IMM(0x1f)));
          EMIT(og_and(gc, R(eax), IMM(planeBit)));
          EMIT(og_or(gc, R(esi), R(eax)));
        }
        planeMask = (planeMask >> 1);
        planeBit = (planeBit << 1);
        plane = (plane + 1);
      }
    }

    /* ---- the flag bits: clip codes | fillFlags | needs computed | polygon mode | vertex size ---- */
    doneNeeds = (needs & 0x11d);      /* PACKED_COLOR | EYE | NORMAL | TEXTURE | FRONT_COLOR */
    if (gc->state.polygon.frontMode != GL_FILL || gc->state.polygon.backMode != GL_FILL) {
      doneNeeds = (doneNeeds | 0x1000);                         /* a polygon mode is not GL_FILL */
    }
    switch (va->vertex.size) {          /* (flagBits >> 14) & 3 picks procs.validateVertex[] */
    case 2:
      doneNeeds = (doneNeeds | 0x4000);
      break;
    case 3:
      doneNeeds = (doneNeeds | 0x8000);
      break;
    case 4:
      doneNeeds = (doneNeeds | 0xc000);
      break;
    }
    EMIT(og_or(gc, R(esi), IMM(gc->vertexArray.fillFlags | doneNeeds)));
    EMIT(og_mov(gc, MEMR(VOFS(flagBits), edi), R(esi)));

    /* ---- clip = blk->clip, window = clip * window.w * viewport scale + center ---- */
    if ((gc->constants.cpuCaps & __GL_CPU_3DNOW) != 0) {
      EMIT(og_femms(gc));
      EMIT(og_movd(gc, R(mm0), MEM(&blk->clip.x)));
      EMIT(og_movd(gc, R(mm1), MEM(&blk->clip.y)));
      EMIT(og_movd(gc, R(mm2), MEM(&blk->clip.z)));
      EMIT(og_movd(gc, R(mm3), MEM(&blk->clip.w)));
      EMIT(og_movd(gc, MEMR(VOFS(clip.x), edi), R(mm0)));
      EMIT(og_movd(gc, MEMR(VOFS(clip.y), edi), R(mm1)));
      EMIT(og_movd(gc, MEMR(VOFS(clip.z), edi), R(mm2)));
      EMIT(og_movd(gc, MEMR(VOFS(clip.w), edi), R(mm3)));
      EMIT(og_movd(gc, R(mm4), MEMR(VOFS(window.w), edi)));
      EMIT(og_pfmul(gc, R(mm0), R(mm4)));
      EMIT(og_pfmul(gc, R(mm1), R(mm4)));
      EMIT(og_pfmul(gc, R(mm2), R(mm4)));
      EMIT(og_pfmul(gc, R(mm0), MEM(&vp->scale.x)));
      EMIT(og_pfmul(gc, R(mm1), MEM(&vp->scale.y)));
      EMIT(og_pfmul(gc, R(mm2), MEM(&vp->scale.z)));
      EMIT(og_pfadd(gc, R(mm0), MEM(&vp->center.x)));
      EMIT(og_pfadd(gc, R(mm1), MEM(&vp->center.y)));
      EMIT(og_pfadd(gc, R(mm2), MEM(&vp->center.z)));
      EMIT(og_movd(gc, MEMR(VOFS(window.x), edi), R(mm0)));
      EMIT(og_movd(gc, MEMR(VOFS(window.y), edi), R(mm1)));
      EMIT(og_movd(gc, MEMR(VOFS(window.z), edi), R(mm2)));
      EMIT(og_femms(gc));
    }
    else {
      if ((gc->constants.cpuCaps & __GL_CPU_SSE) != 0) {
        EMIT(og_movss(gc, R(mm0), MEM(&blk->clip.x)));
        EMIT(og_movss(gc, R(mm1), MEM(&blk->clip.y)));
        EMIT(og_movss(gc, R(mm2), MEM(&blk->clip.z)));
        EMIT(og_movss(gc, R(mm3), MEM(&blk->clip.w)));
        EMIT(og_movss(gc, MEMR(VOFS(clip.x), edi), R(mm0)));
        EMIT(og_movss(gc, MEMR(VOFS(clip.y), edi), R(mm1)));
        EMIT(og_movss(gc, MEMR(VOFS(clip.z), edi), R(mm2)));
        EMIT(og_movss(gc, MEMR(VOFS(clip.w), edi), R(mm3)));
        EMIT(og_movss(gc, R(mm5), MEM(&vp->scale.x)));
        EMIT(og_movss(gc, R(mm6), MEM(&vp->scale.y)));
        EMIT(og_movss(gc, R(mm7), MEM(&vp->scale.z)));
        EMIT(og_mulss(gc, R(mm0), R(mm5)));
        EMIT(og_mulss(gc, R(mm1), R(mm6)));
        EMIT(og_mulss(gc, R(mm2), R(mm7)));
        EMIT(og_movss(gc, R(mm4), MEMR(VOFS(window.w), edi)));
        EMIT(og_mulss(gc, R(mm0), R(mm4)));
        EMIT(og_mulss(gc, R(mm1), R(mm4)));
        EMIT(og_mulss(gc, R(mm2), R(mm4)));
        EMIT(og_movss(gc, R(mm5), MEM(&vp->center.x)));
        EMIT(og_movss(gc, R(mm6), MEM(&vp->center.y)));
        EMIT(og_movss(gc, R(mm7), MEM(&vp->center.z)));
        EMIT(og_addss(gc, R(mm0), R(mm5)));
        EMIT(og_addss(gc, R(mm1), R(mm6)));
        EMIT(og_addss(gc, R(mm2), R(mm7)));
        EMIT(og_movss(gc, MEMR(VOFS(window.x), edi), R(mm0)));
        EMIT(og_movss(gc, MEMR(VOFS(window.y), edi), R(mm1)));
        EMIT(og_movss(gc, MEMR(VOFS(window.z), edi), R(mm2)));
      }
      else {
        EMIT(og_fld(gc, MEM(&blk->clip.x)));
        EMIT(og_fst(gc, MEMR(VOFS(clip.x), edi)));
        EMIT(og_fmul(gc, MEM(&vp->scale.x)));
        EMIT(og_fld(gc, MEM(&blk->clip.y)));
        EMIT(og_fst(gc, MEMR(VOFS(clip.y), edi)));
        EMIT(og_fmul(gc, MEM(&vp->scale.y)));
        EMIT(og_fld(gc, MEM(&blk->clip.z)));
        EMIT(og_fst(gc, MEMR(VOFS(clip.z), edi)));
        EMIT(og_fmul(gc, MEM(&vp->scale.z)));
        EMIT(og_fxch(gc, R(st2)));
        EMIT(og_mov(gc, R(eax), MEM(&blk->clip.w)));
        EMIT(og_mov(gc, MEMR(VOFS(clip.w), edi), R(eax)));
        EMIT(og_fmul(gc, MEMR(VOFS(window.w), edi)));
        EMIT(og_fld(gc, MEMR(VOFS(window.w), edi)));
        EMIT(og_fmulp(gc, R(st2), R(st0)));
        EMIT(og_fld(gc, MEMR(VOFS(window.w), edi)));
        EMIT(og_fmulp(gc, R(st3), R(st0)));
        EMIT(og_fadd(gc, MEM(&vp->center.x)));
        EMIT(og_fxch(gc, R(st1)));
        EMIT(og_fadd(gc, MEM(&vp->center.y)));
        EMIT(og_fxch(gc, R(st2)));
        EMIT(og_fadd(gc, MEM(&vp->center.z)));
        EMIT(og_fxch(gc, R(st1)));
        EMIT(og_fstp(gc, MEMR(VOFS(window.x), edi)));
        EMIT(og_fstp(gc, MEMR(VOFS(window.z), edi)));
        EMIT(og_fstp(gc, MEMR(VOFS(window.y), edi)));
      }
    }
    LABEL(clipDone);
    EMIT(og_sub(gc, R(edx), IMM(vtxStride)));

    /* ---- texture coordinates scaled for Glide: s, t (and q with nonUnityQ) by window.w unless the
    ** vertex is near-clipped, s, t by the texture's texScale ---- */
    if ((needs & __GL_HAS_TEXTURE) != 0) {
      texDone = og_newFixup(gc);
      texUnprojected = og_newFixup(gc);
      EMIT(og_test(gc, R(esi), IMM(0x100000)));                /* the near clip bit */
      EMIT(og_jne(gc, texUnprojected));
      unit = 0;
      for (; unit < gc->fx.numTMUs; unit = (unit + 1)) {
        if (gc->texture.textureEnabled[unit] != 0) {
          curTexA = __glCurrentTexture(gc, unit);
          curTexA2 = curTexA;
          EMIT(og_mov(gc, R(eax), MEM(&gc->texture.boundTexture[unit][gc->texture.currentIndex[unit]])));
          EMIT(og_fld(gc, MEMR(VOFS(texture[unit].x), edi)));
          EMIT(og_fmul(gc, MEMR(VOFS(window.w), edi)));
          EMIT(og_fmul(gc, MEMR(TOFS(texScale), eax)));
          EMIT(og_fstp(gc, MEMR(VOFS(texture[unit].x), edi)));
          EMIT(og_fld(gc, MEMR(VOFS(texture[unit].y), edi)));
          EMIT(og_fmul(gc, MEMR(VOFS(window.w), edi)));
          EMIT(og_fmul(gc, MEMR(TOFS(texScale) + 4, eax)));
          EMIT(og_fstp(gc, MEMR(VOFS(texture[unit].y), edi)));
          if (hw->hwcx.nonUnityQ != 0) {
            EMIT(og_fld(gc, MEMR(VOFS(texture[unit].w), edi)));
            EMIT(og_fmul(gc, MEMR(VOFS(window.w), edi)));
            EMIT(og_fstp(gc, MEMR(VOFS(texture[unit].w), edi)));
          }
        }
      }
      EMIT(og_jmp(gc, texDone));
      LABEL(texUnprojected);
      unit = 0;
      for (; unit < gc->fx.numTMUs; unit = (unit + 1)) {
        if (gc->texture.textureEnabled[unit] != 0) {
          curTexB = __glCurrentTexture(gc, unit);
          curTexB2 = curTexB;
          EMIT(og_mov(gc, R(eax), MEM(&gc->texture.boundTexture[unit][gc->texture.currentIndex[unit]])));
          EMIT(og_fld(gc, MEMR(VOFS(texture[unit].x), edi)));
          EMIT(og_fmul(gc, MEMR(TOFS(texScale), eax)));
          EMIT(og_fstp(gc, MEMR(VOFS(texture[unit].x), edi)));
          EMIT(og_fld(gc, MEMR(VOFS(texture[unit].y), edi)));
          EMIT(og_fmul(gc, MEMR(TOFS(texScale) + 4, eax)));
          EMIT(og_fstp(gc, MEMR(VOFS(texture[unit].y), edi)));
        }
      }
      LABEL(texDone);
    }

    /* ---- the next vertex; the exit ---- */
    EMIT(og_sub(gc, R(edi), IMM(sizeof(__GLvertexICD))));
    EMIT(og_dec(gc, argCount));
    EMIT(og_jne(gc, loopTop));
    if (0 != 0) {
      EMIT(og_fldcw(gc, MEM(&blk->fpuSave)));
    }
    EMIT(og_pop(gc, R(ebp)));
    EMIT(og_pop(gc, R(ecx)));
    EMIT(og_pop(gc, R(ebx)));
    EMIT(og_pop(gc, R(edi)));
    EMIT(og_pop(gc, R(esi)));
    EMIT(og_ret_imm(gc, IMM(8)));

    /* ---- fetchVertex entry (edx = the vertex): its colour, once (flag 0x20) ---- */
    blk->gbClipScale = 0x3f8ccccd;      /* 1.1f */
    va->fetchVertex = (__GLproc)pc;
    blk->fetchVertex = pc;
    if ((arrays & __GL_VA_VERTEX_ENABLE) != 0) {
      EMIT(og_mov(gc, R(eax), MEMR(VOFS(flagBits), edx)));
      EMIT(og_test(gc, R(eax), IMM(0x20)));
      EMIT(og_jne(gc, fetchDone));
      EMIT(og_push(gc, R(esi)));
      EMIT(og_push(gc, R(edi)));
      EMIT(og_push(gc, R(ebx)));
      EMIT(og_mov(gc, R(edi), R(edx)));
      EMIT(og_or(gc, R(eax), IMM(0x21)));
      EMIT(og_mov(gc, MEMR(VOFS(flagBits), edx), R(eax)));
      /* eax = the vertex number: (vertex - vertexCache) / 0xc0 */
      EMIT(og_mov(gc, R(ebx), MEM(&va->vertexCache)));
      EMIT(og_mov(gc, R(eax), R(edx)));
      EMIT(og_sub(gc, R(eax), R(ebx)));
      EMIT(og_mov(gc, R(ecx), IMM(0x1555556)));
      EMIT(og_mul(gc, R(ecx)));
      EMIT(og_mov(gc, R(eax), R(edx)));
      if (va->vertex.type == GL_FLOAT) {
        EMIT(og_mov(gc, R(edx), R(eax)));
        pc = og_scaleby(gc, pc, R(edx), IMM(va->vertex.stride));
        EMIT(og_mov(gc, R(ebx), MEM(&va->vertex.pointer)));
        EMIT(og_add(gc, R(edx), R(ebx)));
      }
      else {
        EMIT(og_lea(gc, R(edx), MEMR(VOFS(obj.x), edi)));
      }
      pc = og_scaleby(gc, pc, R(eax), IMM(va->color.stride));
      EMIT(og_mov(gc, R(ebx), MEM(&va->color.pointer)));
      EMIT(og_add(gc, R(eax), R(ebx)));
      if ((allNeeds & __GL_HAS_FRONT_COLOR) != 0) {
        if (noColorArray == 0) {
          pc = CopyColor(gc, pc, va->color.type, va->color.size, MEM(&blk->fetchTmp), MEM(&blk->colorTmp[0]),
                            MEM(&blk->colorTmp[1]), MEM(&blk->colorTmp[2]), MEM(&blk->colorTmp[3]),
                            (needs & ~__GL_HAS_PACKED_COLOR));
        }
      }
      if (getenv("GB_CLIP") != NULL) {
        /* flag 0x800 when |x| or |y| > 1.1 * |w| (the integer compare of the doubled float bits) */
        gbClipSkip = og_newLabel(gc);
        EMIT(og_mov(gc, R(eax), MEMR(VOFS(flagBits), edi)));
        EMIT(og_test(gc, R(eax), IMM(0xffff0000)));            /* any clip code */
        EMIT(og_je(gc, gbClipSkip));
        EMIT(og_fld(gc, MEMR(VOFS(clip.w), edi)));
        EMIT(og_fmul(gc, MEM(&blk->gbClipScale)));
        EMIT(og_fstp(gc, MEM(&blk->tmp)));
        EMIT(og_mov(gc, R(eax), MEM(&blk->tmp)));
        EMIT(og_add(gc, R(eax), R(eax)));
        EMIT(og_mov(gc, R(ebx), MEMR(VOFS(clip.x), edi)));
        EMIT(og_add(gc, R(ebx), R(ebx)));
        EMIT(og_mov(gc, R(ecx), R(eax)));
        EMIT(og_sub(gc, R(ecx), R(ebx)));
        EMIT(og_mov(gc, R(ebx), MEMR(VOFS(clip.y), edi)));
        EMIT(og_add(gc, R(ebx), R(ebx)));
        EMIT(og_sub(gc, R(eax), R(ebx)));
        EMIT(og_or(gc, R(eax), R(ecx)));
        EMIT(og_sar(gc, R(eax), IMM(0x1f)));
        EMIT(og_and(gc, R(eax), IMM(0x800)));
        EMIT(og_or(gc, MEMR(VOFS(flagBits), edi), R(eax)));
        LABEL(gbClipSkip);
      }
      EMIT(og_pop(gc, R(ebx)));
      EMIT(og_pop(gc, R(edi)));
      EMIT(og_pop(gc, R(esi)));
    }
    LABEL(fetchDone);
    EMIT(og_ret(gc));
  }
}
#undef gbClipSkip
#undef curTexB2
#undef curTexB
#undef curTexA2
#undef curTexA
#undef texDone
#undef texUnprojected
#undef doneNeeds
#undef planeBit
#undef planeMask
#undef plane
#undef noFlagsReset
#undef texMatrix
#undef texCursor
#undef texCursor0
#undef j
#undef texFloat2
#undef key
#undef vtxStride
#undef code
#undef vp
#undef noColorArray
#undef unit
#undef clipCodes
#undef needs
#undef modeFlags
#undef depthWrites
#undef loopTop
#undef mv
#undef noFogArray
#undef i
#undef arrays
#undef hw
#undef allNeeds
#undef clipDone
#undef noTexArray
#undef noNormalArray
#undef argCount
#undef texEnables
#undef pc
#undef argFirst
#undef blk
#undef normalStride
#undef fogStride
#undef va
#undef fetchDone
#pragma optimize("", on)

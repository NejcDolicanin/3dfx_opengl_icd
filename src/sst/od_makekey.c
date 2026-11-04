/*
** od_makekey.c -- make_key, the cache key of the vertex-array code generator hwGenVertexArrayFill (the
** ICD's make_key, GEOM_OG.C).
** The original compiled it without optimisation (/Od).
*/
#include "wglpriv.h"
#include "icdprotos.h"
#include "glsi.h"

#define KEY_WORDS   23      /* = the bufRec[2] keySize 0x5c (wgldev.c) */

/* 69056f40 -- make_key (GEOM_OG.C), rewritten as a fixed 23-dword key (0x5c bytes, all of them written):
**   [0]      bit per texture unit: textureEnabled
**   [1]      enableFlags of the vertex, normal, colour, index, fog-coordinate arrays and the texture
**            coordinate arrays of the enabled units
**   [2]      vertexArray.formats
**   [3..7]   the strides of the enabled vertex, normal, colour, index, fog-coordinate arrays
**   [8..11]  per enabled unit: texFormats, the texture-coordinate stride
**   [12]     bit per unit: texgen enabled;  [13] bit per unit: the texture matrix is the identity
**   [14]     the modelview's mvp matrixType;  [15] enabled clip planes;  [16] lighting
**   [17]     hints.ext590;  [18] a polygon mode is not GL_FILL;  [19] hwcx.nonUnityQ
**   [20]     what_needs & (packed colour | eye | normal | texture | front colour)
**   [21]     the fullscreen guard band without fog;  [22] fog from the fog-coordinate array
** Its only caller (hwGenVertexArrayFill) passes a 23-dword frame buffer at [ebp-0xcc, ebp-0x70). */
#pragma optimize("", off)
void __fastcall make_key(GLuint *key, __GLcontext *gc)
{
  struct {                   /* the /Od frame [ebp-0x18, ebp) */
    GLint unit;              /* ebp-0x18 */
    GLuint needs;            /* ebp-0x14 */
    GLint k;                 /* ebp-0x10  the next key word */
    GLuint texGenMask;       /* ebp-0xc */
    GLuint arrays;           /* ebp-0x8  vertexArray.enableFlags */
    GLint i;                 /* ebp-0x4 */
  } od_f;

  for (od_f.i = 0; od_f.i < KEY_WORDS; od_f.i++) {
    key[od_f.i] = 0;
  }
  for (od_f.unit = 0; od_f.unit < 2; od_f.unit++) {
    key[0] = key[0] | ((gc->texture.textureEnabled[od_f.unit] != 0) << od_f.unit);
  }

  od_f.arrays = gc->vertexArray.enableFlags;
  key[1] = key[1] | (od_f.arrays & 0x20);         /* vertex */
  key[1] = key[1] | (od_f.arrays & 1);            /* normal */
  key[1] = key[1] | (od_f.arrays & 2);            /* colour */
  key[1] = key[1] | (od_f.arrays & 8);            /* index */
  key[1] = key[1] | (od_f.arrays & 0x40);         /* fog coordinate */
  for (od_f.unit = 0; od_f.unit < 2; od_f.unit++) {
    if (gc->texture.textureEnabled[od_f.unit] != 0) {
      key[1] = key[1] | (od_f.arrays & (0x10000 << od_f.unit));
    }
  }
  key[2] = gc->vertexArray.formats;
  if (od_f.arrays & 0x20) {
    key[3] = gc->vertexArray.vertex.stride;
  }
  if (od_f.arrays & 1) {
    key[4] = gc->vertexArray.normal.stride;
  }
  if (od_f.arrays & 2) {
    key[5] = gc->vertexArray.color.stride;
  }
  if (od_f.arrays & 8) {
    key[6] = gc->vertexArray.index.stride;
  }
  if (od_f.arrays & 0x40) {
    key[7] = gc->vertexArray.fogCoord.stride;
  }

  od_f.k = 8;
  for (od_f.unit = 0; od_f.unit < 2; od_f.unit++) {
    if (gc->texture.textureEnabled[od_f.unit] != 0) {
      key[od_f.k] = gc->vertexArray.texFormats[od_f.unit];
      key[od_f.k + 1] = gc->vertexArray.texture.stride[od_f.unit];
    }
    od_f.k = od_f.k + 2;
  }
  od_f.texGenMask = __GL_TEXTURE_GEN_S_ENABLE | __GL_TEXTURE_GEN_T_ENABLE
                  | __GL_TEXTURE_GEN_R_ENABLE | __GL_TEXTURE_GEN_Q_ENABLE;
  for (od_f.unit = 0; od_f.unit < 2; od_f.unit++) {
    key[od_f.k] = key[od_f.k] | (((gc->state.enables.texture[od_f.unit] & od_f.texGenMask) != 0) << od_f.unit);
    key[od_f.k + 1] = key[od_f.k + 1]
                    | ((gc->transform.texture[od_f.unit]->matrix.matrixType == __GL_MT_IDENTITY) << od_f.unit);
  }
  od_f.k = od_f.k + 2;
  key[od_f.k] = gc->transform.modelView->mvp.matrixType;
  od_f.k = od_f.k + 1;
  key[od_f.k] = gc->state.enables.clipPlanes;
  od_f.k = od_f.k + 1;
  key[od_f.k] = gc->state.enables.general & __GL_LIGHTING_ENABLE;
  od_f.k = od_f.k + 1;
  key[od_f.k] = gc->state.hints.ext590;
  od_f.k = od_f.k + 1;
  key[od_f.k] = (gc->state.polygon.frontMode != GL_FILL || gc->state.polygon.backMode != GL_FILL);
  od_f.k = od_f.k + 1;
  key[od_f.k] = gc->hwcx.nonUnityQ;
  od_f.k = od_f.k + 1;
  od_f.needs = what_needs(gc);
  key[od_f.k] = od_f.needs & (__GL_HAS_PACKED_COLOR | __GL_HAS_EYE | __GL_HAS_NORMAL | __GL_HAS_TEXTURE
                              | __GL_HAS_FRONT_COLOR);
  od_f.k = od_f.k + 1;
  key[od_f.k] = (gc->fx.fullscreen != 0 && gc->fx.guardBand != 0
                 && (gc->state.enables.general & __GL_FOG_ENABLE) == 0);
  od_f.k = od_f.k + 1;
  key[od_f.k] = ((gc->state.enables.general & __GL_FOG_ENABLE) != 0
                 && gc->state.fog.coordSource == GL_FOG_COORDINATE_EXT);
  od_f.k = od_f.k + 1;          /* 23 */
}
#pragma optimize("", on)

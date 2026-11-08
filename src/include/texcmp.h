/*
** texcmp.h -- the compressed texture formats of the hardware path: the GL internal formats, the Glide texel
** formats they are stored in, and the DXT codec (sst_dxt.c).  S3TC input is kept as DXT blocks for Napalm's native
** GR_TEXFMT_ARGB_CMP_DXT1 / 3 / 5 (as MesaFX does); only the FXT1 internal formats go through the texus encoder.
*/
#ifndef TEXCMP_H
#define TEXCMP_H

#define GL_RGB_S3TC                         0x83a0      /* GL_S3_s3tc */
#define GL_RGB4_S3TC                        0x83a1
#define GL_RGBA_S3TC                        0x83a2
#define GL_RGBA4_S3TC                       0x83a3
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT     0x83f0      /* GL_EXT_texture_compression_s3tc */
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT    0x83f1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT    0x83f2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT    0x83f3
#define GL_COMPRESSED_RGB_ARB               0x84ed      /* GL_ARB_texture_compression: generic -> FXT1 */
#define GL_COMPRESSED_RGBA_ARB              0x84ee
#define GL_COMPRESSED_RGB_FXT1_3DFX         0x86b0      /* GL_3DFX_texture_compression_FXT1 */
#define GL_COMPRESSED_RGBA_FXT1_3DFX        0x86b1

#define TEXCMP_GR_FXT1                      0x11        /* GR_TEXFMT_ARGB_CMP_FXT1: 8x4 blocks of 16 bytes */
#define TEXCMP_GR_DXT1                      0x16        /* GR_TEXFMT_ARGB_CMP_DXT1: 4x4 blocks of 8 bytes */
#define TEXCMP_GR_DXT3                      0x18        /* GR_TEXFMT_ARGB_CMP_DXT3: 4x4 blocks of 16 bytes */
#define TEXCMP_GR_DXT5                      0x1a        /* GR_TEXFMT_ARGB_CMP_DXT5: 4x4 blocks of 16 bytes */

/* an internal format that is stored compressed (compressedImage holds the level) */
#define TEXCMP_INTERNAL(f) (((f) >= GL_RGB_S3TC && (f) <= GL_RGBA4_S3TC) || \
                            ((f) >= GL_COMPRESSED_RGB_S3TC_DXT1_EXT && (f) <= GL_COMPRESSED_RGBA_S3TC_DXT5_EXT) || \
                            (f) == GL_COMPRESSED_RGB_FXT1_3DFX || (f) == GL_COMPRESSED_RGBA_FXT1_3DFX)
/* a compressed Glide texel format; the DXT ones */
#define TEXCMP_GR(g)       ((g) == TEXCMP_GR_FXT1 || TEXCMP_GR_DXT(g))
#define TEXCMP_GR_DXT(g)   ((g) == TEXCMP_GR_DXT1 || (g) == TEXCMP_GR_DXT3 || (g) == TEXCMP_GR_DXT5)
/* block geometry: width, height, bytes */
#define TEXCMP_BLOCK_W(g)  ((g) == TEXCMP_GR_FXT1 ? 8 : 4)
#define TEXCMP_BLOCK_H(g)  4
#define TEXCMP_BLOCK_BYTES(g) ((g) == TEXCMP_GR_DXT1 ? 8 : 16)

/* sst_dxt.c: ARGB8888 texels (a << 24 | r << 16 | g << 8 | b, w * h of them) <-> DXT blocks of a w x h level.
** punch: DXT1 with 1-bit alpha (texels with alpha < 128 come out transparent); off for the RGB-only formats */
void __fastcall hwDxtEncode(void *dst, const GLuint *src, GLint w, GLint h, GLint grFormat, GLboolean punch);
void __fastcall hwDxtDecode(GLuint *dst, const void *src, GLint w, GLint h, GLint grFormat);

#endif

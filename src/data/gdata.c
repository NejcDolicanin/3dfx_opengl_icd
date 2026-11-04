/*
** gdata.c -- the globals without an owning module: the tables of the generated (og) code, which several generator
** files index (sst_tmumem.c, sst_lockedtri.c, od_vafill.c, sst_lazyprocs.c, sst_tribuild.c; declared in icdglob.h),
** and the original's unreferenced data whose owner is unknown.  Every other global lives in its module.
*/
#include "wglpriv.h"

/* ---- the generated code's tables (icdglob.h) */
/* 690eff90 */
GLuint g_clipSideBits[16] = { 0x002a0000, 0x00290000, 0x00260000, 0x00250000, 0x001a0000, 0x00190000, 0x00160000, 0x00150000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000, 0x00ff0000 };
/* 690effd0 */
GLuint g_clipWNegBits[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0x002a0000, 0x00290000, 0x00260000, 0x00250000, 0x001a0000, 0x00190000, 0x00160000, 0x00150000 };
/* 690f0010 */
GLuint g_clipKeepBits[5] = { 0, 0x00010000, 0x00030000, 0x00070000, 0x000f0000 };
/* 690f0024 -- the generated code's constants (icdglob.h): the vertex source masks by size (2 bits per component:
** 0xc5 size 2, 0xd5 3, 0x55 4), [1] 49152.0f (1.5 * 2^15: x + 49152 leaves x * 256 in the low mantissa bits), [8] 1.0f */
GLuint g_vtxSizeMasks[9] = { 0x001f0000, 0x47400000, 0x000000c5, 0x000000d5, 0x00000055, 0x00000000, 0x00000000, 0x00000000, 0x3f800000 };

/* ---- unreferenced data of unknown owner, named by its address in the original */
/* 690f4548 -- 8 words after s_clip.c's clipProcs ({0, 0, 0, 1} twice), owner unknown; ToDo UNUSED CODE: nothing in the
** driver reads them */
GLuint g_unknown_690f4548[8] = { 0, 0, 0, 0x3f800000, 0, 0, 0, 0x3f800000 };
/* 690f4680 -- 20 words after s_image.c's HighBitsMask, owner unknown; ToDo UNUSED CODE: nothing in the driver reads them */
GLuint g_unknown_690f4680[20] = { 0, 0, 0, 0, 1, 2, 0, 1, 0, 0, 0, 2, 1, 2, 0, 0, 0, 2, 1, 2 };
/* 690f5a40 -- 4 words after the no-context dispatch table ({0, 0, 0, 1.0f}), owner unknown; ToDo UNUSED CODE: nothing in
** the driver reads them */
GLuint g_unknown_690f5a40[4] = { 0, 0, 0, 0x3f800000 };

/* ---- .text constants of the generated code (icdglob.h; g_clipSubdivTable must follow g_clipAxisBits) */
/* 69003b30 */
GLuint g_clipAxisBits[16] = {
    0x00000000, 0x00030000, 0x000c0000, 0x000f0000,
    0x00300000, 0x00330000, 0x003c0000, 0x003f0000,
    0x003f0000, 0x003c0000, 0x00330000, 0x00300000,
    0x000f0000, 0x000c0000, 0x00030000, 0x00000000
};
/* 69003b70 -- the clip subdivision table (sst_tribuild.c): a row of 10 bytes by the 3-bit clip code - 1 */
GLubyte g_clipSubdivTable[6][10] = {
    { 0, 2, 1, 2, 0, 1, 4, 0, 4, 3 },
    { 0, 1, 2, 1, 0, 3, 4, 0, 4, 2 },
    { 0, 1, 0, 2, 0, 0, 0, 0, 3, 4 },
    { 1, 0, 2, 0, 1, 2, 4, 1, 4, 3 },
    { 1, 0, 1, 2, 0, 0, 0, 1, 4, 3 },
    { 2, 0, 2, 1, 0, 0, 0, 2, 3, 4 }
};

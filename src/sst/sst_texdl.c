/*
** sst_texdl.c -- 3dfx back end, original VA range 69034000..6903c000: texture manager (part 1): the og call
** helper, the partial mipmap download and the copyTexImage proc of the sst texture object.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "og.h"
#include <glide.h>

extern void __cdecl txImgQuantize(void *dst, void *src, int w, int h, GLuint format, GLuint dither);
extern int a_color_cc, a_force_cc;      /* texus codec.c (FXT1): -1 = no forced colour coding / mode */
void __fastcall hwTexDownloadRows(GLint tmu, GLuint startAddress, GLint level, GLint largeLod, GLint aspectRatio,
                             GLint format, void *data, GLint t0, GLint t1);

/* 6903a4b0 -- og_callRel32Locked: og_callRel32: emit `call routine` (rel32) at pc: a label already placed at the
** routine */
GLubyte *__fastcall og_callRel32Locked(__GLcontext *gc, GLubyte *pc, GLuint routine)
{
    OgLabel *target;

    target = og_newFixup(gc);
    target->addr = (GLubyte *)routine;
    return og_encode(gc, pc, og_call(gc, target));
}

/* 6903b620 -- hwTexDownloadRows: download rows t0..t1 of mipmap level `level` (both even and odd levels): Glide
** numbers the LODs by log2 size, so the level's LOD is largeLod - level */
void __fastcall hwTexDownloadRows(GLint tmu, GLuint startAddress, GLint level, GLint largeLod, GLint aspectRatio,
                             GLint format, void *data, GLint t0, GLint t1)
{
    grTexDownloadMipMapLevelPartial(tmu, startAddress, largeLod - level, largeLod, aspectRatio, format,
                                    GR_MIPMAPLEVELMASK_BOTH, data, t0, t1);
}

/* 6903baa0 -- __glSSTCopyTexImage: tex->copyTexImage of the sst texture object: the generic image copy, then a
** compressed level is (re)built from it with texus (the FXT1 codec's forced modes reset first) */
void __fastcall __glSSTCopyTexImage(__GLcontext *gc, void *spanInfo, __GLtextureICD *tex, GLint level)
{
    __GLmipMapLevelICD *lp;

    lp = tex->level[level];
    a_force_cc = -1;
    a_color_cc = -1;
    (*gc->procs.copyImage)(gc, spanInfo, GL_TRUE);
    if (lp->compressedImage) {
        txImgQuantize(lp->compressedImage, lp->buffer, lp->width, lp->height,
                      tex->level[0]->texFormat->grFormat, 0);
    }
}

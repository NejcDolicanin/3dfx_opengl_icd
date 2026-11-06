/*
** sst_texmgr.c -- the texture manager (TMU memory, the DirectDraw surfaces of the windowed path) and the sst
** texture object (create, make resident, download, levels).
*/
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "siprotos.h"
#include "glshade.h"
#include "si_names.h"
#include "icdglob.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "og.h"
#include "icdprotos.h"
#include <stdio.h>
#include "gldisp.h"

/* 690f66f8 .. 690f6704 -- the default border colour (0, 0, 0, 0): a texture with another one needs
** GL_CLAMP_TO_EDGE on the hardware */
float g_defaultBorderR = 0, g_defaultBorderG = 0, g_defaultBorderB = 0, g_defaultBorderA = 0;
#define TEX_SLOT(field, tmu)    ((field)[__GL_TMU_SLOT(tmu)])
/* the two FXT1 compressed texel formats (icdglob.h; 6904d740 picks them for GL_COMPRESSED_RGB / RGBA_FXT1_3DFX) */
#define IS_FXT1_FORMAT(fi)  ((fi) == &g_texFmtFXT1RGB || (fi) == &g_texFmtFXT1RGBA)
#define DBG     ((void (__cdecl *)(const char *, ...))texMgrDebugPrintf)
extern int __cdecl txTexCalcMapSize(int w, int h, int fmt);
extern void __fastcall texMgrRelease(__GLcontext *gc);
extern __GLtextureFxICD *__fastcall hwCreateTexture(__GLcontext *gc, GLuint name, GLuint target);
extern GLboolean __fastcall texMgrReleaseSurface(__GLcontext *gc, __GLtexMemBlockICD *blk, GLint tmu);
extern GLuint __fastcall MakeResident(__GLcontext *gc, __GLtextureFxICD *tex, GLuint a, GLuint tmu);
extern void *__fastcall __glSSTTexCreateLevel(__GLcontext *gc, __GLtextureFxICD *tex, GLint level, GLenum ifmt, GLint w, GLint h, GLint a5, GLint a6, GLint a7);
extern GLuint __fastcall MakeResidentRetry(__GLcontext *gc, __GLtextureFxICD *tex, GLuint unused, GLuint tmu);
extern void __fastcall DownloadTexture(__GLcontext *gc, __GLtextureFxICD *tex, GLuint tmu);
extern GLubyte *__fastcall ReplicateAspectTexels(__GLcontext *gc, __GLmipMapLevelICD *lvl, GLubyte *data, GLint *w, GLint *h);
extern GLint __fastcall __glSSTAllocateTextureMemory(__GLcontext *gc, __GLtextureFxICD *tex, GLuint unused, GLuint tmu);
extern GLboolean __fastcall MakeResidentTryAllocateSurface(__GLcontext *gc, __GLtextureFxICD *tex, GLint idx);
/* 6903b660 -- __glSSTInitTextureManager: the device's texture manager constructor: the shared manager (refs 1, its
** free / reset procs, DirectDraw), per TMU slot one free heap block from after the sprite disc (690602d0) over all
** memory (size -1); with TEXUMA both slots hold block 0.  The context's createTexture becomes the sst one (6903b930) */
void __fastcall __glSSTInitTextureManager(__GLcontext *gc)
{
    __GLtexMgrFxICD *m;
    __GLtexMemBlockICD *blk;
    int i;

    m = (__GLtexMgrFxICD *)__GL_MALLOC(gc, sizeof(__GLtexMgrFxICD));
    m->refs = 1;
    m->free = (__GLproc)texMgrRelease;
    m->reset = (__GLproc)texMgrReset;
    m->ddraw = wglDDrawAllocate();
    m->end = 0;
    for (i = 0; i < 2; i++) {
        blk = (__GLtexMemBlockICD *)__GL_MALLOC(gc, sizeof(__GLtexMemBlockICD));
        blk->start = hwPlaceSpriteTexture(gc, i, 0);
        blk->size = 0xffffffff;
        blk->tex = NULL;
        blk->next = NULL;
        m->heap[__GL_TMU_SLOT(i)] = blk;
        m->rover[__GL_TMU_SLOT(i)] = blk;
    }
    m->gc = gc;
    gc->texMgr = (__GLtexMgrICD *)m;
    gc->createTexture = (__GLproc)hwCreateTexture;
}

/* 6903b700 -- texMgrSetMemoryEnd: moves the end of texture memory: each TMU slot's first block now reaches it
** (size = end - start + grGet(GR_TEXTURE_ALIGN)) */
void __fastcall texMgrSetMemoryEnd(__GLcontext *gc, GLuint end)
{
    __GLtexMgrFxICD *m = (__GLtexMgrFxICD *)gc->texMgr;
    __GLtexMemBlockICD *blk;
    GLint i;

    m->end = end;
    for (i = 0; i < 2; i++) {
        blk = m->heap[__GL_TMU_SLOT(i)];
        blk->size = m->end - blk->start + grGetInteger(GR_TEXTURE_ALIGN);
    }
}

/* 6903b740 (size 90) -- texMgrRelease: release the texture manager (the last user frees DirectDraw and the heaps) */
void __fastcall texMgrRelease(__GLcontext *gc)
{
    __GLtexMgrFxICD *m = (__GLtexMgrFxICD *)gc->texMgr;

    if (--m->refs == 0) {
        GLint i;

        wglDDrawFree(m->ddraw);
        for (i = 0; i < 2; i++)
            __GL_FREE(gc, m->heap[__GL_TMU_SLOT(i)]);
        __GL_FREE(gc, m);
    }
    gc->texMgr = NULL;
}

/* 6903b7a0 (size 330) -- texMgrReset: reset the texture manager: free every heap block (evicting its texture's
** surfaces), then one free block per TMU spanning the whole texture memory */
GLboolean __fastcall texMgrReset(__GLcontext *gc, __GLtexMgrFxICD *m)
{
    __GLtexMemBlockICD *b, *next;
    GLint i;

    grFinish();
    for (i = 0; i < gc->fx.numTMUs; i++) {
        b = m->heap[__GL_TMU_SLOT(i)];
        while (b != NULL) {
            if (gc->hwcx.windowed)
                texMgrReleaseSurface(gc, b, i);
            if (b->tex) {
                b->tex->mem[__GL_TMU_SLOT(i)] = NULL;
                b->tex->onTmu[__GL_TMU_SLOT(i)] = 0;
                b->tex->t.hwHandle[__GL_TMU_SLOT(i)] = 0;
            }
            next = b;
            b = b->next;
            __GL_FREE(gc, next);
        }
        b = (__GLtexMemBlockICD *)__GL_MALLOC(gc, 0x18);
        b->start = hwPlaceSpriteTexture(gc, i, grTexMinAddress(0));
        b->size = grGetInteger(GR_TEXTURE_ALIGN) + (m->end - b->start);
        b->tex = NULL;
        b->next = NULL;
        m->heap[__GL_TMU_SLOT(i)] = b;
        m->rover[__GL_TMU_SLOT(i)] = b;
        gc->hwcx.unitTexture[i] = (void *)0xffffffff;
    }
    return GL_TRUE;
}

/* 6903b8f0 -- texMgrReleaseSurface: releases a texture memory block's DirectDraw surface (windowed: hwSurface)
** after grFinish while a Glide context is open; FALSE when Release fails (the surface is kept) */
GLboolean __fastcall texMgrReleaseSurface(__GLcontext *gc, __GLtexMemBlockICD *blk, GLint tmu)
{
    LPDIRECTDRAWSURFACE surf;

    if (blk->hwSurface != 0) {
        if (gc->hwcx.glideContext != 0) {
            grFinish();
        }
        surf = blk->hwSurface;
        if (IDirectDrawSurface_Release(surf) == 0) {
            blk->hwSurface = 0;
        }
        else {
            return GL_FALSE;
        }
    }
    return GL_TRUE;
}

/* 6903b930 (size 350): hwCreateTexture: createTexture of the hardware path (g_device): a 0x574-byte sst texture object for
** target index 0 1D / 1 2D (the sst createLevel 6903c770) / 2, 3 (proxies: the generic createLevel), with
** maxMipMapLevel level records of 0x74 in one block */
__GLtextureFxICD *__fastcall hwCreateTexture(__GLcontext *gc, GLuint name, GLuint target)
{
    __GLtextureFxICD *t;
    __GLmipMapLevelICD *p;
    GLint n, i, k;

    t = (__GLtextureFxICD *)__GL_MALLOC(gc, sizeof(__GLtextureFxICD));
    t->t.gc = gc;
    t->t.refCount = 1;
    t->t.targetIndex = target;
    for (k = 0; k < 2; k++) {
        t->t.hwHandle[k] = 0;
        t->onTmu[k] = 0;
    }
    t->t.free = (__GLproc)__glSSTFreeTexObj;
    t->t.makeResident = (__GLproc)MakeResident;
    t->t.unmakeResident = (__GLproc)UnmakeResident;
    t->t.copyTexImage = (__GLproc)__glSSTCopyTexImage;
    t->t.readTexImage = (__GLproc)__glReadTexImage;
    t->t.name = name;
    *(GLuint *)&t->t.priority = 0x3f800000;             /* 1.0f */
    t->t.CT.internalFormat = GL_RGBA;
    t->memRequired = -1;
    t->t.params.sWrapMode = GL_REPEAT;
    t->t.params.tWrapMode = GL_REPEAT;
    t->t.params.minFilter = GL_NEAREST_MIPMAP_LINEAR;
    t->t.params.magFilter = GL_LINEAR;
    wglGlideTexKey(gc, (BYTE *)t);
    switch (target) {
      case 0:
        t->t.dim = 1;
        t->t.createLevel = (__GLproc)__glSSTTexCreateLevel;
        t->t.CT.target = GL_TEXTURE_1D;
        break;
      case 1:
        t->t.dim = 2;
        t->t.createLevel = (__GLproc)__glSSTTexCreateLevel;
        t->t.CT.target = GL_TEXTURE_2D;
        break;
      case 2:
        t->t.dim = 1;
        t->t.createLevel = (__GLproc)__glTextureCreateProxyLevel;
        break;
      case 3:
        t->t.dim = 2;
        t->t.createLevel = (__GLproc)__glTextureCreateProxyLevel;
        break;
    }
    t->t.releaseLevel = (__GLproc)__glSSTTexReleaseLevel;
    n = gc->constants.maxMipMapLevel;
    t->t.level = (__GLmipMapLevelICD **)__GL_CALLOC(gc, n, sizeof(__GLmipMapLevelICD *));
    t->t.level[0] = (__GLmipMapLevelICD *)__GL_CALLOC(gc, n, sizeof(__GLmipMapLevelICD));
    p = t->t.level[0];
    for (i = 0; i < n; ) {
        i++;
        t->t.level[i - 1] = p;
        p++;
        t->t.level[i - 1]->requestedFormat = 1;
    }
    return t;
}

/* hwTexIsSplit: the texture is drawn with split-TMU trilinear (FX_GL_TRILINEAR_ENABLED 2, fullscreen): a
** *_MIPMAP_LINEAR filter and at least 16 texels on one side (smaller ones, like particles, gain nothing and would
** pay for both TMUs), not compressed (split FXT1 chains show block noise in some levels on the VSA-100; they keep
** the nearest mip level).  Resident as onTmu 3: its odd mip levels for TMU0, then its even ones for TMU1 */
GLboolean __fastcall hwTexIsSplit(__GLcontext *gc, __GLtextureICD *tex)
{
    return gc->fx.trilinearMode == 2 && !gc->hwcx.windowed &&
           (tex->params.minFilter == GL_LINEAR_MIPMAP_LINEAR || tex->params.minFilter == GL_NEAREST_MIPMAP_LINEAR) &&
           (tex->level[0]->width >= 16 || tex->level[0]->height >= 16) &&
           tex->level[0]->compressedImage == NULL;
}

/* hwTexDownloadLevel: download rows t0..t1 of a level into the texture's TMU memory; a split texture (onTmu 3) gets
** them in its odd part for TMU0 or its even part for TMU1 (Glide skips the level in the other part's call) */
void __fastcall hwTexDownloadLevel(__GLcontext *gc, __GLtextureFxICD *tex, GLuint tmu, GLint level, GLint fmt,
                                   void *data, GLint t0, GLint t1)
{
    GLuint start = TEX_SLOT(tex->mem, tmu)->start;
    GrTexInfo *info;

    if (TEX_SLOT(tex->onTmu, tmu) == 3) {
        info = (GrTexInfo *)&tex->smallLodLog2;
        grTexDownloadMipMapLevelPartial(GR_TMU0, start, tex->t.maxLevel - level, tex->t.maxLevel,
                                        tex->aspectRatioLog2, fmt, GR_MIPMAPLEVELMASK_ODD, data, t0, t1);
        grTexDownloadMipMapLevelPartial(GR_TMU1, start + grTexTextureMemRequired(GR_MIPMAPLEVELMASK_ODD, info),
                                        tex->t.maxLevel - level, tex->t.maxLevel, tex->aspectRatioLog2, fmt,
                                        GR_MIPMAPLEVELMASK_EVEN, data, t0, t1);
        return;
    }
    hwTexDownloadRows(gc->fx.tmuTexUnit[tmu], start, level, tex->t.maxLevel, tex->aspectRatioLog2, fmt, data, t0, t1);
}

/* 6903bb00 (size 547) -- MakeResident: tex->makeResident of the sst texture object: make it resident on a TMU. 
** Refuse a border / a non-default border colour unless clamped to edge, and palettes the hardware cannot take; reuse
** a download whose layout fits (onTmu 1 mipmapped, 2 base level only, 3 split trilinear), else download
** (MakeResidentRetry) and load (DownloadTexture) */
GLuint __fastcall MakeResident(__GLcontext *gc, __GLtextureFxICD *tex, GLuint a, GLuint tmu)
{
    __GLmipMapLevelICD *lvl0 = tex->t.level[0];
    GLint fmt, state, want;
    GLuint ok;

    if ((lvl0->border > 0 ||
         tex->t.params.borderColor.r != g_defaultBorderR || tex->t.params.borderColor.g != g_defaultBorderG ||
         tex->t.params.borderColor.b != g_defaultBorderB || tex->t.params.borderColor.a != g_defaultBorderA) &&
        (tex->t.params.sWrapMode != GL_CLAMP_TO_EDGE || tex->t.params.tWrapMode != GL_CLAMP_TO_EDGE)) {
        TEX_SLOT(tex->t.hwHandle, tmu) = 0;
        return 0;
    }
    if (lvl0->baseFormat == GL_COLOR_INDEX) {
        if (gc->state.enables.general & __GL_SHARED_TEXTURE_PALETTE_EXT_ENABLE)
            fmt = gc->hwcx.sharedPalette.baseFormat;
        else
            fmt = tex->t.CT.baseFormat;
        if (fmt != GL_RGB && (fmt != GL_RGBA || !gc->hwcx.palette6666)) {
            TEX_SLOT(tex->t.hwHandle, tmu) = 0;
            return 0;
        }
    }
    state = TEX_SLOT(tex->onTmu, tmu);
    if (state != 0) {
        if (hwTexIsSplit(gc, &tex->t))
            want = 3;
        else if (tex->t.params.minFilter != GL_NEAREST && tex->t.params.minFilter != GL_LINEAR)
            want = 1;
        else
            want = 2;
        if (state == want) {
            if (gc->hwcx.windowed)
                grSurfaceSetTextureSurfaceExt(gc->fx.tmuTexUnit[tmu], TEX_SLOT(tex->mem, tmu)->hwSurface);
            TEX_SLOT(tex->t.hwHandle, tmu) = 1;
            return 1;
        }
    }
    ok = MakeResidentRetry(gc, tex, a, tmu);
    if (!ok) {
        TEX_SLOT(tex->t.hwHandle, tmu) = 0;
        return 0;
    }
    DownloadTexture(gc, tex, tmu);
    TEX_SLOT(tex->t.hwHandle, tmu) = 1;
    return 1;
}

/* 6903bd30 (size 321) -- DownloadTexture: download the texture's levels (all when mipmapped resident, else level 0)
** into its TMU memory block (a split one in its two parts, hwTexDownloadLevel); FXT1 levels go as their compressed
** image in whole 4-row blocks, or, replicated for a wide aspect, as the uncompressed buffer in GR_TEXFMT_ARGB_8888 */
void __fastcall DownloadTexture(__GLcontext *gc, __GLtextureFxICD *tex, GLuint tmu)
{
    GLint h, maxLevel, w;
    GLint slot, level, fmt;
    __GLmipMapLevelICD *lvl;
    GLubyte *data;
    const __GLtextureFormatICD *info;

    slot = __GL_TMU_SLOT(tmu);
    if (tex->onTmu[slot] != 2)
        maxLevel = tex->t.maxLevel;
    else
        maxLevel = 0;
    if (gc->hwcx.windowed)
        grSurfaceSetTextureSurfaceExt(gc->fx.tmuTexUnit[tmu], tex->mem[slot]->hwSurface);
    for (level = 0; level <= maxLevel; level++) {
        lvl = tex->t.level[level];
        info = lvl->texFormat;
        if (info == NULL)
            continue;
        if (!IS_FXT1_FORMAT(info)) {
            if (tex->wideAspect) {
                data = ReplicateAspectTexels(gc, lvl, lvl->buffer, &w, &h);
            } else {
                data = lvl->buffer;
                h = lvl->height;
            }
            fmt = info->grFormat;
        } else if (tex->wideAspect) {
            data = ReplicateAspectTexels(gc, lvl, lvl->buffer, &w, &h);
            fmt = 0x12;                                 /* GR_TEXFMT_ARGB_8888 (g3ext.h): the uncompressed copy */
        } else {
            data = lvl->compressedImage;        /* the FXT1 image (6903c770 allocates it) */
            h = (lvl->height + 3) & ~3;                 /* whole 4-row FXT1 blocks */
            fmt = info->grFormat;
        }
        hwTexDownloadLevel(gc, tex, tmu, level, fmt, data, 0, h - 1);
    }
}

/* 6903be80 (size 366) -- ReplicateAspectTexels: Glide's aspect ratio is limited to 8:1: replicate the texels of a
** narrower level (columns or rows, 2^(|log2 w - log2 h| - 3) times) into the scratch buffer; returns the data to
** download and its width / height */
GLubyte *__fastcall ReplicateAspectTexels(__GLcontext *gc, __GLmipMapLevelICD *lvl, GLubyte *data, GLint *w, GLint *h)
{
    GLint d, ad, rowRep, colRep, x, y, r, c, b;
    GLuint size;
    GLubyte *dst, *src;

    d = lvl->widthLog2 - lvl->heightLog2;
    ad = abs(d);
    if (ad > 3) {
        GLint bpp;

        bpp = (lvl->texFormat->bitsPerTexel + 7) >> 3;
        size = (1 << (ad - 3)) * lvl->imageSize * bpp;
        if (size > gc->hwcx.scratchSize) {
            gc->hwcx.scratchSize = size;
            gc->hwcx.scratch = (GLubyte *)__GL_REALLOC(gc, gc->hwcx.scratch, size);
        }
        if (d > 0) {
            colRep = 1;
            rowRep = 1 << (d - 3);
        } else {
            colRep = 1 << (-3 - d);
            rowRep = 1;
        }
        dst = gc->hwcx.scratch;
        /* the data parameter is the running source pointer (its stack slot is then reused) */
        for (y = 0; y < lvl->height; y++) {
            src = data;
            for (r = 0; r < rowRep; r++) {
                data = src;
                for (x = 0; x < lvl->width; x++) {
                    for (c = 0; c < colRep; c++)
                        for (b = 0; b < bpp; b++)
                            *dst++ = data[b];
                    data += bpp;
                }
            }
        }
        *w = lvl->width * colRep;
        *h = lvl->height * rowRep;
        return gc->hwcx.scratch;
    }
    *w = lvl->width;
    *h = lvl->height;
    return data;
}

/* 6903c14a (size 476) -- MakeResidentTryAllocateSurface (ssttexmgr.c, from its error message): ask Glide for the
** surface size of a texture, build a DDSURFACEDESC and create it with IDirectDraw2::CreateSurface into the texture's
** TMU block (mem[idx]->hwSurface; slot 0 when g_tmuSharedMemory) */
GLboolean __fastcall MakeResidentTryAllocateSurface(__GLcontext *gc, __GLtextureFxICD *tex, GLint idx)
{
    DDSURFACEDESC desc;
    __GLtexMgrFxICD *texMgr = (__GLtexMgrFxICD *)gc->texMgr;
    __GLmipMapLevelICD *lvl = tex->t.level[0];
    GLuint w = lvl->width;
    GLuint h = lvl->height;
    GLenum filter = tex->t.params.minFilter;
    LPDIRECTDRAW dd;
    HRESULT hr;

    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(DDSURFACEDESC);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwWidth = w;
    desc.dwHeight = h;
    grSurfaceCalcTextureWHDExt((GrTexInfo *)&tex->smallLodLog2, &desc.dwWidth, &desc.dwHeight,
                               &desc.ddpfPixelFormat.dwRGBBitCount);
    switch (filter) {                                   /* ddsCaps: the same for every min filter (the */
      case GL_NEAREST:                                  /* load of tex->minFilter stays in the original) */
      case GL_LINEAR:
        desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
        break;
      default:
        desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
        break;
    }
    /* bytes -> bits per texel for an old Glide.  The original tests `(glideVerFlags == 0) & ~2` (probably
    ** a misplaced `!(glideVerFlags & ~2)`: the same while glideVerFlags is 0, 1 or 3) */
    if ((gc->hwcx.glideVerFlags == 0) & 0xfffffffd)
        desc.ddpfPixelFormat.dwRGBBitCount *= 8;
    desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    if (gc->hwcx.hasTexFmt32) {
        if (desc.ddpfPixelFormat.dwRGBBitCount > 32) {
            DBG("grSurfaceCalcTextureWHD returned currently unsupported texture bpt(%ld).",
                    desc.ddpfPixelFormat.dwRGBBitCount);
            return GL_FALSE;
        }
    } else if (desc.ddpfPixelFormat.dwRGBBitCount > 16) {
        DBG("grSurfaceCalcTextureWHD returned currently unsupported texture bpt(%ld).",
                desc.ddpfPixelFormat.dwRGBBitCount);
        return GL_FALSE;
    }
    if (desc.ddpfPixelFormat.dwRGBBitCount == 32) {
        desc.ddpfPixelFormat.dwRBitMask = 0xff0000;
        desc.ddpfPixelFormat.dwGBitMask = 0xff00;
        desc.ddpfPixelFormat.dwBBitMask = 0xff;
    } else if (desc.ddpfPixelFormat.dwRGBBitCount == 16) {
        desc.ddpfPixelFormat.dwRBitMask = 0xf800;
        desc.ddpfPixelFormat.dwGBitMask = 0x7e0;
        desc.ddpfPixelFormat.dwBBitMask = 0x1f;
    } else if (desc.ddpfPixelFormat.dwRGBBitCount == 8) {
        desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_PALETTEINDEXED8;
        desc.ddpfPixelFormat.dwRBitMask = 0;
        desc.ddpfPixelFormat.dwGBitMask = 0;
        desc.ddpfPixelFormat.dwBBitMask = 0;
    } else if (desc.ddpfPixelFormat.dwRGBBitCount == 4) {     /* compressed: a RAW8 FOURCC surface */
        desc.dwWidth >>= 2;
        desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
        desc.ddpfPixelFormat.dwFourCC = MAKEFOURCC('R', 'A', 'W', '8');
        desc.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
        desc.ddpfPixelFormat.dwRBitMask = 0;
        desc.ddpfPixelFormat.dwGBitMask = 0;
        desc.ddpfPixelFormat.dwBBitMask = 0;
    }
    dd = texMgr->ddraw;                                 /* used as IDirectDraw2: CreateSurface is slot 6 in both */
    hr = IDirectDraw_CreateSurface(dd, &desc, &tex->mem[__GL_TMU_SLOT(idx)]->hwSurface, NULL);
    if (hr == DD_OK)
        return GL_TRUE;
    wglMessageInt("ssttexmgr.c/MakeResidentTryAllocateSurface: IDirectDraw2_CreateSurface returned: ", hr & 0xffff);
    return GL_FALSE;
}

/* 6903c340 (size 596) -- __glSSTAllocateTextureMemory: allocate TMU memory for a texture (all levels when
** mipmapped, a split trilinear one as its odd levels then its even ones, else the base level): reuse its block when the size still fits, else take blocks from the rover (or the
** heap start), evicting their textures and merging them; in windowed mode a surface slot only */
GLint __fastcall __glSSTAllocateTextureMemory(__GLcontext *gc, __GLtextureFxICD *tex, GLuint unused, GLuint tmu)
{
    __GLtexMgrFxICD *m = (__GLtexMgrFxICD *)gc->texMgr;
    __GLtexMemBlockICD *blk, *b;
    GLint state, size, left;

    if (tex->t.params.minFilter != GL_NEAREST && tex->t.params.minFilter != GL_LINEAR) {
        state = hwTexIsSplit(gc, &tex->t) ? 3 : 1;
        tex->smallLodLog2 = 0;
    } else {
        state = 2;
        tex->smallLodLog2 = tex->t.maxLevel;
    }
    if (gc->hwcx.windowed) {
        blk = TEX_SLOT(tex->mem, tmu);
        if (blk == NULL) {
            blk = m->heap[__GL_TMU_SLOT(tmu)];
            while (blk->hwSurface != 0 && blk->next != NULL)
                blk = blk->next;
            blk->start = 0;
            if (blk->hwSurface != 0)
                blk = texMgrSplit(gc, blk, 0);
        }
    } else {
        if (state == 3)
            size = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_ODD, (GrTexInfo *)&tex->smallLodLog2) +
                   grTexTextureMemRequired(GR_MIPMAPLEVELMASK_EVEN, (GrTexInfo *)&tex->smallLodLog2);
        else
            size = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, (GrTexInfo *)&tex->smallLodLog2);
        tex->memRequired = size;
        b = TEX_SLOT(tex->mem, tmu);
        if (b != NULL) {
            if ((GLint)b->size == size) {
                TEX_SLOT(tex->onTmu, tmu) = state;      /* the layout may change at the same size (split) */
                return 1;
            }
            __glSSTFreeTextureMemory(gc, tex, tmu);
        }
        blk = m->rover[__GL_TMU_SLOT(tmu)]->next;
        if (blk == NULL || (GLint)(m->end - blk->start) < tex->memRequired)
            blk = m->heap[__GL_TMU_SLOT(tmu)];
        left = tex->memRequired;
        for (b = blk; b != NULL; b = blk->next) {
            if (left <= 0)
                goto done;
            if (b->tex) {
                b->tex->mem[__GL_TMU_SLOT(tmu)] = NULL;
                b->tex->onTmu[__GL_TMU_SLOT(tmu)] = 0;
                b->tex->t.hwHandle[__GL_TMU_SLOT(tmu)] = 0;
            }
            if ((GLint)b->size > left)
                texMgrSplit(gc, b, left);
            left -= b->size;
            if (b != blk)
                texMgrCoalesce(gc, blk, tmu);
        }
        if (left > 0)
            return 0;
    }
done:
    blk->tex = tex;
    TEX_SLOT(tex->mem, tmu) = blk;
    m->rover[__GL_TMU_SLOT(tmu)] = blk;
    TEX_SLOT(tex->onTmu, tmu) = state;
    return gc->hwcx.windowed == 0;
}

/* 6903c6d0 (size 139) -- texMgrFreeNode: free a texture's TMU block and merge it with free neighbours */
void __fastcall texMgrFreeNode(__GLcontext *gc, __GLtexMemBlockICD *blk, GLuint tmu)
{
    __GLtexMemBlockICD *b, *prev = NULL;

    for (b = ((__GLtexMgrFxICD *)gc->texMgr)->heap[__GL_TMU_SLOT(tmu)]; b != NULL && b != blk; b = b->next)
        prev = b;
    if (b != NULL) {
        if (gc->hwcx.windowed)
            texMgrReleaseSurface(gc, b, tmu);
        b->tex = NULL;
        if (b->next != NULL && b->next->tex == NULL)
            texMgrCoalesce(gc, b, tmu);
        if (prev != NULL && prev->tex == NULL)
            texMgrCoalesce(gc, prev, tmu);
    }
}

/* 6903c770 (size 513) -- __glSSTTexCreateLevel: tex->createLevel of the sst texture object: the generic level
** set-up, a buffer for the compressed formats, and for level 0 the Glide texture info (format, data, LOD, aspect
** clamped to 8:1 with the replication flag wideAspect) and the s / t scales of that aspect */
void *__fastcall __glSSTTexCreateLevel(__GLcontext *gc, __GLtextureFxICD *tex, GLint level, GLenum ifmt, GLint w, GLint h,
                            GLint a5, GLint a6, GLint a7)
{
    void *r;
    const __GLtextureFormatICD *fi;
    __GLmipMapLevelICD **lv;
    GLint d, s, t;

    r = __glTextureCreateLevel(gc, &tex->t, level, ifmt, w, h, a5, a6, a7);
    if (r != NULL) {
        if (ifmt == 0x86b0 || ifmt == 0x86b1 || (ifmt >= 0x83a0 && ifmt <= 0x83a3)) {   /* FXT1 RGB / RGBA; the four S3TC formats are stored as FXT1 */
            tex->t.level[level]->compressedSize = txTexCalcMapSize(w, h, 0x11);
            tex->t.level[level]->compressedImage = __GL_MALLOC(gc, tex->t.level[level]->compressedSize);
            if (tex->t.level[level]->compressedImage == NULL)
                return 0;
        } else
            tex->t.level[level]->compressedImage = NULL;
        if (level != 0)
            return r;
        {
            lv = tex->t.level;
            fi = lv[0]->texFormat;
            tex->format = fi->grFormat;
            tex->data = lv[0]->buffer;
            tex->largeLodLog2 = tex->t.maxLevel;
            d = lv[0]->widthLog2 - lv[0]->heightLog2;
            if (abs(d) > 3) {
                tex->wideAspect = 1;
                if (IS_FXT1_FORMAT(fi))
                    tex->format = 0x12;                 /* GR_TEXFMT_ARGB_8888 (g3ext.h): replicated uncompressed */
            }
            if (d > 3)
                d = 3;
            else if (d < -3)
                d = -3;
            else
                tex->wideAspect = 0;
            tex->aspectRatioLog2 = d;
            if (d >= 0) {
                s = 0x100 >> d;
                t = 1 << d;
                tex->texScale[0] = 256.0f;
                tex->texScaleInv[0] = 0.00390625f;
                tex->texScale[1] = (GLfloat)s;
                tex->texScaleInv[1] = (GLfloat)t * 0.00390625f;
            } else {
                d = -d;
                s = 0x100 >> d;
                t = 1 << d;
                tex->texScale[1] = 256.0f;
                tex->texScaleInv[1] = 0.00390625f;
                tex->texScale[0] = (GLfloat)s;
                tex->texScaleInv[0] = (GLfloat)t * 0.00390625f;
            }
            tex->texScaleExp[0] = *(GLint *)&tex->texScale[0] - 0x3f800000;
            tex->texScaleExp[1] = *(GLint *)&tex->texScale[1] - 0x3f800000;
        }
    }
    return r;
}

/* 6903bff0 -- MakeResidentRetry: make a texture resident (6903c340), and when rendering to a Glide surface check it
** (6903c14a); on failure reset the texture manager and retry once */
GLuint __fastcall MakeResidentRetry(__GLcontext *gc, __GLtextureFxICD *tex, GLuint unused, GLuint tmu)
{
    GLboolean ok;
    GLuint result;

    result = __glSSTAllocateTextureMemory(gc, tex, unused, tmu);
    if (gc->hwcx.windowed != 0) {
        ok = MakeResidentTryAllocateSurface(gc, tex, tmu);
        if (!ok) {
            texMgrReset(gc, (__GLtexMgrFxICD *)gc->texMgr);
            __glSSTAllocateTextureMemory(gc, tex, unused, tmu);
            ok = MakeResidentTryAllocateSurface(gc, tex, tmu);
        }
        result = (ok != 0);
    }
    return result;
}

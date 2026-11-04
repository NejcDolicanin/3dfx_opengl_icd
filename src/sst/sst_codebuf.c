/*
** sst_codebuf.c -- 3dfx back end, original VA range 69024000..69028000: the quad-as-two-triangles draw, the
** hardware glEnd, the keyed code-page cache (list, free), the code buffers and the Glide vertex layout /
** dirty-state setup.
** Then the rest of the keyed page cache (init, insert, lookup and their comparators), in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "icdglob.h"
#include "icdprotos.h"
#include "og.h"
#include "glshade.h"

/* ---- forward declarations ---- */
void __fastcall __glSSTClipAndDrawQuad(__GLcontext *gc, GLuint edxUnused, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c, __GLvertexICD *d);
void __fastcall hwPageCacheMoveToTail(__GLhwBufRecICD *t, __GLhwBufEntryICD *e);

/* 69026600 -- hwInitImmProcBuilder: install the immediate-mode proc builder */
void __fastcall hwInitImmProcBuilder(__GLcontext *gc)
{
    gc->hwcx.buildImmProc = hwBuildImmProc;
}

/* 69026e30 -- __grDrawQuad: draw a quad as the triangles (a, b, c) and (a, c, d) (the generated code's draw proc) */
void __fastcall __grDrawQuad(GLuint ecxUnused, GLuint edxUnused, void *a, void *b, void *c, void *d)
{
    grDrawTriangle(a, b, c);
    grDrawTriangle(a, c, d);
}

/* 69026e60 (size 51) -- __glSSTClipAndDrawQuad: clips a quad (a, b, c, d on the stack) as the triangles (a, b, c)
** and (a, c, d) through 69026df0 */
void __fastcall __glSSTClipAndDrawQuad(__GLcontext *gc, GLuint edxUnused, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c, __GLvertexICD *d)
{
    __glSSTClipAndDrawTriangle(gc, edxUnused, a, b, c);
    __glSSTClipAndDrawTriangle(gc, edxUnused, a, c, d);
}

/* 69027060 -- __glsstim_End: glEnd of the hardware path: the "no primitive" descriptor, procs.endPrim, the texture /
** combine state marked dirty while flag_355be; GL_INVALID_OPERATION outside glBegin */
void __fastcall __glsstim_End(void)
{
    __GLcontext *gc;

    gc = __glCurrentGC();
    if (gc->beginMode != __GL_NOT_IN_BEGIN && gc->beginMode != __GL_NEED_VALIDATE) {
        gc->hwcx.hwPrimDesc = (GLuint)&g_hwPrimDescNone;
        (*gc->procs.endPrim)(gc);
        if (gc->hwcx.flag_355be) {
            hwDirtyTexCombine(gc);
        }
        gc->hwcx.hwPrim = -1;
        gc->beginMode = __GL_NOT_IN_BEGIN;
        return;
    }
    __glSetError(GL_INVALID_OPERATION);
}

/* 690273d0 -- hwPageCacheFree: free a code-page cache's allocations */
void __fastcall hwPageCacheFree(__GLcontext *gc, __GLhwBufRecICD *t)
{
    __GL_FREE(gc, t->entries);
    __GL_FREE(gc, t->sorted);
    __GL_FREE(gc, t->arenaA);
    __GL_FREE(gc, t->arenaB);
    __GL_FREE(gc, t->data);
}

/* 69027520 -- hwPageCacheMoveToTail: unlink e and append it at the tail (the most recently used end) */
void __fastcall hwPageCacheMoveToTail(__GLhwBufRecICD *t, __GLhwBufEntryICD *e)
{
    if (e->next) {
        e->next->prev = e->prev;
    }
    if (e->prev) {
        e->prev->next = e->next;
    }
    e->prev = t->tail;
    e->next = 0;
    if (t->tail) {
        t->tail->next = e;
    }
    t->tail = e;
}

/* 69027560 -- hwPageCacheTouch: a lookup hit: make the entry the most recently used */
void __fastcall hwPageCacheTouch(__GLhwBufRecICD *t, __GLhwBufEntryICD **pe)
{
    __GLhwBufEntryICD *e;

    e = *pe;
    if (e == t->head && e->next) {
        t->head = e->next;
    }
    if (e != t->tail) {
        hwPageCacheMoveToTail(t, e);
    }
}

/* 69027630 -- hwAllocCodeBuffers: allocate the generated-code buffers: two 4 KB pages (the first 4 KB aligned) and
** one 4 KB block (32-byte aligned) */
void __fastcall hwAllocCodeBuffers(__GLcontext *gc)
{
    GLuint p;

    p = (GLuint)__GL_CALLOC(gc, 0x1000, 2);
    gc->hwcx.codeBufRaw = (void *)p;
    gc->hwcx.codeBufPage = p + ((0 - p) & 0xfff);     /* rounded up to 4 KB */
    p = (GLuint)__GL_CALLOC(gc, 0x1000, 1);
    gc->hwcx.codeBuf2Raw = (void *)p;
    gc->hwcx.codeBuf2 = (p + 0x1f) & ~0x1f;
}

/* 690276a0 -- hwSetupVertexLayout: the Glide vertex layout of __GLvertexICD's hardware part (x, y, z, q, packed
** ARGB, s / t and q per TMU; with two TMUs GL unit 0 feeds TMU1 and unit 1 TMU0), and the TMU count */
void __fastcall hwSetupVertexLayout(__GLcontext *gc)
{
    GLint n;

    grReset(GR_VERTEX_PARAMETER);
    grVertexLayout(GR_PARAM_XY, 0x00, GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_PARGB, 0x14, GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_Z, 0x08, GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_Q, 0x0c, GR_PARAM_ENABLE);
    grVertexLayout(GR_PARAM_ST0, 0x20, GR_PARAM_ENABLE);
    if (gc->hwcx.nonUnityQ) {
        grVertexLayout(GR_PARAM_Q0, 0x2c, GR_PARAM_ENABLE);
    }
    n = grGetInteger(GR_NUM_TMU);
    gc->fx.numTMUs = n;
    gc->constants.maxTextureUnits = n;
    if (n == 2) {
        gc->fx.tmuTexUnit[0] = 1;
        gc->fx.tmuTexUnit[1] = 0;
        grVertexLayout(GR_PARAM_ST0, 0x30, GR_PARAM_ENABLE);
        if (gc->hwcx.nonUnityQ) {
            grVertexLayout(GR_PARAM_Q0, 0x3c, GR_PARAM_ENABLE);
        }
        grVertexLayout(GR_PARAM_ST1, 0x20, GR_PARAM_ENABLE);
        if (gc->hwcx.nonUnityQ) {
            grVertexLayout(GR_PARAM_Q1, 0x2c, GR_PARAM_ENABLE);
        }
    } else {
        gc->fx.tmuTexUnit[0] = 0;
    }
}

/* 69027780 -- hwDirtyAllGrState: mark every Glide state record dirty, without the ones this board cannot take (TMU1
** with one TMU, the combine extension records, the 32 bpp records) */
void __fastcall hwDirtyAllGrState(__GLcontext *gc)
{
    gc->hwcx.grDirty = 0x7ffffff;
    if (gc->hwcx.hasCombineExt) {
        gc->hwcx.grDirtyExt = 0xff;
    }
    if (gc->hwcx.numTMU == 1) {
        gc->hwcx.grDirty = 0x7fff0ff;
    }
    if (!gc->hwcx.hasCombineExt) {
        gc->hwcx.grDirtyExt &= 0xffffff00;
    } else {
        gc->hwcx.grDirty &= 0xffffc77f;
    }
    if (!gc->hwcx.has32bpp) {
        gc->hwcx.grDirty &= 0xff8fffff;
    }
}

extern void __fastcall hwPageCacheMoveToTail(__GLhwBufRecICD *t, __GLhwBufEntryICD *e);
/* 690272b0 (size 274) -- hwPageCacheInit: build a page cache of n entries: keys of keySize bytes in a data arena, two
** 4 KB-page arenas aligned like addrA / addrB, the LRU list empty and the lookup proc hwPageCacheLookup
** (sizes as in the original: the sorted pointer array is allocated with 0x18-byte elements) */
int __fastcall hwPageCacheInit(__GLcontext *gc, GLuint edx_unused, __GLhwBufRecICD *t, GLint n, GLuint keySize,
                            GLuint addrA, GLuint addrB)
{
    GLuint arenaA, arenaB, data;
    GLint i, pages;

    t->addrA = addrA;
    t->addrB = addrB;
    t->keySize = keySize;
    t->count = 0;
    t->max = n;
    t->entries = (__GLhwBufEntryICD *)__GL_CALLOC(gc, 0x18, n);
    t->sorted = (__GLhwBufEntryICD **)__GL_CALLOC(gc, 0x18, n);
    pages = n + 1;
    arenaA = (GLuint)__GL_CALLOC(gc, 0x1000, pages);
    t->arenaA = (void *)arenaA;
    arenaB = (GLuint)__GL_CALLOC(gc, 0x1000, pages);
    t->arenaB = (void *)arenaB;
    data = (GLuint)__GL_CALLOC(gc, keySize, n);
    t->data = (void *)data;
    arenaA += (addrA - arenaA) & 0xfff;
    arenaB += (addrB - arenaB) & 0xfff;
    for (i = 0; i < n; i++) {
        t->entries[i].owner = t;
        t->entries[i].valA = arenaA + i * 0x1000;
        t->entries[i].valB = arenaB + i * 0x1000;
        t->entries[i].key = (void *)(data + i * keySize);
        t->sorted[i] = &t->entries[i];
    }
    t->alloc = (__GLproc)hwPageCacheLookup;
    t->tail = 0;
    t->head = 0;
    return 0;
}

extern int __cdecl hwPageCacheCompareEntries(const void *pa, const void *pb);
/* 69027400 (size 240) -- hwPageCacheInsert: insert a key (a free entry, else the least recently used one), keep the
** pointer array sorted; returns the entry's page in arena A (the code) and, through outBlock (a generator's
** block pointer), its page in arena B (the data) */
GLubyte *__fastcall hwPageCacheInsert(__GLcontext *gc, __GLhwBufRecICD *t, const void *key, void *outBlock)
{
    __GLhwBufEntryICD *e;
    GLint i;

    if (t->count < t->max) {
        i = t->count;
        t->count = i + 1;
        e = &t->entries[i];
        e->prev = t->tail;
        e->next = 0;
        if (t->tail != 0)
            t->tail->next = e;
        t->tail = e;
        if (t->head == 0)
            t->head = e;
    } else {
        e = t->head;
        i = e - t->entries;
        t->head = e->next;
        hwPageCacheMoveToTail(t, e);
    }
    memcpy(t->entries[i].key, key, t->keySize);
    t->sorted[i] = &t->entries[i];
    qsort(t->sorted, t->count, 4, hwPageCacheCompareEntries);
    if (outBlock != 0)
        *(GLuint *)outBlock = t->entries[i].valB;
    return (GLubyte *)t->entries[i].valA;
}

/* 690274f0 (size 38) -- hwPageCacheCompareEntries: qsort comparator of two entries by key bytes */
int __cdecl hwPageCacheCompareEntries(const void *pa, const void *pb)
{
    __GLhwBufEntryICD *a = *(__GLhwBufEntryICD **)pa;
    __GLhwBufEntryICD *b = *(__GLhwBufEntryICD **)pb;

    return memcmp(a->key, b->key, a->owner->keySize);
}

extern int __cdecl hwPageCacheCompareKey(const void *key, const void *pb);
/* 69027580 (size 125) -- hwPageCacheLookup: look a key up: the most recent entry first, else bsearch (and make it
** the most recent); returns its page in arena A (and B through outB), 0 when absent */
GLuint __fastcall hwPageCacheLookup(void *self, __GLhwBufRecICD *t, const void *key, GLuint *outB)
{
    __GLhwBufEntryICD **pe;

    if (t->tail != 0 && memcmp(key, t->tail->key, t->keySize) == 0) {
        if (outB != 0)
            *outB = t->tail->valB;
        return t->tail->valA;
    }
    pe = (__GLhwBufEntryICD **)bsearch(key, t->sorted, t->count, 4, hwPageCacheCompareKey);
    if (pe == 0)
        return 0;
    hwPageCacheTouch(t, pe);
    if (outB != 0)
        *outB = (*pe)->valB;
    return (*pe)->valA;
}

/* 69027600 (size 34) -- hwPageCacheCompareKey: bsearch comparator of a key against an entry */
int __cdecl hwPageCacheCompareKey(const void *key, const void *pb)
{
    __GLhwBufEntryICD *b = *(__GLhwBufEntryICD **)pb;

    return memcmp(key, b->key, b->owner->keySize);
}

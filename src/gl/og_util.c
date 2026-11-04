/*
** og_util.c -- original VA range 690e8000..690ee000: the vertex-program code generator's (og_*)
** operand / instruction predicates, the pending instruction list, the generated-code caches
** (init, release, invalidate) and the reset of the lazily built pixel-format table.
** Then og_optimizeBlock and og_canV (the scheduler driver; its rules are og_sched.c), og_fpk and the
** code-cache set-up, in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "og.h"
#include "glsitypes.h"                      /* __GLcodeCell */
#include "icdglob.h"
#include <glide.h>
#include "glsi.h"
#include "si_names.h"
#include "glshade.h"
#include "icdprotos.h"

/* 693c3cb8, 693c3c98 -- the scheduler's pending list; 690f5dc4, 693c3cc0 -- the code cache switch */
struct OgInsnRec *g_ogPendingList;
struct OgInsnRec *g_ogPendingTail;
GLint g_ogCacheReadEnv = 1;
char g_ogCacheDisabled;

/* og operands, memory operands and instruction records: og.h */

extern void __fastcall __glInitOneCodeCache(__GLcontext *gc, __GLogCacheICD *cache, GLuint n, GLuint size);

/* g_ogPendingList / g_ogPendingTail (the scheduler's pending instruction list: og_util.c og_optimizeBlock
** schedules and empties it) and g_pfdListStale / g_pfdList (gl_pixfmt.c __glDescribePixelFormat builds the list):
** icdglob.h */

/* ---- forward declarations ---- */
int __fastcall og_isMem(OgOperand *o);
void __fastcall __glReleaseOneCodeCache(__GLcontext *gc, __GLogCacheICD *cache);
void __fastcall __glInvalidateOneCodeCache(__GLcontext *gc, __GLogCacheICD *cache);

/* 690e8a20 -- og_isReg32: is the operand a 32-bit integer register (OG_REG32)? */
int __fastcall og_isReg32(OgOperand *o)
{
    return o->kind == OG_REG32;
}

/* 690e8a30 -- og_isReg8: is the operand an 8-bit integer register (OG_REG8)? */
int __fastcall og_isReg8(OgOperand *o)
{
    return o->kind == OG_REG8;
}

/* 690e8a40 -- og_isMmx: is the operand an MMX / SSE register (OG_MMX)? */
int __fastcall og_isMmx(OgOperand *o)
{
    return o->kind == OG_MMX;
}

/* 690e8a50 -- og_isImm: is the operand an immediate (OG_IMM)? */
int __fastcall og_isImm(OgOperand *o)
{
    return o->kind == OG_IMM;
}

/* 690e8d80 -- og_isReg: an 8- or 32-bit integer register */
GLuint __fastcall og_isReg(OgOperand *o)
{
    if (og_isReg8(o) || og_isReg32(o)) {
        return 1;
    }
    return 0;
}

/* 690e8fe0 -- og_spoolCode (3dfx OG.C spool_code): append insn to the pending instruction list (ctx unused) */
void __fastcall og_spoolCode(void *ctx, OgInsn *insn)
{
    if (g_ogPendingList == NULL) {
        g_ogPendingList = insn;
        g_ogPendingTail = insn;
        insn->next = NULL;
        return;
    }
    g_ogPendingTail->next = insn;
    g_ogPendingTail = insn;
    insn->next = NULL;
}

/* 690e91f0 -- og_isMem: is the operand a memory operand (OG_MEM, [base + index * scale + disp])? */
int __fastcall og_isMem(OgOperand *o)
{
    return o->kind == OG_MEM;
}

/* 690e9200 -- og_isMmxOp (OG.C is_mmxop): is the instruction of op class 0x56..0x84 (MOVD..PXOR; the scheduler
** pairs these in the V pipe) */
GLuint __fastcall og_isMmxOp(OgInsn *insn)
{
    if (insn->op >= 0x56 && insn->op <= 0x84) {
        return 1;
    }
    return 0;
}

/* 690e9300 -- og_isBinop (OG.C is_binop): does the instruction write its first operand: MMX ops 0x56..0x84 and
** any two-operand op */
GLuint __fastcall og_isBinop(OgInsn *insn)
{
    if (insn->op >= 0x56 && insn->op <= 0x84) {
        return 1;
    }
    if (insn->a != NULL && insn->b != NULL) {
        return 1;
    }
    return 0;
}

/* 690e9490 -- og_memUse (OG.C mem_use): the registers a memory operand's address reads (base | index); 0 for
** other operands */
GLuint __fastcall og_memUse(OgMem *o)
{
    if (o != NULL && og_isMem((OgOperand *)o)) {
        return og_whatUse(o->base) | og_whatUse(o->index);
    }
    return 0;
}

/* 690e95a0 -- og_canPrecede (OG.C can_precede): are a and b independent (neither reads a register the other
** writes); 1 if either is NULL */
int __fastcall og_canPrecede(OgInsn *a, OgInsn *b)
{
    if (a != NULL && b != NULL) {
        if (og_dependsOn(a, b)) {
            return 0;
        }
        return !og_dependsOn(b, a);
    }
    return 1;
}

/* 690e9650 -- og_isMmxShiftOp (OG.C is_mmx_shiftop): is the instruction of op class 0x5b..0x69 (PSLLD..; two
** of these never pair, og_canPair) */
GLuint __fastcall og_isMmxShiftOp(OgInsn *insn)
{
    if (insn->op >= 0x5b && insn->op <= 0x69) {
        return 1;
    }
    return 0;
}

/* 690e9700 -- og_stream (3dfx OG.C, SOURCE_TRAIL off: empty): the wgldev.c context teardown calls it with (gc,
** NULL) */
void __fastcall og_stream()
{
}

/* 690e9890 -- __glInitializeCodeCache (3dfx OGCACHE.H): initialise the five generated-code caches and the
** scratch buffer from the constants (gc is the allocator: imports[0] = malloc).  Cache 3 takes
** ogCacheCount3 / Bytes3, which the constants keep after ogScratchBytes.
*/
void __fastcall __glInitializeCodeCache(__GLcontext *gc)
{
    __glInitOneCodeCache(gc, &gc->ogCache[0], gc->constants.ogCacheCount0, gc->constants.ogCacheBytes0);
    __glInitOneCodeCache(gc, &gc->ogCache[1], gc->constants.ogCacheCount1, gc->constants.ogCacheBytes1);
    __glInitOneCodeCache(gc, &gc->ogCache[2], gc->constants.ogCacheCount2, gc->constants.ogCacheBytes2);
    __glInitOneCodeCache(gc, &gc->ogCache[3], gc->constants.ogCacheCount3, gc->constants.ogCacheBytes3);
    __glInitOneCodeCache(gc, &gc->ogCache[4], gc->constants.ogCacheCount4, gc->constants.ogCacheBytes4);
    gc->ogScratch = __GL_MALLOC(gc, gc->constants.ogScratchBytes);
}

/* 690e9a00 -- __glReleaseCodeCache: free the five caches and the scratch buffer */
void __fastcall __glReleaseCodeCache(__GLcontext *gc)
{
    __glReleaseOneCodeCache(gc, &gc->ogCache[0]);
    __glReleaseOneCodeCache(gc, &gc->ogCache[1]);
    __glReleaseOneCodeCache(gc, &gc->ogCache[2]);
    __glReleaseOneCodeCache(gc, &gc->ogCache[3]);
    __glReleaseOneCodeCache(gc, &gc->ogCache[4]);
    __GL_FREE(gc, gc->ogScratch);
    gc->ogScratch = 0;
}

/* 690e9a60 -- __glReleaseOneCodeCache: free one generated-code cache (entries and code) */
void __fastcall __glReleaseOneCodeCache(__GLcontext *gc, __GLogCacheICD *cache)
{
    __GL_FREE(gc, cache->codeInfo);
    __GL_FREE(gc, cache->codeSpace);
    cache->codeInfo = NULL;
    cache->codeSpace = NULL;
}

/* 690e9a90 -- __glInvalidateCodeCache (3dfx OGCACHE.H): invalidate the code of caches 2 and 3 */
void __fastcall __glInvalidateCodeCache(__GLcontext *gc)
{
    __glInvalidateOneCodeCache(gc, &gc->ogCache[2]);
    __glInvalidateOneCodeCache(gc, &gc->ogCache[3]);
}

/* 690e9ab0 -- __glInvalidateOneCodeCache: invalidate one cache, every entry on its list gets key -1 (gc unused) */
void __fastcall __glInvalidateOneCodeCache(__GLcontext *gc, __GLogCacheICD *cache)
{
    __GLcodeCell *e;

    for (e = cache->codeList; e != NULL; e = e->next) {
        e->modeFlags = -1;
    }
}

/* 690ea0a0 -- __glResetPixelFormats: device.resetPixelFormats (+0x20): drop the pixel-format table
** so 690ea0d0 rebuilds it on its next call (argument unused) */
void __fastcall __glResetPixelFormats(int unused)
{
    wglLock();
    if (g_pfdList != NULL) {
        wglFree(g_pfdList);
        g_pfdList = NULL;
    }
    g_pfdListStale = 1;
    wglUnlock();
}

/* ---- og_*: the vertex-program code generator's instruction scheduler (og.h OgInsn records) ---- */
/* ---- og_*: the compiled-code cache, entries on an MRU list ---- */
#define CELL(i)     (((__GLcodeCell *)cache->codeInfo)[i])
extern int __fastcall og_isMem(OgOperand *o);
extern GLuint __fastcall og_isMmxOp(OgInsn *insn);
extern int __fastcall og_canV(OgInsn *insn);
extern int __fastcall og_canPrecede(OgInsn *a, OgInsn *b);
extern GLuint __fastcall og_isBinop(OgInsn *insn);

/* 690e9010 -- og_optimizeBlock (3dfx OG.C Optimize_block): schedule the pending instruction list into issue
** slots (pairs: even = U pipe, odd = V
** pipe), then encode the slots in order at pc.  Returns the new pc; the list is emptied.
*/
GLubyte *__fastcall og_optimizeBlock(__GLcontext *gc, GLubyte *pc)
{
    OgInsn **slots;
    OgInsn *insn;
    int count, pos, i;

    slots = (OgInsn **)calloc(1000, 4);
    count = 0;
    for (insn = g_ogPendingList; insn; insn = insn->next) {
        if (og_canV(insn)) {
            for (i = (count + 1) | 1; i >= 0; i--) {
                if (!og_canPrecede(insn, slots[i + 1]))
                    break;
                if (og_canPair(insn, slots[i ^ 1]) && slots[i] == 0 &&
                    (!(i & 1) || !og_cDependsOn(insn, slots[i - 1])))
                    pos = i;
            }
            if (og_isMmxOp(insn) && !(pos & 1) && slots[pos + 1] == 0)
                pos++;
        } else {
            for (i = (count + 4) & ~1; i >= 0; i -= 2) {
                if (!og_canPrecede(insn, slots[i + 1]) || !og_canPrecede(insn, slots[i + 2]))
                    break;
                if (!((insn->op == 0x57 || insn->op == 0x56) &&
                      (og_isReg32(insn->a) || og_isMem(insn->a)) && i >= 2 &&
                      (og_dependsOn(insn, slots[i - 1]) || og_dependsOn(insn, slots[i - 2]))) &&
                    og_canPair(insn, slots[i ^ 1]) && slots[i] == 0)
                    pos = i;
            }
        }
        slots[pos] = insn;
        pos++;
        if (count < pos)
            count = pos;
    }
    for (i = 0; i < count; i++) {
        if (slots[i])
            pc = og_encode(gc, pc, slots[i]);
    }
    free(slots);
    g_ogPendingList = 0;
    return pc;
}

/* 690e9220 -- og_canV (3dfx OG.C can_v): can the instruction issue in the V pipe (the scheduler's odd slots):
** never for ops
** 0x1e, 0x2a..0x2d and 0x56, always for 0x0a..0x17, otherwise when og_isBinop allows it and it is
** not an MMX op with a og_isMem second operand.
*/
int __fastcall og_canV(OgInsn *insn)
{
    switch (insn->op) {
    case 0x1e:
    case 0x2a:
    case 0x2b:
    case 0x2c:
    case 0x2d:
    case 0x56:
        return 0;
    }
    if (insn->op >= 0x0a && insn->op <= 0x17)
        return 1;
    if (og_isBinop(insn)) {
        if (og_isMmx(insn->a) && og_isMmx(insn->b))
            return 1;
        if (og_isMmx(insn->a) && og_isMem(insn->b))
            return 0;
        if (og_isMmx(insn->a) && og_isImm(insn->b))
            return 1;
        return 1;
    }
    return 0;
}

/* 690e9ad0 -- __glAllocCodeSpace (3dfx OGCACHE.H): look (modeFlags, signature) up in the cache: a hit or a free
** cell (modeFlags -1) moves to the front; a miss recycles the last cell.  Returns GL_TRUE only for a hit; the
** cell's name space (ppns, may be NULL) and code pointer go out.
*/
GLboolean __fastcall __glAllocCodeSpace(__GLogCacheICD *cache, GLint modeFlags, GLint signature, GLubyte **ppns,
                                  GLubyte **code)
{
    __GLcodeCell *e = cache->codeList;
    __GLcodeCell *prev = 0;

    if (g_ogCacheDisabled) {
        *code = e->code;
        if (ppns) {
            *ppns = e->tr;
        }
        return GL_FALSE;
    }
    for (;;) {
        if ((e->modeFlags == modeFlags && e->signature == signature) || e->modeFlags == -1) {
            if (prev) {
                prev->next = e->next;
                e->next = cache->codeList;
                cache->codeList = e;
            }
            if (ppns) {
                *ppns = e->tr;
            }
            *code = e->code;
            if (e->modeFlags == -1) {
                e->modeFlags = modeFlags;
                e->signature = signature;
                return GL_FALSE;
            }
            return GL_TRUE;
        }
        if (!e->next) {
            break;
        }
        prev = e;
        e = e->next;
    }
    e->modeFlags = modeFlags;
    e->signature = signature;
    if (ppns) {
        *ppns = e->tr;
    }
    *code = e->code;
    e->next = cache->codeList;
    cache->codeList = e;
    prev->next = 0;
    return GL_FALSE;
}

/* 690e9710 -- og_fpk (3dfx OG.C fpk): a memory operand for the float constant v, interned in gc->ogFloatPool
** (as 690e97b0:
** a full pool is replaced by an empty one twice the size; v is compared unrounded) */
OgOperand *__fastcall og_fpk(__GLcontext *gc, GLuint edx_unused, double v)
{
    GLuint n = gc->ogFloatPool.count, i;
    GLfloat *pool = (GLfloat *)gc->ogFloatPool.data;

    for (i = 0; i < n; i++)
	if (pool[i] == v)
	    break;
    if (i == n) {
	pool[n] = (GLfloat)v;
	if (++gc->ogFloatPool.count == gc->ogFloatPool.capacity) {
	    gc->ogFloatPool.count = 0;
	    gc->ogFloatPool.capacity *= 2;
	    gc->ogFloatPool.data = calloc(4, gc->ogFloatPool.capacity);
	}
    }
    return og_memBase(gc, og_imm(gc, (GLint)&pool[i]), 0);
}

/* 690e9946 -- __glInitOneCodeCache: one generated-code cache, n cells with `size` bytes of code each, all on the
** MRU list (the 3dfx __glInitializeCodeCache's body for one cache) */
void __fastcall __glInitOneCodeCache(__GLcontext *gc, __GLogCacheICD *cache, GLuint n, GLuint size)
{
    GLuint i;
    char *s;

    if (g_ogCacheReadEnv) {
	s = getenv("__GL_DISABLE_OG_CACHE");
	if (s)
	    g_ogCacheDisabled = atoi(s) != 0;
    }
    cache->codeInfo = __GL_MALLOC(gc, n * sizeof(__GLcodeCell));
    cache->codeSpace = (GLubyte *) __GL_MALLOC(gc, n * size);
    cache->codeList = NULL;
    for (i = 0; i < n; i++) {
	CELL(i).modeFlags = -1;
	CELL(i).signature = 0;
	CELL(i).code = cache->codeSpace + i * size;
	CELL(i).next = cache->codeList;
	cache->codeList = &CELL(i);
    }
}

/*
** sst_grstate.c -- the Glide state records (grState[], grStateExt[]): set-up and flush, the Glide extension
** entry points and one grGet value.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "glshade.h"
#include "si_names.h"
#include "icdglob.h"
#include <glide.h>
#include "gldisp.h"
#include "icdprotos.h"
#include "og.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* 690f66f0 -- TEXUMA: one texture memory for both TMUs (icdglob.h __GL_TMU_SLOT) */
char g_tmuSharedMemory = 0;
/* one Glide state record: allocate, then load each argument word, marking the record dirty on change */
#define GRS_ALLOC(i, size) \
    p = (GLint *)__GL_CALLOC(gc, size, 1); \
    gc->hwcx.grState[i] = p; \
    if (p == 0) return GL_FALSE
#define GRS_FIRST(i, v) \
    if (*p != (v)) { *p = (v); gc->hwcx.grDirty |= 1 << (i); }
#define GRX_ALLOC(i, size) \
    p = (GLint *)__GL_CALLOC(gc, size, 1); \
    gc->hwcx.grStateExt[i] = p; \
    if (p == 0) return GL_FALSE
#define GRX_FIRST(i, k, v) \
    if (p[k] != (v)) { p[k] = (v); gc->hwcx.grDirtyExt |= 1 << (i); }
/* the procs of the {name, proc} table wglGetProcAddress searches (glstubs.c g_extProcTable, the original's 690ee000:
** this view starts at the first proc, as the original's loop does (mov ecx,0x690ee004), so .name is the NEXT entry's
** name; unused) and its entry count (a const in .text in the original: the loop reads it). */
typedef struct __GLExtProcRec {
    PROC        proc;
    const char *name;
} __GLExtProc;
extern struct { const char *name; PROC fn; } g_extProcTable[0x72];   /* glstubs.c */
extern const GLint g_extProcCount;                                    /* glstubs.c: 0x72 */
#define g_extProcTableW27   ((__GLExtProc *)&g_extProcTable[0].fn)
#define GETPROC(field, name) \
    ((gc->hwcx.field = (__GLproc)grGetProcAddress(name)) != NULL)
typedef void (__stdcall *__GLext2)(GLint, GLint);
typedef void (__stdcall *__GLext3)(GLint, GLint, GLint);
typedef void (__stdcall *__GLext4)(GLint, GLint, GLint, GLint);
typedef void (__stdcall *__GLext6)(GLint, GLint, GLint, GLint, GLint, GLint);
typedef void (__stdcall *__GLext10)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint);
typedef void (__stdcall *__GLext11)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint);
typedef void (__stdcall *__GLstencilFunc)(GLint, GLubyte, GLubyte);
typedef void (__stdcall *__GLstencilMask)(GLubyte);
extern void __fastcall hwInitGlideExtensions(__GLcontext *gc);

/* 69027680 -- grGetInteger: one grGet value (the result slot is not initialised) */
GLint __fastcall grGetInteger(GLint pname)
{
    FxI32 value;

    grGet(pname, 4, &value);
    return value;
}

/* 690277f0 (size 4429) -- hwInitGrState: allocates and initialises the Glide state records grState[] /
** grStateExt[] */
GLboolean __fastcall hwInitGrState(__GLcontext *gc)
{
    GLint *p;
    GLubyte *b;

    gc->hwcx.numTMU = grGetInteger(GR_NUM_TMU);
    gc->hwcx.extensionStr = grGetString(GR_EXTENSION);
    hwInitGlideExtensions(gc);
    hwParseGlideVersion(gc);
    hwDirtyAllGrState(gc);
    gc->hwcx.swapInterval = 1;
    gc->hwcx.hwPrim = -1;
    gc->hwcx.primClass = 0xffffffff;               /* primitive class of the open glBegin: none yet */

    GRS_ALLOC(__GL_GR_ALPHA_BLEND, 0xc);
    GRS_FIRST(__GL_GR_ALPHA_BLEND, 4);
    GRS_SET(__GL_GR_ALPHA_BLEND, 1, 0);
    GRS_SET(__GL_GR_ALPHA_BLEND, 2, 0);
    GRS_ALLOC(__GL_GR_DEPTH_FUNC, 4);
    GRS_FIRST(__GL_GR_DEPTH_FUNC, 1);
    GRS_ALLOC(__GL_GR_DEPTH_MODE, 4);
    GRS_FIRST(__GL_GR_DEPTH_MODE, 0);
    GRS_ALLOC(__GL_GR_DEPTH_MASK, 4);
    GRS_FIRST(__GL_GR_DEPTH_MASK, 1);
    GRS_ALLOC(__GL_GR_TMU0_FILTER, 8);
    GRS_FIRST(__GL_GR_TMU0_FILTER, 1);
    GRS_SET(__GL_GR_TMU0_FILTER, 1, 1);
    GRS_ALLOC(__GL_GR_TMU0_CLAMP, 8);
    GRS_FIRST(__GL_GR_TMU0_CLAMP, 0);
    GRS_SET(__GL_GR_TMU0_CLAMP, 1, 0);
    GRS_ALLOC(__GL_GR_TMU0_MIPMAP, 4);
    GRS_FIRST(__GL_GR_TMU0_MIPMAP, 1);
    if (gc->hwcx.numTMU == 2) {
        GRS_ALLOC(__GL_GR_TMU1_FILTER, 8);
        GRS_FIRST(__GL_GR_TMU1_FILTER, 1);
        GRS_SET(__GL_GR_TMU1_FILTER, 1, 1);
        GRS_ALLOC(__GL_GR_TMU1_CLAMP, 8);
        GRS_FIRST(__GL_GR_TMU1_CLAMP, 0);
        GRS_SET(__GL_GR_TMU1_CLAMP, 1, 0);
        GRS_ALLOC(__GL_GR_TMU1_MIPMAP, 4);
        GRS_FIRST(__GL_GR_TMU1_MIPMAP, 1);
        if (gc->hwcx.hasCombineExt == 0) {
            GRS_ALLOC(__GL_GR_TMU1_COMBINE, 8);
            GRS_FIRST(__GL_GR_TMU1_COMBINE, 1);
            GRS_SET(__GL_GR_TMU1_COMBINE, 1, 0);
        } else {
            GRX_ALLOC(__GL_GRX_TMU1_COLOR_COMBINE, 0x2c);
            GRX_FIRST(__GL_GRX_TMU1_COLOR_COMBINE, 1, 0xb);
            GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 2, 1);
            GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 3, 0);
            GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 4, 1);
            GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 5, 0);
            GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 6, 1);
            GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 7, 0);
            GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 8, 0);
            GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 9, 0);
            GRX_SET(__GL_GRX_TMU1_COLOR_COMBINE, 10, 0);
            GRX_ALLOC(__GL_GRX_TMU1_ALPHA_COMBINE, 0x2c);
            GRX_FIRST(__GL_GRX_TMU1_ALPHA_COMBINE, 1, 10);
            GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 2, 1);
            GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 3, 0);
            GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 4, 1);
            GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 5, 0);
            GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 6, 1);
            GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 7, 0);
            GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 8, 0);
            GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 9, 0);
            GRX_SET(__GL_GRX_TMU1_ALPHA_COMBINE, 10, 0);
            GRX_ALLOC(__GL_GRX_TMU1_CONSTANT, 8);
            GRX_FIRST(__GL_GRX_TMU1_CONSTANT, 1, 0);
        }
    }
    if (gc->hwcx.hasCombineExt == 0) {
        GRS_ALLOC(__GL_GR_TMU0_COMBINE, 8);
        GRS_FIRST(__GL_GR_TMU0_COMBINE, 1);
        GRS_SET(__GL_GR_TMU0_COMBINE, 1, 0);
        GRS_ALLOC(__GL_GR_COLOR_COMBINE, 0x10);
        GRS_FIRST(__GL_GR_COLOR_COMBINE, 3);
        GRS_SET(__GL_GR_COLOR_COMBINE, 1, 8);
        GRS_SET(__GL_GR_COLOR_COMBINE, 2, 1);
        GRS_SET(__GL_GR_COLOR_COMBINE, 3, 0);
        GRS_ALLOC(__GL_GR_ALPHA_COMBINE, 0x10);
        GRS_FIRST(__GL_GR_ALPHA_COMBINE, 3);
        GRS_SET(__GL_GR_ALPHA_COMBINE, 1, 8);
        GRS_SET(__GL_GR_ALPHA_COMBINE, 2, 1);
        GRS_SET(__GL_GR_ALPHA_COMBINE, 3, 0);
    } else {
        GRX_ALLOC(__GL_GRX_TMU0_COLOR_COMBINE, 0x2c);
        GRX_FIRST(__GL_GRX_TMU0_COLOR_COMBINE, 1, 0xb);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 2, 1);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 3, 0);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 4, 1);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 5, 0);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 6, 1);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 7, 0);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 8, 0);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 9, 0);
        GRX_SET(__GL_GRX_TMU0_COLOR_COMBINE, 10, 0);
        GRX_ALLOC(__GL_GRX_TMU0_ALPHA_COMBINE, 0x2c);
        GRX_FIRST(__GL_GRX_TMU0_ALPHA_COMBINE, 1, 10);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 2, 1);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 3, 0);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 4, 1);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 5, 0);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 6, 1);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 7, 0);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 8, 0);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 9, 0);
        GRX_SET(__GL_GRX_TMU0_ALPHA_COMBINE, 10, 0);
        GRX_ALLOC(__GL_GRX_COLOR_COMBINE, 0x28);
        GRX_FIRST(__GL_GRX_COLOR_COMBINE, 0, 9);
        GRX_SET(__GL_GRX_COLOR_COMBINE, 1, 1);
        GRX_SET(__GL_GRX_COLOR_COMBINE, 2, 0);
        GRX_SET(__GL_GRX_COLOR_COMBINE, 3, 1);
        GRX_SET(__GL_GRX_COLOR_COMBINE, 4, 0);
        GRX_SET(__GL_GRX_COLOR_COMBINE, 5, 1);
        GRX_SET(__GL_GRX_COLOR_COMBINE, 6, 0);
        GRX_SET(__GL_GRX_COLOR_COMBINE, 7, 0);
        GRX_SET(__GL_GRX_COLOR_COMBINE, 8, 0);
        GRX_SET(__GL_GRX_COLOR_COMBINE, 9, 0);
        GRX_ALLOC(__GL_GRX_ALPHA_COMBINE, 0x28);
        GRX_FIRST(__GL_GRX_ALPHA_COMBINE, 0, 8);
        GRX_SET(__GL_GRX_ALPHA_COMBINE, 1, 1);
        GRX_SET(__GL_GRX_ALPHA_COMBINE, 2, 0);
        GRX_SET(__GL_GRX_ALPHA_COMBINE, 3, 1);
        GRX_SET(__GL_GRX_ALPHA_COMBINE, 4, 0);
        GRX_SET(__GL_GRX_ALPHA_COMBINE, 5, 1);
        GRX_SET(__GL_GRX_ALPHA_COMBINE, 6, 0);
        GRX_SET(__GL_GRX_ALPHA_COMBINE, 7, 0);
        GRX_SET(__GL_GRX_ALPHA_COMBINE, 8, 0);
        GRX_SET(__GL_GRX_ALPHA_COMBINE, 9, 0);
        GRX_ALLOC(__GL_GRX_TMU0_CONSTANT, 8);
        GRX_FIRST(__GL_GRX_TMU0_CONSTANT, 1, 0);
    }
    GRS_ALLOC(__GL_GR_RENDER_BUFFER, 4);
    GRS_FIRST(__GL_GR_RENDER_BUFFER, 1);
    GRS_ALLOC(__GL_GR_CULL, 4);
    GRS_FIRST(__GL_GR_CULL, 0);
    GRS_ALLOC(__GL_GR_ALPHA_TEST_FUNC, 4);
    GRS_FIRST(__GL_GR_ALPHA_TEST_FUNC, 7);
    GRS_ALLOC(__GL_GR_ALPHA_TEST_REF, 4);
    GRS_FIRST(__GL_GR_ALPHA_TEST_REF, 0);
    GRS_ALLOC(__GL_GR_DEPTH_BIAS, 4);
    GRS_FIRST(__GL_GR_DEPTH_BIAS, 0);
    GRS_ALLOC(__GL_GR_COLOR_MASK, 0x14);
    GRS_FIRST(__GL_GR_COLOR_MASK, 1);
    GRS_SET(__GL_GR_COLOR_MASK, 1, 1);
    GRS_SET(__GL_GR_COLOR_MASK, 2, 1);
    GRS_SET(__GL_GR_COLOR_MASK, 3, 1);
    GRS_SET(__GL_GR_COLOR_MASK, 4, 1);
    if (gc->hwcx.has32bpp != 0) {
        GRS_ALLOC(__GL_GR_STENCIL_FUNC, 8);
        GRS_FIRST(__GL_GR_STENCIL_FUNC, 7);
        if (((GLubyte *)gc->hwcx.grState[__GL_GR_STENCIL_FUNC])[4] != 0) {
            ((GLubyte *)gc->hwcx.grState[__GL_GR_STENCIL_FUNC])[4] = 0;
            gc->hwcx.grDirty |= 1 << 20;
        }
        if (((GLubyte *)gc->hwcx.grState[__GL_GR_STENCIL_FUNC])[5] != (1 << gc->modes.stencilBits) - 1) {
            ((GLubyte *)gc->hwcx.grState[__GL_GR_STENCIL_FUNC])[5] = (1 << gc->modes.stencilBits) - 1;
            gc->hwcx.grDirty |= 1 << 20;
        }
        GRS_ALLOC(__GL_GR_STENCIL_OP, 0xc);
        GRS_FIRST(__GL_GR_STENCIL_OP, 0);
        GRS_SET(__GL_GR_STENCIL_OP, 1, 0);
        GRS_SET(__GL_GR_STENCIL_OP, 2, 0);
        b = (GLubyte *)__GL_CALLOC(gc, 1, 1);
        gc->hwcx.grState[__GL_GR_STENCIL_MASK] = (GLint *)b;
        if (b == 0) return GL_FALSE;
        if (*b != (1 << gc->modes.stencilBits) - 1) {
            *b = (1 << gc->modes.stencilBits) - 1;
            gc->hwcx.grDirty |= 1 << 22;
        }
    }
    GRS_ALLOC(__GL_GR_DITHER, 4);
    GRS_FIRST(__GL_GR_DITHER, 2);
    GRS_ALLOC(__GL_GR_FOG_COLOR, 4);
    GRS_FIRST(__GL_GR_FOG_COLOR, 0);
    GRS_ALLOC(__GL_GR_FOG, 0x10);
    GRS_FIRST(__GL_GR_FOG, 0x800);
    GRS_SET(__GL_GR_FOG, 1, 0x3f800000);
    if (((GLfloat *)gc->hwcx.grState[__GL_GR_FOG])[2] != 0.0f) {
        gc->hwcx.grState[__GL_GR_FOG][2] = 0;
        gc->hwcx.grDirty |= 1 << 25;
    }
    GRS_SET(__GL_GR_FOG, 3, 0x3f800000);
    GRS_ALLOC(__GL_GR_FOG_MODE, 4);
    GRS_FIRST(__GL_GR_FOG_MODE, 0);
    p = (GLint *)__GL_CALLOC(gc, 1, grGetInteger(GR_FOG_TABLE_ENTRIES));
    gc->hwcx.grState[__GL_GR_FOG_TABLE] = p;
    if (p == 0) return GL_FALSE;
    if (gc->hwcx.grCommandTransportInfoExt2 != 0) {
        ((void (__stdcall *)(void))gc->hwcx.grCommandTransportInfoExt2)();
    }
    return GL_TRUE;
}

/* 69028bd7 -- hwInitGlideExtensions: the Glide extensions (grGetString(GR_EXTENSION) in hwcx.extensionStr): the
** flags and the entry points of PALETTE6666, TEXUMA (one shared texture memory: g_tmuSharedMemory),
** COMMAND_TRANSPORT, SURFACE, PIXEXT (window open, stencil, buffer clear, colour mask: 32 bpp with an 8888 pixel
** format), COMBINE (the combine entries; the alpha blend function on its own), TEXFMT, GETGAMMA, PIXEXT again for
** the T-buffer write mask (without it the 3DFX multisample / T-buffer entries are removed from the
** wglGetProcAddress table), GETREGISTRY and FOGCOORD */
void __fastcall hwInitGlideExtensions(__GLcontext *gc)
{
    const char *ext = gc->hwcx.extensionStr;
    GLint i;
    PROC proc;

    if (strstr(ext, " PALETTE6666 ") != NULL) {
        gc->hwcx.palette6666 = 1;
        gc->hwcx.hasPalette6666 = 1;
    }
    if (strstr(ext, " TEXUMA ") != NULL) {
        g_tmuSharedMemory = 1;
        gc->hwcx.hasTexUMA = 1;
    }
    if (strstr(ext, " COMMAND_TRANSPORT ") != NULL) {
        gc->hwcx.hasCmdTransport = 1;
    }
    if (strstr(ext, " SURFACE ") != NULL) {
        gc->hwcx.hasSurfaceExt = 1;
    }
    if (strstr(ext, " PIXEXT ") != NULL) {
        if (GETPROC(grSstWinOpenExt, "grSstWinOpenExt") &&
            GETPROC(grStencilFuncExt, "grStencilFuncExt") &&
            GETPROC(grStencilMaskExt, "grStencilMaskExt") &&
            GETPROC(grStencilOpExt, "grStencilOpExt") &&
            GETPROC(grBufferClearExt, "grBufferClearExt") &&
            GETPROC(grColorMaskExt, "grColorMaskExt") &&
            gc->modes.redBits == 8 && gc->modes.greenBits == 8 &&
            gc->modes.blueBits == 8 && gc->modes.alphaBits == 8) {
            gc->hwcx.has32bpp = 1;
        }
    }
    if (strstr(ext, " COMBINE ") != NULL) {
        if (GETPROC(grColorCombineExt, "grColorCombineExt") &&
            GETPROC(grTexColorCombineExt, "grTexColorCombineExt") &&
            GETPROC(grAlphaCombineExt, "grAlphaCombineExt") &&
            GETPROC(grTexAlphaCombineExt, "grTexAlphaCombineExt") &&
            GETPROC(grConstantColorValueExt, "grConstantColorValueExt")) {
            gc->hwcx.hasCombineExt = 1;
        }
        if (GETPROC(grAlphaBlendFunctionExt, "grAlphaBlendFunctionExt")) {
            gc->hwcx.hasAlphaBlendExt = 1;
        }
    }
    if (strstr(ext, " TEXFMT ") != NULL) {
        gc->hwcx.hasTexFmt32 = 1;
        gc->hwcx.hasTexFmtLarge = 1;
        gc->hwcx.hasTexFmtNapalm = 1;
    }
    if (strstr(ext, " GETGAMMA ") != NULL) {
        if (GETPROC(grGetGammaTableExt, "grGetGammaTableExt")) {
            gc->hwcx.hasGetGamma = 1;
        }
    }
    if (strstr(ext, " PIXEXT ") != NULL) {
        if (GETPROC(grTBufferWriteMaskExt, "grTBufferWriteMaskExt")) {
            gc->hwcx.hasTBuffer = 1;
        }
    }
    if (gc->hwcx.hasTBuffer == 0) {
        for (i = 0; i < g_extProcCount; i++) {
            proc = g_extProcTableW27[i].proc;
            if (proc == (PROC)wglQueryMultisample3DFX || proc == (PROC)wglSetMultisample3DFX ||
                proc == (PROC)glTBufferMask3DFX) {
                g_extProcTableW27[i].proc = NULL;
            }
        }
    }
    if (strstr(ext, " GETREGISTRY ") != NULL) {
        if (GETPROC(grGetRegistryOrEnvironmentStringExt, "grGetRegistryOrEnvironmentStringExt")) {
            gc->hwcx.hasGetRegistry = 1;
        }
    }
    if (strstr(ext, " FOGCOORD ") != NULL) {
        gc->hwcx.hasFogCoord = 1;
    }
}

extern void __fastcall hwLinearFogTable(GrFog_t *table, float start, float end);
/* 69029070 (size 1455) -- hwFlushGrState: flush the dirty Glide state records (grDirty bit i -> grState[i], then the
** combine extensions grDirtyExt / grStateExt) to Glide */
void __fastcall hwFlushGrState(__GLcontext *gc, GLuint edx_unused)
{
    GLint *r;
    GLint i;

    i = 0;
    while (gc->hwcx.grDirty != 0) {
        if (gc->hwcx.grDirty & 1) switch (i) {
          case __GL_GR_ALPHA_BLEND:
            if (gc->hwcx.hasAlphaBlendExt) {
                r = gc->hwcx.grState[__GL_GR_ALPHA_BLEND];
                ((__GLext6)gc->hwcx.grAlphaBlendFunctionExt)(r[0], r[1], r[2], r[0], r[1], r[2]);
            } else {
                r = gc->hwcx.grState[__GL_GR_ALPHA_BLEND];
                grAlphaBlendFunction(r[0], r[1], 0, 0);
            }
            break;
          case __GL_GR_DEPTH_FUNC:
            grDepthBufferFunction(*gc->hwcx.grState[__GL_GR_DEPTH_FUNC]);
            break;
          case __GL_GR_DEPTH_MODE:
            gc->hwcx.grDirty |= 2;
            break;
          case __GL_GR_DEPTH_MASK:
            grDepthBufferMode(*gc->hwcx.grState[__GL_GR_DEPTH_MODE]);
            if (*gc->hwcx.grState[__GL_GR_DEPTH_MODE] == 0)
                grDepthMask(0);
            else
                grDepthMask(*gc->hwcx.grState[__GL_GR_DEPTH_MASK]);
            break;
          case __GL_GR_TMU0_FILTER:
            grTexFilterMode(0, gc->hwcx.grState[__GL_GR_TMU0_FILTER][1], gc->hwcx.grState[__GL_GR_TMU0_FILTER][0]);
            break;
          case __GL_GR_TMU0_CLAMP:
            grTexClampMode(0, gc->hwcx.grState[__GL_GR_TMU0_CLAMP][0], gc->hwcx.grState[__GL_GR_TMU0_CLAMP][1]);
            break;
          case __GL_GR_TMU0_MIPMAP:
            grTexMipMapMode(0, gc->hwcx.grState[__GL_GR_TMU0_MIPMAP][0], 0);
            break;
          case __GL_GR_TMU0_COMBINE:
            r = gc->hwcx.grState[__GL_GR_TMU0_COMBINE];
            grTexCombine(0, r[0], r[1], r[0], r[1], 0, 0);
            break;
          case __GL_GR_TMU1_FILTER:
            grTexFilterMode(1, gc->hwcx.grState[__GL_GR_TMU1_FILTER][1], gc->hwcx.grState[__GL_GR_TMU1_FILTER][0]);
            break;
          case __GL_GR_TMU1_CLAMP:
            grTexClampMode(1, gc->hwcx.grState[__GL_GR_TMU1_CLAMP][0], gc->hwcx.grState[__GL_GR_TMU1_CLAMP][1]);
            break;
          case __GL_GR_TMU1_MIPMAP:
            grTexMipMapMode(1, gc->hwcx.grState[__GL_GR_TMU1_MIPMAP][0], 0);
            break;
          case __GL_GR_TMU1_COMBINE:
            r = gc->hwcx.grState[__GL_GR_TMU1_COMBINE];
            grTexCombine(1, r[0], r[1], r[0], r[1], 0, 0);
            break;
          case __GL_GR_COLOR_COMBINE:
            r = gc->hwcx.grState[__GL_GR_COLOR_COMBINE];
            grColorCombine(r[0], r[1], r[2], r[3], 0);
            break;
          case __GL_GR_ALPHA_COMBINE:
            r = gc->hwcx.grState[__GL_GR_ALPHA_COMBINE];
            grAlphaCombine(r[0], r[1], r[2], r[3], 0);
            break;
          case __GL_GR_RENDER_BUFFER:
            grRenderBuffer(*gc->hwcx.grState[__GL_GR_RENDER_BUFFER]);
            break;
          case __GL_GR_CULL:
            grCullMode(*gc->hwcx.grState[__GL_GR_CULL]);
            break;
          case __GL_GR_ALPHA_TEST_FUNC:
            grAlphaTestFunction(*gc->hwcx.grState[__GL_GR_ALPHA_TEST_FUNC]);
            break;
          case __GL_GR_ALPHA_TEST_REF:
            grAlphaTestReferenceValue(*(GrAlpha_t *)gc->hwcx.grState[__GL_GR_ALPHA_TEST_REF]);
            break;
          case __GL_GR_DEPTH_BIAS:
            grDepthBiasLevel(*gc->hwcx.grState[__GL_GR_DEPTH_BIAS]);
            break;
          case __GL_GR_COLOR_MASK:
            if (gc->hwcx.has32bpp) {
                r = gc->hwcx.grState[__GL_GR_COLOR_MASK];
                ((__GLext4)gc->hwcx.grColorMaskExt)(r[1], r[2], r[3], r[4]);
            } else {
                grColorMask(gc->hwcx.grState[__GL_GR_COLOR_MASK][0], gc->modes.alphaBits ? gc->hwcx.grState[__GL_GR_COLOR_MASK][4] : 0);
            }
            break;
          case __GL_GR_STENCIL_FUNC:
            r = gc->hwcx.grState[__GL_GR_STENCIL_FUNC];
            ((__GLstencilFunc)gc->hwcx.grStencilFuncExt)(r[0], ((GLubyte *)r)[4], ((GLubyte *)r)[5]);
            break;
          case __GL_GR_STENCIL_OP:
            r = gc->hwcx.grState[__GL_GR_STENCIL_OP];
            ((__GLext3)gc->hwcx.grStencilOpExt)(r[0], r[1], r[2]);
            break;
          case __GL_GR_STENCIL_MASK:
            ((__GLstencilMask)gc->hwcx.grStencilMaskExt)(*(GLubyte *)gc->hwcx.grState[__GL_GR_STENCIL_MASK]);
            break;
          case __GL_GR_DITHER:
            grDitherMode(*gc->hwcx.grState[__GL_GR_DITHER]);
            break;
          case __GL_GR_FOG_COLOR:
            grFogColorValue(*gc->hwcx.grState[__GL_GR_FOG_COLOR]);
            break;
          case __GL_GR_FOG:
            r = gc->hwcx.grState[__GL_GR_FOG];
            switch (r[0]) {
              case GL_LINEAR:
                if (gc->state.fog.coordSource == GL_FRAGMENT_DEPTH_EXT)
                    guFogGenerateLinear((GrFog_t *)gc->hwcx.grState[__GL_GR_FOG_TABLE], ((GLfloat *)r)[2], ((GLfloat *)r)[3]);
                else
                    hwLinearFogTable((GrFog_t *)gc->hwcx.grState[__GL_GR_FOG_TABLE], ((GLfloat *)r)[2], ((GLfloat *)r)[3]);
                break;
              case GL_EXP:
                guFogGenerateExp((GrFog_t *)gc->hwcx.grState[__GL_GR_FOG_TABLE], ((GLfloat *)r)[1]);
                break;
              case GL_EXP2:
                guFogGenerateExp2((GrFog_t *)gc->hwcx.grState[__GL_GR_FOG_TABLE], ((GLfloat *)r)[1]);
                break;
            }
            grFogTable((GrFog_t *)gc->hwcx.grState[__GL_GR_FOG_TABLE]);
            break;
          case __GL_GR_FOG_MODE:
            grFogMode(*gc->hwcx.grState[__GL_GR_FOG_MODE]);
            break;
        }
        gc->hwcx.grDirty >>= 1;
        i++;
    }
    if (gc->hwcx.hasCombineExt) {
        i = 0;
        while (gc->hwcx.grDirtyExt != 0) {
            if (gc->hwcx.grDirtyExt & 1) switch (i) {
              case __GL_GRX_COLOR_COMBINE:
                r = gc->hwcx.grStateExt[__GL_GRX_COLOR_COMBINE];
                ((__GLext10)gc->hwcx.grColorCombineExt)(r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9]);
                break;
              case __GL_GRX_ALPHA_COMBINE:
                r = gc->hwcx.grStateExt[__GL_GRX_ALPHA_COMBINE];
                ((__GLext10)gc->hwcx.grAlphaCombineExt)(r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9]);
                break;
              case __GL_GRX_TMU0_COLOR_COMBINE:
                r = gc->hwcx.grStateExt[__GL_GRX_TMU0_COLOR_COMBINE];
                ((__GLext11)gc->hwcx.grTexColorCombineExt)(0, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9], r[10]);
                break;
              case __GL_GRX_TMU1_COLOR_COMBINE:
                r = gc->hwcx.grStateExt[__GL_GRX_TMU1_COLOR_COMBINE];
                ((__GLext11)gc->hwcx.grTexColorCombineExt)(1, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9], r[10]);
                break;
              case __GL_GRX_TMU0_ALPHA_COMBINE:
                r = gc->hwcx.grStateExt[__GL_GRX_TMU0_ALPHA_COMBINE];
                ((__GLext11)gc->hwcx.grTexAlphaCombineExt)(0, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9], r[10]);
                break;
              case __GL_GRX_TMU1_ALPHA_COMBINE:
                r = gc->hwcx.grStateExt[__GL_GRX_TMU1_ALPHA_COMBINE];
                ((__GLext11)gc->hwcx.grTexAlphaCombineExt)(1, r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9], r[10]);
                break;
              case __GL_GRX_TMU0_CONSTANT:
                ((__GLext2)gc->hwcx.grConstantColorValueExt)(0, gc->hwcx.grStateExt[__GL_GRX_TMU0_CONSTANT][1]);
                break;
              case __GL_GRX_TMU1_CONSTANT:
                ((__GLext2)gc->hwcx.grConstantColorValueExt)(1, gc->hwcx.grStateExt[__GL_GRX_TMU1_CONSTANT][1]);
                break;
            }
            gc->hwcx.grDirtyExt >>= 1;
            i++;
        }
    }
    if (gc->hwcx.grCommandTransportInfoExt2 != 0)
        ((void (__stdcall *)(void))gc->hwcx.grCommandTransportInfoExt2)();
}

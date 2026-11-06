/*
** gl_store.c -- s_store.c / s_span.c / s_blend.c / s_atest.c / s_itest.c: fragment stores and span operations.
*/
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "icdprotos.h"
#include <string.h>
#include "og.h"

/* ---- s_store.c: the alpha-tested store procs.  The ICD keeps an RGBA variant (the table at
** frontBuffer.alphaTestFuncTable indexed by alpha * alphaTableConv) and a color index variant (the table
** frontBuffer.indexTestFuncTable indexed by the index in color.r) of each. */
#define __GL_RGBA_ALPHA_TEST(gc, frag) \
    ((gc)->frontBuffer.alphaTestFuncTable)[(GLint)((gc)->constants.alphaTableConv * (frag)->color.a)]
typedef void (__fastcall *__GLcfbStore)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
#define __GL_DO_STORE_ASD(ALPHA_TEST) \
    __GLcontext *gc; \
    GLint x, y; \
 \
    gc = cfb->buf.gc; \
 \
    x = frag->x; \
    y = frag->y; \
 \
    /* Pixel ownership, scissor */ \
    if (x < gc->transform.clipX0 || y < gc->transform.clipY0 || \
	    x >= gc->transform.clipX1 || y >= gc->transform.clipY1) { \
	return; \
    } \
 \
    if (!ALPHA_TEST(gc, frag)) { \
	/* alpha test failed */ \
	return; \
    } \
    if (!(*gc->stencilBuffer.testFunc)(&gc->stencilBuffer, x, y)) { \
	/* stencil test failed */ \
	(*gc->stencilBuffer.failOp)(&gc->stencilBuffer, x, y); \
	return; \
    } \
    if (!(*gc->depthBuffer.store)(&gc->depthBuffer, x, y, frag->z)) { \
	/* depth buffer test failed */ \
	(*gc->stencilBuffer.passDepthFailOp)(&gc->stencilBuffer, x, y); \
	return; \
    } \
    (*gc->stencilBuffer.depthPassOp)(&gc->stencilBuffer, x, y); \
 \
    (*(__GLcfbStore)gc->procs.cfbStore)(cfb, frag);
#define __GL_CI_ALPHA_TEST(gc, frag) \
    ((gc)->frontBuffer.indexTestFuncTable)[(GLint)(frag)->color.r]
/* the front buffer's two test tables (alphaTestFuncTable / indexTestFuncTable) */
#define ALPHA_TABLE_RGB(gc)     ((gc)->frontBuffer.alphaTestFuncTable)
typedef GLboolean (__fastcall *__GLstencilTest)(__GLstencilBufferICD *sfb, GLint x, GLint y);
#define STENCIL_TEST(gc, x, y) \
    (*(__GLstencilTest)(gc)->stencilBuffer.testFunc)(&(gc)->stencilBuffer, x, y)
#define ALPHA_TABLE_CI(gc)      ((gc)->frontBuffer.indexTestFuncTable)
#define __GL_DO_STORE_AD(ALPHA_TEST) \
    __GLcontext *gc; \
    GLint x, y; \
 \
    gc = cfb->buf.gc; \
 \
    x = frag->x; \
    y = frag->y; \
 \
    /* Pixel ownership, scissor */ \
    if (x < gc->transform.clipX0 || y < gc->transform.clipY0 || \
	    x >= gc->transform.clipX1 || y >= gc->transform.clipY1) { \
	return; \
    } \
 \
    if (!ALPHA_TEST(gc, frag)) { \
	/* alpha test failed */ \
	return; \
    } \
    if (!(*gc->depthBuffer.store)(&gc->depthBuffer, x, y, frag->z)) { \
	/* depth buffer test failed */ \
	return; \
    } \
 \
    (*(__GLcfbStore)gc->procs.cfbStore)(cfb, frag);
#define __GL_DO_STORE_A(ALPHA_TEST) \
    __GLcontext *gc; \
    GLint x, y; \
 \
    gc = cfb->buf.gc; \
 \
    x = frag->x; \
    y = frag->y; \
 \
    /* Pixel ownership, scissor */ \
    if (x < gc->transform.clipX0 || y < gc->transform.clipY0 || \
	    x >= gc->transform.clipX1 || y >= gc->transform.clipY1) { \
	return; \
    } \
 \
    if (!ALPHA_TEST(gc, frag)) { \
	/* alpha test failed */ \
	return; \
    } \
 \
    (*(__GLcfbStore)gc->procs.cfbStore)(cfb, frag);
static __GLproc g_storeProcs[16];           /* s_pick.c storeProcs, indexed by ix (690f4450; at the end of the file) */
#define SHADE_DEPTH_TEST        0x00000004
#define SHADE_STENCIL_TEST      0x00000020
#define SHADE_ALPHA_TEST        0x00000200
#define SHADE_CI_ALPHA_TEST     0x00200000
/************************************************************************/
/* s_blend.c */
#define AS_PROC(f)             ((__GLproc)(f))
typedef void *(__fastcall *__GLmallocProc)(__GLcontext *gc, size_t size);
#define IMPORTS_MALLOC(gc)  ((__GLmallocProc)(gc)->imports[0])
#define INDEX_TEST_TABLE(gc)  ((gc)->frontBuffer.indexTestFuncTable)   /* +0x30 */
/************************************************************************/
/* s_atest.c */
#define ALPHA_TEST_TABLE(gc)  ((gc)->frontBuffer.alphaTestFuncTable)   /* +0x2c */
/* ---- s_span.c ---- */
typedef void (__fastcall *__GLfn_store)(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
typedef void (__fastcall *__GLfn_validate)(__GLcontext *gc);

/* 6908ac00 -- __glDoStore_ASD */
void __fastcall __glDoStore_ASD(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GL_DO_STORE_ASD(__GL_RGBA_ALPHA_TEST)
}

/* 6908acd0 -- __glDoStore_ASD_CI: __glDoStore_ASD, color index */
void __fastcall __glDoStore_ASD_CI(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GL_DO_STORE_ASD(__GL_CI_ALPHA_TEST)
}

/* 6908ada0 -- __glDoStore_AS (s_store.c) */
void __fastcall __glDoStore_AS(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GLcontext *gc;
    GLint x, y;

    gc = cfb->buf.gc;

    x = frag->x;
    y = frag->y;

    /* Pixel ownership, scissor */
    if (x < gc->transform.clipX0 || y < gc->transform.clipY0 ||
	    x >= gc->transform.clipX1 || y >= gc->transform.clipY1) {
	return;
    }

    if (!ALPHA_TABLE_RGB(gc)[(GLint) (gc->constants.alphaTableConv * frag->color.a)]) {
	/* alpha test failed */
	return;
    }
    if (!STENCIL_TEST(gc, x, y)) {
	/* stencil test failed */
	(*gc->stencilBuffer.failOp)(&gc->stencilBuffer, x, y);
	return;
    }
    (*gc->stencilBuffer.depthPassOp)(&gc->stencilBuffer, x, y);

    (*gc->procs.cfbStore)( cfb, frag );
}

/* 6908ae40 -- __glDoStore_AS_CI: __glDoStore_AS for colour index: the index tested through the second table */
void __fastcall __glDoStore_AS_CI(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GLcontext *gc;
    GLint x, y;

    gc = cfb->buf.gc;

    x = frag->x;
    y = frag->y;

    /* Pixel ownership, scissor */
    if (x < gc->transform.clipX0 || y < gc->transform.clipY0 ||
	    x >= gc->transform.clipX1 || y >= gc->transform.clipY1) {
	return;
    }

    if (!ALPHA_TABLE_CI(gc)[(GLint) frag->color.r]) {
	/* alpha (index) test failed */
	return;
    }
    if (!STENCIL_TEST(gc, x, y)) {
	/* stencil test failed */
	(*gc->stencilBuffer.failOp)(&gc->stencilBuffer, x, y);
	return;
    }
    (*gc->stencilBuffer.depthPassOp)(&gc->stencilBuffer, x, y);

    (*gc->procs.cfbStore)( cfb, frag );
}

/* 6908aed0 -- __glDoStore_AD */
void __fastcall __glDoStore_AD(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GL_DO_STORE_AD(__GL_RGBA_ALPHA_TEST)
}

/* 6908af50 -- __glDoStore_AD_CI: __glDoStore_AD, color index */
void __fastcall __glDoStore_AD_CI(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GL_DO_STORE_AD(__GL_CI_ALPHA_TEST)
}

/* 6908b080 -- __glDoStore_A */
void __fastcall __glDoStore_A(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GL_DO_STORE_A(__GL_RGBA_ALPHA_TEST)
}

/* 6908b0e0 -- __glDoStore_A_CI: __glDoStore_A, color index */
void __fastcall __glDoStore_A_CI(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag)
{
    __GL_DO_STORE_A(__GL_CI_ALPHA_TEST)
}

/* 6908b2a0 -- __glGenericPickStoreProcs (s_store.c) with the ICD's colour-index alpha test bit;
** an unknown draw buffer leaves the procs alone */
void __fastcall __glGenericPickStoreProcs(__GLcontext *gc)
{
    GLint ix = 0;
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);

    if ((modeFlags & SHADE_ALPHA_TEST) && gc->modes.rgbMode) {
	ix |= 1;
    }
    if (modeFlags & SHADE_STENCIL_TEST) {
	ix |= 2;
    }
    if (modeFlags & SHADE_DEPTH_TEST) {
	ix |= 4;
    }
    if ((modeFlags & SHADE_CI_ALPHA_TEST) && !gc->modes.rgbMode) {
	ix |= 8;
    }
    switch (gc->state.raster.drawBuffer) {
      case GL_NONE:
	gc->procs.store = g_storeProcs[ix];
	gc->procs.cfbStore = (__GLproc)__glDoNullStore;
	break;
      case GL_FRONT_AND_BACK:
	if (gc->buffers.doubleStore) {
	    gc->procs.store = g_storeProcs[ix];
	    gc->procs.cfbStore = (__GLproc)__glDoDoubleStore;
	    break;
	}
	/*
	** Note that there is an intentional drop through here.
	*/
      case GL_FRONT:
      case GL_BACK:
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
	if (ix == 0) {
	    gc->procs.store = gc->drawBuffer->store;
	    gc->procs.cfbStore = gc->drawBuffer->store;
	} else {
	    gc->procs.store = g_storeProcs[ix];
	    gc->procs.cfbStore = gc->drawBuffer->store;
	}
	break;
    }
}

/* 69093510 -- __glGenericPickBlendProcs (no blend equation in this SI: always BlendAdd) */
void __fastcall __glGenericPickBlendProcs(__GLcontext *gc)
{
    GLenum s = gc->state.raster.blendSrc;
    GLenum d = gc->state.raster.blendDst;

    if (gc->modes.colorIndexMode) {
        return;
    }

    /* Does the blending function need to fetch the dst color? */
    if (d == GL_ZERO && s != GL_DST_COLOR && s != GL_ONE_MINUS_DST_COLOR &&
            s != GL_DST_ALPHA && s != GL_ONE_MINUS_DST_ALPHA &&
            s != GL_SRC_ALPHA_SATURATE) {
        gc->procs.blend = AS_PROC(NoFetchBlend);
    } else {
        gc->procs.blend = AS_PROC(FetchBlend);
    }
    if (!(gc->state.enables.general & __GL_BLEND_ENABLE)) {
        gc->procs.blendColor = AS_PROC(Nop);
        gc->procs.blendEquation = AS_PROC(BlendAdd);
        return;
    }

    /* Look for any fast paths */
    if (s == GL_SRC_ALPHA) {
        if (d == GL_ZERO) {
            gc->procs.blendColor = AS_PROC(DoBlend_SA_ZERO);
            return;
        }
        if (d == GL_ONE) {
            gc->procs.blendColor = AS_PROC(DoBlend_SA_ONE);
            return;
        }
        if (d == GL_ONE_MINUS_SRC_ALPHA) {
            gc->procs.blendColor = AS_PROC(DoBlend_SA_MSA);
            return;
        }
    } else if (s == GL_ONE_MINUS_SRC_ALPHA) {
        if (d == GL_SRC_ALPHA) {
            gc->procs.blendColor = AS_PROC(DoBlend_MSA_SA);
            return;
        }
    }

    /* Use generic blend function */
    if ((s == GL_DST_ALPHA && d == GL_ONE_MINUS_DST_ALPHA) ||
        (s == GL_ONE_MINUS_DST_ALPHA && d == GL_DST_ALPHA) ||
        (d == GL_ONE_MINUS_SRC_COLOR) ||
        (s == GL_ONE_MINUS_DST_COLOR) ||
        (d == GL_ZERO) ||
        (s == GL_ZERO)) {
        gc->procs.blendColor = AS_PROC(DoBlendNoClamp);
    } else {
        gc->procs.blendColor = AS_PROC(DoBlend);
    }

    switch (s) {
      case GL_ZERO:
        gc->procs.blendSrc = AS_PROC(DoBlendSourceZERO);
        break;
      case GL_ONE:
        gc->procs.blendSrc = AS_PROC(DoBlendSourceONE);
        break;
      case GL_DST_COLOR:
        gc->procs.blendSrc = AS_PROC(DoBlendSourceDC);
        break;
      case GL_ONE_MINUS_DST_COLOR:
        gc->procs.blendSrc = AS_PROC(DoBlendSourceMDC);
        break;
      case GL_SRC_ALPHA:
        gc->procs.blendSrc = AS_PROC(DoBlendSourceSA);
        break;
      case GL_ONE_MINUS_SRC_ALPHA:
        gc->procs.blendSrc = AS_PROC(DoBlendSourceMSA);
        break;
      case GL_DST_ALPHA:
        gc->procs.blendSrc = AS_PROC(DoBlendSourceDA);
        break;
      case GL_ONE_MINUS_DST_ALPHA:
        gc->procs.blendSrc = AS_PROC(DoBlendSourceMDA);
        break;
      case GL_SRC_ALPHA_SATURATE:
        gc->procs.blendSrc = AS_PROC(DoBlendSourceSAT);
        break;
      case GL_SRC_COLOR:                    /* NV_blend_square */
        gc->procs.blendSrc = AS_PROC(DoBlendSourceSC);
        break;
      case GL_ONE_MINUS_SRC_COLOR:
        gc->procs.blendSrc = AS_PROC(DoBlendSourceMSC);
        break;
    }
    switch (d) {
      case GL_ZERO:
        gc->procs.blendDst = AS_PROC(DoBlendDestZERO);
        break;
      case GL_ONE:
        gc->procs.blendDst = AS_PROC(DoBlendDestONE);
        break;
      case GL_SRC_COLOR:
        gc->procs.blendDst = AS_PROC(DoBlendDestSC);
        break;
      case GL_ONE_MINUS_SRC_COLOR:
        gc->procs.blendDst = AS_PROC(DoBlendDestMSC);
        break;
      case GL_SRC_ALPHA:
        gc->procs.blendDst = AS_PROC(DoBlendDestSA);
        break;
      case GL_ONE_MINUS_SRC_ALPHA:
        gc->procs.blendDst = AS_PROC(DoBlendDestMSA);
        break;
      case GL_DST_ALPHA:
        gc->procs.blendDst = AS_PROC(DoBlendDestDA);
        break;
      case GL_ONE_MINUS_DST_ALPHA:
        gc->procs.blendDst = AS_PROC(DoBlendDestMDA);
        break;
      case GL_DST_COLOR:                    /* NV_blend_square */
        gc->procs.blendDst = AS_PROC(DoBlendDestDC);
        break;
      case GL_ONE_MINUS_DST_COLOR:
        gc->procs.blendDst = AS_PROC(DoBlendDestMDC);
        break;
    }
    gc->procs.blendEquation = AS_PROC(BlendAdd);
}

/* 690a0f40 -- __glValidateIndexTest (3dfx S_ITEST.C): the SGI index test table (glIndexFuncSGI), colour-index
** mode only: frontBuffer+0x30 */
void __fastcall __glValidateIndexTest(__GLcontext *gc)
{
    GLubyte *itft;
    GLint i, limit;
    GLint ref;
    GLenum indexTestFunc = gc->state.raster.indexFunction;

    if (!gc->modes.rgbMode) {
        limit = 1 << gc->modes.indexBits;
        ref = (GLint) gc->state.raster.indexReference;

        itft = INDEX_TEST_TABLE(gc);
        if (!itft) {
            itft = (GLubyte*)
                (*IMPORTS_MALLOC(gc))(gc, (size_t) ((limit) * sizeof(GLubyte)));
            INDEX_TEST_TABLE(gc) = itft;
        }

        for (i = 0; i < limit; i++) {
            switch (indexTestFunc) {
              case GL_NEVER:        itft[i] = GL_FALSE; break;
              case GL_LESS:         itft[i] = (GLubyte) (i <  ref); break;
              case GL_EQUAL:        itft[i] = (GLubyte) (i == ref); break;
              case GL_LEQUAL:       itft[i] = (GLubyte) (i <= ref); break;
              case GL_GREATER:      itft[i] = (GLubyte) (i >  ref); break;
              case GL_NOTEQUAL:     itft[i] = (GLubyte) (i != ref); break;
              case GL_GEQUAL:       itft[i] = (GLubyte) (i >= ref); break;
              case GL_ALWAYS:       itft[i] = GL_TRUE; break;
            }
        }
    }
}

/* 690a1030 -- __glValidateAlphaTest (the ICD keeps the alpha table at frontBuffer+0x2c) */
void __fastcall __glValidateAlphaTest(__GLcontext *gc)
{
    GLubyte *atft;
    GLint i, limit;
    GLint ref;
    GLenum alphaTestFunc = gc->state.raster.alphaFunction;

    limit = gc->constants.alphaTestSize;
    ref = (GLint)
        ((gc->frontBuffer.alphaScale * gc->state.raster.alphaReference) *
        gc->constants.alphaTableConv);

    atft = ALPHA_TEST_TABLE(gc);
    if (!atft) {
        atft = (GLubyte*)
            (*IMPORTS_MALLOC(gc))(gc, (size_t) ((limit) * sizeof(GLubyte)));
        ALPHA_TEST_TABLE(gc) = atft;
    }

    for (i = 0; i < limit; i++) {
        switch (alphaTestFunc) {
          case GL_NEVER:        *atft++ = GL_FALSE; break;
          case GL_LESS:         *atft++ = (GLubyte) (i <  ref); break;
          case GL_EQUAL:        *atft++ = (GLubyte) (i == ref); break;
          case GL_LEQUAL:       *atft++ = (GLubyte) (i <= ref); break;
          case GL_GREATER:      *atft++ = (GLubyte) (i >  ref); break;
          case GL_NOTEQUAL:     *atft++ = (GLubyte) (i != ref); break;
          case GL_GEQUAL:       *atft++ = (GLubyte) (i >= ref); break;
          case GL_ALWAYS:       *atft++ = GL_TRUE; break;
        }
    }
}

/* 690c5510 -- __glReturnSpan (s_span.c) */
void __fastcall __glReturnSpan(__GLcolorBufferICD *cfb, GLint x, GLint y,
                             const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    __GLfragmentICD frag;
    GLuint oldEnables;
    __GLcontext *gc = cfb->buf.gc;
    __GLfloat rscale, gscale, bscale, ascale;
    __GLaccumBufferICD *afb = &gc->accumBuffer;

    /* Temporarily disable blending if its enabled */
    oldEnables = gc->state.enables.general;
    if (oldEnables & __GL_BLEND_ENABLE) {
        gc->state.enables.general &= ~__GL_BLEND_ENABLE;
        __GL_DELAY_VALIDATE(gc);
        (*(__GLfn_validate) gc->procs.validate)(gc);
    }

    rscale = scale * afb->oneOverRedScale;
    gscale = scale * afb->oneOverGreenScale;
    bscale = scale * afb->oneOverBlueScale;
    ascale = scale * afb->oneOverAlphaScale;

    frag.x = x;
    frag.y = y;
    while (--w >= 0) {
        frag.color.r = ac->r * rscale;
        frag.color.g = ac->g * gscale;
        frag.color.b = ac->b * bscale;
        frag.color.a = ac->a * ascale;
        __glClampRGBColor(cfb->buf.gc, &frag.color, &frag.color);
        (*(__GLfn_store) cfb->store)(cfb, &frag);
        frag.x++;
        ac++;
    }

    /* Restore blending enable */
    if (oldEnables & __GL_BLEND_ENABLE) {
        gc->state.enables.general = oldEnables;
        __GL_DELAY_VALIDATE(gc);
        (*(__GLfn_validate) gc->procs.validate)(gc);
    }
}

/* 690f4450 -- s_pick.c storeProcs, indexed by ix */
static __GLproc g_storeProcs[16] = {
    (__GLproc)__glDoStore,
    (__GLproc)__glDoStore_A,
    (__GLproc)__glDoStore_S,
    (__GLproc)__glDoStore_AS,
    (__GLproc)__glDoStore_D,
    (__GLproc)__glDoStore_AD,
    (__GLproc)__glDoStore_SD,
    (__GLproc)__glDoStore_ASD,
    (__GLproc)__glDoStore_A_CI,
    (__GLproc)__glDoStore_A_CI,
    (__GLproc)__glDoStore_AS_CI,
    (__GLproc)__glDoStore_AS_CI,
    (__GLproc)__glDoStore_AD_CI,
    (__GLproc)__glDoStore_AD_CI,
    (__GLproc)__glDoStore_ASD_CI,
    (__GLproc)__glDoStore_ASD_CI,
};

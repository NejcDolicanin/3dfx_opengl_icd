/*
** gl_pickspan.c -- s_pick.c __glGenericPickSpanProcs: the span procs of the software rasteriser.
*/
#include <math.h>
#include <stdlib.h>
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* ---- s_pick.c: __glGenericPickSpanProcs ---- */
extern __GLtextureICD *__fastcall __glCurrentTexture(__GLcontext *gc, GLint unit);
extern void __fastcall __glNop1(void);
extern GLboolean __fastcall __glFogSpanSlow(__GLcontext *gc);
extern GLboolean __fastcall __glFogStippledSpanSlow(__GLcontext *gc);
extern GLboolean __fastcall __glStencilTestSpan(__GLcontext *gc);
extern GLboolean __fastcall __glStencilTestStippledSpan(__GLcontext *gc);
extern GLboolean __fastcall __glDepthTestOffsetSpan(__GLcontext *gc);
extern GLboolean __fastcall __glDepthTestStippledOffsetSpan(__GLcontext *gc);
extern GLboolean __fastcall __glDepthTestSpan(__GLcontext *gc);
extern GLboolean __fastcall __glDepthTestStippledSpan(__GLcontext *gc);
extern GLboolean __fastcall __glDepthTestStencilSpan(__GLcontext *gc);
extern GLboolean __fastcall __glDepthTestStencilStippledSpan(__GLcontext *gc);
extern GLboolean __fastcall __glDepthTestStencilOffsetSpan(__GLcontext *gc);
extern GLboolean __fastcall __glDepthTestStencilStippledOffsetSpan(__GLcontext *gc);
extern GLboolean __fastcall __glDepthPassSpan(__GLcontext *gc);
extern GLboolean __fastcall __glDepthPassStippledSpan(__GLcontext *gc);
extern GLboolean __fastcall __glShadeCISpan(__GLcontext *gc);
extern GLboolean __fastcall __glShadeRGBASpan(__GLcontext *gc);
extern GLboolean __fastcall __glTextureSpan(__GLcontext *gc);
extern GLboolean __fastcall __glTextureStippledSpan(__GLcontext *gc);
extern GLboolean __fastcall __glDitherCISpan(__GLcontext *gc);
extern GLboolean __fastcall __glDitherCIStippledSpan(__GLcontext *gc);
extern GLboolean __fastcall __glDitherRGBASpan(__GLcontext *gc);
extern GLboolean __fastcall __glDitherRGBAStippledSpan(__GLcontext *gc);
extern GLboolean __fastcall __glMaskRGBASpan(__GLcontext *gc);
extern GLboolean __fastcall __glMaskCISpan(__GLcontext *gc);
extern GLboolean __fastcall __glIndexTestSpan(__GLcontext *gc);
extern GLboolean __fastcall __glIndexTestStippledSpan(__GLcontext *gc);

#define SHADE_RGB                0x00000001
#define SHADE_SMOOTH             0x00000002
#define SHADE_DEPTH_TEST         0x00000004
#define SHADE_TEXTURE            0x00000008
#define SHADE_STIPPLE            0x00000010
#define SHADE_STENCIL_TEST       0x00000020
#define SHADE_DITHER             0x00000040
#define SHADE_LOGICOP            0x00000080
#define SHADE_BLEND              0x00000100
#define SHADE_ALPHA_TEST         0x00000200
#define SHADE_MASK               0x00000800
#define SHADE_SLOW_FOG           0x00001000
#define SHADE_POLYGON_OFFSET_FILL 0x00100000
#define SHADE_ICD_200000         0x00200000  /* a second per-fragment test, with the alpha test */

#define CFB_NEEDFRAGOPS(cfb)     (((__GLcolorBufferICD *)(cfb))->needColorFragmentOps)

/*
** 6907bb10 -- __glGenericPickSpanProcs (s_pick.c; gc->procs.pickSpanProcs).  The SI routine without
** the secondary-colour span; modeFlags come from the ICD's getModeFlags, the texture span asks
** 6909c760(gc, 0) first, and alpha test shares its phase with a second ICD test (mode bit 0x200000,
** span 690d1090).
*/
void __fastcall __glGenericPickSpanProcs(__GLcontext *gc)
{
    GLuint modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    __GLcolorBufferICD *cfb = gc->drawBuffer;
    __GLproc *sp;
    __GLproc *ssp;
    int spanCount;
    GLboolean replicateSpan;

    replicateSpan = GL_FALSE;
    sp = gc->procs.span.spanFuncs;
    ssp = gc->procs.span.stippledSpanFuncs;

    /* Load phase one procs */
    if (!gc->transform.reasonableViewport) {
        *sp++ = (__GLproc)__glClipSpan;
        *ssp++ = NULL;
    }

    if (modeFlags & SHADE_STIPPLE) {
        *sp++ = (__GLproc)__glStippleSpan;
        *ssp++ = (__GLproc)__glStippleStippledSpan;
    }

    /* Load phase two procs */
    if (!(modeFlags & (SHADE_ALPHA_TEST | SHADE_ICD_200000))) {
        if (modeFlags & SHADE_STENCIL_TEST) {
            *sp++ = (__GLproc)__glStencilTestSpan;
            *ssp++ = (__GLproc)__glStencilTestStippledSpan;
            if (modeFlags & SHADE_DEPTH_TEST) {
                if (modeFlags & SHADE_POLYGON_OFFSET_FILL) {
                    *sp = (__GLproc)__glDepthTestStencilOffsetSpan;
                    *ssp = (__GLproc)__glDepthTestStencilStippledOffsetSpan;
                } else {
                    *sp = (__GLproc)__glDepthTestStencilSpan;
                    *ssp = (__GLproc)__glDepthTestStencilStippledSpan;
                }
            } else {
                *sp = (__GLproc)__glDepthPassSpan;
                *ssp = (__GLproc)__glDepthPassStippledSpan;
            }
            sp++;
            ssp++;
        } else {
            if (modeFlags & SHADE_DEPTH_TEST) {
                if (gc->depthBuffer.testFunc == GL_NEVER) {
                    gc->procs.span.processSpan = (__GLproc)__glNop1;
                    return;
                }
                if (modeFlags & SHADE_POLYGON_OFFSET_FILL) {
                    *sp++ = (__GLproc)__glDepthTestOffsetSpan;
                    *ssp++ = (__GLproc)__glDepthTestStippledOffsetSpan;
                } else {
                    *sp++ = (__GLproc)__glDepthTestSpan;
                    *ssp++ = (__GLproc)__glDepthTestStippledSpan;
                }
            }
        }
    }

    /* Load phase three procs */
    if (modeFlags & SHADE_RGB) {
        if (modeFlags & SHADE_SMOOTH) {
            *sp = (__GLproc)__glShadeRGBASpan;
            *ssp = (__GLproc)__glShadeRGBASpan;
        } else {
            *sp = (__GLproc)__glFlatRGBASpan;
            *ssp = (__GLproc)__glFlatRGBASpan;
        }
    } else {
        if (modeFlags & SHADE_SMOOTH) {
            *sp = (__GLproc)__glShadeCISpan;
            *ssp = (__GLproc)__glShadeCISpan;
        } else {
            *sp = (__GLproc)__glFlatCISpan;
            *ssp = (__GLproc)__glFlatCISpan;
        }
    }
    sp++;
    ssp++;
    if (modeFlags & SHADE_TEXTURE) {
        __glCurrentTexture(gc, 0);                      /* result unused (as in the original) */
        *sp = (__GLproc)__glTextureSpan;
        *ssp = (__GLproc)__glTextureStippledSpan;
        sp++;
        ssp++;
    }
    if (modeFlags & SHADE_SLOW_FOG) {
        if (gc->state.hints.fog == GL_NICEST) {
            *sp = (__GLproc)__glFogSpanSlow;
            *ssp = (__GLproc)__glFogStippledSpanSlow;
        } else {
            *sp = (__GLproc)__glFogSpan;
            *ssp = (__GLproc)__glFogStippledSpan;
        }
        sp++;
        ssp++;
    }

    if (modeFlags & (SHADE_ALPHA_TEST | SHADE_ICD_200000)) {
        if (modeFlags & SHADE_ALPHA_TEST) {
            *sp++ = (__GLproc)__glAlphaTestSpan;
            *ssp++ = (__GLproc)__glAlphaTestStippledSpan;
        } else {
            *sp++ = (__GLproc)__glIndexTestSpan;
            *ssp++ = (__GLproc)__glIndexTestStippledSpan;
        }

        /* If alpha test is on, need to do the depth/stencil tests now. */
        if (modeFlags & SHADE_STENCIL_TEST) {
            *sp++ = (__GLproc)__glStencilTestSpan;
            *ssp++ = (__GLproc)__glStencilTestStippledSpan;
            if (modeFlags & SHADE_DEPTH_TEST) {
                if (modeFlags & SHADE_POLYGON_OFFSET_FILL) {
                    *sp = (__GLproc)__glDepthTestStencilOffsetSpan;
                    *ssp = (__GLproc)__glDepthTestStencilStippledOffsetSpan;
                } else {
                    *sp = (__GLproc)__glDepthTestStencilSpan;
                    *ssp = (__GLproc)__glDepthTestStencilStippledSpan;
                }
            } else {
                *sp = (__GLproc)__glDepthPassSpan;
                *ssp = (__GLproc)__glDepthPassStippledSpan;
            }
            sp++;
            ssp++;
        } else {
            if (modeFlags & SHADE_DEPTH_TEST) {
                if (gc->depthBuffer.testFunc == GL_NEVER) {
                    gc->procs.span.processSpan = (__GLproc)__glNop1;
                    return;
                }
                if (modeFlags & SHADE_POLYGON_OFFSET_FILL) {
                    *sp++ = (__GLproc)__glDepthTestOffsetSpan;
                    *ssp++ = (__GLproc)__glDepthTestStippledOffsetSpan;
                } else {
                    *sp++ = (__GLproc)__glDepthTestSpan;
                    *ssp++ = (__GLproc)__glDepthTestStippledSpan;
                }
            }
        }
    }

    if (gc->buffers.doubleStore) {
        spanCount = (int)(sp - gc->procs.span.spanFuncs);
        gc->procs.span.n = spanCount;
        replicateSpan = GL_TRUE;
    }

    /* Load phase four procs */
    if (CFB_NEEDFRAGOPS(cfb)) {
        if (modeFlags & (SHADE_LOGICOP | SHADE_MASK)) {
            *sp++ = cfb->fetchSpan;
            *ssp++ = cfb->fetchStippledSpan;
        }
        if (modeFlags & SHADE_BLEND) {
            GLenum s = gc->state.raster.blendSrc;
            GLenum d = gc->state.raster.blendDst;

            if ((~modeFlags & SHADE_MASK) &&
                (d != GL_ZERO ||
                 s == GL_DST_COLOR || s == GL_ONE_MINUS_DST_COLOR ||
                 s == GL_DST_ALPHA || s == GL_ONE_MINUS_DST_ALPHA ||
                 s == GL_SRC_ALPHA_SATURATE)) {
                *sp++ = cfb->fetchSpan;
                *ssp++ = cfb->fetchStippledSpan;
            }
            if (s == GL_SRC_ALPHA) {
                if (d == GL_ONE_MINUS_SRC_ALPHA) {
                    *sp = (__GLproc)__glBlendSpan_SA_MSA;
                } else if (d == GL_ONE) {
                    *sp = (__GLproc)__glBlendSpan_SA_ONE;
                } else if (d == GL_ZERO) {
                    *sp = (__GLproc)__glBlendSpan_SA_ZERO;
                } else {
                    *sp = (__GLproc)__glBlendSpan;
                }
            } else if (s == GL_ONE_MINUS_SRC_ALPHA && d == GL_SRC_ALPHA) {
                *sp = (__GLproc)__glBlendSpan_MSA_SA;
            } else {
                *sp = (__GLproc)__glBlendSpan;
            }
            sp++;
            *ssp++ = (__GLproc)__glBlendStippledSpan;
        }
        if (modeFlags & SHADE_DITHER) {
            if (modeFlags & SHADE_RGB) {
                *sp = (__GLproc)__glDitherRGBASpan;
                *ssp = (__GLproc)__glDitherRGBAStippledSpan;
            } else {
                *sp = (__GLproc)__glDitherCISpan;
                *ssp = (__GLproc)__glDitherCIStippledSpan;
            }
        } else {
            if (modeFlags & SHADE_RGB) {
                *sp = (__GLproc)__glRoundRGBASpan;
                *ssp = (__GLproc)__glRoundRGBAStippledSpan;
            } else {
                *sp = (__GLproc)__glRoundCISpan;
                *ssp = (__GLproc)__glRoundCIStippledSpan;
            }
        }
        sp++;
        ssp++;
        if (modeFlags & SHADE_LOGICOP) {
            *sp++ = (__GLproc)__glLogicOpSpan;
            *ssp++ = (__GLproc)__glLogicOpStippledSpan;
        }
        if (modeFlags & SHADE_MASK) {
            if (modeFlags & SHADE_RGB) {
                *sp = (__GLproc)__glMaskRGBASpan;
                *ssp = (__GLproc)__glMaskRGBASpan;
            } else {
                *sp = (__GLproc)__glMaskCISpan;
                *ssp = (__GLproc)__glMaskCISpan;
            }
            sp++;
            ssp++;
        }
    }

    /* Finally, copy over procs from drawBuffer */
    *sp++ = cfb->storeSpan;
    *ssp++ = cfb->storeStippledSpan;

    spanCount = (int)(sp - gc->procs.span.spanFuncs);
    gc->procs.span.m = spanCount;
    if (replicateSpan) {
        gc->procs.span.processSpan = (__GLproc)__glProcessReplicateSpan;
    } else {
        gc->procs.span.processSpan = (__GLproc)__glProcessSpan;
        gc->procs.span.n = spanCount;
    }
}

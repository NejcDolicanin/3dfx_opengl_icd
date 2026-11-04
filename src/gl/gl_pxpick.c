/*
** gl_pxpick.c -- PickSpanModifiers: the ICD's span-modifier picker for DrawPixels / ReadPixels /
** CopyPixels / CopyImage.  It is the older SI's single function: the SI 1.2 helpers
** (PickFast*Modifiers, PickSpanUnpack / Read / Pack / Render) are inline here, the per-transfer
** flags are locals, the fast DrawPixels / ReadPixels paths come after the generic pick and replace
** its last step, and the reader / renderer are handed back through the mod info.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "glsitypes.h"     /* __GLpixelSpanModInfo, __GL_PIXDATA_*, __GL_PIXPATH_* */

typedef __GLpixelSpanInfoICD __GLpixelSpanInfo;

#define __GL_RED_ALPHA          1
#ifndef GL_BGR_EXT
#define GL_BGR_EXT              0x80E0
#define GL_BGRA_EXT             0x80E1
#endif
#ifndef GL_INTENSITY
#define GL_INTENSITY            0x8049
#endif
#define GL_UNSIGNED_BYTE_3_3_2_EXT      0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4_EXT   0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1_EXT   0x8034
#define GL_UNSIGNED_INT_8_8_8_8_EXT     0x8035
#define GL_UNSIGNED_INT_10_10_10_2_EXT  0x8036
/* the ICD's internal packed texel types (SI 1.2 names where it has them; by their (un)packers) */
#define __GL_UNSIGNED_SHORT_5_6_5               0
#define __GL_UNSIGNED_SHORT_X_5_5_5             1
#define __GL_UNSIGNED_SHORT_1_5_5_5             2
#define __GL_UNSIGNED_SHORT_4_4_4_4_ARGB        3
#define __GL_UNSIGNED_INT_8_8_8_8_ARGB          4       /* __glSpanUnpack8888RevUint */
#define __GL_UNSIGNED_INT_TYPE_5                5       /* unpacks like 4; own packer 690c7900 */

/* spanInfo->applyFlags (set by __glInitUnpacker / __glInitPacker) */
#define __GL_APPLY_ALPHA_ONE(si)        ((si)->applyFlags[0])   /* 690cb5a0 after expanding an alpha-less format */
#define __GL_APPLY_SRC_CLAMP(si)        ((si)->applyFlags[1])   /* SI applySrcClamp */
#define __GL_APPLY_FB_SCALE(si)         ((si)->applyFlags[2])   /* SI applyFbScale */
#define __GL_NON_COLOR_COMP(si)         ((si)->applyFlags[3])   /* SI nonColorComp */

/* gc->depthBuffer.buf.other is the WGL depth buffer; its depthPrivate holds the z offset (wgldraw.h) */
#define __GL_DEPTH_DP(db)       ((__WGLdepthPrivate *)((db)->buf.wglBuf)->depthPrivate)

typedef void (__fastcall *__GLspanModProc)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
                                           const GLvoid *inspan, GLvoid *outspan);
/* 1.0 is read from memory (no folding of zoomx = 1 into the zoomx2 test) */
static const __GLfloat __glOne = 1.0f;

#define ADD(f)      (spanInfo->spanModifier[spanCount++] = (__GLspanModProc)(f))
extern GLboolean __fastcall isPackedType(GLenum type);

/* the span procs si_names.h has no SI name for */
extern void __fastcall __glSpanPackUbyteRound(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *, GLvoid *);
extern void __fastcall __glSpanPackXRGB8888Uint(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *, GLvoid *);
extern void __fastcall __glSpanPackRGBX8ToRGB565(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *, GLvoid *);
extern void __fastcall __glSpanPackRGB8ToXRGB8888(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *, GLvoid *);
extern void __fastcall __glSpanPackRGBA8ToARGB8888(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *, GLvoid *);
extern void __fastcall __glSpanPackRGBA8ToXRGB8888(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *, GLvoid *);
extern void __fastcall __glSpanPackRGBA5551ToARGB1555(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *, GLvoid *);
extern void __fastcall __glSpanPackRGBA5551ToXRGB1555(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *, GLvoid *);
extern void __fastcall __glSpanPackRGBA4444ToARGB4444(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *, GLvoid *);
extern void __fastcall __glSpanPackRGBA8888ToARGB8888(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *, GLvoid *);
extern void __fastcall __glSpanPackRGBA8888ToXRGB8888(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *, GLvoid *);
extern void __fastcall __glSpanZeroFillAlpha(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *, GLvoid *);
extern void __fastcall __glSpanRenderDepthUint(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *);
extern void __fastcall __glSpanRenderDepthUint2(__GLcontext *, __GLpixelSpanInfo *, const GLvoid *);
extern void __fastcall __glSpanReadDepthUint2(__GLcontext *, __GLpixelSpanInfo *, GLvoid *);

typedef void (__fastcall *__GLdrawPixelsProcPick)(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
void __fastcall PickSpanModifiers(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, __GLpixelSpanModInfo *spanModInfo);
extern void __fastcall __glDrawPixelsN(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);   /* __glDrawPixels3 */
extern void __fastcall __glDrawPixels2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
extern void __fastcall __glDrawPixels1(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);
extern void __fastcall __glDrawPixels0(__GLcontext *gc, __GLpixelSpanInfo *spanInfo);

/* (compiling a function in front of PickSpanModifiers, as px_paths.c does, also changes VC6's
** tail merging inside it: alone in the file it duplicates three instructions of the CI reader arm) */
/* 69086aa0 -- __glGenericPickDrawPixels (px_paths.c): pixel transfer on, the render proc taken from
** the pick, no convolution path */
void __fastcall __glGenericPickDrawPixels(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    __GLdrawPixelsProcPick dpfn;
    __GLpixelSpanModInfo spanModInfo;

    /* in this order: the stores of the shared 1 then come out in the original's order */
    spanModInfo.srcData = __GL_PIXDATA_MEM;
    spanModInfo.dstData = __GL_PIXDATA_FB;
    spanModInfo.pixelPath = __GL_PIXPATH_DRAWPIX;
    spanModInfo.applyPixelTransfer = GL_TRUE;

    spanInfo->numSpanMods = 0;
    PickSpanModifiers(gc, spanInfo, &spanModInfo);
    spanInfo->spanRender = (void (__fastcall *)(__GLcontext *, __GLpixelSpanInfo *,
						const GLvoid *))spanModInfo.render;

    /*
    ** Pick a DrawPixels function that applies the correct number of 
    ** span modifiers.
    */
    switch(spanInfo->numSpanMods) {
    case 0:
	dpfn = __glDrawPixels0;
	break;
    case 1:
	dpfn = __glDrawPixels1;
	break;
    case 2:
	dpfn = __glDrawPixels2;
	break;
    default:
	dpfn = __glDrawPixelsN;
	break;
    }

    (*dpfn)(gc, spanInfo);
}

/* 69086b40 -- PickSpanModifiers: build the span-modifier chain for a pixel transfer described by spanInfo and the
** caller's mod info (source / destination memory or framebuffer, pixel path, applyPixelTransfer), appending to
** spanInfo->spanModifier from numSpanMods on.  It first derives the per-transfer flags (x zoom, packed user data,
** group skipping, byte swap, misalignment, float conversion, clamping, pixel-transfer modify, RGBA expand / reduce);
** CopyImage of unsigned bytes or matching packed types without any of those takes a single direct packer (or none
** for a trivial copy).  Otherwise: unpack (swap / align / skip, convert to float, clamp, bitmap, expand or modify
** per format) or pick the framebuffer reader (procs.pixel.spanRead*, the *2 form when |zoomx| >= 1), then reduce /
** clamp / pack / swap / align for memory or pick the spanRender* proc (*2 when |zoomx| <= 1).  Fast paths:
** ReadPixels of depth as GL_UNSIGNED_INT with a 2^n-1 scale reads straight with __glSpanReadDepthUint2, dropping one
** modifier before the pack stage adds its packer (which thereby lands in spanModifier[-1], the spanReader slot
** __glGenericPickReadPixels then overwrites); DrawPixels of ubyte RGB / RGBA / stencil / CI, ushort stencil / CI and
** uint depth / CI render the user data directly (selecting the pixel-map tables, building the modify tables when
** stale).  Hands back the reader and render in spanModInfo and the new numSpanMods */
void __fastcall PickSpanModifiers(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             __GLpixelSpanModInfo *spanModInfo)
{
    __GLpixelMachineICD *pm = &gc->pixel;
    GLenum srcFormat, srcType, dstFormat, dstType;
    GLint spanCount;
    __GLfloat zoomx;
    GLboolean zoomx1, zoomx2, packedUserData, skipGroups, srcSwap, dstSwap;
    GLboolean srcAlign, dstAlign, srcConvert, dstConvert, srcClamp, dstClamp;
    GLboolean modify, srcExpand, dstReduce, dstIndex;
    __GLproc reader, render;

    dstType = spanInfo->dstType;
    dstFormat = spanInfo->dstFormat;
    spanCount = spanInfo->numSpanMods;
    srcFormat = spanInfo->srcFormat;
    srcType = spanInfo->srcType;

    /*
    ** Examine pixel zoom factors if the framebuffer is the destination
    */
    if (spanModInfo->dstData == __GL_PIXDATA_FB) {
	zoomx = gc->state.pixel.transferMode.zoomX;
	if (zoomx >= (__GLfloat) -1.0 && zoomx <= __glOne) {
	    zoomx1 = GL_TRUE;
	} else {
	    goto zoomOut;
	}
    } else {
	zoomx1 = GL_TRUE;
	zoomx = __glOne;
    }
    if (0) {
zoomOut:					/* the original places this block after the else arm */
	zoomx1 = GL_FALSE;
    }
    if (zoomx <= (__GLfloat) -1.0 || zoomx >= __glOne) {
	zoomx2 = GL_TRUE;
    } else {
	zoomx2 = GL_FALSE;
    }
    if (spanInfo->srcPackedData && zoomx2) {
	packedUserData = GL_TRUE;
    } else {
	packedUserData = GL_FALSE;
    }
    if (zoomx2 || srcType == GL_BITMAP) {
	skipGroups = GL_FALSE;
    } else {
	skipGroups = GL_TRUE;
    }

    /*
    ** Byte swapping
    */
    if (spanInfo->srcSwapBytes && spanInfo->srcElementSize > 1) {
	srcSwap = GL_TRUE;
    } else {
	srcSwap = GL_FALSE;
    }
    if (spanInfo->dstSwapBytes && spanInfo->dstElementSize > 1) {
	dstSwap = GL_TRUE;
    } else {
	dstSwap = GL_FALSE;
    }

    /*
    ** Pixel transfer
    */
    switch (srcFormat) {
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case __GL_RED_ALPHA:
      case GL_ABGR_EXT:
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
	if (spanModInfo->applyPixelTransfer && pm->modifyRGBA) {
	    modify = GL_TRUE;
	} else {
	    modify = GL_FALSE;
	}
	break;
      case GL_DEPTH_COMPONENT:
	if (spanModInfo->applyPixelTransfer && pm->modifyDepth) {
	    modify = GL_TRUE;
	} else {
	    modify = GL_FALSE;
	}
	break;
      case GL_STENCIL_INDEX:
	if (spanModInfo->applyPixelTransfer && pm->modifyStencil) {
	    modify = GL_TRUE;
	} else {
	    modify = GL_FALSE;
	}
	break;
      case GL_COLOR_INDEX:
	if (spanModInfo->applyPixelTransfer && pm->modifyCI) {
	    modify = GL_TRUE;
	} else {
	    modify = GL_FALSE;
	}
	break;
    }

    /*
    ** Expansion to RGBA
    */
    if (spanModInfo->pixelPath == __GL_PIXPATH_COPYIMAGE && !modify &&
	    (srcFormat == dstFormat ||
	     (srcFormat == GL_LUMINANCE_ALPHA && dstFormat == __GL_RED_ALPHA) ||
	     (srcFormat == __GL_RED_ALPHA && dstFormat == GL_LUMINANCE_ALPHA) ||
	     (srcFormat == GL_LUMINANCE && dstFormat == GL_RED) ||
	     (srcFormat == GL_RED && dstFormat == GL_LUMINANCE))) {
	srcExpand = GL_FALSE;
	dstReduce = GL_FALSE;
    } else {
	srcExpand = GL_TRUE;
	dstReduce = GL_TRUE;
    }

    /*
    ** Handling mis-aligned data access
    */
    if (srcType != GL_BITMAP &&
	    (((unsigned long) spanInfo->srcImage) & (spanInfo->srcElementSize - 1))) {
	srcAlign = GL_TRUE;
    } else {
	srcAlign = GL_FALSE;
    }
    if (dstType != GL_BITMAP &&
	    (((unsigned long) spanInfo->dstImage) & (spanInfo->dstElementSize - 1))) {
	dstAlign = GL_TRUE;
    } else {
	dstAlign = GL_FALSE;
    }

    /*
    ** Conversion to/from floating point
    */
    if (srcType == GL_FLOAT || __GL_NON_COLOR_COMP(spanInfo) ||
	    (spanModInfo->pixelPath != __GL_PIXPATH_COPYIMAGE && dstType == GL_BITMAP)) {
	srcConvert = GL_FALSE;
    } else {
	srcConvert = GL_TRUE;
    }
    if (dstType == GL_FLOAT || __GL_NON_COLOR_COMP(spanInfo) ||
	    (spanModInfo->pixelPath != __GL_PIXPATH_COPYIMAGE && dstType == GL_BITMAP)) {
	dstConvert = GL_FALSE;
    } else {
	dstConvert = GL_TRUE;
    }

    /*
    ** Clamp before / after applying pixel transfer operations
    */
    if (!__GL_APPLY_SRC_CLAMP(spanInfo) ||
	    spanModInfo->srcData == __GL_PIXDATA_FB ||
	    srcType == GL_BITMAP ||
	    srcType == GL_UNSIGNED_BYTE ||
	    srcType == GL_UNSIGNED_SHORT ||
	    srcType == GL_UNSIGNED_INT ||
	    srcFormat == GL_COLOR_INDEX ||
	    srcFormat == GL_STENCIL_INDEX ||
	    (srcFormat == GL_DEPTH_COMPONENT && pm->modifyDepth) ||
	    (srcFormat != GL_DEPTH_COMPONENT && pm->modifyRGBA)) {
	srcClamp = GL_FALSE;
    } else {
	srcClamp = GL_TRUE;
    }
    if (spanModInfo->applyPixelTransfer && srcClamp) {
	dstClamp = GL_TRUE;
    } else {
	dstClamp = GL_FALSE;
    }

    /*
    ** CopyImage fast paths: trivial copies and packed downloads
    */
    if (spanModInfo->pixelPath == __GL_PIXPATH_COPYIMAGE &&
	    srcType == GL_UNSIGNED_BYTE &&
	    srcFormat != GL_BGRA_EXT && srcFormat != GL_BGR_EXT && srcFormat != GL_ABGR_EXT &&
	    (dstType == GL_UNSIGNED_BYTE || isPackedType(dstType)) &&
	    !skipGroups && !modify && !srcAlign && !dstAlign && !srcClamp && !dstClamp) {
	if (dstType == GL_UNSIGNED_BYTE) {
	    if (!srcExpand && !dstReduce) {
		goto done;
	    }
	} else {
	    switch (dstType) {
	      case __GL_UNSIGNED_SHORT_5_6_5:
		switch (spanInfo->srcComponents) {
		  case 1: ADD(__glSpanPackL8ToRGB565); goto done;
		  case 3: ADD(__glSpanPackRGB8ToRGB565); goto done;
		  case 4: ADD(__glSpanPackRGBX8ToRGB565); goto done;
		}
		break;
	      case __GL_UNSIGNED_SHORT_X_5_5_5:
		switch (spanInfo->srcComponents) {
		  case 1: ADD(__glSpanPackL8ToXRGB1555); goto done;
		  case 3: ADD(__glSpanPackRGB8ToXRGB1555); goto done;
		  case 4: ADD(__glSpanPackRGBA8ToXRGB1555); goto done;
		}
		break;
	      case __GL_UNSIGNED_SHORT_1_5_5_5:
		switch (spanInfo->srcComponents) {
		  case 2: ADD(__glSpanPackLA8ToARGB1555); goto done;
		  case 4: ADD(__glSpanPackRGBA8ToARGB1555); goto done;
		}
		break;
	      case __GL_UNSIGNED_SHORT_4_4_4_4_ARGB:
		switch (spanInfo->srcComponents) {
		  case 2: ADD(__glSpanPackLA8ToARGB4); goto done;
		  case 4: ADD(__glSpanPackRGBA8ToARGB4); goto done;
		}
		break;
	      case __GL_UNSIGNED_INT_8_8_8_8_ARGB:
		switch (spanInfo->srcComponents) {
		  case 3: ADD(__glSpanPackRGB8ToXRGB8888); goto done;
		  case 4: ADD(__glSpanPackRGBA8ToARGB8888); goto done;
		}
		break;
	      case __GL_UNSIGNED_INT_TYPE_5:
		switch (spanInfo->srcComponents) {
		  case 3: ADD(__glSpanPackRGB8ToXRGB8888); goto done;
		  case 4: ADD(__glSpanPackRGBA8ToXRGB8888); goto done;
		}
		break;
	    }
	}
    }
    if (spanModInfo->pixelPath == __GL_PIXPATH_COPYIMAGE &&
	    isPackedType(srcType) && isPackedType(dstType) &&
	    srcFormat == GL_RGBA &&
	    !skipGroups && !modify && !srcAlign && !dstAlign) {
	if (srcType == GL_UNSIGNED_SHORT_5_5_5_1_EXT) {
	    if (dstType == __GL_UNSIGNED_SHORT_1_5_5_5) {
		ADD(__glSpanPackRGBA5551ToARGB1555);
		goto done;
	    } else if (dstType == __GL_UNSIGNED_SHORT_X_5_5_5) {
		ADD(__glSpanPackRGBA5551ToXRGB1555);
		goto done;
	    }
	} else if (srcType == GL_UNSIGNED_SHORT_4_4_4_4_EXT) {
	    if (dstType == __GL_UNSIGNED_SHORT_4_4_4_4_ARGB) {
		ADD(__glSpanPackRGBA4444ToARGB4444);
		goto done;
	    }
	} else if (srcType == GL_UNSIGNED_INT_8_8_8_8_EXT) {
	    if (dstType == __GL_UNSIGNED_INT_8_8_8_8_ARGB) {
		ADD(__glSpanPackRGBA8888ToARGB8888);
		goto done;
	    } else if (dstType == __GL_UNSIGNED_INT_TYPE_5) {
		ADD(__glSpanPackRGBA8888ToXRGB8888);
		goto done;
	    }
	}
    }

    switch (dstFormat) {
      case GL_COLOR_INDEX:
      case GL_STENCIL_INDEX:
	dstIndex = GL_TRUE;
	break;
      case GL_DEPTH_COMPONENT:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case __GL_RED_ALPHA:
      case GL_ABGR_EXT:
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
	dstIndex = GL_FALSE;
	break;
    }

    /*
    ** Unpack / read
    */
    switch (spanModInfo->srcData) {
      case __GL_PIXDATA_MEM:
	if (srcSwap) {
	    if (skipGroups) {
		if (spanInfo->srcElementSize == 2) {
		    ADD(__glSpanSwapAndSkipBytes2);
		} else {
		    ADD(__glSpanSwapAndSkipBytes4);
		}
	    } else {
		if (spanInfo->srcElementSize == 2) {
		    ADD(__glSpanSwapBytes2);
		} else {
		    ADD(__glSpanSwapBytes4);
		}
	    }
	} else if (srcAlign) {
	    if (skipGroups) {
		if (spanInfo->srcElementSize == 2) {
		    ADD(__glSpanSlowSkipPixels2);
		} else {
		    ADD(__glSpanSlowSkipPixels4);
		}
	    } else {
		if (spanInfo->srcElementSize == 2) {
		    ADD(__glSpanAlignPixels2);
		} else {
		    ADD(__glSpanAlignPixels4);
		}
	    }
	} else if (skipGroups) {
	    if (spanInfo->srcElementSize == 1) {
		ADD(__glSpanSkipPixels1);
	    } else if (spanInfo->srcElementSize == 2) {
		ADD(__glSpanSkipPixels2);
	    } else {
		ADD(__glSpanSkipPixels4);
	    }
	}

	if (srcConvert) {
	    if (srcFormat == GL_COLOR_INDEX || srcFormat == GL_STENCIL_INDEX) {
		switch (srcType) {
		  case GL_BYTE:			ADD(__glSpanUnpackByteI); break;
		  case GL_UNSIGNED_BYTE:	ADD(__glSpanUnpackUbyteI); break;
		  case GL_SHORT:		ADD(__glSpanUnpackShortI); break;
		  case GL_UNSIGNED_SHORT:	ADD(__glSpanUnpackUshortI); break;
		  case GL_INT:			ADD(__glSpanUnpackIntI); break;
		  case GL_UNSIGNED_INT:		ADD(__glSpanUnpackUintI); break;
		}
	    } else {
		switch (srcType) {
		  case __GL_UNSIGNED_SHORT_4_4_4_4_ARGB:
		    ADD(__glSpanUnpackARGB4444Ushort);
		    spanInfo->srcComponents = 4;
		    break;
		  case __GL_UNSIGNED_SHORT_X_5_5_5:
		    ADD(__glSpanUnpackX555Ushort);
		    spanInfo->srcComponents = 3;
		    break;
		  case __GL_UNSIGNED_SHORT_1_5_5_5:
		    ADD(__glSpanUnpack1555Ushort);
		    spanInfo->srcComponents = 4;
		    break;
		  case __GL_UNSIGNED_SHORT_5_6_5:
		    ADD(__glSpanUnpack565Ushort);
		    spanInfo->srcComponents = 3;
		    break;
		  case __GL_UNSIGNED_INT_8_8_8_8_ARGB:
		  case __GL_UNSIGNED_INT_TYPE_5:
		    ADD(__glSpanUnpack8888RevUint);
		    spanInfo->srcComponents = 4;
		    break;
		  case GL_BYTE:			ADD(__glSpanUnpackByte); break;
		  case GL_UNSIGNED_BYTE:	ADD(__glSpanUnpackUbyte); break;
		  case GL_SHORT:		ADD(__glSpanUnpackShort); break;
		  case GL_UNSIGNED_SHORT:	ADD(__glSpanUnpackUshort); break;
		  case GL_INT:			ADD(__glSpanUnpackInt); break;
		  case GL_UNSIGNED_INT:		ADD(__glSpanUnpackUint); break;
		  case GL_UNSIGNED_BYTE_3_3_2_EXT:
		    ADD(__glSpanUnpack332Ubyte);
		    spanInfo->srcComponents = 3;
		    break;
		  case GL_UNSIGNED_SHORT_4_4_4_4_EXT:
		    ADD(__glSpanUnpack4444Ushort);
		    spanInfo->srcComponents = 4;
		    break;
		  case GL_UNSIGNED_SHORT_5_5_5_1_EXT:
		    ADD(__glSpanUnpack5551Ushort);
		    spanInfo->srcComponents = 4;
		    break;
		  case GL_UNSIGNED_INT_8_8_8_8_EXT:
		    ADD(__glSpanUnpack8888Uint);
		    spanInfo->srcComponents = 4;
		    break;
		  case GL_UNSIGNED_INT_10_10_10_2_EXT:
		    ADD(__glSpanUnpack_10_10_10_2_Uint);
		    spanInfo->srcComponents = 4;
		    break;
		}
	    }
	} else if (__GL_NON_COLOR_COMP(spanInfo)) {
	    ADD(__glSpanUnpackNonCompUint);
	}

	if (srcClamp) {
	    ADD(__glSpanClampFloat);
	}

	if (srcType == GL_BITMAP) {
	    if (zoomx2) {
		ADD(__glSpanUnpackBitmap2);
	    } else {
		ADD(__glSpanUnpackBitmap);
	    }
	}

	if (srcExpand) {
	    switch (srcFormat) {
	      case GL_COLOR_INDEX:
		if (pm->modifyCI) {
		    ADD(__glSpanModifyCI);
		}
		break;
	      case __GL_RED_ALPHA:
		if (modify) {
		    ADD(__glSpanModifyRedAlpha);
		} else {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanExpandRedAlpha : __glSpanExpandRedAlphaNS);
		}
		break;
	      case GL_STENCIL_INDEX:
		if (pm->modifyStencil) {
		    ADD(__glSpanModifyStencil);
		}
		break;
	      case GL_DEPTH_COMPONENT:
		if (pm->modifyDepth) {
		    ADD(__glSpanModifyDepth);
		}
		break;
	      case GL_RED:
		if (pm->modifyRGBA) {
		    ADD(__glSpanModifyRed);
		} else {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanExpandRed : __glSpanExpandRedNS);
		}
		if (__GL_APPLY_ALPHA_ONE(spanInfo)) {
		    ADD(__glSpanZeroFillAlpha);
		}
		break;
	      case GL_ALPHA:
		if (pm->modifyRGBA) {
		    ADD(__glSpanModifyAlpha);
		} else {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanExpandAlpha : __glSpanExpandAlphaNS);
		}
		break;
	      case GL_BLUE:
		if (pm->modifyRGBA) {
		    ADD(__glSpanModifyBlue);
		} else {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanExpandBlue : __glSpanExpandBlueNS);
		}
		if (__GL_APPLY_ALPHA_ONE(spanInfo)) {
		    ADD(__glSpanZeroFillAlpha);
		}
		break;
	      case GL_GREEN:
		if (pm->modifyRGBA) {
		    ADD(__glSpanModifyGreen);
		} else {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanExpandGreen : __glSpanExpandGreenNS);
		}
		if (__GL_APPLY_ALPHA_ONE(spanInfo)) {
		    ADD(__glSpanZeroFillAlpha);
		}
		break;
	      case GL_RGB:
		if (pm->modifyRGBA) {
		    ADD(__glSpanModifyRGB);
		} else {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanExpandRGB : __glSpanExpandRGBNS);
		}
		if (__GL_APPLY_ALPHA_ONE(spanInfo)) {
		    ADD(__glSpanZeroFillAlpha);
		}
		break;
	      case GL_LUMINANCE_ALPHA:
		if (pm->modifyRGBA) {
		    ADD(__glSpanModifyLuminanceAlpha);
		} else {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanExpandLuminanceAlpha : __glSpanExpandLuminanceAlphaNS);
		}
		break;
	      case GL_LUMINANCE:
		if (pm->modifyRGBA) {
		    ADD(__glSpanModifyLuminance);
		} else {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanExpandLuminance : __glSpanExpandLuminanceNS);
		}
		if (__GL_APPLY_ALPHA_ONE(spanInfo)) {
		    ADD(__glSpanZeroFillAlpha);
		}
		break;
	      case GL_RGBA:
		if (pm->modifyRGBA) {
		    ADD(__glSpanModifyRGBA);
		} else if (__GL_APPLY_FB_SCALE(spanInfo)) {
		    ADD(__glSpanScaleRGBA);
		}
		break;
	      case GL_ABGR_EXT:
		if (pm->modifyRGBA) {
		    ADD(__glSpanModifyABGR);
		} else {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanScaleABGR : __glSpanPreReorderABGR);
		}
		break;
	      case GL_BGRA_EXT:
		if (pm->modifyRGBA) {
		    ADD(__glSpanModifyBGRA);
		} else {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanScaleBGRA : __glSpanPreReorderBGRA);
		}
		break;
	      case GL_BGR_EXT:
		if (pm->modifyRGBA) {
		    ADD(__glSpanModifyBGR);
		} else {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanExpandBGR : __glSpanExpandBGRNS);
		}
		if (__GL_APPLY_ALPHA_ONE(spanInfo)) {
		    ADD(__glSpanZeroFillAlpha);
		}
		break;
	      case GL_INTENSITY:
		ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanExpandIntensity : __glSpanExpandIntensityNS);
		if (__GL_APPLY_ALPHA_ONE(spanInfo)) {
		    ADD(__glSpanZeroFillAlpha);
		}
		break;
	    }
	}
	break;

      case __GL_PIXDATA_FB:
	switch (dstFormat) {
	  case GL_RGB:
	  case GL_RED:
	  case GL_GREEN:
	  case GL_BLUE:
	  case GL_LUMINANCE:
	  case GL_LUMINANCE_ALPHA:
	  case GL_ALPHA:
	  case GL_RGBA:
	  case GL_ABGR_EXT:
	  case GL_BGR_EXT:
	  case GL_BGRA_EXT:
	  case GL_INTENSITY:
	  case __GL_RED_ALPHA:
	    if (gc->modes.rgbMode) {
		if (zoomx2) {
		    reader = gc->procs.pixel.spanReadRGBA2;
		} else {
		    reader = gc->procs.pixel.spanReadRGBA;
		}
		if (pm->modifyRGBA) {
		    ADD(__glSpanPreUnscaleRGBA);
		    ADD(__glSpanModifyRGBA);
		}
	    } else {
		if (zoomx2) {
		    reader = gc->procs.pixel.spanReadCI2;
		} else {
		    reader = gc->procs.pixel.spanReadCI;
		}
		ADD(__glSpanModifyCI);
	    }
	    break;
	  case GL_DEPTH_COMPONENT:
	    if (zoomx2) {
		reader = gc->procs.pixel.spanReadDepth2;
	    } else {
		reader = gc->procs.pixel.spanReadDepth;
	    }
	    if (pm->modifyDepth) {
		ADD(__glSpanModifyDepth);
	    }
	    break;
	  case GL_STENCIL_INDEX:
	    if (zoomx2) {
		reader = gc->procs.pixel.spanReadStencil2;
	    } else {
		reader = gc->procs.pixel.spanReadStencil;
	    }
	    if (pm->modifyStencil) {
		ADD(__glSpanModifyStencil);
	    }
	    break;
	  case GL_COLOR_INDEX:
	    if (zoomx2) {
		reader = gc->procs.pixel.spanReadCI2;
	    } else {
		reader = gc->procs.pixel.spanReadCI;
	    }
	    if (pm->modifyCI) {
		ADD(__glSpanModifyCI);
	    }
	    break;
	}

	/* ReadPixels fast path: depth straight to unsigned ints */
	if (spanModInfo->pixelPath == __GL_PIXPATH_READPIX) {
	    switch (dstType) {
	      case GL_UNSIGNED_BYTE:
		switch (dstFormat) {
		  case GL_RGB:
		    break;
		  case GL_RGBA:
		    break;
		  case GL_STENCIL_INDEX:
		    break;
		  case GL_COLOR_INDEX:
		    break;
		  default:
		    break;
		}
		break;
	      case GL_UNSIGNED_SHORT:
		switch (dstFormat) {
		  case GL_STENCIL_INDEX:
		    break;
		  case GL_COLOR_INDEX:
		    break;
		  default:
		    break;
		}
		break;
	      case GL_UNSIGNED_INT:
		switch (dstFormat) {
		  case GL_DEPTH_COMPONENT:
		    if (!pm->modifyDepth) {
			GLuint scale = gc->depthBuffer.scale >>
			    __GL_DEPTH_DP(&gc->depthBuffer)->numFracBits;

			if ((scale & (scale + 1)) == 0) {
			    spanCount--;
			    reader = (__GLproc) __glSpanReadDepthUint2;
			}
		    }
		    break;
		  default:
		    break;
		}
		break;
	      default:
		break;
	    }
	}
	break;
    }

    /*
    ** Pack / render
    */
    switch (spanModInfo->dstData) {
      case __GL_PIXDATA_MEM:
	if (dstReduce) {
	    switch (dstFormat) {
	      case GL_RGB:
		ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReduceRGB : __glSpanReduceRGBNS);
		break;
	      case GL_BGR_EXT:
		ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReduceBGR : __glSpanReduceBGRNS);
		break;
	      case GL_RED:
		ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReduceRed : __glSpanReduceRedNS);
		break;
	      case GL_GREEN:
		ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReduceGreen : __glSpanReduceGreenNS);
		break;
	      case GL_BLUE:
		ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReduceBlue : __glSpanReduceBlueNS);
		break;
	      case GL_LUMINANCE:
		if (spanModInfo->pixelPath == __GL_PIXPATH_READPIX) {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReduceLuminance : __glSpanReduceRedNS);
		} else {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReduceRed : __glSpanReduceRedNS);
		}
		break;
	      case GL_LUMINANCE_ALPHA:
		if (spanModInfo->pixelPath == __GL_PIXPATH_READPIX) {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReduceLuminanceAlpha : __glSpanReduceRedAlphaNS);
		} else {
		    ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReduceRedAlpha : __glSpanReduceRedAlphaNS);
		}
		break;
	      case __GL_RED_ALPHA:
		ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReduceRedAlpha : __glSpanReduceRedAlphaNS);
		break;
	      case GL_ALPHA:
		ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReduceAlpha : __glSpanReduceAlphaNS);
		break;
	      case GL_RGBA:
		if (__GL_APPLY_FB_SCALE(spanInfo)) {
		    ADD(__glSpanPostUnscaleRGBA);
		}
		break;
	      case GL_ABGR_EXT:
		ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReorderABGR : __glSpanReorderABGRNS);
		break;
	      case GL_BGRA_EXT:
		ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReorderBGRA : __glSpanReorderBGRANS);
		break;
	      case GL_INTENSITY:
		ADD(__GL_APPLY_FB_SCALE(spanInfo) ? __glSpanReduceRed : __glSpanReduceRedNS);
		break;
	    }
	}

	if (dstClamp) {
	    switch (srcType) {
	      case GL_BYTE:
	      case GL_SHORT:
	      case GL_INT:
		ADD(__glSpanClampSignedDst);
		break;
	      case GL_FLOAT:
		ADD(__glSpanClampFloatDst);
		break;
	    }
	}

	if (dstConvert) {
	    if (dstIndex) {
		switch (dstType) {
		  case GL_BYTE:			ADD(__glSpanPackByteI); break;
		  case GL_UNSIGNED_BYTE:	ADD(__glSpanPackUbyteI); break;
		  case GL_SHORT:		ADD(__glSpanPackShortI); break;
		  case GL_UNSIGNED_SHORT:	ADD(__glSpanPackUshortI); break;
		  case GL_INT:			ADD(__glSpanPackIntI); break;
		  case GL_UNSIGNED_INT:		ADD(__glSpanPackUintI); break;
		  case GL_BITMAP:		ADD(__glSpanPackBitmap); break;
		}
	    } else {
		switch (dstType) {
		  case __GL_UNSIGNED_SHORT_4_4_4_4_ARGB:
		    ADD(__glSpanPackARGB4444Ushort);
		    spanInfo->dstComponents = 4;
		    break;
		  case __GL_UNSIGNED_SHORT_X_5_5_5:
		    ADD(__glSpanPackX555Ushort);
		    spanInfo->dstComponents = 3;
		    break;
		  case __GL_UNSIGNED_SHORT_1_5_5_5:
		    ADD(__glSpanPack1555Ushort);
		    spanInfo->dstComponents = 4;
		    break;
		  case __GL_UNSIGNED_SHORT_5_6_5:
		    ADD(__glSpanPack565Ushort);
		    spanInfo->dstComponents = 3;
		    break;
		  case __GL_UNSIGNED_INT_8_8_8_8_ARGB:
		    ADD(__glSpanPack8888RevUint);
		    spanInfo->dstComponents = 4;
		    break;
		  case __GL_UNSIGNED_INT_TYPE_5:
		    ADD(__glSpanPackXRGB8888Uint);
		    spanInfo->dstComponents = 4;
		    break;
		  case GL_BYTE:			ADD(__glSpanPackUbyte); break;
		  case GL_UNSIGNED_BYTE:	ADD(__glSpanPackUbyteRound); break;
		  case GL_SHORT:		ADD(__glSpanPackShort); break;
		  case GL_UNSIGNED_SHORT:	ADD(__glSpanPackUshort); break;
		  case GL_INT:			ADD(__glSpanPackInt); break;
		  case GL_UNSIGNED_INT:		ADD(__glSpanPackUint); break;
		  case GL_UNSIGNED_BYTE_3_3_2_EXT:
		    ADD(__glSpanPack332Ubyte);
		    spanInfo->dstComponents = 3;
		    break;
		  case GL_UNSIGNED_SHORT_4_4_4_4_EXT:
		    ADD(__glSpanPack4444Ushort);
		    spanInfo->dstComponents = 4;
		    break;
		  case GL_UNSIGNED_SHORT_5_5_5_1_EXT:
		    ADD(__glSpanPack5551Ushort);
		    spanInfo->dstComponents = 4;
		    break;
		  case GL_UNSIGNED_INT_8_8_8_8_EXT:
		    ADD(__glSpanPack8888Uint);
		    spanInfo->dstComponents = 4;
		    break;
		  case GL_UNSIGNED_INT_10_10_10_2_EXT:
		    ADD(__glSpanPack_10_10_10_2_Uint);
		    spanInfo->dstComponents = 4;
		    break;
		}
	    }
	} else if (__GL_NON_COLOR_COMP(spanInfo)) {
	    switch (dstType) {
	      case GL_BYTE:		ADD(__glSpanPackNonCompByte); break;
	      case GL_UNSIGNED_BYTE:	ADD(__glSpanPackNonCompUbyte); break;
	      case GL_SHORT:		ADD(__glSpanPackNonCompShort); break;
	      case GL_UNSIGNED_SHORT:	ADD(__glSpanPackNonCompUshort); break;
	      case GL_INT:		ADD(__glSpanPackNonCompInt); break;
	      case GL_UNSIGNED_INT:	ADD(__glSpanPackNonCompUint); break;
	    }
	}

	if (dstSwap) {
	    if (spanInfo->dstElementSize == 2) {
		ADD(__glSpanSwapBytes2Dst);
	    } else if (spanInfo->dstElementSize == 4) {
		ADD(__glSpanSwapBytes4Dst);
	    }
	} else if (dstAlign) {
	    if (spanInfo->dstElementSize == 2) {
		ADD(__glSpanAlignPixels2Dst);
	    } else if (spanInfo->dstElementSize == 4) {
		ADD(__glSpanAlignPixels4Dst);
	    }
	}
	break;

      case __GL_PIXDATA_FB:
	switch (dstFormat) {
	  case GL_RGBA:
	  case GL_RGB:
	  case GL_RED:
	  case GL_GREEN:
	  case GL_BLUE:
	  case GL_ALPHA:
	  case GL_LUMINANCE:
	  case GL_LUMINANCE_ALPHA:
	  case GL_ABGR_EXT:
	  case GL_BGR_EXT:
	  case GL_BGRA_EXT:
	  case GL_INTENSITY:
	    if (zoomx1) {
		render = gc->procs.pixel.spanRenderRGBA2;
	    } else {
		render = gc->procs.pixel.spanRenderRGBA;
	    }
	    break;
	  case GL_DEPTH_COMPONENT:
	    if (zoomx1) {
		render = gc->procs.pixel.spanRenderDepth2;
	    } else {
		render = gc->procs.pixel.spanRenderDepth;
	    }
	    break;
	  case GL_COLOR_INDEX:
	    if (zoomx1) {
		render = gc->procs.pixel.spanRenderCI2;
	    } else {
		render = gc->procs.pixel.spanRenderCI;
	    }
	    break;
	  case GL_STENCIL_INDEX:
	    if (zoomx1) {
		render = gc->procs.pixel.spanRenderStencil2;
	    } else {
		render = gc->procs.pixel.spanRenderStencil;
	    }
	    break;
	}

	/*
	** DrawPixels fast paths: (GL_UNSIGNED_BYTE, GL_RGB / GL_RGBA / GL_STENCIL_INDEX /
	** GL_COLOR_INDEX), (GL_UNSIGNED_SHORT, GL_STENCIL_INDEX / GL_COLOR_INDEX),
	** (GL_UNSIGNED_INT, GL_DEPTH_COMPONENT / GL_COLOR_INDEX) render the user's data directly
	*/
	if (spanModInfo->pixelPath == __GL_PIXPATH_DRAWPIX) {
	    switch (srcType) {
	      case GL_UNSIGNED_BYTE:
		switch (srcFormat) {
		  case GL_RGB:
		    spanCount = 0;
		    if (!packedUserData) {
			ADD(__glSpanUnpackRGBubyte);
		    }
		    if (!pm->modifyRGBA) {
			pm->redCurMap = pm->redMap;
			pm->greenCurMap = pm->greenMap;
			pm->blueCurMap = pm->blueMap;
			pm->alphaCurMap = pm->alphaMap;
			if (zoomx1) {
			    render = (__GLproc) __glSpanRenderRGBubyte2;
			} else {
			    render = (__GLproc) __glSpanRenderRGBubyte;
			}
		    } else {
			if (!pm->rgbaCurrent) {
			    __glBuildRGBAModifyTables(gc, pm);
			}
			pm->redCurMap = pm->redModMap;
			pm->greenCurMap = pm->greenModMap;
			pm->blueCurMap = pm->blueModMap;
			pm->alphaCurMap = pm->alphaModMap;
			if (zoomx1) {
			    render = (__GLproc) __glSpanRenderRGBubyte2;
			} else {
			    render = (__GLproc) __glSpanRenderRGBubyte;
			}
		    }
		    break;
		  case GL_RGBA:
		    spanCount = 0;
		    if (!packedUserData) {
			ADD(__glSpanUnpackRGBAubyte);
		    }
		    if (!pm->modifyRGBA) {
			pm->redCurMap = pm->redMap;
			pm->greenCurMap = pm->greenMap;
			pm->blueCurMap = pm->blueMap;
			pm->alphaCurMap = pm->alphaMap;
		    } else {
			if (!pm->rgbaCurrent) {
			    __glBuildRGBAModifyTables(gc, pm);
			}
			pm->redCurMap = pm->redModMap;
			pm->greenCurMap = pm->greenModMap;
			pm->blueCurMap = pm->blueModMap;
			pm->alphaCurMap = pm->alphaModMap;
		    }
		    if (zoomx1) {
			render = (__GLproc) __glSpanRenderRGBAubyte2;
		    } else {
			render = (__GLproc) __glSpanRenderRGBAubyte;
		    }
		    break;
		  case GL_STENCIL_INDEX:
		    if (!pm->modifyStencil) {
			spanCount = 0;
			if (!packedUserData) {
			    ADD(__glSpanUnpackIndexUbyte);
			}
			if (zoomx1) {
			    render = (__GLproc) __glSpanRenderStencilUbyte2;
			} else {
			    render = (__GLproc) __glSpanRenderStencilUbyte;
			}
		    }
		    break;
		  case GL_COLOR_INDEX:
		    spanCount = 0;
		    if (!packedUserData) {
			ADD(__glSpanUnpackIndexUbyte);
		    }
		    if (!pm->modifyCI) {
			pm->iCurMap = pm->iMap;
			if (zoomx1) {
			    render = (__GLproc) __glSpanRenderCIubyte2;
			} else {
			    render = (__GLproc) __glSpanRenderCIubyte;
			}
		    } else {
			if (gc->modes.rgbMode) {
			    if (!pm->iToRGBACurrent) {
				__glBuildItoRGBAModifyTables(gc, pm);
			    }
			    pm->redCurMap = pm->iToRMap;
			    pm->greenCurMap = pm->iToGMap;
			    pm->blueCurMap = pm->iToBMap;
			    pm->alphaCurMap = pm->iToAMap;
			    if (zoomx1) {
				render = (__GLproc) __glSpanRenderCIubyte4;
			    } else {
				render = (__GLproc) __glSpanRenderCIubyte3;
			    }
			} else {
			    if (!pm->iToICurrent) {
				__glBuildItoIModifyTables(gc, pm);
			    }
			    pm->iCurMap = pm->iToIMap;
			    if (zoomx1) {
				render = (__GLproc) __glSpanRenderCIubyte2;
			    } else {
				render = (__GLproc) __glSpanRenderCIubyte;
			    }
			}
		    }
		    break;
		}
		break;
	      case GL_UNSIGNED_SHORT:
		switch (srcFormat) {
		  case GL_STENCIL_INDEX:
		    if (!pm->modifyStencil) {
			spanCount--;
			if (zoomx1) {
			    render = (__GLproc) __glSpanRenderStencilUshort2;
			} else {
			    render = (__GLproc) __glSpanRenderStencilUshort;
			}
		    }
		    break;
		  case GL_COLOR_INDEX:
		    if (!pm->modifyCI) {
			spanCount--;
			if (zoomx1) {
			    render = (__GLproc) __glSpanRenderCIushort2;
			} else {
			    render = (__GLproc) __glSpanRenderCIushort;
			}
		    }
		    break;
		}
		break;
	      case GL_UNSIGNED_INT:
		switch (srcFormat) {
		  case GL_DEPTH_COMPONENT:
		    if (!pm->modifyDepth) {
			GLuint writeMask = gc->depthBuffer.writeMask;
			GLuint scale = gc->depthBuffer.scale >>
			    __GL_DEPTH_DP(&gc->depthBuffer)->numFracBits;

			if (((writeMask & (writeMask + 1)) == 0) &
			    ((scale & (scale + 1)) == 0)) {
			    spanCount--;
			    if (zoomx1) {
				render = (__GLproc) __glSpanRenderDepthUint2;
			    } else {
				render = (__GLproc) __glSpanRenderDepthUint;
			    }
			}
		    }
		    break;
		  case GL_COLOR_INDEX:
		    if (!pm->modifyCI) {
			spanCount--;
			if (zoomx1) {
			    render = (__GLproc) __glSpanRenderCIuint2;
			} else {
			    render = (__GLproc) __glSpanRenderCIuint;
			}
		    }
		    break;
		}
		break;
	    }
	}
	break;
    }

done:
    spanModInfo->reader = reader;
    spanModInfo->render = render;
    spanInfo->numSpanMods = spanCount;
}


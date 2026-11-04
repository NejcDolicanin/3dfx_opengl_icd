/*
** gl_doget.c -- __glConvertResult and DoGet, the common body of glGetBooleanv/Integerv/Floatv/Doublev.
** Written from the SGI Sample Implementation's s_get.c DoGet with the ICD's state layout,
** its 3dfx/EXT queries added and the SI-only queries removed; case order follows the
** original's body layout.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"                      /* __GLtexNamePriICD */
#include "si_names.h"
#include "icdprotos.h"

#define __GL_FLOAT      0
#define __GL_FLOAT32    1
#define __GL_FLOAT64    2
#define __GL_INT32      3
#define __GL_BOOLEAN    4
#define __GL_COLOR      5
#define __GL_SCOLOR     6
#define GL_ARRAY_ELEMENT_LOCK_FIRST_EXT     0x81A8
#define GL_ARRAY_ELEMENT_LOCK_COUNT_EXT     0x81A9
#define GL_CULL_VERTEX_EXT                  0x81AA
#define GL_CULL_VERTEX_EYE_POSITION_EXT     0x81AB
#define GL_CULL_VERTEX_OBJECT_POSITION_EXT  0x81AC

/* 69080e40: the y twin of __glReturnWindowX (float args go on the stack under __fastcall) */
/* 6909c660 returns &tex->name of the object bound to (target, unit) */
#define __glLookupActiveTextureState(gc, target) \
    __glLookUpTextureTexobjs(gc, target, gc->fx.activeTexture)
/* glIsEnabled is reached through its API entry, stdcall-shaped (push cap; call) */

/* 690bc8a0 -- __glim_GetMapfv */
void __stdcall __glim_GetMapfv(GLenum target, GLenum query, GLfloat buf[])
{
    __GLevaluator1ICD *eval1;
    __GLevaluator2ICD *eval2;
    __GLfloat *eval1Data, *eval2Data;
    GLfloat *rp;
    GLint index, i, t;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    /*
    ** Check if target is valid.
    */
    rp = buf;
    switch (target) {
      case GL_MAP1_COLOR_4:
      case GL_MAP1_INDEX:
      case GL_MAP1_NORMAL:
      case GL_MAP1_TEXTURE_COORD_1:
      case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3:
      case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3:
      case GL_MAP1_VERTEX_4:
	index = __GL_EVAL1D_INDEX(target);
	eval1 = &gc->eval.eval1[index];
	switch (query) {
	  case GL_COEFF:
	    t = eval1->order * eval1->k;
	    eval1Data = gc->eval.eval1Data[index];
	    for (i = 0; i < t; i++) {
		*rp++ = eval1Data[i];
	    }
	    break;
	  case GL_DOMAIN:
	    *rp++ = eval1->u1;
	    *rp++ = eval1->u2;
	    break;
	  case GL_ORDER:
	    *rp++ = gc->eval.eval1[index].order;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_MAP2_COLOR_4:
      case GL_MAP2_INDEX:
      case GL_MAP2_NORMAL:
      case GL_MAP2_TEXTURE_COORD_1:
      case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3:
      case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3:
      case GL_MAP2_VERTEX_4:
	index = __GL_EVAL2D_INDEX(target);
	eval2 = &gc->eval.eval2[index];
	switch (query) {
	  case GL_COEFF:
	    eval2Data = gc->eval.eval2Data[index];
	    t = eval2->majorOrder * eval2->minorOrder * eval2->k;
	    for (i = 0; i < t; i++) {
		*rp++ = eval2Data[i];
	    }
	    break;
	  case GL_DOMAIN:
	    *rp++ = eval2->u1;
	    *rp++ = eval2->u2;
	    *rp++ = eval2->v1;
	    *rp++ = eval2->v2;
	    break;
	  case GL_ORDER:
	    *rp++ = gc->eval.eval2[index].majorOrder;
	    *rp++ = gc->eval.eval2[index].minorOrder;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 690bca60 -- __glim_GetMapdv */
void __stdcall __glim_GetMapdv(GLenum target, GLenum query, GLdouble buf[])
{
    __GLevaluator1ICD *eval1;
    __GLevaluator2ICD *eval2;
    __GLfloat *eval1Data, *eval2Data;
    GLdouble *rp;
    GLint index, i, t;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    /*
    ** Check if target is valid.
    */
    rp = buf;
    switch (target) {
      case GL_MAP1_COLOR_4:
      case GL_MAP1_INDEX:
      case GL_MAP1_NORMAL:
      case GL_MAP1_TEXTURE_COORD_1:
      case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3:
      case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3:
      case GL_MAP1_VERTEX_4:
	index = __GL_EVAL1D_INDEX(target);
	eval1 = &gc->eval.eval1[index];
	switch (query) {
	  case GL_COEFF:
	    eval1Data = gc->eval.eval1Data[index];
	    t = eval1->order * eval1->k;
	    for (i = 0; i < t; i++) {
		*rp++ = eval1Data[i];
	    }
	    break;
	  case GL_DOMAIN:
	    *rp++ = eval1->u1;
	    *rp++ = eval1->u2;
	    break;
	  case GL_ORDER:
	    *rp++ = gc->eval.eval1[index].order;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_MAP2_COLOR_4:
      case GL_MAP2_INDEX:
      case GL_MAP2_NORMAL:
      case GL_MAP2_TEXTURE_COORD_1:
      case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3:
      case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3:
      case GL_MAP2_VERTEX_4:
	index = __GL_EVAL2D_INDEX(target);
	eval2 = &gc->eval.eval2[index];
	switch (query) {
	  case GL_COEFF:
	    eval2Data = gc->eval.eval2Data[index];
	    t = eval2->majorOrder * eval2->minorOrder * eval2->k;
	    for (i = 0; i < t; i++) {
		*rp++ = eval2Data[i];
	    }
	    break;
	  case GL_DOMAIN:
	    *rp++ = eval2->u1;
	    *rp++ = eval2->u2;
	    *rp++ = eval2->v1;
	    *rp++ = eval2->v2;
	    break;
	  case GL_ORDER:
	    *rp++ = gc->eval.eval2[index].majorOrder;
	    *rp++ = gc->eval.eval2[index].minorOrder;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 690bcc20 -- __glim_GetMapiv */
void __stdcall __glim_GetMapiv(GLenum target, GLenum query, GLint buf[])
{
    __GLevaluator1ICD *eval1;
    __GLevaluator2ICD *eval2;
    __GLfloat *eval1Data, *eval2Data;
    GLint *rp;
    GLint index, t;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    /*
    ** Check if target is valid.
    */
    rp = buf;
    switch (target) {
      case GL_MAP1_COLOR_4:
      case GL_MAP1_INDEX:
      case GL_MAP1_NORMAL:
      case GL_MAP1_TEXTURE_COORD_1:
      case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3:
      case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3:
      case GL_MAP1_VERTEX_4:
	index = __GL_EVAL1D_INDEX(target);
	eval1 = &gc->eval.eval1[index];
	switch (query) {
	  case GL_COEFF:
	    eval1Data = gc->eval.eval1Data[index];
	    t = eval1->order * eval1->k;
	    __glConvertResult(gc, __GL_FLOAT, eval1Data,
			      __GL_INT32, rp, t);
	    break;
	  case GL_DOMAIN:
	    __glConvertResult(gc, __GL_FLOAT, &eval1->u1,
			      __GL_INT32, rp, 2);
	    break;
	  case GL_ORDER:
	    *rp++ = gc->eval.eval1[index].order;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      case GL_MAP2_COLOR_4:
      case GL_MAP2_INDEX:
      case GL_MAP2_NORMAL:
      case GL_MAP2_TEXTURE_COORD_1:
      case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3:
      case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3:
      case GL_MAP2_VERTEX_4:
	index = __GL_EVAL2D_INDEX(target);
	eval2 = &gc->eval.eval2[index];
	switch (query) {
	  case GL_COEFF:
	    eval2Data = gc->eval.eval2Data[index];
	    t = eval2->majorOrder * eval2->minorOrder * eval2->k;
	    __glConvertResult(gc, __GL_FLOAT, eval2Data,
			      __GL_INT32, rp, t);
	    break;
	  case GL_DOMAIN:
	    __glConvertResult(gc, __GL_FLOAT, &eval2->u1,
			      __GL_INT32, rp, 4);
	    break;
	  case GL_ORDER:
	    *rp++ = gc->eval.eval2[index].majorOrder;
	    *rp++ = gc->eval.eval2[index].minorOrder;
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 690bcdd0 -- __glim_GetPixelMapfv: the ICD map is {size, ?, data} (state.pixel.map) */
void __stdcall __glim_GetPixelMapfv(GLenum map, GLfloat buf[])
{
    GLint index;
    GLint limit;
    GLfloat *rp;
    __GLpixelMapHead *pMap;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    pMap = (__GLpixelMapHead *)gc->state.pixel.map;
    index = map - GL_PIXEL_MAP_I_TO_I;
    rp = buf;
    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
	{
	    GLint *fromp = pMap[index].base.mapI;
	    limit = pMap[index].size;
	    while (--limit >= 0) {
		*rp++ = *fromp++;
	    }
	}
	break;
      case GL_PIXEL_MAP_I_TO_R:      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:      case GL_PIXEL_MAP_I_TO_A:
      case GL_PIXEL_MAP_R_TO_R:      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:      case GL_PIXEL_MAP_A_TO_A:
	{
	    __GLfloat *fromp = pMap[index].base.mapF;
	    limit = pMap[index].size;
	    while (--limit >= 0) {
		*rp++ = *fromp++;
	    }
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 690bceb0 -- __glim_GetPixelMapuiv: the ICD map is {size, ?, data} (state.pixel.map) */
void __stdcall __glim_GetPixelMapuiv(GLenum map, GLuint buf[])
{
    GLint index;
    GLint limit;
    GLuint *rp;
    __GLpixelMapHead *pMap;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    pMap = (__GLpixelMapHead *)gc->state.pixel.map;
    index = map - GL_PIXEL_MAP_I_TO_I;
    rp = buf;
    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
	{
	    GLint *fromp = pMap[index].base.mapI;
	    limit = pMap[index].size;
	    while (--limit >= 0) {
		*rp++ = *fromp++;
	    }
	}
	break;
      case GL_PIXEL_MAP_I_TO_R:      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:      case GL_PIXEL_MAP_I_TO_A:
      case GL_PIXEL_MAP_R_TO_R:      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:      case GL_PIXEL_MAP_A_TO_A:
	{
	    __GLfloat *fromp = pMap[index].base.mapF;
	    limit = pMap[index].size;
	    while (--limit >= 0) {
		*rp++ = (GLuint) *fromp++;
	    }
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 690bcfa0 -- __glim_GetPixelMapusv: the ICD map is {size, ?, data} (state.pixel.map) */
void __stdcall __glim_GetPixelMapusv(GLenum map, GLushort buf[])
{
    GLint index;
    GLint limit;
    GLushort *rp;
    __GLpixelMapHead *pMap;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    pMap = (__GLpixelMapHead *)gc->state.pixel.map;
    index = map - GL_PIXEL_MAP_I_TO_I;
    rp = buf;
    switch (map) {
      case GL_PIXEL_MAP_I_TO_I:
      case GL_PIXEL_MAP_S_TO_S:
	{
	    GLint *fromp = pMap[index].base.mapI;
	    limit = pMap[index].size;
	    while (--limit >= 0) {
		*rp++ = (GLushort) *fromp++;
	    }
	}
	break;
      case GL_PIXEL_MAP_I_TO_R:      case GL_PIXEL_MAP_I_TO_G:
      case GL_PIXEL_MAP_I_TO_B:      case GL_PIXEL_MAP_I_TO_A:
      case GL_PIXEL_MAP_R_TO_R:      case GL_PIXEL_MAP_G_TO_G:
      case GL_PIXEL_MAP_B_TO_B:      case GL_PIXEL_MAP_A_TO_A:
	{
	    __GLfloat *fromp = pMap[index].base.mapF;
	    limit = pMap[index].size;
	    while (--limit >= 0) {
		*rp++ = (GLushort) *fromp++;
	    }
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
}

/* 690bd090 -- __glConvertResult */
void __fastcall __glConvertResult(__GLcontext *gc, GLint fromType, const void *rawdata,
		       GLint toType, void *result, GLint size)
{
    GLint i;

    switch (fromType) {
      case __GL_FLOAT:
	switch (toType) {
	  case __GL_FLOAT32:
	    for (i=0; i < size; i++) {
		((GLfloat *)result)[i] = ((const __GLfloat *)rawdata)[i];
	    }
	    break;
	  case __GL_FLOAT64:
	    for (i=0; i < size; i++) {
		((GLdouble *)result)[i] = ((const __GLfloat *)rawdata)[i];
	    }
	    break;
	  case __GL_INT32:
	    for (i=0; i < size; i++) {
		((GLint *)result)[i] =
			(GLint)(((const __GLfloat *)rawdata)[i] >= 0.0 ?
			((const __GLfloat *)rawdata)[i] + __glHalf:
			((const __GLfloat *)rawdata)[i] - __glHalf);
	    }
	    break;
	  case __GL_BOOLEAN:
	    for (i=0; i < size; i++) {
		((GLboolean *)result)[i] =
		    ((const __GLfloat *)rawdata)[i] ? 1 : 0;
	    }
	    break;
	}
	break;
      case __GL_COLOR:
	switch (toType) {
	  case __GL_FLOAT32:
	    for (i=0; i < size; i++) {
		((GLfloat *)result)[i] = ((const __GLfloat *)rawdata)[i];
	    }
	    break;
	  case __GL_FLOAT64:
	    for (i=0; i < size; i++) {
		((GLdouble *)result)[i] = ((const __GLfloat *)rawdata)[i];
	    }
	    break;
	  case __GL_INT32:
	    for (i=0; i < size; i++) {
		((GLint *)result)[i] =
		    __GL_FLOAT_TO_I(((const __GLfloat *)rawdata)[i]);
	    }
	    break;
	  case __GL_BOOLEAN:
	    for (i=0; i < size; i++) {
		((GLboolean *)result)[i] =
		    ((const __GLfloat *)rawdata)[i] ? 1 : 0;
	    }
	    break;
	}
	break;
      case __GL_SCOLOR:
	switch (toType) {
	  case __GL_FLOAT32:
	    ((GLfloat *)result)[0] =
		((const __GLfloat *)rawdata)[0] * gc->constants.oneOverRedScale;
	    ((GLfloat *)result)[1] =
		((const __GLfloat *)rawdata)[1] * gc->constants.oneOverGreenScale;
	    ((GLfloat *)result)[2] =
		((const __GLfloat *)rawdata)[2] * gc->constants.oneOverBlueScale;
	    ((GLfloat *)result)[3] =
		((const __GLfloat *)rawdata)[3] * gc->constants.oneOverAlphaScale;
	    break;
	  case __GL_FLOAT64:
	    ((GLdouble *)result)[0] =
		((const __GLfloat *)rawdata)[0] * gc->constants.oneOverRedScale;
	    ((GLdouble *)result)[1] =
		((const __GLfloat *)rawdata)[1] * gc->constants.oneOverGreenScale;
	    ((GLdouble *)result)[2] =
		((const __GLfloat *)rawdata)[2] * gc->constants.oneOverBlueScale;
	    ((GLdouble *)result)[3] =
		((const __GLfloat *)rawdata)[3] * gc->constants.oneOverAlphaScale;
	    break;
	  case __GL_INT32:
	    ((GLint *)result)[0] =
		__GL_FLOAT_TO_I(((const __GLfloat *)rawdata)[0] *
				gc->constants.oneOverRedScale);
	    ((GLint *)result)[1] =
		__GL_FLOAT_TO_I(((const __GLfloat *)rawdata)[1] *
				gc->constants.oneOverGreenScale);
	    ((GLint *)result)[2] =
		__GL_FLOAT_TO_I(((const __GLfloat *)rawdata)[2] *
				gc->constants.oneOverBlueScale);
	    ((GLint *)result)[3] =
		__GL_FLOAT_TO_I(((const __GLfloat *)rawdata)[3] *
				gc->constants.oneOverAlphaScale);
	    break;
	  case __GL_BOOLEAN:
	    for (i=0; i < size; i++) {
		((GLboolean *)result)[i] =
		    ((const __GLfloat *)rawdata)[i] ? 1 : 0;
	    }
	    break;
	}
	break;
      case __GL_INT32:
	switch (toType) {
	  case __GL_FLOAT32:
	    for (i=0; i < size; i++) {
		((GLfloat *)result)[i] = ((const GLint *)rawdata)[i];
	    }
	    break;
	  case __GL_FLOAT64:
	    for (i=0; i < size; i++) {
		((GLdouble *)result)[i] = ((const GLint *)rawdata)[i];
	    }
	    break;
	  case __GL_INT32:
	    for (i=0; i < size; i++) {
		((GLint *)result)[i] = ((const GLint *)rawdata)[i];
	    }
	    break;
	  case __GL_BOOLEAN:
	    for (i=0; i < size; i++) {
		((GLboolean *)result)[i] = ((const GLint *)rawdata)[i] ? 1 : 0;
	    }
	    break;
	}
	break;
      case __GL_BOOLEAN:
	switch (toType) {
	  case __GL_FLOAT32:
	    for (i=0; i < size; i++) {
		((GLfloat *)result)[i] = ((const GLboolean *)rawdata)[i];
	    }
	    break;
	  case __GL_FLOAT64:
	    for (i=0; i < size; i++) {
		((GLdouble *)result)[i] = ((const GLboolean *)rawdata)[i];
	    }
	    break;
	  case __GL_INT32:
	    for (i=0; i < size; i++) {
		((GLint *)result)[i] = ((const GLboolean *)rawdata)[i];
	    }
	    break;
	  case __GL_BOOLEAN:
	    for (i=0; i < size; i++) {
		((GLboolean *)result)[i] =
		    ((const GLboolean *)rawdata)[i] ? 1 : 0;
	    }
	    break;
	}
	break;
    }
}

/* 690bd5f0 -- __glDoGet: s_get.c DoGet */
void __fastcall __glDoGet(GLenum sq, void *result, GLint type, const char *procName)
{
    GLint index;
    __GLfloat ftemp[100], *fp = ftemp;		/* NOTE: for floats */
    __GLfloat ctemp[100], *cp = ctemp;		/* NOTE: for colors */
    __GLfloat sctemp[100], *scp = sctemp;	/* NOTE: for scaled colors */
    GLint itemp[100], *ip = itemp;		/* NOTE: for ints */
    GLboolean ltemp[100], *lp = ltemp;		/* NOTE: for logicals */
    __GLfloat *mp;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    switch (sq) {
      case GL_MAX_LIST_NESTING:
	*ip++ = 64;
	break;
      case GL_CURRENT_COLOR:
	*cp++ = gc->state.current.userColor.r;
	*cp++ = gc->state.current.userColor.g;
	*cp++ = gc->state.current.userColor.b;
	*cp++ = gc->state.current.userColor.a;
	break;
      case GL_CURRENT_INDEX:
	*fp++ = gc->state.current.userColorIndex;
	break;
      case GL_CURRENT_NORMAL:
	*cp++ = gc->state.current.normal.x;
	*cp++ = gc->state.current.normal.y;
	*cp++ = gc->state.current.normal.z;
	break;
      case GL_CURRENT_TEXTURE_COORDS:
	*fp++ = gc->state.current.activeTexture->x;
	*fp++ = gc->state.current.activeTexture->y;
	*fp++ = gc->state.current.activeTexture->z;
	*fp++ = gc->state.current.activeTexture->w;
	break;
      case GL_CURRENT_RASTER_INDEX:
	if (gc->modes.rgbMode) {
	    /* Always return 1 */
	    *fp++ = 1.0;
	} else {
	    *fp++ = gc->state.current.rasterPos.colors[__GL_FRONTFACE].r;
	}
	break;
      case GL_CURRENT_RASTER_COLOR:
	if (gc->modes.colorIndexMode) {
	    /* Always return 1,1,1,1 */
	    *fp++ = 1.0;
	    *fp++ = 1.0;
	    *fp++ = 1.0;
	    *fp++ = 1.0;
	} else {
	    *scp++ = gc->state.current.rasterPos.colors[__GL_FRONTFACE].r;
	    *scp++ = gc->state.current.rasterPos.colors[__GL_FRONTFACE].g;
	    *scp++ = gc->state.current.rasterPos.colors[__GL_FRONTFACE].b;
	    *scp++ = gc->state.current.rasterPos.colors[__GL_FRONTFACE].a;
	}
	break;
      case GL_CURRENT_RASTER_TEXTURE_COORDS:
	*fp++ = gc->state.current.rasterPos.texture[0].x;
	*fp++ = gc->state.current.rasterPos.texture[0].y;
	*fp++ = gc->state.current.rasterPos.texture[0].z;
	*fp++ = gc->state.current.rasterPos.texture[0].w;
	break;
      case GL_CURRENT_RASTER_POSITION:
	*fp++ = __glReturnWindowX(gc, gc->state.current.rasterPos.window.x);
	*fp++ = (GLfloat)__glReturnWindowY(gc, gc->state.current.rasterPos.window.y);
	*fp++ = gc->constants.oneOverDepthScale
					* gc->state.current.rasterPos.window.z;
	*fp++ = gc->state.current.rasterPos.clip.w;
	break;
      case GL_CURRENT_RASTER_POSITION_VALID:
	*lp++ = gc->state.current.validRasterPos;
	break;
      case GL_CURRENT_RASTER_DISTANCE:
	*fp++ = gc->state.current.rasterPos.eye.z;
	break;
      case GL_POINT_SIZE:
	*fp++ = gc->state.point.requestedSize;
	break;
      case GL_POINT_SIZE_RANGE:
      /* case GL_SMOOTH_POINT_SIZE_RANGE: */ /* Alias for POINT_SIZE_RANGE */
	*fp++ = gc->constants.pointSizeMinimum;
	*fp++ = gc->constants.pointSizeMaximum;
	break;
      case GL_POINT_SIZE_GRANULARITY:
      /* case GL_SMOOTH_POINT_SIZE_GRANULARITY: */ /* Alias for POINT_SIZE_GRANULARITY */
	*fp++ = gc->constants.pointSizeGranularity;
	break;
      case GL_LINE_WIDTH:
	*fp++ = gc->state.line.requestedWidth;
	break;
      case GL_LINE_WIDTH_RANGE:
      /* case GL_SMOOTH_LINE_WIDTH_RANGE: */ /* Alias for LINE_WIDTH_RANGE */
	*fp++ = gc->constants.lineWidthMinimum;
	*fp++ = gc->constants.lineWidthMaximum;
	break;
      case GL_LINE_WIDTH_GRANULARITY:
      /* case GL_SMOOTH_LINE_WIDTH_GRANULARITY: */ /* Alias for LINE_WIDTH_GRANULARITY */
	*fp++ = gc->constants.lineWidthGranularity;
	break;
      case GL_LINE_STIPPLE_PATTERN:
	*ip++ = gc->state.line.stipple;
	break;
      case GL_LINE_STIPPLE_REPEAT:
	*ip++ = gc->state.line.stippleRepeat;
	break;
      case GL_POLYGON_MODE:
	*ip++ = gc->state.polygon.frontMode;
	*ip++ = gc->state.polygon.backMode;
	break;
      case GL_EDGE_FLAG:
	*lp++ = (gc->state.current.edgeTag & __GL_VERTEX_EDGE_FLAG) ? GL_TRUE
								    : GL_FALSE;
	break;
      case GL_CULL_FACE_MODE:
	*ip++ = gc->state.polygon.cull;
	break;
      case GL_FRONT_FACE:
	*ip++ = gc->state.polygon.frontFaceDirection;
	break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	*lp++ = gc->state.light.model.localViewer;
	break;
      case GL_LIGHT_MODEL_TWO_SIDE:
	*lp++ = gc->state.light.model.twoSided;
	break;
      case GL_LIGHT_MODEL_AMBIENT:
	__glUnScaleColorf(gc, cp, &gc->state.light.model.ambient);
	cp += 4;
	break;
      case GL_COLOR_MATERIAL_FACE:
	*ip++ = gc->state.light.colorMaterialFace;
	break;
      case GL_COLOR_MATERIAL_PARAMETER:
	*ip++ = gc->state.light.colorMaterialParam;
	break;
      case GL_SHADE_MODEL:
	*ip++ = gc->state.light.shadingModel;
	break;
      case GL_FOG_INDEX:
	*fp++ = gc->state.fog.index;
	break;
      case GL_FOG_DENSITY:
	*fp++ = gc->state.fog.density;
	break;
      case GL_FOG_START:
	*fp++ = gc->state.fog.start;
	break;
      case GL_FOG_END:
	*fp++ = gc->state.fog.end;
	break;
      case GL_FOG_MODE:
	*ip++ = gc->state.fog.mode;
	break;
      case GL_FOG_COLOR:
	*scp++ = gc->state.fog.color.r;
	*scp++ = gc->state.fog.color.g;
	*scp++ = gc->state.fog.color.b;
	*scp++ = gc->state.fog.color.a;
	break;
      case GL_DEPTH_RANGE:
	/* These get scaled like colors, to [0, 2^31-1] */
	*cp++ = gc->state.viewport.zNear;
	*cp++ = gc->state.viewport.zFar;
	break;
      case GL_DEPTH_WRITEMASK:
	*lp++ = gc->state.depth.writeEnable;
	break;
      case GL_DEPTH_CLEAR_VALUE:
	/* This gets scaled like colors, to [0, 2^31-1] */
	*cp++ = gc->state.depth.clear;
	break;
      case GL_DEPTH_FUNC:
	*ip++ = gc->state.depth.testFunc;
	break;
      case GL_LIST_BASE:
	*ip++ = gc->state.list.listBase;
	break;
      case GL_LIST_INDEX:
	*ip++ = gc->dlist.currentList;
	break;
      case GL_LIST_MODE:
	*ip++ = gc->dlist.mode;
	break;
      case GL_ACCUM_CLEAR_VALUE:
	*cp++ = gc->state.accum.clear.r;
	*cp++ = gc->state.accum.clear.g;
	*cp++ = gc->state.accum.clear.b;
	*cp++ = gc->state.accum.clear.a;
	break;
      case GL_STENCIL_CLEAR_VALUE:
	*ip++ = gc->state.stencil.clear;
	break;
      case GL_STENCIL_FUNC:
	*ip++ = gc->state.stencil.testFunc;
	break;
      case GL_STENCIL_VALUE_MASK:
	*ip++ = gc->state.stencil.mask;
	break;
      case GL_STENCIL_FAIL:
	*ip++ = gc->state.stencil.fail;
	break;
      case GL_STENCIL_PASS_DEPTH_FAIL:
	*ip++ = gc->state.stencil.depthFail;
	break;
      case GL_STENCIL_PASS_DEPTH_PASS:
	*ip++ = gc->state.stencil.depthPass;
	break;
      case GL_STENCIL_REF:
	*ip++ = gc->state.stencil.reference;
	break;
      case GL_STENCIL_WRITEMASK:
	*ip++ = gc->state.stencil.writeMask;
	break;
      case GL_MATRIX_MODE:
	*ip++ = gc->state.transform.matrixMode;
	break;
      case GL_VIEWPORT:
	*ip++ = gc->state.viewport.x;
	*ip++ = gc->state.viewport.y;
	*ip++ = gc->state.viewport.width;
	*ip++ = gc->state.viewport.height;
	break;
      case GL_ATTRIB_STACK_DEPTH:
	*ip++ = gc->attributes.stackPointer - gc->attributes.stack;
	break;
      case GL_CLIENT_ATTRIB_STACK_DEPTH:
	*ip++ = gc->attributes.clientStackPointer - gc->attributes.clientStack;
	break;
      case GL_MODELVIEW_STACK_DEPTH:
	*ip++ = 1 + (gc->transform.modelView - gc->transform.modelViewStack);
	break;
      case GL_PROJECTION_STACK_DEPTH:
	*ip++ = 1 + (gc->transform.projection - gc->transform.projectionStack);
	break;
      case GL_TEXTURE_STACK_DEPTH:
	*ip++ = 1 + (gc->transform.texture[0] - gc->transform.textureStack[0]);	/* unit 0 only */
	break;
      case GL_MODELVIEW_MATRIX:
	mp = &gc->transform.modelView->matrix.matrix[0][0];
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	break;
      case GL_PROJECTION_MATRIX:
	mp = &gc->transform.projection->matrix.matrix[0][0];
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	break;
      case GL_TEXTURE_MATRIX:
	mp = &gc->transform.texture[gc->fx.activeTexture]->matrix.matrix[0][0];
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	*fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++; *fp++ = *mp++;
	break;
      case GL_ALPHA_TEST_FUNC:
	*ip++ = gc->state.raster.alphaFunction;
	break;
      case GL_ALPHA_TEST_REF:
	*fp++ = gc->state.raster.alphaReference;
	break;
      case GL_BLEND_DST:
	*ip++ = gc->state.raster.blendDst;
	break;
      case GL_BLEND_SRC:
	*ip++ = gc->state.raster.blendSrc;
	break;
      case GL_LOGIC_OP_MODE:
	*ip++ = gc->state.raster.logicOp;
	break;
      case GL_DRAW_BUFFER:
	*ip++ = gc->state.raster.drawBufferReturn;
	break;
      case GL_READ_BUFFER:
	*ip++ = gc->state.pixel.readBufferReturn;
	break;
      case GL_SCISSOR_BOX:
	*ip++ = gc->state.scissor.scissorX;
	*ip++ = gc->state.scissor.scissorY;
	*ip++ = gc->state.scissor.scissorWidth;
	*ip++ = gc->state.scissor.scissorHeight;
	break;
      case GL_INDEX_CLEAR_VALUE:
	*fp++ = gc->state.raster.clearIndex;
	break;
      case GL_INDEX_MODE:
	*lp++ = gc->modes.colorIndexMode ? GL_TRUE : GL_FALSE;
	break;
      case GL_INDEX_WRITEMASK:
	*ip++ = gc->state.raster.writeMask;
	break;
      case GL_COLOR_CLEAR_VALUE:
	*cp++ = gc->state.raster.clear.r;
	*cp++ = gc->state.raster.clear.g;
	*cp++ = gc->state.raster.clear.b;
	*cp++ = gc->state.raster.clear.a;
	break;
      case GL_RGBA_MODE:
	*lp++ = gc->modes.rgbMode ? GL_TRUE : GL_FALSE;
	break;
      case GL_COLOR_WRITEMASK:
	*lp++ = gc->state.raster.rMask;
	*lp++ = gc->state.raster.gMask;
	*lp++ = gc->state.raster.bMask;
	*lp++ = gc->state.raster.aMask;
	break;
      case GL_DOUBLEBUFFER:
	*lp++ = gc->modes.doubleBufferMode ? GL_TRUE : GL_FALSE;
	break;
      case GL_AUX_BUFFERS:
	*ip++ = gc->modes.numAuxBuffers;
	break;
      case GL_STEREO:
	*lp++ = GL_FALSE;
	break;
      case GL_RENDER_MODE:
	*ip++ = gc->renderMode;
	break;
      case GL_PERSPECTIVE_CORRECTION_HINT:
	*ip++ = gc->state.hints.perspectiveCorrection;
	break;
      case GL_POINT_SMOOTH_HINT:
	*ip++ = gc->state.hints.pointSmooth;
	break;
      case GL_LINE_SMOOTH_HINT:
	*ip++ = gc->state.hints.lineSmooth;
	break;
      case GL_POLYGON_SMOOTH_HINT:
	*ip++ = gc->state.hints.polygonSmooth;
	break;
      case GL_FOG_HINT:
	*ip++ = gc->state.hints.fog;
	break;
      case GL_PACK_SWAP_BYTES:
	*lp++ = gc->state.pixel.packModes.swapEndian;
	break;
      case GL_PACK_LSB_FIRST:
	*lp++ = gc->state.pixel.packModes.lsbFirst;
	break;
      case GL_PACK_ROW_LENGTH:
	*ip++ = gc->state.pixel.packModes.lineLength;
	break;
      case GL_PACK_SKIP_ROWS:
	*ip++ = gc->state.pixel.packModes.skipLines;
	break;
      case GL_UNPACK_SWAP_BYTES:
	*lp++ = gc->state.pixel.unpackModes.swapEndian;
	break;
      case GL_UNPACK_LSB_FIRST:
	*lp++ = gc->state.pixel.unpackModes.lsbFirst;
	break;
      case GL_UNPACK_ROW_LENGTH:
	*ip++ = gc->state.pixel.unpackModes.lineLength;
	break;
      case GL_UNPACK_SKIP_ROWS:
	*ip++ = gc->state.pixel.unpackModes.skipLines;
	break;
      case GL_UNPACK_SKIP_PIXELS:
	*ip++ = gc->state.pixel.unpackModes.skipPixels;
	break;
      case GL_UNPACK_ALIGNMENT:
	*ip++ = gc->state.pixel.unpackModes.alignment;
	break;
      case GL_PIXEL_MAP_I_TO_I_SIZE:
      case GL_PIXEL_MAP_S_TO_S_SIZE:
      case GL_PIXEL_MAP_I_TO_R_SIZE:
      case GL_PIXEL_MAP_I_TO_G_SIZE:
      case GL_PIXEL_MAP_I_TO_B_SIZE:
      case GL_PIXEL_MAP_I_TO_A_SIZE:
      case GL_PIXEL_MAP_R_TO_R_SIZE:
      case GL_PIXEL_MAP_G_TO_G_SIZE:
      case GL_PIXEL_MAP_B_TO_B_SIZE:
      case GL_PIXEL_MAP_A_TO_A_SIZE:
	index = sq - GL_PIXEL_MAP_I_TO_I_SIZE;
	*ip++ = gc->state.pixel.map[index].size;
	break;
      case GL_PACK_SKIP_PIXELS:
	*ip++ = gc->state.pixel.packModes.skipPixels;
	break;
      case GL_MAX_TEXTURE_SIZE:
	*ip++ = gc->constants.maxTextureSize;
	break;
      case GL_SUBPIXEL_BITS:
	*ip++ = gc->constants.subpixelBits;
	break;
      case GL_PACK_ALIGNMENT:
	*ip++ = gc->state.pixel.packModes.alignment;
	break;
      case GL_MAP_COLOR:
	*lp++ = gc->state.pixel.transferMode.mapColor;
	break;
      case GL_MAP_STENCIL:
	*lp++ = gc->state.pixel.transferMode.mapStencil;
	break;
      case GL_INDEX_SHIFT:
	*ip++ = gc->state.pixel.transferMode.indexShift;
	break;
      case GL_INDEX_OFFSET:
	*ip++ = gc->state.pixel.transferMode.indexOffset;
	break;
      case GL_RED_SCALE:
	*fp++ = gc->state.pixel.transferMode.scale.r;
	break;
      case GL_GREEN_SCALE:
	*fp++ = gc->state.pixel.transferMode.scale.g;
	break;
      case GL_BLUE_SCALE:
	*fp++ = gc->state.pixel.transferMode.scale.b;
	break;
      case GL_ALPHA_SCALE:
	*fp++ = gc->state.pixel.transferMode.scale.a;
	break;
      case GL_DEPTH_SCALE:
	*fp++ = gc->state.pixel.transferMode.depthScale;
	break;
      case GL_RED_BIAS:
	*fp++ = gc->state.pixel.transferMode.bias.r;
	break;
      case GL_GREEN_BIAS:
	*fp++ = gc->state.pixel.transferMode.bias.g;
	break;
      case GL_BLUE_BIAS:
	*fp++ = gc->state.pixel.transferMode.bias.b;
	break;
      case GL_ALPHA_BIAS:
	*fp++ = gc->state.pixel.transferMode.bias.a;
	break;
      case GL_DEPTH_BIAS:
	*fp++ = gc->state.pixel.transferMode.depthBias;
	break;
      case GL_ZOOM_X:
	*fp++ = gc->state.pixel.transferMode.zoomX;
	break;
      case GL_ZOOM_Y:
	*fp++ = gc->state.pixel.transferMode.zoomY;
	break;
      case GL_MAX_EVAL_ORDER:
	*ip++ = gc->constants.maxEvalOrder;
	break;
      case GL_MAX_LIGHTS:
	*ip++ = gc->constants.numberOfLights;
	break;
      case GL_MAX_CLIP_PLANES:
	*ip++ = gc->constants.numberOfClipPlanes;
	break;
      case GL_MAX_PIXEL_MAP_TABLE:
	*ip++ = gc->constants.maxPixelMapTable;
	break;
      case GL_MAX_ATTRIB_STACK_DEPTH:
	*ip++ = gc->constants.maxAttribStackDepth;
	break;
      case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
	*ip++ = gc->constants.maxClientAttribStackDepth;
	break;
      case GL_MAX_MODELVIEW_STACK_DEPTH:
	*ip++ = gc->constants.maxModelViewStackDepth;
	break;
      case GL_MAX_NAME_STACK_DEPTH:
	*ip++ = gc->constants.maxNameStackDepth;
	break;
      case GL_MAX_PROJECTION_STACK_DEPTH:
	*ip++ = gc->constants.maxProjectionStackDepth;
	break;
      case GL_MAX_TEXTURE_STACK_DEPTH:
	*ip++ = gc->constants.maxTextureStackDepth;
	break;
      case GL_INDEX_BITS:
	*ip++ = gc->modes.indexBits;
	break;
      case GL_RED_BITS:
	*ip++ = gc->modes.redBits;
	break;
      case GL_GREEN_BITS:
	*ip++ = gc->modes.greenBits;
	break;
      case GL_BLUE_BITS:
	*ip++ = gc->modes.blueBits;
	break;
      case GL_ALPHA_BITS:
	*ip++ = gc->modes.alphaBits;
	break;
      case GL_MAX_VIEWPORT_DIMS:
	*ip++ = gc->constants.maxViewportWidth;
	*ip++ = gc->constants.maxViewportHeight;
	break;
      case GL_DEPTH_BITS:
	*ip++ = gc->modes.depthBits;
	break;
      case GL_STENCIL_BITS:
	*ip++ = gc->modes.stencilBits;
	break;
      case GL_ACCUM_RED_BITS:
	*ip++ = gc->modes.accumRedBits;
	break;
      case GL_ACCUM_GREEN_BITS:
	*ip++ = gc->modes.accumGreenBits;
	break;
      case GL_ACCUM_BLUE_BITS:
	*ip++ = gc->modes.accumBlueBits;
	break;
      case GL_ACCUM_ALPHA_BITS:
	*ip++ = gc->modes.accumAlphaBits;
	break;
      case GL_NAME_STACK_DEPTH:
	*ip++ = gc->select.sp - gc->select.stack;
	break;
      case GL_MAP1_GRID_DOMAIN:
	*fp++ = gc->state.evaluator.u1.start;
	*fp++ = gc->state.evaluator.u1.finish;
	break;
      case GL_MAP1_GRID_SEGMENTS:
	*ip++ = gc->state.evaluator.u1.n;
	break;
      case GL_MAP2_GRID_DOMAIN:
	*fp++ = gc->state.evaluator.u2.start;
	*fp++ = gc->state.evaluator.u2.finish;
	*fp++ = gc->state.evaluator.v2.start;
	*fp++ = gc->state.evaluator.v2.finish;
	break;
      case GL_MAP2_GRID_SEGMENTS:
	*ip++ = gc->state.evaluator.u2.n;
	*ip++ = gc->state.evaluator.v2.n;
	break;
      case GL_FEEDBACK_BUFFER_SIZE:
	*ip++ = gc->feedback.resultLength;
	break;
      case GL_POLYGON_OFFSET_UNITS:
	*fp++ = gc->state.polygon.units;
	break;
      case GL_SELECTION_BUFFER_SIZE:
	*ip++ = gc->select.resultLength;
	break;
      case GL_FEEDBACK_BUFFER_TYPE:
	*ip++ = gc->feedback.type;
	break;
      case GL_BLEND_EQUATION:
	*ip++ = gc->state.raster.blendEquation;
	break;
      case GL_POLYGON_OFFSET_FACTOR:
	*fp++ = gc->state.polygon.factor;
	break;
      case GL_TEXTURE_BINDING_1D:
	*ip++ = __glLookupActiveTextureState(gc, GL_TEXTURE_1D)->name;
	break;
      case GL_TEXTURE_BINDING_2D:
	*ip++ = __glLookupActiveTextureState(gc, GL_TEXTURE_2D)->name;
	break;
      case GL_VERTEX_ARRAY_SIZE:
	*ip++ = gc->vertexArray.vertex.size;
	break;
      case GL_VERTEX_ARRAY_TYPE:
	*ip++ = gc->vertexArray.vertex.type;
	break;
      case GL_VERTEX_ARRAY_STRIDE:
	*ip++ = gc->vertexArray.vertex.userStride;
	break;
      case GL_VERTEX_ARRAY_COUNT_EXT:
	*ip++ = gc->vertexArray.vertex.count;
	break;
      case GL_NORMAL_ARRAY_TYPE:
	*ip++ = gc->vertexArray.normal.type;
	break;
      case GL_NORMAL_ARRAY_STRIDE:
	*ip++ = gc->vertexArray.normal.userStride;
	break;
      case GL_NORMAL_ARRAY_COUNT_EXT:
	*ip++ = gc->vertexArray.normal.count;
	break;
      case GL_COLOR_ARRAY_SIZE:
	*ip++ = gc->vertexArray.color.size;
	break;
      case GL_COLOR_ARRAY_TYPE:
	*ip++ = gc->vertexArray.color.type;
	break;
      case GL_COLOR_ARRAY_STRIDE:
	*ip++ = gc->vertexArray.color.userStride;
	break;
      case GL_COLOR_ARRAY_COUNT_EXT:
	*ip++ = gc->vertexArray.color.count;
	break;
      case GL_INDEX_ARRAY_TYPE:
	*ip++ = gc->vertexArray.index.type;
	break;
      case GL_INDEX_ARRAY_STRIDE:
	*ip++ = gc->vertexArray.index.userStride;
	break;
      case GL_INDEX_ARRAY_COUNT_EXT:
	*ip++ = gc->vertexArray.index.count;
	break;
      case GL_TEXTURE_COORD_ARRAY_SIZE:
	*ip++ = gc->vertexArray.texture.size[gc->fx.clientActiveTexture];
	break;
      case GL_TEXTURE_COORD_ARRAY_TYPE:
	*ip++ = gc->vertexArray.texture.type[gc->fx.clientActiveTexture];
	break;
      case GL_TEXTURE_COORD_ARRAY_STRIDE:
	*ip++ = gc->vertexArray.texture.userStride[gc->fx.clientActiveTexture];
	break;
      case GL_TEXTURE_COORD_ARRAY_COUNT_EXT:
	*ip++ = gc->vertexArray.texture.count[gc->fx.clientActiveTexture];
	break;
      case GL_EDGE_FLAG_ARRAY_STRIDE:
	*ip++ = gc->vertexArray.edgeFlag.userStride;
	break;
      case GL_EDGE_FLAG_ARRAY_COUNT_EXT:
	*ip++ = gc->vertexArray.edgeFlag.count;
	break;
      case 0x80F0:
	*ip++ = gc->state.hints.ext590;		/* GL_CLIP_VOLUME_CLIPPING_HINT_EXT */
	break;
      case 0x8126:
	*fp++ = gc->state.point.ext[0];		/* GL_POINT_SIZE_MIN_EXT */
	break;
      case 0x8127:
	*fp++ = gc->state.point.ext[1];		/* GL_POINT_SIZE_MAX_EXT */
	break;
      case 0x8128:
	*fp++ = gc->state.point.ext[2];		/* GL_POINT_FADE_THRESHOLD_SIZE_EXT */
	break;
      case 0x8129:
	*fp++ = gc->state.point.ext[3];		/* GL_DISTANCE_ATTENUATION_EXT */
	*fp++ = gc->state.point.ext[4];
	*fp++ = gc->state.point.ext[5];
	break;
      case 0x81B6:
	*ip++ = gc->state.raster.indexFunction;	/* GL_INDEX_TEST_FUNC_EXT */
	break;
      case 0x81B7:
	*fp++ = gc->state.raster.indexReference;	/* GL_INDEX_TEST_REF_EXT */
	break;
      case GL_ARRAY_ELEMENT_LOCK_FIRST_EXT:
	*ip++ = gc->vertexArray.lockFirst;
	break;
      case GL_ARRAY_ELEMENT_LOCK_COUNT_EXT:
	*ip++ = gc->vertexArray.lockCount;
	break;
      case GL_CULL_VERTEX_EYE_POSITION_EXT:
	*fp++ = gc->state.transform.eyePos.x;
	*fp++ = gc->state.transform.eyePos.y;
	*fp++ = gc->state.transform.eyePos.z;
	*fp++ = gc->state.transform.eyePos.w;
	break;
      case GL_CULL_VERTEX_OBJECT_POSITION_EXT:
	*fp++ = gc->state.transform.eyePosObj.x;
	*fp++ = gc->state.transform.eyePosObj.y;
	*fp++ = gc->state.transform.eyePosObj.z;
	*fp++ = gc->state.transform.eyePosObj.w;
	break;
      case GL_ALPHA_TEST:
      case GL_BLEND:
      case GL_COLOR_MATERIAL:
      case GL_CULL_FACE:
      case GL_CULL_VERTEX_EXT:
      case GL_DEPTH_TEST:
      case GL_DITHER:
      case GL_FOG:
      case GL_LIGHTING:
      case GL_LINE_SMOOTH:
      case GL_LINE_STIPPLE:
      case GL_INDEX_LOGIC_OP:
      case GL_COLOR_LOGIC_OP:
      case GL_NORMALIZE:
      case GL_POINT_SMOOTH:
      case GL_POLYGON_SMOOTH:
      case GL_POLYGON_STIPPLE:
      case GL_SCISSOR_TEST:
      case GL_STENCIL_TEST:
      case GL_TEXTURE_1D:
      case GL_TEXTURE_2D:
      case GL_AUTO_NORMAL:
      case GL_TEXTURE_GEN_S:
      case GL_TEXTURE_GEN_T:
      case GL_TEXTURE_GEN_R:
      case GL_TEXTURE_GEN_Q:
      case GL_CLIP_PLANE0:
      case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2:
      case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4:
      case GL_CLIP_PLANE5:
      case GL_LIGHT0:
      case GL_LIGHT1:
      case GL_LIGHT2:
      case GL_LIGHT3:
      case GL_LIGHT4:
      case GL_LIGHT5:
      case GL_LIGHT6:
      case GL_LIGHT7:
      case GL_MAP1_COLOR_4:
      case GL_MAP1_NORMAL:
      case GL_MAP1_INDEX:
      case GL_MAP1_TEXTURE_COORD_1:
      case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3:
      case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3:
      case GL_MAP1_VERTEX_4:
      case GL_MAP2_COLOR_4:
      case GL_MAP2_NORMAL:
      case GL_MAP2_INDEX:
      case GL_MAP2_TEXTURE_COORD_1:
      case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3:
      case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3:
      case GL_MAP2_VERTEX_4:
      case GL_POLYGON_OFFSET_POINT:
      case GL_POLYGON_OFFSET_LINE:
      case GL_POLYGON_OFFSET_FILL:
      case GL_VERTEX_ARRAY:
      case GL_NORMAL_ARRAY:
      case GL_COLOR_ARRAY:
      case GL_INDEX_ARRAY:
      case GL_TEXTURE_COORD_ARRAY:
      case GL_EDGE_FLAG_ARRAY:
      case 0x81B5:
      case 0x81B8:
	*lp++ = __glim_IsEnabled(sq);
	break;
      case 0x81BA:
	*ip++ = gc->state.light.indexMaterialFace;	/* GL_INDEX_MATERIAL_FACE_EXT */
	break;
      case 0x81B9:
	*ip++ = gc->state.light.indexMaterialParam;	/* GL_INDEX_MATERIAL_PARAMETER_EXT */
	break;
      case GL_FOG_COORDINATE_SOURCE_EXT:
	*ip++ = gc->state.fog.coordSource;		/* GL_FOG_COORDINATE_SOURCE_EXT */
	break;
      case 0x8454:
	*ip++ = gc->vertexArray.fogCoord.type;	/* GL_FOG_COORDINATE_ARRAY_TYPE_EXT */
	break;
      case 0x8453:
	*fp++ = __GL_ONE_F - gc->state.current.fogCoord;	/* GL_CURRENT_FOG_COORDINATE_EXT */
	break;
      case 0x8455:
	*ip++ = gc->vertexArray.fogCoord.userStride;	/* GL_FOG_COORDINATE_ARRAY_STRIDE_EXT */
	break;
      case GL_CLIENT_ACTIVE_TEXTURE_ARB:
	*ip++ = gc->fx.clientActiveTexture + GL_TEXTURE0_ARB;
	break;
      case GL_ACTIVE_TEXTURE_ARB:
	*ip++ = gc->fx.activeTexture + GL_TEXTURE0_ARB;
	break;
      case GL_MAX_TEXTURE_UNITS_ARB:
	*ip++ = gc->constants.maxTextureUnits;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
      case 0x86B4:
	*ip++ = gc->fx.numSamples;			/* GL_SAMPLES_3DFX */
	break;
      case 0x86B3:
	*ip++ = gc->fx.numSamples > 0;		/* GL_SAMPLE_BUFFERS_3DFX */
	break;
    }

    /* Use the motion of the pointers to type convert the result */
    if (ip != itemp) {
	__glConvertResult(gc, __GL_INT32, itemp, type, result, ip - itemp);
    } else
    if (fp != ftemp) {
	__glConvertResult(gc, __GL_FLOAT, ftemp, type, result, fp - ftemp);
    } else
    if (lp != ltemp) {
	__glConvertResult(gc, __GL_BOOLEAN, ltemp, type, result, lp - ltemp);
    } else
    if (cp != ctemp) {
	__glConvertResult(gc, __GL_COLOR, ctemp, type, result, cp - ctemp);
    } else
    if (scp != sctemp) {
	__glConvertResult(gc, __GL_SCOLOR, sctemp, type, result, scp - sctemp);
    }
}
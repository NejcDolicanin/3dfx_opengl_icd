/*
** gl_lcomp3.c -- the display-list compile entries (__gllc_*, SI g_lcomp.c / s_lcomp.c) written by hand;
** gl_lcomp1.c and gl_lcomp2.c hold the rest, in address order.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "si_names.h"
#include "icdprotos.h"
#include <string.h>
#include <math.h>
#include "glshade.h"
#include "icdglob.h"
#include <stdlib.h>

/* ---- generated-style list compilers (g_lcomp.c): a pname-sized array copied after the header.
** The SI opcode numbering: Fogfv 0x50, Lightiv 0x58, LightModeliv 0x5c, Materialfv 0x60,
** Materialiv 0x62, TexParameteriv 0x6b, TexEnviv 0x71, TexGenfv 0x75, TexGeniv 0x77 */
/* the iv / fv size procs: one is a 5-byte jump to its twin (s_size.c / s_tex.c / gl_size.c), called by its own
** name as in the original */
extern GLint __fastcall __glLightiv_size(GLenum pname);
extern GLint __fastcall __glTexParameteriv_size(GLenum pname);
extern const GLubyte *__fastcall __glle_Fogfv(const GLubyte *PC);
extern const GLubyte *__fastcall __glle_Lightfv(const GLubyte *PC);
/* a compile entry with (a, pname, params): the record holds a and pname, then the values */
#define LCOMP3(func, atype, ptype, sizefn, elsize, op, aligned_, exec)                   \
void __fastcall func(GLuint ecx_unused, GLuint edx_unused, atype a, GLenum pname,           \
                     const ptype *params)                                                   \
{                                                                                           \
    __GLdlistOpICD *dlop;                                                                   \
    struct { atype a; GLenum pname; } *data;                                                \
    GLint size;                                                                             \
    __GL_SETUP();                                                                           \
                                                                                            \
    size = sizefn(pname) * elsize;                                                          \
    if (size < 0) {                                                                         \
	__gllc_InvalidEnum(gc);                                                         \
	return;                                                                             \
    }                                                                                       \
    dlop = __glDlistAllocOp2(gc, (size + 8));                                            \
    if (dlop == NULL) return;                                                               \
    dlop->opcode = op;                                                                      \
    aligned_                                                                                \
    data = (void *) dlop->data;                                                             \
    data->a = a;                                                                            \
    data->pname = pname;                                                                    \
    memcpy(data + 1, params, size);                                                         \
    __glDlistAppendOp(gc, dlop, exec);                                                  \
}
/************************************************************************/
/* g_lcomp.c single-value list compilers: the value's pname must have size 1, then the vector
** compiler (a __stdcall-shaped fastcall entry: no register arguments) gets its address */
extern const GLubyte *__fastcall __glle_Lightiv(const GLubyte *PC);
extern const GLubyte *__fastcall __glle_LightModeliv(const GLubyte *PC);
extern void __stdcall __gllc_Error(__GLcontext *gc, GLenum error);
extern const GLubyte *__fastcall __glle_Materialfv(const GLubyte *PC);
extern const GLubyte *__fastcall __glle_Materialiv(const GLubyte *PC);
extern const GLubyte *__fastcall __glle_TexParameterfv(const GLubyte *PC);
extern const GLubyte *__fastcall __glle_TexParameteriv(const GLubyte *PC);
extern const GLubyte *__fastcall __glle_TexEnvfv(const GLubyte *PC);
extern const GLubyte *__fastcall __glle_TexEnviv(const GLubyte *PC);
extern const GLubyte *__fastcall __glle_TexGenfv(const GLubyte *PC);
extern const GLubyte *__fastcall __glle_TexGeniv(const GLubyte *PC);
/************************************************************************/
/************************************************************************/
/* s_lcomp.c: display list compile / execute procs */
struct __gllc_Map1f_W20Rec {
    GLenum    target;
    __GLfloat u1;
    __GLfloat u2;
    GLint     order;
    /*        points  */
};
struct __gllc_Map2f_Rec {
    GLenum    target;
    __GLfloat u1;
    __GLfloat u2;
    GLint     uorder;
    __GLfloat v1;
    __GLfloat v2;
    GLint     vorder;
    /*        points  */
};
/* ---- s_lcomp.c display-list executors ----------------------------------------------------- */
struct __gllc_DrawPixels_Rec {
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
};
/* dlist plumbing: 690b44d0 __glDlistAllocOp2 (opcode word at +0x0c, data at +0x10), 690b4520
** __glDlistAppendOp; 69096060 __glFillImage (__gllc_InvalidValue / InvalidEnum: glsi.h) */
#ifndef GL_UNSIGNED_BYTE_3_3_2
#define GL_UNSIGNED_BYTE_3_3_2          0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4       0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1       0x8034
#define GL_UNSIGNED_INT_8_8_8_8         0x8035
#define GL_UNSIGNED_INT_10_10_10_2      0x8036
#endif
#define __GL_PAD_SHR(x) ((((x) + 3) >> 2) << 2)    /* __GL_PAD in the shift form */
/* the packed pixel types this kit knows */
#define GL_UNSIGNED_BYTE_3_3_2         0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4      0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1      0x8034
#define GL_UNSIGNED_INT_8_8_8_8        0x8035
#define GL_UNSIGNED_INT_10_10_10_2     0x8036
typedef void (APIENTRY *__GLtexImage1DProc)(GLenum target, GLint level, GLint internalformat,
	GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
struct __gllc_TexImage1D_Rec {
    GLenum      target;
    GLint       level;
    GLint       internalformat;
    GLsizei     width;
    GLint       border;
    GLenum      format;
    GLenum      type;
    GLint       imageSize;
};
/* 690a50d0 (__glMipMapFragment) is in gl_texfrag.c */
#ifndef GL_PROXY_TEXTURE_2D
#define GL_PROXY_TEXTURE_2D     0x8064
#endif
struct __gllc_TexImage2D_Rec {
    GLenum target;
    GLint level;
    GLint internalformat;
    GLint width;
    GLint height;
    GLint border;
    GLenum format;
    GLenum type;
    GLint imageSize;
};
/* __gllc_TexSubImage1D (690b3010 below), as gl_lcomp3.c's __gllc_TexSubImage2D */
struct __gllc_TexSubImage1D_W20Rec {
    GLenum      target;
    GLint       level;
    GLint       xoffset;
    GLsizei     width;
    GLenum      format;
    GLenum      type;
    GLint       imageSize;
};
struct __gllc_TexSubImage2D_Rec {
    GLenum      target;
    GLint       level;
    GLint       xoffset;
    GLint       yoffset;
    GLsizei     width;
    GLsizei     height;
    GLenum      format;
    GLenum      type;
    GLint       imageSize;
};
/************************************************************************/
/* display-list compile side (dl_*.c / s_lcomp.c) */
#define __glle_DisableClientState ((__GLlistExecFunc)__glle_Disable)
struct __gllc_DisableClientState_Rec {
    GLenum      array;
};
/************************************************************************/
/* s_lcomp.c: EXT_paletted_texture colour tables in display lists */
struct __gllc_ColorTable_Rec {
    GLenum      target;
    GLenum      internalformat;
    GLsizei     width;
    GLenum      format;
    GLenum      type;
    GLint       imageSize;
};
typedef void (__fastcall *__GLcolorTableProc)(__GLcontext *gc, GLenum target, GLenum internalformat,
	GLsizei width, GLenum format, GLenum type, const GLvoid *table, GLboolean packed);
/* 69082a30 __glCheckColorTableArgs (gl_ctable.c), 690b3810 __glle_ColorSubTable */
#ifndef GL_TABLE_TOO_LARGE
#define GL_TABLE_TOO_LARGE      0x8031
#endif
struct __gllc_ColorSubTable_Rec {
    GLenum target;
    GLsizei start;
    GLsizei count;
    GLenum format;
    GLenum type;
    GLint imageSize;
};
typedef void (__fastcall *__GLcolorSubTableProc)(__GLcontext *gc, GLenum target, GLsizei start,
	GLsizei count, GLenum format, GLenum type, const GLvoid *table, GLboolean packed);
/************************************************************************/
/* s_lcomp.c: ArrayElement while compiling (executed and compiled through the dispatch tables) */
#define __GL_ARRAY_ELEMENT(i)   ((void (APIENTRY *)(GLint)) glArrayElementEXT)(i)
/* ---- g_lcomp.c: display-list compile entries ---------------------------------------------- */
struct __gllc_Fogiv_Rec { GLenum pname; };
struct __gllc_LightModelfv_Rec { GLenum pname; };
struct __gllc_Vertex3d_Rec { GLdouble x, y, z; };
struct __gllc_Vertex4d_Rec { GLdouble x, y, z, w; };
struct __gllc_PixelMapusv_Rec { GLenum map; GLint mapsize; };
struct __gllc_PointParameterfvEXT_Rec { GLenum pname; };
/* ARB_texture_compression compile entries: the record holds the arguments and imageSize, then the
** image padded to a word (the padded byte count is copied from the caller's buffer) */
struct CompressedTexImage1DARB_RecW14 {GLenum target; GLint level; GLenum internalformat; GLsizei width; GLint border; GLsizei imageSize; };
struct CompressedTexImage2DARB_RecW14 {GLenum target; GLint level; GLenum internalformat; GLsizei width; GLsizei height; GLint border; GLsizei imageSize; };
struct CompressedTexSubImage1DARB_RecW14 {GLenum target; GLint level; GLint xoffset; GLsizei width; GLenum format; GLsizei imageSize; };
struct CompressedTexSubImage2DARB_RecW14 {GLenum target; GLint level; GLint xoffset; GLint yoffset; GLsizei width; GLsizei height; GLenum format; GLsizei imageSize; };
struct __gllc_Map2f_W20Rec {
    GLenum    target;
    __GLfloat u1;
    __GLfloat u2;
    GLint     uorder;
    __GLfloat v1;
    __GLfloat v2;
    GLint     vorder;
    /*        points  */
};
typedef void (APIENTRY *__GLmap2fProc)(GLenum, GLfloat, GLfloat, GLint, GLint,
					  GLfloat, GLfloat, GLint, GLint, const GLfloat *);
typedef void (APIENTRY *__GLmap1fProc)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
extern void __fastcall __gllc_Lightfv(GLuint ecx_unused, GLuint edx_unused, GLenum a, GLenum pname, const GLfloat *params);
extern void __stdcall __gllc_Lightiv(GLenum light, GLenum pname, const GLint *params);
extern void __stdcall __gllc_LightModeliv(GLenum pname, const GLint *params);
extern void __stdcall __gllc_Materialfv(GLenum face, GLenum pname, const GLfloat *params);
extern void __stdcall __gllc_Materialiv(GLenum face, GLenum pname, const GLint *params);
extern void __fastcall __gllc_TexParameterfv(GLuint ecx_unused, GLuint edx_unused, GLenum a, GLenum pname, const GLfloat *params);
extern void __stdcall __gllc_TexParameteriv(GLenum target, GLenum pname, const GLint *params);
extern void __fastcall __gllc_TexEnvfv(GLuint ecx_unused, GLuint edx_unused, GLenum a, GLenum pname, const GLfloat *params);
extern void __stdcall __gllc_TexEnviv(GLenum target, GLenum pname, const GLint *params);
extern void __fastcall __gllc_TexGendv(GLuint ecx_unused, GLuint edx_unused, GLenum a, GLenum pname, const GLdouble *params);
extern void __stdcall __gllc_TexGenfv(GLenum coord, GLenum pname, const GLfloat *params);
extern void __stdcall __gllc_TexGeniv(GLenum coord, GLenum pname, const GLint *params);
extern const GLubyte * __fastcall __glle_Bitmap(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_Map1(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_Map2(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_ColorTable(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_ColorSubTable(const GLubyte *PC);

/* 690aaae0 -- __gllc_Fogfv */
void __stdcall __gllc_Fogfv(GLenum pname, const GLfloat *params)
{
    __GLdlistOpICD *dlop;
    GLenum *data;
    GLint arraySize;
    __GL_SETUP();

    arraySize = __glFogfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (GLuint)(4 + arraySize));
    if (dlop == NULL) return;
    dlop->opcode = 0x50;
    data = (GLenum *) dlop->data;
    data[0] = pname;
    memcpy(data + 1, params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_Fogfv);
}

/* 690aacd0 -- __gllc_Lightf */
void __stdcall __gllc_Lightf(GLenum light, GLenum pname, GLfloat param)
{
    __GL_SETUP();

    if (__glLightfv_size(pname) != 1) {
        __gllc_InvalidEnum(gc);
        return;
    }
    ((void (__stdcall *)(GLenum, GLenum, const GLfloat *))__gllc_Lightfv)(light, pname, &param);
}

/* 690aad20 -- __gllc_Lightfv */
LCOMP3(__gllc_Lightfv, GLenum, GLfloat, __glLightfv_size, 4, __glop_Lightfv, ;, __glle_Lightfv)

/* 690aada0 -- __gllc_Lighti */
void __stdcall __gllc_Lighti(GLenum light, GLenum pname, GLint param)
{
    __GL_SETUP();

    if (__glLightiv_size(pname) != 1) {
        __gllc_InvalidEnum(gc);
        return;
    }
    ((void (__stdcall *)(GLenum, GLenum, const GLint *))__gllc_Lightiv)(light, pname, &param);
}

/* 690aadf0 -- __gllc_Lightiv */
void __stdcall __gllc_Lightiv(GLenum light, GLenum pname, const GLint *params)
{
    __GLdlistOpICD *dlop;
    GLenum *data;
    GLint arraySize;
    __GL_SETUP();

    arraySize = __glLightiv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (GLuint)(8 + arraySize));
    if (dlop == NULL) return;
    dlop->opcode = 0x58;
    data = (GLenum *) dlop->data;
    data[0] = light;
    data[1] = pname;
    memcpy(data + 2, params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_Lightiv);
}

/* 690aaf30 -- __gllc_LightModeli */
void __stdcall __gllc_LightModeli(GLenum pname, GLint param)
{
    __GL_SETUP();

    if (__glLightModeliv_size(pname) != 1) {
        __gllc_InvalidEnum(gc);
        return;
    }
    ((void (__stdcall *)(GLenum, const GLint *))__gllc_LightModeliv)(pname, &param);
}

/* 690aaf70 -- __gllc_LightModeliv */
void __stdcall __gllc_LightModeliv(GLenum pname, const GLint *params)
{
    __GLdlistOpICD *dlop;
    GLenum *data;
    GLint arraySize;
    __GL_SETUP();

    arraySize = __glLightModeliv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (GLuint)(4 + arraySize));
    if (dlop == NULL) return;
    dlop->opcode = 0x5c;
    data = (GLenum *) dlop->data;
    data[0] = pname;
    memcpy(data + 1, params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_LightModeliv);
}

/* 690ab090 -- __gllc_Materialf */
void __stdcall __gllc_Materialf(GLenum face, GLenum pname, GLfloat param)
{
    __GL_SETUP();

    if (__glMaterialfv_size(pname) != 1) {
        __gllc_InvalidEnum(gc);
        return;
    }
    ((void (__stdcall *)(GLenum, GLenum, const GLfloat *))__gllc_Materialfv)(face, pname, &param);
}

/* 690ab0e0 -- __gllc_Materialfv */
void __stdcall __gllc_Materialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    __GLdlistOpICD *dlop;
    GLenum *data;
    GLint arraySize;
    GLenum error;
    __GL_SETUP();

    error = __glErrorCheckMaterial(face, pname, params[0]);
    if (error) {
	__gllc_Error(gc, error);
	return;
    }
    arraySize = __glMaterialfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (GLuint)(8 + arraySize));
    if (dlop == NULL) return;
    dlop->opcode = 0x60;
    data = (GLenum *) dlop->data;
    data[0] = face;
    data[1] = pname;
    memcpy(data + 2, params, arraySize);
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_MATERIAL;
    __glDlistAppendOp(gc, dlop, __glle_Materialfv);
}

/* 690ab190 -- __gllc_Materiali */
void __stdcall __gllc_Materiali(GLenum face, GLenum pname, GLint param)
{
    __GL_SETUP();

    if (__glMaterialiv_size(pname) != 1) {
        __gllc_InvalidEnum(gc);
        return;
    }
    ((void (__stdcall *)(GLenum, GLenum, const GLint *))__gllc_Materialiv)(face, pname, &param);
}

/* 690ab1e0 -- __gllc_Materialiv */
void __stdcall __gllc_Materialiv(GLenum face, GLenum pname, const GLint *params)
{
    __GLdlistOpICD *dlop;
    GLenum *data;
    GLint arraySize;
    GLenum error;
    __GL_SETUP();

    error = __glErrorCheckMaterial(face, pname, (GLfloat) params[0]);
    if (error) {
	__gllc_Error(gc, error);
	return;
    }
    arraySize = __glMaterialiv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (GLuint)(8 + arraySize));
    if (dlop == NULL) return;
    dlop->opcode = 0x62;
    data = (GLenum *) dlop->data;
    data[0] = face;
    data[1] = pname;
    memcpy(data + 2, params, arraySize);
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_MATERIAL;
    __glDlistAppendOp(gc, dlop, __glle_Materialiv);
}
/* 690ab3e0 -- __gllc_TexParameterf */
void __stdcall __gllc_TexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    __GL_SETUP();

    if (__glTexParameterfv_size(pname) != 1) {
        __gllc_InvalidEnum(gc);
        return;
    }
    ((void (__stdcall *)(GLenum, GLenum, const GLfloat *))__gllc_TexParameterfv)(target, pname, &param);
}

/* 690ab430 -- __gllc_TexParameterfv */
LCOMP3(__gllc_TexParameterfv, GLenum, GLfloat, __glTexParameterfv_size, 4, __glop_TexParameterfv, ;, __glle_TexParameterfv)

/* 690ab4b0 -- __gllc_TexParameteri */
void __stdcall __gllc_TexParameteri(GLenum target, GLenum pname, GLint param)
{
    __GL_SETUP();

    if (__glTexParameteriv_size(pname) != 1) {
        __gllc_InvalidEnum(gc);
        return;
    }
    ((void (__stdcall *)(GLenum, GLenum, const GLint *))__gllc_TexParameteriv)(target, pname, &param);
}

/* 690ab500 -- __gllc_TexParameteriv */
void __stdcall __gllc_TexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
    __GLdlistOpICD *dlop;
    GLenum *data;
    GLint arraySize;
    __GL_SETUP();

    arraySize = __glTexParameteriv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (GLuint)(8 + arraySize));
    if (dlop == NULL) return;
    dlop->opcode = 0x6b;
    data = (GLenum *) dlop->data;
    data[0] = target;
    data[1] = pname;
    memcpy(data + 2, params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_TexParameteriv);
}

/* 690ab580 -- __gllc_TexEnvf */
void __stdcall __gllc_TexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    __GL_SETUP();

    if (__glTexEnvfv_size(pname) != 1) {
        __gllc_InvalidEnum(gc);
        return;
    }
    ((void (__stdcall *)(GLenum, GLenum, const GLfloat *))__gllc_TexEnvfv)(target, pname, &param);
}

/* 690ab5d0 -- __gllc_TexEnvfv */
LCOMP3(__gllc_TexEnvfv, GLenum, GLfloat, __glTexEnvfv_size, 4, __glop_TexEnvfv, ;, __glle_TexEnvfv)

/* 690ab650 -- __gllc_TexEnvi */
void __stdcall __gllc_TexEnvi(GLenum target, GLenum pname, GLint param)
{
    __GL_SETUP();

    if (__glTexEnviv_size(pname) != 1) {
        __gllc_InvalidEnum(gc);
        return;
    }
    ((void (__stdcall *)(GLenum, GLenum, const GLint *))__gllc_TexEnviv)(target, pname, &param);
}

/* 690ab6a0 -- __gllc_TexEnviv */
void __stdcall __gllc_TexEnviv(GLenum target, GLenum pname, const GLint *params)
{
    __GLdlistOpICD *dlop;
    GLenum *data;
    GLint arraySize;
    __GL_SETUP();

    arraySize = __glTexEnviv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (GLuint)(8 + arraySize));
    if (dlop == NULL) return;
    dlop->opcode = 0x71;
    data = (GLenum *) dlop->data;
    data[0] = target;
    data[1] = pname;
    memcpy(data + 2, params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_TexEnviv);
}

/* 690ab720 -- __gllc_TexGend */
void __stdcall __gllc_TexGend(GLenum coord, GLenum pname, GLdouble param)
{
    __GL_SETUP();

    if (__glTexGendv_size(pname) != 1) {
        __gllc_InvalidEnum(gc);
        return;
    }
    ((void (__stdcall *)(GLenum, GLenum, const GLdouble *))__gllc_TexGendv)(coord, pname, &param);
}

/* 690ab770 -- __gllc_TexGendv (doubles: the op is marked aligned) */
LCOMP3(__gllc_TexGendv, GLenum, GLdouble, __glTexGendv_size, 8, __glop_TexGendv, dlop->aligned = 1;, __glle_TexGendv)

/* 690ab7f0 -- __gllc_TexGenf */
void __stdcall __gllc_TexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    __GL_SETUP();

    if (__glTexGenfv_size(pname) != 1) {
        __gllc_InvalidEnum(gc);
        return;
    }
    ((void (__stdcall *)(GLenum, GLenum, const GLfloat *))__gllc_TexGenfv)(coord, pname, &param);
}

/* 690ab840 -- __gllc_TexGenfv */
void __stdcall __gllc_TexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
    __GLdlistOpICD *dlop;
    GLenum *data;
    GLint arraySize;
    __GL_SETUP();

    arraySize = __glTexGenfv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (GLuint)(8 + arraySize));
    if (dlop == NULL) return;
    dlop->opcode = 0x75;
    data = (GLenum *) dlop->data;
    data[0] = coord;
    data[1] = pname;
    memcpy(data + 2, params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_TexGenfv);
}

/* 690ab8c0 -- __gllc_TexGeni */
void __stdcall __gllc_TexGeni(GLenum coord, GLenum pname, GLint param)
{
    __GL_SETUP();

    if (__glTexGeniv_size(pname) != 1) {
        __gllc_InvalidEnum(gc);
        return;
    }
    ((void (__stdcall *)(GLenum, GLenum, const GLint *))__gllc_TexGeniv)(coord, pname, &param);
}

/* 690ab910 -- __gllc_TexGeniv */
void __stdcall __gllc_TexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
    __GLdlistOpICD *dlop;
    GLenum *data;
    GLint arraySize;
    __GL_SETUP();

    arraySize = __glTexGeniv_size(pname) * 4;
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (GLuint)(8 + arraySize));
    if (dlop == NULL) return;
    dlop->opcode = 0x77;
    data = (GLenum *) dlop->data;
    data[0] = coord;
    data[1] = pname;
    memcpy(data + 2, params, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_TexGeniv);
}

/* 690ac7e0 -- __gllc_PixelMapfv */
void __stdcall __gllc_PixelMapfv(GLenum map, GLint mapsize, const GLfloat *values)
{
    __GLdlistOpICD *dlop;
    GLenum *data;
    GLint arraySize;
    __GL_SETUP();

    arraySize = mapsize * 4;
    if (arraySize < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (GLuint)(8 + arraySize));
    if (dlop == NULL) return;
    dlop->opcode = 0xa6;
    data = (GLenum *) dlop->data;
    data[0] = map;
    data[1] = mapsize;
    memcpy(data + 2, values, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_PixelMapfv);
}

/* 690ac860 -- __gllc_PixelMapuiv */
void __stdcall __gllc_PixelMapuiv(GLenum map, GLint mapsize, const GLuint *values)
{
    __GLdlistOpICD *dlop;
    GLenum *data;
    GLint arraySize;
    __GL_SETUP();

    arraySize = mapsize * 4;
    if (arraySize < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (GLuint)(8 + arraySize));
    if (dlop == NULL) return;
    dlop->opcode = 0xa7;
    data = (GLenum *) dlop->data;
    data[0] = map;
    data[1] = mapsize;
    memcpy(data + 2, values, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_PixelMapuiv);
}

/* 690acb40 -- __gllc_LoadMatrixf */
void __stdcall __gllc_LoadMatrixf(const GLfloat m[16])
{
    __GLdlistOpICD *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(GLfloat) * 16);
    if (dlop == NULL) return;
    dlop->opcode = 0xaf;
    memcpy(dlop->data, m, sizeof(GLfloat) * 16);
    __glDlistAppendOp(gc, dlop, __glle_LoadMatrixf);
}

/* 690acb90 -- __gllc_LoadMatrixd */
void __stdcall __gllc_LoadMatrixd(const GLdouble m[16])
{
    __GLdlistOpICD *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(GLdouble) * 16);
    if (dlop == NULL) return;
    dlop->opcode = 0xb0;
    dlop->aligned = GL_TRUE;
    memcpy(dlop->data, m, sizeof(GLdouble) * 16);
    __glDlistAppendOp(gc, dlop, __glle_LoadMatrixd);
}

/* 690acc40 -- __gllc_MultMatrixf */
void __stdcall __gllc_MultMatrixf(const GLfloat m[16])
{
    __GLdlistOpICD *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(GLfloat) * 16);
    if (dlop == NULL) return;
    dlop->opcode = 0xb2;
    memcpy(dlop->data, m, sizeof(GLfloat) * 16);
    __glDlistAppendOp(gc, dlop, __glle_MultMatrixf);
}

/* 690acc90 -- __gllc_MultMatrixd */
void __stdcall __gllc_MultMatrixd(const GLdouble m[16])
{
    __GLdlistOpICD *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(GLdouble) * 16);
    if (dlop == NULL) return;
    dlop->opcode = 0xb3;
    dlop->aligned = GL_TRUE;
    memcpy(dlop->data, m, sizeof(GLdouble) * 16);
    __glDlistAppendOp(gc, dlop, __glle_MultMatrixd);
}

/* 690ad380 -- __gllc_PrioritizeTextures: n, the names, then the priorities */
void __stdcall __gllc_PrioritizeTextures(GLsizei n, const GLuint *textures,
			     const GLclampf *priorities)
{
    __GLdlistOpICD *dlop;
    GLint *data;
    GLint arraySize;
    __GL_SETUP();

    arraySize = n * 4;
    if (arraySize < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (GLuint)(4 + arraySize * 2));
    if (dlop == NULL) return;
    dlop->opcode = 0xce;
    data = (GLint *) dlop->data;
    data[0] = n;
    memcpy(data + 1, textures, arraySize);
    memcpy((GLubyte *) (data + 1) + arraySize, priorities, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_PrioritizeTextures);
}

/* 690b20c0 -- __gllc_Bitmap */
void __stdcall __gllc_Bitmap(GLsizei width, GLsizei height,
			     GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove,
			     const GLubyte *oldbits)
{
    __GLdlistOpICD *dlop;
    __GLbitmap *bitmap;
    GLubyte *newbits;
    GLint imageSize;
    __GL_SETUP();

    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue(gc);
	return;
    }

    imageSize = height * ((width + 7) >> 3);
    imageSize = __GL_PAD(imageSize);

    dlop = __glDlistAllocOp2(gc, (GLuint)(imageSize + sizeof(__GLbitmap)));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Bitmap;

    bitmap = (__GLbitmap *) dlop->data;
    bitmap->width = width;
    bitmap->height = height;
    bitmap->xorig = xorig;
    bitmap->yorig = yorig;
    bitmap->xmove = xmove;
    bitmap->ymove = ymove;
    bitmap->imageSize = imageSize;

    newbits = &dlop->data[0] + sizeof(__GLbitmap);
    __glFillImage(gc, width, height, GL_COLOR_INDEX, GL_BITMAP,
	    oldbits, newbits);

    __glDlistAppendOp(gc, dlop, __glle_Bitmap);
}

/* 690b2170 -- __glle_Bitmap: the ICD waits for the hardware first and revalidates */
const GLubyte * __fastcall __glle_Bitmap(const GLubyte *PC)
{
    const __GLbitmap *bitmap;
    GLuint beginMode;
    __GL_SETUP();

    (*gc->procs.waitIdle)(gc);
    bitmap = (const __GLbitmap *) PC;

    beginMode = __gl_beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    __gl_beginMode = __GL_NOT_IN_BEGIN;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return PC + sizeof(__GLbitmap) + bitmap->imageSize;
	}
    }
    (*gc->procs.revalidate)(gc);

    (*gc->procs.renderBitmap)(gc, bitmap, (const GLubyte *) (bitmap+1));

    return PC + sizeof(__GLbitmap) + bitmap->imageSize;
}

/************************************************************************/

/* 690b2310 -- __gllc_Map1f */
void __stdcall __gllc_Map1f(GLenum target,
			     GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Map1f_W20Rec *map1data;
    GLint k;
    __GL_SETUP();

    k = __glEvalComputeK(target);
    if (k < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }

    if (order > gc->constants.maxEvalOrder || stride < k ||
	    order < 1 || u1 == u2) {
	__gllc_InvalidValue(gc);
	return;
    }

    dlop = __glDlistAllocOp2(gc, sizeof(*map1data) + __glMap1_size(k, order) * sizeof(__GLfloat));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Map1f;
    map1data = (struct __gllc_Map1f_W20Rec *) dlop->data;
    map1data->target = target;
    map1data->u1 = u1;
    map1data->u2 = u2;
    map1data->order = order;
    __glFillMap1f(k, order, stride, points, (GLfloat *) (map1data + 1));
    __glDlistAppendOp(gc, dlop, __glle_Map1);
}

/* 690b2430 -- __gllc_Map1d */
void __stdcall __gllc_Map1d(GLenum target,
			     GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Map1f_W20Rec *map1data;
    GLint k;
    __GL_SETUP();

    k = __glEvalComputeK(target);
    if (k < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }

    if (order > gc->constants.maxEvalOrder || stride < k ||
	    order < 1 || u1 == u2) {
	__gllc_InvalidValue(gc);
	return;
    }

    dlop = __glDlistAllocOp2(gc, sizeof(*map1data) + __glMap1_size(k, order) * sizeof(__GLfloat));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Map1d;
    map1data = (struct __gllc_Map1f_W20Rec *) dlop->data;
    map1data->target = target;
    map1data->u1 = u1;
    map1data->u2 = u2;
    map1data->order = order;
    __glFillMap1d(k, order, stride, points, (GLfloat *) (map1data + 1));
    __glDlistAppendOp(gc, dlop, __glle_Map1);
}

/* 690b2500 -- __gllc_Map2f */
void __stdcall __gllc_Map2f(GLenum target,
			     GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
			     GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
			     const GLfloat *points)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Map2f_Rec *map2data;
    GLint k;
    GLint cmdsize;
    __GLfloat *data;
    __GL_SETUP();

    k=__glEvalComputeK(target);
    if (k < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }

    if (vorder > gc->constants.maxEvalOrder || vstride < k ||
	    vorder < 1 || u1 == u2 || ustride < k ||
	    uorder > gc->constants.maxEvalOrder || uorder < 1 ||
	    v1 == v2) {
	__gllc_InvalidValue(gc);
	return;
    }

    cmdsize = (GLint)sizeof(*map2data) +
	    __glMap2_size(k, uorder, vorder) * (GLint)sizeof(__GLfloat);

    dlop = __glDlistAllocOp2(gc, cmdsize);
    if (dlop == NULL) return;
    dlop->opcode = __glop_Map2f;

    map2data = (struct __gllc_Map2f_Rec *) dlop->data;
    map2data->target = target;
    map2data->u1 = u1;
    map2data->u2 = u2;
    map2data->uorder = uorder;
    map2data->v1 = v1;
    map2data->v2 = v2;
    map2data->vorder = vorder;

    data = (__GLfloat *) (&dlop->data[0] + sizeof(*map2data));
    __glFillMap2f(k, uorder, vorder, ustride, vstride, points, data);

    __glDlistAppendOp(gc, dlop, __glle_Map2);
}

/* 690b26a0 -- __gllc_Map2d */
void __stdcall __gllc_Map2d(GLenum target,
			     GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
			     GLdouble v1, GLdouble v2, GLint vstride, GLint vorder,
			     const GLdouble *points)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Map2f_Rec *map2data;
    GLint k;
    GLint cmdsize;
    __GLfloat *data;
    __GL_SETUP();

    k=__glEvalComputeK(target);
    if (k < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }

    if (vorder > gc->constants.maxEvalOrder || vstride < k ||
	    vorder < 1 || u1 == u2 || ustride < k ||
	    uorder > gc->constants.maxEvalOrder || uorder < 1 ||
	    v1 == v2) {
	__gllc_InvalidValue(gc);
	return;
    }

    cmdsize = (GLint)sizeof(*map2data) +
	    __glMap2_size(k, uorder, vorder) * (GLint)sizeof(__GLfloat);

    dlop = __glDlistAllocOp2(gc, cmdsize);
    if (dlop == NULL) return;
    dlop->opcode = __glop_Map2d;

    map2data = (struct __gllc_Map2f_Rec *) dlop->data;
    map2data->target = target;
    map2data->u1 = u1;
    map2data->u2 = u2;
    map2data->uorder = uorder;
    map2data->v1 = v1;
    map2data->v2 = v2;
    map2data->vorder = vorder;

    data = (__GLfloat *) (&dlop->data[0] + sizeof(*map2data));
    __glFillMap2d(k, uorder, vorder, ustride, vstride, points, data);

    __glDlistAppendOp(gc, dlop, __glle_Map2);
}

/* 690b27d0 -- __glle_DrawPixels: the ICD waits for the hardware first and revalidates (procs at
** 0x2428 / 0x2430) before drawing */
const GLubyte * __fastcall __glle_DrawPixels(const GLubyte *PC)
{
    const struct __gllc_DrawPixels_Rec *pixdata;
    GLint imageSize;
    GLuint beginMode;
    __GL_SETUP();

    (*gc->procs.waitIdle)(gc);
    pixdata = (const struct __gllc_DrawPixels_Rec *) PC;
    imageSize = __glImageSize(pixdata->width, pixdata->height,
			      pixdata->format, pixdata->type);

    beginMode = __gl_beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
	if (beginMode == __GL_NEED_VALIDATE) {
	    (*gc->procs.validate)(gc);
	    __gl_beginMode = __GL_NOT_IN_BEGIN;
	} else {
	    __glSetError(GL_INVALID_OPERATION);
	    return PC + sizeof(*pixdata) + __GL_PAD(imageSize);
	}
    }
    (*gc->procs.revalidate)(gc);

    if (!gc->state.current.validRasterPos) {
	return PC + sizeof(*pixdata) + __GL_PAD(imageSize);
    }

    if (gc->renderMode == GL_FEEDBACK) {
	__glFeedbackDrawPixels(gc, &gc->state.current.rasterPos);
	return PC + sizeof(*pixdata) + __GL_PAD(imageSize);
    }

    if (gc->renderMode == GL_RENDER) {
	(*gc->procs.drawPixels)(gc, pixdata->width, pixdata->height,
				pixdata->format, pixdata->type,
				(const GLubyte *)(PC + sizeof(*pixdata)),
				GL_TRUE);
    }

    return PC + sizeof(*pixdata) + __GL_PAD(imageSize);
}

/* 690b28c0 -- __gllc_DrawPixels: the EXT_packed_pixels types only (no _REV / 5_6_5) */
void __stdcall __gllc_DrawPixels(GLint width, GLint height,
			     GLenum format, GLenum type, const GLvoid *pixels)
{
    GLenum adjustFormat= format;
    GLenum adjustType= type;
    __GLdlistOpICD *dlop;
    struct __gllc_DrawPixels_Rec *pixdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue(gc);
	return;
    }
    switch (format) {
      case GL_STENCIL_INDEX:
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_DEPTH_COMPONENT:
      case GL_ABGR_EXT:
      case GL_BGR:
      case GL_BGRA:
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum(gc);
	    return;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      case GL_UNSIGNED_BYTE_3_3_2:
	switch (format) {
	  case GL_RGB:
	  case GL_BGR:
	    adjustFormat= GL_LUMINANCE;	/* or anything that's 1 component */
	    adjustType= GL_UNSIGNED_BYTE;
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_10_10_10_2:
	switch (format) {
	  case GL_RGBA:
	  case GL_ABGR_EXT:
          case GL_BGRA:
	    adjustFormat= GL_LUMINANCE;	/* or anything that's 1 component */
	    adjustType= GL_UNSIGNED_SHORT;
            if (type == GL_UNSIGNED_INT_8_8_8_8 ||
                type == GL_UNSIGNED_INT_10_10_10_2)
	       adjustType= GL_UNSIGNED_INT;
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }

    imageSize = __glImageSize(width, height, format, type);
    imageSize = __GL_PAD(imageSize);

    dlop = __glDlistAllocOp2(gc, (GLuint)(sizeof(*pixdata) + imageSize));
    if (dlop == NULL) return;
    dlop->opcode = __glop_DrawPixels;

    pixdata = (struct __gllc_DrawPixels_Rec *) dlop->data;
    pixdata->width = width;
    pixdata->height = height;
    pixdata->format = format;
    pixdata->type = type;

    __glFillImage(gc, width, height, adjustFormat, adjustType, pixels,
	    (GLubyte *)(pixdata + 1));

    __glDlistAppendOp(gc, dlop, __glle_DrawPixels);
}

/* 690b2b40 -- __gllc_TexImage1D */
void __stdcall __gllc_TexImage1D(GLenum target, GLint level,
			     GLint internalformat,
			     GLint width, GLint border, GLenum format,
			     GLenum type, const GLvoid *pixels)
{
    GLenum adjustFormat= format;
    GLenum adjustType= type;
    __GLdlistOpICD *dlop;
    struct __gllc_TexImage1D_Rec *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

    if (target == GL_PROXY_TEXTURE_1D) {
	(*(__GLtexImage1DProc)gc->dispatchState->fn[0x2dc / 4])(target, level,
						  internalformat, width,
						  border, format, type,
						  pixels);
	return;
    }

    if (border < 0 || border > 1) {
	__gllc_InvalidValue(gc);
	return;
    }
    if (width < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    switch (format) {
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_ABGR_EXT:
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum(gc);
	    return;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      case GL_UNSIGNED_BYTE_3_3_2:
	switch (format) {
	  case GL_RGB:
	  case GL_BGR_EXT:
	    adjustFormat= GL_LUMINANCE;	/* or anything that's 1 component */
	    adjustType= GL_UNSIGNED_BYTE;
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_10_10_10_2:
	switch (format) {
	  case GL_RGBA:
	  case GL_ABGR_EXT:
	  case GL_BGRA_EXT:
	    adjustFormat= GL_LUMINANCE;	/* or anything that's 1 component */
	    adjustType= GL_UNSIGNED_SHORT;
	    if (type == GL_UNSIGNED_INT_8_8_8_8 ||
		type == GL_UNSIGNED_INT_10_10_10_2)
	       adjustType= GL_UNSIGNED_INT;
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }

    imageSize = __glImageSize(width, 1, format, type);
    imageSize = __GL_PAD_SHR(imageSize);

    dlop = __glDlistAllocOp2(gc, (GLuint)(sizeof(*texdata) + imageSize));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexImage1D;

    texdata = (struct __gllc_TexImage1D_Rec *) dlop->data;
    texdata->target = target;
    texdata->level = level;
    texdata->internalformat = internalformat;
    texdata->width = width;
    texdata->border = border;
    texdata->format = format;
    texdata->type = type;
    texdata->imageSize = imageSize;

    if (imageSize > 0 && pixels != NULL) {
	__glFillImage(gc, width, 1, adjustFormat, adjustType, pixels,
		&dlop->data[0] + sizeof(*texdata));
    }

    __glDlistAppendOp(gc, dlop, __glle_TexImage1D);
}

/* 690b2d80 -- __gllc_TexImage2D: color-index or color formats only, the EXT_packed_pixels types */
void __stdcall __gllc_TexImage2D(GLenum target, GLint level,
			     GLint internalformat, GLint width, GLint height, GLint border,
			     GLenum format, GLenum type, const GLvoid *pixels)
{
    GLenum adjustFormat= format;
    GLenum adjustType= type;
    __GLdlistOpICD *dlop;
    struct __gllc_TexImage2D_Rec  *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

    if (target == GL_PROXY_TEXTURE_2D) {
	(*gc->dispatchState->fn[__GL_DISP_TexImage2D])(target, level,
						  internalformat,
						  width, height, border,
						  format, type, pixels);

	return;
    }

    if (border < 0 || border > 1 || (width < 0) || (height < 0)) {
	__gllc_InvalidValue(gc);
	return;
    }
    switch (format) {
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_ABGR_EXT:
      case GL_BGR:
      case GL_BGRA:
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum(gc);
	    return;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      case GL_UNSIGNED_BYTE_3_3_2:
	switch (format) {
	  case GL_RGB:
	  case GL_BGR:
	    adjustFormat= GL_LUMINANCE;	/* or anything that's 1 component */
	    adjustType= GL_UNSIGNED_BYTE;
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_10_10_10_2:
	switch (format) {
	  case GL_RGBA:
	  case GL_ABGR_EXT:
          case GL_BGRA:
	    adjustFormat= GL_LUMINANCE;	/* or anything that's 1 component */
	    adjustType= GL_UNSIGNED_SHORT;
            if (type == GL_UNSIGNED_INT_8_8_8_8 ||
                type == GL_UNSIGNED_INT_10_10_10_2)
	       adjustType= GL_UNSIGNED_INT;
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }

    imageSize = __glImageSize(width, height, format, type);
    imageSize = __GL_PAD(imageSize);

    dlop = __glDlistAllocOp2(gc, (GLuint)(sizeof(*texdata) + imageSize));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexImage2D;

    texdata = (struct __gllc_TexImage2D_Rec *) dlop->data;
    texdata->target = target;
    texdata->level = level;
    texdata->internalformat = internalformat;
    texdata->width = width;
    texdata->height = height;
    texdata->border = border;
    texdata->format = format;
    texdata->type = type;
    texdata->imageSize = imageSize;

    if (imageSize > 0 && pixels != NULL) {
	__glFillImage(gc, width, height, adjustFormat, adjustType, pixels,
		      (GLubyte *)(texdata + 1));
    }
    __glDlistAppendOp(gc, dlop, __glle_TexImage2D);
}

/* 690b3010 -- __gllc_TexSubImage1D */
void __stdcall __gllc_TexSubImage1D(GLenum target, GLint level,
			     GLint xoffset, GLsizei width, GLenum format, GLenum type,
			     const GLvoid *pixels)
{
    GLenum adjustFormat= format;
    GLenum adjustType= type;
    __GLdlistOpICD *dlop;
    struct __gllc_TexSubImage1D_W20Rec *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

    if (width < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    switch (format) {
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_ABGR_EXT:
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum(gc);
	    return;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      case 0x8032:	/* GL_UNSIGNED_BYTE_3_3_2 */
	switch (format) {
	  case GL_RGB:
	  case GL_BGR_EXT:
	    adjustFormat= GL_LUMINANCE;	/* or anything that's 1 component */
	    adjustType= GL_UNSIGNED_BYTE;
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	break;
      case 0x8033:	/* GL_UNSIGNED_SHORT_4_4_4_4 */
      case 0x8034:	/* GL_UNSIGNED_SHORT_5_5_5_1 */
      case 0x8035:	/* GL_UNSIGNED_INT_8_8_8_8 */
      case 0x8036:	/* GL_UNSIGNED_INT_10_10_10_2 */
	switch (format) {
	  case GL_RGBA:
	  case GL_ABGR_EXT:
	  case GL_BGRA_EXT:
	    adjustFormat= GL_LUMINANCE;	/* or anything that's 1 component */
	    adjustType= GL_UNSIGNED_SHORT;
	    if (type == 0x8035 || type == 0x8036)
	       adjustType= GL_UNSIGNED_INT;
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }

    imageSize = __glImageSize(width, 1, format, type);
    imageSize = __GL_PAD(imageSize);

    dlop = __glDlistAllocOp2(gc, (GLuint)(sizeof(*texdata) + imageSize));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexSubImage1D;

    texdata = (struct __gllc_TexSubImage1D_W20Rec *) dlop->data;
    texdata->target = target;
    texdata->level = level;
    texdata->xoffset = xoffset;
    texdata->width = width;
    texdata->format = format;
    texdata->type = type;
    texdata->imageSize = imageSize;

    if (imageSize > 0) {
	__glFillImage(gc, width, 1, adjustFormat, adjustType, pixels,
		&dlop->data[0] + sizeof(*texdata));
    }

    __glDlistAppendOp(gc, dlop, __glle_TexSubImage1D);
}

/* 690b3250 -- __gllc_TexSubImage2D */
void __stdcall __gllc_TexSubImage2D(GLenum target, GLint level,
				GLint xoffset, GLint yoffset,
				GLsizei width, GLsizei height,
				GLenum format, GLenum type, const GLvoid *pixels)
{
    GLenum adjustFormat= format;
    GLenum adjustType= type;
    __GLdlistOpICD *dlop;
    struct __gllc_TexSubImage2D_Rec *texdata;
    GLint imageSize;
    GLboolean index;
    __GL_SETUP();

    if ((width < 0) || (height < 0)) {
	__gllc_InvalidValue(gc);
	return;
    }
    switch (format) {
      case GL_COLOR_INDEX:
	index = GL_TRUE;
	break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_ABGR_EXT:
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
	index = GL_FALSE;
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }
    switch (type) {
      case GL_BITMAP:
	if (!index) {
	    __gllc_InvalidEnum(gc);
	    return;
	}
	break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	break;
      case GL_UNSIGNED_BYTE_3_3_2:
	switch (format) {
	  case GL_RGB:
	  case GL_BGR_EXT:
	    adjustFormat= GL_LUMINANCE;	/* or anything that's 1 component */
	    adjustType= GL_UNSIGNED_BYTE;
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_INT_8_8_8_8:
      case GL_UNSIGNED_INT_10_10_10_2:
	switch (format) {
	  case GL_RGBA:
	  case GL_ABGR_EXT:
	  case GL_BGRA_EXT:
	    adjustFormat= GL_LUMINANCE;	/* or anything that's 1 component */
	    adjustType= GL_UNSIGNED_SHORT;
	    if (type == GL_UNSIGNED_INT_8_8_8_8 ||
		type == GL_UNSIGNED_INT_10_10_10_2)
	       adjustType= GL_UNSIGNED_INT;
	    break;
	  default:
	    __glSetError(GL_INVALID_OPERATION);
	    return;
	}
	break;
      default:
	__gllc_InvalidEnum(gc);
	return;
    }

    imageSize = __glImageSize(width, height, format, type);
    imageSize = __GL_PAD_SHR(imageSize);

    dlop = __glDlistAllocOp2(gc, (GLuint)(sizeof(*texdata) + imageSize));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TexSubImage2D;

    texdata = (struct __gllc_TexSubImage2D_Rec *) dlop->data;
    texdata->target = target;
    texdata->level = level;
    texdata->xoffset = xoffset;
    texdata->yoffset = yoffset;
    texdata->width = width;
    texdata->height = height;
    texdata->format = format;
    texdata->type = type;
    texdata->imageSize = imageSize;

    if (imageSize > 0) {
	__glFillImage(gc, width, height, adjustFormat, adjustType, pixels,
		(GLubyte *) &dlop->data[0] + sizeof(*texdata));
    }

    __glDlistAppendOp(gc, dlop, __glle_TexSubImage2D);
}

/* 690b3460 -- __gllc_Disable (the list-compile tables' glDisable slot): the vertex-array enums are
** client state and change now (EXT_vertex_array style); every other enum is compiled */
void __stdcall __gllc_Disable(GLenum array)
{
    __GLdlistOpICD *dlop;
    struct __gllc_DisableClientState_Rec *data;
    __GL_SETUP();

    switch (array) {
      case GL_FOG_COORDINATE_ARRAY_EXT:
	gc->vertexArray.enables &= ~0x1f00000;
	gc->vertexArray.enableFlags &= ~__GL_VA_FOGCOORD_ENABLE;
	return;
      case GL_VERTEX_ARRAY:
	gc->vertexArray.enables &= ~0xff;
	gc->vertexArray.enableFlags &= ~__GL_VA_VERTEX_ENABLE;
	return;
      case GL_NORMAL_ARRAY:
	gc->vertexArray.enables &= ~0xf00;
	gc->vertexArray.enableFlags &= ~__GL_VA_NORMAL_ENABLE;
	return;
      case GL_COLOR_ARRAY:
	gc->vertexArray.enables &= ~0xff000;
	gc->vertexArray.enableFlags &= ~__GL_VA_COLOR_ENABLE;
	gc->dirtyMask |= __GL_DIRTY_COLORARRAY_ICD;
	return;
      case GL_INDEX_ARRAY:
	gc->vertexArray.enables &= ~0xf0000000;
	gc->vertexArray.enableFlags &= ~__GL_VA_INDEX_ENABLE;
	return;
      case GL_TEXTURE_COORD_ARRAY:
	gc->vertexArray.enableFlags &= ~__GL_VA_TEXCOORD_ENABLE(gc->fx.clientActiveTexture);
	return;
      case GL_EDGE_FLAG_ARRAY:
	gc->vertexArray.enables &= ~0x8000000;
	gc->vertexArray.enableFlags &= ~__GL_VA_EDGEFLAG_ENABLE;
	return;
    }

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_DisableClientState_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_DisableClientState;
    data = (struct __gllc_DisableClientState_Rec *) dlop->data;
    data->array = array;
    __glDlistAppendOp(gc, dlop, __glle_DisableClientState);
}

/* 690b3620 -- __gllc_Enable: client state is not compiled (set at once, without a
** validate); an unknown array is compiled so the error comes at execution */
void __stdcall __gllc_Enable(GLenum array)
{
    __GLdlistOpICD *dlop;
    GLenum *data;
    __GL_SETUP();

    switch (array) {
      case GL_VERTEX_ARRAY:
	gc->vertexArray.enables |= 0x000000ff;
	gc->vertexArray.enableFlags |= __GL_VA_VERTEX_ENABLE;
	return;
      case GL_NORMAL_ARRAY:
	gc->vertexArray.enables |= 0x00000f00;
	gc->vertexArray.enableFlags |= __GL_VA_NORMAL_ENABLE;
	return;
      case GL_COLOR_ARRAY:
	gc->vertexArray.enables |= 0x000ff000;
	gc->vertexArray.enableFlags |= __GL_VA_COLOR_ENABLE;
	gc->dirtyMask |= __GL_DIRTY_COLORARRAY_ICD;
	return;
      case GL_INDEX_ARRAY:
	gc->vertexArray.enables |= 0xf0000000;
	gc->vertexArray.enableFlags |= __GL_VA_INDEX_ENABLE;
	return;
      case GL_TEXTURE_COORD_ARRAY:
	gc->vertexArray.enableFlags |= __GL_VA_TEXCOORD_ENABLE(gc->fx.clientActiveTexture);
	return;
      case GL_EDGE_FLAG_ARRAY:
	gc->vertexArray.enables |= 0x08000000;
	gc->vertexArray.enableFlags |= __GL_VA_EDGEFLAG_ENABLE;
	return;
      case GL_FOG_COORDINATE_ARRAY_EXT:
	gc->vertexArray.enables |= 0x01f00000;
	gc->vertexArray.enableFlags |= __GL_VA_FOGCOORD_ENABLE;
	return;
    }

    dlop = __glDlistAllocOp2(gc, sizeof(GLenum));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EnableClientState;
    data = (GLenum *) dlop->data;
    *data = array;
    __glDlistAppendOp(gc, dlop, __glle_Enable);
}

/* 690b3810 -- __glle_ColorTable */
const GLubyte * __fastcall __glle_ColorTable(const GLubyte *PC)
{
    const struct __gllc_ColorTable_Rec *data;
    __GL_SETUP();
    __GL_API_STATE();

    data = (const struct __gllc_ColorTable_Rec *) PC;
    (*(__GLcolorTableProc)gc->procs.colorTable)(gc, data->target, data->internalformat,
	data->width, data->format, data->type,
	(const GLvoid *) (PC + sizeof(*data)), GL_TRUE);
    return PC + (GLuint)sizeof(*data) + __GL_PAD_SHR(data->imageSize);
}

/* 690b3850 -- __gllc_ColorTable: GL_TABLE_TOO_LARGE from the check compiles with start 0.
** Original bug (kept): the check is the glColorTableEXT one (69082a30) with start as the internal format, so any
** start that is not an internal-format enum (start 0 included) compiles GL_INVALID_ENUM instead of the call, and
** GL_INVALID_VALUE is also recorded as GL_INVALID_ENUM.  (__gllc_ColorTable above has the subtable check.) */
void __stdcall __gllc_ColorTable(GLenum target, GLsizei start,
			     GLsizei count, GLenum format, GLenum type, const void *table)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ColorSubTable_Rec *data;
    GLint imageSize;
    GLubyte *tableContents;
    GLint rvalue;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    if ((rvalue = __glCheckColorTableArgs(gc, target, start, count, format, type))) {     /* sic: see above */
	switch (rvalue) {
	  case GL_INVALID_ENUM:
	    __gllc_InvalidEnum(gc);
	    return;
	  case GL_INVALID_VALUE:
	    __gllc_InvalidEnum(gc);
	    return;
	  case GL_TABLE_TOO_LARGE:
	    start = 0;
	    break;
	}
    }
    imageSize = __glImageSize(count, 1, format, type);
    imageSize = __GL_PAD(imageSize);

    dlop = __glDlistAllocOp2(gc, (GLuint)(imageSize + sizeof(*data)));
    if(dlop == NULL) return;
    dlop->opcode = __glop_ColorSubTable;
    data = (struct __gllc_ColorSubTable_Rec *)dlop->data;

    data->target = target;
    data->start = start;
    data->count = count;
    data->format = format;
    data->type = type;
    data->imageSize = imageSize;

    tableContents = &dlop->data[0] + (int)sizeof(*data);
    __glFillImage(gc, count, 1, format, type, table, tableContents);
    __glDlistAppendOp(gc, dlop, __glle_ColorTable);
}

/* 690b3970 -- __glle_ColorSubTable */
const GLubyte * __fastcall __glle_ColorSubTable(const GLubyte *PC)
{
    const struct __gllc_ColorSubTable_Rec *data;
    __GL_SETUP();
    __GL_API_STATE();

    data = (const struct __gllc_ColorSubTable_Rec *) PC;
    (*(__GLcolorSubTableProc)gc->procs.colorSubTable)(gc, data->target, data->start,
	data->count, data->format, data->type,
	(const GLvoid *) (PC + sizeof(*data)), GL_TRUE);
    return PC + (GLuint)sizeof(*data) + __GL_PAD_SHR(data->imageSize);
}

/* 690b39b0 -- __gllc_ColorSubTable: no proxy targets, every argument error is GL_INVALID_ENUM.  Original bug (kept):
** the check is the subtable one (69082bb0), so a bad internal format or width is compiled, not rejected */
void __stdcall __gllc_ColorSubTable(GLenum target, GLenum internalformat,
			     GLsizei width, GLenum format, GLenum type, const void *table)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ColorTable_Rec *data;
    GLint imageSize;
    GLubyte *tableContents;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    if (__glCheckColorSubTableArgs(gc, target, internalformat, width, format, type)) {    /* sic: see above */
	__gllc_InvalidEnum(gc);
	return;
    }
    imageSize = __glImageSize(width, 1, format, type);
    imageSize = __GL_PAD(imageSize);

    dlop = __glDlistAllocOp2(gc, (GLuint)(imageSize + sizeof(*data)));
    if(dlop == NULL) return;
    dlop->opcode = __glop_ColorTable;
    data = (struct __gllc_ColorTable_Rec *)dlop->data;

    data->target = target;
    data->internalformat = internalformat;
    data->width = width;
    data->format = format;
    data->type = type;
    data->imageSize = imageSize;

    tableContents = &dlop->data[0] + (int)sizeof(*data);
    __glFillImage(gc, width, 1, format, type, table, tableContents);
    __glDlistAppendOp(gc, dlop, __glle_ColorSubTable);
}

/************************************************************************/
/* dlist: glEndList while compiling -- restore the execute-table entry, then continue in it
** (VC6 turns the final argument-less call into the stub's `jmp [gc->dispatch + 2*4]`) */

/* 690b3c00 -- __gllc_EndList: glEndList while compiling, restore the execute-table entry (fn[2]) and continue
** in it */
void __fastcall __gllc_EndList(GLuint ecx_unused, GLuint edx_unused)
{
    __GLcontext *gc;

    gc = __glCurrentGC();
    __glCurrentGC()->currentDispatchState.fn[2] = gc->savedDispatchState.fn[2];
    (*__glCurrentGC()->currentDispatchState.fn[2])();
}

/* 690b43e0 -- __glce_ArrayElement (3dfx G_LEXEC.C): GL_COMPILE_AND_EXECUTE, compiled and executed through the
** dispatch tables; unreferenced */
void __fastcall __glce_ArrayElement(GLint i)
{
    __GLcontext *gc;
    __GLdispatchTable save;

    save = __glCurrentGC()->currentDispatchState;
    gc = __glCurrentGC();
    __glCopyDispatchTable(&gc->listCompState, &__glCurrentGC()->currentDispatchState);
    __GL_ARRAY_ELEMENT(i);
    __glCopyDispatchTable(&gc->savedDispatchState, &__glCurrentGC()->currentDispatchState);
    gc->dispatchState = &__glCurrentGC()->currentDispatchState;
    __GL_ARRAY_ELEMENT(i);
    __glCopyDispatchTable(&__glCurrentGC()->currentDispatchState, &gc->savedDispatchState);
    gc->dispatchState = &gc->savedDispatchState;
    __glCopyDispatchTable(&save, &__glCurrentGC()->currentDispatchState);
}

/* 690aabb0 -- __gllc_Fogiv */
void __stdcall __gllc_Fogiv(GLenum pname, const GLint *params)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Fogiv_Rec *data;
    GLint size;
    __GL_SETUP();

    size = __glFogiv_size(pname) * 4;
    if (size < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (size + 4));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Fogiv;
    data = (struct __gllc_Fogiv_Rec *) dlop->data;
    data->pname = pname;
    memcpy(data + 1, params, size);
    __glDlistAppendOp(gc, dlop, __glle_Fogiv);
}

/* 690aaeb0 -- __gllc_LightModelfv */
void __stdcall __gllc_LightModelfv(GLenum pname, const GLfloat *params)
{
    __GLdlistOpICD *dlop;
    struct __gllc_LightModelfv_Rec *data;
    GLint size;
    __GL_SETUP();

    size = __glLightModelfv_size(pname) * 4;
    if (size < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (size + 4));
    if (dlop == NULL) return;
    dlop->opcode = __glop_LightModelfv;
    data = (struct __gllc_LightModelfv_Rec *) dlop->data;
    data->pname = pname;
    memcpy(data + 1, params, size);
    __glDlistAppendOp(gc, dlop, __glle_LightModelfv);
}

/* 690aa250 -- __gllc_Vertex3d */
void __stdcall __gllc_Vertex3d(GLdouble x, GLdouble y, GLdouble z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex3d;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Vertex3d_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= 1;      /* genericFlags: the list holds a vertex */
    __glDlistAppendOp(gc, dlop, __glle_Vertex3dv);
}

/* 690aa5b0 -- __gllc_Vertex4d */
void __stdcall __gllc_Vertex4d(GLdouble x, GLdouble y, GLdouble z,
                             GLdouble w)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex4d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Vertex4d;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Vertex4d_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= 1;      /* genericFlags: the list holds a vertex */
    __glDlistAppendOp(gc, dlop, __glle_Vertex4dv);
}

/* 690ac8e0 -- __gllc_PixelMapusv */
void __stdcall __gllc_PixelMapusv(GLenum map, GLint mapsize,
                             const GLushort *values)
{
    __GLdlistOpICD *dlop;
    struct __gllc_PixelMapusv_Rec *data;
    GLint values_size;
    __GL_SETUP();

    values_size = __GL_PAD(mapsize * 2);
    if (values_size < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (values_size + 8));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PixelMapusv;
    data = (struct __gllc_PixelMapusv_Rec *) dlop->data;
    data->map = map;
    data->mapsize = mapsize;
    memcpy(data + 1, values, values_size);
    __glDlistAppendOp(gc, dlop, __glle_PixelMapusv);
}

/* 690ae250 -- __gllc_PointParameterfvEXT */
void __stdcall __gllc_PointParameterfvEXT(GLenum pname, const GLfloat *params)
{
    __GLdlistOpICD *dlop;
    struct __gllc_PointParameterfvEXT_Rec *data;
    GLint size;
    __GL_SETUP();

    size = __glPointParameterfvEXT_size(pname) * 4;
    if (size < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (size + 4));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PointParameterfvEXT;
    data = (struct __gllc_PointParameterfvEXT_Rec *) dlop->data;
    data->pname = pname;
    memcpy(data + 1, params, size);
    __glDlistAppendOp(gc, dlop, __glle_PointParameterfvEXT);
}

/* 690ae320 -- __gllc_CompressedTexImage1DARB */
void __stdcall __gllc_CompressedTexImage1DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border,
                             GLsizei imageSize, const GLvoid *image)
{
    __GLdlistOpICD *dlop;
    struct CompressedTexImage1DARB_RecW14 *data;
    GLint imageSize_size;
    __GL_SETUP();

    imageSize_size = __GL_PAD(imageSize);
    if (imageSize_size < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (imageSize_size + sizeof(struct CompressedTexImage1DARB_RecW14)));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CompressedTexImage1DARB;
    data = (struct CompressedTexImage1DARB_RecW14 *) dlop->data;
    data->target = target;
    data->level = level;
    data->internalformat = internalformat;
    data->width = width;
    data->border = border;
    data->imageSize = imageSize;
    memcpy(data + 1, image, imageSize_size);
    __glDlistAppendOp(gc, dlop, __glle_CompressedTexImage1DARB);
}

/* 690ae3b0 -- __gllc_CompressedTexImage2DARB */
void __stdcall __gllc_CompressedTexImage2DARB(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border,
                             GLsizei imageSize, const GLvoid *image)
{
    __GLdlistOpICD *dlop;
    struct CompressedTexImage2DARB_RecW14 *data;
    GLint imageSize_size;
    __GL_SETUP();

    imageSize_size = __GL_PAD(imageSize);
    if (imageSize_size < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (imageSize_size + sizeof(struct CompressedTexImage2DARB_RecW14)));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CompressedTexImage2DARB;
    data = (struct CompressedTexImage2DARB_RecW14 *) dlop->data;
    data->target = target;
    data->level = level;
    data->internalformat = internalformat;
    data->width = width;
    data->height = height;
    data->border = border;
    data->imageSize = imageSize;
    memcpy(data + 1, image, imageSize_size);
    __glDlistAppendOp(gc, dlop, __glle_CompressedTexImage2DARB);
}

/* 690ae450 -- __gllc_CompressedTexSubImage1DARB */
void __stdcall __gllc_CompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format,
                             GLsizei imageSize, const GLvoid *image)
{
    __GLdlistOpICD *dlop;
    struct CompressedTexSubImage1DARB_RecW14 *data;
    GLint imageSize_size;
    __GL_SETUP();

    imageSize_size = __GL_PAD(imageSize);
    if (imageSize_size < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (imageSize_size + sizeof(struct CompressedTexSubImage1DARB_RecW14)));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CompressedTexSubImage1DARB;
    data = (struct CompressedTexSubImage1DARB_RecW14 *) dlop->data;
    data->target = target;
    data->level = level;
    data->xoffset = xoffset;
    data->width = width;
    data->format = format;
    data->imageSize = imageSize;
    memcpy(data + 1, image, imageSize_size);
    __glDlistAppendOp(gc, dlop, __glle_CompressedTexSubImage1DARB);
}

/* 690ae4e0 -- __gllc_CompressedTexSubImage2DARB */
void __stdcall __gllc_CompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format,
                             GLsizei imageSize, const GLvoid *image)
{
    __GLdlistOpICD *dlop;
    struct CompressedTexSubImage2DARB_RecW14 *data;
    GLint imageSize_size;
    __GL_SETUP();

    imageSize_size = __GL_PAD(imageSize);
    if (imageSize_size < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    dlop = __glDlistAllocOp2(gc, (imageSize_size + sizeof(struct CompressedTexSubImage2DARB_RecW14)));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CompressedTexSubImage2DARB;
    data = (struct CompressedTexSubImage2DARB_RecW14 *) dlop->data;
    data->target = target;
    data->level = level;
    data->xoffset = xoffset;
    data->yoffset = yoffset;
    data->width = width;
    data->height = height;
    data->format = format;
    data->imageSize = imageSize;
    memcpy(data + 1, image, imageSize_size);
    __glDlistAppendOp(gc, dlop, __glle_CompressedTexSubImage2DARB);
}

/* 690b2630 -- __glle_Map2 */
const GLubyte * __fastcall __glle_Map2(const GLubyte *PC)
{
    const struct __gllc_Map2f_W20Rec *map2data;
    GLint k;
    __GL_SETUP();

    map2data = (const struct __gllc_Map2f_W20Rec *) PC;
    k = __glEvalComputeK(map2data->target);

    /* Stride of "k" and "k * vorder" matches internal strides */
    (*(__GLmap2fProc)gc->dispatchState->fn[__GL_DISP_Map2f])
	    (map2data->target,
	    map2data->u1, map2data->u2, k * map2data->vorder, map2data->uorder,
	    map2data->v1, map2data->v2, k, map2data->vorder,
	    (const __GLfloat *)(PC + sizeof(*map2data)));

    return PC + sizeof(*map2data) +
	    __glMap2_size(k, map2data->uorder, map2data->vorder) *
	    sizeof(__GLfloat);
}

/* 690b23e0 -- __glle_Map1 */
const GLubyte * __fastcall __glle_Map1(const GLubyte *PC)
{
    const struct __gllc_Map1f_W20Rec *map1data;
    GLint k;
    __GL_SETUP();

    map1data = (const struct __gllc_Map1f_W20Rec *) PC;
    k = __glEvalComputeK(map1data->target);

    /* Stride of "k" matches internal stride */
    (*(__GLmap1fProc)gc->dispatchState->fn[__GL_DISP_Map1f])
	    (map1data->target, map1data->u1, map1data->u2,
	    k, map1data->order, (const __GLfloat *)(PC + sizeof(*map1data)));

    return PC + sizeof(*map1data) +
	    __glMap1_size(k, map1data->order) * sizeof(__GLfloat);
}
typedef __GLdlistOpICD __GLdlistOp;         /* glsi.h */
#define GLLC_ERR(va) ((void (__stdcall *)(__GLcontext *))(va))
#define GL_TABLE_TOO_LARGE  0x8031
struct __gllc_TexCoord2d_Rec { GLdouble s, t; };
struct __gllc_Vertex2d_Rec { GLdouble x, y; };
extern void __stdcall __gllc_LightModelfv(GLenum pname, const GLfloat *params);

/* 690aae70: __gllc_LightModelf */
void __stdcall __gllc_LightModelf(GLenum pname, GLfloat param)
{
    __GL_SETUP();

    if (__glLightModelfv_size(pname) != 1) {
        __gllc_InvalidEnum(gc);
        return;
    }
    __gllc_LightModelfv(pname, &param);
}

/* 690b2250: __gllc_PolygonStipple (S_LCOMP.C) */
void __stdcall __gllc_PolygonStipple(const GLubyte *mask)
{
    __GLdlistOp *dlop;
    __GLcontext *gc = __glCurrentGC();
    GLubyte *newbits;

    dlop = __glDlistAllocOp2(gc, __glImageSize(32, 32, GL_COLOR_INDEX, GL_BITMAP));
    if (dlop == NULL) return;
    dlop->opcode = 0x65;

    newbits = (GLubyte *) dlop->data;
    __glFillImage(gc, 32, 32, GL_COLOR_INDEX, GL_BITMAP, mask, newbits);

    __glDlistAppendOp(gc, dlop, __glle_PolygonStipple);
}

/* 6907f8c0 -- g_lcomp.c __gllc_Error: the __gllc_Invalid* helpers are __stdcall (gc) */
void __stdcall __gllc_Error(__GLcontext *gc, GLenum error)
{
    switch (error) {
      case GL_INVALID_VALUE:
        __gllc_InvalidValue(gc);
        break;
      case GL_INVALID_ENUM:
        __gllc_InvalidEnum(gc);
        break;
      case GL_INVALID_OPERATION:
        GLLC_ERR(__gllc_InvalidOperation)(gc);
        break;
      case GL_TABLE_TOO_LARGE:
        GLLC_ERR(__gllc_TableTooLarge)(gc);
        break;
    }
}

/* 690a8420 -- __gllc_RasterPos2d (g_lcomp.c): the list-compile tables' glRasterPos2d slot, appends
** __glle_RasterPos2dv (the record type is still named after TexCoord2d) */
void __stdcall __gllc_RasterPos2d(GLdouble s, GLdouble t)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord2d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x20;        /* __glop_TexCoord2dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord2d_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    gc->dlist.listData.genericFlags |= 0x20;   /* genericFlags */
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2dv);
}

/* 690a9f30 -- __gllc_Vertex2d (g_lcomp.c) */
void __stdcall __gllc_Vertex2d(GLdouble x, GLdouble y)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex2d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x40;        /* __glop_Vertex2dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Vertex2d_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= 1;      /* genericFlags: the list holds a vertex */
    __glDlistAppendOp(gc, dlop, __glle_Vertex2dv);
}

/*
** gl_varray.c -- s_varray.c: the vertex-array entries (gl*Pointer, glArrayElement, glDrawArrays,
** glInterleavedArrays) and the vertex-cache renderers (SI port: si_s_varray.c).
*/
#include <string.h>
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "icdprotos.h"
#include "icdglob.h"
#include <stdlib.h>

/* 693c3c90, 693c3c91 -- __GL_DISABLE_OG_DRAWVERTEXES, __GL_DISABLE_OG_VARRAY */
GLubyte g_disableOgDrawVertexes;
GLubyte g_disableOgVarray;

#define GL_IUI_V2F_EXT          0x81AD
#define GL_T4F_C4F_N3F_V4F_EXT_LAST 0x81B4      /* the EXT range reuses the core table from its start */
typedef struct __GLinterleavedFormatRec {
    GLboolean enableTexCoord;
    GLboolean enableColor;
    GLboolean enableIndex;
    GLboolean enableNormal;
    GLint sizeTexCoord;
    GLint sizeColor;
    GLint sizeVertex;
    GLenum typeColor;
    GLint offsetColor;
    GLint offsetIndex;
    GLint offsetNormal;
    GLint offsetVertex;
    GLint stride;
} __GLinterleavedFormat;

/* 69005f78 -- s_varray.c interleavedFormats (0x28 bytes each), plus the 3dfx IUI rows */
#define __GL_SIZE_F \
	sizeof(GLfloat)

#define __GL_SIZE_C \
	(((4*sizeof(GLubyte) + (__GL_SIZE_F-1)) / __GL_SIZE_F) * __GL_SIZE_F)

#define __GL_SIZE_I \
	(((sizeof(GLuint) + (__GL_SIZE_F-1)) / __GL_SIZE_F) * __GL_SIZE_F)

static const __GLinterleavedFormat interleavedFormats[] = {
    {	/* V2F */
	GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, 0, 0, 2, 0,
	0, 0, 0, 0,
	2*__GL_SIZE_F,
    },
    {	/* V3F */
	GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE, 0, 0, 3, 0,
	0, 0, 0, 0,
	3*__GL_SIZE_F,
    },
    {	/* C4UB_V2F */
	GL_FALSE,  GL_TRUE, GL_FALSE, GL_FALSE, 0, 4, 2, GL_UNSIGNED_BYTE,
	0, 0, 0, __GL_SIZE_C,
	__GL_SIZE_C+2*__GL_SIZE_F,
    },
    {	/* C4UB_V3F */
	GL_FALSE,  GL_TRUE, GL_FALSE, GL_FALSE, 0, 4, 3, GL_UNSIGNED_BYTE,
	0, 0, 0, __GL_SIZE_C,
	__GL_SIZE_C+3*__GL_SIZE_F,
    },
    {	/* C3F_V3F */
	GL_FALSE,  GL_TRUE, GL_FALSE, GL_FALSE, 0, 3, 3, GL_FLOAT,
	0, 0, 0, 3*__GL_SIZE_F,
	6*__GL_SIZE_F,
    },
    {	/* N3F_V3F */
	GL_FALSE, GL_FALSE, GL_FALSE,  GL_TRUE, 0, 0, 3, 0,
	0, 0, 0, 3*__GL_SIZE_F,
	6*__GL_SIZE_F,
    },
    {	/* C4F_N3F_V3F */
	GL_FALSE,  GL_TRUE, GL_FALSE,  GL_TRUE, 0, 4, 3, GL_FLOAT,
	0, 0, 4*__GL_SIZE_F, 7*__GL_SIZE_F,
	10*__GL_SIZE_F,
    },
    {	/* T2F_V3F */
	 GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE, 2, 0, 3, 0,
	0, 0, 0, 2*__GL_SIZE_F,
	5*__GL_SIZE_F,
    },
    {	/* T4F_V4F */
	 GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE, 4, 0, 4, 0,
	0, 0, 0, 4*__GL_SIZE_F,
	8*__GL_SIZE_F,
    },
    {	/* T2F_C4UB_V3F */
	 GL_TRUE,  GL_TRUE, GL_FALSE, GL_FALSE, 2, 4, 3, GL_UNSIGNED_BYTE,
	2*__GL_SIZE_F, 0, 0, __GL_SIZE_C+2*__GL_SIZE_F,
	__GL_SIZE_C+5*__GL_SIZE_F,
    },
    {	/* T2F_C3F_V3F */
	 GL_TRUE,  GL_TRUE, GL_FALSE, GL_FALSE, 2, 3, 3, GL_FLOAT,
	2*__GL_SIZE_F, 0, 0, 5*__GL_SIZE_F, 8*__GL_SIZE_F,
    },
    {	/* T2F_N3F_V3F */
	 GL_TRUE, GL_FALSE, GL_FALSE,  GL_TRUE, 2, 0, 3, 0,
	0, 0, 2*__GL_SIZE_F, 5*__GL_SIZE_F,
	8*__GL_SIZE_F,
    },
    {	/* T2F_C4F_N3F_V3F */
	 GL_TRUE,  GL_TRUE, GL_FALSE,  GL_TRUE, 2, 4, 3, GL_FLOAT,
	2*__GL_SIZE_F, 0, 6*__GL_SIZE_F, 9*__GL_SIZE_F,
	12*__GL_SIZE_F,
    },
    {	/* T4F_C4F_N3F_V4F */
	 GL_TRUE,  GL_TRUE, GL_FALSE,  GL_TRUE, 4, 4, 4, GL_FLOAT,
	4*__GL_SIZE_F, 0, 8*__GL_SIZE_F, 11*__GL_SIZE_F,
	15*__GL_SIZE_F,
    },
    /* GL_EXT_index_array_formats (3dfx): never read, as glInterleavedArrays indexes the EXT range from row 0 */
    {	/* IUI_V2F */
	GL_FALSE, GL_FALSE,  GL_TRUE, GL_FALSE, 0, 0, 2, 0,
	0, 0, 0, __GL_SIZE_I,
	__GL_SIZE_I+2*__GL_SIZE_F,
    },
    {	/* IUI_V3F */
	GL_FALSE, GL_FALSE,  GL_TRUE, GL_FALSE, 0, 0, 3, 0,
	0, 0, 0, __GL_SIZE_I,
	__GL_SIZE_I+3*__GL_SIZE_F,
    },
    {	/* IUI_N3F_V2F */
	GL_FALSE, GL_FALSE,  GL_TRUE,  GL_TRUE, 0, 0, 2, 0,
	0, 0, __GL_SIZE_I, __GL_SIZE_I+3*__GL_SIZE_F,
	__GL_SIZE_I+5*__GL_SIZE_F,
    },
    {	/* IUI_N3F_V3F */
	GL_FALSE, GL_FALSE,  GL_TRUE,  GL_TRUE, 0, 0, 3, 0,
	0, 0, __GL_SIZE_I, __GL_SIZE_I+3*__GL_SIZE_F,
	__GL_SIZE_I+6*__GL_SIZE_F,
    },
    {	/* T2F_IUI_V2F: the index offset is in the normal's place (original bug, kept) */
	 GL_TRUE, GL_FALSE,  GL_TRUE, GL_FALSE, 2, 0, 2, 0,
	0, 0, 2*__GL_SIZE_F, 2*__GL_SIZE_F+__GL_SIZE_I,
	__GL_SIZE_I+4*__GL_SIZE_F,
    },
    {	/* T2F_IUI_V3F */
	 GL_TRUE, GL_FALSE,  GL_TRUE, GL_FALSE, 2, 0, 3, 0,
	0, 2*__GL_SIZE_F, 0, 2*__GL_SIZE_F+__GL_SIZE_I,
	__GL_SIZE_I+5*__GL_SIZE_F,
    },
    {	/* T2F_IUI_N3F_V2F */
	 GL_TRUE, GL_FALSE,  GL_TRUE,  GL_TRUE, 2, 0, 2, 0,
	0, 2*__GL_SIZE_F, 2*__GL_SIZE_F+__GL_SIZE_I, 5*__GL_SIZE_F+__GL_SIZE_I,
	__GL_SIZE_I+7*__GL_SIZE_F,
    },
    {	/* T2F_IUI_N3F_V3F */
	 GL_TRUE, GL_FALSE,  GL_TRUE,  GL_TRUE, 2, 0, 3, 0,
	0, 2*__GL_SIZE_F, 2*__GL_SIZE_F+__GL_SIZE_I, 5*__GL_SIZE_F+__GL_SIZE_I,
	__GL_SIZE_I+8*__GL_SIZE_F,
    },
};
/* ---- the vertex-array quad clip ---- */
#define CC(v)  ((v)->flagBits)
/* ---- vertex array drawing from the vertex cache (gc->vertexArray.drawArrays[mode]); siblings in
** src/gl/gl_attrib.c --------------------------------------------------------------------------- */
#define __GL_VARRAY_CACHE_DIRTY 0x2
#define __glVarrayClipTriangle(gc, v0, v1, v2) \
    ((void (__fastcall *)(__GLcontext *, __GLvertexICD *, __GLvertexICD *, __GLvertexICD *)) __glDoClipTriangle)(gc, v0, v1, v2)
/* 6907da80: a second __glNop4 (ret 8), the texture coordinate fetch default */
extern void __fastcall __glDrawVertexes_Lines(__GLcontext *gc, GLint first, GLsizei count, const GLint *indices);    /* GL_LINES */
extern void __fastcall __glDrawVertexes_Lstrip(__GLcontext *gc, GLint first, GLsizei count, const GLint *indices);   /* GL_LINE_STRIP */
extern void __fastcall __glDrawVertexes_Tstrip(__GLcontext *gc, GLint first, GLsizei count, const GLint *indices);   /* GL_TRIANGLE_STRIP */
extern void __fastcall __glDrawVertexes_Tfan(__GLcontext *gc, GLint first, GLsizei count, const GLint *indices);   /* GL_TRIANGLE_FAN */
extern void __fastcall __glDrawVertexes_Quads(__GLcontext *gc, GLint first, GLsizei count, const GLint *indices);    /* GL_QUADS */
extern void __fastcall __glDrawVertexes_Qstrip(__GLcontext *gc, GLint first, GLsizei count, const GLint *indices);    /* GL_QUAD_STRIP */
/* gc->imports: malloc [0], realloc [2] */
#define __GL_IMPORT_MALLOC(gc, n) \
    ((GLuint (__fastcall *)(__GLcontext *, size_t)) (gc)->imports[0])(gc, n)
#define __GL_IMPORT_REALLOC(gc, p, n) \
    ((GLuint (__fastcall *)(__GLcontext *, GLuint, size_t)) (gc)->imports[2])(gc, p, n)
/* the gl*v entry points the array fetch procs are (APIENTRY: one pointer, or unit + pointer) */
typedef void (APIENTRY *__GLvaFetchProc)(const GLvoid *);
typedef void (APIENTRY *__GLvaTexFetchProc)(GLint, const GLvoid *);
#define __GL_VA_FETCH(a, i) \
    (*(__GLvaFetchProc) (a).fetch)((const GLvoid *) ((a).pointer + (i) * (a).stride))
#define __GL_LOCK_RENDER_BUFFERS(gc) \
    if ((++(gc)->buffers.lockCnt == 1) && (gc)->buffers.lockRenderBuffers) \
	((void (__fastcall *)(__GLcontext *))(gc)->buffers.lockRenderBuffers)(gc)
#define __GL_UNLOCK_RENDER_BUFFERS(gc) \
    if ((--(gc)->buffers.lockCnt == 0) && (gc)->buffers.unlockRenderBuffers) \
	((void (__fastcall *)(__GLcontext *))(gc)->buffers.unlockRenderBuffers)(gc)
/* vertexArray.flags: 0x60 draw through glBegin / glArrayElement / glEnd, 2 cache dirty, 1 locked range */
#define VERTARRAY_SLOWPATH_ICD          0x60
#define VERTARRAY_NEEDS_FILL_ICD        0x2
#define VERTARRAY_LOCKED_ICD            0x1
/* s_varray.c gl*Pointer tables, the ICD's layout (as gl_vadraw.c): per array a pointer to [fetch, fetch2][type][size]
** (fog_coord_func_table is the ICD's), the default strides [type][size] and the per-type format codes */
typedef __GLproc __GLvaProcTable[2][GL_DOUBLE - GL_BYTE + 1][5];
/* the procs below that are defined further down or in other files */
extern void __fastcall CopyColor3f(__GLcontext *gc, const GLfloat *cp, __GLvertexICD *v);
extern void __fastcall CopyColor3ui(__GLcontext *gc, const GLuint *cp, __GLvertexICD *v);
extern void __fastcall CopyColor4b(__GLcontext *gc, const GLbyte *cp, __GLvertexICD *v);
extern void __fastcall CopyColor4f(__GLcontext *gc, const GLfloat *cp, __GLvertexICD *v);
extern void __fastcall CopyColor4s(__GLcontext *gc, const GLshort *cp, __GLvertexICD *v);
extern void __fastcall CopyColor4ui(__GLcontext *gc, const GLuint *cp, __GLvertexICD *v);
extern void __fastcall CopyFogCoordd(__GLcontext *gc, const GLdouble *fp, __GLvertexICD *v);
extern void __fastcall CopyFogCoordf(__GLcontext *gc, const GLfloat *fp, __GLvertexICD *v);
extern void __fastcall CopyNormal3f(__GLcontext *gc, const GLfloat *np, __GLvertexICD *v);
extern void __fastcall CopyTexCoord1d(GLint unit, __GLcontext *gc, const GLdouble *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord1f(GLint unit, __GLcontext *gc, const GLfloat *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord1i(GLint unit, __GLcontext *gc, const GLint *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord1s(GLint unit, __GLcontext *gc, const GLshort *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord2d(GLint unit, __GLcontext *gc, const GLdouble *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord2f(GLint unit, __GLcontext *gc, const GLfloat *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord2i(GLint unit, __GLcontext *gc, const GLint *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord2s(GLint unit, __GLcontext *gc, const GLshort *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord3d(GLint unit, __GLcontext *gc, const GLdouble *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord3f(GLint unit, __GLcontext *gc, const GLfloat *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord3i(GLint unit, __GLcontext *gc, const GLint *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord3s(GLint unit, __GLcontext *gc, const GLshort *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord4d(GLint unit, __GLcontext *gc, const GLdouble *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord4f(GLint unit, __GLcontext *gc, const GLfloat *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord4i(GLint unit, __GLcontext *gc, const GLint *tp, __GLvertexICD *v);
extern void __fastcall CopyTexCoord4s(GLint unit, __GLcontext *gc, const GLshort *tp, __GLvertexICD *v);
extern void __fastcall CopyVertex3d(__GLcontext *gc, const GLdouble *vp, __GLvertexICD *v);
extern void __fastcall CopyVertex3f(__GLcontext *gc, const GLfloat *vp, __GLvertexICD *v);
extern void __fastcall CopyVertex3i(__GLcontext *gc, const GLint *vp, __GLvertexICD *v);
extern void __fastcall CopyVertex3s(__GLcontext *gc, const GLshort *vp, __GLvertexICD *v);
extern void __fastcall CopyVertex4f(__GLcontext *gc, const GLfloat *vp, __GLvertexICD *v);
extern void __fastcall CopyVertex4s(__GLcontext *gc, const GLshort *vp, __GLvertexICD *v);
/* 690f46d0 .. 690f5258 -- s_varray.c gl*Pointer tables, the ICD's layout: the default strides [type][size], per array its
** procs [fetch, copy][type][size] and a pointer to them (fog_coord_func_table is the ICD's), the per-type format codes */
static int stride_array[GL_DOUBLE - GL_BYTE + 1][5] = {
    0, sizeof(GLbyte),		2*sizeof(GLbyte),	3*sizeof(GLbyte),
    4*sizeof(GLbyte),
    0, sizeof(GLubyte), 	2*sizeof(GLubyte),      3*sizeof(GLubyte),
    4*sizeof(GLubyte),
    0, sizeof(GLshort), 	2*sizeof(GLshort),      3*sizeof(GLshort), 
    4*sizeof(GLshort),
    0, sizeof(GLushort), 	2*sizeof(GLushort),     3*sizeof(GLushort),
    4*sizeof(GLushort),
    0, sizeof(GLint), 		2*sizeof(GLint),        3*sizeof(GLint), 
    4*sizeof(GLint),
    0, sizeof(GLuint), 		2*sizeof(GLuint),       3*sizeof(GLuint),
    4*sizeof(GLuint),
    0, sizeof(GLfloat), 	2*sizeof(GLfloat),  	3*sizeof(GLfloat),
    4*sizeof(GLfloat),
    0, 0, 0,                    0,                   	0,
    0, 0, 0,                    0,                      0,
    0, 0, 0,                    0,                      0,
    0, sizeof(GLdouble),	2*sizeof(GLdouble),	3*sizeof(GLdouble),
    4*sizeof(GLdouble),
};

static __GLproc vertex_funcs[2][GL_DOUBLE - GL_BYTE + 1][5] = {
    {   /* the slow path: the gl* entry points */
        { 0, 0, 0, 0, 0 },   /* GL_BYTE */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_BYTE */
        { 0, 0, (__GLproc)glVertex2sv, (__GLproc)glVertex3sv, (__GLproc)glVertex4sv },   /* GL_SHORT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_SHORT */
        { 0, 0, (__GLproc)glVertex2iv, (__GLproc)glVertex3iv, (__GLproc)glVertex4iv },   /* GL_INT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_INT */
        { 0, 0, (__GLproc)glVertex2fv, (__GLproc)glVertex3fv, (__GLproc)glVertex4fv },   /* GL_FLOAT */
        { 0, 0, 0, 0, 0 },   /* GL_2_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_3_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_4_BYTES */
        { 0, 0, (__GLproc)glVertex2dv, (__GLproc)glVertex3dv, (__GLproc)glVertex4dv },   /* GL_DOUBLE */
    },
    {   /* the fast path: copy procs */
        { 0, 0, 0, 0, 0 },   /* GL_BYTE */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_BYTE */
        { 0, 0, (__GLproc)CopyVertex2s, (__GLproc)CopyVertex3s, (__GLproc)CopyVertex4s },   /* GL_SHORT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_SHORT */
        { 0, 0, (__GLproc)CopyVertex2i, (__GLproc)CopyVertex3i, (__GLproc)CopyVertex4i },   /* GL_INT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_INT */
        { 0, 0, (__GLproc)CopyVertex2f, (__GLproc)CopyVertex3f, (__GLproc)CopyVertex4f },   /* GL_FLOAT */
        { 0, 0, 0, 0, 0 },   /* GL_2_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_3_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_4_BYTES */
        { 0, 0, (__GLproc)CopyVertex2d, (__GLproc)CopyVertex3d, (__GLproc)CopyVertex4d },   /* GL_DOUBLE */
    },
};
static __GLvaProcTable *vertex_func_table = &vertex_funcs;

static __GLproc normal_funcs[2][GL_DOUBLE - GL_BYTE + 1][5] = {
    {   /* the slow path: the gl* entry points */
        { 0, 0, 0, (__GLproc)glNormal3bv, 0 },   /* GL_BYTE */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_BYTE */
        { 0, 0, 0, (__GLproc)glNormal3sv, 0 },   /* GL_SHORT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_SHORT */
        { 0, 0, 0, (__GLproc)glNormal3iv, 0 },   /* GL_INT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_INT */
        { 0, 0, 0, (__GLproc)glNormal3fv, 0 },   /* GL_FLOAT */
        { 0, 0, 0, 0, 0 },   /* GL_2_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_3_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_4_BYTES */
        { 0, 0, 0, (__GLproc)glNormal3dv, 0 },   /* GL_DOUBLE */
    },
    {   /* the fast path: copy procs */
        { 0, 0, 0, (__GLproc)CopyNormal3b, 0 },   /* GL_BYTE */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_BYTE */
        { 0, 0, 0, (__GLproc)CopyNormal3s, 0 },   /* GL_SHORT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_SHORT */
        { 0, 0, 0, (__GLproc)CopyNormal3i, 0 },   /* GL_INT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_INT */
        { 0, 0, 0, (__GLproc)CopyNormal3f, 0 },   /* GL_FLOAT */
        { 0, 0, 0, 0, 0 },   /* GL_2_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_3_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_4_BYTES */
        { 0, 0, 0, (__GLproc)CopyNormal3d, 0 },   /* GL_DOUBLE */
    },
};
static __GLvaProcTable *normal_func_table = &normal_funcs;

static __GLproc color_funcs[2][GL_DOUBLE - GL_BYTE + 1][5] = {
    {   /* the slow path: the gl* entry points */
        { 0, 0, 0, (__GLproc)glColor3bv, (__GLproc)glColor4bv },   /* GL_BYTE */
        { 0, 0, 0, (__GLproc)glColor3ubv, (__GLproc)glColor4ubv },   /* GL_UNSIGNED_BYTE */
        { 0, 0, 0, (__GLproc)glColor3sv, (__GLproc)glColor4sv },   /* GL_SHORT */
        { 0, 0, 0, (__GLproc)glColor3usv, (__GLproc)glColor4usv },   /* GL_UNSIGNED_SHORT */
        { 0, 0, 0, (__GLproc)glColor3iv, (__GLproc)glColor4iv },   /* GL_INT */
        { 0, 0, 0, (__GLproc)glColor3uiv, (__GLproc)glColor4uiv },   /* GL_UNSIGNED_INT */
        { 0, 0, 0, (__GLproc)glColor3fv, (__GLproc)glColor4fv },   /* GL_FLOAT */
        { 0, 0, 0, 0, 0 },   /* GL_2_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_3_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_4_BYTES */
        { 0, 0, 0, (__GLproc)glColor3dv, (__GLproc)glColor4dv },   /* GL_DOUBLE */
    },
    {   /* the fast path: copy procs */
        { 0, 0, 0, (__GLproc)CopyColor3b, (__GLproc)CopyColor4b },   /* GL_BYTE */
        { 0, 0, 0, (__GLproc)CopyColor3ub, (__GLproc)CopyColor4ub },   /* GL_UNSIGNED_BYTE */
        { 0, 0, 0, (__GLproc)CopyColor3s, (__GLproc)CopyColor4s },   /* GL_SHORT */
        { 0, 0, 0, (__GLproc)CopyColor3us, (__GLproc)CopyColor4us },   /* GL_UNSIGNED_SHORT */
        { 0, 0, 0, (__GLproc)CopyColor3i, (__GLproc)CopyColor4i },   /* GL_INT */
        { 0, 0, 0, (__GLproc)CopyColor3ui, (__GLproc)CopyColor4ui },   /* GL_UNSIGNED_INT */
        { 0, 0, 0, (__GLproc)CopyColor3f, (__GLproc)CopyColor4f },   /* GL_FLOAT */
        { 0, 0, 0, 0, 0 },   /* GL_2_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_3_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_4_BYTES */
        { 0, 0, 0, (__GLproc)CopyColor3d, (__GLproc)CopyColor4d },   /* GL_DOUBLE */
    },
};
static __GLvaProcTable *color_func_table = &color_funcs;

static __GLproc index_funcs[2][GL_DOUBLE - GL_BYTE + 1][5] = {
    {   /* the slow path: the gl* entry points */
        { 0, 0, 0, 0, 0 },   /* GL_BYTE */
        { 0, (__GLproc)glIndexubv, 0, 0, 0 },   /* GL_UNSIGNED_BYTE */
        { 0, (__GLproc)glIndexsv, 0, 0, 0 },   /* GL_SHORT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_SHORT */
        { 0, (__GLproc)glIndexiv, 0, 0, 0 },   /* GL_INT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_INT */
        { 0, (__GLproc)glIndexfv, 0, 0, 0 },   /* GL_FLOAT */
        { 0, 0, 0, 0, 0 },   /* GL_2_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_3_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_4_BYTES */
        { 0, (__GLproc)glIndexdv, 0, 0, 0 },   /* GL_DOUBLE */
    },
    {   /* the fast path: copy procs */
        { 0, 0, 0, 0, 0 },   /* GL_BYTE */
        { 0, (__GLproc)CopyIndexub, 0, 0, 0 },   /* GL_UNSIGNED_BYTE */
        { 0, (__GLproc)CopyIndexs, 0, 0, 0 },   /* GL_SHORT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_SHORT */
        { 0, (__GLproc)CopyIndexi, 0, 0, 0 },   /* GL_INT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_INT */
        { 0, (__GLproc)CopyIndexf, 0, 0, 0 },   /* GL_FLOAT */
        { 0, 0, 0, 0, 0 },   /* GL_2_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_3_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_4_BYTES */
        { 0, (__GLproc)CopyIndexd, 0, 0, 0 },   /* GL_DOUBLE */
    },
};
static __GLvaProcTable *index_func_table = &index_funcs;

static __GLproc tex_coord_funcs[2][GL_DOUBLE - GL_BYTE + 1][5] = {
    {   /* the slow path: the gl* entry points */
        { 0, 0, 0, 0, 0 },   /* GL_BYTE */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_BYTE */
        { 0, (__GLproc)glMultiTexCoord1svARB, (__GLproc)glMultiTexCoord2svARB, (__GLproc)glMultiTexCoord3svARB, (__GLproc)glMultiTexCoord4svARB },   /* GL_SHORT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_SHORT */
        { 0, (__GLproc)glMultiTexCoord1ivARB, (__GLproc)glMultiTexCoord2ivARB, (__GLproc)glMultiTexCoord3ivARB, (__GLproc)glMultiTexCoord4ivARB },   /* GL_INT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_INT */
        { 0, (__GLproc)glMultiTexCoord1fvARB, (__GLproc)glMultiTexCoord2fvARB, (__GLproc)glMultiTexCoord3fvARB, (__GLproc)glMultiTexCoord4fvARB },   /* GL_FLOAT */
        { 0, 0, 0, 0, 0 },   /* GL_2_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_3_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_4_BYTES */
        { 0, (__GLproc)glMultiTexCoord1dvARB, (__GLproc)glMultiTexCoord2dvARB, (__GLproc)glMultiTexCoord3dvARB, (__GLproc)glMultiTexCoord4dvARB },   /* GL_DOUBLE */
    },
    {   /* the fast path: copy procs */
        { 0, 0, 0, 0, 0 },   /* GL_BYTE */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_BYTE */
        { 0, (__GLproc)CopyTexCoord1s, (__GLproc)CopyTexCoord2s, (__GLproc)CopyTexCoord3s, (__GLproc)CopyTexCoord4s },   /* GL_SHORT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_SHORT */
        { 0, (__GLproc)CopyTexCoord1i, (__GLproc)CopyTexCoord2i, (__GLproc)CopyTexCoord3i, (__GLproc)CopyTexCoord4i },   /* GL_INT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_INT */
        { 0, (__GLproc)CopyTexCoord1f, (__GLproc)CopyTexCoord2f, (__GLproc)CopyTexCoord3f, (__GLproc)CopyTexCoord4f },   /* GL_FLOAT */
        { 0, 0, 0, 0, 0 },   /* GL_2_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_3_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_4_BYTES */
        { 0, (__GLproc)CopyTexCoord1d, (__GLproc)CopyTexCoord2d, (__GLproc)CopyTexCoord3d, (__GLproc)CopyTexCoord4d },   /* GL_DOUBLE */
    },
};
static __GLvaProcTable *tex_coord_func_table = &tex_coord_funcs;

static __GLproc fog_coord_funcs[2][GL_DOUBLE - GL_BYTE + 1][5] = {
    {   /* the slow path: the gl* entry points */
        { 0, 0, 0, 0, 0 },   /* GL_BYTE */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_BYTE */
        { 0, 0, 0, 0, 0 },   /* GL_SHORT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_SHORT */
        { 0, 0, 0, 0, 0 },   /* GL_INT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_INT */
        { 0, (__GLproc)glFogCoordfvEXT, 0, 0, 0 },   /* GL_FLOAT */
        { 0, 0, 0, 0, 0 },   /* GL_2_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_3_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_4_BYTES */
        { 0, (__GLproc)glFogCoorddvEXT, 0, 0, 0 },   /* GL_DOUBLE */
    },
    {   /* the fast path: copy procs */
        { 0, 0, 0, 0, 0 },   /* GL_BYTE */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_BYTE */
        { 0, 0, 0, 0, 0 },   /* GL_SHORT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_SHORT */
        { 0, 0, 0, 0, 0 },   /* GL_INT */
        { 0, 0, 0, 0, 0 },   /* GL_UNSIGNED_INT */
        { 0, (__GLproc)CopyFogCoordf, 0, 0, 0 },   /* GL_FLOAT */
        { 0, 0, 0, 0, 0 },   /* GL_2_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_3_BYTES */
        { 0, 0, 0, 0, 0 },   /* GL_4_BYTES */
        { 0, (__GLproc)CopyFogCoordd, 0, 0, 0 },   /* GL_DOUBLE */
    },
};
static __GLvaProcTable *fog_coord_func_table = &fog_coord_funcs;

static GLuint type_flag_table[GL_DOUBLE - GL_BYTE + 1] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0, 0, 0, 0xb };
extern void __fastcall DrawVerticesPoints(__GLcontext *gc, GLint first, GLsizei count, GLuint *elements);
extern void __fastcall DrawVerticesTriangles(__GLcontext *gc, GLint first, GLsizei count, GLuint *elements);
extern GLboolean __fastcall AllocateVbuf(__GLcontext *gc, int first, int count);

/* ---- s_varray.c colour copies: these were compiled with no-aliasing-across-pointers (/Ow or /Oa:
** the next element's byte load is scheduled above the previous element's float store) ---------- */
#pragma optimize("w", on)
/* 690aed70 -- s_varray.c CopyColor4b: the front colour from four GLbytes (__GL_B_TO_FLOAT), clamped and scaled */
void __fastcall CopyColor4b(__GLcontext *gc, const GLbyte *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = __GL_B_TO_FLOAT(cp[0]);
    v->colors[__GL_FRONTFACE].g = __GL_B_TO_FLOAT(cp[1]);
    v->colors[__GL_FRONTFACE].b = __GL_B_TO_FLOAT(cp[2]);
    v->colors[__GL_FRONTFACE].a = __GL_B_TO_FLOAT(cp[3]);
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}
#pragma optimize("", on)

#pragma optimize("w", on)
/* 690aee50 -- s_varray.c CopyColor4s: the front colour from four GLshorts (__GL_S_TO_FLOAT), clamped and scaled */
void __fastcall CopyColor4s(__GLcontext *gc, const GLshort *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = __GL_S_TO_FLOAT(cp[0]);
    v->colors[__GL_FRONTFACE].g = __GL_S_TO_FLOAT(cp[1]);
    v->colors[__GL_FRONTFACE].b = __GL_S_TO_FLOAT(cp[2]);
    v->colors[__GL_FRONTFACE].a = __GL_S_TO_FLOAT(cp[3]);
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}
#pragma optimize("", on)

#pragma optimize("a", on)
/* 690aefb0 -- CopyColor4ui (s_varray.c): the front colour from four GLuints (__GL_UI_TO_FLOAT), clamped and scaled */
void __fastcall CopyColor4ui(__GLcontext *gc, const GLuint *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = __GL_UI_TO_FLOAT(cp[0]);
    v->colors[__GL_FRONTFACE].g = __GL_UI_TO_FLOAT(cp[1]);
    v->colors[__GL_FRONTFACE].b = __GL_UI_TO_FLOAT(cp[2]);
    v->colors[__GL_FRONTFACE].a = __GL_UI_TO_FLOAT(cp[3]);
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}
#pragma optimize("", on)

/************************************************************************/
/* s_varray.c texture coordinate copy procs, per unit in the ICD (2s / 3s below) */

/* 690af100 -- CopyTexCoord1s: texture[unit] = (s, 0, 0, 1) from 1 GLshort */
void __fastcall CopyTexCoord1s(GLint unit, __GLcontext *gc, const GLshort *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = 0.0F; v->texture[unit].z = 0.0F; v->texture[unit].w = 1.0F;
}

/* 690af140 -- CopyTexCoord1i: texture[unit] = (s, 0, 0, 1) from 1 GLint */
void __fastcall CopyTexCoord1i(GLint unit, __GLcontext *gc, const GLint *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = 0.0F; v->texture[unit].z = 0.0F; v->texture[unit].w = 1.0F;
}

/* 690af170 -- CopyTexCoord1f: texture[unit] = (s, 0, 0, 1) from 1 GLfloat */
void __fastcall CopyTexCoord1f(GLint unit, __GLcontext *gc, const GLfloat *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = 0.0F; v->texture[unit].z = 0.0F; v->texture[unit].w = 1.0F;
}

/* 690af1a0 -- CopyTexCoord1d: texture[unit] = (s, 0, 0, 1) from 1 GLdouble */
void __fastcall CopyTexCoord1d(GLint unit, __GLcontext *gc, const GLdouble *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = 0.0F; v->texture[unit].z = 0.0F; v->texture[unit].w = 1.0F;
}

/************************************************************************/
/* s_varray.c texture coordinate copy procs, per unit in the ICD */

/* 690af1d0 -- CopyTexCoord2s: texture[unit] = (s, t, 0, 1) from 2 GLshorts */
void __fastcall CopyTexCoord2s(GLint unit, __GLcontext *gc, const GLshort *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = tp[1]; v->texture[unit].z = 0.0F; v->texture[unit].w = 1.0F;
}

#pragma optimize("a", on)
/* 690af220 -- CopyTexCoord2i: texture[unit] = (s, t, 0, 1) from 2 GLints */
void __fastcall CopyTexCoord2i(GLint unit, __GLcontext *gc, const GLint *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = tp[1]; v->texture[unit].z = 0.0F; v->texture[unit].w = 1.0F;
}
#pragma optimize("", on)

#pragma optimize("a", on)
/* 690af260 -- CopyTexCoord2f: texture[unit] = (s, t, 0, 1) from 2 GLfloats */
void __fastcall CopyTexCoord2f(GLint unit, __GLcontext *gc, const GLfloat *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = tp[1]; v->texture[unit].z = 0.0F; v->texture[unit].w = 1.0F;
}
#pragma optimize("", on)

#pragma optimize("a", on)
/* 690af2a0 -- CopyTexCoord2d: texture[unit] = (s, t, 0, 1) from 2 GLdoubles */
void __fastcall CopyTexCoord2d(GLint unit, __GLcontext *gc, const GLdouble *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = tp[1]; v->texture[unit].z = 0.0F; v->texture[unit].w = 1.0F;
}
#pragma optimize("", on)

/* The original schedules the source loads above the float stores: compiled assuming no aliasing
** (/Oa); the pragma gives that per function. */
#pragma optimize("a", on)
/* 690af2e0 -- CopyTexCoord3s: texture[unit] = (s, t, r, 1) from 3 GLshorts */
void __fastcall CopyTexCoord3s(GLint unit, __GLcontext *gc, const GLshort *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = tp[1]; v->texture[unit].z = tp[2]; v->texture[unit].w = 1.0F;
}
#pragma optimize("", on)

#pragma optimize("a", on)
/* 690af330 -- CopyTexCoord3i: texture[unit] = (s, t, r, 1) from 3 GLints */
void __fastcall CopyTexCoord3i(GLint unit, __GLcontext *gc, const GLint *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = tp[1]; v->texture[unit].z = tp[2]; v->texture[unit].w = 1.0F;
}
#pragma optimize("", on)

#pragma optimize("a", on)
/* 690af360 -- CopyTexCoord3f: texture[unit] = (s, t, r, 1) from 3 GLfloats */
void __fastcall CopyTexCoord3f(GLint unit, __GLcontext *gc, const GLfloat *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = tp[1]; v->texture[unit].z = tp[2]; v->texture[unit].w = 1.0F;
}
#pragma optimize("", on)

#pragma optimize("a", on)
/* 690af3a0 -- CopyTexCoord3d: texture[unit] = (s, t, r, 1) from 3 GLdoubles */
void __fastcall CopyTexCoord3d(GLint unit, __GLcontext *gc, const GLdouble *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = tp[1]; v->texture[unit].z = tp[2]; v->texture[unit].w = 1.0F;
}
#pragma optimize("", on)

#pragma optimize("a", on)
/* 690af3d0 -- CopyTexCoord4s: texture[unit] = (s, t, r, q) from 4 GLshorts */
void __fastcall CopyTexCoord4s(GLint unit, __GLcontext *gc, const GLshort *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = tp[1]; v->texture[unit].z = tp[2]; v->texture[unit].w = tp[3];
}
#pragma optimize("", on)

/* 690af630 -- __glim_InterleavedArrays */
void __stdcall __glim_InterleavedArrays(GLenum format, GLsizei stride,
			     const GLvoid *pointer)
{
    const GLubyte *p = (const GLubyte *) pointer;
    const __GLinterleavedFormat *f;

    __GL_SETUP_NOT_IN_BEGIN();

    if (stride < 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    if ((int) format >= GL_V2F &&
	(int) format <= GL_T4F_C4F_N3F_V4F) {
	f = &interleavedFormats[(int) format - (int) GL_V2F];

    } else if ((int) format >= GL_IUI_V2F_EXT &&
	(int) format <= GL_T4F_C4F_N3F_V4F_EXT_LAST) {
	f = &interleavedFormats[(int) format - (int) GL_IUI_V2F_EXT];

    } else {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    __GL_API_BLAND();

    if (stride == 0) stride = f->stride;

    glDisableClientState(GL_EDGE_FLAG_ARRAY);

    if (f->enableTexCoord) {
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(f->sizeTexCoord, GL_FLOAT, stride, p);
    } else {
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    if (f->enableColor) {
	glColorPointer(f->sizeColor, f->typeColor, stride, p+f->offsetColor);
	glEnableClientState(GL_COLOR_ARRAY);
    } else {
	glDisableClientState(GL_COLOR_ARRAY);
    }
    if (f->enableIndex) {
	glIndexPointer(GL_INT, stride, p+f->offsetIndex);
	glEnableClientState(GL_INDEX_ARRAY);
    } else {
	glDisableClientState(GL_INDEX_ARRAY);
    }
    if (f->enableNormal) {
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, stride, p+f->offsetNormal);
    } else {
	glDisableClientState(GL_NORMAL_ARRAY);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(f->sizeVertex, GL_FLOAT, stride, p+f->offsetVertex);

    gc->vertexArray.interleavedPointer = pointer;
    gc->vertexArray.interleavedFormat = format;

    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
}

/* 690af880 -- __glDrawVertexes_Tstrip2 (3dfx S_VARRAY.C): an indexed triangle strip without clipping, edge
** flags or provoking vertex: a triangle
** is drawn only when none of its vertices has a clip code */
void __fastcall __glDrawVertexes_Tstrip2(__GLcontext *gc, GLint first, GLsizei count, GLuint *elements)
{
    __GLvertexICD *vBuf = gc->vertexArray.vertexCache;
    __GLvertexICD *v0, *v1, *v2;
    int i;

    if (count < 3) return;

    v0 = vBuf + elements[0];
    v2 = vBuf + elements[1];

    for (i = 2; i < count;) {
	if (i & 1) {
	    v0 = v2;
	} else {
	    v1 = v2;
	}
	v2 = vBuf + elements[i++];

	if (((v1->flagBits | v2->flagBits | v0->flagBits) & __GL_CLIP_ALL_MASK) == 0) {
	    (*gc->procs.renderTriangle)(gc, v0, v1, v2);
	}
    }
}

/* 690afde0 -- ClipQuad (3dfx S_VARRAY.C): fetch four cached vertices, then clip the quad unless all four share an
** outcode */
void __fastcall ClipQuad(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c, __GLvertexICD *d)
{
    __GLvertexICD *iv[4];
    (*gc->vertexArray.fetchVertex)(gc, a);
    (*gc->vertexArray.fetchVertex)(gc, b);
    (*gc->vertexArray.fetchVertex)(gc, c);
    (*gc->vertexArray.fetchVertex)(gc, d);
    if ((CC(a) & CC(b) & CC(c) & CC(d) & 0xffff2000) == 0) {
        iv[0] = a;
        iv[1] = b;
        iv[2] = c;
        iv[3] = d;
        __glDoPolygonClip(gc, iv, 4, (CC(a) | CC(b) | CC(c) | CC(d)) & 0xffff2000);
    }
}

/* 690b0190 -- DrawVerticesTriangles (drawArrays[GL_TRIANGLES]): indexed lists go to procs.drawIndexedTriangles
** as (gc, elements + first, vertex cache, count - first - 2) */
void __fastcall DrawVerticesTriangles(__GLcontext *gc, GLint first, GLsizei count, GLuint *elements)
{
    __GLvertexICD *vBuf;
    __GLvertexICD *v0, *v1, *v2;
    int i;

    if (count < 3) return;

    if (gc->vertexArray.flags & __GL_VARRAY_CACHE_DIRTY) {
	(*gc->vertexArray.prepareCache)(gc, 0, first, count, elements);
    }
    vBuf = gc->vertexArray.vertexCache;

    if (elements) {
	(*gc->procs.drawIndexedTriangles)(gc, elements + first, vBuf, count - first - 2);
	return;
    }

    for (i = 0; i + 2 < count;) {
	v0 = vBuf + first + i++;
	v1 = vBuf + first + i++;
	v2 = vBuf + first + i++;
	gc->line.notResetStipple = GL_FALSE;
	gc->vertex.provoking = v2;
	if (((v0->flagBits | v1->flagBits | v2->flagBits) & __GL_CLIP_ALL_MASK) == 0) {
	    (*gc->procs.renderTriangle)(gc, v0, v1, v2);
	} else if ((v0->flagBits & v1->flagBits & v2->flagBits & __GL_CLIP_ALL_MASK) == 0) {
	    __glVarrayClipTriangle(gc, v0, v1, v2);
	}
    }
}

/* 690b0770 -- DrawVerticesPoints (drawArrays[GL_POINTS]): refill the vertex cache if dirty, then validate (for
** getFaceNeeds0 | 1) and renderPoint each cached vertex (first + i, or elements[i]) that is inside all clip planes;
** clipped points are dropped */
void __fastcall DrawVerticesPoints(__GLcontext *gc, GLint first, GLsizei count, GLuint *elements)
{
    __GLvertexICD *vBuf;
    __GLvertexICD *v;
    GLuint flags;
    int i;

    if (gc->vertexArray.flags & __GL_VARRAY_CACHE_DIRTY) {
	(*gc->vertexArray.prepareCache)(gc, 0, first, count, elements);
    }
    vBuf = gc->vertexArray.vertexCache;

    for (i = 0; i < count;) {
	if (elements) {
	    v = vBuf + elements[i++];
	} else {
	    v = vBuf + first + i++;
	}
	flags = v->flagBits;
	if ((flags & __GL_CLIP_ALL_MASK) == 0) {
	    (*gc->procs.validateVertex[(flags >> 14) & 3])(gc, v, (*gc->vertex.getFaceNeeds0)(gc) | 1);
	    (*gc->procs.renderPoint)(gc, v);
	}
    }
}

/* g_disableOgDrawVertexes / g_disableOgVarray (icdglob.h): write-only switches (nothing in the ICD reads them back) */

/* 690b0875 -- s_varray.c __glInitVertexArrayState (the ICD: two environment switches first, per-unit
** texture coordinate arrays, no drawVertexes table but drawArrays; batch size 36, 0x1000 cache steps) */
void __fastcall __glInitVertexArrayState(__GLcontext *gc)
{
    char *s;
    GLint unit;

    s = getenv("__GL_DISABLE_OG_VARRAY");
    if (s) {
	g_disableOgVarray = (GLubyte)atoi(s);
    }
    s = getenv("__GL_DISABLE_OG_DRAWVERTEXES");
    if (s) {
	g_disableOgDrawVertexes = (GLubyte)atoi(s);
    }

    /*
     * Set up vertex array default values
     */
    gc->vertexArray.vertex.size = 4;
    gc->vertexArray.vertex.type = GL_FLOAT;
    gc->vertexArray.vertex.fetch = (__GLproc)__glGLAPINop1;
    gc->vertexArray.normal.type = GL_FLOAT;
    gc->vertexArray.normal.fetch = (__GLproc)__glGLAPINop1;
    gc->vertexArray.color.size = 4;
    gc->vertexArray.color.type = GL_FLOAT;
    gc->vertexArray.color.fetch = (__GLproc)__glGLAPINop1;
    gc->vertexArray.index.type = GL_FLOAT;
    gc->vertexArray.index.fetch = (__GLproc)__glGLAPINop1;
    for (unit = 0; unit < 2; unit++) {
	gc->vertexArray.texture.size[unit] = 4;
	gc->vertexArray.texture.type[unit] = GL_FLOAT;
	gc->vertexArray.texture.fetch[unit] = (__GLproc)__glNop4b;
    }
    gc->vertexArray.edgeFlag.fetch = (__GLproc)__glGLAPINop1;

    gc->vertexArray.interleavedFormat = GL_NONE;

    gc->vertexArray.drawArrays[GL_POINTS] = (__GLproc)DrawVerticesPoints;
    gc->vertexArray.drawArrays[GL_LINES] = (__GLproc)__glDrawVertexes_Lines;
    gc->vertexArray.drawArrays[GL_LINE_LOOP] = (__GLproc)DrawVerticesLLoop;
    gc->vertexArray.drawArrays[GL_LINE_STRIP] = (__GLproc)__glDrawVertexes_Lstrip;
    gc->vertexArray.drawArrays[GL_TRIANGLES] = (__GLproc)DrawVerticesTriangles;
    gc->vertexArray.drawArrays[GL_TRIANGLE_STRIP] = (__GLproc)__glDrawVertexes_Tstrip;
    gc->vertexArray.drawArrays[GL_TRIANGLE_FAN] = (__GLproc)__glDrawVertexes_Tfan;
    gc->vertexArray.drawArrays[GL_QUADS] = (__GLproc)__glDrawVertexes_Quads;
    gc->vertexArray.drawArrays[GL_QUAD_STRIP] = (__GLproc)__glDrawVertexes_Qstrip;
    gc->vertexArray.drawArrays[GL_POLYGON] = (__GLproc)DrawVerticesPolygon;

    gc->vertexArray.batchSize = 36;           /* MAX_VERTEX_CACHE */
    gc->vertexArray.cacheGranule = 0x1000;  /* SI blockSize */
    (void) AllocateVbuf(gc, 0, 36 + 2);
}

/* 690b09d0 -- AllocateVbuf (s_varray.c): the vertex cache.  The ICD keeps the allocation and the
** 32-byte aligned base apart (the SI's "#if 0" alignment, done right) and points every cached
** vertex's color at its front color */
GLboolean __fastcall AllocateVbuf(__GLcontext *gc, int first, int count)
{
    int bufSize, currentBufSize;

    bufSize = count * sizeof(__GLvertexICD);
    currentBufSize = gc->vertexArray.cacheBytes;

    if (bufSize > currentBufSize) {
	int blockSize = gc->vertexArray.cacheGranule;
	GLuint vBuf = gc->vertexArray.cacheAlloc;
	__GLvertexICD *vx;

	/* smallest # of blocks that will contain <count> vertexes */
	bufSize = ((bufSize + (blockSize-1)) / blockSize) * blockSize;

	/* Align to 32byte cache line */
	bufSize += 32;

	/* allocate buffer */
	if (currentBufSize && vBuf) {
	    vBuf = __GL_IMPORT_REALLOC(gc, vBuf, (size_t)bufSize);
	} else {
	    vBuf = __GL_IMPORT_MALLOC(gc, (size_t)bufSize);
	}

	gc->vertexArray.cacheAlloc = vBuf;
	/* Align to 32byte cache line */
	vx = (__GLvertexICD *) ((vBuf & 0xFFFFFFE0UL) + 32);
	gc->vertexArray.cacheBytes = bufSize;
	gc->vertexArray.cacheBase = vx;

	/* Initialize color pointers */
	{
	    int i, n = bufSize / sizeof(__GLvertexICD);

	    for (i=0; i<n; i++, vx++) {
		vx->color = &vx->colors[__GL_FRONTFACE];
	    }
	}

	/* Update vcache pointer */
	gc->vaCacheBase = gc->vertexArray.cacheBase;
    }

    if (first) {
	gc->vertexArray.vertexCache = gc->vertexArray.cacheBase - first;
    } else {
	gc->vertexArray.vertexCache = gc->vertexArray.cacheBase;
    }

    return GL_TRUE;
}

/* 690b0ae0 -- __glim_ArrayElement: one vertex through the immediate-mode entry points, vertex last */
void __stdcall __glim_ArrayElement(GLint i)
{
    __GLcontext *gc = __gl_context;
    GLuint flags = gc->vertexArray.enableFlags;
    GLint unit;

    if (flags & __GL_VA_NORMAL_ENABLE) {
	__GL_VA_FETCH(gc->vertexArray.normal, i);
    }
    if (flags & __GL_VA_COLOR_ENABLE) {
	__GL_VA_FETCH(gc->vertexArray.color, i);
    }
    for (unit = 0; unit < gc->fx.numTMUs; unit++) {
	if (flags & __GL_VA_TEXCOORD_ENABLE(unit)) {
	    (*(__GLvaTexFetchProc) gc->vertexArray.texture.fetch[unit])(unit,
		(const GLvoid *) (gc->vertexArray.texture.pointer[unit] + i * gc->vertexArray.texture.stride[unit]));
	}
    }
    if (flags & __GL_VA_INDEX_ENABLE) {
	__GL_VA_FETCH(gc->vertexArray.index, i);
    }
    if (flags & __GL_VA_EDGEFLAG_ENABLE) {
	__GL_VA_FETCH(gc->vertexArray.edgeFlag, i);
    }
    if (flags & __GL_VA_FOGCOORD_ENABLE) {
	__GL_VA_FETCH(gc->vertexArray.fogCoord, i);
    }
    if (flags & __GL_VA_VERTEX_ENABLE) {
	__GL_VA_FETCH(gc->vertexArray.vertex, i);
    }
}

/* 690b0c20 -- __glim_DrawArrays */
void __stdcall __glim_DrawArrays(GLenum mode, GLint first, GLsizei count)
{
    GLuint lockBuffers;
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();

    /* check that first and count are positive */
    if (first < 0 || count < 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    if (mode > GL_POLYGON) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    if (count == 0) return;

    lockBuffers = gc->vertexArray.lockPrimMask & (1<<mode);

    if (lockBuffers) {
	__GL_LOCK_RENDER_BUFFERS(gc);
    }

    if (gc->vertexArray.flags & VERTARRAY_SLOWPATH_ICD) {
	int i, last = first+count;

	glBegin(mode);
	for (i=first; i<last; i++) {
	    glArrayElement(i);
	}
	glEnd();

    } else {
	if (gc->vertexArray.flags & VERTARRAY_NEEDS_FILL_ICD) {
	    if (gc->vertexArray.flags & VERTARRAY_LOCKED_ICD) {
		(*gc->vertexArray.fillLockedRange)(gc, gc->vertexArray.lockFirst,
				gc->vertexArray.lockFirst, gc->vertexArray.lockCount);
		gc->vertexArray.flags &= ~VERTARRAY_NEEDS_FILL_ICD;
	    }
	}

	(*gc->vertexArray.drawArrays[mode])(gc, first, count, NULL);
    }

    if (lockBuffers) {
	__GL_UNLOCK_RENDER_BUFFERS(gc);
    }
}

/* 690b0fc0 -- __glim_VertexPointer */
void __stdcall __glim_VertexPointer(GLint size, GLenum type,
			     GLsizei stride, const GLvoid *pointer)
{
    __GLproc fetch;
    GLint t;
    __GL_SETUP_NOT_IN_BEGIN();

    if (stride < 0 || size < 2 || size > 4) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    if (type < GL_BYTE || type > GL_DOUBLE) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    t = type - GL_BYTE;
    fetch = (*vertex_func_table)[0][t][size];
    if (!fetch) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    gc->vertexArray.vertex.fetch = fetch;
    fetch = (*vertex_func_table)[1][type - GL_BYTE][size];
    gc->vertexArray.vertex.type = type;
    gc->vertexArray.vertex.fetch2 = fetch;
    gc->vertexArray.vertex.pointer = pointer;
    gc->vertexArray.vertex.size = size;
    gc->vertexArray.vertex.stride = stride ? stride : stride_array[t][size];
    gc->vertexArray.vertex.userStride = stride;
    gc->vertexArray.formats = type_flag_table[t] | (gc->vertexArray.formats & 0xffffff00) | (size << 4);
    gc->vertexArray.vertex.count = 0;
    gc->vertexArray.interleavedFormat = 0;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
}

/* 690b1120 -- __glim_NormalPointer */
void __stdcall __glim_NormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    __GLproc fetch;
    GLuint fmt;
    __GL_SETUP_NOT_IN_BEGIN();

    if (stride < 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    if (type < GL_BYTE || type > GL_DOUBLE
	|| (fetch = (*normal_func_table)[0][type - GL_BYTE][3]) == 0) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    gc->vertexArray.normal.fetch = fetch;
    fetch = (*normal_func_table)[1][type - GL_BYTE][3];
    gc->vertexArray.normal.type = type;
    gc->vertexArray.normal.fetch2 = fetch;
    gc->vertexArray.normal.pointer = pointer;
    gc->vertexArray.normal.stride = stride ? stride : stride_array[type - GL_BYTE][3];
    gc->vertexArray.normal.userStride = stride;
    fmt = (type_flag_table[type - GL_BYTE] << 8) | (gc->vertexArray.formats & 0xfffff0ff);
    gc->vertexArray.normal.count = 0;
    gc->vertexArray.interleavedFormat = 0;
    gc->vertexArray.formats = fmt;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
}

/* 690b13d0 -- __glim_IndexPointer */
void __stdcall __glim_IndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    __GLproc fetch;
    GLuint fmt;
    __GL_SETUP_NOT_IN_BEGIN();

    if (stride < 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    if (type < GL_BYTE || type > GL_DOUBLE
	|| (fetch = (*index_func_table)[0][type - GL_BYTE][1]) == 0) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    gc->vertexArray.index.fetch = fetch;
    fetch = (*index_func_table)[1][type - GL_BYTE][1];
    gc->vertexArray.index.type = type;
    gc->vertexArray.index.fetch2 = fetch;
    gc->vertexArray.index.pointer = pointer;
    gc->vertexArray.index.stride = stride ? stride : stride_array[type - GL_BYTE][1];
    gc->vertexArray.index.userStride = stride;
    fmt = (type_flag_table[type - GL_BYTE] << 28) | (gc->vertexArray.formats & 0x0fffffff);
    gc->vertexArray.index.count = 0;
    gc->vertexArray.interleavedFormat = 0;
    gc->vertexArray.formats = fmt;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
}

/* 690b1f80 -- __glim_FogCoordPointerEXT: only GL_FLOAT / GL_DOUBLE; the format bits are ORed in
** without clearing the old ones (as the original) */
void __stdcall __glim_FogCoordPointerEXT(GLenum type, GLsizei stride,
			     const GLvoid *pointer)
{
    __GLproc fetch;
    GLuint fmt;
    __GL_SETUP_NOT_IN_BEGIN();

    if (type != GL_FLOAT && type != GL_DOUBLE) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    if (stride < 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    if ((fetch = (*fog_coord_func_table)[0][type - GL_BYTE][1]) == 0) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    gc->vertexArray.fogCoord.fetch = fetch;
    fetch = (*fog_coord_func_table)[1][type - GL_BYTE][1];
    gc->vertexArray.fogCoord.type = type;
    gc->vertexArray.fogCoord.fetch2 = fetch;
    gc->vertexArray.fogCoord.pointer = pointer;
    gc->vertexArray.fogCoord.stride = stride ? stride : stride_array[type - GL_BYTE][1];
    gc->vertexArray.fogCoord.userStride = stride;
    fmt = type_flag_table[type - GL_BYTE];
    gc->vertexArray.fogCoord.count = 0;
    gc->vertexArray.interleavedFormat = 0;
    gc->vertexArray.formats |= (fmt | 0x1f) << 20;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
}

#pragma optimize("w", on)
/* 690aeca0 -- CopyColor3ui (the ICD stores alpha first) */
void __fastcall CopyColor3ui(__GLcontext *gc, const GLuint *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].a = 1.0F;
    v->colors[__GL_FRONTFACE].r = __GL_UI_TO_FLOAT(cp[0]);
    v->colors[__GL_FRONTFACE].g = __GL_UI_TO_FLOAT(cp[1]);
    v->colors[__GL_FRONTFACE].b = __GL_UI_TO_FLOAT(cp[2]);
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}
#pragma optimize("", on)

#define VA_CACHE_DIRTY  0x2         /* vertexArray.flags: the vertex cache must be (re)filled (prepareCache) */
/* the 3dfx s_varray.c clip helpers: fetch the cached vertices, then clip unless they share an outcode */
extern void __fastcall ClipQuad(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c, __GLvertexICD *d);
extern void __stdcall __glim_VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

/* 690af920 -- __glDrawVertexes_Tstrip (3dfx S_VARRAY.C; SI DrawVerticesTStrip): triangle strip */
void __fastcall __glDrawVertexes_Tstrip(__GLcontext *gc, GLint first, GLsizei count, const GLint *indices)
{
    __GLvertexICD *base, *a, *b, *c;
    GLint i;

    if (count < 3) return;
    if (gc->vertexArray.flags & VA_CACHE_DIRTY) {
        (*gc->vertexArray.prepareCache)(gc, 0, first, count, indices);
    }
    base = gc->vertexArray.vertexCache;
    if (indices) {
        a = &base[indices[0]];
        c = &base[indices[1]];
    } else {
        a = &base[first];
        c = &base[first + 1];
    }
    a->flagBits |= __GL_VERTEX_EDGE_FLAG;
    c->flagBits |= __GL_VERTEX_EDGE_FLAG;
    for (i = 2; i < count; ) {
        if (i & 1) a = c; else b = c;
        c = indices ? &base[indices[i++]] : &base[first + i++];
        c->flagBits |= __GL_VERTEX_EDGE_FLAG;
        gc->line.notResetStipple = GL_FALSE;
        gc->vertex.provoking = c;
        if (((b->flagBits | c->flagBits | a->flagBits) & __GL_CLIP_ALL_MASK) == 0) {
            (*gc->procs.renderTriangle)(gc, a, b, c);
        } else if ((b->flagBits & c->flagBits & a->flagBits & __GL_CLIP_ALL_MASK) == 0) {
            __glDoClipTriangle(gc, a, b, c);
        }
    }
}

/* 690afa90 -- __glDrawVertexes_Tfan (3dfx S_VARRAY.C; SI DrawVerticesTFan): triangle fan */
void __fastcall __glDrawVertexes_Tfan(__GLcontext *gc, GLint first, GLsizei count, const GLint *indices)
{
    __GLvertexICD *base, *a, *b, *c;
    GLint i;

    if (count < 3) return;
    if (gc->vertexArray.flags & VA_CACHE_DIRTY) {
        (*gc->vertexArray.prepareCache)(gc, 0, first, count, indices);
    }
    base = gc->vertexArray.vertexCache;
    if (indices) {
        a = &base[indices[0]];
        c = &base[indices[1]];
    } else {
        a = &base[first];
        c = &base[first + 1];
    }
    a->flagBits |= __GL_VERTEX_EDGE_FLAG;
    c->flagBits |= __GL_VERTEX_EDGE_FLAG;
    for (i = 2; i < count; ) {
        b = c;
        c = indices ? &base[indices[i++]] : &base[first + i++];
        c->flagBits |= __GL_VERTEX_EDGE_FLAG;
        gc->line.notResetStipple = GL_FALSE;
        gc->vertex.provoking = c;
        if (((b->flagBits | c->flagBits | a->flagBits) & __GL_CLIP_ALL_MASK) == 0) {
            (*gc->procs.renderTriangle)(gc, a, b, c);
        } else if ((b->flagBits & c->flagBits & a->flagBits & __GL_CLIP_ALL_MASK) == 0) {
            __glDoClipTriangle(gc, a, b, c);
        }
    }
}

/* 690afc00 -- __glDrawVertexes_Qstrip (3dfx S_VARRAY.C; SI DrawVerticesQStrip): quad strip */
void __fastcall __glDrawVertexes_Qstrip(__GLcontext *gc, GLint first, GLsizei count, const GLint *indices)
{
    __GLvertexICD *base, *a, *b, *c, *d;
    GLint i;

    if (count < 4) return;
    count &= ~1;
    if (gc->vertexArray.flags & VA_CACHE_DIRTY) {
        (*gc->vertexArray.prepareCache)(gc, 0, first, count, indices);
    }
    base = gc->vertexArray.vertexCache;
    if (indices) {
        c = &base[indices[0]];
        d = &base[indices[1]];
    } else {
        c = &base[first];
        d = &base[first + 1];
    }
    c->flagBits |= __GL_VERTEX_EDGE_FLAG;
    d->flagBits |= __GL_VERTEX_EDGE_FLAG;
    for (i = 2; i < count; ) {
        a = c;
        b = d;
        if (indices) {
            c = &base[indices[i++]];
            d = &base[indices[i++]];
        } else {
            c = &base[first + i++];
            d = &base[first + i++];
        }
        c->flagBits |= __GL_VERTEX_EDGE_FLAG;
        d->flagBits |= __GL_VERTEX_EDGE_FLAG;
        gc->line.notResetStipple = GL_FALSE;
        gc->vertex.provoking = d;
        if (((a->flagBits | b->flagBits | d->flagBits | c->flagBits) & __GL_CLIP_ALL_MASK) == 0) {
            b->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
            (*gc->procs.renderTriangle)(gc, a, b, c);
            b->flagBits |= __GL_VERTEX_EDGE_FLAG;
            c->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
            (*gc->procs.renderTriangle)(gc, c, b, d);
            c->flagBits |= __GL_VERTEX_EDGE_FLAG;
        } else if ((a->flagBits & b->flagBits & d->flagBits & c->flagBits & __GL_CLIP_ALL_MASK) == 0) {
            ClipQuad(gc, a, b, d, c);
        }
    }
}

/* 690afe90 -- __glDrawVertexes_Lstrip (3dfx S_VARRAY.C; SI DrawVerticesLStrip): line strip */
void __fastcall __glDrawVertexes_Lstrip(__GLcontext *gc, GLint first, GLsizei count, const GLint *indices)
{
    __GLvertexICD *base, *a, *c;
    GLint i;

    if (count < 2) return;
    if (gc->vertexArray.flags & VA_CACHE_DIRTY) {
        (*gc->vertexArray.prepareCache)(gc, 0, first, count, indices);
    }
    base = gc->vertexArray.vertexCache;
    if (indices) c = &base[indices[0]]; else c = &base[first];
    (*gc->procs.validateVertex[(c->flagBits >> 14) & 3])(gc, c, (*gc->vertex.getFaceNeeds0)(gc));
    gc->line.notResetStipple = GL_FALSE;
    for (i = 1; i < count; ) {
        a = c;
        c = indices ? &base[indices[i++]] : &base[first + i++];
        gc->vertex.provoking = c;
        if (((a->flagBits | c->flagBits) & __GL_CLIP_ALL_MASK) == 0) {
            (*gc->procs.validateVertex[(c->flagBits >> 14) & 3])(gc, c, (*gc->vertex.getFaceNeeds0)(gc));
            (*gc->procs.renderLine)(gc, a, c);
        } else if ((a->flagBits & c->flagBits & __GL_CLIP_ALL_MASK) == 0) {
            ClipLine(gc, a, c);
        }
    }
}

/* 690b0290 -- __glDrawVertexes_Quads (3dfx S_VARRAY.C; SI DrawVerticesQuads): quads */
void __fastcall __glDrawVertexes_Quads(__GLcontext *gc, GLint first, GLsizei count, const GLint *indices)
{
    __GLvertexICD *base, *a, *b, *c, *d;
    GLuint edge;
    GLint i;

    if (count < 4) return;
    count &= ~3;
    if (gc->vertexArray.flags & VA_CACHE_DIRTY) {
        (*gc->vertexArray.prepareCache)(gc, 0, first, count, indices);
    }
    base = gc->vertexArray.vertexCache;
    for (i = 0; i < count; ) {
        if (indices) {
            a = &base[indices[i++]];
            b = &base[indices[i++]];
            c = &base[indices[i++]];
            d = &base[indices[i++]];
        } else {
            a = &base[first + i++];
            b = &base[first + i++];
            c = &base[first + i++];
            d = &base[first + i++];
        }
        gc->line.notResetStipple = GL_FALSE;
        gc->vertex.provoking = d;
        if (((a->flagBits | b->flagBits | c->flagBits | d->flagBits) & __GL_CLIP_ALL_MASK) == 0) {
            edge = b->flagBits & __GL_VERTEX_EDGE_FLAG;
            b->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
            (*gc->procs.renderTriangle)(gc, a, b, d);
            b->flagBits |= edge;
            edge = d->flagBits & __GL_VERTEX_EDGE_FLAG;
            d->flagBits &= ~__GL_VERTEX_EDGE_FLAG;
            (*gc->procs.renderTriangle)(gc, b, c, d);
            d->flagBits |= edge;
        } else if ((a->flagBits & b->flagBits & c->flagBits & d->flagBits & __GL_CLIP_ALL_MASK) == 0) {
            ClipQuad(gc, a, b, c, d);
        }
    }
}

/* 690b0610 -- __glDrawVertexes_Lines (3dfx S_VARRAY.C; SI DrawVerticesLines): lines */
void __fastcall __glDrawVertexes_Lines(__GLcontext *gc, GLint first, GLsizei count, const GLint *indices)
{
    __GLvertexICD *base, *a, *b;
    GLint i;

    if (count < 2) return;
    count &= ~1;
    if (gc->vertexArray.flags & VA_CACHE_DIRTY) {
        (*gc->vertexArray.prepareCache)(gc, 0, first, count, indices);
    }
    base = gc->vertexArray.vertexCache;
    for (i = 0; i < count; ) {
        if (indices) {
            a = &base[indices[i++]];
            b = &base[indices[i++]];
        } else {
            a = &base[first + i++];
            b = &base[first + i++];
        }
        gc->line.notResetStipple = GL_FALSE;
        gc->vertex.provoking = b;
        if (((a->flagBits | b->flagBits) & __GL_CLIP_ALL_MASK) == 0) {
            (*gc->procs.validateVertex[(a->flagBits >> 14) & 3])(gc, a, (*gc->vertex.getFaceNeeds0)(gc));
            (*gc->procs.validateVertex[(b->flagBits >> 14) & 3])(gc, b, (*gc->vertex.getFaceNeeds0)(gc));
            (*gc->procs.renderLine)(gc, a, b);
        } else if ((a->flagBits & b->flagBits & __GL_CLIP_ALL_MASK) == 0) {
            ClipLine(gc, a, b);
        }
    }
}

/* 690b1260: __glim_ColorPointer (stdcall; ecx / edx unused) */
void __stdcall __glim_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    __GLproc proc;
    __GL_SETUP_NOT_IN_BEGIN();

    if (stride < 0 || size < 3 || size > 4) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    if (type < GL_BYTE || type > GL_DOUBLE) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    proc = (*color_func_table)[0][type - GL_BYTE][size];
    if (proc == 0) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->vertexArray.color.fetch = proc;
    gc->vertexArray.color.fetch2 = (*color_func_table)[1][type - GL_BYTE][size];
    gc->vertexArray.color.pointer = pointer;
    gc->vertexArray.color.size = size;
    gc->vertexArray.color.type = type;
    gc->vertexArray.color.stride = stride ? stride : stride_array[type - GL_BYTE][size];
    gc->vertexArray.color.userStride = stride;
    gc->vertexArray.formats = (gc->vertexArray.formats & 0xfff00fff) |
        ((type_flag_table[type - GL_BYTE] | (size << 4)) << 12);
    gc->vertexArray.color.count = 0;
    gc->vertexArray.interleavedFormat = GL_NONE;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
}

/* 690b1510: __glim_TexCoordPointer (stdcall; ecx / edx unused) */
void __stdcall __glim_TexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    __GLproc proc;
    __GL_SETUP_NOT_IN_BEGIN();

    if (stride < 0 || size < 1 || size > 4) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    if (type < GL_BYTE || type > GL_DOUBLE) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    proc = (*tex_coord_func_table)[0][type - GL_BYTE][size];
    if (size > 3) {
        (*gc->procs.nonUnityQ)(gc);
    }
    if (proc == 0) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->vertexArray.texture.fetch[gc->fx.clientActiveTexture] = proc;
    gc->vertexArray.texture.fetch2[gc->fx.clientActiveTexture] =
        (*tex_coord_func_table)[1][type - GL_BYTE][size];
    gc->vertexArray.texture.pointer[gc->fx.clientActiveTexture] = pointer;
    gc->vertexArray.texture.size[gc->fx.clientActiveTexture] = size;
    gc->vertexArray.texture.type[gc->fx.clientActiveTexture] = type;
    gc->vertexArray.texture.stride[gc->fx.clientActiveTexture] =
        stride ? stride : stride_array[type - GL_BYTE][size];
    gc->vertexArray.texture.userStride[gc->fx.clientActiveTexture] = stride;
    gc->vertexArray.texFormats[gc->fx.clientActiveTexture] =
        (gc->vertexArray.texFormats[gc->fx.clientActiveTexture] & 0xf80fffff) |
        ((type_flag_table[type - GL_BYTE] | (size << 4)) << 20);
    gc->vertexArray.texture.count[gc->fx.clientActiveTexture] = 0;
    gc->vertexArray.interleavedFormat = GL_NONE;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
}

/* 690b1cb0 -- __glim_VertexPointerEXT: glVertexPointerEXT (EXT_vertex_array: the count is stored after the 1.1
** call) */
void __stdcall __glim_VertexPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (count < 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    __glim_VertexPointer(size, type, stride, pointer);
    gc->vertexArray.vertex.count = count;
}

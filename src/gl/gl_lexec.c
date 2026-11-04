/*
** gl_lexec.c -- original VA range 690dc000..690e0000: the display-list memory arena (s_memmgr.c),
** __glInitLUTCache (s_lcache.c) and the __glle_* functions that replay a compiled display-list
** record through the current dispatch table
** Then the rest of g_lexec.c (the __glle_* replays written by hand, several made by the W21_REC_* macros)
** and s_memmgr.c, in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"       /* the s_size.c size functions */
#include <glide.h>
#include "glsitypes.h"
#include "icdprotos.h"
#include "glshade.h"
#include "og.h"

/************************************************************************/
/* s_memmgr.c: the display-list arena (glsi.h __GLarena; __glArenaAlloc 690dd2e0 is in gl_lexec.c) */

#define MINBLOCKSIZE    (256 * 1024)

__GLarenaBlock * __fastcall NewBlock(__GLcontext *gc, GLuint size);
void __fastcall DeleteBlock(__GLcontext *gc, __GLarenaBlock *block);

/* 690dd200 -- __glNewArena */
__GLarena * __fastcall __glNewArena(__GLcontext *gc)
{
    __GLarena *arena;
    __GLarenaBlock *block;

    arena = (__GLarena *) __GL_MALLOC(gc, sizeof(__GLarena));
    if (arena == NULL) return NULL;
    arena->gc = gc;
    block = NewBlock(gc, MINBLOCKSIZE);
    if (block == NULL) {
	__GL_FREE(gc, arena);
	return NULL;
    }
    arena->firstBlock = arena->lastBlock = block;
    return arena;
}

/* 690dd240 -- NewBlock */
__GLarenaBlock * __fastcall NewBlock(__GLcontext *gc, GLuint size)
{
    __GLarenaBlock *block;

    block = (__GLarenaBlock *) __GL_MALLOC(gc, sizeof(__GLarenaBlock));
    if (block == NULL) return NULL;
    block->next = NULL;
    block->size = size;
    block->allocated = 0;
    block->data = (GLubyte *) __GL_MALLOC(gc, size);
    if (block->data == NULL) {
	__GL_FREE(gc, block);
	return NULL;
    }
    return block;
}

/* 690dd290 -- __glDeleteArena */
void __fastcall __glDeleteArena(__GLarena *arena)
{
    __GLcontext *gc;
    __GLarenaBlock *block, *next;

    gc = arena->gc;
    for (block = arena->firstBlock; block; block = next) {
	next = block->next;
	DeleteBlock(gc, block);
    }
    __GL_FREE(gc, arena);
}

/* 690dd2c0 -- DeleteBlock */
void __fastcall DeleteBlock(__GLcontext *gc, __GLarenaBlock *block)
{
    __GL_FREE(gc, block->data);
    __GL_FREE(gc, block);
}

/************************************************************************/
/* s_lcache.c */

/* 690dd380 -- __glInitLUTCache */
void __fastcall __glInitLUTCache(__GLcontext *gc)
{
    __GLspecLUTCache *lutCache;

    lutCache = gc->light.lutCache = (__GLspecLUTCache *)
	    __GL_MALLOC(gc, sizeof(__GLspecLUTCache));
    lutCache->nentries = 0;
    lutCache->allocatedSize = 1;
}

/************************************************************************/

/* the dispatch entries are APIENTRY; one proc type per argument shape (GLint stands for any
** 32-bit integer argument: GLenum, GLuint, GLbitfield) */
typedef void (APIENTRY *__GLvoidProc)(void);
typedef void (APIENTRY *__GLvectorProc)(const void *v);
typedef void (APIENTRY *__GLintProc)(GLint);
typedef void (APIENTRY *__GLfloatProc)(GLfloat);
typedef void (APIENTRY *__GLdoubleProc)(GLdouble);
typedef void (APIENTRY *__GLint2Proc)(GLint, GLint);
typedef void (APIENTRY *__GLintFloatProc)(GLint, GLfloat);
typedef void (APIENTRY *__GLfloat2Proc)(GLfloat, GLfloat);
typedef void (APIENTRY *__GLintVectorProc)(GLint, const void *v);

#define __GL_DISPATCH(name, type) \
    ((type) __glCurrentGC()->currentDispatchState.fn[__GL_DISP_##name])

/* the records (g_lcomp.h __gllc_*_Rec) of the entries with more than one argument */
struct __gllc_ColorMaterial_Rec {
    GLenum      face;
    GLenum      mode;
};
struct __gllc_Fogf_Rec {
    GLenum      pname;
    GLfloat     param;
};
struct __gllc_Fogi_Rec {
    GLenum      pname;
    GLint       param;
};
struct __gllc_Fogiv_Rec {
    GLenum      pname;
    /*  GLint params[] follow */
};
struct __gllc_Hint_Rec {
    GLenum      target;
    GLenum      mode;
};
struct __gllc_LightModelfv_Rec {
    GLenum      pname;
    /*  GLfloat params[] follow */
};
struct __gllc_PolygonMode_Rec {
    GLenum      face;
    GLenum      mode;
};
struct __gllc_Accum_Rec {
    GLenum      op;
    GLfloat     value;
};
struct __gllc_EvalPoint2_Rec {
    GLint       i;
    GLint       j;
};
struct __gllc_AlphaFunc_Rec {
    GLenum      func;
    GLclampf    ref;
};
struct __gllc_BlendFunc_Rec {
    GLenum      sfactor;
    GLenum      dfactor;
};
struct __gllc_PixelZoom_Rec {
    GLfloat     xfactor;
    GLfloat     yfactor;
};
struct __gllc_PixelTransferf_Rec {
    GLenum      pname;
    GLfloat     param;
};
struct __gllc_PixelTransferi_Rec {
    GLenum      pname;
    GLint       param;
};
struct __gllc_PolygonOffset_Rec {
    GLfloat     factor;
    GLfloat     units;
};
struct __gllc_BindTexture_Rec {
    GLenum      target;
    GLuint      texture;
};
struct __gllc_IndexFuncSGI_Rec {
    GLenum      func;
    GLclampf    ref;
};
struct __gllc_IndexMaterialSGI_Rec {
    GLenum      face;
    GLenum      mode;
};
struct __gllc_MultiTexCoord1fARB_Rec {
    GLenum      target;
    GLfloat     s;
};
struct __gllc_MultiTexCoord1fvARB_Rec {
    GLenum      target;
    GLfloat     v[1];
};
struct __gllc_MultiTexCoord1iARB_Rec {
    GLenum      target;
    GLint       s;
};
struct __gllc_MultiTexCoord1ivARB_Rec {
    GLenum      target;
    GLint       v[1];
};
struct __gllc_MultiTexCoord1svARB_Rec {
    GLenum      target;
    GLshort     v[1];
};
struct __gllc_MultiTexCoord2fvARB_Rec {
    GLenum      target;
    GLfloat     v[2];
};
struct __gllc_MultiTexCoord2ivARB_Rec {
    GLenum      target;
    GLint       v[2];
};
struct __gllc_MultiTexCoord2svARB_Rec {
    GLenum      target;
    GLshort     v[2];
};
struct __gllc_MultiTexCoord3fvARB_Rec {
    GLenum      target;
    GLfloat     v[3];
};
struct __gllc_MultiTexCoord3ivARB_Rec {
    GLenum      target;
    GLint       v[3];
};
struct __gllc_MultiTexCoord3svARB_Rec {
    GLenum      target;
    GLshort     v[3];
};
struct __gllc_MultiTexCoord4fvARB_Rec {
    GLenum      target;
    GLfloat     v[4];
};
struct __gllc_MultiTexCoord4ivARB_Rec {
    GLenum      target;
    GLint       v[4];
};
struct __gllc_MultiTexCoord4svARB_Rec {
    GLenum      target;
    GLshort     v[4];
};
struct __gllc_PointParameterfEXT_Rec {
    GLenum      pname;
    GLfloat     param;
};
struct __gllc_PointParameterfvEXT_Rec {
    GLenum      pname;
    /*  GLfloat params[] follow */
};

/* 690dd610 -- __glle_ListBase */
const GLubyte * __fastcall __glle_ListBase(const GLubyte *PC)
{
    (*__GL_DISPATCH(ListBase, __GLintProc))(*(const GLuint *) PC);
    return PC + sizeof(GLuint);
}

/* 690dd630 -- __glle_Color3bv */
const GLubyte * __fastcall __glle_Color3bv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color3bv, __GLvectorProc))(PC);
    return PC + __GL_PAD(sizeof(GLbyte[3]));
}

/* 690dd650 -- __glle_Color3dv */
const GLubyte * __fastcall __glle_Color3dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color3dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[3]);
}

/* 690dd670 -- __glle_Color3fv */
const GLubyte * __fastcall __glle_Color3fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color3fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[3]);
}

/* 690dd690 -- __glle_Color3iv */
const GLubyte * __fastcall __glle_Color3iv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color3iv, __GLvectorProc))(PC);
    return PC + sizeof(GLint[3]);
}

/* 690dd6b0 -- __glle_Color3sv */
const GLubyte * __fastcall __glle_Color3sv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color3sv, __GLvectorProc))(PC);
    return PC + __GL_PAD(sizeof(GLshort[3]));
}

/* 690dd6d0 -- __glle_Color3ubv */
const GLubyte * __fastcall __glle_Color3ubv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color3ubv, __GLvectorProc))(PC);
    return PC + __GL_PAD(sizeof(GLubyte[3]));
}

/* 690dd6f0 -- __glle_Color3uiv */
const GLubyte * __fastcall __glle_Color3uiv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color3uiv, __GLvectorProc))(PC);
    return PC + sizeof(GLuint[3]);
}

/* 690dd710 -- __glle_Color3usv */
const GLubyte * __fastcall __glle_Color3usv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color3usv, __GLvectorProc))(PC);
    return PC + __GL_PAD(sizeof(GLushort[3]));
}

/* 690dd730 -- __glle_Color4bv */
const GLubyte * __fastcall __glle_Color4bv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color4bv, __GLvectorProc))(PC);
    return PC + sizeof(GLbyte[4]);
}

/* 690dd750 -- __glle_Color4dv */
const GLubyte * __fastcall __glle_Color4dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color4dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[4]);
}

/* 690dd770 -- __glle_Color4fv */
const GLubyte * __fastcall __glle_Color4fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color4fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[4]);
}

/* 690dd790 -- __glle_Color4iv */
const GLubyte * __fastcall __glle_Color4iv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color4iv, __GLvectorProc))(PC);
    return PC + sizeof(GLint[4]);
}

/* 690dd7b0 -- __glle_Color4sv */
const GLubyte * __fastcall __glle_Color4sv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color4sv, __GLvectorProc))(PC);
    return PC + sizeof(GLshort[4]);
}

/* 690dd7d0 -- __glle_Color4ubv */
const GLubyte * __fastcall __glle_Color4ubv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color4ubv, __GLvectorProc))(PC);
    return PC + sizeof(GLubyte[4]);
}

/* 690dd7f0 -- __glle_Color4uiv */
const GLubyte * __fastcall __glle_Color4uiv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color4uiv, __GLvectorProc))(PC);
    return PC + sizeof(GLuint[4]);
}

/* 690dd810 -- __glle_Color4usv */
const GLubyte * __fastcall __glle_Color4usv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Color4usv, __GLvectorProc))(PC);
    return PC + sizeof(GLushort[4]);
}

/* 690dd830 -- __glle_EdgeFlagv */
const GLubyte * __fastcall __glle_EdgeFlagv(const GLubyte *PC)
{
    (*__GL_DISPATCH(EdgeFlagv, __GLvectorProc))(PC);
    return PC + __GL_PAD(sizeof(GLboolean[1]));
}

/* 690dd850 -- __glle_End */
const GLubyte * __fastcall __glle_End(const GLubyte *PC)
{
    (*__GL_DISPATCH(End, __GLvoidProc))();
    return PC;
}

/* 690dd870 -- __glle_Indexdv */
const GLubyte * __fastcall __glle_Indexdv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Indexdv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble);
}

/* 690dd890 -- __glle_Indexfv */
const GLubyte * __fastcall __glle_Indexfv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Indexfv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat);
}

/* 690dd8b0 -- __glle_Indexiv */
const GLubyte * __fastcall __glle_Indexiv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Indexiv, __GLvectorProc))(PC);
    return PC + sizeof(GLint);
}

/* 690dd8d0 -- __glle_Indexsv */
const GLubyte * __fastcall __glle_Indexsv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Indexsv, __GLvectorProc))(PC);
    return PC + __GL_PAD(sizeof(GLshort[1]));
}

/* 690dd8f0 -- __glle_Normal3bv */
const GLubyte * __fastcall __glle_Normal3bv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Normal3bv, __GLvectorProc))(PC);
    return PC + __GL_PAD(sizeof(GLbyte[3]));
}

/* 690dd910 -- __glle_Normal3dv */
const GLubyte * __fastcall __glle_Normal3dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Normal3dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[3]);
}

/* 690dd930 -- __glle_Normal3fv */
const GLubyte * __fastcall __glle_Normal3fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Normal3fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[3]);
}

/* 690dd950 -- __glle_Normal3iv */
const GLubyte * __fastcall __glle_Normal3iv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Normal3iv, __GLvectorProc))(PC);
    return PC + sizeof(GLint[3]);
}

/* 690dd970 -- __glle_Normal3sv */
const GLubyte * __fastcall __glle_Normal3sv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Normal3sv, __GLvectorProc))(PC);
    return PC + __GL_PAD(sizeof(GLshort[3]));
}

/* 690dd990 -- __glle_RasterPos2dv */
const GLubyte * __fastcall __glle_RasterPos2dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(RasterPos2dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[2]);
}

/* 690dd9b0 -- __glle_RasterPos2fv */
const GLubyte * __fastcall __glle_RasterPos2fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(RasterPos2fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[2]);
}

/* 690dd9d0 -- __glle_RasterPos2iv */
const GLubyte * __fastcall __glle_RasterPos2iv(const GLubyte *PC)
{
    (*__GL_DISPATCH(RasterPos2iv, __GLvectorProc))(PC);
    return PC + sizeof(GLint[2]);
}

/* 690dd9f0 -- __glle_RasterPos2sv */
const GLubyte * __fastcall __glle_RasterPos2sv(const GLubyte *PC)
{
    (*__GL_DISPATCH(RasterPos2sv, __GLvectorProc))(PC);
    return PC + sizeof(GLshort[2]);
}

/* 690dda10 -- __glle_RasterPos3dv */
const GLubyte * __fastcall __glle_RasterPos3dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(RasterPos3dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[3]);
}

/* 690dda30 -- __glle_RasterPos3fv */
const GLubyte * __fastcall __glle_RasterPos3fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(RasterPos3fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[3]);
}

/* 690dda50 -- __glle_RasterPos3iv */
const GLubyte * __fastcall __glle_RasterPos3iv(const GLubyte *PC)
{
    (*__GL_DISPATCH(RasterPos3iv, __GLvectorProc))(PC);
    return PC + sizeof(GLint[3]);
}

/* 690dda70 -- __glle_RasterPos3sv */
const GLubyte * __fastcall __glle_RasterPos3sv(const GLubyte *PC)
{
    (*__GL_DISPATCH(RasterPos3sv, __GLvectorProc))(PC);
    return PC + __GL_PAD(sizeof(GLshort[3]));
}

/* 690dda90 -- __glle_RasterPos4dv */
const GLubyte * __fastcall __glle_RasterPos4dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(RasterPos4dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[4]);
}

/* 690ddab0 -- __glle_RasterPos4fv */
const GLubyte * __fastcall __glle_RasterPos4fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(RasterPos4fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[4]);
}

/* 690ddad0 -- __glle_RasterPos4iv */
const GLubyte * __fastcall __glle_RasterPos4iv(const GLubyte *PC)
{
    (*__GL_DISPATCH(RasterPos4iv, __GLvectorProc))(PC);
    return PC + sizeof(GLint[4]);
}

/* 690ddaf0 -- __glle_RasterPos4sv */
const GLubyte * __fastcall __glle_RasterPos4sv(const GLubyte *PC)
{
    (*__GL_DISPATCH(RasterPos4sv, __GLvectorProc))(PC);
    return PC + sizeof(GLshort[4]);
}

/* 690ddbd0 -- __glle_TexCoord1dv */
const GLubyte * __fastcall __glle_TexCoord1dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord1dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble);
}

/* 690ddbf0 -- __glle_TexCoord1fv */
const GLubyte * __fastcall __glle_TexCoord1fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord1fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat);
}

/* 690ddc10 -- __glle_TexCoord1iv */
const GLubyte * __fastcall __glle_TexCoord1iv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord1iv, __GLvectorProc))(PC);
    return PC + sizeof(GLint);
}

/* 690ddc30 -- __glle_TexCoord1sv */
const GLubyte * __fastcall __glle_TexCoord1sv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord1sv, __GLvectorProc))(PC);
    return PC + __GL_PAD(sizeof(GLshort[1]));
}

/* 690ddc50 -- __glle_TexCoord2dv */
const GLubyte * __fastcall __glle_TexCoord2dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord2dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[2]);
}

/* 690ddc70 -- __glle_TexCoord2fv */
const GLubyte * __fastcall __glle_TexCoord2fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord2fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[2]);
}

/* 690ddc90 -- __glle_TexCoord2iv */
const GLubyte * __fastcall __glle_TexCoord2iv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord2iv, __GLvectorProc))(PC);
    return PC + sizeof(GLint[2]);
}

/* 690ddcb0 -- __glle_TexCoord2sv */
const GLubyte * __fastcall __glle_TexCoord2sv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord2sv, __GLvectorProc))(PC);
    return PC + sizeof(GLshort[2]);
}

/* 690ddcd0 -- __glle_TexCoord3dv */
const GLubyte * __fastcall __glle_TexCoord3dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord3dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[3]);
}

/* 690ddcf0 -- __glle_TexCoord3fv */
const GLubyte * __fastcall __glle_TexCoord3fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord3fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[3]);
}

/* 690ddd10 -- __glle_TexCoord3iv */
const GLubyte * __fastcall __glle_TexCoord3iv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord3iv, __GLvectorProc))(PC);
    return PC + sizeof(GLint[3]);
}

/* 690ddd30 -- __glle_TexCoord3sv */
const GLubyte * __fastcall __glle_TexCoord3sv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord3sv, __GLvectorProc))(PC);
    return PC + __GL_PAD(sizeof(GLshort[3]));
}

/* 690ddd50 -- __glle_TexCoord4dv */
const GLubyte * __fastcall __glle_TexCoord4dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord4dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[4]);
}

/* 690ddd70 -- __glle_TexCoord4fv */
const GLubyte * __fastcall __glle_TexCoord4fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord4fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[4]);
}

/* 690ddd90 -- __glle_TexCoord4iv */
const GLubyte * __fastcall __glle_TexCoord4iv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord4iv, __GLvectorProc))(PC);
    return PC + sizeof(GLint[4]);
}

/* 690dddb0 -- __glle_TexCoord4sv */
const GLubyte * __fastcall __glle_TexCoord4sv(const GLubyte *PC)
{
    (*__GL_DISPATCH(TexCoord4sv, __GLvectorProc))(PC);
    return PC + sizeof(GLshort[4]);
}

/* 690dddd0 -- __glle_Vertex2dv */
const GLubyte * __fastcall __glle_Vertex2dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Vertex2dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[2]);
}

/* 690dddf0 -- __glle_Vertex2fv */
const GLubyte * __fastcall __glle_Vertex2fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Vertex2fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[2]);
}

/* 690dde10 -- __glle_Vertex2iv */
const GLubyte * __fastcall __glle_Vertex2iv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Vertex2iv, __GLvectorProc))(PC);
    return PC + sizeof(GLint[2]);
}

/* 690dde30 -- __glle_Vertex2sv */
const GLubyte * __fastcall __glle_Vertex2sv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Vertex2sv, __GLvectorProc))(PC);
    return PC + sizeof(GLshort[2]);
}

/* 690dde50 -- __glle_Vertex3dv */
const GLubyte * __fastcall __glle_Vertex3dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Vertex3dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[3]);
}

/* 690dde70 -- __glle_Vertex3fv */
const GLubyte * __fastcall __glle_Vertex3fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Vertex3fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[3]);
}

/* 690dde90 -- __glle_Vertex3iv */
const GLubyte * __fastcall __glle_Vertex3iv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Vertex3iv, __GLvectorProc))(PC);
    return PC + sizeof(GLint[3]);
}

/* 690ddeb0 -- __glle_Vertex3sv */
const GLubyte * __fastcall __glle_Vertex3sv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Vertex3sv, __GLvectorProc))(PC);
    return PC + __GL_PAD(sizeof(GLshort[3]));
}

/* 690dded0 -- __glle_Vertex4dv */
const GLubyte * __fastcall __glle_Vertex4dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Vertex4dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[4]);
}

/* 690ddef0 -- __glle_Vertex4fv */
const GLubyte * __fastcall __glle_Vertex4fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Vertex4fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[4]);
}

/* 690ddf10 -- __glle_Vertex4iv */
const GLubyte * __fastcall __glle_Vertex4iv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Vertex4iv, __GLvectorProc))(PC);
    return PC + sizeof(GLint[4]);
}

/* 690ddf30 -- __glle_Vertex4sv */
const GLubyte * __fastcall __glle_Vertex4sv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Vertex4sv, __GLvectorProc))(PC);
    return PC + sizeof(GLshort[4]);
}

/* 690ddf80 -- __glle_ColorMaterial */
const GLubyte * __fastcall __glle_ColorMaterial(const GLubyte *PC)
{
    const struct __gllc_ColorMaterial_Rec *data;

    data = (const struct __gllc_ColorMaterial_Rec *) PC;
    (*__GL_DISPATCH(ColorMaterial, __GLint2Proc))(data->face, data->mode);
    return PC + sizeof(struct __gllc_ColorMaterial_Rec);
}

/* 690ddfb0 -- __glle_CullFace */
const GLubyte * __fastcall __glle_CullFace(const GLubyte *PC)
{
    (*__GL_DISPATCH(CullFace, __GLintProc))(*(const GLenum *) PC);
    return PC + sizeof(GLenum);
}

/* 690ddfd0 -- __glle_Fogf */
const GLubyte * __fastcall __glle_Fogf(const GLubyte *PC)
{
    const struct __gllc_Fogf_Rec *data;

    data = (const struct __gllc_Fogf_Rec *) PC;
    (*__GL_DISPATCH(Fogf, __GLintFloatProc))(data->pname, data->param);
    return PC + sizeof(struct __gllc_Fogf_Rec);
}

/* 690de030 -- __glle_Fogi */
const GLubyte * __fastcall __glle_Fogi(const GLubyte *PC)
{
    const struct __gllc_Fogi_Rec *data;

    data = (const struct __gllc_Fogi_Rec *) PC;
    (*__GL_DISPATCH(Fogi, __GLint2Proc))(data->pname, data->param);
    return PC + sizeof(struct __gllc_Fogi_Rec);
}

/* 690de060 -- __glle_Fogiv */
const GLubyte * __fastcall __glle_Fogiv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    const struct __gllc_Fogiv_Rec *data;

    data = (const struct __gllc_Fogiv_Rec *) PC;
    (*__GL_DISPATCH(Fogiv, __GLintVectorProc))(data->pname,
	(const GLint *) (PC + sizeof(struct __gllc_Fogiv_Rec)));
    arraySize = __glFogiv_size(data->pname) * 4;
    size = (GLuint) sizeof(struct __gllc_Fogiv_Rec) + arraySize;
    return PC + size;
}

/* 690de090 -- __glle_FrontFace */
const GLubyte * __fastcall __glle_FrontFace(const GLubyte *PC)
{
    (*__GL_DISPATCH(FrontFace, __GLintProc))(*(const GLenum *) PC);
    return PC + sizeof(GLenum);
}

/* 690de0b0 -- __glle_Hint */
const GLubyte * __fastcall __glle_Hint(const GLubyte *PC)
{
    const struct __gllc_Hint_Rec *data;

    data = (const struct __gllc_Hint_Rec *) PC;
    (*__GL_DISPATCH(Hint, __GLint2Proc))(data->target, data->mode);
    return PC + sizeof(struct __gllc_Hint_Rec);
}

/* 690de140 -- __glle_LightModelfv */
const GLubyte * __fastcall __glle_LightModelfv(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    const struct __gllc_LightModelfv_Rec *data;

    data = (const struct __gllc_LightModelfv_Rec *) PC;
    (*__GL_DISPATCH(LightModelfv, __GLintVectorProc))(data->pname,
	(const GLfloat *) (PC + sizeof(struct __gllc_LightModelfv_Rec)));
    arraySize = __glLightModelfv_size(data->pname) * 4;
    size = (GLuint) sizeof(struct __gllc_LightModelfv_Rec) + arraySize;
    return PC + size;
}

/* 690de1d0 -- __glle_LineWidth */
const GLubyte * __fastcall __glle_LineWidth(const GLubyte *PC)
{
    (*__GL_DISPATCH(LineWidth, __GLfloatProc))(*(const GLfloat *) PC);
    return PC + sizeof(GLfloat);
}

/* 690de250 -- __glle_PointSize */
const GLubyte * __fastcall __glle_PointSize(const GLubyte *PC)
{
    (*__GL_DISPATCH(PointSize, __GLfloatProc))(*(const GLfloat *) PC);
    return PC + sizeof(GLfloat);
}

/* 690de270 -- __glle_PolygonMode */
const GLubyte * __fastcall __glle_PolygonMode(const GLubyte *PC)
{
    const struct __gllc_PolygonMode_Rec *data;

    data = (const struct __gllc_PolygonMode_Rec *) PC;
    (*__GL_DISPATCH(PolygonMode, __GLint2Proc))(data->face, data->mode);
    return PC + sizeof(struct __gllc_PolygonMode_Rec);
}

/* 690de2d0 -- __glle_ShadeModel */
const GLubyte * __fastcall __glle_ShadeModel(const GLubyte *PC)
{
    (*__GL_DISPATCH(ShadeModel, __GLintProc))(*(const GLenum *) PC);
    return PC + sizeof(GLenum);
}

/* 690de440 -- __glle_InitNames */
const GLubyte * __fastcall __glle_InitNames(const GLubyte *PC)
{
    (*__GL_DISPATCH(InitNames, __GLvoidProc))();
    return PC;
}

/* 690de460 -- __glle_LoadName */
const GLubyte * __fastcall __glle_LoadName(const GLubyte *PC)
{
    (*__GL_DISPATCH(LoadName, __GLintProc))(*(const GLuint *) PC);
    return PC + sizeof(GLuint);
}

/* 690de480 -- __glle_PassThrough */
const GLubyte * __fastcall __glle_PassThrough(const GLubyte *PC)
{
    (*__GL_DISPATCH(PassThrough, __GLfloatProc))(*(const GLfloat *) PC);
    return PC + sizeof(GLfloat);
}

/* 690de4a0 -- __glle_PopName */
const GLubyte * __fastcall __glle_PopName(const GLubyte *PC)
{
    (*__GL_DISPATCH(PopName, __GLvoidProc))();
    return PC;
}

/* 690de4c0 -- __glle_PushName */
const GLubyte * __fastcall __glle_PushName(const GLubyte *PC)
{
    (*__GL_DISPATCH(PushName, __GLintProc))(*(const GLuint *) PC);
    return PC + sizeof(GLuint);
}

/* 690de4e0 -- __glle_DrawBuffer */
const GLubyte * __fastcall __glle_DrawBuffer(const GLubyte *PC)
{
    (*__GL_DISPATCH(DrawBuffer, __GLintProc))(*(const GLenum *) PC);
    return PC + sizeof(GLenum);
}

/* 690de500 -- __glle_Clear */
const GLubyte * __fastcall __glle_Clear(const GLubyte *PC)
{
    (*__GL_DISPATCH(Clear, __GLintProc))(*(const GLbitfield *) PC);
    return PC + sizeof(GLbitfield);
}

/* 690de550 -- __glle_ClearIndex */
const GLubyte * __fastcall __glle_ClearIndex(const GLubyte *PC)
{
    (*__GL_DISPATCH(ClearIndex, __GLfloatProc))(*(const GLfloat *) PC);
    return PC + sizeof(GLfloat);
}

/* 690de5a0 -- __glle_ClearStencil */
const GLubyte * __fastcall __glle_ClearStencil(const GLubyte *PC)
{
    (*__GL_DISPATCH(ClearStencil, __GLintProc))(*(const GLint *) PC);
    return PC + sizeof(GLint);
}

/* 690de5c0 -- __glle_ClearDepth */
const GLubyte * __fastcall __glle_ClearDepth(const GLubyte *PC)
{
    (*__GL_DISPATCH(ClearDepth, __GLdoubleProc))(*(const GLclampd *) PC);
    return PC + sizeof(GLclampd);
}

/* 690de5f0 -- __glle_StencilMask */
const GLubyte * __fastcall __glle_StencilMask(const GLubyte *PC)
{
    (*__GL_DISPATCH(StencilMask, __GLintProc))(*(const GLuint *) PC);
    return PC + sizeof(GLuint);
}

/* 690de660 -- __glle_IndexMask */
const GLubyte * __fastcall __glle_IndexMask(const GLubyte *PC)
{
    (*__GL_DISPATCH(IndexMask, __GLintProc))(*(const GLuint *) PC);
    return PC + sizeof(GLuint);
}

/* 690de680 -- __glle_Accum */
const GLubyte * __fastcall __glle_Accum(const GLubyte *PC)
{
    const struct __gllc_Accum_Rec *data;

    data = (const struct __gllc_Accum_Rec *) PC;
    (*__GL_DISPATCH(Accum, __GLintFloatProc))(data->op, data->value);
    return PC + sizeof(struct __gllc_Accum_Rec);
}

/* 690de6b0 -- __glle_PopAttrib */
const GLubyte * __fastcall __glle_PopAttrib(const GLubyte *PC)
{
    (*__GL_DISPATCH(PopAttrib, __GLvoidProc))();
    return PC;
}

/* 690de6d0 -- __glle_PushAttrib */
const GLubyte * __fastcall __glle_PushAttrib(const GLubyte *PC)
{
    (*__GL_DISPATCH(PushAttrib, __GLintProc))(*(const GLbitfield *) PC);
    return PC + sizeof(GLbitfield);
}

/* 690de7e0 -- __glle_EvalCoord1dv */
const GLubyte * __fastcall __glle_EvalCoord1dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(EvalCoord1dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble);
}

/* 690de800 -- __glle_EvalCoord1fv */
const GLubyte * __fastcall __glle_EvalCoord1fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(EvalCoord1fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat);
}

/* 690de820 -- __glle_EvalCoord2dv */
const GLubyte * __fastcall __glle_EvalCoord2dv(const GLubyte *PC)
{
    (*__GL_DISPATCH(EvalCoord2dv, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[2]);
}

/* 690de840 -- __glle_EvalCoord2fv */
const GLubyte * __fastcall __glle_EvalCoord2fv(const GLubyte *PC)
{
    (*__GL_DISPATCH(EvalCoord2fv, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[2]);
}

/* 690de890 -- __glle_EvalPoint1 */
const GLubyte * __fastcall __glle_EvalPoint1(const GLubyte *PC)
{
    (*__GL_DISPATCH(EvalPoint1, __GLintProc))(*(const GLint *) PC);
    return PC + sizeof(GLint);
}

/* 690de8e0 -- __glle_EvalPoint2 */
const GLubyte * __fastcall __glle_EvalPoint2(const GLubyte *PC)
{
    const struct __gllc_EvalPoint2_Rec *data;

    data = (const struct __gllc_EvalPoint2_Rec *) PC;
    (*__GL_DISPATCH(EvalPoint2, __GLint2Proc))(data->i, data->j);
    return PC + sizeof(struct __gllc_EvalPoint2_Rec);
}

/* 690de910 -- __glle_AlphaFunc */
const GLubyte * __fastcall __glle_AlphaFunc(const GLubyte *PC)
{
    const struct __gllc_AlphaFunc_Rec *data;

    data = (const struct __gllc_AlphaFunc_Rec *) PC;
    (*__GL_DISPATCH(AlphaFunc, __GLintFloatProc))(data->func, data->ref);
    return PC + sizeof(struct __gllc_AlphaFunc_Rec);
}

/* 690de940 -- __glle_BlendFunc */
const GLubyte * __fastcall __glle_BlendFunc(const GLubyte *PC)
{
    const struct __gllc_BlendFunc_Rec *data;

    data = (const struct __gllc_BlendFunc_Rec *) PC;
    (*__GL_DISPATCH(BlendFunc, __GLint2Proc))(data->sfactor, data->dfactor);
    return PC + sizeof(struct __gllc_BlendFunc_Rec);
}

/* 690de970 -- __glle_LogicOp */
const GLubyte * __fastcall __glle_LogicOp(const GLubyte *PC)
{
    (*__GL_DISPATCH(LogicOp, __GLintProc))(*(const GLenum *) PC);
    return PC + sizeof(GLenum);
}

/* 690de9f0 -- __glle_DepthFunc */
const GLubyte * __fastcall __glle_DepthFunc(const GLubyte *PC)
{
    (*__GL_DISPATCH(DepthFunc, __GLintProc))(*(const GLenum *) PC);
    return PC + sizeof(GLenum);
}

/* 690dea10 -- __glle_PixelZoom */
const GLubyte * __fastcall __glle_PixelZoom(const GLubyte *PC)
{
    const struct __gllc_PixelZoom_Rec *data;

    data = (const struct __gllc_PixelZoom_Rec *) PC;
    (*__GL_DISPATCH(PixelZoom, __GLfloat2Proc))(data->xfactor, data->yfactor);
    return PC + sizeof(struct __gllc_PixelZoom_Rec);
}

/* 690dea40 -- __glle_PixelTransferf */
const GLubyte * __fastcall __glle_PixelTransferf(const GLubyte *PC)
{
    const struct __gllc_PixelTransferf_Rec *data;

    data = (const struct __gllc_PixelTransferf_Rec *) PC;
    (*__GL_DISPATCH(PixelTransferf, __GLintFloatProc))(data->pname, data->param);
    return PC + sizeof(struct __gllc_PixelTransferf_Rec);
}

/* 690dea70 -- __glle_PixelTransferi */
const GLubyte * __fastcall __glle_PixelTransferi(const GLubyte *PC)
{
    const struct __gllc_PixelTransferi_Rec *data;

    data = (const struct __gllc_PixelTransferi_Rec *) PC;
    (*__GL_DISPATCH(PixelTransferi, __GLint2Proc))(data->pname, data->param);
    return PC + sizeof(struct __gllc_PixelTransferi_Rec);
}

/* 690deb40 -- __glle_ReadBuffer */
const GLubyte * __fastcall __glle_ReadBuffer(const GLubyte *PC)
{
    (*__GL_DISPATCH(ReadBuffer, __GLintProc))(*(const GLenum *) PC);
    return PC + sizeof(GLenum);
}

/* 690dec10 -- __glle_LoadIdentity */
const GLubyte * __fastcall __glle_LoadIdentity(const GLubyte *PC)
{
    (*__GL_DISPATCH(LoadIdentity, __GLvoidProc))();
    return PC;
}

/* 690dec30 -- __glle_LoadMatrixf */
const GLubyte * __fastcall __glle_LoadMatrixf(const GLubyte *PC)
{
    (*__GL_DISPATCH(LoadMatrixf, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[16]);
}

/* 690dec50 -- __glle_LoadMatrixd */
const GLubyte * __fastcall __glle_LoadMatrixd(const GLubyte *PC)
{
    (*__GL_DISPATCH(LoadMatrixd, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[16]);
}

/* 690dec70 -- __glle_MatrixMode */
const GLubyte * __fastcall __glle_MatrixMode(const GLubyte *PC)
{
    (*__GL_DISPATCH(MatrixMode, __GLintProc))(*(const GLenum *) PC);
    return PC + sizeof(GLenum);
}

/* 690dec90 -- __glle_MultMatrixf */
const GLubyte * __fastcall __glle_MultMatrixf(const GLubyte *PC)
{
    (*__GL_DISPATCH(MultMatrixf, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat[16]);
}

/* 690decb0 -- __glle_MultMatrixd */
const GLubyte * __fastcall __glle_MultMatrixd(const GLubyte *PC)
{
    (*__GL_DISPATCH(MultMatrixd, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble[16]);
}

/* 690ded20 -- __glle_PopMatrix */
const GLubyte * __fastcall __glle_PopMatrix(const GLubyte *PC)
{
    (*__GL_DISPATCH(PopMatrix, __GLvoidProc))();
    return PC;
}

/* 690ded40 -- __glle_PushMatrix */
const GLubyte * __fastcall __glle_PushMatrix(const GLubyte *PC)
{
    (*__GL_DISPATCH(PushMatrix, __GLvoidProc))();
    return PC;
}

/* 690def10 -- __glle_PolygonOffset */
const GLubyte * __fastcall __glle_PolygonOffset(const GLubyte *PC)
{
    const struct __gllc_PolygonOffset_Rec *data;

    data = (const struct __gllc_PolygonOffset_Rec *) PC;
    (*__GL_DISPATCH(PolygonOffset, __GLfloat2Proc))(data->factor, data->units);
    return PC + sizeof(struct __gllc_PolygonOffset_Rec);
}

/* 690df040 -- __glle_BindTexture */
const GLubyte * __fastcall __glle_BindTexture(const GLubyte *PC)
{
    const struct __gllc_BindTexture_Rec *data;

    data = (const struct __gllc_BindTexture_Rec *) PC;
    (*__GL_DISPATCH(BindTexture, __GLint2Proc))(data->target, data->texture);
    return PC + sizeof(struct __gllc_BindTexture_Rec);
}

/* 690df0b0 -- __glle_Indexubv */
const GLubyte * __fastcall __glle_Indexubv(const GLubyte *PC)
{
    (*__GL_DISPATCH(Indexubv, __GLvectorProc))(PC);
    return PC + __GL_PAD(sizeof(GLubyte[1]));
}

/* 690df0d0 -- __glle_IndexFuncSGI */
const GLubyte * __fastcall __glle_IndexFuncSGI(const GLubyte *PC)
{
    const struct __gllc_IndexFuncSGI_Rec *data;

    data = (const struct __gllc_IndexFuncSGI_Rec *) PC;
    (*__GL_DISPATCH(IndexFuncSGI, __GLintFloatProc))(data->func, data->ref);
    return PC + sizeof(struct __gllc_IndexFuncSGI_Rec);
}

/* 690df100 -- __glle_IndexMaterialSGI */
const GLubyte * __fastcall __glle_IndexMaterialSGI(const GLubyte *PC)
{
    const struct __gllc_IndexMaterialSGI_Rec *data;

    data = (const struct __gllc_IndexMaterialSGI_Rec *) PC;
    (*__GL_DISPATCH(IndexMaterialSGI, __GLint2Proc))(data->face, data->mode);
    return PC + sizeof(struct __gllc_IndexMaterialSGI_Rec);
}

/* 690df130 -- __glle_ActiveTextureARB */
const GLubyte * __fastcall __glle_ActiveTextureARB(const GLubyte *PC)
{
    (*__GL_DISPATCH(ActiveTextureARB, __GLintProc))(*(const GLenum *) PC);
    return PC + sizeof(GLenum);
}

/* 690df150 -- __glle_ClientActiveTextureARB */
const GLubyte * __fastcall __glle_ClientActiveTextureARB(const GLubyte *PC)
{
    (*__GL_DISPATCH(ClientActiveTextureARB, __GLintProc))(*(const GLenum *) PC);
    return PC + sizeof(GLenum);
}

/* 690df1d0 -- __glle_MultiTexCoord1fARB */
const GLubyte * __fastcall __glle_MultiTexCoord1fARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord1fARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord1fARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord1fARB, __GLintFloatProc))(data->target, data->s);
    return PC + sizeof(struct __gllc_MultiTexCoord1fARB_Rec);
}

/* 690df200 -- __glle_MultiTexCoord1fvARB */
const GLubyte * __fastcall __glle_MultiTexCoord1fvARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord1fvARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord1fvARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord1fvARB, __GLintVectorProc))(data->target, data->v);
    return PC + sizeof(struct __gllc_MultiTexCoord1fvARB_Rec);
}

/* 690df230 -- __glle_MultiTexCoord1iARB */
const GLubyte * __fastcall __glle_MultiTexCoord1iARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord1iARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord1iARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord1iARB, __GLint2Proc))(data->target, data->s);
    return PC + sizeof(struct __gllc_MultiTexCoord1iARB_Rec);
}

/* 690df260 -- __glle_MultiTexCoord1ivARB */
const GLubyte * __fastcall __glle_MultiTexCoord1ivARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord1ivARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord1ivARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord1ivARB, __GLintVectorProc))(data->target, data->v);
    return PC + sizeof(struct __gllc_MultiTexCoord1ivARB_Rec);
}

/* 690df2c0 -- __glle_MultiTexCoord1svARB */
const GLubyte * __fastcall __glle_MultiTexCoord1svARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord1svARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord1svARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord1svARB, __GLintVectorProc))(data->target, data->v);
    return PC + sizeof(struct __gllc_MultiTexCoord1svARB_Rec);
}

/* 690df380 -- __glle_MultiTexCoord2fvARB */
const GLubyte * __fastcall __glle_MultiTexCoord2fvARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord2fvARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord2fvARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord2fvARB, __GLintVectorProc))(data->target, data->v);
    return PC + sizeof(struct __gllc_MultiTexCoord2fvARB_Rec);
}

/* 690df3e0 -- __glle_MultiTexCoord2ivARB */
const GLubyte * __fastcall __glle_MultiTexCoord2ivARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord2ivARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord2ivARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord2ivARB, __GLintVectorProc))(data->target, data->v);
    return PC + sizeof(struct __gllc_MultiTexCoord2ivARB_Rec);
}

/* 690df440 -- __glle_MultiTexCoord2svARB */
const GLubyte * __fastcall __glle_MultiTexCoord2svARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord2svARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord2svARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord2svARB, __GLintVectorProc))(data->target, data->v);
    return PC + sizeof(struct __gllc_MultiTexCoord2svARB_Rec);
}

/* 690df510 -- __glle_MultiTexCoord3fvARB */
const GLubyte * __fastcall __glle_MultiTexCoord3fvARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord3fvARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord3fvARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord3fvARB, __GLintVectorProc))(data->target, data->v);
    return PC + sizeof(struct __gllc_MultiTexCoord3fvARB_Rec);
}

/* 690df570 -- __glle_MultiTexCoord3ivARB */
const GLubyte * __fastcall __glle_MultiTexCoord3ivARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord3ivARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord3ivARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord3ivARB, __GLintVectorProc))(data->target, data->v);
    return PC + sizeof(struct __gllc_MultiTexCoord3ivARB_Rec);
}

/* 690df5d0 -- __glle_MultiTexCoord3svARB */
const GLubyte * __fastcall __glle_MultiTexCoord3svARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord3svARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord3svARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord3svARB, __GLintVectorProc))(data->target, data->v);
    return PC + sizeof(struct __gllc_MultiTexCoord3svARB_Rec);
}

/* 690df6a0 -- __glle_MultiTexCoord4fvARB */
const GLubyte * __fastcall __glle_MultiTexCoord4fvARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord4fvARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord4fvARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord4fvARB, __GLintVectorProc))(data->target, data->v);
    return PC + sizeof(struct __gllc_MultiTexCoord4fvARB_Rec);
}

/* 690df700 -- __glle_MultiTexCoord4ivARB */
const GLubyte * __fastcall __glle_MultiTexCoord4ivARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord4ivARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord4ivARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord4ivARB, __GLintVectorProc))(data->target, data->v);
    return PC + sizeof(struct __gllc_MultiTexCoord4ivARB_Rec);
}

/* 690df770 -- __glle_MultiTexCoord4svARB */
const GLubyte * __fastcall __glle_MultiTexCoord4svARB(const GLubyte *PC)
{
    const struct __gllc_MultiTexCoord4svARB_Rec *data;

    data = (const struct __gllc_MultiTexCoord4svARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord4svARB, __GLintVectorProc))(data->target, data->v);
    return PC + sizeof(struct __gllc_MultiTexCoord4svARB_Rec);
}

/* 690df7a0 -- __glle_PointParameterfEXT */
const GLubyte * __fastcall __glle_PointParameterfEXT(const GLubyte *PC)
{
    const struct __gllc_PointParameterfEXT_Rec *data;

    data = (const struct __gllc_PointParameterfEXT_Rec *) PC;
    (*__GL_DISPATCH(PointParameterfEXT, __GLintFloatProc))(data->pname, data->param);
    return PC + sizeof(struct __gllc_PointParameterfEXT_Rec);
}

/* 690df7d0 -- __glle_PointParameterfvEXT */
const GLubyte * __fastcall __glle_PointParameterfvEXT(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    const struct __gllc_PointParameterfvEXT_Rec *data;

    data = (const struct __gllc_PointParameterfvEXT_Rec *) PC;
    (*__GL_DISPATCH(PointParameterfvEXT, __GLintVectorProc))(data->pname,
	(const GLfloat *) (PC + sizeof(struct __gllc_PointParameterfvEXT_Rec)));
    arraySize = __glPointParameterfvEXT_size(data->pname) * 4;
    size = (GLuint) sizeof(struct __gllc_PointParameterfvEXT_Rec) + arraySize;
    return PC + size;
}

/* 690df800 -- __glle_BlendEquationEXT */
const GLubyte * __fastcall __glle_BlendEquationEXT(const GLubyte *PC)
{
    (*__GL_DISPATCH(BlendEquationEXT, __GLintProc))(*(const GLenum *) PC);
    return PC + sizeof(GLenum);
}

/* 690df960 -- __glle_TBufferMask3DFX */
const GLubyte * __fastcall __glle_TBufferMask3DFX(const GLubyte *PC)
{
    (*__GL_DISPATCH(TBufferMask3DFX, __GLintProc))(*(const GLuint *) PC);
    return PC + sizeof(GLuint);
}

/* 690df980 -- __glle_FogCoordfEXT */
const GLubyte * __fastcall __glle_FogCoordfEXT(const GLubyte *PC)
{
    (*__GL_DISPATCH(FogCoordfEXT, __GLfloatProc))(*(const GLfloat *) PC);
    return PC + sizeof(GLfloat);
}

/* 690df9a0 -- __glle_FogCoorddEXT */
const GLubyte * __fastcall __glle_FogCoorddEXT(const GLubyte *PC)
{
    (*__GL_DISPATCH(FogCoorddEXT, __GLdoubleProc))(*(const GLdouble *) PC);
    return PC + sizeof(GLdouble);
}

/* 690df9d0 -- __glle_FogCoordfvEXT */
const GLubyte * __fastcall __glle_FogCoordfvEXT(const GLubyte *PC)
{
    (*__GL_DISPATCH(FogCoordfvEXT, __GLvectorProc))(PC);
    return PC + sizeof(GLfloat);
}

/* 690df9f0 -- __glle_FogCoorddvEXT */
const GLubyte * __fastcall __glle_FogCoorddvEXT(const GLubyte *PC)
{
    (*__GL_DISPATCH(FogCoorddvEXT, __GLvectorProc))(PC);
    return PC + sizeof(GLdouble);
}

/* ---- 690dd340: reset a block's node list (head and count) and free its nodes through
** DeleteBlock (which frees node->data and the node with gc->imports.free) ---- */
/* ---- display-list execute routines (dlist "execute" table): two-argument API entries ----
** The dispatch entries are __stdcall APIs; the PC is the first argument (or the second, after a
** pname stored behind the data). */
typedef void (__stdcall *PFNAPI2)(const void *a, const void *b);
#define DISP(n) (__glCurrentGC()->currentDispatchState.fn[n])
typedef void (__stdcall *PFNAPI2E)(GLenum e, const void *b);
#define DISPATCH_FN(n, type)     ((type) __glCurrentGC()->currentDispatchState.fn[n])
typedef void (APIENTRY *__GLfn_1p)(GLint, const void *);
/* the *_size procs that size the records (si_names.h; the 5-byte jumps among them in og_build.c / gl_texstate.c) */
extern GLint __fastcall __glLightiv_size(GLenum pname);
extern GLint __fastcall __glTexParameteriv_size(GLenum pname);
typedef GLint (__fastcall *__GLsize_fn)(GLenum);
#define CALL_SIZE(size, pname)  (((__GLsize_fn) size)(pname))
/* records: (pname, params[]) and (face/target, pname, params[]) */
#define REC_1V(NAME, IDX, SIZE) \
const GLubyte * __fastcall NAME(const GLubyte *PC) \
{ \
    const GLint *p = (const GLint *) PC; \
    (*DISPATCH_FN(IDX, __GLfn_1p))(p[0], p + 1); \
    return PC + 4 * CALL_SIZE(SIZE, p[0]) + 4; \
}
typedef void (APIENTRY *__GLfn_2p)(GLint, GLint, const void *);
#define REC_2V(NAME, IDX, SIZE) \
const GLubyte * __fastcall NAME(const GLubyte *PC) \
{ \
    const GLint *p = (const GLint *) PC; \
    (*DISPATCH_FN(IDX, __GLfn_2p))(p[0], p[1], p + 2); \
    return PC + 4 * CALL_SIZE(SIZE, p[1]) + 8; \
}
typedef void (APIENTRY *__GLfn_1s)(GLint, GLushort);
typedef void (APIENTRY *__GLfn_1)(GLint);
typedef void (APIENTRY *__GLfn_2)(GLint, GLint);
typedef void (APIENTRY *__GLfn_3)(GLint, GLint, GLint);
typedef void (APIENTRY *__GLfn_4)(GLint, GLint, GLint, GLint);
typedef void (APIENTRY *__GLfn_5)(GLint, GLint, GLint, GLint, GLint);
typedef void (APIENTRY *__GLfn_6)(GLint, GLint, GLint, GLint, GLint, GLint);
typedef void (APIENTRY *__GLfn_1ss)(GLint, GLushort, GLushort);
typedef void (APIENTRY *__GLfn_1sss)(GLint, GLushort, GLushort, GLushort);
typedef void (APIENTRY *__GLfn_1ssss)(GLint, GLushort, GLushort, GLushort, GLushort);
typedef void (APIENTRY *__GLfn_b)(GLubyte);
typedef void (APIENTRY *__GLfn_bbbb)(GLubyte, GLubyte, GLubyte, GLubyte);
typedef void (APIENTRY *__GLfn_idd)(GLint, GLdouble, GLdouble);
typedef void (APIENTRY *__GLfn_ipp)(GLsizei, const GLuint *, const GLclampf *);
#define REC_N(NAME, IDX, N, ARGS) \
const GLubyte * __fastcall NAME(const GLubyte *PC) \
{ \
    const GLint *p = (const GLint *) PC; \
    (*DISPATCH_FN(IDX, __GLfn_##N)) ARGS; \
    return PC + 4 * N; \
}
struct __gllc_MapGrid2d_Rec { GLdouble u1, u2, v1, v2; GLint un, vn; };
typedef void (APIENTRY *__GLMapGrid2dProc)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble);
#define PAD(x)          (((x) + 3) & ~3)
/* ---- display list execution (g_lexec.c) ---- */
struct __gllc_Frustum_Rec {
    GLdouble    left, right, bottom, top, zNear, zFar;
};
typedef void (APIENTRY *__GLfn_6d)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
struct __gllc_Ortho_Rec {
    GLdouble    left, right, bottom, top, zNear, zFar;
};
struct __gllc_Rotated_Rec { GLdouble angle, x, y, z; };
typedef void (APIENTRY *__GLRotatedProc)(GLdouble, GLdouble, GLdouble, GLdouble);
struct __gllc_CopyTexImage1D_Rec { GLenum target; GLint level; GLenum internalformat;
    GLint x, y; GLsizei width; GLint border; };
typedef void (APIENTRY *__GLCopyTexImage1DProc)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
struct __gllc_CopyTexImage2D_Rec { GLenum target; GLint level; GLenum internalformat;
    GLint x, y; GLsizei width, height; GLint border; };
typedef void (APIENTRY *__GLCopyTexImage2DProc)(GLenum, GLint, GLenum, GLint, GLint, GLsizei,
						    GLsizei, GLint);
struct __gllc_CopyTexSubImage2D_Rec { GLenum target; GLint level; GLint xoffset, yoffset;
    GLint x, y; GLsizei width, height; };
typedef void (APIENTRY *__GLCopyTexSubImage2DProc)(GLenum, GLint, GLint, GLint, GLint, GLint,
						       GLsizei, GLsizei);
struct MapGrid1d_Rec { GLdouble u1, u2; GLint un; GLint pad; };     /* padded to 0x18 */
struct __gllc_MultiTexCoord3dARB_Rec { GLdouble s, t, r; GLenum target; GLuint pad; };
typedef void (APIENTRY *__GLMultiTexCoord3dProc)(GLenum, GLdouble, GLdouble, GLdouble);
struct __gllc_MultiTexCoord4dARB_Rec { GLdouble s, t, r, q; GLenum target; GLuint pad; };
typedef void (APIENTRY *__GLMultiTexCoord4dProc)(GLenum, GLdouble, GLdouble, GLdouble, GLdouble);
struct __gllc_CompressedTexImage1DARB_Rec { GLenum target; GLint level; GLenum internalformat;
    GLsizei width; GLint border; GLsizei imageSize; };
typedef void (APIENTRY *__GLCompressedTexImage1DProc)(GLenum, GLint, GLenum, GLsizei, GLint,
							  GLsizei, const GLvoid *);
struct __gllc_CompressedTexImage2DARB_Rec { GLenum target; GLint level; GLenum internalformat;
    GLsizei width, height; GLint border; GLsizei imageSize; };
typedef void (APIENTRY *__GLCompressedTexImage2DProc)(GLenum, GLint, GLenum, GLsizei, GLsizei,
							  GLint, GLsizei, const GLvoid *);
struct __gllc_CompressedTexSubImage1DARB_Rec { GLenum target; GLint level; GLint xoffset;
    GLsizei width; GLenum format; GLsizei imageSize; };
typedef void (APIENTRY *__GLCompressedTexSubImage1DProc)(GLenum, GLint, GLint, GLsizei, GLenum,
							     GLsizei, const GLvoid *);
struct __gllc_CompressedTexSubImage2DARB_Rec { GLenum target; GLint level; GLint xoffset, yoffset;
    GLsizei width, height; GLenum format; GLsizei imageSize; };
typedef void (APIENTRY *__GLCompressedTexSubImage2DProc)(GLenum, GLint, GLint, GLint, GLsizei,
							     GLsizei, GLenum, GLsizei, const GLvoid *);
struct PrioritizeTextures_Rec { GLsizei n; };
extern void __fastcall DeleteBlock(__GLcontext *gc, __GLarenaBlock *block);
extern __GLarenaBlock * __fastcall NewBlock(__GLcontext *gc, GLuint size);

/* 690dd340 -- __glArenaFreeAll (s_memmgr.c): empty the arena's first block and free the others through DeleteBlock
** (gc->procs.freeAll) */
void __fastcall __glArenaFreeAll(__GLarena *arena)
{
    __GLarenaBlock *block = arena->firstBlock;
    __GLcontext *gc = arena->gc;
    __GLarenaBlock *node, *tmp;

    node = block->next;
    block->next = NULL;
    block->allocated = 0;
    arena->lastBlock = block;
    while ((tmp = node) != NULL) {
	node = node->next;
	DeleteBlock(gc, tmp);
    }
}

/* 690ddb10 -- __glle_Rectdv (g_lexec.c): (v1, v2) = two GLdouble[2] */
const GLubyte *__fastcall __glle_Rectdv(const GLubyte *PC)
{
    (*(PFNAPI2)DISP(88))(PC, PC + 16);
    return PC + 32;
}

/* 690ddb40 -- __glle_Rectfv (g_lexec.c): (v1, v2) = two GLfloat[2] */
const GLubyte *__fastcall __glle_Rectfv(const GLubyte *PC)
{
    (*(PFNAPI2)DISP(90))(PC, PC + 8);
    return PC + 16;
}

/* 690ddb70 -- __glle_Rectiv (g_lexec.c): (v1, v2) = two GLint[2] */
const GLubyte *__fastcall __glle_Rectiv(const GLubyte *PC)
{
    (*(PFNAPI2)DISP(92))(PC, PC + 8);
    return PC + 16;
}

/* 690ddba0 -- __glle_Rectsv (g_lexec.c): (v1, v2) = two GLshort[2] */
const GLubyte *__fastcall __glle_Rectsv(const GLubyte *PC)
{
    (*(PFNAPI2)DISP(94))(PC, PC + 4);
    return PC + 8;
}

struct __gllc_ClipPlane_Rec { GLdouble equation[4]; GLenum plane; };
/* 690ddf50 -- __glle_ClipPlane (g_lexec.c): (equation[4] as doubles, plane at +0x20) */
const GLubyte *__fastcall __glle_ClipPlane(const GLubyte *PC)
{
    (*(PFNAPI2E)DISP(151))(((const struct __gllc_ClipPlane_Rec *)PC)->plane, PC);
    return PC + 0x28;
}

/* 690de000 -- __glle_Fogfv: dispatch slot 155 = glFogfv */
REC_1V(__glle_Fogfv, 155, __glFogfv_size)

/* 690de0e0 -- __glle_Lightfv */
REC_2V(__glle_Lightfv, 161, __glLightfv_size)

/* 690de110 -- __glle_Lightiv */
REC_2V(__glle_Lightiv, 163, __glLightiv_size)

/* 690de170 -- __glle_LightModeliv */
REC_1V(__glle_LightModeliv, 167, __glLightModeliv_size)

/* 690de1a0 -- __glle_LineStipple (factor, GLushort pattern) */
const GLubyte * __fastcall __glle_LineStipple(const GLubyte *PC)
{
    const GLint *p = (const GLint *) PC;
    (*DISPATCH_FN(168, __GLfn_1s))(p[0], ((const GLushort *) PC)[2]);
    return PC + 8;
}

/* 690de1f0 -- __glle_Materialfv */
REC_2V(__glle_Materialfv, 171, __glMaterialfv_size)

/* 690de220 -- __glle_Materialiv */
REC_2V(__glle_Materialiv, 173, __glMaterialiv_size)

/* records of 4 dwords */
/* 690de2a0 -- __glle_Scissor */
REC_N(__glle_Scissor, 177, 4, (p[0], p[1], p[2], p[3]))

/* 690de2f0 -- __glle_TexParameterfv */
REC_2V(__glle_TexParameterfv, 180, __glTexParameterfv_size)

/* 690de320 -- __glle_TexParameteriv */
REC_2V(__glle_TexParameteriv, 182, __glTexParameteriv_size)

/* 690de350 -- __glle_TexEnvfv */
REC_2V(__glle_TexEnvfv, 186, __glTexEnvfv_size)

/* 690de380 -- __glle_TexEnviv */
REC_2V(__glle_TexEnviv, 188, __glTexEnviv_size)

/* 690de3b0 -- __glle_TexGendv (double params) */
const GLubyte * __fastcall __glle_TexGendv(const GLubyte *PC)
{
    const GLint *p = (const GLint *) PC;
    (*DISPATCH_FN(190, __GLfn_2p))(p[0], p[1], p + 2);
    return PC + 8 * __glTexGendv_size(p[1]) + 8;
}

/* 690de3e0 -- __glle_TexGenfv */
REC_2V(__glle_TexGenfv, 192, __glTexGenfv_size)

/* 690de410 -- __glle_TexGeniv */
REC_2V(__glle_TexGeniv, 194, __glTexGeniv_size)

/* 690de520 -- __glle_ClearAccum */
REC_N(__glle_ClearAccum, 205, 4, (p[0], p[1], p[2], p[3]))

/* 690de570 -- __glle_ClearColor */
REC_N(__glle_ClearColor, 207, 4, (p[0], p[1], p[2], p[3]))

/* 690de610 -- __glle_ColorMask (4 GLboolean) */
const GLubyte * __fastcall __glle_ColorMask(const GLubyte *PC)
{
    (*DISPATCH_FN(211, __GLfn_bbbb))(PC[0], PC[1], PC[2], PC[3]);
    return PC + 4;
}

/* 690de640 -- __glle_DepthMask (dispatch slot 212) */
const GLubyte * __fastcall __glle_DepthMask(const GLubyte *PC)
{
    (*DISPATCH_FN(212, __GLfn_b))(PC[0]);
    return PC + 4;
}

/* 3 dwords */
/* 690de720 -- __glle_MapGrid1f */
REC_N(__glle_MapGrid1f, 226, 3, (p[0], p[1], p[2]))

/* 690de750 -- __glle_MapGrid2d */
const GLubyte * __fastcall __glle_MapGrid2d(const GLubyte *PC)
{
    struct __gllc_MapGrid2d_Rec *data = (struct __gllc_MapGrid2d_Rec *) PC;
    (*__GL_DISPATCH(MapGrid2d, __GLMapGrid2dProc))(data->un, data->u1, data->u2,
						     data->vn, data->v1, data->v2);
    return PC + sizeof(struct __gllc_MapGrid2d_Rec);
}

/* 6 dwords */
/* 690de7a0 -- __glle_MapGrid2f */
REC_N(__glle_MapGrid2f, 228, 6, (p[0], p[1], p[2], p[3], p[4], p[5]))

/* 690de860 -- __glle_EvalMesh1 */
REC_N(__glle_EvalMesh1, 237, 3, (p[0], p[1], p[2]))

/* 5 dwords */
/* 690de8b0 -- __glle_EvalMesh2 */
REC_N(__glle_EvalMesh2, 239, 5, (p[0], p[1], p[2], p[3], p[4]))

/* 690de990 -- __glle_StencilFunc */
REC_N(__glle_StencilFunc, 244, 3, (p[0], p[1], p[2]))

/* 690de9c0 -- __glle_StencilOp */
REC_N(__glle_StencilOp, 245, 3, (p[0], p[1], p[2]))

/* 690deaa0 -- __glle_PixelMapfv: (map, mapsize, values[mapsize]) */
const GLubyte * __fastcall __glle_PixelMapfv(const GLubyte *PC)
{
    const GLint *p = (const GLint *) PC;
    (*DISPATCH_FN(252, __GLfn_2p))(p[0], p[1], p + 2);
    return PC + 4 * p[1] + 8;
}

/* 690dead0 -- __glle_PixelMapuiv: (map, mapsize, values[mapsize]) */
const GLubyte * __fastcall __glle_PixelMapuiv(const GLubyte *PC)
{
    const GLint *p = (const GLint *) PC;
    (*DISPATCH_FN(253, __GLfn_2p))(p[0], p[1], p + 2);
    return PC + 4 * p[1] + 8;
}

/* 690deb00 -- __glle_PixelMapusv: (map, mapsize, GLushort values[mapsize], padded to 4 bytes) */
const GLubyte * __fastcall __glle_PixelMapusv(const GLubyte *PC)
{
    const GLint *p = (const GLint *) PC;
    (*DISPATCH_FN(254, __GLfn_2p))(p[0], p[1], p + 2);
    return PC + PAD(2 * p[1]) + 8;
}

/* 690deb60 -- __glle_CopyPixels */
REC_N(__glle_CopyPixels, 256, 5, (p[0], p[1], p[2], p[3], p[4]))

/* 690deb90 -- __glle_DepthRange (two doubles as four dwords) */
REC_N(__glle_DepthRange, 289, 4, (p[0], p[1], p[2], p[3]))

/* 690debc0 -- __glle_Frustum */
const GLubyte *__fastcall __glle_Frustum(const GLubyte *PC)
{
    struct __gllc_Frustum_Rec *data;

    data = (struct __gllc_Frustum_Rec *) PC;
    (*(__GLfn_6d) __glCurrentGC()->currentDispatchState.fn[__GL_DISP_Frustum])(data->left,
        data->right, data->bottom, data->top, data->zNear, data->zFar);
    return PC + sizeof(struct __gllc_Frustum_Rec);
}

/* 690decd0 -- __glle_Ortho */
const GLubyte *__fastcall __glle_Ortho(const GLubyte *PC)
{
    struct __gllc_Ortho_Rec *data;

    data = (struct __gllc_Ortho_Rec *) PC;
    (*(__GLfn_6d) __glCurrentGC()->currentDispatchState.fn[__GL_DISP_Ortho])(data->left,
        data->right, data->bottom, data->top, data->zNear, data->zFar);
    return PC + sizeof(struct __gllc_Ortho_Rec);
}

/* 690ded60 -- __glle_Rotated */
const GLubyte * __fastcall __glle_Rotated(const GLubyte *PC)
{
    struct __gllc_Rotated_Rec *data = (struct __gllc_Rotated_Rec *) PC;
    (*__GL_DISPATCH(Rotated, __GLRotatedProc))(data->angle, data->x, data->y, data->z);
    return PC + sizeof(struct __gllc_Rotated_Rec);
}

/* 690deda0 -- __glle_Rotatef */
REC_N(__glle_Rotatef, 301, 4, (p[0], p[1], p[2], p[3]))

/* 690dedd0 -- __glle_Scaled */
REC_N(__glle_Scaled, 302, 6, (p[0], p[1], p[2], p[3], p[4], p[5]))

/* 690dee10 -- __glle_Scalef */
REC_N(__glle_Scalef, 303, 3, (p[0], p[1], p[2]))

/* 690dee40 -- __glle_Translated */
REC_N(__glle_Translated, 304, 6, (p[0], p[1], p[2], p[3], p[4], p[5]))

/* 690dee80 -- __glle_Translatef */
REC_N(__glle_Translatef, 305, 3, (p[0], p[1], p[2]))

/* 690deeb0 -- __glle_Viewport */
REC_N(__glle_Viewport, 306, 4, (p[0], p[1], p[2], p[3]))

/* 690deee0 -- __glle_CopyColorTableEXT */
REC_N(__glle_CopyColorTableEXT, 348, 5, (p[0], p[1], p[2], p[3], p[4]))

/* 690def40 -- __glle_CopyTexImage1D */
const GLubyte * __fastcall __glle_CopyTexImage1D(const GLubyte *PC)
{
    struct __gllc_CopyTexImage1D_Rec *data = (struct __gllc_CopyTexImage1D_Rec *) PC;
    (*__GL_DISPATCH(CopyTexImage1D, __GLCopyTexImage1DProc))(data->target, data->level,
	data->internalformat, data->x, data->y, data->width, data->border);
    return PC + sizeof(struct __gllc_CopyTexImage1D_Rec);
}

/* 690def80 -- __glle_CopyTexImage2D */
const GLubyte * __fastcall __glle_CopyTexImage2D(const GLubyte *PC)
{
    struct __gllc_CopyTexImage2D_Rec *data = (struct __gllc_CopyTexImage2D_Rec *) PC;
    (*__GL_DISPATCH(CopyTexImage2D, __GLCopyTexImage2DProc))(data->target, data->level,
	data->internalformat, data->x, data->y, data->width, data->height, data->border);
    return PC + sizeof(struct __gllc_CopyTexImage2D_Rec);
}

/* 690defc0 -- __glle_CopyTexSubImage1D */
REC_N(__glle_CopyTexSubImage1D, 326, 6, (p[0], p[1], p[2], p[3], p[4], p[5]))

/* 690df000 -- __glle_CopyTexSubImage2D */
const GLubyte * __fastcall __glle_CopyTexSubImage2D(const GLubyte *PC)
{
    struct __gllc_CopyTexSubImage2D_Rec *data = (struct __gllc_CopyTexSubImage2D_Rec *) PC;
    (*__GL_DISPATCH(CopyTexSubImage2D, __GLCopyTexSubImage2DProc))(data->target, data->level,
	data->xoffset, data->yoffset, data->x, data->y, data->width, data->height);
    return PC + sizeof(struct __gllc_CopyTexSubImage2D_Rec);
}

/* 690df170 -- __glle_MultiTexCoord1dARB: record (s as a double, target): the entry takes (target, s) */
const GLubyte * __fastcall __glle_MultiTexCoord1dARB(const GLubyte *PC)
{
    const GLint *p = (const GLint *) PC;
    (*DISPATCH_FN(361, __GLfn_3))(p[2], p[0], p[1]);
    return PC + 16;
}

/* 690df1a0 -- __glle_MultiTexCoord1dvARB: (v[1] as doubles, target at +8) */
const GLubyte *__fastcall __glle_MultiTexCoord1dvARB(const GLubyte *PC)
{
    (*(PFNAPI2E)DISP(362))(*(GLenum *)(PC + 8), PC);
    return PC + 0x10;
}

/* 690df290 -- __glle_MultiTexCoord1sARB: (target, GLshort) */
const GLubyte * __fastcall __glle_MultiTexCoord1sARB(const GLubyte *PC)
{
    const GLint *p = (const GLint *) PC;
    (*DISPATCH_FN(367, __GLfn_1s))(p[0], ((const GLushort *) PC)[2]);
    return PC + 8;
}

/* 690df2f0 -- __glle_MultiTexCoord2dARB: record (s, t, target) in the MapGrid1d layout; dispatch slot 369 =
** glMultiTexCoord2dARB(target, s, t) */
const GLubyte * __fastcall __glle_MultiTexCoord2dARB(const GLubyte *PC)
{
    struct MapGrid1d_Rec *data = (struct MapGrid1d_Rec *) PC;
    (*DISPATCH_FN(369, __GLfn_idd))(data->un, data->u1, data->u2);
    return PC + sizeof(struct MapGrid1d_Rec);
}

struct __gllc_MultiTexCoord2dvARB_Rec { GLdouble v[2]; GLenum target; };
/* 690df320 -- __glle_MultiTexCoord2dvARB: (v[2] as doubles, target at +0x10) */
const GLubyte *__fastcall __glle_MultiTexCoord2dvARB(const GLubyte *PC)
{
    (*(PFNAPI2E)DISP(370))(((const struct __gllc_MultiTexCoord2dvARB_Rec *)PC)->target, PC);
    return PC + 0x18;
}

/* 690df350 -- __glle_MultiTexCoord2fARB */
REC_N(__glle_MultiTexCoord2fARB, 371, 3, (p[0], p[1], p[2]))

/* 690df3b0 -- __glle_MultiTexCoord2iARB */
REC_N(__glle_MultiTexCoord2iARB, 373, 3, (p[0], p[1], p[2]))

/* 690df410 -- __glle_MultiTexCoord2sARB: (target, GLshort, GLshort) */
const GLubyte * __fastcall __glle_MultiTexCoord2sARB(const GLubyte *PC)
{
    const GLint *p = (const GLint *) PC;
    const GLushort *s = (const GLushort *) PC;
    (*DISPATCH_FN(375, __GLfn_1ss))(p[0], s[2], s[3]);
    return PC + 8;
}

/* 690df470 -- __glle_MultiTexCoord3dARB */
const GLubyte * __fastcall __glle_MultiTexCoord3dARB(const GLubyte *PC)
{
    struct __gllc_MultiTexCoord3dARB_Rec *data = (struct __gllc_MultiTexCoord3dARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord3dARB, __GLMultiTexCoord3dProc))(data->target, data->s, data->t, data->r);
    return PC + sizeof(struct __gllc_MultiTexCoord3dARB_Rec);
}

struct __gllc_MultiTexCoord3dvARB_Rec { GLdouble v[3]; GLenum target; };
/* 690df4b0 -- __glle_MultiTexCoord3dvARB: (v[3] as doubles, target at +0x18) */
const GLubyte *__fastcall __glle_MultiTexCoord3dvARB(const GLubyte *PC)
{
    (*(PFNAPI2E)DISP(378))(((const struct __gllc_MultiTexCoord3dvARB_Rec *)PC)->target, PC);
    return PC + 0x20;
}

/* 690df4e0 -- __glle_MultiTexCoord3fARB */
REC_N(__glle_MultiTexCoord3fARB, 379, 4, (p[0], p[1], p[2], p[3]))

/* 690df540 -- __glle_MultiTexCoord3iARB */
REC_N(__glle_MultiTexCoord3iARB, 381, 4, (p[0], p[1], p[2], p[3]))

/* 690df5a0 -- __glle_MultiTexCoord3sARB: (target, GLshort x3) */
const GLubyte * __fastcall __glle_MultiTexCoord3sARB(const GLubyte *PC)
{
    const GLint *p = (const GLint *) PC;
    const GLushort *s = (const GLushort *) PC;
    (*DISPATCH_FN(383, __GLfn_1sss))(p[0], s[2], s[3], s[4]);
    return PC + 12;
}

/* 690df600 -- __glle_MultiTexCoord4dARB */
const GLubyte * __fastcall __glle_MultiTexCoord4dARB(const GLubyte *PC)
{
    struct __gllc_MultiTexCoord4dARB_Rec *data = (struct __gllc_MultiTexCoord4dARB_Rec *) PC;
    (*__GL_DISPATCH(MultiTexCoord4dARB, __GLMultiTexCoord4dProc))(data->target, data->s, data->t, data->r,
							   data->q);
    return PC + sizeof(struct __gllc_MultiTexCoord4dARB_Rec);
}

struct __gllc_MultiTexCoord4dvARB_Rec { GLdouble v[4]; GLenum target; };
/* 690df640 -- __glle_MultiTexCoord4dvARB: (v[4] as doubles, target at +0x20) */
const GLubyte *__fastcall __glle_MultiTexCoord4dvARB(const GLubyte *PC)
{
    (*(PFNAPI2E)DISP(386))(((const struct __gllc_MultiTexCoord4dvARB_Rec *)PC)->target, PC);
    return PC + 0x28;
}

/* 690df670 -- __glle_MultiTexCoord4fARB */
REC_N(__glle_MultiTexCoord4fARB, 387, 5, (p[0], p[1], p[2], p[3], p[4]))

/* 690df6d0 -- __glle_MultiTexCoord4iARB */
REC_N(__glle_MultiTexCoord4iARB, 389, 5, (p[0], p[1], p[2], p[3], p[4]))

/* 690df730 -- __glle_MultiTexCoord4sARB: (target, GLshort x4) */
const GLubyte * __fastcall __glle_MultiTexCoord4sARB(const GLubyte *PC)
{
    const GLint *p = (const GLint *) PC;
    const GLushort *s = (const GLushort *) PC;
    (*DISPATCH_FN(391, __GLfn_1ssss))(p[0], s[2], s[3], s[4], s[5]);
    return PC + 12;
}

/* 690df820 -- __glle_CompressedTexImage1DARB (the image follows the record) */
const GLubyte * __fastcall __glle_CompressedTexImage1DARB(const GLubyte *PC)
{
    struct __gllc_CompressedTexImage1DARB_Rec *data = (struct __gllc_CompressedTexImage1DARB_Rec *) PC;
    (*__GL_DISPATCH(CompressedTexImage1DARB, __GLCompressedTexImage1DProc))(data->target, data->level,
	data->internalformat, data->width, data->border, data->imageSize,
	(const GLvoid *)(PC + sizeof(struct __gllc_CompressedTexImage1DARB_Rec)));
    return PC + sizeof(struct __gllc_CompressedTexImage1DARB_Rec) + __GL_PAD(data->imageSize);
}

/* 690df870 -- __glle_CompressedTexImage2DARB */
const GLubyte * __fastcall __glle_CompressedTexImage2DARB(const GLubyte *PC)
{
    struct __gllc_CompressedTexImage2DARB_Rec *data = (struct __gllc_CompressedTexImage2DARB_Rec *) PC;
    (*__GL_DISPATCH(CompressedTexImage2DARB, __GLCompressedTexImage2DProc))(data->target, data->level,
	data->internalformat, data->width, data->height, data->border, data->imageSize,
	(const GLvoid *)(PC + sizeof(struct __gllc_CompressedTexImage2DARB_Rec)));
    return PC + sizeof(struct __gllc_CompressedTexImage2DARB_Rec) + __GL_PAD(data->imageSize);
}

/* 690df8c0 -- __glle_CompressedTexSubImage1DARB */
const GLubyte * __fastcall __glle_CompressedTexSubImage1DARB(const GLubyte *PC)
{
    struct __gllc_CompressedTexSubImage1DARB_Rec *data = (struct __gllc_CompressedTexSubImage1DARB_Rec *) PC;
    (*__GL_DISPATCH(CompressedTexSubImage1DARB, __GLCompressedTexSubImage1DProc))(data->target, data->level,
	data->xoffset, data->width, data->format, data->imageSize,
	(const GLvoid *)(PC + sizeof(struct __gllc_CompressedTexSubImage1DARB_Rec)));
    return PC + sizeof(struct __gllc_CompressedTexSubImage1DARB_Rec) + __GL_PAD(data->imageSize);
}

/* 690df910 -- __glle_CompressedTexSubImage2DARB */
const GLubyte * __fastcall __glle_CompressedTexSubImage2DARB(const GLubyte *PC)
{
    struct __gllc_CompressedTexSubImage2DARB_Rec *data = (struct __gllc_CompressedTexSubImage2DARB_Rec *) PC;
    (*__GL_DISPATCH(CompressedTexSubImage2DARB, __GLCompressedTexSubImage2DProc))(data->target, data->level,
	data->xoffset, data->yoffset, data->width, data->height, data->format, data->imageSize,
	(const GLvoid *)(PC + sizeof(struct __gllc_CompressedTexSubImage2DARB_Rec)));
    return PC + sizeof(struct __gllc_CompressedTexSubImage2DARB_Rec) + __GL_PAD(data->imageSize);
}

/* 690dd2e0 -- __glArenaAlloc (s_memmgr.c; gc->procs.arenaAlloc): 8-byte steps, a new block of at
** least 256K when the last one is full */
void * __fastcall __glArenaAlloc(__GLarena *arena, GLuint size)
{
    __GLarenaBlock *block = arena->lastBlock;
    __GLarenaBlock *newBlock;
    GLuint allocated = block->allocated;
    GLuint blockSize;

    size += 8 - (size & 7);
    if (size <= block->size - allocated) {
	block->allocated = allocated + size;
	return block->data + allocated;
    }
    blockSize = size;
    if (blockSize < 0x40000) {
	blockSize = 0x40000;
    }
    newBlock = NewBlock(arena->gc, blockSize);
    block->next = newBlock;
    arena->lastBlock = newBlock;
    newBlock->allocated = size;
    return newBlock->data;
}

/* 690de6f0 -- __glle_MapGrid1d: record (u1, u2, un) */
const GLubyte * __fastcall __glle_MapGrid1d(const GLubyte *PC)
{
    struct MapGrid1d_Rec *data = (struct MapGrid1d_Rec *) PC;
    (*DISPATCH_FN(225, __GLfn_idd))(data->un, data->u1, data->u2);
    return PC + sizeof(struct MapGrid1d_Rec);
}

/* 690df070 -- __glle_PrioritizeTextures: (n, textures[n], priorities[n]); the size is computed
** up front from the record and n is re-read for the call */
const GLubyte * __fastcall __glle_PrioritizeTextures(const GLubyte *PC)
{
    struct PrioritizeTextures_Rec *data = (struct PrioritizeTextures_Rec *) PC;
    GLint size = data->n * sizeof(GLuint);
    GLint total = sizeof(struct PrioritizeTextures_Rec) + size + data->n * sizeof(GLclampf);
    (*DISPATCH_FN(332, __GLfn_ipp))(data->n,
        (const GLuint *) (PC + sizeof(struct PrioritizeTextures_Rec)),
        (const GLclampf *) (PC + sizeof(struct PrioritizeTextures_Rec) + size));
    return PC + total;
}

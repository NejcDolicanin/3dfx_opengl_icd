/*
** gl_lcomp2.c -- original VA range 690ac000..690b0000: SGI Sample Implementation glcore (MS ICD kit
** flavour): __gllc_* display-list compile functions (MapGrid2d .. the extension entries; the
** MultiTexCoord*ARB ones do not set __GL_DLFLAG_HAS_TEXCOORDS as the SI does), the
** s_varray.c copy procs, glLockArraysEXT / glUnlockArraysEXT and the vertex-array clip helpers
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include "icdprotos.h"
#include <glide.h>

/************************************************************************/
/* g_lcomp.c: the display-list compile functions (__gllc_*).  Each allocates an op whose record
** holds its arguments (SI order: doubles first, then words, shorts, bytes, each group padded to a
** whole word; a record with doubles is a multiple of 8 bytes) and appends it with the execute
** function that replays it. */

/* (__glDlistAllocOp2 / __glDlistAppendOp, __GL_DLFLAG_HAS_*, the __glop_* opcodes: glsi.h) */

/* the execute functions (gl_lexec.c) of the ops compiled here */
extern const GLubyte * __fastcall __glle_MapGrid2f(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_EvalMesh1(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_EvalMesh2(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_StencilFunc(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_StencilOp(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_CopyPixels(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_DepthRange(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_Rotatef(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_Scaled(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_Scalef(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_Translated(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_Translatef(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_Viewport(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_CopyColorTableEXT(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_CopyTexSubImage1D(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_MultiTexCoord2fARB(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_MultiTexCoord2iARB(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_MultiTexCoord3fARB(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_MultiTexCoord3iARB(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_MultiTexCoord4fARB(const GLubyte *PC);
extern const GLubyte * __fastcall __glle_MultiTexCoord4iARB(const GLubyte *PC);

/************************************************************************/

#define VERTARRAY_CW_LOCKED     0x4         /* gc->vertexArray.flags: glLockArraysEXT is in effect */

struct __gllc_MapGrid2d_Rec {
    GLdouble    u1;
    GLdouble    u2;
    GLdouble    v1;
    GLdouble    v2;
    GLint       un;
    GLint       vn;
};

/* 690ac030 -- __gllc_MapGrid2d */
void __stdcall __gllc_MapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MapGrid2d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MapGrid2d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MapGrid2d;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_MapGrid2d_Rec *) dlop->data;
    data->un = un;
    data->u1 = u1;
    data->u2 = u2;
    data->vn = vn;
    data->v1 = v1;
    data->v2 = v2;
    __glDlistAppendOp(gc, dlop, __glle_MapGrid2d);
}

struct __gllc_MapGrid2f_Rec {
    GLint       un;
    GLfloat     u1;
    GLfloat     u2;
    GLint       vn;
    GLfloat     v1;
    GLfloat     v2;
};

/* 690ac0b0 -- __gllc_MapGrid2f */
void __stdcall __gllc_MapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MapGrid2f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MapGrid2f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MapGrid2f;
    data = (struct __gllc_MapGrid2f_Rec *) dlop->data;
    data->un = un;
    data->u1 = u1;
    data->u2 = u2;
    data->vn = vn;
    data->v1 = v1;
    data->v2 = v2;
    __glDlistAppendOp(gc, dlop, __glle_MapGrid2f);
}

struct __gllc_EvalCoord1d_Rec {
    GLdouble    u;
};

/* 690ac120 -- __gllc_EvalCoord1d */
void __stdcall __gllc_EvalCoord1d(GLdouble u)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EvalCoord1d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord1d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord1dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_EvalCoord1d_Rec *) dlop->data;
    data->u = u;
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord1dv);
}

struct __gllc_EvalCoord1dv_Rec {
    GLdouble    u[1];
};

/* 690ac170 -- __gllc_EvalCoord1dv */
void __stdcall __gllc_EvalCoord1dv(const GLdouble *u)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EvalCoord1dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord1dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord1dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_EvalCoord1dv_Rec *) dlop->data;
    data->u[0] = u[0];
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord1dv);
}

struct __gllc_EvalCoord1f_Rec {
    GLfloat     u;
};

/* 690ac1c0 -- __gllc_EvalCoord1f */
void __stdcall __gllc_EvalCoord1f(GLfloat u)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EvalCoord1f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord1f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord1fv;
    data = (struct __gllc_EvalCoord1f_Rec *) dlop->data;
    data->u = u;
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord1fv);
}

struct __gllc_EvalCoord1fv_Rec {
    GLfloat     u[1];
};

/* 690ac210 -- __gllc_EvalCoord1fv */
void __stdcall __gllc_EvalCoord1fv(const GLfloat *u)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EvalCoord1fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord1fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord1fv;
    data = (struct __gllc_EvalCoord1fv_Rec *) dlop->data;
    data->u[0] = u[0];
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord1fv);
}

struct __gllc_EvalCoord2d_Rec {
    GLdouble    u;
    GLdouble    v;
};

/* 690ac260 -- __gllc_EvalCoord2d */
void __stdcall __gllc_EvalCoord2d(GLdouble u, GLdouble v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EvalCoord2d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord2d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord2dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_EvalCoord2d_Rec *) dlop->data;
    data->u = u;
    data->v = v;
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord2dv);
}

struct __gllc_EvalCoord2dv_Rec {
    GLdouble    u[2];
};

/* 690ac2c0 -- __gllc_EvalCoord2dv */
void __stdcall __gllc_EvalCoord2dv(const GLdouble *u)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EvalCoord2dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord2dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord2dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_EvalCoord2dv_Rec *) dlop->data;
    data->u[0] = u[0];
    data->u[1] = u[1];
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord2dv);
}

struct __gllc_EvalCoord2f_Rec {
    GLfloat     u;
    GLfloat     v;
};

/* 690ac320 -- __gllc_EvalCoord2f */
void __stdcall __gllc_EvalCoord2f(GLfloat u, GLfloat v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EvalCoord2f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord2f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord2fv;
    data = (struct __gllc_EvalCoord2f_Rec *) dlop->data;
    data->u = u;
    data->v = v;
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord2fv);
}

struct __gllc_EvalCoord2fv_Rec {
    GLfloat     u[2];
};

/* 690ac370 -- __gllc_EvalCoord2fv */
void __stdcall __gllc_EvalCoord2fv(const GLfloat *u)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EvalCoord2fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalCoord2fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalCoord2fv;
    data = (struct __gllc_EvalCoord2fv_Rec *) dlop->data;
    data->u[0] = u[0];
    data->u[1] = u[1];
    __glDlistAppendOp(gc, dlop, __glle_EvalCoord2fv);
}

struct __gllc_EvalMesh1_Rec {
    GLenum      mode;
    GLint       i1;
    GLint       i2;
};

/* 690ac3c0 -- __gllc_EvalMesh1 */
void __stdcall __gllc_EvalMesh1(GLenum mode, GLint i1, GLint i2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EvalMesh1_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalMesh1_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalMesh1;
    data = (struct __gllc_EvalMesh1_Rec *) dlop->data;
    data->mode = mode;
    data->i1 = i1;
    data->i2 = i2;
    __glDlistAppendOp(gc, dlop, __glle_EvalMesh1);
}

struct __gllc_EvalPoint1_Rec {
    GLint       i;
};

/* 690ac410 -- __gllc_EvalPoint1 */
void __stdcall __gllc_EvalPoint1(GLint i)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EvalPoint1_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalPoint1_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalPoint1;
    data = (struct __gllc_EvalPoint1_Rec *) dlop->data;
    data->i = i;
    __glDlistAppendOp(gc, dlop, __glle_EvalPoint1);
}

struct __gllc_EvalMesh2_Rec {
    GLenum      mode;
    GLint       i1;
    GLint       i2;
    GLint       j1;
    GLint       j2;
};

/* 690ac460 -- __gllc_EvalMesh2 */
void __stdcall __gllc_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EvalMesh2_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalMesh2_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalMesh2;
    data = (struct __gllc_EvalMesh2_Rec *) dlop->data;
    data->mode = mode;
    data->i1 = i1;
    data->i2 = i2;
    data->j1 = j1;
    data->j2 = j2;
    __glDlistAppendOp(gc, dlop, __glle_EvalMesh2);
}

struct __gllc_EvalPoint2_Rec {
    GLint       i;
    GLint       j;
};

/* 690ac4c0 -- __gllc_EvalPoint2 */
void __stdcall __gllc_EvalPoint2(GLint i, GLint j)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EvalPoint2_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EvalPoint2_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EvalPoint2;
    data = (struct __gllc_EvalPoint2_Rec *) dlop->data;
    data->i = i;
    data->j = j;
    __glDlistAppendOp(gc, dlop, __glle_EvalPoint2);
}

struct __gllc_AlphaFunc_Rec {
    GLenum      func;
    GLclampf    ref;
};

/* 690ac510 -- __gllc_AlphaFunc */
void __stdcall __gllc_AlphaFunc(GLenum func, GLclampf ref)
{
    __GLdlistOpICD *dlop;
    struct __gllc_AlphaFunc_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_AlphaFunc_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_AlphaFunc;
    data = (struct __gllc_AlphaFunc_Rec *) dlop->data;
    data->func = func;
    data->ref = ref;
    __glDlistAppendOp(gc, dlop, __glle_AlphaFunc);
}

struct __gllc_BlendFunc_Rec {
    GLenum      sfactor;
    GLenum      dfactor;
};

/* 690ac560 -- __gllc_BlendFunc */
void __stdcall __gllc_BlendFunc(GLenum sfactor, GLenum dfactor)
{
    __GLdlistOpICD *dlop;
    struct __gllc_BlendFunc_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_BlendFunc_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_BlendFunc;
    data = (struct __gllc_BlendFunc_Rec *) dlop->data;
    data->sfactor = sfactor;
    data->dfactor = dfactor;
    __glDlistAppendOp(gc, dlop, __glle_BlendFunc);
}

struct __gllc_LogicOp_Rec {
    GLenum      opcode;
};

/* 690ac5b0 -- __gllc_LogicOp */
void __stdcall __gllc_LogicOp(GLenum opcode)
{
    __GLdlistOpICD *dlop;
    struct __gllc_LogicOp_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_LogicOp_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_LogicOp;
    data = (struct __gllc_LogicOp_Rec *) dlop->data;
    data->opcode = opcode;
    __glDlistAppendOp(gc, dlop, __glle_LogicOp);
}

struct __gllc_StencilFunc_Rec {
    GLenum      func;
    GLint       ref;
    GLuint      mask;
};

/* 690ac600 -- __gllc_StencilFunc */
void __stdcall __gllc_StencilFunc(GLenum func, GLint ref, GLuint mask)
{
    __GLdlistOpICD *dlop;
    struct __gllc_StencilFunc_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_StencilFunc_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_StencilFunc;
    data = (struct __gllc_StencilFunc_Rec *) dlop->data;
    data->func = func;
    data->ref = ref;
    data->mask = mask;
    __glDlistAppendOp(gc, dlop, __glle_StencilFunc);
}

struct __gllc_StencilOp_Rec {
    GLenum      fail;
    GLenum      zfail;
    GLenum      zpass;
};

/* 690ac650 -- __gllc_StencilOp */
void __stdcall __gllc_StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    __GLdlistOpICD *dlop;
    struct __gllc_StencilOp_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_StencilOp_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_StencilOp;
    data = (struct __gllc_StencilOp_Rec *) dlop->data;
    data->fail = fail;
    data->zfail = zfail;
    data->zpass = zpass;
    __glDlistAppendOp(gc, dlop, __glle_StencilOp);
}

struct __gllc_DepthFunc_Rec {
    GLenum      func;
};

/* 690ac6a0 -- __gllc_DepthFunc */
void __stdcall __gllc_DepthFunc(GLenum func)
{
    __GLdlistOpICD *dlop;
    struct __gllc_DepthFunc_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_DepthFunc_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_DepthFunc;
    data = (struct __gllc_DepthFunc_Rec *) dlop->data;
    data->func = func;
    __glDlistAppendOp(gc, dlop, __glle_DepthFunc);
}

struct __gllc_PixelZoom_Rec {
    GLfloat     xfactor;
    GLfloat     yfactor;
};

/* 690ac6f0 -- __gllc_PixelZoom */
void __stdcall __gllc_PixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    __GLdlistOpICD *dlop;
    struct __gllc_PixelZoom_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PixelZoom_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PixelZoom;
    data = (struct __gllc_PixelZoom_Rec *) dlop->data;
    data->xfactor = xfactor;
    data->yfactor = yfactor;
    __glDlistAppendOp(gc, dlop, __glle_PixelZoom);
}

struct __gllc_PixelTransferf_Rec {
    GLenum      pname;
    GLfloat     param;
};

/* 690ac740 -- __gllc_PixelTransferf */
void __stdcall __gllc_PixelTransferf(GLenum pname, GLfloat param)
{
    __GLdlistOpICD *dlop;
    struct __gllc_PixelTransferf_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PixelTransferf_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PixelTransferf;
    data = (struct __gllc_PixelTransferf_Rec *) dlop->data;
    data->pname = pname;
    data->param = param;
    __glDlistAppendOp(gc, dlop, __glle_PixelTransferf);
}

struct __gllc_PixelTransferi_Rec {
    GLenum      pname;
    GLint       param;
};

/* 690ac790 -- __gllc_PixelTransferi */
void __stdcall __gllc_PixelTransferi(GLenum pname, GLint param)
{
    __GLdlistOpICD *dlop;
    struct __gllc_PixelTransferi_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PixelTransferi_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PixelTransferi;
    data = (struct __gllc_PixelTransferi_Rec *) dlop->data;
    data->pname = pname;
    data->param = param;
    __glDlistAppendOp(gc, dlop, __glle_PixelTransferi);
}

struct __gllc_ReadBuffer_Rec {
    GLenum      mode;
};

/* 690ac960 -- __gllc_ReadBuffer */
void __stdcall __gllc_ReadBuffer(GLenum mode)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ReadBuffer_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ReadBuffer_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ReadBuffer;
    data = (struct __gllc_ReadBuffer_Rec *) dlop->data;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_ReadBuffer);
}

struct __gllc_CopyPixels_Rec {
    GLint       x;
    GLint       y;
    GLsizei     width;
    GLsizei     height;
    GLenum      type;
};

/* 690ac9b0 -- __gllc_CopyPixels */
void __stdcall __gllc_CopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    __GLdlistOpICD *dlop;
    struct __gllc_CopyPixels_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_CopyPixels_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CopyPixels;
    data = (struct __gllc_CopyPixels_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
    data->type = type;
    __glDlistAppendOp(gc, dlop, __glle_CopyPixels);
}

struct __gllc_DepthRange_Rec {
    GLclampd    zNear;
    GLclampd    zFar;
};

/* 690aca10 -- __gllc_DepthRange */
void __stdcall __gllc_DepthRange(GLclampd zNear, GLclampd zFar)
{
    __GLdlistOpICD *dlop;
    struct __gllc_DepthRange_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_DepthRange_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_DepthRange;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_DepthRange_Rec *) dlop->data;
    data->zNear = zNear;
    data->zFar = zFar;
    __glDlistAppendOp(gc, dlop, __glle_DepthRange);
}

struct __gllc_Frustum_Rec {
    GLdouble    left;
    GLdouble    right;
    GLdouble    bottom;
    GLdouble    top;
    GLdouble    zNear;
    GLdouble    zFar;
};

/* 690aca70 -- __gllc_Frustum */
void __stdcall __gllc_Frustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Frustum_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Frustum_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Frustum;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Frustum_Rec *) dlop->data;
    data->left = left;
    data->right = right;
    data->bottom = bottom;
    data->top = top;
    data->zNear = zNear;
    data->zFar = zFar;
    __glDlistAppendOp(gc, dlop, __glle_Frustum);
}

/* 690acb00 -- __gllc_LoadIdentity */
void __fastcall __gllc_LoadIdentity(void)
{
    __GLdlistOpICD *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_LoadIdentity;
    __glDlistAppendOp(gc, dlop, __glle_LoadIdentity);
}

struct __gllc_MatrixMode_Rec {
    GLenum      mode;
};

/* 690acbf0 -- __gllc_MatrixMode */
void __stdcall __gllc_MatrixMode(GLenum mode)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MatrixMode_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MatrixMode_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MatrixMode;
    data = (struct __gllc_MatrixMode_Rec *) dlop->data;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_MatrixMode);
}

struct __gllc_Ortho_Rec {
    GLdouble    left;
    GLdouble    right;
    GLdouble    bottom;
    GLdouble    top;
    GLdouble    zNear;
    GLdouble    zFar;
};

/* 690accf0 -- __gllc_Ortho */
void __stdcall __gllc_Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Ortho_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Ortho_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Ortho;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Ortho_Rec *) dlop->data;
    data->left = left;
    data->right = right;
    data->bottom = bottom;
    data->top = top;
    data->zNear = zNear;
    data->zFar = zFar;
    __glDlistAppendOp(gc, dlop, __glle_Ortho);
}

/* 690acd80 -- __gllc_PopMatrix */
void __fastcall __gllc_PopMatrix(void)
{
    __GLdlistOpICD *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_PopMatrix;
    __glDlistAppendOp(gc, dlop, __glle_PopMatrix);
}

/* 690acdc0 -- __gllc_PushMatrix */
void __fastcall __gllc_PushMatrix(void)
{
    __GLdlistOpICD *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_PushMatrix;
    __glDlistAppendOp(gc, dlop, __glle_PushMatrix);
}

struct __gllc_Rotated_Rec {
    GLdouble    angle;
    GLdouble    x;
    GLdouble    y;
    GLdouble    z;
};

/* 690ace00 -- __gllc_Rotated */
void __stdcall __gllc_Rotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Rotated_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rotated_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Rotated;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Rotated_Rec *) dlop->data;
    data->angle = angle;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, dlop, __glle_Rotated);
}

struct __gllc_Rotatef_Rec {
    GLfloat     angle;
    GLfloat     x;
    GLfloat     y;
    GLfloat     z;
};

/* 690ace80 -- __gllc_Rotatef */
void __stdcall __gllc_Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Rotatef_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rotatef_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Rotatef;
    data = (struct __gllc_Rotatef_Rec *) dlop->data;
    data->angle = angle;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, dlop, __glle_Rotatef);
}

struct __gllc_Scaled_Rec {
    GLdouble    x;
    GLdouble    y;
    GLdouble    z;
};

/* 690acee0 -- __gllc_Scaled */
void __stdcall __gllc_Scaled(GLdouble x, GLdouble y, GLdouble z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Scaled_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Scaled_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Scaled;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Scaled_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, dlop, __glle_Scaled);
}

struct __gllc_Scalef_Rec {
    GLfloat     x;
    GLfloat     y;
    GLfloat     z;
};

/* 690acf50 -- __gllc_Scalef */
void __stdcall __gllc_Scalef(GLfloat x, GLfloat y, GLfloat z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Scalef_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Scalef_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Scalef;
    data = (struct __gllc_Scalef_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, dlop, __glle_Scalef);
}

struct __gllc_Translated_Rec {
    GLdouble    x;
    GLdouble    y;
    GLdouble    z;
};

/* 690acfa0 -- __gllc_Translated */
void __stdcall __gllc_Translated(GLdouble x, GLdouble y, GLdouble z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Translated_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Translated_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Translated;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Translated_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, dlop, __glle_Translated);
}

struct __gllc_Translatef_Rec {
    GLfloat     x;
    GLfloat     y;
    GLfloat     z;
};

/* 690ad010 -- __gllc_Translatef */
void __stdcall __gllc_Translatef(GLfloat x, GLfloat y, GLfloat z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Translatef_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Translatef_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Translatef;
    data = (struct __gllc_Translatef_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    __glDlistAppendOp(gc, dlop, __glle_Translatef);
}

struct __gllc_Viewport_Rec {
    GLint       x;
    GLint       y;
    GLsizei     width;
    GLsizei     height;
};

/* 690ad060 -- __gllc_Viewport */
void __stdcall __gllc_Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Viewport_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Viewport_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Viewport;
    data = (struct __gllc_Viewport_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
    __glDlistAppendOp(gc, dlop, __glle_Viewport);
}

struct __gllc_CopyColorTableEXT_Rec {
    GLenum      target;
    GLenum      internalformat;
    GLint       x;
    GLint       y;
    GLsizei     width;
};

/* 690ad0c0 -- __gllc_CopyColorTableEXT */
void __stdcall __gllc_CopyColorTableEXT(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    __GLdlistOpICD *dlop;
    struct __gllc_CopyColorTableEXT_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_CopyColorTableEXT_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CopyColorTableEXT;
    data = (struct __gllc_CopyColorTableEXT_Rec *) dlop->data;
    data->target = target;
    data->internalformat = internalformat;
    data->x = x;
    data->y = y;
    data->width = width;
    __glDlistAppendOp(gc, dlop, __glle_CopyColorTableEXT);
}

struct __gllc_PolygonOffset_Rec {
    GLfloat     factor;
    GLfloat     units;
};

/* 690ad120 -- __gllc_PolygonOffset */
void __stdcall __gllc_PolygonOffset(GLfloat factor, GLfloat units)
{
    __GLdlistOpICD *dlop;
    struct __gllc_PolygonOffset_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PolygonOffset_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PolygonOffset;
    data = (struct __gllc_PolygonOffset_Rec *) dlop->data;
    data->factor = factor;
    data->units = units;
    __glDlistAppendOp(gc, dlop, __glle_PolygonOffset);
}

struct __gllc_CopyTexImage1D_Rec {
    GLenum      target;
    GLint       level;
    GLenum      internalFormat;
    GLint       x;
    GLint       y;
    GLsizei     width;
    GLint       border;
};

/* 690ad170 -- __gllc_CopyTexImage1D */
void __stdcall __gllc_CopyTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border)
{
    __GLdlistOpICD *dlop;
    struct __gllc_CopyTexImage1D_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_CopyTexImage1D_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CopyTexImage1D;
    data = (struct __gllc_CopyTexImage1D_Rec *) dlop->data;
    data->target = target;
    data->level = level;
    data->internalFormat = internalFormat;
    data->x = x;
    data->y = y;
    data->width = width;
    data->border = border;
    __glDlistAppendOp(gc, dlop, __glle_CopyTexImage1D);
}

struct __gllc_CopyTexImage2D_Rec {
    GLenum      target;
    GLint       level;
    GLenum      internalFormat;
    GLint       x;
    GLint       y;
    GLsizei     width;
    GLsizei     height;
    GLint       border;
};

/* 690ad1e0 -- __gllc_CopyTexImage2D */
void __stdcall __gllc_CopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    __GLdlistOpICD *dlop;
    struct __gllc_CopyTexImage2D_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_CopyTexImage2D_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CopyTexImage2D;
    data = (struct __gllc_CopyTexImage2D_Rec *) dlop->data;
    data->target = target;
    data->level = level;
    data->internalFormat = internalFormat;
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
    data->border = border;
    __glDlistAppendOp(gc, dlop, __glle_CopyTexImage2D);
}

struct __gllc_CopyTexSubImage1D_Rec {
    GLenum      target;
    GLint       level;
    GLint       xoffset;
    GLint       x;
    GLint       y;
    GLsizei     width;
};

/* 690ad250 -- __gllc_CopyTexSubImage1D */
void __stdcall __gllc_CopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    __GLdlistOpICD *dlop;
    struct __gllc_CopyTexSubImage1D_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_CopyTexSubImage1D_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CopyTexSubImage1D;
    data = (struct __gllc_CopyTexSubImage1D_Rec *) dlop->data;
    data->target = target;
    data->level = level;
    data->xoffset = xoffset;
    data->x = x;
    data->y = y;
    data->width = width;
    __glDlistAppendOp(gc, dlop, __glle_CopyTexSubImage1D);
}

struct __gllc_CopyTexSubImage2D_Rec {
    GLenum      target;
    GLint       level;
    GLint       xoffset;
    GLint       yoffset;
    GLint       x;
    GLint       y;
    GLsizei     width;
    GLsizei     height;
};

/* 690ad2c0 -- __gllc_CopyTexSubImage2D */
void __stdcall __gllc_CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    __GLdlistOpICD *dlop;
    struct __gllc_CopyTexSubImage2D_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_CopyTexSubImage2D_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CopyTexSubImage2D;
    data = (struct __gllc_CopyTexSubImage2D_Rec *) dlop->data;
    data->target = target;
    data->level = level;
    data->xoffset = xoffset;
    data->yoffset = yoffset;
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
    __glDlistAppendOp(gc, dlop, __glle_CopyTexSubImage2D);
}

struct __gllc_BindTexture_Rec {
    GLenum      target;
    GLuint      texture;
};

/* 690ad330 -- __gllc_BindTexture */
void __stdcall __gllc_BindTexture(GLenum target, GLuint texture)
{
    __GLdlistOpICD *dlop;
    struct __gllc_BindTexture_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_BindTexture_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_BindTexture;
    data = (struct __gllc_BindTexture_Rec *) dlop->data;
    data->target = target;
    data->texture = texture;
    __glDlistAppendOp(gc, dlop, __glle_BindTexture);
}

struct __gllc_Indexub_Rec {
    GLubyte     c;
    GLubyte     pad1;
    GLubyte     pad2;
    GLubyte     pad3;
};

/* 690ad410 -- __gllc_Indexub */
void __stdcall __gllc_Indexub(GLubyte c)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Indexub_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexub_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexubv;
    data = (struct __gllc_Indexub_Rec *) dlop->data;
    data->c = c;
    __glDlistAppendOp(gc, dlop, __glle_Indexubv);
}

struct __gllc_Indexubv_Rec {
    GLubyte     c[1];
    GLubyte     pad1;
    GLubyte     pad2;
    GLubyte     pad3;
};

/* 690ad460 -- __gllc_Indexubv */
void __stdcall __gllc_Indexubv(const GLubyte *c)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Indexubv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexubv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexubv;
    data = (struct __gllc_Indexubv_Rec *) dlop->data;
    data->c[0] = c[0];
    __glDlistAppendOp(gc, dlop, __glle_Indexubv);
}

struct __gllc_IndexFuncSGI_Rec {
    GLenum      func;
    GLclampf    ref;
};

/* 690ad4b0 -- __gllc_IndexFuncSGI */
void __stdcall __gllc_IndexFuncSGI(GLenum func, GLclampf ref)
{
    __GLdlistOpICD *dlop;
    struct __gllc_IndexFuncSGI_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_IndexFuncSGI_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_IndexFuncSGI;
    data = (struct __gllc_IndexFuncSGI_Rec *) dlop->data;
    data->func = func;
    data->ref = ref;
    __glDlistAppendOp(gc, dlop, __glle_IndexFuncSGI);
}

struct __gllc_IndexMaterialSGI_Rec {
    GLenum      face;
    GLenum      mode;
};

/* 690ad500 -- __gllc_IndexMaterialSGI */
void __stdcall __gllc_IndexMaterialSGI(GLenum face, GLenum mode)
{
    __GLdlistOpICD *dlop;
    struct __gllc_IndexMaterialSGI_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_IndexMaterialSGI_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_IndexMaterialSGI;
    data = (struct __gllc_IndexMaterialSGI_Rec *) dlop->data;
    data->face = face;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_IndexMaterialSGI);
}

struct __gllc_ActiveTextureARB_Rec {
    GLenum      texture;
};

/* 690ad550 -- __gllc_ActiveTextureARB */
void __stdcall __gllc_ActiveTextureARB(GLenum texture)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ActiveTextureARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ActiveTextureARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ActiveTextureARB;
    data = (struct __gllc_ActiveTextureARB_Rec *) dlop->data;
    data->texture = texture;
    __glDlistAppendOp(gc, dlop, __glle_ActiveTextureARB);
}

struct __gllc_ClientActiveTextureARB_Rec {
    GLenum      texture;
};

/* 690ad5a0 -- __gllc_ClientActiveTextureARB */
void __stdcall __gllc_ClientActiveTextureARB(GLenum texture)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ClientActiveTextureARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ClientActiveTextureARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ClientActiveTextureARB;
    data = (struct __gllc_ClientActiveTextureARB_Rec *) dlop->data;
    data->texture = texture;
    __glDlistAppendOp(gc, dlop, __glle_ClientActiveTextureARB);
}

struct __gllc_MultiTexCoord1dARB_Rec {
    GLdouble    s;
    GLenum      target;
    GLuint      pad1;
};

/* 690ad5f0 -- __gllc_MultiTexCoord1dARB */
void __stdcall __gllc_MultiTexCoord1dARB(GLenum target, GLdouble s)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord1dARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord1dARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord1dARB;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_MultiTexCoord1dARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord1dARB);
}

struct __gllc_MultiTexCoord1dvARB_Rec {
    GLdouble    v[1];
    GLenum      target;
    GLuint      pad1;
};

/* 690ad650 -- __gllc_MultiTexCoord1dvARB */
void __stdcall __gllc_MultiTexCoord1dvARB(GLenum target, const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord1dvARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord1dvARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord1dvARB;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_MultiTexCoord1dvARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord1dvARB);
}

struct __gllc_MultiTexCoord1fARB_Rec {
    GLenum      target;
    GLfloat     s;
};

/* 690ad6b0 -- __gllc_MultiTexCoord1fARB */
void __stdcall __gllc_MultiTexCoord1fARB(GLenum target, GLfloat s)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord1fARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord1fARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord1fARB;
    data = (struct __gllc_MultiTexCoord1fARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord1fARB);
}

struct __gllc_MultiTexCoord1fvARB_Rec {
    GLenum      target;
    GLfloat     v[1];
};

/* 690ad700 -- __gllc_MultiTexCoord1fvARB */
void __stdcall __gllc_MultiTexCoord1fvARB(GLenum target, const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord1fvARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord1fvARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord1fvARB;
    data = (struct __gllc_MultiTexCoord1fvARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord1fvARB);
}

struct __gllc_MultiTexCoord1iARB_Rec {
    GLenum      target;
    GLint       s;
};

/* 690ad750 -- __gllc_MultiTexCoord1iARB */
void __stdcall __gllc_MultiTexCoord1iARB(GLenum target, GLint s)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord1iARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord1iARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord1iARB;
    data = (struct __gllc_MultiTexCoord1iARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord1iARB);
}

struct __gllc_MultiTexCoord1ivARB_Rec {
    GLenum      target;
    GLint       v[1];
};

/* 690ad7a0 -- __gllc_MultiTexCoord1ivARB */
void __stdcall __gllc_MultiTexCoord1ivARB(GLenum target, const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord1ivARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord1ivARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord1ivARB;
    data = (struct __gllc_MultiTexCoord1ivARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord1ivARB);
}

struct __gllc_MultiTexCoord1sARB_Rec {
    GLenum      target;
    GLshort     s;
    GLushort    pad1;
};

/* 690ad7f0 -- __gllc_MultiTexCoord1sARB */
void __stdcall __gllc_MultiTexCoord1sARB(GLenum target, GLshort s)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord1sARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord1sARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord1sARB;
    data = (struct __gllc_MultiTexCoord1sARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord1sARB);
}

struct __gllc_MultiTexCoord1svARB_Rec {
    GLenum      target;
    GLshort     v[1];
    GLushort    pad1;
};

/* 690ad840 -- __gllc_MultiTexCoord1svARB */
void __stdcall __gllc_MultiTexCoord1svARB(GLenum target, const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord1svARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord1svARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord1svARB;
    data = (struct __gllc_MultiTexCoord1svARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord1svARB);
}

struct __gllc_MultiTexCoord2dARB_Rec {
    GLdouble    s;
    GLdouble    t;
    GLenum      target;
    GLuint      pad1;
};

/* 690ad890 -- __gllc_MultiTexCoord2dARB */
void __stdcall __gllc_MultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord2dARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord2dARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord2dARB;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_MultiTexCoord2dARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    data->t = t;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord2dARB);
}

struct __gllc_MultiTexCoord2dvARB_Rec {
    GLdouble    v[2];
    GLenum      target;
    GLuint      pad1;
};

/* 690ad8f0 -- __gllc_MultiTexCoord2dvARB */
void __stdcall __gllc_MultiTexCoord2dvARB(GLenum target, const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord2dvARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord2dvARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord2dvARB;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_MultiTexCoord2dvARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    data->v[1] = v[1];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord2dvARB);
}

struct __gllc_MultiTexCoord2fARB_Rec {
    GLenum      target;
    GLfloat     s;
    GLfloat     t;
};

/* 690ad960 -- __gllc_MultiTexCoord2fARB */
void __stdcall __gllc_MultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord2fARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord2fARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord2fARB;
    data = (struct __gllc_MultiTexCoord2fARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    data->t = t;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord2fARB);
}

struct __gllc_MultiTexCoord2fvARB_Rec {
    GLenum      target;
    GLfloat     v[2];
};

/* 690ad9b0 -- __gllc_MultiTexCoord2fvARB */
void __stdcall __gllc_MultiTexCoord2fvARB(GLenum target, const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord2fvARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord2fvARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord2fvARB;
    data = (struct __gllc_MultiTexCoord2fvARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    data->v[1] = v[1];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord2fvARB);
}

struct __gllc_MultiTexCoord2iARB_Rec {
    GLenum      target;
    GLint       s;
    GLint       t;
};

/* 690ada10 -- __gllc_MultiTexCoord2iARB */
void __stdcall __gllc_MultiTexCoord2iARB(GLenum target, GLint s, GLint t)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord2iARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord2iARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord2iARB;
    data = (struct __gllc_MultiTexCoord2iARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    data->t = t;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord2iARB);
}

struct __gllc_MultiTexCoord2ivARB_Rec {
    GLenum      target;
    GLint       v[2];
};

/* 690ada60 -- __gllc_MultiTexCoord2ivARB */
void __stdcall __gllc_MultiTexCoord2ivARB(GLenum target, const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord2ivARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord2ivARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord2ivARB;
    data = (struct __gllc_MultiTexCoord2ivARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    data->v[1] = v[1];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord2ivARB);
}

struct __gllc_MultiTexCoord2sARB_Rec {
    GLenum      target;
    GLshort     s;
    GLshort     t;
};

/* 690adac0 -- __gllc_MultiTexCoord2sARB */
void __stdcall __gllc_MultiTexCoord2sARB(GLenum target, GLshort s, GLshort t)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord2sARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord2sARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord2sARB;
    data = (struct __gllc_MultiTexCoord2sARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    data->t = t;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord2sARB);
}

struct __gllc_MultiTexCoord2svARB_Rec {
    GLenum      target;
    GLshort     v[2];
};

/* 690adb20 -- __gllc_MultiTexCoord2svARB */
void __stdcall __gllc_MultiTexCoord2svARB(GLenum target, const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord2svARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord2svARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord2svARB;
    data = (struct __gllc_MultiTexCoord2svARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    data->v[1] = v[1];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord2svARB);
}

struct __gllc_MultiTexCoord3dARB_Rec {
    GLdouble    s;
    GLdouble    t;
    GLdouble    r;
    GLenum      target;
    GLuint      pad1;
};

/* 690adb80 -- __gllc_MultiTexCoord3dARB */
void __stdcall __gllc_MultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord3dARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord3dARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord3dARB;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_MultiTexCoord3dARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    data->t = t;
    data->r = r;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord3dARB);
}

struct __gllc_MultiTexCoord3dvARB_Rec {
    GLdouble    v[3];
    GLenum      target;
    GLuint      pad1;
};

/* 690adbf0 -- __gllc_MultiTexCoord3dvARB */
void __stdcall __gllc_MultiTexCoord3dvARB(GLenum target, const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord3dvARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord3dvARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord3dvARB;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_MultiTexCoord3dvARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord3dvARB);
}

struct __gllc_MultiTexCoord3fARB_Rec {
    GLenum      target;
    GLfloat     s;
    GLfloat     t;
    GLfloat     r;
};

/* 690adc60 -- __gllc_MultiTexCoord3fARB */
void __stdcall __gllc_MultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord3fARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord3fARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord3fARB;
    data = (struct __gllc_MultiTexCoord3fARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    data->t = t;
    data->r = r;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord3fARB);
}

struct __gllc_MultiTexCoord3fvARB_Rec {
    GLenum      target;
    GLfloat     v[3];
};

/* 690adcc0 -- __gllc_MultiTexCoord3fvARB */
void __stdcall __gllc_MultiTexCoord3fvARB(GLenum target, const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord3fvARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord3fvARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord3fvARB;
    data = (struct __gllc_MultiTexCoord3fvARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord3fvARB);
}

struct __gllc_MultiTexCoord3iARB_Rec {
    GLenum      target;
    GLint       s;
    GLint       t;
    GLint       r;
};

/* 690add20 -- __gllc_MultiTexCoord3iARB */
void __stdcall __gllc_MultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord3iARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord3iARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord3iARB;
    data = (struct __gllc_MultiTexCoord3iARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    data->t = t;
    data->r = r;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord3iARB);
}

struct __gllc_MultiTexCoord3ivARB_Rec {
    GLenum      target;
    GLint       v[3];
};

/* 690add80 -- __gllc_MultiTexCoord3ivARB */
void __stdcall __gllc_MultiTexCoord3ivARB(GLenum target, const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord3ivARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord3ivARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord3ivARB;
    data = (struct __gllc_MultiTexCoord3ivARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord3ivARB);
}

struct __gllc_MultiTexCoord3sARB_Rec {
    GLenum      target;
    GLshort     s;
    GLshort     t;
    GLshort     r;
    GLushort    pad1;
};

/* 690adde0 -- __gllc_MultiTexCoord3sARB */
void __stdcall __gllc_MultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord3sARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord3sARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord3sARB;
    data = (struct __gllc_MultiTexCoord3sARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    data->t = t;
    data->r = r;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord3sARB);
}

struct __gllc_MultiTexCoord3svARB_Rec {
    GLenum      target;
    GLshort     v[3];
    GLushort    pad1;
};

/* 690ade40 -- __gllc_MultiTexCoord3svARB */
void __stdcall __gllc_MultiTexCoord3svARB(GLenum target, const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord3svARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord3svARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord3svARB;
    data = (struct __gllc_MultiTexCoord3svARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord3svARB);
}

struct __gllc_MultiTexCoord4dARB_Rec {
    GLdouble    s;
    GLdouble    t;
    GLdouble    r;
    GLdouble    q;
    GLenum      target;
    GLuint      pad1;
};

/* 690adea0 -- __gllc_MultiTexCoord4dARB */
void __stdcall __gllc_MultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord4dARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord4dARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord4dARB;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_MultiTexCoord4dARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    data->t = t;
    data->r = r;
    data->q = q;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord4dARB);
}

struct __gllc_MultiTexCoord4dvARB_Rec {
    GLdouble    v[4];
    GLenum      target;
    GLuint      pad1;
};

/* 690adf20 -- __gllc_MultiTexCoord4dvARB */
void __stdcall __gllc_MultiTexCoord4dvARB(GLenum target, const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord4dvARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord4dvARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord4dvARB;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_MultiTexCoord4dvARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord4dvARB);
}

struct __gllc_MultiTexCoord4fARB_Rec {
    GLenum      target;
    GLfloat     s;
    GLfloat     t;
    GLfloat     r;
    GLfloat     q;
};

/* 690adfa0 -- __gllc_MultiTexCoord4fARB */
void __stdcall __gllc_MultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord4fARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord4fARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord4fARB;
    data = (struct __gllc_MultiTexCoord4fARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    data->t = t;
    data->r = r;
    data->q = q;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord4fARB);
}

struct __gllc_MultiTexCoord4fvARB_Rec {
    GLenum      target;
    GLfloat     v[4];
};

/* 690ae000 -- __gllc_MultiTexCoord4fvARB */
void __stdcall __gllc_MultiTexCoord4fvARB(GLenum target, const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord4fvARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord4fvARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord4fvARB;
    data = (struct __gllc_MultiTexCoord4fvARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord4fvARB);
}

struct __gllc_MultiTexCoord4iARB_Rec {
    GLenum      target;
    GLint       s;
    GLint       t;
    GLint       r;
    GLint       q;
};

/* 690ae060 -- __gllc_MultiTexCoord4iARB */
void __stdcall __gllc_MultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord4iARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord4iARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord4iARB;
    data = (struct __gllc_MultiTexCoord4iARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    data->t = t;
    data->r = r;
    data->q = q;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord4iARB);
}

struct __gllc_MultiTexCoord4ivARB_Rec {
    GLenum      target;
    GLint       v[4];
};

/* 690ae0c0 -- __gllc_MultiTexCoord4ivARB */
void __stdcall __gllc_MultiTexCoord4ivARB(GLenum target, const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord4ivARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord4ivARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord4ivARB;
    data = (struct __gllc_MultiTexCoord4ivARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord4ivARB);
}

struct __gllc_MultiTexCoord4sARB_Rec {
    GLenum      target;
    GLshort     s;
    GLshort     t;
    GLshort     r;
    GLshort     q;
};

/* 690ae120 -- __gllc_MultiTexCoord4sARB */
void __stdcall __gllc_MultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord4sARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord4sARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord4sARB;
    data = (struct __gllc_MultiTexCoord4sARB_Rec *) dlop->data;
    data->target = target;
    data->s = s;
    data->t = t;
    data->r = r;
    data->q = q;
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord4sARB);
}

struct __gllc_MultiTexCoord4svARB_Rec {
    GLenum      target;
    GLshort     v[4];
};

/* 690ae190 -- __gllc_MultiTexCoord4svARB */
void __stdcall __gllc_MultiTexCoord4svARB(GLenum target, const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MultiTexCoord4svARB_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MultiTexCoord4svARB_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_MultiTexCoord4svARB;
    data = (struct __gllc_MultiTexCoord4svARB_Rec *) dlop->data;
    data->target = target;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    __glDlistAppendOp(gc, dlop, __glle_MultiTexCoord4svARB);
}

struct __gllc_PointParameterfEXT_Rec {
    GLenum      pname;
    GLfloat     param;
};

/* 690ae200 -- __gllc_PointParameterfEXT */
void __stdcall __gllc_PointParameterfEXT(GLenum pname, GLfloat param)
{
    __GLdlistOpICD *dlop;
    struct __gllc_PointParameterfEXT_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PointParameterfEXT_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_PointParameterfEXT;
    data = (struct __gllc_PointParameterfEXT_Rec *) dlop->data;
    data->pname = pname;
    data->param = param;
    __glDlistAppendOp(gc, dlop, __glle_PointParameterfEXT);
}

struct __gllc_BlendEquationEXT_Rec {
    GLenum      mode;
};

/* 690ae2d0 -- __gllc_BlendEquationEXT */
void __stdcall __gllc_BlendEquationEXT(GLenum mode)
{
    __GLdlistOpICD *dlop;
    struct __gllc_BlendEquationEXT_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_BlendEquationEXT_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_BlendEquationEXT;
    data = (struct __gllc_BlendEquationEXT_Rec *) dlop->data;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_BlendEquationEXT);
}

struct __gllc_TBufferMask3DFX_Rec {
    GLuint      mask;
};

/* 690ae580 -- __gllc_TBufferMask3DFX */
void __stdcall __gllc_TBufferMask3DFX(GLuint mask)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TBufferMask3DFX_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TBufferMask3DFX_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_TBufferMask3DFX;
    data = (struct __gllc_TBufferMask3DFX_Rec *) dlop->data;
    data->mask = mask;
    __glDlistAppendOp(gc, dlop, __glle_TBufferMask3DFX);
}

struct __gllc_FogCoordfEXT_Rec {
    GLfloat     coord;
};

/* 690ae5d0 -- __gllc_FogCoordfEXT */
void __stdcall __gllc_FogCoordfEXT(GLfloat coord)
{
    __GLdlistOpICD *dlop;
    struct __gllc_FogCoordfEXT_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_FogCoordfEXT_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_FogCoordfEXT;
    data = (struct __gllc_FogCoordfEXT_Rec *) dlop->data;
    data->coord = coord;
    __glDlistAppendOp(gc, dlop, __glle_FogCoordfEXT);
}

struct __gllc_FogCoorddEXT_Rec {
    GLdouble    coord;
};

/* 690ae620 -- __gllc_FogCoorddEXT */
void __stdcall __gllc_FogCoorddEXT(GLdouble coord)
{
    __GLdlistOpICD *dlop;
    struct __gllc_FogCoorddEXT_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_FogCoorddEXT_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_FogCoorddEXT;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_FogCoorddEXT_Rec *) dlop->data;
    data->coord = coord;
    __glDlistAppendOp(gc, dlop, __glle_FogCoorddEXT);
}

struct __gllc_FogCoordfvEXT_Rec {
    GLfloat     coord[1];
};

/* 690ae670 -- __gllc_FogCoordfvEXT */
void __stdcall __gllc_FogCoordfvEXT(const GLfloat *coord)
{
    __GLdlistOpICD *dlop;
    struct __gllc_FogCoordfvEXT_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_FogCoordfvEXT_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_FogCoordfvEXT;
    data = (struct __gllc_FogCoordfvEXT_Rec *) dlop->data;
    data->coord[0] = coord[0];
    __glDlistAppendOp(gc, dlop, __glle_FogCoordfvEXT);
}

struct __gllc_FogCoorddvEXT_Rec {
    GLdouble    coord[1];
};

/* 690ae6c0 -- __gllc_FogCoorddvEXT */
void __stdcall __gllc_FogCoorddvEXT(const GLdouble *coord)
{
    __GLdlistOpICD *dlop;
    struct __gllc_FogCoorddvEXT_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_FogCoorddvEXT_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_FogCoorddvEXT;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_FogCoorddvEXT_Rec *) dlop->data;
    data->coord[0] = coord[0];
    __glDlistAppendOp(gc, dlop, __glle_FogCoorddvEXT);
}

/************************************************************************/
/* s_varray.c copy procs (gc, element, vertex): the sizes not in si_s_varray.c / gl_varray.c.  The original compiled them assuming no aliasing (/Ow: the next element is loaded before
** the previous one is stored). */
#pragma optimize("w", on)

/* 690ae7e0 -- CopyVertex3s */
void __fastcall CopyVertex3s(__GLcontext *gc, const GLshort *vp, __GLvertexICD *v)
{
    v->obj.x = vp[0]; v->obj.y = vp[1]; v->obj.z = vp[2]; v->obj.w = 1.0F;
}

/* 690ae830 -- CopyVertex3i */
void __fastcall CopyVertex3i(__GLcontext *gc, const GLint *vp, __GLvertexICD *v)
{
    v->obj.x = vp[0]; v->obj.y = vp[1]; v->obj.z = vp[2]; v->obj.w = 1.0F;
}

/* 690ae860 -- CopyVertex3f */
void __fastcall CopyVertex3f(__GLcontext *gc, const GLfloat *vp, __GLvertexICD *v)
{
    v->obj.x = vp[0]; v->obj.y = vp[1]; v->obj.z = vp[2]; v->obj.w = 1.0F;
}

/* 690ae890 -- CopyVertex3d */
void __fastcall CopyVertex3d(__GLcontext *gc, const GLdouble *vp, __GLvertexICD *v)
{
    v->obj.x = vp[0]; v->obj.y = vp[1]; v->obj.z = vp[2]; v->obj.w = 1.0F;
}

/* 690ae8c0 -- CopyVertex4s */
void __fastcall CopyVertex4s(__GLcontext *gc, const GLshort *vp, __GLvertexICD *v)
{
    v->obj.x = vp[0]; v->obj.y = vp[1]; v->obj.z = vp[2]; v->obj.w = vp[3];
}

/* 690ae940 -- CopyVertex4f */
void __fastcall CopyVertex4f(__GLcontext *gc, const GLfloat *vp, __GLvertexICD *v)
{
    v->obj.x = vp[0]; v->obj.y = vp[1]; v->obj.z = vp[2]; v->obj.w = vp[3];
}

/* 690aea50 -- CopyNormal3f */
void __fastcall CopyNormal3f(__GLcontext *gc, const GLfloat *np, __GLvertexICD *v)
{
    v->normal.x = np[0]; v->normal.y = np[1]; v->normal.z = np[2];
}

/* 690aed10 -- CopyColor3f */
void __fastcall CopyColor3f(__GLcontext *gc, const GLfloat *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = cp[0];
    v->colors[__GL_FRONTFACE].g = cp[1];
    v->colors[__GL_FRONTFACE].b = cp[2];
    v->colors[__GL_FRONTFACE].a = 1.0F;
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}

/* 690af030 -- CopyColor4f */
void __fastcall CopyColor4f(__GLcontext *gc, const GLfloat *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = cp[0];
    v->colors[__GL_FRONTFACE].g = cp[1];
    v->colors[__GL_FRONTFACE].b = cp[2];
    v->colors[__GL_FRONTFACE].a = cp[3];
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}
#pragma optimize("", on)

/* 690af430 -- CopyTexCoord4i (s_varray.c, per unit in the ICD; the other sizes are in gl_varray.c) */
void __fastcall CopyTexCoord4i(GLint unit, __GLcontext *gc, const GLint *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = tp[1]; v->texture[unit].z = tp[2]; v->texture[unit].w = tp[3];
}

/* 690af460 -- CopyTexCoord4f: compiled assuming no aliasing (w is loaded before z is stored) */
#pragma optimize("w", on)
void __fastcall CopyTexCoord4f(GLint unit, __GLcontext *gc, const GLfloat *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = tp[1]; v->texture[unit].z = tp[2]; v->texture[unit].w = tp[3];
}
#pragma optimize("", on)

/* 690af4a0 -- CopyTexCoord4d */
void __fastcall CopyTexCoord4d(GLint unit, __GLcontext *gc, const GLdouble *tp, __GLvertexICD *v)
{
    v->texture[unit].x = tp[0]; v->texture[unit].y = tp[1]; v->texture[unit].z = tp[2]; v->texture[unit].w = tp[3];
}

/* 690af4d0 -- CopyFogCoordf (ICD, EXT_fog_coord arrays): the coordinate as given (glFogCoordfEXT stores 1 - c) */
void __fastcall CopyFogCoordf(__GLcontext *gc, const GLfloat *fp, __GLvertexICD *v)
{
    v->fogCoord = fp[0];
}

/* 690af4e0 -- CopyFogCoordd */
void __fastcall CopyFogCoordd(__GLcontext *gc, const GLdouble *fp, __GLvertexICD *v)
{
    v->fogCoord = fp[0];
}

/* 690af4f0 -- __glim_LockArraysEXT (s_varray.c) */
void __stdcall __glim_LockArraysEXT(GLint start, GLint count)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (start < 0 || count < 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    if (gc->vertexArray.flags & VERTARRAY_CW_LOCKED) {
	__glSetError(GL_INVALID_OPERATION);
	return;
    }

    gc->vertexArray.flags |= VERTARRAY_CW_LOCKED;
    gc->vertexArray.lockFirst = start;
    gc->vertexArray.lockCount = count;

    /*
    ** Compilation is delayed until validation so it catches all of the
    ** current state at glBegin.
    */
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
}

/* 690af5a0 -- __glim_UnlockArraysEXT (s_varray.c) */
void __fastcall __glim_UnlockArraysEXT(void)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (!(gc->vertexArray.flags & VERTARRAY_CW_LOCKED)) {
	__glSetError(GL_INVALID_OPERATION);
	return;
    }

    gc->vertexArray.flags &= ~VERTARRAY_CW_LOCKED;
    gc->vertexArray.lockFirst = gc->vertexArray.lockCount = 0;

    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
}

/* 690af810 -- __glDoClipTriangle (the 3dfx s_varray.c): fetch the three cached vertices, then clip
** the triangle unless all of them are outside one plane */
void __fastcall __glDoClipTriangle(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1,
			     __GLvertexICD *v2)
{
    (*gc->vertexArray.fetchVertex)(gc, v0);
    (*gc->vertexArray.fetchVertex)(gc, v1);
    (*gc->vertexArray.fetchVertex)(gc, v2);

    if ((v0->flagBits & v1->flagBits & v2->flagBits & __GL_CLIP_ALL_MASK) == 0) {
	GLuint orCodes = (v0->flagBits | v1->flagBits | v2->flagBits) & __GL_CLIP_ALL_MASK;
	(*gc->procs.clipTriangle)(gc, v0, v1, v2, orCodes);
    }
}

/* 690affd0 -- ClipLine (the 3dfx s_varray.c): fetch both cached vertices, then clip the line unless
** both are outside one plane (__glClipLine gets them in reverse order) */
void __fastcall ClipLine(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1)
{
    (*gc->vertexArray.fetchVertex)(gc, v0);
    (*gc->vertexArray.fetchVertex)(gc, v1);

    if ((v0->flagBits & v1->flagBits & __GL_CLIP_ALL_MASK) == 0) {
	__glClipLine(gc, v1, v0);
    }
}

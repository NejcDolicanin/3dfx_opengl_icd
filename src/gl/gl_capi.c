/*
** gl_capi.c -- original VA range 690a4000..690a8000: SGI Sample Implementation glcore (MS ICD kit
** flavour): the nop rho procs (s_texel.c), glColor / glIndex / glEdgeFlag immediate-mode entries
** (s_capi.c, s_vapi.c) and the first __gllc_* display-list compile functions (ListBase .. Indexsv)
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "glsi.h"
#include <glide.h>
#include "si_names.h"
#include "icdprotos.h"

/************************************************************************/
/* g_lcomp.c: the display-list compile functions (__gllc_*).  Each allocates an op whose record
** holds its arguments (SI order: doubles first, then words, shorts, bytes, each group padded to a
** whole word) and appends it with the execute function that replays it. */

/* dlistopt.h: gc->dlist.listData.genericFlags */

/* 690a4d90 -- __glNopPolygonRho (s_texel.c): the polygon rho when the unit has no mipmapping */
__GLfloat __fastcall __glNopPolygonRho(__GLcontext *gc, __GLshadeTailICD *sh, __GLfloat s, __GLfloat t,
				   __GLfloat qw)
{
    return 0.0f;
}

/* 690a4f10 -- __glNopLineRho (s_texel.c): the line rho when the unit has no mipmapping */
__GLfloat __fastcall __glNopLineRho(__GLcontext *gc, GLint unit, __GLfloat s, __GLfloat t,
				   __GLfloat qw)
{
    return 0.0f;
}

/* 690a53b0 -- __glim_EdgeFlag (s_vapi.c) */
void __stdcall __glim_EdgeFlag(GLboolean tag)
{
    __GL_SETUP();
    __GL_API_VAPI();
    gc->state.current.edgeTag = tag ? __GL_VERTEX_EDGE_FLAG : 0x0;
}

/* 690a53e0 -- __glim_EdgeFlagv (s_vapi.c) */
void __stdcall __glim_EdgeFlagv(const GLboolean tag[1])
{
    __GL_SETUP();
    __GL_API_VAPI();
    gc->state.current.edgeTag = tag[0] ? __GL_VERTEX_EDGE_FLAG : 0x0;
}

/* 690a5f70 -- __glim_Color3dv */
void __stdcall __glim_Color3dv(const GLdouble v[3])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = v[0];
    gc->state.current.userColor.g = v[1];
    gc->state.current.userColor.b = v[2];
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6010 -- __glim_Color3fv */
void __stdcall __glim_Color3fv(const GLfloat v[3])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = v[0];
    gc->state.current.userColor.g = v[1];
    gc->state.current.userColor.b = v[2];
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6060 -- __glim_Color3i */
void __stdcall __glim_Color3i(GLint r, GLint g, GLint b)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_I_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_I_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_I_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6160 -- __glim_Color3ui */
void __stdcall __glim_Color3ui(GLuint r, GLuint g, GLuint b)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_UI_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_UI_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_UI_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a66d0 -- __glim_Color4dv */
void __stdcall __glim_Color4dv(const GLdouble v[4])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = v[0];
    gc->state.current.userColor.g = v[1];
    gc->state.current.userColor.b = v[2];
    gc->state.current.userColor.a = v[3];
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6720 -- __glim_Color4f */
void __stdcall __glim_Color4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = r;
    gc->state.current.userColor.g = g;
    gc->state.current.userColor.b = b;
    gc->state.current.userColor.a = a;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6770 -- __glim_Color4fv */
void __stdcall __glim_Color4fv(const GLfloat v[4])
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = v[0];
    gc->state.current.userColor.g = v[1];
    gc->state.current.userColor.b = v[2];
    gc->state.current.userColor.a = v[3];
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a67c0 -- __glim_Color4i */
void __stdcall __glim_Color4i(GLint r, GLint g, GLint b, GLint a)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_I_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_I_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_I_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_I_TO_FLOAT(a);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a68d0 -- __glim_Color4ui */
void __stdcall __glim_Color4ui(GLuint r, GLuint g, GLuint b, GLuint a)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = __GL_UI_TO_FLOAT(r);
    gc->state.current.userColor.g = __GL_UI_TO_FLOAT(g);
    gc->state.current.userColor.b = __GL_UI_TO_FLOAT(b);
    gc->state.current.userColor.a = __GL_UI_TO_FLOAT(a);
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6c60 -- __glim_Indexf */
void __stdcall __glim_Indexf(GLfloat c)
{
    __GL_SETUP();
    __GL_API_VAPI();
    gc->state.current.userColorIndex = c;
    gc->colorSet = GL_TRUE;
}

/* 690a6c90 -- __glim_Indexi */
void __stdcall __glim_Indexi(GLint c)
{
    __GL_SETUP();
    __GL_API_VAPI();
    gc->state.current.userColorIndex = c;
    gc->colorSet = GL_TRUE;
}

/* 690a6cc0 -- __glim_Indexs */
void __stdcall __glim_Indexs(GLshort c)
{
    __GL_SETUP();
    __GL_API_VAPI();
    gc->state.current.userColorIndex = c;
    gc->colorSet = GL_TRUE;
}

/* 690a6cf0 -- __glim_Indexub */
void __stdcall __glim_Indexub(GLubyte c)
{
    __GL_SETUP();
    __GL_API_VAPI();
    gc->state.current.userColorIndex = c;
    gc->colorSet = GL_TRUE;
}

/* 690a6d30 -- __glim_Indexdv */
void __stdcall __glim_Indexdv(const GLdouble c[1])
{
    __GL_SETUP();
    __GL_API_VAPI();
    gc->state.current.userColorIndex = c[0];
    gc->colorSet = GL_TRUE;
}

struct __gllc_ListBase_Rec {
    GLuint      base;
};

/* 690a6e20 -- __gllc_ListBase */
void __stdcall __gllc_ListBase(GLuint base)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ListBase_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ListBase_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_ListBase;
    data = (struct __gllc_ListBase_Rec *) dlop->data;
    data->base = base;
    __glDlistAppendOp(gc, dlop, __glle_ListBase);
}

struct __gllc_Color3b_Rec {
    GLbyte      red;
    GLbyte      green;
    GLbyte      blue;
    GLubyte     pad1;
};

/* 690a6e70 -- __gllc_Color3b */
void __stdcall __gllc_Color3b(GLbyte red, GLbyte green, GLbyte blue)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3b_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3b_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3bv;
    data = (struct __gllc_Color3b_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3bv);
}

struct __gllc_Color3bv_Rec {
    GLbyte      v[3];
    GLubyte     pad1;
};

/* 690a6ed0 -- __gllc_Color3bv */
void __stdcall __gllc_Color3bv(const GLbyte *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3bv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3bv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3bv;
    data = (struct __gllc_Color3bv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3bv);
}

struct __gllc_Color3d_Rec {
    GLdouble    red;
    GLdouble    green;
    GLdouble    blue;
};

/* 690a6f30 -- __gllc_Color3d */
void __stdcall __gllc_Color3d(GLdouble red, GLdouble green, GLdouble blue)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Color3d_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3dv);
}

struct __gllc_Color3dv_Rec {
    GLdouble    v[3];
};

/* 690a6fb0 -- __gllc_Color3dv */
void __stdcall __gllc_Color3dv(const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Color3dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3dv);
}

struct __gllc_Color3f_Rec {
    GLfloat     red;
    GLfloat     green;
    GLfloat     blue;
};

/* 690a7030 -- __gllc_Color3f */
void __stdcall __gllc_Color3f(GLfloat red, GLfloat green, GLfloat blue)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3fv;
    data = (struct __gllc_Color3f_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3fv);
}

struct __gllc_Color3fv_Rec {
    GLfloat     v[3];
};

/* 690a7090 -- __gllc_Color3fv */
void __stdcall __gllc_Color3fv(const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3fv;
    data = (struct __gllc_Color3fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3fv);
}

struct __gllc_Color3i_Rec {
    GLint       red;
    GLint       green;
    GLint       blue;
};

/* 690a70f0 -- __gllc_Color3i */
void __stdcall __gllc_Color3i(GLint red, GLint green, GLint blue)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3iv;
    data = (struct __gllc_Color3i_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3iv);
}

struct __gllc_Color3iv_Rec {
    GLint       v[3];
};

/* 690a7150 -- __gllc_Color3iv */
void __stdcall __gllc_Color3iv(const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3iv;
    data = (struct __gllc_Color3iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3iv);
}

struct __gllc_Color3s_Rec {
    GLshort     red;
    GLshort     green;
    GLshort     blue;
    GLushort    pad1;
};

/* 690a71b0 -- __gllc_Color3s */
void __stdcall __gllc_Color3s(GLshort red, GLshort green, GLshort blue)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3sv;
    data = (struct __gllc_Color3s_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3sv);
}

struct __gllc_Color3sv_Rec {
    GLshort     v[3];
    GLushort    pad1;
};

/* 690a7220 -- __gllc_Color3sv */
void __stdcall __gllc_Color3sv(const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3sv;
    data = (struct __gllc_Color3sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3sv);
}

struct __gllc_Color3ub_Rec {
    GLubyte     red;
    GLubyte     green;
    GLubyte     blue;
    GLubyte     pad1;
};

/* 690a7290 -- __gllc_Color3ub */
void __stdcall __gllc_Color3ub(GLubyte red, GLubyte green, GLubyte blue)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3ub_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3ub_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3ubv;
    data = (struct __gllc_Color3ub_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3ubv);
}

struct __gllc_Color3ubv_Rec {
    GLubyte     v[3];
    GLubyte     pad1;
};

/* 690a72f0 -- __gllc_Color3ubv */
void __stdcall __gllc_Color3ubv(const GLubyte *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3ubv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3ubv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3ubv;
    data = (struct __gllc_Color3ubv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3ubv);
}

struct __gllc_Color3ui_Rec {
    GLuint      red;
    GLuint      green;
    GLuint      blue;
};

/* 690a7350 -- __gllc_Color3ui */
void __stdcall __gllc_Color3ui(GLuint red, GLuint green, GLuint blue)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3ui_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3ui_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3uiv;
    data = (struct __gllc_Color3ui_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3uiv);
}

struct __gllc_Color3uiv_Rec {
    GLuint      v[3];
};

/* 690a73b0 -- __gllc_Color3uiv */
void __stdcall __gllc_Color3uiv(const GLuint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3uiv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3uiv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3uiv;
    data = (struct __gllc_Color3uiv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3uiv);
}

struct __gllc_Color3us_Rec {
    GLushort    red;
    GLushort    green;
    GLushort    blue;
    GLushort    pad1;
};

/* 690a7410 -- __gllc_Color3us */
void __stdcall __gllc_Color3us(GLushort red, GLushort green, GLushort blue)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3us_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3us_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3usv;
    data = (struct __gllc_Color3us_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3usv);
}

struct __gllc_Color3usv_Rec {
    GLushort    v[3];
    GLushort    pad1;
};

/* 690a7480 -- __gllc_Color3usv */
void __stdcall __gllc_Color3usv(const GLushort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color3usv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color3usv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color3usv;
    data = (struct __gllc_Color3usv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color3usv);
}

struct __gllc_Color4b_Rec {
    GLbyte      red;
    GLbyte      green;
    GLbyte      blue;
    GLbyte      alpha;
};

/* 690a74f0 -- __gllc_Color4b */
void __stdcall __gllc_Color4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4b_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4b_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4bv;
    data = (struct __gllc_Color4b_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4bv);
}

struct __gllc_Color4bv_Rec {
    GLbyte      v[4];
};

/* 690a7560 -- __gllc_Color4bv */
void __stdcall __gllc_Color4bv(const GLbyte *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4bv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4bv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4bv;
    data = (struct __gllc_Color4bv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4bv);
}

struct __gllc_Color4d_Rec {
    GLdouble    red;
    GLdouble    green;
    GLdouble    blue;
    GLdouble    alpha;
};

/* 690a75d0 -- __gllc_Color4d */
void __stdcall __gllc_Color4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Color4d_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4dv);
}

struct __gllc_Color4dv_Rec {
    GLdouble    v[4];
};

/* 690a7650 -- __gllc_Color4dv */
void __stdcall __gllc_Color4dv(const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4dv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4dv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4dv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Color4dv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4dv);
}

struct __gllc_Color4f_Rec {
    GLfloat     red;
    GLfloat     green;
    GLfloat     blue;
    GLfloat     alpha;
};

/* 690a76e0 -- __gllc_Color4f */
void __stdcall __gllc_Color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4fv;
    data = (struct __gllc_Color4f_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4fv);
}

struct __gllc_Color4fv_Rec {
    GLfloat     v[4];
};

/* 690a7750 -- __gllc_Color4fv */
void __stdcall __gllc_Color4fv(const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4fv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4fv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4fv;
    data = (struct __gllc_Color4fv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4fv);
}

struct __gllc_Color4i_Rec {
    GLint       red;
    GLint       green;
    GLint       blue;
    GLint       alpha;
};

/* 690a77c0 -- __gllc_Color4i */
void __stdcall __gllc_Color4i(GLint red, GLint green, GLint blue, GLint alpha)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4iv;
    data = (struct __gllc_Color4i_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4iv);
}

struct __gllc_Color4iv_Rec {
    GLint       v[4];
};

/* 690a7830 -- __gllc_Color4iv */
void __stdcall __gllc_Color4iv(const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4iv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4iv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4iv;
    data = (struct __gllc_Color4iv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4iv);
}

struct __gllc_Color4s_Rec {
    GLshort     red;
    GLshort     green;
    GLshort     blue;
    GLshort     alpha;
};

/* 690a78a0 -- __gllc_Color4s */
void __stdcall __gllc_Color4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4sv;
    data = (struct __gllc_Color4s_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4sv);
}

struct __gllc_Color4sv_Rec {
    GLshort     v[4];
};

/* 690a7910 -- __gllc_Color4sv */
void __stdcall __gllc_Color4sv(const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4sv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4sv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4sv;
    data = (struct __gllc_Color4sv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4sv);
}

struct __gllc_Color4ub_Rec {
    GLubyte     red;
    GLubyte     green;
    GLubyte     blue;
    GLubyte     alpha;
};

/* 690a7980 -- __gllc_Color4ub */
void __stdcall __gllc_Color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4ub_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4ub_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4ubv;
    data = (struct __gllc_Color4ub_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4ubv);
}

struct __gllc_Color4ubv_Rec {
    GLubyte     v[4];
};

/* 690a79f0 -- __gllc_Color4ubv */
void __stdcall __gllc_Color4ubv(const GLubyte *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4ubv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4ubv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4ubv;
    data = (struct __gllc_Color4ubv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4ubv);
}

struct __gllc_Color4ui_Rec {
    GLuint      red;
    GLuint      green;
    GLuint      blue;
    GLuint      alpha;
};

/* 690a7a60 -- __gllc_Color4ui */
void __stdcall __gllc_Color4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4ui_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4ui_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4uiv;
    data = (struct __gllc_Color4ui_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4uiv);
}

struct __gllc_Color4uiv_Rec {
    GLuint      v[4];
};

/* 690a7ad0 -- __gllc_Color4uiv */
void __stdcall __gllc_Color4uiv(const GLuint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4uiv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4uiv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4uiv;
    data = (struct __gllc_Color4uiv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4uiv);
}

struct __gllc_Color4us_Rec {
    GLushort    red;
    GLushort    green;
    GLushort    blue;
    GLushort    alpha;
};

/* 690a7b40 -- __gllc_Color4us */
void __stdcall __gllc_Color4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4us_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4us_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4usv;
    data = (struct __gllc_Color4us_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4usv);
}

struct __gllc_Color4usv_Rec {
    GLushort    v[4];
};

/* 690a7bb0 -- __gllc_Color4usv */
void __stdcall __gllc_Color4usv(const GLushort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Color4usv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Color4usv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Color4usv;
    data = (struct __gllc_Color4usv_Rec *) dlop->data;
    data->v[0] = v[0];
    data->v[1] = v[1];
    data->v[2] = v[2];
    data->v[3] = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_COLOR;
    __glDlistAppendOp(gc, dlop, __glle_Color4usv);
}

struct __gllc_EdgeFlag_Rec {
    GLboolean   flag;
    GLubyte     pad1;
    GLubyte     pad2;
    GLubyte     pad3;
};

/* 690a7c20 -- __gllc_EdgeFlag */
void __stdcall __gllc_EdgeFlag(GLboolean flag)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EdgeFlag_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EdgeFlag_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EdgeFlagv;
    data = (struct __gllc_EdgeFlag_Rec *) dlop->data;
    data->flag = flag;
    __glDlistAppendOp(gc, dlop, __glle_EdgeFlagv);
}

struct __gllc_EdgeFlagv_Rec {
    GLboolean   flag[1];
    GLubyte     pad1;
    GLubyte     pad2;
    GLubyte     pad3;
};

/* 690a7c70 -- __gllc_EdgeFlagv */
void __stdcall __gllc_EdgeFlagv(const GLboolean *flag)
{
    __GLdlistOpICD *dlop;
    struct __gllc_EdgeFlagv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_EdgeFlagv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_EdgeFlagv;
    data = (struct __gllc_EdgeFlagv_Rec *) dlop->data;
    data->flag[0] = flag[0];
    __glDlistAppendOp(gc, dlop, __glle_EdgeFlagv);
}

/* 690a7cc0 -- __gllc_End */
void __fastcall __gllc_End(void)
{
    __GLdlistOpICD *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = __glop_End;
    __glDlistAppendOp(gc, dlop, __glle_End);
}

struct __gllc_Indexd_Rec {
    GLdouble    c;
};

/* 690a7d00 -- __gllc_Indexd */
void __stdcall __gllc_Indexd(GLdouble c)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Indexd_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexd_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexdv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Indexd_Rec *) dlop->data;
    data->c = c;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexdv);
}

struct __gllc_Indexdv_Rec {
    GLdouble    c[1];
};

/* 690a7d60 -- __gllc_Indexdv */
void __stdcall __gllc_Indexdv(const GLdouble *c)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Indexdv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexdv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexdv;
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Indexdv_Rec *) dlop->data;
    data->c[0] = c[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexdv);
}

struct __gllc_Indexf_Rec {
    GLfloat     c;
};

/* 690a7dc0 -- __gllc_Indexf */
void __stdcall __gllc_Indexf(GLfloat c)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Indexf_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexf_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexfv;
    data = (struct __gllc_Indexf_Rec *) dlop->data;
    data->c = c;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexfv);
}

struct __gllc_Indexfv_Rec {
    GLfloat     c[1];
};

/* 690a7e20 -- __gllc_Indexfv */
void __stdcall __gllc_Indexfv(const GLfloat *c)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Indexfv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexfv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexfv;
    data = (struct __gllc_Indexfv_Rec *) dlop->data;
    data->c[0] = c[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexfv);
}

struct __gllc_Indexi_Rec {
    GLint       c;
};

/* 690a7e80 -- __gllc_Indexi */
void __stdcall __gllc_Indexi(GLint c)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Indexi_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexi_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexiv;
    data = (struct __gllc_Indexi_Rec *) dlop->data;
    data->c = c;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexiv);
}

struct __gllc_Indexiv_Rec {
    GLint       c[1];
};

/* 690a7ee0 -- __gllc_Indexiv */
void __stdcall __gllc_Indexiv(const GLint *c)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Indexiv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexiv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexiv;
    data = (struct __gllc_Indexiv_Rec *) dlop->data;
    data->c[0] = c[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexiv);
}

struct __gllc_Indexs_Rec {
    GLshort     c;
    GLushort    pad1;
};

/* 690a7f40 -- __gllc_Indexs */
void __stdcall __gllc_Indexs(GLshort c)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Indexs_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexs_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexsv;
    data = (struct __gllc_Indexs_Rec *) dlop->data;
    data->c = c;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexsv);
}

struct __gllc_Indexsv_Rec {
    GLshort     c[1];
    GLushort    pad1;
};

/* 690a7fa0 -- __gllc_Indexsv */
void __stdcall __gllc_Indexsv(const GLshort *c)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Indexsv_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Indexsv_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_Indexsv;
    data = (struct __gllc_Indexsv_Rec *) dlop->data;
    data->c[0] = c[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_INDEX;
    __glDlistAppendOp(gc, dlop, __glle_Indexsv);
}

/* 690a5f20 -- __glim_Color3d (s_capi.c) */
void __stdcall __glim_Color3d(GLdouble r, GLdouble g, GLdouble b)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = r;
    gc->state.current.userColor.g = g;
    gc->state.current.userColor.b = b;
    gc->state.current.userColor.a = __GL_ONE_F;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6680 -- __glim_Color4d (s_capi.c) */
void __stdcall __glim_Color4d(GLdouble r, GLdouble g, GLdouble b, GLdouble a)
{
    __GL_SETUP();
    __GL_API_VAPI();

    gc->state.current.userColor.r = r;
    gc->state.current.userColor.g = g;
    gc->state.current.userColor.b = b;
    gc->state.current.userColor.a = a;
    gc->colorSet = GL_TRUE;
    (*gc->procs.applyColor)(gc);
}

/* 690a6c30 -- __glim_Indexd (s_capi.c) */
void __stdcall __glim_Indexd(GLdouble c)
{
    __GL_SETUP();
    __GL_API_VAPI();
    gc->state.current.userColorIndex = c;
    gc->colorSet = GL_TRUE;
}

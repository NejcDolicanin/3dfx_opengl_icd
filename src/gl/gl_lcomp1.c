/*
** gl_lcomp1.c -- original VA range 690a8000..690ac000: the display-list compile entries (SI g_lcomp.c,
** generated from listcomp.ls / dlist.ls) of Normal3* .. MapGrid1*, in dispatch order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "icdprotos.h"
#include "glsi.h"
#include <glide.h>

/*
** Every entry allocates an op (__glDlistAllocOp2, 690b44d0), stores the opcode and its arguments in
** the op's record and appends it with the op's execute function (__glDlistAppendOp, 690b4520).  The
** ICD's opcodes are its own numbering; a v form shares the record and the opcode of its scalar form
** (the SI's vectorequiv), and both store the same __glle_*v execute function.
*/

/* dlistopt.h */

/* the execute functions (g_lcomp.c __glle_*, src/gl/gl_lexec.c ..) */
extern const GLubyte *__fastcall __glle_Scissor(const GLubyte *PC);
extern const GLubyte *__fastcall __glle_ClearAccum(const GLubyte *PC);
extern const GLubyte *__fastcall __glle_ClearColor(const GLubyte *PC);
extern const GLubyte *__fastcall __glle_MapGrid1f(const GLubyte *PC);

/* the op records (g_lcomp.h __gllc_*_Rec): doubles first, padded to a word (8 bytes when aligned) */
struct __gllc_Normal3b_Rec { GLbyte nx, ny, nz; GLubyte pad; };
struct __gllc_Normal3d_Rec { GLdouble nx, ny, nz; };
struct __gllc_Normal3f_Rec { GLfloat nx, ny, nz; };
struct __gllc_Normal3i_Rec { GLint nx, ny, nz; };
struct __gllc_Normal3s_Rec { GLshort nx, ny, nz; GLushort pad; };
struct __gllc_RasterPos2d_Rec { GLdouble x, y; };
struct __gllc_RasterPos2f_Rec { GLfloat x, y; };
struct __gllc_RasterPos2i_Rec { GLint x, y; };
struct __gllc_RasterPos2s_Rec { GLshort x, y; };
struct __gllc_RasterPos3d_Rec { GLdouble x, y, z; };
struct __gllc_RasterPos3f_Rec { GLfloat x, y, z; };
struct __gllc_RasterPos3i_Rec { GLint x, y, z; };
struct __gllc_RasterPos3s_Rec { GLshort x, y, z; GLushort pad; };
struct __gllc_RasterPos4d_Rec { GLdouble x, y, z, w; };
struct __gllc_RasterPos4f_Rec { GLfloat x, y, z, w; };
struct __gllc_RasterPos4i_Rec { GLint x, y, z, w; };
struct __gllc_RasterPos4s_Rec { GLshort x, y, z, w; };
struct __gllc_Rectd_Rec { GLdouble x1, y1, x2, y2; };
struct __gllc_Rectf_Rec { GLfloat x1, y1, x2, y2; };
struct __gllc_Recti_Rec { GLint x1, y1, x2, y2; };
struct __gllc_Rects_Rec { GLshort x1, y1, x2, y2; };
struct __gllc_TexCoord1d_Rec { GLdouble s; };
struct __gllc_TexCoord1f_Rec { GLfloat s; };
struct __gllc_TexCoord1i_Rec { GLint s; };
struct __gllc_TexCoord1s_Rec { GLshort s; GLushort pad; };
struct __gllc_TexCoord2d_Rec { GLdouble s, t; };
struct __gllc_TexCoord2f_Rec { GLfloat s, t; };
struct __gllc_TexCoord2i_Rec { GLint s, t; };
struct __gllc_TexCoord2s_Rec { GLshort s, t; };
struct __gllc_TexCoord3d_Rec { GLdouble s, t, r; };
struct __gllc_TexCoord3f_Rec { GLfloat s, t, r; };
struct __gllc_TexCoord3i_Rec { GLint s, t, r; };
struct __gllc_TexCoord3s_Rec { GLshort s, t, r; GLushort pad; };
struct __gllc_TexCoord4d_Rec { GLdouble s, t, r, q; };
struct __gllc_TexCoord4f_Rec { GLfloat s, t, r, q; };
struct __gllc_TexCoord4i_Rec { GLint s, t, r, q; };
struct __gllc_TexCoord4s_Rec { GLshort s, t, r, q; };
struct __gllc_Vertex2d_Rec { GLdouble x, y; };
struct __gllc_Vertex2f_Rec { GLfloat x, y; };
struct __gllc_Vertex2i_Rec { GLint x, y; };
struct __gllc_Vertex2s_Rec { GLshort x, y; };
struct __gllc_Vertex3d_Rec { GLdouble x, y, z; };
struct __gllc_Vertex3f_Rec { GLfloat x, y, z; };
struct __gllc_Vertex3i_Rec { GLint x, y, z; };
struct __gllc_Vertex3s_Rec { GLshort x, y, z; GLushort pad; };
struct __gllc_Vertex4d_Rec { GLdouble x, y, z, w; };
struct __gllc_Vertex4f_Rec { GLfloat x, y, z, w; };
struct __gllc_Vertex4i_Rec { GLint x, y, z, w; };
struct __gllc_Vertex4s_Rec { GLshort x, y, z, w; };
struct __gllc_ClipPlane_Rec { GLdouble equation[4]; GLenum plane; GLuint pad; };
struct __gllc_ColorMaterial_Rec { GLenum face, mode; };
struct __gllc_CullFace_Rec { GLenum mode; };
struct __gllc_Fogf_Rec { GLenum pname; GLfloat param; };
struct __gllc_Fogi_Rec { GLenum pname; GLint param; };
struct __gllc_FrontFace_Rec { GLenum mode; };
struct __gllc_Hint_Rec { GLenum target, mode; };
struct __gllc_LineStipple_Rec { GLint factor; GLushort pattern; GLushort pad; };
struct __gllc_LineWidth_Rec { GLfloat width; };
struct __gllc_PointSize_Rec { GLfloat size; };
struct __gllc_PolygonMode_Rec { GLenum face, mode; };
struct __gllc_Scissor_Rec { GLint x, y; GLsizei width, height; };
struct __gllc_ShadeModel_Rec { GLenum mode; };
struct __gllc_LoadName_Rec { GLuint name; };
struct __gllc_PassThrough_Rec { GLfloat token; };
struct __gllc_PushName_Rec { GLuint name; };
struct __gllc_DrawBuffer_Rec { GLenum mode; };
struct __gllc_Clear_Rec { GLbitfield mask; };
struct __gllc_ClearAccum_Rec { GLfloat red, green, blue, alpha; };
struct __gllc_ClearIndex_Rec { GLfloat c; };
struct __gllc_ClearColor_Rec { GLclampf red, green, blue, alpha; };
struct __gllc_ClearStencil_Rec { GLint s; };
struct __gllc_ClearDepth_Rec { GLclampd depth; };
struct __gllc_StencilMask_Rec { GLuint mask; };
struct __gllc_ColorMask_Rec { GLboolean red, green, blue, alpha; };
struct __gllc_DepthMask_Rec { GLboolean flag; GLubyte pad[3]; };
struct __gllc_IndexMask_Rec { GLuint mask; };
struct __gllc_Accum_Rec { GLenum op; GLfloat value; };
struct __gllc_PushAttrib_Rec { GLbitfield mask; };
struct __gllc_MapGrid1d_Rec { GLdouble u1, u2; GLint un; GLuint pad; };
struct __gllc_MapGrid1f_Rec { GLint un; GLfloat u1, u2; };

/* 690a8000 -- __gllc_Normal3b */
void __stdcall __gllc_Normal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Normal3b_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3b_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x1b;        /* __glop_Normal3bv */
    data = (struct __gllc_Normal3b_Rec *) dlop->data;
    data->nx = nx;
    data->ny = ny;
    data->nz = nz;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3bv);    /* __glle_Normal3bv */
}

/* 690a8060 -- __gllc_Normal3bv */
void __stdcall __gllc_Normal3bv(const GLbyte *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Normal3b_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3b_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x1b;        /* __glop_Normal3bv */
    data = (struct __gllc_Normal3b_Rec *) dlop->data;
    data->nx = v[0];
    data->ny = v[1];
    data->nz = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3bv);    /* __glle_Normal3bv */
}

/* 690a80c0 -- __gllc_Normal3d */
void __stdcall __gllc_Normal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Normal3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x1c;        /* __glop_Normal3dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Normal3d_Rec *) dlop->data;
    data->nx = nx;
    data->ny = ny;
    data->nz = nz;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3dv);    /* __glle_Normal3dv */
}

/* 690a8140 -- __gllc_Normal3dv */
void __stdcall __gllc_Normal3dv(const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Normal3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x1c;        /* __glop_Normal3dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Normal3d_Rec *) dlop->data;
    data->nx = v[0];
    data->ny = v[1];
    data->nz = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3dv);    /* __glle_Normal3dv */
}

/* 690a81c0 -- __gllc_Normal3f */
void __stdcall __gllc_Normal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Normal3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x1d;        /* __glop_Normal3fv */
    data = (struct __gllc_Normal3f_Rec *) dlop->data;
    data->nx = nx;
    data->ny = ny;
    data->nz = nz;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3fv);    /* __glle_Normal3fv */
}

/* 690a8220 -- __gllc_Normal3fv */
void __stdcall __gllc_Normal3fv(const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Normal3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x1d;        /* __glop_Normal3fv */
    data = (struct __gllc_Normal3f_Rec *) dlop->data;
    data->nx = v[0];
    data->ny = v[1];
    data->nz = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3fv);    /* __glle_Normal3fv */
}

/* 690a8280 -- __gllc_Normal3i */
void __stdcall __gllc_Normal3i(GLint nx, GLint ny, GLint nz)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Normal3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x1e;        /* __glop_Normal3iv */
    data = (struct __gllc_Normal3i_Rec *) dlop->data;
    data->nx = nx;
    data->ny = ny;
    data->nz = nz;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3iv);    /* __glle_Normal3iv */
}

/* 690a82e0 -- __gllc_Normal3iv */
void __stdcall __gllc_Normal3iv(const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Normal3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x1e;        /* __glop_Normal3iv */
    data = (struct __gllc_Normal3i_Rec *) dlop->data;
    data->nx = v[0];
    data->ny = v[1];
    data->nz = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3iv);    /* __glle_Normal3iv */
}

/* 690a8340 -- __gllc_Normal3s */
void __stdcall __gllc_Normal3s(GLshort nx, GLshort ny, GLshort nz)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Normal3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x1f;        /* __glop_Normal3sv */
    data = (struct __gllc_Normal3s_Rec *) dlop->data;
    data->nx = nx;
    data->ny = ny;
    data->nz = nz;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3sv);    /* __glle_Normal3sv */
}

/* 690a83b0 -- __gllc_Normal3sv */
void __stdcall __gllc_Normal3sv(const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Normal3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Normal3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x1f;        /* __glop_Normal3sv */
    data = (struct __gllc_Normal3s_Rec *) dlop->data;
    data->nx = v[0];
    data->ny = v[1];
    data->nz = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_NORMAL;
    __glDlistAppendOp(gc, dlop, __glle_Normal3sv);    /* __glle_Normal3sv */
}

/* 690a8490 -- __gllc_RasterPos2dv */
void __stdcall __gllc_RasterPos2dv(const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos2d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x20;        /* __glop_RasterPos2dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_RasterPos2d_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2dv);    /* __glle_RasterPos2dv */
}

/* 690a8500 -- __gllc_RasterPos2f */
void __stdcall __gllc_RasterPos2f(GLfloat x, GLfloat y)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos2f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x21;        /* __glop_RasterPos2fv */
    data = (struct __gllc_RasterPos2f_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2fv);    /* __glle_RasterPos2fv */
}

/* 690a8560 -- __gllc_RasterPos2fv */
void __stdcall __gllc_RasterPos2fv(const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos2f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x21;        /* __glop_RasterPos2fv */
    data = (struct __gllc_RasterPos2f_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2fv);    /* __glle_RasterPos2fv */
}

/* 690a85c0 -- __gllc_RasterPos2i */
void __stdcall __gllc_RasterPos2i(GLint x, GLint y)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos2i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x22;        /* __glop_RasterPos2iv */
    data = (struct __gllc_RasterPos2i_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2iv);    /* __glle_RasterPos2iv */
}

/* 690a8620 -- __gllc_RasterPos2iv */
void __stdcall __gllc_RasterPos2iv(const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos2i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x22;        /* __glop_RasterPos2iv */
    data = (struct __gllc_RasterPos2i_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2iv);    /* __glle_RasterPos2iv */
}

/* 690a8680 -- __gllc_RasterPos2s */
void __stdcall __gllc_RasterPos2s(GLshort x, GLshort y)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos2s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x23;        /* __glop_RasterPos2sv */
    data = (struct __gllc_RasterPos2s_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2sv);    /* __glle_RasterPos2sv */
}

/* 690a86e0 -- __gllc_RasterPos2sv */
void __stdcall __gllc_RasterPos2sv(const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos2s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos2s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x23;        /* __glop_RasterPos2sv */
    data = (struct __gllc_RasterPos2s_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos2sv);    /* __glle_RasterPos2sv */
}

/* 690a8740 -- __gllc_RasterPos3d */
void __stdcall __gllc_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x24;        /* __glop_RasterPos3dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_RasterPos3d_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3dv);    /* __glle_RasterPos3dv */
}

/* 690a87c0 -- __gllc_RasterPos3dv */
void __stdcall __gllc_RasterPos3dv(const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x24;        /* __glop_RasterPos3dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_RasterPos3d_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3dv);    /* __glle_RasterPos3dv */
}

/* 690a8840 -- __gllc_RasterPos3f */
void __stdcall __gllc_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x25;        /* __glop_RasterPos3fv */
    data = (struct __gllc_RasterPos3f_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3fv);    /* __glle_RasterPos3fv */
}

/* 690a88a0 -- __gllc_RasterPos3fv */
void __stdcall __gllc_RasterPos3fv(const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x25;        /* __glop_RasterPos3fv */
    data = (struct __gllc_RasterPos3f_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3fv);    /* __glle_RasterPos3fv */
}

/* 690a8900 -- __gllc_RasterPos3i */
void __stdcall __gllc_RasterPos3i(GLint x, GLint y, GLint z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x26;        /* __glop_RasterPos3iv */
    data = (struct __gllc_RasterPos3i_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3iv);    /* __glle_RasterPos3iv */
}

/* 690a8960 -- __gllc_RasterPos3iv */
void __stdcall __gllc_RasterPos3iv(const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x26;        /* __glop_RasterPos3iv */
    data = (struct __gllc_RasterPos3i_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3iv);    /* __glle_RasterPos3iv */
}

/* 690a89c0 -- __gllc_RasterPos3s */
void __stdcall __gllc_RasterPos3s(GLshort x, GLshort y, GLshort z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x27;        /* __glop_RasterPos3sv */
    data = (struct __gllc_RasterPos3s_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3sv);    /* __glle_RasterPos3sv */
}

/* 690a8a30 -- __gllc_RasterPos3sv */
void __stdcall __gllc_RasterPos3sv(const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x27;        /* __glop_RasterPos3sv */
    data = (struct __gllc_RasterPos3s_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos3sv);    /* __glle_RasterPos3sv */
}

/* 690a8aa0 -- __gllc_RasterPos4d */
void __stdcall __gllc_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos4d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x28;        /* __glop_RasterPos4dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_RasterPos4d_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4dv);    /* __glle_RasterPos4dv */
}

/* 690a8b20 -- __gllc_RasterPos4dv */
void __stdcall __gllc_RasterPos4dv(const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos4d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x28;        /* __glop_RasterPos4dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_RasterPos4d_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    data->w = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4dv);    /* __glle_RasterPos4dv */
}

/* 690a8bb0 -- __gllc_RasterPos4f */
void __stdcall __gllc_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos4f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x29;        /* __glop_RasterPos4fv */
    data = (struct __gllc_RasterPos4f_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4fv);    /* __glle_RasterPos4fv */
}

/* 690a8c20 -- __gllc_RasterPos4fv */
void __stdcall __gllc_RasterPos4fv(const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos4f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x29;        /* __glop_RasterPos4fv */
    data = (struct __gllc_RasterPos4f_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    data->w = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4fv);    /* __glle_RasterPos4fv */
}

/* 690a8c90 -- __gllc_RasterPos4i */
void __stdcall __gllc_RasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos4i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x2a;        /* __glop_RasterPos4iv */
    data = (struct __gllc_RasterPos4i_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4iv);    /* __glle_RasterPos4iv */
}

/* 690a8d00 -- __gllc_RasterPos4iv */
void __stdcall __gllc_RasterPos4iv(const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos4i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x2a;        /* __glop_RasterPos4iv */
    data = (struct __gllc_RasterPos4i_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    data->w = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4iv);    /* __glle_RasterPos4iv */
}

/* 690a8d70 -- __gllc_RasterPos4s */
void __stdcall __gllc_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos4s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x2b;        /* __glop_RasterPos4sv */
    data = (struct __gllc_RasterPos4s_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4sv);    /* __glle_RasterPos4sv */
}

/* 690a8de0 -- __gllc_RasterPos4sv */
void __stdcall __gllc_RasterPos4sv(const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_RasterPos4s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_RasterPos4s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x2b;        /* __glop_RasterPos4sv */
    data = (struct __gllc_RasterPos4s_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    data->w = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RASTERPOS;
    __glDlistAppendOp(gc, dlop, __glle_RasterPos4sv);    /* __glle_RasterPos4sv */
}

/* 690a8e50 -- __gllc_Rectd */
void __stdcall __gllc_Rectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Rectd_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rectd_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x2c;        /* __glop_Rectdv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Rectd_Rec *) dlop->data;
    data->x1 = x1;
    data->y1 = y1;
    data->x2 = x2;
    data->y2 = y2;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectdv);    /* __glle_Rectdv */
}

/* 690a8ed0 -- __gllc_Rectdv */
void __stdcall __gllc_Rectdv(const GLdouble *v1, const GLdouble *v2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Rectd_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rectd_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x2c;        /* __glop_Rectdv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Rectd_Rec *) dlop->data;
    data->x1 = v1[0];
    data->y1 = v1[1];
    data->x2 = v2[0];
    data->y2 = v2[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectdv);    /* __glle_Rectdv */
}

/* 690a8f60 -- __gllc_Rectf */
void __stdcall __gllc_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Rectf_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rectf_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x2d;        /* __glop_Rectfv */
    data = (struct __gllc_Rectf_Rec *) dlop->data;
    data->x1 = x1;
    data->y1 = y1;
    data->x2 = x2;
    data->y2 = y2;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectfv);    /* __glle_Rectfv */
}

/* 690a8fd0 -- __gllc_Rectfv */
void __stdcall __gllc_Rectfv(const GLfloat *v1, const GLfloat *v2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Rectf_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rectf_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x2d;        /* __glop_Rectfv */
    data = (struct __gllc_Rectf_Rec *) dlop->data;
    data->x1 = v1[0];
    data->y1 = v1[1];
    data->x2 = v2[0];
    data->y2 = v2[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectfv);    /* __glle_Rectfv */
}

/* 690a9040 -- __gllc_Recti */
void __stdcall __gllc_Recti(GLint x1, GLint y1, GLint x2, GLint y2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Recti_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Recti_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x2e;        /* __glop_Rectiv */
    data = (struct __gllc_Recti_Rec *) dlop->data;
    data->x1 = x1;
    data->y1 = y1;
    data->x2 = x2;
    data->y2 = y2;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectiv);    /* __glle_Rectiv */
}

/* 690a90b0 -- __gllc_Rectiv */
void __stdcall __gllc_Rectiv(const GLint *v1, const GLint *v2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Recti_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Recti_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x2e;        /* __glop_Rectiv */
    data = (struct __gllc_Recti_Rec *) dlop->data;
    data->x1 = v1[0];
    data->y1 = v1[1];
    data->x2 = v2[0];
    data->y2 = v2[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectiv);    /* __glle_Rectiv */
}

/* 690a9120 -- __gllc_Rects */
void __stdcall __gllc_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Rects_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rects_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x2f;        /* __glop_Rectsv */
    data = (struct __gllc_Rects_Rec *) dlop->data;
    data->x1 = x1;
    data->y1 = y1;
    data->x2 = x2;
    data->y2 = y2;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectsv);    /* __glle_Rectsv */
}

/* 690a9190 -- __gllc_Rectsv */
void __stdcall __gllc_Rectsv(const GLshort *v1, const GLshort *v2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Rects_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Rects_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x2f;        /* __glop_Rectsv */
    data = (struct __gllc_Rects_Rec *) dlop->data;
    data->x1 = v1[0];
    data->y1 = v1[1];
    data->x2 = v2[0];
    data->y2 = v2[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_RECT;
    __glDlistAppendOp(gc, dlop, __glle_Rectsv);    /* __glle_Rectsv */
}

/* 690a9200 -- __gllc_TexCoord1d */
void __stdcall __gllc_TexCoord1d(GLdouble s)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord1d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x30;        /* __glop_TexCoord1dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord1d_Rec *) dlop->data;
    data->s = s;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1dv);    /* __glle_TexCoord1dv */
}

/* 690a9260 -- __gllc_TexCoord1dv */
void __stdcall __gllc_TexCoord1dv(const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord1d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x30;        /* __glop_TexCoord1dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord1d_Rec *) dlop->data;
    data->s = v[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1dv);    /* __glle_TexCoord1dv */
}

/* 690a92c0 -- __gllc_TexCoord1f */
void __stdcall __gllc_TexCoord1f(GLfloat s)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord1f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x31;        /* __glop_TexCoord1fv */
    data = (struct __gllc_TexCoord1f_Rec *) dlop->data;
    data->s = s;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1fv);    /* __glle_TexCoord1fv */
}

/* 690a9320 -- __gllc_TexCoord1fv */
void __stdcall __gllc_TexCoord1fv(const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord1f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x31;        /* __glop_TexCoord1fv */
    data = (struct __gllc_TexCoord1f_Rec *) dlop->data;
    data->s = v[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1fv);    /* __glle_TexCoord1fv */
}

/* 690a9380 -- __gllc_TexCoord1i */
void __stdcall __gllc_TexCoord1i(GLint s)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord1i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x32;        /* __glop_TexCoord1iv */
    data = (struct __gllc_TexCoord1i_Rec *) dlop->data;
    data->s = s;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1iv);    /* __glle_TexCoord1iv */
}

/* 690a93e0 -- __gllc_TexCoord1iv */
void __stdcall __gllc_TexCoord1iv(const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord1i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x32;        /* __glop_TexCoord1iv */
    data = (struct __gllc_TexCoord1i_Rec *) dlop->data;
    data->s = v[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1iv);    /* __glle_TexCoord1iv */
}

/* 690a9440 -- __gllc_TexCoord1s */
void __stdcall __gllc_TexCoord1s(GLshort s)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord1s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x33;        /* __glop_TexCoord1sv */
    data = (struct __gllc_TexCoord1s_Rec *) dlop->data;
    data->s = s;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1sv);    /* __glle_TexCoord1sv */
}

/* 690a94a0 -- __gllc_TexCoord1sv */
void __stdcall __gllc_TexCoord1sv(const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord1s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord1s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x33;        /* __glop_TexCoord1sv */
    data = (struct __gllc_TexCoord1s_Rec *) dlop->data;
    data->s = v[0];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord1sv);    /* __glle_TexCoord1sv */
}

/* 690a9500 -- __gllc_TexCoord2d */
void __stdcall __gllc_TexCoord2d(GLdouble s, GLdouble t)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord2d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x34;        /* __glop_TexCoord2dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord2d_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2dv);    /* __glle_TexCoord2dv */
}

/* 690a9570 -- __gllc_TexCoord2dv */
void __stdcall __gllc_TexCoord2dv(const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord2d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x34;        /* __glop_TexCoord2dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord2d_Rec *) dlop->data;
    data->s = v[0];
    data->t = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2dv);    /* __glle_TexCoord2dv */
}

/* 690a95e0 -- __gllc_TexCoord2f */
void __stdcall __gllc_TexCoord2f(GLfloat s, GLfloat t)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord2f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x35;        /* __glop_TexCoord2fv */
    data = (struct __gllc_TexCoord2f_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2fv);    /* __glle_TexCoord2fv */
}

/* 690a9640 -- __gllc_TexCoord2fv */
void __stdcall __gllc_TexCoord2fv(const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord2f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x35;        /* __glop_TexCoord2fv */
    data = (struct __gllc_TexCoord2f_Rec *) dlop->data;
    data->s = v[0];
    data->t = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2fv);    /* __glle_TexCoord2fv */
}

/* 690a96a0 -- __gllc_TexCoord2i */
void __stdcall __gllc_TexCoord2i(GLint s, GLint t)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord2i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x36;        /* __glop_TexCoord2iv */
    data = (struct __gllc_TexCoord2i_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2iv);    /* __glle_TexCoord2iv */
}

/* 690a9700 -- __gllc_TexCoord2iv */
void __stdcall __gllc_TexCoord2iv(const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord2i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x36;        /* __glop_TexCoord2iv */
    data = (struct __gllc_TexCoord2i_Rec *) dlop->data;
    data->s = v[0];
    data->t = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2iv);    /* __glle_TexCoord2iv */
}

/* 690a9760 -- __gllc_TexCoord2s */
void __stdcall __gllc_TexCoord2s(GLshort s, GLshort t)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord2s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x37;        /* __glop_TexCoord2sv */
    data = (struct __gllc_TexCoord2s_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2sv);    /* __glle_TexCoord2sv */
}

/* 690a97c0 -- __gllc_TexCoord2sv */
void __stdcall __gllc_TexCoord2sv(const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord2s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord2s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x37;        /* __glop_TexCoord2sv */
    data = (struct __gllc_TexCoord2s_Rec *) dlop->data;
    data->s = v[0];
    data->t = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord2sv);    /* __glle_TexCoord2sv */
}

/* 690a9820 -- __gllc_TexCoord3d */
void __stdcall __gllc_TexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x38;        /* __glop_TexCoord3dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord3d_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3dv);    /* __glle_TexCoord3dv */
}

/* 690a98a0 -- __gllc_TexCoord3dv */
void __stdcall __gllc_TexCoord3dv(const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x38;        /* __glop_TexCoord3dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord3d_Rec *) dlop->data;
    data->s = v[0];
    data->t = v[1];
    data->r = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3dv);    /* __glle_TexCoord3dv */
}

/* 690a9920 -- __gllc_TexCoord3f */
void __stdcall __gllc_TexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x39;        /* __glop_TexCoord3fv */
    data = (struct __gllc_TexCoord3f_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3fv);    /* __glle_TexCoord3fv */
}

/* 690a9980 -- __gllc_TexCoord3fv */
void __stdcall __gllc_TexCoord3fv(const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x39;        /* __glop_TexCoord3fv */
    data = (struct __gllc_TexCoord3f_Rec *) dlop->data;
    data->s = v[0];
    data->t = v[1];
    data->r = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3fv);    /* __glle_TexCoord3fv */
}

/* 690a99e0 -- __gllc_TexCoord3i */
void __stdcall __gllc_TexCoord3i(GLint s, GLint t, GLint r)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x3a;        /* __glop_TexCoord3iv */
    data = (struct __gllc_TexCoord3i_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3iv);    /* __glle_TexCoord3iv */
}

/* 690a9a40 -- __gllc_TexCoord3iv */
void __stdcall __gllc_TexCoord3iv(const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x3a;        /* __glop_TexCoord3iv */
    data = (struct __gllc_TexCoord3i_Rec *) dlop->data;
    data->s = v[0];
    data->t = v[1];
    data->r = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3iv);    /* __glle_TexCoord3iv */
}

/* 690a9aa0 -- __gllc_TexCoord3s */
void __stdcall __gllc_TexCoord3s(GLshort s, GLshort t, GLshort r)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x3b;        /* __glop_TexCoord3sv */
    data = (struct __gllc_TexCoord3s_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3sv);    /* __glle_TexCoord3sv */
}

/* 690a9b10 -- __gllc_TexCoord3sv */
void __stdcall __gllc_TexCoord3sv(const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x3b;        /* __glop_TexCoord3sv */
    data = (struct __gllc_TexCoord3s_Rec *) dlop->data;
    data->s = v[0];
    data->t = v[1];
    data->r = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord3sv);    /* __glle_TexCoord3sv */
}

/* 690a9b80 -- __gllc_TexCoord4d */
void __stdcall __gllc_TexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord4d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x3c;        /* __glop_TexCoord4dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord4d_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    data->q = q;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4dv);    /* __glle_TexCoord4dv */
}

/* 690a9c00 -- __gllc_TexCoord4dv */
void __stdcall __gllc_TexCoord4dv(const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord4d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x3c;        /* __glop_TexCoord4dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_TexCoord4d_Rec *) dlop->data;
    data->s = v[0];
    data->t = v[1];
    data->r = v[2];
    data->q = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4dv);    /* __glle_TexCoord4dv */
}

/* 690a9c90 -- __gllc_TexCoord4f */
void __stdcall __gllc_TexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord4f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x3d;        /* __glop_TexCoord4fv */
    data = (struct __gllc_TexCoord4f_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    data->q = q;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4fv);    /* __glle_TexCoord4fv */
}

/* 690a9d00 -- __gllc_TexCoord4fv */
void __stdcall __gllc_TexCoord4fv(const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord4f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x3d;        /* __glop_TexCoord4fv */
    data = (struct __gllc_TexCoord4f_Rec *) dlop->data;
    data->s = v[0];
    data->t = v[1];
    data->r = v[2];
    data->q = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4fv);    /* __glle_TexCoord4fv */
}

/* 690a9d70 -- __gllc_TexCoord4i */
void __stdcall __gllc_TexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord4i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x3e;        /* __glop_TexCoord4iv */
    data = (struct __gllc_TexCoord4i_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    data->q = q;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4iv);    /* __glle_TexCoord4iv */
}

/* 690a9de0 -- __gllc_TexCoord4iv */
void __stdcall __gllc_TexCoord4iv(const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord4i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x3e;        /* __glop_TexCoord4iv */
    data = (struct __gllc_TexCoord4i_Rec *) dlop->data;
    data->s = v[0];
    data->t = v[1];
    data->r = v[2];
    data->q = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4iv);    /* __glle_TexCoord4iv */
}

/* 690a9e50 -- __gllc_TexCoord4s */
void __stdcall __gllc_TexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord4s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x3f;        /* __glop_TexCoord4sv */
    data = (struct __gllc_TexCoord4s_Rec *) dlop->data;
    data->s = s;
    data->t = t;
    data->r = r;
    data->q = q;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4sv);    /* __glle_TexCoord4sv */
}

/* 690a9ec0 -- __gllc_TexCoord4sv */
void __stdcall __gllc_TexCoord4sv(const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_TexCoord4s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_TexCoord4s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x3f;        /* __glop_TexCoord4sv */
    data = (struct __gllc_TexCoord4s_Rec *) dlop->data;
    data->s = v[0];
    data->t = v[1];
    data->r = v[2];
    data->q = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_TEXCOORDS;
    __glDlistAppendOp(gc, dlop, __glle_TexCoord4sv);    /* __glle_TexCoord4sv */
}

/* 690a9fa0 -- __gllc_Vertex2dv */
void __stdcall __gllc_Vertex2dv(const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex2d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x40;        /* __glop_Vertex2dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Vertex2d_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2dv);    /* __glle_Vertex2dv */
}

/* 690aa010 -- __gllc_Vertex2f */
void __stdcall __gllc_Vertex2f(GLfloat x, GLfloat y)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex2f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x41;        /* __glop_Vertex2fv */
    data = (struct __gllc_Vertex2f_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2fv);    /* __glle_Vertex2fv */
}

/* 690aa070 -- __gllc_Vertex2fv */
void __stdcall __gllc_Vertex2fv(const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex2f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x41;        /* __glop_Vertex2fv */
    data = (struct __gllc_Vertex2f_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2fv);    /* __glle_Vertex2fv */
}

/* 690aa0d0 -- __gllc_Vertex2i */
void __stdcall __gllc_Vertex2i(GLint x, GLint y)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex2i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x42;        /* __glop_Vertex2iv */
    data = (struct __gllc_Vertex2i_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2iv);    /* __glle_Vertex2iv */
}

/* 690aa130 -- __gllc_Vertex2iv */
void __stdcall __gllc_Vertex2iv(const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex2i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x42;        /* __glop_Vertex2iv */
    data = (struct __gllc_Vertex2i_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2iv);    /* __glle_Vertex2iv */
}

/* 690aa190 -- __gllc_Vertex2s */
void __stdcall __gllc_Vertex2s(GLshort x, GLshort y)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex2s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x43;        /* __glop_Vertex2sv */
    data = (struct __gllc_Vertex2s_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2sv);    /* __glle_Vertex2sv */
}

/* 690aa1f0 -- __gllc_Vertex2sv */
void __stdcall __gllc_Vertex2sv(const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex2s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex2s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x43;        /* __glop_Vertex2sv */
    data = (struct __gllc_Vertex2s_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex2sv);    /* __glle_Vertex2sv */
}

/* 690aa2d0 -- __gllc_Vertex3dv */
void __stdcall __gllc_Vertex3dv(const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex3d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x44;        /* __glop_Vertex3dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Vertex3d_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3dv);    /* __glle_Vertex3dv */
}

/* 690aa350 -- __gllc_Vertex3f */
void __stdcall __gllc_Vertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x45;        /* __glop_Vertex3fv */
    data = (struct __gllc_Vertex3f_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3fv);    /* __glle_Vertex3fv */
}

/* 690aa3b0 -- __gllc_Vertex3fv */
void __stdcall __gllc_Vertex3fv(const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex3f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x45;        /* __glop_Vertex3fv */
    data = (struct __gllc_Vertex3f_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3fv);    /* __glle_Vertex3fv */
}

/* 690aa410 -- __gllc_Vertex3i */
void __stdcall __gllc_Vertex3i(GLint x, GLint y, GLint z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x46;        /* __glop_Vertex3iv */
    data = (struct __gllc_Vertex3i_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3iv);    /* __glle_Vertex3iv */
}

/* 690aa470 -- __gllc_Vertex3iv */
void __stdcall __gllc_Vertex3iv(const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex3i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x46;        /* __glop_Vertex3iv */
    data = (struct __gllc_Vertex3i_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3iv);    /* __glle_Vertex3iv */
}

/* 690aa4d0 -- __gllc_Vertex3s */
void __stdcall __gllc_Vertex3s(GLshort x, GLshort y, GLshort z)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x47;        /* __glop_Vertex3sv */
    data = (struct __gllc_Vertex3s_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3sv);    /* __glle_Vertex3sv */
}

/* 690aa540 -- __gllc_Vertex3sv */
void __stdcall __gllc_Vertex3sv(const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex3s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex3s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x47;        /* __glop_Vertex3sv */
    data = (struct __gllc_Vertex3s_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex3sv);    /* __glle_Vertex3sv */
}

/* 690aa640 -- __gllc_Vertex4dv */
void __stdcall __gllc_Vertex4dv(const GLdouble *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex4d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x48;        /* __glop_Vertex4dv */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_Vertex4d_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    data->w = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4dv);    /* __glle_Vertex4dv */
}

/* 690aa6d0 -- __gllc_Vertex4f */
void __stdcall __gllc_Vertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex4f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x49;        /* __glop_Vertex4fv */
    data = (struct __gllc_Vertex4f_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4fv);    /* __glle_Vertex4fv */
}

/* 690aa740 -- __gllc_Vertex4fv */
void __stdcall __gllc_Vertex4fv(const GLfloat *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex4f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x49;        /* __glop_Vertex4fv */
    data = (struct __gllc_Vertex4f_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    data->w = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4fv);    /* __glle_Vertex4fv */
}

/* 690aa7b0 -- __gllc_Vertex4i */
void __stdcall __gllc_Vertex4i(GLint x, GLint y, GLint z, GLint w)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex4i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x4a;        /* __glop_Vertex4iv */
    data = (struct __gllc_Vertex4i_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4iv);    /* __glle_Vertex4iv */
}

/* 690aa820 -- __gllc_Vertex4iv */
void __stdcall __gllc_Vertex4iv(const GLint *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex4i_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4i_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x4a;        /* __glop_Vertex4iv */
    data = (struct __gllc_Vertex4i_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    data->w = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4iv);    /* __glle_Vertex4iv */
}

/* 690aa890 -- __gllc_Vertex4s */
void __stdcall __gllc_Vertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex4s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x4b;        /* __glop_Vertex4sv */
    data = (struct __gllc_Vertex4s_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->z = z;
    data->w = w;
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4sv);    /* __glle_Vertex4sv */
}

/* 690aa900 -- __gllc_Vertex4sv */
void __stdcall __gllc_Vertex4sv(const GLshort *v)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Vertex4s_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Vertex4s_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x4b;        /* __glop_Vertex4sv */
    data = (struct __gllc_Vertex4s_Rec *) dlop->data;
    data->x = v[0];
    data->y = v[1];
    data->z = v[2];
    data->w = v[3];
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_VERTEX;
    __glDlistAppendOp(gc, dlop, __glle_Vertex4sv);    /* __glle_Vertex4sv */
}

/* 690aa970 -- __gllc_ClipPlane */
void __stdcall __gllc_ClipPlane(GLenum plane, const GLdouble *equation)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ClipPlane_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ClipPlane_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x4c;        /* __glop_ClipPlane */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_ClipPlane_Rec *) dlop->data;
    data->plane = plane;
    data->equation[0] = equation[0];
    data->equation[1] = equation[1];
    data->equation[2] = equation[2];
    data->equation[3] = equation[3];
    __glDlistAppendOp(gc, dlop, __glle_ClipPlane);    /* __glle_ClipPlane */
}

/* 690aa9f0 -- __gllc_ColorMaterial */
void __stdcall __gllc_ColorMaterial(GLenum face, GLenum mode)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ColorMaterial_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ColorMaterial_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x4d;        /* __glop_ColorMaterial */
    data = (struct __gllc_ColorMaterial_Rec *) dlop->data;
    data->face = face;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_ColorMaterial);    /* __glle_ColorMaterial */
}

/* 690aaa40 -- __gllc_CullFace */
void __stdcall __gllc_CullFace(GLenum mode)
{
    __GLdlistOpICD *dlop;
    struct __gllc_CullFace_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_CullFace_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x4e;        /* __glop_CullFace */
    data = (struct __gllc_CullFace_Rec *) dlop->data;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_CullFace);    /* __glle_CullFace */
}

/* 690aaa90 -- __gllc_Fogf */
void __stdcall __gllc_Fogf(GLenum pname, GLfloat param)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Fogf_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Fogf_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x4f;        /* __glop_Fogf */
    data = (struct __gllc_Fogf_Rec *) dlop->data;
    data->pname = pname;
    data->param = param;
    __glDlistAppendOp(gc, dlop, __glle_Fogf);    /* __glle_Fogf */
}

/* 690aab60 -- __gllc_Fogi */
void __stdcall __gllc_Fogi(GLenum pname, GLint param)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Fogi_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Fogi_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x51;        /* __glop_Fogi */
    data = (struct __gllc_Fogi_Rec *) dlop->data;
    data->pname = pname;
    data->param = param;
    __glDlistAppendOp(gc, dlop, __glle_Fogi);    /* __glle_Fogi */
}

/* 690aac30 -- __gllc_FrontFace */
void __stdcall __gllc_FrontFace(GLenum mode)
{
    __GLdlistOpICD *dlop;
    struct __gllc_FrontFace_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_FrontFace_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x53;        /* __glop_FrontFace */
    data = (struct __gllc_FrontFace_Rec *) dlop->data;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_FrontFace);    /* __glle_FrontFace */
}

/* 690aac80 -- __gllc_Hint */
void __stdcall __gllc_Hint(GLenum target, GLenum mode)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Hint_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Hint_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x54;        /* __glop_Hint */
    data = (struct __gllc_Hint_Rec *) dlop->data;
    data->target = target;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_Hint);    /* __glle_Hint */
}

/* 690aaff0 -- __gllc_LineStipple */
void __stdcall __gllc_LineStipple(GLint factor, GLushort pattern)
{
    __GLdlistOpICD *dlop;
    struct __gllc_LineStipple_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_LineStipple_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x5d;        /* __glop_LineStipple */
    data = (struct __gllc_LineStipple_Rec *) dlop->data;
    data->factor = factor;
    data->pattern = pattern;
    __glDlistAppendOp(gc, dlop, __glle_LineStipple);    /* __glle_LineStipple */
}

/* 690ab040 -- __gllc_LineWidth */
void __stdcall __gllc_LineWidth(GLfloat width)
{
    __GLdlistOpICD *dlop;
    struct __gllc_LineWidth_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_LineWidth_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x5e;        /* __glop_LineWidth */
    data = (struct __gllc_LineWidth_Rec *) dlop->data;
    data->width = width;
    __glDlistAppendOp(gc, dlop, __glle_LineWidth);    /* __glle_LineWidth */
}

/* 690ab290 -- __gllc_PointSize */
void __stdcall __gllc_PointSize(GLfloat size)
{
    __GLdlistOpICD *dlop;
    struct __gllc_PointSize_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PointSize_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x63;        /* __glop_PointSize */
    data = (struct __gllc_PointSize_Rec *) dlop->data;
    data->size = size;
    __glDlistAppendOp(gc, dlop, __glle_PointSize);    /* __glle_PointSize */
}

/* 690ab2e0 -- __gllc_PolygonMode */
void __stdcall __gllc_PolygonMode(GLenum face, GLenum mode)
{
    __GLdlistOpICD *dlop;
    struct __gllc_PolygonMode_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PolygonMode_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x64;        /* __glop_PolygonMode */
    data = (struct __gllc_PolygonMode_Rec *) dlop->data;
    data->face = face;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_PolygonMode);    /* __glle_PolygonMode */
}

/* 690ab330 -- __gllc_Scissor */
void __stdcall __gllc_Scissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Scissor_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Scissor_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x66;        /* __glop_Scissor */
    data = (struct __gllc_Scissor_Rec *) dlop->data;
    data->x = x;
    data->y = y;
    data->width = width;
    data->height = height;
    __glDlistAppendOp(gc, dlop, __glle_Scissor);    /* __glle_Scissor */
}

/* 690ab390 -- __gllc_ShadeModel */
void __stdcall __gllc_ShadeModel(GLenum mode)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ShadeModel_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ShadeModel_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x67;        /* __glop_ShadeModel */
    data = (struct __gllc_ShadeModel_Rec *) dlop->data;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_ShadeModel);    /* __glle_ShadeModel */
}

/* 690ab990 -- __gllc_InitNames (no arguments: kept __fastcall, the same code as the __stdcall entry) */
void __fastcall __gllc_InitNames(void)
{
    __GLdlistOpICD *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = 0x78;        /* __glop_InitNames */
    __glDlistAppendOp(gc, dlop, __glle_InitNames);    /* __glle_InitNames */
}

/* 690ab9d0 -- __gllc_LoadName */
void __stdcall __gllc_LoadName(GLuint name)
{
    __GLdlistOpICD *dlop;
    struct __gllc_LoadName_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_LoadName_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x79;        /* __glop_LoadName */
    data = (struct __gllc_LoadName_Rec *) dlop->data;
    data->name = name;
    __glDlistAppendOp(gc, dlop, __glle_LoadName);    /* __glle_LoadName */
}

/* 690aba20 -- __gllc_PassThrough */
void __stdcall __gllc_PassThrough(GLfloat token)
{
    __GLdlistOpICD *dlop;
    struct __gllc_PassThrough_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PassThrough_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x7a;        /* __glop_PassThrough */
    data = (struct __gllc_PassThrough_Rec *) dlop->data;
    data->token = token;
    __glDlistAppendOp(gc, dlop, __glle_PassThrough);    /* __glle_PassThrough */
}

/* 690aba70 -- __gllc_PopName (no arguments: kept __fastcall, the same code as the __stdcall entry) */
void __fastcall __gllc_PopName(void)
{
    __GLdlistOpICD *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = 0x7b;        /* __glop_PopName */
    __glDlistAppendOp(gc, dlop, __glle_PopName);    /* __glle_PopName */
}

/* 690abab0 -- __gllc_PushName */
void __stdcall __gllc_PushName(GLuint name)
{
    __GLdlistOpICD *dlop;
    struct __gllc_PushName_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PushName_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x7c;        /* __glop_PushName */
    data = (struct __gllc_PushName_Rec *) dlop->data;
    data->name = name;
    __glDlistAppendOp(gc, dlop, __glle_PushName);    /* __glle_PushName */
}

/* 690abb00 -- __gllc_DrawBuffer */
void __stdcall __gllc_DrawBuffer(GLenum mode)
{
    __GLdlistOpICD *dlop;
    struct __gllc_DrawBuffer_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_DrawBuffer_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x7d;        /* __glop_DrawBuffer */
    data = (struct __gllc_DrawBuffer_Rec *) dlop->data;
    data->mode = mode;
    __glDlistAppendOp(gc, dlop, __glle_DrawBuffer);    /* __glle_DrawBuffer */
}

/* 690abb50 -- __gllc_Clear */
void __stdcall __gllc_Clear(GLbitfield mask)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Clear_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Clear_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x7e;        /* __glop_Clear */
    data = (struct __gllc_Clear_Rec *) dlop->data;
    data->mask = mask;
    __glDlistAppendOp(gc, dlop, __glle_Clear);    /* __glle_Clear */
}

/* 690abba0 -- __gllc_ClearAccum */
void __stdcall __gllc_ClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ClearAccum_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ClearAccum_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x7f;        /* __glop_ClearAccum */
    data = (struct __gllc_ClearAccum_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    __glDlistAppendOp(gc, dlop, __glle_ClearAccum);    /* __glle_ClearAccum */
}

/* 690abc00 -- __gllc_ClearIndex */
void __stdcall __gllc_ClearIndex(GLfloat c)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ClearIndex_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ClearIndex_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x80;        /* __glop_ClearIndex */
    data = (struct __gllc_ClearIndex_Rec *) dlop->data;
    data->c = c;
    __glDlistAppendOp(gc, dlop, __glle_ClearIndex);    /* __glle_ClearIndex */
}

/* 690abc50 -- __gllc_ClearColor */
void __stdcall __gllc_ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ClearColor_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ClearColor_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x81;        /* __glop_ClearColor */
    data = (struct __gllc_ClearColor_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    __glDlistAppendOp(gc, dlop, __glle_ClearColor);    /* __glle_ClearColor */
}

/* 690abcb0 -- __gllc_ClearStencil */
void __stdcall __gllc_ClearStencil(GLint s)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ClearStencil_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ClearStencil_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x82;        /* __glop_ClearStencil */
    data = (struct __gllc_ClearStencil_Rec *) dlop->data;
    data->s = s;
    __glDlistAppendOp(gc, dlop, __glle_ClearStencil);    /* __glle_ClearStencil */
}

/* 690abd00 -- __gllc_ClearDepth */
void __stdcall __gllc_ClearDepth(GLclampd depth)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ClearDepth_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ClearDepth_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x83;        /* __glop_ClearDepth */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_ClearDepth_Rec *) dlop->data;
    data->depth = depth;
    __glDlistAppendOp(gc, dlop, __glle_ClearDepth);    /* __glle_ClearDepth */
}

/* 690abd50 -- __gllc_StencilMask */
void __stdcall __gllc_StencilMask(GLuint mask)
{
    __GLdlistOpICD *dlop;
    struct __gllc_StencilMask_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_StencilMask_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x84;        /* __glop_StencilMask */
    data = (struct __gllc_StencilMask_Rec *) dlop->data;
    data->mask = mask;
    __glDlistAppendOp(gc, dlop, __glle_StencilMask);    /* __glle_StencilMask */
}

/* 690abda0 -- __gllc_ColorMask */
void __stdcall __gllc_ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    __GLdlistOpICD *dlop;
    struct __gllc_ColorMask_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_ColorMask_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x85;        /* __glop_ColorMask */
    data = (struct __gllc_ColorMask_Rec *) dlop->data;
    data->red = red;
    data->green = green;
    data->blue = blue;
    data->alpha = alpha;
    __glDlistAppendOp(gc, dlop, __glle_ColorMask);    /* __glle_ColorMask */
}

/* 690abe00 -- __gllc_DepthMask */
void __stdcall __gllc_DepthMask(GLboolean flag)
{
    __GLdlistOpICD *dlop;
    struct __gllc_DepthMask_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_DepthMask_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x86;        /* __glop_DepthMask */
    data = (struct __gllc_DepthMask_Rec *) dlop->data;
    data->flag = flag;
    __glDlistAppendOp(gc, dlop, __glle_DepthMask);    /* __glle_DepthMask */
}

/* 690abe50 -- __gllc_IndexMask */
void __stdcall __gllc_IndexMask(GLuint mask)
{
    __GLdlistOpICD *dlop;
    struct __gllc_IndexMask_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_IndexMask_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x87;        /* __glop_IndexMask */
    data = (struct __gllc_IndexMask_Rec *) dlop->data;
    data->mask = mask;
    __glDlistAppendOp(gc, dlop, __glle_IndexMask);    /* __glle_IndexMask */
}

/* 690abea0 -- __gllc_Accum */
void __stdcall __gllc_Accum(GLenum op, GLfloat value)
{
    __GLdlistOpICD *dlop;
    struct __gllc_Accum_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_Accum_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x88;        /* __glop_Accum */
    data = (struct __gllc_Accum_Rec *) dlop->data;
    data->op = op;
    data->value = value;
    __glDlistAppendOp(gc, dlop, __glle_Accum);    /* __glle_Accum */
}

/* 690abef0 -- __gllc_PopAttrib (no arguments: kept __fastcall, the same code as the __stdcall entry) */
void __fastcall __gllc_PopAttrib(void)
{
    __GLdlistOpICD *dlop;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    dlop->opcode = 0x8b;        /* __glop_PopAttrib */
    __glDlistAppendOp(gc, dlop, __glle_PopAttrib);    /* __glle_PopAttrib */
}

/* 690abf30 -- __gllc_PushAttrib */
void __stdcall __gllc_PushAttrib(GLbitfield mask)
{
    __GLdlistOpICD *dlop;
    struct __gllc_PushAttrib_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_PushAttrib_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x8c;        /* __glop_PushAttrib */
    data = (struct __gllc_PushAttrib_Rec *) dlop->data;
    data->mask = mask;
    __glDlistAppendOp(gc, dlop, __glle_PushAttrib);    /* __glle_PushAttrib */
}

/* 690abf80 -- __gllc_MapGrid1d */
void __stdcall __gllc_MapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MapGrid1d_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MapGrid1d_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x91;        /* __glop_MapGrid1d */
    dlop->aligned = GL_TRUE;
    data = (struct __gllc_MapGrid1d_Rec *) dlop->data;
    data->un = un;
    data->u1 = u1;
    data->u2 = u2;
    __glDlistAppendOp(gc, dlop, __glle_MapGrid1d);    /* __glle_MapGrid1d */
}

/* 690abfe0 -- __gllc_MapGrid1f */
void __stdcall __gllc_MapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
    __GLdlistOpICD *dlop;
    struct __gllc_MapGrid1f_Rec *data;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_MapGrid1f_Rec));
    if (dlop == NULL) return;
    dlop->opcode = 0x92;        /* __glop_MapGrid1f */
    data = (struct __gllc_MapGrid1f_Rec *) dlop->data;
    data->un = un;
    data->u1 = u1;
    data->u2 = u2;
    __glDlistAppendOp(gc, dlop, __glle_MapGrid1f);    /* __glle_MapGrid1f */
}

/*
** gl_varrayext.c -- original VA range 690b0000..690b4000: s_varray.c (__glFreeVertexArrayState, the
** EXT_vertex_array entries, glEdgeFlagPointer, the list-compile ArrayElement / DrawArraysEXT),
** s_lcomp.c list-execute functions (PolygonStipple, Tex[Sub]Image1D / 2D, Disable, Enable) and
** dl_list.c glCallList while compiling / executing (DoCallList).
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "siprotos.h"
#include "icdprotos.h"

#define __GL_MAX_LIST_NESTING       64
#define VERTARRAY_E_MASK            0x08000000      /* vertexArray.formats: edge flag array set */

typedef void (__fastcall *__GLfreeProcB00)(__GLcontext *gc, void *p);
typedef void (APIENTRY *__GLcapProcB00)(GLenum cap);

/* the list-op records of s_lcomp.c */
struct __gllc_CallList_Rec { GLuint list; };
struct __gllc_Disable_Rec { GLenum cap; };
struct __gllc_Enable_Rec { GLenum cap; };
struct __gllc_TexImage1D_Rec {
    GLenum target; GLint level; GLint internalformat; GLsizei width; GLint border;
    GLenum format; GLenum type; GLint imageSize;
};
struct __gllc_TexImage2D_Rec {
    GLenum target; GLint level; GLint internalformat; GLsizei width; GLsizei height; GLint border;
    GLenum format; GLenum type; GLint imageSize;
};
struct __gllc_TexSubImage1D_Rec {
    GLenum target; GLint level; GLint xoffset; GLsizei width; GLenum format; GLenum type; GLint imageSize;
};
struct __gllc_TexSubImage2D_Rec {
    GLenum target; GLint level; GLint xoffset; GLint yoffset; GLsizei width; GLsizei height;
    GLenum format; GLenum type; GLint imageSize;
};

/* ---- forward declarations ---- */
void __fastcall CopyEdgeFlag(__GLcontext *gc, const GLboolean *ep, __GLvertexICD *v);
void __stdcall __glim_EdgeFlagPointer(GLint stride, const GLboolean *pointer);
void __fastcall __gllei_PolygonStipple(__GLcontext *gc, const GLubyte *bits);
const GLubyte *__fastcall __glle_CallList(const GLubyte *PC);

/* 690b0aa0 -- __glFreeVertexArrayState (s_varray.c): the ICD's vertex cache */
void __fastcall __glFreeVertexArrayState(__GLcontext *gc)
{
    if (gc->vertexArray.cacheBase != 0) {
        (*(__GLfreeProcB00)gc->imports[3])(gc, (void *)gc->vertexArray.cacheAlloc);
        gc->vertexArray.cacheBase = 0;
        gc->vertexArray.vertexCache = 0;
        gc->vertexArray.cacheBytes = 0;
    }
}

/* 690b0c10 -- __glim_ArrayElementEXT (s_varray.c) */
void __stdcall __glim_ArrayElementEXT(GLint i)
{
    __glim_ArrayElement(i);
}

/* 690b0da0 -- __glim_DrawArraysEXT (s_varray.c) */
void __stdcall __glim_DrawArraysEXT(GLenum mode, GLint first, GLsizei count)
{
    __glim_DrawArrays(mode, first, count);
}

/* 690b16e0 -- __glim_EdgeFlagPointer (s_varray.c) */
void __stdcall __glim_EdgeFlagPointer(GLint stride, const GLboolean *pointer)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (stride < 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }

    gc->vertexArray.edgeFlag.fetch = (__GLproc)glEdgeFlagv;
    gc->vertexArray.edgeFlag.fetch2 = (__GLproc)CopyEdgeFlag;     /* CopyEdgeFlag */
    gc->vertexArray.edgeFlag.pointer = pointer;
    gc->vertexArray.edgeFlag.stride = stride ? stride : sizeof(GLboolean);
    gc->vertexArray.edgeFlag.userStride = stride;
    gc->vertexArray.formats = gc->vertexArray.formats | VERTARRAY_E_MASK;
    gc->vertexArray.edgeFlag.count = 0;
    gc->vertexArray.interleavedFormat = GL_NONE;

    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
}

/* 690b17c0 -- CopyEdgeFlag (s_varray.c): OR the edge flag bit into flagBits if *ep is set (SI: boundaryEdge = *ep) */
void __fastcall CopyEdgeFlag(__GLcontext *gc, const GLboolean *ep, __GLvertexICD *v)
{
    v->flagBits |= ep[0] ? __GL_VERTEX_EDGE_FLAG : 0;
}

/* 690b17e0 -- __gllc_ArrayElement (s_varray.c): the elements are fetched while compiling */
void __stdcall __gllc_ArrayElement(GLint i)
{
    __glim_ArrayElement(i);
}

/* 690b17f0 -- __gllc_ArrayElementEXT (s_varray.c) */
void __stdcall __gllc_ArrayElementEXT(GLint i)
{
    __glim_ArrayElement(i);
}

/* 690b1a00 -- __gllc_DrawArraysEXT (s_varray.c) */
void __stdcall __gllc_DrawArraysEXT(GLenum mode, GLint first, GLsizei count)
{
    __gllc_DrawArrays(mode, first, count);
}

/* 690b1d30 -- __glim_NormalPointerEXT (s_varray.c) */
void __stdcall __glim_NormalPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (count < 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    __glim_NormalPointer(type, stride, pointer);
    gc->vertexArray.normal.count = count;   /* XXX only if no errors occured */
}

/* 690b1da0 -- __glim_ColorPointerEXT (s_varray.c) */
void __stdcall __glim_ColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (count < 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    __glim_ColorPointer(size, type, stride, pointer);
    gc->vertexArray.color.count = count;    /* XXX only if no errors occured */
}

/* 690b1e20 -- __glim_IndexPointerEXT (s_varray.c) */
void __stdcall __glim_IndexPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (count < 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    __glim_IndexPointer(type, stride, pointer);
    gc->vertexArray.index.count = count;
}

/* 690b1e90 -- __glim_TexCoordPointerEXT (s_varray.c): the count of the client active unit */
void __stdcall __glim_TexCoordPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (count < 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    __glim_TexCoordPointer(size, type, stride, pointer);
    gc->vertexArray.texture.count[gc->fx.clientActiveTexture] = count;
}

/* 690b1f10 -- __glim_EdgeFlagPointerEXT (s_varray.c) */
void __stdcall __glim_EdgeFlagPointerEXT(GLint stride, GLsizei count, const GLboolean *pointer)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (count < 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    __glim_EdgeFlagPointer(stride, pointer);
    gc->vertexArray.edgeFlag.count = count;
}

/* 690b2200 -- __gllei_PolygonStipple (s_lcomp.c): copy the bits, convertPolygonStipple does the rest */
void __fastcall __gllei_PolygonStipple(__GLcontext *gc, const GLubyte *bits)
{
    if (__GL_IN_BEGIN()) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }

    __GL_MEMCOPY(&gc->state.polygonStipple.stipple[0], bits, sizeof(gc->state.polygonStipple.stipple));
    (*gc->procs.convertPolygonStipple)(gc, bits);
}

/* 690b22d0 -- __glle_PolygonStipple (s_lcomp.c) */
const GLubyte *__fastcall __glle_PolygonStipple(const GLubyte *PC)
{
    __gllei_PolygonStipple(__glCurrentGC(), PC);
    return PC + __glImageSize(32, 32, GL_COLOR_INDEX, GL_BITMAP);
}

/* 690b2aa0 -- __glle_TexImage1D (s_lcomp.c) */
const GLubyte *__fastcall __glle_TexImage1D(const GLubyte *PC)
{
    const struct __gllc_TexImage1D_Rec *data = (const struct __gllc_TexImage1D_Rec *)PC;

    __gllei_TexImage1D(__glCurrentGC(), data->target, data->level, data->internalformat,
                 data->width, data->border, data->format, data->type, PC + sizeof(*data));
    return PC + sizeof(*data) + __GL_PAD(data->imageSize);
}

/* 690b2af0 -- __glle_TexImage2D (s_lcomp.c) */
const GLubyte *__fastcall __glle_TexImage2D(const GLubyte *PC)
{
    const struct __gllc_TexImage2D_Rec *data = (const struct __gllc_TexImage2D_Rec *)PC;

    __gllei_TexImage2D(__glCurrentGC(), data->target, data->level, data->internalformat,
                 data->width, data->height, data->border, data->format, data->type, PC + sizeof(*data));
    return PC + sizeof(*data) + __GL_PAD(data->imageSize);
}

/* 690b2fd0 -- __glle_TexSubImage1D (s_lcomp.c) */
const GLubyte *__fastcall __glle_TexSubImage1D(const GLubyte *PC)
{
    const struct __gllc_TexSubImage1D_Rec *data = (const struct __gllc_TexSubImage1D_Rec *)PC;

    __gllei_TexSubImage1D(__glCurrentGC(), data->target, data->level, data->xoffset,
                 data->width, data->format, data->type, PC + sizeof(*data));
    return PC + sizeof(*data) + __GL_PAD(data->imageSize);
}

/* 690b3200 -- __glle_TexSubImage2D (s_lcomp.c) */
const GLubyte *__fastcall __glle_TexSubImage2D(const GLubyte *PC)
{
    const struct __gllc_TexSubImage2D_Rec *data = (const struct __gllc_TexSubImage2D_Rec *)PC;

    __gllei_TexSubImage2D(__glCurrentGC(), data->target, data->level, data->xoffset, data->yoffset,
                 data->width, data->height, data->format, data->type, PC + sizeof(*data));
    return PC + sizeof(*data) + __GL_PAD(data->imageSize);
}

/* 690b3440 -- __glle_Disable (s_lcomp.c): through the current dispatch table (glDisable, slot 215) */
const GLubyte *__fastcall __glle_Disable(const GLubyte *PC)
{
    const struct __gllc_Disable_Rec *data = (const struct __gllc_Disable_Rec *)PC;

    (*(__GLcapProcB00)__glCurrentGC()->currentDispatchState.fn[215])(data->cap);
    return PC + sizeof(struct __gllc_Disable_Rec);
}

/* 690b3600 -- __glle_Enable (s_lcomp.c): glEnable, slot 216 */
const GLubyte *__fastcall __glle_Enable(const GLubyte *PC)
{
    const struct __gllc_Enable_Rec *data = (const struct __gllc_Enable_Rec *)PC;

    (*(__GLcapProcB00)__glCurrentGC()->currentDispatchState.fn[216])(data->cap);
    return PC + sizeof(struct __gllc_Enable_Rec);
}

/* 690b3d30 -- __gllc_CallList (dl_list.c) */
void __stdcall __gllc_CallList(GLuint list)
{
    __GLdlistOpICD *dlop;
    struct __gllc_CallList_Rec *data;
    __GL_SETUP();

    if (list == 0) {
        __gllc_InvalidEnum(gc);
        return;
    }

    dlop = __glDlistAllocOp2(gc, sizeof(struct __gllc_CallList_Rec));
    if (dlop == NULL) return;
    dlop->opcode = __glop_CallList;
    data = (struct __gllc_CallList_Rec *)dlop->data;
    data->list = list;
    __glDlistAppendOp(gc, dlop, __glle_CallList);
}

/* 690b3d90 -- __glle_CallList (dl_list.c) */
const GLubyte *__fastcall __glle_CallList(const GLubyte *PC)
{
    const struct __gllc_CallList_Rec *data = (const struct __gllc_CallList_Rec *)PC;

    DoCallList(data->list);
    return PC + sizeof(struct __gllc_CallList_Rec);
}

/* 690b3da0 -- DoCallList (dl_list.c): execute display list `list` with it locked in the names array (unknown
** names do nothing); past __GL_MAX_LIST_NESTING it sets dlist.nesting to twice the limit so the outer lists
** unwind, else nesting is counted up around the call */
void __fastcall DoCallList(GLuint list)
{
    __GLdlistICD *dlist;
    __GL_SETUP();

    if (gc->dlist.nesting >= __GL_MAX_LIST_NESTING) {
        /* force unwinding of the display list */
        gc->dlist.nesting = __GL_MAX_LIST_NESTING * 2;
        return;
    }

    if ((dlist = __glNamesLockData(gc, (__GLnamesArray *)gc->dlist.namesArray, list)) == NULL) {
        return;
    }

    gc->dlist.nesting++;

    /* no longer check for empty dlist: empty dlist does nothing */
    (*dlist->execute)(gc, dlist);

    gc->dlist.nesting--;

    __glNamesUnlockData(gc, dlist);
}

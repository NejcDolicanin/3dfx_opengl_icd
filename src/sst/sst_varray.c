/*
** sst_varray.c -- vertex arrays of the hardware path: the field-major vertex arrays and cache, the gl*Pointer
** entries, glDrawElements and the locked-triangle check.
*/
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "icdglob.h"
#include "icdprotos.h"
#include "si_names.h"
#include "glshade.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "og.h"
#include "glsitypes.h"

/* 690eefb4 -- vertex arrays: per GL type (GL_BYTE .. GL_DOUBLE, 0x1400..0x140a) 5 strides by size 0..4, then
** [55 + type - GL_BYTE] the hardware format of the type */
static GLuint g_vaTypeInfo[67] = {
    0, 1, 2, 3, 4,   /* GL_BYTE */
    0, 1, 2, 3, 4,   /* GL_UNSIGNED_BYTE */
    0, 2, 4, 6, 8,   /* GL_SHORT */
    0, 2, 4, 6, 8,   /* GL_UNSIGNED_SHORT */
    0, 4, 8, 12, 16,   /* GL_INT */
    0, 4, 8, 12, 16,   /* GL_UNSIGNED_INT */
    0, 4, 8, 12, 16,   /* GL_FLOAT */
    0, 0, 0, 0, 0,   /* GL_2_BYTES */
    0, 0, 0, 0, 0,   /* GL_3_BYTES */
    0, 0, 0, 0, 0,   /* GL_4_BYTES */
    0, 8, 16, 24, 32,   /* GL_DOUBLE */
    0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x0, 0x0, 0x0, 0xb, 0x0   /* the hardware format by type */
};
#define g_vaTypeStride(type, size)  ((GLint)g_vaTypeInfo[(size) + ((type) - GL_BYTE) * 5])
#define g_vaTypeFormat(type)        g_vaTypeInfo[(type) + (55 - GL_BYTE)]

/* 693c3ce4, 693c3d00 -- glDrawElements calls this frame and the last (wglDevSwapBuffers) */
GLuint g_elementsThisFrame;
GLuint g_elementsLastFrame;

/************************************************************************/

/* gc->imports: malloc [0], realloc [2] */
#define IMPORT_MALLOC(gc, n) \
    ((GLuint (__fastcall *)(__GLcontext *, size_t)) (gc)->imports[0])(gc, n)
#define IMPORT_REALLOC(gc, p, n) \
    ((GLuint (__fastcall *)(__GLcontext *, GLuint, size_t)) (gc)->imports[2])(gc, p, n)
/************************************************************************/

/* the default strides [type - GL_BYTE][size] of the vertex arrays (690eefb4, 11 x 5), followed by the
** vertex-format code of each GL type (690ef090 = g_vaTypeInfo[55..65]: GL_BYTE 1 .. GL_FLOAT 7, GL_DOUBLE 0xb).
** The original reads the codes as 690ea090[type]: the address of 690ef090[type - GL_BYTE] with the bias
** folded in (690ea090 itself is unrelated .text).  Indexing g_vaTypeInfo with the bias
** written out gives the same instruction and the original's effective address ([type*4 - 0x4f24] + table).
** Both gl*Pointer copies below are unreferenced in the original and in ours (dead code).  The table:
** g_vaTypeStride / g_vaTypeFormat (icdglob.h). */
#define STRIP(T)                                    \
    {                                                   \
        const T *p = (const T *)indices;                \
        v0 = p[0];                                      \
        v2 = p[1];                                      \
        for (i = 2; i < (GLuint)count; ) {              \
            if (i & 1) v0 = v2; else v1 = v2;           \
            v2 = p[i++];                                \
            dst[0] = v0;                                \
            dst[1] = v1;                                \
            dst[2] = v2;                                \
            dst += 3;                                   \
        }                                               \
    }
#define FAN(T)                                      \
    {                                                   \
        const T *p = (const T *)indices;                \
        v0 = p[0];                                      \
        v2 = p[1];                                      \
        for (i = 2; i < (GLuint)count; ) {              \
            v1 = v2;                                    \
            v2 = p[i++];                                \
            dst[0] = v0;                                \
            dst[1] = v1;                                \
            dst[2] = v2;                                \
            dst += 3;                                   \
        }                                               \
    }
/* ---- glDrawElements of the hardware path ---- */
typedef void *(__fastcall *__GLfn_malloc)(__GLcontext *gc, size_t size);
typedef void *(__fastcall *__GLfn_realloc)(__GLcontext *gc, void *p, size_t size);
extern void __fastcall hwExpandTriangleIndices(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
extern GLint __fastcall hwCanDrawLockedTriangles(__GLcontext *gc);

/* 69019b10 (size 170) -- sstVarray_AllocateVbuf: AllocateVbuf of the hardware path (the software one is 690b09d0,
** gl_varray.c): the count rounded up to 4 vertices, the cache indexed from its base whatever `first` */
GLboolean __fastcall sstVarray_AllocateVbuf(__GLcontext *gc, GLint first, GLint count)
{
    int bufSize, currentBufSize;

    bufSize = ((count + 3) & ~3) * sizeof(__GLvertexICD);
    currentBufSize = gc->vertexArray.cacheBytes;

    if (bufSize > currentBufSize) {
	int blockSize = gc->vertexArray.cacheGranule;
	GLuint vBuf = gc->vertexArray.cacheAlloc;
	__GLvertexICD *vx;

	bufSize = ((bufSize + (blockSize-1)) / blockSize) * blockSize;
	bufSize += 32;
	if (currentBufSize && vBuf) {
	    vBuf = IMPORT_REALLOC(gc, vBuf, (size_t)bufSize);
	} else {
	    vBuf = IMPORT_MALLOC(gc, (size_t)bufSize);
	}
	gc->vertexArray.cacheAlloc = vBuf;
	vx = (__GLvertexICD *) ((vBuf & 0xFFFFFFE0UL) + 32);
	gc->vertexArray.cacheBytes = bufSize;
	gc->vertexArray.cacheBase = vx;
	{
	    int i, n = bufSize / sizeof(__GLvertexICD);

	    for (i=0; i<n; i++, vx++) {
		vx->color = &vx->colors[0];
	    }
	}
	gc->vaCacheBase = gc->vertexArray.cacheBase;
    }
    gc->vertexArray.vertexCache = gc->vertexArray.cacheBase;
    return GL_TRUE;
}

/* 69019bc0 -- hwGrowVertexArrays: grows the field-major hardware vertex arrays to hold n vertices: the capacity
** (hwcx.vtxCapacity) rounded up to 1024 plus 0x80, (capacity + 1) * 0x20 bytes from imports.malloc (the block in
** hwcx.vtxAlloc), vtxBase its 32-byte aligned start; dirtyMask 0x20000000 (the generated code depends on them).  TRUE
** when reallocated */
GLboolean __fastcall hwGrowVertexArrays(__GLcontext *gc, GLint n)
{
    void *block;

    if (n > gc->hwcx.vtxCapacity) {
        gc->hwcx.vtxCapacity = ((n + 0x3ff) & ~0x3ff) + 0x80;
        if (gc->hwcx.vtxAlloc != NULL) {
            __GL_FREE(gc, gc->hwcx.vtxAlloc);
        }
        block = __GL_MALLOC(gc, (gc->hwcx.vtxCapacity + 1) * 0x20);
        gc->hwcx.vtxAlloc = block;
        gc->hwcx.vtxBase = (GLubyte *)(((GLuint)block + 0x1f) & ~0x1f);
        gc->dirtyMask |= __GL_DIRTY_HWVERTEXBUF_ICD;
        return GL_TRUE;
    }
    return GL_FALSE;
}

/* 69019d50 (size 128): hwPrepareVertexCache: vertexArray.prepareCache of the hardware path: sizes the vertex cache for the
** index range (or first .. first + count) and fills it */
void __fastcall hwPrepareVertexCache(__GLcontext *gc, GLuint unused, GLint first, GLsizei count, const GLuint *indices)
{
    if (indices) {
        GLuint minIndex = 0xffffffff, maxIndex = 0;
        GLint i;

        for (i = 0; i < count; i++) {
            GLuint x = indices[first + i];
            if (x < minIndex) minIndex = x;
            if (maxIndex < x) maxIndex = x;
        }
        sstVarray_AllocateVbuf(gc, 0, maxIndex + 1);
        ((void (__fastcall *)(__GLcontext *, GLint, GLint, GLint))gc->vertexArray.fillLockedRange)(gc,
            minIndex, minIndex, maxIndex - minIndex + 1);
    } else {
        sstVarray_AllocateVbuf(gc, first, first + count);
        ((void (__fastcall *)(__GLcontext *, GLint, GLint, GLint))gc->vertexArray.fillLockedRange)(gc,
            first, first, count);
    }
}

/************************************************************************/
#pragma optimize("y", off)      /* the original keeps the ebp frame here (no frame-pointer omission) */
/* 6901f1f0 (size 329) -- __glsstim_VertexPointer: (unreferenced) glVertexPointer of the hardware path: no fetch
** procs; an unchanged format only flags the cache */
void __fastcall __glsstim_VertexPointer(GLuint ecx_, GLuint edx_, GLint size, GLenum type, GLsizei stride,
                             const GLvoid *pointer)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (stride < 0 || size < 2 || size > 4) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    if (type < GL_BYTE || type > GL_DOUBLE) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->vertexArray.vertex.pointer = pointer;
    if (gc->vertexArray.vertex.size == size && gc->vertexArray.vertex.type == type
        && gc->vertexArray.vertex.userStride == stride) {
        gc->vertexArray.flags |= 0x20002;
        return;
    }
    gc->vertexArray.vertex.size = size;
    gc->vertexArray.vertex.type = type;
    gc->vertexArray.vertex.userStride = stride;
    gc->vertexArray.vertex.stride = stride ? stride : g_vaTypeStride(type, size);
    gc->vertexArray.formats = g_vaTypeFormat(type) | (gc->vertexArray.formats & 0xffffff00) | (size << 4);
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
}
#pragma optimize("", on)

#pragma optimize("y", off)
/* 6901f340 (size 338) -- __glsstim_ColorPointer: (unreferenced) glColorPointer of the hardware path (the software
** one is 690b1260, gl_vadraw.c): no fetch procs; an unchanged format only flags the cache */
void __fastcall __glsstim_ColorPointer(GLuint ecx_, GLuint edx_, GLint size, GLenum type, GLsizei stride,
                             const GLvoid *pointer)
{
    GLsizei s;
    __GL_SETUP_NOT_IN_BEGIN();

    if (stride < 0 || size < 3 || size > 4) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    if (type < GL_BYTE || type > GL_DOUBLE) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->vertexArray.color.pointer = pointer;
    s = stride ? stride : g_vaTypeStride(type, size);
    if (gc->vertexArray.color.size == size && gc->vertexArray.color.type == type
        && gc->vertexArray.color.userStride == stride && gc->vertexArray.color.stride == s) {
        hwArrayPointerChanged(gc);
        return;
    }
    gc->vertexArray.color.size = size;
    gc->vertexArray.color.type = type;
    gc->vertexArray.color.userStride = stride;
    gc->vertexArray.color.stride = s;
    gc->vertexArray.formats = ((g_vaTypeFormat(type) | (size << 4)) << 12) |
        (gc->vertexArray.formats & 0xfff00fff);
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
}
#pragma optimize("", on)

/* g_vaTypeStride / g_vaTypeFormat (icdglob.h): the default strides [type - GL_BYTE][size] of the vertex arrays
** (690eefb4, 11 x 5), followed by the vertex-format code of each GL type (690ef090 = g_vaTypeInfo[55..65]:
** GL_BYTE 1 .. GL_FLOAT 7, GL_DOUBLE 0xb).
** The original reads the codes as 690ea090[type], the address of 690ef090[type - GL_BYTE] with the bias
** folded in (690ea090 itself is unrelated .text).
** Indexing g_vaTypeInfo with the bias written out gives the same instruction and the original's effective
** address (as the two copies above; all three gl*Pointer functions are unreferenced, dead code). */

/* 6901f4d0 -- __glsstim_TexCoordPointer: (unreferenced) glTexCoordPointer of the client-active unit, as a fastcall
** with the two register arguments unused (edx is handed to procs.nonUnityQ as is): size 4 reports a non-unity q; the
** array record, the stride (the type's element size when 0) and the unit's format bits (20..27: format code | size <<
** 4). The original indexes the format codes as 690ea090[type], the folded address of 690ef090[type - GL_BYTE] */
void __fastcall __glsstim_TexCoordPointer(GLuint ecx_unused, GLuint edx, GLint size, GLenum type, GLsizei stride,
                             const GLvoid *pointer)
{
    GLint effStride;
    __GL_SETUP_NOT_IN_BEGIN();

    if (stride >= 0 && size >= 1 && size <= 4) {
        switch (type) {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        case GL_INT:
        case GL_UNSIGNED_INT:
        case GL_FLOAT:
        case GL_2_BYTES:
        case GL_3_BYTES:
        case GL_4_BYTES:
        case GL_DOUBLE:
            if (3 < size) {
                (*(void (__fastcall *)(__GLcontext *, GLuint))gc->procs.nonUnityQ)(gc, edx);
                __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_GENERIC);
            }
            effStride = stride;
            if (stride == 0) {
                effStride = g_vaTypeStride(type, size);
            }
            gc->vertexArray.texture.pointer[gc->fx.clientActiveTexture] = pointer;
            gc->vertexArray.texture.size[gc->fx.clientActiveTexture] = size;
            gc->vertexArray.texture.type[gc->fx.clientActiveTexture] = type;
            gc->vertexArray.texture.userStride[gc->fx.clientActiveTexture] = stride;
            gc->vertexArray.texture.stride[gc->fx.clientActiveTexture] = effStride;
            gc->vertexArray.texFormats[gc->fx.clientActiveTexture] =
                ((g_vaTypeFormat(type) | size << 4) << 20) |
                (gc->vertexArray.texFormats[gc->fx.clientActiveTexture] & 0xf80fffff);
            __GL_DELAY_VALIDATE_MASK(gc, 0x200);
            return;
        default:
            __glSetError(GL_INVALID_ENUM);
            return;
        }
    }
    __glSetError(GL_INVALID_VALUE);
}

/*
** 6904cbf0 -- hwExpandTriangleIndices: glDrawElements helper: expands the index list of a GL_TRIANGLES (GLushort / GLubyte),
** GL_TRIANGLE_STRIP or GL_TRIANGLE_FAN draw into a GLuint triangle list in hwcx.growBufA (32-byte
** aligned).  Strips keep the winding by replacing the first or the second vertex in turn.
*/
void __fastcall hwExpandTriangleIndices(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    GLuint *dst = (GLuint *)(((GLuint)__glCurrentGC()->hwcx.growBufA & ~0x1f) + 0x20);
    GLuint i, v0, v1, v2;
    GLint j;

    switch (mode) {
      case GL_TRIANGLES:
        if (type == GL_UNSIGNED_INT) {
        } else if (type == GL_UNSIGNED_SHORT) {
            for (j = 0; j < count; j++) *dst++ = ((const GLushort *)indices)[j];
        } else {
            for (j = 0; j < count; j++) *dst++ = ((const GLubyte *)indices)[j];
        }
        break;
      case GL_TRIANGLE_STRIP:
        if (type == GL_UNSIGNED_INT) {
            STRIP(GLuint)
        } else if (type == GL_UNSIGNED_SHORT) {
            STRIP(GLushort)
        } else {
            STRIP(GLubyte)
        }
        break;
      case GL_TRIANGLE_FAN:
        if (type == GL_UNSIGNED_INT) {
            FAN(GLuint)
        } else if (type == GL_UNSIGNED_SHORT) {
            FAN(GLushort)
        } else {
            FAN(GLubyte)
        }
        break;
    }
}

/*
** 6904ce50 -- __glsstim_DrawElements: glDrawElements (hardware path).  Strips, fans and non-GLuint triangle lists
** are turned into a GLuint triangle list in hwcx.growBufA (6904cbf0) first; a locked GL_TRIANGLES list is then
** drawn from the locked vertex cache, through the generated proc when 6904d270 picks one, or through
** the FX_GL_HSR depth pre-pass (an __asm copy of every front-most triangle into the FIFO word at
** 0x3820c); anything else goes to the generic __glim_DrawElements.
*/
void __stdcall __glsstim_DrawElements(GLenum mode, GLsizei count,
                             GLenum type, const GLvoid *indices)
{
    __GLvertexICD tmp[4];
    GLuint *dst;
    GLint i;
    __GLvertexICD *vc;
    __GLvertexICD *v0, *v1, *v2;
    GLuint level;
    GLint hsrMode;
    GLint convert;
    const GLuint *ip;
    __GLcontext *gc = __glCurrentGC();

    hsrMode = hwCanDrawLockedTriangles(gc);
    if (mode == GL_TRIANGLES) {
        if (type == GL_UNSIGNED_INT) {
            convert = GL_FALSE;
            goto done;
        }
    } else if (mode != GL_TRIANGLE_STRIP && mode != GL_TRIANGLE_FAN) {
        convert = GL_FALSE;
        goto done;
    }
    convert = GL_TRUE;
done:
    level = 0;
    if (gc->fx.fullscreen == 1 && gc->fx.hsrEnable == 1 && g_elementsLastFrame > 5000) {
        level = grGetInteger(GR_PENDING_BUFFERSWAPS);
    }
    g_elementsThisFrame += count;
    if (convert) {
        GLuint n = count;
        if (mode != GL_TRIANGLES) n = count * 3;
        if (n > gc->hwcx.growBufACap) {
            if (gc->hwcx.growBufA) {
                gc->hwcx.growBufA = (*(__GLfn_realloc)gc->imports[2])(gc, gc->hwcx.growBufA, n * 4 + 0x20);
            } else {
                gc->hwcx.growBufA = (*(__GLfn_malloc)gc->imports[0])(gc, n * 4 + 0x20);
            }
            gc->hwcx.growBufACap = n;
        }
        hwExpandTriangleIndices(mode, count, type, indices);
        indices = (const GLvoid *)(((GLuint)gc->hwcx.growBufA & ~0x1f) + 0x20);
        if (mode != GL_TRIANGLES) count = count * 3 - 6;
        mode = GL_TRIANGLES;
        type = GL_UNSIGNED_INT;
    }
    if (count > 0 && mode == GL_TRIANGLES && type == GL_UNSIGNED_INT && (gc->vertexArray.flags & 4) &&
        gc->beginMode != 1 && gc->renderMode != GL_FEEDBACK) {
        if (level <= (GLuint)gc->fx.hsrLevel) {
            if (gc->beginMode == 2 || hsrMode != gc->fx.lastHsrMode) {
                if (hsrMode) {
                    hwValidateFinish(gc);
                } else {
                    (*gc->procs.validate)(gc);
                    gc->vertexArray.flags |= 2;
                }
                gc->beginMode = 0;
                if (hsrMode != gc->fx.lastHsrMode) {
                    gc->fx.lastHsrMode = hsrMode;
                    gc->vertexArray.flags |= 2;
                }
            }
            if (hsrMode) {
                (*gc->hwcx.genProc8)(count, indices);
                return;
            }
            vc = gc->vertexArray.vertexCache;
            if (gc->vertexArray.flags & 2) {
                (*gc->vertexArray.fillLockedRange)(gc, 0, 0, gc->vertexArray.lockCount);
                gc->vertexArray.flags &= ~2;
            }
            (*gc->procs.drawIndexedTriangles)(gc, indices, vc, count - 2);
            return;
        }
        dst = &gc->hwcx.raw_38208[1];
        vc = gc->vertexArray.vertexCache;
        (*gc->vertexArray.fillLockedRange)(gc, 0, 0, gc->vertexArray.lockCount);
        ip = (const GLuint *)indices;
        for (i = 0; i < count; ) {
            GLuint a, b, c;
            a = ip[i++];
            b = ip[i++];
            c = ip[i++];
            v0 = vc + a;
            v1 = vc + b;
            v2 = vc + c;
            /* compared as doubles (fld / fld / fcompp); the cache index is (i & 4), as the original; the
            ** parenthesised first operand gives the original's spill-store scheduling */
            if ((double)(v0->window.z) < tmp[i & 4].window.z && (double)v1->window.z < tmp[(i + 1) & 4].window.z &&
                (double)v2->window.z < tmp[(i + 2) & 4].window.z) {
                __asm {
                    push edi
                    push eax
                    push ebx
                    push ecx
                    push edx
                    mov eax, v0
                    mov ebx, v1
                    mov ecx, v2
                    mov edi, dst
                    mov edx, [eax]
                    mov [edi], edx
                    mov edx, [eax+0x4]
                    mov [edi+0x4], edx
                    mov edx, [eax+0x8]
                    mov [edi+0x8], edx
                    mov edx, [eax+0xc]
                    mov [edi+0xc], edx
                    mov edx, [eax+0x10]
                    mov [edi+0x10], edx
                    mov edx, [eax+0x14]
                    mov [edi+0x14], edx
                    mov edx, [eax+0x18]
                    mov [edi+0x18], edx
                    mov edx, [ebx]
                    mov [edi], edx
                    mov edx, [ebx+0x4]
                    mov [edi+0x4], edx
                    mov edx, [ebx+0x8]
                    mov [edi+0x8], edx
                    mov edx, [ebx+0xc]
                    mov [edi+0xc], edx
                    mov edx, [ebx+0x10]
                    mov [edi+0x10], edx
                    mov edx, [ebx+0x14]
                    mov [edi+0x14], edx
                    mov edx, [ebx+0x18]
                    mov [edi+0x18], edx
                    mov edx, [ecx]
                    mov [edi], edx
                    mov edx, [ecx+0x4]
                    mov [edi+0x4], edx
                    mov edx, [ecx+0x8]
                    mov [edi+0x8], edx
                    mov edx, [ecx+0xc]
                    mov [edi+0xc], edx
                    mov edx, [ecx+0x10]
                    mov [edi+0x10], edx
                    mov edx, [ecx+0x14]
                    mov [edi+0x14], edx
                    mov edx, [ecx+0x18]
                    mov [edi+0x18], edx
                    pop edx
                    pop ecx
                    pop ebx
                    pop eax
                    pop edi
                }
            }
            tmp[0].window.z = v0->window.z;
            tmp[1].window.z = v1->window.z;
            tmp[2].window.z = v2->window.z;
        }
    } else {
        ((void (__stdcall *)(GLenum, GLsizei, GLenum, const GLvoid *))__glim_DrawElements)(mode, count, type, indices);
    }
}

/*
** 6904d270 -- hwCanDrawLockedTriangles: can the locked GL_TRIANGLES arrays go through the generated draw proc
** (hwcx.genProc8)? Direct FIFO, filled polygons both faces, no lighting-ish 0x40 / 0x20 enables, no clip planes,
** GL_RENDER, only vertex (3 floats), colour (4 ubytes, stride 4) and 2-float texcoords (stride 8) per
** enabled unit (`en ? (va & bit) : 1` gives the original's mov/and/test), no texgen, a model-view type that is not
** 3..5 (the original compares the address of the matrix's type word), the 0x1c0000 enables only with 0x100000, and
** q always 1.
*/
GLint __fastcall hwCanDrawLockedTriangles(__GLcontext *gc)
{
    GLuint enables = gc->state.enables.general;
    GLint ok;
    GLuint va;

    if (enables & __GL_POLYGON_OFFSET_ANY_ENABLE) {
        ok = (enables >> 20) & 1;
    } else {
        ok = 1;
    }
    if (gc->hwcx.cmdTransportInfo &&
        gc->state.polygon.frontMode == gc->state.polygon.backMode &&
        gc->state.polygon.frontMode == GL_FILL &&
        !(enables & __GL_LIGHTING_ENABLE) && !gc->state.enables.clipPlanes &&
        gc->renderMode == GL_RENDER &&
        !(enables & __GL_FOG_ENABLE) &&
        ((va = gc->vertexArray.enableFlags) & __GL_VA_VERTEX_ENABLE) && !(va & 0xfffcffdd) &&
        gc->vertexArray.vertex.size == 3 && gc->vertexArray.vertex.type == GL_FLOAT &&
        (!(va & 2) || (gc->vertexArray.color.size == 4 && gc->vertexArray.color.type == GL_UNSIGNED_BYTE &&
                       gc->vertexArray.color.stride == 4)) &&
        (gc->texture.textureEnabled[0] ? (va & 0x10000) : 1) &&
        (gc->texture.textureEnabled[1] ? (va & 0x20000) : 1) &&
        (!(va & 0x10000) || (gc->vertexArray.texture.size[0] == 2 && gc->vertexArray.texture.type[0] == GL_FLOAT &&
                             gc->vertexArray.texture.stride[0] == 8)) &&
        (!(va & 0x20000) || (gc->vertexArray.texture.size[1] == 2 && gc->vertexArray.texture.type[1] == GL_FLOAT &&
                             gc->vertexArray.texture.stride[1] == 8)) &&
        !((gc->state.enables.texture[1] | gc->state.enables.texture[0]) & (__GL_TEXTURE_GEN_S_ENABLE | __GL_TEXTURE_GEN_T_ENABLE)) &&
        (GLint)((GLubyte *)gc->transform.modelView + 0xc0) != 3 &&
        (GLint)((GLubyte *)gc->transform.modelView + 0xc0) != 5 &&
        (GLint)((GLubyte *)gc->transform.modelView + 0xc0) != 4 &&
        ok && !gc->hwcx.nonUnityQ) {
        return 1;
    }
    return 0;
}

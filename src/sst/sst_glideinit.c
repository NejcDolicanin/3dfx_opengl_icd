/*
** sst_glideinit.c -- 3dfx back end, original VA range 6904c000..69054000: glLockArraysEXT / glUnlockArraysEXT of the
** hardware path, the Glide device probes (init, SURFACE / PIXEXT extensions), and og emitters of the
** guardband clipper (hwGenVertexArrayFill, in od_vafill.c).
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include <g3ext.h>
#include "glsi.h"
#include "og.h"
#include "icdprotos.h"

#define ESI(d)          og_mem(gc, (d), R(esi), NULL, 1)        /* [esi + d] (d an immediate operand or NULL) */
#define ECX(d)          og_memBase(gc, IMM(d), R(ecx))          /* [ecx + d] */

/* ---- forward declarations ---- */
void __stdcall __glsstim_LockArraysEXT(GLint first, GLsizei count);
void __fastcall __glsstim_UnlockArraysEXT(void);
void __fastcall __glSSTExtractTexelRGBA8_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result);
GLubyte *__fastcall og_matVec3x87(__GLcontext *gc, GLubyte *pc, GLuint matrix, OgOperand *outX, OgOperand *outY,
                                 OgOperand *outZ);
GLubyte *__fastcall og_callOperand(__GLcontext *gc, GLubyte *pc, OgOperand *target, OgOperand *line);

/* 6904ca53 -- __glsstim_LockArraysEXT: glLockArraysEXT, hardware path: a changed range marks the array cache stale
** (fx.flags 8). The FX_GL_GLOW_WORM debug mode 2 writes the count to c:\<count>.txt. */
void __stdcall __glsstim_LockArraysEXT(GLint first, GLsizei count)
{
    char name[256];
    FILE *f;
    __GL_SETUP_NOT_IN_BEGIN();

    if (first < 0 || count < 0) {
        __glSetError(GL_INVALID_VALUE);
    } else {
        if (gc->vertexArray.flags & 4) {                /* already locked */
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
        gc->vertexArray.flags |= 6;
        if (gc->vertexArray.lockFirst != first) {
            gc->vertexArray.lockFirst = first;
            gc->fx.flags |= 8;
        }
        if (gc->vertexArray.lockCount != count) {
            gc->vertexArray.lockCount = count;
            gc->fx.flags |= 8;
        }
        if (gc->fx.glowWorm == 2) {
            sprintf(name, "c:\\%i.txt", gc->vertexArray.lockCount);
            f = fopen(name, "wt");
            fprintf(f, "%s", name);
            fclose(f);
        }
    }
}

/* 6904cb90 -- __glsstim_UnlockArraysEXT: glUnlockArraysEXT, hardware path */
void __fastcall __glsstim_UnlockArraysEXT(void)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (!(gc->vertexArray.flags & 4)) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }
    gc->vertexArray.flags &= ~4;
    gc->vertexArray.lockCount = 0;
    gc->vertexArray.lockFirst = 0;
}

/* 6904d550 -- hwUpdateClipRegion: the Glide window's updateClipRegion: nothing to do */
void __fastcall hwUpdateClipRegion(void)
{
}

/* 6904d5f0 -- hwGlideInit: initialise Glide in OpenGL mode */
GLboolean __fastcall hwGlideInit(void)
{
    grGlideInit();
    grEnable(GR_OPENGL_MODE_EXT);
    return GL_TRUE;
}

/* 6904d60a -- hwHasSurfaceExt: Glide has the SURFACE extension (windowed rendering) */
GLboolean __fastcall hwHasSurfaceExt(void)
{
    return strstr(grGetString(GR_EXTENSION), " SURFACE ") != NULL;
}

/* 6904d639 -- hwHasPixextExt: Glide has the PIXEXT extension (32 bpp pixel formats) */
GLboolean __fastcall hwHasPixextExt(void)
{
    return strstr(grGetString(GR_EXTENSION), " PIXEXT ") != NULL;
}

/* hwRegOrEnvString: a Glide setting without a gc: the environment variable, else the value in Glide's registry key
** (grGetRegistryOrEnvironmentStringExt); NULL when unset or when Glide lacks the extension */
const char *__fastcall hwRegOrEnvString(const char *name)
{
    static const char *(__stdcall *getString)(const char *);
    if (getString == NULL)
        getString = (const char *(__stdcall *)(const char *))grGetProcAddress("grGetRegistryOrEnvironmentStringExt");
    return getString != NULL ? getString(name) : NULL;
}

/* 6904d700 -- __glSSTExtractTexelRGBA8_B: s_texextr.c __glExtractTexelRGBA8_B for the hardware texture images (the
** bordered form of 6904d660): the texels are stored B, G, R, A */
void __fastcall __glSSTExtractTexelRGBA8_B(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                             GLint img, GLint row, GLint col, __GLtexel *result)
{
    GLubyte *image;

    row++;
    col++;
    image = (GLubyte *)level->buffer + ((row * level->width) + col) * 4;
    result->a = image[3];
    result->r = image[2];
    result->g = image[1];
    result->b = image[0];
}

/* 6904da60 -- og_matVec3x87: emit the x87 product of the 3-vector at [esi] with the 3x3 part of the matrix at
** matrix + 0x60 (row-major, rows 0x10 apart; ecx = the matrix): x -> outX, y -> outY, z -> outZ (og operands) */
#pragma optimize("", off)       /* compiled without optimisation (/Od) in the original */
GLubyte *__fastcall og_matVec3x87(__GLcontext *gc, GLubyte *pc, GLuint matrix, OgOperand *outX, OgOperand *outY,
                                 OgOperand *outZ)
{
    EMIT(og_mov(gc, R(ecx), IMM(matrix + 0x60)));
    EMIT(og_fld(gc, ESI(0)));
    EMIT(og_fmul(gc, ECX(0x00)));
    EMIT(og_fld(gc, ESI(IMM(4))));
    EMIT(og_fmul(gc, ECX(0x10)));
    EMIT(og_fld(gc, ESI(IMM(8))));
    EMIT(og_fmul(gc, ECX(0x20)));
    EMIT(og_fxch(gc, R(st2)));
    EMIT(og_faddp(gc, R(st1), R(st0)));
    EMIT(og_fld(gc, ESI(0)));
    EMIT(og_fmul(gc, ECX(0x04)));
    EMIT(og_fxch(gc, R(st1)));
    EMIT(og_faddp(gc, R(st2), R(st0)));
    EMIT(og_fld(gc, ESI(IMM(4))));
    EMIT(og_fmul(gc, ECX(0x14)));
    EMIT(og_fld(gc, ESI(IMM(8))));
    EMIT(og_fmul(gc, ECX(0x24)));
    EMIT(og_fxch(gc, R(st2)));
    EMIT(og_faddp(gc, R(st1), R(st0)));
    EMIT(og_fxch(gc, R(st2)));
    EMIT(og_fstp(gc, outX));
    EMIT(og_faddp(gc, R(st1), R(st0)));
    EMIT(og_fld(gc, ESI(0)));
    EMIT(og_fmul(gc, ECX(0x08)));
    EMIT(og_fld(gc, ESI(IMM(4))));
    EMIT(og_fmul(gc, ECX(0x18)));
    EMIT(og_fld(gc, ESI(IMM(8))));
    EMIT(og_fmul(gc, ECX(0x28)));
    EMIT(og_fxch(gc, R(st2)));
    EMIT(og_faddp(gc, R(st1), R(st0)));
    EMIT(og_fxch(gc, R(st2)));
    EMIT(og_fstp(gc, outY));
    EMIT(og_faddp(gc, R(st1), R(st0)));
    EMIT(og_fstp(gc, outZ));
    return pc;
}
#pragma optimize("", on)

/* 6904e010 -- og_callOperand: emit `call target` (an og operand); line, the caller's source line, is unused */
#pragma optimize("", off)
GLubyte *__fastcall og_callOperand(__GLcontext *gc, GLubyte *pc, OgOperand *target, OgOperand *line)
{
    EMIT(og_call(gc, target));
    return pc;
}
#pragma optimize("", on)

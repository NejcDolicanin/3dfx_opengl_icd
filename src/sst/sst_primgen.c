/*
** sst_primgen.c -- og_genRenderPrim (og_genRenderPrim): generates the render-primitive tail for one primitive type
** (0 point, 1 line, 2 triangle, 3 quad) with the og.h code generator: fetch the staging vertices, test the clip
** codes (out -> the clip routine), optional back-face test, flat-shading colour copy, then call the Glide draw
** routine (with the polygon-offset FIFO routines around it on the direct-FIFO path).  The
** fall-through `switch (prim)` blocks load one register per vertex (eax, ebx, ecx for vertices
** 3, 2, 1; edx for vertex 0).
*/
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "glshade.h"
#include "og.h"
#include "icdprotos.h"

/* og.h emitters (EMIT / R / IMM / MEM / MEMR / LABEL) and: */
#define VTX             (&gc->hwcx.stagingVtx[0])                       /* the staging vertex pointers */
#define VX_WINDOW_X     VOFS(window.x)
#define VX_WINDOW_Y     VOFS(window.y)
#define VX_FLAGBITS     VOFS(flagBits)      /* the clip codes */
#define VX_PACKEDCOLOR  VOFS(packedColor)
/* a label's name for the generator's listing: __GLogOperand +0x10 (og.h has it as unused10) */
#define OG_NAME(l)      (*(const char **)&(l)->unused10)

/* 6902431b -- og_genRenderPrim: generates the render-primitive entry of one primitive type (sst_primgen.c header) */
void __fastcall og_genRenderPrim(__GLcontext *gc, GLubyte *pc, GLuint prim, char bigPoint)
{
    OgLabel *clipped, *done, *skip, *draw, *clip;
    GLuint flags;

    og_newFixup(gc);
    clipped = og_newFixup(gc);
    done = og_newFixup(gc);
    skip = og_newFixup(gc);
    draw = og_newFixup(gc);
    clip = og_newFixup(gc);

    if (gc->renderMode == GL_RENDER) {
        switch (prim) {
        case 0:
            draw->addr = (GLubyte *)grDrawPoint;
            OG_NAME(draw) = "grDrawPoint";
            break;
        case 1:
            draw->addr = (GLubyte *)grDrawLine;
            OG_NAME(draw) = "grDrawLine";
            clip->addr = (GLubyte *)__glSSTClipAndDrawLine;
            OG_NAME(clip) = "__glSSTClipAndDrawLine";
            break;
        case 2:
            draw->addr = (GLubyte *)grDrawTriangle;
            OG_NAME(draw) = "grDrawTriangle";
            clip->addr = (GLubyte *)gc->hwcx.genCode[3];
            OG_NAME(clip) = "hwcx->core.clip_and_draw_triangle";
            break;
        case 3:
            draw->addr = (GLubyte *)__grDrawQuad;
            OG_NAME(draw) = "__grDrawQuad";
            clip->addr = (GLubyte *)gc->hwcx.genCode[4];
            OG_NAME(clip) = "hwcx->core.clip_and_draw_quad";
            break;
        }
    } else if (gc->renderMode == GL_FEEDBACK && prim == 0) {
        draw->addr = (GLubyte *)gc->procs.renderPoint;
        OG_NAME(draw) = "*gc->procs.renderPoint";
    }

    /* edx = vertex 0, ecx / ebx / eax = vertices 1..3 (the argument is the vertex index) */
    if (prim != 0) {
        switch (prim) {
        case 3:
            EMIT(og_mov(gc, R(eax), R(edx)));
            EMIT(og_and(gc, R(eax), IMM(3)));
            EMIT(og_shr(gc, R(edx), IMM(2)));
            EMIT(og_mov(gc, R(eax), TAB(VTX, eax)));
        case 2:
            EMIT(og_mov(gc, R(ebx), R(edx)));
            EMIT(og_and(gc, R(ebx), IMM(3)));
            EMIT(og_shr(gc, R(edx), IMM(2)));
            EMIT(og_mov(gc, R(ebx), TAB(VTX, ebx)));
        case 1:
            EMIT(og_mov(gc, R(ecx), R(edx)));
            EMIT(og_and(gc, R(ecx), IMM(3)));
            EMIT(og_shr(gc, R(edx), IMM(2)));
            EMIT(og_mov(gc, R(ecx), TAB(VTX, ecx)));
        }
        EMIT(og_mov(gc, R(edx), TAB(VTX, edx)));
    }

    if (prim != 0) {
        /* or the clip codes: any vertex out -> the clip routine */
        EMIT(og_mov(gc, R(esi), MEMR(VX_FLAGBITS, edx)));
        switch (prim) {
        case 3: ;   /* the empty statement keeps case 3 in the compare chain (the original's dead `dec eax`) */
        default:
            EMIT(og_mov(gc, R(ebp), MEMR(VX_FLAGBITS, eax)));
            EMIT(og_or(gc, R(esi), R(ebp)));
        case 2:
            EMIT(og_mov(gc, R(ebp), MEMR(VX_FLAGBITS, ebx)));
            EMIT(og_or(gc, R(esi), R(ebp)));
        case 1:
            break;
        }
        EMIT(og_mov(gc, R(ebp), MEMR(VX_FLAGBITS, ecx)));
        EMIT(og_or(gc, R(esi), R(ebp)));
        EMIT(og_test(gc, R(esi), IMM(0xfff00000)));
        EMIT(og_jne(gc, clipped));
        if (prim >= 2 && (gc->state.enables.general & __GL_CULL_FACE_ENABLE)) {
            /* back-face test: the signed area into genConst[0] */
            if (gc->constants.cpuCaps & 2) {
                EMIT(og_femms(gc));
                EMIT(og_movq(gc, R(mm0), MEMR(VX_WINDOW_X, ebx)));
                EMIT(og_movq(gc, R(mm2), MEMR(VX_WINDOW_X, edx)));
                EMIT(og_pfsub(gc, R(mm0), R(mm2)));
                EMIT(og_movq(gc, R(mm1), MEMR(VX_WINDOW_X, ecx)));
                EMIT(og_pfsub(gc, R(mm1), R(mm2)));
                EMIT(og_movq(gc, R(mm3), MEMR(VX_WINDOW_Y, ebx)));
                EMIT(og_movq(gc, R(mm5), MEMR(VX_WINDOW_Y, edx)));
                EMIT(og_pfsub(gc, R(mm3), R(mm5)));
                EMIT(og_pfmul(gc, R(mm1), R(mm3)));
                EMIT(og_movq(gc, R(mm4), MEMR(VX_WINDOW_Y, ecx)));
                EMIT(og_pfsub(gc, R(mm4), R(mm5)));
                EMIT(og_pfmul(gc, R(mm0), R(mm4)));
                EMIT(og_pfsub(gc, R(mm0), R(mm1)));
                EMIT(og_movd(gc, MEM(&gc->hwcx.genConst[0]), R(mm0)));
                EMIT(og_femms(gc));
            } else {
                EMIT(og_fld(gc, MEMR(VX_WINDOW_X, ebx)));
                EMIT(og_fsub(gc, MEMR(VX_WINDOW_X, edx)));
                EMIT(og_fld(gc, MEMR(VX_WINDOW_X, ecx)));
                EMIT(og_fsub(gc, MEMR(VX_WINDOW_X, edx)));
                EMIT(og_fld(gc, MEMR(VX_WINDOW_Y, ebx)));
                EMIT(og_fsub(gc, MEMR(VX_WINDOW_Y, edx)));
                EMIT(og_fld(gc, MEMR(VX_WINDOW_Y, ecx)));
                EMIT(og_fsub(gc, MEMR(VX_WINDOW_Y, edx)));
                EMIT(og_fmulp(gc, R(st3), R(st0)));
                EMIT(og_fmulp(gc, R(st1), R(st0)));
                EMIT(og_fsubp(gc, R(st1), R(st0)));
                EMIT(og_fstp(gc, MEM(&gc->hwcx.genConst[0])));
            }
            EMIT(og_mov(gc, MEM(&gc->hwcx.genConst[1]), R(eax)));
            EMIT(og_mov(gc, MEM(&gc->hwcx.genConst[2]), R(ebx)));
            EMIT(og_mov(gc, R(eax), MEM(&gc->hwcx.genConst[0])));
            EMIT(og_shr(gc, R(eax), IMM(0x1f)));
            EMIT(og_xor(gc, R(eax), IMM(1)));
            EMIT(og_movzx8(gc, R(al), og_mem(gc, IMM(&gc->polygon.face[0]), R(eax), NULL, 1)));
            EMIT(og_movzx8(gc, R(bl), MEM(&gc->polygon.cullFace)));
            EMIT(og_cmp(gc, R(al), R(bl)));
            EMIT(og_mov(gc, R(eax), MEM(&gc->hwcx.genConst[1])));
            EMIT(og_mov(gc, R(ebx), MEM(&gc->hwcx.genConst[2])));
            EMIT(og_je(gc, skip));
        }
        EMIT(og_test(gc, R(esi), IMM(0xffff0000)));
        EMIT(og_jne(gc, clipped));
    } else {
        /* a point: inside a glBegin only */
        EMIT(og_test(gc, R(esi), IMM(0xffff0000)));
        EMIT(og_jne(gc, skip));
        EMIT(og_mov(gc, R(esi), MEM(&gc->beginMode)));
        EMIT(og_cmp(gc, R(esi), IMM(1)));
        EMIT(og_jne(gc, skip));
    }

    /* unclipped: draw the vertices as they are */
    flags = __GL_SHADE_MODEFLAGS(gc);
    if (!(flags & __GL_SHADE_SMOOTH_LIGHT) && (prim == 1 || gc->hwcx.cmdTransportInfo == 0)) {
        /* flat shading: give every vertex the packed colour of the vertex in edx (the old ones pushed, restored below) */
        EMIT(og_mov(gc, R(edi), MEMR(VX_PACKEDCOLOR, edx)));
        switch (prim) {
        case 3:
            EMIT(og_mov(gc, R(ebp), MEMR(VX_PACKEDCOLOR, eax)));
            EMIT(og_mov(gc, MEMR(VX_PACKEDCOLOR, eax), R(edi)));
            EMIT(og_push(gc, R(ebp)));
            EMIT(og_push(gc, R(eax)));
        case 2:
            EMIT(og_mov(gc, R(ebp), MEMR(VX_PACKEDCOLOR, ebx)));
            EMIT(og_mov(gc, MEMR(VX_PACKEDCOLOR, ebx), R(edi)));
            EMIT(og_push(gc, R(ebp)));
            EMIT(og_push(gc, R(ebx)));
        case 1:
            EMIT(og_mov(gc, R(ebp), MEMR(VX_PACKEDCOLOR, ecx)));
            EMIT(og_mov(gc, MEMR(VX_PACKEDCOLOR, ecx), R(edi)));
            EMIT(og_push(gc, R(ebp)));
            EMIT(og_push(gc, R(ecx)));
        }
    }
    /* the draw call; on the direct-FIFO path (cmdTransportInfo) with polygon offset fill, the bufRec[9] routines
    ** (sst_offsetgen.c, gc->hwcx.genProcs9) run before and after it */
    switch (prim) {
    case 3:
        if (gc->hwcx.cmdTransportInfo != 0) {
            OgLabel *noOffset = og_newLabel(gc), *noOffset2 = og_newLabel(gc);
            EMIT(og_push(gc, R(edx)));
            EMIT(og_push(gc, R(ecx)));
            EMIT(og_push(gc, R(ebx)));
            EMIT(og_push(gc, R(eax)));
            EMIT(og_mov(gc, R(eax), R(ecx)));
            EMIT(og_mov(gc, R(ecx), R(ebx)));
            EMIT(og_mov(gc, R(ebx), R(edx)));
            EMIT(og_mov(gc, R(edx), MEM(&gc->state.enables.general)));
            EMIT(og_test(gc, R(edx), IMM(__GL_POLYGON_OFFSET_FILL_ENABLE)));
            EMIT(og_je(gc, noOffset));
            EMIT(og_mov(gc, R(edx), MEM(&gc->hwcx.genConst[0])));
            EMIT(og_call(gc, MEM(&gc->hwcx.genProcs9[0])));
            LABEL(noOffset);
            EMIT(og_pop(gc, R(eax)));
            EMIT(og_pop(gc, R(ebx)));
            EMIT(og_pop(gc, R(ecx)));
            EMIT(og_pop(gc, R(edx)));
            EMIT(og_push(gc, R(eax)));
            EMIT(og_push(gc, R(ebx)));
            EMIT(og_call(gc, MEM(&gc->hwcx.drawQuad)));
            EMIT(og_push(gc, R(edx)));
            EMIT(og_push(gc, R(eax)));
            EMIT(og_mov(gc, R(edx), MEM(&gc->state.enables.general)));
            EMIT(og_test(gc, R(edx), IMM(__GL_POLYGON_OFFSET_FILL_ENABLE)));
            EMIT(og_je(gc, noOffset2));
            EMIT(og_call(gc, MEM(&gc->hwcx.genProcs9[2])));
            LABEL(noOffset2);
            EMIT(og_pop(gc, R(eax)));
            EMIT(og_pop(gc, R(edx)));
        } else {
            EMIT(og_push(gc, R(eax)));
            EMIT(og_push(gc, R(ebx)));
            EMIT(og_push(gc, R(ecx)));
            EMIT(og_push(gc, R(edx)));
            EMIT(og_call(gc, draw));
        }
        break;
    case 1:
        EMIT(og_push(gc, R(ecx)));
        EMIT(og_push(gc, R(edx)));
        EMIT(og_call(gc, draw));
        break;
    case 2:
        if (gc->hwcx.cmdTransportInfo != 0) {
            OgLabel *noOffset = og_newLabel(gc), *noOffset2 = og_newLabel(gc);
            EMIT(og_push(gc, R(edx)));
            EMIT(og_push(gc, R(ecx)));
            EMIT(og_push(gc, R(ebx)));
            EMIT(og_push(gc, R(eax)));
            EMIT(og_mov(gc, R(eax), R(ecx)));
            EMIT(og_mov(gc, R(ecx), R(ebx)));
            EMIT(og_mov(gc, R(ebx), R(edx)));
            EMIT(og_mov(gc, R(edx), MEM(&gc->state.enables.general)));
            EMIT(og_test(gc, R(edx), IMM(__GL_POLYGON_OFFSET_FILL_ENABLE)));
            EMIT(og_je(gc, noOffset));
            EMIT(og_mov(gc, R(edx), MEM(&gc->hwcx.genConst[0])));
            EMIT(og_call(gc, MEM(&gc->hwcx.genProcs9[0])));
            LABEL(noOffset);
            EMIT(og_pop(gc, R(eax)));
            EMIT(og_pop(gc, R(ebx)));
            EMIT(og_pop(gc, R(ecx)));
            EMIT(og_pop(gc, R(edx)));
            EMIT(og_push(gc, R(ebx)));
            EMIT(og_call(gc, MEM(&gc->hwcx.drawTri)));
            EMIT(og_push(gc, R(edx)));
            EMIT(og_push(gc, R(eax)));
            EMIT(og_mov(gc, R(edx), MEM(&gc->state.enables.general)));
            EMIT(og_test(gc, R(edx), IMM(__GL_POLYGON_OFFSET_FILL_ENABLE)));
            EMIT(og_je(gc, noOffset2));
            EMIT(og_call(gc, MEM(&gc->hwcx.genProcs9[2])));
            LABEL(noOffset2);
            EMIT(og_pop(gc, R(eax)));
            EMIT(og_pop(gc, R(edx)));
        } else {
            EMIT(og_push(gc, R(ebx)));
            EMIT(og_push(gc, R(ecx)));
            EMIT(og_push(gc, R(edx)));
            EMIT(og_call(gc, draw));
        }
        break;
    case 0:
        if (bigPoint) {
            EMIT(og_mov(gc, R(ecx), IMM(gc)));
            if (gc->hwcx.windowed != 0)
                pc = og_callRel32(gc, pc, (GLuint)__glSSTRenderWideAAPoint);
            else
                pc = og_callRel32(gc, pc, (GLuint)__glSSTRenderBigPoint);
        } else {
            EMIT(og_push(gc, R(edx)));
            EMIT(og_call(gc, draw));
        }
        break;
    }
    flags = __GL_SHADE_MODEFLAGS(gc);
    if (!(flags & __GL_SHADE_SMOOTH_LIGHT) && (prim == 1 || gc->hwcx.cmdTransportInfo == 0)) {
        switch (prim) {
        case 3:
            EMIT(og_pop(gc, R(eax)));
            EMIT(og_pop(gc, R(ebp)));
            EMIT(og_mov(gc, MEMR(VX_PACKEDCOLOR, eax), R(ebp)));
        case 2:
            EMIT(og_pop(gc, R(eax)));
            EMIT(og_pop(gc, R(ebp)));
            EMIT(og_mov(gc, MEMR(VX_PACKEDCOLOR, eax), R(ebp)));
        case 1:
            EMIT(og_pop(gc, R(eax)));
            EMIT(og_pop(gc, R(ebp)));
            EMIT(og_mov(gc, MEMR(VX_PACKEDCOLOR, eax), R(ebp)));
        }
    }
    LABEL(skip);
    EMIT(og_pop(gc, R(ebx)));
    EMIT(og_pop(gc, R(esi)));
    EMIT(og_pop(gc, R(edi)));
    EMIT(og_pop(gc, R(ebp)));
    EMIT(og_ret_imm(gc, IMM(4)));

    /* clipped: the and of the clip codes; all out -> nothing, else the clip routine */
    LABEL(clipped);
    if (prim != 0) {
        EMIT(og_mov(gc, R(esi), MEMR(VX_FLAGBITS, edx)));
        switch (prim) {
        case 3:
            EMIT(og_mov(gc, R(ebp), MEMR(VX_FLAGBITS, eax)));
            EMIT(og_and(gc, R(esi), R(ebp)));
        case 2:
            EMIT(og_mov(gc, R(ebp), MEMR(VX_FLAGBITS, ebx)));
            EMIT(og_and(gc, R(esi), R(ebp)));
        case 1:
            EMIT(og_mov(gc, R(ebp), MEMR(VX_FLAGBITS, ecx)));
            EMIT(og_and(gc, R(esi), R(ebp)));
        }
        EMIT(og_jne(gc, done));
        flags = __GL_SHADE_MODEFLAGS(gc);
        if (!(flags & __GL_SHADE_SMOOTH_LIGHT)) {
            EMIT(og_mov(gc, R(edi), MEMR(VX_PACKEDCOLOR, edx)));
            switch (prim) {
            case 3:
                EMIT(og_mov(gc, R(ebp), MEMR(VX_PACKEDCOLOR, eax)));
                EMIT(og_mov(gc, MEMR(VX_PACKEDCOLOR, eax), R(edi)));
                EMIT(og_push(gc, R(ebp)));
                EMIT(og_push(gc, R(eax)));
            case 2:
                EMIT(og_mov(gc, R(ebp), MEMR(VX_PACKEDCOLOR, ebx)));
                EMIT(og_mov(gc, MEMR(VX_PACKEDCOLOR, ebx), R(edi)));
                EMIT(og_push(gc, R(ebp)));
                EMIT(og_push(gc, R(ebx)));
            case 1:
                EMIT(og_mov(gc, R(ebp), MEMR(VX_PACKEDCOLOR, ecx)));
                EMIT(og_mov(gc, MEMR(VX_PACKEDCOLOR, ecx), R(edi)));
                EMIT(og_push(gc, R(ebp)));
                EMIT(og_push(gc, R(ecx)));
            }
        }
        switch (prim) {
        case 1:
            EMIT(og_push(gc, R(edx)));
            EMIT(og_push(gc, R(ecx)));
            break;
        case 2:
            EMIT(og_push(gc, R(edx)));
            EMIT(og_push(gc, R(ecx)));
            EMIT(og_push(gc, R(ebx)));
            break;
        case 3:
            EMIT(og_push(gc, R(edx)));
            EMIT(og_push(gc, R(ecx)));
            EMIT(og_push(gc, R(ebx)));
            EMIT(og_push(gc, R(eax)));
            break;
        }
        if (gc->constants.cpuCaps & 2)
            EMIT(og_femms(gc));
        EMIT(og_mov(gc, R(edx), IMM(0x15)));
        EMIT(og_mov(gc, R(ecx), IMM(gc)));
        EMIT(og_call(gc, clip));
        if (gc->constants.cpuCaps & 2)
            EMIT(og_femms(gc));
        flags = __GL_SHADE_MODEFLAGS(gc);
        if (!(flags & __GL_SHADE_SMOOTH_LIGHT)) {
            switch (prim) {
            case 3:
                EMIT(og_pop(gc, R(eax)));
                EMIT(og_pop(gc, R(ebp)));
                EMIT(og_mov(gc, MEMR(VX_PACKEDCOLOR, eax), R(ebp)));
            case 2:
                EMIT(og_pop(gc, R(eax)));
                EMIT(og_pop(gc, R(ebp)));
                EMIT(og_mov(gc, MEMR(VX_PACKEDCOLOR, eax), R(ebp)));
            case 1:
                EMIT(og_pop(gc, R(eax)));
                EMIT(og_pop(gc, R(ebp)));
                EMIT(og_mov(gc, MEMR(VX_PACKEDCOLOR, eax), R(ebp)));
            }
        }
    }
    LABEL(done);
    EMIT(og_pop(gc, R(ebx)));
    EMIT(og_pop(gc, R(esi)));
    EMIT(og_pop(gc, R(edi)));
    EMIT(og_pop(gc, R(ebp)));
    EMIT(og_ret_imm(gc, IMM(4)));
}

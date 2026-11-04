/*
** sst_lostctx.c -- 3dfx back end, original VA range 6902c000..69034000: triangle setup (hwGenLockedTriangles, 37 KB, is
** in sst_lockedtri.c); here the og helper that emits the lost-context check of the generated routines.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "og.h"
#include <glide.h>


/* 6902e9f0 -- og_lostContextCheck: emit `mov reg, [lostContext]; test reg, reg; je ok; ret retImm; ok:`: a
** generated routine returns at once while Glide's context is lost (the flag *fifo->ptrLostContext) */
GLubyte *__fastcall og_lostContextCheck(__GLcontext *gc, GLubyte *pc, OgOperand *reg, OgOperand *retImm)
{
    GrCmdTransportInfo *fifo;
    OgLabel *ok;
    OgOperand *lost;

    fifo = gc->hwcx.cmdTransportInfo;
    ok = og_newLabel(gc);
    lost = MEM(*fifo->ptrLostContext);
    EMIT(og_mov(gc, reg, lost));
    EMIT(og_test(gc, reg, reg));
    EMIT(og_je(gc, ok));
    EMIT(og_ret_imm(gc, retImm));
    LABEL(ok);
    return pc;
}

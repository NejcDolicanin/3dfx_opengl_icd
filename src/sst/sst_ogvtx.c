/*
** sst_ogvtx.c -- 3dfx back end, original VA range 69054000..6905c000: vertex setup (doMatrix 17 KB, in
** od_domatrix.c); here two og helpers of the vertex-setup generator, unoptimised in the original.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "og.h"
#include "icdprotos.h"
#include <glide.h>


GLubyte *__fastcall og_minSigned(__GLcontext *gc, GLubyte *pc, OgOperand *d, OgOperand *s);

/* 69054fd0 -- og_callRel32Gen: og_callRel32: emit `call routine` (rel32) at pc: a label already placed at the
** routine */
#pragma optimize("", off)
GLubyte *__fastcall og_callRel32Gen(__GLcontext *gc, GLubyte *pc, GLuint routine)
{
    OgLabel *target;

    target = og_newFixup(gc);
    target->addr = (GLubyte *)routine;
    pc = og_encode(gc, pc, og_call(gc, target));
    return pc;
}
#pragma optimize("", on)

/* 690564d0 -- og_minSigned: emit the signed minimum `cmp d, s; jl skip; mov d, s; skip:` */
#pragma optimize("", off)
GLubyte *__fastcall og_minSigned(__GLcontext *gc, GLubyte *pc, OgOperand *d, OgOperand *s)
{
    OgLabel *skip;

    skip = og_newFixup(gc);
    pc = og_encode(gc, pc, og_cmp(gc, d, s));
    pc = og_encode(gc, pc, og_jl(gc, skip));
    pc = og_encode(gc, pc, og_mov(gc, d, s));
    skip->addr = pc;
    og_bindLabel(gc, skip, pc);
    return pc;
}
#pragma optimize("", on)

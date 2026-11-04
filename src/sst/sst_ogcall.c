/*
** sst_ogcall.c -- 3dfx back end, original VA range 69020000..69024000: renderer core (part 1): the og
** helper that emits a direct call.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include "og.h"
#include "icdprotos.h"
#include <glide.h>



/* 69021c40 -- og_callRel32: emit `call routine` (rel32) at pc: a label already placed at the routine */
GLubyte *__fastcall og_callRel32(__GLcontext *gc, GLubyte *pc, GLuint routine)
{
    OgLabel *target;

    target = og_newFixup(gc);
    target->addr = (GLubyte *)routine;
    return og_encode(gc, pc, og_call(gc, target));
}

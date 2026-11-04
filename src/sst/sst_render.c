/*
** sst_render.c -- draw procs of the hardware path beyond sst_linepoint.c: flat / wide lines, wide and
** antialiased points, sprites (textured points), the polygon-mode renderers, triangles (smooth, flat, indexed,
** culling) and the polygon offset.
*/
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "glshade.h"
#include "si_names.h"
#include "icdglob.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "og.h"
#include "icdprotos.h"

/* 690f0048 -- the point-sprite disc texture */
static GLuint g_spriteDiscTexels[1366] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xad815015, 0xfff6e7cd,
    0xcde7f6ff, 0x155081ad, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xd0822b00, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0x002b82d0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x5e060000, 0xffffffca, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xcaffffff, 0x0000065e, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffe26604, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0x0466e2ff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x3f000000, 0xffffffd5, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xd5ffffff, 0x0000003f, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xff950a00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000a95ff, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xffffd528, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x28d5ffff, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x47000000, 0xfffffff1, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xf1ffffff, 0x00000047, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0xfa590000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000059fa, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0xfffd5900, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0059fdff, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0xfffffa47, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x47faffff, 0x00000000, 0x00000000,
    0x00000000, 0x28000000, 0xfffffff1, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xf1ffffff, 0x00000028, 0x00000000,
    0x00000000, 0xd50a0000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000ad5, 0x00000000,
    0x00000000, 0xff950000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000095ff, 0x00000000,
    0x00000000, 0xffff3f00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x003fffff, 0x00000000,
    0x00000000, 0xffffd504, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x04d5ffff, 0x00000000,
    0x00000000, 0xffffff66, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x66ffffff, 0x00000000,
    0x06000000, 0xffffffe2, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xe2ffffff, 0x00000006,
    0x5e000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0000005e,
    0xca000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000000ca,
    0xff2b0000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00002bff,
    0xff820000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000082ff,
    0xffd00000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0000d0ff,
    0xffff1500, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0015ffff,
    0xffff5000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0050ffff,
    0xffff8100, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0081ffff,
    0xffffad00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00adffff,
    0xffffcd00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00cdffff,
    0xffffe700, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00e7ffff,
    0xfffff600, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00f6ffff,
    0xffffff00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00ffffff,
    0xffffff00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00ffffff,
    0xfffff600, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00f6ffff,
    0xffffe700, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00e7ffff,
    0xffffcd00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00cdffff,
    0xffffad00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00adffff,
    0xffff8100, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0081ffff,
    0xffff5000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0050ffff,
    0xffff1500, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0015ffff,
    0xffd00000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0000d0ff,
    0xff820000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000082ff,
    0xff2b0000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00002bff,
    0xca000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000000ca,
    0x5e000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0000005e,
    0x06000000, 0xffffffe2, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xe2ffffff, 0x00000006,
    0x00000000, 0xffffff66, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x66ffffff, 0x00000000,
    0x00000000, 0xffffd504, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x04d5ffff, 0x00000000,
    0x00000000, 0xffff3f00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x003fffff, 0x00000000,
    0x00000000, 0xff950000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000095ff, 0x00000000,
    0x00000000, 0xd50a0000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000ad5, 0x00000000,
    0x00000000, 0x28000000, 0xfffffff1, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xf1ffffff, 0x00000028, 0x00000000,
    0x00000000, 0x00000000, 0xfffffa47, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x47faffff, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0xfffd5900, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0059fdff, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0xfa590000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000059fa, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x47000000, 0xfffffff1, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xf1ffffff, 0x00000047, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xffffd528, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x28d5ffff, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xff950a00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000a95ff, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x3f000000, 0xffffffd5, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xd5ffffff, 0x0000003f, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffe26604, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0x0466e2ff, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x5e060000, 0xffffffca, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xcaffffff, 0x0000065e, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xd0822b00, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0x002b82d0, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xad815015, 0xfff6e7cd,
    0xcde7f6ff, 0x155081ad, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x4d070000, 0xffebca95, 0x95caebff, 0x0000074d, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0xffec8510, 0xffffffff, 0xffffffff, 0x1085ecff, 0x00000000, 0x00000000,
    0x00000000, 0x5a000000, 0xffffffec, 0xffffffff, 0xffffffff, 0xecffffff, 0x0000005a, 0x00000000,
    0x00000000, 0xff9a0500, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00059aff, 0x00000000,
    0x00000000, 0xffffb205, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x05b2ffff, 0x00000000,
    0x00000000, 0xffffff9a, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x9affffff, 0x00000000,
    0x5a000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0000005a,
    0xec100000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000010ec,
    0xff850000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000085ff,
    0xffec0700, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0007ecff,
    0xffff4d00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x004dffff,
    0xffff9500, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0095ffff,
    0xffffca00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00caffff,
    0xffffeb00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00ebffff,
    0xffffff00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00ffffff,
    0xffffff00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00ffffff,
    0xffffeb00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00ebffff,
    0xffffca00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00caffff,
    0xffff9500, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0095ffff,
    0xffff4d00, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x004dffff,
    0xffec0700, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0007ecff,
    0xff850000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000085ff,
    0xec100000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x000010ec,
    0x5a000000, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x0000005a,
    0x00000000, 0xffffff9a, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x9affffff, 0x00000000,
    0x00000000, 0xffffb205, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x05b2ffff, 0x00000000,
    0x00000000, 0xff9a0500, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00059aff, 0x00000000,
    0x00000000, 0x5a000000, 0xffffffec, 0xffffffff, 0xffffffff, 0xecffffff, 0x0000005a, 0x00000000,
    0x00000000, 0x00000000, 0xffec8510, 0xffffffff, 0xffffffff, 0x1085ecff, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x4d070000, 0xffebca95, 0x95caebff, 0x0000074d, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xfbd58819, 0x1988d5fb, 0x00000000,
    0x5c000000, 0xfffffff4, 0xf4ffffff, 0x0000005c, 0xff5c0000, 0xffffffff, 0xffffffff, 0x00005cff,
    0xfff41900, 0xffffffff, 0xffffffff, 0x0019f4ff, 0xffff8800, 0xffffffff, 0xffffffff, 0x0088ffff,
    0xffffd500, 0xffffffff, 0xffffffff, 0x00d5ffff, 0xfffffb00, 0xffffffff, 0xffffffff, 0x00fbffff,
    0xfffffb00, 0xffffffff, 0xffffffff, 0x00fbffff, 0xffffd500, 0xffffffff, 0xffffffff, 0x00d5ffff,
    0xffff8800, 0xffffffff, 0xffffffff, 0x0088ffff, 0xfff41900, 0xffffffff, 0xffffffff, 0x0019f4ff,
    0xff5c0000, 0xffffffff, 0xffffffff, 0x00005cff, 0x5c000000, 0xfffffff4, 0xf4ffffff, 0x0000005c,
    0x00000000, 0xfbd58819, 0x1988d5fb, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0xf2930600, 0x000693f2, 0xffff9300, 0x0093ffff, 0xfffff200, 0x00f2ffff,
    0xfffff200, 0x00f2ffff, 0xffff9300, 0x0093ffff, 0xf2930600, 0x000693f2, 0x00000000, 0x00000000,
    0x00000000, 0x00cbcb00, 0x00cbcb00, 0x00000000, 0x00000000, 0x000000d0,
};

#pragma warning(disable: 4273)
typedef void (__fastcall *__GLLineProc)(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b);
#define SST_SNAP    786432.0f       /* 3 << 18: rounds a coordinate to the 1/16 grid */
typedef void (__fastcall *__GLPointProc)(__GLcontext *gc, __GLvertexICD *v);
/* ---- sstbigpt.c: big points drawn as a 4-vertex strip ---------------------------------------- */
static const char sstbigpt_c[] = ".\\sstbigpt.c";
#define FIFO_MAKEROOM(f, n, line) \
    if ((f)->fifoRoom < (n)) \
        ((void (__stdcall *)(GLint, const char *, GLint))gc->hwcx.grCommandTransportMakeRoomExt2)((n), sstbigpt_c, (line))
#define SPRITE_INFO(info) \
    (info).smallLodLog2 = GR_LOD_LOG2_1, (info).largeLodLog2 = GR_LOD_LOG2_64, \
    (info).aspectRatioLog2 = GR_ASPECT_LOG2_1x1, (info).format = GR_TEXFMT_ALPHA_8, (info).data = g_spriteDiscTexels
/* ---- s_pgmode.c: triangles with a non-fill polygon mode / offset / smoothing on the Glide path --- */
#define DO_VALIDATE(gc, v, needs) \
    (*(gc)->procs.validateVertex[((v)->flagBits >> 14) & 3])(gc, v, needs)
/* 690f6708 -- g_stripVerts: the vertex-pointer list handed to grDrawVertexArray (the strip under construction) of the
** indexed-triangle path (690619c0, unbounded n++): the original reserves 6000 entries, 0x5dc0 bytes up to 690fc4c8 */
static GLuint g_stripVerts[6000];
extern float __fastcall EXT_point_parameters(__GLcontext *gc, __GLvertexICD *vx);
extern int __fastcall __glSSTPointSizeInt(GLuint ecx_unused, GLuint edx_unused, float size);
extern void __fastcall hwBeginSpritePoints(__GLcontext *gc);
extern int __fastcall hwTriangleCulled(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);

/* 6905f5a0 -- __glSSTRenderFlatLine: draw a flat-shaded line: a takes b's packed colour (the provoking vertex of a
** GL line) for grDrawLine, then gets its own back (6901a180 here and sst_pick.c 690161d0 install it as
** renderLine).  The original is
**   mov eax,[esp+4]; push esi; mov esi,edx; push edi; mov ecx,[eax+0x14]; push eax; mov edi,[esi+0x14];
**   push esi; mov [esi+0x14],ecx; call grDrawLine; mov [esi+0x14],edi; pop edi; pop esi; ret 4 */
void __fastcall __glSSTRenderFlatLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b)
{
    GLuint save = a->packedColor;

    a->packedColor = b->packedColor;
    grDrawLine(a, b);
    a->packedColor = save;
}

/* ---- wide aliased lines as two Glide triangles ---------------------------------------------------- */
/* 6905f850 (size 328) -- __glSSTRenderWideFlatLine: flat shaded (both ends take b's packed colour) */
void __fastcall __glSSTRenderWideFlatLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b)
{
    __GLvertexICD va, vb;
    __GLfloat w, dx, dy;
    GLuint packed;

    w = (__GLfloat)gc->state.line.aliasedWidth * 0.5f;
    dx = b->window.x - a->window.x;
    dy = b->window.y - a->window.y;
    if ((*(GLint *)&dx & 0x7fffffff) > (*(GLint *)&dy & 0x7fffffff)) {
        dy = w;
        dx = 0.0f;
    } else {
        dx = w;
        dy = 0.0f;
    }
    packed = a->packedColor;
    a->packedColor = b->packedColor;
    va = *a;
    vb = *b;
    a->window.x = dx + a->window.x;
    a->window.y = a->window.y - dy;
    va.window.x = va.window.x - dx;
    va.window.y = va.window.y + dy;
    b->window.x = dx + b->window.x;
    b->window.y = b->window.y - dy;
    vb.window.x = vb.window.x - dx;
    vb.window.y = vb.window.y + dy;
    grDrawTriangle(&va, a, b);
    grDrawTriangle(&va, b, &vb);
    a->packedColor = packed;
    a->window.x = a->window.x - dx;
    a->window.y = dy + a->window.y;
    b->window.x = b->window.x - dx;
    b->window.y = dy + b->window.y;
}

/* 6905f9a0 (size 308) -- __glSSTRenderWideLine: smooth shaded */
void __fastcall __glSSTRenderWideLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b)
{
    __GLvertexICD va, vb;
    __GLfloat w, dx, dy;

    w = (__GLfloat)gc->state.line.aliasedWidth * 0.5f;
    dx = b->window.x - a->window.x;
    dy = b->window.y - a->window.y;
    if ((*(GLint *)&dx & 0x7fffffff) > (*(GLint *)&dy & 0x7fffffff)) {
        dy = w;
        dx = 0.0f;
    } else {
        dx = w;
        dy = 0.0f;
    }
    va = *a;
    vb = *b;
    a->window.x = dx + a->window.x;
    a->window.y = a->window.y - dy;
    va.window.x = va.window.x - dx;
    va.window.y = va.window.y + dy;
    b->window.x = dx + b->window.x;
    b->window.y = b->window.y - dy;
    vb.window.x = vb.window.x - dx;
    vb.window.y = vb.window.y + dy;
    grDrawTriangle(&va, a, b);
    grDrawTriangle(&va, b, &vb);
    a->window.x = a->window.x - dx;
    a->window.y = dy + a->window.y;
    b->window.x = b->window.x - dx;
    b->window.y = dy + b->window.y;
}
/* 6905fae0 -- hwRenderTrianglePolygonModeLine: renderTrianglePolygonModeLine as 690601e0 for GL_LINE: a line along each boundary edge (v0-v1,
** v1-v2, v2-v0) */
void __fastcall hwRenderTrianglePolygonModeLine(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1, __GLvertexICD *v2)
{
    if (gc->buffers.lockCnt != 0) {
        if (--gc->buffers.lockCnt == 0 && gc->buffers.unlockRenderBuffers != 0) {
            (*gc->buffers.unlockRenderBuffers)(gc);
        }
        hwPackColor(gc, v0);
        hwPackColor(gc, v1);
        hwPackColor(gc, v2);
        if (v0->flagBits & 0x1000) {     /* boundary edge */
            (*(__GLLineProc)gc->procs.renderLine)(gc, v0, v1);
        }
        if (v1->flagBits & 0x1000) {     /* boundary edge */
            (*(__GLLineProc)gc->procs.renderLine)(gc, v1, v2);
        }
        if (v2->flagBits & 0x1000) {     /* boundary edge */
            (*(__GLLineProc)gc->procs.renderLine)(gc, v2, v0);
        }
        if (++gc->buffers.lockCnt == 1 && gc->buffers.lockRenderBuffers != 0) {
            (*gc->buffers.lockRenderBuffers)(gc);
        }
    }
    else {
        if (v0->flagBits & 0x1000) {     /* boundary edge */
            (*(__GLLineProc)gc->procs.renderLine)(gc, v0, v1);
        }
        if (v1->flagBits & 0x1000) {     /* boundary edge */
            (*(__GLLineProc)gc->procs.renderLine)(gc, v1, v2);
        }
        if (v2->flagBits & 0x1000) {     /* boundary edge */
            (*(__GLLineProc)gc->procs.renderLine)(gc, v2, v0);
        }
    }
}

/* 6905ff40 (size 327) -- __glSSTRenderWidePoint: aliased big point */
void __fastcall __glSSTRenderWidePoint(__GLcontext *gc, __GLvertexICD *v)
{
    __GLvertexICD va, vb;
    __GLfloat x, y, sx, sy;
    GLint size, half;

    size = ((GLint (__fastcall *)(__GLfloat))__glSSTPointSizeInt)(EXT_point_parameters(gc, v));
    sx = v->window.x;
    sy = v->window.y;
    half = size >> 1;
    x = v->window.x = v->window.x + SST_SNAP;
    v->window.y = y = v->window.y + SST_SNAP;
    if (size & 1) {
        v->window.x = (__GLfloat)((GLint)(x - SST_SNAP) - half);
        v->window.y = (__GLfloat)((GLint)(y - SST_SNAP) - half);
    } else {
        v->window.x = (__GLfloat)((GLint)(x - SST_SNAP + gc->constants.half) - half);
        v->window.y = (__GLfloat)((GLint)(y - SST_SNAP + gc->constants.half) - half);
    }
    va = *v;
    vb = *v;
    /* (__GLfloat)size written out each time: the compiler keeps it in its own temp (a named
    ** fsize variable changes the stack slots and the x87 operand order) */
    vb.window.y += (__GLfloat)size;
    va.window.x += (__GLfloat)size;
    grDrawTriangle(v, &vb, &va);
    v->window.x += (__GLfloat)size;
    v->window.y += (__GLfloat)size;
    grDrawTriangle(v, &vb, &va);
    v->window.x = sx;
    v->window.y = sy;
}

/* 69060090 -- __glSSTPointSizeInt: a point size rounded to an int, at least 1 */
int __fastcall __glSSTPointSizeInt(GLuint ecx_unused, GLuint edx_unused, float size)
{
  if (size < 1.0f) {
    return 1;
  }
  return (int)(size + 0.5f);
}

/* 690600c0 (size 275) -- __glSSTRenderWideAAPoint: antialiased big point, a fan of AA triangles around the centre */
void __fastcall __glSSTRenderWideAAPoint(__GLcontext *gc, __GLvertexICD *v)
{
    __GLvertexICD va, vb;
    __GLfloat r, angle, x, y, px, py;
    GLint n, i;

    r = EXT_point_parameters(gc, v) * 0.5f;
    py = 0.0f;
    va = *v;
    px = 0.0f;
    vb = *v;
    n = (GLint)((__GLfloat)(gc->constants.half) + r * 4.0f);
    if (n < 4) n = 4;
    for (i = 0; i <= n; i++) {
        vb.window.x = px;
        vb.window.y = py;
        angle = (__GLfloat)i / (__GLfloat)n * 6.283185307179586;
        x = (__GLfloat)(cos(angle) * r + v->window.x);
        y = (__GLfloat)(sin(angle) * r + v->window.y);
        va.window.x = x;
        va.window.y = y;
        px = x;
        py = y;
        if (i) {
            grAADrawTriangle(v, &vb, &va, 0, 1, 0);
        }
    }
}

/* 690601e0 -- hwRenderTrianglePolygonModePoint: renderTrianglePolygonModePoint (glPolygonMode GL_POINT): a point per boundary vertex; with the
** render buffers locked they are unlocked around the colour packing (6904a3f0) and the points */
void __fastcall hwRenderTrianglePolygonModePoint(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1, __GLvertexICD *v2)
{
    if (gc->buffers.lockCnt != 0) {
        if (--gc->buffers.lockCnt == 0 && gc->buffers.unlockRenderBuffers != 0) {
            (*gc->buffers.unlockRenderBuffers)(gc);
        }
        hwPackColor(gc, v0);
        hwPackColor(gc, v1);
        hwPackColor(gc, v2);
        if (v0->flagBits & 0x1000) {     /* boundary edge */
            (*(__GLPointProc)gc->procs.renderPoint)(gc, v0);
        }
        if (v1->flagBits & 0x1000) {     /* boundary edge */
            (*(__GLPointProc)gc->procs.renderPoint)(gc, v1);
        }
        if (v2->flagBits & 0x1000) {     /* boundary edge */
            (*(__GLPointProc)gc->procs.renderPoint)(gc, v2);
        }
        if (++gc->buffers.lockCnt == 1 && gc->buffers.lockRenderBuffers != 0) {
            (*gc->buffers.lockRenderBuffers)(gc);
        }
    }
    else {
        if (v0->flagBits & 0x1000) {     /* boundary edge */
            (*(__GLPointProc)gc->procs.renderPoint)(gc, v0);
        }
        if (v1->flagBits & 0x1000) {     /* boundary edge */
            (*(__GLPointProc)gc->procs.renderPoint)(gc, v1);
        }
        if (v2->flagBits & 0x1000) {     /* boundary edge */
            (*(__GLPointProc)gc->procs.renderPoint)(gc, v2);
        }
    }
}

/* 690602d0 -- hwPlaceSpriteTexture: fullscreen: the sprite disc's place in texture memory at addr
** (hwcx.downloadAddress); returns the address after it (windowed: addr unchanged) */
int __fastcall hwPlaceSpriteTexture(__GLcontext *gc, GLuint edx_unused, int addr)
{
    GrTexInfo info;

    if (gc->hwcx.windowed != 0) {
        return addr;
    }
    info.smallLodLog2 = GR_LOD_LOG2_1;
    info.aspectRatioLog2 = GR_ASPECT_LOG2_1x1;
    gc->hwcx.downloadAddress = addr;
    info.largeLodLog2 = GR_LOD_LOG2_64;
    info.format = GR_TEXFMT_ALPHA_8;
    info.data = g_spriteDiscTexels;
    return grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &info) + addr;
}

extern float __fastcall atmOOSqrt(GLuint ecx_unused, GLuint edx_unused, float x);
/* 69060330 -- EXT_point_parameters: the point size of a vertex, size / sqrt(a + b d + c d^2) with d = |eye z|
** (the fast 1/sqrt 69060390; the vertex stays in EDX for it), clamped to [min, max] */
float __fastcall EXT_point_parameters(__GLcontext *gc, __GLvertexICD *vx)
{
  float d, s;

  d = (float)(fabs(vx->eye.z));   /* parenthesised: fabs before the prologue's push */
  s = atmOOSqrt((GLuint)gc, (GLuint)vx,
                   (gc->state.point.ext[5] * d + gc->state.point.ext[4]) * d + gc->state.point.ext[3])
      * gc->state.point.smoothSize;
  if (s < gc->state.point.ext[0]) {
    s = gc->state.point.ext[0];
  }
  if (s > gc->state.point.ext[1]) {
    s = gc->state.point.ext[1];
  }
  return s;
}

/* 69060390 -- atmOOSqrt: 1/sqrt(x) by the magic-constant estimate and one refinement step */
float __fastcall atmOOSqrt(GLuint ecx_unused, GLuint edx_unused, float x)
{
  union { float f; GLuint i; } y;
  float t, a;

  y.f = x;
  y.i = 0x5f375a00 - (y.i >> 1);
  t = y.f * y.f * x;
  a = 3.0f - t;
  return (12.0f - a * a * t) * a * y.f * 0.0625f;
}

/* 690603ed (size 907) -- __glSSTRenderBigPoint: a big point (fixed texture coordinates 0..256) */
void __fastcall __glSSTRenderBigPoint(__GLcontext *gc, __GLvertexICD *v)
{
    __GLfloat r;
    __GLfloat x0, y0, x1, y1;
    __GLvertexICD vx[4];
    GrCmdTransportInfo *fifo;
    GLuint *pkt;

    if (!gc->hwcx.flag_355be) {
        hwBeginSpritePoints(gc);
    }
    r = EXT_point_parameters(gc, v) * 0.5f + 1.0f;
    fifo = gc->hwcx.cmdTransportInfo;
    if (!fifo) {
        vx[0].window.z = v->window.z;
        vx[1].window.z = v->window.z;
        vx[2].window.z = v->window.z;
        vx[3].window.z = v->window.z;
        vx[0].window.w = 1.0f;
        vx[1].window.w = 1.0f;
        vx[2].window.w = 1.0f;
        vx[3].window.w = 1.0f;
        vx[0].packedColor = v->packedColor;
        vx[1].packedColor = v->packedColor;
        vx[2].packedColor = v->packedColor;
        vx[3].packedColor = v->packedColor;
        if (gc->fx.numTMUs == 2) {
            vx[0].texture[1].w = 1.0f;
            vx[1].texture[1].w = 1.0f;
            vx[2].texture[1].w = 1.0f;
            vx[3].texture[1].w = 1.0f;
            vx[0].window.x = x0 = v->window.x - r;
            y0 = v->window.y - r;
            vx[0].window.y = y0;
            vx[1].window.x = x0;
            vx[1].window.y = y1 = r + v->window.y;
            vx[2].window.y = y0;
            vx[2].window.x = x1 = r + v->window.x;
            vx[3].window.x = x1;
            vx[3].window.y = y1;
            vx[0].texture[1].x = 0.0f;
            vx[0].texture[1].y = 0.0f;
            vx[1].texture[1].x = 0.0f;
            vx[1].texture[1].y = 256.0f;
            vx[2].texture[1].x = 256.0f;
            vx[2].texture[1].y = 0.0f;
            vx[3].texture[1].x = 256.0f;
            vx[3].texture[1].y = 256.0f;
        } else {
            vx[0].texture[0].w = 1.0f;
            vx[1].texture[0].w = 1.0f;
            vx[2].texture[0].w = 1.0f;
            vx[3].texture[0].w = 1.0f;
            vx[0].window.x = x0 = v->window.x - r;
            y0 = v->window.y - r;
            vx[0].window.y = y0;
            vx[1].window.x = x0;
            vx[1].window.y = y1 = r + v->window.y;
            vx[2].window.y = y0;
            vx[2].window.x = x1 = r + v->window.x;
            vx[3].window.x = x1;
            vx[3].window.y = y1;
            vx[0].texture[0].x = 0.0f;
            vx[0].texture[0].y = 0.0f;
            vx[1].texture[0].x = 0.0f;
            vx[1].texture[0].y = 256.0f;
            vx[2].texture[0].x = 256.0f;
            vx[2].texture[0].y = 0.0f;
            vx[3].texture[0].x = 256.0f;
            vx[3].texture[0].y = 256.0f;
        }
        grDrawVertexArrayContiguous(GR_TRIANGLE_STRIP, 4, vx, sizeof(__GLvertexICD));
        return;
    }
    if (**fifo->ptrLostContext == 0) {
        __GLfloat x, y;

        FIFO_MAKEROOM(fifo, 0x74, 0x12e);
        fifo->fifoRoom -= 0x74;
        x = v->window.x;
        y = v->window.y;
        pkt = fifo->fifoPtr;
        *pkt++ = 0x1000bd0b;
        *(__GLfloat *)pkt++ = x - r;
        *(__GLfloat *)pkt++ = y - r;
        *pkt++ = v->packedColor;
        *(__GLfloat *)pkt++ = (__GLfloat)(v->window.z);
        *(__GLfloat *)pkt++ = 1.0f;
        *(__GLfloat *)pkt++ = 0.0f;
        *(__GLfloat *)pkt++ = 0.0f;
        *(__GLfloat *)pkt++ = x + r;
        *(__GLfloat *)pkt++ = y - r;
        *pkt++ = v->packedColor;
        *(__GLfloat *)pkt++ = (__GLfloat)(v->window.z);
        *(__GLfloat *)pkt++ = 1.0f;
        *(__GLfloat *)pkt++ = 256.0f;
        *(__GLfloat *)pkt++ = 0.0f;
        *(__GLfloat *)pkt++ = x - r;
        *(__GLfloat *)pkt++ = y + r;
        *pkt++ = v->packedColor;
        *(__GLfloat *)pkt++ = (__GLfloat)(v->window.z);
        *(__GLfloat *)pkt++ = 1.0f;
        *(__GLfloat *)pkt++ = 0.0f;
        *(__GLfloat *)pkt++ = 256.0f;
        *(__GLfloat *)pkt++ = x + r;
        *(__GLfloat *)pkt++ = y + r;
        *pkt++ = v->packedColor;
        *(__GLfloat *)pkt++ = (__GLfloat)(v->window.z);
        *(__GLfloat *)pkt++ = 1.0f;
        *(__GLfloat *)pkt++ = 256.0f;
        *(__GLfloat *)pkt++ = 256.0f;
        fifo->fifoPtr = pkt;
    }
}

/* 69060780 -- hwBeginSpritePoints: Glide state of the textured points: the disc texture downloaded once into every
** TMU at hwcx.downloadAddress (flag_355bd), bound on TMU0 bilinear with nearest mipmapping, alpha test GREATER 0; colour =
** iterated, alpha = iterated * texture (the combine extension when present, which marks its records dirty); the
** command transport hook; flag_355be makes the next glEnd restore the texture / combine state (69026f60) */
void __fastcall hwBeginSpritePoints(__GLcontext *gc)
{
    GrTexInfo info;
    GLint i;

    SPRITE_INFO(info);
    if (gc->hwcx.flag_355bd == 0) {
        for (i = 0; i < gc->fx.numTMUs; i++) {
            grTexDownloadMipMap(gc->fx.tmuTexUnit[i], gc->hwcx.downloadAddress, GR_MIPMAPLEVELMASK_BOTH, &info);
        }
        gc->hwcx.flag_355bd = 1;
    }
    grTexSource(GR_TMU0, gc->hwcx.downloadAddress, GR_MIPMAPLEVELMASK_BOTH, &info);
    grTexFilterMode(GR_TMU0, GR_TEXTUREFILTER_BILINEAR, GR_TEXTUREFILTER_BILINEAR);
    grTexMipMapMode(GR_TMU0, GR_MIPMAP_NEAREST, FXFALSE);
    grAlphaTestFunction(GR_CMP_GREATER);
    grAlphaTestReferenceValue(0);
    if (gc->hwcx.hasCombineExt == 0) {
        grTexCombine(GR_TMU0, GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ZERO,
                     GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ZERO, FXFALSE, FXFALSE);
        grColorCombine(GR_COMBINE_FUNCTION_LOCAL, GR_COMBINE_FACTOR_ZERO, GR_COMBINE_LOCAL_ITERATED,
                       GR_COMBINE_OTHER_CONSTANT, FXFALSE);
        grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER, GR_COMBINE_FACTOR_LOCAL, GR_COMBINE_LOCAL_ITERATED,
                       GR_COMBINE_OTHER_TEXTURE, FXFALSE);
    }
    else {
        /* the same through the extension (__stdcall; 10 / 11 arguments) */
        ((int (__stdcall *)())gc->hwcx.grColorCombineExt)(9, 1, 0, 1, 0, 1, 0, 0, 0, 0);
        ((int (__stdcall *)())gc->hwcx.grAlphaCombineExt)(8, 1, 0, 1, 1, 0, 0, 0, 0, 0);
        ((int (__stdcall *)())gc->hwcx.grTexColorCombineExt)(0, 0xb, 1, 0, 1, 0, 1, 0, 0, 0, 0);
        ((int (__stdcall *)())gc->hwcx.grTexAlphaCombineExt)(0, 10, 1, 0, 1, 0, 1, 0, 0, 0, 0);
        gc->hwcx.grDirtyExt |= 0xf;
    }
    if (gc->hwcx.grCommandTransportInfoExt2 != NULL) {
        ((int (__stdcall *)())gc->hwcx.grCommandTransportInfoExt2)();
    }
    gc->hwcx.flag_355be = 1;
}

/* ---- ssttri.c: triangles written straight into the command FIFO ---------------------------------- */
/* the packet 3 parameter-mask bits (SSTCP_PKT3_*, header bits 10..) */
#define PKT3_RGB        0x01    /* with PKT3_A and the packed-colour header bit: one ARGB dword */
#define PKT3_A          0x02
#define PKT3_Z          0x04
#define PKT3_WB         0x08    /* window.w (1/w) */
#define PKT3_W0         0x10    /* TMU0 q */
#define PKT3_ST0        0x20    /* TMU0 s, t */
#define PKT3_W1         0x40    /* TMU1 q */
#define PKT3_ST1        0x80    /* TMU1 s, t */
/* one vertex in the order of the parameter mask.  The (__GLfloat) casts are as in __glSSTRenderBigPoint: they make
** VC6 copy through fld / fstp as the original does (a plain float copy becomes a dword move, which the original
** has only for x) */
#define PKT3_VERTEX(pkt, v, params) \
    *(__GLfloat *)(pkt)++ = (v)->window.x; \
    *(__GLfloat *)(pkt)++ = (__GLfloat)((v)->window.y); \
    *(pkt)++ = (v)->packedColor; \
    if ((params) & PKT3_Z) { \
        *(__GLfloat *)(pkt)++ = (__GLfloat)((v)->window.z); \
    } \
    *(__GLfloat *)(pkt)++ = (__GLfloat)((v)->window.w); \
    if ((params) & PKT3_W0) { \
        *(__GLfloat *)(pkt)++ = (__GLfloat)((v)->texture[gc->fx.tmuTexUnit[0]].w); \
    } \
    if ((params) & PKT3_ST0) { \
        *(__GLfloat *)(pkt)++ = (__GLfloat)((v)->texture[gc->fx.tmuTexUnit[0]].x); \
        *(__GLfloat *)(pkt)++ = (__GLfloat)((v)->texture[gc->fx.tmuTexUnit[0]].y); \
    } \
    if ((params) & PKT3_W1) { \
        *(__GLfloat *)(pkt)++ = (__GLfloat)((v)->texture[gc->fx.tmuTexUnit[1]].w); \
    } \
    if ((params) & PKT3_ST1) { \
        *(__GLfloat *)(pkt)++ = (__GLfloat)((v)->texture[gc->fx.tmuTexUnit[1]].x); \
        *(__GLfloat *)(pkt)++ = (__GLfloat)((v)->texture[gc->fx.tmuTexUnit[1]].y); \
    }

/* the parameter mask of the current state, and vertexSize = bytes per vertex without 1/w: x, y, ARGB, z with depth
** test, 1/w, and per textured TMU s, t (q too with hwcx.nonUnityQ).  One TMU: unit 0 on TMU0; two TMUs: unit 0's
** coordinates go to TMU1, unit 1's to TMU0 */
#define PKT3_PARAMS(gc, params, vertexSize) \
    (params) = PKT3_RGB | PKT3_A; \
    (vertexSize) = 12; \
    if ((gc)->state.enables.general & __GL_DEPTH_TEST_ENABLE) { \
        (params) = PKT3_RGB | PKT3_A | PKT3_Z; \
        (vertexSize) = 16; \
    } \
    if ((gc)->hwcx.numTMU == 1) { \
        if ((gc)->texture.textureEnabled[0]) { \
            (params) |= PKT3_ST0; \
            (vertexSize) += 8; \
        } \
        if (((params) & PKT3_ST0) && (gc)->hwcx.nonUnityQ) { \
            (params) |= PKT3_W0; \
            (vertexSize) += 4; \
        } \
    } else { \
        if ((gc)->texture.textureEnabled[0]) { \
            (params) |= PKT3_ST1; \
            (vertexSize) += 8; \
        } \
        if ((gc)->texture.textureEnabled[1]) { \
            (params) |= PKT3_ST0; \
            (vertexSize) += 8; \
        } \
        if (((params) & (PKT3_ST0 | PKT3_ST1)) && (gc)->hwcx.nonUnityQ) { \
            (params) |= PKT3_W0 | PKT3_W1; \
            (vertexSize) += 8; \
        } \
    } \
    (params) |= PKT3_WB
/* packet 3 headers (packed colour): type 3, the command in bits 3..5, the vertex count in bits 6..9 */
#define PKT3_HDR_TRIANGLE   0x100000c3      /* independent triangles, 3 vertices */
#define PKT3_HDR_STRIP3     0x100000cb      /* start a new strip, 3 vertices */
#define PKT3_HDR_CONTINUE1  0x10000053      /* continue the strip, 1 vertex */
#define FIFO_MAKEROOM_TRI(fifo, n, line) \
    if ((fifo)->fifoRoom < (n)) \
        ((void (__stdcall *)(GLint, const char *, GLint))gc->hwcx.grCommandTransportMakeRoomExt2)((n), ".\\ssttri.c", (line))
/* (the original's __FILE__ of ssttri.c: a literal per use, so each function has its own copy, as the original) */

/* 6906090b -- hwFifoDrawTriangle: draw a triangle by writing one independent-triangle packet 3 straight into Glide's
** command FIFO; ToDo UNUSED CODE: nothing in the driver calls it */
void __fastcall hwFifoDrawTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    GrCmdTransportInfo *fifo = gc->hwcx.cmdTransportInfo;
    GLuint params, hdr;
    GLint vertexSize, size;
    GLuint *pkt;

    if (**fifo->ptrLostContext == 0) {
        PKT3_PARAMS(gc, params, vertexSize);
        hdr = (params << 10) | PKT3_HDR_TRIANGLE;
        size = (vertexSize + 4) * 3 + 4;
        FIFO_MAKEROOM_TRI(fifo, size, 0xad);
        fifo->fifoRoom -= size;
        pkt = fifo->fifoPtr;
        *pkt++ = hdr;
        PKT3_VERTEX(pkt, a, params);
        PKT3_VERTEX(pkt, b, params);
        PKT3_VERTEX(pkt, c, params);
        fifo->fifoPtr = pkt;
    }
}

/* 69060c6b -- hwFifoBeginStrip: start a new triangle strip in Glide's command FIFO with the triangle a, b, c (one
** packet 3; hwFifoStripVertex adds vertices to it); ToDo UNUSED CODE: nothing in the driver calls it */
void __fastcall hwFifoBeginStrip(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    GrCmdTransportInfo *fifo = gc->hwcx.cmdTransportInfo;
    GLuint params, hdr;
    GLint vertexSize, size;
    GLuint *pkt;

    if (**fifo->ptrLostContext == 0) {
        PKT3_PARAMS(gc, params, vertexSize);
        hdr = (params << 10) | PKT3_HDR_STRIP3;
        size = (vertexSize + 4) * 3 + 4;
        FIFO_MAKEROOM_TRI(fifo, size, 0x16a);
        fifo->fifoRoom -= size;
        pkt = fifo->fifoPtr;
        *pkt++ = hdr;
        PKT3_VERTEX(pkt, a, params);
        PKT3_VERTEX(pkt, b, params);
        PKT3_VERTEX(pkt, c, params);
        fifo->fifoPtr = pkt;
    }
}

/* 69060fcb -- hwFifoStripVertex: continue the triangle strip in Glide's command FIFO with the vertex v (one packet
** 3); ToDo UNUSED CODE: nothing in the driver calls it */
void __fastcall hwFifoStripVertex(__GLcontext *gc, __GLvertexICD *v)
{
    GrCmdTransportInfo *fifo = gc->hwcx.cmdTransportInfo;
    GLuint params, hdr;
    GLint vertexSize, size;
    GLuint *pkt;

    if (**fifo->ptrLostContext == 0) {
        PKT3_PARAMS(gc, params, vertexSize);
        hdr = (params << 10) | PKT3_HDR_CONTINUE1;
        size = vertexSize + 4 + 4;
        FIFO_MAKEROOM_TRI(fifo, size, 0x227);
        fifo->fifoRoom -= size;
        pkt = fifo->fifoPtr;
        *pkt++ = hdr;
        PKT3_VERTEX(pkt, v, params);
        fifo->fifoPtr = pkt;
    }
}

extern void __fastcall hwPolygonOffset(__GLcontext *gc, __GLfloat area, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c, __GLfloat dxAC, __GLfloat dxBC, __GLfloat dyAC, __GLfloat dyBC);
/* 69061180 (size 917) -- __glSSTRenderTriangle: __glRenderTriangle */
void __fastcall __glSSTRenderTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    GLuint needs, modeFlags, faceNeeds;
    GLint ccw, colorFace, face;
    __GLfloat dxAC, dxBC, dyAC, dyBC, area;
    __GLvertexICD *pv;
    GLuint smooth, twoSided;
    GLuint pa, pb, pc;

    /* Compute signed area of the triangle */
    dxAC = a->window.x - c->window.x;
    dxBC = b->window.x - c->window.x;
    dyAC = a->window.y - c->window.y;
    dyBC = b->window.y - c->window.y;
    area = dxAC * dyBC - dxBC * dyAC;
    ccw = !(*(GLint *)&area & (GLint)0x80000000);

    /* Figure out if face is culled or not */
    face = gc->polygon.face[ccw];
    if (face == gc->polygon.cullFace && (gc->state.enables.general & __GL_CULL_FACE_ENABLE)) {
        return;
    }

    modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    twoSided = modeFlags & __GL_SHADE_TWOSIDED;
    if (twoSided) {
        colorFace = face;
        faceNeeds = (*(&gc->vertex.getFaceNeeds0)[face])(gc);
    } else {
        colorFace = 0;
        faceNeeds = (*gc->vertex.getFaceNeeds0)(gc);
    }

    needs = (*gc->vertex.getNeeds)(gc);
    pv = gc->vertex.provoking;
    smooth = modeFlags & __GL_SHADE_SMOOTH_LIGHT;
    if (smooth) {
        a->color = &a->colors[colorFace];
        b->color = &b->colors[colorFace];
        c->color = &c->colors[colorFace];
        needs |= faceNeeds;
    } else {
        GLuint pvneeds;

        pv->color = &pv->colors[colorFace];
        a->color = pv->color;
        b->color = pv->color;
        c->color = pv->color;
        pvneeds = faceNeeds & 0x11b;
        if (~pv->flagBits & pvneeds) DO_VALIDATE(gc, pv, pvneeds);
        pa = a->packedColor;
        pb = b->packedColor;
        pc = c->packedColor;
        a->packedColor = pv->packedColor;
        b->packedColor = pv->packedColor;
        c->packedColor = pv->packedColor;
    }

    /* Validate vertices */
    if (~a->flagBits & needs) DO_VALIDATE(gc, a, needs);
    if (~b->flagBits & needs) DO_VALIDATE(gc, b, needs);
    if (~c->flagBits & needs) DO_VALIDATE(gc, c, needs);

    if (twoSided) {
        if (smooth) {
            hwPackColor(gc, a);
            hwPackColor(gc, b);
            hwPackColor(gc, c);
        } else {
            hwPackColor(gc, pv);
        }
    }

    /* Render triangle using the faces polygon mode */
    switch (gc->polygon.mode[face]) {
      case 2:
        if (*(GLint *)&area & 0x7fffffff) {
            if (gc->state.enables.general & __GL_POLYGON_OFFSET_FILL_ENABLE) {
                hwPolygonOffset(gc, area, a, b, c, dxAC, dxBC, dyAC, dyBC);
            }
            if (gc->state.enables.general & __GL_POLYGON_SMOOTH_ENABLE) {
                grAADrawTriangle(a, b, c, a->flagBits & 0x1000, b->flagBits & 0x1000, c->flagBits & 0x1000);
            } else {
                grDrawTriangle(a, b, c);
            }
        }
        break;
      case 0:
        if (gc->state.enables.general & __GL_POLYGON_OFFSET_POINT_ENABLE) {
            hwPolygonOffset(gc, area, a, b, c, dxAC, dxBC, dyAC, dyBC);
        }
        (*gc->procs.renderTrianglePolygonModePoint)(gc, a, b, c);
        break;
      case 1:
        if (gc->state.enables.general & __GL_POLYGON_OFFSET_LINE_ENABLE) {
            hwPolygonOffset(gc, area, a, b, c, dxAC, dxBC, dyAC, dyBC);
        }
        (*gc->procs.renderTrianglePolygonModeLine)(gc, a, b, c);
        break;
    }

    if (!smooth) {
        a->packedColor = pa;
        b->packedColor = pb;
        c->packedColor = pc;
    }
    /* Restore color pointers */
    a->color = &a->colors[0];
    b->color = &b->colors[0];
    c->color = &c->colors[0];
    if (!smooth) {
        pv->color = &pv->colors[0];
    }
}

/* 69061520 (size 238) -- hwPolygonOffset: polygon offset -> grDepthBiasLevel (units, plus factor * max |dz/dx|,
** |dz/dy|). area precedes a: a float never goes in a register, so a is still EDX, and the callers evaluate a before
** pushing area (arguments right to left), as the original does */
void __fastcall hwPolygonOffset(__GLcontext *gc, __GLfloat area, __GLvertexICD *a, __GLvertexICD *b,
                             __GLvertexICD *c, __GLfloat dxAC, __GLfloat dxBC, __GLfloat dyAC, __GLfloat dyBC)
{
    GLint depthBits = gc->modes.depthBits;
    GLint bias;

    if (depthBits > 16 && gc->fx.zPrecision32) {
        bias = (GLint)(gc->state.polygon.units * 256.0f);
    } else {
        bias = (GLint)gc->state.polygon.units;
        if (depthBits > 16) {
            bias <<= 8;
        }
    }
    if (gc->state.polygon.factor != 0.0f) {
        __GLfloat oneOverArea, t1, t2, t3, t4, dzAC, dzBC, dzdx, dzdy, maxdZ;

        oneOverArea = 1.0f / area;
        t1 = dyAC * oneOverArea;
        t2 = dyBC * oneOverArea;
        t3 = dxAC * oneOverArea;
        t4 = dxBC * oneOverArea;
        dzAC = a->window.z - c->window.z;
        dzBC = b->window.z - c->window.z;
        dzdx = dzAC * t2 - dzBC * t1;
        dzdy = dzBC * t3 - dzAC * t4;
        if ((__GLfloat)fabs(dzdx) > (__GLfloat)fabs(dzdy)) {
            maxdZ = (__GLfloat)fabs(dzdx);
        } else {
            maxdZ = (__GLfloat)fabs(dzdy);
        }
        bias += (GLint)(maxdZ * gc->state.polygon.factor);
    }
    grDepthBiasLevel(bias);
    gc->hwcx.grDirty |= 1 << 18;
}

/* 69061610 (size 434): __glSSTRenderSmoothTriangle (Glide fill) */
void __fastcall __glSSTRenderSmoothTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    GLuint needs, modeFlags, faceNeeds, twoSided;
    GLint ccw, colorFace, face;
    __GLfloat dxAC, dxBC, dyAC, dyBC, area;

    /* Compute signed area of the triangle */
    dxAC = a->window.x - c->window.x;
    dxBC = b->window.x - c->window.x;
    dyAC = a->window.y - c->window.y;
    dyBC = b->window.y - c->window.y;
    area = dxAC * dyBC - dxBC * dyAC;
    ccw = !(*(GLint *)&area & (GLint)0x80000000);

    /* Figure out if face is culled or not */
    face = gc->polygon.face[ccw];
    if (face == gc->polygon.cullFace && (gc->state.enables.general & __GL_CULL_FACE_ENABLE)) {
        return;
    }

    modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    twoSided = modeFlags & __GL_SHADE_TWOSIDED;
    if (twoSided) {
        colorFace = face;
        faceNeeds = (*(&gc->vertex.getFaceNeeds0)[face])(gc);
    } else {
        colorFace = 0;
        faceNeeds = (*gc->vertex.getFaceNeeds0)(gc);
    }
    a->color = &a->colors[colorFace];
    b->color = &b->colors[colorFace];
    c->color = &c->colors[colorFace];
    needs = (*gc->vertex.getNeeds)(gc) | faceNeeds;

    /* Validate vertices */
    if (~a->flagBits & needs) DO_VALIDATE(gc, a, needs);
    if (~b->flagBits & needs) DO_VALIDATE(gc, b, needs);
    if (~c->flagBits & needs) DO_VALIDATE(gc, c, needs);

    if (twoSided) {
        hwPackColor(gc, a);
        hwPackColor(gc, b);
        hwPackColor(gc, c);
    }
    if (gc->state.enables.general & __GL_POLYGON_SMOOTH_ENABLE) {
        grAADrawTriangle(a, b, c, a->flagBits & 0x1000, b->flagBits & 0x1000, c->flagBits & 0x1000);
    } else {
        grDrawTriangle(a, b, c);
    }

    /* Restore color pointers */
    a->color = &a->colors[0];
    b->color = &b->colors[0];
    c->color = &c->colors[0];
}

/* 690617d0 (size 494) -- __glSSTRenderFlatTriangle: __glRenderFlatTriangle (Glide fill: the provoking colour
** packed into all three) */
void __fastcall __glSSTRenderFlatTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    GLuint needs, pvneeds, modeFlags, faceNeeds, twoSided;
    GLint ccw, colorFace, face;
    __GLfloat dxAC, dxBC, dyAC, dyBC, area;
    __GLvertexICD *pv;
    GLuint pa, pb, pc;

    /* Compute signed area of the triangle */
    dxAC = a->window.x - c->window.x;
    dxBC = b->window.x - c->window.x;
    dyAC = a->window.y - c->window.y;
    dyBC = b->window.y - c->window.y;
    area = dxAC * dyBC - dxBC * dyAC;
    ccw = !(*(GLint *)&area & (GLint)0x80000000);

    /* Figure out if face is culled or not */
    face = gc->polygon.face[ccw];
    if (face == gc->polygon.cullFace && (gc->state.enables.general & __GL_CULL_FACE_ENABLE)) {
        return;
    }

    modeFlags = (*gc->polygon.shader.getModeFlags)(gc);
    twoSided = modeFlags & __GL_SHADE_TWOSIDED;
    if (twoSided) {
        colorFace = face;
        faceNeeds = (*(&gc->vertex.getFaceNeeds0)[face])(gc);
    } else {
        colorFace = 0;
        faceNeeds = (*gc->vertex.getFaceNeeds0)(gc);
    }
    needs = (*gc->vertex.getNeeds)(gc);
    pv = gc->vertex.provoking;

    /* Validate vertices */
    if (~a->flagBits & needs) DO_VALIDATE(gc, a, needs);
    if (~b->flagBits & needs) DO_VALIDATE(gc, b, needs);
    if (~c->flagBits & needs) DO_VALIDATE(gc, c, needs);

    pv->color = &pv->colors[colorFace];
    pvneeds = faceNeeds & 0x11b;
    if (~pv->flagBits & pvneeds) DO_VALIDATE(gc, pv, pvneeds);

    pa = a->packedColor;
    pb = b->packedColor;
    pc = c->packedColor;
    if (twoSided) {
        hwPackColor(gc, pv);
    }
    a->packedColor = pv->packedColor;
    b->packedColor = pv->packedColor;
    c->packedColor = pv->packedColor;
    if (gc->state.enables.general & __GL_POLYGON_SMOOTH_ENABLE) {
        grAADrawTriangle(a, b, c, a->flagBits & 0x1000, b->flagBits & 0x1000, c->flagBits & 0x1000);
    } else {
        grDrawTriangle(a, b, c);
    }
    a->packedColor = pa;
    b->packedColor = pb;
    c->packedColor = pc;

    /* Restore color pointers */
    pv->color = &pv->colors[0];
}

/* 690619c0 (size 888) -- hwDrawIndexedTriangles: indexed triangles; unclipped, filled, visible ones are joined into
** Glide strips. The flag words are re-read as expressions (`orAll |= ...; orCodes = <same>;`), not cached: caching
** them changes the whole register allocation.  p[3] is the original's contiguous frame array; the p[] stores are
** written once after the strip if/else (VC6 copies them into both arms) */
void __fastcall hwDrawIndexedTriangles(__GLcontext *gc, GLint *indices, __GLvertexICD *base, GLint count)
{
    GLint n;
    GLint needFlush;
    GLint parity;
    GLint i;
    GLuint orAll, andAll;
    GLuint orCodes, andCodes;
    __GLvertexICD *p[3];

    needFlush = 1;
    n = 0;
    parity = 0;
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    andAll = 0xffffffff;
    orAll = 0;
    if (gc->state.polygon.frontMode == gc->state.polygon.backMode &&
        gc->state.polygon.frontMode == GL_FILL &&
        !(gc->state.enables.general & (__GL_LIGHTING_ENABLE | __GL_POLYGON_OFFSET_ANY_ENABLE)) &&
        (!(gc->state.enables.general & __GL_FOG_ENABLE) || gc->state.fog.coordSource != GL_FOG_COORDINATE_EXT)) {
        for (i = 0; i < count; ) {
            GLint a, b, c;
            __GLvertexICD *v0, *v1, *v2;

            a = indices[i++];
            b = indices[i++];
            c = indices[i++];
            v0 = &base[a];
            v1 = &base[b];
            v2 = &base[c];
            gc->line.notResetStipple = GL_FALSE;
            gc->vertex.provoking = v2;
            orAll |= v0->flagBits | v1->flagBits | v2->flagBits;
            orCodes = v0->flagBits | v1->flagBits | v2->flagBits;
            andAll &= v0->flagBits & v1->flagBits & v2->flagBits;
            andCodes = v0->flagBits & v1->flagBits & v2->flagBits;
            if ((andCodes & 0xffff2000) ||
                (!(orCodes & 0xfff00000) && hwTriangleCulled(gc, v0, v1, v2))) {
                needFlush = 1;
            } else if (!(orCodes & 0xffff2000)) {
                if (!parity ? (v0 != p[2] || v1 != p[1]) : (v0 != p[0] || v1 != p[2]))
                    goto flush;
                if (needFlush) {
flush:
                    grDrawVertexArray(GR_TRIANGLE_STRIP, n, g_stripVerts);
                    needFlush = 0;
                    parity = 0;
                    g_stripVerts[0] = (GLuint)v0;
                    g_stripVerts[1] = (GLuint)v1;
                    g_stripVerts[2] = (GLuint)v2;
                    n = 3;
                } else {
                    g_stripVerts[n++] = (GLuint)v2;
                    parity ^= 1;
                }
                p[0] = v0;
                p[1] = v1;
                p[2] = v2;
            } else if (gc->state.enables.clipPlanes) {
                grDrawVertexArray(GR_TRIANGLE_STRIP, n, g_stripVerts);
                n = 0;
                needFlush = 1;
                __glDoClipTriangle(gc, v0, v1, v2);
            } else {
                if (orCodes & 0x300000) {
                    (*gc->vertexArray.fetchVertex)(gc, v0);
                    (*gc->vertexArray.fetchVertex)(gc, v1);
                    (*gc->vertexArray.fetchVertex)(gc, v2);
                } else {
                    (*gc->vertexArray.fetchVertex)(gc, v0);
                    (*gc->vertexArray.fetchVertex)(gc, v1);
                    (*gc->vertexArray.fetchVertex)(gc, v2);
                }
                grDrawVertexArray(GR_TRIANGLE_STRIP, n, g_stripVerts);
                n = 0;
                needFlush = 1;
                (*(void (__fastcall *)(__GLcontext *, GLint, __GLvertexICD *, __GLvertexICD *, __GLvertexICD *))
                    gc->hwcx.genCode[__GL_GEN_CLIP_TRIANGLE])(gc, 0x15, v0, v1, v2);
            }
        }
        grDrawVertexArray(GR_TRIANGLE_STRIP, n, g_stripVerts);
        return;
    }
    for (i = 0; i < count; ) {
        GLint a, b, c;
        __GLvertexICD *v0, *v1, *v2;

        a = indices[i++];
        b = indices[i++];
        c = indices[i++];
        gc->line.notResetStipple = GL_FALSE;
        v0 = &base[a];
        v1 = &base[b];
        v2 = &base[c];
        gc->vertex.provoking = v2;
        if (!((v0->flagBits | v1->flagBits | v2->flagBits) & 0xffff2000)) {
            (*gc->procs.renderTriangle)(gc, v0, v1, v2);
        } else if (!(v0->flagBits & v1->flagBits & v2->flagBits & 0xffff2000)) {
            __glDoClipTriangle(gc, v0, v1, v2);
        }
    }
}

/* 69061d40 (size 95) -- hwTriangleCulled: is the triangle back-face culled? */
int __fastcall hwTriangleCulled(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c)
{
    __GLfloat dxAC, dxBC, dyAC, dyBC, area;
    GLint ccw;

    if (gc->state.enables.general & __GL_CULL_FACE_ENABLE) {
        dxAC = a->window.x - c->window.x;
        dxBC = b->window.x - c->window.x;
        dyAC = a->window.y - c->window.y;
        dyBC = b->window.y - c->window.y;
        area = dxAC * dyBC - dxBC * dyAC;
        ccw = !(*(GLint *)&area & (GLint)0x80000000);
        return gc->polygon.face[ccw] == gc->polygon.cullFace;
    }
    return 0;
}

/* hwcx.growBufB as used below: a word per vertex-cache index, 1 once the vertex has been filled */
#define VERTEX_FILLED(gc)   ((GLint *)(gc)->hwcx.growBufB)
#define FILL_VERTEX(gc, i) \
    ((void (__fastcall *)(__GLcontext *, GLint, GLint, GLint))(gc)->vertexArray.fillLockedRange)(gc, i, i, 1)
/* 69061da0 -- hwDrawIndexedTrianglesLazyFill: hwDrawIndexedTriangles (690619c0) filling the vertex cache on demand: each index
** not yet marked in hwcx.growBufB is filled alone (vertexArray.fillLockedRange, count 1) and marked before the
** triangle is used; no GL_FOG_COORDINATE_EXT test on the strip path; ToDo UNUSED CODE: nothing in the driver calls it */
void __fastcall hwDrawIndexedTrianglesLazyFill(__GLcontext *gc, GLint *indices, __GLvertexICD *base, GLint count)
{
    GLint n;
    GLint needFlush;
    GLint parity;
    GLint i;
    GLuint orAll, andAll;
    GLuint orCodes, andCodes;
    __GLvertexICD *p[3];

    needFlush = 1;
    n = 0;
    parity = 0;
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    andAll = 0xffffffff;
    orAll = 0;
    if (gc->state.polygon.frontMode == gc->state.polygon.backMode &&
        gc->state.polygon.frontMode == GL_FILL &&
        !(gc->state.enables.general & (__GL_LIGHTING_ENABLE | __GL_POLYGON_OFFSET_ANY_ENABLE))) {
        for (i = 0; i < count; ) {
            GLint a, b, c;
            __GLvertexICD *v0, *v1, *v2;

            a = indices[i++];
            b = indices[i++];
            c = indices[i++];
            if (VERTEX_FILLED(gc)[a] != 1) FILL_VERTEX(gc, a);
            if (VERTEX_FILLED(gc)[b] != 1) FILL_VERTEX(gc, b);
            if (VERTEX_FILLED(gc)[c] != 1) FILL_VERTEX(gc, c);
            VERTEX_FILLED(gc)[a] = 1;
            VERTEX_FILLED(gc)[b] = 1;
            VERTEX_FILLED(gc)[c] = 1;
            v0 = &base[a];
            v1 = &base[b];
            v2 = &base[c];
            gc->line.notResetStipple = GL_FALSE;
            gc->vertex.provoking = v2;
            orAll |= v0->flagBits | v1->flagBits | v2->flagBits;
            orCodes = v0->flagBits | v1->flagBits | v2->flagBits;
            andAll &= v0->flagBits & v1->flagBits & v2->flagBits;
            andCodes = v0->flagBits & v1->flagBits & v2->flagBits;
            if ((andCodes & 0xffff2000) ||
                (!(orCodes & 0xfff00000) && hwTriangleCulled(gc, v0, v1, v2))) {
                needFlush = 1;
            } else if (!(orCodes & 0xffff2000)) {
                if (!parity ? (v0 != p[2] || v1 != p[1]) : (v0 != p[0] || v1 != p[2]))
                    goto flush;
                if (needFlush) {
flush:
                    grDrawVertexArray(GR_TRIANGLE_STRIP, n, g_stripVerts);
                    needFlush = 0;
                    parity = 0;
                    g_stripVerts[0] = (GLuint)v0;
                    g_stripVerts[1] = (GLuint)v1;
                    g_stripVerts[2] = (GLuint)v2;
                    n = 3;
                } else {
                    g_stripVerts[n++] = (GLuint)v2;
                    parity ^= 1;
                }
                p[0] = v0;
                p[1] = v1;
                p[2] = v2;
            } else if (gc->state.enables.clipPlanes) {
                grDrawVertexArray(GR_TRIANGLE_STRIP, n, g_stripVerts);
                n = 0;
                needFlush = 1;
                __glDoClipTriangle(gc, v0, v1, v2);
            } else {
                if (orCodes & 0x300000) {
                    (*gc->vertexArray.fetchVertex)(gc, v0);
                    (*gc->vertexArray.fetchVertex)(gc, v1);
                    (*gc->vertexArray.fetchVertex)(gc, v2);
                } else {
                    (*gc->vertexArray.fetchVertex)(gc, v0);
                    (*gc->vertexArray.fetchVertex)(gc, v1);
                    (*gc->vertexArray.fetchVertex)(gc, v2);
                }
                grDrawVertexArray(GR_TRIANGLE_STRIP, n, g_stripVerts);
                n = 0;
                needFlush = 1;
                (*(void (__fastcall *)(__GLcontext *, GLint, __GLvertexICD *, __GLvertexICD *, __GLvertexICD *))
                    gc->hwcx.genCode[__GL_GEN_CLIP_TRIANGLE])(gc, 0x15, v0, v1, v2);
            }
        }
        grDrawVertexArray(GR_TRIANGLE_STRIP, n, g_stripVerts);
        return;
    }
    for (i = 0; i < count; ) {
        GLint a, b, c;
        __GLvertexICD *v0, *v1, *v2;

        a = indices[i++];
        b = indices[i++];
        c = indices[i++];
        if (VERTEX_FILLED(gc)[a] != 1) FILL_VERTEX(gc, a);
        if (VERTEX_FILLED(gc)[b] != 1) FILL_VERTEX(gc, b);
        if (VERTEX_FILLED(gc)[c] != 1) FILL_VERTEX(gc, c);
        VERTEX_FILLED(gc)[a] = 1;
        VERTEX_FILLED(gc)[b] = 1;
        VERTEX_FILLED(gc)[c] = 1;
        gc->line.notResetStipple = GL_FALSE;
        v0 = &base[a];
        v1 = &base[b];
        v2 = &base[c];
        gc->vertex.provoking = v2;
        if (!((v0->flagBits | v1->flagBits | v2->flagBits) & 0xffff2000)) {
            (*gc->procs.renderTriangle)(gc, v0, v1, v2);
        } else if (!(v0->flagBits & v1->flagBits & v2->flagBits & 0xffff2000)) {
            __glDoClipTriangle(gc, v0, v1, v2);
        }
    }
}

/* 69063250 -- hwCullTriangleKey: the bufRec[1] key of the culled-triangle block (sst_tribuild.c 69062310): two-sided
** lighting, polygon smooth, cull face */
void __fastcall hwCullTriangleKey(__GLcontext *gc, GLuint *key)
{
  key[0] = (*gc->polygon.shader.getModeFlags)(gc) & __GL_SHADE_TWOSIDED;
  key[1] = gc->state.enables.general & __GL_POLYGON_SMOOTH_ENABLE;
  key[2] = gc->state.enables.general & __GL_CULL_FACE_ENABLE;
}

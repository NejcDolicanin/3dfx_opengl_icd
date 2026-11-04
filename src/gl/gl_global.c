/*
** gl_global.c -- s_global.c: the SI's global tables (690f5328 .. 690f53d8 in the original's .data).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "icdglob.h"

/* 690f5328 */
__GLcoord __gl_frustumClipPlanes[6] = {
    {  1.0,  0.0,  0.0,  1.0 },		/* left */
    { -1.0,  0.0,  0.0,  1.0 },		/* right */
    {  0.0,  1.0,  0.0,  1.0 },		/* bottom */
    {  0.0, -1.0,  0.0,  1.0 },		/* top */
    {  0.0,  0.0,  1.0,  1.0 },		/* near */
    {  0.0,  0.0, -1.0,  1.0 },		/* far */
};

/* 690f5388 */
GLbyte __glDitherTable[16] = {
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5,
};

/* 690f5398 */
__GLfloat __glDitherTableFloat[16] = {
    -0.468750,  0.031250, -0.343750,  0.156250,
     0.281250, -0.218750,  0.406250, -0.093750,
    -0.281250,  0.218750, -0.406250,  0.093750,
     0.468750, -0.031250,  0.343750, -0.156250,
};

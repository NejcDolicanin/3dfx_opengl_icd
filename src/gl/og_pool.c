/*
** og_pool.c -- og code generator helpers: the 8-byte constant pool and the bit-run of a mask.
*/
#include <string.h>
#include <stdlib.h>
#include "wglpriv.h"
#include "og.h"

/* 690e97b0 -- og_mxk4 (3dfx OG.C mxk4): a memory operand for the 8-byte constant {a, b, c, d}, interned in
** gc->ogPool2
   (the pool is reallocated empty when full: the code generated so far keeps the old one) */
OgOperand *__fastcall og_mxk4(__GLcontext *gc, GLushort a, GLushort b, GLushort c, GLushort d)
{
    GLushort v[4];
    GLuint n, i;
    GLuint *pool = (GLuint *)gc->ogPool2.data;

    v[0] = a;
    v[1] = b;
    v[2] = c;
    v[3] = d;
    n = gc->ogPool2.count;          /* read after the constant is built (the original's order) */
    for (i = 0; i < n; i++)
        if (memcmp(&pool[i * 2], v, 8) == 0)
            goto found;
    pool[n * 2] = ((GLuint *)v)[0];
    pool[n * 2 + 1] = ((GLuint *)v)[1];
    if (++gc->ogPool2.count == gc->ogPool2.capacity) {
        gc->ogPool2.count = 0;
        gc->ogPool2.capacity *= 2;
        gc->ogPool2.data = calloc(4, gc->ogPool2.capacity);
    }
found:
    return og_memBase(gc, IMM(&pool[i * 2]), NULL);
}

/* 690e9bb0 -- __glMaskToBitsShift: the lowest run of set bits of mask, its width and shift */
void __fastcall __glMaskToBitsShift(GLuint mask, int *width, int *shift)
{
    int i = 0, lo = 0;

    if (mask) {
        while (~mask & (1 << i))
            i++;
        lo = i;
        while (mask & (1 << i))
            i++;
    }
    *width = i - lo;
    *shift = lo;
}

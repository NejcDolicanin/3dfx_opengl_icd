/*
** gl_nulldisp.c -- the entries of the empty ("no context") dispatch table: __glNullDispatchReturn0 for the
** functions that return a value and __glNullDispatchN, one per stack-argument count (ret 4N), that do nothing.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glide.h>
#include "glsitypes.h"

/* ---- no-op entries of the empty dispatch table (__glNopDispatchState, gl_disp.c): `xor eax,eax; ret N` (the argument
** count follows N) ---- */
/* 690cd940 -- __glNullDispatch2: no-op entry of the empty dispatch table (two arguments, ret 8) */
GLuint __stdcall __glNullDispatch2(int a1, int a2)
{
    return 0;
}

/* 690cd950 -- __glNullDispatchReturn0: the "no context" dispatch entry of the functions that return a value, 0 */
GLuint __fastcall __glNullDispatchReturn0(void)
{
    return 0;
}

/* 690cd960 -- __glNullDispatch1: (one argument, ret 4) */
GLuint __stdcall __glNullDispatch1(int a1)
{
    return 0;
}

/* 690cd970 -- __glNullDispatch3: (three arguments, ret 0xc) */
GLuint __stdcall __glNullDispatch3(int a1, int a2, int a3)
{
    return 0;
}

/* 690cd980 -- __glNullDispatch7: (seven arguments, ret 0x1c) */
GLuint __stdcall __glNullDispatch7(int a1, int a2, int a3, int a4, int a5, int a6, int a7)
{
    return 0;
}

/* 690cd990 -- __glNullDispatch6: (six arguments, ret 0x18) */
GLuint __stdcall __glNullDispatch6(int a1, int a2, int a3, int a4, int a5, int a6)
{
    return 0;
}

/* 690cd9a0 -- __glNullDispatch4: (four arguments, ret 0x10) */
GLuint __stdcall __glNullDispatch4(int a1, int a2, int a3, int a4)
{
    return 0;
}

/* 690cd9b0 -- __glNullDispatch8: (eight arguments, ret 0x20) */
GLuint __stdcall __glNullDispatch8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
    return 0;
}

/* 690cd9c0 -- __glNullDispatch9: (nine arguments, ret 0x24) */
GLuint __stdcall __glNullDispatch9(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
    return 0;
}

/* 690cd9d0 -- __glNullDispatch14: (fourteen arguments, ret 0x38) */
GLuint __stdcall __glNullDispatch14(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10, int a11, int a12, int a13, int a14)
{
    return 0;
}

/* 690cd9e0 -- __glNullDispatch10: (ten arguments, ret 0x28) */
GLuint __stdcall __glNullDispatch10(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10)
{
    return 0;
}

/* 690cd9f0 -- __glNullDispatch5: (five arguments, ret 0x14) */
GLuint __stdcall __glNullDispatch5(int a1, int a2, int a3, int a4, int a5)
{
    return 0;
}

/* 690cda00 -- __glNullDispatch12: no-op entry of the empty dispatch table (twelve arguments, ret 0x30) */
GLuint __stdcall __glNullDispatch12(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9,
                              int a10, int a11, int a12)
{
    return 0;
}

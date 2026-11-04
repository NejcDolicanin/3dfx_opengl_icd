/*
** glxform_asm.c -- the routines whose body is an inline-assembly block (the hand-scheduled x87
** matrix transforms): copied from the original.
*/
#include "wglpriv.h"

/* 690e0110 -- __glI386XForm3: xf3 of a general matrix in x87 assembly; the block below is the original's, the
** frame around it
** is VC6's */
void __fastcall __glI386XForm3(float *res, const float *v, const float *m)
{
    const float *mm = m;
    __asm {
        mov eax,mm
        mov esi,v
        mov edi,res
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax]
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax+0x4]
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax+0x8]
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax+0xc]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x10]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x14]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x18]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x1c]
        fxch st(7)
        faddp st(3),st
        fxch st(5)
        faddp st(1),st
        fxch st(3)
        faddp st(4),st
        fxch st(1)
        faddp st(4),st
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x20]
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x24]
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x28]
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x2c]
        fxch st(3)
        faddp st(4),st
        fxch st(1)
        faddp st(4),st
        faddp st(4),st
        faddp st(4),st
        fadd DWORD PTR [eax+0x30]
        fxch st(1)
        fadd DWORD PTR [eax+0x34]
        fxch st(2)
        fadd DWORD PTR [eax+0x38]
        fxch st(3)
        fadd DWORD PTR [eax+0x3c]
        fxch st(1)
        fstp DWORD PTR [edi]
        fxch st(1)
        fstp DWORD PTR [edi+0x4]
        fstp DWORD PTR [edi+0xc]
        fstp DWORD PTR [edi+0x8]
    }
}

#pragma optimize("", off)       /* compiled unoptimised (the spilled ecx / edx slots and the
                                ** int3 tail of the original: no nop padding) */
/* 690e01c0 -- __glI386XForm3Batch: 690e0110 over count vertices (vStride / resStride; unreferenced); the block
** below is the original's, the frame around it
** is VC6's */
/* C equivalent (m is the 4x4 matrix, row-major m[row * 4 + col]; the strides are in bytes):
**
**     for (; count > 0; count--) {
**         for (j = 0; j < 4; j++)
**             res[j] = v[0] * m[j] + v[1] * m[4 + j] + v[2] * m[8 + j] + m[12 + j];   (w = 1)
**         v = (const float *)((const char *)v + vStride);
**         res = (float *)((char *)res + resStride);
**     }
*/
void __fastcall __glI386XForm3Batch(float *res, const float *v, const float *m, int vStride, int resStride, int count)
{
    const float *mm;
    __asm {
        /* the original copies m inside the block (mm gets its own slot) */
        mov eax,m
        mov mm,eax
        mov eax,mm
        mov esi,v
        mov edi,res
        mov ecx,count
        mov ebx,vStride
        mov edx,resStride
        cmp ecx,0x0
        jle L_690e027b
L_690e01f0:
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax]
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax+0x4]
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax+0x8]
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax+0xc]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x10]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x14]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x18]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x1c]
        fxch st(7)
        faddp st(3),st
        fxch st(5)
        faddp st(1),st
        fxch st(3)
        faddp st(4),st
        fxch st(1)
        faddp st(4),st
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x20]
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x24]
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x28]
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x2c]
        fxch st(3)
        faddp st(4),st
        fxch st(1)
        faddp st(4),st
        faddp st(4),st
        faddp st(4),st
        fadd DWORD PTR [eax+0x30]
        fxch st(1)
        fadd DWORD PTR [eax+0x34]
        fxch st(2)
        fadd DWORD PTR [eax+0x38]
        fxch st(3)
        fadd DWORD PTR [eax+0x3c]
        fxch st(1)
        fstp DWORD PTR [edi]
        fxch st(1)
        fstp DWORD PTR [edi+0x4]
        fstp DWORD PTR [edi+0xc]
        fstp DWORD PTR [edi+0x8]
        add esi,ebx
        add edi,edx
        dec ecx
        jne L_690e01f0
L_690e027b:
        nop
    }
}
#pragma optimize("", on)

/* 690e04d0 -- __glI386XForm3_W: xf3 of a W0001 matrix; the block below is the original's, the frame around it
** is VC6's */
void __fastcall __glI386XForm3_W(float *res, const float *v, const float *m)
{
    const float *mm = m;
    __asm {
        mov edi,res
        mov esi,v
        mov eax,mm
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax]
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax+0x4]
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax+0x8]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x10]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x14]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x18]
        fxch st(5)
        faddp st(2),st
        faddp st(3),st
        fxch st(1)
        faddp st(3),st
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x20]
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x24]
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x28]
        fxch st(2)
        faddp st(3),st
        faddp st(3),st
        faddp st(3),st
        fadd DWORD PTR [eax+0x30]
        fxch st(1)
        fadd DWORD PTR [eax+0x34]
        fxch st(2)
        fadd DWORD PTR [eax+0x38]
        mov DWORD PTR [edi+0xc],0x3f800000
        fxch st(1)
        fstp DWORD PTR [edi]
        fstp DWORD PTR [edi+0x8]
        fstp DWORD PTR [edi+0x4]
    }
}

/* 690e0290 -- __glI386XForm4: xf4 of a general matrix: inline-assembly routine with a C test around two blocks */
void __fastcall __glI386XForm4(float *res, const float *v, const float *m)
{
    const float *mm = m;
    if (*(const GLuint *)&v[3] == 0x3f800000) {
        __asm {
            mov eax,mm
            mov esi,v
            mov edi,res
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax]
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax+0x4]
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax+0x8]
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax+0xc]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x10]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x14]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x18]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x1c]
            fxch st(7)
            faddp st(3),st
            fxch st(5)
            faddp st(1),st
            fxch st(3)
            faddp st(4),st
            fxch st(1)
            faddp st(4),st
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x20]
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x24]
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x28]
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x2c]
            fxch st(3)
            faddp st(4),st
            fxch st(1)
            faddp st(4),st
            faddp st(4),st
            faddp st(4),st
            fadd DWORD PTR [eax+0x30]
            fxch st(1)
            fadd DWORD PTR [eax+0x34]
            fxch st(2)
            fadd DWORD PTR [eax+0x38]
            fxch st(3)
            fadd DWORD PTR [eax+0x3c]
            fxch st(1)
            fstp DWORD PTR [edi]
            fxch st(1)
            fstp DWORD PTR [edi+0x4]
            fstp DWORD PTR [edi+0xc]
            fstp DWORD PTR [edi+0x8]
        }

        return;
    }
    else {
        __asm {
            mov eax,mm
            mov esi,v
            mov edi,res
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax]
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax+0x4]
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax+0x8]
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax+0xc]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x10]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x14]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x18]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x1c]
            fxch st(7)
            faddp st(3),st
            fxch st(5)
            faddp st(1),st
            fxch st(3)
            faddp st(4),st
            fxch st(1)
            faddp st(4),st
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x20]
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x24]
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x28]
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x2c]
            fxch st(3)
            faddp st(4),st
            fxch st(1)
            faddp st(4),st
            faddp st(4),st
            faddp st(4),st
            fld DWORD PTR [esi+0xc]
            fmul DWORD PTR [eax+0x30]
            fld DWORD PTR [esi+0xc]
            fmul DWORD PTR [eax+0x34]
            fld DWORD PTR [esi+0xc]
            fmul DWORD PTR [eax+0x38]
            fld DWORD PTR [esi+0xc]
            fmul DWORD PTR [eax+0x3c]
            fxch st(3)
            faddp st(4),st
            fxch st(1)
            faddp st(4),st
            faddp st(4),st
            faddp st(4),st
            fstp DWORD PTR [edi]
            fstp DWORD PTR [edi+0x4]
            fstp DWORD PTR [edi+0x8]
            fstp DWORD PTR [edi+0xc]
        }

    }
}

/* 690e0560 -- __glI386XForm4_W: xf4 of a W0001 matrix: inline-assembly routine with a C test around two blocks */
void __fastcall __glI386XForm4_W(float *res, const float *v, const float *m)
{
    const float *mm = m;
    if (*(const GLuint *)&v[3] == 0x3f800000) {
        __asm {
            mov edi,res
            mov esi,v
            mov eax,mm
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax]
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax+0x4]
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax+0x8]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x10]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x14]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x18]
            fxch st(5)
            faddp st(2),st
            faddp st(3),st
            fxch st(1)
            faddp st(3),st
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x20]
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x24]
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x28]
            fxch st(2)
            faddp st(3),st
            faddp st(3),st
            faddp st(3),st
            fadd DWORD PTR [eax+0x30]
            fxch st(1)
            fadd DWORD PTR [eax+0x34]
            fxch st(2)
            fadd DWORD PTR [eax+0x38]
            mov DWORD PTR [edi+0xc],0x3f80000
            fxch st(1)
            fstp DWORD PTR [edi]
            fstp DWORD PTR [edi+0x8]
            fstp DWORD PTR [edi+0x4]
        }
        res[3] = v[3];                                   /* overwrites the 0x3f80000 above */
        return;
    }
    else {
        __asm {
            mov edi,res
            mov esi,v
            mov eax,mm
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax]
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax+0x4]
            fld DWORD PTR [esi]
            fmul DWORD PTR [eax+0x8]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x10]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x14]
            fld DWORD PTR [esi+0x4]
            fmul DWORD PTR [eax+0x18]
            fxch st(2)
            faddp st(5),st
            faddp st(3),st
            faddp st(1),st
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x20]
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x24]
            fld DWORD PTR [esi+0x8]
            fmul DWORD PTR [eax+0x28]
            fxch st(2)
            faddp st(5),st
            faddp st(3),st
            faddp st(1),st
            fld DWORD PTR [esi+0xc]
            fmul DWORD PTR [eax+0x30]
            fld DWORD PTR [esi+0xc]
            fmul DWORD PTR [eax+0x34]
            fld DWORD PTR [esi+0xc]
            fmul DWORD PTR [eax+0x38]
            fxch st(2)
            faddp st(5),st
            faddp st(3),st
            faddp st(1),st
            fxch st(2)
            fstp DWORD PTR [edi]
            fstp DWORD PTR [edi+0x4]
            fstp DWORD PTR [edi+0x8]
        }
        res[3] = v[3];                                   /* w passes through (C after the block) */
    }
}

#pragma optimize("", off)       /* compiled unoptimised (the spilled ecx / edx slots and the
                                ** int3 tail of the original: no nop padding) */
/* 690e03f0 -- __glI386XForm4Batch: 690e0290 over count vertices (vStride / resStride; unreferenced); the block
** below is the original's, the frame around it
** is VC6's */
/* C equivalent (m row-major as above; the strides are in bytes):
**
**     for (; count > 0; count--) {
**         for (j = 0; j < 4; j++)
**             res[j] = v[0] * m[j] + v[1] * m[4 + j] + v[2] * m[8 + j] + v[3] * m[12 + j];
**         v = (const float *)((const char *)v + vStride);
**         res = (float *)((char *)res + resStride);
**     }
*/
void __fastcall __glI386XForm4Batch(float *res, const float *v, const float *m, int vStride, int resStride, int count)
{
    const float *mm;
    __asm {
        /* the original copies m inside the block (mm gets its own slot) */
        mov eax,m
        mov mm,eax
        mov edi,res
        mov esi,v
        mov eax,mm
        mov ecx,count
        mov ebx,vStride
        mov edx,resStride
        cmp ecx,0x0
        jle L_690e04b9
L_690e0420:
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax]
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax+0x4]
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax+0x8]
        fld DWORD PTR [esi]
        fmul DWORD PTR [eax+0xc]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x10]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x14]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x18]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [eax+0x1c]
        fxch st(7)
        faddp st(3),st
        fxch st(5)
        faddp st(1),st
        fxch st(3)
        faddp st(4),st
        fxch st(1)
        faddp st(4),st
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x20]
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x24]
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x28]
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [eax+0x2c]
        fxch st(3)
        faddp st(4),st
        fxch st(1)
        faddp st(4),st
        faddp st(4),st
        faddp st(4),st
        fld DWORD PTR [esi+0xc]
        fmul DWORD PTR [eax+0x30]
        fld DWORD PTR [esi+0xc]
        fmul DWORD PTR [eax+0x34]
        fld DWORD PTR [esi+0xc]
        fmul DWORD PTR [eax+0x38]
        fld DWORD PTR [esi+0xc]
        fmul DWORD PTR [eax+0x3c]
        fxch st(3)
        faddp st(4),st
        fxch st(1)
        faddp st(4),st
        faddp st(4),st
        faddp st(4),st
        fstp DWORD PTR [edi]
        fstp DWORD PTR [edi+0x4]
        fstp DWORD PTR [edi+0x8]
        fstp DWORD PTR [edi+0xc]
        add esi,ebx
        add edi,edx
        dec ecx
        jne L_690e0420
L_690e04b9:
        nop
    }
}
#pragma optimize("", on)

#pragma optimize("", off)       /* compiled unoptimised (the spilled ecx / edx slots and the
                                ** int3 tail of the original: no nop padding) */
/* 690e32d0 -- __glI386Vvec: s_light.c Vvec in assembly (gl_pickspan.c lighting); the block below is the original's,
** the frame around it
** is VC6's */
/* C equivalent: the vector from point a to point b (homogeneous x, y, z, w; a w of +0 or -0 counts as 0,
** the asm tests the w bits shifted left by one):
**
**     if (a[3] == 0 && b[3] == 0) {                 both directions: b - a
**         res[0] = b[0] - a[0]; res[1] = b[1] - a[1]; res[2] = b[2] - a[2];
**     } else if (a[3] != 0 && b[3] != 0) {          both points: b * a.w - a * b.w
**         res[0] = b[0] * a[3] - a[0] * b[3];
**         res[1] = b[1] * a[3] - a[1] * b[3];
**         res[2] = b[2] * a[3] - a[2] * b[3];
**     } else if (a[3] != 0) {                       b is a direction: b
**         res[0] = b[0]; res[1] = b[1]; res[2] = b[2];
**     } else {                                      a is a direction: -a (the sign bit flipped)
**         res[0] = -a[0]; res[1] = -a[1]; res[2] = -a[2];
**     }
*/
void __fastcall __glI386Vvec(float *res, const float *a, const float *b)
{
    float wa;
    float wb;
    __asm {
        mov edi,res
        mov eax,a
        mov ebx,b
        mov ecx,DWORD PTR [eax+0xc]
        mov edx,DWORD PTR [ebx+0xc]
        shl ecx,1
        shl edx,1
        mov esi,ecx
        or esi,edx
        je L_690e3383
        mov esi,ecx
        and esi,edx
        jne L_690e3341
        and ecx,ecx
        jne L_690e3308
        jmp L_690e331d
L_690e3308:
        mov esi,DWORD PTR [ebx]
        mov DWORD PTR [edi],esi
        mov esi,DWORD PTR [ebx+0x4]
        mov DWORD PTR [edi+0x4],esi
        mov esi,DWORD PTR [ebx+0x8]
        mov DWORD PTR [edi+0x8],esi
        jmp L_690e339d
L_690e331d:
        mov esi,DWORD PTR [eax]
        xor esi,0x80000000
        mov DWORD PTR [edi],esi
        mov esi,DWORD PTR [eax+0x4]
        xor esi,0x80000000
        mov DWORD PTR [edi+0x4],esi
        mov esi,DWORD PTR [eax+0x8]
        xor esi,0x80000000
        mov DWORD PTR [edi+0x8],esi
        jmp L_690e339d
L_690e3341:
        mov esi,DWORD PTR [eax+0xc]
        mov wa,esi
        mov esi,DWORD PTR [ebx+0xc]
        mov wb,esi
        fld DWORD PTR [ebx]
        fmul wa
        fld DWORD PTR [ebx+0x4]
        fmul wa
        fld DWORD PTR [ebx+0x8]
        fmul wa
        fld DWORD PTR [eax]
        fmul wb
        fld DWORD PTR [eax+0x4]
        fmul wb
        fld DWORD PTR [eax+0x8]
        fmul wb
        fxch st(5)
        fsubrp st(2),st
        fsubp st(3),st
        fxch st(1)
        fsubrp st(3),st
        fstp DWORD PTR [edi]
        fstp DWORD PTR [edi+0x4]
        fstp DWORD PTR [edi+0x8]
        jmp L_690e339d
L_690e3383:
        fld DWORD PTR [ebx]
        fsub DWORD PTR [eax]
        fld DWORD PTR [ebx+0x4]
        fsub DWORD PTR [eax+0x4]
        fld DWORD PTR [ebx+0x8]
        fsub DWORD PTR [eax+0x8]
        fxch st(2)
        fstp DWORD PTR [edi]
        fstp DWORD PTR [edi+0x4]
        fstp DWORD PTR [edi+0x8]
L_690e339d:
        nop
    }
}
#pragma optimize("", on)

#pragma optimize("", off)       /* compiled unoptimised: locals in declaration order, dead store kept */
/* 690e33b0 -- __glI386NormalizeCoord: s_light.c NormalizeCoord in assembly (reciprocal square root by Newton
** steps); the block below is the original's, the frame around it
** is VC6's */
void __fastcall __glI386NormalizeCoord(float *res, const float *v)
{
    float three;
    float r;                        /* the /Od frame slots follow the names */
    float t;
    float sixteenth;
    float zero;
    float normSq;
    float twelve;
    zero = 0.0f;
    three = 3.0f;
    sixteenth = 0.0625f;
    twelve = 12.0f;
    __asm {
        mov esi,v
        mov edi,res
        fld DWORD PTR [esi]
        fmul DWORD PTR [esi]
        fld DWORD PTR [esi+0x4]
        fmul DWORD PTR [esi+0x4]
        fld DWORD PTR [esi+0x8]
        fmul DWORD PTR [esi+0x8]
        fxch st(2)
        faddp st(1),st
        faddp st(1),st
        fstp normSq
        mov eax,normSq
        and eax,eax
        js L_690e345a
        je L_690e345a
        cmp eax,0x3f800000
        je L_690e3470
        shr eax,1
        mov ebx,0x5f375a00
        sub ebx,eax
        mov r,ebx
        fld r
        fmul r
        fmul normSq
        fld three
        fsub st,st(1)
        fld st(0)
        fld st(0)
        fmulp st(2),st
        fxch st(2)
        fmulp st(1),st
        fld twelve
        fxch st(1)
        fsubp st(1),st
        fst t
        fmulp st(1),st
        fmul r
        fmul sixteenth
        fld DWORD PTR [esi]
        fmul st,st(1)
        fld DWORD PTR [esi+0x4]
        fmul st,st(2)
        fld DWORD PTR [esi+0x8]
        fmulp st(3),st
        fxch st(1)
        fstp DWORD PTR [edi]
        fstp DWORD PTR [edi+0x4]
        fstp DWORD PTR [edi+0x8]
        jmp L_end
L_690e345a:
        mov DWORD PTR [edi],0x0
        mov DWORD PTR [edi+0x4],0x0
        mov DWORD PTR [edi+0x8],0x0
        jmp L_end
L_690e3470:
        mov eax,DWORD PTR [esi]
        mov ebx,DWORD PTR [esi+0x4]
        mov ecx,DWORD PTR [esi+0x8]
        mov DWORD PTR [edi],eax
        mov DWORD PTR [edi+0x4],ebx
        mov DWORD PTR [edi+0x8],ecx
L_end:
    }
}
#pragma optimize("", on)


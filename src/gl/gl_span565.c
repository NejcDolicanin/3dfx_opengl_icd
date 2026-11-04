/*
** gl_span565.c -- the 8888 -> 565 span packer, an inline-asm loop in the original.
*/
#include "wglpriv.h"
#include "glsi.h"            /* __GLpixelSpanInfoICD */

/* 690c8620 -- __glSpanPackRGBX8ToRGB565: pack spanInfo->width 0x00BBGGRR pixels from src to 565 at dst;
** two per iteration
   when the count is even */
void __fastcall __glSpanPackRGBX8ToRGB565(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, const GLvoid *src, GLvoid *dst)
{
    GLint n = spanInfo->width;

    if (n != 0) {
        if ((n & 1) == 0) {
            n >>= 1;
            __asm {
                mov esi, src
                mov edi, dst
                mov ecx, n
            pair:
                mov eax, [esi]
                mov ebx, eax
                and ebx, 0xf8
                shl ebx, 8
                mov edx, eax
                and edx, 0xfc00
                shr edx, 5
                or ebx, edx
                and eax, 0xf80000
                shr eax, 0x13
                or ebx, eax
                mov [edi], bx
                mov eax, [esi+4]
                mov ebx, eax
                and ebx, 0xf8
                shl ebx, 8
                mov edx, eax
                and edx, 0xfc00
                shr edx, 5
                or ebx, edx
                and eax, 0xf80000
                shr eax, 0x13
                or ebx, eax
                mov [edi+2], bx
                add edi, 4
                add esi, 8
                dec ecx
                jne pair
            }
        } else {
            __asm {
                mov esi, src
                mov edi, dst
                mov ecx, n
            one:
                mov eax, [esi]
                mov ebx, eax
                and ebx, 0xf8
                shl ebx, 8
                mov edx, eax
                and edx, 0xfc00
                shr edx, 5
                or ebx, edx
                and eax, 0xf80000
                shr eax, 0x13
                or ebx, eax
                mov [edi], bx
                add edi, 2
                add esi, 4
                dec ecx
                jne one
            }
        }
    }
}

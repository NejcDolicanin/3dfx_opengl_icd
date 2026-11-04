/*
** gl_finish.c -- s_finish.c: glFinish. kernel32 is declared without dllimport here: the original calls
** GetModuleHandleA / GetProcAddress through the linker jmp [IAT] thunks.
*/
/* kernel32 without dllimport: the original calls GetModuleHandleA / GetProcAddress through the
** linker's `jmp [IAT]` thunks (690cda29) */
#define _KERNEL32_
#include <string.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "si_names.h"
#include "icdglob.h"
#include "icdprotos.h"
#include <stdlib.h>

/* 693c3c94 -- opengl32!wglSwapBuffers, the software path's copy */
FARPROC g_opengl32SwapBuffersSw;

/************************************************************************/
/* s_finish.c */

/* 690cda29 -- s_finish.c __glim_Finish, entered from the dispatch entry 690cdac0 with the caller's
** return address.  The ICD: on NT4 a glFinish issued from inside opengl32!wglSwapBuffers (the return
** address within 0x100 bytes of its entry) also swaps the context's buffers. */
void __fastcall __glim_Finish(int retAddr)
{
    int dist;
    __GL_SETUP_NOT_IN_BEGIN();

    (*gc->procs.finish)(gc);
    if (g_isNT4) {
	if (g_opengl32SwapBuffersSw == NULL) {
	    g_opengl32SwapBuffersSw = GetProcAddress(GetModuleHandleA("opengl32"), "wglSwapBuffers");
	    if (g_opengl32SwapBuffersSw == NULL) {
		return;
	    }
	}
	if (retAddr - (int)g_opengl32SwapBuffersSw < 0) {
	    dist = (int)g_opengl32SwapBuffersSw - retAddr;
	} else {
	    dist = retAddr - (int)g_opengl32SwapBuffersSw;
	}
	if (dist < 0x100) {
	    wglRcSwapBuffers(gc);
	}
    }
}

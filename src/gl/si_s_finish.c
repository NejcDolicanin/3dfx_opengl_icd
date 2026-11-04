/*
** si_s_finish.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_finish.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 690c0bb0 -- __glim_Flush */
void __stdcall __glim_Flush(void)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_FLUSH();

    (*gc->procs.flush)(gc);
}

/*
** si_dl_opt.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/dl_opt.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 6907f920 -- __glle_InvalidValue */
const GLubyte * __fastcall __glle_InvalidValue(const GLubyte *PC)
{
    __glSetError(GL_INVALID_VALUE);
    return PC;
}

/* 6907f940 -- __glle_InvalidEnum */
const GLubyte * __fastcall __glle_InvalidEnum(const GLubyte *PC)
{
    __glSetError(GL_INVALID_ENUM);
    return PC;
}

/* 6907f960 -- __glle_InvalidOperation */
const GLubyte * __fastcall __glle_InvalidOperation(const GLubyte *PC)
{
    __glSetError(GL_INVALID_OPERATION);
    return PC;
}

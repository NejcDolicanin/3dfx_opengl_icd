/*
** si_s_prim.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_prim.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 690b4510 -- __glNop0 */
void __fastcall __glNop0(void) {}

/* 6907d8f0 -- __glim_Begin */
void __stdcall __glim_Begin(GLenum mode)
{
    __GL_SETUP();
    GLuint beginMode;

    beginMode = __gl_beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
        if (beginMode == __GL_NEED_VALIDATE) {
            (*gc->procs.validate)(gc);
            __gl_beginMode = __GL_NOT_IN_BEGIN;
            glBegin(mode);
            return;
        } else {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
    }

    if ((GLuint)mode > GL_POLYGON) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    __gl_beginMode = __GL_IN_BEGIN;
    __GL_API_BGN_RENDER();

    (*gc->procs.beginPrim[mode])(gc);
}

/* 6907da40 -- __glNop4 */
void __fastcall __glNop4(GLint a1, GLint a2, GLint a3, GLint a4) {}

/* 6907da50 -- __glNop5 */
void __fastcall __glNop5(GLint a1, GLint a2, GLint a3, GLint a4, GLint a5) {}

/* 6907da60 -- __glNop6 */
void __fastcall __glNop6(GLint a1, GLint a2, GLint a3, GLint a4, GLint a5, GLint a6) {}

/* 6907da70 -- __glGLAPINop1 */
void __stdcall __glGLAPINop1(GLint a) {}

/* 6907dab0 -- __glim_UnimplementedExtension */
void __fastcall __glim_UnimplementedExtension(GLuint ecx_unused, GLuint edx_unused) 
{
    __glSetError(GL_INVALID_OPERATION);
    return;
}

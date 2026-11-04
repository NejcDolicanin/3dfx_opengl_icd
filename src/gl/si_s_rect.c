/*
** si_s_rect.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_rect.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690976a0 -- __glRect */
void __fastcall __glRect(__GLcontext *gc, __GLfloat x0, __GLfloat y0, __GLfloat x1, __GLfloat y1)
{
    glBegin(GL_QUADS);
    glVertex2f(x0, y0);
    glVertex2f(x1, y0);
    glVertex2f(x1, y1);
    glVertex2f(x0, y1);
    glEnd();
}

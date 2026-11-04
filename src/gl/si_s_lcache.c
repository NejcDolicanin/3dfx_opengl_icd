/*
** si_s_lcache.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_lcache.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690dd5f0 -- __glFreeSpecLUT */
void __fastcall __glFreeSpecLUT(__GLcontext *gc, __GLspecLUTEntry *lut)
{
    if (lut == NULL) return;

    assert(lut->refcount != 0);

    lut->refcount--;

    if (lut->refcount > 0) return; /* someone is referring to table entry */

    /* refcount == 0, no-one has a pointer to this table, not even a
    ** specular cache.
    **
    ** NOTE: I made the context pointer NULL to make it clear that
    ** the context being passed may not be the context that was
    ** current when the space was allocated. In fact the context
    ** may have been destroyed between the malloc and the free.
    ** Currently, the argument isn't used, but this should be fixed,
    ** or at least the intended use of this argument should be documented.
    */
    ((void (__fastcall *)(__GLcontext *, void *))gc->imports[3])((__GLcontext *)NULL, lut); 
}

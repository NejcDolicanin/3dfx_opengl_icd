/*
** gl_dlopt.c -- dl_mopt.c: __glDlistOptimizeMaterial, which folds runs of display-list
** Material() ops into one packed FastMaterial op.  As in the SI, except that the ICD's
** __glDlistFreeOp is an empty function (690b4510: the ops live in the list's arena).
*/
#include <string.h>
#include "wglpriv.h"
#include "icdprotos.h"
#include "glsi.h"
#include "si_names.h"

/* dlistopt.h with the ICD's layout */
typedef __GLdlistOpICD __GLdlistOp;         /* glsi.h */

struct __gllc_Materialfv_Rec {
    GLenum      face;
    GLenum      pname;
};

#include "glsitypes.h"                      /* __GLmatChange */

/* the ICD's __glDlistFreeOp: an empty function */
#define __glDlistFreeOp(gc, op) \
    ((void (__fastcall *)(__GLcontext *, __GLdlistOp *)) __glNop0)(gc, op)

/* 690dfa10 -- __glDlistOptimizeMaterial */
void __fastcall __glDlistOptimizeMaterial(__GLcontext *gc, __GLcompiledDlist *cdlist)
{
    __GLdlistOp **first;
    __GLdlistOp *start, *next;
    __GLdlistOp *dlop;
    __GLmatChange front, back;
    __GLmatChange *f[2], *mc;
    GLint fcount, pcount = 0;      /* fcount uninitialised in the ICD */
    __GLfloat fparams[4];
    /* Large enough to describe all possible material changes */
    __GLfloat matChanges[100];
    struct __gllc_Materialfv_Rec *args;
    GLenum face, pname;
    GLvoid *params;
    GLint i, j;
    GLint dirtyBits;
    GLboolean needsFreeing;
    __GLspecLUTEntry *lutEntry;

    if ((cdlist->genericFlags & __GL_DLFLAG_HAS_MATERIAL) == 0) return;

    front.dirtyBits = 0;
    back.dirtyBits = 0;
    front.faceName = GL_FRONT;
    back.faceName = GL_BACK;
    first = &cdlist->dlist;
    while (*first) {
	if ((*first)->opcode == __glop_Materialfv ||
		(*first)->opcode == __glop_Materialiv) {

	    /* Find chain of consecutive Material() commands. */
	    start = *first;
	    for (next = start->next; next; next = next->next) {
		if (next->opcode != __glop_Materialfv &&
			next->opcode != __glop_Materialiv) {
		    /* End of chain of material commands */
		    break;
		}
	    }

	    /*
	    ** Ok, these material ops (from start to next) have already been
	    ** error checked.  We now attempt to compress them into a single
	    ** material command.
	    */

	    for (dlop = start; dlop != next; dlop = dlop->next) {
		args = (struct __gllc_Materialfv_Rec *) dlop->data;
		face = args->face;
		pname = args->pname;
		params = (GLvoid *) (args+1);

		switch(face) {
		  case GL_FRONT:
		    f[0] = &front;
		    fcount = 1;
		    break;
		  case GL_BACK:
		    f[0] = &back;
		    fcount = 1;
		    break;
		  case GL_FRONT_AND_BACK:
		    f[0] = &front;
		    f[1] = &back;
		    fcount = 2;
		    break;
		}

		pcount = __glMaterialfv_size(pname);

		for (j = 0; j < pcount; j++) {
		    if (dlop->opcode == __glop_Materialiv) {
			fparams[j] = (__GLfloat) ((GLint *) params)[j];
		    } else {
			fparams[j] = ((GLfloat *) params)[j];
		    }
		}

		for (i = 0; i < fcount; i++) {
		    ApplyMatChange(gc, f[i], pname, fparams);
		}
	    }

	    /*
	    ** Ok, we have saved our changes to our chalkboard "front" and
	    ** "back".  We now build an op designed to read this information,
	    ** and replace the string from start to next with it.
	    */
	    f[0] = &front;
	    f[1] = &back;
	    fcount = 2;

	    j = 0;
	    needsFreeing = GL_FALSE;
	    for (i = 0; i < fcount; i++) {
		mc = f[i];

		if (mc->dirtyBits) {
		    dirtyBits = mc->dirtyBits;

		    *((GLenum *) (&matChanges[j++])) = mc->faceName;
		    *((GLint *) (&matChanges[j++])) = dirtyBits;

		    if (dirtyBits & __GL_MATERIAL_AMBIENT) {
			matChanges[j++] = mc->ambient.r;
			matChanges[j++] = mc->ambient.g;
			matChanges[j++] = mc->ambient.b;
			matChanges[j++] = mc->ambient.a;
		    }
		    if (dirtyBits & __GL_MATERIAL_DIFFUSE) {
			matChanges[j++] = mc->diffuse.r;
			matChanges[j++] = mc->diffuse.g;
			matChanges[j++] = mc->diffuse.b;
			matChanges[j++] = mc->diffuse.a;
		    }
		    if (dirtyBits & __GL_MATERIAL_SPECULAR) {
			matChanges[j++] = mc->specular.r;
			matChanges[j++] = mc->specular.g;
			matChanges[j++] = mc->specular.b;
			matChanges[j++] = mc->specular.a;
		    }
		    if (dirtyBits & __GL_MATERIAL_EMISSIVE) {
			matChanges[j++] = mc->emissive.r;
			matChanges[j++] = mc->emissive.g;
			matChanges[j++] = mc->emissive.b;
			matChanges[j++] = mc->emissive.a;
		    }
		    if (dirtyBits & __GL_MATERIAL_SHININESS) {
			lutEntry = __glCreateSpecLUT(gc, mc->shininess);
			matChanges[j++] = mc->shininess;
			*((__GLspecLUTEntry **) (&matChanges[j++])) = lutEntry;
			needsFreeing = GL_TRUE;
		    }
		    if (dirtyBits & __GL_MATERIAL_COLORINDEXES) {
			matChanges[j++] = mc->cmapa;
			matChanges[j++] = mc->cmaps;
			matChanges[j++] = mc->cmapd;
		    }
		}
	    }
	    matChanges[j++] = 0.0f;

	    dlop = __glDlistAllocOp2(gc, j * (unsigned int)sizeof(__GLfloat));
	    if (dlop == NULL) return;
	    if (needsFreeing) {
		dlop->dlistFree = FreeFastMaterial;
	    }
	    dlop->opcode = __glop_FastMaterial;
	    *first = dlop;
	    dlop->next = next;
	    __GL_MEMCOPY(dlop->data, matChanges, j * sizeof(__GLfloat));
	    first = &(dlop->next);

	    /* Free chain from start to next */
	    dlop = start;
	    while (dlop != next) {
		start = dlop;
		dlop = dlop->next;
		__glDlistFreeOp(gc, start);
	    }

	    /* reset "dirtyness" of faces */
	    front.dirtyBits = 0;
	    back.dirtyBits = 0;

	} else {
	    first = &((*first)->next);
	}
    }
    return;
}

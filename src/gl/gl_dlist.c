/*
** gl_dlist.c -- dl_list.c / s_dlist.c / dl_table.c / dl_init.c / dl_opt.c: display-list objects, glNewList /
** glEndList / glCallList(s) (SI ports: si_dl_*.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "siprotos.h"
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "icdglob.h"
#include "icdprotos.h"
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <string.h>

/* ---- dl_*.c: glBegin compiled into a display list ---------------------------------------------
** Each primitive mode gets its own zero-size op (opcodes 0x3e8.. in the order below) whose
** execute proc calls Begin(mode); any other mode records op 0x3f3. */
typedef void (__fastcall *__GLdlFree)(__GLcontext *gc, void *mem);
/* ---- display list execution: each op is [proc][data...]; proc(data) returns the next op ------ */
typedef GLubyte *(__fastcall *__GLdlistOpProc)(GLubyte *data);
/* ---- dlist.c: allocation of display list blocks ------------------------------------------------ */
typedef void *(__fastcall *__GLdlistMalloc)(__GLcontext *gc, GLuint size);
/************************************************************************/
/* dl_init.c */
#define __gl_dispatch   (__glCurrentGC()->currentDispatchState)
/* dlist.h __GLcompiledDlist: glctx.h */
typedef void (__fastcall *__GLdlistOptimizer)(__GLcontext *gc, __GLcompiledDlist *compDlist);
typedef __GLdlistICD *(__fastcall *__GLdlistCompiler)(__GLcontext *gc, __GLcompiledDlist *compDlist);
typedef void (__fastcall *__GLfreeAll)(void *arena);
/* dl_list.c: the CallLists element sizes (the original's copy lives in .text at 690062e8) */
static const GLubyte __GLdlsize_tab[] = {
      /* GL_BYTE		*/	1,
      /* GL_UNSIGNED_BYTE	*/	1,
      /* GL_SHORT		*/	2,
      /* GL_UNSIGNED_SHORT	*/	2,
      /* GL_INT			*/	4,
      /* GL_UNSIGNED_INT	*/	4,
      /* GL_FLOAT		*/	4,
      /* GL_2_BYTES		*/	2,
      /* GL_3_BYTES		*/	3,
      /* GL_4_BYTES		*/	4,
};
#define __glCallListsSize(type)				\
	((type) >= GL_BYTE && (type) <= GL_4_BYTES ?	\
	__GLdlsize_tab[(type)-GL_BYTE] : -1)
struct __gllc_CallLists_Rec {
    GLsizei n;
    GLenum type;
};
/* __glCallListsSizeFolded: the same lookup with the GL_BYTE bias folded into the displacement, as the original's
** `mov cl,[eax+0x69004ee8]` (= 690062e8 - GL_BYTE). */
#define __glCallListsSizeFolded(type)				\
	((type) >= GL_BYTE && (type) <= GL_4_BYTES ?	\
	__GLdlsize_tab[(type)-GL_BYTE] : -1)
#define __GL_PAD_SHR(x) ((((x) + 3) >> 2) << 2)    /* __GL_PAD in the shift form */
/************************************************************************/
/* dl_list.c */
#define MAX_LISTS_CACHE 256
#define __GL_MAX_LIST_NESTING 64
/* the glDrawArraysEXT stub (69008a10) is generated without parameters */
#define glDrawArraysEXTProc ((void (APIENTRY *)(GLenum, GLint, GLsizei))glDrawArraysEXT)
/* ---- shared-object release: drop a reference, free on the last one ---- */
typedef void (__fastcall *PFNRELEASE)(__GLcontext *gc, GLint *obj);
extern void __fastcall __glGenericFreeDlist(__GLcontext *gc, __GLdlistICD *dlist);
extern __GLdlistICD *__fastcall __glNopAllocDlist(__GLcontext *gc, GLuint size);
extern const GLubyte * __fastcall __glle_CallLists(const GLubyte *PC);

/* 6907f560 -- __gllc_Begin */
void __stdcall __gllc_Begin(GLenum mode)
{
    __GLdlistOpICD *dlop;
    GLushort opcode;
    __GLlistExecFunc execute;
    __GL_SETUP();

    dlop = __glDlistAllocOp2(gc, 0);
    if (dlop == NULL) return;
    switch (mode) {
      case GL_LINE_LOOP:
        opcode = 0x3e8; execute = __glle_Begin_LineLoop;
        break;
      case GL_LINE_STRIP:
        opcode = 0x3e9; execute = __glle_Begin_LineStrip;
        break;
      case GL_LINES:
        opcode = 0x3ea; execute = __glle_Begin_Lines;
        break;
      case GL_POINTS:
        opcode = 0x3eb; execute = __glle_Begin_Points;
        break;
      case GL_POLYGON:
        opcode = 0x3ec; execute = __glle_Begin_Polygon;
        break;
      case GL_TRIANGLE_STRIP:
        opcode = 0x3ed; execute = __glle_Begin_TriangleStrip;
        break;
      case GL_TRIANGLE_FAN:
        opcode = 0x3ee; execute = __glle_Begin_TriangleFan;
        break;
      case GL_TRIANGLES:
        opcode = 0x3ef; execute = __glle_Begin_Triangles;
        break;
      case GL_QUAD_STRIP:
        opcode = 0x3f0; execute = __glle_Begin_QuadStrip;
        break;
      case GL_QUADS:
        opcode = 0x3f1; execute = __glle_Begin_Quads;
        break;
      default:
        dlop->opcode = __glop_InvalidEnum;
        __glDlistAppendOp(gc, dlop, __glle_InvalidEnum);
        return;
    }
    gc->dlist.listData.genericFlags |= __GL_DLFLAG_HAS_BEGIN;
    dlop->opcode = opcode;
    __glDlistAppendOp(gc, dlop, execute);
}

/* 6907f9c0 -- __glGenericFreeDlist (s_dlist.c); freeRec is not initialised in the ICD */
void __fastcall __glGenericFreeDlist(__GLcontext *gc, __GLdlistICD *dlist)
{
    GLuint freeCount;
    GLboolean freeSegment = GL_FALSE;
    __GLDlistFreeFns *freeRec;

    freeCount = __GL_DLIST_FREECOUNT(dlist);

    if(freeCount) { /* if non-zero, segment isn't empty */
	freeSegment = GL_TRUE;
	freeCount--;
    }

    if(freeCount)
	freeRec = __GL_DLIST_FREEFNS(dlist);

    /* call any free functions */

    while(freeCount--) {
	(*freeRec->freeFn)(gc, freeRec->data);
	freeRec++;
    }

    if(freeSegment) {
	(*(__GLdlFree)gc->dlist.free)(gc, dlist->segment); /* free the segment */
    }

    (*(__GLdlFree)gc->dlist.free)(gc, dlist);
}

/* 6907fa40 -- __glDoInterpret (3dfx S_DLIST.C): run the ops up to the sentinel */
void __fastcall __glDoInterpret(GLubyte *PC)
{
    while (*(__GLdlistOpProc *)PC != (__GLdlistOpProc)__glle_Sentinel) {
	PC = (**(__GLdlistOpProc *)PC)(PC + 4);
    }
}

/* 6907fa70 -- __glGenericAllocDlist: the header with room for freeCount free functions (one more
** for the segment when there is one) */
__GLdlistICD *__fastcall __glGenericAllocDlist(__GLcontext *gc, GLuint size, GLint freeCount)
{
    __GLdlistICD *dlist;

    dlist = (*(__GLdlistMalloc)gc->dlist.malloc)(gc, freeCount * sizeof(__GLDlistFreeFns) + sizeof(__GLdlistICD));
    if (dlist == NULL) {
	return NULL;
    }
    dlist->refcount = 1;
    dlist->free = __glGenericFreeDlist;
    if (size) {
	dlist->execute = __glGenericExecuteDlist;
    } else {
	dlist->execute = (void (__fastcall *)(__GLcontext *, __GLdlistICD *))__glNop_2;
    }
    dlist->segment = (GLubyte *)(*(__GLdlistMalloc)gc->dlist.malloc)(gc, size);
    if (size) {
	freeCount++;
    }
    __GL_DLIST_FREECOUNT(dlist) = freeCount;
    return dlist;
}

/* 6907fce0 -- __glNopAllocDlist (s_dlist.c): a machine dlist: the header, then size bytes in local */
__GLdlistICD *__fastcall __glNopAllocDlist(__GLcontext *gc, GLuint size)
{
    __GLdlistICD *dlist;

    dlist = (*(__GLdlistMalloc)gc->dlist.malloc)(gc, offsetof(__GLdlistICD, local) + size);
    if (dlist == NULL) {
	return NULL;
    }
    dlist->refcount = 1;
    dlist->free = __glNopFreeDlist;
    dlist->execute = __glNopExecuteDlist;
    dlist->segment = NULL;
    return dlist;
}

/* 6907fd10 -- __glNopDlistCompiler (s_dlist.c): allocate the list's first block; out of memory: drop the list
** being compiled */
__GLdlistICD * __fastcall __glNopDlistCompiler(__GLcontext *gc, __GLcompiledDlist *input_list)
{
    __GLdlistICD *dlist = __glNopAllocDlist(gc, 0);
    if (!dlist) {
        (*gc->procs.freeAll)(gc->dlist.arena);
        input_list->dlist = NULL;
        input_list->lastDlist = NULL;
        gc->dlist.currentList = 0;
        __glSetError(GL_OUT_OF_MEMORY);
    }
    return dlist;
}

/* 690a0e60 -- __glInitDlistState */
void __fastcall __glInitDlistState(__GLcontext *gc)
{
    __GLdlistMachine *dlist;

    dlist = &gc->dlist;

    dlist->nesting = 0;
    dlist->currentList = 0;

    dlist->malloc = (void *)gc->imports[0];
    dlist->realloc = (void *)gc->imports[2];
    dlist->free = (void *)gc->imports[3];

    if (dlist->dlistArray == NULL) {
        dlist->dlistArray = __glDlistNewArray(gc);
    }
    if (dlist->namesArray == NULL) {
        dlist->namesArray = __glNamesNewArray(gc, __GL_NAMES_DLIST);
    }
}

/* 690a0ed0 -- __glFreeDlistState */
void __fastcall __glFreeDlistState(__GLcontext *gc)
{
    gc->dlist.namesArray->refcount--;

    if (gc->dlist.namesArray->refcount == 0) {
        __glNamesFreeArray(gc, gc->dlist.namesArray);
    }
    gc->dlist.namesArray = NULL;

    gc->dlist.dlistArray->refcount--;

    if (gc->dlist.dlistArray->refcount == 0) {
        __glDlistFreeArray(gc, gc->dlist.dlistArray);
    }
    gc->dlist.dlistArray = NULL;

    if (gc->dlist.arena != NULL) {
        (*(void (__fastcall *)(void *))gc->procs.deleteArena)(gc->dlist.arena);
    }
}

/* 690b3ac0 -- __glim_NewList: glNewList (dl_list.c __glim_NewList, without NEW_LISTS) */
void __stdcall __glim_NewList(GLuint list, GLenum mode)
{
    __GLdlistMachine *dlstate;
    __GLdispatchTable *globalDispatch;
    __GL_SETUP_NOT_IN_BEGIN();

    dlstate = &gc->dlist;

    /* Valid mode? */
    switch(mode) {
      case GL_COMPILE:
      case GL_COMPILE_AND_EXECUTE:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    if (dlstate->currentList) {
	/* Must call EndList before calling NewList again! */
	__glSetError(GL_INVALID_OPERATION);
	return;
    }

    if (list == 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    if (dlstate->listExec && dlstate->arena == NULL) {
	if (!(dlstate->arena = (void *)(*gc->procs.newArena)(gc))) {
	    __glSetError(GL_OUT_OF_MEMORY);
	    return;
	}
    }

    globalDispatch = &__glCurrentGC()->currentDispatchState;
    __glCopyDispatchTable(globalDispatch, &gc->savedDispatchState);
    __glCopyDispatchTable(&gc->listCompState, globalDispatch);
    gc->dispatchState = &gc->savedDispatchState;

    dlstate->currentList = list;
    dlstate->mode = mode;
    dlstate->listData.genericFlags = 0;
    dlstate->listData.machineFlags = 0;
    dlstate->listData.dlist = 0;
    dlstate->listData.lastDlist = 0;

    (*(void (__fastcall *)(__GLcontext *)) dlstate->initState)(gc);
}

/* 690b3c40 -- __glim_EndList: glEndList */
void __fastcall __glim_EndList(void)
{
    __GLdlistMachine *dlstate;
    __GLdlistICD *dlist;
    __GLcompiledDlist *compDlist;
    __GL_SETUP_NOT_IN_BEGIN();

    dlstate = &gc->dlist;

    /* Must call NewList() first! */
    if (dlstate->currentList == 0) {
	__glSetError(GL_INVALID_OPERATION);
	return;
    }

    /*
    ** First we optimize the display list by invoking the machine specific
    ** optimizer.
    */
    compDlist = (__GLcompiledDlist *)&dlstate->listData;
    (*(__GLdlistOptimizer)dlstate->optimizer)(gc, compDlist);

    dlist = (*(__GLdlistCompiler)dlstate->compiler)(gc, compDlist);

    /* free linked list memory */

    if (dlstate->listExec) /* XXXblythe */
	(*(__GLfreeAll)gc->procs.freeAll)(dlstate->arena);
    compDlist->dlist = NULL;
    compDlist->lastDlist = NULL;

    /*
    ** Now we connect the new, compiled dlist to the display list tree
    */
    if (!__glNamesNewData(gc, gc->dlist.namesArray,
				dlstate->currentList, dlist)) {
	/*
	** No memory!
	** Nuke the list!
	*/
	if (dlist) {
	  (*dlist->free)(gc,dlist);
	}
    }

    /* Switch back to saved dispatch state (compile mode -> immediate mode */
    __glCopyDispatchTable(&gc->savedDispatchState, &__gl_dispatch);
    gc->dispatchState = &__gl_dispatch;

    dlstate->currentList = 0;
}

/* 690b3e10 -- __glce_CallList (3dfx G_LEXEC.C): GL_COMPILE_AND_EXECUTE, compiled and executed through the
** dispatch tables (as 690b43e0); unreferenced */
void __fastcall __glce_CallList(GLuint list)
{
    __GLcontext *gc;
    __GLdispatchTable save;

    save = __glCurrentGC()->currentDispatchState;
    gc = __glCurrentGC();
    __glCopyDispatchTable(&gc->listCompState, &__glCurrentGC()->currentDispatchState);
    glCallList(list);
    __glCopyDispatchTable(&gc->savedDispatchState, &__glCurrentGC()->currentDispatchState);
    gc->dispatchState = &__glCurrentGC()->currentDispatchState;
    glCallList(list);
    __glCopyDispatchTable(&__glCurrentGC()->currentDispatchState, &gc->savedDispatchState);
    gc->dispatchState = &gc->savedDispatchState;
    __glCopyDispatchTable(&save, &__glCurrentGC()->currentDispatchState);
}

/* 690b3f00 -- __glce_CallLists */
void __fastcall __glce_CallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    __GLdispatchTable compExecState = __gl_dispatch;
    __GL_SETUP();

    /* compile... */
    __glCopyDispatchTable(&gc->listCompState, &__gl_dispatch);
    glCallLists(n, type, lists);
    /* and execute... */
    __glCopyDispatchTable(&gc->savedDispatchState, &__gl_dispatch);
    gc->dispatchState = &__gl_dispatch;
    glCallLists(n, type, lists);
    __glCopyDispatchTable(&__gl_dispatch, &gc->savedDispatchState);
    gc->dispatchState = &gc->savedDispatchState;
    /* and go back to compiling and executing */
    __glCopyDispatchTable(&compExecState, &__gl_dispatch);
}

/* 690b4040 -- __gllc_CallLists */
void __stdcall __gllc_CallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    __GLdlistOpICD *dlop;
    GLuint size;
    GLint arraySize;
    struct __gllc_CallLists_Rec *data;
    __GL_SETUP();

    if (n < 0) {
	__gllc_InvalidValue(gc);
	return;
    }
    arraySize = n*__glCallListsSize(type);
    if (arraySize < 0) {
	__gllc_InvalidEnum(gc);
	return;
    }
    size = (GLuint)sizeof(struct __gllc_CallLists_Rec) + __GL_PAD(arraySize);
    dlop = __glDlistAllocOp2(gc, size);
    if (dlop == NULL) return;
    dlop->opcode = __glop_CallLists;
    data = (struct __gllc_CallLists_Rec *) dlop->data;
    data->n = n;
    data->type = type;
    size = (GLuint)sizeof(struct __gllc_CallLists_Rec);
    memcpy(dlop->data + size, lists, arraySize);
    __glDlistAppendOp(gc, dlop, __glle_CallLists);
}

extern void __fastcall DoCallLists(GLsizei n, GLenum type, const GLvoid *lists);
/* 690b40f0 -- __glle_CallLists */
const GLubyte * __fastcall __glle_CallLists(const GLubyte *PC)
{
    GLuint size;
    GLuint arraySize;
    const struct __gllc_CallLists_Rec *data;

    data = (const struct __gllc_CallLists_Rec *) PC;
    (void) DoCallLists(data->n, data->type, (const GLvoid *) (data+1));
    arraySize = __GL_PAD_SHR(data->n*__glCallListsSizeFolded(data->type));
    size = (GLuint)sizeof(struct __gllc_CallLists_Rec) + arraySize;
    return PC + size;
}

/* 690b4150 -- DoCallLists: execute n display lists named by lists (of type, offset by listBase), looked up and
** locked MAX_LISTS_CACHE names at a time; past __GL_MAX_LIST_NESTING it sets dlist.nesting to twice the limit
** (so the outer lists unwind) and returns, else nesting is counted up around the calls */
void __fastcall DoCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    __GLdlistICD *dlists[MAX_LISTS_CACHE];
    __GLdlistICD *dlist;
    __GLdlistMachine *dlstate;
    GLint i, dlcount, datasize;
    const GLubyte *listiter;
    __GL_SETUP();

    dlstate = &gc->dlist;

    datasize = __glCallListsSizeFolded(type);

    if (dlstate->nesting >= __GL_MAX_LIST_NESTING) {
	/* Force unwinding of the display list */
	dlstate->nesting = __GL_MAX_LIST_NESTING*2;
	return;
    }
    dlstate->nesting++;

    i=0;
    listiter = (const GLubyte *) lists;
    while (n) {
	dlcount = n;
	if (dlcount > MAX_LISTS_CACHE) dlcount = MAX_LISTS_CACHE;

	__glNamesLockDataList(gc, gc->dlist.namesArray, dlcount, type,
		gc->state.list.listBase,
		(const GLvoid *) listiter, (void **)dlists);

	i = 0;
	while (i < dlcount) {
	    dlist = dlists[i];
	    (*dlist->execute)(gc, dlist);
	    i++;
	  }

	__glNamesUnlockDataList(gc, dlcount, (void **)dlists);

	listiter += dlcount * datasize;
	n -= dlcount;
    }

    dlstate->nesting--;
}

/* 690b4260 -- __glim_CallLists */
void __stdcall __glim_CallLists(GLsizei n, GLenum type,
			     const GLvoid *lists)
{
    __GL_SETUP();

    if (n < 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    if (__glCallListsSize(type) == -1) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    DoCallLists(n, type, lists);
    gc->dlist.nesting = 0;
}

/* 690b42e0 -- __glce_DrawArraysEXT */
void __fastcall __glce_DrawArraysEXT(GLenum mode, GLint first, GLsizei count)
{
    __GLdispatchTable compExecState = __gl_dispatch;
    __GL_SETUP();

    /* compile... */
    __glCopyDispatchTable(&gc->listCompState, &__gl_dispatch);
    glDrawArraysEXTProc(mode, first, count);
    /* and execute... */
    __glCopyDispatchTable(&gc->savedDispatchState, &__gl_dispatch);
    gc->dispatchState = &__gl_dispatch;
    glDrawArraysEXTProc(mode, first, count);
    __glCopyDispatchTable(&__gl_dispatch, &gc->savedDispatchState);
    gc->dispatchState = &gc->savedDispatchState;
    /* and go back to compiling and executing */
    __glCopyDispatchTable(&compExecState, &__gl_dispatch);
}

/* 690b4520 -- __glDlistAppendOp (COMPILE_AND_EXECUTE runs the op through the execute table) */
void __fastcall __glDlistAppendOp(__GLcontext *gc, __GLdlistOpICD *dlop, __GLlistExecFunc func)
{
    (*(void (__fastcall *)(__GLcontext *, __GLdlistOpICD *)) gc->dlist.checkOp)(gc, dlop);
    if (gc->dlist.mode == GL_COMPILE_AND_EXECUTE) {
	__glCopyDispatchTable(&gc->savedDispatchState, &__glCurrentGC()->currentDispatchState);
	gc->dispatchState = &__glCurrentGC()->currentDispatchState;
	(*func)(dlop->data);
	__glCopyDispatchTable(&__glCurrentGC()->currentDispatchState, &gc->savedDispatchState);
	__glCopyDispatchTable(&gc->listCompState, &__glCurrentGC()->currentDispatchState);
	gc->dispatchState = &gc->savedDispatchState;
    }
    if (gc->dlist.listData.lastDlist) {
	gc->dlist.listData.lastDlist->next = dlop;
	gc->dlist.listData.lastDlist = dlop;
    } else {
	gc->dlist.listData.dlist = dlop;
	gc->dlist.listData.lastDlist = dlop;
    }
}

/* 690bf3a0 -- __glDisposeDlist (s_names.c namesTypeInfo[DLIST].free): drop a reference; the dlist's free proc
** (+4) on the last one */
void __fastcall __glDisposeDlist(__GLcontext *gc, GLint *obj)
{
    if (--obj[0] == 0) (*(PFNRELEASE)obj[1])(gc, obj);
}

/* 690bf430 -- __glim_IsList: glIsList (shape of wglSwapIntervalEXT 69013760) */
GLboolean __stdcall __glim_IsList(GLuint list)
{
    __GLcontext *gc;

    gc = __glCurrentGC();
    if (*(int *)&__glCurrentGC()->beginMode == 1) {
	__glSetError(GL_INVALID_OPERATION);
	return GL_FALSE;
    }
    return __glNamesIsName(gc, (__GLnamesArray *) gc->dlist.namesArray, list);
}

/* 690bf480 -- __glim_GenLists: glGenLists */
GLuint __stdcall __glim_GenLists(GLsizei range)
{
    __GLnamesArray *array;
    __GL_SETUP_NOT_IN_BEGIN2();

    array = (__GLnamesArray *) gc->dlist.namesArray;

    if (range < 0) {
	__glSetError(GL_INVALID_VALUE);
	return 0;
    }
    if (range == 0) {
	return 0;
    }
    return __glNamesGenRange(gc, array, range);
}

/* 690dfd70 -- ApplyMatChange (dl_mopt.c): fold one material parameter (pname, pv) into the packed material change
** f being built for a display list -- copy the values raw and OR in its __GL_MATERIAL_* dirty bit(s); unknown
** pnames are ignored */
void __fastcall ApplyMatChange(__GLcontext *gc, __GLmatChange *f, GLenum pname, __GLfloat *pv)
{
    switch(pname) {
      case GL_COLOR_INDEXES:
	f->cmapa = pv[0];
	f->cmapd = pv[1];
	f->cmaps = pv[2];
	f->dirtyBits |= __GL_MATERIAL_COLORINDEXES;
	break;
      case GL_EMISSION:
	f->emissive.r = pv[0];
	f->emissive.g = pv[1];
	f->emissive.b = pv[2];
	f->emissive.a = pv[3];
	f->dirtyBits |= __GL_MATERIAL_EMISSIVE;
	break;
      case GL_SPECULAR:
	f->specular.r = pv[0];
	f->specular.g = pv[1];
	f->specular.b = pv[2];
	f->specular.a = pv[3];
	f->dirtyBits |= __GL_MATERIAL_SPECULAR;
	break;
      case GL_SHININESS:
	f->shininess = pv[0];
	f->dirtyBits |= __GL_MATERIAL_SHININESS;
	break;
      case GL_AMBIENT:
	f->ambient.r = pv[0];
	f->ambient.g = pv[1];
	f->ambient.b = pv[2];
	f->ambient.a = pv[3];
	f->dirtyBits |= __GL_MATERIAL_AMBIENT;
	break;
      case GL_DIFFUSE:
	f->diffuse.r = pv[0];
	f->diffuse.g = pv[1];
	f->diffuse.b = pv[2];
	f->diffuse.a = pv[3];
	f->dirtyBits |= __GL_MATERIAL_DIFFUSE;
	break;
      case GL_AMBIENT_AND_DIFFUSE:
	f->ambient.r = pv[0];
	f->ambient.g = pv[1];
	f->ambient.b = pv[2];
	f->ambient.a = pv[3];
	f->diffuse = f->ambient;
	f->dirtyBits |= __GL_MATERIAL_AMBIENT | __GL_MATERIAL_DIFFUSE;
	break;
    }
}

/* ---- dl_mopt.c: FreeFastMaterial (690dfeb0), the dlistFree of the packed FastMaterial op ---- */
/* 690dfeb0 -- FreeFastMaterial (dl_mopt.c): the dlistFree of the packed FastMaterial op */
void __fastcall FreeFastMaterial(__GLcontext *gc, GLubyte *PC)
{
    __GLfloat *ptr;
    GLint dirtyBits;
    __GLspecLUTEntry *lutEntry;

    ptr = (__GLfloat *) PC;
    while ((*(GLenum *) (ptr++)) != 0) {
	dirtyBits = (*(GLint *) (ptr++));
	if (dirtyBits & __GL_MATERIAL_AMBIENT) ptr += 4;
	if (dirtyBits & __GL_MATERIAL_DIFFUSE) ptr += 4;
	if (dirtyBits & __GL_MATERIAL_SPECULAR) ptr += 4;
	if (dirtyBits & __GL_MATERIAL_EMISSIVE) ptr += 4;
	if (dirtyBits & __GL_MATERIAL_SHININESS) {
	    ptr++;
	    lutEntry = (*(__GLspecLUTEntry **) (ptr++));
	    __glFreeSpecLUT(gc, lutEntry);
	}
	if (dirtyBits & __GL_MATERIAL_COLORINDEXES) ptr += 3;
    }
}

/* ---- s_dlist.c: __glGenericDlistCompiler ---- */
typedef struct {
    void       *freeFn;
    GLubyte    *data;
} __GLDlistFreeFnsW3;
#define __GL_GENERIC_DLIST_OPCODE       1000
#define __GL_MACHINE_DLIST_OPCODE       10000
extern __GLdlistICD *__fastcall __glGenericAllocDlist(__GLcontext *gc, GLuint size, GLint freeCount);

/*
** 6907faf0 -- __glGenericDlistCompiler (s_dlist.c; gc->dlist compile hook): flatten the chain of
** compiled ops into one display list segment.  As in the SI.
*/
__GLdlistICD * __fastcall __glGenericDlistCompiler(__GLcontext *gc, __GLcompiledDlist *compDlist)
{
    __GLdlistOpICD *dlistop;
    GLubyte *data;
    void *fp;
    __GLDlistFreeFnsW3 *freeFnArray;
    GLuint totalSize;
    GLint freeCount;
    __GLdlistICD *dlist;

    /*
    ** Now we compress the chain of display list ops into an optimized dlist.
    */
    totalSize = 0;
    freeCount = 0;

    for (dlistop = compDlist->dlist; dlistop; dlistop = dlistop->next) {
        if (dlistop->aligned && ((totalSize & 7) != 4)) {
            totalSize += sizeof(void *);
        }
        totalSize += dlistop->size + sizeof(void *);
        if (dlistop->dlistFree) freeCount++;
    }
    /* space needed for sentinel function; __glle_Sentinel() */
    totalSize += sizeof(void *);

    dlistop = compDlist->dlist;

    if (dlistop) {
        dlist = __glGenericAllocDlist(gc, totalSize, freeCount);

        /* local memory used to hold free functions, if any */
        freeFnArray = (__GLDlistFreeFnsW3 *)((GLubyte *)(dlist->local + sizeof(GLuint)));

        if (dlist == NULL) {
            (*gc->procs.freeAll)(gc->dlist.arena);
            compDlist->dlist = NULL;
            compDlist->lastDlist = NULL;
            gc->dlist.currentList = 0;
            __glSetError(GL_OUT_OF_MEMORY);
            return NULL;
        }

        totalSize = 0;
        data = dlist->segment;
        for (; dlistop; dlistop = dlistop->next) {
            GLshort opcode;

            opcode = dlistop->opcode;

            if (dlistop->aligned && ((totalSize & 7) != 4)) {
                /* Need to stick a nop entry in here to doubleword align */
                *((void **) data) = (void *)__glle_Nop;
                data += sizeof(void **);
                totalSize += 4;
            }

            if (opcode < __GL_GENERIC_DLIST_OPCODE) {
                fp = gc->dlist.baseListExec[opcode];
            } else if (opcode < __GL_MACHINE_DLIST_OPCODE) {
                fp = gc->dlist.listExec[opcode - __GL_GENERIC_DLIST_OPCODE];
            } else {
                fp = gc->dlist.machineListExec[opcode - __GL_MACHINE_DLIST_OPCODE];
            }
            *((void **) data) = fp;
            data += sizeof(void **);
            if (dlistop->dlistFree) {
                freeFnArray->freeFn = dlistop->dlistFree;
                freeFnArray->data = data;
                freeFnArray++;
            }

            {   /* __GL_MEMCOPY(data, dlistop->data, dlistop->size); */
                GLuint *src, *dst;
                GLuint a,b,c,d;
                GLint copyWords, copyQuadWords;

                copyWords = (dlistop->size) >> 2;
                copyQuadWords = copyWords >> 2;
                src = (GLuint *) dlistop->data;
                dst = (GLuint *) data;
                while (--copyQuadWords >= 0) {
                    a = src[0];
                    b = src[1];
                    c = src[2];
                    d = src[3];
                    dst[0] = a;
                    dst[1] = b;
                    dst[2] = c;
                    dst[3] = d;
                    src += 4;
                    dst += 4;
                }
                copyWords &= 3;
                while (--copyWords >= 0) {
                    *dst++ = *src++;
                }
            }

            data += dlistop->size;

            totalSize += dlistop->size + sizeof(void *);
        }
        /* append sentinel function to end of list */
        *((void **) data) = (void *)__glle_Sentinel;
    } else /* this is an empty display list */
        dlist = __glGenericAllocDlist(gc, 0, 0); /* create empty dlist */

    return dlist;
}

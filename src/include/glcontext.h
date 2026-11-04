/*
** glcontext.h -- the GL context object as laid out in 3dfxOGL.dll 1.0.0.0761.
**
** The real structure is in glctx.h; the code uses its fields directly.  What is left here: the proc types of
** the imports / exports slots (GLuint arrays in glctx.h) and the GC_imports_ / GC_exports_ accessors for them.
**
** Allocation: 0x38527 bytes, 8-byte aligned, by wglDevCreateContext [690124b5]
** (the "null" gc made by __glCreateContext is only the 0x35150-byte core part).
*/
#ifndef __GLCONTEXT_H__
#define __GLCONTEXT_H__

#include <windows.h>
#include <GL/gl.h>
#include <math.h>

#include "glctx.h"            /* the real __GLcontext */

/* 6907de00 __glTestOwnership (gl_buffer.c): is window pixel (x, y) inside the drawable's ownership region? */
GLboolean __fastcall __glTestOwnership(__GLcontext *gc, GLint x, GLint y);

/* the proc types the imports / exports slots are read as */
typedef int       (__fastcall *GCfn_any)();          /* unspecified-args call through a gc slot */
typedef GLboolean (__fastcall *GCfn_bool_gc)(__GLcontext *);
typedef GLboolean (__fastcall *GCfn_bool_gc_gc)(__GLcontext *, __GLcontext *);
typedef GLboolean (__fastcall *GCfn_bool_gc_gc_u)(__GLcontext *, __GLcontext *, GLuint);
typedef void      (__fastcall *GCfn_void_gc)(__GLcontext *);
typedef GLubyte   (__fastcall *GCfn_resize)(__GLcontext *, GLint, GLint, GLuint, GLuint);
/* the imports allocators all take the gc first (ecx), as wglutil.c's impMalloc .. impFree */
typedef void     *(__fastcall *GCfn_malloc)(__GLcontext *, size_t);
typedef void     *(__fastcall *GCfn_calloc)(__GLcontext *, size_t, size_t);
typedef void     *(__fastcall *GCfn_realloc)(__GLcontext *, void *, size_t);
typedef void      (__fastcall *GCfn_free)(__GLcontext *, void *);
typedef struct __WGLdrawableRec *(__fastcall *GCfn_getdp)(__GLcontext *);
typedef GLboolean (__fastcall *GCfn_getsize)(__GLcontext *, GLint *, GLint *, GLuint *, GLuint *);
typedef void      (__stdcall *GCfn_getgamma)(GLuint, GLuint *, GLuint *, GLuint *);
typedef void      (__stdcall *GCfn_tbufmask)(GLuint);

/* ---- imports (gc->imports[0..8], 0x00..0x20) and exports (gc->exports[], 0x24..0x4c), MS-kit layout: GLuint
** slots in glctx.h, read as the proc type */
#define GC_imports_malloc(gc)        (*(GCfn_malloc *)&(gc)->imports[0])
#define GC_imports_calloc(gc)        (*(GCfn_calloc *)&(gc)->imports[1])
#define GC_imports_realloc(gc)       (*(GCfn_realloc *)&(gc)->imports[2])
#define GC_imports_free(gc)          (*(GCfn_free *)&(gc)->imports[3])
#define GC_imports_getDrawablePrivate(gc) (*(GCfn_getdp *)&(gc)->imports[6])
#define GC_imports_getDrawableSize(gc) (*(GCfn_getsize *)&(gc)->imports[7])
#define GC_imports_wscx(gc)          (*(struct __WGLrcRec **)&(gc)->imports[8])      /* the owning rc */
/* calls through the imports (the SI's (*gc->imports.malloc)(gc, size) ...) */
#define __GL_MALLOC(gc, size)        (GC_imports_malloc(gc)((gc), (size)))
#define __GL_CALLOC(gc, size, n)     (GC_imports_calloc(gc)((gc), (size), (n)))
#define __GL_REALLOC(gc, p, size)    (GC_imports_realloc(gc)((gc), (p), (size)))
#define __GL_FREE(gc, p)             (GC_imports_free(gc)((gc), (p)))
#define __GL_GET_DRAWABLE_PRIVATE(gc) (GC_imports_getDrawablePrivate(gc)(gc))
#define GC_exports_destroyContext(gc)   (*(GCfn_bool_gc *)&(gc)->exports[__GL_EXP_DESTROY_CONTEXT])
#define GC_exports_loseCurrent(gc)      (*(GCfn_bool_gc *)&(gc)->exports[__GL_EXP_LOSE_CURRENT])
#define GC_exports_makeCurrent(gc)      (*(GCfn_bool_gc *)&(gc)->exports[__GL_EXP_MAKE_CURRENT])
#define GC_exports_shareContext(gc)     (*(GCfn_bool_gc_gc *)&(gc)->exports[__GL_EXP_SHARE_CONTEXT])
#define GC_exports_copyContext(gc)      (*(GCfn_bool_gc_gc_u *)&(gc)->exports[__GL_EXP_COPY_CONTEXT])
#define GC_exports_notifySwapBuffers(gc) (*(GCfn_void_gc *)&(gc)->exports[__GL_EXP_SWAP_BUFFERS])
#define GC_exports_notifyDestroy(gc)    (*(GCfn_void_gc *)&(gc)->exports[__GL_EXP_NOTIFY_DESTROY])
#define GC_exports_notifyResize(gc)     (*(GCfn_resize *)&(gc)->exports[__GL_EXP_NOTIFY_RESIZE])
/* gc->exports[8..10] (0x044 .. 0x04c): three ICD procs, set by __glEarlyInitContext 6907e440 */

#endif /* __GLCONTEXT_H__ */

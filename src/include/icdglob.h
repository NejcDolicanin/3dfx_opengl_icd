/*
** icdglob.h -- the globals more than one source file uses, declared with their types.  Each is defined in the module
** that owns it (named at its declaration); the generated code's tables are in src/data/gdata.c.
** Include after wglpriv.h (needs the DirectDraw types and __GLcontext).
*/
#ifndef __ICDGLOB_H__
#define __ICDGLOB_H__

/* ---- windowed rendering: the DirectDraw objects every windowed context shares (sstsurf.c 6901bfda creates them,
** ssthand.c releases them with the last user) */
extern LPDIRECTDRAW2 g_winDDraw;                /* IDirectDraw2 */
extern LPDIRECTDRAWCLIPPER g_winClipper;        /* the window clipper */
extern CRITICAL_SECTION g_winPrimaryLock;       /* guards the blits to / from the primary */
extern GLint g_winUsers;                        /* windowed contexts using the objects */
extern LPDIRECTDRAWSURFACE2 g_winPrimary;       /* the primary surface */

/* ---- the Glide SURFACE extension (glide3x gsfc.c), fetched by 6901bfda with grGetProcAddress; FX_CALL =
** __stdcall.  A surface (GrSurface_t) is the DirectDraw surface pointer; a context (GrContext_t) a handle.
** grSurfaceSetRenderingSurfaceExt takes a second argument (textureP) when hwcx.glideVerFlags & ~1. */
extern void (__stdcall *grSurfaceReleaseContextExt)(GLuint ctx);
extern int (__stdcall *grSurfaceCalcTextureWHDExt)(void *info, DWORD *w, DWORD *h, DWORD *d);
extern GLuint (__stdcall *grSurfaceCreateContextExt)(GLuint type);
extern void (__stdcall *grSurfaceSetAuxSurfaceExt)(void *sfc);
extern void (__stdcall *grSurfaceSetTextureSurfaceExt)(GLint tmu, void *sfc);
extern void (__stdcall *grSurfaceSetRenderingSurfaceExt)(void *sfc);
#define grSurfaceSetRenderingSurfaceExt1    grSurfaceSetRenderingSurfaceExt
#define grSurfaceSetRenderingSurfaceExt2    (*(void (__stdcall **)(void *sfc, GLint textureP))&grSurfaceSetRenderingSurfaceExt)

/* ---- the hardware path's TMUs: TEXUMA (one texture memory for both units; set at Glide open, 69028bd7) makes
** every per-TMU slot 0 (the texture's hwHandle[] / mem[] / onTmu[], the manager's heap[] / rover[]) */
extern char g_tmuSharedMemory;                  /* sst_grstate.c */
#define __GL_TMU_SLOT(tmu)  (g_tmuSharedMemory ? 0 : (tmu))

/* ---- statistics and caches */
extern GLuint g_elementsThisFrame;          /* sst_varray.c: glDrawElements counts; wglDevSwapBuffers copies and clears it */
extern GLuint g_elementsLastFrame;
extern FARPROC g_opengl32SwapBuffers;       /* sst_buffers.c: opengl32!wglSwapBuffers, looked up once (hw glFinish 69067cc9) */
extern FARPROC g_opengl32SwapBuffersSw;     /* gl_finish.c: the same, the software __glim_Finish's copy (690cda29) */
extern GLubyte g_disableOgDrawVertexes;     /* gl_varray.c: __GL_DISABLE_OG_DRAWVERTEXES */
extern GLubyte g_disableOgVarray;           /* __GL_DISABLE_OG_VARRAY */

/* ---- the software texture manager template (gl_tex.c; 690c3640 copies it) */
extern __GLtexMgrICD g_swTexMgrTemplate;


/* ---- generated-code constants (690f0024 .. 690f0047): [0] of 690f0028 = 49152.0f (1.5 * 2^15): x + 49152
** leaves x * 256 in the low mantissa bits (above 0x400000) of the float's bit pattern; its words 1..3 are the
** vertex source masks 690480d0 indexes from 690f0024 by size (2 bits per component: 0xc5 size 2, 0xd5 3,
** 0x55 4; the bias folded into the address, so the two must stay one array) */
extern GLuint g_vtxSizeMasks[9];                       /* gdata.c */
#define g_floatFrac8Bias    (&g_vtxSizeMasks[1])            /* the address the generated fadd reads */
#define g_vtxSizeMask(size) g_vtxSizeMasks[size]

/* ---- the hardware path's texture formats (sst_tex.c; hwTexelFormat 6904d740 picks one per internal format) */
extern const __GLtextureFormatICD g_texFmtL8, g_texFmtLA88, g_texFmtRGB332, g_texFmtRGB565, g_texFmtARGB4444, g_texFmtARGB1555,
                                  g_texFmtARGB8888, g_texFmtXRGB8888, g_texFmtA8, g_texFmtI8, g_texFmtP8, g_texFmtFXT1RGBA,
                                  g_texFmtFXT1RGB, g_texFmtDXT1RGB, g_texFmtDXT1RGBA, g_texFmtDXT3, g_texFmtDXT5;

/* ---- s_global.c (gl_global.c): the 4x4 dither table as 16 signed chars and as floats, -0.46875 .. 0.46875, and the
** six frustum clip planes of the clippers; the float-to-int bias the /Od span code adds, 12582912.0F = 3 << 22
** (__glFloatToIntBias: the compiler's constant, 69003bc0) */
extern GLbyte __glDitherTable[16];
extern __GLfloat __glDitherTableFloat[16];
extern __GLcoord __gl_frustumClipPlanes[6];
#define __glFloatToIntBias      ((__GLfloat) 12582912.0)
/* s_pick.c __glCDTPixel (gl_pick.c): the depth test of one pixel by function (GL_NEVER .. GL_ALWAYS), 16- and 32-bit,
** without and with a mask */
typedef GLboolean (__fastcall *__GLdepthTestPixel)(GLuint z, GLuint *zfb);     /* (__GLzValue z, __GLzValue *zfb) */
extern __GLdepthTestPixel __glCDTPixel[32];

/* ---- the clip-code tables of the generated code (sst_tmumem.c, sst_lockedtri.c, od_vafill.c, sst_lazyprocs.c) */
extern GLuint g_clipAxisBits[16];      /* by the |z|, |y|, |x| > |w| bits: the clip bits of those axes */
extern GLuint g_clipSideBits[16];      /* by the w, z, y, x sign bits: the clip bits of the sides they pick */
extern GLuint g_clipWNegBits[16];      /* by the same sign bits: clip bits set regardless (w negative) */

/* ---- dispatch tables (0x19a entries, copied into gc->currentDispatchState / listCompState) and the display-list
** execute tables (gc->dlist.baseListExec / listExec); SI / 3dfx S_DISP.C names, gl_disp.c */
extern __GLdispatchTable __glSSTImmedState, __glSSTListCompState;  /* hardware path: immediate mode, list compile */
extern __GLdispatchTable __glImmedState, __glListCompState;        /* software path (ReturnToSWRendering) */
extern void *__glListExecTable[256];
extern void *__gl_GenericDlOps[16];
extern GLuint g_swapDumpCount;              /* wgldev.c: swaps counted while glowWorm == 4 (the c:/swap<n>_<i>.txt
                                               dumps); unsigned: the original compares with jbe */

/* ---- the hardware path's primitive tables (sst_codebuf.c / sst_lazyprocs.c) */
typedef struct __GLhwPrimDescRec {          /* the vertex state machine of a primitive (hwcx.hwPrimDesc) */
    GLuint flags;
    struct __GLhwPrimDescRec *next;         /* the state after the next vertex */
} __GLhwPrimDesc;
extern __GLhwPrimDesc g_hwPrimDescs[39];            /* sst_lazyprocs.c */
extern __GLhwPrimDesc *g_hwPrimDescByMode[10];      /* by the glBegin mode: its first state */
extern GLuint g_immProcClassByMode[10];             /* the immediate-proc class: 0 points, 1 lines, 2 triangles, 3 quads */
extern __GLhwPrimDesc g_hwPrimDescNone;             /* the "no primitive" state (glEnd 69027060) */
extern GLubyte g_floorLog2Byte[64];                 /* sst_clip.c: floor(log2(i)) for i < 64 */
extern float g_defaultBorderR, g_defaultBorderG, g_defaultBorderB, g_defaultBorderA;   /* sst_texmgr.c */

/* ---- the hardware state-change test (sst_multitex.c / sst_shadestate.c): the shader state the last validation saw */
extern GLuint g_savedModeFlags, g_savedNeeds, g_savedFaceNeeds0, g_savedFaceNeeds1, g_savedMaterialNeeds;

/* ---- the pixel formats (wglDescribePixelFormat's list, gl_pixfmt.c / og_util.c) and the OG code cache */
extern GLint g_pfdListStale;                    /* gl_pixfmt.c: 1: rebuild the list */
extern GLint g_pfdCount;
extern PIXELFORMATDESCRIPTOR **g_pfdList;       /* wglMalloc'd [32] */
extern struct OgInsnRec *g_ogPendingList;       /* og_util.c: og.h OgInsn; the scheduler's list */
extern struct OgInsnRec *g_ogPendingTail;
extern GLint g_ogCacheReadEnv;                  /* 1 (never cleared): read __GL_DISABLE_OG_CACHE */
extern char g_ogCacheDisabled;                  /* __GL_DISABLE_OG_CACHE != 0: lookups return the head */


/* GR_TEXFMT_ARGB_8888 for the files that do not include g3ext.h */
#ifndef GR_TEXFMT_ARGB_8888
#define GR_TEXFMT_ARGB_8888     0x12
#endif

#endif /* __ICDGLOB_H__ */

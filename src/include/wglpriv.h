/*
** wglpriv.h -- private types of the WGL / window layer of 3dfxOGL.dll (1.0.0.0761)
**
** Every struct field carries its byte offset as
** observed in the original code; unknown fields are kept as "unk_XX" padding so
** that sizeof() and offsets stay identical to the original.  Original VAs are
** given as 6900xxxx (hex VA) in a comment next to each function.
**
** Lineage: Microsoft OpenGL ICD kit window layer ("wglDrvCliSetContext",
** "__wglInitializeDCI", ...) on top of the SGI Sample Implementation
** (__GLcontextModes / __GLimports / __GLexports in ref/ogl-sample).
*/
#ifndef __WGLPRIV_H__
#define __WGLPRIV_H__

#ifndef _GDI32_
#define _GDI32_           /* this DLL exports the GL and WGL entry points itself */
#endif
#include <windows.h>
#include <GL/gl.h>
#if defined(_MSC_VER) && _MSC_VER <= 1200     /* VC6 headers predate the *_PTR types */
typedef unsigned long ULONG_PTR;
typedef unsigned long UINT_PTR;
#endif
#include "wgldraw.h"

#include "glcontext.h"
#include "glstubs.h"      /* generated: prototypes of the gl dispatch stubs */
typedef struct __WGLwindowRec   __WGLwindow;
typedef struct __WGLrcRec       __WGLrc;
typedef struct __WGLthreadRec   __WGLthread;
typedef struct __WGLdeviceRec   __WGLdevice;

/*
** Context modes.  Identical to SGI's __GLcontextModes (0x54 bytes) plus one
** extra dword used by the ICD (byte 0x55 = !doubleBufferMode).  Filled from a
** PIXELFORMATDESCRIPTOR by wglModesFromPFD() [6900a6ad].
*/
typedef struct __WGLmodesRec {
    GLboolean rgbMode;              /* 0x00  iPixelType == PFD_TYPE_RGBA   */
    GLboolean colorIndexMode;       /* 0x01  iPixelType == PFD_TYPE_COLORINDEX */
    GLboolean doubleBufferMode;     /* 0x02  dwFlags & PFD_DOUBLEBUFFER    */
    GLboolean stereoMode;           /* 0x03  dwFlags & PFD_STEREO          */
    GLboolean haveAccumBuffer;      /* 0x04  cAccumBits != 0               */
    GLboolean haveDepthBuffer;      /* 0x05  cDepthBits != 0               */
    GLboolean haveStencilBuffer;    /* 0x06  cStencilBits != 0             */
    GLboolean pad7;                 /* 0x07 */
    GLint   redBits, greenBits, blueBits, alphaBits;       /* 0x08..0x14 */
    GLuint  redMask, greenMask, blueMask, alphaMask;       /* 0x18..0x24 */
    GLint   rgbBits;                /* 0x28  cColorBits                    */
    GLint   indexBits;              /* 0x2c  cColorBits                    */
    GLint   accumRedBits, accumGreenBits, accumBlueBits, accumAlphaBits; /* 0x30..0x3c */
    GLint   depthBits;              /* 0x40  cDepthBits, or GLFORCEZBITS   */
    GLint   stencilBits;            /* 0x44  cStencilBits                  */
    GLint   numAuxBuffers;          /* 0x48  always 0                      */
    GLint   level;                  /* 0x4c  always 0                      */
    GLint   pixmapMode;             /* 0x50  (left 0)                      */
    GLboolean layerPlane;           /* 0x54  set from DrvCreateLayerContext (passed in DL) */
    GLboolean singleBuffered;       /* 0x55  = !doubleBufferMode           */
    GLboolean pad56, pad57;
} __WGLmodes;                       /* sizeof == 0x58 (0x16 dwords)        */

/*
** Device callback table -- the one 3dfx-specific object the generic window
** layer talks to.  A pointer to it lives at 690eee84 (-> table at 690eee48).
** 0x24 is the first dword (a size/version), the rest are callbacks.
*/
struct __WGLdeviceRec {
    DWORD  size;                                    /* +0x00 = 0x24 (also tested as a caps word by the DDraw code) */
    DWORD  unk_04;                                  /* +0x04 */
    void  *(__fastcall *chooseTexFormat)(__GLcontext *gc, GLenum internalFormat, GLenum *baseFormat); /* +0x08 hwTexelFormat */
    BOOL   (__fastcall *processAttach)(DWORD threadId);       /* +0x0c wglGlideProcessAttach */
    BOOL   (__fastcall *processDetach)(DWORD threadId);       /* +0x10 wglGlideProcessDetach (returns 1) */
    BOOL   (__fastcall *threadAttach)(DWORD threadId);       /* +0x14 wglGlideThreadAttach (grSstSelect(0)) */
    BOOL   (__fastcall *threadDetach)(DWORD threadId);       /* +0x18 NULL */
    LONG   (__fastcall *describePixelFormat)(HDC hdc, void *pfd, int iPixelFormat); /* +0x1c wglGlideDescribePixelFormatCB */
    void   (__fastcall *resetPixelFormats)(void);   /* +0x20 690ea0a0 __glResetPixelFormats: drop the PFD list */
    void  *initTextureManager;                      /* +0x24 __glSSTInitTextureManager (__fastcall, (gc)) */
    __GLcontext *(__fastcall *createContext)(void *imports, const __WGLmodes *modes); /* +0x28 wglDevCreateContext */
    void   (__fastcall *initWindow)(__WGLwindow *w, const __WGLmodes *m);   /* +0x2c wglGlideInitWindow  on-screen window: Glide LFB buffers */
    void   (__fastcall *initBitmap)(__WGLwindow *w, const __WGLmodes *m);   /* +0x30 wglDrawInitDIB  memory DC: DIB */
    void   (__fastcall *destroyWindow)(__WGLwindow *w);                      /* +0x34 wglGlideDestroyWindow */
    void   (__fastcall *paletteChanged)(__WGLwindow *w);                     /* +0x38 NULL */
};                                                  /* 0x3c: g_device follows at 690eee84, then g_resTable */

/*
** Per-window object (one per HWND / memory DC with a pixel format set), 0xa1 dwords
** (0x284 bytes), allocated in wglCreateWindow() [6900a0d0].  The second part is the
** drawable private handed to the GL core (see wgldraw.h).
*/
struct __WGLwindowRec {
    __WGLwindow *prev;              /* 0x000  list link (head 690f62d8)    */
    __WGLwindow *next;              /* 0x004                                   */
    HDC        hdc;                 /* 0x008                                   */
    HWND       hwnd;                /* 0x00c                                   */
    POINT      clientOrigin;        /* 0x010  ClientToScreen(0,0)              */
    RECT       clientRect;          /* 0x018  screen coords                    */
    LONG       width;               /* 0x028                                   */
    LONG       height;              /* 0x02c                                   */
    int        iPixelFormat;        /* 0x030  set by DrvSetPixelFormat         */
    __WGLswapRegion *swapRegion;    /* 0x034  swap-hint rects (glAddSwapHintRectWIN) */
    LONG       swapRectCapacity;    /* 0x038  = 4 at creation                  */
    __WGLregionRect clipRect;       /* 0x03c  from setClipRect                 */
    GLint      clipChanged;         /* 0x04c                                   */
    void     (__fastcall *destroy)(__WGLwindow *w);          /* 0x050  = device->destroyWindow */
    void     (__fastcall *paletteChanged)(__WGLwindow *w);   /* 0x054  = device->paletteChanged */
    GLuint   (__fastcall *swapBuffers)(__WGLwindow *w);      /* 0x058  DDraw: wglDDrawSwapBuffers */
    GLuint   (__fastcall *present)(__WGLwindow *w);          /* 0x05c  DDraw: wglDDrawSwapBuffers */
    GLuint   (__fastcall *copyBackToFront)(__WGLwindow *w);  /* 0x060  DDraw: wglDDrawCopyBackToFront */
    __WGLdrawable dp;               /* 0x064 .. 0x284                          */
};
#define WGL_WINDOW_FROM_DP(dp)  ((dp)->window)

/*
** Rendering context handle object (HGLRC body), 0x19 dwords = 0x64 bytes,
** allocated in DrvCreateLayerContext.
*/
struct __WGLrcRec {
    __WGLrc   *prev;                /* 0x00  list head 690f6568            */
    __WGLrc   *next;                /* 0x04                                    */
    __GLcontext *gc;                /* 0x08  device->createContext()           */
    __WGLmodes  *modes;             /* 0x0c  wglNewModes()                     */
    GLboolean  isCurrent;           /* 0x10                                    */
    BYTE       pad_11[3];
    DWORD      threadId;            /* 0x14  owner thread, -1 when released    */
    GLboolean  everCurrent;         /* 0x18                                    */
    BYTE       pad_19[3];
    DWORD      flags;               /* 0x1c  bit0 = valid?, bit4 = being made current */
    DWORD      unk_20;              /* 0x20  low byte set to 1 at create (drv.c, wglstand.c); never read */
    DWORD      handle;              /* 0x24  value returned to opengl32 (counter 690ee3c0) */
    HWND       hwnd;                /* 0x28                                    */
    HDC        hdc;                 /* 0x2c  our own DC (GetDC / CreateCompatibleDC) */
    PIXELFORMATDESCRIPTOR pfd;      /* 0x30 .. 0x57                            */
    int        iPixelFormat;        /* 0x58                                    */
    void      *callbacks;           /* 0x5c  3rd arg of DrvSetContext          */
    __WGLwindow *window;            /* 0x60                                    */
};

/*
** Per-thread record kept in TLS slot g_tlsThread (690ee394).
*/
struct __WGLthreadRec {
    HHOOK        hook;              /* 0x00  SetWindowsHookEx(WH_CALLWNDPROC) */
    __WGLthread *next;              /* 0x04  list, head/sentinel at 690f65d0 */
    /* size unknown; allocated with wglCalloc() */
};

/* ---- globals (names ours, addresses original) ---- */
extern HINSTANCE     g_hInstance;        /* 693c3cd0 */
extern BOOL          g_isWin9x;          /* 693c3cdc  dwPlatformId == VER_PLATFORM_WIN32_WINDOWS */
extern BOOL          g_isNT4;            /* 693c3cd4 */
extern BOOL          g_isWin2000;        /* 693c3cd8 */
extern __GLcontext  *g_nullGC;           /* 693c3ccc  "no context" gc with null dispatch */
extern DWORD         g_tlsContext;       /* 690ee390  TlsAlloc(): current gc */
extern DWORD         g_tlsThread;        /* 690ee394  TlsAlloc(): __WGLthread* */
extern DWORD         g_tebOffContext;    /* 690f65d8  fs:[] offset of TLS slot g_tlsContext */
extern DWORD         g_tebOffThread;     /* 690f65dc */
extern __WGLdevice  *g_device;           /* 690eee84 -> 690eee48 */
extern int           g_errorLogLevel;    /* 690f6560  __GL_ERROR_LOG / GLinfo registry */
extern HANDLE        g_errorLogFile;     /* 690ee3bc  OGLError.txt, INVALID_HANDLE_VALUE */
extern BOOL          g_processDetached;  /* 690f65e0 */
extern HMODULE       g_hStandAloneGL;    /* 690f65e4  non-NULL when running stand-alone */
extern CRITICAL_SECTION g_wglLock;       /* 690f65e8 */
extern int           g_wglLockRefs;      /* 690f6600 */
extern DWORD         g_nextRcHandle;     /* 690ee3c0 */
extern __WGLwindow  *g_windowList;       /* 690f62dc  (sentinel 690f62d8) */
extern __WGLrc      *g_rcList;           /* 690f656c  (sentinel 690f6568) */
extern __WGLthread  *g_threadList;       /* 690f65d4  (sentinel 690f65d0) */
extern __WGLrc      *g_lastRc;           /* 690ee3b8 = g_wglImports[8] (the wscx slot): no separate variable, see wglRcCreateGC */

/* ---- wglutil.c ---- */
void *__fastcall wglMalloc(size_t n);                              /* 69009650 */
void *__fastcall wglCalloc(size_t n, size_t sz);                   /* 69009670 */
void *__fastcall wglRealloc(void *p, size_t n);                    /* 69009690 */
void  __fastcall wglFree(void *p);                                 /* 69009700 */
void  wglOpenErrorLog(void);                                       /* 6900999d */
void  wglMessage(const char *s);                                   /* 69009920 */
void  wglMessageInt(const char *s, int v);                         /* 69009a5d */
void  wglOglError(const char *s);                                  /* 69009cac */
void  __fastcall wglSystemError(const char *where, DWORD code);    /* 69009d26 */
void  wglInitErrorLogLevel(void);                                  /* 6900b3b9 */
DWORD __fastcall wglRegReadDword(LPCSTR key, LPCSTR value);        /* 6900a870 */
DWORD __fastcall wglReadGLinfo(LPCSTR name);                      /* 6900a911 */
BOOL  wglGetOSVersion(void);                                       /* 6900b3f0 */
void  wglLockCreate(void);                                         /* 6900c900 */
void  wglLockDestroy(void);                                        /* 6900c920 */
void  wglLock(void);                                               /* 6900c940 */
void  wglUnlock(void);                                             /* 6900c950 */
LPCRITICAL_SECTION wglNewCritSect(void);                           /* 6900c960 */
void  __fastcall wglDeleteCritSect(LPCRITICAL_SECTION cs);         /* 6900c990 */
/* ICD_TRACE builds (nmake EXTRA=/DICD_TRACE): one line per step appended to C:\3DFXOGL.LOG */
#ifdef ICD_TRACE
void icdTrace(const char *msg, DWORD value);
#define ICD_T(msg, v) icdTrace(msg, (DWORD)(v))
#define ICD_TD(msg)   int icd_trace_ = (icdTrace(msg, 0), 0);    /* a declaration: legal before the other declarations */
extern LONG g_icdLfbLocks;          /* outstanding grLfbLock()s (lifecycle tracing) */
#define ICD_LFB(n)    (g_icdLfbLocks += (n))
#define ICD_LFB_N     g_icdLfbLocks
#elif defined(ICD_MARK)
/* ICD_MARK builds (nmake EXTRA=/DICD_MARK): each trace point is three plain stores (file, line, value) into a
** ring -- no calls, no I/O, no stack -- dumped to C:\3DFXOGL.LOG at process detach (for timing-sensitive bugs) */
extern volatile DWORD g_icdMarks[1024];
extern volatile DWORD g_icdMarkN;
void icdMarkDump(void);
DWORD icdEsp(void);                 /* naked: the caller's ESP at the call (esp + 4) */
#define ICD_M_(x)     (g_icdMarks[g_icdMarkN++ % 1024] = (DWORD)(x))
#define ICD_T(msg, v) (ICD_M_(__FILE__), ICD_M_(__LINE__), ICD_M_(v), ICD_M_(icdEsp()))
#define ICD_TD(msg)
/* GL errors (__glSetError): code, frame number and 30 stack dwords per error, first 64 errors */
extern volatile DWORD g_icdSwapN;
void __fastcall icdErrMark(DWORD code, const DWORD *sp);
void __stdcall icdGLHook(DWORD slot, const DWORD *sp);
void icdHookGlide(HMODULE self);
int __fastcall icdCount(DWORD id);
int __stdcall icdProbe(DWORD tag);
int __stdcall icdProbeV(DWORD tag, DWORD v);
#define ICD_SWAP(h)   (g_icdSwapN++ < 4 ? (ICD_T("wglSwapBuffers hdc", h), 0) : 0)
#else
#define ICD_T(msg, v) ((void)0)
#define ICD_TD(msg)
#endif
#ifndef ICD_LFB
#define ICD_LFB(n)    ((void)0)
#define ICD_LFB_N     0
#endif
#ifndef ICD_SWAP
#define ICD_SWAP(h)   ICD_T("wglSwapBuffers hdc", h)
#endif
/* list sentinel: a dummy head node whose "next" (+4) is the list (690f62d8 windows) */
typedef struct { void *prev; void *next; } __WGLlistHead;
extern __WGLrc s_rcHead;                                            /* 690f6568: the rc sentinel, a whole zeroed __WGLrc */
extern __GLcontext  *s_nullGCCopy;                                  /* 690f6570 = s_rcHead.gc (see wglutil.c) */
#define s_nullGCCopy (s_rcHead.gc)
__GLcontext *wglGetCurrentGC(void);                                /* 6900af30 */
/* The glcore's own fetch of the current context: the MS-kit macro reads the TEB slot in one inline
** asm block (TEB + g_tebOffContext), wherever the original reads `fs:[18h]`; the asm (not a call)
** is what gives those functions their EBP frame and the repeated fetches, as in the original. */
#ifdef _MSC_VER
#pragma warning(disable: 4035)      /* no return value: it is in EAX */
static __inline __GLcontext *__glCurrentGC(void)
{
    __asm { mov eax, fs:[18h] }
    __asm { add eax, g_tebOffContext }
    __asm { mov eax, [eax] }
}
static __inline void __glSetCurrentGC(__GLcontext *gc)
{
    __asm { mov eax, fs:[18h] }
    __asm { add eax, g_tebOffContext }
    __asm { mov ecx, gc }
    __asm { mov [eax], ecx }
}
#else
#define __glCurrentGC() wglGetCurrentGC()
#define __glSetCurrentGC(gc) wglSetCurrentGC(gc)
#endif
void  __fastcall wglSetCurrentGC(__GLcontext *gc);                 /* 6900af60 */
BOOL  __fastcall wglAttachThread(HINSTANCE hinst);                 /* 6900af8d (hinst only passed on) */
void  wglThreadListInit(void);                                     /* 6900b480 */
__WGLthread *wglThreadListFirst(void);                             /* 6900b0a0 */
void  __fastcall wglThreadListAdd(__WGLthread *t);                 /* 6900b0c0 */
BOOL  __fastcall wglThreadListRemoveCurrent(HINSTANCE hinst);       /* 6900b0e0 (hinst only passed on) */
void  __fastcall wglThreadListUnlink(__WGLthread *t);              /* 6900b120 */
BOOL  __fastcall wglThreadDetach(__WGLthread *t);                  /* 6900b160 */
BOOL  __fastcall wglAttachMessageHook(HWND hwnd);                  /* 6900ae12 */
BOOL  wglDetachMessageHook(void);                                  /* 6900aed2 */
__WGLrc *__fastcall wglRcFromHandle(DWORD handle);                 /* 69009ea0 */
void  __fastcall wglRcListAdd(__WGLrc *rc);                        /* 69009e60 */
void  __fastcall wglRcListRemove(__WGLrc *rc);                     /* 69009e80 */
__WGLrc *__fastcall wglRcFromHwnd(HWND hwnd, __WGLrc *after);      /* 69009f20 */
__WGLwindow *__fastcall wglFindWindow(HDC hdc, HWND hwnd);         /* 6900a2b0 */
__WGLwindow *__fastcall wglCreateWindow(HDC hdc, HWND hwnd, const __WGLmodes *modes); /* 6900a0d0 */
__WGLwindow *__fastcall wglGetWindow(HDC hdc, HWND hwnd, const __WGLmodes *modes);    /* 6900a650 */
GLboolean __fastcall wglDestroyWindow(__WGLwindow *w);             /* 6900a240 */
void  __fastcall wglWindowListAdd(__WGLwindow *w);                 /* 69009f60 */
void  __fastcall wglWindowListRemove(__WGLwindow *w);              /* 69009f90 */
void  wglDestroyAllWindows(void);                                  /* 6900b530 */
void  __fastcall wglUpdateWindowRect(__WGLwindow *w);              /* 6900a4d0 */
GLboolean __fastcall wglWindowResized(__WGLwindow *w);             /* 6900a570 */
GLboolean __fastcall wglIsFullScreenWindow(__WGLwindow *w);        /* 6900a3bc */
void  __fastcall wglModesFromPFD(__WGLmodes *m, const PIXELFORMATDESCRIPTOR *pfd); /* 6900a6ad */
GLboolean __fastcall wglRcCreateGC(__WGLrc *rc, int layerPlane);  /* 6900a800 */
GLboolean __fastcall wglRcDestroyGC(__WGLrc *rc);                  /* 6900a850 */

extern void *g_wglImports[9];                                      /* 690ee398 */

/* ---- dllmain.c ---- */
BOOL  wglProcessAttach(HINSTANCE hinst);                           /* 6900b2e0 */
BOOL  wglProcessDetach(HINSTANCE hinst);                           /* 6900b4a0 (hinst unused) */
void  wglCreateNullContext(void);                                  /* 69009d70 */
BOOL  wglBindStandAlone(HINSTANCE hinst);                          /* 6900c7c0 (hinst unused) */
GLboolean __fastcall wglBindGLEntryPoints(HMODULE h);              /* 6900c830 */
GLboolean __fastcall wglBindWGLEntryPoints(HMODULE h);             /* 6900c890 */
BOOL  wglUnloadStandAlone(void);                                   /* 6900c8d0 */
BOOL  __fastcall wglIsStandAlone(HINSTANCE hinst);                  /* 6900c8f0 (hinst unused; the callers pass it in ECX) */

__GLcontext *wglTebReadContext(void);                              /* fs:[g_tebOffContext] without attach */
void  wglReleaseAllRcs(void);                                      /* 69009dd5 */
void *__fastcall wglGetProcAddressTable(const char *name);         /* 69009882 */
__WGLrc *__fastcall wglRcFromGC(__GLcontext *gc);                  /* 69009ee0 */

/* ---- the debug trace (dllmain.c) and the glcore's context entry points (glcore_hand.c) ---- */
void  __cdecl wglDebugPrintf(const char *fmt, ...);                          /* 6900adf0  wglDebugPrintf: debug trace, empty in release */
void *__fastcall __glCreateContext(void *imports, const __WGLmodes *m); /* 69078f50  __glCreateContext: allocate the __GLcontext (0x35150 bytes) */
void  __glInstallNullDispatch(void);                                          /* 69079a50  __glInstallNullDispatch: copy the null dispatch into the current gc */
void *__glGetCurrentDispatch(void);                                          /* 69079ad0  __glGetCurrentDispatch: &gc->dispatch (gc+0x1738) */

/* ---- wgldev.c / wglglide.c / wglext*.c ---- */
__GLcontext *__fastcall wglDevCreateContext(void *imports, const __WGLmodes *modes);   /* 690124b5 */
GLboolean __fastcall wglDevDestroyContext(__GLcontext *gc);                           /* 69012a01 */
GLboolean __fastcall wglDevLoseCurrent(__GLcontext *gc);                              /* 69012b50 */
GLboolean __fastcall wglDevMakeCurrent(__GLcontext *gc);                              /* 69012bd1 */
GLboolean __fastcall wglDevShareContext(__GLcontext *gc, __GLcontext *share);         /* 69013660 */
GLubyte   __fastcall wglDevNotifyResize(__GLcontext *gc, GLint x, GLint y, GLuint w, GLuint h); /* 69012790 */
void      __fastcall wglDevNotifyDestroy(__GLcontext *gc);                            /* 690129f0 */
void      __fastcall wglDevSwapBuffers(__GLcontext *gc);                              /* 69014a3b */
void      __fastcall wglDevForceSWRendering(__GLcontext *gc, void *arg);              /* 69011c0c */
void      __fastcall wglDevReturnToHWRendering(__GLcontext *gc, void *arg);           /* 69011f14 */
void      __fastcall wglDevInstallHWProcs(__GLcontext *gc);                           /* 690130b5 */
void      __fastcall wglDevInitBuffers(__GLcontext *gc);                              /* 69013500 */
void      __fastcall wglDevInitPools(__GLcontext *gc);                                /* 69012260 */
void      __fastcall wglDevDetectScreenSaver(__GLcontext *gc);                        /* 6901239d */
void      __fastcall wglDevCheckViewportInside(__GLcontext *gc);                      /* 69011d80 */
GLboolean __fastcall wglDevClipWindow(__GLcontext *gc);                               /* 690120c0 */
GLboolean __fastcall wglDevViewportChanged(__GLcontext *gc);                          /* 69013470 */
void      __fastcall wglDevNonUnityQ(__GLcontext *gc, void *arg);                     /* 69012430 */
void      __fastcall wglDevFinish(__GLcontext *gc);                                   /* 69012170 */
void      __fastcall wglDevFlush(__GLcontext *gc);                                    /* 690121e0 */
void      __fastcall wglDevFinishFront(__GLcontext *gc);                              /* 69012060 */
void      __fastcall wglDevFlushSW(__GLcontext *gc);                                  /* 69011e20 */
void      __fastcall wglDevFlushSW2(__GLcontext *gc);                                 /* 69011e90 (identical copy) */
void      __fastcall wglDevFinishFrontSW(__GLcontext *gc);                            /* 69011d30 */
GLboolean __fastcall wglDevCheckViewportSW(__GLcontext *gc);                          /* 69011d60 */
void      __fastcall wglDevNop1(void); void __fastcall wglDevNop2(void);              /* 690120a0 / 690120b0 */
GLint     wglDisplayQuery(void);                                                      /* 69014c10 */
void      wglDisplayNotifyOpen(void); void wglDisplayNotifyClose(void);               /* 69014c70 / 69014cc0 */
BOOL      __fastcall wglGlideProcessAttach(DWORD threadId); BOOL __fastcall wglGlideProcessDetach(DWORD threadId); BOOL __fastcall wglGlideThreadAttach(DWORD threadId);
LONG      __fastcall wglGlideDescribePixelFormatCB(HDC hdc, void *out, int index);     /* 69014d40 */
GLint     __fastcall wglGlideDescribePixelFormat(HDC hdc, PIXELFORMATDESCRIPTOR *out, int index); /* 69014db8 */
GLint     __fastcall wglGlideChooseResolution(__GLcontext *gc);                       /* 69014f2f */
GLint     __fastcall wglGlideWinOpen(__GLcontext *gc, GLint resolution);              /* 690150e3 */
void      __fastcall wglGlideWinClose(__GLcontext *gc);                               /* 690151e0 */
GLint     __fastcall wglGlideOpen(__GLcontext *gc);                                   /* 69015619 */
void      __fastcall wglGlideClose(__GLcontext *gc);                                  /* 69015710 */
void      __fastcall wglGlideLose(HWND hwnd); void __fastcall wglGlideRestore(HWND hwnd); /* 69015750 / 690157a1 */
GLboolean __fastcall wglGlideWindowSwap(__WGLwindow *w);                              /* 69015553 */
void      __cdecl wglGlideTrace(const char *fmt, ...);                                /* 690155b0 (empty) */
void      __fastcall wglGlideInitLFBWindow(__WGLwindow *w, const __WGLmodes *m);      /* 69015200 */
void      __fastcall wglGlideInitWindow(__WGLwindow *w, const __WGLmodes *m);         /* 6904d4c0 */
void      __fastcall wglGlideDestroyWindow(__WGLwindow *w);                           /* 6904d560 */
void      __fastcall wglGlideValidate(__GLcontext *gc);                               /* 69015c40 */
void      __fastcall wglGlideValidateSW(__GLcontext *gc);                             /* 69015c70 */
void      __fastcall wglGlideSetupBufferModes(__GLcontext *gc);                       /* 69015cd0 */
void      __fastcall wglGlideSetupDrawBuffers(__GLcontext *gc);                       /* 69015e40 */
extern GLint g_defaultResolution;                                                     /* 690eeedc */
extern __GLcontext *g_glideOwnerGC;                                                   /* 690f66c0 */

/* ---- wglext.c / wglext3dfx.c entry points ---- */
const char *WINAPI wglGetExtensionsStringEXT(void);
const char *WINAPI wglGetExtensionsStringARB(HDC);
GLboolean WINAPI wglSwapIntervalEXT(int);     /* (returns a byte; BOOL in the extension spec) */
int  WINAPI wglGetSwapIntervalEXT(void);
BOOL WINAPI wglSetDeviceGammaRamp3DFX(HDC, LPVOID);
BOOL WINAPI wglGetDeviceGammaRamp3DFX(HDC, LPVOID);
BOOL WINAPI wglQueryMultisample3DFX(int);
BOOL WINAPI wglSetMultisample3DFX(int);
BOOL WINAPI wglGetPixelFormatAttribivARB(HDC, int, int, UINT, const int *, int *);
BOOL WINAPI wglGetPixelFormatAttribfvARB(HDC, int, int, UINT, const int *, FLOAT *);
BOOL WINAPI wglChoosePixelFormatARB(HDC, const int *, const FLOAT *, UINT, int *, UINT *);
void APIENTRY glTBufferMask3DFX_impl(GLuint mask);
void __fastcall wglReloadGammaRamp(__GLcontext *gc);
void      __fastcall wglRcCallback(__GLcontext *gc, void *table);             /* 6900c370 */
GLuint    __fastcall wglRcSwapBuffers(__GLcontext *gc);             /* 6900c380 */

/*
** The ICD's pending-resize check after a make-current (DrvSetContext 6900bed3, wglMakeCurrent
** 6900d6ba, the sst validator 6901a539): written `!flags & 0x10` (sic) -- '!' binds first, so the
** condition is never true and notifyResize with the clip box is never called.  Kept as written.
*/
/* `if (MMX) emms`: the 3dfx paths that may run MMX code (generated or Glide's) clear the MMX state
** before x87 work (hwDrawTriPackColor 6902abf0: three times).  cpuCaps bit 0 = MMX. */
#define GC_EMMS_IF_MMX(gc_) do { if (*(GLubyte *)&(gc_)->constants.cpuCaps & __GL_CPU_MMX) { __asm emms } } while (0)

#define WGL_CHECK_RESIZE(gc_) do {                                                        \
        __GLcontext *g_ = (gc_);                                                          \
        __WGLrc *r_ = GC_imports_wscx(g_);                                                \
        if ((r_->flags & 1) && (!r_->flags & 0x10)) {                                     \
            r_->flags &= ~1u;                                                             \
            GC_exports_notifyResize(g_)(g_, g_->transform.clipX0, g_->transform.clipY0,   \
                                        g_->transform.clipX1, g_->transform.clipY1);   \
        }                                                                                 \
    } while (0)

/* OG code generator (og.h): an operand record (24 bytes, as every record in gc->ogInsn[]) and the
** static register operands (src/gl/og_regs.c) */
typedef struct __GLogOperandRec {
    GLuint      kind;                       /* +0x00  0 imm, 1 reg8, 3 reg32, 4 st(i), 5 mmx/xmm, 8 label (og.h OG_*) */
    GLint       val;                        /* +0x04  register number / immediate; label 0, fixup 1 = rel32 (0 rel8) */
    struct __GLogOperandRec *link;          /* +0x08  label: its first fixup; fixup: the next one */
    GLubyte    *addr;                       /* +0x0c  label: its address once bound; fixup: where it is patched */
    GLuint      unused10, unused14;         /* +0x10 */
} __GLogOperand;
extern __GLogOperand og_eax, og_ecx, og_edx, og_ebx, og_esp, og_ebp, og_esi, og_edi;
extern __GLogOperand og_al, og_cl, og_dl, og_bl, og_ah, og_ch, og_dh, og_bh;
extern __GLogOperand og_st0, og_st1, og_st2, og_st3, og_st4, og_st5, og_st6, og_st7;
extern __GLogOperand og_mm0, og_mm1, og_mm2, og_mm3, og_mm4, og_mm5, og_mm6, og_mm7;


#endif /* __WGLPRIV_H__ */

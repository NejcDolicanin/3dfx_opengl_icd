/*
** wglutil.c -- allocation, logging, locking, thread/rc/window bookkeeping of the
** WGL layer.  Original VA in each header
** comment.  Internal functions use __fastcall (the DLL was built with /Gr).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wglpriv.h"
#include "icdprotos.h"

/* ------------------------------------------------------------------ globals */
HINSTANCE     g_hInstance;                              /* 693c3cd0 */
BOOL          g_isWin9x;                                /* 693c3cdc */
BOOL          g_isNT4;                                  /* 693c3cd4 */
BOOL          g_isWin2000;                              /* 693c3cd8 */
__GLcontext  *g_nullGC;                                 /* 693c3ccc */
DWORD         g_tlsContext = 0x40;                      /* 690ee390 (TlsAlloc at attach; 0x40 in the file) */
DWORD         g_tlsThread = 0x40;                       /* 690ee394 (TlsAlloc at attach; 0x40 in the file) */
DWORD         g_tebOffContext;                          /* 690f65d8 */
DWORD         g_tebOffThread;                           /* 690f65dc */
int           g_errorLogLevel;                          /* 690f6560 */
HANDLE        g_errorLogFile = INVALID_HANDLE_VALUE;    /* 690ee3bc */
BOOL          g_processDetached;                        /* 690f65e0 */
HMODULE       g_hStandAloneGL;                          /* 690f65e4 */
CRITICAL_SECTION g_wglLock;                             /* 690f65e8 */
int           g_wglLockRefs;                            /* 690f6600 */
DWORD         g_nextRcHandle = 1;                       /* 690ee3c0 ICD rc handles start at 1: 0 is a NULL HGLRC */

/* list sentinels: the original keeps a dummy head node whose "next" (+4) is the list */
/* The window and rc sentinels are whole zeroed objects in the original (690f62d8 + 0x284 + 4 = 690f6560, the next
** global; 690f6568 + 0x64 + 4 = 690f65d0, the thread sentinel): lookups start AT the sentinel and read its fields
** (wglRcFromHandle(0) finds the rc sentinel, handle 0, whose gc (690f6570) is the null gc; the window-flag loop
** writes into the window sentinel).  Two-pointer heads made those reads/writes hit unrelated globals: Quake 2
** crashed in wglSAMakeCurrent(NULL, NULL). */
static __WGLwindow s_windowHead;                          /* 690f62d8 / g_windowList = 690f62dc */
__WGLrc s_rcHead;                                         /* 690f6568 / g_rcList     = 690f656c */
static struct { void *prev; void *next; } s_threadHead;
/* the thread sentinel 690f65d0 / g_threadList = 690f65d4 is just {hook, next} */
#define g_windowList (*(__WGLwindow **)&s_windowHead.next)
#define g_rcList     (*(__WGLrc **)&s_rcHead.next)
#define g_threadList (*(__WGLthread **)&s_threadHead.next)

/* ------------------------------------------------------------- allocation */
/* 69009650 -- wglMalloc: a zeroed fixed GlobalAlloc block of n bytes; NULL for n == 0 */
void *__fastcall wglMalloc(size_t n)
{
    if (n == 0) return NULL;
    return GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, n);
}

/* 69009670 -- wglCalloc: a zeroed fixed GlobalAlloc block of n * sz bytes; NULL when either is 0 */
void *__fastcall wglCalloc(size_t n, size_t sz)
{
    if (n != 0 && sz != 0)
        return GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, n * sz);
    return NULL;
}

/* 69009690 -- GlobalAlloc-based realloc; a failed allocation returns NULL and leaves p alone */
void *__fastcall wglRealloc(void *p, size_t n)
{
    void *q = NULL;
    SIZE_T old;
    if (n != 0) {
        q = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, n);
        if (p != NULL && q != NULL) {
            old = GlobalSize(p);
            if (old > n) old = n;
            memcpy(q, p, old);
            GlobalFree(p);
        }
    } else if (p != NULL) {
        GlobalFree(p);
        return NULL;
    }
    if (q == NULL) return NULL;
    return q;
}

/* 69009700 -- wglFree: GlobalFree p unless it is NULL */
void __fastcall wglFree(void *p)
{
    if (p != NULL) GlobalFree(p);
}

/* ---------------------------------------------------------------- logging */
/* 6900999d -- open %TEMP%\OGLError.txt once logging is enabled */
void wglOpenErrorLog(void)
{
    SECURITY_ATTRIBUTES sa;
    CHAR path[80];

    if (g_errorLogLevel != 0 && g_errorLogFile == INVALID_HANDLE_VALUE) {
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = FALSE;
        GetTempPathA(sizeof(path), path);
        strcat(path, "OGLError.txt");
        g_errorLogFile = CreateFileA(path, GENERIC_WRITE, 0, &sa, CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
    }
}

/* 69009920 -- wglMessage: write "WGL Message:<s>" to the error log (opened if logging is on), else to
** OutputDebugString */
void wglMessage(const char *s)
{
    DWORD written;
    char buf[256];

    sprintf(buf, "WGL Message:%s\n", s);
    wglOpenErrorLog();
    if (g_errorLogFile != INVALID_HANDLE_VALUE)
        WriteFile(g_errorLogFile, buf, strlen(buf), &written, NULL);
    else
        OutputDebugStringA(buf);
}

/* 69009a5d -- wglMessageInt: "WGL Message:<s>: <v> (0x<v>)" to OutputDebugString and, when open, the error log */
void wglMessageInt(const char *s, int v)
{
    DWORD written;
    char buf[256];

    sprintf(buf, "WGL Message:%s: %d (0x%08x)\n", s, v, v);
    OutputDebugStringA(buf);
    wglOpenErrorLog();
    if (g_errorLogFile != INVALID_HANDLE_VALUE)
        WriteFile(g_errorLogFile, buf, strlen(buf), &written, NULL);
}

/* 69009aef -- wglMessageArg2 (3dfx WGLCOMM.C __wglMessageArg2): "WGL Message:<s>: 0x<a> 0x<b>" to
** OutputDebugString and, when open, the error log; ToDo UNUSED CODE: nothing in the driver calls it */
void wglMessageArg2(const char *s, int a, int b)
{
    DWORD written;
    char buf[256];

    sprintf(buf, "WGL Message:%s: 0x%08x 0x%08x\n", s, a, b);
    OutputDebugStringA(buf);
    wglOpenErrorLog();
    if (g_errorLogFile != INVALID_HANDLE_VALUE)
        WriteFile(g_errorLogFile, buf, strlen(buf), &written, NULL);
}

/* 69009b86 -- wglMessageArg3: as wglMessageArg2 with three values; ToDo UNUSED CODE: nothing in the driver
** calls it */
void wglMessageArg3(const char *s, int a, int b, int c)
{
    DWORD written;
    char buf[256];

    sprintf(buf, "WGL Message:%s: 0x%08x 0x%08x 0x%08x\n", s, a, b, c);
    OutputDebugStringA(buf);
    wglOpenErrorLog();
    if (g_errorLogFile != INVALID_HANDLE_VALUE)
        WriteFile(g_errorLogFile, buf, strlen(buf), &written, NULL);
}

/* 69009c1b -- wglMessageFloat (3dfx WGLCOMM.C __wglMessageFloat): the value as a float and as its bits;
** ToDo UNUSED CODE: nothing in the driver calls it */
void wglMessageFloat(const char *s, float f)
{
    DWORD written;
    char buf[256];

    sprintf(buf, "WGL Message:%s: %f (%08x)\n", s, f, *(int *)&f);
    OutputDebugStringA(buf);
    wglOpenErrorLog();
    if (g_errorLogFile != INVALID_HANDLE_VALUE)
        WriteFile(g_errorLogFile, buf, strlen(buf), &written, NULL);
}

/* 69009cac -- wglOglError: "OGL ERR:<s>" to OutputDebugString and, when open, the error log (the core's
** warning / fatal imports) */
void wglOglError(const char *s)
{
    DWORD written;
    char buf[256];

    sprintf(buf, "OGL ERR:%s\n", s);
    OutputDebugStringA(buf);
    wglOpenErrorLog();
    if (g_errorLogFile != INVALID_HANDLE_VALUE)
        WriteFile(g_errorLogFile, buf, strlen(buf), &written, NULL);
}

/* 69009d26 -- log and SetLastError(0xC007xxxx) */
void __fastcall wglSystemError(const char *where, DWORD code)
{
    char buf[256];

    sprintf(buf, "System Error: (%d):%s", (int)code, where);
    wglMessage(buf);
    SetLastError(code | 0xC0070000);
}

/* 6900a870 -- read one REG_DWORD under HKLM; -1 on any failure */
DWORD __fastcall wglRegReadDword(LPCSTR key, LPCSTR value)
{
    HKEY  hkey;
    DWORD data, type, size = sizeof(DWORD);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key, 0, KEY_ALL_ACCESS, &hkey) != ERROR_SUCCESS)
        return (DWORD)-1;
    if (RegQueryValueExA(hkey, value, NULL, &type, (LPBYTE)&data, &size) != ERROR_SUCCESS)
        data = (DWORD)-1;
    if (RegCloseKey(hkey) != ERROR_SUCCESS)
        return (DWORD)-1;
    return data;
}

/* 6900a911 -- HKLM\Software\3Dfx Interactive\GLinfo\<name>; 0 when missing */
DWORD __fastcall wglReadGLinfo(LPCSTR name)
{
    DWORD v = wglRegReadDword("Software\\3Dfx Interactive\\GLinfo", name);
    return (v != (DWORD)-1) ? v : 0;
}

/* 6900b3b9 -- wglInitErrorLogLevel: g_errorLogLevel: env __GL_ERROR_LOG (atoi), else GLinfo\ErrorLog (0 if unset) */
void wglInitErrorLogLevel(void)
{
    const char *s = getenv("__GL_ERROR_LOG");
    if (s != NULL) { g_errorLogLevel = atoi(s); return; }
    g_errorLogLevel = wglReadGLinfo("ErrorLog");
}

/* inlined into 6900b3f0 (an __inline helper: VC6 then pushes esi only after the early return) */
static __inline BOOL wglSetOS(const OSVERSIONINFOA *vi)
{
    if (vi->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
        g_isWin9x = TRUE;
    } else {
        g_isWin9x = FALSE;
        if (vi->dwPlatformId == VER_PLATFORM_WIN32_NT && vi->dwMajorVersion == 4) {
            g_isNT4 = TRUE;
            g_isWin2000 = FALSE;
            return TRUE;
        }
    }
    g_isNT4 = FALSE;
    if (vi->dwPlatformId == VER_PLATFORM_WIN32_NT && vi->dwMajorVersion == 5) g_isWin2000 = TRUE;
    else                                                                      g_isWin2000 = FALSE;
    return TRUE;
}

/* 6900b3f0 -- 3dfx __wglGetOS (WGLCMDS.C), extended with the NT4 / Windows 2000 flags */
BOOL wglGetOSVersion(void)
{
    OSVERSIONINFOA vi;

    vi.dwOSVersionInfoSize = sizeof(vi);
    if (!GetVersionExA(&vi)) return FALSE;
    return wglSetOS(&vi);
}

/* ---------------------------------------------------------------- locking */
/* 6900c900 -- wglLockCreate: take a reference on the wgl lock; the first one initialises g_wglLock */
void wglLockCreate(void)  { if (++g_wglLockRefs == 1) InitializeCriticalSection(&g_wglLock); } /* 6900c900 */
/* 6900c920 -- wglLockDestroy: drop a reference on the wgl lock; the last one deletes g_wglLock */
void wglLockDestroy(void) { if (--g_wglLockRefs == 0) DeleteCriticalSection(&g_wglLock); }     /* 6900c920 */
/* 6900c940 -- wglLock: enter the wgl lock (g_wglLock) */
void wglLock(void)        { EnterCriticalSection(&g_wglLock); }                                /* 6900c940 */
/* 6900c950 -- wglUnlock: leave the wgl lock (g_wglLock) */
void wglUnlock(void)      { LeaveCriticalSection(&g_wglLock); }                                /* 6900c950 */

/* 6900c960 -- a critical section followed by a "valid" dword (cs+0x18 = 1) */
LPCRITICAL_SECTION wglNewCritSect(void)
{
    LPCRITICAL_SECTION cs = (LPCRITICAL_SECTION)wglMalloc(sizeof(CRITICAL_SECTION) + 4);
    if (cs == NULL) return NULL;
    InitializeCriticalSection(cs);
    *(DWORD *)(cs + 1) = 1;
    return cs;
}

/* 6900c990 -- wglDeleteCritSect: delete a critical section from wglNewCritSect and free its block */
void __fastcall wglDeleteCritSect(LPCRITICAL_SECTION cs)
{
    DeleteCriticalSection(cs);
    wglFree(cs);
}

/* ------------------------------------------------- per-thread current gc */
/*
** The current context pointer is read straight out of the TEB TLS slot:
** fs:[g_tebOffContext], where g_tebOffContext = tlsIndex*4 + 0xe10 (NT) or
** + 0x88 (Win9x).  See wglProcessAttach.
*/
static __inline __GLcontext *wglTebRead(void)
{
    /* the MS-kit form: TEB self pointer + byte offset, the global referenced from the asm (a
    ** parameter would get a stack home and an extra load) */
    __asm { mov eax, fs:[18h] }
    __asm { add eax, g_tebOffContext }
    __asm { mov eax, [eax] }
}
/* wglTebWrite: store gc in this thread's TEB TLS context slot (TEB self pointer + g_tebOffContext); inlined */
static __inline void wglTebWrite(__GLcontext *gc)
{
    __asm { mov eax, fs:[18h] }
    __asm { add eax, g_tebOffContext }
    __asm { mov ecx, gc }
    __asm { mov [eax], ecx }
}

/* 6900af30 -- wglGetCurrentGC: the current gc from the TEB TLS slot; a thread without one is attached first
** (wglAttachThread, which installs the null gc) */
__GLcontext *wglGetCurrentGC(void)
{
    __GLcontext *gc = wglTebRead();
    if (gc == NULL) {
        wglAttachThread(g_hInstance);
        gc = wglTebRead();
    }
    return gc;
}

/* 6900af60 -- wglSetCurrentGC: make gc current on this thread: the TEB TLS slot only (wglTebWrite) */
void __fastcall wglSetCurrentGC(__GLcontext *gc)
{
    wglTebWrite(gc);
}

/* ------------------------------------------------------------ thread list */
/* 6900b480 -- wglThreadListInit: empty the thread list (its head points back at the sentinel, which ends it) */
void wglThreadListInit(void)
{
    s_threadHead.prev = NULL;
    g_threadList = (__WGLthread *)&s_threadHead;
}

/* 6900b0a0 -- first real node or NULL */
__WGLthread *wglThreadListFirst(void)
{
    return (g_threadList != (__WGLthread *)&s_threadHead) ? g_threadList : NULL;
}

/* 6900b0c0 -- wglThreadListAdd: under the wgl lock, push t at the head of the thread list */
void __fastcall wglThreadListAdd(__WGLthread *t)
{
    wglLock();
    t->next = g_threadList;
    g_threadList = t;
    wglUnlock();
}

/* 6900b120 -- wglThreadListUnlink: under the wgl lock, unlink t from the thread list (singly linked, ending at
** the sentinel); t is not freed */
void __fastcall wglThreadListUnlink(__WGLthread *t)
{
    __WGLthread *prev;

    wglLock();
    for (prev = (__WGLthread *)&s_threadHead; prev->next != (__WGLthread *)&s_threadHead; prev = prev->next) {
        if (prev->next == t) { prev->next = t->next; break; }
    }
    wglUnlock();
}

/* 6900b160 -- wglThreadDetach: clear this thread's context TLS slot, reset the device's pixel formats, call the
** device's threadDetach (FALSE from it aborts here), free t and clear the thread TLS slot */
BOOL __fastcall wglThreadDetach(__WGLthread *t)
{
    TlsGetValue(g_tlsContext);
    TlsSetValue(g_tlsContext, NULL);
    g_device->resetPixelFormats();
    if (g_device->threadDetach != NULL) {
                if (!g_device->threadDetach(GetCurrentThreadId())) return FALSE;
    }
    if (t != NULL) wglFree(t);
    TlsSetValue(g_tlsThread, NULL);
    return TRUE;
}

/* 6900b0e0 -- wglThreadListRemoveCurrent: stand-alone: TRUE, nothing done; else unlink and wglThreadDetach this
** thread's record (TLS); wglThreadDetach's result */
BOOL __fastcall wglThreadListRemoveCurrent(HINSTANCE hinst)   /* hinst: only passed on */
{
    __WGLthread *t = (__WGLthread *)TlsGetValue(g_tlsThread);
    if (wglIsStandAlone(hinst)) return TRUE;
    wglThreadListUnlink(t);
    return wglThreadDetach(t) != 0;
}

/* 6900af8d -- "attachThread": make sure this thread has a __WGLthread and a current gc */
BOOL __fastcall wglAttachThread(HINSTANCE hinst)      /* ECX = g_hInstance at every call site; only passed on */
{
    __WGLthread *t;

    if (TlsGetValue(g_tlsThread) != NULL) {
        if (TlsGetValue(g_tlsContext) == NULL) {
            wglSetCurrentGC(g_nullGC);
            __glInstallNullDispatch();                          /* install null dispatch */
            return TRUE;
        }
    } else {
        if (!wglIsStandAlone(hinst) || (t = wglThreadListFirst()) == NULL) {
            t = (__WGLthread *)wglCalloc(1, 8);
            if (t == NULL) wglSystemError("attachThread", ERROR_NOT_ENOUGH_MEMORY);
            wglThreadListAdd(t);
        }
        TlsSetValue(g_tlsThread, t);
        if (wglIsStandAlone(hinst)) {
            wglSetCurrentGC(g_nullGC);
            return TRUE;
        }
        if (TlsGetValue(g_tlsContext) == NULL) {
            wglSetCurrentGC(g_nullGC);
            __glInstallNullDispatch();
        }
        if (g_device->threadAttach != NULL && !g_device->threadAttach(GetCurrentThreadId())) {
            wglThreadListRemoveCurrent(hinst);
            return FALSE;
        }
        g_device->resetPixelFormats();
    }
    return TRUE;
}

/* 6900ae12 -- "attachMessageHook": once per thread record (attached first if needed), install the WH_CALLWNDPROC
** hook wglMonitorWindowChanges on the thread that owns hwnd (on Win9x: its top-level ancestor) and keep it in
** t->hook; FALSE with a system error when SetWindowsHookEx fails */
BOOL __fastcall wglAttachMessageHook(HWND hwnd)
{
    __WGLthread *t;
    HHOOK hook;

    t = (__WGLthread *)TlsGetValue(g_tlsThread);
    if (t == NULL) {
        wglAttachThread(g_hInstance);
        t = (__WGLthread *)TlsGetValue(g_tlsThread);
    }
    if (t->hook == NULL) {
        if (g_isWin9x) {                      /* hook the top-level ancestor on 9x */
            HWND parent;
            while ((parent = GetParent(hwnd)) != NULL) hwnd = parent;
        }
        hook = SetWindowsHookExA(WH_CALLWNDPROC, wglMonitorWindowChanges, g_hInstance,
                                 GetWindowThreadProcessId(hwnd, NULL));
        if (hook == NULL) {
            wglSystemError("attachMessageHook", 0x45a);
            return FALSE;
        }
        t->hook = hook;
    }
    return TRUE;
}

/* 6900aed2 -- "detachMessageHook": UnhookWindowsHookEx this thread's hook, if any, and clear it; FALSE with a
** system error when that fails */
BOOL wglDetachMessageHook(void)
{
    __WGLthread *t = (__WGLthread *)TlsGetValue(g_tlsThread);
    if (t != NULL && t->hook != NULL) {
        if (!UnhookWindowsHookEx(t->hook)) {
            wglSystemError("detachMessageHook", 0x45a);
            return FALSE;
        }
        t->hook = NULL;
    }
    return TRUE;
}

/* --------------------------------------------------------------- rc list */
/* 69009ea0 -- wglRcFromHandle: under the wgl lock, the rc with that handle or NULL; the walk starts at the
** sentinel, so handle 0 finds the sentinel rc (whose gc is the null gc) */
__WGLrc *__fastcall wglRcFromHandle(DWORD handle)
{
    __WGLrc *rc;
    wglLock();
    for (rc = (__WGLrc *)&s_rcHead; rc != NULL; rc = rc->next) {
        if (rc->handle == handle) { wglUnlock(); return rc; }
    }
    wglUnlock();
    return NULL;
}

/* 69009e60 -- wglRcListAdd: push rc at the head of the rc list (prev = the sentinel); caller holds the wgl lock */
void __fastcall wglRcListAdd(__WGLrc *rc)
{
    rc->next = g_rcList;
    rc->prev = (__WGLrc *)&s_rcHead;
    if (g_rcList != NULL) g_rcList->prev = rc;
    g_rcList = rc;
}

/* 69009e80 -- wglRcListRemove: unlink rc from the rc list (no lock taken; rc's own links are left) */
void __fastcall wglRcListRemove(__WGLrc *rc)
{
    if (rc->prev != NULL) rc->prev->next = rc->next;
    if (rc->next != NULL) rc->next->prev = rc->prev;
}

/* 69009f20 -- next rc bound to hwnd, starting after 'after' (NULL = from head) */
__WGLrc *__fastcall wglRcFromHwnd(HWND hwnd, __WGLrc *after)
{
    __WGLrc *rc;
    wglLock();
    rc = (after == NULL) ? (__WGLrc *)&s_rcHead : after->next;
    for (; rc != NULL; rc = rc->next) {
        if (rc->hwnd == hwnd) { wglUnlock(); return rc; }
    }
    wglUnlock();
    return NULL;
}

/* ------------------------------------------------------------ window list */
/* 69009f60 -- wglWindowListAdd: under the wgl lock, push w at the head of the window list */
void __fastcall wglWindowListAdd(__WGLwindow *w)
{
    wglLock();
    w->next = g_windowList;
    w->prev = (__WGLwindow *)&s_windowHead;
    if (g_windowList != NULL) g_windowList->prev = w;
    g_windowList = w;
    wglUnlock();
}

/* 69009f90 -- wglWindowListRemove: under the wgl lock, unlink w from the window list */
void __fastcall wglWindowListRemove(__WGLwindow *w)
{
    wglLock();
    w->prev->next = w->next;
    if (w->next != NULL) w->next->prev = w->prev;
    wglUnlock();
}

/* 6900a2b0 -- find window by hdc (memory DC) or hwnd */
__WGLwindow *__fastcall wglFindWindow(HDC hdc, HWND hwnd)
{
    __WGLwindow *w;
    wglLock();
    for (w = (__WGLwindow *)&s_windowHead; w != NULL; w = w->next) {
        if (w->hwnd == NULL || hwnd == NULL) {
            if (w->hdc == hdc) { wglUnlock(); return w; }
        } else if (w->hwnd == hwnd) {
            wglUnlock(); return w;
        }
    }
    wglUnlock();
    return NULL;
}

/* 6900a0d0 -- wglCreateWindow: drop a window record already on this DC, then allocate a zeroed one for hdc /
** hwnd: a copy of the modes, its own lock (wglNewCritSect), the wgl allocators and drawable procs, a swap
** region (RGNDATAHEADER with room for 4 rects), the device's destroy / paletteChanged callbacks, and the device's
** initWindow (hwnd) or initBitmap (memory DC: pixmapMode 1).  Linked into the window list with refCount 1 and
** the message hook attached (wglAttachMessageHook(hwnd), also for a NULL hwnd); no allocation is checked */
__WGLwindow *__fastcall wglCreateWindow(HDC hdc, HWND hwnd, const __WGLmodes *modes)
{
    __WGLwindow *w;
    DWORD *rects;
    __WGLdrawable *glPriv;

    w = wglFindWindow(hdc, NULL);                                /* (sic: by DC only) */
    if (w != NULL) wglDestroyWindow(w);

    w = (__WGLwindow *)wglMalloc(sizeof(__WGLwindow));           /* 0xa1 dwords */
    memset(w, 0, sizeof(__WGLwindow));
    w->hdc  = hdc;
    w->hwnd = hwnd;
    w->dp.childWindow = (GetParent(hwnd) != NULL);
    glPriv = &w->dp;
    glPriv->modes = (__WGLmodes *)wglMalloc(sizeof(__WGLmodes));
    glPriv->lock  = wglNewCritSect();
    memcpy(glPriv->modes, modes, sizeof(__WGLmodes));
    glPriv->malloc  = wglMalloc;
    glPriv->calloc  = wglCalloc;
    glPriv->realloc = wglRealloc;
    glPriv->free    = wglFree;
    glPriv->addSwapRect = wglDrawAddSwapRect;
    glPriv->setClipRect = wglDrawSetClipRect;
    glPriv->lockDP   = wglDrawLockDP;
    glPriv->unlockDP = wglDrawUnlockDP;
    glPriv->window   = w;
    glPriv->regionValid = 1;
    w->swapRectCapacity = 4;
    rects = (DWORD *)wglMalloc(0x64);                            /* RGNDATAHEADER + room for the 4 rects of swapRectCapacity */
    w->swapRegion = (__WGLswapRegion *)rects;
    rects[0] = 0x20; rects[1] = 1;
    rects[2] = 0; rects[3] = 0; rects[7] = 0; rects[5] = 0; rects[6] = 0; rects[4] = 0;
    w->destroy        = g_device->destroyWindow;
    w->paletteChanged = g_device->paletteChanged;
    if (hwnd != NULL) {
        w->dp.modes->pixmapMode = 0;
        g_device->initWindow(w, modes);
    } else {
        w->dp.modes->pixmapMode = 1;                 /* memory DC */
        g_device->initBitmap(w, modes);
    }
    wglWindowListAdd(w);
    w->dp.refCount = 1;
    wglAttachMessageHook(hwnd);
    return w;
}

/* 6900a650 -- wglGetWindow: under the wgl lock, the window record of hdc / hwnd, created (wglCreateWindow) with
** these modes when there is none */
__WGLwindow *__fastcall wglGetWindow(HDC hdc, HWND hwnd, const __WGLmodes *modes)
{
    __WGLwindow *w;
    wglLock();
    w = wglFindWindow(hdc, hwnd);
    if (w == NULL) w = wglCreateWindow(hdc, hwnd, modes);
    wglUnlock();
    return w;
}

/* 6900a240 -- drop a reference; TRUE when the window was actually freed */
GLboolean __fastcall wglDestroyWindow(__WGLwindow *w)
{
    __WGLdrawable *dp = &w->dp;
    if (--w->dp.refCount > 0) return FALSE;
    wglDetachMessageHook();
    wglWindowListRemove(w);
    if (dp->freeBuffers != NULL) dp->freeBuffers(dp);
    if (w->destroy != NULL) w->destroy(w);
    wglFree(w->swapRegion);
    wglFree(dp->modes);
    wglFree(w->dp.rects);                         /* the ownership region; the lock is the caller's */
    wglFree(w);
    return TRUE;
}

/* 6900b530 -- process detach: destroy every window */
void wglDestroyAllWindows(void)
{
    __WGLwindow *w;
    wglLock();
    for (w = g_windowList; w != NULL; w = g_windowList) {
        LPCRITICAL_SECTION cs = w->dp.lock;
        EnterCriticalSection(cs);
        if (wglDestroyWindow(w)) {
            LeaveCriticalSection(cs);
            wglDeleteCritSect(cs);
        } else {
            LeaveCriticalSection(cs);
        }
    }
    wglUnlock();
}

/* 6900a4d0 -- refresh clientOrigin / clientRect / width / height from USER */
void __fastcall wglUpdateWindowRect(__WGLwindow *w)
{
    HWND hwnd;
    if (w == NULL) return;
    hwnd = w->hwnd;
    if (hwnd == NULL) {
        w->clientOrigin.x = w->clientOrigin.y = 0;
        w->width = w->height = 0;
    } else {
        w->clientOrigin.x = w->clientOrigin.y = 0;
        ClientToScreen(hwnd, &w->clientOrigin);
        GetClientRect(hwnd, &w->clientRect);
        w->clientRect.left   += w->clientOrigin.x;
        w->clientRect.top    += w->clientOrigin.y;
        w->clientRect.right  += w->clientOrigin.x;
        w->clientRect.bottom += w->clientOrigin.y;
        w->width  = w->clientRect.right  - w->clientRect.left;
        w->height = w->clientRect.bottom - w->clientRect.top;
    }
    if (w->height == 0 && w->width == 0) {
        w->clientRect.left = 0;
        w->clientRect.right = 0;
        w->clientRect.top = 0;
        w->clientRect.bottom = 0;
    }
}

/* 6900a570 -- TRUE if nothing changed, else reallocate buffers for the new size */
GLboolean __fastcall wglWindowResized(__WGLwindow *w)
{
    GLboolean ret = GL_TRUE;
    __WGLdrawable *dp = &w->dp;
    GLint x, y;
    GLuint width, height;

    wglUpdateWindowRect(w);
    w->dp.fullScreen = (GLboolean)wglIsFullScreenWindow(w);
    x = w->clientOrigin.x;
    width = w->width;
    height = w->height;
    y = w->clientOrigin.y;
    if (x != dp->xOrigin || y != dp->yOrigin || width != dp->width || height != dp->height || (width == 0 && height == 0)) {
        dp->height  = height;
        dp->width   = width;
        dp->xOrigin = x;
        dp->yOrigin = y;
        ret = wglDrawResize(dp, x, y, width, height);
    }
    return ret;
}

/* 6900a3bc -- is the GL window covering the whole desktop (or forced)? */
GLboolean __fastcall wglIsFullScreenWindow(__WGLwindow *w)
{
    LONG style, exstyle;
    HMODULE hmod;
    HWND hwnd = w->hwnd;
    RECT client, desktop;
    CHAR module[256];
    char msg[256];

    style   = GetWindowLongA(hwnd, GWL_STYLE);
    exstyle = GetWindowLongA(hwnd, GWL_EXSTYLE);
    hmod    = (HMODULE)GetWindowLongA(hwnd, GWL_HINSTANCE);
    GetModuleFileNameA(hmod, module, 0xff);
    if (strstr(module, ".SCR") != NULL || strstr(module, ".scr") != NULL)
        return FALSE;                                   /* screen savers: never fullscreen */
    if (getenv("GL_SHOW_WINDOW_STYLES") != NULL) {
        sprintf(msg, "Style %x ExtStyle %x", (unsigned)style, (unsigned)exstyle);
        MessageBoxA(NULL, msg, "Style", MB_OK);
    }
    if (style == (LONG)0x968C0000) return FALSE;        /* WS_OVERLAPPEDWINDOW|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN -> plain window */
    GetClientRect(hwnd, &client);
    GetWindowRect(GetDesktopWindow(), &desktop);
    if (memcmp(&client, &desktop, sizeof(RECT)) != 0 && getenv("GL_FORCE_FULLSCREEN") == NULL)
        return FALSE;
    return TRUE;
}

/* 6900a6ad -- PIXELFORMATDESCRIPTOR -> __WGLmodes */
void __fastcall wglModesFromPFD(__WGLmodes *m, const PIXELFORMATDESCRIPTOR *pfd)
{
    memset(m, 0, sizeof(*m));
    m->rgbMode           = (pfd->iPixelType == PFD_TYPE_RGBA);
    m->colorIndexMode    = (pfd->iPixelType == PFD_TYPE_COLORINDEX);
    m->doubleBufferMode  = (GLboolean)(pfd->dwFlags & PFD_DOUBLEBUFFER);
    m->stereoMode        = (GLboolean)(pfd->dwFlags & PFD_STEREO);
    m->haveAccumBuffer   = (pfd->cAccumBits != 0);
    m->haveDepthBuffer   = (pfd->cDepthBits != 0);
    m->haveStencilBuffer = (pfd->cStencilBits != 0);
    m->redBits   = pfd->cRedBits;
    m->greenBits = pfd->cGreenBits;
    m->blueBits  = pfd->cBlueBits;
    m->alphaBits = pfd->cAlphaBits;
    m->redMask   = ((1 << pfd->cRedBits)   - 1) << pfd->cRedShift;
    m->greenMask = ((1 << pfd->cGreenBits) - 1) << pfd->cGreenShift;
    m->blueMask  = ((1 << pfd->cBlueBits)  - 1) << pfd->cBlueShift;
    m->alphaMask = ((1 << pfd->cAlphaBits) - 1) << pfd->cAlphaShift;
    m->rgbBits   = pfd->cColorBits;
    m->indexBits = pfd->cColorBits;
    m->accumRedBits   = pfd->cAccumRedBits;
    m->accumGreenBits = pfd->cAccumGreenBits;
    m->accumBlueBits  = pfd->cAccumBlueBits;
    m->accumAlphaBits = pfd->cAccumAlphaBits;
    if (getenv("GLFORCEZBITS") != NULL) m->depthBits = atoi(getenv("GLFORCEZBITS"));
    else                                m->depthBits = pfd->cDepthBits;
    m->stencilBits   = pfd->cStencilBits;
    m->numAuxBuffers = 0;
    m->level         = 0;
    m->singleBuffered = (m->doubleBufferMode == 0);
}

/* 6900a800 -- allocate modes + core gc for an rc */
GLboolean __fastcall wglRcCreateGC(__WGLrc *rc, int layerPlane)
{
    __WGLmodes *m = (__WGLmodes *)wglMalloc(sizeof(__WGLmodes));
    rc->modes = m;
    wglModesFromPFD(m, &rc->pfd);
    /* 690ee3b8 = g_wglImports + 0x20, the imports block's wscx slot: createContext copies the whole block
    ** into the gc, which is how gc->imports.wscx gets the rc.  (A separate g_lastRc variable left it NULL:
    ** Quake 2 crashed in wglDevMakeCurrent on rc->window.) */
    g_wglImports[8] = rc;
    m->layerPlane = (GLboolean)layerPlane;
    rc->gc = g_device->createContext(g_wglImports, m);     /* ecx = 690ee398 g_wglImports, edx = modes (checked) */
    return rc->gc != NULL;
}

/* 6900a850 -- gc->exports.destroyContext() then free modes */
GLboolean __fastcall wglRcDestroyGC(__WGLrc *rc)
{
    if (!GC_exports_destroyContext(rc->gc)(rc->gc)) return FALSE;
    wglFree(rc->modes);
    return TRUE;
}

/* raw TEB read used by the stand-alone binder (no implicit attach) */
__GLcontext *wglTebReadContext(void)
{
    return wglTebRead();
}

/* the sentinel head node is itself walked by some loops (hook, flag clears) */

/* list accessors for other files (sentinel-based lists, see top of file) */

/* ------------------------------------------------------------------------
** The __GLimports block handed to the core: MS-kit layout, gc is the first
** argument of every entry (69009710 .. 69009800).  g_wglImports is the static
** copy the device copies into each new gc (and __glCreateContext into the null gc).
*/
static void *__fastcall impMalloc(__GLcontext *gc, size_t n)              /* 69009710 */
{ (void)gc; if (n == 0) return NULL; return GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, n); }
/* 69009730 -- impCalloc: the imports' calloc: a zeroed GlobalAlloc block of n * sz bytes, NULL when either is 0 */
static void *__fastcall impCalloc(__GLcontext *gc, size_t n, size_t sz)   /* 69009730 */
{ (void)gc; return (n && sz) ? GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, n * sz) : NULL; }
/* 69009750 -- impRealloc: the imports' realloc: GlobalAlloc a zeroed block of n bytes, copy the old contents
** (up to n) and free p; n == 0 frees p and returns NULL; a failed allocation returns NULL and leaves p alone */
static void *__fastcall impRealloc(__GLcontext *gc, void *p, size_t n)    /* 69009750 */
{
    /* wglRealloc's body (69009690) written out again, not a call */
    void *q = NULL;
    SIZE_T old;
    (void)gc;
    if (n != 0) {
        q = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, n);
        if (p != NULL && q != NULL) {
            old = GlobalSize(p);
            if (old > n) old = n;
            memcpy(q, p, old);
            GlobalFree(p);
        }
    } else if (p != NULL) {
        GlobalFree(p);
        return NULL;
    }
    if (q == NULL) return NULL;
    return q;
}
/* 690097c0 -- impFree: the imports' free: GlobalFree p unless it is NULL */
static void  __fastcall impFree(__GLcontext *gc, void *p)                 /* 690097c0 */
{ (void)gc; if (p) GlobalFree(p); }
/* 690097d0 -- impWarning: the imports' warning: wglOglError(msg) */
static void  __fastcall impWarning(__GLcontext *gc, const char *msg)      /* 690097d0 */
{ (void)gc; wglOglError(msg); }
/* 690097e0 -- impFatal: the imports' fatal error: wglOglError(msg), then abort() */
static void  __fastcall impFatal(__GLcontext *gc, const char *msg)        /* 690097e0 */
{ (void)gc; wglOglError(msg); abort(); }
/* 690097f0 -- impGetDrawablePrivate: &rc->window->dp of the gc's rc (the window is not NULL-checked) */
static __WGLdrawable *__fastcall impGetDrawablePrivate(__GLcontext *gc)   /* 690097f0 */
{ return &GC_imports_wscx(gc)->window->dp; }
/* 69009800 -- impGetDrawableSize: refresh the rc's window rect and return its client origin and size (GL_TRUE);
** all zero and GL_FALSE when the rc has no window */
static GLboolean __fastcall impGetDrawableSize(__GLcontext *gc, GLint *x, GLint *y, GLuint *w, GLuint *h)  /* 69009800 */
{
    __WGLwindow *win = GC_imports_wscx(gc)->window;
    if (win != NULL) {
        wglUpdateWindowRect(win);
        *x = win->clientOrigin.x; *y = win->clientOrigin.y;
        *w = win->width; *h = win->height;
        return GL_TRUE;
    }
    *h = 0; *w = 0; *y = 0; *x = 0;
    return GL_FALSE;
}
void *g_wglImports[9] = {                                                 /* 690ee398 */
    (void *)impMalloc, (void *)impCalloc, (void *)impRealloc, (void *)impFree, (void *)impWarning,
    (void *)impFatal, (void *)impGetDrawablePrivate, (void *)impGetDrawableSize, NULL /* +0x20 wscx (g_lastRc): the rc being created */ };

/* 6900a300 / 6900a330 sit between the wglutil functions in the original: this file */
/* 6900a300 -- clear byte 0x23c ("buffers valid"?) on every window after a mode change */
void wglClearAllWindowFlags(void)
{
    __WGLwindow *w;
    wglLock();
    for (w = (__WGLwindow *)&s_windowHead; w != NULL; w = w->next)    /* (sic: starts at the sentinel) */
        w->dp.regionValid = 0;
    wglUnlock();
}

/* 6900a330 -- desktop size: wglGetDesktopSize: *w, *h = the desktop window rect's right and bottom */
void __fastcall wglGetDesktopSize(LONG *w, LONG *h)
{
    RECT r;
    GetWindowRect(GetDesktopWindow(), &r);
    *w = r.right;
    *h = r.bottom;
}

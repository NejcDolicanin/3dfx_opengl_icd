/*
** dllmain.c -- DllMain and process/thread attach of 3dfxOGL.dll 1.0.0.0761.
**
** The DLL has no CRT start-up: DllMain *is* the PE entry point (link /ENTRY:DllMain).
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wglpriv.h"

/* 6900adf0 -- wglDebugPrintf: debug trace; compiled out in the release build (single `ret`) */
void __cdecl wglDebugPrintf(const char *fmt, ...) { (void)fmt; }

/* 6900c8d0 -- wglUnloadStandAlone: FreeLibrary the stand-alone GL module if one is loaded and clear it; TRUE */
BOOL wglUnloadStandAlone(void)
{
    if (g_hStandAloneGL != NULL) {
        FreeLibrary(g_hStandAloneGL);
        g_hStandAloneGL = NULL;
    }
    return TRUE;
}

/* 6900c8f0 -- wglIsStandAlone: is a stand-alone GL module loaded (g_hStandAloneGL non-NULL)?  hinst is unused */
BOOL __fastcall wglIsStandAlone(HINSTANCE hinst)
{
    (void)hinst;
    return g_hStandAloneGL != NULL;
}

/*
** Stand-alone mode (the DLL dropped into a game directory as opengl32.dll):
** the 336 gl* and 19 wgl* forwarding pointers are re-bound by name.
** Name tables: 690ee3c4 (gl, 0x150 entries) and 690ee978 (wgl, 19 entries);
** targets: current gc dispatch (gc+0x173c) and the wgl pointer table 690ee92c.
*/
extern const char *g_glEntryNames[0x150];     /* 690ee3c4  "glNewList", ... */
extern const char *g_wglEntryNames[19];       /* 690ee978  "wglCreateContext", ... */
extern FARPROC     g_wglEntryPoints[19];      /* 690ee92c  defaults = own wgl implementations */

/* 6900c830 -- wglBindGLEntryPoints: GetProcAddress each of the 0x150 g_glEntryNames in module h into the current
** gc's dispatch table (gc+0x173c, gc read from the TEB); GL_FALSE at the first missing name, the entries before it
** already overwritten */
GLboolean __fastcall wglBindGLEntryPoints(HMODULE h)
{
    FARPROC *disp = (FARPROC *)((BYTE *)__glCurrentGC() + 0x173c);   /* the inline TEB read */
    int i;
    for (i = 0; i < 0x150; i++) {
        FARPROC p = GetProcAddress(h, g_glEntryNames[i]);
        if (p == NULL) return GL_FALSE;
        disp[i] = p;
    }
    return GL_TRUE;
}

/* 6900c890 -- wglBindWGLEntryPoints: GetProcAddress each of the 19 g_wglEntryNames in module h into
** g_wglEntryPoints; GL_FALSE at the first missing name, the entries before it already overwritten */
GLboolean __fastcall wglBindWGLEntryPoints(HMODULE h)
{
    int i;
    for (i = 0; i < 19; i++) {
        FARPROC p = GetProcAddress(h, g_wglEntryNames[i]);
        if (p == NULL) return GL_FALSE;
        g_wglEntryPoints[i] = p;
    }
    return GL_TRUE;
}

BOOL __fastcall wglHasHardwareAcceleration(HINSTANCE hInstance);   /* 6900c727, below */

/* 6900c5e7 -- wglInitializeDispatch (3dfx WGLCMDS.C __wglInitializeDispatch, without its __wglSetupMSDispatch):
** choose whether to hand GL to Microsoft's OPENGL32.DLL.  The mode is GLinfo "OverrideDispatch", overridden by
** the environment's __GL_OVERRIDE_DISPATCH (0 = only when OPENGL32 is hardware accelerated, 1 = never, 2 = always;
** out of range = 0); a DLL renamed to opengl32.dll never forwards.  Modes 0 / 2 load OPENGL32.DLL into
** g_hStandAloneGL (mode 0 drops it again unless it is accelerated).  FALSE when GetModuleFileName fails or mode 2
** cannot load OPENGL32.DLL (after a message box), else TRUE; ToDo UNUSED CODE: nothing in the driver calls it */
BOOL wglInitializeDispatch(HINSTANCE hInst)
{
    DWORD dispatch = wglReadGLinfo("OverrideDispatch");
    char *env = getenv("__GL_OVERRIDE_DISPATCH");

    if (env != NULL)
        dispatch = atoi(env);
    if (dispatch > 2)
        dispatch = 0;

    if (dispatch != 1) {
        char name[256];
        char *ptr;
        int len;

        len = GetModuleFileNameA(hInst, name, 255);
        if (len == 0)
            return FALSE;
        ptr = name + len - 1;
        while (ptr > name && *ptr != '\\')
            ptr--;
        if (*ptr == '\\')
            ptr++;

        /* renamed to opengl32.dll: do not forward to ourselves */
        if (_stricmp(ptr, "opengl32.dll") == 0)
            dispatch = 1;
    }

    switch (dispatch) {
    case 0:
        /* use OPENGL32 only if it is hardware accelerated */
        g_hStandAloneGL = LoadLibraryA("OPENGL32.DLL");
        if (g_hStandAloneGL != NULL && !wglHasHardwareAcceleration(g_hStandAloneGL)) {
            FreeLibrary(g_hStandAloneGL);
            g_hStandAloneGL = NULL;
        }
        break;
    case 2:
        /* OPENGL32 or die */
        g_hStandAloneGL = LoadLibraryA("OPENGL32.DLL");
        if (g_hStandAloneGL == NULL) {
            MessageBoxA(NULL, "A required .DLL file, OPENGL32.DLL, was not found.", "Error Starting Program",
                        MB_ICONEXCLAMATION | MB_SYSTEMMODAL | MB_OK);
            return FALSE;
        }
        break;
    }
    return TRUE;
}

/* 6900c727 -- __wglHasHardwareAcceleration (3dfx WGLCMDS.C): does the given OpenGL DLL describe a hardware pixel
** format?  PFD_GENERIC_FORMAT and PFD_GENERIC_ACCELERATED both clear = an ICD, both set = an MCD.  Unreferenced in
** the original (dead code, dropped by the linker). */
BOOL __fastcall wglHasHardwareAcceleration(HINSTANCE hInstance)
{
    HDC hDC;
    int ii, count;
    int (WINAPI *Describe)(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);

    Describe = (int (WINAPI *)(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR))GetProcAddress(hInstance, "wglDescribePixelFormat");
    if (NULL == Describe)
        return FALSE;

    hDC = GetDC(0);
    count = (*Describe)(hDC, 0, 0, NULL);

    for (ii = 1; ii <= count; ii++) {
        PIXELFORMATDESCRIPTOR pfd;

        if ((*Describe)(hDC, ii, sizeof(pfd), &pfd)) {
            BOOL genericFormat = (pfd.dwFlags & PFD_GENERIC_FORMAT) != 0;
            BOOL genericAccel = (pfd.dwFlags & PFD_GENERIC_ACCELERATED) != 0;

            if (genericFormat == genericAccel) {
                ReleaseDC(0, hDC);
                return TRUE;
            }
        }
    }

    ReleaseDC(0, hDC);
    return FALSE;
}

/* 6900c7ea -- bind the gl* / wgl* forwarders to the stand-alone GL module (Gdi32 must be loadable) */
static BOOL __fastcall wglBindStandAloneModule(HMODULE h)
{
    if (LoadLibraryA("Gdi32.dll") == NULL) return FALSE;
    if (!wglBindGLEntryPoints(h)) return FALSE;
    return wglBindWGLEntryPoints(h) != 0;
}

/* 6900c7c0 (tail-jumps to 6900c7ea) */
BOOL wglBindStandAlone(HINSTANCE hinst)
{
    (void)hinst;
    if (g_hStandAloneGL != NULL) return wglBindStandAloneModule(g_hStandAloneGL);
    return TRUE;
}

/* 69009d70 -- the "no context" gc every thread starts with */
void wglCreateNullContext(void)
{
    __WGLmodes nullModes;                      /* left uninitialised, as in the original */
    wglLock();
    g_nullGC = (__GLcontext *)__glCreateContext(g_wglImports, &nullModes);
    GC_imports_wscx(g_nullGC) = &s_rcHead;                          /* the rc list head (690f6568) */
    s_nullGCCopy = g_nullGC;                                       /* 690f6570 */
    wglUnlock();
}

/* 6900b2e0 -- DLL_PROCESS_ATTACH: wglProcessAttach: record the instance, read the OS version (FALSE if that fails),
** create the wgl lock, read the error-log level, TlsAlloc the context and thread slots and compute their TEB offsets
** (Win9x TIB 0x88, NT TEB 0xe10), init the thread list, run the device's processAttach (on failure undo through
** wglProcessDetach and fail), create the null context, then bind the stand-alone module if one is loaded */
BOOL wglProcessAttach(HINSTANCE hinst)
{
    ICD_T("wglProcessAttach", hinst);
    g_hInstance = hinst;
    if (!wglGetOSVersion()) return FALSE;
    wglLockCreate();
    wglInitErrorLogLevel();
    g_tlsContext = TlsAlloc();
    g_tlsThread  = TlsAlloc();
    if (g_isWin9x) {
        g_tebOffContext = g_tlsContext * 4 + 0x88;     /* Win9x TIB: TlsSlots at 0x88 */
        g_tebOffThread  = g_tlsThread  * 4 + 0x88;
    } else {
        g_tebOffContext = g_tlsContext * 4 + 0xe10;    /* NT TEB: TlsSlots at 0xe10 */
        g_tebOffThread  = g_tlsThread  * 4 + 0xe10;
    }
    wglThreadListInit();
    ICD_T("wglProcessAttach: device attach", g_isWin9x);
    if (g_device->processAttach != NULL && !g_device->processAttach(GetCurrentThreadId())) {
        wglProcessDetach(hinst);
        return FALSE;
    }
    ICD_T("wglProcessAttach: null context", 0);
    wglCreateNullContext();
    ICD_T("wglProcessAttach: bind stand-alone", 0);
    return wglBindStandAlone(hinst) != 0;
}

/* 6900b4a0 -- DLL_PROCESS_DETACH: wglProcessDetach: free the stand-alone GL module, release all rcs, destroy all
** tracked windows, run the device's processDetach (FALSE if it fails), detach and unlink every listed thread, free
** both TLS slots and the wgl lock, and set g_processDetached */
BOOL wglProcessDetach(HINSTANCE hinst)
{
    __WGLthread *t;

    (void)hinst;
    ICD_T("wglProcessDetach glide owner gc", g_glideOwnerGC);
    ICD_T("  owner's Glide open", g_glideOwnerGC != NULL ? g_glideOwnerGC->fx.glideOpen : -1);
    ICD_T("  LFB locks held", ICD_LFB_N);
    if (!wglUnloadStandAlone()) return FALSE;
    wglReleaseAllRcs();                        /* 69009dd5 */
    ICD_T("wglProcessDetach: rcs released", 0);
    wglDestroyAllWindows();
    ICD_T("wglProcessDetach: windows destroyed", 0);
    if (g_device->processDetach != NULL && !g_device->processDetach(GetCurrentThreadId())) return FALSE;
    while ((t = wglThreadListFirst()) != NULL) {
        wglThreadListUnlink(t);
        wglThreadDetach(t);
    }
    TlsFree(g_tlsContext);
    TlsFree(g_tlsThread);
    wglLockDestroy();
    g_processDetached = TRUE;
    return TRUE;
}

/* 6900b24c -- PE entry point */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    ICD_T("DllMain reason", reason);
    if (reason == DLL_PROCESS_ATTACH) ICD_T("3dfxogl.dll 1.0.0.0761 RE-ICD, trace build", 0);
    switch (reason) {
    case DLL_PROCESS_ATTACH:
#ifdef ICD_MARK
        { extern LONG WINAPI icdExceptionFilter(EXCEPTION_POINTERS *); SetUnhandledExceptionFilter(icdExceptionFilter); icdHookGlide((HMODULE)hinst); }
#endif
        return wglProcessAttach(hinst);
    case DLL_PROCESS_DETACH:
#ifdef ICD_MARK
        icdMarkDump();
#endif
        wglDebugPrintf("DllMain( DLL_PROCESS_DETACH), thread %x\n", GetCurrentThreadId() & 0xffff);
        return wglProcessDetach(hinst);
    case DLL_THREAD_ATTACH:
        wglDebugPrintf("DllMain( DLL_THREAD_ATTACH), thread %x\n", GetCurrentThreadId() & 0xffff);
        /* FALLTHROUGH (as in the original) */
    case DLL_THREAD_DETACH:
        wglDebugPrintf("DllMain( DLL_THREAD_DETACH), thread %x\n", GetCurrentThreadId() & 0xffff);
        /* FALLTHROUGH */
    default:
        return TRUE;
    }
}

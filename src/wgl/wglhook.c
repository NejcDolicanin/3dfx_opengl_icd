/*
** wglhook.c -- "wglMonitorWindowChanges": WH_CALLWNDPROC hook installed per thread on
** the GL window (wglAttachMessageHook).  Original VAs 6900ab21..6900adef.
*/
#include <stdlib.h>
#include "wglpriv.h"
#include "icdprotos.h"

/* 3dfx Glide side: lose / restore the fullscreen Glide context (sst layer, wglglide.c) */

extern void __fastcall wglWindowPaletteChanged(__WGLwindow *w);               /* 6900a560 */
extern GLboolean __fastcall wglWindowResizedEx(__WGLwindow *w, BOOL haveThread);   /* 6900a5e0 */

/* display-mode cache compared against WM_DISPLAYCHANGE */
DWORD g_displayBpp;      /* 690f62d0 */
DWORD g_displayWidth;    /* 690f6564 */
DWORD g_displayHeight;   /* 690f655c */

/* 6900ab21 -- wglMonitorWindowChanges: the WH_CALLWNDPROC hook proc.  WM_DESTROY of a GL window: mark its rcs
** (flags |= 2) and drop the window reference (freed: delete its lock, else hwnd = NULL); WM_ACTIVATE: lose /
** restore the fullscreen Glide context on deactivate / activate (skipped when HAL_CSIM is set);
** WM_WINDOWPOSCHANGED: mark its rcs for revalidation (flags |= 1) and wglWindowResizedEx; WM_DISPLAYCHANGE to a new
** size or depth: cache it, reset the device's pixel formats and clear every window's regionValid;
** WM_PALETTECHANGED: the window's paletteChanged callback.  Always chains on with this thread's hook (NULL without
** a thread record) */
LRESULT CALLBACK wglMonitorWindowChanges(int code, WPARAM wParam, LPARAM lParam)
{
    CWPSTRUCT *msg = (CWPSTRUCT *)lParam;
    __WGLthread *t;
    HHOOK hook;
    BOOL haveThread;
    __WGLwindow *w;
    __WGLrc *rc;
    LPCRITICAL_SECTION cs;

    t = (__WGLthread *)TlsGetValue(g_tlsThread);
    if (t != NULL) { hook = t->hook; haveThread = TRUE; }
    else           { hook = NULL;    haveThread = FALSE; }

    switch (msg->message) {
    case WM_SHOWWINDOW:   ICD_T("hook WM_SHOWWINDOW shown", msg->wParam); ICD_T("  hwnd", msg->hwnd); break;
    case WM_SIZE:         ICD_T("hook WM_SIZE wParam", msg->wParam); break;
    case WM_ACTIVATEAPP:  ICD_T("hook WM_ACTIVATEAPP active", msg->wParam); break;
    case WM_DESTROY:
        ICD_T("hook WM_DESTROY hwnd", msg->hwnd);
        wglLock();
        w = wglFindWindow((HDC)-1, msg->hwnd);
        if (w == NULL) { wglUnlock(); break; }
        wglDebugPrintf(">> wglMonitorWindowChanges( WM_DESTROY), thread %x\n", GetCurrentThreadId() & 0xffff);
        wglCSEnter(w->dp.lock);
        for (rc = wglRcFromHwnd(msg->hwnd, NULL); rc != NULL; rc = wglRcFromHwnd(msg->hwnd, rc))
            rc->flags |= 2;                      /* window gone */
        cs = w->dp.lock;
        if (wglDestroyWindow(w)) {
            LeaveCriticalSection(cs);
            wglDeleteCritSect(cs);
        } else {
            LeaveCriticalSection(cs);
            w->hwnd = NULL;
        }
        wglUnlock();
        wglDebugPrintf("<< wglMonitorWindowChanges( WM_DESTROY), thread %x\n", GetCurrentThreadId() & 0xffff);
        break;

    case WM_ACTIVATE:
        ICD_T("hook WM_ACTIVATE wParam", msg->wParam); ICD_T("  hwnd", msg->hwnd);
        wglDebugPrintf(">> wglMonitorWindowChanges( WM_ACTIVATE), thread %x\n", GetCurrentThreadId() & 0xffff);
        wglDebugPrintf("In WM_ACTIVATE: wParam = 0x%x\n", msg->wParam);
        if (getenv("HAL_CSIM") == NULL) {
            if (LOWORD(msg->wParam) == WA_INACTIVE) {
                wglDebugPrintf("LOWORD(callBackMessage->wParam) == WA_INACTIVE\n");
                wglGlideLose(msg->hwnd);
            } else {
                wglDebugPrintf("LOWORD(callBackMessage->wParam) == WA_ACTIVE\n");
                wglGlideRestore(msg->hwnd);
            }
        }
        wglDebugPrintf("<< wglMonitorWindowChanges( WM_ACTIVATE), thread %x\n", GetCurrentThreadId() & 0xffff);
        break;

    case WM_WINDOWPOSCHANGED:
        wglLock();
        w = wglFindWindow((HDC)-1, msg->hwnd);
        if (w == NULL) { wglUnlock(); break; }
        wglDebugPrintf(">> wglMonitorWindowChanges( WM_WINDOWPOSCHANGED), thread %x\n", GetCurrentThreadId() & 0xffff);
        wglCSEnter(w->dp.lock);
        for (rc = wglRcFromHwnd(msg->hwnd, NULL); rc != NULL; rc = wglRcFromHwnd(msg->hwnd, rc))
            rc->flags |= 1;                      /* needs revalidation */
        wglWindowResizedEx(w, haveThread);
        wglCSLeave(w->dp.lock);
        wglUnlock();
        wglDebugPrintf("<< wglMonitorWindowChanges( WM_WINDOWPOSCHANGED), thread %x\n", GetCurrentThreadId() & 0xffff);
        break;

    case WM_DISPLAYCHANGE: {
        DWORD bpp = (DWORD)msg->wParam;
        DWORD width  = LOWORD(msg->lParam);
        DWORD height = HIWORD(msg->lParam);
        ICD_T("hook WM_DISPLAYCHANGE w<<16|h", (width << 16) | height); ICD_T("  bpp", bpp);
        if (bpp != g_displayBpp || width != g_displayWidth || height != g_displayHeight) {
            g_displayWidth  = width;
            g_displayHeight = height;
            g_displayBpp    = bpp;
            g_device->resetPixelFormats();
            wglClearAllWindowFlags();
        }
        break;
    }

    case WM_PALETTECHANGED:
        wglLock();
        w = wglFindWindow((HDC)-1, msg->hwnd);
        if (w != NULL) wglWindowPaletteChanged(w);
        wglUnlock();
        break;
    }
    return CallNextHookEx(hook, code, wParam, lParam);
}

/* 6900a560 -- forward WM_PALETTECHANGED to the device callback stored in the window */
void __fastcall wglWindowPaletteChanged(__WGLwindow *w)
{
    if (w->paletteChanged != NULL) w->paletteChanged(w);
}

/* 6900a5e0 -- like wglWindowResized() but only reallocates when 'haveThread' */
GLboolean __fastcall wglWindowResizedEx(__WGLwindow *w, BOOL haveThread)
{
    __WGLdrawable *dp = &w->dp;
    GLboolean ret = GL_TRUE;
    GLint x, y;
    GLuint width, height;

    wglUpdateWindowRect(w);
    w->dp.fullScreen = (GLboolean)wglIsFullScreenWindow(w);
    x = w->clientOrigin.x;
    width = w->width;
    height = w->height;
    y = w->clientOrigin.y;
    if ((x != dp->xOrigin || y != dp->yOrigin || width != dp->width || height != dp->height || (width == 0 && height == 0)) && haveThread) {
        dp->height  = height;
        dp->width   = width;
        dp->xOrigin = x;
        dp->yOrigin = y;
        ret = wglDrawResize(dp, x, y, width, height);
    }
    return ret;
}


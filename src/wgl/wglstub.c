/*
** wglstub.c -- the exported wgl* / pixel-format entry points.  Each export is a 6-byte
** `jmp dword ptr [table]` in the original (6900c3b0..6900c530); the tables default to the
** stand-alone implementations in wglstand.c and are rebound by wglBindStandAlone().
** wglMakeCurrent (6900c450) additionally attaches the thread first.
*/
#ifndef _GDI32_
#define _GDI32_            /* we ARE the GL/WGL exporter: no dllimport on wgl* and gl* */
#endif
#include "wglpriv.h"
#include "icdprotos.h"

/* 690ee904 .. 690ee914 */
FARPROC g_pfEntryPoints[5] = {
    (FARPROC)wglSAChoosePixelFormat, (FARPROC)wglSADescribePixelFormat, (FARPROC)wglSAGetPixelFormat,
    (FARPROC)wglSASetPixelFormat, (FARPROC)wglSASwapBuffers };
/* 690ee918 .. 690ee928 -- the GDI names of the five pixel-format entries, in g_pfEntryPoints' order (as
** g_wglEntryNames is to g_wglEntryPoints); ToDo UNUSED CODE: nothing in the driver reads it (found by the exact
** link, 2026-11-04) */
const char *g_pfEntryNames[5] = {
    "ChoosePixelFormat", "DescribePixelFormat", "GetPixelFormat", "SetPixelFormat", "SwapBuffers" };

/* 690ee92c .. 690ee974, same order as g_wglEntryNames (690ee978) */
FARPROC g_wglEntryPoints[19] = {
    (FARPROC)wglSACreateContext, (FARPROC)wglSADeleteContext, (FARPROC)wglSAGetCurrentContext,
    (FARPROC)wglSAGetCurrentDC, (FARPROC)wglSAGetProcAddress, (FARPROC)wglSAMakeCurrent,
    (FARPROC)wglSAShareLists, (FARPROC)wglSAUseFontBitmapsA, (FARPROC)wglSAUseFontBitmapsW,
    (FARPROC)wglSAUseFontOutlinesA, (FARPROC)wglSAUseFontOutlinesW, (FARPROC)wglSACopyContext,
    (FARPROC)wglSACreateLayerContext, (FARPROC)wglSADescribeLayerPlane, (FARPROC)wglSAGetDefaultProcAddress,
    (FARPROC)wglSAGetLayerPaletteEntries, (FARPROC)wglSARealizeLayerPalette, (FARPROC)wglSASetLayerPaletteEntries,
    (FARPROC)wglSASwapLayerBuffers };
const char *g_wglEntryNames[19] = {
    "wglCreateContext", "wglDeleteContext", "wglGetCurrentContext", "wglGetCurrentDC", "wglGetProcAddress",
    "wglMakeCurrent", "wglShareLists", "wglUseFontBitmapsA", "wglUseFontBitmapsW", "wglUseFontOutlinesA",
    "wglUseFontOutlinesW", "wglCopyContext", "wglCreateLayerContext", "wglDescribeLayerPlane",
    "wglGetDefaultProcAddress", "wglGetLayerPaletteEntries", "wglRealizeLayerPalette",
    "wglSetLayerPaletteEntries", "wglSwapLayerBuffers" };

/*
** Export stubs.  Under MSVC each is a naked 6-byte `jmp dword ptr [table+idx*4]`, exactly
** like the original; the portable branch is for the Linux syntax check only.
*/
#ifdef _MSC_VER
#define JMP_BODY(table, idx, cast, args) __asm jmp dword ptr [table + idx * 4]
#define STUB_ATTR __declspec(naked)
#else
#define JMP_BODY(table, idx, cast, args) return (cast table[idx]) args;
#define STUB_ATTR
#endif
#define JMP_BODY_VOID(table, idx, cast, args) JMP_BODY(table, idx, cast, args)

/* 6900c3b0 -- wglChoosePixelFormat: export: jmp [g_pfEntryPoints[0]] (wglSAChoosePixelFormat) */
STUB_ATTR int WINAPI wglChoosePixelFormat(HDC a, CONST PIXELFORMATDESCRIPTOR *b)
{
    JMP_BODY(g_pfEntryPoints, 0, (int (WINAPI *)(HDC, CONST PIXELFORMATDESCRIPTOR *)), (a, b))
}

/* 6900c3c0 -- wglDescribePixelFormat: export: jmp [g_pfEntryPoints[1]] (wglSADescribePixelFormat) */
STUB_ATTR int WINAPI wglDescribePixelFormat(HDC a, int b, UINT c, LPPIXELFORMATDESCRIPTOR d)
{
    JMP_BODY(g_pfEntryPoints, 1, (int (WINAPI *)(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR)), (a, b, c, d))
}

/* 6900c3d0 -- wglGetPixelFormat: export: jmp [g_pfEntryPoints[2]] (wglSAGetPixelFormat) */
STUB_ATTR int WINAPI wglGetPixelFormat(HDC a)
{
    JMP_BODY(g_pfEntryPoints, 2, (int (WINAPI *)(HDC)), (a))
}

/* 6900c3e0 -- wglSetPixelFormat: export: jmp [g_pfEntryPoints[3]] (wglSASetPixelFormat) */
STUB_ATTR BOOL WINAPI wglSetPixelFormat(HDC a, int b, CONST PIXELFORMATDESCRIPTOR *c)
{
    JMP_BODY(g_pfEntryPoints, 3, (BOOL (WINAPI *)(HDC, int, CONST PIXELFORMATDESCRIPTOR *)), (a, b, c))
}

/* 6900c3f0 -- wglSwapBuffers: export: jmp [g_pfEntryPoints[4]] (wglSASwapBuffers) */
STUB_ATTR BOOL WINAPI wglSwapBuffers(HDC a)
{
    JMP_BODY(g_pfEntryPoints, 4, (BOOL (WINAPI *)(HDC)), (a))
}

/* 6900c400 -- wglCreateContext: export: jmp [g_wglEntryPoints[0]] (default wglSACreateContext) */
STUB_ATTR HGLRC WINAPI wglCreateContext(HDC a)
{
    JMP_BODY(g_wglEntryPoints, 0, (HGLRC (WINAPI *)(HDC)), (a))
}

/* 6900c410 -- wglDeleteContext: export: jmp [g_wglEntryPoints[1]] (default wglSADeleteContext) */
STUB_ATTR BOOL WINAPI wglDeleteContext(HGLRC a)
{
    JMP_BODY(g_wglEntryPoints, 1, (BOOL (WINAPI *)(HGLRC)), (a))
}

/* 6900c420 -- wglGetCurrentContext: export: jmp [g_wglEntryPoints[2]] (default wglSAGetCurrentContext) */
STUB_ATTR HGLRC WINAPI wglGetCurrentContext(void)
{
    JMP_BODY(g_wglEntryPoints, 2, (HGLRC (WINAPI *)(void)), ())
}

/* 6900c430 -- wglGetCurrentDC: export: jmp [g_wglEntryPoints[3]] (default wglSAGetCurrentDC) */
STUB_ATTR HDC WINAPI wglGetCurrentDC(void)
{
    JMP_BODY(g_wglEntryPoints, 3, (HDC (WINAPI *)(void)), ())
}

/* 6900c440 -- wglGetProcAddress: export: jmp [g_wglEntryPoints[4]] (default wglSAGetProcAddress) */
STUB_ATTR PROC WINAPI wglGetProcAddress(LPCSTR a)
{
    JMP_BODY(g_wglEntryPoints, 4, (PROC (WINAPI *)(LPCSTR)), (a))
}

/* 6900c450 -- the only non-jump stub: attach the thread first */
BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hrc)
{
    wglGetCurrentGC();
#if defined(ICD_TRACE) || defined(ICD_MARK)
    { BOOL r_ = ((BOOL (WINAPI *)(HDC, HGLRC))g_wglEntryPoints[5])(hdc, hrc); ICD_T("wglMakeCurrent export returns", r_);
      ICD_T("  entry point", g_wglEntryPoints[5]); return r_; }
#else
    return ((BOOL (WINAPI *)(HDC, HGLRC))g_wglEntryPoints[5])(hdc, hrc);
#endif
}

/* 6900c470 -- wglShareLists: export: jmp [g_wglEntryPoints[6]] (default wglSAShareLists) */
STUB_ATTR BOOL WINAPI wglShareLists(HGLRC a, HGLRC b)
{
    JMP_BODY(g_wglEntryPoints, 6, (BOOL (WINAPI *)(HGLRC, HGLRC)), (a, b))
}

/* 6900c480 -- wglUseFontBitmapsA: export: jmp [g_wglEntryPoints[7]] (default wglSAUseFontBitmapsA) */
STUB_ATTR BOOL WINAPI wglUseFontBitmapsA(HDC a, DWORD b, DWORD c, DWORD d)
{
    JMP_BODY(g_wglEntryPoints, 7, (BOOL (WINAPI *)(HDC, DWORD, DWORD, DWORD)), (a, b, c, d))
}

/* 6900c490 -- wglUseFontBitmapsW: export: jmp [g_wglEntryPoints[8]] (default wglSAUseFontBitmapsW) */
STUB_ATTR BOOL WINAPI wglUseFontBitmapsW(HDC a, DWORD b, DWORD c, DWORD d)
{
    JMP_BODY(g_wglEntryPoints, 8, (BOOL (WINAPI *)(HDC, DWORD, DWORD, DWORD)), (a, b, c, d))
}

/* 6900c4a0 -- wglUseFontOutlinesA: export: jmp [g_wglEntryPoints[9]] (default wglSAUseFontOutlinesA) */
STUB_ATTR BOOL WINAPI wglUseFontOutlinesA(HDC a, DWORD b, DWORD c, DWORD d, FLOAT e, FLOAT f, int g, LPGLYPHMETRICSFLOAT h)
{
    JMP_BODY(g_wglEntryPoints, 9, (BOOL (WINAPI *)(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT)), (a, b, c, d, e, f, g, h))
}

/* 6900c4b0 -- wglUseFontOutlinesW: export: jmp [g_wglEntryPoints[10]] (default wglSAUseFontOutlinesW) */
STUB_ATTR BOOL WINAPI wglUseFontOutlinesW(HDC a, DWORD b, DWORD c, DWORD d, FLOAT e, FLOAT f, int g, LPGLYPHMETRICSFLOAT h)
{
    JMP_BODY(g_wglEntryPoints, 10, (BOOL (WINAPI *)(HDC, DWORD, DWORD, DWORD, FLOAT, FLOAT, int, LPGLYPHMETRICSFLOAT)), (a, b, c, d, e, f, g, h))
}

/* 6900c4c0 -- wglCopyContext: export: jmp [g_wglEntryPoints[11]] (default wglSACopyContext) */
STUB_ATTR BOOL WINAPI wglCopyContext(HGLRC a, HGLRC b, UINT c)
{
    JMP_BODY(g_wglEntryPoints, 11, (BOOL (WINAPI *)(HGLRC, HGLRC, UINT)), (a, b, c))
}

/* 6900c4d0 -- wglCreateLayerContext: export: jmp [g_wglEntryPoints[12]] (default wglSACreateLayerContext) */
STUB_ATTR HGLRC WINAPI wglCreateLayerContext(HDC a, int b)
{
    JMP_BODY(g_wglEntryPoints, 12, (HGLRC (WINAPI *)(HDC, int)), (a, b))
}

/* 6900c4e0 -- wglDescribeLayerPlane: export: jmp [g_wglEntryPoints[13]] (default wglSADescribeLayerPlane) */
STUB_ATTR BOOL WINAPI wglDescribeLayerPlane(HDC a, int b, int c, UINT d, LPLAYERPLANEDESCRIPTOR e)
{
    JMP_BODY(g_wglEntryPoints, 13, (BOOL (WINAPI *)(HDC, int, int, UINT, LPLAYERPLANEDESCRIPTOR)), (a, b, c, d, e))
}

/* 6900c4f0 -- wglGetDefaultProcAddress: export: jmp [g_wglEntryPoints[14]] (default wglSAGetDefaultProcAddress) */
STUB_ATTR PROC WINAPI wglGetDefaultProcAddress(LPCSTR a)
{
    JMP_BODY(g_wglEntryPoints, 14, (PROC (WINAPI *)(LPCSTR)), (a))
}

/* 6900c500 -- wglGetLayerPaletteEntries: export: jmp [g_wglEntryPoints[15]] (default wglSAGetLayerPaletteEntries) */
STUB_ATTR int WINAPI wglGetLayerPaletteEntries(HDC a, int b, int c, int d, COLORREF *e)
{
    JMP_BODY(g_wglEntryPoints, 15, (int (WINAPI *)(HDC, int, int, int, COLORREF *)), (a, b, c, d, e))
}

/* 6900c510 -- wglRealizeLayerPalette: export: jmp [g_wglEntryPoints[16]] (default wglSARealizeLayerPalette) */
STUB_ATTR BOOL WINAPI wglRealizeLayerPalette(HDC a, int b, BOOL c)
{
    JMP_BODY(g_wglEntryPoints, 16, (BOOL (WINAPI *)(HDC, int, BOOL)), (a, b, c))
}

/* 6900c520 -- wglSetLayerPaletteEntries: export: jmp [g_wglEntryPoints[17]] (default wglSASetLayerPaletteEntries) */
STUB_ATTR int WINAPI wglSetLayerPaletteEntries(HDC a, int b, int c, int d, CONST COLORREF *e)
{
    JMP_BODY(g_wglEntryPoints, 17, (int (WINAPI *)(HDC, int, int, int, CONST COLORREF *)), (a, b, c, d, e))
}

/* 6900c530 -- wglSwapLayerBuffers: export: jmp [g_wglEntryPoints[18]] (default wglSASwapLayerBuffers) */
STUB_ATTR BOOL WINAPI wglSwapLayerBuffers(HDC a, UINT b)
{
    JMP_BODY(g_wglEntryPoints, 18, (BOOL (WINAPI *)(HDC, UINT)), (a, b))
}

/* 6900c370 -- call the rc's user callback (rc->callbacks) if any */
void __fastcall wglRcCallback(__GLcontext *gc, void *table)
{
    __WGLrc *rc = GC_imports_wscx(gc);
    if (rc->callbacks != NULL) ((void (__stdcall *)(void *))rc->callbacks)(table);   /* the new dispatch table */
}

/* 6900c380 -- swap the rc's window under the wgl lock, after telling the core */
GLuint __fastcall wglRcSwapBuffers(__GLcontext *gc)
{
    __WGLrc *rc = GC_imports_wscx(gc);
    GLuint r;
    wglLock();
    GC_exports_notifySwapBuffers(rc->gc)(gc);         /* (the gc passed in, through rc->gc's exports) */
    r = ((GLubyte (__fastcall *)(__WGLwindow *))rc->window->swapBuffers)(rc->window);   /* returns a GLboolean */
    wglUnlock();
    return r;
}

/*
** wgldiag.c -- the diagnostic builds' support code; none of it is in the original DLL, and the normal build
** compiles this file to an empty object.
**   ICD_TRACE (nmake EXTRA=/DICD_TRACE): icdTrace, the line logger behind ICD_T (wglpriv.h), and the grLfbLock
**     counter g_icdLfbLocks.
**   ICD_MARK (nmake EXTRA=/DICD_MARK): the mark ring ICD_T writes in that build (three stores, no calls), the GL
**     error records, the Glide / import / gl* stub hooks and their log, dumped to C:\3DFXOGL.LOG at process
**     detach or from the unhandled-exception filter.
** wglpriv.h makes the two exclusive.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wglpriv.h"
#include "icdprotos.h"
#include "glsi.h"          /* __GLtransformICD (the ICD_MARK hooks read the matrices) */

#ifdef ICD_TRACE
LONG g_icdLfbLocks;
#endif
#ifdef ICD_TRACE
/* trace build only: append "msg value" to C:\3DFXOGL.LOG; the file is closed every time so the
** lines survive a crash.  Kernel32 only (safe inside DllMain). */
void icdTrace(const char *msg, DWORD value)
{
    char line[160];
    DWORD n, w;
    HANDLE h = CreateFileA("C:\\3DFXOGL.LOG", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    SetFilePointer(h, 0, NULL, FILE_END);
    n = (DWORD)wsprintfA(line, "[%08lx] %s %08lx\r\n", GetCurrentThreadId(), msg, value);
    WriteFile(h, line, n, &w, NULL);
    CloseHandle(h);
}
#endif
#ifdef ICD_MARK
volatile DWORD g_icdMarks[1024];
/* icdEsp: ICD_MARK build only: the caller's esp after the call (esp + 4 at entry), the esp column of an ICD_T mark */
__declspec(naked) DWORD icdEsp(void) { __asm { lea eax, [esp + 4] } __asm { ret } }
volatile DWORD g_icdMarkN;
volatile DWORD g_icdSwapN;
static volatile DWORD s_icdErr[64 * 32];
static volatile DWORD s_icdErrN;
/* icdErrMark: ICD_MARK build only (our diagnostic code, not in the original DLL): record a GL error, the first 64
** only: its code, the frame number (g_icdSwapN) and 30 dwords from sp (0 where unreadable), for icdMarkDump; nothing
** in src/ calls it at present */
void __fastcall icdErrMark(DWORD code, const DWORD *sp)
{
    volatile DWORD *r;
    DWORD k;
    if (s_icdErrN++ >= 64) return;
    r = &s_icdErr[(s_icdErrN - 1) * 32];
    r[0] = code;
    r[1] = g_icdSwapN;
    for (k = 0; k < 30; k++) r[2 + k] = IsBadReadPtr(sp + k, 4) ? 0 : sp[k];
}
/* unhandled exception: log the fault, registers and stack, then the marks; let the crash continue */
LONG WINAPI icdExceptionFilter(EXCEPTION_POINTERS *ep)
{
    char line[200];
    DWORD n, w, k;
    CONTEXT *c = ep->ContextRecord;
    HANDLE h = CreateFileA("C:\\3DFXOGL.LOG", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        SetFilePointer(h, 0, NULL, FILE_END);
        n = (DWORD)wsprintfA(line, "EXCEPTION %08lx at %08lx (flags %lx)\r\n", ep->ExceptionRecord->ExceptionCode,
                             (DWORD)ep->ExceptionRecord->ExceptionAddress, ep->ExceptionRecord->ExceptionFlags);
        WriteFile(h, line, n, &w, NULL);
        n = (DWORD)wsprintfA(line, "  eip %08lx esp %08lx ebp %08lx eax %08lx ebx %08lx ecx %08lx edx %08lx esi %08lx edi %08lx\r\n",
                             c->Eip, c->Esp, c->Ebp, c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi);
        WriteFile(h, line, n, &w, NULL);
        for (k = 0; k < 32; k += 4) {
            const DWORD *sp = (const DWORD *)c->Esp + k;
            if (IsBadReadPtr(sp, 16)) break;
            n = (DWORD)wsprintfA(line, "  [esp+%02lx] %08lx %08lx %08lx %08lx\r\n", k * 4, sp[0], sp[1], sp[2], sp[3]);
            WriteFile(h, line, n, &w, NULL);
        }
        CloseHandle(h);
    }
    icdMarkDump();
    return EXCEPTION_CONTINUE_SEARCH;
}
/* The whole GL call stream (every exported gl* stub, first four arguments) of a few frames,
** the matrices at each glBegin, and the Glide draw calls with their first vertex (import table hooks) */
#define ICD_LOGCAP (1u << 20)
static DWORD *s_icdLog;
static volatile DWORD s_icdLogN;
/* The frame in which GL_POINTS are first drawn and the next two, then again for the first
** points frame at least 600 frames later (two windows) */
static DWORD s_icdWin[2][2], s_icdWinN;
/* icdLogFrame: ICD_MARK build only: 1 when this frame is logged (before the first SwapBuffers or in a window) */
static int icdLogFrame(void)
{
    DWORD k;
    if (g_icdSwapN == 0) return 1;      /* everything before the first SwapBuffers */
    for (k = 0; k < s_icdWinN; k++)
        if (g_icdSwapN >= s_icdWin[k][0] && g_icdSwapN <= s_icdWin[k][1]) return 1;
    return 0;
}
/* icdLogPointsSeen: ICD_MARK build only: at glBegin(GL_POINTS) open a 3-frame log window (at most two, the second at
** least 600 frames after the first one ends) unless this frame is already logged */
static void icdLogPointsSeen(void)
{
    if (s_icdWinN == 2 || icdLogFrame()) return;
    if (s_icdWinN == 1 && g_icdSwapN < s_icdWin[0][1] + 600) return;
    s_icdWin[s_icdWinN][0] = g_icdSwapN;
    s_icdWin[s_icdWinN][1] = g_icdSwapN + 2;
    s_icdWinN++;
}
/* icdLogPut: ICD_MARK build only: append v to the call log (VirtualAlloc'd on first use; dropped once full) */
static void icdLogPut(DWORD v)
{
    if (s_icdLog == NULL) s_icdLog = (DWORD *)VirtualAlloc(NULL, ICD_LOGCAP * 4, MEM_COMMIT, PAGE_READWRITE);
    if (s_icdLog != NULL && s_icdLogN < ICD_LOGCAP) s_icdLog[s_icdLogN++] = v;
}
/* icdLogMatrix: ICD_MARK build only: log tag and the 17 dwords at m (16 floats + matrixType; deadbeef unreadable) */
static void icdLogMatrix(DWORD tag, const DWORD *m)
{
    DWORD k;
    icdLogPut(tag);
    for (k = 0; k < 17; k++) icdLogPut(IsBadReadPtr(m + k, 4) ? 0xdeadbeef : m[k]);   /* 16 floats + matrixType */
}
/* Entry counters of the point renderers (tag e0|id, frame-window only; totals at the dump) */
static DWORD s_icdCnt[8];
/* icdCount: ICD_MARK build only (our diagnostic code, not in the original DLL): count an entry of point renderer id
** (s_icdCnt[id & 7], totals in icdLogDump); the first id 3 opens a 3-frame log window, and inside a logged frame tag
** e0|id is logged; returns 0.  Nothing in src/ calls it at present */
int __fastcall icdCount(DWORD id)
{
    s_icdCnt[id & 7]++;
    if (id == 3 && s_icdCnt[3] == 1 && s_icdWinN < 2 && !icdLogFrame()) {   /* first disc setup: open a window */
        s_icdWin[s_icdWinN][0] = g_icdSwapN;
        s_icdWin[s_icdWinN][1] = g_icdSwapN + 2;
        s_icdWinN++;
    }
    if (icdLogFrame() && s_icdLogN + 2 < ICD_LOGCAP) icdLogPut(0xe0000000 | id);
    return 0;
}
/* The glBegin return watch (for a stack overwrite under glBegin): watch glBegin's return address and argument from
** the stub entry to the next GL call; every probe (icdProbe in the validation path, the Glide hooks) logs
** f0|tag, its caller, the two watched dwords; the first probe that sees them changed is kept for the dump */
static DWORD *s_wAt, s_wVal, s_wArg, s_wN;
static DWORD s_wHit[6];
/* icdWatchCheck: ICD_MARK build only (our diagnostic code, not in the original DLL): the glBegin return watch: while
** a watch is set, log f0|tag, ret, the two watched dwords and esp (up to 4096 probes, logged frames only), and keep
** the first probe that sees the watched dwords changed in s_wHit */
static void icdWatchCheck(DWORD tag, DWORD ret, DWORD esp)
{
    if (s_wAt == NULL) return;
    if (s_wN < 4096 && icdLogFrame() && s_icdLogN + 8 < ICD_LOGCAP) {   /* + the probe's esp */
        icdLogPut(0xf0000000 | tag); icdLogPut(ret); icdLogPut(s_wAt[0]); icdLogPut(s_wAt[1]); icdLogPut(esp); s_wN++;
    }
    if ((s_wAt[0] != s_wVal || s_wAt[1] != s_wArg) && s_wHit[0] == 0) {
        s_wHit[0] = 1; s_wHit[1] = tag; s_wHit[2] = ret; s_wHit[3] = s_wAt[0]; s_wHit[4] = s_wAt[1]; s_wHit[5] = (DWORD)s_wAt;
    }
}
/* icdProbe: ICD_MARK build only: a watch probe (icdWatchCheck) with its return address as the caller; returns 0 */
int __stdcall icdProbe(DWORD tag)
{
    icdWatchCheck(tag, ((const DWORD *)&tag)[-1], (DWORD)&tag);
    return 0;
}
/* icdProbeV: ICD_MARK build only: icdProbe with v logged in place of the caller; returns 0 */
int __stdcall icdProbeV(DWORD tag, DWORD v)      /* A probe that logs a value in the caller field */
{
    icdWatchCheck(tag, v, (DWORD)&tag);
    return 0;
}
static DWORD s_icdLastMode = 0xffffffff;
/* icdGLHook: ICD_MARK build only (our diagnostic code, not in the original DLL): the per-call hook of the gl* stubs
** (slot, sp -> return address, sp[1..4] the arguments; called from the stubs of a mark copy of glstubs.c).  glBegin (slot 8) watches its return address / mode and opens a log
** window on GL_POINTS; glEnd (44) after a points batch logs the point procs and their inputs (b5); in logged frames
** every call logs a0|slot and four arguments, glBegin also the modelview, mvp and projection matrices (b1..b3) and b4
** (nonUnityQ, downloadAddress, texture enables).  Nothing in src/ calls it at present */
void __stdcall icdGLHook(DWORD slot, const DWORD *sp)
{
    s_wAt = NULL;       /* the caller has returned from glBegin and reuses the watched slots for this call's arguments */
    if (slot == 8) { s_wAt = (DWORD *)sp; s_wVal = sp[0]; s_wArg = sp[1]; }
    if (slot == 8) {
        s_icdLastMode = sp[1];
        if (sp[1] == 0) icdLogPointsSeen();     /* glBegin(GL_POINTS) */
    }
    if (slot == 44 && s_icdLastMode == 0 && icdLogFrame() && s_icdLogN + 16 < ICD_LOGCAP) {
        /* glEnd of a points batch (validated at glBegin): the point proc and the picker's inputs */
        const __GLcontext *gc = wglGetCurrentGC();
        if (gc != NULL) {
            icdLogPut(0xb5000000);
            icdLogPut((DWORD)gc->procs.renderPoint); icdLogPut(gc->polygon.shader.modeFlags);
            icdLogPut(gc->state.enables.general);
            icdLogPut((DWORD)gc->hwcx.windowed); icdLogPut((DWORD)gc->hwcx.has32bpp); icdLogPut(gc->renderMode);
            icdLogPut(gc->state.raster.drawBuffer);
            icdLogPut(*(const DWORD *)&gc->state.point.requestedSize);     /* the float's bits */
            icdLogPut(*(const DWORD *)&gc->state.raster.rMask);            /* rMask, gMask, bMask, aMask */
            icdLogPut((DWORD)gc->procs.renderTrianglePolygonModePoint); icdLogPut((DWORD)gc->hwcx.swPoints);
        }
    }
    if (!icdLogFrame() || s_icdLogN + 128 > ICD_LOGCAP) return;
    icdLogPut(0xa0000000 | slot);
    icdLogPut(sp[1]); icdLogPut(sp[2]); icdLogPut(sp[3]); icdLogPut(sp[4]);
    if (slot == 8) {            /* glBegin: modelview, its mvp, projection */
        const __GLcontext *gc = wglGetCurrentGC();
        if (gc != NULL) {
            const __GLtransformICD *mv = gc->transform.modelView, *pr = gc->transform.projection;
            icdLogMatrix(0xb1000000, (const DWORD *)&mv->matrix);
            icdLogMatrix(0xb2000000, (const DWORD *)&mv->mvp);
            icdLogMatrix(0xb3000000, (const DWORD *)&pr->matrix);
            icdLogPut(0xb4000000);
            icdLogPut(*(const DWORD *)&gc->hwcx.nonUnityQ);                 /* nonUnityQ and the 3 bytes after it */
            icdLogPut(gc->hwcx.downloadAddress);
            icdLogPut(*(const WORD *)gc->texture.textureEnabled);          /* both units' flags */
        }
    }
}
/* Glide draw hooks */
typedef void (__stdcall *IcdFn3)(DWORD, DWORD, DWORD);
static FARPROC s_grReal[8];
/* icdLogVertex: ICD_MARK build only: probe the watch (400|fn); in logged frames log c0|fn, a, b, c, 12 dwords at v */
static void icdLogVertex(DWORD fn, DWORD a, DWORD b, DWORD c, const DWORD *v)
{
    DWORD k;
    icdWatchCheck(0x400 | fn, 0, (DWORD)&fn);
    if (!icdLogFrame() || s_icdLogN + 64 > ICD_LOGCAP) return;
    icdLogPut(0xc0000000 | fn); icdLogPut(a); icdLogPut(b); icdLogPut(c);
    for (k = 0; k < 12; k++) icdLogPut(v != NULL && !IsBadReadPtr(v + k, 4) ? v[k] : 0xdeadbeef);
}
/* icdGrDrawTriangle: ICD_MARK build only: grDrawTriangle hook: icdLogVertex 0 (first vertex), then the real call */
static void __stdcall icdGrDrawTriangle(DWORD a, DWORD b, DWORD c)
{ icdLogVertex(0, a, b, c, (const DWORD *)a); ((void (__stdcall *)(DWORD, DWORD, DWORD))s_grReal[0])(a, b, c); }
/* icdGrDrawVertexArray: ICD_MARK build only: its hook: icdLogVertex 1 (vertex p[0]), then the real call */
static void __stdcall icdGrDrawVertexArray(DWORD mode, DWORD n, DWORD p)
{ icdLogVertex(1, mode, n, p, (n && !IsBadReadPtr((void *)p, 4)) ? *(const DWORD * const *)p : NULL);
  ((void (__stdcall *)(DWORD, DWORD, DWORD))s_grReal[1])(mode, n, p); }
/* icdGrDrawVertexArrayContiguous: ICD_MARK build only: its hook: icdLogVertex 2 (vertex at p), then the real
** call */
static void __stdcall icdGrDrawVertexArrayContiguous(DWORD mode, DWORD n, DWORD p, DWORD stride)
{ icdLogVertex(2, mode, n, stride, (const DWORD *)p); ((void (__stdcall *)(DWORD, DWORD, DWORD, DWORD))s_grReal[2])(mode, n, p, stride); }
/* icdGrDrawPoint: ICD_MARK build only: grDrawPoint hook: icdLogVertex 3, then the real call */
static void __stdcall icdGrDrawPoint(DWORD a)
{ icdLogVertex(3, a, 0, 0, (const DWORD *)a); ((void (__stdcall *)(DWORD))s_grReal[3])(a); }
/* icdGrDrawLine: ICD_MARK build only: grDrawLine hook: icdLogVertex 4 (first vertex), then the real call */
static void __stdcall icdGrDrawLine(DWORD a, DWORD b)
{ icdLogVertex(4, a, b, 0, (const DWORD *)a); ((void (__stdcall *)(DWORD, DWORD))s_grReal[4])(a, b); }
/* icdGrAADrawTriangle: ICD_MARK build only: its hook: icdLogVertex 5 (first 3 arguments), then the real call */
static void __stdcall icdGrAADrawTriangle(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f)
{ icdLogVertex(5, a, b, c, (const DWORD *)a); ((void (__stdcall *)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))s_grReal[5])(a, b, c, d, e, f); }
static DWORD s_layoutN;
static DWORD s_layout[64 * 3];
/* icdGrVertexLayout: ICD_MARK build only: its hook: keep the first 64 calls in s_layout, log c0000006 and
** the arguments in a logged frame, then the real call */
static void __stdcall icdGrVertexLayout(DWORD param, DWORD off, DWORD mode)
{
    if (s_layoutN < 64) { s_layout[s_layoutN * 3] = param; s_layout[s_layoutN * 3 + 1] = off; s_layout[s_layoutN * 3 + 2] = mode; s_layoutN++; }
    if (icdLogFrame() && s_icdLogN + 8 < ICD_LOGCAP) { icdLogPut(0xc0000006); icdLogPut(param); icdLogPut(off); icdLogPut(mode); }
    ((void (__stdcall *)(DWORD, DWORD, DWORD))s_grReal[6])(param, off, mode);
}
/* Glide texture / combine state calls (in the logged frames) and every texture download */
static DWORD s_dl[512 * 8];
static DWORD s_dlN;
/* icdLogGr: ICD_MARK build only (our diagnostic code, not in the original DLL): probe the watch with 200|id and, in a
** logged frame, log a Glide state call: tag d0 | count << 16 | id, its n arguments, then 5 dwords of the texture info
** when info is given */
static void icdLogGr(DWORD id, DWORD n, const DWORD *a, const DWORD *info)
{
    DWORD k;
    icdWatchCheck(0x200 | id, 0, (DWORD)&id);
    if (!icdLogFrame() || s_icdLogN + 32 > ICD_LOGCAP) return;
    icdLogPut(0xd0000000 | ((n + (info ? 5 : 0)) << 16) | id);
    for (k = 0; k < n; k++) icdLogPut(a[k]);
    if (info) for (k = 0; k < 5; k++) icdLogPut(IsBadReadPtr(info + k, 4) ? 0xdeadbeef : info[k]);
}
static DWORD s_dl2[256 * 8];
static DWORD s_dl2N;
/* icdLogDl: ICD_MARK build only (our diagnostic code, not in the original DLL): probe the watch with 300|id and
** record a texture download (id | frame << 8 and up to 7 arguments): full grTexDownloadMipMap (id 2) in s_dl2 (first
** 256), the others in s_dl (first 512) */
static void icdLogDl(DWORD id, const DWORD *a, DWORD n)
{
    DWORD k;
    icdWatchCheck(0x300 | id, 0, (DWORD)&id);
    if (id == 2) {
        if (s_dl2N >= 256) return;
        s_dl2[s_dl2N * 8] = id | (g_icdSwapN << 8);
        for (k = 0; k < 7; k++) s_dl2[s_dl2N * 8 + 1 + k] = k < n ? a[k] : 0;
        s_dl2N++;
        return;
    }
    if (s_dlN >= 512) return;
    s_dl[s_dlN * 8] = id | (g_icdSwapN << 8);
    for (k = 0; k < 7; k++) s_dl[s_dlN * 8 + 1 + k] = k < n ? a[k] : 0;
    s_dlN++;
}
static FARPROC s_grT[16];
#define GRREAL(i) s_grT[i]
/* icdGrTexSource: ICD_MARK build only: its hook: icdLogGr 1 (4 arguments + 5 dwords of info), then the real call */
static void __stdcall icdGrTexSource(DWORD a, DWORD b, DWORD c, DWORD d)
{ DWORD v[4]; v[0] = a; v[1] = b; v[2] = c; v[3] = d; icdLogGr(1, 4, v, (const DWORD *)d);
  ((void (__stdcall *)(DWORD, DWORD, DWORD, DWORD))GRREAL(1))(a, b, c, d); }
/* icdGrTexDownloadMipMap: ICD_MARK build only: its hook: record (icdLogDl 2) and log (icdLogGr 2) tmu, address,
** evenOdd and the info's first dwords, then the real call */
static void __stdcall icdGrTexDownloadMipMap(DWORD a, DWORD b, DWORD c, DWORD d)
{ DWORD v[7]; v[0] = a; v[1] = b; v[2] = c;
  if (!IsBadReadPtr((void *)d, 16)) { v[3] = ((DWORD *)d)[0]; v[4] = ((DWORD *)d)[1]; v[5] = ((DWORD *)d)[2]; v[6] = ((DWORD *)d)[3]; } else v[3] = v[4] = v[5] = v[6] = 0xdeadbeef;
  icdLogDl(2, v, 7); icdLogGr(2, 4, v, (const DWORD *)d);
  ((void (__stdcall *)(DWORD, DWORD, DWORD, DWORD))GRREAL(2))(a, b, c, d); }
/* icdGrTexDownloadMipMapLevelPartial: ICD_MARK build only: its hook: record the first 7 arguments (icdLogDl 3)
** when start (the 9th) is 0, then the real call */
static void __stdcall icdGrTexDownloadMipMapLevelPartial(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, DWORD h, DWORD i, DWORD j)
{ DWORD v[10]; v[0] = a; v[1] = b; v[2] = c; v[3] = d; v[4] = e; v[5] = f; v[6] = g; v[7] = h; v[8] = i; v[9] = j;
  if (i == 0) icdLogDl(3, v, 7);
  ((void (__stdcall *)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))GRREAL(3))(a, b, c, d, e, f, g, h, i, j); }
/* icdGrTexCombine: ICD_MARK build only: grTexCombine hook: icdLogGr 4 with its 7 arguments, then the real call */
static void __stdcall icdGrTexCombine(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g)
{ DWORD v[7]; v[0] = a; v[1] = b; v[2] = c; v[3] = d; v[4] = e; v[5] = f; v[6] = g; icdLogGr(4, 7, v, NULL);
  ((void (__stdcall *)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))GRREAL(4))(a, b, c, d, e, f, g); }
/* icdGrColorCombine: ICD_MARK build only: grColorCombine hook: icdLogGr 5 with its 5 arguments, then the real call */
static void __stdcall icdGrColorCombine(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e)
{ DWORD v[5]; v[0] = a; v[1] = b; v[2] = c; v[3] = d; v[4] = e; icdLogGr(5, 5, v, NULL);
  ((void (__stdcall *)(DWORD, DWORD, DWORD, DWORD, DWORD))GRREAL(5))(a, b, c, d, e); }
/* icdGrAlphaCombine: ICD_MARK build only: grAlphaCombine hook: icdLogGr 6 with its 5 arguments, then the real call */
static void __stdcall icdGrAlphaCombine(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e)
{ DWORD v[5]; v[0] = a; v[1] = b; v[2] = c; v[3] = d; v[4] = e; icdLogGr(6, 5, v, NULL);
  ((void (__stdcall *)(DWORD, DWORD, DWORD, DWORD, DWORD))GRREAL(6))(a, b, c, d, e); }
/* icdGrAlphaBlendFunction: ICD_MARK build only: its hook: icdLogGr 7 with its 4 arguments, then the real call */
static void __stdcall icdGrAlphaBlendFunction(DWORD a, DWORD b, DWORD c, DWORD d)
{ DWORD v[4]; v[0] = a; v[1] = b; v[2] = c; v[3] = d; icdLogGr(7, 4, v, NULL);
  ((void (__stdcall *)(DWORD, DWORD, DWORD, DWORD))GRREAL(7))(a, b, c, d); }
/* icdGrAlphaTestFunction: ICD_MARK build only: its hook: icdLogGr 8 with its argument, then the real call */
static void __stdcall icdGrAlphaTestFunction(DWORD a)
{ icdLogGr(8, 1, &a, NULL); ((void (__stdcall *)(DWORD))GRREAL(8))(a); }
/* icdGrTexClampMode: ICD_MARK build only: grTexClampMode hook: icdLogGr 9 with its 3 arguments, then the real call */
static void __stdcall icdGrTexClampMode(DWORD a, DWORD b, DWORD c)
{ DWORD v[3]; v[0] = a; v[1] = b; v[2] = c; icdLogGr(9, 3, v, NULL); ((void (__stdcall *)(DWORD, DWORD, DWORD))GRREAL(9))(a, b, c); }
/* icdGrTexFilterMode: ICD_MARK build only: its hook: icdLogGr 10 with its 3 arguments, then the real call */
static void __stdcall icdGrTexFilterMode(DWORD a, DWORD b, DWORD c)
{ DWORD v[3]; v[0] = a; v[1] = b; v[2] = c; icdLogGr(10, 3, v, NULL); ((void (__stdcall *)(DWORD, DWORD, DWORD))GRREAL(10))(a, b, c); }
/* icdGrTexMipMapMode: ICD_MARK build only: its hook: icdLogGr 11 with its 3 arguments, then the real call */
static void __stdcall icdGrTexMipMapMode(DWORD a, DWORD b, DWORD c)
{ DWORD v[3]; v[0] = a; v[1] = b; v[2] = c; icdLogGr(11, 3, v, NULL); ((void (__stdcall *)(DWORD, DWORD, DWORD))GRREAL(11))(a, b, c); }
/* the combine extensions come from grGetProcAddress */
static FARPROC s_ext[4];
/* icdColorCombineExt: ICD_MARK build only: grColorCombineExt wrapper: icdLogGr 12 (10 arguments), then s_ext[0] */
static void __stdcall icdColorCombineExt(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, DWORD h, DWORD i, DWORD j)
{ DWORD v[10]; v[0] = a; v[1] = b; v[2] = c; v[3] = d; v[4] = e; v[5] = f; v[6] = g; v[7] = h; v[8] = i; v[9] = j; icdLogGr(12, 10, v, NULL);
  ((void (__stdcall *)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))s_ext[0])(a, b, c, d, e, f, g, h, i, j); }
/* icdAlphaCombineExt: ICD_MARK build only: grAlphaCombineExt wrapper: icdLogGr 13 (10 arguments), then s_ext[1] */
static void __stdcall icdAlphaCombineExt(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, DWORD h, DWORD i, DWORD j)
{ DWORD v[10]; v[0] = a; v[1] = b; v[2] = c; v[3] = d; v[4] = e; v[5] = f; v[6] = g; v[7] = h; v[8] = i; v[9] = j; icdLogGr(13, 10, v, NULL);
  ((void (__stdcall *)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))s_ext[1])(a, b, c, d, e, f, g, h, i, j); }
/* icdTexColorCombineExt: ICD_MARK build only: its wrapper: icdLogGr 14 (11 arguments), then s_ext[2] */
static void __stdcall icdTexColorCombineExt(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, DWORD h, DWORD i, DWORD j, DWORD k)
{ DWORD v[11]; v[0] = a; v[1] = b; v[2] = c; v[3] = d; v[4] = e; v[5] = f; v[6] = g; v[7] = h; v[8] = i; v[9] = j; v[10] = k; icdLogGr(14, 11, v, NULL);
  ((void (__stdcall *)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))s_ext[2])(a, b, c, d, e, f, g, h, i, j, k); }
/* icdTexAlphaCombineExt: ICD_MARK build only: its wrapper: icdLogGr 15 (11 arguments), then s_ext[3] */
static void __stdcall icdTexAlphaCombineExt(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e, DWORD f, DWORD g, DWORD h, DWORD i, DWORD j, DWORD k)
{ DWORD v[11]; v[0] = a; v[1] = b; v[2] = c; v[3] = d; v[4] = e; v[5] = f; v[6] = g; v[7] = h; v[8] = i; v[9] = j; v[10] = k; icdLogGr(15, 11, v, NULL);
  ((void (__stdcall *)(DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD))s_ext[3])(a, b, c, d, e, f, g, h, i, j, k); }
/* icdGrGetProcAddress: ICD_MARK build only (our diagnostic code, not in the original DLL): the grGetProcAddress hook:
** the real result, except that the four combine extensions (gr{Color,Alpha,TexColor,TexAlpha}CombineExt) are kept in
** s_ext and their logging wrappers returned */
static FARPROC __stdcall icdGrGetProcAddress(const char *name)
{
    static const char *n[4] = { "grColorCombineExt", "grAlphaCombineExt", "grTexColorCombineExt", "grTexAlphaCombineExt" };
    static const FARPROC w[4] = { (FARPROC)icdColorCombineExt, (FARPROC)icdAlphaCombineExt, (FARPROC)icdTexColorCombineExt, (FARPROC)icdTexAlphaCombineExt };
    FARPROC p = ((FARPROC (__stdcall *)(const char *))GRREAL(0))(name);
    DWORD k;
    for (k = 0; k < 4; k++)
        if (p != NULL && !IsBadReadPtr(name, 1) && lstrcmpA(name, n[k]) == 0) { s_ext[k] = p; return w[k]; }
    return p;
}
/* icdHookImports: ICD_MARK build only (our diagnostic code, not in the original DLL): IAT patching: look the names up
** in glide3x.dll (none if it is not loaded) into real[], then in every import descriptor of self replace each thunk
** equal to a real[k] with hooks[k] (VirtualProtect around the write) */
static void icdHookImports(HMODULE self, const char *const *names, const FARPROC *hooks, FARPROC *real, DWORD count)
{
    BYTE *base = (BYTE *)self;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_IMPORT_DESCRIPTOR *d = (IMAGE_IMPORT_DESCRIPTOR *)(base + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    HMODULE g = GetModuleHandleA("glide3x.dll");
    DWORD k;
    if (g == NULL) return;
    for (k = 0; k < count; k++) real[k] = GetProcAddress(g, names[k]);
    for (; d->Name; d++) {
        IMAGE_THUNK_DATA *t = (IMAGE_THUNK_DATA *)(base + d->FirstThunk);
        for (; t->u1.Function; t++)
            for (k = 0; k < count; k++)
                if (real[k] != NULL && (FARPROC)t->u1.Function == real[k]) {
                    DWORD old;
                    VirtualProtect(&t->u1.Function, 4, PAGE_READWRITE, &old);
                    t->u1.Function = (PDWORD)hooks[k];
                    VirtualProtect(&t->u1.Function, 4, old, &old);
                }
    }
}
/* icdHookGlideTex: ICD_MARK build only (our diagnostic code, not in the original DLL): hook grGetProcAddress and the
** texture, combine and alpha Glide calls of self's imports with their icdGr* loggers (real ones in s_grT) */
static void icdHookGlideTex(HMODULE self)
{
    static const char *names[12] = { "_grGetProcAddress@4", "_grTexSource@16", "_grTexDownloadMipMap@16",
        "_grTexDownloadMipMapLevelPartial@40", "_grTexCombine@28", "_grColorCombine@20", "_grAlphaCombine@20",
        "_grAlphaBlendFunction@16", "_grAlphaTestFunction@4", "_grTexClampMode@12", "_grTexFilterMode@12", "_grTexMipMapMode@12" };
    static const FARPROC hooks[12] = { (FARPROC)icdGrGetProcAddress, (FARPROC)icdGrTexSource, (FARPROC)icdGrTexDownloadMipMap,
        (FARPROC)icdGrTexDownloadMipMapLevelPartial, (FARPROC)icdGrTexCombine, (FARPROC)icdGrColorCombine, (FARPROC)icdGrAlphaCombine,
        (FARPROC)icdGrAlphaBlendFunction, (FARPROC)icdGrAlphaTestFunction, (FARPROC)icdGrTexClampMode, (FARPROC)icdGrTexFilterMode,
        (FARPROC)icdGrTexMipMapMode };
    icdHookImports(self, names, hooks, s_grT, 12);
}
/* icdHookGlide: ICD_MARK build only (our diagnostic code, not in the original DLL): called at process attach:
** icdHookGlideTex, then hook the draw calls and grVertexLayout of self's imports (real ones in s_grReal;
** icdHookImports' loop inline) */
void icdHookGlide(HMODULE self)
{
    icdHookGlideTex(self);
    {
    static const char *names[7] = { "_grDrawTriangle@12", "_grDrawVertexArray@12", "_grDrawVertexArrayContiguous@16",
                                    "_grDrawPoint@4", "_grDrawLine@8", "_grAADrawTriangle@24", "_grVertexLayout@12" };
    static const FARPROC hooks[7] = { (FARPROC)icdGrDrawTriangle, (FARPROC)icdGrDrawVertexArray, (FARPROC)icdGrDrawVertexArrayContiguous,
                                      (FARPROC)icdGrDrawPoint, (FARPROC)icdGrDrawLine, (FARPROC)icdGrAADrawTriangle, (FARPROC)icdGrVertexLayout };
    BYTE *base = (BYTE *)self;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_IMPORT_DESCRIPTOR *d = (IMAGE_IMPORT_DESCRIPTOR *)(base + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    HMODULE g = GetModuleHandleA("glide3x.dll");
    DWORD k;
    if (g == NULL) return;
    for (k = 0; k < 7; k++) s_grReal[k] = GetProcAddress(g, names[k]);
    for (; d->Name; d++) {
        IMAGE_THUNK_DATA *t = (IMAGE_THUNK_DATA *)(base + d->FirstThunk);
        for (; t->u1.Function; t++)
            for (k = 0; k < 7; k++)
                if (s_grReal[k] != NULL && (FARPROC)t->u1.Function == s_grReal[k]) {
                    DWORD old;
                    VirtualProtect(&t->u1.Function, 4, PAGE_READWRITE, &old);
                    t->u1.Function = (PDWORD)hooks[k];
                    VirtualProtect(&t->u1.Function, 4, old, &old);
                }
    }
    }
}
/* icdLogDump: ICD_MARK build only (our diagnostic code, not in the original DLL): append to the log h the
** grVertexLayout calls, the texture download records, the point renderer counters, the glBegin watch result, then the
** call log one record per line (record lengths from their tags) */
static void icdLogDump(HANDLE h)
{
    char line[400];
    DWORD n, w, i, k;
    n = (DWORD)wsprintfA(line, "grVertexLayout calls %lu:", s_layoutN);
    WriteFile(h, line, n, &w, NULL);
    for (k = 0; k < s_layoutN && k < 64; k++) {
        n = (DWORD)wsprintfA(line, " (%lx %lx %lx)", s_layout[k * 3], s_layout[k * 3 + 1], s_layout[k * 3 + 2]);
        WriteFile(h, line, n, &w, NULL);
    }
    n = (DWORD)wsprintfA(line, "\r\ntexture downloads %lu (id|frame<<8, tmu, addr, ...)\r\n", s_dlN);
    WriteFile(h, line, n, &w, NULL);
    for (k = 0; k < s_dlN; k++) {
        n = (DWORD)wsprintfA(line, "dl %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\r\n", s_dl[k * 8], s_dl[k * 8 + 1], s_dl[k * 8 + 2],
                             s_dl[k * 8 + 3], s_dl[k * 8 + 4], s_dl[k * 8 + 5], s_dl[k * 8 + 6], s_dl[k * 8 + 7]);
        WriteFile(h, line, n, &w, NULL);
    }
    n = (DWORD)wsprintfA(line, "full mipmap downloads %lu (2|frame<<8, tmu, addr, evenOdd, small, large, aspect, format)\r\n", s_dl2N);
    WriteFile(h, line, n, &w, NULL);
    for (k = 0; k < s_dl2N; k++) {
        n = (DWORD)wsprintfA(line, "dl2 %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\r\n", s_dl2[k * 8], s_dl2[k * 8 + 1], s_dl2[k * 8 + 2],
                             s_dl2[k * 8 + 3], s_dl2[k * 8 + 4], s_dl2[k * 8 + 5], s_dl2[k * 8 + 6], s_dl2[k * 8 + 7]);
        WriteFile(h, line, n, &w, NULL);
    }
    n = (DWORD)wsprintfA(line, "point renderer entries: sprite 690603ed %lu, aa 690600c0 %lu, square 6905ff40 %lu, setup 69060780 %lu, size1 6905ff30 %lu; buildImm 69020c90 %lu (generated %lu), lazy vertex proc 69026350 %lu\r\n",
                         s_icdCnt[0], s_icdCnt[1], s_icdCnt[2], s_icdCnt[3], s_icdCnt[4], s_icdCnt[5], s_icdCnt[7], s_icdCnt[6]);
    WriteFile(h, line, n, &w, NULL);
    n = (DWORD)wsprintfA(line, "glBegin return watch: hit %lu tag %lx caller %08lx now %08lx %08lx at %08lx (was %08lx %08lx); probes logged %lu\r\n",
                         s_wHit[0], s_wHit[1], s_wHit[2], s_wHit[3], s_wHit[4], s_wHit[5], s_wVal, s_wArg, s_wN);
    WriteFile(h, line, n, &w, NULL);
    n = (DWORD)wsprintfA(line, "call log: %lu dwords (windows %lu..%lu, %lu..%lu)\r\n", s_icdLogN, s_icdWin[0][0], s_icdWin[0][1], s_icdWin[1][0], s_icdWin[1][1]);
    WriteFile(h, line, n, &w, NULL);
    for (i = 0; i < s_icdLogN; ) {
        DWORD tag = s_icdLog[i], cnt = (tag >> 28) == 0xa ? 5 : (tag >> 24) >= 0xb1 && (tag >> 24) <= 0xb3 ? 18 : tag == 0xb4000000 ? 4 : tag == 0xb5000000 ? 12 : (tag >> 28) == 0xe ? 1 : (tag >> 28) == 0xf ? 5 :
                    tag == 0xc0000006 ? 4 : (tag >> 28) == 0xc ? 16 : (tag >> 28) == 0xd ? 1 + ((tag >> 16) & 0xff) : 1;
        n = (DWORD)wsprintfA(line, "%08lx", tag);
        for (k = 1; k < cnt && i + k < s_icdLogN; k++) n += (DWORD)wsprintfA(line + n, " %08lx", s_icdLog[i + k]);
        line[n++] = '\r'; line[n++] = '\n';
        WriteFile(h, line, n, &w, NULL);
        i += cnt;
    }
}
/* icdMarkDump: ICD_MARK build only (our diagnostic code, not in the original DLL): append to C:\3DFXOGL.LOG the last
** 256 marks of the ring (file:line value esp), the frame count and the first 64 GL error records, then icdLogDump;
** called at process detach and from icdExceptionFilter (Kernel32 + wsprintf only: safe inside DllMain) */
void icdMarkDump(void)
{
    char line[200];
    DWORD n, w, k, first, count;
    HANDLE h = CreateFileA("C:\\3DFXOGL.LOG", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    SetFilePointer(h, 0, NULL, FILE_END);
    count = g_icdMarkN / 4;
    first = count > 256 ? count - 256 : 0;
    n = (DWORD)wsprintfA(line, "%lu marks recorded (showing %lu); columns: file:line value esp\r\n", count, count - first);
    WriteFile(h, line, n, &w, NULL);
    for (k = first; k < count; k++) {
        DWORD b = (k * 4) % 1024;
        n = (DWORD)wsprintfA(line, "  %s:%lu %08lx esp %08lx\r\n", (const char *)g_icdMarks[b], g_icdMarks[b + 1], g_icdMarks[b + 2], g_icdMarks[b + 3]);
        WriteFile(h, line, n, &w, NULL);
    }
    n = (DWORD)wsprintfA(line, "frames (SwapBuffers): %lu; GL errors: %lu (first 64: code, frame, stack)\r\n", g_icdSwapN, s_icdErrN);
    WriteFile(h, line, n, &w, NULL);
    for (k = 0; k < s_icdErrN && k < 64; k++) {
        const volatile DWORD *r = &s_icdErr[k * 32];
        DWORD j;
        n = (DWORD)wsprintfA(line, "  err %04lx frame %lu:", r[0], r[1]);
        WriteFile(h, line, n, &w, NULL);
        for (j = 2; j < 32; j++) {
            n = (DWORD)wsprintfA(line, " %08lx", r[j]);
            WriteFile(h, line, n, &w, NULL);
        }
        WriteFile(h, "\r\n", 2, &w, NULL);
    }
    icdLogDump(h);
    CloseHandle(h);
}
#endif

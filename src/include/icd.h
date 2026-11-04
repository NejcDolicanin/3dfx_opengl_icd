/* icd.h -- the opengl32 <-> ICD interface types used by drv.c (from the public ICD DDK headers) */
#ifndef __ICD_H__
#define __ICD_H__
#include <windows.h>
typedef ULONG DHGLRC;
typedef ULONG DHPDEV;
typedef struct _GLCLTPROCTABLE *PGLCLTPROCTABLE;
typedef VOID (APIENTRY *PFN_SETPROCTABLE)(PGLCLTPROCTABLE);
BOOL  WINAPI DrvValidateVersion(ULONG);
PROC  WINAPI DrvGetProcAddress(LPCSTR);
LONG  WINAPI DrvDescribePixelFormat(HDC, INT, UINT, LPPIXELFORMATDESCRIPTOR);
BOOL  WINAPI DrvSetPixelFormat(HDC, INT);
HGLRC WINAPI DrvCreateContext(HDC);
HGLRC WINAPI DrvCreateLayerContext(HDC, INT);
BOOL  WINAPI DrvDeleteContext(DHGLRC);
PGLCLTPROCTABLE WINAPI DrvSetContext(HDC, DHGLRC, PFN_SETPROCTABLE);
BOOL  WINAPI DrvReleaseContext(DHGLRC);
BOOL  WINAPI DrvCopyContext(DHGLRC, DHGLRC, UINT);
BOOL  WINAPI DrvShareLists(DHGLRC, DHGLRC);
BOOL  WINAPI DrvSwapBuffers(HDC);
BOOL  WINAPI DrvSwapLayerBuffers(HDC, UINT);
BOOL  WINAPI DrvDescribeLayerPlane(HDC, INT, INT, UINT, LPLAYERPLANEDESCRIPTOR);
BOOL  WINAPI DrvRealizeLayerPalette(HDC, INT, BOOL);
int   WINAPI DrvGetLayerPaletteEntries(HDC, INT, INT, INT, COLORREF *);
int   WINAPI DrvSetLayerPaletteEntries(HDC, INT, INT, INT, CONST COLORREF *);
#endif

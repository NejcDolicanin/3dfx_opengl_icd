/*
** gl_disp.c -- the dispatch tables (SI s_disp.c; 3dfx S_DISP.C): gc->currentDispatchState and gc->listCompState are
** copied from the immediate / list-compile pair of the path in use (wgldev.c, gl_export.c), and the display-list
** execute tables gc->dlist.baseListExec / listExec point at.  Slot i (1 .. 0x150) serves g_glEntryNames[i - 1]
** (glstubs.c); slot 0 is the count.
*/
#include "wglpriv.h"
#include "glstubs.h"

/* the procs the tables hold: only their names and calling conventions matter here (the argument lists give the
** original's argument sizes) */
extern int __stdcall __glsstim_AlphaFunc(GLuint, float);
extern int __stdcall __glsstim_BlendFunc(GLuint, GLuint);
extern int __stdcall __glsstim_BlendEquationEXT(int);
extern int __stdcall __glsstim_ColorMask(GLubyte, GLubyte, GLubyte, GLubyte);
extern int __stdcall __glsstim_CullFace(GLuint);
extern int __stdcall __glsstim_DepthFunc(GLuint);
extern int __stdcall __glsstim_DepthMask(GLubyte);
extern int __stdcall __glsstim_DrawBuffer(GLuint);
extern int __stdcall __glsstim_Fogfv(GLuint, float *);
extern int __stdcall __glsstim_Fogf(GLuint, int);
extern int __stdcall __glsstim_Fogiv(GLuint, GLuint *);
extern int __stdcall __glsstim_Fogi(GLuint, int);
extern int __stdcall __glsstim_FrontFace(GLuint);
extern int __stdcall __glsstim_StencilMask(GLushort);
extern int __stdcall __glsstim_StencilFunc(GLuint, int, GLushort);
extern int __stdcall __glsstim_StencilOp(GLuint, GLuint, GLuint);
extern int __stdcall __glsstim_Enable(GLuint);
extern int __stdcall __glsstim_Disable(GLuint);
extern int __stdcall __glsstim_PointParameterfEXT(GLuint, GLuint);
extern int __stdcall __glsstim_PointParameterfvEXT(GLuint, GLuint *);
extern int __fastcall __glsstim_PopAttrib(void);
extern int __stdcall __glsstim_Begin(GLuint);
extern int __fastcall __glsstim_End(void);
extern int __stdcall __glsstim_Color3ub(GLuint, GLuint, GLuint);
extern int __stdcall __glsstim_Color3ubv(GLubyte *);
extern int __stdcall __glsstim_Color3b(char, char, char);
extern int __stdcall __glsstim_Color3bv(char *);
extern int __stdcall __glsstim_Color3d(double, double, double);
extern int __stdcall __glsstim_Color3dv(double *);
extern int __stdcall __glsstim_Color3f(float, float, float);
extern int __stdcall __glsstim_Color3fv(float *);
extern int __stdcall __glsstim_Color3i(int, int, int);
extern int __stdcall __glsstim_Color3iv(int *);
extern int __stdcall __glsstim_Color3ui(GLuint, GLuint, GLuint);
extern int __stdcall __glsstim_Color3uiv(GLuint *);
extern int __stdcall __glsstim_Color3s(short, short, short);
extern int __stdcall __glsstim_Color3sv(short *);
extern int __stdcall __glsstim_Color3us(GLushort, GLuint, GLushort);
extern int __stdcall __glsstim_Color3usv(GLushort *);
extern int __stdcall __glsstim_Color4ub(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glsstim_Color4ubv(GLuint *);
extern int __stdcall __glsstim_Color4b(char, char, char, char);
extern int __stdcall __glsstim_Color4bv(char *);
extern int __stdcall __glsstim_Color4d(double, double, double, double);
extern int __stdcall __glsstim_Color4dv(double *);
extern int __stdcall __glsstim_Color4f(float, float, float, float);
extern int __stdcall __glsstim_Color4fv(float *);
extern int __stdcall __glsstim_Color4i(int, int, int, int);
extern int __stdcall __glsstim_Color4iv(int *);
extern int __stdcall __glsstim_Color4ui(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glsstim_Color4uiv(GLuint *);
extern int __stdcall __glsstim_Color4s(short, short, short, short);
extern int __stdcall __glsstim_Color4sv(short *);
extern int __stdcall __glsstim_Color4us(GLushort, GLushort, GLushort, GLuint);
extern int __stdcall __glsstim_Color4usv(GLushort *);
extern int __stdcall __glsstim_LockArraysEXT(int, int);
extern int __fastcall __glsstim_UnlockArraysEXT(void);
extern int __stdcall __glsstim_DrawElements(GLuint, GLuint, int, int *);
extern int __stdcall __glsstim_TexParameterfv(GLuint, GLuint, float *);
extern int __stdcall __glsstim_TexParameterf(GLuint, GLuint, int);
extern int __stdcall __glsstim_TexParameteriv(GLuint, GLuint, GLuint *);
extern int __stdcall __glsstim_TexParameteri(GLuint, GLuint, int);
extern int __stdcall __glsstim_TexEnvfv(GLuint, GLuint, float *);
extern int __stdcall __glsstim_TexEnvf(GLuint, GLuint, int);
extern int __stdcall __glsstim_TexEnviv(GLuint, GLuint, GLuint *);
extern int __stdcall __glsstim_TexEnvi(GLuint, GLuint, int);
extern int __stdcall __glsstim_TexImage1D(GLuint, int, int, int, int, GLuint, GLuint, int);
extern int __stdcall __glsstim_TexImage2D(int, int, int, int, int, int, int, int, GLuint *);
extern int __stdcall __glsstim_TexSubImage1D(GLuint, int, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glsstim_TexSubImage2D(GLuint, int, int *, int, GLuint, int, int, GLushort *, GLubyte *);
extern int __stdcall __glsstim_CompressedTexImage2DARB(GLuint, int, int, int, int, int, GLuint, GLuint *);
extern int __stdcall __glsstim_CompressedTexSubImage2DARB(GLuint, int, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint *);
extern int __stdcall __glsstim_GetCompressedTexImageARB(int, int, GLuint *);
extern int __stdcall __glsstim_BindTexture(int, GLuint);
extern int __stdcall __glsstim_ReadBuffer(GLuint);
extern int __stdcall __glsstim_ActiveTextureARB(GLuint);
extern int __stdcall __glsstim_ClientActiveTextureARB(GLuint);
extern int __stdcall __glsstim_MultiTexCoord1dARB(GLuint, double);
extern int __stdcall __glsstim_MultiTexCoord1dvARB(GLuint, double *);
extern int __stdcall __glsstim_MultiTexCoord1fARB(GLuint, GLuint);
extern int __stdcall __glsstim_MultiTexCoord1fvARB(GLuint, GLuint *);
extern int __stdcall __glsstim_MultiTexCoord1iARB(GLuint, int);
extern int __stdcall __glsstim_MultiTexCoord1ivARB(GLuint, int *);
extern int __stdcall __glsstim_MultiTexCoord1sARB(GLuint, short);
extern int __stdcall __glsstim_MultiTexCoord1svARB(GLuint, short *);
extern int __stdcall __glsstim_MultiTexCoord2dARB(GLuint, double, double);
extern int __stdcall __glsstim_MultiTexCoord2dvARB(GLuint, double *);
extern int __stdcall __glsstim_MultiTexCoord2fARB(GLuint, GLuint, GLuint);
extern int __stdcall __glsstim_MultiTexCoord2fvARB(GLuint, GLuint *);
extern int __stdcall __glsstim_MultiTexCoord2iARB(GLuint, int, int);
extern int __stdcall __glsstim_MultiTexCoord2ivARB(GLuint, int *);
extern int __stdcall __glsstim_MultiTexCoord2sARB(GLuint, short, short);
extern int __stdcall __glsstim_MultiTexCoord2svARB(GLuint, short *);
extern int __stdcall __glsstim_MultiTexCoord3dARB(GLuint, double, double, double);
extern int __stdcall __glsstim_MultiTexCoord3dvARB(GLuint, double *);
extern int __stdcall __glsstim_MultiTexCoord3fARB(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glsstim_MultiTexCoord3fvARB(GLuint, GLuint *);
extern int __stdcall __glsstim_MultiTexCoord3iARB(GLuint, int, int, int);
extern int __stdcall __glsstim_MultiTexCoord3ivARB(GLuint, int *);
extern int __stdcall __glsstim_MultiTexCoord3sARB(GLuint, short, short, short);
extern int __stdcall __glsstim_MultiTexCoord3svARB(GLuint, short *);
extern int __stdcall __glsstim_MultiTexCoord4dARB(GLuint, double, double, double, double);
extern int __stdcall __glsstim_MultiTexCoord4dvARB(GLuint, double *);
extern int __stdcall __glsstim_MultiTexCoord4fARB(GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glsstim_MultiTexCoord4fvARB(GLuint, GLuint *);
extern int __stdcall __glsstim_MultiTexCoord4iARB(GLuint, int, int, int, int);
extern int __stdcall __glsstim_MultiTexCoord4ivARB(GLuint, int *);
extern int __stdcall __glsstim_MultiTexCoord4sARB(GLuint, short, short, short, short);
extern int __stdcall __glsstim_MultiTexCoord4svARB(GLuint, short *);
extern int __stdcall __glsstim_ColorSubTableEXT(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glsstim_ColorTable(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glsstim_GetTexLevelParameteriv(GLuint, int, int, GLuint *);
extern int __stdcall __glsstim_GetTexLevelParameterfv(GLuint, int, int, float *);
extern int __fastcall swapReturnAddrThunk(GLuint, GLuint);
extern int __stdcall __glsstim_Clear(GLuint);
extern int __stdcall __glim_Begin(GLuint);
extern int __fastcall __glim_End(void);
extern int __stdcall __glim_AddSwapHintRectWIN(GLuint, GLuint, int, int);
extern GLuint __stdcall __glim_RenderMode(GLuint);
extern int __stdcall __gllc_Begin(GLuint);
extern int __fastcall __glle_Begin_LineLoop(int);
extern int __fastcall __glle_Begin_LineStrip(int);
extern int __fastcall __glle_Begin_Lines(int);
extern int __fastcall __glle_Begin_Points(int);
extern int __fastcall __glle_Begin_Polygon(int);
extern int __fastcall __glle_Begin_TriangleStrip(int);
extern int __fastcall __glle_Begin_TriangleFan(int);
extern int __fastcall __glle_Begin_Triangles(int);
extern int __fastcall __glle_Begin_QuadStrip(int);
extern int __fastcall __glle_Begin_Quads(int);
extern GLuint __fastcall __glle_InvalidValue(GLuint);
extern GLuint __fastcall __glle_InvalidEnum(GLuint);
extern GLuint __fastcall __glle_InvalidOperation(GLuint);
extern GLuint __fastcall __glle_TableTooLarge(GLuint);
extern GLuint __fastcall __glle_UnimplementedExtension(GLuint);
extern int __stdcall __glim_MatrixMode(GLuint);
extern int __stdcall __glim_LoadIdentity(void);
extern int __stdcall __glim_LoadMatrixf(GLuint *);
extern int __stdcall __glim_LoadMatrixd(double *);
extern int __stdcall __glim_MultMatrixf(GLuint *);
extern int __stdcall __glim_MultMatrixd(double *);
extern int __stdcall __glim_Rotatef(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glim_Rotated(double, double, double, double);
extern int __stdcall __glim_Scalef(GLuint, GLuint, GLuint);
extern int __stdcall __glim_Scaled(double, double, double);
extern int __stdcall __glim_Translatef(GLuint, GLuint, GLuint);
extern int __stdcall __glim_Translated(double, double, double);
extern int __stdcall __glim_PushMatrix(void);
extern int __stdcall __glim_PopMatrix(void);
extern int __stdcall __glim_Frustum(double, double, double, double, double, double);
extern int __stdcall __glim_Ortho(double, double, double, double, double, double);
extern int __stdcall __glim_Viewport(GLuint, GLuint, int, int);
extern int __stdcall __glim_DepthRange(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glim_Scissor(int, int, int, int);
extern int __stdcall __glim_ClipPlane(GLuint, double *);
extern int __stdcall __glim_ColorSubTable(int, int, int, int, int, int);
extern int __stdcall __glim_ColorTable(GLuint, int, int, int, int, int);
extern int __stdcall __glim_GetColorTable(GLuint, GLuint, GLuint, int);
extern int __stdcall __glim_GetColorTableParameterivEXT(GLuint, GLuint, GLuint);
extern int __stdcall __glim_GetColorTableParameterfvEXT(GLuint, GLuint, GLuint);
extern int __stdcall __glim_CopyColorTableEXT(GLuint, GLuint, GLuint, GLuint, int);
extern int __stdcall __glim_Rectd(double, double, double, double);
extern int __stdcall __glim_Rectdv(double *, double *);
extern int __stdcall __glim_Rectf(int, int, int, int);
extern int __stdcall __glim_Rectfv(int *, int *);
extern int __stdcall __glim_Recti(int, int, int, int);
extern int __stdcall __glim_Rectiv(int *, int *);
extern int __stdcall __glim_Rects(short, short, short, short);
extern int __stdcall __glim_Rectsv(short *, short *);
extern int __stdcall __glim_Map1d(int, double, double, int, GLuint, GLuint);
extern int __stdcall __glim_Map1f(int, GLuint, GLuint, int, GLuint, GLuint);
extern int __stdcall __glim_Map2d(int, double, double, int, GLuint, double, double, int, GLuint, GLuint);
extern int __stdcall __glim_Map2f(int, GLuint, GLuint, int, GLuint, GLuint, GLuint, int, GLuint, GLuint);
extern int __stdcall __glim_EvalCoord1d(double);
extern int __stdcall __glim_EvalCoord1dv(double *);
extern int __stdcall __glim_EvalCoord1f(int);
extern int __stdcall __glim_EvalCoord1fv(int *);
extern int __stdcall __glim_EvalCoord2d(double, double);
extern int __stdcall __glim_EvalCoord2dv(double *);
extern int __stdcall __glim_EvalCoord2f(int, int);
extern int __stdcall __glim_EvalCoord2fv(int *);
extern int __stdcall __glim_MapGrid1d(int, double, double);
extern int __stdcall __glim_MapGrid1f(int, GLuint, GLuint);
extern int __stdcall __glim_MapGrid2d(int, double, double, int, double, double);
extern int __stdcall __glim_MapGrid2f(int, GLuint, GLuint, int, GLuint, GLuint);
extern int __stdcall __glim_EvalMesh1(int, GLuint, GLuint);
extern int __stdcall __glim_EvalPoint1(int);
extern int __stdcall __glim_EvalMesh2(int, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glim_EvalPoint2(float, float);
extern int __stdcall __glim_Accum(int, int);
extern int __stdcall __glim_TexGenfv(int, int, float *);
extern int __stdcall __glim_TexGenf(GLuint, int, int);
extern int __stdcall __glim_TexGendv(int, int, double *);
extern int __stdcall __glim_TexGend(GLuint, int, int, int);
extern int __stdcall __glim_TexGeniv(int, int, GLuint *);
extern int __stdcall __glim_TexGeni(GLuint, int, int);
extern int __stdcall __glim_TexParameterfv(GLuint, GLuint, float *);
extern int __stdcall __glim_TexParameterf(GLuint, GLuint, int);
extern int __stdcall __glim_TexParameteriv(GLuint, GLuint, GLuint *);
extern int __stdcall __glim_TexParameteri(GLuint, GLuint, int);
extern int __stdcall __glim_TexEnvfv(GLuint, GLuint, float *);
extern int __stdcall __glim_TexEnvf(GLuint, GLuint, int);
extern int __stdcall __glim_TexEnviv(GLuint, GLuint, GLuint *);
extern int __stdcall __glim_TexEnvi(GLuint, GLuint, int);
extern int __stdcall __glim_TexImage1D(GLuint, int, int, int, int, GLuint, GLuint, int);
extern int __stdcall __glim_TexImage2D(GLuint, int, int, int, int, int, GLuint, GLuint, int);
extern int __fastcall __glCalcMixedTexture(int, int);
extern int __stdcall __glim_TexSubImage1D(GLuint, int, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glim_TexSubImage2D(GLuint, int, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glim_GenTextures(int, int);
extern int __stdcall __glim_DeleteTextures(int, int *);
extern int __stdcall __glim_BindTexture(int, GLuint);
extern int __stdcall __glim_PrioritizeTextures(int, int *, int);
extern GLboolean __stdcall __glim_AreTexturesResident(int, int *, int);
extern GLboolean __stdcall __glim_IsTexture(int);
extern int __stdcall __glim_CopyTexImage1D(GLuint, int, int, GLuint, GLuint, int, int);
extern int __stdcall __glim_CopyTexImage2D(GLuint, int, int, GLuint, GLuint, int, int, int);
extern int __stdcall __glim_CopyTexSubImage1D(GLuint, int, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glim_CopyTexSubImage2D(GLuint, int, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glim_EdgeFlag(char);
extern int __stdcall __glim_EdgeFlagv(char *);
extern int __stdcall __glim_Vertex2fv(GLuint *);
extern int __stdcall __glim_Vertex3fv(GLuint *);
extern int __stdcall __glim_Vertex4fv(GLuint *);
extern int __stdcall __glim_Vertex2f(int, int);
extern int __stdcall __glim_Vertex2d(double, double);
extern int __stdcall __glim_Vertex2dv(double *);
extern int __stdcall __glim_Vertex2i(int, int);
extern int __stdcall __glim_Vertex2iv(int *);
extern int __stdcall __glim_Vertex2s(short, short);
extern int __stdcall __glim_Vertex2sv(short *);
extern int __stdcall __glim_Vertex3f(int, int, int);
extern int __stdcall __glim_Vertex3d(double, double, double);
extern int __stdcall __glim_Vertex3dv(double *);
extern int __stdcall __glim_Vertex3i(int, int, int);
extern int __stdcall __glim_Vertex3iv(int *);
extern int __stdcall __glim_Vertex3s(short, short, short);
extern int __stdcall __glim_Vertex3sv(short *);
extern int __stdcall __glim_Vertex4f(int, int, int, int);
extern int __stdcall __glim_Vertex4d(double, double, double, double);
extern int __stdcall __glim_Vertex4dv(double *);
extern int __stdcall __glim_Vertex4i(int, int, int, int);
extern int __stdcall __glim_Vertex4iv(int *);
extern int __stdcall __glim_Vertex4s(short, short, short, short);
extern int __stdcall __glim_Vertex4sv(short *);
extern int __stdcall __glim_Color3ub(GLuint, GLuint, GLuint);
extern int __stdcall __glim_Color3ubv(GLubyte *);
extern int __stdcall __glim_Color3b(char, char, char);
extern int __stdcall __glim_Color3bv(char *);
extern int __stdcall __glim_Color3d(double, double, double);
extern int __stdcall __glim_Color3dv(double *);
extern int __stdcall __glim_Color3f(GLuint, int, GLuint);
extern int __stdcall __glim_Color3fv(GLuint *);
extern int __stdcall __glim_Color3i(int, int, int);
extern int __stdcall __glim_Color3iv(int *);
extern int __stdcall __glim_Color3ui(GLuint, GLuint, GLuint);
extern int __stdcall __glim_Color3uiv(GLuint *);
extern int __stdcall __glim_Color3s(short, short, short);
extern int __stdcall __glim_Color3sv(short *);
extern int __stdcall __glim_Color3us(GLushort, GLuint, GLushort);
extern int __stdcall __glim_Color3usv(GLushort *);
extern int __stdcall __glim_Color4ub(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glim_Color4ubv(GLubyte *);
extern int __stdcall __glim_Color4b(char, char, char, char);
extern int __stdcall __glim_Color4bv(char *);
extern int __stdcall __glim_Color4d(double, double, double, double);
extern int __stdcall __glim_Color4dv(double *);
extern int __stdcall __glim_Color4f(GLuint, GLuint, GLuint, int);
extern int __stdcall __glim_Color4fv(GLuint *);
extern int __stdcall __glim_Color4i(int, int, int, int);
extern int __stdcall __glim_Color4iv(int *);
extern int __stdcall __glim_Color4ui(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glim_Color4uiv(GLuint *);
extern int __stdcall __glim_Color4s(short, short, short, short);
extern int __stdcall __glim_Color4sv(short *);
extern int __stdcall __glim_Color4us(GLushort, GLushort, GLushort, GLuint);
extern int __stdcall __glim_Color4usv(GLushort *);
extern int __stdcall __glim_Indexd(double);
extern int __stdcall __glim_Indexf(GLuint);
extern int __stdcall __glim_Indexi(int);
extern int __stdcall __glim_Indexs(short);
extern int __stdcall __glim_Indexub(GLubyte);
extern int __stdcall __glim_Indexdv(double *);
extern int __stdcall __glim_Indexfv(GLuint *);
extern int __stdcall __glim_Indexiv(int *);
extern int __stdcall __glim_Indexsv(short *);
extern int __stdcall __glim_Indexubv(GLubyte *);
extern int __stdcall __gllc_ListBase(GLuint);
extern int __stdcall __gllc_Color3b(GLubyte, GLubyte, GLubyte);
extern int __stdcall __gllc_Color3bv(GLubyte *);
extern int __stdcall __gllc_Color3d(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Color3dv(GLuint *);
extern int __stdcall __gllc_Color3f(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Color3fv(GLuint *);
extern int __stdcall __gllc_Color3i(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Color3iv(GLuint *);
extern int __stdcall __gllc_Color3s(GLushort, GLushort, GLushort);
extern int __stdcall __gllc_Color3sv(GLushort *);
extern int __stdcall __gllc_Color3ub(GLubyte, GLubyte, GLubyte);
extern int __stdcall __gllc_Color3ubv(GLubyte *);
extern int __stdcall __gllc_Color3ui(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Color3uiv(GLuint *);
extern int __stdcall __gllc_Color3us(GLushort, GLushort, GLushort);
extern int __stdcall __gllc_Color3usv(GLushort *);
extern int __stdcall __gllc_Color4b(GLubyte, GLubyte, GLubyte, GLubyte);
extern int __stdcall __gllc_Color4bv(GLubyte *);
extern int __stdcall __gllc_Color4d(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Color4dv(GLuint *);
extern int __stdcall __gllc_Color4f(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Color4fv(GLuint *);
extern int __stdcall __gllc_Color4i(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Color4iv(GLuint *);
extern int __stdcall __gllc_Color4s(GLushort, GLushort, GLushort, GLushort);
extern int __stdcall __gllc_Color4sv(GLushort *);
extern int __stdcall __gllc_Color4ub(GLubyte, GLubyte, GLubyte, GLubyte);
extern int __stdcall __gllc_Color4ubv(GLubyte *);
extern int __stdcall __gllc_Color4ui(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Color4uiv(GLuint *);
extern int __stdcall __gllc_Color4us(GLushort, GLushort, GLushort, GLushort);
extern int __stdcall __gllc_Color4usv(GLushort *);
extern int __stdcall __gllc_EdgeFlag(GLubyte);
extern int __stdcall __gllc_EdgeFlagv(GLubyte *);
extern int __fastcall __gllc_End(void);
extern int __stdcall __gllc_Indexd(GLuint, GLuint);
extern int __stdcall __gllc_Indexdv(GLuint *);
extern int __stdcall __gllc_Indexf(GLuint);
extern int __stdcall __gllc_Indexfv(GLuint *);
extern int __stdcall __gllc_Indexi(GLuint);
extern int __stdcall __gllc_Indexiv(GLuint *);
extern int __stdcall __gllc_Indexs(GLushort);
extern int __stdcall __gllc_Indexsv(GLushort *);
extern int __stdcall __gllc_Normal3b(GLubyte, GLubyte, GLubyte);
extern int __stdcall __gllc_Normal3bv(GLubyte *);
extern int __stdcall __gllc_Normal3d(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Normal3dv(GLuint *);
extern int __stdcall __gllc_Normal3f(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Normal3fv(GLuint *);
extern int __stdcall __gllc_Normal3i(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Normal3iv(GLuint *);
extern int __stdcall __gllc_Normal3s(GLushort, GLushort, GLushort);
extern int __stdcall __gllc_Normal3sv(GLushort *);
extern int __stdcall __gllc_RasterPos2d(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_RasterPos2dv(GLuint *);
extern int __stdcall __gllc_RasterPos2f(GLuint, GLuint);
extern int __stdcall __gllc_RasterPos2fv(GLuint *);
extern int __stdcall __gllc_RasterPos2i(GLuint, GLuint);
extern int __stdcall __gllc_RasterPos2iv(GLuint *);
extern int __stdcall __gllc_RasterPos2s(GLushort, GLushort);
extern int __stdcall __gllc_RasterPos2sv(GLushort *);
extern int __stdcall __gllc_RasterPos3d(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_RasterPos3dv(GLuint *);
extern int __stdcall __gllc_RasterPos3f(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_RasterPos3fv(GLuint *);
extern int __stdcall __gllc_RasterPos3i(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_RasterPos3iv(GLuint *);
extern int __stdcall __gllc_RasterPos3s(GLushort, GLushort, GLushort);
extern int __stdcall __gllc_RasterPos3sv(GLushort *);
extern int __stdcall __gllc_RasterPos4d(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_RasterPos4dv(GLuint *);
extern int __stdcall __gllc_RasterPos4f(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_RasterPos4fv(GLuint *);
extern int __stdcall __gllc_RasterPos4i(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_RasterPos4iv(GLuint *);
extern int __stdcall __gllc_RasterPos4s(GLushort, GLushort, GLushort, GLushort);
extern int __stdcall __gllc_RasterPos4sv(GLushort *);
extern int __stdcall __gllc_Rectd(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Rectdv(GLuint *, GLuint *);
extern int __stdcall __gllc_Rectf(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Rectfv(GLuint *, GLuint *);
extern int __stdcall __gllc_Recti(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Rectiv(GLuint *, GLuint *);
extern int __stdcall __gllc_Rects(GLushort, GLushort, GLushort, GLushort);
extern int __stdcall __gllc_Rectsv(GLushort *, GLushort *);
extern int __stdcall __gllc_TexCoord1d(GLuint, GLuint);
extern int __stdcall __gllc_TexCoord1dv(GLuint *);
extern int __stdcall __gllc_TexCoord1f(GLuint);
extern int __stdcall __gllc_TexCoord1fv(GLuint *);
extern int __stdcall __gllc_TexCoord1i(GLuint);
extern int __stdcall __gllc_TexCoord1iv(GLuint *);
extern int __stdcall __gllc_TexCoord1s(GLushort);
extern int __stdcall __gllc_TexCoord1sv(GLushort *);
extern int __stdcall __gllc_TexCoord2d(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_TexCoord2dv(GLuint *);
extern int __stdcall __gllc_TexCoord2f(GLuint, GLuint);
extern int __stdcall __gllc_TexCoord2fv(GLuint *);
extern int __stdcall __gllc_TexCoord2i(GLuint, GLuint);
extern int __stdcall __gllc_TexCoord2iv(GLuint *);
extern int __stdcall __gllc_TexCoord2s(GLushort, GLushort);
extern int __stdcall __gllc_TexCoord2sv(GLushort *);
extern int __stdcall __gllc_TexCoord3d(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_TexCoord3dv(GLuint *);
extern int __stdcall __gllc_TexCoord3f(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_TexCoord3fv(GLuint *);
extern int __stdcall __gllc_TexCoord3i(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_TexCoord3iv(GLuint *);
extern int __stdcall __gllc_TexCoord3s(GLushort, GLushort, GLushort);
extern int __stdcall __gllc_TexCoord3sv(GLushort *);
extern int __stdcall __gllc_TexCoord4d(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_TexCoord4dv(GLuint *);
extern int __stdcall __gllc_TexCoord4f(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_TexCoord4fv(GLuint *);
extern int __stdcall __gllc_TexCoord4i(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_TexCoord4iv(GLuint *);
extern int __stdcall __gllc_TexCoord4s(GLushort, GLushort, GLushort, GLushort);
extern int __stdcall __gllc_TexCoord4sv(GLushort *);
extern int __stdcall __gllc_Vertex2d(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Vertex2dv(GLuint *);
extern int __stdcall __gllc_Vertex2f(GLuint, GLuint);
extern int __stdcall __gllc_Vertex2fv(GLuint *);
extern int __stdcall __gllc_Vertex2i(GLuint, GLuint);
extern int __stdcall __gllc_Vertex2iv(GLuint *);
extern int __stdcall __gllc_Vertex2s(GLushort, GLushort);
extern int __stdcall __gllc_Vertex2sv(GLushort *);
extern int __stdcall __gllc_Vertex3d(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Vertex3dv(GLuint *);
extern int __stdcall __gllc_Vertex3f(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Vertex3fv(GLuint *);
extern int __stdcall __gllc_Vertex3i(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Vertex3iv(GLuint *);
extern int __stdcall __gllc_Vertex3s(GLushort, GLushort, GLushort);
extern int __stdcall __gllc_Vertex3sv(GLushort *);
extern int __stdcall __gllc_Vertex4d(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Vertex4dv(GLuint *);
extern int __stdcall __gllc_Vertex4f(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Vertex4fv(GLuint *);
extern int __stdcall __gllc_Vertex4i(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Vertex4iv(GLuint *);
extern int __stdcall __gllc_Vertex4s(GLushort, GLushort, GLushort, GLushort);
extern int __stdcall __gllc_Vertex4sv(GLushort *);
extern int __stdcall __gllc_ClipPlane(GLuint, GLuint *);
extern int __stdcall __gllc_ColorMaterial(GLuint, GLuint);
extern int __stdcall __gllc_CullFace(GLuint);
extern int __stdcall __gllc_Fogf(GLuint, GLuint);
extern int __stdcall __gllc_Fogfv(GLuint, GLuint *);
extern int __stdcall __gllc_Fogi(GLuint, GLuint);
extern int __stdcall __gllc_Fogiv(GLuint, GLuint *);
extern int __stdcall __gllc_FrontFace(GLuint);
extern int __stdcall __gllc_Hint(GLuint, GLuint);
extern int __stdcall __gllc_Lightf(GLuint, GLuint, int);
extern int __fastcall __gllc_Lightfv(GLuint, GLuint, GLuint, GLuint, GLuint *);
extern int __stdcall __gllc_Lighti(GLuint, GLuint, int);
extern int __stdcall __gllc_Lightiv(GLuint, GLuint, GLuint *);
extern int __stdcall __gllc_LightModelf(GLuint, int);
extern int __stdcall __gllc_LightModelfv(GLuint, GLuint *);
extern int __stdcall __gllc_LightModeli(GLuint, int);
extern int __stdcall __gllc_LightModeliv(GLuint, GLuint *);
extern int __stdcall __gllc_LineStipple(GLuint, GLushort);
extern int __stdcall __gllc_LineWidth(GLuint);
extern int __stdcall __gllc_Materialf(GLuint, GLuint, int);
extern int __stdcall __gllc_Materialfv(GLuint, GLuint, GLuint *);
extern int __stdcall __gllc_Materiali(GLuint, GLuint, int);
extern int __stdcall __gllc_Materialiv(GLuint, GLuint, int *);
extern int __stdcall __gllc_PointSize(GLuint);
extern int __stdcall __gllc_PolygonMode(GLuint, GLuint);
extern int __stdcall __gllc_Scissor(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_ShadeModel(GLuint);
extern int __stdcall __gllc_TexParameterf(GLuint, GLuint, int);
extern int __fastcall __gllc_TexParameterfv(GLuint, GLuint, GLuint, GLuint, GLuint *);
extern int __stdcall __gllc_TexParameteri(GLuint, GLuint, int);
extern int __stdcall __gllc_TexParameteriv(GLuint, GLuint, GLuint *);
extern int __stdcall __gllc_TexEnvf(GLuint, GLuint, int);
extern int __fastcall __gllc_TexEnvfv(GLuint, GLuint, GLuint, GLuint, GLuint *);
extern int __stdcall __gllc_TexEnvi(GLuint, GLuint, int);
extern int __stdcall __gllc_TexEnviv(GLuint, GLuint, GLuint *);
extern int __stdcall __gllc_TexGend(GLuint, GLuint, int, int);
extern int __fastcall __gllc_TexGendv(GLuint, GLuint, GLuint, GLuint, GLuint *);
extern int __stdcall __gllc_TexGenf(GLuint, GLuint, int);
extern int __stdcall __gllc_TexGenfv(GLuint, GLuint, GLuint *);
extern int __stdcall __gllc_TexGeni(GLuint, GLuint, int);
extern int __stdcall __gllc_TexGeniv(GLuint, GLuint, GLuint *);
extern int __fastcall __gllc_InitNames(void);
extern int __stdcall __gllc_LoadName(GLuint);
extern int __stdcall __gllc_PassThrough(GLuint);
extern int __fastcall __gllc_PopName(void);
extern int __stdcall __gllc_PushName(GLuint);
extern int __stdcall __gllc_DrawBuffer(GLuint);
extern int __stdcall __gllc_Clear(GLuint);
extern int __stdcall __gllc_ClearAccum(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_ClearIndex(GLuint);
extern int __stdcall __gllc_ClearColor(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_ClearStencil(GLuint);
extern int __stdcall __gllc_ClearDepth(GLuint, GLuint);
extern int __stdcall __gllc_StencilMask(GLuint);
extern int __stdcall __gllc_ColorMask(GLubyte, GLubyte, GLubyte, GLubyte);
extern int __stdcall __gllc_DepthMask(GLubyte);
extern int __stdcall __gllc_IndexMask(GLuint);
extern int __stdcall __gllc_Accum(GLuint, GLuint);
extern int __fastcall __gllc_PopAttrib(void);
extern int __stdcall __gllc_PushAttrib(GLuint);
extern int __stdcall __gllc_MapGrid1d(GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_MapGrid1f(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_MapGrid2d(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_MapGrid2f(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_EvalCoord1d(GLuint, GLuint);
extern int __stdcall __gllc_EvalCoord1dv(GLuint *);
extern int __stdcall __gllc_EvalCoord1f(GLuint);
extern int __stdcall __gllc_EvalCoord1fv(GLuint *);
extern int __stdcall __gllc_EvalCoord2d(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_EvalCoord2dv(GLuint *);
extern int __stdcall __gllc_EvalCoord2f(GLuint, GLuint);
extern int __stdcall __gllc_EvalCoord2fv(GLuint *);
extern int __stdcall __gllc_EvalMesh1(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_EvalPoint1(GLuint);
extern int __stdcall __gllc_EvalMesh2(GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_EvalPoint2(GLuint, GLuint);
extern int __stdcall __gllc_AlphaFunc(GLuint, GLuint);
extern int __stdcall __gllc_BlendFunc(GLuint, GLuint);
extern int __stdcall __gllc_LogicOp(GLuint);
extern int __stdcall __gllc_StencilFunc(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_StencilOp(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_DepthFunc(GLuint);
extern int __stdcall __gllc_PixelZoom(GLuint, GLuint);
extern int __stdcall __gllc_PixelTransferf(GLuint, GLuint);
extern int __stdcall __gllc_PixelTransferi(GLuint, GLuint);
extern int __stdcall __gllc_PixelMapfv(GLuint, GLuint, GLuint *);
extern int __stdcall __gllc_PixelMapuiv(GLuint, GLuint, GLuint *);
extern int __stdcall __gllc_PixelMapusv(GLuint, int, GLuint *);
extern int __stdcall __gllc_ReadBuffer(GLuint);
extern int __stdcall __gllc_CopyPixels(GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_DepthRange(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Frustum(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __fastcall __gllc_LoadIdentity(void);
extern int __stdcall __gllc_LoadMatrixf(GLuint *);
extern int __stdcall __gllc_LoadMatrixd(GLuint *);
extern int __stdcall __gllc_MatrixMode(GLuint);
extern int __stdcall __gllc_MultMatrixf(GLuint *);
extern int __stdcall __gllc_MultMatrixd(GLuint *);
extern int __stdcall __gllc_Ortho(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __fastcall __gllc_PopMatrix(void);
extern int __fastcall __gllc_PushMatrix(void);
extern int __stdcall __gllc_Rotated(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Rotatef(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Scaled(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Scalef(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Translated(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Translatef(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_Viewport(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_CopyColorTableEXT(GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_PolygonOffset(GLuint, GLuint);
extern int __stdcall __gllc_CopyTexImage1D(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_CopyTexImage2D(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_CopyTexSubImage1D(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_CopyTexSubImage2D(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_BindTexture(GLuint, GLuint);
extern int __stdcall __gllc_PrioritizeTextures(GLuint, GLuint *, GLuint *);
extern int __stdcall __gllc_Indexub(GLubyte);
extern int __stdcall __gllc_Indexubv(GLubyte *);
extern int __stdcall __gllc_IndexFuncSGI(GLuint, GLuint);
extern int __stdcall __gllc_IndexMaterialSGI(GLuint, GLuint);
extern int __stdcall __gllc_ActiveTextureARB(GLuint);
extern int __stdcall __gllc_ClientActiveTextureARB(GLuint);
extern int __stdcall __gllc_MultiTexCoord1dARB(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_MultiTexCoord1dvARB(GLuint, GLuint *);
extern int __stdcall __gllc_MultiTexCoord1fARB(GLuint, GLuint);
extern int __stdcall __gllc_MultiTexCoord1fvARB(GLuint, GLuint *);
extern int __stdcall __gllc_MultiTexCoord1iARB(GLuint, GLuint);
extern int __stdcall __gllc_MultiTexCoord1ivARB(GLuint, GLuint *);
extern int __stdcall __gllc_MultiTexCoord1sARB(GLuint, GLushort);
extern int __stdcall __gllc_MultiTexCoord1svARB(GLuint, GLushort *);
extern int __stdcall __gllc_MultiTexCoord2dARB(GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_MultiTexCoord2dvARB(GLuint, GLuint *);
extern int __stdcall __gllc_MultiTexCoord2fARB(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_MultiTexCoord2fvARB(GLuint, GLuint *);
extern int __stdcall __gllc_MultiTexCoord2iARB(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_MultiTexCoord2ivARB(GLuint, GLuint *);
extern int __stdcall __gllc_MultiTexCoord2sARB(GLuint, GLushort, GLushort);
extern int __stdcall __gllc_MultiTexCoord2svARB(GLuint, GLushort *);
extern int __stdcall __gllc_MultiTexCoord3dARB(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_MultiTexCoord3dvARB(GLuint, GLuint *);
extern int __stdcall __gllc_MultiTexCoord3fARB(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_MultiTexCoord3fvARB(GLuint, GLuint *);
extern int __stdcall __gllc_MultiTexCoord3iARB(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_MultiTexCoord3ivARB(GLuint, GLuint *);
extern int __stdcall __gllc_MultiTexCoord3sARB(GLuint, GLushort, GLushort, GLushort);
extern int __stdcall __gllc_MultiTexCoord3svARB(GLuint, GLushort *);
extern int __stdcall __gllc_MultiTexCoord4dARB(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_MultiTexCoord4dvARB(GLuint, GLuint *);
extern int __stdcall __gllc_MultiTexCoord4fARB(GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_MultiTexCoord4fvARB(GLuint, GLuint *);
extern int __stdcall __gllc_MultiTexCoord4iARB(GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __gllc_MultiTexCoord4ivARB(GLuint, GLuint *);
extern int __stdcall __gllc_MultiTexCoord4sARB(GLuint, GLushort, GLushort, GLushort, GLushort);
extern int __stdcall __gllc_MultiTexCoord4svARB(GLuint, GLushort *);
extern int __stdcall __gllc_PointParameterfEXT(GLuint, GLuint);
extern int __stdcall __gllc_PointParameterfvEXT(GLuint, GLuint *);
extern int __stdcall __gllc_BlendEquationEXT(GLuint);
extern int __stdcall __gllc_CompressedTexImage1DARB(GLuint, GLuint, GLuint, GLuint, GLuint, int, GLuint *);
extern int __stdcall __gllc_CompressedTexImage2DARB(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, int, GLuint *);
extern int __stdcall __gllc_CompressedTexSubImage1DARB(GLuint, GLuint, GLuint, GLuint, GLuint, int, GLuint *);
extern int __stdcall __gllc_CompressedTexSubImage2DARB(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, int, GLuint *);
extern int __stdcall __gllc_TBufferMask3DFX(GLuint);
extern int __stdcall __gllc_FogCoordfEXT(GLuint);
extern int __stdcall __gllc_FogCoorddEXT(GLuint, GLuint);
extern int __stdcall __gllc_FogCoordfvEXT(GLuint *);
extern int __stdcall __gllc_FogCoorddvEXT(GLuint *);
extern int __stdcall __glim_LockArraysEXT(int, int);
extern int __fastcall __glim_UnlockArraysEXT(void);
extern int __stdcall __glim_InterleavedArrays(int, int, int);
extern int __stdcall __glim_ArrayElement(int);
extern int __stdcall __glim_ArrayElementEXT(GLuint);
extern int __stdcall __glim_DrawArrays(GLuint, int, int);
extern int __stdcall __glim_DrawArraysEXT(GLuint, GLuint, GLuint);
extern int __stdcall __glim_DrawElements(GLuint, GLuint *, int, GLuint *);
extern int __stdcall __glim_VertexPointer(int, GLuint, int, GLuint);
extern int __stdcall __glim_NormalPointer(GLuint, int, GLuint);
extern int __stdcall __glim_ColorPointer(int, GLuint, int, GLuint);
extern int __stdcall __glim_IndexPointer(GLuint, int, GLuint);
extern int __stdcall __glim_TexCoordPointer(int, GLuint, int, GLuint);
extern int __stdcall __glim_EdgeFlagPointer(int, GLuint);
extern int __stdcall __gllc_ArrayElement(GLuint);
extern int __stdcall __gllc_ArrayElementEXT(GLuint);
extern int __stdcall __gllc_DrawArrays(GLuint, int, int);
extern int __stdcall __gllc_DrawArraysEXT(GLuint, GLuint, GLuint);
extern int __stdcall __gllc_DrawElements(GLuint, GLuint *, int, GLuint *);
extern int __stdcall __glim_VertexPointerEXT(GLuint, GLuint, GLuint, int, GLuint);
extern int __stdcall __glim_NormalPointerEXT(GLuint, GLuint, int, GLuint);
extern int __stdcall __glim_ColorPointerEXT(GLuint, GLuint, GLuint, int, GLuint);
extern int __stdcall __glim_IndexPointerEXT(GLuint, GLuint, int, GLuint);
extern int __stdcall __glim_TexCoordPointerEXT(GLuint, GLuint, GLuint, int, GLuint);
extern int __stdcall __glim_EdgeFlagPointerEXT(GLuint, int, GLuint);
extern int __stdcall __glim_FogCoordPointerEXT(int, int, GLuint);
extern int __stdcall __gllc_Bitmap(int, int, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __fastcall __glle_Bitmap(int);
extern int __stdcall __gllc_PolygonStipple(GLuint);
extern int __fastcall __glle_PolygonStipple(int);
extern int __stdcall __gllc_Map1f(GLuint, float, float, int, int, GLuint);
extern int * __fastcall __glle_Map1(int *);
extern int __stdcall __gllc_Map1d(int, double, double, int, int, double *);
extern int __stdcall __gllc_Map2f(GLuint, float, float, int, int, float, float, int, int, GLuint);
extern int * __fastcall __glle_Map2(int *);
extern int __stdcall __gllc_Map2d(int, double, double, int, int, double, double, int, int, double *);
extern int __fastcall __glle_DrawPixels(int *);
extern int __stdcall __gllc_DrawPixels(int, int, GLuint, GLuint, GLuint);
extern int __fastcall __glle_TexImage1D(GLuint *);
extern int __fastcall __glle_TexImage2D(GLuint *);
extern int __stdcall __gllc_TexImage1D(int, int, int, int, int, GLuint, GLuint, int);
extern int __stdcall __gllc_TexImage2D(int, int, int, int, int, int, GLuint, GLuint, int);
extern int __fastcall __glle_TexSubImage1D(GLuint *);
extern int __stdcall __gllc_TexSubImage1D(GLuint, GLuint, GLuint, int, GLuint, GLuint, GLuint);
extern int __fastcall __glle_TexSubImage2D(GLuint *);
extern int __stdcall __gllc_TexSubImage2D(GLuint, GLuint, GLuint, GLuint, int, int, GLuint, GLuint, GLuint);
extern int * __fastcall __glle_Disable(int *);
extern int __stdcall __gllc_Disable(GLuint);
extern int * __fastcall __glle_Enable(int *);
extern int __stdcall __gllc_Enable(GLuint);
extern GLuint __fastcall __glle_DrawArraysEXT(GLuint);
extern GLuint __fastcall __glle_DrawArrays(GLuint);
extern GLuint __fastcall __glle_DrawElements(GLuint);
extern GLuint __fastcall __glle_DrawRangeElements(GLuint);
extern GLuint __fastcall __glle_ArrayElementEXT(GLuint);
extern int __fastcall __glle_ColorTable(int *);
extern int __stdcall __gllc_ColorTable(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __fastcall __glle_ColorSubTable(int *);
extern int __stdcall __gllc_ColorSubTable(GLuint, GLuint, GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glim_NewList(int, GLuint);
extern int __fastcall __glim_EndList(void);
extern int __stdcall __gllc_CallList(int);
extern GLuint * __fastcall __glle_CallList(GLuint *);
extern int __stdcall __glim_CallList(int);
extern int __stdcall __gllc_CallLists(int, GLuint, GLuint *);
extern int __fastcall __glle_CallLists(int *);
extern int __stdcall __glim_CallLists(int, GLuint, GLuint);
extern int __stdcall __glim_FogCoordfEXT(float);
extern int __stdcall __glim_FogCoorddEXT(double);
extern int __stdcall __glim_FogCoordfvEXT(float *);
extern int __stdcall __glim_FogCoorddvEXT(double *);
extern int __stdcall __glim_AlphaFunc(GLuint, float);
extern int __stdcall __glim_BlendFunc(GLuint, GLuint);
extern int __stdcall __glim_ClearAccum(float, float, float, float);
extern int __stdcall __glim_ClearColor(float, float, float, float);
extern int __stdcall __glim_ClearDepth(double);
extern int __stdcall __glim_ClearIndex(float);
extern int __stdcall __glim_ClearStencil(GLushort);
extern int __stdcall __glim_ColorMask(GLubyte, GLubyte, GLubyte, GLubyte);
extern int __stdcall __glim_ColorMaterial(GLuint, GLuint);
extern int __stdcall __glim_CullFace(GLuint);
extern int __stdcall __glim_DepthFunc(GLuint);
extern int __stdcall __glim_DepthMask(GLubyte);
extern int __stdcall __glim_DrawBuffer(GLuint);
extern int __stdcall __glim_Fogfv(GLuint, float *);
extern int __stdcall __glim_Fogf(GLuint, int);
extern int __stdcall __glim_Fogiv(GLuint, GLuint *);
extern int __stdcall __glim_Fogi(GLuint, int);
extern int __stdcall __glim_FrontFace(GLuint);
extern int __stdcall __glim_Hint(GLuint, GLuint);
extern int __stdcall __glim_IndexFuncSGI(GLuint, GLuint);
extern int __stdcall __glim_IndexMask(GLuint);
extern int __stdcall __glim_IndexMaterialSGI(GLuint, int);
extern int __stdcall __glim_Lightfv(int, int, float *);
extern int __stdcall __glim_Lightf(GLuint, GLuint, int);
extern int __stdcall __glim_Lightiv(int, int, int *);
extern int __stdcall __glim_Lighti(GLuint, GLuint, int);
extern int __stdcall __glim_LightModelfv(int, float *);
extern int __stdcall __glim_LightModelf(GLuint, int);
extern int __stdcall __glim_LightModeliv(int, int *);
extern int __stdcall __glim_LightModeli(GLuint, int);
extern int __stdcall __glim_LineStipple(int, GLushort);
extern int __stdcall __glim_LineWidth(float);
extern int __stdcall __glim_LogicOp(GLuint);
extern int __stdcall __glim_Materialfv(int, GLuint, GLuint *);
extern int __stdcall __glim_Materialf(GLuint, int, int);
extern int __stdcall __glim_Materialiv(int, GLuint, int *);
extern int __stdcall __glim_Materiali(GLuint, GLuint, int);
extern int __stdcall __glim_PointSize(float);
extern int __stdcall __glim_PolygonMode(int, int);
extern int __stdcall __glim_PolygonStipple(GLuint);
extern int __stdcall __glim_ShadeModel(GLuint);
extern int __stdcall __glim_StencilMask(GLushort);
extern int __stdcall __glim_StencilFunc(GLuint, int, GLushort);
extern int __stdcall __glim_StencilOp(GLuint, GLuint, GLuint);
extern int __stdcall __glim_PushAttrib(GLuint);
extern int __fastcall __glim_PopAttrib(void);
extern int __stdcall __glim_Enable(GLuint);
extern int __stdcall __glim_Disable(GLuint);
extern GLboolean __stdcall __glim_IsEnabled(GLuint);
extern int __stdcall __glim_PolygonOffset(float, float);
extern int __stdcall __glim_PushClientAttrib(GLuint);
extern int __fastcall __glim_PopClientAttrib(void);
extern int __stdcall __glim_EnableClientState(GLuint);
extern int __stdcall __glim_DisableClientState(GLuint);
extern int __stdcall __glim_CullParameterfvEXT(int, GLuint *);
extern int __stdcall glCullParameterdvEXT(GLuint, double *);
extern int __stdcall __glim_GetTexEnvfv(int, GLuint, float *);
extern int __stdcall __glim_GetTexEnviv(int, GLuint, GLuint *);
extern int __stdcall __glim_GetTexGenfv(int, int, float *);
extern int __stdcall __glim_GetTexGendv(int, int, double *);
extern int __stdcall __glim_GetTexGeniv(int, int, GLuint *);
extern int __stdcall __glim_GetTexParameterfv(GLuint, GLuint, float *);
extern int __stdcall __glim_GetTexParameteriv(GLuint, GLuint, int *);
extern int __stdcall __glim_GetTexLevelParameterfv(GLuint, int, GLuint, float *);
extern int __stdcall __glim_GetTexLevelParameteriv(GLuint, int, GLuint, int *);
extern int __stdcall __glim_GetClipPlane(int, double *);
extern int __stdcall __glim_GetTexImage(int, int, GLuint, GLuint, GLuint);
extern int __stdcall __glim_GetPolygonStipple(GLuint);
extern int __stdcall __glim_GetLightfv(int, int, GLuint *);
extern int __stdcall __glim_GetLightiv(int, int, GLuint);
extern int __stdcall __glim_GetMaterialfv(int, GLuint, GLuint *);
extern int __stdcall __glim_GetMaterialiv(int, GLuint, GLuint *);
extern int __stdcall __glim_GetMapfv(int, int, float *);
extern int __stdcall __glim_GetMapdv(int, int, double *);
extern int __stdcall __glim_GetMapiv(int, int, GLuint *);
extern int __stdcall __glim_GetPixelMapfv(int, float *);
extern int __stdcall __glim_GetPixelMapuiv(int, GLuint *);
extern int __stdcall __glim_GetPixelMapusv(int, GLushort *);
extern int __stdcall __glim_GetDoublev(GLuint, GLuint);
extern int __stdcall __glim_GetFloatv(GLuint, GLuint);
extern int __stdcall __glim_GetIntegerv(GLuint, GLuint);
extern int __stdcall __glim_GetBooleanv(GLuint, GLuint);
extern GLuint __fastcall __glim_GetError(void);
extern GLuint __stdcall __glim_GetString(int);
extern int __stdcall __glim_GetPointerv(GLuint, GLuint *);
extern int __stdcall __glim_GetPointervEXT(GLuint, GLuint);
extern GLboolean __stdcall __glim_IsList(GLuint);
extern GLuint __stdcall __glim_GenLists(int);
extern int __stdcall __glim_ListBase(GLuint);
extern int __stdcall __glim_DeleteLists(GLuint, int);
extern int __stdcall __glim_PixelStoref(int, float);
extern int __stdcall __glim_PixelStorei(int, int);
extern int __stdcall __glim_PixelZoom(float, float);
extern int __stdcall __glim_PixelTransferf(int, float);
extern int __stdcall __glim_PixelTransferi(GLuint, int);
extern int __stdcall __glim_PixelMapfv(int, GLuint, int);
extern int __stdcall __glim_PixelMapuiv(int, GLuint, int);
extern int __stdcall __glim_PixelMapusv(int, GLuint, int);
extern int __stdcall __glim_ReadBuffer(GLuint);
extern int __stdcall __glim_DrawPixels(int, int, int, int, int);
extern int __stdcall __glim_ReadPixels(int, int, int, int, int, int, int);
extern int __stdcall __glim_CopyPixels(int, int, int, int, int);
extern int __stdcall __glim_Flush(void);
extern int __stdcall __glim_SelectBuffer(int, GLuint);
extern int __fastcall __glim_InitNames(void);
extern int __stdcall __glim_LoadName(GLuint);
extern int __fastcall __glim_PopName(void);
extern int __stdcall __glim_PushName(GLuint);
extern int __stdcall __glim_FeedbackBuffer(int, GLuint, GLuint);
extern int __stdcall __glim_PassThrough(GLuint);
extern int __stdcall __glim_TexCoord1d(double);
extern int __stdcall __glim_TexCoord1f(GLuint);
extern int __stdcall __glim_TexCoord1i(int);
extern int __stdcall __glim_TexCoord1s(short);
extern int __stdcall __glim_TexCoord1dv(double *);
extern int __stdcall __glim_TexCoord1fv(GLuint *);
extern int __stdcall __glim_TexCoord1iv(int *);
extern int __stdcall __glim_TexCoord1sv(short *);
extern int __stdcall __glim_TexCoord2d(double, double);
extern int __stdcall __glim_TexCoord2f(GLuint, GLuint);
extern int __stdcall __glim_TexCoord2i(int, int);
extern int __stdcall __glim_TexCoord2s(short, short);
extern int __stdcall __glim_TexCoord2dv(double *);
extern int __stdcall __glim_TexCoord2fv(GLuint *);
extern int __stdcall __glim_TexCoord2iv(int *);
extern int __stdcall __glim_TexCoord2sv(short *);
extern int __stdcall __glim_TexCoord3d(double, double, double);
extern int __stdcall __glim_TexCoord3f(GLuint, GLuint, GLuint);
extern int __stdcall __glim_TexCoord3i(int, int, int);
extern int __stdcall __glim_TexCoord3s(short, short, short);
extern int __stdcall __glim_TexCoord3dv(double *);
extern int __stdcall __glim_TexCoord3fv(GLuint *);
extern int __stdcall __glim_TexCoord3iv(int *);
extern int __stdcall __glim_TexCoord3sv(short *);
extern int __stdcall __glim_TexCoord4d(double, double, double, double);
extern int __stdcall __glim_TexCoord4f(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glim_TexCoord4i(int, int, int, int);
extern int __stdcall __glim_TexCoord4s(short, short, short, short);
extern int __stdcall __glim_TexCoord4dv(double *);
extern int __stdcall __glim_TexCoord4fv(GLuint *);
extern int __stdcall __glim_TexCoord4iv(int *);
extern int __stdcall __glim_TexCoord4sv(short *);
extern int __stdcall __glim_RasterPos2dv(double *);
extern int __stdcall __glim_RasterPos2fv(int);
extern int __stdcall __glim_RasterPos2d(double, double);
extern int __stdcall __glim_RasterPos2f(GLuint, GLuint);
extern int __stdcall __glim_RasterPos2i(int, int);
extern int __stdcall __glim_RasterPos2iv(int *);
extern int __stdcall __glim_RasterPos2s(short, short);
extern int __stdcall __glim_RasterPos2sv(short *);
extern int __stdcall __glim_RasterPos3dv(double *);
extern int __stdcall __glim_RasterPos3fv(int);
extern int __stdcall __glim_RasterPos3d(double, double, double);
extern int __stdcall __glim_RasterPos3f(GLuint, GLuint, GLuint);
extern int __stdcall __glim_RasterPos3i(int, int, int);
extern int __stdcall __glim_RasterPos3iv(int *);
extern int __stdcall __glim_RasterPos3s(short, short, short);
extern int __stdcall __glim_RasterPos3sv(short *);
extern int __stdcall __glim_RasterPos4dv(double *);
extern int __stdcall __glim_RasterPos4fv(int);
extern int __stdcall __glim_RasterPos4d(double, double, double, double);
extern int __stdcall __glim_RasterPos4f(GLuint, GLuint, GLuint, GLuint);
extern int __stdcall __glim_RasterPos4i(int, int, int, int);
extern int __stdcall __glim_RasterPos4iv(int *);
extern int __stdcall __glim_RasterPos4s(short, short, short, short);
extern int __stdcall __glim_RasterPos4sv(short *);
extern int __stdcall __glim_Normal3d(double, double, double);
extern int __stdcall __glim_Normal3dv(double *);
extern int __stdcall __glim_Normal3f(GLuint, GLuint, GLuint);
extern int __stdcall __glim_Normal3fv(GLuint *);
extern int __stdcall __glim_Normal3b(char, char, char);
extern int __stdcall __glim_Normal3bv(char *);
extern int __stdcall __glim_Normal3s(short, short, short);
extern int __stdcall __glim_Normal3sv(short *);
extern int __stdcall __glim_Normal3i(int, int, int);
extern int __stdcall __glim_Normal3iv(int *);
extern int __stdcall __glim_Bitmap(int, int, int, int, int, int, int);
extern int __fastcall finishReturnAddrThunk(int);
extern int __stdcall __glim_Clear(GLuint);
extern int * __fastcall __glle_ListBase(int *);
extern int __fastcall __glle_Color3bv(int);
extern int __fastcall __glle_Color3dv(int);
extern int __fastcall __glle_Color3fv(int);
extern int __fastcall __glle_Color3iv(int);
extern int __fastcall __glle_Color3sv(int);
extern int __fastcall __glle_Color3ubv(int);
extern int __fastcall __glle_Color3uiv(int);
extern int __fastcall __glle_Color3usv(int);
extern int __fastcall __glle_Color4bv(int);
extern int __fastcall __glle_Color4dv(int);
extern int __fastcall __glle_Color4fv(int);
extern int __fastcall __glle_Color4iv(int);
extern int __fastcall __glle_Color4sv(int);
extern int __fastcall __glle_Color4ubv(int);
extern int __fastcall __glle_Color4uiv(int);
extern int __fastcall __glle_Color4usv(int);
extern int __fastcall __glle_EdgeFlagv(int);
extern int __fastcall __glle_End(int);
extern int __fastcall __glle_Indexdv(int);
extern int __fastcall __glle_Indexfv(int);
extern int __fastcall __glle_Indexiv(int);
extern int __fastcall __glle_Indexsv(int);
extern int __fastcall __glle_Normal3bv(int);
extern int __fastcall __glle_Normal3dv(int);
extern int __fastcall __glle_Normal3fv(int);
extern int __fastcall __glle_Normal3iv(int);
extern int __fastcall __glle_Normal3sv(int);
extern int __fastcall __glle_RasterPos2dv(int);
extern int __fastcall __glle_RasterPos2fv(int);
extern int __fastcall __glle_RasterPos2iv(int);
extern int __fastcall __glle_RasterPos2sv(int);
extern int __fastcall __glle_RasterPos3dv(int);
extern int __fastcall __glle_RasterPos3fv(int);
extern int __fastcall __glle_RasterPos3iv(int);
extern int __fastcall __glle_RasterPos3sv(int);
extern int __fastcall __glle_RasterPos4dv(int);
extern int __fastcall __glle_RasterPos4fv(int);
extern int __fastcall __glle_RasterPos4iv(int);
extern int __fastcall __glle_RasterPos4sv(int);
extern int __fastcall __glle_Rectdv(int);
extern int __fastcall __glle_Rectfv(int);
extern int __fastcall __glle_Rectiv(int);
extern int __fastcall __glle_Rectsv(int);
extern int __fastcall __glle_TexCoord1dv(int);
extern int __fastcall __glle_TexCoord1fv(int);
extern int __fastcall __glle_TexCoord1iv(int);
extern int __fastcall __glle_TexCoord1sv(int);
extern int __fastcall __glle_TexCoord2dv(int);
extern int __fastcall __glle_TexCoord2fv(int);
extern int __fastcall __glle_TexCoord2iv(int);
extern int __fastcall __glle_TexCoord2sv(int);
extern int __fastcall __glle_TexCoord3dv(int);
extern int __fastcall __glle_TexCoord3fv(int);
extern int __fastcall __glle_TexCoord3iv(int);
extern int __fastcall __glle_TexCoord3sv(int);
extern int __fastcall __glle_TexCoord4dv(int);
extern int __fastcall __glle_TexCoord4fv(int);
extern int __fastcall __glle_TexCoord4iv(int);
extern int __fastcall __glle_TexCoord4sv(int);
extern int __fastcall __glle_Vertex2dv(int);
extern int __fastcall __glle_Vertex2fv(int);
extern int __fastcall __glle_Vertex2iv(int);
extern int __fastcall __glle_Vertex2sv(int);
extern int __fastcall __glle_Vertex3dv(int);
extern int __fastcall __glle_Vertex3fv(int);
extern int __fastcall __glle_Vertex3iv(int);
extern int __fastcall __glle_Vertex3sv(int);
extern int __fastcall __glle_Vertex4dv(int);
extern int __fastcall __glle_Vertex4fv(int);
extern int __fastcall __glle_Vertex4iv(int);
extern int __fastcall __glle_Vertex4sv(int);
extern int __fastcall __glle_ClipPlane(int);
extern int * __fastcall __glle_ColorMaterial(int *);
extern int * __fastcall __glle_CullFace(int *);
extern int * __fastcall __glle_Fogf(int *);
extern int * __fastcall __glle_Fogfv(int *);
extern int * __fastcall __glle_Fogi(int *);
extern int * __fastcall __glle_Fogiv(int *);
extern int * __fastcall __glle_FrontFace(int *);
extern int * __fastcall __glle_Hint(int *);
extern int * __fastcall __glle_Lightfv(int *);
extern int * __fastcall __glle_Lightiv(int *);
extern int * __fastcall __glle_LightModelfv(int *);
extern int * __fastcall __glle_LightModeliv(int *);
extern int * __fastcall __glle_LineStipple(int *);
extern int * __fastcall __glle_LineWidth(int *);
extern int * __fastcall __glle_Materialfv(int *);
extern int * __fastcall __glle_Materialiv(int *);
extern int * __fastcall __glle_PointSize(int *);
extern int * __fastcall __glle_PolygonMode(int *);
extern int * __fastcall __glle_Scissor(int *);
extern int * __fastcall __glle_ShadeModel(int *);
extern int * __fastcall __glle_TexParameterfv(int *);
extern int * __fastcall __glle_TexParameteriv(int *);
extern int * __fastcall __glle_TexEnvfv(int *);
extern int * __fastcall __glle_TexEnviv(int *);
extern int * __fastcall __glle_TexGendv(int *);
extern int * __fastcall __glle_TexGenfv(int *);
extern int * __fastcall __glle_TexGeniv(int *);
extern int __fastcall __glle_InitNames(int);
extern int * __fastcall __glle_LoadName(int *);
extern int * __fastcall __glle_PassThrough(int *);
extern int __fastcall __glle_PopName(int);
extern int * __fastcall __glle_PushName(int *);
extern int * __fastcall __glle_DrawBuffer(int *);
extern int * __fastcall __glle_Clear(int *);
extern int * __fastcall __glle_ClearAccum(int *);
extern int * __fastcall __glle_ClearIndex(int *);
extern int * __fastcall __glle_ClearColor(int *);
extern int * __fastcall __glle_ClearStencil(int *);
extern int * __fastcall __glle_ClearDepth(int *);
extern int * __fastcall __glle_StencilMask(int *);
extern GLubyte * __fastcall __glle_ColorMask(GLubyte *);
extern GLubyte * __fastcall __glle_DepthMask(GLubyte *);
extern int * __fastcall __glle_IndexMask(int *);
extern int * __fastcall __glle_Accum(int *);
extern int __fastcall __glle_PopAttrib(int);
extern int * __fastcall __glle_PushAttrib(int *);
extern int * __fastcall __glle_MapGrid1d(int *);
extern int * __fastcall __glle_MapGrid1f(int *);
extern int * __fastcall __glle_MapGrid2d(int *);
extern int * __fastcall __glle_MapGrid2f(int *);
extern int __fastcall __glle_EvalCoord1dv(int);
extern int __fastcall __glle_EvalCoord1fv(int);
extern int __fastcall __glle_EvalCoord2dv(int);
extern int __fastcall __glle_EvalCoord2fv(int);
extern int * __fastcall __glle_EvalMesh1(int *);
extern int * __fastcall __glle_EvalPoint1(int *);
extern int * __fastcall __glle_EvalMesh2(int *);
extern int * __fastcall __glle_EvalPoint2(int *);
extern int * __fastcall __glle_AlphaFunc(int *);
extern int * __fastcall __glle_BlendFunc(int *);
extern int * __fastcall __glle_LogicOp(int *);
extern int * __fastcall __glle_StencilFunc(int *);
extern int * __fastcall __glle_StencilOp(int *);
extern int * __fastcall __glle_DepthFunc(int *);
extern int * __fastcall __glle_PixelZoom(int *);
extern int * __fastcall __glle_PixelTransferf(int *);
extern int * __fastcall __glle_PixelTransferi(int *);
extern int * __fastcall __glle_PixelMapfv(int *);
extern int * __fastcall __glle_PixelMapuiv(int *);
extern int __fastcall __glle_PixelMapusv(int *);
extern int * __fastcall __glle_ReadBuffer(int *);
extern int * __fastcall __glle_CopyPixels(int *);
extern int * __fastcall __glle_DepthRange(int *);
extern int * __fastcall __glle_Frustum(int *);
extern int __fastcall __glle_LoadIdentity(int);
extern int __fastcall __glle_LoadMatrixf(int);
extern int __fastcall __glle_LoadMatrixd(int);
extern int * __fastcall __glle_MatrixMode(int *);
extern int __fastcall __glle_MultMatrixf(int);
extern int __fastcall __glle_MultMatrixd(int);
extern int * __fastcall __glle_Ortho(int *);
extern int __fastcall __glle_PopMatrix(int);
extern int __fastcall __glle_PushMatrix(int);
extern int * __fastcall __glle_Rotated(int *);
extern int * __fastcall __glle_Rotatef(int *);
extern int * __fastcall __glle_Scaled(int *);
extern int * __fastcall __glle_Scalef(int *);
extern int * __fastcall __glle_Translated(int *);
extern int * __fastcall __glle_Translatef(int *);
extern int * __fastcall __glle_Viewport(int *);
extern int * __fastcall __glle_CopyColorTableEXT(int *);
extern int * __fastcall __glle_PolygonOffset(int *);
extern int * __fastcall __glle_CopyTexImage1D(int *);
extern int * __fastcall __glle_CopyTexImage2D(int *);
extern int * __fastcall __glle_CopyTexSubImage1D(int *);
extern int * __fastcall __glle_CopyTexSubImage2D(int *);
extern int * __fastcall __glle_BindTexture(int *);
extern int * __fastcall __glle_PrioritizeTextures(int *);
extern int __fastcall __glle_Indexubv(int);
extern int * __fastcall __glle_IndexFuncSGI(int *);
extern int * __fastcall __glle_IndexMaterialSGI(int *);
extern int * __fastcall __glle_ActiveTextureARB(int *);
extern int * __fastcall __glle_ClientActiveTextureARB(int *);
extern int * __fastcall __glle_MultiTexCoord1dARB(int *);
extern int __fastcall __glle_MultiTexCoord1dvARB(int);
extern int * __fastcall __glle_MultiTexCoord1fARB(int *);
extern int * __fastcall __glle_MultiTexCoord1fvARB(int *);
extern int * __fastcall __glle_MultiTexCoord1iARB(int *);
extern int * __fastcall __glle_MultiTexCoord1ivARB(int *);
extern int * __fastcall __glle_MultiTexCoord1sARB(int *);
extern int * __fastcall __glle_MultiTexCoord1svARB(int *);
extern int * __fastcall __glle_MultiTexCoord2dARB(int *);
extern int __fastcall __glle_MultiTexCoord2dvARB(int);
extern int * __fastcall __glle_MultiTexCoord2fARB(int *);
extern int * __fastcall __glle_MultiTexCoord2fvARB(int *);
extern int * __fastcall __glle_MultiTexCoord2iARB(int *);
extern int * __fastcall __glle_MultiTexCoord2ivARB(int *);
extern int * __fastcall __glle_MultiTexCoord2sARB(int *);
extern int * __fastcall __glle_MultiTexCoord2svARB(int *);
extern int * __fastcall __glle_MultiTexCoord3dARB(int *);
extern int __fastcall __glle_MultiTexCoord3dvARB(int);
extern int * __fastcall __glle_MultiTexCoord3fARB(int *);
extern int * __fastcall __glle_MultiTexCoord3fvARB(int *);
extern int * __fastcall __glle_MultiTexCoord3iARB(int *);
extern int * __fastcall __glle_MultiTexCoord3ivARB(int *);
extern int * __fastcall __glle_MultiTexCoord3sARB(int *);
extern int * __fastcall __glle_MultiTexCoord3svARB(int *);
extern int * __fastcall __glle_MultiTexCoord4dARB(int *);
extern int __fastcall __glle_MultiTexCoord4dvARB(int);
extern int * __fastcall __glle_MultiTexCoord4fARB(int *);
extern int * __fastcall __glle_MultiTexCoord4fvARB(int *);
extern int * __fastcall __glle_MultiTexCoord4iARB(int *);
extern int * __fastcall __glle_MultiTexCoord4ivARB(int *);
extern int * __fastcall __glle_MultiTexCoord4sARB(int *);
extern int * __fastcall __glle_MultiTexCoord4svARB(int *);
extern int * __fastcall __glle_PointParameterfEXT(int *);
extern int * __fastcall __glle_PointParameterfvEXT(int *);
extern int * __fastcall __glle_BlendEquationEXT(int *);
extern int __fastcall __glle_CompressedTexImage1DARB(int *);
extern int __fastcall __glle_CompressedTexImage2DARB(int *);
extern int __fastcall __glle_CompressedTexSubImage1DARB(int *);
extern int __fastcall __glle_CompressedTexSubImage2DARB(int *);
extern int * __fastcall __glle_TBufferMask3DFX(int *);
extern int * __fastcall __glle_FogCoordfEXT(int *);
extern int * __fastcall __glle_FogCoorddEXT(int *);
extern int __fastcall __glle_FogCoordfvEXT(int);
extern int __fastcall __glle_FogCoorddvEXT(int);
extern float * __fastcall __glle_FastMaterial(float *);
extern GLuint __stdcall __glNullDispatch2(int, int);
extern GLuint __fastcall __glNullDispatchReturn0(void);
extern GLuint __stdcall __glNullDispatch1(int);
extern GLuint __stdcall __glNullDispatch3(int, int, int);
extern GLuint __stdcall __glNullDispatch7(int, int, int, int, int, int, int);
extern GLuint __stdcall __glNullDispatch6(int, int, int, int, int, int);
extern GLuint __stdcall __glNullDispatch4(int, int, int, int);
extern GLuint __stdcall __glNullDispatch8(int, int, int, int, int, int, int, int);
extern GLuint __stdcall __glNullDispatch9(int, int, int, int, int, int, int, int, int);
extern GLuint __stdcall __glNullDispatch14(int, int, int, int, int, int, int, int, int, int, int, int, int, int);
extern GLuint __stdcall __glNullDispatch10(int, int, int, int, int, int, int, int, int, int);
extern GLuint __stdcall __glNullDispatch5(int, int, int, int, int);
extern GLuint __stdcall __glNullDispatch12(int, int, int, int, int, int, int, int, int, int, int, int);

/* 690ef0c0 -- __glSSTImmedState: the hardware path, immediate mode */
__GLdispatchTable __glSSTImmedState = { {
    (__GLdispatchFn)0x150,                              /* the entry count */
    (__GLdispatchFn)__glim_NewList,                     /* glNewList */
    (__GLdispatchFn)__glim_EndList,                     /* glEndList */
    (__GLdispatchFn)__glim_CallList,                    /* glCallList */
    (__GLdispatchFn)__glim_CallLists,                   /* glCallLists */
    (__GLdispatchFn)__glim_DeleteLists,                 /* glDeleteLists */
    (__GLdispatchFn)__glim_GenLists,                    /* glGenLists */
    (__GLdispatchFn)__glim_ListBase,                       /* glListBase */
    (__GLdispatchFn)__glsstim_Begin,                    /* glBegin */
    (__GLdispatchFn)__glim_Bitmap,                      /* glBitmap */
    (__GLdispatchFn)__glsstim_Color3b,                  /* glColor3b */
    (__GLdispatchFn)__glsstim_Color3bv,                 /* glColor3bv */
    (__GLdispatchFn)__glsstim_Color3d,                  /* glColor3d */
    (__GLdispatchFn)__glsstim_Color3dv,                 /* glColor3dv */
    (__GLdispatchFn)__glsstim_Color3f,                  /* glColor3f */
    (__GLdispatchFn)__glsstim_Color3fv,                 /* glColor3fv */
    (__GLdispatchFn)__glsstim_Color3i,                  /* glColor3i */
    (__GLdispatchFn)__glsstim_Color3iv,                 /* glColor3iv */
    (__GLdispatchFn)__glsstim_Color3s,                  /* glColor3s */
    (__GLdispatchFn)__glsstim_Color3sv,                 /* glColor3sv */
    (__GLdispatchFn)__glsstim_Color3ub,                 /* glColor3ub */
    (__GLdispatchFn)__glsstim_Color3ubv,                /* glColor3ubv */
    (__GLdispatchFn)__glsstim_Color3ui,                 /* glColor3ui */
    (__GLdispatchFn)__glsstim_Color3uiv,                /* glColor3uiv */
    (__GLdispatchFn)__glsstim_Color3us,                 /* glColor3us */
    (__GLdispatchFn)__glsstim_Color3usv,                /* glColor3usv */
    (__GLdispatchFn)__glsstim_Color4b,                  /* glColor4b */
    (__GLdispatchFn)__glsstim_Color4bv,                 /* glColor4bv */
    (__GLdispatchFn)__glsstim_Color4d,                  /* glColor4d */
    (__GLdispatchFn)__glsstim_Color4dv,                 /* glColor4dv */
    (__GLdispatchFn)__glsstim_Color4f,                  /* glColor4f */
    (__GLdispatchFn)__glsstim_Color4fv,                 /* glColor4fv */
    (__GLdispatchFn)__glsstim_Color4i,                  /* glColor4i */
    (__GLdispatchFn)__glsstim_Color4iv,                 /* glColor4iv */
    (__GLdispatchFn)__glsstim_Color4s,                  /* glColor4s */
    (__GLdispatchFn)__glsstim_Color4sv,                 /* glColor4sv */
    (__GLdispatchFn)__glsstim_Color4ub,                 /* glColor4ub */
    (__GLdispatchFn)__glsstim_Color4ubv,                /* glColor4ubv */
    (__GLdispatchFn)__glsstim_Color4ui,                 /* glColor4ui */
    (__GLdispatchFn)__glsstim_Color4uiv,                /* glColor4uiv */
    (__GLdispatchFn)__glsstim_Color4us,                 /* glColor4us */
    (__GLdispatchFn)__glsstim_Color4usv,                /* glColor4usv */
    (__GLdispatchFn)__glim_EdgeFlag,                    /* glEdgeFlag */
    (__GLdispatchFn)__glim_EdgeFlagv,                   /* glEdgeFlagv */
    (__GLdispatchFn)__glsstim_End,                      /* glEnd */
    (__GLdispatchFn)__glim_Indexd,                      /* glIndexd */
    (__GLdispatchFn)__glim_Indexdv,                     /* glIndexdv */
    (__GLdispatchFn)__glim_Indexf,                      /* glIndexf */
    (__GLdispatchFn)__glim_Indexfv,                       /* glIndexfv */
    (__GLdispatchFn)__glim_Indexi,                      /* glIndexi */
    (__GLdispatchFn)__glim_Indexiv,                       /* glIndexiv */
    (__GLdispatchFn)__glim_Indexs,                      /* glIndexs */
    (__GLdispatchFn)__glim_Indexsv,                       /* glIndexsv */
    (__GLdispatchFn)__glim_Normal3b,                       /* glNormal3b */
    (__GLdispatchFn)__glim_Normal3bv,                       /* glNormal3bv */
    (__GLdispatchFn)__glim_Normal3d,                       /* glNormal3d */
    (__GLdispatchFn)__glim_Normal3dv,                       /* glNormal3dv */
    (__GLdispatchFn)__glim_Normal3f,                       /* glNormal3f */
    (__GLdispatchFn)__glim_Normal3fv,                       /* glNormal3fv */
    (__GLdispatchFn)__glim_Normal3i,                       /* glNormal3i */
    (__GLdispatchFn)__glim_Normal3iv,                       /* glNormal3iv */
    (__GLdispatchFn)__glim_Normal3s,                       /* glNormal3s */
    (__GLdispatchFn)__glim_Normal3sv,                       /* glNormal3sv */
    (__GLdispatchFn)__glim_RasterPos2d,                       /* glRasterPos2d */
    (__GLdispatchFn)__glim_RasterPos2dv,                /* glRasterPos2dv */
    (__GLdispatchFn)__glim_RasterPos2f,                       /* glRasterPos2f */
    (__GLdispatchFn)__glim_RasterPos2fv,                       /* glRasterPos2fv */
    (__GLdispatchFn)__glim_RasterPos2i,                       /* glRasterPos2i */
    (__GLdispatchFn)__glim_RasterPos2iv,                       /* glRasterPos2iv */
    (__GLdispatchFn)__glim_RasterPos2s,                       /* glRasterPos2s */
    (__GLdispatchFn)__glim_RasterPos2sv,                       /* glRasterPos2sv */
    (__GLdispatchFn)__glim_RasterPos3d,                       /* glRasterPos3d */
    (__GLdispatchFn)__glim_RasterPos3dv,                /* glRasterPos3dv */
    (__GLdispatchFn)__glim_RasterPos3f,                       /* glRasterPos3f */
    (__GLdispatchFn)__glim_RasterPos3fv,                       /* glRasterPos3fv */
    (__GLdispatchFn)__glim_RasterPos3i,                       /* glRasterPos3i */
    (__GLdispatchFn)__glim_RasterPos3iv,                       /* glRasterPos3iv */
    (__GLdispatchFn)__glim_RasterPos3s,                       /* glRasterPos3s */
    (__GLdispatchFn)__glim_RasterPos3sv,                       /* glRasterPos3sv */
    (__GLdispatchFn)__glim_RasterPos4d,                       /* glRasterPos4d */
    (__GLdispatchFn)__glim_RasterPos4dv,                /* glRasterPos4dv */
    (__GLdispatchFn)__glim_RasterPos4f,                       /* glRasterPos4f */
    (__GLdispatchFn)__glim_RasterPos4fv,                       /* glRasterPos4fv */
    (__GLdispatchFn)__glim_RasterPos4i,                       /* glRasterPos4i */
    (__GLdispatchFn)__glim_RasterPos4iv,                       /* glRasterPos4iv */
    (__GLdispatchFn)__glim_RasterPos4s,                       /* glRasterPos4s */
    (__GLdispatchFn)__glim_RasterPos4sv,                       /* glRasterPos4sv */
    (__GLdispatchFn)__glim_Rectd,                       /* glRectd */
    (__GLdispatchFn)__glim_Rectdv,                      /* glRectdv */
    (__GLdispatchFn)__glim_Rectf,                       /* glRectf */
    (__GLdispatchFn)__glim_Rectfv,                      /* glRectfv */
    (__GLdispatchFn)__glim_Recti,                       /* glRecti */
    (__GLdispatchFn)__glim_Rectiv,                      /* glRectiv */
    (__GLdispatchFn)__glim_Rects,                       /* glRects */
    (__GLdispatchFn)__glim_Rectsv,                      /* glRectsv */
    (__GLdispatchFn)__glim_TexCoord1d,                  /* glTexCoord1d */
    (__GLdispatchFn)__glim_TexCoord1dv,                 /* glTexCoord1dv */
    (__GLdispatchFn)__glim_TexCoord1f,                  /* glTexCoord1f */
    (__GLdispatchFn)__glim_TexCoord1fv,                 /* glTexCoord1fv */
    (__GLdispatchFn)__glim_TexCoord1i,                  /* glTexCoord1i */
    (__GLdispatchFn)__glim_TexCoord1iv,                 /* glTexCoord1iv */
    (__GLdispatchFn)__glim_TexCoord1s,                  /* glTexCoord1s */
    (__GLdispatchFn)__glim_TexCoord1sv,                 /* glTexCoord1sv */
    (__GLdispatchFn)__glim_TexCoord2d,                  /* glTexCoord2d */
    (__GLdispatchFn)__glim_TexCoord2dv,                 /* glTexCoord2dv */
    (__GLdispatchFn)__glim_TexCoord2f,                  /* glTexCoord2f */
    (__GLdispatchFn)__glim_TexCoord2fv,                 /* glTexCoord2fv */
    (__GLdispatchFn)__glim_TexCoord2i,                  /* glTexCoord2i */
    (__GLdispatchFn)__glim_TexCoord2iv,                 /* glTexCoord2iv */
    (__GLdispatchFn)__glim_TexCoord2s,                  /* glTexCoord2s */
    (__GLdispatchFn)__glim_TexCoord2sv,                 /* glTexCoord2sv */
    (__GLdispatchFn)__glim_TexCoord3d,                  /* glTexCoord3d */
    (__GLdispatchFn)__glim_TexCoord3dv,                 /* glTexCoord3dv */
    (__GLdispatchFn)__glim_TexCoord3f,                  /* glTexCoord3f */
    (__GLdispatchFn)__glim_TexCoord3fv,                 /* glTexCoord3fv */
    (__GLdispatchFn)__glim_TexCoord3i,                  /* glTexCoord3i */
    (__GLdispatchFn)__glim_TexCoord3iv,                 /* glTexCoord3iv */
    (__GLdispatchFn)__glim_TexCoord3s,                  /* glTexCoord3s */
    (__GLdispatchFn)__glim_TexCoord3sv,                 /* glTexCoord3sv */
    (__GLdispatchFn)__glim_TexCoord4d,                  /* glTexCoord4d */
    (__GLdispatchFn)__glim_TexCoord4dv,                 /* glTexCoord4dv */
    (__GLdispatchFn)__glim_TexCoord4f,                  /* glTexCoord4f */
    (__GLdispatchFn)__glim_TexCoord4fv,                 /* glTexCoord4fv */
    (__GLdispatchFn)__glim_TexCoord4i,                  /* glTexCoord4i */
    (__GLdispatchFn)__glim_TexCoord4iv,                 /* glTexCoord4iv */
    (__GLdispatchFn)__glim_TexCoord4s,                  /* glTexCoord4s */
    (__GLdispatchFn)__glim_TexCoord4sv,                 /* glTexCoord4sv */
    (__GLdispatchFn)__glim_Vertex2d,                    /* glVertex2d */
    (__GLdispatchFn)__glim_Vertex2dv,                   /* glVertex2dv */
    (__GLdispatchFn)__glim_Vertex2f,                    /* glVertex2f */
    (__GLdispatchFn)__glim_Vertex2fv,                   /* glVertex2fv */
    (__GLdispatchFn)__glim_Vertex2i,                    /* glVertex2i */
    (__GLdispatchFn)__glim_Vertex2iv,                   /* glVertex2iv */
    (__GLdispatchFn)__glim_Vertex2s,                    /* glVertex2s */
    (__GLdispatchFn)__glim_Vertex2sv,                   /* glVertex2sv */
    (__GLdispatchFn)__glim_Vertex3d,                    /* glVertex3d */
    (__GLdispatchFn)__glim_Vertex3dv,                   /* glVertex3dv */
    (__GLdispatchFn)__glim_Vertex3f,                    /* glVertex3f */
    (__GLdispatchFn)__glim_Vertex3fv,                   /* glVertex3fv */
    (__GLdispatchFn)__glim_Vertex3i,                    /* glVertex3i */
    (__GLdispatchFn)__glim_Vertex3iv,                   /* glVertex3iv */
    (__GLdispatchFn)__glim_Vertex3s,                    /* glVertex3s */
    (__GLdispatchFn)__glim_Vertex3sv,                   /* glVertex3sv */
    (__GLdispatchFn)__glim_Vertex4d,                    /* glVertex4d */
    (__GLdispatchFn)__glim_Vertex4dv,                   /* glVertex4dv */
    (__GLdispatchFn)__glim_Vertex4f,                    /* glVertex4f */
    (__GLdispatchFn)__glim_Vertex4fv,                   /* glVertex4fv */
    (__GLdispatchFn)__glim_Vertex4i,                    /* glVertex4i */
    (__GLdispatchFn)__glim_Vertex4iv,                   /* glVertex4iv */
    (__GLdispatchFn)__glim_Vertex4s,                    /* glVertex4s */
    (__GLdispatchFn)__glim_Vertex4sv,                   /* glVertex4sv */
    (__GLdispatchFn)__glim_ClipPlane,                   /* glClipPlane */
    (__GLdispatchFn)__glim_ColorMaterial,                       /* glColorMaterial */
    (__GLdispatchFn)__glsstim_CullFace,                 /* glCullFace */
    (__GLdispatchFn)__glsstim_Fogf,                     /* glFogf */
    (__GLdispatchFn)__glsstim_Fogfv,                    /* glFogfv */
    (__GLdispatchFn)__glsstim_Fogi,                     /* glFogi */
    (__GLdispatchFn)__glsstim_Fogiv,                    /* glFogiv */
    (__GLdispatchFn)__glsstim_FrontFace,                /* glFrontFace */
    (__GLdispatchFn)__glim_Hint,                        /* glHint */
    (__GLdispatchFn)__glim_Lightf,                      /* glLightf */
    (__GLdispatchFn)__glim_Lightfv,                     /* glLightfv */
    (__GLdispatchFn)__glim_Lighti,                      /* glLighti */
    (__GLdispatchFn)__glim_Lightiv,                     /* glLightiv */
    (__GLdispatchFn)__glim_LightModelf,                 /* glLightModelf */
    (__GLdispatchFn)__glim_LightModelfv,                /* glLightModelfv */
    (__GLdispatchFn)__glim_LightModeli,                 /* glLightModeli */
    (__GLdispatchFn)__glim_LightModeliv,                /* glLightModeliv */
    (__GLdispatchFn)__glim_LineStipple,                       /* glLineStipple */
    (__GLdispatchFn)__glim_LineWidth,                   /* glLineWidth */
    (__GLdispatchFn)__glim_Materialf,                   /* glMaterialf */
    (__GLdispatchFn)__glim_Materialfv,                  /* glMaterialfv */
    (__GLdispatchFn)__glim_Materiali,                   /* glMateriali */
    (__GLdispatchFn)__glim_Materialiv,                  /* glMaterialiv */
    (__GLdispatchFn)__glim_PointSize,                   /* glPointSize */
    (__GLdispatchFn)__glim_PolygonMode,                       /* glPolygonMode */
    (__GLdispatchFn)__glim_PolygonStipple,              /* glPolygonStipple */
    (__GLdispatchFn)__glim_Scissor,                       /* glScissor */
    (__GLdispatchFn)__glim_ShadeModel,                  /* glShadeModel */
    (__GLdispatchFn)__glsstim_TexParameterf,            /* glTexParameterf */
    (__GLdispatchFn)__glsstim_TexParameterfv,           /* glTexParameterfv */
    (__GLdispatchFn)__glsstim_TexParameteri,            /* glTexParameteri */
    (__GLdispatchFn)__glsstim_TexParameteriv,           /* glTexParameteriv */
    (__GLdispatchFn)__glsstim_TexImage1D,               /* glTexImage1D */
    (__GLdispatchFn)__glsstim_TexImage2D,               /* glTexImage2D */
    (__GLdispatchFn)__glsstim_TexEnvf,                  /* glTexEnvf */
    (__GLdispatchFn)__glsstim_TexEnvfv,                 /* glTexEnvfv */
    (__GLdispatchFn)__glsstim_TexEnvi,                  /* glTexEnvi */
    (__GLdispatchFn)__glsstim_TexEnviv,                 /* glTexEnviv */
    (__GLdispatchFn)__glim_TexGend,                     /* glTexGend */
    (__GLdispatchFn)__glim_TexGendv,                    /* glTexGendv */
    (__GLdispatchFn)__glim_TexGenf,                     /* glTexGenf */
    (__GLdispatchFn)__glim_TexGenfv,                    /* glTexGenfv */
    (__GLdispatchFn)__glim_TexGeni,                     /* glTexGeni */
    (__GLdispatchFn)__glim_TexGeniv,                    /* glTexGeniv */
    (__GLdispatchFn)__glim_FeedbackBuffer,                       /* glFeedbackBuffer */
    (__GLdispatchFn)__glim_SelectBuffer,                       /* glSelectBuffer */
    (__GLdispatchFn)__glim_RenderMode,                  /* glRenderMode */
    (__GLdispatchFn)__glim_InitNames,                   /* glInitNames */
    (__GLdispatchFn)__glim_LoadName,                       /* glLoadName */
    (__GLdispatchFn)__glim_PassThrough,                       /* glPassThrough */
    (__GLdispatchFn)__glim_PopName,                     /* glPopName */
    (__GLdispatchFn)__glim_PushName,                       /* glPushName */
    (__GLdispatchFn)__glsstim_DrawBuffer,               /* glDrawBuffer */
    (__GLdispatchFn)__glsstim_Clear,                    /* glClear */
    (__GLdispatchFn)__glim_ClearAccum,                       /* glClearAccum */
    (__GLdispatchFn)__glim_ClearIndex,                       /* glClearIndex */
    (__GLdispatchFn)__glim_ClearColor,                       /* glClearColor */
    (__GLdispatchFn)__glim_ClearStencil,                /* glClearStencil */
    (__GLdispatchFn)__glim_ClearDepth,                  /* glClearDepth */
    (__GLdispatchFn)__glsstim_StencilMask,              /* glStencilMask */
    (__GLdispatchFn)__glsstim_ColorMask,                /* glColorMask */
    (__GLdispatchFn)__glsstim_DepthMask,                /* glDepthMask */
    (__GLdispatchFn)__glim_IndexMask,                   /* glIndexMask */
    (__GLdispatchFn)__glim_Accum,                       /* glAccum */
    (__GLdispatchFn)__glsstim_Disable,                  /* glDisable */
    (__GLdispatchFn)__glsstim_Enable,                   /* glEnable */
    (__GLdispatchFn)swapReturnAddrThunk,                /* glFinish */
    (__GLdispatchFn)__glim_Flush,                       /* glFlush */
    (__GLdispatchFn)__glsstim_PopAttrib,                /* glPopAttrib */
    (__GLdispatchFn)__glim_PushAttrib,                  /* glPushAttrib */
    (__GLdispatchFn)__glim_Map1d,                       /* glMap1d */
    (__GLdispatchFn)__glim_Map1f,                       /* glMap1f */
    (__GLdispatchFn)__glim_Map2d,                       /* glMap2d */
    (__GLdispatchFn)__glim_Map2f,                       /* glMap2f */
    (__GLdispatchFn)__glim_MapGrid1d,                   /* glMapGrid1d */
    (__GLdispatchFn)__glim_MapGrid1f,                       /* glMapGrid1f */
    (__GLdispatchFn)__glim_MapGrid2d,                   /* glMapGrid2d */
    (__GLdispatchFn)__glim_MapGrid2f,                       /* glMapGrid2f */
    (__GLdispatchFn)__glim_EvalCoord1d,                       /* glEvalCoord1d */
    (__GLdispatchFn)__glim_EvalCoord1dv,                       /* glEvalCoord1dv */
    (__GLdispatchFn)__glim_EvalCoord1f,                       /* glEvalCoord1f */
    (__GLdispatchFn)__glim_EvalCoord1fv,                       /* glEvalCoord1fv */
    (__GLdispatchFn)__glim_EvalCoord2d,                       /* glEvalCoord2d */
    (__GLdispatchFn)__glim_EvalCoord2dv,                       /* glEvalCoord2dv */
    (__GLdispatchFn)__glim_EvalCoord2f,                       /* glEvalCoord2f */
    (__GLdispatchFn)__glim_EvalCoord2fv,                       /* glEvalCoord2fv */
    (__GLdispatchFn)__glim_EvalMesh1,                       /* glEvalMesh1 */
    (__GLdispatchFn)__glim_EvalPoint1,                  /* glEvalPoint1 */
    (__GLdispatchFn)__glim_EvalMesh2,                       /* glEvalMesh2 */
    (__GLdispatchFn)__glim_EvalPoint2,                  /* glEvalPoint2 */
    (__GLdispatchFn)__glsstim_AlphaFunc,                /* glAlphaFunc */
    (__GLdispatchFn)__glsstim_BlendFunc,                /* glBlendFunc */
    (__GLdispatchFn)__glim_LogicOp,                       /* glLogicOp */
    (__GLdispatchFn)__glsstim_StencilFunc,              /* glStencilFunc */
    (__GLdispatchFn)__glsstim_StencilOp,                /* glStencilOp */
    (__GLdispatchFn)__glsstim_DepthFunc,                /* glDepthFunc */
    (__GLdispatchFn)__glim_PixelZoom,                   /* glPixelZoom */
    (__GLdispatchFn)__glim_PixelTransferf,              /* glPixelTransferf */
    (__GLdispatchFn)__glim_PixelTransferi,              /* glPixelTransferi */
    (__GLdispatchFn)__glim_PixelStoref,                 /* glPixelStoref */
    (__GLdispatchFn)__glim_PixelStorei,                 /* glPixelStorei */
    (__GLdispatchFn)__glim_PixelMapfv,                       /* glPixelMapfv */
    (__GLdispatchFn)__glim_PixelMapuiv,                       /* glPixelMapuiv */
    (__GLdispatchFn)__glim_PixelMapusv,                       /* glPixelMapusv */
    (__GLdispatchFn)__glsstim_ReadBuffer,               /* glReadBuffer */
    (__GLdispatchFn)__glim_CopyPixels,                  /* glCopyPixels */
    (__GLdispatchFn)__glim_ReadPixels,                       /* glReadPixels */
    (__GLdispatchFn)__glim_DrawPixels,                       /* glDrawPixels */
    (__GLdispatchFn)__glim_GetBooleanv,                 /* glGetBooleanv */
    (__GLdispatchFn)__glim_GetClipPlane,                       /* glGetClipPlane */
    (__GLdispatchFn)__glim_GetDoublev,                  /* glGetDoublev */
    (__GLdispatchFn)__glim_GetError,                    /* glGetError */
    (__GLdispatchFn)__glim_GetFloatv,                   /* glGetFloatv */
    (__GLdispatchFn)__glim_GetIntegerv,                 /* glGetIntegerv */
    (__GLdispatchFn)__glim_GetLightfv,                  /* glGetLightfv */
    (__GLdispatchFn)__glim_GetLightiv,                  /* glGetLightiv */
    (__GLdispatchFn)__glim_GetMapdv,                    /* glGetMapdv */
    (__GLdispatchFn)__glim_GetMapfv,                    /* glGetMapfv */
    (__GLdispatchFn)__glim_GetMapiv,                    /* glGetMapiv */
    (__GLdispatchFn)__glim_GetMaterialfv,               /* glGetMaterialfv */
    (__GLdispatchFn)__glim_GetMaterialiv,               /* glGetMaterialiv */
    (__GLdispatchFn)__glim_GetPixelMapfv,               /* glGetPixelMapfv */
    (__GLdispatchFn)__glim_GetPixelMapuiv,              /* glGetPixelMapuiv */
    (__GLdispatchFn)__glim_GetPixelMapusv,              /* glGetPixelMapusv */
    (__GLdispatchFn)__glim_GetPolygonStipple,           /* glGetPolygonStipple */
    (__GLdispatchFn)__glim_GetString,                       /* glGetString */
    (__GLdispatchFn)__glim_GetTexEnvfv,                 /* glGetTexEnvfv */
    (__GLdispatchFn)__glim_GetTexEnviv,                 /* glGetTexEnviv */
    (__GLdispatchFn)__glim_GetTexGendv,                       /* glGetTexGendv */
    (__GLdispatchFn)__glim_GetTexGenfv,                       /* glGetTexGenfv */
    (__GLdispatchFn)__glim_GetTexGeniv,                 /* glGetTexGeniv */
    (__GLdispatchFn)__glim_GetTexImage,                 /* glGetTexImage */
    (__GLdispatchFn)__glim_GetTexParameterfv,           /* glGetTexParameterfv */
    (__GLdispatchFn)__glim_GetTexParameteriv,           /* glGetTexParameteriv */
    (__GLdispatchFn)__glsstim_GetTexLevelParameterfv,   /* glGetTexLevelParameterfv */
    (__GLdispatchFn)__glsstim_GetTexLevelParameteriv,   /* glGetTexLevelParameteriv */
    (__GLdispatchFn)__glim_IsEnabled,                   /* glIsEnabled */
    (__GLdispatchFn)__glim_IsList,                      /* glIsList */
    (__GLdispatchFn)__glim_DepthRange,                  /* glDepthRange */
    (__GLdispatchFn)__glim_Frustum,                     /* glFrustum */
    (__GLdispatchFn)__glim_LoadIdentity,                       /* glLoadIdentity */
    (__GLdispatchFn)__glim_LoadMatrixf,                       /* glLoadMatrixf */
    (__GLdispatchFn)__glim_LoadMatrixd,                       /* glLoadMatrixd */
    (__GLdispatchFn)__glim_MatrixMode,                       /* glMatrixMode */
    (__GLdispatchFn)__glim_MultMatrixf,                 /* glMultMatrixf */
    (__GLdispatchFn)__glim_MultMatrixd,                 /* glMultMatrixd */
    (__GLdispatchFn)__glim_Ortho,                       /* glOrtho */
    (__GLdispatchFn)__glim_PopMatrix,                       /* glPopMatrix */
    (__GLdispatchFn)__glim_PushMatrix,                       /* glPushMatrix */
    (__GLdispatchFn)__glim_Rotated,                     /* glRotated */
    (__GLdispatchFn)__glim_Rotatef,                     /* glRotatef */
    (__GLdispatchFn)__glim_Scaled,                      /* glScaled */
    (__GLdispatchFn)__glim_Scalef,                      /* glScalef */
    (__GLdispatchFn)__glim_Translated,                  /* glTranslated */
    (__GLdispatchFn)__glim_Translatef,                  /* glTranslatef */
    (__GLdispatchFn)__glim_Viewport,                    /* glViewport */
    (__GLdispatchFn)__glim_ArrayElement,                /* glArrayElement */
    (__GLdispatchFn)__glsstim_BindTexture,              /* glBindTexture */
    (__GLdispatchFn)__glim_ColorPointer,                /* glColorPointer */
    (__GLdispatchFn)__glim_DisableClientState,          /* glDisableClientState */
    (__GLdispatchFn)__glim_DrawArrays,                  /* glDrawArrays */
    (__GLdispatchFn)__glsstim_DrawElements,             /* glDrawElements */
    (__GLdispatchFn)__glim_EdgeFlagPointer,             /* glEdgeFlagPointer */
    (__GLdispatchFn)__glim_EnableClientState,           /* glEnableClientState */
    (__GLdispatchFn)__glim_IndexPointer,                /* glIndexPointer */
    (__GLdispatchFn)__glim_Indexub,                     /* glIndexub */
    (__GLdispatchFn)__glim_Indexubv,                       /* glIndexubv */
    (__GLdispatchFn)__glim_InterleavedArrays,           /* glInterleavedArrays */
    (__GLdispatchFn)__glim_NormalPointer,               /* glNormalPointer */
    (__GLdispatchFn)__glim_PolygonOffset,               /* glPolygonOffset */
    (__GLdispatchFn)__glim_TexCoordPointer,             /* glTexCoordPointer */
    (__GLdispatchFn)__glim_VertexPointer,               /* glVertexPointer */
    (__GLdispatchFn)__glim_AreTexturesResident,         /* glAreTexturesResident */
    (__GLdispatchFn)__glim_CopyTexImage1D,              /* glCopyTexImage1D */
    (__GLdispatchFn)__glim_CopyTexImage2D,              /* glCopyTexImage2D */
    (__GLdispatchFn)__glim_CopyTexSubImage1D,           /* glCopyTexSubImage1D */
    (__GLdispatchFn)__glim_CopyTexSubImage2D,           /* glCopyTexSubImage2D */
    (__GLdispatchFn)__glim_DeleteTextures,              /* glDeleteTextures */
    (__GLdispatchFn)__glim_GenTextures,                 /* glGenTextures */
    (__GLdispatchFn)__glim_GetPointerv,                 /* glGetPointerv */
    (__GLdispatchFn)__glim_IsTexture,                       /* glIsTexture */
    (__GLdispatchFn)__glim_PrioritizeTextures,          /* glPrioritizeTextures */
    (__GLdispatchFn)__glsstim_TexSubImage1D,            /* glTexSubImage1D */
    (__GLdispatchFn)__glsstim_TexSubImage2D,            /* glTexSubImage2D */
    (__GLdispatchFn)__glim_PopClientAttrib,             /* glPopClientAttrib */
    (__GLdispatchFn)__glim_PushClientAttrib,            /* glPushClientAttrib */
    (__GLdispatchFn)__glim_ArrayElementEXT,             /* extension slot 0x151 */
    (__GLdispatchFn)__glim_ColorPointerEXT,             /* extension slot 0x152 */
    (__GLdispatchFn)__glim_DrawArraysEXT,               /* extension slot 0x153 */
    (__GLdispatchFn)__glim_EdgeFlagPointerEXT,          /* extension slot 0x154 */
    (__GLdispatchFn)__glim_GetPointervEXT,              /* extension slot 0x155 */
    (__GLdispatchFn)__glim_IndexPointerEXT,             /* extension slot 0x156 */
    (__GLdispatchFn)__glim_NormalPointerEXT,            /* extension slot 0x157 */
    (__GLdispatchFn)__glim_TexCoordPointerEXT,          /* extension slot 0x158 */
    (__GLdispatchFn)__glim_VertexPointerEXT,            /* extension slot 0x159 */
    (__GLdispatchFn)__glsstim_ColorSubTableEXT,         /* extension slot 0x15a */
    (__GLdispatchFn)__glsstim_ColorTable,               /* extension slot 0x15b */
    (__GLdispatchFn)__glim_CopyColorTableEXT,           /* extension slot 0x15c */
    (__GLdispatchFn)__glim_GetColorTable,               /* extension slot 0x15d */
    (__GLdispatchFn)__glim_GetColorTableParameterfvEXT, /* extension slot 0x15e */
    (__GLdispatchFn)__glim_GetColorTableParameterivEXT, /* extension slot 0x15f */
    (__GLdispatchFn)__glsstim_LockArraysEXT,            /* extension slot 0x160 */
    (__GLdispatchFn)__glsstim_UnlockArraysEXT,          /* extension slot 0x161 */
    (__GLdispatchFn)glCullParameterdvEXT,               /* extension slot 0x162 */
    (__GLdispatchFn)__glim_CullParameterfvEXT,          /* extension slot 0x163 */
    (__GLdispatchFn)__glim_IndexFuncSGI,                /* extension slot 0x164 */
    (__GLdispatchFn)__glim_IndexMaterialSGI,            /* extension slot 0x165 */
    (__GLdispatchFn)__glim_AddSwapHintRectWIN,          /* extension slot 0x166 */
    (__GLdispatchFn)__glsstim_ActiveTextureARB,         /* extension slot 0x167 */
    (__GLdispatchFn)__glsstim_ClientActiveTextureARB,   /* extension slot 0x168 */
    (__GLdispatchFn)__glsstim_MultiTexCoord1dARB,       /* extension slot 0x169 */
    (__GLdispatchFn)__glsstim_MultiTexCoord1dvARB,      /* extension slot 0x16a */
    (__GLdispatchFn)__glsstim_MultiTexCoord1fARB,       /* extension slot 0x16b */
    (__GLdispatchFn)__glsstim_MultiTexCoord1fvARB,      /* extension slot 0x16c */
    (__GLdispatchFn)__glsstim_MultiTexCoord1iARB,       /* extension slot 0x16d */
    (__GLdispatchFn)__glsstim_MultiTexCoord1ivARB,      /* extension slot 0x16e */
    (__GLdispatchFn)__glsstim_MultiTexCoord1sARB,       /* extension slot 0x16f */
    (__GLdispatchFn)__glsstim_MultiTexCoord1svARB,      /* extension slot 0x170 */
    (__GLdispatchFn)__glsstim_MultiTexCoord2dARB,       /* extension slot 0x171 */
    (__GLdispatchFn)__glsstim_MultiTexCoord2dvARB,      /* extension slot 0x172 */
    (__GLdispatchFn)__glsstim_MultiTexCoord2fARB,       /* extension slot 0x173 */
    (__GLdispatchFn)__glsstim_MultiTexCoord2fvARB,      /* extension slot 0x174 */
    (__GLdispatchFn)__glsstim_MultiTexCoord2iARB,       /* extension slot 0x175 */
    (__GLdispatchFn)__glsstim_MultiTexCoord2ivARB,      /* extension slot 0x176 */
    (__GLdispatchFn)__glsstim_MultiTexCoord2sARB,       /* extension slot 0x177 */
    (__GLdispatchFn)__glsstim_MultiTexCoord2svARB,      /* extension slot 0x178 */
    (__GLdispatchFn)__glsstim_MultiTexCoord3dARB,       /* extension slot 0x179 */
    (__GLdispatchFn)__glsstim_MultiTexCoord3dvARB,      /* extension slot 0x17a */
    (__GLdispatchFn)__glsstim_MultiTexCoord3fARB,       /* extension slot 0x17b */
    (__GLdispatchFn)__glsstim_MultiTexCoord3fvARB,      /* extension slot 0x17c */
    (__GLdispatchFn)__glsstim_MultiTexCoord3iARB,       /* extension slot 0x17d */
    (__GLdispatchFn)__glsstim_MultiTexCoord3ivARB,      /* extension slot 0x17e */
    (__GLdispatchFn)__glsstim_MultiTexCoord3sARB,       /* extension slot 0x17f */
    (__GLdispatchFn)__glsstim_MultiTexCoord3svARB,      /* extension slot 0x180 */
    (__GLdispatchFn)__glsstim_MultiTexCoord4dARB,       /* extension slot 0x181 */
    (__GLdispatchFn)__glsstim_MultiTexCoord4dvARB,      /* extension slot 0x182 */
    (__GLdispatchFn)__glsstim_MultiTexCoord4fARB,       /* extension slot 0x183 */
    (__GLdispatchFn)__glsstim_MultiTexCoord4fvARB,      /* extension slot 0x184 */
    (__GLdispatchFn)__glsstim_MultiTexCoord4iARB,       /* extension slot 0x185 */
    (__GLdispatchFn)__glsstim_MultiTexCoord4ivARB,      /* extension slot 0x186 */
    (__GLdispatchFn)__glsstim_MultiTexCoord4sARB,       /* extension slot 0x187 */
    (__GLdispatchFn)__glsstim_MultiTexCoord4svARB,      /* extension slot 0x188 */
    (__GLdispatchFn)__glsstim_PointParameterfEXT,       /* extension slot 0x189 */
    (__GLdispatchFn)__glsstim_PointParameterfvEXT,      /* extension slot 0x18a */
    (__GLdispatchFn)__glsstim_BlendEquationEXT,         /* extension slot 0x18b */
    (__GLdispatchFn)__glCalcMixedTexture,               /* extension slot 0x18c */
    (__GLdispatchFn)__glsstim_CompressedTexImage2DARB,  /* extension slot 0x18d */
    (__GLdispatchFn)__glCalcMixedTexture,               /* extension slot 0x18e */
    (__GLdispatchFn)__glsstim_CompressedTexSubImage2DARB,/* extension slot 0x18f */
    (__GLdispatchFn)__glsstim_GetCompressedTexImageARB, /* extension slot 0x190 */
    (__GLdispatchFn)glTBufferMask3DFX_impl,             /* extension slot 0x191 */
    (__GLdispatchFn)__glim_FogCoordfEXT,                /* extension slot 0x192 */
    (__GLdispatchFn)__glim_FogCoorddEXT,                /* extension slot 0x193 */
    (__GLdispatchFn)__glim_FogCoordfvEXT,               /* extension slot 0x194 */
    (__GLdispatchFn)__glim_FogCoorddvEXT,               /* extension slot 0x195 */
    (__GLdispatchFn)__glim_FogCoordPointerEXT,          /* extension slot 0x196 */
} };   /* slots 0x197 .. 0x199: 0 */

/* 690ef728 -- __glSSTListCompState: the hardware path, display-list compile */
__GLdispatchTable __glSSTListCompState = { {
    (__GLdispatchFn)0x150,                              /* the entry count */
    (__GLdispatchFn)__glim_NewList,                     /* glNewList */
    (__GLdispatchFn)__glim_EndList,                     /* glEndList */
    (__GLdispatchFn)__gllc_CallList,                    /* glCallList */
    (__GLdispatchFn)__gllc_CallLists,                   /* glCallLists */
    (__GLdispatchFn)__glim_DeleteLists,                 /* glDeleteLists */
    (__GLdispatchFn)__glim_GenLists,                    /* glGenLists */
    (__GLdispatchFn)__gllc_ListBase,                    /* glListBase */
    (__GLdispatchFn)__gllc_Begin,                       /* glBegin */
    (__GLdispatchFn)__gllc_Bitmap,                      /* glBitmap */
    (__GLdispatchFn)__gllc_Color3b,                     /* glColor3b */
    (__GLdispatchFn)__gllc_Color3bv,                    /* glColor3bv */
    (__GLdispatchFn)__gllc_Color3d,                     /* glColor3d */
    (__GLdispatchFn)__gllc_Color3dv,                    /* glColor3dv */
    (__GLdispatchFn)__gllc_Color3f,                     /* glColor3f */
    (__GLdispatchFn)__gllc_Color3fv,                    /* glColor3fv */
    (__GLdispatchFn)__gllc_Color3i,                     /* glColor3i */
    (__GLdispatchFn)__gllc_Color3iv,                    /* glColor3iv */
    (__GLdispatchFn)__gllc_Color3s,                     /* glColor3s */
    (__GLdispatchFn)__gllc_Color3sv,                    /* glColor3sv */
    (__GLdispatchFn)__gllc_Color3ub,                    /* glColor3ub */
    (__GLdispatchFn)__gllc_Color3ubv,                   /* glColor3ubv */
    (__GLdispatchFn)__gllc_Color3ui,                    /* glColor3ui */
    (__GLdispatchFn)__gllc_Color3uiv,                   /* glColor3uiv */
    (__GLdispatchFn)__gllc_Color3us,                    /* glColor3us */
    (__GLdispatchFn)__gllc_Color3usv,                   /* glColor3usv */
    (__GLdispatchFn)__gllc_Color4b,                     /* glColor4b */
    (__GLdispatchFn)__gllc_Color4bv,                    /* glColor4bv */
    (__GLdispatchFn)__gllc_Color4d,                     /* glColor4d */
    (__GLdispatchFn)__gllc_Color4dv,                    /* glColor4dv */
    (__GLdispatchFn)__gllc_Color4f,                     /* glColor4f */
    (__GLdispatchFn)__gllc_Color4fv,                    /* glColor4fv */
    (__GLdispatchFn)__gllc_Color4i,                     /* glColor4i */
    (__GLdispatchFn)__gllc_Color4iv,                    /* glColor4iv */
    (__GLdispatchFn)__gllc_Color4s,                     /* glColor4s */
    (__GLdispatchFn)__gllc_Color4sv,                    /* glColor4sv */
    (__GLdispatchFn)__gllc_Color4ub,                    /* glColor4ub */
    (__GLdispatchFn)__gllc_Color4ubv,                   /* glColor4ubv */
    (__GLdispatchFn)__gllc_Color4ui,                    /* glColor4ui */
    (__GLdispatchFn)__gllc_Color4uiv,                   /* glColor4uiv */
    (__GLdispatchFn)__gllc_Color4us,                    /* glColor4us */
    (__GLdispatchFn)__gllc_Color4usv,                   /* glColor4usv */
    (__GLdispatchFn)__gllc_EdgeFlag,                    /* glEdgeFlag */
    (__GLdispatchFn)__gllc_EdgeFlagv,                   /* glEdgeFlagv */
    (__GLdispatchFn)__gllc_End,                         /* glEnd */
    (__GLdispatchFn)__gllc_Indexd,                      /* glIndexd */
    (__GLdispatchFn)__gllc_Indexdv,                     /* glIndexdv */
    (__GLdispatchFn)__gllc_Indexf,                      /* glIndexf */
    (__GLdispatchFn)__gllc_Indexfv,                     /* glIndexfv */
    (__GLdispatchFn)__gllc_Indexi,                      /* glIndexi */
    (__GLdispatchFn)__gllc_Indexiv,                     /* glIndexiv */
    (__GLdispatchFn)__gllc_Indexs,                      /* glIndexs */
    (__GLdispatchFn)__gllc_Indexsv,                     /* glIndexsv */
    (__GLdispatchFn)__gllc_Normal3b,                    /* glNormal3b */
    (__GLdispatchFn)__gllc_Normal3bv,                   /* glNormal3bv */
    (__GLdispatchFn)__gllc_Normal3d,                    /* glNormal3d */
    (__GLdispatchFn)__gllc_Normal3dv,                   /* glNormal3dv */
    (__GLdispatchFn)__gllc_Normal3f,                    /* glNormal3f */
    (__GLdispatchFn)__gllc_Normal3fv,                   /* glNormal3fv */
    (__GLdispatchFn)__gllc_Normal3i,                    /* glNormal3i */
    (__GLdispatchFn)__gllc_Normal3iv,                   /* glNormal3iv */
    (__GLdispatchFn)__gllc_Normal3s,                    /* glNormal3s */
    (__GLdispatchFn)__gllc_Normal3sv,                   /* glNormal3sv */
    (__GLdispatchFn)__gllc_RasterPos2d,                 /* glRasterPos2d */
    (__GLdispatchFn)__gllc_RasterPos2dv,                /* glRasterPos2dv */
    (__GLdispatchFn)__gllc_RasterPos2f,                 /* glRasterPos2f */
    (__GLdispatchFn)__gllc_RasterPos2fv,                /* glRasterPos2fv */
    (__GLdispatchFn)__gllc_RasterPos2i,                 /* glRasterPos2i */
    (__GLdispatchFn)__gllc_RasterPos2iv,                /* glRasterPos2iv */
    (__GLdispatchFn)__gllc_RasterPos2s,                 /* glRasterPos2s */
    (__GLdispatchFn)__gllc_RasterPos2sv,                /* glRasterPos2sv */
    (__GLdispatchFn)__gllc_RasterPos3d,                 /* glRasterPos3d */
    (__GLdispatchFn)__gllc_RasterPos3dv,                /* glRasterPos3dv */
    (__GLdispatchFn)__gllc_RasterPos3f,                 /* glRasterPos3f */
    (__GLdispatchFn)__gllc_RasterPos3fv,                /* glRasterPos3fv */
    (__GLdispatchFn)__gllc_RasterPos3i,                 /* glRasterPos3i */
    (__GLdispatchFn)__gllc_RasterPos3iv,                /* glRasterPos3iv */
    (__GLdispatchFn)__gllc_RasterPos3s,                 /* glRasterPos3s */
    (__GLdispatchFn)__gllc_RasterPos3sv,                /* glRasterPos3sv */
    (__GLdispatchFn)__gllc_RasterPos4d,                 /* glRasterPos4d */
    (__GLdispatchFn)__gllc_RasterPos4dv,                /* glRasterPos4dv */
    (__GLdispatchFn)__gllc_RasterPos4f,                 /* glRasterPos4f */
    (__GLdispatchFn)__gllc_RasterPos4fv,                /* glRasterPos4fv */
    (__GLdispatchFn)__gllc_RasterPos4i,                 /* glRasterPos4i */
    (__GLdispatchFn)__gllc_RasterPos4iv,                /* glRasterPos4iv */
    (__GLdispatchFn)__gllc_RasterPos4s,                 /* glRasterPos4s */
    (__GLdispatchFn)__gllc_RasterPos4sv,                /* glRasterPos4sv */
    (__GLdispatchFn)__gllc_Rectd,                       /* glRectd */
    (__GLdispatchFn)__gllc_Rectdv,                      /* glRectdv */
    (__GLdispatchFn)__gllc_Rectf,                       /* glRectf */
    (__GLdispatchFn)__gllc_Rectfv,                      /* glRectfv */
    (__GLdispatchFn)__gllc_Recti,                       /* glRecti */
    (__GLdispatchFn)__gllc_Rectiv,                      /* glRectiv */
    (__GLdispatchFn)__gllc_Rects,                       /* glRects */
    (__GLdispatchFn)__gllc_Rectsv,                      /* glRectsv */
    (__GLdispatchFn)__gllc_TexCoord1d,                  /* glTexCoord1d */
    (__GLdispatchFn)__gllc_TexCoord1dv,                 /* glTexCoord1dv */
    (__GLdispatchFn)__gllc_TexCoord1f,                  /* glTexCoord1f */
    (__GLdispatchFn)__gllc_TexCoord1fv,                 /* glTexCoord1fv */
    (__GLdispatchFn)__gllc_TexCoord1i,                  /* glTexCoord1i */
    (__GLdispatchFn)__gllc_TexCoord1iv,                 /* glTexCoord1iv */
    (__GLdispatchFn)__gllc_TexCoord1s,                  /* glTexCoord1s */
    (__GLdispatchFn)__gllc_TexCoord1sv,                 /* glTexCoord1sv */
    (__GLdispatchFn)__gllc_TexCoord2d,                  /* glTexCoord2d */
    (__GLdispatchFn)__gllc_TexCoord2dv,                 /* glTexCoord2dv */
    (__GLdispatchFn)__gllc_TexCoord2f,                  /* glTexCoord2f */
    (__GLdispatchFn)__gllc_TexCoord2fv,                 /* glTexCoord2fv */
    (__GLdispatchFn)__gllc_TexCoord2i,                  /* glTexCoord2i */
    (__GLdispatchFn)__gllc_TexCoord2iv,                 /* glTexCoord2iv */
    (__GLdispatchFn)__gllc_TexCoord2s,                  /* glTexCoord2s */
    (__GLdispatchFn)__gllc_TexCoord2sv,                 /* glTexCoord2sv */
    (__GLdispatchFn)__gllc_TexCoord3d,                  /* glTexCoord3d */
    (__GLdispatchFn)__gllc_TexCoord3dv,                 /* glTexCoord3dv */
    (__GLdispatchFn)__gllc_TexCoord3f,                  /* glTexCoord3f */
    (__GLdispatchFn)__gllc_TexCoord3fv,                 /* glTexCoord3fv */
    (__GLdispatchFn)__gllc_TexCoord3i,                  /* glTexCoord3i */
    (__GLdispatchFn)__gllc_TexCoord3iv,                 /* glTexCoord3iv */
    (__GLdispatchFn)__gllc_TexCoord3s,                  /* glTexCoord3s */
    (__GLdispatchFn)__gllc_TexCoord3sv,                 /* glTexCoord3sv */
    (__GLdispatchFn)__gllc_TexCoord4d,                  /* glTexCoord4d */
    (__GLdispatchFn)__gllc_TexCoord4dv,                 /* glTexCoord4dv */
    (__GLdispatchFn)__gllc_TexCoord4f,                  /* glTexCoord4f */
    (__GLdispatchFn)__gllc_TexCoord4fv,                 /* glTexCoord4fv */
    (__GLdispatchFn)__gllc_TexCoord4i,                  /* glTexCoord4i */
    (__GLdispatchFn)__gllc_TexCoord4iv,                 /* glTexCoord4iv */
    (__GLdispatchFn)__gllc_TexCoord4s,                  /* glTexCoord4s */
    (__GLdispatchFn)__gllc_TexCoord4sv,                 /* glTexCoord4sv */
    (__GLdispatchFn)__gllc_Vertex2d,                    /* glVertex2d */
    (__GLdispatchFn)__gllc_Vertex2dv,                   /* glVertex2dv */
    (__GLdispatchFn)__gllc_Vertex2f,                    /* glVertex2f */
    (__GLdispatchFn)__gllc_Vertex2fv,                   /* glVertex2fv */
    (__GLdispatchFn)__gllc_Vertex2i,                    /* glVertex2i */
    (__GLdispatchFn)__gllc_Vertex2iv,                   /* glVertex2iv */
    (__GLdispatchFn)__gllc_Vertex2s,                    /* glVertex2s */
    (__GLdispatchFn)__gllc_Vertex2sv,                   /* glVertex2sv */
    (__GLdispatchFn)__gllc_Vertex3d,                    /* glVertex3d */
    (__GLdispatchFn)__gllc_Vertex3dv,                   /* glVertex3dv */
    (__GLdispatchFn)__gllc_Vertex3f,                    /* glVertex3f */
    (__GLdispatchFn)__gllc_Vertex3fv,                   /* glVertex3fv */
    (__GLdispatchFn)__gllc_Vertex3i,                    /* glVertex3i */
    (__GLdispatchFn)__gllc_Vertex3iv,                   /* glVertex3iv */
    (__GLdispatchFn)__gllc_Vertex3s,                    /* glVertex3s */
    (__GLdispatchFn)__gllc_Vertex3sv,                   /* glVertex3sv */
    (__GLdispatchFn)__gllc_Vertex4d,                    /* glVertex4d */
    (__GLdispatchFn)__gllc_Vertex4dv,                   /* glVertex4dv */
    (__GLdispatchFn)__gllc_Vertex4f,                    /* glVertex4f */
    (__GLdispatchFn)__gllc_Vertex4fv,                   /* glVertex4fv */
    (__GLdispatchFn)__gllc_Vertex4i,                    /* glVertex4i */
    (__GLdispatchFn)__gllc_Vertex4iv,                   /* glVertex4iv */
    (__GLdispatchFn)__gllc_Vertex4s,                    /* glVertex4s */
    (__GLdispatchFn)__gllc_Vertex4sv,                   /* glVertex4sv */
    (__GLdispatchFn)__gllc_ClipPlane,                   /* glClipPlane */
    (__GLdispatchFn)__gllc_ColorMaterial,               /* glColorMaterial */
    (__GLdispatchFn)__gllc_CullFace,                    /* glCullFace */
    (__GLdispatchFn)__gllc_Fogf,                        /* glFogf */
    (__GLdispatchFn)__gllc_Fogfv,                       /* glFogfv */
    (__GLdispatchFn)__gllc_Fogi,                        /* glFogi */
    (__GLdispatchFn)__gllc_Fogiv,                       /* glFogiv */
    (__GLdispatchFn)__gllc_FrontFace,                   /* glFrontFace */
    (__GLdispatchFn)__gllc_Hint,                        /* glHint */
    (__GLdispatchFn)__gllc_Lightf,                      /* glLightf */
    (__GLdispatchFn)__gllc_Lightfv,                     /* glLightfv */
    (__GLdispatchFn)__gllc_Lighti,                      /* glLighti */
    (__GLdispatchFn)__gllc_Lightiv,                     /* glLightiv */
    (__GLdispatchFn)__gllc_LightModelf,                 /* glLightModelf */
    (__GLdispatchFn)__gllc_LightModelfv,                /* glLightModelfv */
    (__GLdispatchFn)__gllc_LightModeli,                 /* glLightModeli */
    (__GLdispatchFn)__gllc_LightModeliv,                /* glLightModeliv */
    (__GLdispatchFn)__gllc_LineStipple,                 /* glLineStipple */
    (__GLdispatchFn)__gllc_LineWidth,                   /* glLineWidth */
    (__GLdispatchFn)__gllc_Materialf,                   /* glMaterialf */
    (__GLdispatchFn)__gllc_Materialfv,                  /* glMaterialfv */
    (__GLdispatchFn)__gllc_Materiali,                   /* glMateriali */
    (__GLdispatchFn)__gllc_Materialiv,                  /* glMaterialiv */
    (__GLdispatchFn)__gllc_PointSize,                   /* glPointSize */
    (__GLdispatchFn)__gllc_PolygonMode,                 /* glPolygonMode */
    (__GLdispatchFn)__gllc_PolygonStipple,              /* glPolygonStipple */
    (__GLdispatchFn)__gllc_Scissor,                     /* glScissor */
    (__GLdispatchFn)__gllc_ShadeModel,                  /* glShadeModel */
    (__GLdispatchFn)__gllc_TexParameterf,               /* glTexParameterf */
    (__GLdispatchFn)__gllc_TexParameterfv,              /* glTexParameterfv */
    (__GLdispatchFn)__gllc_TexParameteri,               /* glTexParameteri */
    (__GLdispatchFn)__gllc_TexParameteriv,              /* glTexParameteriv */
    (__GLdispatchFn)__gllc_TexImage1D,                  /* glTexImage1D */
    (__GLdispatchFn)__gllc_TexImage2D,                  /* glTexImage2D */
    (__GLdispatchFn)__gllc_TexEnvf,                     /* glTexEnvf */
    (__GLdispatchFn)__gllc_TexEnvfv,                    /* glTexEnvfv */
    (__GLdispatchFn)__gllc_TexEnvi,                     /* glTexEnvi */
    (__GLdispatchFn)__gllc_TexEnviv,                    /* glTexEnviv */
    (__GLdispatchFn)__gllc_TexGend,                     /* glTexGend */
    (__GLdispatchFn)__gllc_TexGendv,                    /* glTexGendv */
    (__GLdispatchFn)__gllc_TexGenf,                     /* glTexGenf */
    (__GLdispatchFn)__gllc_TexGenfv,                    /* glTexGenfv */
    (__GLdispatchFn)__gllc_TexGeni,                     /* glTexGeni */
    (__GLdispatchFn)__gllc_TexGeniv,                    /* glTexGeniv */
    (__GLdispatchFn)__glim_FeedbackBuffer,                       /* glFeedbackBuffer */
    (__GLdispatchFn)__glim_SelectBuffer,                       /* glSelectBuffer */
    (__GLdispatchFn)__glim_RenderMode,                  /* glRenderMode */
    (__GLdispatchFn)__gllc_InitNames,                   /* glInitNames */
    (__GLdispatchFn)__gllc_LoadName,                    /* glLoadName */
    (__GLdispatchFn)__gllc_PassThrough,                 /* glPassThrough */
    (__GLdispatchFn)__gllc_PopName,                     /* glPopName */
    (__GLdispatchFn)__gllc_PushName,                    /* glPushName */
    (__GLdispatchFn)__gllc_DrawBuffer,                  /* glDrawBuffer */
    (__GLdispatchFn)__gllc_Clear,                       /* glClear */
    (__GLdispatchFn)__gllc_ClearAccum,                  /* glClearAccum */
    (__GLdispatchFn)__gllc_ClearIndex,                  /* glClearIndex */
    (__GLdispatchFn)__gllc_ClearColor,                  /* glClearColor */
    (__GLdispatchFn)__gllc_ClearStencil,                /* glClearStencil */
    (__GLdispatchFn)__gllc_ClearDepth,                  /* glClearDepth */
    (__GLdispatchFn)__gllc_StencilMask,                 /* glStencilMask */
    (__GLdispatchFn)__gllc_ColorMask,                   /* glColorMask */
    (__GLdispatchFn)__gllc_DepthMask,                   /* glDepthMask */
    (__GLdispatchFn)__gllc_IndexMask,                   /* glIndexMask */
    (__GLdispatchFn)__gllc_Accum,                       /* glAccum */
    (__GLdispatchFn)__gllc_Disable,                     /* glDisable */
    (__GLdispatchFn)__gllc_Enable,                      /* glEnable */
    (__GLdispatchFn)swapReturnAddrThunk,                /* glFinish */
    (__GLdispatchFn)__glim_Flush,                       /* glFlush */
    (__GLdispatchFn)__gllc_PopAttrib,                   /* glPopAttrib */
    (__GLdispatchFn)__gllc_PushAttrib,                  /* glPushAttrib */
    (__GLdispatchFn)__gllc_Map1d,                       /* glMap1d */
    (__GLdispatchFn)__gllc_Map1f,                       /* glMap1f */
    (__GLdispatchFn)__gllc_Map2d,                       /* glMap2d */
    (__GLdispatchFn)__gllc_Map2f,                       /* glMap2f */
    (__GLdispatchFn)__gllc_MapGrid1d,                   /* glMapGrid1d */
    (__GLdispatchFn)__gllc_MapGrid1f,                   /* glMapGrid1f */
    (__GLdispatchFn)__gllc_MapGrid2d,                   /* glMapGrid2d */
    (__GLdispatchFn)__gllc_MapGrid2f,                   /* glMapGrid2f */
    (__GLdispatchFn)__gllc_EvalCoord1d,                 /* glEvalCoord1d */
    (__GLdispatchFn)__gllc_EvalCoord1dv,                /* glEvalCoord1dv */
    (__GLdispatchFn)__gllc_EvalCoord1f,                 /* glEvalCoord1f */
    (__GLdispatchFn)__gllc_EvalCoord1fv,                /* glEvalCoord1fv */
    (__GLdispatchFn)__gllc_EvalCoord2d,                 /* glEvalCoord2d */
    (__GLdispatchFn)__gllc_EvalCoord2dv,                /* glEvalCoord2dv */
    (__GLdispatchFn)__gllc_EvalCoord2f,                 /* glEvalCoord2f */
    (__GLdispatchFn)__gllc_EvalCoord2fv,                /* glEvalCoord2fv */
    (__GLdispatchFn)__gllc_EvalMesh1,                   /* glEvalMesh1 */
    (__GLdispatchFn)__gllc_EvalPoint1,                  /* glEvalPoint1 */
    (__GLdispatchFn)__gllc_EvalMesh2,                   /* glEvalMesh2 */
    (__GLdispatchFn)__gllc_EvalPoint2,                  /* glEvalPoint2 */
    (__GLdispatchFn)__gllc_AlphaFunc,                   /* glAlphaFunc */
    (__GLdispatchFn)__gllc_BlendFunc,                   /* glBlendFunc */
    (__GLdispatchFn)__gllc_LogicOp,                     /* glLogicOp */
    (__GLdispatchFn)__gllc_StencilFunc,                 /* glStencilFunc */
    (__GLdispatchFn)__gllc_StencilOp,                   /* glStencilOp */
    (__GLdispatchFn)__gllc_DepthFunc,                   /* glDepthFunc */
    (__GLdispatchFn)__gllc_PixelZoom,                   /* glPixelZoom */
    (__GLdispatchFn)__gllc_PixelTransferf,              /* glPixelTransferf */
    (__GLdispatchFn)__gllc_PixelTransferi,              /* glPixelTransferi */
    (__GLdispatchFn)__glim_PixelStoref,                 /* glPixelStoref */
    (__GLdispatchFn)__glim_PixelStorei,                 /* glPixelStorei */
    (__GLdispatchFn)__gllc_PixelMapfv,                  /* glPixelMapfv */
    (__GLdispatchFn)__gllc_PixelMapuiv,                 /* glPixelMapuiv */
    (__GLdispatchFn)__gllc_PixelMapusv,                 /* glPixelMapusv */
    (__GLdispatchFn)__gllc_ReadBuffer,                  /* glReadBuffer */
    (__GLdispatchFn)__gllc_CopyPixels,                  /* glCopyPixels */
    (__GLdispatchFn)__glim_ReadPixels,                       /* glReadPixels */
    (__GLdispatchFn)__gllc_DrawPixels,                  /* glDrawPixels */
    (__GLdispatchFn)__glim_GetBooleanv,                 /* glGetBooleanv */
    (__GLdispatchFn)__glim_GetClipPlane,                       /* glGetClipPlane */
    (__GLdispatchFn)__glim_GetDoublev,                  /* glGetDoublev */
    (__GLdispatchFn)__glim_GetError,                    /* glGetError */
    (__GLdispatchFn)__glim_GetFloatv,                   /* glGetFloatv */
    (__GLdispatchFn)__glim_GetIntegerv,                 /* glGetIntegerv */
    (__GLdispatchFn)__glim_GetLightfv,                  /* glGetLightfv */
    (__GLdispatchFn)__glim_GetLightiv,                  /* glGetLightiv */
    (__GLdispatchFn)__glim_GetMapdv,                    /* glGetMapdv */
    (__GLdispatchFn)__glim_GetMapfv,                    /* glGetMapfv */
    (__GLdispatchFn)__glim_GetMapiv,                    /* glGetMapiv */
    (__GLdispatchFn)__glim_GetMaterialfv,               /* glGetMaterialfv */
    (__GLdispatchFn)__glim_GetMaterialiv,               /* glGetMaterialiv */
    (__GLdispatchFn)__glim_GetPixelMapfv,               /* glGetPixelMapfv */
    (__GLdispatchFn)__glim_GetPixelMapuiv,              /* glGetPixelMapuiv */
    (__GLdispatchFn)__glim_GetPixelMapusv,              /* glGetPixelMapusv */
    (__GLdispatchFn)__glim_GetPolygonStipple,           /* glGetPolygonStipple */
    (__GLdispatchFn)__glim_GetString,                       /* glGetString */
    (__GLdispatchFn)__glim_GetTexEnvfv,                 /* glGetTexEnvfv */
    (__GLdispatchFn)__glim_GetTexEnviv,                 /* glGetTexEnviv */
    (__GLdispatchFn)__glim_GetTexGendv,                       /* glGetTexGendv */
    (__GLdispatchFn)__glim_GetTexGenfv,                       /* glGetTexGenfv */
    (__GLdispatchFn)__glim_GetTexGeniv,                 /* glGetTexGeniv */
    (__GLdispatchFn)__glim_GetTexImage,                 /* glGetTexImage */
    (__GLdispatchFn)__glim_GetTexParameterfv,           /* glGetTexParameterfv */
    (__GLdispatchFn)__glim_GetTexParameteriv,           /* glGetTexParameteriv */
    (__GLdispatchFn)__glsstim_GetTexLevelParameterfv,   /* glGetTexLevelParameterfv */
    (__GLdispatchFn)__glsstim_GetTexLevelParameteriv,   /* glGetTexLevelParameteriv */
    (__GLdispatchFn)__glim_IsEnabled,                   /* glIsEnabled */
    (__GLdispatchFn)__glim_IsList,                      /* glIsList */
    (__GLdispatchFn)__gllc_DepthRange,                  /* glDepthRange */
    (__GLdispatchFn)__gllc_Frustum,                     /* glFrustum */
    (__GLdispatchFn)__gllc_LoadIdentity,                /* glLoadIdentity */
    (__GLdispatchFn)__gllc_LoadMatrixf,                 /* glLoadMatrixf */
    (__GLdispatchFn)__gllc_LoadMatrixd,                 /* glLoadMatrixd */
    (__GLdispatchFn)__gllc_MatrixMode,                  /* glMatrixMode */
    (__GLdispatchFn)__gllc_MultMatrixf,                 /* glMultMatrixf */
    (__GLdispatchFn)__gllc_MultMatrixd,                 /* glMultMatrixd */
    (__GLdispatchFn)__gllc_Ortho,                       /* glOrtho */
    (__GLdispatchFn)__gllc_PopMatrix,                   /* glPopMatrix */
    (__GLdispatchFn)__gllc_PushMatrix,                  /* glPushMatrix */
    (__GLdispatchFn)__gllc_Rotated,                     /* glRotated */
    (__GLdispatchFn)__gllc_Rotatef,                     /* glRotatef */
    (__GLdispatchFn)__gllc_Scaled,                      /* glScaled */
    (__GLdispatchFn)__gllc_Scalef,                      /* glScalef */
    (__GLdispatchFn)__gllc_Translated,                  /* glTranslated */
    (__GLdispatchFn)__gllc_Translatef,                  /* glTranslatef */
    (__GLdispatchFn)__gllc_Viewport,                    /* glViewport */
    (__GLdispatchFn)__gllc_ArrayElement,                /* glArrayElement */
    (__GLdispatchFn)__gllc_BindTexture,                 /* glBindTexture */
    (__GLdispatchFn)__glim_ColorPointer,                /* glColorPointer */
    (__GLdispatchFn)__glim_DisableClientState,          /* glDisableClientState */
    (__GLdispatchFn)__gllc_DrawArrays,                  /* glDrawArrays */
    (__GLdispatchFn)__gllc_DrawElements,                /* glDrawElements */
    (__GLdispatchFn)__glim_EdgeFlagPointer,             /* glEdgeFlagPointer */
    (__GLdispatchFn)__glim_EnableClientState,           /* glEnableClientState */
    (__GLdispatchFn)__glim_IndexPointer,                /* glIndexPointer */
    (__GLdispatchFn)__gllc_Indexub,                     /* glIndexub */
    (__GLdispatchFn)__gllc_Indexubv,                    /* glIndexubv */
    (__GLdispatchFn)__glim_InterleavedArrays,           /* glInterleavedArrays */
    (__GLdispatchFn)__glim_NormalPointer,               /* glNormalPointer */
    (__GLdispatchFn)__gllc_PolygonOffset,               /* glPolygonOffset */
    (__GLdispatchFn)__glim_TexCoordPointer,             /* glTexCoordPointer */
    (__GLdispatchFn)__glim_VertexPointer,               /* glVertexPointer */
    (__GLdispatchFn)__glim_AreTexturesResident,         /* glAreTexturesResident */
    (__GLdispatchFn)__gllc_CopyTexImage1D,              /* glCopyTexImage1D */
    (__GLdispatchFn)__gllc_CopyTexImage2D,              /* glCopyTexImage2D */
    (__GLdispatchFn)__gllc_CopyTexSubImage1D,           /* glCopyTexSubImage1D */
    (__GLdispatchFn)__gllc_CopyTexSubImage2D,           /* glCopyTexSubImage2D */
    (__GLdispatchFn)__glim_DeleteTextures,              /* glDeleteTextures */
    (__GLdispatchFn)__glim_GenTextures,                 /* glGenTextures */
    (__GLdispatchFn)__glim_GetPointerv,                 /* glGetPointerv */
    (__GLdispatchFn)__glim_IsTexture,                       /* glIsTexture */
    (__GLdispatchFn)__gllc_PrioritizeTextures,          /* glPrioritizeTextures */
    (__GLdispatchFn)__gllc_TexSubImage1D,               /* glTexSubImage1D */
    (__GLdispatchFn)__gllc_TexSubImage2D,               /* glTexSubImage2D */
    (__GLdispatchFn)__glim_PopClientAttrib,             /* glPopClientAttrib */
    (__GLdispatchFn)__glim_PushClientAttrib,            /* glPushClientAttrib */
    (__GLdispatchFn)__gllc_ArrayElementEXT,             /* extension slot 0x151 */
    (__GLdispatchFn)__glim_ColorPointerEXT,             /* extension slot 0x152 */
    (__GLdispatchFn)__gllc_DrawArraysEXT,               /* extension slot 0x153 */
    (__GLdispatchFn)__glim_EdgeFlagPointerEXT,          /* extension slot 0x154 */
    (__GLdispatchFn)__glim_GetPointervEXT,              /* extension slot 0x155 */
    (__GLdispatchFn)__glim_IndexPointerEXT,             /* extension slot 0x156 */
    (__GLdispatchFn)__glim_NormalPointerEXT,            /* extension slot 0x157 */
    (__GLdispatchFn)__glim_TexCoordPointerEXT,          /* extension slot 0x158 */
    (__GLdispatchFn)__glim_VertexPointerEXT,            /* extension slot 0x159 */
    (__GLdispatchFn)__gllc_ColorSubTable,               /* extension slot 0x15a */
    (__GLdispatchFn)__gllc_ColorTable,                  /* extension slot 0x15b */
    (__GLdispatchFn)__gllc_CopyColorTableEXT,           /* extension slot 0x15c */
    (__GLdispatchFn)__glim_GetColorTable,               /* extension slot 0x15d */
    (__GLdispatchFn)__glim_GetColorTableParameterfvEXT, /* extension slot 0x15e */
    (__GLdispatchFn)__glim_GetColorTableParameterivEXT, /* extension slot 0x15f */
    (__GLdispatchFn)__glim_LockArraysEXT,               /* extension slot 0x160 */
    (__GLdispatchFn)__glim_UnlockArraysEXT,             /* extension slot 0x161 */
    (__GLdispatchFn)glCullParameterdvEXT,               /* extension slot 0x162 */
    (__GLdispatchFn)__glim_CullParameterfvEXT,          /* extension slot 0x163 */
    (__GLdispatchFn)__gllc_IndexFuncSGI,                /* extension slot 0x164 */
    (__GLdispatchFn)__gllc_IndexMaterialSGI,            /* extension slot 0x165 */
    (__GLdispatchFn)__glim_AddSwapHintRectWIN,          /* extension slot 0x166 */
    (__GLdispatchFn)__gllc_ActiveTextureARB,            /* extension slot 0x167 */
    (__GLdispatchFn)__gllc_ClientActiveTextureARB,      /* extension slot 0x168 */
    (__GLdispatchFn)__gllc_MultiTexCoord1dARB,          /* extension slot 0x169 */
    (__GLdispatchFn)__gllc_MultiTexCoord1dvARB,         /* extension slot 0x16a */
    (__GLdispatchFn)__gllc_MultiTexCoord1fARB,          /* extension slot 0x16b */
    (__GLdispatchFn)__gllc_MultiTexCoord1fvARB,         /* extension slot 0x16c */
    (__GLdispatchFn)__gllc_MultiTexCoord1iARB,          /* extension slot 0x16d */
    (__GLdispatchFn)__gllc_MultiTexCoord1ivARB,         /* extension slot 0x16e */
    (__GLdispatchFn)__gllc_MultiTexCoord1sARB,          /* extension slot 0x16f */
    (__GLdispatchFn)__gllc_MultiTexCoord1svARB,         /* extension slot 0x170 */
    (__GLdispatchFn)__gllc_MultiTexCoord2dARB,          /* extension slot 0x171 */
    (__GLdispatchFn)__gllc_MultiTexCoord2dvARB,         /* extension slot 0x172 */
    (__GLdispatchFn)__gllc_MultiTexCoord2fARB,          /* extension slot 0x173 */
    (__GLdispatchFn)__gllc_MultiTexCoord2fvARB,         /* extension slot 0x174 */
    (__GLdispatchFn)__gllc_MultiTexCoord2iARB,          /* extension slot 0x175 */
    (__GLdispatchFn)__gllc_MultiTexCoord2ivARB,         /* extension slot 0x176 */
    (__GLdispatchFn)__gllc_MultiTexCoord2sARB,          /* extension slot 0x177 */
    (__GLdispatchFn)__gllc_MultiTexCoord2svARB,         /* extension slot 0x178 */
    (__GLdispatchFn)__gllc_MultiTexCoord3dARB,          /* extension slot 0x179 */
    (__GLdispatchFn)__gllc_MultiTexCoord3dvARB,         /* extension slot 0x17a */
    (__GLdispatchFn)__gllc_MultiTexCoord3fARB,          /* extension slot 0x17b */
    (__GLdispatchFn)__gllc_MultiTexCoord3fvARB,         /* extension slot 0x17c */
    (__GLdispatchFn)__gllc_MultiTexCoord3iARB,          /* extension slot 0x17d */
    (__GLdispatchFn)__gllc_MultiTexCoord3ivARB,         /* extension slot 0x17e */
    (__GLdispatchFn)__gllc_MultiTexCoord3sARB,          /* extension slot 0x17f */
    (__GLdispatchFn)__gllc_MultiTexCoord3svARB,         /* extension slot 0x180 */
    (__GLdispatchFn)__gllc_MultiTexCoord4dARB,          /* extension slot 0x181 */
    (__GLdispatchFn)__gllc_MultiTexCoord4dvARB,         /* extension slot 0x182 */
    (__GLdispatchFn)__gllc_MultiTexCoord4fARB,          /* extension slot 0x183 */
    (__GLdispatchFn)__gllc_MultiTexCoord4fvARB,         /* extension slot 0x184 */
    (__GLdispatchFn)__gllc_MultiTexCoord4iARB,          /* extension slot 0x185 */
    (__GLdispatchFn)__gllc_MultiTexCoord4ivARB,         /* extension slot 0x186 */
    (__GLdispatchFn)__gllc_MultiTexCoord4sARB,          /* extension slot 0x187 */
    (__GLdispatchFn)__gllc_MultiTexCoord4svARB,         /* extension slot 0x188 */
    (__GLdispatchFn)__gllc_PointParameterfEXT,          /* extension slot 0x189 */
    (__GLdispatchFn)__gllc_PointParameterfvEXT,         /* extension slot 0x18a */
    (__GLdispatchFn)__gllc_BlendEquationEXT,            /* extension slot 0x18b */
    (__GLdispatchFn)__gllc_CompressedTexImage1DARB,     /* extension slot 0x18c */
    (__GLdispatchFn)__gllc_CompressedTexImage2DARB,     /* extension slot 0x18d */
    (__GLdispatchFn)__gllc_CompressedTexSubImage1DARB,  /* extension slot 0x18e */
    (__GLdispatchFn)__gllc_CompressedTexSubImage2DARB,  /* extension slot 0x18f */
    (__GLdispatchFn)__glsstim_GetCompressedTexImageARB, /* extension slot 0x190 */
    (__GLdispatchFn)__gllc_TBufferMask3DFX,             /* extension slot 0x191 */
    (__GLdispatchFn)__gllc_FogCoordfEXT,                /* extension slot 0x192 */
    (__GLdispatchFn)__gllc_FogCoorddEXT,                /* extension slot 0x193 */
    (__GLdispatchFn)__gllc_FogCoordfvEXT,               /* extension slot 0x194 */
    (__GLdispatchFn)__gllc_FogCoorddvEXT,               /* extension slot 0x195 */
    (__GLdispatchFn)__glim_FogCoordPointerEXT,          /* extension slot 0x196 */
} };   /* slots 0x197 .. 0x199: 0 */

/* 690f3200 -- __glImmedState: the software path, immediate mode (ReturnToSWRendering) */
__GLdispatchTable __glImmedState = { {
    (__GLdispatchFn)0x150,                              /* the entry count */
    (__GLdispatchFn)__glim_NewList,                     /* glNewList */
    (__GLdispatchFn)__glim_EndList,                     /* glEndList */
    (__GLdispatchFn)__glim_CallList,                    /* glCallList */
    (__GLdispatchFn)__glim_CallLists,                   /* glCallLists */
    (__GLdispatchFn)__glim_DeleteLists,                 /* glDeleteLists */
    (__GLdispatchFn)__glim_GenLists,                    /* glGenLists */
    (__GLdispatchFn)__glim_ListBase,                       /* glListBase */
    (__GLdispatchFn)__glim_Begin,                       /* glBegin */
    (__GLdispatchFn)__glim_Bitmap,                      /* glBitmap */
    (__GLdispatchFn)__glim_Color3b,                       /* glColor3b */
    (__GLdispatchFn)__glim_Color3bv,                       /* glColor3bv */
    (__GLdispatchFn)__glim_Color3d,                     /* glColor3d */
    (__GLdispatchFn)__glim_Color3dv,                    /* glColor3dv */
    (__GLdispatchFn)__glim_Color3f,                       /* glColor3f */
    (__GLdispatchFn)__glim_Color3fv,                    /* glColor3fv */
    (__GLdispatchFn)__glim_Color3i,                     /* glColor3i */
    (__GLdispatchFn)__glim_Color3iv,                       /* glColor3iv */
    (__GLdispatchFn)__glim_Color3s,                       /* glColor3s */
    (__GLdispatchFn)__glim_Color3sv,                       /* glColor3sv */
    (__GLdispatchFn)__glim_Color3ub,                       /* glColor3ub */
    (__GLdispatchFn)__glim_Color3ubv,                       /* glColor3ubv */
    (__GLdispatchFn)__glim_Color3ui,                    /* glColor3ui */
    (__GLdispatchFn)__glim_Color3uiv,                       /* glColor3uiv */
    (__GLdispatchFn)__glim_Color3us,                       /* glColor3us */
    (__GLdispatchFn)__glim_Color3usv,                       /* glColor3usv */
    (__GLdispatchFn)__glim_Color4b,                       /* glColor4b */
    (__GLdispatchFn)__glim_Color4bv,                       /* glColor4bv */
    (__GLdispatchFn)__glim_Color4d,                     /* glColor4d */
    (__GLdispatchFn)__glim_Color4dv,                    /* glColor4dv */
    (__GLdispatchFn)__glim_Color4f,                     /* glColor4f */
    (__GLdispatchFn)__glim_Color4fv,                    /* glColor4fv */
    (__GLdispatchFn)__glim_Color4i,                     /* glColor4i */
    (__GLdispatchFn)__glim_Color4iv,                       /* glColor4iv */
    (__GLdispatchFn)__glim_Color4s,                       /* glColor4s */
    (__GLdispatchFn)__glim_Color4sv,                       /* glColor4sv */
    (__GLdispatchFn)__glim_Color4ub,                       /* glColor4ub */
    (__GLdispatchFn)__glim_Color4ubv,                       /* glColor4ubv */
    (__GLdispatchFn)__glim_Color4ui,                    /* glColor4ui */
    (__GLdispatchFn)__glim_Color4uiv,                       /* glColor4uiv */
    (__GLdispatchFn)__glim_Color4us,                       /* glColor4us */
    (__GLdispatchFn)__glim_Color4usv,                       /* glColor4usv */
    (__GLdispatchFn)__glim_EdgeFlag,                    /* glEdgeFlag */
    (__GLdispatchFn)__glim_EdgeFlagv,                   /* glEdgeFlagv */
    (__GLdispatchFn)__glim_End,                         /* glEnd */
    (__GLdispatchFn)__glim_Indexd,                      /* glIndexd */
    (__GLdispatchFn)__glim_Indexdv,                     /* glIndexdv */
    (__GLdispatchFn)__glim_Indexf,                      /* glIndexf */
    (__GLdispatchFn)__glim_Indexfv,                       /* glIndexfv */
    (__GLdispatchFn)__glim_Indexi,                      /* glIndexi */
    (__GLdispatchFn)__glim_Indexiv,                       /* glIndexiv */
    (__GLdispatchFn)__glim_Indexs,                      /* glIndexs */
    (__GLdispatchFn)__glim_Indexsv,                       /* glIndexsv */
    (__GLdispatchFn)__glim_Normal3b,                       /* glNormal3b */
    (__GLdispatchFn)__glim_Normal3bv,                       /* glNormal3bv */
    (__GLdispatchFn)__glim_Normal3d,                       /* glNormal3d */
    (__GLdispatchFn)__glim_Normal3dv,                       /* glNormal3dv */
    (__GLdispatchFn)__glim_Normal3f,                       /* glNormal3f */
    (__GLdispatchFn)__glim_Normal3fv,                       /* glNormal3fv */
    (__GLdispatchFn)__glim_Normal3i,                       /* glNormal3i */
    (__GLdispatchFn)__glim_Normal3iv,                       /* glNormal3iv */
    (__GLdispatchFn)__glim_Normal3s,                       /* glNormal3s */
    (__GLdispatchFn)__glim_Normal3sv,                       /* glNormal3sv */
    (__GLdispatchFn)__glim_RasterPos2d,                       /* glRasterPos2d */
    (__GLdispatchFn)__glim_RasterPos2dv,                /* glRasterPos2dv */
    (__GLdispatchFn)__glim_RasterPos2f,                       /* glRasterPos2f */
    (__GLdispatchFn)__glim_RasterPos2fv,                       /* glRasterPos2fv */
    (__GLdispatchFn)__glim_RasterPos2i,                       /* glRasterPos2i */
    (__GLdispatchFn)__glim_RasterPos2iv,                       /* glRasterPos2iv */
    (__GLdispatchFn)__glim_RasterPos2s,                       /* glRasterPos2s */
    (__GLdispatchFn)__glim_RasterPos2sv,                       /* glRasterPos2sv */
    (__GLdispatchFn)__glim_RasterPos3d,                       /* glRasterPos3d */
    (__GLdispatchFn)__glim_RasterPos3dv,                /* glRasterPos3dv */
    (__GLdispatchFn)__glim_RasterPos3f,                       /* glRasterPos3f */
    (__GLdispatchFn)__glim_RasterPos3fv,                       /* glRasterPos3fv */
    (__GLdispatchFn)__glim_RasterPos3i,                       /* glRasterPos3i */
    (__GLdispatchFn)__glim_RasterPos3iv,                       /* glRasterPos3iv */
    (__GLdispatchFn)__glim_RasterPos3s,                       /* glRasterPos3s */
    (__GLdispatchFn)__glim_RasterPos3sv,                       /* glRasterPos3sv */
    (__GLdispatchFn)__glim_RasterPos4d,                       /* glRasterPos4d */
    (__GLdispatchFn)__glim_RasterPos4dv,                /* glRasterPos4dv */
    (__GLdispatchFn)__glim_RasterPos4f,                       /* glRasterPos4f */
    (__GLdispatchFn)__glim_RasterPos4fv,                       /* glRasterPos4fv */
    (__GLdispatchFn)__glim_RasterPos4i,                       /* glRasterPos4i */
    (__GLdispatchFn)__glim_RasterPos4iv,                       /* glRasterPos4iv */
    (__GLdispatchFn)__glim_RasterPos4s,                       /* glRasterPos4s */
    (__GLdispatchFn)__glim_RasterPos4sv,                       /* glRasterPos4sv */
    (__GLdispatchFn)__glim_Rectd,                       /* glRectd */
    (__GLdispatchFn)__glim_Rectdv,                      /* glRectdv */
    (__GLdispatchFn)__glim_Rectf,                       /* glRectf */
    (__GLdispatchFn)__glim_Rectfv,                      /* glRectfv */
    (__GLdispatchFn)__glim_Recti,                       /* glRecti */
    (__GLdispatchFn)__glim_Rectiv,                      /* glRectiv */
    (__GLdispatchFn)__glim_Rects,                       /* glRects */
    (__GLdispatchFn)__glim_Rectsv,                      /* glRectsv */
    (__GLdispatchFn)__glim_TexCoord1d,                  /* glTexCoord1d */
    (__GLdispatchFn)__glim_TexCoord1dv,                 /* glTexCoord1dv */
    (__GLdispatchFn)__glim_TexCoord1f,                  /* glTexCoord1f */
    (__GLdispatchFn)__glim_TexCoord1fv,                 /* glTexCoord1fv */
    (__GLdispatchFn)__glim_TexCoord1i,                  /* glTexCoord1i */
    (__GLdispatchFn)__glim_TexCoord1iv,                 /* glTexCoord1iv */
    (__GLdispatchFn)__glim_TexCoord1s,                  /* glTexCoord1s */
    (__GLdispatchFn)__glim_TexCoord1sv,                 /* glTexCoord1sv */
    (__GLdispatchFn)__glim_TexCoord2d,                  /* glTexCoord2d */
    (__GLdispatchFn)__glim_TexCoord2dv,                 /* glTexCoord2dv */
    (__GLdispatchFn)__glim_TexCoord2f,                  /* glTexCoord2f */
    (__GLdispatchFn)__glim_TexCoord2fv,                 /* glTexCoord2fv */
    (__GLdispatchFn)__glim_TexCoord2i,                  /* glTexCoord2i */
    (__GLdispatchFn)__glim_TexCoord2iv,                 /* glTexCoord2iv */
    (__GLdispatchFn)__glim_TexCoord2s,                  /* glTexCoord2s */
    (__GLdispatchFn)__glim_TexCoord2sv,                 /* glTexCoord2sv */
    (__GLdispatchFn)__glim_TexCoord3d,                  /* glTexCoord3d */
    (__GLdispatchFn)__glim_TexCoord3dv,                 /* glTexCoord3dv */
    (__GLdispatchFn)__glim_TexCoord3f,                  /* glTexCoord3f */
    (__GLdispatchFn)__glim_TexCoord3fv,                 /* glTexCoord3fv */
    (__GLdispatchFn)__glim_TexCoord3i,                  /* glTexCoord3i */
    (__GLdispatchFn)__glim_TexCoord3iv,                 /* glTexCoord3iv */
    (__GLdispatchFn)__glim_TexCoord3s,                  /* glTexCoord3s */
    (__GLdispatchFn)__glim_TexCoord3sv,                 /* glTexCoord3sv */
    (__GLdispatchFn)__glim_TexCoord4d,                  /* glTexCoord4d */
    (__GLdispatchFn)__glim_TexCoord4dv,                 /* glTexCoord4dv */
    (__GLdispatchFn)__glim_TexCoord4f,                  /* glTexCoord4f */
    (__GLdispatchFn)__glim_TexCoord4fv,                 /* glTexCoord4fv */
    (__GLdispatchFn)__glim_TexCoord4i,                  /* glTexCoord4i */
    (__GLdispatchFn)__glim_TexCoord4iv,                 /* glTexCoord4iv */
    (__GLdispatchFn)__glim_TexCoord4s,                  /* glTexCoord4s */
    (__GLdispatchFn)__glim_TexCoord4sv,                 /* glTexCoord4sv */
    (__GLdispatchFn)__glim_Vertex2d,                    /* glVertex2d */
    (__GLdispatchFn)__glim_Vertex2dv,                   /* glVertex2dv */
    (__GLdispatchFn)__glim_Vertex2f,                    /* glVertex2f */
    (__GLdispatchFn)__glim_Vertex2fv,                   /* glVertex2fv */
    (__GLdispatchFn)__glim_Vertex2i,                    /* glVertex2i */
    (__GLdispatchFn)__glim_Vertex2iv,                   /* glVertex2iv */
    (__GLdispatchFn)__glim_Vertex2s,                    /* glVertex2s */
    (__GLdispatchFn)__glim_Vertex2sv,                   /* glVertex2sv */
    (__GLdispatchFn)__glim_Vertex3d,                    /* glVertex3d */
    (__GLdispatchFn)__glim_Vertex3dv,                   /* glVertex3dv */
    (__GLdispatchFn)__glim_Vertex3f,                    /* glVertex3f */
    (__GLdispatchFn)__glim_Vertex3fv,                   /* glVertex3fv */
    (__GLdispatchFn)__glim_Vertex3i,                    /* glVertex3i */
    (__GLdispatchFn)__glim_Vertex3iv,                   /* glVertex3iv */
    (__GLdispatchFn)__glim_Vertex3s,                    /* glVertex3s */
    (__GLdispatchFn)__glim_Vertex3sv,                   /* glVertex3sv */
    (__GLdispatchFn)__glim_Vertex4d,                    /* glVertex4d */
    (__GLdispatchFn)__glim_Vertex4dv,                   /* glVertex4dv */
    (__GLdispatchFn)__glim_Vertex4f,                    /* glVertex4f */
    (__GLdispatchFn)__glim_Vertex4fv,                   /* glVertex4fv */
    (__GLdispatchFn)__glim_Vertex4i,                    /* glVertex4i */
    (__GLdispatchFn)__glim_Vertex4iv,                   /* glVertex4iv */
    (__GLdispatchFn)__glim_Vertex4s,                    /* glVertex4s */
    (__GLdispatchFn)__glim_Vertex4sv,                   /* glVertex4sv */
    (__GLdispatchFn)__glim_ClipPlane,                   /* glClipPlane */
    (__GLdispatchFn)__glim_ColorMaterial,                       /* glColorMaterial */
    (__GLdispatchFn)__glim_CullFace,                       /* glCullFace */
    (__GLdispatchFn)__glim_Fogf,                        /* glFogf */
    (__GLdispatchFn)__glim_Fogfv,                       /* glFogfv */
    (__GLdispatchFn)__glim_Fogi,                        /* glFogi */
    (__GLdispatchFn)__glim_Fogiv,                       /* glFogiv */
    (__GLdispatchFn)__glim_FrontFace,                       /* glFrontFace */
    (__GLdispatchFn)__glim_Hint,                        /* glHint */
    (__GLdispatchFn)__glim_Lightf,                      /* glLightf */
    (__GLdispatchFn)__glim_Lightfv,                     /* glLightfv */
    (__GLdispatchFn)__glim_Lighti,                      /* glLighti */
    (__GLdispatchFn)__glim_Lightiv,                     /* glLightiv */
    (__GLdispatchFn)__glim_LightModelf,                 /* glLightModelf */
    (__GLdispatchFn)__glim_LightModelfv,                /* glLightModelfv */
    (__GLdispatchFn)__glim_LightModeli,                 /* glLightModeli */
    (__GLdispatchFn)__glim_LightModeliv,                /* glLightModeliv */
    (__GLdispatchFn)__glim_LineStipple,                       /* glLineStipple */
    (__GLdispatchFn)__glim_LineWidth,                   /* glLineWidth */
    (__GLdispatchFn)__glim_Materialf,                   /* glMaterialf */
    (__GLdispatchFn)__glim_Materialfv,                  /* glMaterialfv */
    (__GLdispatchFn)__glim_Materiali,                   /* glMateriali */
    (__GLdispatchFn)__glim_Materialiv,                  /* glMaterialiv */
    (__GLdispatchFn)__glim_PointSize,                   /* glPointSize */
    (__GLdispatchFn)__glim_PolygonMode,                       /* glPolygonMode */
    (__GLdispatchFn)__glim_PolygonStipple,              /* glPolygonStipple */
    (__GLdispatchFn)__glim_Scissor,                       /* glScissor */
    (__GLdispatchFn)__glim_ShadeModel,                  /* glShadeModel */
    (__GLdispatchFn)__glim_TexParameterf,               /* glTexParameterf */
    (__GLdispatchFn)__glim_TexParameterfv,              /* glTexParameterfv */
    (__GLdispatchFn)__glim_TexParameteri,               /* glTexParameteri */
    (__GLdispatchFn)__glim_TexParameteriv,              /* glTexParameteriv */
    (__GLdispatchFn)__glim_TexImage1D,                  /* glTexImage1D */
    (__GLdispatchFn)__glim_TexImage2D,                  /* glTexImage2D */
    (__GLdispatchFn)__glim_TexEnvf,                     /* glTexEnvf */
    (__GLdispatchFn)__glim_TexEnvfv,                    /* glTexEnvfv */
    (__GLdispatchFn)__glim_TexEnvi,                     /* glTexEnvi */
    (__GLdispatchFn)__glim_TexEnviv,                    /* glTexEnviv */
    (__GLdispatchFn)__glim_TexGend,                     /* glTexGend */
    (__GLdispatchFn)__glim_TexGendv,                    /* glTexGendv */
    (__GLdispatchFn)__glim_TexGenf,                     /* glTexGenf */
    (__GLdispatchFn)__glim_TexGenfv,                    /* glTexGenfv */
    (__GLdispatchFn)__glim_TexGeni,                     /* glTexGeni */
    (__GLdispatchFn)__glim_TexGeniv,                    /* glTexGeniv */
    (__GLdispatchFn)__glim_FeedbackBuffer,                       /* glFeedbackBuffer */
    (__GLdispatchFn)__glim_SelectBuffer,                       /* glSelectBuffer */
    (__GLdispatchFn)__glim_RenderMode,                  /* glRenderMode */
    (__GLdispatchFn)__glim_InitNames,                   /* glInitNames */
    (__GLdispatchFn)__glim_LoadName,                       /* glLoadName */
    (__GLdispatchFn)__glim_PassThrough,                       /* glPassThrough */
    (__GLdispatchFn)__glim_PopName,                     /* glPopName */
    (__GLdispatchFn)__glim_PushName,                       /* glPushName */
    (__GLdispatchFn)__glim_DrawBuffer,                  /* glDrawBuffer */
    (__GLdispatchFn)__glim_Clear,                       /* glClear */
    (__GLdispatchFn)__glim_ClearAccum,                       /* glClearAccum */
    (__GLdispatchFn)__glim_ClearIndex,                       /* glClearIndex */
    (__GLdispatchFn)__glim_ClearColor,                       /* glClearColor */
    (__GLdispatchFn)__glim_ClearStencil,                /* glClearStencil */
    (__GLdispatchFn)__glim_ClearDepth,                  /* glClearDepth */
    (__GLdispatchFn)__glim_StencilMask,                 /* glStencilMask */
    (__GLdispatchFn)__glim_ColorMask,                       /* glColorMask */
    (__GLdispatchFn)__glim_DepthMask,                       /* glDepthMask */
    (__GLdispatchFn)__glim_IndexMask,                   /* glIndexMask */
    (__GLdispatchFn)__glim_Accum,                       /* glAccum */
    (__GLdispatchFn)__glim_Disable,                     /* glDisable */
    (__GLdispatchFn)__glim_Enable,                      /* glEnable */
    (__GLdispatchFn)finishReturnAddrThunk,              /* glFinish */
    (__GLdispatchFn)__glim_Flush,                       /* glFlush */
    (__GLdispatchFn)__glim_PopAttrib,                   /* glPopAttrib */
    (__GLdispatchFn)__glim_PushAttrib,                  /* glPushAttrib */
    (__GLdispatchFn)__glim_Map1d,                       /* glMap1d */
    (__GLdispatchFn)__glim_Map1f,                       /* glMap1f */
    (__GLdispatchFn)__glim_Map2d,                       /* glMap2d */
    (__GLdispatchFn)__glim_Map2f,                       /* glMap2f */
    (__GLdispatchFn)__glim_MapGrid1d,                   /* glMapGrid1d */
    (__GLdispatchFn)__glim_MapGrid1f,                       /* glMapGrid1f */
    (__GLdispatchFn)__glim_MapGrid2d,                   /* glMapGrid2d */
    (__GLdispatchFn)__glim_MapGrid2f,                       /* glMapGrid2f */
    (__GLdispatchFn)__glim_EvalCoord1d,                       /* glEvalCoord1d */
    (__GLdispatchFn)__glim_EvalCoord1dv,                       /* glEvalCoord1dv */
    (__GLdispatchFn)__glim_EvalCoord1f,                       /* glEvalCoord1f */
    (__GLdispatchFn)__glim_EvalCoord1fv,                       /* glEvalCoord1fv */
    (__GLdispatchFn)__glim_EvalCoord2d,                       /* glEvalCoord2d */
    (__GLdispatchFn)__glim_EvalCoord2dv,                       /* glEvalCoord2dv */
    (__GLdispatchFn)__glim_EvalCoord2f,                       /* glEvalCoord2f */
    (__GLdispatchFn)__glim_EvalCoord2fv,                       /* glEvalCoord2fv */
    (__GLdispatchFn)__glim_EvalMesh1,                       /* glEvalMesh1 */
    (__GLdispatchFn)__glim_EvalPoint1,                  /* glEvalPoint1 */
    (__GLdispatchFn)__glim_EvalMesh2,                       /* glEvalMesh2 */
    (__GLdispatchFn)__glim_EvalPoint2,                  /* glEvalPoint2 */
    (__GLdispatchFn)__glim_AlphaFunc,                   /* glAlphaFunc */
    (__GLdispatchFn)__glim_BlendFunc,                       /* glBlendFunc */
    (__GLdispatchFn)__glim_LogicOp,                       /* glLogicOp */
    (__GLdispatchFn)__glim_StencilFunc,                 /* glStencilFunc */
    (__GLdispatchFn)__glim_StencilOp,                   /* glStencilOp */
    (__GLdispatchFn)__glim_DepthFunc,                       /* glDepthFunc */
    (__GLdispatchFn)__glim_PixelZoom,                   /* glPixelZoom */
    (__GLdispatchFn)__glim_PixelTransferf,              /* glPixelTransferf */
    (__GLdispatchFn)__glim_PixelTransferi,              /* glPixelTransferi */
    (__GLdispatchFn)__glim_PixelStoref,                 /* glPixelStoref */
    (__GLdispatchFn)__glim_PixelStorei,                 /* glPixelStorei */
    (__GLdispatchFn)__glim_PixelMapfv,                       /* glPixelMapfv */
    (__GLdispatchFn)__glim_PixelMapuiv,                       /* glPixelMapuiv */
    (__GLdispatchFn)__glim_PixelMapusv,                       /* glPixelMapusv */
    (__GLdispatchFn)__glim_ReadBuffer,                       /* glReadBuffer */
    (__GLdispatchFn)__glim_CopyPixels,                  /* glCopyPixels */
    (__GLdispatchFn)__glim_ReadPixels,                       /* glReadPixels */
    (__GLdispatchFn)__glim_DrawPixels,                       /* glDrawPixels */
    (__GLdispatchFn)__glim_GetBooleanv,                 /* glGetBooleanv */
    (__GLdispatchFn)__glim_GetClipPlane,                       /* glGetClipPlane */
    (__GLdispatchFn)__glim_GetDoublev,                  /* glGetDoublev */
    (__GLdispatchFn)__glim_GetError,                    /* glGetError */
    (__GLdispatchFn)__glim_GetFloatv,                   /* glGetFloatv */
    (__GLdispatchFn)__glim_GetIntegerv,                 /* glGetIntegerv */
    (__GLdispatchFn)__glim_GetLightfv,                  /* glGetLightfv */
    (__GLdispatchFn)__glim_GetLightiv,                  /* glGetLightiv */
    (__GLdispatchFn)__glim_GetMapdv,                    /* glGetMapdv */
    (__GLdispatchFn)__glim_GetMapfv,                    /* glGetMapfv */
    (__GLdispatchFn)__glim_GetMapiv,                    /* glGetMapiv */
    (__GLdispatchFn)__glim_GetMaterialfv,               /* glGetMaterialfv */
    (__GLdispatchFn)__glim_GetMaterialiv,               /* glGetMaterialiv */
    (__GLdispatchFn)__glim_GetPixelMapfv,               /* glGetPixelMapfv */
    (__GLdispatchFn)__glim_GetPixelMapuiv,              /* glGetPixelMapuiv */
    (__GLdispatchFn)__glim_GetPixelMapusv,              /* glGetPixelMapusv */
    (__GLdispatchFn)__glim_GetPolygonStipple,           /* glGetPolygonStipple */
    (__GLdispatchFn)__glim_GetString,                       /* glGetString */
    (__GLdispatchFn)__glim_GetTexEnvfv,                 /* glGetTexEnvfv */
    (__GLdispatchFn)__glim_GetTexEnviv,                 /* glGetTexEnviv */
    (__GLdispatchFn)__glim_GetTexGendv,                       /* glGetTexGendv */
    (__GLdispatchFn)__glim_GetTexGenfv,                       /* glGetTexGenfv */
    (__GLdispatchFn)__glim_GetTexGeniv,                 /* glGetTexGeniv */
    (__GLdispatchFn)__glim_GetTexImage,                 /* glGetTexImage */
    (__GLdispatchFn)__glim_GetTexParameterfv,           /* glGetTexParameterfv */
    (__GLdispatchFn)__glim_GetTexParameteriv,           /* glGetTexParameteriv */
    (__GLdispatchFn)__glim_GetTexLevelParameterfv,      /* glGetTexLevelParameterfv */
    (__GLdispatchFn)__glim_GetTexLevelParameteriv,      /* glGetTexLevelParameteriv */
    (__GLdispatchFn)__glim_IsEnabled,                   /* glIsEnabled */
    (__GLdispatchFn)__glim_IsList,                      /* glIsList */
    (__GLdispatchFn)__glim_DepthRange,                  /* glDepthRange */
    (__GLdispatchFn)__glim_Frustum,                     /* glFrustum */
    (__GLdispatchFn)__glim_LoadIdentity,                       /* glLoadIdentity */
    (__GLdispatchFn)__glim_LoadMatrixf,                       /* glLoadMatrixf */
    (__GLdispatchFn)__glim_LoadMatrixd,                       /* glLoadMatrixd */
    (__GLdispatchFn)__glim_MatrixMode,                       /* glMatrixMode */
    (__GLdispatchFn)__glim_MultMatrixf,                 /* glMultMatrixf */
    (__GLdispatchFn)__glim_MultMatrixd,                 /* glMultMatrixd */
    (__GLdispatchFn)__glim_Ortho,                       /* glOrtho */
    (__GLdispatchFn)__glim_PopMatrix,                       /* glPopMatrix */
    (__GLdispatchFn)__glim_PushMatrix,                       /* glPushMatrix */
    (__GLdispatchFn)__glim_Rotated,                     /* glRotated */
    (__GLdispatchFn)__glim_Rotatef,                     /* glRotatef */
    (__GLdispatchFn)__glim_Scaled,                      /* glScaled */
    (__GLdispatchFn)__glim_Scalef,                      /* glScalef */
    (__GLdispatchFn)__glim_Translated,                  /* glTranslated */
    (__GLdispatchFn)__glim_Translatef,                  /* glTranslatef */
    (__GLdispatchFn)__glim_Viewport,                    /* glViewport */
    (__GLdispatchFn)__glim_ArrayElement,                /* glArrayElement */
    (__GLdispatchFn)__glim_BindTexture,                 /* glBindTexture */
    (__GLdispatchFn)__glim_ColorPointer,                /* glColorPointer */
    (__GLdispatchFn)__glim_DisableClientState,          /* glDisableClientState */
    (__GLdispatchFn)__glim_DrawArrays,                  /* glDrawArrays */
    (__GLdispatchFn)__glim_DrawElements,                /* glDrawElements */
    (__GLdispatchFn)__glim_EdgeFlagPointer,             /* glEdgeFlagPointer */
    (__GLdispatchFn)__glim_EnableClientState,           /* glEnableClientState */
    (__GLdispatchFn)__glim_IndexPointer,                /* glIndexPointer */
    (__GLdispatchFn)__glim_Indexub,                     /* glIndexub */
    (__GLdispatchFn)__glim_Indexubv,                       /* glIndexubv */
    (__GLdispatchFn)__glim_InterleavedArrays,           /* glInterleavedArrays */
    (__GLdispatchFn)__glim_NormalPointer,               /* glNormalPointer */
    (__GLdispatchFn)__glim_PolygonOffset,               /* glPolygonOffset */
    (__GLdispatchFn)__glim_TexCoordPointer,             /* glTexCoordPointer */
    (__GLdispatchFn)__glim_VertexPointer,               /* glVertexPointer */
    (__GLdispatchFn)__glim_AreTexturesResident,         /* glAreTexturesResident */
    (__GLdispatchFn)__glim_CopyTexImage1D,              /* glCopyTexImage1D */
    (__GLdispatchFn)__glim_CopyTexImage2D,              /* glCopyTexImage2D */
    (__GLdispatchFn)__glim_CopyTexSubImage1D,           /* glCopyTexSubImage1D */
    (__GLdispatchFn)__glim_CopyTexSubImage2D,           /* glCopyTexSubImage2D */
    (__GLdispatchFn)__glim_DeleteTextures,              /* glDeleteTextures */
    (__GLdispatchFn)__glim_GenTextures,                 /* glGenTextures */
    (__GLdispatchFn)__glim_GetPointerv,                 /* glGetPointerv */
    (__GLdispatchFn)__glim_IsTexture,                       /* glIsTexture */
    (__GLdispatchFn)__glim_PrioritizeTextures,          /* glPrioritizeTextures */
    (__GLdispatchFn)__glim_TexSubImage1D,               /* glTexSubImage1D */
    (__GLdispatchFn)__glim_TexSubImage2D,               /* glTexSubImage2D */
    (__GLdispatchFn)__glim_PopClientAttrib,             /* glPopClientAttrib */
    (__GLdispatchFn)__glim_PushClientAttrib,            /* glPushClientAttrib */
    (__GLdispatchFn)__glim_ArrayElementEXT,             /* extension slot 0x151 */
    (__GLdispatchFn)__glim_ColorPointerEXT,             /* extension slot 0x152 */
    (__GLdispatchFn)__glim_DrawArraysEXT,               /* extension slot 0x153 */
    (__GLdispatchFn)__glim_EdgeFlagPointerEXT,          /* extension slot 0x154 */
    (__GLdispatchFn)__glim_GetPointervEXT,              /* extension slot 0x155 */
    (__GLdispatchFn)__glim_IndexPointerEXT,             /* extension slot 0x156 */
    (__GLdispatchFn)__glim_NormalPointerEXT,            /* extension slot 0x157 */
    (__GLdispatchFn)__glim_TexCoordPointerEXT,          /* extension slot 0x158 */
    (__GLdispatchFn)__glim_VertexPointerEXT,            /* extension slot 0x159 */
    (__GLdispatchFn)__glim_ColorSubTable,               /* extension slot 0x15a */
    (__GLdispatchFn)__glim_ColorTable,                  /* extension slot 0x15b */
    (__GLdispatchFn)__glim_CopyColorTableEXT,           /* extension slot 0x15c */
    (__GLdispatchFn)__glim_GetColorTable,               /* extension slot 0x15d */
    (__GLdispatchFn)__glim_GetColorTableParameterfvEXT, /* extension slot 0x15e */
    (__GLdispatchFn)__glim_GetColorTableParameterivEXT, /* extension slot 0x15f */
    (__GLdispatchFn)__glim_LockArraysEXT,               /* extension slot 0x160 */
    (__GLdispatchFn)__glim_UnlockArraysEXT,             /* extension slot 0x161 */
    (__GLdispatchFn)glCullParameterdvEXT,               /* extension slot 0x162 */
    (__GLdispatchFn)__glim_CullParameterfvEXT,          /* extension slot 0x163 */
    (__GLdispatchFn)__glim_IndexFuncSGI,                /* extension slot 0x164 */
    (__GLdispatchFn)__glim_IndexMaterialSGI,            /* extension slot 0x165 */
    (__GLdispatchFn)__glim_AddSwapHintRectWIN,          /* extension slot 0x166 */
} };   /* slots 0x167 .. 0x199: 0 */

/* 690f3868 -- __glListCompState: the software path, display-list compile */
__GLdispatchTable __glListCompState = { {
    (__GLdispatchFn)0x150,                              /* the entry count */
    (__GLdispatchFn)__glim_NewList,                     /* glNewList */
    (__GLdispatchFn)__glim_EndList,                     /* glEndList */
    (__GLdispatchFn)__gllc_CallList,                    /* glCallList */
    (__GLdispatchFn)__gllc_CallLists,                   /* glCallLists */
    (__GLdispatchFn)__glim_DeleteLists,                 /* glDeleteLists */
    (__GLdispatchFn)__glim_GenLists,                    /* glGenLists */
    (__GLdispatchFn)__gllc_ListBase,                    /* glListBase */
    (__GLdispatchFn)__gllc_Begin,                       /* glBegin */
    (__GLdispatchFn)__gllc_Bitmap,                      /* glBitmap */
    (__GLdispatchFn)__gllc_Color3b,                     /* glColor3b */
    (__GLdispatchFn)__gllc_Color3bv,                    /* glColor3bv */
    (__GLdispatchFn)__gllc_Color3d,                     /* glColor3d */
    (__GLdispatchFn)__gllc_Color3dv,                    /* glColor3dv */
    (__GLdispatchFn)__gllc_Color3f,                     /* glColor3f */
    (__GLdispatchFn)__gllc_Color3fv,                    /* glColor3fv */
    (__GLdispatchFn)__gllc_Color3i,                     /* glColor3i */
    (__GLdispatchFn)__gllc_Color3iv,                    /* glColor3iv */
    (__GLdispatchFn)__gllc_Color3s,                     /* glColor3s */
    (__GLdispatchFn)__gllc_Color3sv,                    /* glColor3sv */
    (__GLdispatchFn)__gllc_Color3ub,                    /* glColor3ub */
    (__GLdispatchFn)__gllc_Color3ubv,                   /* glColor3ubv */
    (__GLdispatchFn)__gllc_Color3ui,                    /* glColor3ui */
    (__GLdispatchFn)__gllc_Color3uiv,                   /* glColor3uiv */
    (__GLdispatchFn)__gllc_Color3us,                    /* glColor3us */
    (__GLdispatchFn)__gllc_Color3usv,                   /* glColor3usv */
    (__GLdispatchFn)__gllc_Color4b,                     /* glColor4b */
    (__GLdispatchFn)__gllc_Color4bv,                    /* glColor4bv */
    (__GLdispatchFn)__gllc_Color4d,                     /* glColor4d */
    (__GLdispatchFn)__gllc_Color4dv,                    /* glColor4dv */
    (__GLdispatchFn)__gllc_Color4f,                     /* glColor4f */
    (__GLdispatchFn)__gllc_Color4fv,                    /* glColor4fv */
    (__GLdispatchFn)__gllc_Color4i,                     /* glColor4i */
    (__GLdispatchFn)__gllc_Color4iv,                    /* glColor4iv */
    (__GLdispatchFn)__gllc_Color4s,                     /* glColor4s */
    (__GLdispatchFn)__gllc_Color4sv,                    /* glColor4sv */
    (__GLdispatchFn)__gllc_Color4ub,                    /* glColor4ub */
    (__GLdispatchFn)__gllc_Color4ubv,                   /* glColor4ubv */
    (__GLdispatchFn)__gllc_Color4ui,                    /* glColor4ui */
    (__GLdispatchFn)__gllc_Color4uiv,                   /* glColor4uiv */
    (__GLdispatchFn)__gllc_Color4us,                    /* glColor4us */
    (__GLdispatchFn)__gllc_Color4usv,                   /* glColor4usv */
    (__GLdispatchFn)__gllc_EdgeFlag,                    /* glEdgeFlag */
    (__GLdispatchFn)__gllc_EdgeFlagv,                   /* glEdgeFlagv */
    (__GLdispatchFn)__gllc_End,                         /* glEnd */
    (__GLdispatchFn)__gllc_Indexd,                      /* glIndexd */
    (__GLdispatchFn)__gllc_Indexdv,                     /* glIndexdv */
    (__GLdispatchFn)__gllc_Indexf,                      /* glIndexf */
    (__GLdispatchFn)__gllc_Indexfv,                     /* glIndexfv */
    (__GLdispatchFn)__gllc_Indexi,                      /* glIndexi */
    (__GLdispatchFn)__gllc_Indexiv,                     /* glIndexiv */
    (__GLdispatchFn)__gllc_Indexs,                      /* glIndexs */
    (__GLdispatchFn)__gllc_Indexsv,                     /* glIndexsv */
    (__GLdispatchFn)__gllc_Normal3b,                    /* glNormal3b */
    (__GLdispatchFn)__gllc_Normal3bv,                   /* glNormal3bv */
    (__GLdispatchFn)__gllc_Normal3d,                    /* glNormal3d */
    (__GLdispatchFn)__gllc_Normal3dv,                   /* glNormal3dv */
    (__GLdispatchFn)__gllc_Normal3f,                    /* glNormal3f */
    (__GLdispatchFn)__gllc_Normal3fv,                   /* glNormal3fv */
    (__GLdispatchFn)__gllc_Normal3i,                    /* glNormal3i */
    (__GLdispatchFn)__gllc_Normal3iv,                   /* glNormal3iv */
    (__GLdispatchFn)__gllc_Normal3s,                    /* glNormal3s */
    (__GLdispatchFn)__gllc_Normal3sv,                   /* glNormal3sv */
    (__GLdispatchFn)__gllc_RasterPos2d,                 /* glRasterPos2d */
    (__GLdispatchFn)__gllc_RasterPos2dv,                /* glRasterPos2dv */
    (__GLdispatchFn)__gllc_RasterPos2f,                 /* glRasterPos2f */
    (__GLdispatchFn)__gllc_RasterPos2fv,                /* glRasterPos2fv */
    (__GLdispatchFn)__gllc_RasterPos2i,                 /* glRasterPos2i */
    (__GLdispatchFn)__gllc_RasterPos2iv,                /* glRasterPos2iv */
    (__GLdispatchFn)__gllc_RasterPos2s,                 /* glRasterPos2s */
    (__GLdispatchFn)__gllc_RasterPos2sv,                /* glRasterPos2sv */
    (__GLdispatchFn)__gllc_RasterPos3d,                 /* glRasterPos3d */
    (__GLdispatchFn)__gllc_RasterPos3dv,                /* glRasterPos3dv */
    (__GLdispatchFn)__gllc_RasterPos3f,                 /* glRasterPos3f */
    (__GLdispatchFn)__gllc_RasterPos3fv,                /* glRasterPos3fv */
    (__GLdispatchFn)__gllc_RasterPos3i,                 /* glRasterPos3i */
    (__GLdispatchFn)__gllc_RasterPos3iv,                /* glRasterPos3iv */
    (__GLdispatchFn)__gllc_RasterPos3s,                 /* glRasterPos3s */
    (__GLdispatchFn)__gllc_RasterPos3sv,                /* glRasterPos3sv */
    (__GLdispatchFn)__gllc_RasterPos4d,                 /* glRasterPos4d */
    (__GLdispatchFn)__gllc_RasterPos4dv,                /* glRasterPos4dv */
    (__GLdispatchFn)__gllc_RasterPos4f,                 /* glRasterPos4f */
    (__GLdispatchFn)__gllc_RasterPos4fv,                /* glRasterPos4fv */
    (__GLdispatchFn)__gllc_RasterPos4i,                 /* glRasterPos4i */
    (__GLdispatchFn)__gllc_RasterPos4iv,                /* glRasterPos4iv */
    (__GLdispatchFn)__gllc_RasterPos4s,                 /* glRasterPos4s */
    (__GLdispatchFn)__gllc_RasterPos4sv,                /* glRasterPos4sv */
    (__GLdispatchFn)__gllc_Rectd,                       /* glRectd */
    (__GLdispatchFn)__gllc_Rectdv,                      /* glRectdv */
    (__GLdispatchFn)__gllc_Rectf,                       /* glRectf */
    (__GLdispatchFn)__gllc_Rectfv,                      /* glRectfv */
    (__GLdispatchFn)__gllc_Recti,                       /* glRecti */
    (__GLdispatchFn)__gllc_Rectiv,                      /* glRectiv */
    (__GLdispatchFn)__gllc_Rects,                       /* glRects */
    (__GLdispatchFn)__gllc_Rectsv,                      /* glRectsv */
    (__GLdispatchFn)__gllc_TexCoord1d,                  /* glTexCoord1d */
    (__GLdispatchFn)__gllc_TexCoord1dv,                 /* glTexCoord1dv */
    (__GLdispatchFn)__gllc_TexCoord1f,                  /* glTexCoord1f */
    (__GLdispatchFn)__gllc_TexCoord1fv,                 /* glTexCoord1fv */
    (__GLdispatchFn)__gllc_TexCoord1i,                  /* glTexCoord1i */
    (__GLdispatchFn)__gllc_TexCoord1iv,                 /* glTexCoord1iv */
    (__GLdispatchFn)__gllc_TexCoord1s,                  /* glTexCoord1s */
    (__GLdispatchFn)__gllc_TexCoord1sv,                 /* glTexCoord1sv */
    (__GLdispatchFn)__gllc_TexCoord2d,                  /* glTexCoord2d */
    (__GLdispatchFn)__gllc_TexCoord2dv,                 /* glTexCoord2dv */
    (__GLdispatchFn)__gllc_TexCoord2f,                  /* glTexCoord2f */
    (__GLdispatchFn)__gllc_TexCoord2fv,                 /* glTexCoord2fv */
    (__GLdispatchFn)__gllc_TexCoord2i,                  /* glTexCoord2i */
    (__GLdispatchFn)__gllc_TexCoord2iv,                 /* glTexCoord2iv */
    (__GLdispatchFn)__gllc_TexCoord2s,                  /* glTexCoord2s */
    (__GLdispatchFn)__gllc_TexCoord2sv,                 /* glTexCoord2sv */
    (__GLdispatchFn)__gllc_TexCoord3d,                  /* glTexCoord3d */
    (__GLdispatchFn)__gllc_TexCoord3dv,                 /* glTexCoord3dv */
    (__GLdispatchFn)__gllc_TexCoord3f,                  /* glTexCoord3f */
    (__GLdispatchFn)__gllc_TexCoord3fv,                 /* glTexCoord3fv */
    (__GLdispatchFn)__gllc_TexCoord3i,                  /* glTexCoord3i */
    (__GLdispatchFn)__gllc_TexCoord3iv,                 /* glTexCoord3iv */
    (__GLdispatchFn)__gllc_TexCoord3s,                  /* glTexCoord3s */
    (__GLdispatchFn)__gllc_TexCoord3sv,                 /* glTexCoord3sv */
    (__GLdispatchFn)__gllc_TexCoord4d,                  /* glTexCoord4d */
    (__GLdispatchFn)__gllc_TexCoord4dv,                 /* glTexCoord4dv */
    (__GLdispatchFn)__gllc_TexCoord4f,                  /* glTexCoord4f */
    (__GLdispatchFn)__gllc_TexCoord4fv,                 /* glTexCoord4fv */
    (__GLdispatchFn)__gllc_TexCoord4i,                  /* glTexCoord4i */
    (__GLdispatchFn)__gllc_TexCoord4iv,                 /* glTexCoord4iv */
    (__GLdispatchFn)__gllc_TexCoord4s,                  /* glTexCoord4s */
    (__GLdispatchFn)__gllc_TexCoord4sv,                 /* glTexCoord4sv */
    (__GLdispatchFn)__gllc_Vertex2d,                    /* glVertex2d */
    (__GLdispatchFn)__gllc_Vertex2dv,                   /* glVertex2dv */
    (__GLdispatchFn)__gllc_Vertex2f,                    /* glVertex2f */
    (__GLdispatchFn)__gllc_Vertex2fv,                   /* glVertex2fv */
    (__GLdispatchFn)__gllc_Vertex2i,                    /* glVertex2i */
    (__GLdispatchFn)__gllc_Vertex2iv,                   /* glVertex2iv */
    (__GLdispatchFn)__gllc_Vertex2s,                    /* glVertex2s */
    (__GLdispatchFn)__gllc_Vertex2sv,                   /* glVertex2sv */
    (__GLdispatchFn)__gllc_Vertex3d,                    /* glVertex3d */
    (__GLdispatchFn)__gllc_Vertex3dv,                   /* glVertex3dv */
    (__GLdispatchFn)__gllc_Vertex3f,                    /* glVertex3f */
    (__GLdispatchFn)__gllc_Vertex3fv,                   /* glVertex3fv */
    (__GLdispatchFn)__gllc_Vertex3i,                    /* glVertex3i */
    (__GLdispatchFn)__gllc_Vertex3iv,                   /* glVertex3iv */
    (__GLdispatchFn)__gllc_Vertex3s,                    /* glVertex3s */
    (__GLdispatchFn)__gllc_Vertex3sv,                   /* glVertex3sv */
    (__GLdispatchFn)__gllc_Vertex4d,                    /* glVertex4d */
    (__GLdispatchFn)__gllc_Vertex4dv,                   /* glVertex4dv */
    (__GLdispatchFn)__gllc_Vertex4f,                    /* glVertex4f */
    (__GLdispatchFn)__gllc_Vertex4fv,                   /* glVertex4fv */
    (__GLdispatchFn)__gllc_Vertex4i,                    /* glVertex4i */
    (__GLdispatchFn)__gllc_Vertex4iv,                   /* glVertex4iv */
    (__GLdispatchFn)__gllc_Vertex4s,                    /* glVertex4s */
    (__GLdispatchFn)__gllc_Vertex4sv,                   /* glVertex4sv */
    (__GLdispatchFn)__gllc_ClipPlane,                   /* glClipPlane */
    (__GLdispatchFn)__gllc_ColorMaterial,               /* glColorMaterial */
    (__GLdispatchFn)__gllc_CullFace,                    /* glCullFace */
    (__GLdispatchFn)__gllc_Fogf,                        /* glFogf */
    (__GLdispatchFn)__gllc_Fogfv,                       /* glFogfv */
    (__GLdispatchFn)__gllc_Fogi,                        /* glFogi */
    (__GLdispatchFn)__gllc_Fogiv,                       /* glFogiv */
    (__GLdispatchFn)__gllc_FrontFace,                   /* glFrontFace */
    (__GLdispatchFn)__gllc_Hint,                        /* glHint */
    (__GLdispatchFn)__gllc_Lightf,                      /* glLightf */
    (__GLdispatchFn)__gllc_Lightfv,                     /* glLightfv */
    (__GLdispatchFn)__gllc_Lighti,                      /* glLighti */
    (__GLdispatchFn)__gllc_Lightiv,                     /* glLightiv */
    (__GLdispatchFn)__gllc_LightModelf,                 /* glLightModelf */
    (__GLdispatchFn)__gllc_LightModelfv,                /* glLightModelfv */
    (__GLdispatchFn)__gllc_LightModeli,                 /* glLightModeli */
    (__GLdispatchFn)__gllc_LightModeliv,                /* glLightModeliv */
    (__GLdispatchFn)__gllc_LineStipple,                 /* glLineStipple */
    (__GLdispatchFn)__gllc_LineWidth,                   /* glLineWidth */
    (__GLdispatchFn)__gllc_Materialf,                   /* glMaterialf */
    (__GLdispatchFn)__gllc_Materialfv,                  /* glMaterialfv */
    (__GLdispatchFn)__gllc_Materiali,                   /* glMateriali */
    (__GLdispatchFn)__gllc_Materialiv,                  /* glMaterialiv */
    (__GLdispatchFn)__gllc_PointSize,                   /* glPointSize */
    (__GLdispatchFn)__gllc_PolygonMode,                 /* glPolygonMode */
    (__GLdispatchFn)__gllc_PolygonStipple,              /* glPolygonStipple */
    (__GLdispatchFn)__gllc_Scissor,                     /* glScissor */
    (__GLdispatchFn)__gllc_ShadeModel,                  /* glShadeModel */
    (__GLdispatchFn)__gllc_TexParameterf,               /* glTexParameterf */
    (__GLdispatchFn)__gllc_TexParameterfv,              /* glTexParameterfv */
    (__GLdispatchFn)__gllc_TexParameteri,               /* glTexParameteri */
    (__GLdispatchFn)__gllc_TexParameteriv,              /* glTexParameteriv */
    (__GLdispatchFn)__gllc_TexImage1D,                  /* glTexImage1D */
    (__GLdispatchFn)__gllc_TexImage2D,                  /* glTexImage2D */
    (__GLdispatchFn)__gllc_TexEnvf,                     /* glTexEnvf */
    (__GLdispatchFn)__gllc_TexEnvfv,                    /* glTexEnvfv */
    (__GLdispatchFn)__gllc_TexEnvi,                     /* glTexEnvi */
    (__GLdispatchFn)__gllc_TexEnviv,                    /* glTexEnviv */
    (__GLdispatchFn)__gllc_TexGend,                     /* glTexGend */
    (__GLdispatchFn)__gllc_TexGendv,                    /* glTexGendv */
    (__GLdispatchFn)__gllc_TexGenf,                     /* glTexGenf */
    (__GLdispatchFn)__gllc_TexGenfv,                    /* glTexGenfv */
    (__GLdispatchFn)__gllc_TexGeni,                     /* glTexGeni */
    (__GLdispatchFn)__gllc_TexGeniv,                    /* glTexGeniv */
    (__GLdispatchFn)__glim_FeedbackBuffer,                       /* glFeedbackBuffer */
    (__GLdispatchFn)__glim_SelectBuffer,                       /* glSelectBuffer */
    (__GLdispatchFn)__glim_RenderMode,                  /* glRenderMode */
    (__GLdispatchFn)__gllc_InitNames,                   /* glInitNames */
    (__GLdispatchFn)__gllc_LoadName,                    /* glLoadName */
    (__GLdispatchFn)__gllc_PassThrough,                 /* glPassThrough */
    (__GLdispatchFn)__gllc_PopName,                     /* glPopName */
    (__GLdispatchFn)__gllc_PushName,                    /* glPushName */
    (__GLdispatchFn)__gllc_DrawBuffer,                  /* glDrawBuffer */
    (__GLdispatchFn)__gllc_Clear,                       /* glClear */
    (__GLdispatchFn)__gllc_ClearAccum,                  /* glClearAccum */
    (__GLdispatchFn)__gllc_ClearIndex,                  /* glClearIndex */
    (__GLdispatchFn)__gllc_ClearColor,                  /* glClearColor */
    (__GLdispatchFn)__gllc_ClearStencil,                /* glClearStencil */
    (__GLdispatchFn)__gllc_ClearDepth,                  /* glClearDepth */
    (__GLdispatchFn)__gllc_StencilMask,                 /* glStencilMask */
    (__GLdispatchFn)__gllc_ColorMask,                   /* glColorMask */
    (__GLdispatchFn)__gllc_DepthMask,                   /* glDepthMask */
    (__GLdispatchFn)__gllc_IndexMask,                   /* glIndexMask */
    (__GLdispatchFn)__gllc_Accum,                       /* glAccum */
    (__GLdispatchFn)__gllc_Disable,                     /* glDisable */
    (__GLdispatchFn)__gllc_Enable,                      /* glEnable */
    (__GLdispatchFn)finishReturnAddrThunk,              /* glFinish */
    (__GLdispatchFn)__glim_Flush,                       /* glFlush */
    (__GLdispatchFn)__gllc_PopAttrib,                   /* glPopAttrib */
    (__GLdispatchFn)__gllc_PushAttrib,                  /* glPushAttrib */
    (__GLdispatchFn)__gllc_Map1d,                       /* glMap1d */
    (__GLdispatchFn)__gllc_Map1f,                       /* glMap1f */
    (__GLdispatchFn)__gllc_Map2d,                       /* glMap2d */
    (__GLdispatchFn)__gllc_Map2f,                       /* glMap2f */
    (__GLdispatchFn)__gllc_MapGrid1d,                   /* glMapGrid1d */
    (__GLdispatchFn)__gllc_MapGrid1f,                   /* glMapGrid1f */
    (__GLdispatchFn)__gllc_MapGrid2d,                   /* glMapGrid2d */
    (__GLdispatchFn)__gllc_MapGrid2f,                   /* glMapGrid2f */
    (__GLdispatchFn)__gllc_EvalCoord1d,                 /* glEvalCoord1d */
    (__GLdispatchFn)__gllc_EvalCoord1dv,                /* glEvalCoord1dv */
    (__GLdispatchFn)__gllc_EvalCoord1f,                 /* glEvalCoord1f */
    (__GLdispatchFn)__gllc_EvalCoord1fv,                /* glEvalCoord1fv */
    (__GLdispatchFn)__gllc_EvalCoord2d,                 /* glEvalCoord2d */
    (__GLdispatchFn)__gllc_EvalCoord2dv,                /* glEvalCoord2dv */
    (__GLdispatchFn)__gllc_EvalCoord2f,                 /* glEvalCoord2f */
    (__GLdispatchFn)__gllc_EvalCoord2fv,                /* glEvalCoord2fv */
    (__GLdispatchFn)__gllc_EvalMesh1,                   /* glEvalMesh1 */
    (__GLdispatchFn)__gllc_EvalPoint1,                  /* glEvalPoint1 */
    (__GLdispatchFn)__gllc_EvalMesh2,                   /* glEvalMesh2 */
    (__GLdispatchFn)__gllc_EvalPoint2,                  /* glEvalPoint2 */
    (__GLdispatchFn)__gllc_AlphaFunc,                   /* glAlphaFunc */
    (__GLdispatchFn)__gllc_BlendFunc,                   /* glBlendFunc */
    (__GLdispatchFn)__gllc_LogicOp,                     /* glLogicOp */
    (__GLdispatchFn)__gllc_StencilFunc,                 /* glStencilFunc */
    (__GLdispatchFn)__gllc_StencilOp,                   /* glStencilOp */
    (__GLdispatchFn)__gllc_DepthFunc,                   /* glDepthFunc */
    (__GLdispatchFn)__gllc_PixelZoom,                   /* glPixelZoom */
    (__GLdispatchFn)__gllc_PixelTransferf,              /* glPixelTransferf */
    (__GLdispatchFn)__gllc_PixelTransferi,              /* glPixelTransferi */
    (__GLdispatchFn)__glim_PixelStoref,                 /* glPixelStoref */
    (__GLdispatchFn)__glim_PixelStorei,                 /* glPixelStorei */
    (__GLdispatchFn)__gllc_PixelMapfv,                  /* glPixelMapfv */
    (__GLdispatchFn)__gllc_PixelMapuiv,                 /* glPixelMapuiv */
    (__GLdispatchFn)__gllc_PixelMapusv,                 /* glPixelMapusv */
    (__GLdispatchFn)__gllc_ReadBuffer,                  /* glReadBuffer */
    (__GLdispatchFn)__gllc_CopyPixels,                  /* glCopyPixels */
    (__GLdispatchFn)__glim_ReadPixels,                       /* glReadPixels */
    (__GLdispatchFn)__gllc_DrawPixels,                  /* glDrawPixels */
    (__GLdispatchFn)__glim_GetBooleanv,                 /* glGetBooleanv */
    (__GLdispatchFn)__glim_GetClipPlane,                       /* glGetClipPlane */
    (__GLdispatchFn)__glim_GetDoublev,                  /* glGetDoublev */
    (__GLdispatchFn)__glim_GetError,                    /* glGetError */
    (__GLdispatchFn)__glim_GetFloatv,                   /* glGetFloatv */
    (__GLdispatchFn)__glim_GetIntegerv,                 /* glGetIntegerv */
    (__GLdispatchFn)__glim_GetLightfv,                  /* glGetLightfv */
    (__GLdispatchFn)__glim_GetLightiv,                  /* glGetLightiv */
    (__GLdispatchFn)__glim_GetMapdv,                    /* glGetMapdv */
    (__GLdispatchFn)__glim_GetMapfv,                    /* glGetMapfv */
    (__GLdispatchFn)__glim_GetMapiv,                    /* glGetMapiv */
    (__GLdispatchFn)__glim_GetMaterialfv,               /* glGetMaterialfv */
    (__GLdispatchFn)__glim_GetMaterialiv,               /* glGetMaterialiv */
    (__GLdispatchFn)__glim_GetPixelMapfv,               /* glGetPixelMapfv */
    (__GLdispatchFn)__glim_GetPixelMapuiv,              /* glGetPixelMapuiv */
    (__GLdispatchFn)__glim_GetPixelMapusv,              /* glGetPixelMapusv */
    (__GLdispatchFn)__glim_GetPolygonStipple,           /* glGetPolygonStipple */
    (__GLdispatchFn)__glim_GetString,                       /* glGetString */
    (__GLdispatchFn)__glim_GetTexEnvfv,                 /* glGetTexEnvfv */
    (__GLdispatchFn)__glim_GetTexEnviv,                 /* glGetTexEnviv */
    (__GLdispatchFn)__glim_GetTexGendv,                       /* glGetTexGendv */
    (__GLdispatchFn)__glim_GetTexGenfv,                       /* glGetTexGenfv */
    (__GLdispatchFn)__glim_GetTexGeniv,                 /* glGetTexGeniv */
    (__GLdispatchFn)__glim_GetTexImage,                 /* glGetTexImage */
    (__GLdispatchFn)__glim_GetTexParameterfv,           /* glGetTexParameterfv */
    (__GLdispatchFn)__glim_GetTexParameteriv,           /* glGetTexParameteriv */
    (__GLdispatchFn)__glim_GetTexLevelParameterfv,      /* glGetTexLevelParameterfv */
    (__GLdispatchFn)__glim_GetTexLevelParameteriv,      /* glGetTexLevelParameteriv */
    (__GLdispatchFn)__glim_IsEnabled,                   /* glIsEnabled */
    (__GLdispatchFn)__glim_IsList,                      /* glIsList */
    (__GLdispatchFn)__gllc_DepthRange,                  /* glDepthRange */
    (__GLdispatchFn)__gllc_Frustum,                     /* glFrustum */
    (__GLdispatchFn)__gllc_LoadIdentity,                /* glLoadIdentity */
    (__GLdispatchFn)__gllc_LoadMatrixf,                 /* glLoadMatrixf */
    (__GLdispatchFn)__gllc_LoadMatrixd,                 /* glLoadMatrixd */
    (__GLdispatchFn)__gllc_MatrixMode,                  /* glMatrixMode */
    (__GLdispatchFn)__gllc_MultMatrixf,                 /* glMultMatrixf */
    (__GLdispatchFn)__gllc_MultMatrixd,                 /* glMultMatrixd */
    (__GLdispatchFn)__gllc_Ortho,                       /* glOrtho */
    (__GLdispatchFn)__gllc_PopMatrix,                   /* glPopMatrix */
    (__GLdispatchFn)__gllc_PushMatrix,                  /* glPushMatrix */
    (__GLdispatchFn)__gllc_Rotated,                     /* glRotated */
    (__GLdispatchFn)__gllc_Rotatef,                     /* glRotatef */
    (__GLdispatchFn)__gllc_Scaled,                      /* glScaled */
    (__GLdispatchFn)__gllc_Scalef,                      /* glScalef */
    (__GLdispatchFn)__gllc_Translated,                  /* glTranslated */
    (__GLdispatchFn)__gllc_Translatef,                  /* glTranslatef */
    (__GLdispatchFn)__gllc_Viewport,                    /* glViewport */
    (__GLdispatchFn)__gllc_ArrayElement,                /* glArrayElement */
    (__GLdispatchFn)__gllc_BindTexture,                 /* glBindTexture */
    (__GLdispatchFn)__glim_ColorPointer,                /* glColorPointer */
    (__GLdispatchFn)__glim_DisableClientState,          /* glDisableClientState */
    (__GLdispatchFn)__gllc_DrawArrays,                  /* glDrawArrays */
    (__GLdispatchFn)__gllc_DrawElements,                /* glDrawElements */
    (__GLdispatchFn)__glim_EdgeFlagPointer,             /* glEdgeFlagPointer */
    (__GLdispatchFn)__glim_EnableClientState,           /* glEnableClientState */
    (__GLdispatchFn)__glim_IndexPointer,                /* glIndexPointer */
    (__GLdispatchFn)__gllc_Indexub,                     /* glIndexub */
    (__GLdispatchFn)__gllc_Indexubv,                    /* glIndexubv */
    (__GLdispatchFn)__glim_InterleavedArrays,           /* glInterleavedArrays */
    (__GLdispatchFn)__glim_NormalPointer,               /* glNormalPointer */
    (__GLdispatchFn)__gllc_PolygonOffset,               /* glPolygonOffset */
    (__GLdispatchFn)__glim_TexCoordPointer,             /* glTexCoordPointer */
    (__GLdispatchFn)__glim_VertexPointer,               /* glVertexPointer */
    (__GLdispatchFn)__glim_AreTexturesResident,         /* glAreTexturesResident */
    (__GLdispatchFn)__gllc_CopyTexImage1D,              /* glCopyTexImage1D */
    (__GLdispatchFn)__gllc_CopyTexImage2D,              /* glCopyTexImage2D */
    (__GLdispatchFn)__gllc_CopyTexSubImage1D,           /* glCopyTexSubImage1D */
    (__GLdispatchFn)__gllc_CopyTexSubImage2D,           /* glCopyTexSubImage2D */
    (__GLdispatchFn)__glim_DeleteTextures,              /* glDeleteTextures */
    (__GLdispatchFn)__glim_GenTextures,                 /* glGenTextures */
    (__GLdispatchFn)__glim_GetPointerv,                 /* glGetPointerv */
    (__GLdispatchFn)__glim_IsTexture,                       /* glIsTexture */
    (__GLdispatchFn)__gllc_PrioritizeTextures,          /* glPrioritizeTextures */
    (__GLdispatchFn)__gllc_TexSubImage1D,               /* glTexSubImage1D */
    (__GLdispatchFn)__gllc_TexSubImage2D,               /* glTexSubImage2D */
    (__GLdispatchFn)__glim_PopClientAttrib,             /* glPopClientAttrib */
    (__GLdispatchFn)__glim_PushClientAttrib,            /* glPushClientAttrib */
    (__GLdispatchFn)__gllc_ArrayElementEXT,             /* extension slot 0x151 */
    (__GLdispatchFn)__glim_ColorPointerEXT,             /* extension slot 0x152 */
    (__GLdispatchFn)__gllc_DrawArraysEXT,               /* extension slot 0x153 */
    (__GLdispatchFn)__glim_EdgeFlagPointerEXT,          /* extension slot 0x154 */
    (__GLdispatchFn)__glim_GetPointervEXT,              /* extension slot 0x155 */
    (__GLdispatchFn)__glim_IndexPointerEXT,             /* extension slot 0x156 */
    (__GLdispatchFn)__glim_NormalPointerEXT,            /* extension slot 0x157 */
    (__GLdispatchFn)__glim_TexCoordPointerEXT,          /* extension slot 0x158 */
    (__GLdispatchFn)__glim_VertexPointerEXT,            /* extension slot 0x159 */
    (__GLdispatchFn)__gllc_ColorSubTable,               /* extension slot 0x15a */
    (__GLdispatchFn)__gllc_ColorTable,                  /* extension slot 0x15b */
    (__GLdispatchFn)__gllc_CopyColorTableEXT,           /* extension slot 0x15c */
    (__GLdispatchFn)__glim_GetColorTable,               /* extension slot 0x15d */
    (__GLdispatchFn)__glim_GetColorTableParameterfvEXT, /* extension slot 0x15e */
    (__GLdispatchFn)__glim_GetColorTableParameterivEXT, /* extension slot 0x15f */
    (__GLdispatchFn)__glim_LockArraysEXT,               /* extension slot 0x160 */
    (__GLdispatchFn)__glim_UnlockArraysEXT,             /* extension slot 0x161 */
    (__GLdispatchFn)glCullParameterdvEXT,               /* extension slot 0x162 */
    (__GLdispatchFn)__glim_CullParameterfvEXT,          /* extension slot 0x163 */
    (__GLdispatchFn)__gllc_IndexFuncSGI,                /* extension slot 0x164 */
    (__GLdispatchFn)__gllc_IndexMaterialSGI,            /* extension slot 0x165 */
    (__GLdispatchFn)__glim_AddSwapHintRectWIN,          /* extension slot 0x166 */
} };   /* slots 0x167 .. 0x199: 0 */

/* 690f4010 -- __glListExecTable: the display-list execute procs by opcode (gc->dlist.baseListExec) */
void *__glListExecTable[256] = {
    __glle_CallList                                 ,/* 0x00 */
    __glle_CallLists                                ,/* 0x01 */
    __glle_ListBase                                 ,/* 0x02 */
    NULL                                            ,/* 0x03 */
    __glle_Bitmap                                   ,/* 0x04 */
    __glle_Color3bv                                 ,/* 0x05 */
    __glle_Color3dv                                 ,/* 0x06 */
    __glle_Color3fv                                 ,/* 0x07 */
    __glle_Color3iv                                 ,/* 0x08 */
    __glle_Color3sv                                 ,/* 0x09 */
    __glle_Color3ubv                                ,/* 0x0a */
    __glle_Color3uiv                                ,/* 0x0b */
    __glle_Color3usv                                ,/* 0x0c */
    __glle_Color4bv                                 ,/* 0x0d */
    __glle_Color4dv                                 ,/* 0x0e */
    __glle_Color4fv                                 ,/* 0x0f */
    __glle_Color4iv                                 ,/* 0x10 */
    __glle_Color4sv                                 ,/* 0x11 */
    __glle_Color4ubv                                ,/* 0x12 */
    __glle_Color4uiv                                ,/* 0x13 */
    __glle_Color4usv                                ,/* 0x14 */
    __glle_EdgeFlagv                                ,/* 0x15 */
    __glle_End                                      ,/* 0x16 */
    __glle_Indexdv                                  ,/* 0x17 */
    __glle_Indexfv                                  ,/* 0x18 */
    __glle_Indexiv                                  ,/* 0x19 */
    __glle_Indexsv                                  ,/* 0x1a */
    __glle_Normal3bv                                ,/* 0x1b */
    __glle_Normal3dv                                ,/* 0x1c */
    __glle_Normal3fv                                ,/* 0x1d */
    __glle_Normal3iv                                ,/* 0x1e */
    __glle_Normal3sv                                ,/* 0x1f */
    __glle_RasterPos2dv                             ,/* 0x20 */
    __glle_RasterPos2fv                             ,/* 0x21 */
    __glle_RasterPos2iv                             ,/* 0x22 */
    __glle_RasterPos2sv                             ,/* 0x23 */
    __glle_RasterPos3dv                             ,/* 0x24 */
    __glle_RasterPos3fv                             ,/* 0x25 */
    __glle_RasterPos3iv                             ,/* 0x26 */
    __glle_RasterPos3sv                             ,/* 0x27 */
    __glle_RasterPos4dv                             ,/* 0x28 */
    __glle_RasterPos4fv                             ,/* 0x29 */
    __glle_RasterPos4iv                             ,/* 0x2a */
    __glle_RasterPos4sv                             ,/* 0x2b */
    __glle_Rectdv                                   ,/* 0x2c */
    __glle_Rectfv                                   ,/* 0x2d */
    __glle_Rectiv                                   ,/* 0x2e */
    __glle_Rectsv                                   ,/* 0x2f */
    __glle_TexCoord1dv                              ,/* 0x30 */
    __glle_TexCoord1fv                              ,/* 0x31 */
    __glle_TexCoord1iv                              ,/* 0x32 */
    __glle_TexCoord1sv                              ,/* 0x33 */
    __glle_TexCoord2dv                              ,/* 0x34 */
    __glle_TexCoord2fv                              ,/* 0x35 */
    __glle_TexCoord2iv                              ,/* 0x36 */
    __glle_TexCoord2sv                              ,/* 0x37 */
    __glle_TexCoord3dv                              ,/* 0x38 */
    __glle_TexCoord3fv                              ,/* 0x39 */
    __glle_TexCoord3iv                              ,/* 0x3a */
    __glle_TexCoord3sv                              ,/* 0x3b */
    __glle_TexCoord4dv                              ,/* 0x3c */
    __glle_TexCoord4fv                              ,/* 0x3d */
    __glle_TexCoord4iv                              ,/* 0x3e */
    __glle_TexCoord4sv                              ,/* 0x3f */
    __glle_Vertex2dv                                ,/* 0x40 */
    __glle_Vertex2fv                                ,/* 0x41 */
    __glle_Vertex2iv                                ,/* 0x42 */
    __glle_Vertex2sv                                ,/* 0x43 */
    __glle_Vertex3dv                                ,/* 0x44 */
    __glle_Vertex3fv                                ,/* 0x45 */
    __glle_Vertex3iv                                ,/* 0x46 */
    __glle_Vertex3sv                                ,/* 0x47 */
    __glle_Vertex4dv                                ,/* 0x48 */
    __glle_Vertex4fv                                ,/* 0x49 */
    __glle_Vertex4iv                                ,/* 0x4a */
    __glle_Vertex4sv                                ,/* 0x4b */
    __glle_ClipPlane                                ,/* 0x4c */
    __glle_ColorMaterial                            ,/* 0x4d */
    __glle_CullFace                                 ,/* 0x4e */
    __glle_Fogf                                     ,/* 0x4f */
    __glle_Fogfv                                    ,/* 0x50 */
    __glle_Fogi                                     ,/* 0x51 */
    __glle_Fogiv                                    ,/* 0x52 */
    __glle_FrontFace                                ,/* 0x53 */
    __glle_Hint                                     ,/* 0x54 */
    __glle_Lightfv                                  ,/* 0x55 */
    __glle_Lightfv                                  ,/* 0x56 */
    __glle_Lightiv                                  ,/* 0x57 */
    __glle_Lightiv                                  ,/* 0x58 */
    __glle_LightModelfv                             ,/* 0x59 */
    __glle_LightModelfv                             ,/* 0x5a */
    __glle_LightModeliv                             ,/* 0x5b */
    __glle_LightModeliv                             ,/* 0x5c */
    __glle_LineStipple                              ,/* 0x5d */
    __glle_LineWidth                                ,/* 0x5e */
    __glle_Materialfv                               ,/* 0x5f */
    __glle_Materialfv                               ,/* 0x60 */
    __glle_Materialiv                               ,/* 0x61 */
    __glle_Materialiv                               ,/* 0x62 */
    __glle_PointSize                                ,/* 0x63 */
    __glle_PolygonMode                              ,/* 0x64 */
    __glle_PolygonStipple                           ,/* 0x65 */
    __glle_Scissor                                  ,/* 0x66 */
    __glle_ShadeModel                               ,/* 0x67 */
    __glle_TexParameterfv                           ,/* 0x68 */
    __glle_TexParameterfv                           ,/* 0x69 */
    __glle_TexParameteriv                           ,/* 0x6a */
    __glle_TexParameteriv                           ,/* 0x6b */
    __glle_TexImage1D                               ,/* 0x6c */
    __glle_TexImage2D                               ,/* 0x6d */
    __glle_TexEnvfv                                 ,/* 0x6e */
    __glle_TexEnvfv                                 ,/* 0x6f */
    __glle_TexEnviv                                 ,/* 0x70 */
    __glle_TexEnviv                                 ,/* 0x71 */
    __glle_TexGendv                                 ,/* 0x72 */
    __glle_TexGendv                                 ,/* 0x73 */
    __glle_TexGenfv                                 ,/* 0x74 */
    __glle_TexGenfv                                 ,/* 0x75 */
    __glle_TexGeniv                                 ,/* 0x76 */
    __glle_TexGeniv                                 ,/* 0x77 */
    __glle_InitNames                                ,/* 0x78 */
    __glle_LoadName                                 ,/* 0x79 */
    __glle_PassThrough                              ,/* 0x7a */
    __glle_PopName                                  ,/* 0x7b */
    __glle_PushName                                 ,/* 0x7c */
    __glle_DrawBuffer                               ,/* 0x7d */
    __glle_Clear                                    ,/* 0x7e */
    __glle_ClearAccum                               ,/* 0x7f */
    __glle_ClearIndex                               ,/* 0x80 */
    __glle_ClearColor                               ,/* 0x81 */
    __glle_ClearStencil                             ,/* 0x82 */
    __glle_ClearDepth                               ,/* 0x83 */
    __glle_StencilMask                              ,/* 0x84 */
    __glle_ColorMask                                ,/* 0x85 */
    __glle_DepthMask                                ,/* 0x86 */
    __glle_IndexMask                                ,/* 0x87 */
    __glle_Accum                                    ,/* 0x88 */
    __glle_Disable                                  ,/* 0x89 */
    __glle_Enable                                   ,/* 0x8a */
    __glle_PopAttrib                                ,/* 0x8b */
    __glle_PushAttrib                               ,/* 0x8c */
    __glle_Map1                                     ,/* 0x8d */
    __glle_Map1                                     ,/* 0x8e */
    __glle_Map2                                     ,/* 0x8f */
    __glle_Map2                                     ,/* 0x90 */
    __glle_MapGrid1d                                ,/* 0x91 */
    __glle_MapGrid1f                                ,/* 0x92 */
    __glle_MapGrid2d                                ,/* 0x93 */
    __glle_MapGrid2f                                ,/* 0x94 */
    __glle_EvalCoord1dv                             ,/* 0x95 */
    __glle_EvalCoord1fv                             ,/* 0x96 */
    __glle_EvalCoord2dv                             ,/* 0x97 */
    __glle_EvalCoord2fv                             ,/* 0x98 */
    __glle_EvalMesh1                                ,/* 0x99 */
    __glle_EvalPoint1                               ,/* 0x9a */
    __glle_EvalMesh2                                ,/* 0x9b */
    __glle_EvalPoint2                               ,/* 0x9c */
    __glle_AlphaFunc                                ,/* 0x9d */
    __glle_BlendFunc                                ,/* 0x9e */
    __glle_LogicOp                                  ,/* 0x9f */
    __glle_StencilFunc                              ,/* 0xa0 */
    __glle_StencilOp                                ,/* 0xa1 */
    __glle_DepthFunc                                ,/* 0xa2 */
    __glle_PixelZoom                                ,/* 0xa3 */
    __glle_PixelTransferf                           ,/* 0xa4 */
    __glle_PixelTransferi                           ,/* 0xa5 */
    __glle_PixelMapfv                               ,/* 0xa6 */
    __glle_PixelMapuiv                              ,/* 0xa7 */
    __glle_PixelMapusv                              ,/* 0xa8 */
    __glle_ReadBuffer                               ,/* 0xa9 */
    __glle_CopyPixels                               ,/* 0xaa */
    __glle_DrawPixels                               ,/* 0xab */
    __glle_DepthRange                               ,/* 0xac */
    __glle_Frustum                                  ,/* 0xad */
    __glle_LoadIdentity                             ,/* 0xae */
    __glle_LoadMatrixf                              ,/* 0xaf */
    __glle_LoadMatrixd                              ,/* 0xb0 */
    __glle_MatrixMode                               ,/* 0xb1 */
    __glle_MultMatrixf                              ,/* 0xb2 */
    __glle_MultMatrixd                              ,/* 0xb3 */
    __glle_Ortho                                    ,/* 0xb4 */
    __glle_PopMatrix                                ,/* 0xb5 */
    __glle_PushMatrix                               ,/* 0xb6 */
    __glle_Rotated                                  ,/* 0xb7 */
    __glle_Rotatef                                  ,/* 0xb8 */
    __glle_Scaled                                   ,/* 0xb9 */
    __glle_Scalef                                   ,/* 0xba */
    __glle_Translated                               ,/* 0xbb */
    __glle_Translatef                               ,/* 0xbc */
    __glle_Viewport                                 ,/* 0xbd */
    __glle_ArrayElementEXT                                    ,/* 0xbe */
    __glle_DrawArraysEXT                                    ,/* 0xbf */
    __glle_ColorSubTable                            ,/* 0xc0 */
    __glle_ColorTable                               ,/* 0xc1 */
    __glle_CopyColorTableEXT                        ,/* 0xc2 */
    __glle_DrawRangeElements                                    ,/* 0xc3 */
    __glle_DrawArrays                                    ,/* 0xc4 */
    __glle_DrawElements                                    ,/* 0xc5 */
    __glle_PolygonOffset                            ,/* 0xc6 */
    __glle_CopyTexImage1D                           ,/* 0xc7 */
    __glle_CopyTexImage2D                           ,/* 0xc8 */
    __glle_CopyTexSubImage1D                        ,/* 0xc9 */
    __glle_CopyTexSubImage2D                        ,/* 0xca */
    __glle_TexSubImage1D                            ,/* 0xcb */
    __glle_TexSubImage2D                            ,/* 0xcc */
    __glle_BindTexture                              ,/* 0xcd */
    __glle_PrioritizeTextures                       ,/* 0xce */
    __glle_Indexubv                                 ,/* 0xcf */
    __glle_IndexFuncSGI                             ,/* 0xd0 */
    __glle_IndexMaterialSGI                         ,/* 0xd1 */
    __glle_ActiveTextureARB                         ,/* 0xd2 */
    __glle_ClientActiveTextureARB                   ,/* 0xd3 */
    __glle_MultiTexCoord1dARB                       ,/* 0xd4 */
    __glle_MultiTexCoord1dvARB                      ,/* 0xd5 */
    __glle_MultiTexCoord1fARB                       ,/* 0xd6 */
    __glle_MultiTexCoord1fvARB                      ,/* 0xd7 */
    __glle_MultiTexCoord1iARB                       ,/* 0xd8 */
    __glle_MultiTexCoord1ivARB                      ,/* 0xd9 */
    __glle_MultiTexCoord1sARB                       ,/* 0xda */
    __glle_MultiTexCoord1svARB                      ,/* 0xdb */
    __glle_MultiTexCoord2dARB                       ,/* 0xdc */
    __glle_MultiTexCoord2dvARB                      ,/* 0xdd */
    __glle_MultiTexCoord2fARB                       ,/* 0xde */
    __glle_MultiTexCoord2fvARB                      ,/* 0xdf */
    __glle_MultiTexCoord2iARB                       ,/* 0xe0 */
    __glle_MultiTexCoord2ivARB                      ,/* 0xe1 */
    __glle_MultiTexCoord2sARB                       ,/* 0xe2 */
    __glle_MultiTexCoord2svARB                      ,/* 0xe3 */
    __glle_MultiTexCoord3dARB                       ,/* 0xe4 */
    __glle_MultiTexCoord3dvARB                      ,/* 0xe5 */
    __glle_MultiTexCoord3fARB                       ,/* 0xe6 */
    __glle_MultiTexCoord3fvARB                      ,/* 0xe7 */
    __glle_MultiTexCoord3iARB                       ,/* 0xe8 */
    __glle_MultiTexCoord3ivARB                      ,/* 0xe9 */
    __glle_MultiTexCoord3sARB                       ,/* 0xea */
    __glle_MultiTexCoord3svARB                      ,/* 0xeb */
    __glle_MultiTexCoord4dARB                       ,/* 0xec */
    __glle_MultiTexCoord4dvARB                      ,/* 0xed */
    __glle_MultiTexCoord4fARB                       ,/* 0xee */
    __glle_MultiTexCoord4fvARB                      ,/* 0xef */
    __glle_MultiTexCoord4iARB                       ,/* 0xf0 */
    __glle_MultiTexCoord4ivARB                      ,/* 0xf1 */
    __glle_MultiTexCoord4sARB                       ,/* 0xf2 */
    __glle_MultiTexCoord4svARB                      ,/* 0xf3 */
    __glle_PointParameterfEXT                       ,/* 0xf4 */
    __glle_PointParameterfvEXT                      ,/* 0xf5 */
    __glle_BlendEquationEXT                         ,/* 0xf6 */
    __glle_CompressedTexImage1DARB                  ,/* 0xf7 */
    __glle_CompressedTexImage2DARB                  ,/* 0xf8 */
    __glle_CompressedTexSubImage1DARB               ,/* 0xf9 */
    __glle_CompressedTexSubImage2DARB               ,/* 0xfa */
    __glle_TBufferMask3DFX                          ,/* 0xfb */
    __glle_FogCoordfEXT                             ,/* 0xfc */
    __glle_FogCoorddEXT                             ,/* 0xfd */
    __glle_FogCoordfvEXT                            ,/* 0xfe */
    __glle_FogCoorddvEXT                            ,/* 0xff */
};

/* 690f4410 -- __gl_GenericDlOps: gc->dlist.listExec */
void *__gl_GenericDlOps[16] = {
    __glle_Begin_LineLoop                           ,/* 0x00 */
    __glle_Begin_LineStrip                          ,/* 0x01 */
    __glle_Begin_Lines                              ,/* 0x02 */
    __glle_Begin_Points                             ,/* 0x03 */
    __glle_Begin_Polygon                            ,/* 0x04 */
    __glle_Begin_TriangleStrip                      ,/* 0x05 */
    __glle_Begin_TriangleFan                        ,/* 0x06 */
    __glle_Begin_Triangles                          ,/* 0x07 */
    __glle_Begin_QuadStrip                          ,/* 0x08 */
    __glle_Begin_Quads                              ,/* 0x09 */
    __glle_InvalidValue                                    ,/* 0x0a */
    __glle_InvalidEnum                                    ,/* 0x0b */
    __glle_InvalidOperation                                    ,/* 0x0c */
    __glle_UnimplementedExtension                   ,/* 0x0d */
    __glle_FastMaterial                                    ,/* 0x0e */
    __glle_TableTooLarge                            ,/* 0x0f */
};

/* 690f53d8 -- __glNopDispatchState: the "no context" table (glcore_hand.c: the gl* calls without a current context) */
__GLdispatchTable __glNopDispatchState = { {
    (__GLdispatchFn)0x150,                              /* the entry count */
    (__GLdispatchFn)__glNullDispatch2,                  /* glNewList */
    (__GLdispatchFn)__glNullDispatchReturn0,            /* glEndList */
    (__GLdispatchFn)__glNullDispatch1,                  /* glCallList */
    (__GLdispatchFn)__glNullDispatch3,                  /* glCallLists */
    (__GLdispatchFn)__glNullDispatch2,                  /* glDeleteLists */
    (__GLdispatchFn)__glNullDispatch1,                  /* glGenLists */
    (__GLdispatchFn)__glNullDispatch1,                  /* glListBase */
    (__GLdispatchFn)__glNullDispatch1,                  /* glBegin */
    (__GLdispatchFn)__glNullDispatch7,                  /* glBitmap */
    (__GLdispatchFn)__glNullDispatch3,                  /* glColor3b */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor3bv */
    (__GLdispatchFn)__glNullDispatch6,                  /* glColor3d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor3dv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glColor3f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor3fv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glColor3i */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor3iv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glColor3s */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor3sv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glColor3ub */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor3ubv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glColor3ui */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor3uiv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glColor3us */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor3usv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glColor4b */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor4bv */
    (__GLdispatchFn)__glNullDispatch8,                  /* glColor4d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor4dv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glColor4f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor4fv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glColor4i */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor4iv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glColor4s */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor4sv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glColor4ub */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor4ubv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glColor4ui */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor4uiv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glColor4us */
    (__GLdispatchFn)__glNullDispatch1,                  /* glColor4usv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glEdgeFlag */
    (__GLdispatchFn)__glNullDispatch1,                  /* glEdgeFlagv */
    (__GLdispatchFn)__glNullDispatchReturn0,            /* glEnd */
    (__GLdispatchFn)__glNullDispatch2,                  /* glIndexd */
    (__GLdispatchFn)__glNullDispatch1,                  /* glIndexdv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glIndexf */
    (__GLdispatchFn)__glNullDispatch1,                  /* glIndexfv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glIndexi */
    (__GLdispatchFn)__glNullDispatch1,                  /* glIndexiv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glIndexs */
    (__GLdispatchFn)__glNullDispatch1,                  /* glIndexsv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glNormal3b */
    (__GLdispatchFn)__glNullDispatch1,                  /* glNormal3bv */
    (__GLdispatchFn)__glNullDispatch6,                  /* glNormal3d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glNormal3dv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glNormal3f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glNormal3fv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glNormal3i */
    (__GLdispatchFn)__glNullDispatch1,                  /* glNormal3iv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glNormal3s */
    (__GLdispatchFn)__glNullDispatch1,                  /* glNormal3sv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glRasterPos2d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glRasterPos2dv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glRasterPos2f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glRasterPos2fv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glRasterPos2i */
    (__GLdispatchFn)__glNullDispatch1,                  /* glRasterPos2iv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glRasterPos2s */
    (__GLdispatchFn)__glNullDispatch1,                  /* glRasterPos2sv */
    (__GLdispatchFn)__glNullDispatch6,                  /* glRasterPos3d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glRasterPos3dv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glRasterPos3f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glRasterPos3fv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glRasterPos3i */
    (__GLdispatchFn)__glNullDispatch1,                  /* glRasterPos3iv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glRasterPos3s */
    (__GLdispatchFn)__glNullDispatch1,                  /* glRasterPos3sv */
    (__GLdispatchFn)__glNullDispatch8,                  /* glRasterPos4d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glRasterPos4dv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glRasterPos4f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glRasterPos4fv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glRasterPos4i */
    (__GLdispatchFn)__glNullDispatch1,                  /* glRasterPos4iv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glRasterPos4s */
    (__GLdispatchFn)__glNullDispatch1,                  /* glRasterPos4sv */
    (__GLdispatchFn)__glNullDispatch8,                  /* glRectd */
    (__GLdispatchFn)__glNullDispatch2,                  /* glRectdv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glRectf */
    (__GLdispatchFn)__glNullDispatch2,                  /* glRectfv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glRecti */
    (__GLdispatchFn)__glNullDispatch2,                  /* glRectiv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glRects */
    (__GLdispatchFn)__glNullDispatch2,                  /* glRectsv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glTexCoord1d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord1dv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord1f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord1fv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord1i */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord1iv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord1s */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord1sv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glTexCoord2d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord2dv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glTexCoord2f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord2fv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glTexCoord2i */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord2iv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glTexCoord2s */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord2sv */
    (__GLdispatchFn)__glNullDispatch6,                  /* glTexCoord3d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord3dv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexCoord3f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord3fv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexCoord3i */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord3iv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexCoord3s */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord3sv */
    (__GLdispatchFn)__glNullDispatch8,                  /* glTexCoord4d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord4dv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glTexCoord4f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord4fv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glTexCoord4i */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord4iv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glTexCoord4s */
    (__GLdispatchFn)__glNullDispatch1,                  /* glTexCoord4sv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glVertex2d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glVertex2dv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glVertex2f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glVertex2fv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glVertex2i */
    (__GLdispatchFn)__glNullDispatch1,                  /* glVertex2iv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glVertex2s */
    (__GLdispatchFn)__glNullDispatch1,                  /* glVertex2sv */
    (__GLdispatchFn)__glNullDispatch6,                  /* glVertex3d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glVertex3dv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glVertex3f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glVertex3fv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glVertex3i */
    (__GLdispatchFn)__glNullDispatch1,                  /* glVertex3iv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glVertex3s */
    (__GLdispatchFn)__glNullDispatch1,                  /* glVertex3sv */
    (__GLdispatchFn)__glNullDispatch8,                  /* glVertex4d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glVertex4dv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glVertex4f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glVertex4fv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glVertex4i */
    (__GLdispatchFn)__glNullDispatch1,                  /* glVertex4iv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glVertex4s */
    (__GLdispatchFn)__glNullDispatch1,                  /* glVertex4sv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glClipPlane */
    (__GLdispatchFn)__glNullDispatch2,                  /* glColorMaterial */
    (__GLdispatchFn)__glNullDispatch1,                  /* glCullFace */
    (__GLdispatchFn)__glNullDispatch2,                  /* glFogf */
    (__GLdispatchFn)__glNullDispatch2,                  /* glFogfv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glFogi */
    (__GLdispatchFn)__glNullDispatch2,                  /* glFogiv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glFrontFace */
    (__GLdispatchFn)__glNullDispatch2,                  /* glHint */
    (__GLdispatchFn)__glNullDispatch3,                  /* glLightf */
    (__GLdispatchFn)__glNullDispatch3,                  /* glLightfv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glLighti */
    (__GLdispatchFn)__glNullDispatch3,                  /* glLightiv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glLightModelf */
    (__GLdispatchFn)__glNullDispatch2,                  /* glLightModelfv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glLightModeli */
    (__GLdispatchFn)__glNullDispatch2,                  /* glLightModeliv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glLineStipple */
    (__GLdispatchFn)__glNullDispatch1,                  /* glLineWidth */
    (__GLdispatchFn)__glNullDispatch3,                  /* glMaterialf */
    (__GLdispatchFn)__glNullDispatch3,                  /* glMaterialfv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glMateriali */
    (__GLdispatchFn)__glNullDispatch3,                  /* glMaterialiv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glPointSize */
    (__GLdispatchFn)__glNullDispatch2,                  /* glPolygonMode */
    (__GLdispatchFn)__glNullDispatch1,                  /* glPolygonStipple */
    (__GLdispatchFn)__glNullDispatch4,                  /* glScissor */
    (__GLdispatchFn)__glNullDispatch1,                  /* glShadeModel */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexParameterf */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexParameterfv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexParameteri */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexParameteriv */
    (__GLdispatchFn)__glNullDispatch8,                  /* glTexImage1D */
    (__GLdispatchFn)__glNullDispatch9,                  /* glTexImage2D */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexEnvf */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexEnvfv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexEnvi */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexEnviv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glTexGend */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexGendv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexGenf */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexGenfv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexGeni */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTexGeniv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glFeedbackBuffer */
    (__GLdispatchFn)__glNullDispatch2,                  /* glSelectBuffer */
    (__GLdispatchFn)__glNullDispatch1,                  /* glRenderMode */
    (__GLdispatchFn)__glNullDispatchReturn0,            /* glInitNames */
    (__GLdispatchFn)__glNullDispatch1,                  /* glLoadName */
    (__GLdispatchFn)__glNullDispatch1,                  /* glPassThrough */
    (__GLdispatchFn)__glNullDispatchReturn0,            /* glPopName */
    (__GLdispatchFn)__glNullDispatch1,                  /* glPushName */
    (__GLdispatchFn)__glNullDispatch1,                  /* glDrawBuffer */
    (__GLdispatchFn)__glNullDispatch1,                  /* glClear */
    (__GLdispatchFn)__glNullDispatch4,                  /* glClearAccum */
    (__GLdispatchFn)__glNullDispatch1,                  /* glClearIndex */
    (__GLdispatchFn)__glNullDispatch4,                  /* glClearColor */
    (__GLdispatchFn)__glNullDispatch1,                  /* glClearStencil */
    (__GLdispatchFn)__glNullDispatch2,                  /* glClearDepth */
    (__GLdispatchFn)__glNullDispatch1,                  /* glStencilMask */
    (__GLdispatchFn)__glNullDispatch4,                  /* glColorMask */
    (__GLdispatchFn)__glNullDispatch1,                  /* glDepthMask */
    (__GLdispatchFn)__glNullDispatch1,                  /* glIndexMask */
    (__GLdispatchFn)__glNullDispatch2,                  /* glAccum */
    (__GLdispatchFn)__glNullDispatch1,                  /* glDisable */
    (__GLdispatchFn)__glNullDispatch1,                  /* glEnable */
    (__GLdispatchFn)__glNullDispatchReturn0,            /* glFinish */
    (__GLdispatchFn)__glNullDispatchReturn0,            /* glFlush */
    (__GLdispatchFn)__glNullDispatchReturn0,            /* glPopAttrib */
    (__GLdispatchFn)__glNullDispatch1,                  /* glPushAttrib */
    (__GLdispatchFn)__glNullDispatch8,                  /* glMap1d */
    (__GLdispatchFn)__glNullDispatch6,                  /* glMap1f */
    (__GLdispatchFn)__glNullDispatch14,                 /* glMap2d */
    (__GLdispatchFn)__glNullDispatch10,                 /* glMap2f */
    (__GLdispatchFn)__glNullDispatch5,                  /* glMapGrid1d */
    (__GLdispatchFn)__glNullDispatch3,                  /* glMapGrid1f */
    (__GLdispatchFn)__glNullDispatch10,                 /* glMapGrid2d */
    (__GLdispatchFn)__glNullDispatch6,                  /* glMapGrid2f */
    (__GLdispatchFn)__glNullDispatch2,                  /* glEvalCoord1d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glEvalCoord1dv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glEvalCoord1f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glEvalCoord1fv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glEvalCoord2d */
    (__GLdispatchFn)__glNullDispatch1,                  /* glEvalCoord2dv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glEvalCoord2f */
    (__GLdispatchFn)__glNullDispatch1,                  /* glEvalCoord2fv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glEvalMesh1 */
    (__GLdispatchFn)__glNullDispatch1,                  /* glEvalPoint1 */
    (__GLdispatchFn)__glNullDispatch5,                  /* glEvalMesh2 */
    (__GLdispatchFn)__glNullDispatch2,                  /* glEvalPoint2 */
    (__GLdispatchFn)__glNullDispatch2,                  /* glAlphaFunc */
    (__GLdispatchFn)__glNullDispatch2,                  /* glBlendFunc */
    (__GLdispatchFn)__glNullDispatch1,                  /* glLogicOp */
    (__GLdispatchFn)__glNullDispatch3,                  /* glStencilFunc */
    (__GLdispatchFn)__glNullDispatch3,                  /* glStencilOp */
    (__GLdispatchFn)__glNullDispatch1,                  /* glDepthFunc */
    (__GLdispatchFn)__glNullDispatch2,                  /* glPixelZoom */
    (__GLdispatchFn)__glNullDispatch2,                  /* glPixelTransferf */
    (__GLdispatchFn)__glNullDispatch2,                  /* glPixelTransferi */
    (__GLdispatchFn)__glNullDispatch2,                  /* glPixelStoref */
    (__GLdispatchFn)__glNullDispatch2,                  /* glPixelStorei */
    (__GLdispatchFn)__glNullDispatch3,                  /* glPixelMapfv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glPixelMapuiv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glPixelMapusv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glReadBuffer */
    (__GLdispatchFn)__glNullDispatch5,                  /* glCopyPixels */
    (__GLdispatchFn)__glNullDispatch7,                  /* glReadPixels */
    (__GLdispatchFn)__glNullDispatch5,                  /* glDrawPixels */
    (__GLdispatchFn)__glNullDispatch2,                  /* glGetBooleanv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glGetClipPlane */
    (__GLdispatchFn)__glNullDispatch2,                  /* glGetDoublev */
    (__GLdispatchFn)__glNullDispatchReturn0,            /* glGetError */
    (__GLdispatchFn)__glNullDispatch2,                  /* glGetFloatv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glGetIntegerv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetLightfv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetLightiv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetMapdv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetMapfv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetMapiv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetMaterialfv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetMaterialiv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glGetPixelMapfv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glGetPixelMapuiv */
    (__GLdispatchFn)__glNullDispatch2,                  /* glGetPixelMapusv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glGetPolygonStipple */
    (__GLdispatchFn)__glNullDispatch1,                  /* glGetString */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetTexEnvfv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetTexEnviv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetTexGendv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetTexGenfv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetTexGeniv */
    (__GLdispatchFn)__glNullDispatch5,                  /* glGetTexImage */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetTexParameterfv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glGetTexParameteriv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glGetTexLevelParameterfv */
    (__GLdispatchFn)__glNullDispatch4,                  /* glGetTexLevelParameteriv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glIsEnabled */
    (__GLdispatchFn)__glNullDispatch1,                  /* glIsList */
    (__GLdispatchFn)__glNullDispatch4,                  /* glDepthRange */
    (__GLdispatchFn)__glNullDispatch12,                 /* glFrustum */
    (__GLdispatchFn)__glNullDispatchReturn0,            /* glLoadIdentity */
    (__GLdispatchFn)__glNullDispatch1,                  /* glLoadMatrixf */
    (__GLdispatchFn)__glNullDispatch1,                  /* glLoadMatrixd */
    (__GLdispatchFn)__glNullDispatch1,                  /* glMatrixMode */
    (__GLdispatchFn)__glNullDispatch1,                  /* glMultMatrixf */
    (__GLdispatchFn)__glNullDispatch1,                  /* glMultMatrixd */
    (__GLdispatchFn)__glNullDispatch12,                 /* glOrtho */
    (__GLdispatchFn)__glNullDispatchReturn0,            /* glPopMatrix */
    (__GLdispatchFn)__glNullDispatchReturn0,            /* glPushMatrix */
    (__GLdispatchFn)__glNullDispatch8,                  /* glRotated */
    (__GLdispatchFn)__glNullDispatch4,                  /* glRotatef */
    (__GLdispatchFn)__glNullDispatch6,                  /* glScaled */
    (__GLdispatchFn)__glNullDispatch3,                  /* glScalef */
    (__GLdispatchFn)__glNullDispatch6,                  /* glTranslated */
    (__GLdispatchFn)__glNullDispatch3,                  /* glTranslatef */
    (__GLdispatchFn)__glNullDispatch4,                  /* glViewport */
    (__GLdispatchFn)__glNullDispatch1,                  /* glArrayElement */
    (__GLdispatchFn)__glNullDispatch2,                  /* glBindTexture */
    (__GLdispatchFn)__glNullDispatch4,                  /* glColorPointer */
    (__GLdispatchFn)__glNullDispatch1,                  /* glDisableClientState */
    (__GLdispatchFn)__glNullDispatch3,                  /* glDrawArrays */
    (__GLdispatchFn)__glNullDispatch4,                  /* glDrawElements */
    (__GLdispatchFn)__glNullDispatch2,                  /* glEdgeFlagPointer */
    (__GLdispatchFn)__glNullDispatch1,                  /* glEnableClientState */
    (__GLdispatchFn)__glNullDispatch3,                  /* glIndexPointer */
    (__GLdispatchFn)__glNullDispatch1,                  /* glIndexub */
    (__GLdispatchFn)__glNullDispatch1,                  /* glIndexubv */
    (__GLdispatchFn)__glNullDispatch3,                  /* glInterleavedArrays */
    (__GLdispatchFn)__glNullDispatch3,                  /* glNormalPointer */
    (__GLdispatchFn)__glNullDispatch2,                  /* glPolygonOffset */
    (__GLdispatchFn)__glNullDispatch4,                  /* glTexCoordPointer */
    (__GLdispatchFn)__glNullDispatch4,                  /* glVertexPointer */
    (__GLdispatchFn)__glNullDispatch3,                  /* glAreTexturesResident */
    (__GLdispatchFn)__glNullDispatch7,                  /* glCopyTexImage1D */
    (__GLdispatchFn)__glNullDispatch8,                  /* glCopyTexImage2D */
    (__GLdispatchFn)__glNullDispatch6,                  /* glCopyTexSubImage1D */
    (__GLdispatchFn)__glNullDispatch8,                  /* glCopyTexSubImage2D */
    (__GLdispatchFn)__glNullDispatch2,                  /* glDeleteTextures */
    (__GLdispatchFn)__glNullDispatch2,                  /* glGenTextures */
    (__GLdispatchFn)__glNullDispatch2,                  /* glGetPointerv */
    (__GLdispatchFn)__glNullDispatch1,                  /* glIsTexture */
    (__GLdispatchFn)__glNullDispatch3,                  /* glPrioritizeTextures */
    (__GLdispatchFn)__glNullDispatch7,                  /* glTexSubImage1D */
    (__GLdispatchFn)__glNullDispatch9,                  /* glTexSubImage2D */
    (__GLdispatchFn)__glNullDispatchReturn0,            /* glPopClientAttrib */
    (__GLdispatchFn)__glNullDispatch1,                  /* glPushClientAttrib */
    (__GLdispatchFn)__glNullDispatch1,                  /* extension slot 0x151 */
    (__GLdispatchFn)__glNullDispatch5,                  /* extension slot 0x152 */
    (__GLdispatchFn)__glNullDispatch3,                  /* extension slot 0x153 */
    (__GLdispatchFn)__glNullDispatch3,                  /* extension slot 0x154 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x155 */
    (__GLdispatchFn)__glNullDispatch4,                  /* extension slot 0x156 */
    (__GLdispatchFn)__glNullDispatch4,                  /* extension slot 0x157 */
    (__GLdispatchFn)__glNullDispatch5,                  /* extension slot 0x158 */
    (__GLdispatchFn)__glNullDispatch5,                  /* extension slot 0x159 */
    (__GLdispatchFn)__glNullDispatch6,                  /* extension slot 0x15a */
    (__GLdispatchFn)__glNullDispatch6,                  /* extension slot 0x15b */
    (__GLdispatchFn)__glNullDispatch5,                  /* extension slot 0x15c */
    (__GLdispatchFn)__glNullDispatch4,                  /* extension slot 0x15d */
    (__GLdispatchFn)__glNullDispatch3,                  /* extension slot 0x15e */
    (__GLdispatchFn)__glNullDispatch3,                  /* extension slot 0x15f */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x160 */
    (__GLdispatchFn)__glNullDispatchReturn0,            /* extension slot 0x161 */
    (__GLdispatchFn)__glNullDispatch1,                  /* extension slot 0x162 */
    (__GLdispatchFn)__glNullDispatch1,                  /* extension slot 0x163 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x164 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x165 */
    (__GLdispatchFn)__glNullDispatch4,                  /* extension slot 0x166 */
    (__GLdispatchFn)__glNullDispatch1,                  /* extension slot 0x167 */
    (__GLdispatchFn)__glNullDispatch1,                  /* extension slot 0x168 */
    (__GLdispatchFn)__glNullDispatch3,                  /* extension slot 0x169 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x16a */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x16b */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x16c */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x16d */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x16e */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x16f */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x170 */
    (__GLdispatchFn)__glNullDispatch5,                  /* extension slot 0x171 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x172 */
    (__GLdispatchFn)__glNullDispatch3,                  /* extension slot 0x173 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x174 */
    (__GLdispatchFn)__glNullDispatch3,                  /* extension slot 0x175 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x176 */
    (__GLdispatchFn)__glNullDispatch3,                  /* extension slot 0x177 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x178 */
    (__GLdispatchFn)__glNullDispatch7,                  /* extension slot 0x179 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x17a */
    (__GLdispatchFn)__glNullDispatch4,                  /* extension slot 0x17b */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x17c */
    (__GLdispatchFn)__glNullDispatch4,                  /* extension slot 0x17d */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x17e */
    (__GLdispatchFn)__glNullDispatch4,                  /* extension slot 0x17f */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x180 */
    (__GLdispatchFn)__glNullDispatch9,                  /* extension slot 0x181 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x182 */
    (__GLdispatchFn)__glNullDispatch5,                  /* extension slot 0x183 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x184 */
    (__GLdispatchFn)__glNullDispatch5,                  /* extension slot 0x185 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x186 */
    (__GLdispatchFn)__glNullDispatch5,                  /* extension slot 0x187 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x188 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x189 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x18a */
    (__GLdispatchFn)__glNullDispatch1,                  /* extension slot 0x18b */
    (__GLdispatchFn)__glNullDispatch7,                  /* extension slot 0x18c */
    (__GLdispatchFn)__glNullDispatch8,                  /* extension slot 0x18d */
    (__GLdispatchFn)__glNullDispatch7,                  /* extension slot 0x18e */
    (__GLdispatchFn)__glNullDispatch9,                  /* extension slot 0x18f */
    (__GLdispatchFn)__glNullDispatch3,                  /* extension slot 0x190 */
    (__GLdispatchFn)__glNullDispatch1,                  /* extension slot 0x191 */
    (__GLdispatchFn)__glNullDispatch1,                  /* extension slot 0x192 */
    (__GLdispatchFn)__glNullDispatch2,                  /* extension slot 0x193 */
    (__GLdispatchFn)__glNullDispatch1,                  /* extension slot 0x194 */
    (__GLdispatchFn)__glNullDispatch1,                  /* extension slot 0x195 */
    (__GLdispatchFn)__glNullDispatch3,                  /* extension slot 0x196 */
    (__GLdispatchFn)0,                                  /* extension slot 0x197 */
    (__GLdispatchFn)0,                                  /* extension slot 0x198 */
    (__GLdispatchFn)0,                                  /* extension slot 0x199 */
} };

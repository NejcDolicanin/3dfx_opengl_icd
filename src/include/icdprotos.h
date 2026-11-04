/*
** icdprotos.h -- the prototypes of the functions that are called from another source file than the one defining
** them: one declaration each, instead of a local extern in every caller.  Include after wglpriv.h.
**   - The first group was written by hand: the signature most callers used.
**   - The hoisted block below is copied from each function's own definition, grouped by the defining file.
** Not here, on purpose: static functions; functions defined inside an #if; functions that some caller still
** declares differently (a deliberate local variant, kept beside that caller).
*/
#ifndef ICDPROTOS_H
#define ICDPROTOS_H

#include "glsitypes.h"      /* __GLtexNamePriICD */

void __fastcall winReattachFrontBuffer(__GLcontext *gc);
void __stdcall __glsstim_DepthFunc(GLenum func);
GLubyte *__fastcall og_callRel32(__GLcontext *gc, GLubyte *pc, GLuint routine);
GLint __fastcall grGetInteger(GLint pname);    /* sst_padding.c */
GLubyte *__fastcall hwPageCacheInsert(__GLcontext *gc, __GLhwBufRecICD *rec, const void *key, void *outBlock);    /* sst_codebuf.c: the code page; *outBlock = the data page */
void __fastcall hwFlushGrState(__GLcontext *gc, GLuint edx_unused);
void __fastcall og_genOffsetFifo(__GLcontext *gc);
void __fastcall hwBuildFifoTriProcs(__GLcontext *gc);
GLboolean __fastcall texMgrReset(__GLcontext *gc, __GLtexMgrFxICD *m);
GLubyte *__fastcall og_callRel32Gen(__GLcontext *gc, GLubyte *pc, GLuint routine);
void __fastcall og_buildClipInterp(__GLcontext *gc);
void __fastcall __glSSTRenderWideAAPoint(__GLcontext *gc, __GLvertexICD *v);
void __fastcall __glSSTRenderBigPoint(__GLcontext *gc, __GLvertexICD *v);
void __fastcall __glCopyDispatchTable(const __GLdispatchTable *src, __GLdispatchTable *dst);    /* gl_export.c */
void __fastcall __glValidateVertex2(__GLcontext *gc, __GLvertexICD *vx, GLuint needs);
void __fastcall __glValidateVertex3(__GLcontext *gc, __GLvertexICD *vx, GLuint needs);
void __fastcall __glValidateVertex4(__GLcontext *gc, __GLvertexICD *vx, GLuint needs);
void __fastcall __glNop1(void);
void __fastcall __glNop2(void);
void __fastcall __glEndPrim(__GLcontext *gc);
void __fastcall __glInitBuffer(__GLbufferICD *buf);
void __fastcall __glMakeCurrentBuffer(__GLbufferICD *buf, __WGLbuffer *drawableBuf, __GLcontext *gc);
void __fastcall __glLoseCurrentBuffer(__GLbufferICD *buf);
void __fastcall __glSetError(GLenum error);
GLint __fastcall __glImageSize(GLsizei width, GLsizei height, GLenum format, GLenum type);
void __fastcall __glFillImage(__GLcontext *gc, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *userdata, GLubyte *newimage);
void __fastcall __glDoPolygonClip(__GLcontext *gc, __GLvertexICD **iv, GLint nout, GLuint allClipCodes);
__GLtexObjParamICD *__fastcall __glLookUpTextureParams(__GLcontext *gc, GLenum target, GLint unit);
__GLtexNamePriICD *__fastcall __glLookUpTextureTexobjs(__GLcontext *gc, GLenum target, GLint unit);
__GLtextureICD *__fastcall __glLookUpTexture(__GLcontext *gc, GLenum target, GLint texUnit);
__GLtextureICD *__fastcall __glCurrentTexture(__GLcontext *gc, GLint unit);
double __fastcall tex_Clamp(GLuint unused1, GLuint unused2, __GLfloat val, __GLfloat min, __GLfloat max);
GLuint __fastcall __glTexParameterfv_size(GLenum e);
void __fastcall __glBindTexture(__GLcontext *gc, GLint targetIndex, GLuint texture);    /* gl_tex.c */
void __fastcall __glFogFragmentSlow(__GLcontext *gc, __GLfragmentICD *frag, __GLfloat eyeZ);
void __stdcall __glim_Vertex2fv(const GLfloat *v);
void __stdcall __glim_Vertex3fv(const GLfloat *v);
void __stdcall __glim_Vertex4fv(const GLfloat *v);
void __fastcall __glDoClipTriangle(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1, __GLvertexICD *v2);
void __fastcall ClearFill(__GLcolorBufferICD *cfb, GLint x0, GLint y0, GLint x1, GLint y1);
void __fastcall ClearRect_16_Dither(__GLcolorBufferICD *cfb, GLint x0, GLint y0, GLint x1, GLint y1);
GLint __fastcall __glBytesPerElement(GLenum type);    /* also in si_names.h */
__GLfloat __fastcall __glBuildAntiAliasIndex(__GLfloat index, __GLfloat antiAliasPercent);   /* s_ptaa.c: both floats on the stack */
void __fastcall __glClipLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b);

struct __GLshadeRhoRec;                 /* gl_tex.c's __GLshadeRho (__glComputePolygonRho): a file-scope tag */

/* ---- hoisted prototypes: one per function, written from its definition ---- */
/* gl/gl_accum.c */
void __fastcall __glInitAccum64(__GLaccumBufferICD *afb, __GLcontext *gc);
/* gl/gl_attrib.c */
GLboolean __fastcall __glCopyContext(__GLcontext *dst, const __GLcontext *src, GLuint mask);
void __fastcall __glFreeAttributeState(__GLcontext *gc);
void __stdcall __glim_DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void __stdcall __gllc_DrawArrays(GLenum mode, GLint first, GLsizei count);
void __fastcall DrawVerticesLLoop(__GLcontext *gc, GLint first, GLsizei count, GLuint *elements);
void __fastcall DrawVerticesPolygon(__GLcontext *gc, GLint first, GLsizei count, GLuint *elements);
/* gl/gl_begin.c */
void __fastcall __glBeginLines(__GLcontext *gc);
void __fastcall __glBeginLLoop(__GLcontext *gc);
void __fastcall __glBeginLStrip(__GLcontext *gc);
void __fastcall __glBeginPoints(__GLcontext *gc);
void __fastcall __glBeginPolygon(__GLcontext *gc);
void __fastcall __glBeginQStrip(__GLcontext *gc);
void __fastcall __glBeginQuads(__GLcontext *gc);
void __fastcall __glBeginTFan(__GLcontext *gc);
void __fastcall __glBeginTriangles(__GLcontext *gc);
void __fastcall __glBeginTStrip(__GLcontext *gc);
void __fastcall __glEndLLoop(__GLcontext *gc);
void __fastcall __glFreeEvaluatorState(__GLcontext *gc);
void __fastcall __glim_End(void);
void __fastcall __glMatValidateV1(__GLcontext *gc);
void __fastcall __glMatValidateV1V2(__GLcontext *gc);
void __fastcall __glMatValidateV1V2V3(__GLcontext *gc);
void __fastcall __glMatValidateV2V3(__GLcontext *gc);
void __fastcall __glPointFast(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall FirstQuadsVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall FirstTrianglesVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall SecondLLoopVertex(__GLcontext *gc, __GLvertexICD *v0);
/* gl/gl_bitmap.c */
void __fastcall __glDrawBitmap(__GLcontext *gc, GLsizei width, GLsizei height, GLfloat xOrig, GLfloat yOrig,
                               GLfloat xMove, GLfloat yMove, const GLubyte oldbits[]);
void __fastcall __glRenderBitmap(__GLcontext *gc, const __GLbitmap *bitmap, const GLubyte *data);
/* gl/gl_buffer.c */
GLboolean __fastcall __glFindWindowSize(__GLcontext *gc);
void __fastcall __glLockBuffer(__GLcontext *gc, GLuint mask);
void __fastcall __glLockColorBuffer(__GLcontext *gc);
GLboolean __fastcall __glResizeBuffers(__GLcontext *gc, GLint x, GLint y, GLint width, GLint height);
void __fastcall __glUnlockBuffer(__GLcontext *gc, GLuint mask);
void __fastcall __glUnlockColorBuffer(__GLcontext *gc);
void __fastcall __glUnlockColorBufferDepthBuffer(__GLcontext *gc);
/* gl/gl_capi.c */
void __stdcall __glim_Color3fv(const GLfloat v[3]);
void __stdcall __glim_Color4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void __stdcall __glim_Color4fv(const GLfloat v[4]);
__GLfloat __fastcall __glNopLineRho(__GLcontext *gc, GLint unit, __GLfloat s, __GLfloat t, __GLfloat qw);
__GLfloat __fastcall __glNopPolygonRho(__GLcontext *gc, __GLshadeTailICD *sh, __GLfloat s, __GLfloat t, __GLfloat qw);
/* gl/gl_ci.c */
void __fastcall __glInitCI(__GLcolorBufferICD *cfb, __GLcontext *gc);
/* gl/gl_clip.c */
GLuint __fastcall __glClipCheckFast(__GLcontext *gc, __GLvertexICD *v);
void __fastcall __glGenericPickParameterClipProcs(__GLcontext *gc);
/* gl/gl_context.c */
void __fastcall __glContextSetColorScales(__GLcontext *gc);
GLboolean __fastcall __glDestroyContext(__GLcontext *gc);
void __fastcall __glEarlyInitContext(__GLcontext *gc);
void __fastcall __glSoftResetContext(__GLcontext *gc);
/* gl/gl_ctable.c */
GLenum __fastcall __glCheckColorTableArgs(__GLcontext *gc_unused, GLenum target, GLenum internalFormat, GLsizei width,
                                          GLenum format, GLenum type);
void __fastcall __glColorSubTableEXT(__GLcontext *gc, GLenum target, GLsizei start, GLsizei count, GLenum format,
                                     GLenum type, const GLvoid *table, GLboolean dlist);
void __fastcall __glColorTableEXT(__GLcontext *gc, GLenum target, GLenum internalFormat, GLsizei width, GLenum format,
                                  GLenum type, const GLvoid *table, GLboolean dlist);
GLint *__fastcall __glColorTableModifiers(__GLcontext *gc, GLenum target, GLenum pname);
/* gl/gl_depth.c */
void __fastcall __glInitDepth(__GLdepthBufferICD *fb, __GLcontext *gc);
void __fastcall __glInitDepth16(__GLdepthBufferICD *fb);
void __fastcall __glInitDepth24(__GLdepthBufferICD *fb);
void __fastcall __glInitDepth32(__GLdepthBufferICD *fb);
void __fastcall __glValidateZCount(__GLdepthBufferICD *fb);
GLboolean __fastcall LoseCurrent(__GLcontext *gc);
/* gl/gl_dlist.c */
void __fastcall __glDoInterpret(GLubyte *PC);
void __fastcall __glFreeDlistState(__GLcontext *gc);
void __fastcall __glInitDlistState(__GLcontext *gc);
void __fastcall ApplyMatChange(__GLcontext *gc, __GLmatChange *f, GLenum pname, __GLfloat *pv);
void __fastcall FreeFastMaterial(__GLcontext *gc, GLubyte *PC);
/* gl/gl_dlopt.c */
void __fastcall __glDlistOptimizeMaterial(__GLcontext *gc, __GLcompiledDlist *cdlist);
/* gl/gl_doget.c */
void __fastcall __glConvertResult(__GLcontext *gc, GLint fromType, const void *rawdata, GLint toType, void *result,
                                  GLint size);
void __fastcall __glDoGet(GLenum sq, void *result, GLint type, const char *procName);
/* gl/gl_enable.c */
GLboolean __stdcall __glim_IsEnabled(GLenum cap);
/* gl/gl_eval.c */
void __fastcall __glInitEvaluatorState(__GLcontext *gc);
/* gl/gl_export.c */
GLuint __fastcall __glClipCheckAll2(__GLcontext *gc, __GLvertexICD *vx);
GLuint __fastcall __glClipCheckAll3(__GLcontext *gc, __GLvertexICD *vx);
GLuint __fastcall __glClipCheckAll4(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __glCopyMatrix(__GLmatrixICD *dst, const __GLmatrixICD *src);
void __fastcall __glGenericPickPointProcs(__GLcontext *gc, int unused);
void __fastcall __glMakeIdentity(__GLmatrixICD *m);
GLboolean __fastcall null_DestroyContext(__GLcontext *gc);
/* gl/gl_feedback.c */
void __fastcall __glFeedbackLine(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1);
void __fastcall __glFeedbackTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall __glSelectLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b);
void __fastcall __glSelectPoint(__GLcontext *gc, __GLvertexICD *v);
/* gl/gl_finish.c */
void __fastcall __glim_Finish(int retAddr);
/* gl/gl_fog.c */
GLboolean __fastcall __glFogSpanSlow(__GLcontext *gc);
GLboolean __fastcall __glFogStippledSpanSlow(__GLcontext *gc);
__GLfloat __fastcall __glFogVertex(__GLcontext *gc, __GLvertexICD *vx);
__GLfloat __fastcall __glFogVertexFogCoord(__GLcontext *gc, __GLvertexICD *vx);
/* gl/gl_get.c */
void __fastcall __glDlistFreeArray(__GLcontext *gc, __GLdlistArray *array);
__GLdlistArray *__fastcall __glDlistNewArray(__GLcontext *gc);
void __fastcall __glFreePixelState(__GLcontext *gc);
void __stdcall __glim_GetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat v[]);
void __stdcall __glim_GetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint v[]);
void __fastcall __glNop_2(void);
/* gl/gl_image.c */
void __fastcall __glFillImage3D(__GLcontext *gc, GLsizei width, GLsizei height, GLsizei depth, GLenum format,
                                GLenum type, const GLvoid *userdata, GLubyte *newimage);
/* gl/gl_lcomp2.c */
void __fastcall ClipLine(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1);
/* gl/gl_lexec.c */
void __fastcall __glInitLUTCache(__GLcontext *gc);
const GLubyte * __fastcall __glle_Accum(const GLubyte *PC);
const GLubyte * __fastcall __glle_ActiveTextureARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_AlphaFunc(const GLubyte *PC);
const GLubyte * __fastcall __glle_BindTexture(const GLubyte *PC);
const GLubyte * __fastcall __glle_BlendEquationEXT(const GLubyte *PC);
const GLubyte * __fastcall __glle_BlendFunc(const GLubyte *PC);
const GLubyte * __fastcall __glle_Clear(const GLubyte *PC);
const GLubyte * __fastcall __glle_ClearDepth(const GLubyte *PC);
const GLubyte * __fastcall __glle_ClearIndex(const GLubyte *PC);
const GLubyte * __fastcall __glle_ClearStencil(const GLubyte *PC);
const GLubyte * __fastcall __glle_ClientActiveTextureARB(const GLubyte *PC);
const GLubyte *__fastcall __glle_ClipPlane(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color3bv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color3dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color3fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color3iv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color3sv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color3ubv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color3uiv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color3usv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color4bv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color4dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color4fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color4iv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color4sv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color4ubv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color4uiv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Color4usv(const GLubyte *PC);
const GLubyte * __fastcall __glle_ColorMask(const GLubyte *PC);
const GLubyte * __fastcall __glle_ColorMaterial(const GLubyte *PC);
const GLubyte * __fastcall __glle_CompressedTexImage1DARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_CompressedTexImage2DARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_CompressedTexSubImage1DARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_CompressedTexSubImage2DARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_CopyTexImage1D(const GLubyte *PC);
const GLubyte * __fastcall __glle_CopyTexImage2D(const GLubyte *PC);
const GLubyte * __fastcall __glle_CopyTexSubImage2D(const GLubyte *PC);
const GLubyte * __fastcall __glle_CullFace(const GLubyte *PC);
const GLubyte * __fastcall __glle_DepthFunc(const GLubyte *PC);
const GLubyte * __fastcall __glle_DepthMask(const GLubyte *PC);
const GLubyte * __fastcall __glle_DrawBuffer(const GLubyte *PC);
const GLubyte * __fastcall __glle_EdgeFlagv(const GLubyte *PC);
const GLubyte * __fastcall __glle_End(const GLubyte *PC);
const GLubyte * __fastcall __glle_EvalCoord1dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_EvalCoord1fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_EvalCoord2dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_EvalCoord2fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_EvalPoint1(const GLubyte *PC);
const GLubyte * __fastcall __glle_EvalPoint2(const GLubyte *PC);
const GLubyte * __fastcall __glle_FogCoorddEXT(const GLubyte *PC);
const GLubyte * __fastcall __glle_FogCoorddvEXT(const GLubyte *PC);
const GLubyte * __fastcall __glle_FogCoordfEXT(const GLubyte *PC);
const GLubyte * __fastcall __glle_FogCoordfvEXT(const GLubyte *PC);
const GLubyte * __fastcall __glle_Fogf(const GLubyte *PC);
const GLubyte * __fastcall __glle_Fogi(const GLubyte *PC);
const GLubyte * __fastcall __glle_Fogiv(const GLubyte *PC);
const GLubyte * __fastcall __glle_FrontFace(const GLubyte *PC);
const GLubyte *__fastcall __glle_Frustum(const GLubyte *PC);
const GLubyte * __fastcall __glle_Hint(const GLubyte *PC);
const GLubyte * __fastcall __glle_Indexdv(const GLubyte *PC);
const GLubyte * __fastcall __glle_IndexFuncSGI(const GLubyte *PC);
const GLubyte * __fastcall __glle_Indexfv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Indexiv(const GLubyte *PC);
const GLubyte * __fastcall __glle_IndexMask(const GLubyte *PC);
const GLubyte * __fastcall __glle_IndexMaterialSGI(const GLubyte *PC);
const GLubyte * __fastcall __glle_Indexsv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Indexubv(const GLubyte *PC);
const GLubyte * __fastcall __glle_InitNames(const GLubyte *PC);
const GLubyte * __fastcall __glle_LightModelfv(const GLubyte *PC);
const GLubyte * __fastcall __glle_LineStipple(const GLubyte *PC);
const GLubyte * __fastcall __glle_LineWidth(const GLubyte *PC);
const GLubyte * __fastcall __glle_ListBase(const GLubyte *PC);
const GLubyte * __fastcall __glle_LoadIdentity(const GLubyte *PC);
const GLubyte * __fastcall __glle_LoadMatrixd(const GLubyte *PC);
const GLubyte * __fastcall __glle_LoadMatrixf(const GLubyte *PC);
const GLubyte * __fastcall __glle_LoadName(const GLubyte *PC);
const GLubyte * __fastcall __glle_LogicOp(const GLubyte *PC);
const GLubyte * __fastcall __glle_MapGrid1d(const GLubyte *PC);
const GLubyte * __fastcall __glle_MapGrid2d(const GLubyte *PC);
const GLubyte * __fastcall __glle_MatrixMode(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord1dARB(const GLubyte *PC);
const GLubyte *__fastcall __glle_MultiTexCoord1dvARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord1fARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord1fvARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord1iARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord1ivARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord1sARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord1svARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord2dARB(const GLubyte *PC);
const GLubyte *__fastcall __glle_MultiTexCoord2dvARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord2fvARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord2ivARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord2sARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord2svARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord3dARB(const GLubyte *PC);
const GLubyte *__fastcall __glle_MultiTexCoord3dvARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord3fvARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord3ivARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord3sARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord3svARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord4dARB(const GLubyte *PC);
const GLubyte *__fastcall __glle_MultiTexCoord4dvARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord4fvARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord4ivARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord4sARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultiTexCoord4svARB(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultMatrixd(const GLubyte *PC);
const GLubyte * __fastcall __glle_MultMatrixf(const GLubyte *PC);
const GLubyte * __fastcall __glle_Normal3bv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Normal3dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Normal3fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Normal3iv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Normal3sv(const GLubyte *PC);
const GLubyte *__fastcall __glle_Ortho(const GLubyte *PC);
const GLubyte * __fastcall __glle_PassThrough(const GLubyte *PC);
const GLubyte * __fastcall __glle_PixelMapfv(const GLubyte *PC);
const GLubyte * __fastcall __glle_PixelMapuiv(const GLubyte *PC);
const GLubyte * __fastcall __glle_PixelMapusv(const GLubyte *PC);
const GLubyte * __fastcall __glle_PixelTransferf(const GLubyte *PC);
const GLubyte * __fastcall __glle_PixelTransferi(const GLubyte *PC);
const GLubyte * __fastcall __glle_PixelZoom(const GLubyte *PC);
const GLubyte * __fastcall __glle_PointParameterfEXT(const GLubyte *PC);
const GLubyte * __fastcall __glle_PointParameterfvEXT(const GLubyte *PC);
const GLubyte * __fastcall __glle_PointSize(const GLubyte *PC);
const GLubyte * __fastcall __glle_PolygonMode(const GLubyte *PC);
const GLubyte * __fastcall __glle_PolygonOffset(const GLubyte *PC);
const GLubyte * __fastcall __glle_PopAttrib(const GLubyte *PC);
const GLubyte * __fastcall __glle_PopMatrix(const GLubyte *PC);
const GLubyte * __fastcall __glle_PopName(const GLubyte *PC);
const GLubyte * __fastcall __glle_PrioritizeTextures(const GLubyte *PC);
const GLubyte * __fastcall __glle_PushAttrib(const GLubyte *PC);
const GLubyte * __fastcall __glle_PushMatrix(const GLubyte *PC);
const GLubyte * __fastcall __glle_PushName(const GLubyte *PC);
const GLubyte * __fastcall __glle_RasterPos2dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_RasterPos2fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_RasterPos2iv(const GLubyte *PC);
const GLubyte * __fastcall __glle_RasterPos2sv(const GLubyte *PC);
const GLubyte * __fastcall __glle_RasterPos3dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_RasterPos3fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_RasterPos3iv(const GLubyte *PC);
const GLubyte * __fastcall __glle_RasterPos3sv(const GLubyte *PC);
const GLubyte * __fastcall __glle_RasterPos4dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_RasterPos4fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_RasterPos4iv(const GLubyte *PC);
const GLubyte * __fastcall __glle_RasterPos4sv(const GLubyte *PC);
const GLubyte * __fastcall __glle_ReadBuffer(const GLubyte *PC);
const GLubyte *__fastcall __glle_Rectdv(const GLubyte *PC);
const GLubyte *__fastcall __glle_Rectfv(const GLubyte *PC);
const GLubyte *__fastcall __glle_Rectiv(const GLubyte *PC);
const GLubyte *__fastcall __glle_Rectsv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Rotated(const GLubyte *PC);
const GLubyte * __fastcall __glle_ShadeModel(const GLubyte *PC);
const GLubyte * __fastcall __glle_StencilMask(const GLubyte *PC);
const GLubyte * __fastcall __glle_TBufferMask3DFX(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord1dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord1fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord1iv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord1sv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord2dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord2fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord2iv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord2sv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord3dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord3fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord3iv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord3sv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord4dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord4fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord4iv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexCoord4sv(const GLubyte *PC);
const GLubyte * __fastcall __glle_TexGendv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Vertex2dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Vertex2fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Vertex2iv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Vertex2sv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Vertex3dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Vertex3fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Vertex3iv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Vertex3sv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Vertex4dv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Vertex4fv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Vertex4iv(const GLubyte *PC);
const GLubyte * __fastcall __glle_Vertex4sv(const GLubyte *PC);
/* gl/gl_light.c */
void __fastcall __glFreeLUTCache(__GLcontext *gc);
void __fastcall __glGenericPickColorMaterialProcs(__GLcontext *gc);
void __fastcall __glValidateLighting(__GLcontext *gc);
void __fastcall __glValidateModelViewForLighting(__GLcontext *gc);
/* gl/gl_line.c */
void __fastcall __gl1stLStripVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __gl2ndLLoopVertexLock(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __glEndLLoopLock(__GLcontext *gc);
void __fastcall __glOtherLStripVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __glOtherLStripVertexFast(__GLcontext *gc, __GLvertexICD *v1);
void __fastcall __glOtherLStripVertexFastLock(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __glOtherLStripVertexLock(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __glSecondLStripVertex(__GLcontext *gc, __GLvertexICD *v1);
/* gl/gl_lnclip.c */
void __fastcall __glFastClipFlatLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b);
void __fastcall __glFastClipSmoothLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b);
/* gl/gl_lndraw.c */
void __fastcall __glRenderAliasLine(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1);
void __fastcall __glRenderAntiAliasLine(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1);
void __fastcall __glRenderFlatFogLine(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1);
/* gl/gl_lnfast.c */
GLboolean __fastcall __glStoreLine_CI_8_Flat(__GLcontext *gc);
GLboolean __fastcall __glStoreLine_CI_8_Flat_Dither(__GLcontext *gc);
GLboolean __fastcall __glStoreLine_CI_8_Smooth(__GLcontext *gc);
GLboolean __fastcall __glStoreLine_CI_8_Smooth_Dither(__GLcontext *gc);
GLboolean __fastcall __glStoreLine_RGB_16_Dither(__GLcontext *gc);
GLboolean __fastcall __glStoreLine_RGB_16_Flat(__GLcontext *gc);
GLboolean __fastcall __glStoreLine_RGB_16_Smooth(__GLcontext *gc);
GLboolean __fastcall __glStoreStippledLine_CI_8_Flat(__GLcontext *gc);
GLboolean __fastcall __glStoreStippledLine_CI_8_Flat_Dither(__GLcontext *gc);
GLboolean __fastcall __glStoreStippledLine_CI_8_Smooth(__GLcontext *gc);
GLboolean __fastcall __glStoreStippledLine_CI_8_Smooth_Dither(__GLcontext *gc);
GLboolean __fastcall __glStoreStippledLine_RGB_16_Dither(__GLcontext *gc);
GLboolean __fastcall __glStoreStippledLine_RGB_16_Flat(__GLcontext *gc);
GLboolean __fastcall __glStoreStippledLine_RGB_16_Smooth(__GLcontext *gc);
/* gl/gl_lnspan.c */
GLboolean __fastcall __glAntiAliasLine(__GLcontext *gc);
GLboolean __fastcall __glAntiAliasStippledLine(__GLcontext *gc);
GLboolean __fastcall __glDepthPassLine(__GLcontext *gc);
GLboolean __fastcall __glDepthPassStippledLine(__GLcontext *gc);
GLboolean __fastcall __glDepthTestLine(__GLcontext *gc);
GLboolean __fastcall __glDepthTestStencilLine(__GLcontext *gc);
GLboolean __fastcall __glDepthTestStencilStippledLine(__GLcontext *gc);
GLboolean __fastcall __glDepthTestStippledLine(__GLcontext *gc);
GLboolean __fastcall __glDrawBothLine(__GLcontext *gc);
GLboolean __fastcall __glDrawBothStippledLine(__GLcontext *gc);
GLboolean __fastcall __glProcessLine(__GLcontext *gc);
GLboolean __fastcall __glProcessLine3NW(__GLcontext *gc);
GLboolean __fastcall __glScissorLine(__GLcontext *gc);
GLboolean __fastcall __glScissorStippledLine(__GLcontext *gc);
GLboolean __fastcall __glStencilTestLine(__GLcontext *gc);
GLboolean __fastcall __glStencilTestStippledLine(__GLcontext *gc);
GLboolean __fastcall __glStoreLine(__GLcontext *gc);
GLboolean __fastcall __glStoreStippledLine(__GLcontext *gc);
GLboolean __fastcall __glWideLineRep(__GLcontext *gc);
GLboolean __fastcall __glWideStippleLineRep(__GLcontext *gc);
/* gl/gl_memclr.c */
void __fastcall __glCalcCIColor(__GLcontext *gc, GLint face, __GLvertexICD *vx);
void __fastcall FastMemClear1(__GLcontext *gc, GLubyte *cptr, GLint w, GLint h, GLint skip, GLuint val);
void __fastcall fillRectDwords(__GLcontext *gc, GLuint *dst, GLuint w, GLint h, GLint skip, GLuint value);
/* gl/gl_pgaa.c */
void __fastcall __glFillAntiAliasedTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
/* gl/gl_pgclip.c */
void __fastcall __glCalc2DOutcode(__GLcontext *gc, __GLclipVertex *cv, __GLregionRect *rect);
void __fastcall __glClipPolygon(__GLcontext *gc, __GLvertexICD *v0, GLint nv);
void __fastcall __glClipTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c,
                                 GLuint orCodes);
/* gl/gl_pgdraw.c */
void __fastcall __glFillFlatFogTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall __glFillTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
/* gl/gl_pick.c */
void __fastcall __glBeginDispatchOverride(void);
__GLdispatchTable *__fastcall __glDispatchExec(__GLcontext *gc);
void __fastcall __glEndDispatchOverride(void);
void __fastcall __glFreeBufData(__GLcontext *gc);
void __fastcall __glFreePrivate(__WGLdrawable *dp);
void __fastcall __glGenericPickAllProcsPhase2(__GLcontext *gc);
void __fastcall __glGenericPickBufferProcs(__GLcontext *gc);
void __fastcall __glGenericPickCalcTextureProcs(__GLcontext *gc);
void __fastcall __glGenericPickClipProcs(__GLcontext *gc);
GLint __fastcall __glGenericPickDepthProcs(__GLcontext *gc);
void __fastcall __glGenericPickFogProcs(__GLcontext *gc);
void __fastcall __glGenericPickLineProcs(__GLcontext *gc, GLint unused);
void __fastcall __glGenericPickPixelProcs(__GLcontext *gc);
void __fastcall __glGenericPickRenderBitmapProcs(__GLcontext *gc);
void __fastcall __glGenericPickTextureProcs(__GLcontext *gc);
void __fastcall __glGenericPickTransformProcs(__GLcontext *gc);
void __fastcall __glGenericPickTriangleProcs(__GLcontext *gc, GLuint edx_unused);
void __fastcall __glGenericValidate(__GLcontext *gc, int arg);
void __stdcall __gllc_InvalidOperation(__GLcontext *gc);
void __stdcall __gllc_TableTooLarge(__GLcontext *gc);
const GLubyte *__fastcall __glle_Begin_LineLoop(const GLubyte *PC);
const GLubyte *__fastcall __glle_Begin_Lines(const GLubyte *PC);
const GLubyte *__fastcall __glle_Begin_LineStrip(const GLubyte *PC);
const GLubyte *__fastcall __glle_Begin_Points(const GLubyte *PC);
const GLubyte *__fastcall __glle_Begin_Polygon(const GLubyte *PC);
const GLubyte *__fastcall __glle_Begin_Quads(const GLubyte *PC);
const GLubyte *__fastcall __glle_Begin_QuadStrip(const GLubyte *PC);
const GLubyte *__fastcall __glle_Begin_TriangleFan(const GLubyte *PC);
const GLubyte *__fastcall __glle_Begin_Triangles(const GLubyte *PC);
const GLubyte *__fastcall __glle_Begin_TriangleStrip(const GLubyte *PC);
const GLubyte *__fastcall __glle_Nop(const GLubyte *PC);
const GLubyte *__fastcall __glle_Sentinel(void);
void __fastcall __glLockColorBufferDepthBuffer(__GLcontext *gc);
void __fastcall __glLockRenderBuffers(__GLcontext *gc);
void __fastcall __glLoseCurrentBuffers(__GLcontext *gc);
void __fastcall __glMakeCurrentBuffers(__GLcontext *gc);
void __fastcall __glNop4b(int a1, int a2, int a3, int a4);
void __fastcall __glSetError(GLenum code);
void __fastcall __glUnlockRenderBuffers(__GLcontext *gc);
void __fastcall __glUpdateDepthRange(__GLcontext *gc);
/* gl/gl_pickspan.c */
void __fastcall __glGenericPickSpanProcs(__GLcontext *gc);
/* gl/gl_pixfmt.c */
GLuint __fastcall __glScreenPixelLayout(HDC hdc, DWORD *redMask, DWORD *greenMask, DWORD *blueMask, DWORD *alphaMask);
/* gl/gl_point.c */
void __fastcall __glPointFastLock(__GLcontext *gc, __GLvertexICD *v);
void __fastcall __glPointVertex(__GLcontext *gc, __GLvertexICD *v);
void __fastcall __glPointVertexLock(__GLcontext *gc, __GLvertexICD *v);
void __fastcall __glRenderAliasedPoint1(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __glRenderAliasedPoint1_NoTex(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __glRenderAliasedPointN(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __glRenderAntiAliasedCIPoint(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __glRenderAntiAliasedRGBPoint(__GLcontext *gc, __GLvertexICD *vx);
/* gl/gl_polygon.c */
void __fastcall __gl3rdTrianglesVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __gl3rdTrianglesVertexLock(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __gl4thQStripVertex(__GLcontext *gc, __GLvertexICD *v3);
void __fastcall __gl4thQStripVertexLock(__GLcontext *gc, __GLvertexICD *v3);
void __fastcall __gl4thQuadsVertex(__GLcontext *gc, __GLvertexICD *v3);
void __fastcall __gl4thQuadsVertexLock(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __glEndPolygon(__GLcontext *gc, GLuint edx_unused);
void __fastcall __glEndPolygonLock(__GLcontext *gc, GLuint edx_unused);
void __fastcall __glEvenTStripVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __glEvenTStripVertexLock(__GLcontext *gc, __GLvertexICD *v0);
GLboolean __fastcall __glIndexTestSpan(__GLcontext *gc);
GLboolean __fastcall __glIndexTestStippledSpan(__GLcontext *gc);
void __fastcall __glMatValidateFan(__GLcontext *gc);
void __fastcall __glOddTStripVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __glOddTStripVertexLock(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __glOtherTFanVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __glOtherTFanVertexLock(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __glPolygonVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __glPolygonVertexLock(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall __glRenderFlatTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall __glRenderPointModeTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall __glRenderSmoothTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall __glRenderTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall __glRenderTriangleLineLock(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall __glSecondTFanVertex(__GLcontext *gc, __GLvertexICD *v);
void __fastcall __glSecondTStripVertex(__GLcontext *gc, __GLvertexICD *v);
GLboolean __fastcall __glShadeCISpan(__GLcontext *gc);
GLboolean __fastcall __glShadeRGBASpan(__GLcontext *gc);
GLboolean __fastcall __glTextureSpan(__GLcontext *gc);
GLboolean __fastcall __glTextureStippledSpan(__GLcontext *gc);
void __fastcall __glThirdQStripVertex(__GLcontext *gc, __GLvertexICD *v);
/* gl/gl_polyoffset.c */
void __stdcall __glim_CullParameterfvEXT(GLenum pname, const GLfloat *params);
/* gl/gl_pxapi.c */
void __fastcall __glInitPixelState(__GLcontext *gc);
void __fastcall __glSlowDrawPixelsStore(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
/* gl/gl_pxdrawpick.c */
void __fastcall __glSlowPickDrawPixels(__GLcontext *gc, GLint width, GLint height, GLenum format, GLenum type,
                                       const GLvoid *pixels, GLboolean packed);
/* gl/gl_pxfast.c */
void __fastcall __glOptPickDrawPixels(__GLcontext *gc, GLsizei width, GLsizei height, GLenum format, GLenum type,
                                      const GLvoid *pixels, GLboolean dlist);
void __fastcall __glOptPickRenderBitmapProcs(__GLcontext *gc);
/* gl/gl_pxreadpick.c */
void __fastcall __glDoNullStore(void);
void __fastcall __glSlowPickCopyPixels(__GLcontext *gc, GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void __fastcall __glSlowPickReadPixels(__GLcontext *gc, GLint x, GLint y, GLsizei width, GLsizei height,
                                       GLenum format, GLenum type, GLvoid *pixels);
/* gl/gl_rasterpos.c */
void __fastcall __glRasterPos2(__GLcontext *gc, const __GLfloat v[2]);
void __fastcall __glRasterPos3(__GLcontext *gc, const __GLfloat v[3]);
void __fastcall __glRasterPos4(__GLcontext *gc, const __GLfloat v[4]);
/* gl/gl_rgb.c */
void __fastcall __glInitRGB(__GLcolorBufferICD *cfb, __GLcontext *gc);
/* gl/gl_size.c */
GLint __fastcall __glLightModelfv_size(GLenum pname);
GLint __fastcall __glLightModeliv_size(GLenum pname);
/* gl/gl_stencil.c */
void __fastcall __glInitStencil8(__GLstencilBufferICD *sfb);
void __fastcall accum_Pick(void);
/* gl/gl_store.c */
void __fastcall __glGenericPickBlendProcs(__GLcontext *gc);
void __fastcall __glGenericPickStoreProcs(__GLcontext *gc);
void __fastcall __glReturnSpan(__GLcolorBufferICD *cfb, GLint x, GLint y, const __GLaccumCell *ac, __GLfloat scale,
                               GLint w);
void __fastcall __glValidateAlphaTest(__GLcontext *gc);
void __fastcall __glValidateIndexTest(__GLcontext *gc);
/* gl/gl_tex.c */
__GLfloat __fastcall __glComputePolygonRho(__GLcontext *gc, struct __GLshadeRhoRec *sh, __GLfloat s, __GLfloat t,
                                         __GLfloat qw);    /* sh: gl_tex.c's __GLshadeRho */
void __fastcall __glCalcMixedTexture(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __glCalcSphereMap(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __glCalcTexture(__GLcontext *gc, __GLvertexICD *vx);
__GLfloat __fastcall __glComputeLineRho(__GLcontext *gc, GLint unit, __GLfloat s, __GLfloat t, __GLfloat qw);
void __fastcall __glEarlyInitTextureState(__GLcontext *gc);
void __fastcall __glFastCalcTexturePersp(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __glFastTextureFragment(__GLcontext *gc, __GLcolor *color, __GLfloat s, __GLfloat t, __GLfloat rho);
void __fastcall __glFreeTextureManager(__GLcontext *gc);
void __fastcall __glFreeTextureState(__GLcontext *gc);
void __fastcall __glInitTextureState(__GLcontext *gc);
GLboolean __fastcall __glIsTextureConsistent(__GLcontext *gc, __GLtextureICD *tex, GLint unit);
void __fastcall __glMipMapFragment(__GLcontext *gc, __GLcolor *color, __GLfloat s, __GLfloat t, __GLfloat rho);
GLuint __fastcall __glTexParameterfv_size(GLenum e);
GLuint __fastcall __glTexTargetIndex(__GLcontext *gc, GLenum target, GLint unit);
void *__fastcall __glTextureCreateLevel(__GLcontext *gc, __GLtextureICD *tex, GLint lod, GLint components, GLsizei w,
                                        GLsizei h, GLsizei d, GLint border, GLint dim);
GLint __fastcall __glTextureCreateProxyLevel(__GLcontext *gc, __GLtextureICD *tex, GLint lod, GLint components,
                                             GLsizei w, GLsizei h, GLsizei d, GLint border, GLint dim);
void __fastcall __glTextureFragment(__GLcontext *gc, __GLcolor *color, __GLfloat s, __GLfloat t, __GLfloat rho);
void __fastcall __glTextureFragmentUVScale(__GLcontext *gc, __GLcolor *color, __GLfloat s, __GLfloat t,
                                           __GLfloat rho);
__GLtextureICD * __fastcall CheckTexImageArgs(__GLcontext *gc, GLenum target, GLint lod, GLint components,
                                              GLint border, GLenum format, GLenum type, GLint dim);
__GLtextureICD * __fastcall CheckTexSubImageArgs(__GLcontext *gc, GLenum target, GLint lod, GLenum format,
                                                 GLenum type, GLint dim);
void __fastcall texGenEyeLinearST(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall texGenObjectLinearST(__GLcontext *gc, __GLvertexICD *vx);
/* gl/gl_texcoord.c */
void __fastcall __glFreeTexture(__GLcontext *gc, __GLtextureICD *tex);
GLuint __fastcall __glMakeResidentTexture(__GLcontext *gc, __GLtextureICD *tex, GLuint arg, GLint unit);
void __fastcall __glShareTextureManager(__GLcontext *dst, __GLcontext *src);
void __fastcall __glTextureDeleteLevel(__GLcontext *gc, __GLtextureICD *tex, GLint lod);
GLboolean __fastcall __glUnmakeResidentTexture(__GLcontext *gc, __GLtextureICD *tex, GLint unit);
/* gl/gl_texextract.c */
void __fastcall __glFreeIndexTestTable(__GLcontext *gc);
void __fastcall __glRenderTriangleLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall __glRenderTrianglePoint(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall __glShareDlist(__GLcontext *dst, __GLcontext *src);
/* gl/gl_texstate.c */
void __fastcall __glCalcTexturePersp(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __glCalcTexturePerspUVScale(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __glCalcTextureUVScale(__GLcontext *gc, __GLvertexICD *vx);
__GLtextureICD *__fastcall __glCheckCompressedTexImage2DArgs(__GLcontext *gc, GLenum target, GLint lod,
                                                             GLint internalformat, GLsizei w, GLsizei h,
                                                             GLint border);
__GLtextureICD *__fastcall __glCheckCompressedTexSubImage2DArgs(__GLcontext *gc, GLenum target, GLint lod,
                                                                GLint xoffset, GLint yoffset, GLsizei w, GLsizei h,
                                                                GLenum format);
__GLtextureICD *__fastcall __glCheckCopyTexSubImageArgs(__GLcontext *gc, GLenum target, GLint lod, GLint xoffset,
                                                        GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei w,
                                                        GLsizei h, GLint dim);
__GLtextureICD *__fastcall __glCheckTexImage1DArgs(__GLcontext *gc, GLenum target, GLint lod, GLint components,
                                                   GLsizei length, GLint border, GLenum format, GLenum type);
__GLtextureICD *__fastcall __glCheckTexImage2DArgs(__GLcontext *gc, GLenum target, GLint lod, GLint components,
                                                   GLsizei w, GLsizei h, GLint border, GLenum format, GLenum type);
__GLtextureICD *__fastcall __glCheckTexSubImage1DArgs(__GLcontext *gc, GLenum target, GLint lod, GLint xoffset,
                                                      GLint length, GLenum format, GLenum type);
__GLtextureICD *__fastcall __glCheckTexSubImage2DArgs(__GLcontext *gc, GLenum target, GLint lod, GLint xoffset,
                                                      GLint yoffset, GLsizei w, GLsizei h, GLenum format,
                                                      GLenum type);
void __fastcall __glFastCalcTexturePerspUVScale(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __glFastCalcTextureUVScale(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __gllei_TexImage1D(__GLcontext *gc, GLenum target, GLint lod, GLint components, GLsizei length,
                                   GLint border, GLenum format, GLenum type, const GLubyte *image);
void __fastcall __gllei_TexImage2D(__GLcontext *gc, GLenum target, GLint lod, GLint components, GLsizei w, GLsizei h,
                                   GLint border, GLenum format, GLenum type, const GLubyte *image);
void __fastcall __gllei_TexSubImage1D(__GLcontext *gc, GLenum target, GLint lod, GLint xoffset, GLsizei length,
                                      GLenum format, GLenum type, const GLubyte *image);
void __fastcall __gllei_TexSubImage2D(__GLcontext *gc, GLenum target, GLint lod, GLint xoffset, GLint yoffset,
                                      GLsizei w, GLsizei h, GLenum format, GLenum type, const GLubyte *image);
void __fastcall __glShareTextureObjects(__GLcontext *gc, __GLcontext *shareMe);
/* gl/gl_varray.c */
void __stdcall __glim_ArrayElement(GLint i);
void __stdcall __glim_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void __stdcall __glim_DrawArrays(GLenum mode, GLint first, GLsizei count);
void __stdcall __glim_IndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void __stdcall __glim_NormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void __stdcall __glim_TexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void __fastcall __glInitVertexArrayState(__GLcontext *gc);
/* gl/gl_varrayext.c */
void __fastcall __glFreeVertexArrayState(__GLcontext *gc);
const GLubyte *__fastcall __glle_Disable(const GLubyte *PC);
const GLubyte *__fastcall __glle_Enable(const GLubyte *PC);
const GLubyte *__fastcall __glle_PolygonStipple(const GLubyte *PC);
const GLubyte *__fastcall __glle_TexImage1D(const GLubyte *PC);
const GLubyte *__fastcall __glle_TexImage2D(const GLubyte *PC);
const GLubyte *__fastcall __glle_TexSubImage1D(const GLubyte *PC);
const GLubyte *__fastcall __glle_TexSubImage2D(const GLubyte *PC);
void __fastcall DoCallList(GLuint list);
/* gl/gl_vertex.c */
GLuint __fastcall __glClipCheckAll(__GLcontext *gc, __GLvertexICD *vx);
GLuint __fastcall __glClipCheckFrustum2D(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __glGenericPickVertexProcs(__GLcontext *gc);
void __fastcall __glSaveCAllFog(__GLcontext *gc, __GLvertexICD *vx);
/* gl/gl_xform.c */
GLenum __fastcall __glCheckColorSubTableArgs(__GLcontext *gc, GLenum target, GLsizei start, GLsizei count,
                                             GLenum format, GLenum type);
void __fastcall __glDetectCPU(__GLcontext *gc);
void __fastcall __glFreeColorTables(void);
void __fastcall __glGenericPickInvTransposeProcs(__GLcontext *gc, __GLmatrixICD *m);
void __fastcall __glGenericPickMatrixProcs(__GLcontext *gc, __GLmatrixICD *m);
void __fastcall __glGenericPickMvpMatrixProcs(__GLcontext *gc, __GLmatrixICD *m);
void __fastcall __glInitColorTables(__GLcontext *gc);
void __fastcall __glInitTransformState(__GLcontext *gc);
void __fastcall __glLoadIdentityModelViewMatrix(__GLcontext *gc);
void __fastcall __glLoadIdentityProjectionMatrix(__GLcontext *gc);
void __fastcall __glLoadIdentityTextureMatrix(__GLcontext *gc);
void __fastcall __glPickMatrixType(__GLmatrixICD *dst, __GLmatrixICD *a, __GLmatrixICD *b);
void __fastcall __glPopModelViewMatrix(__GLcontext *gc);
void __fastcall __glPopProjectionMatrix(__GLcontext *gc);
void __fastcall __glPopTextureMatrix(__GLcontext *gc);
void __fastcall __glPushModelViewMatrix(__GLcontext *gc);
void __fastcall __glPushProjectionMatrix(__GLcontext *gc);
void __fastcall __glPushTextureMatrix(__GLcontext *gc);
double __fastcall __glReturnWindowY(__GLcontext *gc, __GLfloat y);
void __fastcall __glUpdateViewport(__GLcontext *gc);
void __fastcall __glValidateCullVertexFromModelView(__GLcontext *gc);
void __fastcall __glXForm4_Pre(__GLcoord *res, const __GLfloat v[4], const __GLmatrixICD *m);
void __fastcall SetDepthRange(__GLcontext *gc, GLuint edx_unused, GLdouble zNear, GLdouble zFar);
/* gl/gl_zfuncs.c */
__GLfloat __fastcall __glBuildAntiAliasIndex(__GLfloat index, __GLfloat antiAliasPercent);
GLint __fastcall __glElementsPerGroup(GLenum format, GLenum type);
void __fastcall __glRenderFlatFogPoint(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall Fetch_32A(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result);
/* gl/glcore_hand.c */
void __fastcall __glBuildPixelFormatList(HDC hdc, PIXELFORMATDESCRIPTOR *list, int n, PIXELFORMATDESCRIPTOR **out,
                                         int *outCount);
/* gl/glxform_asm.c */
void __fastcall __glI386NormalizeCoord(float *res, const float *v);
void __fastcall __glI386Vvec(float *res, const float *a, const float *b);
void __fastcall __glI386XForm3(float *res, const float *v, const float *m);
void __fastcall __glI386XForm3_W(float *res, const float *v, const float *m);
void __fastcall __glI386XForm4(float *res, const float *v, const float *m);
void __fastcall __glI386XForm4_W(float *res, const float *v, const float *m);
/* gl/od_clear8dither.c */
void __fastcall ClearRect_8_Dither(__GLcolorBufferICD *cfb, GLint x0, GLint y, GLint x1, GLint y1);
/* gl/og_pool.c */
void __fastcall __glMaskToBitsShift(GLuint mask, int *width, int *shift);
/* gl/og_util.c */
void __fastcall __glInitializeCodeCache(__GLcontext *gc);
void __fastcall __glReleaseCodeCache(__GLcontext *gc);
void __fastcall __glResetPixelFormats(int unused);
void __fastcall og_stream(void);
/* gl/si_dl_opt.c */
const GLubyte * __fastcall __glle_InvalidEnum(const GLubyte *PC);
const GLubyte * __fastcall __glle_InvalidOperation(const GLubyte *PC);
const GLubyte * __fastcall __glle_InvalidValue(const GLubyte *PC);
/* gl/si_px_api.c */
void __fastcall __glInitDefaultPixelMap(__GLcontext *gc, GLenum map);
void __fastcall __glPixelSetColorScales(__GLcontext *gc);
/* gl/si_s_capi.c */
void __stdcall __glim_Color3f(GLfloat r, GLfloat g, GLfloat b);
void __stdcall __glim_Color4ubv(const GLubyte v[4]);
/* gl/si_s_eval.c */
void __fastcall __glDoEvalCoord1(__GLcontext *gc, __GLfloat u);
void __fastcall __glDoEvalCoord2(__GLcontext *gc, __GLfloat u, __GLfloat v);
GLint __fastcall __glEvalComputeK(GLenum target);
/* gl/si_s_feed.c */
void __fastcall __glFeedbackPoint(__GLcontext *gc, __GLvertexICD *vx);
/* gl/si_s_fog.c */
void __fastcall __glFogColorSlow(__GLcontext *gc, __GLcolor *out, __GLcolor *in, __GLfloat fog);
/* gl/si_s_image.c */
void __fastcall __glConvertStipple(__GLcontext *gc);
/* gl/si_s_light.c */
void __fastcall __glClampAndScaleColorf(__GLcontext *gc, __GLcolor *d, const GLfloat s[4]);
void __fastcall __glClampAndScaleColori(__GLcontext *gc, __GLcolor *d, const GLint s[4]);
void __fastcall FogCIColor(__GLcontext *gc, GLint face, __GLvertexICD *vx);
void __fastcall FogLitCIColor(__GLcontext *gc, GLint face, __GLvertexICD *vx);
void __fastcall FogLitRGBColor(__GLcontext *gc, GLint face, __GLvertexICD *vx);
void __fastcall FogRGBColor(__GLcontext *gc, GLint face, __GLvertexICD *vx);
void __fastcall Vvec(__GLcoord *r, const __GLcoord *p1, const __GLcoord *p2);
/* gl/si_s_lnfin.c */
void __fastcall __gl1stLinesVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall FirstLLoopVertex(__GLcontext *gc, __GLvertexICD *v0);
/* gl/si_s_math.c */
void __fastcall __glInvertTransposeMatrix(__GLmatrixICD *inverse, const __GLmatrixICD *src);
void __fastcall __glMultMatrix(__GLmatrixICD *r, const __GLmatrixICD *a, const __GLmatrixICD *b);
void __fastcall __glNormalize(__GLfloat vout[3], const __GLfloat v[3]);
/* gl/si_s_names.c */
void __fastcall __glNamesUnlockData(__GLcontext *gc, void *data);
/* gl/si_s_pgfin.c */
void __fastcall SecondQuadsVertex(__GLcontext *gc, __GLvertexICD *v0);
void __fastcall SecondTrianglesVertex(__GLcontext *gc, __GLvertexICD *v0);
/* gl/si_s_pgmode.c */
void __fastcall __glDontRenderTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
/* gl/si_s_prim.c */
void __stdcall __glim_Begin(GLenum mode);
/* gl/si_s_rect.c */
void __fastcall __glRect(__GLcontext *gc, __GLfloat x0, __GLfloat y0, __GLfloat x1, __GLfloat y1);
/* gl/si_s_size.c */
GLint __fastcall __glFogfv_size(GLenum pname);
GLint __fastcall __glLightfv_size(GLenum pname);
GLint __fastcall __glMaterialfv_size(GLenum pname);
GLint __fastcall __glMaterialiv_size(GLenum pname);
/* gl/si_s_store.c */
void __fastcall __glDoDoubleStore(__GLcolorBufferICD *cfb, const __GLfragmentICD *frag);
/* gl/si_s_tex.c */
GLint __fastcall __glTexEnvfv_size(GLenum e);
GLint __fastcall __glTexEnviv_size(GLenum e);
GLint __fastcall __glTexGendv_size(GLenum e);
GLint __fastcall __glTexGenfv_size(GLenum e);
GLint __fastcall __glTexGeniv_size(GLenum e);
/* gl/si_s_xform.c */
void __fastcall __glComputeClipBox(__GLcontext *gc);
void __fastcall __glUpdateViewportTransform(__GLcontext *gc);
void __fastcall ReCompute2DMatrix(__GLcontext *gc, __GLmatrixICD *mvp);
/* sst/od_calcsize.c */
void __fastcall calc_size_and_scale(GLuint *isize, GLfloat *scale, GLenum type);
/* sst/od_clipkey.c */
void __fastcall og_clipInterpKey(__GLcontext *gc, GLuint *key);
/* sst/od_copycoord.c */
GLubyte *__fastcall CopyCoord(__GLcontext *gc, GLubyte *pc, GLenum type, GLuint size, GLint vtxOffset);
/* sst/od_makekey.c */
void __fastcall make_key(GLuint *key, __GLcontext *gc);
/* sst/od_vafill.c */
void __fastcall hwGenVertexArrayFill(__GLcontext *gc);
/* sst/od_whatneeds.c */
GLuint __fastcall what_needs(__GLcontext *gc);
/* sst/sst_buffers.c */
void __fastcall hwDrawQuad(__GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c, __GLvertexICD *d);
void __fastcall hwDrawTri(__GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall hwDrawTriPackColor(__GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall hwFinishFromCaller(int retaddr);
void __fastcall hwFreeGrState(__GLcontext *gc);
void __fastcall hwInitBufferHead(__GLbufferHeadICD *buf, __GLcontext *gc);
void __fastcall hwLoseBuffersCurrent(__GLcontext *gc);
void __fastcall hwMakeBuffersCurrent(__GLcontext *gc, GLuint edxUnused);
void __fastcall hwParseGlideVersion(__GLcontext *gc);
void __fastcall hwReadPixels(__GLcontext *gc, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
                             GLenum type, GLvoid *pixels);
/* sst/sst_clip.c */
void __fastcall __glSSTClipAndDrawLine(__GLcontext *gc, GLuint plane, __GLvertexICD *a, __GLvertexICD *b);
void __fastcall __glSSTClipAndDrawTriangle(__GLcontext *gc, GLuint edx_unused, __GLvertexICD *a, __GLvertexICD *b,
                                           __GLvertexICD *c);
/* sst/sst_codebuf.c */
void __fastcall __glsstim_End(void);
void __fastcall __grDrawQuad(GLuint ecxUnused, GLuint edxUnused, void *a, void *b, void *c, void *d);
void __fastcall hwAllocCodeBuffers(__GLcontext *gc);
void __fastcall hwDirtyAllGrState(__GLcontext *gc);
void __fastcall hwInitImmProcBuilder(__GLcontext *gc);
void __fastcall hwPageCacheFree(__GLcontext *gc, __GLhwBufRecICD *t);
int __fastcall hwPageCacheInit(__GLcontext *gc, GLuint edx_unused, __GLhwBufRecICD *t, GLint n, GLuint keySize,
                               GLuint addrA, GLuint addrB);
GLuint __fastcall hwPageCacheLookup(void *self, __GLhwBufRecICD *t, const void *key, GLuint *outB);
void __fastcall hwPageCacheTouch(__GLhwBufRecICD *t, __GLhwBufEntryICD **pe);
void __fastcall hwSetupVertexLayout(__GLcontext *gc);
/* sst/sst_color.c */
void __stdcall __glsstim_Color3f(GLfloat r, GLfloat g, GLfloat b);
void __stdcall __glsstim_Color3fv(const GLfloat *v);
void __stdcall __glsstim_Color4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
void __stdcall __glsstim_Color4fv(const GLfloat *v);
void __stdcall __glsstim_Color4ubv(const GLubyte *v);
void __fastcall hwPackColor(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall hwPackVertexColor(__GLcontext *gc, __GLvertexICD *vx);
GLuint __fastcall packColor255(__GLcontext *gc, const __GLcolor *c);
/* sst/sst_colorgen.c */
GLubyte *__fastcall og_genColor4f(__GLcontext *gc, GLubyte *pc, GLuint **pool);
/* sst/sst_combine.c */
void __fastcall __glSSTLoadCombineFunction(__GLcontext *gc, GLuint unused, GLenum unit);
void __fastcall __glSSTLoadCombineFunctionExt(__GLcontext *gc, int unused, int unused2);
void __fastcall hwLoadTexFilterWrap(__GLcontext *gc, __GLtexObjParamICD *params, GLint unit);
void __fastcall hwSetTmuSource(__GLcontext *gc, __GLtextureICD *tex, GLuint unit);
/* sst/sst_depth.c */
void __fastcall __glSSTInitDepth(__GLdepthBufferICD *fb, __GLcontext *gc);
/* sst/sst_glideinit.c */
GLboolean __fastcall hwGlideInit(void);
GLboolean __fastcall hwHasPixextExt(void);
GLboolean __fastcall hwHasSurfaceExt(void);
void __fastcall hwUpdateClipRegion(void);
/* sst/sst_grstate.c */
GLboolean __fastcall hwInitGrState(__GLcontext *gc);
/* sst/sst_immed.c */
void __stdcall __glsstim_Begin(GLenum mode);
void __fastcall hwEndLineLoop(__GLcontext *gc);
void __fastcall hwInstallInlineImmProcs(__GLcontext *gc);
void __stdcall hwLazyVertex2fv(const GLfloat *v);
void __stdcall hwLazyVertex3fv(const GLfloat *v);
void __stdcall hwLazyVertex4fv(const GLfloat *v);
void __fastcall hwTexCoords(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall hwTexCoordsXform0(__GLcontext *gc, __GLvertexICD *vx);
GLuint __fastcall hwVertexProcKey(__GLcontext *gc, GLuint *key, GLint prim);
void __fastcall hwXformTexCoords(__GLcontext *gc, __GLvertexICD *vx);
GLubyte *__fastcall og_texCoordXform(__GLcontext *gc, GLubyte *pc, int unit);
/* sst/sst_lazyprocs.c */
void __fastcall __glInitStencil32(__GLstencilBufferICD *sfb);
void __fastcall hwClassifyMatrix(__GLcontext *gc, __GLmatrixICD *m);
void __fastcall og_genContextCode(__GLcontext *gc);
void __fastcall og_initLazyProcs(__GLcontext *gc);
void __fastcall sstDepth_FreeBuf(__GLdepthBufferICD *dfb, __GLcontext *gc);
void __fastcall sstRGB_Fetch_32A(__GLcolorBufferICD *cfb, GLint x, GLint y, __GLcolor *result);
/* sst/sst_light.c */
void __fastcall hwCalcColorFront(__GLcontext *gc, __GLvertexICD *vx);
/* sst/sst_linepoint.c */
void __fastcall __glSSTRenderAntiAliasedFlatLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b);
void __fastcall __glSSTRenderAntiAliasedLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b);
void __fastcall __glSSTRenderLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b);
void __fastcall __glSSTRenderPoint(__GLcontext *gc, __GLvertexICD *v);
double __fastcall dotProduct3d(const __GLcoord *a, const __GLcoord *b);
void __fastcall hwInitClipInterp(__GLcontext *gc);
/* sst/sst_lockedtri.c */
void __fastcall hwGenLockedTriangles(__GLcontext *gc);
/* sst/sst_multitex.c */
GLenum __fastcall checkColorTableEXTArgs(__GLcontext *gc, GLenum target, GLenum internalFormat, GLsizei width,
                                         GLenum format, GLenum type);
GLuint __fastcall hwGetFaceNeeds0(__GLcontext *gc);
GLuint __fastcall hwGetFaceNeeds1(__GLcontext *gc);
GLuint __fastcall hwGetMaterialNeeds(__GLcontext *gc);
GLuint __fastcall hwGetModeFlags(__GLcontext *gc);
GLuint __fastcall hwGetNeeds(__GLcontext *gc);
void __fastcall hwMaskBufferWrites(__GLcontext *gc, GLuint mask);
GLubyte *__fastcall og_callRel32Tri(__GLcontext *gc, GLubyte *pc, GLuint routine);
/* sst/sst_pick.c */
void __fastcall __glSSTGenericPickTextureEnables(__GLcontext *gc);
void __fastcall __glSSTGenericPickTextureProcs(__GLcontext *gc, int unused);
void __fastcall computeModeFlags(__GLcontext *gc);
void __fastcall computeVertexNeeds(__GLcontext *gc);
void __fastcall hwPickCalcTextureProcs(__GLcontext *gc);
void __fastcall hwPickDrawRoutines(__GLcontext *gc);
void __fastcall hwPickFogProcs(__GLcontext *gc);
void __fastcall hwPickImmedProcs(__GLcontext *gc);
void __fastcall hwPickLineProcs(__GLcontext *gc);
void __fastcall hwPickPointProcs(__GLcontext *gc);
GLuint __fastcall hwPickRenderProcs(__GLcontext *gc, GLuint edx_unused);
void __fastcall hwPickTriangleProcs(__GLcontext *gc);
void __fastcall hwPickVertexArrayProcs(__GLcontext *gc);
void __fastcall hwPickVertexProcs(__GLcontext *gc);
/* sst/sst_primgen.c */
void __fastcall og_genRenderPrim(__GLcontext *gc, GLubyte *pc, GLuint prim, char bigPoint);
/* sst/sst_render.c */
void __fastcall __glSSTRenderFlatLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b);
void __fastcall __glSSTRenderFlatTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall __glSSTRenderSmoothTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall __glSSTRenderTriangle(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b, __GLvertexICD *c);
void __fastcall __glSSTRenderWideFlatLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b);
void __fastcall __glSSTRenderWideLine(__GLcontext *gc, __GLvertexICD *a, __GLvertexICD *b);
void __fastcall __glSSTRenderWidePoint(__GLcontext *gc, __GLvertexICD *v);
void __fastcall hwCullTriangleKey(__GLcontext *gc, GLuint *key);
void __fastcall hwDrawIndexedTriangles(__GLcontext *gc, GLint *indices, __GLvertexICD *base, GLint count);
int __fastcall hwPlaceSpriteTexture(__GLcontext *gc, GLuint edx_unused, int addr);
void __fastcall hwRenderTrianglePolygonModeLine(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1,
                                                __GLvertexICD *v2);
void __fastcall hwRenderTrianglePolygonModePoint(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1,
                                                 __GLvertexICD *v2);
/* sst/sst_rgb.c */
void __fastcall sstRGB_Init(__GLcolorBufferICD *cfb, __GLcontext *gc);
/* sst/sst_settings.c */
void __fastcall hwBuildGetStrings(__GLcontext *gc);
void __fastcall hwDefaultGuardBand(GLuint *def, RECT *rect);
void __fastcall hwReadDepthPrecision(__GLcontext *gc);
void __fastcall hwReadForce16bppTextures(__GLcontext *gc);
void __fastcall hwReadGuardBandTweaks(__GLcontext *gc);
void __fastcall hwReadObjectKeyGlow(__GLcontext *gc);
void __fastcall hwReadPerfSettings(__GLcontext *gc);
/* sst/sst_stencil.c */
void __fastcall __glSSTValidateStencil(__GLcontext *gc);
void __fastcall hwArrayPointerChanged(__GLcontext *gc);
void __fastcall hwValidateStencil(__GLcontext *gc);
GLuint __fastcall Stencil32_Fetch(__GLstencilBufferICD *sfb, GLint x, GLint y);
void __fastcall Stencil32_Store(__GLstencilBufferICD *sfb, GLint x, GLint y, GLint v);
/* sst/sst_tex.c */
void __fastcall __glSSTCalcMixedTexture(__GLcontext *gc, __GLvertexICD *vx);
void __fastcall __glSSTCalcMixedTextureUnit(__GLcontext *gc, __GLvertexICD *vx, GLint unit);
const __GLtextureFormatICD *__fastcall hwTexelFormat(__GLcontext *gc, GLenum internalFormat, GLenum *baseFormat);
/* sst/sst_texdl.c */
void __fastcall __glSSTCopyTexImage(__GLcontext *gc, void *spanInfo, __GLtextureICD *tex, GLint level);
void __fastcall hwTexDownloadRows(GLint tmu, GLuint startAddress, GLint level, GLint largeLod, GLint aspectRatio,
                                  GLint format, void *data, GLint t0, GLint t1);
GLubyte *__fastcall og_callRel32Locked(__GLcontext *gc, GLubyte *pc, GLuint routine);
/* sst/sst_texgen.c */
GLubyte *__fastcall og_windowTransform(__GLcontext *gc, GLubyte *pc);
/* sst/sst_texmgr.c */
void __fastcall __glSSTInitTextureManager(__GLcontext *gc);
void __fastcall texMgrFreeNode(__GLcontext *gc, __GLtexMemBlockICD *blk, GLuint tmu);
void __fastcall texMgrSetMemoryEnd(__GLcontext *gc, GLuint end);
/* sst/sst_tmumem.c */
void __fastcall __glSSTFreeTexObj(__GLcontext *gc, __GLtextureFxICD *tex);
void __fastcall __glSSTFreeTextureMemory(__GLcontext *gc, __GLtextureFxICD *tex, GLuint tmu);
void __fastcall __glSSTTexReleaseLevel(__GLcontext *gc, __GLtextureFxICD *tex, GLint level);
GLubyte *__fastcall og_callAbs(__GLcontext *gc, GLubyte *pc, GLuint target);
GLubyte *__fastcall og_genClipAndDrawQuad(__GLcontext *gc, GLubyte *pc);
GLubyte *__fastcall og_genClipEdge3DNow(__GLcontext *gc, GLubyte *pc);
GLubyte *__fastcall og_genClipEdgeSSE(__GLcontext *gc, GLubyte *pc);
GLubyte *__fastcall og_genClipEdgeX87(__GLcontext *gc, GLubyte *pc);
GLubyte *__fastcall og_genVertexTransform(__GLcontext *gc, GLubyte *pc);
void __fastcall texMgrCoalesce(__GLcontext *gc, __GLtexMemBlockICD *blk, GLuint tmu);
void __fastcall texMgrDebugPrintf(void);
__GLtexMemBlockICD *__fastcall texMgrSplit(__GLcontext *gc, __GLtexMemBlockICD *blk, GLint size);
GLboolean __fastcall UnmakeResident(__GLcontext *gc, __GLtextureFxICD *tex, GLuint tmu);
/* sst/sst_tribuild.c */
GLubyte *__fastcall hwBuildImmProc(__GLcontext *gc, GLuint n);
GLubyte *__fastcall hwBuildIndexedTriangles(__GLcontext *gc, int unused);
GLubyte *__fastcall hwGenClipTriangle(__GLcontext *gc, GLubyte *pc);
/* sst/sst_trigen.c */
void __fastcall hwFifoStateKey(__GLcontext *gc, GLuint *key);
/* sst/sst_validate.c */
void __fastcall __glSSTRevalidate(__GLcontext *gc, int arg);
void __fastcall __glSSTValidate(__GLcontext *gc);
void __fastcall __glSSTValidateBlend(__GLcontext *gc);
void __fastcall __glSSTValidateWindowed(__GLcontext *gc);
void __fastcall hwDirtyTexCombine(__GLcontext *gc);
GLboolean __fastcall hwIsTextureDrawable(__GLcontext *gc, __GLtextureICD *tex, GLint unit);
void __fastcall hwLoadAllState(__GLcontext *gc);
void __fastcall hwValidateDrawBuffer(__GLcontext *gc, GLuint edx_unused);
void __fastcall hwValidateFinish(__GLcontext *gc);
void __fastcall hwValidateTextures(__GLcontext *gc);
/* sst/sst_varray.c */
GLboolean __fastcall hwGrowVertexArrays(__GLcontext *gc, GLint n);
void __fastcall hwPrepareVertexCache(__GLcontext *gc, GLuint unused, GLint first, GLsizei count,
                                     const GLuint *indices);
GLboolean __fastcall sstVarray_AllocateVbuf(__GLcontext *gc, GLint first, GLint count);
/* sst/sst_vtxcopy.c */
GLubyte *__fastcall hwCopyTexCoords(__GLcontext *gc, GLubyte *pc);
GLubyte *__fastcall hwGenTexCoords(__GLcontext *gc, GLubyte *pc);
GLubyte *__fastcall og_genMatrixRow(__GLcontext *gc, GLubyte *pc, GLint row, GLuint *m, GLuint mask);
/* sst/ssthand.c */
void __fastcall hwPixelFormatCaps(__GLcontext *gc, GLuint *out);
void __fastcall winFreeDrawableBuffers(__WGLwindow *w);
/* sst/sststate.c */
void __fastcall __glSSTPickPixelProcs(__GLcontext *gc);
GLint __fastcall hwCanUseFastPath(__GLcontext *gc, GLuint edx);
void __fastcall hwDefaultCombine(__GLcontext *gc);
void __fastcall hwDefaultCombineExt(__GLcontext *gc);
void __fastcall hwLoadColorAlphaCombine(__GLcontext *gc, GLint cFunc, GLint cFactor, GLint cLocal, GLint cOther,
                                        GLint aFunc, GLint aFactor, GLint aLocal, GLint aOther);
void __fastcall hwLoadColorAlphaCombineExt(__GLcontext *gc, GLint ca, GLint caMode, GLint cb, GLint cbMode, GLint cc,
                                           GLint ccInvert, GLint cd, GLint cdInvert, GLint aa, GLint aaMode, GLint ab,
                                           GLint abMode, GLint ac, GLint acInvert, GLint ad, GLint adInvert);
void __fastcall hwLoadTexAlphaCombineExt(__GLcontext *gc, GLint unit, GLint a, GLint aMode, GLint b, GLint bMode,
                                         GLint c, GLint cInvert, GLint d, GLint dInvert, GLint shift);
void __fastcall hwLoadTexColorCombineExt(__GLcontext *gc, GLint unit, GLint a, GLint aMode, GLint b, GLint bMode,
                                         GLint c, GLint cInvert, GLint d, GLint dInvert, GLint shift);
void __fastcall hwLoadTexCombine(__GLcontext *gc, GLint unit, GLint rgbFunc, GLint rgbFactor);
void __fastcall hwLoadTmuConstantColor(__GLcontext *gc, GLint unit, GLuint color);
void __fastcall hwValidateSphereMapInverse(__GLcontext *gc);
/* sst/sstsurf.c */
void __fastcall hwInitDDrawWindow(__WGLwindow *w, const __WGLmodes *m);
void __fastcall hwPickColorProcs(__GLcontext *gc);
GLuint __fastcall PrimaryMakeCurrent(__GLcontext *gc);
GLboolean __fastcall winPresent(__GLcontext *gc);
GLboolean __fastcall winReadBack(__GLcontext *gc);
void __fastcall winReleaseSurfaceContext(__GLcontext *gc);
GLboolean __fastcall winRestoreSurfaces(__WGLdrawable *dp, __GLcontext *gc);
void __fastcall winSetBackSurface(__GLcontext *gc, int edx_unused);
void __fastcall winSetFrontSurface(__GLcontext *gc, int edx_unused);
/* wgl/wgldev.c */
void __fastcall wglDevForceSWRendering(__GLcontext *gc, void *arg);
/* wgl/wglext3dfx.c */
void wglReloadGammaRamp(void);
/* wgl/wglfont.c */
BOOL WINAPI wglSAUseFontBitmapsA(HDC hdc, DWORD first, DWORD count, DWORD listBase);
BOOL WINAPI wglSAUseFontBitmapsW(HDC hdc, DWORD first, DWORD count, DWORD listBase);
BOOL WINAPI wglSAUseFontOutlinesA(HDC hdc, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion,
                                  int format, LPGLYPHMETRICSFLOAT gmf);
BOOL WINAPI wglSAUseFontOutlinesW(HDC hdc, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion,
                                  int format, LPGLYPHMETRICSFLOAT gmf);
/* wgl/wglglide.c */
GLint __fastcall wglGlideDescribePixelFormat(HDC hdc, PIXELFORMATDESCRIPTOR *out, int index);
void __fastcall wglGlideLose(HWND hwnd);
void __fastcall wglGlideRestore(HWND hwnd);
void __fastcall wglGlideTexKey(__GLcontext *gc, BYTE *texObj);
/* wgl/wglhook.c */
LRESULT CALLBACK wglMonitorWindowChanges(int code, WPARAM wParam, LPARAM lParam);
/* wgl/wglstand.c */
int WINAPI wglSAChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd);
BOOL WINAPI wglSACopyContext(HGLRC src, HGLRC dst, UINT mask);
HGLRC WINAPI wglSACreateContext(HDC hdc);
HGLRC WINAPI wglSACreateLayerContext(HDC hdc, int layer);
BOOL WINAPI wglSADeleteContext(HGLRC hrc);
BOOL WINAPI wglSADescribeLayerPlane(HDC hdc, int fmt, int layer, UINT n, LPLAYERPLANEDESCRIPTOR lpd);
int WINAPI wglSADescribePixelFormat(HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd);
HGLRC WINAPI wglSAGetCurrentContext(void);
HDC WINAPI wglSAGetCurrentDC(void);
PROC WINAPI wglSAGetDefaultProcAddress(LPCSTR name);
int WINAPI wglSAGetLayerPaletteEntries(HDC hdc, int layer, int start, int n, COLORREF *c);
int WINAPI wglSAGetPixelFormat(HDC hdc);
PROC WINAPI wglSAGetProcAddress(LPCSTR name);
BOOL WINAPI wglSAMakeCurrent(HDC hdc, HGLRC hrc);
BOOL WINAPI wglSARealizeLayerPalette(HDC hdc, int layer, BOOL realize);
int WINAPI wglSASetLayerPaletteEntries(HDC hdc, int layer, int start, int n, CONST COLORREF *c);
BOOL WINAPI wglSASetPixelFormat(HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR *ppfd);
BOOL WINAPI wglSAShareLists(HGLRC a, HGLRC b);
BOOL WINAPI wglSASwapBuffers(HDC hdc);
BOOL WINAPI wglSASwapLayerBuffers(HDC hdc, UINT planes);
/* wgl/wglutil.c */
void wglClearAllWindowFlags(void);
void __fastcall wglGetDesktopSize(LONG *w, LONG *h);
/* ---- end of hoisted prototypes ---- */

#endif

/* the pixel-path functions take glsi.h's __GLpixelSpanInfoICD: declared once glsi.h is in (a file that includes
** this header before glsi.h gets them from a second #include "icdprotos.h" after it) */
#if defined(GLSI_H) && !defined(ICDPROTOS_SI_H)
#define ICDPROTOS_SI_H
void __fastcall NopSpanRender(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, const GLvoid *inspan);    /* si_px_paths.c */
void __fastcall __glInitColorTableStore(__GLcolorTableICD *ct, __GLpixelSpanInfoICD *spanInfo);
void __fastcall __glLoadUnpackModes(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLboolean packed);
void __fastcall __glLoadPackModes(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo);
void __fastcall __glInitMemUnpack(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLsizei width, GLsizei height,
                                GLsizei depth, GLenum format, GLenum type, const GLvoid *buf, GLboolean dltex);
void __fastcall __glInitTexImageStore(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, __GLtextureICD *tex, GLint lod);
void __fastcall __glInitTexSourceUnpack(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLsizei width, GLsizei height,
                                GLsizei depth, GLenum format, GLenum type, const GLvoid *buf, GLboolean packed);
void __fastcall __glInitTexSubImageStore(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, __GLtextureICD *tex, GLint lod,
                                GLint xoffset, GLint yoffset, GLint zoffset);

#endif

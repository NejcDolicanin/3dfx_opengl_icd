/*
** siprotos.h -- the prototypes whose parameter types come from glsi.h (span info, texels, arenas, display lists),
** one per function, written from its definition, like icdprotos.h's hoisted block.  They are not in icdprotos.h
** because not every user of that header includes glsi.h.
** Include right after glsi.h.
*/
#ifndef SIPROTOS_H
#define SIPROTOS_H

#include "glsitypes.h"      /* __GLpixelSpanModInfo */

/* ---- hoisted prototypes: one per function, written from its definition ---- */
/* gl/gl_dlist.c */
__GLdlistICD * __fastcall __glGenericDlistCompiler(__GLcontext *gc, __GLcompiledDlist *compDlist);
/* gl/gl_feedback.c */
__GLnamesArray *__fastcall __glNamesNewArray(__GLcontext *gc, __GLnamesArrayType type);
/* gl/gl_lexec.c */
void * __fastcall __glArenaAlloc(__GLarena *arena, GLuint size);
void __fastcall __glArenaFreeAll(__GLarena *arena);
void __fastcall __glDeleteArena(__GLarena *arena);
__GLarena * __fastcall __glNewArena(__GLcontext *gc);
/* gl/gl_pick.c */
void __fastcall __glGenericExecuteDlist(__GLcontext *gc, __GLdlistICD *dlist);
void __fastcall __glNopExecuteDlist(__GLcontext *gc, __GLdlistICD *dlist);
void __fastcall __glNopFreeDlist(__GLcontext *gc, __GLdlistICD *dlist);
/* gl/gl_pxapi.c */
void __fastcall __glSpanReadDepth(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLvoid *span);
void __fastcall __glSpanReadDepth2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLvoid *span);
/* gl/gl_pxpaths.c */
void __fastcall __glGenericFetchTexImage(__GLcontext *gc, __GLtextureICD *image, __GLpixelSpanInfoICD *spanInfo,
                                         GLboolean applyPixelTransfer);
void __fastcall __glGenericPickCopyImage(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                         GLboolean applyPixelTransfer);
void __fastcall __glGenericPickCopyPixels(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo);
void __fastcall __glGenericPickReadPixels(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo);
void __fastcall __glGenericReadTexImage(__GLcontext *gc, __GLtextureICD *image, __GLpixelSpanInfoICD *spanInfo);
void __fastcall __glGenericStoreTexImage(__GLcontext *gc, __GLtextureICD *image, __GLpixelSpanInfoICD *spanInfo,
                                         GLboolean applyPixelTransfer);
void __fastcall __glInitCopyPixelsInfo(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLint x, GLint y, GLint width,
                                       GLint height, GLenum format);
void __fastcall __glInitDrawPixelsInfo(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLint width, GLint height,
                                       GLenum format, GLenum type, const GLvoid *pixels);
void __fastcall __glInitReadImageSrcInfo(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLint x, GLint y,
                                         GLsizei width, GLsizei height);
void __fastcall __glInitReadPixelsInfo(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLint x, GLint y, GLint width,
                                       GLint height, GLenum format, GLenum type, GLvoid *pixels);
/* gl/gl_pxpick.c */
void __fastcall __glGenericPickDrawPixels(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo);
void __fastcall PickSpanModifiers(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, __GLpixelSpanModInfo *spanModInfo);
/* gl/gl_tex.c */
void __fastcall __glLinearFilter(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod, __GLfloat s, __GLfloat t);
void __fastcall __glLinearFilter1(__GLtextureICD *tex, __GLmipMapLevelICD *lp, __GLfloat s, __GLfloat t,
                                  __GLtexel *result);
void __fastcall __glLMLFilter(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod, __GLfloat s, __GLfloat t);
void __fastcall __glNearestFilter(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod, __GLfloat s, __GLfloat t);
void __fastcall __glNearestFilter1(__GLtextureICD *tex, __GLmipMapLevelICD *lp, __GLfloat s, __GLfloat t,
                                   __GLtexel *result);
void __fastcall __glNearestFilter2(__GLtextureICD *tex, __GLmipMapLevelICD *lp, __GLfloat s, __GLfloat t,
                                   __GLtexel *result);
void __fastcall __glNMLFilter(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod, __GLfloat s, __GLfloat t);
/* gl/gl_texcoord.c */
void __fastcall __glCopyTexImage(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, __GLtextureICD *tex, GLint lod);
void __fastcall __glReadTexImage(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, __GLtextureICD *tex, GLint lod);
/* gl/gl_texextract.c */
void __fastcall __glLinearFilterUVScaled(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod, __GLfloat s,
                                         __GLfloat t);
void __fastcall __glNearestFilterUVScaled(__GLtextureICD *tex, __GLtexel *result, __GLfloat lod, __GLfloat s,
                                          __GLfloat t);
/* gl/gl_texstate.c */
void __fastcall __glInitTexDestPack(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLsizei width, GLsizei height,
                                    GLsizei depth, GLenum format, GLenum type, GLvoid *buf);
void __fastcall __glInitTexImageGet(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, __GLtextureICD *tex, GLint lod);
/* gl/gl_xform.c */
void __fastcall __glComputeInverseTranspose(__GLcontext *gc, __GLtransformICD *tr);
/* gl/glcore_hand.c */
void __fastcall __glLinearFilter2(__GLtextureICD *tex, __GLmipMapLevelICD *lp, GLfloat s, GLfloat t,
                                  __GLtexel *result);
/* gl/si_px_read.c */
void __fastcall __glSpanReadCI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLvoid *span);
void __fastcall __glSpanReadCI2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLvoid *span);
void __fastcall __glSpanReadRGBA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLvoid *span);
void __fastcall __glSpanReadRGBA2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLvoid *span);
void __fastcall __glSpanReadStencil(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLvoid *span);
void __fastcall __glSpanReadStencil2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, GLvoid *span);
/* gl/si_px_rend.c */
void __fastcall __glSpanRenderCI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, const GLvoid *span);
void __fastcall __glSpanRenderCI2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, const GLvoid *span);
void __fastcall __glSpanRenderDepth(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, const GLvoid *span);
void __fastcall __glSpanRenderDepth2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, const GLvoid *span);
void __fastcall __glSpanRenderRGBA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, const GLvoid *span);
void __fastcall __glSpanRenderRGBA2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, const GLvoid *span);
void __fastcall __glSpanRenderStencil(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, const GLvoid *span);
void __fastcall __glSpanRenderStencil2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, const GLvoid *span);
/* gl/si_s_names.c */
void * __fastcall __glNamesLockData(__GLcontext *gc, __GLnamesArray *array, GLuint name);
/* sst/sst_multitex.c */
void __fastcall __glSSTInitColorTableStore(__GLcolorTableICD *ct, __GLpixelSpanInfoICD *spanInfo);
/* ---- end of hoisted prototypes ---- */

#endif

/*
** gl_texcoord.c -- original VA range 690c0000..690c4000: s_select.c glInitNames / glPopName, the
** glTexCoord* entries (3dfx S_TAPI.C: through state.current.activeTexture, the four-component
** forms report the q to procs.nonUnityQ), glBitmap, and s_texmgr.c texture manager and texture
** object procs.
** Then the rest of s_tapi.c, in address order.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "wglpriv.h"
#include <glide.h>
#include "glsi.h"
#include "icdprotos.h"
#include "glsitypes.h"
#include "si_names.h"
#include "icdglob.h"

typedef void (__fastcall *__GLfreeProcC00)(__GLcontext *gc, void *p);

/* 690c0c80 -- __glim_InitNames (s_select.c) */
void __fastcall __glim_InitNames()
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->renderMode == GL_SELECT) {
        gc->select.hit = GL_FALSE;
        gc->select.sp = gc->select.stack;
    }
}

/* 690c0d60 -- __glim_PopName (s_select.c) */
void __fastcall __glim_PopName()
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->renderMode == GL_SELECT) {
        if (gc->select.sp == gc->select.stack) {
            __glSetError(GL_STACK_UNDERFLOW);
            return;
        }
        gc->select.hit = GL_FALSE;
        gc->select.sp = gc->select.sp - 1;
    }
}

/* 690c18d0 -- __glim_TexCoord1f */
void __stdcall __glim_TexCoord1f(GLfloat x)
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x;
    tc->y = __GL_ZERO_F;
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c1900 -- __glim_TexCoord1i */
void __stdcall __glim_TexCoord1i(GLint x)
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = (__GLfloat)x;
    tc->y = __GL_ZERO_F;
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c1970 -- __glim_TexCoord1dv */
void __stdcall __glim_TexCoord1dv(const GLdouble x[1])
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = (__GLfloat)x[0];
    tc->y = __GL_ZERO_F;
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c19b0 -- __glim_TexCoord1fv */
void __stdcall __glim_TexCoord1fv(const GLfloat x[1])
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x[0];
    tc->y = __GL_ZERO_F;
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c1ab0 -- __glim_TexCoord2f */
void __stdcall __glim_TexCoord2f(GLfloat x, GLfloat y)
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x;
    tc->y = y;
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c1af0 -- __glim_TexCoord2i */
void __stdcall __glim_TexCoord2i(GLint x, GLint y)
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = (__GLfloat)x;
    tc->y = (__GLfloat)y;
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c1b30 -- __glim_TexCoord2s */
void __stdcall __glim_TexCoord2s(GLshort x, GLshort y)
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = (__GLfloat)x;
    tc->y = (__GLfloat)y;
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c1b80 -- __glim_TexCoord2dv */
void __stdcall __glim_TexCoord2dv(const GLdouble x[2])
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = (__GLfloat)x[0];
    tc->y = (__GLfloat)x[1];
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c1bc0 -- __glim_TexCoord2fv */
void __stdcall __glim_TexCoord2fv(const GLfloat x[2])
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x[0];
    tc->y = x[1];
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c1cd0 -- __glim_TexCoord3f */
void __stdcall __glim_TexCoord3f(GLfloat x, GLfloat y, GLfloat z)
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x;
    tc->y = y;
    tc->z = z;
    tc->w = __GL_ONE_F;
}

/* 690c1d10 -- __glim_TexCoord3i */
void __stdcall __glim_TexCoord3i(GLint x, GLint y, GLint z)
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = (__GLfloat)x;
    tc->y = (__GLfloat)y;
    tc->z = (__GLfloat)z;
    tc->w = __GL_ONE_F;
}

/* 690c1d50 -- __glim_TexCoord3s */
void __stdcall __glim_TexCoord3s(GLshort x, GLshort y, GLshort z)
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = (__GLfloat)x;
    tc->y = (__GLfloat)y;
    tc->z = (__GLfloat)z;
    tc->w = __GL_ONE_F;
}

/* 690c1da0 -- __glim_TexCoord3dv */
void __stdcall __glim_TexCoord3dv(const GLdouble x[3])
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = (__GLfloat)x[0];
    tc->y = (__GLfloat)x[1];
    tc->z = (__GLfloat)x[2];
    tc->w = __GL_ONE_F;
}

/* 690c1de0 -- __glim_TexCoord3fv */
void __stdcall __glim_TexCoord3fv(const GLfloat x[3])
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x[0];
    tc->y = x[1];
    tc->z = x[2];
    tc->w = __GL_ONE_F;
}

/* 690c1ef0 -- __glim_TexCoord4f */
void __stdcall __glim_TexCoord4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    __GL_SETUP();
    __GLcoord *tc = gc->state.current.activeTexture;

    tc->x = x;
    tc->y = y;
    tc->z = z;
    tc->w = w;
    (*gc->procs.nonUnityQ)(gc, tc);
}

/* 690c1f30 -- __glim_TexCoord4i */
void __stdcall __glim_TexCoord4i(GLint x, GLint y, GLint z, GLint w)
{
    __GL_SETUP();
    __GLcoord *tc = gc->state.current.activeTexture;

    tc->x = (__GLfloat)x;
    tc->y = (__GLfloat)y;
    tc->z = (__GLfloat)z;
    tc->w = (__GLfloat)w;
    (*gc->procs.nonUnityQ)(gc, tc);
}

/* 690c1f70 -- __glim_TexCoord4s */
void __stdcall __glim_TexCoord4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    __GL_SETUP();
    __GLcoord *tc = gc->state.current.activeTexture;

    tc->x = (__GLfloat)x;
    tc->y = (__GLfloat)y;
    tc->z = (__GLfloat)z;
    tc->w = (__GLfloat)w;
    (*gc->procs.nonUnityQ)(gc, tc);
}

/* 690c1fd0 -- __glim_TexCoord4dv */
void __stdcall __glim_TexCoord4dv(const GLdouble x[4])
{
    __GL_SETUP();
    __GLcoord *tc = gc->state.current.activeTexture;

    tc->x = (__GLfloat)x[0];
    tc->y = (__GLfloat)x[1];
    tc->z = (__GLfloat)x[2];
    tc->w = (__GLfloat)x[3];
    (*gc->procs.nonUnityQ)(gc, tc);
}

/* 690c2010 -- __glim_TexCoord4fv */
void __stdcall __glim_TexCoord4fv(const GLfloat x[4])
{
    __GL_SETUP();
    __GLcoord *tc = gc->state.current.activeTexture;

    tc->x = x[0];
    tc->y = x[1];
    tc->z = x[2];
    tc->w = x[3];
    (*gc->procs.nonUnityQ)(gc, tc);
}

/* 690c2050 -- __glim_TexCoord4iv */
void __stdcall __glim_TexCoord4iv(const GLint x[4])
{
    __GL_SETUP();
    __GLcoord *tc = gc->state.current.activeTexture;

    tc->x = (__GLfloat)x[0];
    tc->y = (__GLfloat)x[1];
    tc->z = (__GLfloat)x[2];
    tc->w = (__GLfloat)x[3];
    (*gc->procs.nonUnityQ)(gc, tc);
}

/* 690c2090 -- __glim_TexCoord4sv */
void __stdcall __glim_TexCoord4sv(const GLshort x[4])
{
    __GL_SETUP();
    __GLcoord *tc = gc->state.current.activeTexture;

    tc->x = (__GLfloat)x[0];
    tc->y = (__GLfloat)x[1];
    tc->z = (__GLfloat)x[2];
    tc->w = (__GLfloat)x[3];
    (*gc->procs.nonUnityQ)(gc, tc);
}

/* 690c3220 -- __glim_Bitmap (s_bitmap.c).  ICD: when a validation is pending it validates and then
** re-enters through the glBitmap dispatch stub.
*/
void __stdcall __glim_Bitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
                            GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
    GLint beginMode;
    __GL_SETUP();

    beginMode = __gl_beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
        if (beginMode == __GL_NEED_VALIDATE) {
            (*gc->procs.validate)(gc);
            __gl_beginMode = __GL_NOT_IN_BEGIN;
            glBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);
            return;
        }
        __glSetError(GL_INVALID_OPERATION);
        return;
    }
    if (width < 0 || height < 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    (*gc->procs.revalidate)(gc);
    (*gc->procs.waitIdle)(gc);
    (*gc->procs.bitmap)(gc, width, height, xorig, yorig, xmove, ymove, bitmap);
}

/* 690c36a0 -- __glInvalidateAllTextures (s_texmgr.c, the texture manager's proc): just say OK */
GLboolean __fastcall __glInvalidateAllTextures()
{
    return GL_TRUE;
}

/* 690c36b0 -- __glShareTextureManager (s_texmgr.c) */
void __fastcall __glShareTextureManager(__GLcontext *dst, __GLcontext *src)
{
    /* first free the texture manager of the dst context, then share the src one */
    __glFreeTextureManager(dst);

    dst->texMgr = src->texMgr;
    dst->texMgr->refs++;
}

/* 690c3820 -- __glFreeTexture (s_texmgr.c; tex->free) */
void __fastcall __glFreeTexture(__GLcontext *gc, __GLtextureICD *tex)
{
    GLint level, maxLevel;

    maxLevel = gc->constants.maxMipMapLevel;
    for (level = 0; level < maxLevel; level++) {
        (*tex->releaseLevel)(gc, tex, level);
    }
    (*(__GLfreeProcC00)gc->imports[3])(gc, tex->level[0]);
    (*(__GLfreeProcC00)gc->imports[3])(gc, tex->level);
    (*(__GLfreeProcC00)gc->imports[3])(gc, tex);
}

/* 690c3870 -- __glCopyTexImage (s_texmgr.c; tex->copyTexImage): the resident copy is out of date */
void __fastcall __glCopyTexImage(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, __GLtextureICD *tex, GLint lod)
{
    (*gc->procs.copyImage)(gc, spanInfo, GL_TRUE);

    /* resident texture is out of date.  remove it */
    (*tex->unmakeResident)(gc, tex, gc->fx.activeTexture);
}

/* 690c3890 -- __glReadTexImage (s_texmgr.c; tex->readTexImage): readImage gets the texture itself */
void __fastcall __glReadTexImage(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, __GLtextureICD *tex, GLint lod)
{
    (*gc->buffers.lockBuffers)(gc, gc->buffers.readBufferMask);
    (*gc->procs.readImage)(gc, tex, spanInfo);
    (*gc->buffers.unlockBuffers)(gc, gc->buffers.readBufferMask);

    /* resident texture is out of date.  remove it */
    (*tex->unmakeResident)(gc, tex, gc->fx.activeTexture);
}

/* 690c38e0 -- __glMakeResidentTexture (s_texmgr.c; tex->makeResident): loaded */
GLuint __fastcall __glMakeResidentTexture(__GLcontext *gc, __GLtextureICD *tex, GLuint arg, GLint unit)
{
    return 1;       /* __GL_TEXTURE_RESIDENCE_LOADED */
}

/* 690c38f0 -- __glUnmakeResidentTexture (s_texmgr.c; tex->unmakeResident) */
GLboolean __fastcall __glUnmakeResidentTexture(__GLcontext *gc, __GLtextureICD *tex, GLint unit)
{
    return GL_TRUE;
}

/* 690c3d40 -- __glTextureDeleteLevel (s_texmgr.c; tex->releaseLevel) */
void __fastcall __glTextureDeleteLevel(__GLcontext *gc, __GLtextureICD *tex, GLint lod)
{
    __GLmipMapLevelICD *lp = tex->level[lod];

    if (lp->buffer) {
        (*(__GLfreeProcC00)gc->imports[3])(gc, lp->buffer);
        lp->buffer = NULL;
    }

    if (lp->pixelBuffer) {
        (*(__GLfreeProcC00)gc->imports[3])(gc, lp->pixelBuffer);
        lp->pixelBuffer = NULL;
    }
}

/* 690c3d80 -- StoreNEVER16 (s_depth.c, entry 0 of the depth store table 690f5264) */
GLboolean __fastcall StoreNEVER16(__GLdepthBufferICD *fb, GLint x, GLint y, __GLzValue z)
{
    return GL_FALSE;
}

/* 690c1930 -- __glim_TexCoord1s: glTexCoord1s */
void __stdcall __glim_TexCoord1s(GLshort x)
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x;
    tc->y = __GL_ZERO_F;
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c19f0 -- __glim_TexCoord1iv: glTexCoord1iv */
void __stdcall __glim_TexCoord1iv(const GLint x[1])
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x[0];
    tc->y = __GL_ZERO_F;
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/************************************************************************/
/* s_vapi.c texture coordinates (through gc->state.current.activeTexture) */

/* 690c1a30 -- __glim_TexCoord1sv: glTexCoord1sv */
void __stdcall __glim_TexCoord1sv(const GLshort x[1])
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x[0];
    tc->y = __GL_ZERO_F;
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c1c00 -- __glim_TexCoord2iv: glTexCoord2iv */
void __stdcall __glim_TexCoord2iv(const GLint x[2])
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x[0];
    tc->y = x[1];
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c1c40 -- __glim_TexCoord2sv: glTexCoord2sv */
void __stdcall __glim_TexCoord2sv(const GLshort x[2])
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x[0];
    tc->y = x[1];
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c1e20 -- __glim_TexCoord3iv: glTexCoord3iv */
void __stdcall __glim_TexCoord3iv(const GLint x[3])
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x[0];
    tc->y = x[1];
    tc->z = x[2];
    tc->w = __GL_ONE_F;
}

/* 690c1e60 -- __glim_TexCoord3sv: glTexCoord3sv */
void __stdcall __glim_TexCoord3sv(const GLshort x[3])
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x[0];
    tc->y = x[1];
    tc->z = x[2];
    tc->w = __GL_ONE_F;
}

/* 690c18a0 -- __glim_TexCoord1d (s_tapi.c, through the active unit's coordinate) */
void __stdcall __glim_TexCoord1d(GLdouble x)
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x;
    tc->y = __GL_ZERO_F;
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c1a70 -- __glim_TexCoord2d */
void __stdcall __glim_TexCoord2d(GLdouble x, GLdouble y)
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x;
    tc->y = y;
    tc->z = __GL_ZERO_F;
    tc->w = __GL_ONE_F;
}

/* 690c1c90 -- __glim_TexCoord3d */
void __stdcall __glim_TexCoord3d(GLdouble x, GLdouble y, GLdouble z)
{
    __GLcoord *tc = __glCurrentGC()->state.current.activeTexture;

    tc->x = x;
    tc->y = y;
    tc->z = z;
    tc->w = __GL_ONE_F;
}

/* 690c1eb0 -- __glim_TexCoord4d: a q other than 1 goes through procs.nonUnityQ */
void __stdcall __glim_TexCoord4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    __GL_SETUP();
    __GLcoord *tc = gc->state.current.activeTexture;

    tc->x = x;
    tc->y = y;
    tc->z = z;
    tc->w = w;
    (*gc->procs.nonUnityQ)(gc, tc);
}

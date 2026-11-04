/*
** gl_bitmap.c -- s_btdraw.c: bitmap drawing.
*/
#include <string.h>
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "si_names.h"
#include "icdprotos.h"
#include "glshade.h"
#include <math.h>

typedef void *(__fastcall *__GLmallocProc)(__GLcontext *gc, size_t size);
#define IMPORTS_MALLOC(gc)  ((__GLmallocProc)(gc)->imports[0])
/************************************************************************/
/* s_btdraw.c */
typedef void (__fastcall *__GLfreeProc)(__GLcontext *gc, void *p);
#define IMPORTS_FREE(gc)    ((__GLfreeProc)(gc)->imports[3])
typedef void (__fastcall *__GLrenderBitmapProc)(__GLcontext *gc, const __GLbitmap *bitmap, const GLubyte *data);

/* 690976f0 -- __glDrawBitmap */
void __fastcall __glDrawBitmap(__GLcontext *gc, GLsizei width, GLsizei height,
                    GLfloat xOrig, GLfloat yOrig,
                    GLfloat xMove, GLfloat yMove,
                    const GLubyte oldbits[])
{
    __GLbitmap bitmap;

    bitmap.width = width;
    bitmap.height = height;
    bitmap.xorig = xOrig;
    bitmap.yorig = yOrig;
    bitmap.xmove = xMove;
    bitmap.ymove = yMove;

    if (width > 0 && height > 0) {
        GLubyte *newbits;

        newbits = (GLubyte *) (*IMPORTS_MALLOC(gc))(gc, (size_t)
                                __glImageSize(width, height, GL_COLOR_INDEX, GL_BITMAP));

        __glFillImage(gc, width, height, GL_COLOR_INDEX, GL_BITMAP,
                      oldbits, newbits);

        (*(__GLrenderBitmapProc)gc->procs.renderBitmap)(gc, &bitmap, newbits);

        (*IMPORTS_FREE(gc))(gc, newbits);
    } else {
        (*(__GLrenderBitmapProc)gc->procs.renderBitmap)(gc, &bitmap, NULL);
    }
}

#define LOCK_BUFFERS(gc, m)   (((void (__fastcall *)(__GLcontext *, GLuint))(gc)->buffers.lockBuffers)(gc, m))
#define UNLOCK_BUFFERS(gc, m) (((void (__fastcall *)(__GLcontext *, GLuint))(gc)->buffers.unlockBuffers)(gc, m))

/************************************************************************/


/* 690977a0: __glRenderBitmap (s_btdraw.c); unit 0's texture coordinate only, fog by eye z or by the
** fog coordinate */
void __fastcall __glRenderBitmap(__GLcontext *gc, const __GLbitmap *bitmap, const GLubyte *data)
{
    __GLfragmentICD frag;
    __GLvertexICD *rp;
    __GLfloat fx;
    GLint x, y, bit;
    GLint ySign;
    GLuint modeFlags = ((GLuint (__fastcall *)(__GLcontext *))gc->polygon.shader.getModeFlags)(gc);
    GLint x0 = gc->transform.clipX0;
    GLint x1 = gc->transform.clipX1;
    GLint y0 = gc->transform.clipY0;
    GLint y1 = gc->transform.clipY1;

    ySign = gc->constants.ySign;

    rp = &gc->state.current.rasterPos;
    if (!gc->state.current.validRasterPos) {
	return;
    }

    if (gc->renderMode == GL_SELECT) {
	rp->window.x += bitmap->xmove;
	rp->window.y += ySign * bitmap->ymove;
	return;
    }

    if (gc->renderMode == GL_FEEDBACK) {
	__glFeedbackBitmap(gc, rp);
	/*
	** Advance the raster position as if the bitmap had been rendered.
	*/
	rp->window.x += bitmap->xmove;
	rp->window.y += (__GLfloat)(ySign * bitmap->ymove);   /* (cast: keeps this tail apart from GL_SELECT's) */
	return;
    }

    frag.color.r = rp->color->r * gc->constants.redRescale;
    frag.color.g = rp->color->g * gc->constants.greenRescale;
    frag.color.b = rp->color->b * gc->constants.blueRescale;
    frag.color.a = rp->color->a * gc->constants.alphaRescale;
    if (modeFlags & __GL_SHADE_TEXTURE) {
	__GLfloat rhow = __GL_ONE_F / rp->texture[0].w;
	((void (__fastcall *)(__GLcontext *, __GLcolor *, __GLfloat, __GLfloat, __GLfloat))
	    gc->procs.textureRaster[0])(gc, &frag.color, rp->texture[0].x * rhow,
					rp->texture[0].y * rhow, __GL_ONE_F);
    }
    if (gc->state.enables.general & __GL_FOG_ENABLE) {
	if (gc->state.fog.coordSource == GL_FRAGMENT_DEPTH_EXT) {
	    ((void (__fastcall *)(__GLcontext *, __GLfragmentICD *, __GLfloat))
		gc->procs.fogPoint)(gc, &frag, rp->eye.z);
	} else {
	    ((void (__fastcall *)(__GLcontext *, __GLfragmentICD *, __GLfloat))
		gc->procs.fogPoint)(gc, &frag, rp->fogCoord);
	}
    }

    LOCK_BUFFERS(gc, gc->buffers.renderBufferMask);

    fx = (__GLfloat) (GLint) (rp->window.x - bitmap->xorig);
    frag.y = (GLint) (rp->window.y - ySign * bitmap->yorig);
    frag.z = (GLuint) (rp->window.z * gc->constants.depthRescale);

    bit = 7;
    for (y = 0; y < bitmap->height; y++) {
	frag.x = (GLint) fx;
	for (x = 0; x < bitmap->width; x++) {
	    if (y0 <= frag.y && frag.y < y1 && x0 <= frag.x && frag.x < x1) {
		if (*data & (1<<bit)) {
		    ((void (__fastcall *)(__GLcolorBufferICD *, __GLfragmentICD *))
			gc->procs.store)(gc->drawBuffer, &frag);
		}
	    }
	    frag.x++;
	    bit--;
	    if (bit < 0) {
		bit = 7;
		data++;
	    }
	}
	frag.y += ySign;
	if (bit != 7) {
	    bit = 7;
	    data++;
	}
    }

    UNLOCK_BUFFERS(gc, gc->buffers.renderBufferMask);

    /*
    ** Advance current raster position.
    */
    rp->window.x += bitmap->xmove;
    rp->window.y += ySign * bitmap->ymove;
}

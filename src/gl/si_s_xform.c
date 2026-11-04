/*
** si_s_xform.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_xform.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 69080420 -- __glim_LoadIdentity */
void __stdcall __glim_LoadIdentity(void)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();
    (*gc->procs.loadIdentity)(gc);
}

/* 69080980 -- __glim_PushMatrix */
void __stdcall __glim_PushMatrix(void)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();
    (*gc->procs.pushMatrix)(gc);
}

/* 69081060 -- __glim_Scissor */
void __stdcall __glim_Scissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    GLuint uwidth, uheight;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    if ((width < 0) || (height < 0)) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }

    uwidth = (GLuint) width;
    uheight = (GLuint) height;

    /* return if nothing changed */
    if ((x == gc->state.scissor.scissorX) &&
        (y == gc->state.scissor.scissorY) &&
        (uwidth == gc->state.scissor.scissorWidth) &&
        (uheight == gc->state.scissor.scissorHeight)) {
        return;
    }

    gc->state.scissor.scissorX = x;
    gc->state.scissor.scissorY = y;
    gc->state.scissor.scissorWidth = uwidth;
    gc->state.scissor.scissorHeight = uheight;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_SCISSOR);
    (*gc->procs.computeClipBox)(gc);
    (*gc->procs.applyScissor)(gc);
}

/* 69081c70 -- __glComputeClipBox */
void __fastcall __glComputeClipBox(__GLcontext *gc)
{
    __GLscissor *sp = &gc->state.scissor;
    GLint llx, lly, urx, ury;
    GLint y0, y1;

    if (gc->state.enables.general & __GL_SCISSOR_TEST_ENABLE) {
        llx = sp->scissorX;
        lly = sp->scissorY;
        urx = llx + sp->scissorWidth;
        ury = lly + sp->scissorHeight;

        if ((urx < 0) || (ury < 0) ||
            (urx <= llx) || (ury <= lly) ||
            (llx >= (GLint)gc->constants.width) || 
            (lly >= (GLint)gc->constants.height)) {
            llx = lly = urx = ury = 0;
        } else {
            if (llx < 0) llx = 0;
            if (lly < 0) lly = 0;
            if (urx > (GLint)gc->constants.width) urx = gc->constants.width;
            if (ury > (GLint)gc->constants.height) ury = gc->constants.height;
        }
    } else {
        llx = 0;
        lly = 0;
        urx = gc->constants.width;
        ury = gc->constants.height;
    }

    if (gc->constants.yInverted) {
        y0 = gc->constants.height - ury;
        y1 = gc->constants.height - lly;
    } else {
        y0 = lly;
        y1 = ury;
    }

    (*gc->drawablePrivate->setClipRect)(gc->drawablePrivate,
                                        llx, y0, urx-llx, y1-y0);

#if defined(__GL_ALIGNED_BUFFERS)
    gc->transform.clipX0 = llx + gc->constants.viewportXAdjustOffset;
    gc->transform.clipY0 = y0 + gc->constants.viewportYAdjustOffset;
    gc->transform.clipX1 = urx + gc->constants.viewportXAdjustOffset;
    gc->transform.clipY1 = y1 + gc->constants.viewportYAdjustOffset;
#else /* __GL_ALIGNED_BUFFERS */
    gc->transform.clipX0 = llx + gc->constants.viewportXAdjust;
    gc->transform.clipY0 = y0 + gc->constants.viewportYAdjust;
    gc->transform.clipX1 = urx + gc->constants.viewportXAdjust;
    gc->transform.clipY1 = y1 + gc->constants.viewportYAdjust;
#endif /* __GL_ALIGNED_BUFFERS */
}

/* 690803b0 -- __glim_MatrixMode */
void __stdcall __glim_MatrixMode(GLenum mode)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    switch (mode) {
      case GL_MODELVIEW:
      case GL_PROJECTION:
      case GL_TEXTURE:
#if defined(__GL_SUPPORT_ARB_IMAGING_SUBSET)
      case GL_COLOR:
#endif
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    gc->state.transform.matrixMode = mode;
    (*gc->procs.pickTransformProcs)(gc);
}

/* 690809c0 -- __glim_PopMatrix */
void __stdcall __glim_PopMatrix(void)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();
    (*gc->procs.popMatrix)(gc);
}

/* 69080e30 -- __glReturnWindowX */
__GLfloat __fastcall __glReturnWindowX(__GLcontext *gc, __GLfloat x)
{
#if defined(__GL_HALF_PIXEL_OFFSET)
    x -= __glHalf;
#endif /* __GL_HALF_PIXEL_OFFSET */
#if defined(__GL_ALIGNED_BUFFERS)
    return (x - gc->constants.fviewportXAdjustOffset);
#else /* __GL_ALIGNED_BUFFERS */
    return (x - gc->constants.fviewportXAdjust);
#endif /* __GL_ALIGNED_BUFFERS */
}

/* 69080150 -- __glEarlyInitTransformState */
void __fastcall __glEarlyInitTransformState(__GLcontext *gc)
{
    GLint numClipPlanes;

    /* Allocate memory for clip planes */
    numClipPlanes = gc->constants.numberOfClipPlanes;
    gc->state.transform.eyeClipPlanes = (__GLcoord *)
        ((void *(__fastcall *)(__GLcontext *, size_t, size_t))gc->imports[1])(gc, (size_t) numClipPlanes, sizeof(__GLcoord));
}

/* 69080d80 -- __glUpdateViewportTransform */
void __fastcall __glUpdateViewportTransform(__GLcontext *gc)
{
    /* 
    ** Now that the implementation may have found us a new window size,
    ** we compute these offsets...
    */
#if defined(__GL_ALIGNED_BUFFERS)
    gc->transform.minx = gc->state.viewport.x + 
        gc->constants.viewportXAdjustOffset;
#else /* __GL_ALIGNED_BUFFERS */
    gc->transform.minx = gc->state.viewport.x + gc->constants.viewportXAdjust;
#endif /* __GL_ALIGNED_BUFFERS */
    gc->transform.maxx = gc->transform.minx + gc->state.viewport.width;
    gc->transform.fminx = gc->transform.minx;
    gc->transform.fmaxx = gc->transform.maxx;

#if defined(__GL_ALIGNED_BUFFERS)
    gc->transform.miny = (gc->constants.height - 
                          (gc->state.viewport.y + gc->state.viewport.height)) + 
        gc->constants.viewportYAdjustOffset;
#else /* __GL_ALIGNED_BUFFERS */
    gc->transform.miny = (gc->constants.height - 
                          (gc->state.viewport.y + gc->state.viewport.height)) + 
        gc->constants.viewportYAdjust;
#endif /* __GL_ALIGNED_BUFFERS */
    gc->transform.maxy = gc->transform.miny + gc->state.viewport.height;
    gc->transform.fminy = gc->transform.miny;
    gc->transform.fmaxy = gc->transform.maxy;

#if defined(__GL_HALF_PIXEL_OFFSET)
    gc->transform.fminx += __glHalf;
    gc->transform.fmaxx += __glHalf;

    gc->transform.fminy += __glHalf;
    gc->transform.fmaxy += __glHalf;
#endif /* __GL_HALF_PIXEL_OFFSET */

    /*
    ** Pickers that notice when the transformation matches the viewport
    ** exactly need to be revalidated.  Ugh.
    */
    (*gc->procs.pickMvpMatrixProcs)(gc, &gc->transform.modelView->mvp);
}

/* 69082170 -- __glXForm2_W */
void __fastcall __glXForm2_W(__GLcoord *res, const __GLfloat v[2], const __GLmatrixICD *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];

    res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
    res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
    res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + m->matrix[3][2];
    res->w = ((__GLfloat) 1.0);
}

/* 690821c0 -- __glXForm3_W */
void __fastcall __glXForm3_W(__GLcoord *res, const __GLfloat v[3], const __GLmatrixICD *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];

    res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0]
        + m->matrix[3][0];
    res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1]
        + m->matrix[3][1];
    res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2]
        + m->matrix[3][2];
    res->w = ((__GLfloat) 1.0);
}

/* 69082220 -- __glXForm4_W */
void __fastcall __glXForm4_W(__GLcoord *res, const __GLfloat v[4], const __GLmatrixICD *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];
    __GLfloat w = v[3];
  
    if (w == ((__GLfloat) 1.0)) {
        res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0]
            + m->matrix[3][0];
        res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1]
            + m->matrix[3][1];
        res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2]
            + m->matrix[3][2];
    } else {
        res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0]
            + w*m->matrix[3][0];
        res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1]
            + w*m->matrix[3][1];
        res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2]
            + w*m->matrix[3][2];
    }
    res->w = w;
}

/* 69082330 -- __glXForm2_2DW */
void __fastcall __glXForm2_2DW(__GLcoord *res, const __GLfloat v[2], 
                    const __GLmatrixICD *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];

    res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
    res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
    res->z = m->matrix[3][2];
    res->w = ((__GLfloat) 1.0);
}

/* 69082370 -- __glXForm3_2DW */
void __fastcall __glXForm3_2DW(__GLcoord *res, const __GLfloat v[3], 
                    const __GLmatrixICD *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];

    res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
    res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
    res->z = z*m->matrix[2][2] + m->matrix[3][2];
    res->w = ((__GLfloat) 1.0);
}

/* 690823c0 -- __glXForm4_2DW */
void __fastcall __glXForm4_2DW(__GLcoord *res, const __GLfloat v[4], 
                    const __GLmatrixICD *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];
    __GLfloat w = v[3];

    if (w == ((__GLfloat) 1.0)) {
        res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
        res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
        res->z = z*m->matrix[2][2] + m->matrix[3][2];
    } else {
        res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + w*m->matrix[3][0];
        res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + w*m->matrix[3][1];
        res->z = z*m->matrix[2][2] + w*m->matrix[3][2];
    }
    res->w = w;
}

/* 69082480 -- __glXForm2_2DNRW */
void __fastcall __glXForm2_2DNRW(__GLcoord *res, const __GLfloat v[2], 
                      const __GLmatrixICD *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];

    res->x = x*m->matrix[0][0] + m->matrix[3][0];
    res->y = y*m->matrix[1][1] + m->matrix[3][1];
    res->z = m->matrix[3][2];
    res->w = ((__GLfloat) 1.0);
}

/* 690824b0 -- __glXForm3_2DNRW */
void __fastcall __glXForm3_2DNRW(__GLcoord *res, const __GLfloat v[3], 
                      const __GLmatrixICD *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];

    res->x = x*m->matrix[0][0] + m->matrix[3][0];
    res->y = y*m->matrix[1][1] + m->matrix[3][1];
    res->z = z*m->matrix[2][2] + m->matrix[3][2];
    res->w = ((__GLfloat) 1.0);
}

/* 690824f0 -- __glXForm4_2DNRW */
void __fastcall __glXForm4_2DNRW(__GLcoord *res, const __GLfloat v[4], 
                      const __GLmatrixICD *m)
{
    __GLfloat x = v[0];
    __GLfloat y = v[1];
    __GLfloat z = v[2];
    __GLfloat w = v[3];

    if (w == ((__GLfloat) 1.0)) {
        res->x = x*m->matrix[0][0] + m->matrix[3][0];
        res->y = y*m->matrix[1][1] + m->matrix[3][1];
        res->z = z*m->matrix[2][2] + m->matrix[3][2];
    } else {
        res->x = x*m->matrix[0][0] + w*m->matrix[3][0];
        res->y = y*m->matrix[1][1] + w*m->matrix[3][1];
        res->z = z*m->matrix[2][2] + w*m->matrix[3][2];
    }
    res->w = w;
}

/* 690825e0 -- ReCompute2DMatrix */
void __fastcall ReCompute2DMatrix(__GLcontext *gc, __GLmatrixICD *mvp)
{
    __GLviewport *vp;
    __GLmatrixICD *m;

    if (mvp->matrixType >= __GL_MT_IS2D) {
        m = &(gc->transform.matrix2D);
        vp = &(gc->state.viewport);
        m->matrix[0][0] = mvp->matrix[0][0] * vp->scale.x;
        m->matrix[0][1] = mvp->matrix[0][1] * vp->scale.y;
        m->matrix[1][0] = mvp->matrix[1][0] * vp->scale.x;
        m->matrix[1][1] = mvp->matrix[1][1] * vp->scale.y;
        m->matrix[2][2] = mvp->matrix[2][2];
        m->matrix[3][0] = mvp->matrix[3][0] * vp->scale.x + vp->center.x;
        m->matrix[3][1] = mvp->matrix[3][1] * vp->scale.y + vp->center.y;
        m->matrix[3][2] = mvp->matrix[3][2];
        m->matrix[3][3] = 1.0;
        m->matrixType = mvp->matrixType;
    }
}

/* 69080460 -- __glim_LoadMatrixf */
void __stdcall __glim_LoadMatrixf(const GLfloat m[16])
{
    __GLmatrixICD m1;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    m1.matrix[0][0] = m[0];
    m1.matrix[0][1] = m[1];
    m1.matrix[0][2] = m[2];
    m1.matrix[0][3] = m[3];
    m1.matrix[1][0] = m[4];
    m1.matrix[1][1] = m[5];
    m1.matrix[1][2] = m[6];
    m1.matrix[1][3] = m[7];
    m1.matrix[2][0] = m[8];
    m1.matrix[2][1] = m[9];
    m1.matrix[2][2] = m[10];
    m1.matrix[2][3] = m[11];
    m1.matrix[3][0] = m[12];
    m1.matrix[3][1] = m[13];
    m1.matrix[3][2] = m[14];
    m1.matrix[3][3] = m[15];
    m1.matrixType = __GL_MT_GENERAL;
    __glDoLoadMatrix(gc, &m1);
}

/* 69080520 -- __glim_LoadMatrixd */
void __stdcall __glim_LoadMatrixd(const GLdouble m[16])
{
    __GLmatrixICD m1;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_STATE();

    m1.matrix[0][0] = m[0];
    m1.matrix[0][1] = m[1];
    m1.matrix[0][2] = m[2];
    m1.matrix[0][3] = m[3];
    m1.matrix[1][0] = m[4];
    m1.matrix[1][1] = m[5];
    m1.matrix[1][2] = m[6];
    m1.matrix[1][3] = m[7];
    m1.matrix[2][0] = m[8];
    m1.matrix[2][1] = m[9];
    m1.matrix[2][2] = m[10];
    m1.matrix[2][3] = m[11];
    m1.matrix[3][0] = m[12];
    m1.matrix[3][1] = m[13];
    m1.matrix[3][2] = m[14];
    m1.matrix[3][3] = m[15];
    m1.matrixType = __GL_MT_GENERAL;
    __glDoLoadMatrix(gc, &m1);
}

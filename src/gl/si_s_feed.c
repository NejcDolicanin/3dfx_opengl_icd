/*
** si_s_feed.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_feed.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 690c1080 -- __glim_FeedbackBuffer */
void __stdcall __glim_FeedbackBuffer(GLsizei bufferLength, GLenum type, GLfloat buffer[])
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((type < GL_2D) || (type > GL_4D_COLOR_TEXTURE)) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    if (bufferLength < 0) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    if (gc->renderMode == GL_FEEDBACK) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }

    __GL_API_BLAND();

    gc->feedback.resultBase = buffer;
    gc->feedback.result = buffer;
    gc->feedback.resultLength = bufferLength;
    gc->feedback.overFlowed = GL_FALSE;
    gc->feedback.type = type;
}

/* 690c11b0 -- __glFeedbackTag */
void __fastcall __glFeedbackTag(__GLcontext *gc, GLfloat f)
{
    if (!gc->feedback.overFlowed) {
        if (gc->feedback.result >=
                    gc->feedback.resultBase + gc->feedback.resultLength) {
            gc->feedback.overFlowed = GL_TRUE;
        } else {
            gc->feedback.result[0] = f;
            gc->feedback.result = gc->feedback.result + 1;
        }
    }
}

/* 690c1140 -- __glim_PassThrough */
void __stdcall __glim_PassThrough(GLfloat element)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_BLAND();

    if (gc->renderMode == GL_FEEDBACK) {
        __glFeedbackTag(gc, GL_PASS_THROUGH_TOKEN);
        __glFeedbackTag(gc, element);
    }
}

/* 690c14a0 -- __glFeedbackPoint */
void __fastcall __glFeedbackPoint(__GLcontext *gc, __GLvertexICD *vx)
{
    __glFeedbackTag(gc, GL_POINT_TOKEN);
    Feedback(gc, vx);
}

/* 690c1480 -- __glFeedbackBitmap */
void __fastcall __glFeedbackBitmap(__GLcontext *gc, __GLvertexICD *vx)
{
    __glFeedbackTag(gc, GL_BITMAP_TOKEN);
    Feedback(gc, vx);
}

/* 690c1200 -- __glFeedbackCopyPixels */
void __fastcall __glFeedbackCopyPixels(__GLcontext *gc, __GLvertexICD *vx)
{
    __glFeedbackTag(gc, GL_COPY_PIXEL_TOKEN);
    Feedback(gc, vx);
}

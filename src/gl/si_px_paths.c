/*
** si_px_paths.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/px_paths.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 69088720 -- __glClipReadPixels */
GLboolean __fastcall __glClipReadPixels(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    GLint clipLeft, clipRight, clipTop, clipBottom;
    GLint x,y,x2,y2;
    GLint skipPixels, skipRows;
    GLint width, height;
    GLint tempint;

    width = spanInfo->width;
    height = spanInfo->height;
    x = (GLint) spanInfo->readX;
    y = (GLint) spanInfo->readY;
    x2 = x + spanInfo->width;
    if (gc->constants.yInverted) {
        y2 = y - spanInfo->height;
    } else {
        y2 = y + spanInfo->height;
    }
#if defined(__GL_ALIGNED_BUFFERS)
    clipLeft = gc->constants.viewportXAdjustOffset;
    clipRight = gc->constants.width + gc->constants.viewportXAdjustOffset;
    clipBottom = gc->constants.viewportYAdjustOffset;
    clipTop = gc->constants.height + gc->constants.viewportYAdjustOffset;
#else /* __GL_ALIGNED_BUFFERS */
    clipLeft = gc->constants.viewportXAdjust;
    clipRight = gc->constants.width + gc->constants.viewportXAdjust;
    clipBottom = gc->constants.viewportYAdjust;
    clipTop = gc->constants.height + gc->constants.viewportYAdjust;
#endif /* __GL_ALIGNED_BUFFERS */
    skipPixels = 0;
    skipRows = 0;
    if (x < clipLeft) {
        skipPixels = clipLeft - x;
        if (skipPixels > width) return GL_FALSE;

        width -= skipPixels;
        x = clipLeft;
        spanInfo->dstSkipPixels += skipPixels;
        spanInfo->readX = (__GLfloat) x;
    }
    if (x2 > clipRight) {
        tempint = x2 - clipRight;
        if (tempint > width) return GL_FALSE;

        width -= tempint;
    }
    if (gc->constants.yInverted) {
        if (y >= clipTop) {
            skipRows = y - clipTop + 1;
            if (skipRows > height) return GL_FALSE;

            height -= skipRows;
            y = clipTop - 1;
            spanInfo->dstSkipLines += skipRows;
            spanInfo->readY = (__GLfloat) y;
        }
        if (y2 < clipBottom - 1) {
            tempint = clipBottom - y2 - 1;
            if (tempint > height) return GL_FALSE;

            height -= tempint;
        }
    } else {
        if (y < clipBottom) {
            skipRows = clipBottom - y;
            if (skipRows > height) return GL_FALSE;

            height -= skipRows;
            y = clipBottom;
            spanInfo->dstSkipLines += skipRows;
            spanInfo->readY = (__GLfloat) y;
        }
        if (y2 > clipTop) {
            tempint = y2 - clipTop;
            if (tempint > height) return GL_FALSE;

            height -= tempint;
        }
    }

    spanInfo->width = width;
    spanInfo->height = height;

    return GL_TRUE;
}

/* 69088c20 -- __glReadPixels0 */
void __fastcall __glReadPixels0(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    GLint i, ySign;
    GLint height;
    void (*reader)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                   GLvoid *outspan);

    reader = spanInfo->spanReader;

    ySign = gc->constants.ySign;
    height = spanInfo->height;

    for (i=0; i<height; i++) {
        (*reader)(gc, spanInfo, spanInfo->dstCurrent);
        spanInfo->dstCurrent = (GLvoid *) ((GLubyte *) spanInfo->dstCurrent +
                spanInfo->dstRowIncrement);
        spanInfo->readY += ySign;
    }
}

/* 6908a3a0 -- __glCopyImage1 */
void __fastcall __glCopyImage1(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    GLint i;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                  const GLvoid *inspan, GLvoid *outspan);

    height = spanInfo->height;
    span1  = spanInfo->spanModifier[0];
    for (i=0; i<height; i++) {
        (*span1)(gc, spanInfo, spanInfo->srcCurrent, spanInfo->dstCurrent);
        spanInfo->srcCurrent = (const GLubyte *) spanInfo->srcCurrent + 
                spanInfo->srcRowIncrement;
        spanInfo->dstCurrent = (GLubyte *) spanInfo->dstCurrent +
                spanInfo->dstRowIncrement;
    }
}

/* 6907da30 -- NopSpanRender */
void __fastcall NopSpanRender(__GLcontext *gc,
                __GLpixelSpanInfoICD *spanInfo, const GLvoid *inspan)
{
}

/* 690861a0 -- __glComputeSpanPixelArray */
void __fastcall __glComputeSpanPixelArray(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    GLint width, intx;
    __GLfloat zoomx, oldx, newx;
    GLint i;
    GLshort *array;
    
    zoomx = spanInfo->zoomx;
    if (zoomx > (__GLfloat) -1.0 && zoomx < __GL_ONE_F) {
        GLint lasti;

        /* Build pixel skip array */
        width = spanInfo->width;
        oldx = spanInfo->x;
        array = spanInfo->pixelArray;

        intx = (GLint) oldx;
        newx = oldx;

        lasti = 0;
        for (i=0; i<width; i++) {
            /* Skip groups which will not be rasterized */
            newx += zoomx;
            while ((GLint) newx == intx && i<width) {
                newx += zoomx;
                i++;
            }
            assert (i != width);
            if (i != 0) {
                *array++ = (GLshort) (i - lasti);
            }
            lasti = i;
            intx = (GLint) newx;
        }
        *array++ = 1;
    } else if (zoomx < (__GLfloat) -1.0 || zoomx > __GL_ONE_F) {
        __GLfloat right;
        GLint iright;
        GLint coladd, column;
        GLint startCol;

        /* Build pixel replication array */
        width = spanInfo->width - 1;
        startCol = spanInfo->startCol;
        column = startCol;
        coladd = spanInfo->coladd;
        array = spanInfo->pixelArray;
        right = spanInfo->x;
        for (i=0; i<width; i++) {
            right = right + zoomx;
            iright = (GLint) right;
            *array++ = (GLshort) (iright - column);
            column = iright;
        }
        if (coladd == 1) {
            *array++ = (GLshort) (spanInfo->columns - (column - startCol));
        } else {
            *array++ = (GLshort) ((startCol - column) - spanInfo->columns);
        }
    }
}

/* 69088e00 -- __glClipCopyPixels */
GLboolean __fastcall __glClipCopyPixels(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    __GLfloat num, den;
    __GLfloat rpyUp, rpyDown;
    GLint rowsUp, rowsDown, startUp, startDown;
    __GLfloat midPoint;
    GLint intMidPoint, rowCount;
    GLint width, height;
    GLint readX, readY;
    __GLfloat zoomx, zoomy;
    __GLfloat rpx, rpy;
    __GLfloat rx1, rx2, ry1, ry2, wx1, wx2, wy1, wy2;
    __GLfloat abszoomy;
    GLint readUp, readDown;

    /*
    ** NOTE:
    ** A "nice" thing we could do for our application writers would be 
    ** to copy white when they try to copy from off window memory.  This
    ** would alert them to a bug in their program which they could then
    ** fix.
    **
    ** However, that seems like unnecessary code which would never be used
    ** anyway (no reason to bloat unnecessarily).
    */

    /*
    ** We take the easy approach, and just call the DrawPixels and ReadPixels
    ** clippers directly.
    */
    spanInfo->dstSkipLines = 0;
    spanInfo->dstSkipPixels = 0;
    if (!__glClipReadPixels(gc, spanInfo)) return GL_FALSE;
    spanInfo->x += spanInfo->dstSkipPixels * spanInfo->zoomx;
    spanInfo->y += spanInfo->dstSkipLines * spanInfo->zoomy;

    spanInfo->srcSkipLines = 0;
    spanInfo->srcSkipPixels = 0;
    if (!__glClipDrawPixels(gc, spanInfo)) return GL_FALSE;
    spanInfo->readX += spanInfo->srcSkipPixels;
    if (gc->constants.yInverted) {
        spanInfo->readY -= spanInfo->srcSkipLines;
    } else {
        spanInfo->readY += spanInfo->srcSkipLines;
    }

    /*
    ** Now for the incredibly tricky part!
    **
    ** This code attempts to deal with overlapping CopyPixels regions.
    ** It is a very difficult problem given that zoomy may be negative.
    ** The IrisGL used a cheap hack to solve this problem, which is 
    ** to read in the entire source image, and then write the destination
    ** image.  The problem with this approach, of course, is that it 
    ** requires a large amount of memory.
    **
    ** If zoomy can only be positive, then any image can be copied by
    ** copying a single span at a time, as long as you are careful about
    ** what order you process the spans.  However, since zoomy may be
    ** negative, the worst case images require copying two spans at 
    ** a time.  This means reading both spans, possibly modifying them,
    ** and then writing them back out. 
    **
    ** An example of this can be seen as follows:  Suppose an image
    ** covering 4 spans is copied onto itself with a zoomy of -1.  This
    ** means that the first row will be copied to the fourth row,
    ** and the fourth row will be copied to the first row.  In order 
    ** to accomplish both of these copies, they must be performed 
    ** simultaneously (after all, if you copy the first row to
    ** the fourth row first, then you have just destroyed the data 
    ** on the fourth row, and you can no longer copy it!).
    **
    ** In the most general case, any rectangular image can be copied
    ** by simultaneously iterating two spans over the source image
    ** and copying as you go.  Sometimes these spans will start at 
    ** the outside of the image and move their way inwards meeting 
    ** in the middle, and sometimes they will start in the middle 
    ** and work their way outward.
    **
    ** The middle point where the spans both start or end depends
    ** upon how the source and destination images overlap.  This point
    ** may be exactly in the middle, or at either end.  This means 
    ** that you may only end up with just a single span iterating over the 
    ** entire image (starting at one end and moving to the other).
    **
    ** The code that follows computes if the images overlap, and if they
    ** do, how two spans can be used to iterate over the source image
    ** so that it can be successfully copied to the destination image.
    **
    ** The following fields in the spanInfo record will be set in the 
    ** process of making these calculations:
    **
    ** overlap - set to GL_TRUE if the regions overlap at all.  Set to
    **                 GL_FALSE otherwise.
    **
    ** rowsUp, rowsDown - The number of rows of the source image that
    **                           need to be dealt with by the span that moves up
    **                          over the source image and the one that moves down
    **                          over the source image.  For example, if rowsUp is
    **                          equal to 10 and rowsDown is 0, then all 10 rows of 
    **                          the image should be copied by the up moving span
    **                          (the one that starts at readY and works it's way
    **                          up to readY+height).
    **
    ** startUp, startDown - At what relative points in time the spans should
    **                            start iterating.  For example, if startUp is 0
    **                            and startDown is 2, then the up moving span 
    **                            should be started first, and after it has 
    **                            iterated over 2 rows of the source image then
    **                            the down moving span should be started.
    **
    ** rpyUp, rpyDown - The starting raster positions for the two spans.
    **                        These numbers are not exactly what they claim to
    **                        be, but they are close.  They should be used by
    **                        the span iterators in the following manner:  When
    **                        the up moving span starts, it starts iterating 
    **                        the float "rp_y" at rpyUp.  After reading and
    **                        modifying a span, the span is written to rows
    **                   floor(rp_y) through floor(rp_y+zoomy) of the
    **                        screen (not-inclusive of floor(rp_y+zoomy)).
    **                        rp_y is then incremented by zoomy.  The same 
    **                        algorithm is applied to the down moving span except
    **                        that zoomy is subtracted from rp_y instead of
    **                        being added.
    **
    ** readUp, readDown - The spans that are to be used for reading from
    **                          the source image.  The up moving span should start
    **                          reading at line "readUp", and the down moving span
    **                          should start at "readDown". 
    **
    ** Remember that the up moving and down moving spans must be iterated
    ** over the image simultaneously such that both spans are read before
    ** either one is written.
    **
    ** The actual algorithm applied here took many many hours of scratch 
    ** paper, and graph diagrams to derive.  It is very complicated, and
    ** hard to understand.  Do not attempt to change it without first
    ** understanding what it does completely.
    **
    ** In a nutshell, it first computes what span of the source image 
    ** will be copied onto itself (if any), and if |zoomy| < 1 it starts the
    ** up and down moving spans there and moves them outwards, or if 
    ** |zoomy| >= 1 it starts the spans at the outside of the image 
    ** and moves them inward so that they meet at the computed point.
    **
    ** Computing what span of the source image copies onto itself is 
    ** relatively easy.  For any span j of the source image from 0 through
    ** height, the span is read from row "readY + j" and written to
    ** any row centers falling within the range "rp_y + j * zoomy" through 
    ** "rp_y + (j+1) * zoomy".  If you set these equations equal to 
    ** each other (and subtract 0.5 from the raster position -- effectively
    ** moving the row centers from X.5 to X.0), you can determine that for 
    ** j = (readY - (rpy - 0.5)) / (zoomy-1) the source image concides with
    ** the destination image.  This is a floating point solution to a discrete
    ** problem, meaning that it is not a complete solution, but that is 
    ** the general idea.  Explaining this algorithm in any more detail would
    ** take another 1000 lines of comments, so I will leave it at that.
    */

    width = spanInfo->width;
    height = spanInfo->height;
    rpx = spanInfo->x;
    rpy = spanInfo->y;
    readX = (GLint) spanInfo->readX;
    readY = (GLint) spanInfo->readY;
    zoomx = spanInfo->zoomx;
    zoomy = spanInfo->zoomy;

    /* First check if the regions overlap at all */
    if (gc->constants.yInverted) {
        ry1 = readY - height + __glHalf;
        ry2 = readY - gc->constants.viewportAlmostHalf;
    } else {
        ry1 = readY + __glHalf;
        ry2 = readY + height - gc->constants.viewportAlmostHalf;
    }
    rx1 = readX + __glHalf;
    rx2 = readX + width - gc->constants.viewportAlmostHalf;
    if (zoomx > 0) {
        /* Undo some math done by ClipDrawPixels */
        rpx = rpx - gc->constants.viewportAlmostHalf;
        wx1 = rpx;
        wx2 = rpx + zoomx * width;
    } else {
        /* Undo some math done by ClipDrawPixels */
        rpx = rpx - gc->constants.viewportAlmostHalf + __GL_ONE_F;
        wx1 = rpx + zoomx * width;
        wx2 = rpx;
    }
    if (zoomy > 0) {
        /* Undo some math done by ClipDrawPixels */
        rpy = rpy - gc->constants.viewportAlmostHalf;
        abszoomy = zoomy;
        wy1 = rpy;
        wy2 = rpy + zoomy * height;
    } else {
        /* Undo some math done by ClipDrawPixels */
        rpy = rpy - gc->constants.viewportAlmostHalf + __GL_ONE_F;
        abszoomy = -zoomy;
        wy1 = rpy + zoomy * height;
        wy2 = rpy;
    }

    if (rx2 < wx1 || wx2 < rx1 || ry2 < wy1 || wy2 < ry1) {
        /* No overlap! */
        spanInfo->overlap = GL_FALSE;
        spanInfo->rowsUp = height;
        spanInfo->rowsDown = 0;
        spanInfo->startUp = 0;
        spanInfo->startDown = 0;
        spanInfo->rpyUp = rpy;
        spanInfo->rpyDown = rpy;
        return GL_TRUE;
    }

    spanInfo->overlap = GL_TRUE;

    /* Time to compute how we should set up our spans */
    if (gc->constants.yInverted) {
        num = (rpy - 0.5) - readY;
        den = -zoomy - 1;
    } else {
        num = readY - (rpy - 0.5);
        den = zoomy - 1;
    }
    startDown = startUp = 0;
    rowsUp = rowsDown = 0;
    rpyUp = rpy;
    rpyDown = rpy + zoomy*height;
    readUp = readY;
    if (gc->constants.yInverted) {
        readDown = readY - height + 1;
    } else {
        readDown = readY + height - 1;
    }

    if (den == __GL_ZERO_F) {
        /* Better not divide! */
        if (num > 0) {
            midPoint = (__GLfloat) height;
        } else {
            midPoint = __GL_ZERO_F;
        }
    } else {
        midPoint = num/den;
        if (midPoint < __GL_ZERO_F) {
            midPoint = __GL_ZERO_F;
        } else if (midPoint > height) {
            midPoint = (__GLfloat) height;
        }
    }
    if (midPoint == __GL_ZERO_F) {
        /* Only one span needed */
        if (abszoomy < __GL_ONE_F) {
            rowsUp = height;
        } else {
            rowsDown = height;
        }
    } else if (midPoint == height) {
        /* Only one span needed */
        if (abszoomy < __GL_ONE_F) {
            rowsDown = height;
        } else {
            rowsUp = height;
        }
    } else {
        /* Almost definitely need two spans to copy this image! */
        intMidPoint = (GLint) __GL_CEILF(midPoint);

        rowCount = height - intMidPoint;
        if (intMidPoint > rowCount) {
            rowCount = intMidPoint;
        }

        if (abszoomy > __GL_ONE_F) {
            GLint temp;

            /* Move from outside of image inward */
            startUp = rowCount - intMidPoint;
            startDown = rowCount - (height - intMidPoint);
            rowsUp = intMidPoint;
            rowsDown = height - rowsUp;

            if (gc->constants.yInverted) {
                temp = readY - intMidPoint + 1;
            } else {
                temp = readY + intMidPoint - 1;
            }

            if (__GL_FLOORF( (temp - 
                    (rpy-__glHalf-gc->constants.viewportEpsilon)) 
                    / zoomy) == intMidPoint-1) {
                /* 
                ** row "intMidPoint-1" copies exactly onto itself.  Let's 
                ** make it the midpoint which we converge to.
                */
                if (startDown) {
                    startDown--;
                } else {
                    startUp++;
                }
            }
        } else {
            /* Move from inside of image outward */
            rowsDown = intMidPoint;
            rowsUp = height - rowsDown;
            rpyUp = rpyDown = rpy + zoomy * intMidPoint;
            if (gc->constants.yInverted) {
                readUp = readY - intMidPoint;
                readDown = readY - intMidPoint + 1;
            } else {
                readUp = readY + intMidPoint;
                readDown = readY + intMidPoint - 1;
            }

            if (__GL_FLOORF( (readDown - 
                    (rpy-__glHalf-gc->constants.viewportEpsilon))
                    / zoomy) == intMidPoint-1) {
                /* 
                ** row "intMidPoint-1" copies exactly onto itself.  Let's
                ** make it the midpoint which we diverge from.
                */
                startUp = 1;
            }
        }
    }

    /* 
    ** Adjust rpyUp and rpyDown so that they will change integer values 
    ** when fragments should be produced.  This basically takes the 0.5
    ** out of the inner loop when these spans are actually iterated.
    */
    if (zoomy > 0) {
        spanInfo->rpyUp = rpyUp + gc->constants.viewportAlmostHalf;
        spanInfo->rpyDown = rpyDown + gc->constants.viewportAlmostHalf - 
                __GL_ONE_F;
    } else {
        spanInfo->rpyUp = rpyUp + gc->constants.viewportAlmostHalf - __GL_ONE_F;
        spanInfo->rpyDown = rpyDown + gc->constants.viewportAlmostHalf;
    }
    spanInfo->startUp = startUp;
    spanInfo->startDown = startDown;
    spanInfo->rowsUp = rowsUp;
    spanInfo->rowsDown = rowsDown;
    spanInfo->readUp = readUp;
    spanInfo->readDown = readDown;

    return GL_TRUE;
}

/* 69085cd0 -- __glClipDrawPixels */
GLboolean __fastcall __glClipDrawPixels(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    GLint skipPixels;
    GLint skipRows;
    GLint width, height;
    GLint tempint;
    GLint endCol, endRow;
    __GLfloat x,y,x2,y2;
    __GLfloat zoomx, zoomy;
    __GLfloat clipLeft, clipRight, clipBottom, clipTop;

    zoomx = spanInfo->zoomx;
    zoomy = spanInfo->zoomy;
    if (zoomx == __GL_ZERO_F || zoomy == __GL_ZERO_F) {
        return GL_FALSE;
    }

    skipPixels = skipRows = 0;
    width = spanInfo->width;
    height = spanInfo->height;
    clipLeft = gc->transform.clipX0 + __glHalf;
    clipBottom = gc->transform.clipY0 + __glHalf;
    clipRight = gc->transform.clipX1 - gc->constants.viewportAlmostHalf;
    clipTop = gc->transform.clipY1 - gc->constants.viewportAlmostHalf;

#if defined(__GL_HALF_PIXEL_OFFSET)
    clipLeft -= __glHalf;
    clipBottom -= __glHalf;
    clipRight -= __glHalf;
    clipTop -= __glHalf;
#endif

    /* first corner of destination rectangle */
    x = spanInfo->x;
    y = spanInfo->y;

    /* opposite corner of destination rectangle */
    x2 = x + zoomx * width;
    y2 = y + zoomy * height;

    if (zoomx > 0) {
        /* Zoomx is positive, clip the left edge */
        if (x > clipLeft) {
            /* Clip to the first fragment that will be produced */
            clipLeft = (__GLfloat)(GLint)(x + gc->constants.viewportAlmostHalf);
            clipLeft += __glHalf;
        }
        skipPixels = (GLint) ((clipLeft-x) / zoomx);
        if (skipPixels >= width) return GL_FALSE;

        width -= skipPixels;
        spanInfo->startCol = (GLint) clipLeft;
        x = x + skipPixels * zoomx;
        spanInfo->x = x + gc->constants.viewportAlmostHalf;
        spanInfo->srcSkipPixels += skipPixels;

        /* Zoomx is positive, clip the right edge */
        if (x2 < clipRight) {
            /* Clip to the last fragment that will be produced */
            clipRight = (__GLfloat)(GLint)(x2 + gc->constants.viewportAlmostHalf);
            clipRight -= gc->constants.viewportAlmostHalf;
        }
        tempint = (GLint) ((x2-clipRight) / zoomx);
        if (tempint >= width) return GL_FALSE;

        width -= tempint;
        endCol = (GLint) clipRight + 1;
        spanInfo->endCol = endCol;
        spanInfo->columns = endCol - spanInfo->startCol;
    } else /* zoomx < 0 */ {
        /* Zoomx is negative, clip the right edge */
        if (x < clipRight) {
            /* Clip to the first fragment that will be produced */
            clipRight = (__GLfloat)(GLint)(x + gc->constants.viewportAlmostHalf);
            clipRight -= gc->constants.viewportAlmostHalf;
        }
        skipPixels = (GLint) ((clipRight-x) / zoomx);
        if (skipPixels >= width) return GL_FALSE;

        width -= skipPixels;
        spanInfo->startCol = (GLint) clipRight;
        x = x + skipPixels * zoomx;
        spanInfo->x = x + gc->constants.viewportAlmostHalf - __GL_ONE_F;
        spanInfo->srcSkipPixels += skipPixels;

        /* Zoomx is negative, clip the left edge */
        if (x2 > clipLeft) {
            clipLeft = (__GLfloat)(GLint)(x2 + gc->constants.viewportAlmostHalf);
            clipLeft += __glHalf;
        }
        tempint = (GLint) ((x2-clipLeft) / zoomx);
        if (tempint >= width) return GL_FALSE;

        width -= tempint;
        endCol = (GLint) clipLeft - 1;
        spanInfo->endCol = endCol;
        spanInfo->columns = spanInfo->startCol - endCol;
    }

    if (zoomy > 0) {
        /* Zoomy is positive, clip the bottom edge */
        if (y > clipBottom) {
            /* Clip to the first row that will be produced */
            clipBottom = (__GLfloat)(GLint)(y + gc->constants.viewportAlmostHalf);
            clipBottom += __glHalf;
        }
        skipRows = (GLint) ((clipBottom-y) / zoomy);
        if (skipRows >= height) return GL_FALSE;

        height -= skipRows;
        spanInfo->startRow = (GLint) clipBottom;
        y = y + skipRows * zoomy;
        spanInfo->y = y + gc->constants.viewportAlmostHalf;
        spanInfo->srcSkipLines += skipRows;

        /* Zoomy is positive, clip the top edge */
        if (y2 < clipTop) {
            /* Clip to the last row that will be produced */
            clipTop = (__GLfloat)(GLint)(y2 + gc->constants.viewportAlmostHalf);
            clipTop -= gc->constants.viewportAlmostHalf;
        }
        tempint = (GLint) ((y2-clipTop) / zoomy);
        if (tempint >= height) return GL_FALSE;

        height -= tempint;
        endRow = (GLint) clipTop + 1;
        spanInfo->rows = endRow - spanInfo->startRow;
    } else /* zoomy < 0 */ {
        /* Zoomy is negative, clip the top edge */
        if (y < clipTop) {
            /* Clip to the first row that will be produced */
            clipTop = (__GLfloat)(GLint)(y + gc->constants.viewportAlmostHalf);
            clipTop -= gc->constants.viewportAlmostHalf;
        }
        skipRows = (GLint) ((clipTop-y) / zoomy);
        if (skipRows >= height) return GL_FALSE;

        height -= skipRows;
        spanInfo->startRow = (GLint) clipTop;
        y = y + skipRows * zoomy;
        /* spanInfo->y = y - __glHalf; */
        spanInfo->y = y + gc->constants.viewportAlmostHalf - __GL_ONE_F;
        spanInfo->srcSkipLines += skipRows;

        /* Zoomy is negative, clip the bottom edge */
        if (y2 > clipBottom) {
            clipBottom = (__GLfloat)(GLint)(y2 + gc->constants.viewportAlmostHalf);
            clipBottom += __glHalf;
        }
        tempint = (GLint) ((y2-clipBottom) / zoomy);
        if (tempint >= height) return GL_FALSE;

        height -= tempint;
        endRow = (GLint) clipBottom - 1;
        spanInfo->rows = spanInfo->startRow - endRow;
    }

    spanInfo->width = width;
    spanInfo->height = height;

    return GL_TRUE;
}

/* 690889f0 -- __glReadPixelsN */
void __fastcall __glReadPixelsN(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    GLint i, ySign;
    GLint height;
    GLint numSpanMods = spanInfo->numSpanMods;
    GLint spanModNum;
    void (**spanModifier)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                          const GLvoid *inspan, GLvoid *outspan);
    void (*reader)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                   GLvoid *outspan);
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
    GLubyte *tmpSpanData, *spanDataIn, *spanDataOut;

    reader = spanInfo->spanReader;

    ySign = gc->constants.ySign;
    height = spanInfo->height;
    spanModifier = spanInfo->spanModifier;

    for (i=0; i<height; i++) {
        (*reader)(gc, spanInfo, spanData1);
        spanDataIn = spanData1; spanDataOut = spanData2;
        for (spanModNum = 0; spanModNum < numSpanMods-1; spanModNum++) {
            (*spanModifier[spanModNum])(gc, spanInfo, spanDataIn, spanDataOut);
            tmpSpanData = spanDataIn;
            spanDataIn = spanDataOut;
            spanDataOut = tmpSpanData;
        }
        (*spanModifier[spanModNum])(gc, spanInfo, spanDataIn, spanInfo->dstCurrent);

        spanInfo->dstCurrent = (GLvoid *) ((GLubyte *) spanInfo->dstCurrent +
                spanInfo->dstRowIncrement);
        spanInfo->readY += ySign;
    }
}

/* 69088ae0 -- __glReadPixels2 */
void __fastcall __glReadPixels2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    GLint i, ySign;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                  const GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                  const GLvoid *inspan, GLvoid *outspan);
    void (*reader)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                   GLvoid *outspan);
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];

    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    reader = spanInfo->spanReader;

    ySign = gc->constants.ySign;
    height = spanInfo->height;

    for (i=0; i<height; i++) {
        (*reader)(gc, spanInfo, spanData1);
        (*span1)(gc, spanInfo, spanData1, spanData2);
        (*span2)(gc, spanInfo, spanData2, spanInfo->dstCurrent);
        spanInfo->dstCurrent = (GLvoid *) ((GLubyte *) spanInfo->dstCurrent +
                spanInfo->dstRowIncrement);
        spanInfo->readY += ySign;
    }
}

/* 69088b90 -- __glReadPixels1 */
void __fastcall __glReadPixels1(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    GLint i, ySign;
    GLint height;
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                  const GLvoid *inspan, GLvoid *outspan);
    void (*reader)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                   GLvoid *outspan);
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];

    span1 = spanInfo->spanModifier[0];
    reader = spanInfo->spanReader;

    ySign = gc->constants.ySign;
    height = spanInfo->height;

    for (i=0; i<height; i++) {
        (*reader)(gc, spanInfo, spanData1);
        (*span1)(gc, spanInfo, spanData1, spanInfo->dstCurrent);
        spanInfo->dstCurrent = (GLvoid *) ((GLubyte *) spanInfo->dstCurrent +
                spanInfo->dstRowIncrement);
        spanInfo->readY += ySign;
    }
}

/* 69089570 -- __glCopyPixelsN */
void __fastcall __glCopyPixelsN(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    __GLfloat newy;
    __GLfloat zoomy;
    GLint inty, i, ySign;
    GLint height;
    GLint numSpanMods = spanInfo->numSpanMods;
    GLint spanModNum;
    void (**spanModifier)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                  const GLvoid *inspan, GLvoid *outspan);
    void (*reader)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                   GLvoid *outspan);
    void (*render)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                   const GLvoid *inspan);
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
    GLubyte *tmpSpanData, *spanDataIn, *spanDataOut;
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    if (spanInfo->overlap) {
        __glCopyPixelsOverlapping(gc, spanInfo, numSpanMods);
        return;
    }

    reader = spanInfo->spanReader;
    render = spanInfo->spanRender;

    ySign = gc->constants.ySign;
    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;
    spanModifier = spanInfo->spanModifier;

    for (i=0; i<height; i++) {
        spanInfo->y = newy;
        newy += zoomy;
        /*
        ** Skip over spans when zoomy is in the range (-1,1)
        */
        while ((GLint) newy == inty && i<height) {
            spanInfo->readY += ySign;
            spanInfo->y = newy;
            newy += zoomy;
            i++;
            assert (i != height);
        }
        inty = (GLint) newy;

        (*reader)(gc, spanInfo, spanData1);
        spanDataIn = spanData1; spanDataOut = spanData2;
        for (spanModNum = 0; spanModNum < numSpanMods; spanModNum++) {
            (*spanModifier[spanModNum])(gc, spanInfo, spanDataIn, spanDataOut);
            tmpSpanData = spanDataIn;
            spanDataIn = spanDataOut;
            spanDataOut = tmpSpanData;
        }
        (*render)(gc, spanInfo, spanDataIn);

        spanInfo->readY += ySign;
    }
}

/* 69089710 -- __glCopyPixels2 */
void __fastcall __glCopyPixels2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    __GLfloat newy;
    __GLfloat zoomy;
    GLint inty, i, ySign;
    GLint height;
    void (*reader)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                   GLvoid *outspan);
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                  const GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                  const GLvoid *inspan, GLvoid *outspan);
    void (*render)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                   const GLvoid *inspan);
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    if (spanInfo->overlap) {
        __glCopyPixelsOverlapping(gc, spanInfo, 2);
        return;
    }

    reader = spanInfo->spanReader;
    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    render = spanInfo->spanRender;

    ySign = gc->constants.ySign;
    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;

    for (i=0; i<height; i++) {
        spanInfo->y = newy;
        newy += zoomy;
        /*
        ** Skip over spans when zoomy is in the range (-1,1)
        */
        while ((GLint) newy == inty && i<height) {
            spanInfo->readY += ySign;
            spanInfo->y = newy;
            newy += zoomy;
            i++;
            assert (i != height);
        }
        inty = (GLint) newy;
        (*reader)(gc, spanInfo, spanData1);
        (*span1)(gc, spanInfo, spanData1, spanData2);
        (*span2)(gc, spanInfo, spanData2, spanData1);
        (*render)(gc, spanInfo, spanData1);
        spanInfo->readY += ySign;
    }
}

/* 69089890 -- __glCopyPixels1 */
void __fastcall __glCopyPixels1(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    __GLfloat newy;
    __GLfloat zoomy;
    GLint inty, i, ySign;
    GLint height;
    void (*reader)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                   GLvoid *outspan);
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                  const GLvoid *inspan, GLvoid *outspan);
    void (*render)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                   const GLvoid *inspan);
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    if (spanInfo->overlap) {
        __glCopyPixelsOverlapping(gc, spanInfo, 1);
        return;
    }

    reader = spanInfo->spanReader;
    span1 = spanInfo->spanModifier[0];
    render = spanInfo->spanRender;

    ySign = gc->constants.ySign;
    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;

    for (i=0; i<height; i++) {
        spanInfo->y = newy;
        newy += zoomy;
        /*
        ** Skip over spans when zoomy is in the range (-1,1)
        */
        while ((GLint) newy == inty && i<height) {
            spanInfo->readY += ySign;
            spanInfo->y = newy;
            newy += zoomy;
            i++;
            assert (i != height);
        }
        inty = (GLint) newy;
        (*reader)(gc, spanInfo, spanData1);
        (*span1)(gc, spanInfo, spanData1, spanData2);
        (*render)(gc, spanInfo, spanData2);
        spanInfo->readY += ySign;
    }
}

/* 690899f0 -- __glCopyPixels0 */
void __fastcall __glCopyPixels0(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    __GLfloat newy;
    __GLfloat zoomy;
    GLint inty, i, ySign;
    GLint height;
    void (*reader)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                   GLvoid *outspan);
    void (*render)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                   const GLvoid *inspan);
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];
    GLshort pixelArray[__GL_MAX_MAX_VIEWPORT];

    spanInfo->pixelArray = pixelArray;
    __glComputeSpanPixelArray(gc, spanInfo);

    if (spanInfo->overlap) {
        __glCopyPixelsOverlapping(gc, spanInfo, 0);
        return;
    }

    reader = spanInfo->spanReader;
    render = spanInfo->spanRender;

    ySign = gc->constants.ySign;
    zoomy = spanInfo->zoomy;
    inty = (GLint) spanInfo->y;
    newy = spanInfo->y;
    height = spanInfo->height;

    for (i=0; i<height; i++) {
        spanInfo->y = newy;
        newy += zoomy;
        while ((GLint) newy == inty && i<height) {
            spanInfo->readY += ySign;
            spanInfo->y = newy;
            newy += zoomy;
            i++;
            assert (i != height);
        }
        inty = (GLint) newy;
        (*reader)(gc, spanInfo, spanData1);
        (*render)(gc, spanInfo, spanData1);
        spanInfo->readY += ySign;
    }
}

/* 69089b30 -- __glCopyPixelsOverlapping */
void __fastcall __glCopyPixelsOverlapping(__GLcontext *gc, 
                               __GLpixelSpanInfoICD *spanInfo, GLint modifiers)
{
    GLint i;
    __GLfloat zoomy, newy;
    GLint inty, ySign;
    GLubyte spanData1[__GL_MAX_SPAN_SIZE], spanData2[__GL_MAX_SPAN_SIZE];
    GLubyte spanData3[__GL_MAX_SPAN_SIZE];
    GLubyte *outSpan1, *outSpan2;
    GLint rowsUp, rowsDown;
    GLint startUp, startDown;
    __GLfloat rpyUp, rpyDown;
    GLint readUp, readDown;
    GLint gotUp, gotDown;
    __GLpixelSpanInfoICD downSpanInfo;
    GLint clipLow, clipHigh;
    GLint startRow, endRow;
    void (*reader)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                   GLvoid *outspan);
    void (*render)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo, 
                   const GLvoid *inspan);

    reader = spanInfo->spanReader;
    render = spanInfo->spanRender;

    if (modifiers & 1) {
        outSpan1 = outSpan2 = spanData3;
    } else {
        outSpan1 = spanData1;
        outSpan2 = spanData2;
    }

    zoomy = spanInfo->zoomy;
    rowsUp = spanInfo->rowsUp;
    rowsDown = spanInfo->rowsDown;
    startUp = spanInfo->startUp;
    startDown = spanInfo->startDown;
    rpyUp = spanInfo->rpyUp;
    rpyDown = spanInfo->rpyDown;
    readUp = spanInfo->readUp;
    readDown = spanInfo->readDown;
    downSpanInfo = *spanInfo;
    downSpanInfo.rowadd = -spanInfo->rowadd;
    downSpanInfo.zoomy = -zoomy;
    spanInfo->y = rpyUp;
    downSpanInfo.y = rpyDown;
    spanInfo->readY = (__GLfloat) readUp;
    downSpanInfo.readY = (__GLfloat) readDown;
    gotUp = gotDown = 0;
    ySign = gc->constants.ySign;

    /* Clip upgoing and downgoing spans */
    if (zoomy > 0) {
        clipLow = spanInfo->startRow;
        clipHigh = spanInfo->startRow + spanInfo->rows - 1;

        /* Clip down span first */
        startRow = (GLint) rpyDown;
        endRow = (GLint) (rpyDown - zoomy*rowsDown) + 1;
        if (startRow > clipHigh) startRow = clipHigh;
        if (endRow < clipLow) endRow = clipLow;
        downSpanInfo.startRow = startRow;
        downSpanInfo.rows = startRow - endRow + 1;

        /* Now clip up span */
        startRow = (GLint) rpyUp;
        endRow = (GLint) (rpyUp + zoomy*rowsUp) - 1;
        if (startRow < clipLow) startRow = clipLow;
        if (endRow > clipHigh) endRow = clipHigh;
        spanInfo->startRow = startRow;
        spanInfo->rows = endRow - startRow + 1;
    } else /* zoomy < 0 */ {
        clipHigh = spanInfo->startRow;
        clipLow = spanInfo->startRow - spanInfo->rows + 1;

        /* Clip down span first */
        startRow = (GLint) rpyDown;
        endRow = (GLint) (rpyDown - zoomy*rowsDown) - 1;
        if (startRow < clipLow) startRow = clipLow;
        if (endRow > clipHigh) endRow = clipHigh;
        downSpanInfo.startRow = startRow;
        downSpanInfo.rows = endRow - startRow + 1;

        /* Now clip up span */
        startRow = (GLint) rpyUp;
        endRow = (GLint) (rpyUp + zoomy*rowsUp) + 1;
        if (startRow > clipHigh) startRow = clipHigh;
        if (endRow < clipLow) endRow = clipLow;
        spanInfo->startRow = startRow;
        spanInfo->rows = startRow - endRow + 1;
    }

    while (rowsUp && rowsDown) {
        if (startUp) {
            startUp--;
        } else {
            gotUp = 1;
            rowsUp--;
            spanInfo->y = rpyUp;
            newy = rpyUp + zoomy;
            inty = (GLint) rpyUp;
            while (rowsUp && (GLint) newy == inty) {
                spanInfo->y = newy;
                newy += zoomy;
                rowsUp--;
                spanInfo->readY += ySign;
            }
            if (inty == (GLint) newy) break;
            rpyUp = newy;
            (*reader)(gc, spanInfo, spanData1);
            spanInfo->readY += ySign;
        }
        if (startDown) {
            startDown--;
        } else {
            gotDown = 1;
            rowsDown--;
            downSpanInfo.y = rpyDown;
            newy = rpyDown - zoomy;
            inty = (GLint) rpyDown;
            while (rowsDown && (GLint) newy == inty) {
                downSpanInfo.y = newy;
                newy -= zoomy;
                rowsDown--;
                downSpanInfo.readY -= ySign;
            }
            if (inty == (GLint) newy) {
                if (gotUp) {
                    for (i=0; i<modifiers; i++) {
                        if (i & 1) {
                            (*(spanInfo->spanModifier[i]))(gc, spanInfo, 
                                    spanData3, spanData1);
                        } else {
                            (*(spanInfo->spanModifier[i]))(gc, spanInfo, 
                                    spanData1, spanData3);
                        }
                    }
                    (*render)(gc, spanInfo, outSpan1);
                }
                break;
            }
            rpyDown = newy;
            (*reader)(gc, &downSpanInfo, spanData2);
            downSpanInfo.readY -= ySign;
        }

        if (gotUp) {
            for (i=0; i<modifiers; i++) {
                if (i & 1) {
                    (*(spanInfo->spanModifier[i]))(gc, spanInfo, 
                            spanData3, spanData1);
                } else {
                    (*(spanInfo->spanModifier[i]))(gc, spanInfo, 
                            spanData1, spanData3);
                }
            }
            (*render)(gc, spanInfo, outSpan1);
        }

        if (gotDown) {
            for (i=0; i<modifiers; i++) {
                if (i & 1) {
                    (*(spanInfo->spanModifier[i]))(gc, &downSpanInfo, 
                            spanData3, spanData2);
                } else {
                    (*(spanInfo->spanModifier[i]))(gc, &downSpanInfo, 
                            spanData2, spanData3);
                }
            }
            (*render)(gc, &downSpanInfo, outSpan2);
        }
    }

    /*
    ** Only one of the spanners is left to iterate.
    */

    while (rowsUp) {
        /* Do what is left of up spans */
        rowsUp--;
        spanInfo->y = rpyUp;
        newy = rpyUp + zoomy;
        inty = (GLint) rpyUp;
        while (rowsUp && (GLint) newy == inty) {
            spanInfo->y = newy;
            newy += zoomy;
            rowsUp--;
            spanInfo->readY += ySign;
        }
        if (inty == (GLint) newy) break;
        rpyUp = newy;

        (*reader)(gc, spanInfo, spanData1);
        for (i=0; i<modifiers; i++) {
            if (i & 1) {
                (*(spanInfo->spanModifier[i]))(gc, spanInfo, 
                        spanData3, spanData1);
            } else {
                (*(spanInfo->spanModifier[i]))(gc, spanInfo, 
                        spanData1, spanData3);
            }
        }
        (*render)(gc, spanInfo, outSpan1);

        spanInfo->readY += ySign;
    }

    while (rowsDown) {
        /* Do what is left of down spans */
        rowsDown--;
        downSpanInfo.y = rpyDown;
        newy = rpyDown - zoomy;
        inty = (GLint) rpyDown;
        while (rowsDown && (GLint) newy == inty) {
            downSpanInfo.y = newy;
            newy -= zoomy;
            rowsDown--;
            downSpanInfo.readY -= ySign;
        }
        if (inty == (GLint) newy) break;
        rpyDown = newy;

        (*reader)(gc, &downSpanInfo, spanData2);
        for (i=0; i<modifiers; i++) {
            if (i & 1) {
                (*(spanInfo->spanModifier[i]))(gc, &downSpanInfo, 
                        spanData3, spanData2);
            } else {
                (*(spanInfo->spanModifier[i]))(gc, &downSpanInfo, 
                        spanData2, spanData3);
            }
        }
        (*render)(gc, &downSpanInfo, outSpan2);

        downSpanInfo.readY -= ySign;
    }
}

/* 6908a3f0 -- __glCopyImage2 */
void __fastcall __glCopyImage2(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    GLint i;
    GLint height;
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];
    void (*span1)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                  const GLvoid *inspan, GLvoid *outspan);
    void (*span2)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                  const GLvoid *inspan, GLvoid *outspan);

    height = spanInfo->height;
    span1 = spanInfo->spanModifier[0];
    span2 = spanInfo->spanModifier[1];
    for (i=0; i<height; i++) {
        (*span1)(gc, spanInfo, spanInfo->srcCurrent, spanData1);
        spanInfo->srcCurrent = (const GLubyte *) spanInfo->srcCurrent + 
                spanInfo->srcRowIncrement;
        (*span2)(gc, spanInfo, spanData1, spanInfo->dstCurrent);
        spanInfo->dstCurrent = (GLubyte *) spanInfo->dstCurrent +
                spanInfo->dstRowIncrement;
    }
}

/* 6908a470 -- __glCopyImageN */
void __fastcall __glCopyImageN(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo)
{
    GLint i;
    GLint height;
    GLint numSpanMods = spanInfo->numSpanMods;
    GLint spanModNum;
    void (**spanModifier)(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                          const GLvoid *inspan, GLvoid *outspan);
    GLubyte spanData1[__GL_MAX_SPAN_SIZE];
    GLubyte spanData2[__GL_MAX_SPAN_SIZE];
    GLubyte *tmpSpanData, *spanDataIn, *spanDataOut;

    height = spanInfo->height;
    spanModifier = spanInfo->spanModifier;
    for (i=0; i<height; i++) {

        (*spanModifier[0])(gc, spanInfo, spanInfo->srcCurrent, spanData1);
        spanInfo->srcCurrent = (const GLubyte *) spanInfo->srcCurrent + 
                spanInfo->srcRowIncrement;
        spanDataIn = spanData1; spanDataOut = spanData2;
        for (spanModNum = 1; spanModNum < numSpanMods-1; spanModNum++) {
            (*spanModifier[spanModNum])(gc, spanInfo, spanDataIn, spanDataOut);
            tmpSpanData = spanDataIn;
            spanDataIn = spanDataOut;
            spanDataOut = tmpSpanData;
        }
        (*spanModifier[spanModNum])(gc, spanInfo, spanDataIn, spanInfo->dstCurrent);
        spanInfo->dstCurrent = (GLubyte *) spanInfo->dstCurrent +
                spanInfo->dstRowIncrement;
    }
}

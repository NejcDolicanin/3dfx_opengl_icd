/*
** gl_lndraw.c -- the s_lndraw.c line setup (aliased, flat-fog and antialiased lines), written
** from the SGI Sample Implementation source with the ICD's shader (glshade.h): the line
** options sit inline in gc->line, one colour and one texture set {s, t, qw, rhow}, and the
** vertices are biased / rescaled around the work like the triangle fillers.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "glshade.h"

/* fmacros.h */
#define FLOAT_MANTISSA_BITS     23
#define FLOAT_MANTISSA_MASK     ((1 << FLOAT_MANTISSA_BITS) - 1)
#define Float2Int(_flt_, _int_) \
    (*((float *)(&(_int_)))=(_flt_)+((float)(1<<(FLOAT_MANTISSA_BITS))), \
     (_int_)&=FLOAT_MANTISSA_MASK)

/* the ICD's sqrt rounds to a float (__GL_CEILF is glsi.h's float ceil) */


/*
** __GL_ADJUST_LINE_VERTS, ICD form (see __GL_ADJUST_TRIANGLE_VERTS in glshade.h): both window
** positions are saved before they are biased, v0's colour is always rescaled and v1's only when
** it is a different colour.
*/
/* the saved vertex values in one struct, as SGI's __GLadjustLineData: the members' order is the original's frame
** order (separate locals are laid out by use weight instead) */
#define __GL_LINE_VERT_SAVES \
    struct { __GLcoord window[2]; __GLcolor color[2]; __GLcoord texture[2]; } adjustData

#define __GL_ADJUST_LINE_VERT(gc, v, X, Y, Z, T) \
    X = v->window.x; v->window.x += __GL_VERTEX_BIAS_F; \
    Y = v->window.y; v->window.y += __GL_VERTEX_BIAS_F; \
    Z = v->window.z; v->window.z *= gc->constants.depthRescale; \
    T = v->texture[0]; \
    v->texture[0].x *= gc->constants.texSScale; \
    v->texture[0].y *= gc->constants.texTScale

#define __GL_ADJUST_LINE_COLOR(gc, v, C) \
    C = *v->color; \
    v->color->r *= gc->constants.redRescale; \
    v->color->g *= gc->constants.greenRescale; \
    v->color->b *= gc->constants.blueRescale; \
    v->color->a *= gc->constants.alphaRescale

#define __GL_ADJUST_LINE_VERTS(gc, v0, v1) \
    gc->constants.viewportXAdjust += __GL_VERTEX_BIAS; \
    gc->constants.viewportYAdjust += __GL_VERTEX_BIAS; \
    gc->transform.clipX0 += __GL_VERTEX_BIAS; \
    gc->transform.clipX1 += __GL_VERTEX_BIAS; \
    gc->transform.clipY0 += __GL_VERTEX_BIAS; \
    gc->transform.clipY1 += __GL_VERTEX_BIAS; \
    __GL_ADJUST_LINE_VERT(gc, v0, adjustData.window[0].x, adjustData.window[0].y, adjustData.window[0].z, adjustData.texture[0]); \
    __GL_ADJUST_LINE_VERT(gc, v1, adjustData.window[1].x, adjustData.window[1].y, adjustData.window[1].z, adjustData.texture[1]); \
    __GL_ADJUST_LINE_COLOR(gc, v0, adjustData.color[0]); \
    if (v1->color != v0->color) { \
	__GL_ADJUST_LINE_COLOR(gc, v1, adjustData.color[1]); \
    }

#define __GL_RESTORE_LINE_VERTS(gc, v0, v1) \
    gc->constants.viewportXAdjust -= __GL_VERTEX_BIAS; \
    gc->constants.viewportYAdjust -= __GL_VERTEX_BIAS; \
    gc->transform.clipX0 -= __GL_VERTEX_BIAS; \
    gc->transform.clipX1 -= __GL_VERTEX_BIAS; \
    gc->transform.clipY0 -= __GL_VERTEX_BIAS; \
    gc->transform.clipY1 -= __GL_VERTEX_BIAS; \
    __GL_RESTORE_VERT(v0, adjustData.window[0].x, adjustData.window[0].y, adjustData.window[0].z, adjustData.texture[0]); \
    __GL_RESTORE_VERT(v1, adjustData.window[1].x, adjustData.window[1].y, adjustData.window[1].z, adjustData.texture[1]); \
    v0->color->r = adjustData.color[0].r; \
    v0->color->g = adjustData.color[0].g; \
    v0->color->b = adjustData.color[0].b; \
    v0->color->a = adjustData.color[0].a; \
    if (v1->color != v0->color) { \
	v1->color->r = adjustData.color[1].r; \
	v1->color->g = adjustData.color[1].g; \
	v1->color->b = adjustData.color[1].b; \
	v1->color->a = adjustData.color[1].a; \
    }

typedef GLfloat (__fastcall *__GLcalcLineRhoProc)(__GLcontext *gc, __GLfloat s, __GLfloat t,
						  __GLfloat q);

/* 690d22a0 -- __glInitLineData */
void __fastcall __glInitLineData(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1)
{
    __GLfloat x0,y0,x1,y1;
    __GLfloat minorStart;
    GLint intMinorStart;
    __GLfloat dx, dy;
    __GLfloat offset;
    __GLfloat slope;
    __GLlineState *ls = &gc->state.line;
    __GLfloat halfWidth;
    __GLfloat x0frac, x1frac, y0frac, y1frac, half, totDist;
    GLint ix0, ix1, iy0, iy1;

    gc->line.v0 = v0;
    gc->line.v1 = v1;
    gc->line.width = ls->aliasedWidth;

    half = gc->constants.half;
    x0=v0->window.x;
    y0=v0->window.y;
    x1=v1->window.x;
    y1=v1->window.y;

    dx=x1-x0;
    dy=y1-y0;

    halfWidth = ls->aliasedWidth * half;

    /* Ugh.  This is slow.  Bummer. */
    /*    x0frac = x0 - ((GLint) x0);
     *    x1frac = x1 - ((GLint) x1);
     *    y0frac = y0 - ((GLint) y0);
     *    y1frac = y1 - ((GLint) y1);
     */
    Float2Int(x0, ix0);
    Float2Int(y0, iy0);
    Float2Int(x1, ix1);
    Float2Int(y1, iy1);
    x0frac = x0 - ix0--;
    y0frac = y0 - iy0--;
    x1frac = x1 - ix1--;
    y1frac = y1 - iy1--;

    if (dx > __GL_ZERO_F) {
	if (dy > __GL_ZERO_F) {
	    if (dx > dy) {	/* dx > dy > 0 */
		gc->line.yBig = 1;
posxmajor:			/* dx > |dy| >= 0 */
		gc->line.yLittle = 0;
		gc->line.xBig = 1;
		gc->line.xLittle = 1;
		slope = dy/dx;

		if (y0frac < 0) y0frac = -y0frac;

		totDist = y0frac + x0frac;
		if (totDist > half) ix0++;

		if (y1frac < 0) y1frac = -y1frac;

		totDist = y1frac + x1frac;
		if (totDist > half) ix1++;

		offset = ix0 + 1.0 - x0;

		gc->line.length = dx;
		gc->line.numPixels = ix1 - ix0;

xmajorfinish:
		gc->line.axis = __GL_X_MAJOR;
		gc->line.xStart = ix0;
		gc->line.offset = offset;
		minorStart = y0 + offset*slope - halfWidth;
		intMinorStart = (GLint) minorStart;
		minorStart -= intMinorStart;
		gc->line.yStart = intMinorStart;
		gc->line.dfraction = (GLint)(slope * __TWO_31);
		gc->line.fraction = (GLint)(minorStart * __TWO_31);
	    } else {		/* dy >= dx > 0 */
		gc->line.xBig = 1;
posymajor:			/* dy >= |dx| >= 0, dy != 0 */
		gc->line.xLittle = 0;
		gc->line.yBig = 1;
		gc->line.yLittle = 1;
		slope = dx/dy;

		if (x0frac < 0) x0frac = -x0frac;

		totDist = y0frac + x0frac;
		if (totDist > half) iy0++;

		if (x1frac < 0) x1frac = -x1frac;

		totDist = y1frac + x1frac;
		if (totDist > half) iy1++;

		offset = iy0 + 1.0 - y0;

		gc->line.length = dy;
		gc->line.numPixels = iy1 - iy0;

ymajorfinish:
		gc->line.axis = __GL_Y_MAJOR;
		gc->line.yStart = iy0;
		gc->line.offset = offset;
		minorStart = x0 + offset*slope - halfWidth;
		intMinorStart = (GLint) minorStart;
		minorStart -= intMinorStart;
		gc->line.xStart = intMinorStart;
		gc->line.dfraction = (GLint)(slope * __TWO_31);
		gc->line.fraction = (GLint)(minorStart * __TWO_31);
	    }
	} else {
	    if (dx > -dy) {	/* dx > -dy >= 0 */
		gc->line.yBig = -1;
		goto posxmajor;
	    } else {		/* -dy >= dx >= 0, dy != 0 */
		gc->line.xBig = 1;
negymajor:			/* -dy >= |dx| >= 0, dy != 0 */
		gc->line.xLittle = 0;
		gc->line.yBig = -1;
		gc->line.yLittle = -1;
		slope = dx/-dy;

		if (x0frac < 0) x0frac = -x0frac;

		totDist = x0frac - y0frac;
		if (totDist > half) iy0--;

		if (x1frac < 0) x1frac = -x1frac;

		totDist = x1frac - y1frac;
		if (totDist > half) iy1--;

		offset = y0 - iy0 - 1.0;

		gc->line.length = -dy;
		gc->line.numPixels = iy0 - iy1;
		goto ymajorfinish;
	    }
	}
    } else {
	if (dy > __GL_ZERO_F) {
	    if (-dx > dy) {	/* -dx > dy > 0 */
		gc->line.yBig = 1;
negxmajor:			/* -dx > |dy| >= 0 */
		gc->line.yLittle = 0;
		gc->line.xBig = -1;
		gc->line.xLittle = -1;
		slope = dy/-dx;

		if (y0frac < 0) y0frac = -y0frac;

		totDist = y0frac - x0frac;
		if (totDist > half) ix0--;

		if (y1frac < 0) y1frac = -y1frac;

		totDist = y1frac - x1frac;
		if (totDist > half) ix1--;

		offset = x0 - ix0 - 1.0;

		gc->line.length = -dx;
		gc->line.numPixels = ix0 - ix1;

		goto xmajorfinish;
	    } else {		/* dy >= -dx >= 0, dy != 0 */
		gc->line.xBig = -1;
		goto posymajor;
	    }
	} else {
	    if (dx < dy) {	/* -dx > -dy >= 0 */
		gc->line.yBig = -1;
		goto negxmajor;
	    } else {		/* -dy >= -dx >= 0 */
		if (dx == dy && dy == 0) {
		    gc->line.numPixels = 0;
		    return;
		}
		gc->line.xBig = -1;
		goto negymajor;
	    }
	}
    }
}

/* 690d27a0 -- __glRenderAliasLine: one colour and one texture set; the fog value honours
** EXT_fog_coord under GL_NICEST */
void __fastcall __glRenderAliasLine(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1)
{
    __GLfloat invDelta;
    __GLfloat r;
    __GLcolor *cp;
    __GLfloat offset;
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    __GL_LINE_VERT_SAVES;

    __GL_ADJUST_LINE_VERTS(gc, v0, v1);

    __glInitLineData(gc, v0, v1);
    if (gc->line.numPixels == 0) {
	__GL_RESTORE_LINE_VERTS(gc, v0, v1);
	return;
    }

    offset = gc->line.offset;

    /*
    ** Set up increments for any enabled line options.
    */
    invDelta = __GL_ONE_F / gc->line.length;
    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	__GLfloat dzdx;

	/*
	** Calculate window z coordinate increment and starting position.
	*/
	dzdx = (v1->window.z - v0->window.z) * invDelta;
	gc->polygon.shader.frag.z = (__GLzValue)(v0->window.z + dzdx * offset);
	gc->polygon.shader.dzdx = (GLint)dzdx;
	gc->polygon.shader.dzdxBig = 
	    gc->polygon.shader.dzdx << __GL_STIPPLE_COUNT_BITS;
    }

    if (modeFlags & __GL_SHADE_LINE_STIPPLE) {
	if (!gc->line.notResetStipple) {
	    gc->line.stipplePosition = 0;
	    gc->line.repeat = 0;
	    gc->line.notResetStipple = GL_TRUE;
	}
    }

    if (modeFlags & __GL_SHADE_SLOW_FOG) {
	__GLfloat f1, f0;
	__GLfloat dfdx;

	/*
	** Calculate eye z coordinate increment.
	*/
	if (gc->state.hints.fog == GL_NICEST) {
	    if (gc->state.fog.coordSource == GL_FRAGMENT_DEPTH_EXT) {
		gc->line.f0 = f0 = v0->eye.z;
		gc->polygon.shader.dfdx = dfdx = 
			(v1->eye.z - v0->eye.z) * invDelta;
	    } else {
		gc->line.f0 = f0 = v0->fogCoord;
		gc->polygon.shader.dfdx = dfdx = 
			(v1->fogCoord - v0->fogCoord) * invDelta;
	    }
	} else {
	    f0 = v0->fog;
	    f1 = v1->fog;
	    gc->line.f0 = f0;
	    gc->polygon.shader.dfdx = dfdx = (f1 - f0) * invDelta;
	}
	gc->polygon.shader.frag.f = f0 + dfdx * offset;
    }

    if (modeFlags & __GL_SHADE_TEXTURE) {
	__GLfloat v0S, v1S, v0T, v1T, v0QW, v1QW;
	__GLfloat dS, dT, dQWdX;
	__GLfloat v0rhow, v1rhow, drhow;

	v0S = v0->texture[0].x;
	v1S = v1->texture[0].x;
	v0T = v0->texture[0].y;
	v1T = v1->texture[0].y;
	v0QW = v0->texture[0].w;
	v1QW = v1->texture[0].w;

	if ((modeFlags & __GL_SHADE_PERSP_TEXTURE) &&
	    !(modeFlags & __GL_SHADE_PROJSCALED_TEXTURE))
	{
	    v0S *= v0->window.w;
	    v1S *= v1->window.w;
	    v0T *= v0->window.w;
	    v1T *= v1->window.w;
	    v0QW *= v0->window.w;
	    v1QW *= v1->window.w;
	}

	/*
	** Calculate texture s, t and q/w value increments.
	*/
	dS = (v1S - v0S) * invDelta;
	dT = (v1T - v0T) * invDelta;
	gc->polygon.shader.texIter[8] = dS;
	gc->polygon.shader.texIter[9] = dT;
	gc->polygon.shader.texIter[10] = dQWdX = (v1QW - v0QW) * invDelta;
	gc->polygon.shader.frag.texture[0] = v0S + dS * offset;
	gc->polygon.shader.frag.texture[1] = v0T + dT * offset;
	gc->polygon.shader.frag.texture[2] = v0QW + dQWdX * offset;

	v0rhow = (*(__GLcalcLineRhoProc)gc->procs.calcLineRho[0])(gc, v0S, v0T, v0QW)*v0QW;

	v1rhow = (*(__GLcalcLineRhoProc)gc->procs.calcLineRho[0])(gc, v1S, v1T, v1QW)*v1QW;

	drhow = (v1rhow - v0rhow) * invDelta;
	gc->polygon.shader.texIter[11] = drhow;
	gc->polygon.shader.frag.texture[3] = v0rhow + drhow * offset;
    } 

    if (modeFlags & __GL_SHADE_SMOOTH) {
	__GLcolor *c0 = v0->color;
	__GLcolor *c1 = v1->color;
	__GLfloat drdx, dgdx, dbdx, dadx;

	/*
	** Calculate red, green, blue and alpha value increments.
	*/
	drdx = (c1->r - c0->r) * invDelta;
	if (gc->modes.rgbMode) {
	    dgdx = (c1->g - c0->g) * invDelta;
	    dbdx = (c1->b - c0->b) * invDelta;
	    dadx = (c1->a - c0->a) * invDelta;
	    gc->polygon.shader.colorIter.dgdx = dgdx;
	    gc->polygon.shader.colorIter.dbdx = dbdx;
	    gc->polygon.shader.colorIter.dadx = dadx;
	}
	gc->polygon.shader.colorIter.drdx = drdx;
	cp = v0->color;
    } else {
	cp = v1->color;
    }

    r = cp->r;
    if (modeFlags & __GL_SHADE_RGB) {
	__GLfloat g, b, a;

	g = cp->g;
	b = cp->b;
	a = cp->a;
	gc->polygon.shader.frag.color.g = g;
	gc->polygon.shader.frag.color.b = b;
	gc->polygon.shader.frag.color.a = a;
    }
    gc->polygon.shader.frag.color.r = r;

    gc->polygon.shader.length = gc->line.numPixels;
    (*gc->procs.line.processLine)(gc);

    __GL_RESTORE_LINE_VERTS(gc, v0, v1);
}

/* 690d2ea0 -- __glRenderFlatFogLine */
void __fastcall __glRenderFlatFogLine(__GLcontext *gc, __GLvertexICD *v0,
			   __GLvertexICD *v1)
{
    __GLcolor v0col, v1col;
    __GLcolor *v0ocp, *v1ocp;

    (*(__GLfogColorProc)gc->procs.fogColor)(gc, &v0col, v1->color, v0->fog);
    (*(__GLfogColorProc)gc->procs.fogColor)(gc, &v1col, v1->color, v1->fog);
    v0ocp = v0->color;
    v1ocp = v1->color;
    v0->color = &v0col;
    v1->color = &v1col;

    (*(__GLrenderLineProc)gc->procs.renderLine2)(gc, v0, v1);
    
    v0->color = v0ocp;
    v1->color = v1ocp;
}

/* 690d2f10 -- InitAALineData (s_lndraw.c): set up gc->line (v0 / v1 recorded) for an antialiased line (window
** coordinates less half a pixel): the real length and the unit along-line (dldx/dldy) and across-line (dddx/dddy)
** deltas, then per octant the major axis, the big / little x / y steps with their dl / dd increments, start pixel and
** offset, length, pixel count, the width in minor-axis pixels (ceil(smoothWidth * realLength / major delta) + 1), and
** the minor start and slope as 2^31 fixed-point fraction / dfraction; a zero-length line just sets line.length = 0
** (after dividing by the zero length) */
void __fastcall InitAALineData(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1)
{
    GLint start;
    __GLfloat width;
    __GLfloat x0,y0,x1,y1;
    __GLfloat minorStart;
    GLint intMinorStart;
    __GLfloat dx, dy;
    __GLfloat offset;
    __GLfloat slope;
    __GLfloat halfWidth;
    __GLfloat realLength, oneOverRealLength;
    __GLfloat dldx, dldy;
    __GLfloat dddx, dddy;

    gc->line.v0 = v0;
    gc->line.v1 = v1;

    x0=v0->window.x;
    y0=v0->window.y;
    x1=v1->window.x;
    y1=v1->window.y;

    x0 -= gc->constants.half;
    y0 -= gc->constants.half;
    x1 -= gc->constants.half;
    y1 -= gc->constants.half;

    dx=x1-x0;
    dy=y1-y0;
    realLength = sqrt(dx*dx+dy*dy);
    oneOverRealLength = __GL_ONE_F / realLength;
    gc->line.realLength = realLength;
    gc->line.dldx = dldx = dx * oneOverRealLength;
    gc->line.dldy = dldy = dy * oneOverRealLength;
    gc->line.dddx = dddx = -dldy;
    gc->line.dddy = dddy = dldx;

    if (dx > __GL_ZERO_F) {
	if (dy > __GL_ZERO_F) {	/* dx > 0, dy > 0 */
	    gc->line.dlBig = dldx + dldy;
	    gc->line.ddBig = dddx + dddy;
	    if (dx > dy) {	/* dx > dy > 0 */
		gc->line.yBig = 1;
posxmajor:			/* dx > |dy| >= 0 */
		gc->line.yLittle = 0;
		gc->line.xBig = 1;
		gc->line.xLittle = 1;
		gc->line.dlLittle = dldx;
		gc->line.ddLittle = dddx;
		slope = dy/dx;
		start = (GLint) x0;
		offset = start + (__GLfloat)(gc->constants.half) - x0;

		gc->line.length = dx;
		gc->line.numPixels = (GLint)(__GL_CEILF(x1 - x0) + 1);

		width = __GL_CEILF(gc->state.line.smoothWidth * 
			realLength / dx);
xmajorfinish:
		gc->line.width = (GLint)width + 1;
		halfWidth = width * (__GLfloat)(gc->constants.half);

		gc->line.axis = __GL_X_MAJOR;
		gc->line.xStart = start;
		gc->line.offset = offset;
		minorStart = y0 + offset*slope - halfWidth;
		intMinorStart = (GLint) minorStart;
		minorStart -= intMinorStart;
		gc->line.yStart = intMinorStart;
		gc->line.dfraction = (GLint)(slope * __TWO_31);
		gc->line.fraction = (GLint)(minorStart * __TWO_31);
	    } else {		/* dy >= dx > 0 */
		gc->line.xBig = 1;
posymajor:			/* dy >= |dx| >= 0, dy != 0 */
		gc->line.xLittle = 0;
		gc->line.yBig = 1;
		gc->line.yLittle = 1;
		gc->line.dlLittle = dldy;
		gc->line.ddLittle = dddy;
		slope = dx/dy;
		start = (GLint) y0;
		offset = start + (__GLfloat)(gc->constants.half) - y0;

		gc->line.length = dy;
		gc->line.numPixels = (GLint)__GL_CEILF(y1 - y0) + 1;

		width = __GL_CEILF(gc->state.line.smoothWidth * 
			realLength / dy);
ymajorfinish:
		gc->line.width = (GLint)width + 1;
		halfWidth = width * (__GLfloat)(gc->constants.half);

		gc->line.axis = __GL_Y_MAJOR;
		gc->line.yStart = start;
		gc->line.offset = offset;
		minorStart = x0 + offset*slope - halfWidth;
		intMinorStart = (GLint) minorStart;
		minorStart -= intMinorStart;
		gc->line.xStart = intMinorStart;
		gc->line.dfraction = (GLint)(slope * __TWO_31);
		gc->line.fraction = (GLint)(minorStart * __TWO_31);
	    }
	} else {		/* dx > 0, dy <= 0 */
	    gc->line.dlBig = dldx - dldy;
	    gc->line.ddBig = dddx - dddy;
	    if (dx > -dy) {	/* dx > -dy >= 0 */
		gc->line.yBig = -1;
		goto posxmajor;
	    } else {		/* -dy >= dx >= 0, dy != 0 */
		gc->line.xBig = 1;
negymajor:			/* -dy >= |dx| >= 0, dy != 0 */
		gc->line.xLittle = 0;
		gc->line.yBig = -1;
		gc->line.yLittle = -1;
		gc->line.dlLittle = -dldy;
		gc->line.ddLittle = -dddy;
		slope = dx/-dy;
		start = (GLint) y0;
		offset = y0 - (start + (__GLfloat)(gc->constants.half));

		gc->line.length = -dy;
		gc->line.numPixels = (GLint)__GL_CEILF(y0 - y1) + 1;

		width = __GL_CEILF(-gc->state.line.smoothWidth * 
			realLength / dy);

		goto ymajorfinish;
	    }
	}
    } else {
	if (dy > __GL_ZERO_F) {	/* dx <= 0, dy > 0 */
	    gc->line.dlBig = dldy - dldx;
	    gc->line.ddBig = dddy - dddx;
	    if (-dx > dy) {	/* -dx > dy > 0 */
		gc->line.yBig = 1;
negxmajor:			/* -dx > |dy| >= 0 */
		gc->line.yLittle = 0;
		gc->line.xBig = -1;
		gc->line.xLittle = -1;
		gc->line.dlLittle = -dldx;
		gc->line.ddLittle = -dddx;
		slope = dy/-dx;
		start = (GLint) x0;
		offset = x0 - (start + (__GLfloat)(gc->constants.half));

		gc->line.length = -dx;
		gc->line.numPixels = (GLint)__GL_CEILF(x0 - x1) + 1;

		width = __GL_CEILF(-gc->state.line.smoothWidth * 
			realLength / dx);

		goto xmajorfinish;
	    } else {		/* dy >= -dx >= 0, dy != 0 */
		gc->line.xBig = -1;
		goto posymajor;
	    }
	} else {		/* dx <= 0, dy <= 0 */
	    gc->line.dlBig = -dldx - dldy;
	    gc->line.ddBig = -dddx - dddy;
	    if (dx < dy) {	/* -dx > -dy >= 0 */
		gc->line.yBig = -1;
		goto negxmajor;
	    } else {		/* -dy >= -dx >= 0 */
		if (dx == dy && dy == 0) {
		    gc->line.length = 0;
		    return;
		}
		gc->line.xBig = -1;
		goto negymajor;
	    }
	}
    }
}

/* 690d3480 -- __glRenderAntiAliasLine */
void __fastcall __glRenderAntiAliasLine(__GLcontext *gc, __GLvertexICD *v0, __GLvertexICD *v1)
{
    __GLfloat invDelta;
    __GLcolor *cp;
    __GLfloat offset;
    GLint lineRep;
    GLint x, y, xBig, xLittle, yBig, yLittle;
    GLint fraction, dfraction;
    __GLfloat dlLittle, dlBig;
    __GLfloat ddLittle, ddBig;
    __GLfloat length, width;
    __GLfloat dx, dy;
    GLuint modeFlags = __GL_SHADE_MODEFLAGS(gc);
    __GL_LINE_VERT_SAVES;

    __GL_ADJUST_LINE_VERTS(gc, v0, v1);

    InitAALineData(gc, v0, v1);
    if (gc->line.length == 0) {
	__GL_RESTORE_LINE_VERTS(gc, v0, v1);
	return;
    }

    offset = gc->line.offset;

    /*
    ** Set up increments for any enabled line options.
    */
    invDelta = __GL_ONE_F / gc->line.length;
    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
	/*
	** Calculate window z coordinate increment and starting position.
	*/
	gc->polygon.shader.dzdx = (GLint)((v1->window.z - v0->window.z) * 
			invDelta);
	gc->polygon.shader.dzdxBig =
	    gc->polygon.shader.dzdx << __GL_STIPPLE_COUNT_BITS;
	gc->polygon.shader.frag.z = (GLint)(v0->window.z + 
		gc->polygon.shader.dzdx * offset);
    } 

    if (modeFlags & __GL_SHADE_LINE_STIPPLE) {
	if (!gc->line.notResetStipple) {
	    gc->line.stipplePosition = 0;
	    gc->line.repeat = 0;
	    gc->line.notResetStipple = GL_TRUE;
	}
    }

    if (modeFlags & __GL_SHADE_SLOW_FOG) {
	/*
	** Calculate eye z coordinate increment.
	*/
	if (gc->state.hints.fog == GL_NICEST) {
	    if (gc->state.fog.coordSource == GL_FRAGMENT_DEPTH_EXT) {
		gc->line.f0 = v0->eye.z;
		gc->polygon.shader.dfdx = (v1->eye.z - v0->eye.z) * invDelta;
	    } else {
		gc->line.f0 = v0->fogCoord;
		gc->polygon.shader.dfdx = (v1->fogCoord - v0->fogCoord) * invDelta;
	    }
	} else {
	    __GLfloat f1, f0;

	    f0 = v0->fog;
	    f1 = v1->fog;
	    gc->line.f0 = f0;
	    gc->polygon.shader.dfdx = (f1 - f0) * invDelta;
	}
	gc->polygon.shader.frag.f = gc->line.f0 + 
		gc->polygon.shader.dfdx * offset;
    } 

    if (modeFlags & __GL_SHADE_SMOOTH) {
	__GLcolor *c0 = v0->color;
	__GLcolor *c1 = v1->color;

	/*
	** Calculate red, green, blue and alpha value increments.
	*/
	gc->polygon.shader.colorIter.drdx = (c1->r - c0->r) * invDelta;
	if (gc->modes.rgbMode) {
	    gc->polygon.shader.colorIter.dgdx = (c1->g - c0->g) * invDelta;
	    gc->polygon.shader.colorIter.dbdx = (c1->b - c0->b) * invDelta;
	    gc->polygon.shader.colorIter.dadx = (c1->a - c0->a) * invDelta;
	}
	cp = v0->color;
    } else {
	cp = v1->color;
    }
    
    gc->polygon.shader.frag.color.r = cp->r;
    if (gc->modes.rgbMode) {
	gc->polygon.shader.frag.color.g = cp->g;
	gc->polygon.shader.frag.color.b = cp->b;
	gc->polygon.shader.frag.color.a = cp->a;
    }
    
    if (modeFlags & __GL_SHADE_TEXTURE) {
	__GLfloat v0S, v1S, v0T, v1T, v0QW, v1QW;
	__GLfloat dS, dT, dQWdX;
	__GLfloat v0rhow, v1rhow, drhow;

	v0S = v0->texture[0].x;
	v1S = v1->texture[0].x;
	v0T = v0->texture[0].y;
	v1T = v1->texture[0].y;
	v0QW = v0->texture[0].w;
	v1QW = v1->texture[0].w;

	if ((modeFlags & __GL_SHADE_PERSP_TEXTURE) &&
	    !(modeFlags & __GL_SHADE_PROJSCALED_TEXTURE))
	{
	    v0S *= v0->window.w;
	    v1S *= v1->window.w;
	    v0T *= v0->window.w;
	    v1T *= v1->window.w;
	    v0QW *= v0->window.w;
	    v1QW *= v1->window.w;
	}

	/*
	** Calculate texture s, t and q/w value increments.
	*/
	dS = (v1S - v0S) * invDelta;
	dT = (v1T - v0T) * invDelta;
	gc->polygon.shader.texIter[8] = dS;
	gc->polygon.shader.texIter[9] = dT;
	gc->polygon.shader.texIter[10] = dQWdX = (v1QW - v0QW) * invDelta;
	gc->polygon.shader.frag.texture[0] = v0S + dS * offset;
	gc->polygon.shader.frag.texture[1] = v0T + dT * offset;
	gc->polygon.shader.frag.texture[2] = v0QW + dQWdX * offset;

	v0rhow = (*(__GLcalcLineRhoProc)gc->procs.calcLineRho[0])(gc, v0S, v0T, v0QW)*v0QW;

	v1rhow = (*(__GLcalcLineRhoProc)gc->procs.calcLineRho[0])(gc, v1S, v1T, v1QW)*v1QW;

	drhow = (v1rhow - v0rhow) * invDelta;
	gc->polygon.shader.texIter[11] = drhow;
	gc->polygon.shader.frag.texture[3] = v0rhow + drhow * offset;
    }

    lineRep = gc->line.width;

    fraction = gc->line.fraction;
    dfraction = gc->line.dfraction;

    x = gc->line.xStart;
    y = gc->line.yStart;
    xBig = gc->line.xBig;
    yBig = gc->line.yBig;
    xLittle = gc->line.xLittle;
    yLittle = gc->line.yLittle;

    dlLittle = gc->line.dlLittle;
    dlBig = gc->line.dlBig;
    ddLittle = gc->line.ddLittle;
    ddBig = gc->line.ddBig;

    dx = x + gc->constants.half - v0->window.x;
    dy = y + gc->constants.half - v0->window.y;
    length = dx * gc->line.dldx +
	    dy * gc->line.dldy;
    width = dx * gc->line.dddx +
	    dy * gc->line.dddy;

    if (modeFlags & __GL_SHADE_LINE_STIPPLE) {
	gc->line.stippleOffset = 
		gc->line.stipplePosition * gc->state.line.stippleRepeat +
		gc->line.repeat - gc->constants.half;

	/* XXX Move to a validation routine? */
	gc->line.oneOverStippleRepeat = 
		__GL_ONE_F / gc->state.line.stippleRepeat;
    }

    while (--lineRep >= 0) {
	/* Trace the line backwards as needed */
	while (length > -gc->constants.half) {
	    fraction -= dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		length -= dlBig;
		width -= ddBig;
		x -= xBig;
		y -= yBig;
	    } else {
		length -= dlLittle;
		width -= ddLittle;
		x -= xLittle;
		y -= yLittle;
	    }
	}

	/* Trace line forwards to correct */
	while (length <= -gc->constants.half) {
	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		length += dlBig;
		width += ddBig;
		x += xBig;
		y += yBig;
	    } else {
		length += dlLittle;
		width += ddLittle;
		x += xLittle;
		y += yLittle;
	    }
	}

	/* Save new fraction/dfraction */
	gc->line.plength = length;
	gc->line.pwidth = width;
	gc->line.fraction = fraction;
	gc->line.dfraction = dfraction;
	gc->line.xStart = x;
	gc->line.yStart = y;
    
	gc->polygon.shader.length = gc->line.numPixels;
	(*gc->procs.line.processLine)(gc);

	if (gc->line.axis == __GL_X_MAJOR) {
	    y++;
	    length += gc->line.dldy;
	    width += gc->line.dddy;
	} else {
	    x++;
	    length += gc->line.dldx;
	    width += gc->line.dddx;
	}
    }

    if (modeFlags & __GL_SHADE_LINE_STIPPLE) {
	/* Update stipple.  Ugh. */
	int increase;
	int posInc;

	/* Shift stipple by 'increase' bits */
	increase = (GLint)__GL_CEILF(gc->line.realLength);

	posInc = increase / gc->state.line.stippleRepeat;

	gc->line.stipplePosition = (gc->line.stipplePosition + posInc) & 0xf;
	gc->line.repeat = (gc->line.repeat + increase) % gc->state.line.stippleRepeat;
    }

    __GL_RESTORE_LINE_VERTS(gc, v0, v1);
}

/*
** glshade.h -- the ICD's polygon shader as the s_pgdraw.c / s_pgaa.c ports use it (gl_pgdraw.c,
** gl_pgaa.c): render.h shade flags (the ICD moved the texture ones), modeFlags read through
** polygon.shader.getModeFlags, buffers.h addressing on the WGL buffers, and the ICD's
** __GL_ADJUST_TRIANGLE_VERTS / __GL_RESTORE_TRIANGLE_VERTS.
*/
#ifndef __GLSHADE_H__
#define __GLSHADE_H__

#define __TWO_31 2147483648.0

#define __FRACTION(result,f) \
    result = (GLint) ((f) * __TWO_31)

/* render.h shade flags */
#define __GL_SHADE_RGB                  0x0001
#define __GL_SHADE_SMOOTH               0x0002
#define __GL_SHADE_DEPTH_TEST           0x0004
#define __GL_SHADE_TEXTURE              0x0008
#define __GL_SHADE_STIPPLE              0x0010
#define __GL_SHADE_STENCIL_TEST         0x0020
#define __GL_SHADE_DITHER               0x00000040
#define __GL_SHADE_LOGICOP              0x00000080
#define __GL_SHADE_BLEND                0x00000100
#define __GL_SHADE_ALPHA_TEST           0x0200
#define __GL_SHADE_TWOSIDED             0x0400
#define __GL_SHADE_MASK                 0x0800          /* a colour / index write mask is set */
#define __GL_SHADE_SLOW_FOG             0x1000
#define __GL_SHADE_CHEAP_FOG            0x2000
#define __GL_SHADE_DEPTH_ITER           0x4000
#define __GL_SHADE_LINE_STIPPLE         0x8000
#define __GL_SHADE_CULL_FACE           0x00010000
#define __GL_SHADE_SMOOTH_LIGHT         0x00020000
#define __GL_SHADE_POLYGON_OFFSET_FILL  0x00100000
#define __GL_SHADE_INDEX_TEST          0x00200000      /* ICD: CI mode with GL_INDEX_TEST_EXT */
#define __GL_SHADE_OWNERSHIP_TEST       0x00400000
/* the ICD moved the texture flags: perspective 0x2000000, projection-scaled 0x800000 */
#define __GL_SHADE_PROJSCALED_TEXTURE   0x00800000
#define __GL_SHADE_PERSP_TEXTURE        0x02000000
#define __GL_SHADE_MIPMAP_TEXTURE       0x04000000      /* ICD: unit 0 minifies with a mipmap filter */
/* ICD: the fragment alpha is needed (blending, the alpha test or alpha bitplanes; the sst mode flags 69064... set
** it with the blend / alpha-test bits) */
#define __GL_SHADE_NEEDS_ALPHA          0x08000000

#define GL_FRAGMENT_DEPTH_EXT   0x8452

/* buffers.h with the ICD's buffers (see gl_lnspan.c): a stencil cell is the top byte of its element */
typedef GLubyte __GLstencilCell;
typedef __WGLbuffer __GLdrawableBuffer;
#define __GL_DEPTH_ADDR(gc,fb,drawableBuf,cast,x,y) \
    (cast (((GLubyte *) (drawableBuf)->base) \
	+ (((y) - (gc)->constants.viewportYAdjust) \
            * (drawableBuf)->outerWidth \
	   + (x) - (gc)->constants.viewportXAdjust) \
	* (drawableBuf)->elementSize))
#define __GL_STENCIL_ADDR(gc,fb,drawableBuf,cast,x,y) \
    (cast (((GLubyte *) (drawableBuf)->base) \
	+ (((y) - (gc)->constants.viewportYAdjust) \
            * (drawableBuf)->outerWidth \
	   + (x) - (gc)->constants.viewportXAdjust + 1) \
	* (drawableBuf)->elementSize - 1))

typedef __GLshadeTailICD __GLshade;
#define __GL_SHADE_MODEFLAGS(gc) \
    (((GLuint (__fastcall *)(__GLcontext *))(gc)->polygon.shader.getModeFlags)(gc))
#define __GL_BUF_WGL(b)         ((b).buf.wglBuf)
#define __GL_ABSF(f)            fabs(f)
#define __GL_PGON_OFFSET_NEAR_ZERO .00001

typedef GLfloat (__fastcall *__GLcalcRhoProc)(__GLcontext *gc, __GLshade *sh,
					      __GLfloat s, __GLfloat t, __GLfloat q);
typedef void (__fastcall *__GLfogColorProc)(__GLcontext *gc, __GLcolor *out, __GLcolor *in,
					    __GLfloat fog);
typedef void (__fastcall *__GLfillTriangleProc)(__GLcontext *gc, __GLvertexICD *a,
						__GLvertexICD *b, __GLvertexICD *c);
typedef void (__fastcall *__GLrenderLineProc)(__GLcontext *gc, __GLvertexICD *v0,
					      __GLvertexICD *v1);

/* s_pgdraw.c procedures shared with s_pgaa.c */
extern void __fastcall __glSnapXLeftProc(__GLcontext *gc, GLuint edx_unused, __GLfloat xLeft, __GLfloat dxdyLeft);
extern void __fastcall __glSnapXRightProc(__GLshade *sh, GLuint edx_unused, __GLfloat xRight, __GLfloat dxdyRight);
extern __GLzValue __fastcall __glPolygonOffsetZ(__GLcontext *gc, const __GLvertexICD *a,
					  __GLfloat dx, __GLfloat dy);
/* the snap procs' EDX slot is unused: call them as (ptr, float, float) so EDX is not loaded */
#define __glSnapXLeft(gc, x, dxdy) \
    (((void (__fastcall *)(__GLcontext *, __GLfloat, __GLfloat))__glSnapXLeftProc)(gc, x, dxdy))
#define __glSnapXRight(sh, x, dxdy) \
    (((void (__fastcall *)(__GLshade *, __GLfloat, __GLfloat))__glSnapXRightProc)(sh, x, dxdy))

/*
** __GL_ADJUST_TRIANGLE_VERTS, ICD form: the window coordinates are biased by 0x1800 (6144.5 with
** the pixel centre) so they stay positive, z and the texture s, t are rescaled and the colours
** are scaled to the internal ranges; __GL_RESTORE_TRIANGLE_VERTS puts everything back.  The
** saved values live in the caller's __GL_TRIANGLE_VERT_SAVES struct, laid out as SGI's
** __GLadjustTriangleData (+ the textures): its member order is the original's frame order.
*/
#define __GL_VERTEX_BIAS        0x1800
#define __GL_VERTEX_BIAS_F      6144.5f

#define __GL_TRIANGLE_VERT_SAVES \
    struct { __GLcoord window[3]; __GLcolor color[4], *pvcolor; __GLcoord texture[3]; } adjustData

#define __GL_ADJUST_VERT(gc, v, X, Y, Z, T) \
    X = v->window.x; v->window.x += __GL_VERTEX_BIAS_F; \
    Y = v->window.y; v->window.y += __GL_VERTEX_BIAS_F; \
    Z = v->window.z; v->window.z *= gc->constants.depthRescale; \
    T = v->texture[0]; \
    v->texture[0].x *= gc->constants.texSScale; \
    v->texture[0].y *= gc->constants.texTScale

#define __GL_ADJUST_VERT_COLOR(gc, v, C) \
    if (v->color != adjustData.pvcolor) { \
	C = *v->color; \
	v->color->r *= gc->constants.redRescale; \
	v->color->g *= gc->constants.greenRescale; \
	v->color->b *= gc->constants.blueRescale; \
	v->color->a *= gc->constants.alphaRescale; \
    }

#define __GL_ADJUST_TRIANGLE_VERTS(gc, oa, ob, oc, pv) \
    gc->constants.viewportXAdjust += __GL_VERTEX_BIAS; \
    gc->constants.viewportYAdjust += __GL_VERTEX_BIAS; \
    gc->transform.clipX0 += __GL_VERTEX_BIAS; \
    gc->transform.clipX1 += __GL_VERTEX_BIAS; \
    gc->transform.clipY0 += __GL_VERTEX_BIAS; \
    gc->transform.clipY1 += __GL_VERTEX_BIAS; \
    __GL_ADJUST_VERT(gc, oa, adjustData.window[0].x, adjustData.window[0].y, adjustData.window[0].z, adjustData.texture[0]); \
    __GL_ADJUST_VERT(gc, ob, adjustData.window[1].x, adjustData.window[1].y, adjustData.window[1].z, adjustData.texture[1]); \
    __GL_ADJUST_VERT(gc, oc, adjustData.window[2].x, adjustData.window[2].y, adjustData.window[2].z, adjustData.texture[2]); \
    adjustData.pvcolor = pv ? pv->color : NULL; \
    if (adjustData.pvcolor) { \
	adjustData.color[3] = *adjustData.pvcolor; \
	pv->color->r *= gc->constants.redRescale; \
	pv->color->g *= gc->constants.greenRescale; \
	pv->color->b *= gc->constants.blueRescale; \
	pv->color->a *= gc->constants.alphaRescale; \
    } \
    __GL_ADJUST_VERT_COLOR(gc, oa, adjustData.color[0]) \
    __GL_ADJUST_VERT_COLOR(gc, ob, adjustData.color[1]) \
    __GL_ADJUST_VERT_COLOR(gc, oc, adjustData.color[2])

#define __GL_RESTORE_VERT(v, X, Y, Z, T) \
    v->window.x = X; \
    v->window.y = Y; \
    v->window.z = Z; \
    v->texture[0].x = T.x; \
    v->texture[0].y = T.y

#define __GL_RESTORE_VERT_COLOR(v, C) \
    if (v->color != adjustData.pvcolor) { \
	v->color->r = C.r; \
	v->color->g = C.g; \
	v->color->b = C.b; \
	v->color->a = C.a; \
    }

#define __GL_RESTORE_TRIANGLE_VERTS(gc, oa, ob, oc, pv) \
    gc->constants.viewportXAdjust -= __GL_VERTEX_BIAS; \
    gc->constants.viewportYAdjust -= __GL_VERTEX_BIAS; \
    gc->transform.clipX0 -= __GL_VERTEX_BIAS; \
    gc->transform.clipX1 -= __GL_VERTEX_BIAS; \
    gc->transform.clipY0 -= __GL_VERTEX_BIAS; \
    gc->transform.clipY1 -= __GL_VERTEX_BIAS; \
    __GL_RESTORE_VERT(oa, adjustData.window[0].x, adjustData.window[0].y, adjustData.window[0].z, adjustData.texture[0]); \
    __GL_RESTORE_VERT(ob, adjustData.window[1].x, adjustData.window[1].y, adjustData.window[1].z, adjustData.texture[1]); \
    __GL_RESTORE_VERT(oc, adjustData.window[2].x, adjustData.window[2].y, adjustData.window[2].z, adjustData.texture[2]); \
    if (adjustData.pvcolor) { \
	pv->color->r = adjustData.color[3].r; \
	pv->color->g = adjustData.color[3].g; \
	pv->color->b = adjustData.color[3].b; \
	pv->color->a = adjustData.color[3].a; \
    } \
    __GL_RESTORE_VERT_COLOR(oa, adjustData.color[0]) \
    __GL_RESTORE_VERT_COLOR(ob, adjustData.color[1]) \
    __GL_RESTORE_VERT_COLOR(oc, adjustData.color[2])

#endif /* __GLSHADE_H__ */

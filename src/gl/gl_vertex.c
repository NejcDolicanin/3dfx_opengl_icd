/*
** gl_vertex.c -- s_vertex.c / s_vapi.c: the vertex save procs, glVertex* / glNormal* / glColor* entries and
** their converters (SI port: si_s_vertex.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "glsitypes.h"
#include "si_names.h"
#include "glshade.h"
#include "icdglob.h"
#include "icdprotos.h"
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>

/* s_vertex.c's save procs by the needs bits (690f3ed0, 690f3f10; defined at the end of the file) */
static __GLproc CISaveProcs[16];
static __GLproc RGBSaveProcs[16];

#define DO_VALIDATE(gc, v, needs) \
    (*gc->procs.validateVertex[((v)->flagBits >> 14) & 3])(gc, v, needs)
/* ---- s_vertex.c save procs ---------------------------------------------------------------
** The ICD's __glSave* procs: the SI bodies with the texture-unit loop written out for its two
** units, plus "F" variants that also copy the EXT_fog_coord value (vx->fogCoord). */
typedef void (__fastcall *__GLvertexProc)(__GLcontext *gc, __GLvertexICD *vx);
#define SAVE_TEX(i) \
    x = gc->state.current.texture[i].x; \
    y = gc->state.current.texture[i].y; \
    z = gc->state.current.texture[i].z; \
    w = gc->state.current.texture[i].w; \
    vx->texture[i].x = x; \
    vx->texture[i].y = y; \
    vx->texture[i].z = z; \
    vx->texture[i].w = w;
#define SAVE_T() SAVE_TEX(0) SAVE_TEX(1)
#define SAVE_C() \
    r = gc->state.current.color.r; \
    g = gc->state.current.color.g; \
    b = gc->state.current.color.b; \
    a = gc->state.current.color.a; \
    vx->colors[__GL_FRONTFACE].r = r; \
    vx->colors[__GL_FRONTFACE].g = g; \
    vx->colors[__GL_FRONTFACE].b = b; \
    vx->colors[__GL_FRONTFACE].a = a;
#define SAVE_N() \
    x = gc->state.current.normal.x; \
    y = gc->state.current.normal.y; \
    z = gc->state.current.normal.z; \
    vx->normal.x = x; \
    vx->normal.y = y; \
    vx->normal.z = z;
#define SAVE_F() \
    x = gc->state.current.fogCoord; \
    vx->fogCoord = x;
/* ---- s_vertex.c: __glGenericPickVertexProcs ------------------------------------------------
** The ICD's version: four-bit save-proc index (bit 3 = the EXT_fog_coord copy), both texture
** units, the i386 frustum checker __glClipCheckFast and no vertex cache. */
/* 6907da30 (__glNop3: the empty 3-argument proc, si_names.h NopSpanRender) */
#define __NORMAL 1
#define __COLOR 2
#define __TEX 4
#define __FOGCOORD 8
/* ---- s_vertex.c: validate procs that transform the object position to clip space ------------ */
typedef void (__fastcall *__GLxf)(__GLcoord *res, const __GLfloat *v, __GLmatrixICD *m);
/************************************************************************/
/* the immediate-mode Vertex*fv entries inside Begin/End: an inline vertex routine (its parameters are
** evaluated before any store, which is what makes the original load all coordinates first) stores the
** object coordinates into gc->vertex.v0, transforms them by the modelview's mvp, clip-checks and
** hands the vertex to procs.v */
typedef void (__fastcall *__GLxformProc)(__GLcoord *res, const __GLfloat *v, const __GLmatrixICD *m);
typedef GLuint (__fastcall *__GLclipCheckProc)(__GLcontext *gc, __GLvertexICD *vx);
static __inline void Vertex2W19(__GLcontext *gc, GLfloat x, GLfloat y)
{
    __GLvertexICD *vx = gc->vertex.v0;
    __GLtransformICD *tr = gc->transform.modelView;

    vx->obj.x = x;
    vx->obj.y = y;
    vx->obj.z = __GL_ZERO_F;
    vx->obj.w = __GL_ONE_F;
    (*(__GLxformProc)tr->mvp.xf2)(&vx->clip, &vx->obj.x, &tr->mvp);
    vx->flagBits = 0x4000;
    vx->flagBits |= (*(__GLclipCheckProc)gc->procs.clipCheck2)(gc, vx);
    (*(__GLvertexProc)gc->procs.v)(gc, vx);
}
/* 690a5070 (__glTextureFragment) is in gl_texfrag.c */

/************************************************************************/
/* the Vertex converters (dispatch +0x208 / +0x228 / +0x248 = Vertex2fv / 3fv / 4fv; earlier comments called
** them RasterPos): convert to floats, then the fv entry of the current dispatch */
typedef void (APIENTRY *__GLfvProc)(const GLfloat *v);
#define DISP_FV(off)    ((__GLfvProc)__glCurrentGC()->dispatchState->fn[(off) / 4])
/* ---- vertex API entries that convert and call through the dispatch table ------------------- */
#define DISPATCH(gc, n, type)       ((type)(gc)->dispatchState->fn[n])
typedef void (APIENTRY *__GLvertex2fv)(const GLfloat *);
typedef void (APIENTRY *__GLvertexfv)(const GLfloat *);

/* ---- s_vertex.c clip checks: no w == 0 test and the window coordinates are always written --- */

/* 6907a5f0 -- __glClipCheckFrustum */
GLuint __fastcall __glClipCheckFrustum(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat x, y, z, w, negW, invW;
    GLuint code;

    w = vx->clip.w;
    invW = __GL_ONE_F / w;
    x = vx->clip.x;
    y = vx->clip.y;
    z = vx->clip.z;
    negW = -w;

    code = 0;
    if (x < negW) code |= __GL_CLIP_LEFT;
    if (x > w) code |= __GL_CLIP_RIGHT;
    if (y < negW) code |= __GL_CLIP_BOTTOM;
    if (y > w) code |= __GL_CLIP_TOP;
    if (z < negW) code |= __GL_CLIP_NEAR;
    if (z > w) code |= __GL_CLIP_FAR;

    vx->window.w = invW;
    vx->flagBits |= __GL_HAS_CLIP;

    {
        __GLviewport *vp = &gc->state.viewport;
        __GLfloat wx, wy, wz;

        wx = x * vp->scale.x * invW + vp->center.x;
        wy = y * vp->scale.y * invW + vp->center.y;
        wz = z * vp->scale.z * invW + vp->center.z;
        vx->window.x = wx;
        vx->window.y = wy;
        vx->window.z = wz;
    }

    return code;
}

/* 6907a6f0 -- __glClipCheckFrustum2D */
GLuint __fastcall __glClipCheckFrustum2D(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat x, y, z, w, negW, invW;
    GLuint code;

    w = vx->clip.w;
    invW = w;		/* W is 1.0 */
    x = vx->clip.x;
    y = vx->clip.y;
    negW = -w;

    code = 0;
    if (x < negW) code |= __GL_CLIP_LEFT;
    if (x > w) code |= __GL_CLIP_RIGHT;
    if (y < negW) code |= __GL_CLIP_BOTTOM;
    if (y > w) code |= __GL_CLIP_TOP;

    vx->window.w = invW;
    vx->flagBits |= __GL_HAS_CLIP;

    {
	__GLviewport *vp = &gc->state.viewport;
	__GLfloat wx, wy, wz;

	z = vx->clip.z;
	wx = x * vp->scale.x + vp->center.x;
	wy = y * vp->scale.y + vp->center.y;
	wz = z * vp->scale.z + vp->center.z;
	vx->window.x = wx;
	vx->window.y = wy;
	vx->window.z = wz;
    }

    return code;
}

/* 6907a810 -- s_vertex.c __glClipCheckAll (3dfx GLCORE/S_VERTEX.C): frustum and user clip planes,
** returns the vertex flags with the clip codes.  x / y / z / w are reused for the eye coordinates as in
** the ancestor: that slot sharing gives the original's summation order of the plane dot product. */
GLuint __fastcall __glClipCheckAll(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat x, y, z, w, negW, invW, zero, dot;
    GLuint code, bit, clipPlanesMask;
    __GLcoord *plane;

    /* We need eye coordinates to do user clip plane clipping */
    DO_VALIDATE(gc, vx, __GL_HAS_EYE);

    /*
    ** Do frustum checks.
    **
    ** NOTE: it is possible for x to be less than negW and greater than w
    ** (if w is negative).  Otherwise there would be "else" clauses here.
    */
    w = vx->clip.w;
    invW = __GL_ONE_F / w;
    x = vx->clip.x;
    y = vx->clip.y;
    z = vx->clip.z;
    negW = -w;
    code = 0;
    if (x < negW) code |= 0x10000;     /* __GL_CLIP_LEFT */
    if (x > w) code |= 0x20000;        /* __GL_CLIP_RIGHT */
    if (y < negW) code |= 0x40000;     /* __GL_CLIP_BOTTOM */
    if (y > w) code |= 0x80000;        /* __GL_CLIP_TOP */
    if (z < negW) code |= 0x100000;    /* __GL_CLIP_NEAR */
    if (z > w) code |= 0x200000;       /* __GL_CLIP_FAR */
    vx->window.w = invW;

    /*
    ** Now do user clip plane checks
    */
    x = vx->eye.x;
    y = vx->eye.y;
    z = vx->eye.z;
    w = vx->eye.w;
    clipPlanesMask = gc->state.enables.clipPlanes;
    plane = &gc->state.transform.eyeClipPlanes[0];
    bit = 0x400000;                     /* __GL_CLIP_USER0 */
    zero = __GL_ZERO_F;
    while (clipPlanesMask) {
	if (clipPlanesMask & 1) {
	    /*
	    ** Dot the vertex clip coordinate against the clip plane and see
	    ** if the sign is negative.  If so, then the point is out.
	    */
	    dot = x * plane->x + y * plane->y + z * plane->z + w * plane->w;
	    if (dot < zero) {
		code |= bit;
	    }
	}
	clipPlanesMask >>= 1;
	bit <<= 1;
	plane++;
    }
    vx->flagBits |= __GL_HAS_CLIP;

    /* Compute window coordinates if not clipped */
    if (!code) {
	__GLviewport *vp = &gc->state.viewport;
	__GLfloat wx, wy, wz;

	x = vx->clip.x;
	y = vx->clip.y;
	z = vx->clip.z;
	wx = x * vp->scale.x * invW + vp->center.x;
	wy = y * vp->scale.y * invW + vp->center.y;
	wz = z * vp->scale.z * invW + vp->center.z;
	vx->window.x = wx;
	vx->window.y = wy;
	vx->window.z = wz;
    }

    return vx->flagBits | code;
}

/* 6907aa60 -- __glSaveT */
void __fastcall __glSaveT(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLvertexProc vertex;
    __GLfloat x,y,z,w;

    vertex = (__GLvertexProc)gc->procs.vertex;
    SAVE_T()
    (*vertex)(gc, vx);
}

/* 6907aae0 -- __glSaveCT */
void __fastcall __glSaveCT(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat r,g,b,a;
    __GLfloat x,y,z,w;
    __GLvertexProc vertex;

    vertex = (__GLvertexProc)gc->procs.vertex;
    SAVE_C()
    SAVE_T()
    (*vertex)(gc, vx);
}

/* 6907ab90 -- __glSaveNT */
void __fastcall __glSaveNT(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat x,y,z,w;
    __GLvertexProc vertex;

    vertex = (__GLvertexProc)gc->procs.vertex;
    SAVE_N()
    SAVE_T()
    (*vertex)(gc, vx);
}

/* 6907ac40 -- __glSaveF (s_vertex.c style): save the fog coordinate */
void __fastcall __glSaveF(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLvertexProc vertex;

    vertex = (__GLvertexProc)gc->procs.vertex;
    vx->fogCoord = (__GLfloat)(gc->state.current.fogCoord);
    (*vertex)(gc, vx);
}

/* 6907ac60 -- __glSaveNFog: __glSaveN with the fog coordinate (s_vertex.c) */
void __fastcall __glSaveNFog(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat x,y,z;
    void (*vertex)(__GLcontext *gc, __GLvertexICD *vx);

    vertex = (void (*)(__GLcontext *, __GLvertexICD *))gc->procs.vertex;
    x = gc->state.current.normal.x;
    y = gc->state.current.normal.y;
    z = gc->state.current.normal.z;
    vx->normal.x = x;
    vx->normal.y = y;
    vx->normal.z = z;
    vx->fogCoord = (__GLfloat)(gc->state.current.fogCoord);
    (*vertex)(gc, vx);
}

/* 6907acb0 -- __glSaveCIF: save the color index and the fog coordinate */
void __fastcall __glSaveCIF(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLvertexProc vertex;

    vertex = (__GLvertexProc)gc->procs.vertex;
    vx->colors[__GL_FRONTFACE].r = (__GLfloat)(gc->state.current.userColorIndex);
    vx->fogCoord = (__GLfloat)(gc->state.current.fogCoord);
    (*vertex)(gc, vx);
}

/* 6907acd0 -- __glSaveCFog: __glSaveC with the fog coordinate (s_vertex.c) */
void __fastcall __glSaveCFog(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat r,g,b,a;
    void (*vertex)(__GLcontext *gc, __GLvertexICD *vx);

    vertex = (void (*)(__GLcontext *, __GLvertexICD *))gc->procs.vertex;
    r = gc->state.current.color.r;
    g = gc->state.current.color.g;
    b = gc->state.current.color.b;
    a = gc->state.current.color.a;
    vx->colors[__GL_FRONTFACE].r = r;
    vx->colors[__GL_FRONTFACE].g = g;
    vx->colors[__GL_FRONTFACE].b = b;
    vx->colors[__GL_FRONTFACE].a = a;
    vx->fogCoord = (__GLfloat)(gc->state.current.fogCoord);
    (*vertex)(gc, vx);
}

/* 6907ad30 -- __glSaveNCIFog: __glSaveN + __glSaveCI with the fog coordinate (s_vertex.c) */
void __fastcall __glSaveNCIFog(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat x,y,z;
    void (*vertex)(__GLcontext *gc, __GLvertexICD *vx);

    vertex = (void (*)(__GLcontext *, __GLvertexICD *))gc->procs.vertex;
    x = gc->state.current.normal.x;
    y = gc->state.current.normal.y;
    z = gc->state.current.normal.z;
    vx->normal.x = x;
    vx->normal.y = y;
    vx->normal.z = z;
    vx->colors[__GL_FRONTFACE].r = (__GLfloat)(gc->state.current.userColorIndex);
    vx->fogCoord = (__GLfloat)(gc->state.current.fogCoord);
    (*vertex)(gc, vx);
}

/* 6907ad80 -- __glSaveNCF: save normal, color and fog coordinate */
void __fastcall __glSaveNCF(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat r,g,b,a;
    __GLfloat x,y,z;
    __GLvertexProc vertex;

    vertex = (__GLvertexProc)gc->procs.vertex;
    SAVE_N()
    SAVE_C()
    SAVE_F()
    (*vertex)(gc, vx);
}

/* 6907ae00 -- __glSaveTF: save textures and fog coordinate */
void __fastcall __glSaveTF(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLvertexProc vertex;
    __GLfloat x,y,z,w;

    vertex = (__GLvertexProc)gc->procs.vertex;
    SAVE_T()
    SAVE_F()
    (*vertex)(gc, vx);
}

/* 6907ae90 -- __glSaveNTF: save normal, textures and fog coordinate */
void __fastcall __glSaveNTF(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat x,y,z,w;
    __GLvertexProc vertex;

    vertex = (__GLvertexProc)gc->procs.vertex;
    SAVE_N()
    SAVE_T()
    SAVE_F()
    (*vertex)(gc, vx);
}

/* 6907af40 -- __glSaveCITF: save color index, textures and fog coordinate */
void __fastcall __glSaveCITF(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat index;
    __GLfloat x,y,z,w;
    __GLvertexProc vertex;

    vertex = (__GLvertexProc)gc->procs.vertex;
    index = gc->state.current.userColorIndex;
    vx->colors[__GL_FRONTFACE].r = index;
    SAVE_T()
    SAVE_F()
    (*vertex)(gc, vx);
}

/* 6907afd0 -- __glSaveCTF: save color, textures and fog coordinate */
void __fastcall __glSaveCTF(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat r,g,b,a;
    __GLfloat x,y,z,w;
    __GLvertexProc vertex;

    vertex = (__GLvertexProc)gc->procs.vertex;
    SAVE_C()
    SAVE_T()
    SAVE_F()
    (*vertex)(gc, vx);
}

/* 6907b090 -- __glSaveCIAll */
void __fastcall __glSaveCIAll(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat x,y,z,w;
    __GLvertexProc vertex;
    __GLfloat index;

    index = gc->state.current.userColorIndex;
    vertex = (__GLvertexProc)gc->procs.vertex;
    x = gc->state.current.normal.x;
    y = gc->state.current.normal.y;
    z = gc->state.current.normal.z;
    vx->colors[__GL_FRONTFACE].r = index;
    vx->normal.x = x;
    vx->normal.y = y;
    vx->normal.z = z;
    SAVE_T()
    (*vertex)(gc, vx);
}

/* 6907b150 -- __glSaveCAll */
void __fastcall __glSaveCAll(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat r,g,b,a;
    __GLfloat x,y,z,w;
    __GLvertexProc vertex;

    vertex = (__GLvertexProc)gc->procs.vertex;
    x = gc->state.current.normal.x;
    y = gc->state.current.normal.y;
    z = gc->state.current.normal.z;
    r = gc->state.current.color.r;
    g = gc->state.current.color.g;
    b = gc->state.current.color.b;
    a = gc->state.current.color.a;
    vx->colors[__GL_FRONTFACE].r = r;
    vx->colors[__GL_FRONTFACE].g = g;
    vx->colors[__GL_FRONTFACE].b = b;
    vx->colors[__GL_FRONTFACE].a = a;
    vx->normal.x = x;
    vx->normal.y = y;
    vx->normal.z = z;
    SAVE_T()
    (*vertex)(gc, vx);
}

/* 6907b240 -- __glSaveCIAllFog: __glSaveCIAll plus the fog coordinate */
void __fastcall __glSaveCIAllFog(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat x,y,z,w;
    __GLvertexProc vertex;
    __GLfloat index;

    index = gc->state.current.userColorIndex;
    vertex = (__GLvertexProc)gc->procs.vertex;
    x = gc->state.current.normal.x;
    y = gc->state.current.normal.y;
    z = gc->state.current.normal.z;
    vx->colors[__GL_FRONTFACE].r = index;
    vx->normal.x = x;
    vx->normal.y = y;
    vx->normal.z = z;
    SAVE_T()
    SAVE_F()
    (*vertex)(gc, vx);
}

/* 6907b310 -- __glSaveCAllFog: __glSaveCAll plus the fog coordinate */
void __fastcall __glSaveCAllFog(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat r,g,b,a;
    __GLfloat x,y,z,w;
    __GLvertexProc vertex;

    vertex = (__GLvertexProc)gc->procs.vertex;
    x = gc->state.current.normal.x;
    y = gc->state.current.normal.y;
    z = gc->state.current.normal.z;
    r = gc->state.current.color.r;
    g = gc->state.current.color.g;
    b = gc->state.current.color.b;
    a = gc->state.current.color.a;
    vx->colors[__GL_FRONTFACE].r = r;
    vx->colors[__GL_FRONTFACE].g = g;
    vx->colors[__GL_FRONTFACE].b = b;
    vx->colors[__GL_FRONTFACE].a = a;
    vx->normal.x = x;
    vx->normal.y = y;
    vx->normal.z = z;
    SAVE_T()
    SAVE_F()
    (*vertex)(gc, vx);
}

/* 6907b400 -- __glSaveCIT */
void __fastcall __glSaveCIT(__GLcontext *gc, __GLvertexICD *vx)
{
    __GLfloat index;
    __GLfloat x,y,z,w;
    __GLvertexProc vertex;

    vertex = (__GLvertexProc)gc->procs.vertex;
    index = gc->state.current.userColorIndex;
    vx->colors[__GL_FRONTFACE].r = index;
    SAVE_T()
    (*vertex)(gc, vx);
}

/* 6907b490 -- __glGenericPickVertexProcs */
void __fastcall __glGenericPickVertexProcs(__GLcontext *gc)
{
    GLuint enables = gc->state.enables.general;
    GLuint texEnables;
    GLint ix;
    GLenum mvpMatrixType;
    __GLmatrixICD *m;

    texEnables = gc->state.enables.texture[0] | gc->state.enables.texture[1];
    m = &gc->transform.modelView->mvp;
    mvpMatrixType = m->matrixType;

    if (gc->state.enables.clipPlanes) {
        gc->procs.clipCheck2 = (__GLproc)__glClipCheckAll2;
        gc->procs.clipCheck3 = (__GLproc)__glClipCheckAll3;
        gc->procs.clipCheck4 = (__GLproc)__glClipCheckAll4;
    } else {
        gc->procs.clipCheck3 = (__GLproc)__glClipCheckFast;
        gc->procs.clipCheck4 = (__GLproc)__glClipCheckFast;
        if (mvpMatrixType >= __GL_MT_IS2D &&
                m->matrix[3][2] >= -1.0f && m->matrix[3][2] <= 1.0f) {
            gc->procs.clipCheck2 = (__GLproc)__glClipCheckFrustum2D;
        } else {
            gc->procs.clipCheck2 = (__GLproc)__glClipCheckFast;
        }
    }

    if ((*gc->vertex.getFaceNeeds0)(gc) == 0) {
        gc->procs.validateVertex[0] = (__GLproc)NopSpanRender;
        gc->procs.validateVertex[1] = (__GLproc)NopSpanRender;
        gc->procs.validateVertex[2] = (__GLproc)NopSpanRender;
        gc->procs.validateVertex[3] = (__GLproc)NopSpanRender;
    } else {
        gc->procs.validateVertex[0] = (__GLproc)NopSpanRender;
        gc->procs.validateVertex[1] = (__GLproc)__glValidateVertex2;
        gc->procs.validateVertex[2] = (__GLproc)__glValidateVertex3;
        gc->procs.validateVertex[3] = (__GLproc)__glValidateVertex4;
    }

    gc->dispatchState->fn[__GL_DISP_Begin] = (__GLdispatchFn)__glim_Begin;
    gc->dispatchState->fn[__GL_DISP_End] = (__GLdispatchFn)__glim_End;
    gc->dispatchState->fn[__GL_DISP_Vertex2fv] = (__GLdispatchFn)__glim_Vertex2fv;
    gc->dispatchState->fn[__GL_DISP_Vertex3fv] = (__GLdispatchFn)__glim_Vertex3fv;
    gc->dispatchState->fn[__GL_DISP_Vertex4fv] = (__GLdispatchFn)__glim_Vertex4fv;

    if (gc->renderMode == GL_FEEDBACK) {
        if (gc->modes.colorIndexMode) {
            gc->procs.v = (__GLproc)__glSaveCIAllFog;
        } else {
            gc->procs.v = (__GLproc)__glSaveCAllFog;
        }
    } else {
        ix = (enables & __GL_LIGHTING_ENABLE) ? __NORMAL : __COLOR;
        if (gc->texture.textureEnabled[0] || gc->texture.textureEnabled[1]) {
            ix |= __TEX;
            if (texEnables & __GL_TEXTURE_GEN_S_ENABLE) {
                if (gc->state.texture.unit[0].s.mode == GL_SPHERE_MAP ||
                        gc->state.texture.unit[1].s.mode == GL_SPHERE_MAP) {
                    ix |= __NORMAL;
                }
            }
            if (texEnables & __GL_TEXTURE_GEN_T_ENABLE) {
                if (gc->state.texture.unit[0].t.mode == GL_SPHERE_MAP ||
                        gc->state.texture.unit[1].t.mode == GL_SPHERE_MAP) {
                    ix |= __NORMAL;
                }
            }
        }
        if ((enables & __GL_FOG_ENABLE) && gc->state.fog.coordSource == GL_FOG_COORDINATE_EXT) {
            ix |= __FOGCOORD;
        }
        if (gc->modes.rgbMode) {
            gc->procs.v = RGBSaveProcs[ix];
        } else {
            gc->procs.v = CISaveProcs[ix];
        }
    }
}

/* 6907ba50 -- __glValidateVertex2Clip (3dfx S_VERTEX.C): clip coordinates of a 2D vertex */
void __fastcall __glValidateVertex2Clip(__GLcontext *gc, __GLvertexICD *vx, GLuint needs)
{
    struct __GLtransformICDRec *tr = gc->transform.modelView;

    if (needs & __GL_HAS_CLIP) {
	(*(__GLxf)tr->mvp.xf2)(&vx->clip, &vx->obj.x, &tr->mvp);
	vx->flagBits |= __GL_HAS_CLIP;
    }
}

/* 6907ba90 -- __glValidateVertex3Clip (3dfx S_VERTEX.C): clip coordinates of a 3D vertex */
void __fastcall __glValidateVertex3Clip(__GLcontext *gc, __GLvertexICD *vx, GLuint needs)
{
    struct __GLtransformICDRec *tr = gc->transform.modelView;

    if (needs & __GL_HAS_CLIP) {
	(*(__GLxf)tr->mvp.xf3)(&vx->clip, &vx->obj.x, &tr->mvp);
	vx->flagBits |= __GL_HAS_CLIP;
    }
}

/* 6907bad0 -- __glValidateVertex4Clip (3dfx S_VERTEX.C): clip coordinates of a 4D vertex */
void __fastcall __glValidateVertex4Clip(__GLcontext *gc, __GLvertexICD *vx, GLuint needs)
{
    struct __GLtransformICDRec *tr = gc->transform.modelView;

    if (needs & __GL_HAS_CLIP) {
	(*(__GLxf)tr->mvp.xf4)(&vx->clip, &vx->obj.x, &tr->mvp);
	vx->flagBits |= __GL_HAS_CLIP;
    }
}

/* 690a5410 -- __glim_Vertex2fv: glVertex2fv */
void __stdcall __glim_Vertex2fv(const GLfloat *v)
{
    Vertex2W19(__glCurrentGC(), v[0], v[1]);
}

/* 690a5530 -- __glim_Vertex4fv: glVertex4fv (3dfx GLIDE3X/GLCORE/S_VAPI.C __glim_Vertex4fv, as written there) */
void __stdcall __glim_Vertex4fv(const GLfloat *v)
{
    __GLcontext *gc = __glCurrentGC();
    __GLvertexICD *vx;
    __GLtransformICD *tr;
    __GLfloat x, y, z, w;

    vx = gc->vertex.v0;
    tr = gc->transform.modelView;
    x = v[0];
    y = v[1];
    z = v[2];
    w = v[3];
    vx->obj.x = x;
    vx->obj.y = y;
    vx->obj.z = z;
    vx->obj.w = w;
    (*(__GLxformProc)tr->mvp.xf4)(&vx->clip, &vx->obj.x, &tr->mvp);
    vx->flagBits = 0xc000;
    vx->flagBits |= (*(__GLclipCheckProc)gc->procs.clipCheck4)(gc, vx);
    (*(__GLvertexProc)gc->procs.v)(gc, vx);
}

/* 690a5600 -- __glim_Vertex2d */
void __stdcall __glim_Vertex2d(GLdouble x, GLdouble y)
{
    __GLfvProc fn = DISP_FV(0x208);
    GLfloat v[2];
    v[0] = (GLfloat)x;
    v[1] = (GLfloat)y;
    (*fn)(v);
}

/* 690a5640 -- __glim_Vertex2dv */
void __stdcall __glim_Vertex2dv(const GLdouble *p)
{
    __GLfvProc fn = DISP_FV(0x208);
    GLfloat v[2];
    v[0] = (GLfloat)p[0];
    v[1] = (GLfloat)p[1];
    (*fn)(v);
}

/* 690a5680 -- __glim_Vertex2i */
void __stdcall __glim_Vertex2i(GLint x, GLint y)
{
    __GLfvProc fn = DISP_FV(0x208);
    GLfloat v[2];
    v[0] = (GLfloat)x;
    v[1] = (GLfloat)y;
    (*fn)(v);
}

/* 690a56c0 -- __glim_Vertex2iv */
void __stdcall __glim_Vertex2iv(const GLint *p)
{
    __GLfvProc fn = DISP_FV(0x208);
    GLfloat v[2];
    v[0] = (GLfloat)p[0];
    v[1] = (GLfloat)p[1];
    (*fn)(v);
}

/* 690a5750 -- __glim_Vertex2sv */
void __stdcall __glim_Vertex2sv(const GLshort v[2])
{
    GLfloat fv[2];
    __GLvertex2fv vertex2fv;
    __GL_SETUP();

    vertex2fv = DISPATCH(gc, __GL_DISP_Vertex2fv, __GLvertex2fv);
    fv[0] = v[0];
    fv[1] = v[1];
    (*vertex2fv)(fv);
}

/* 690a57d0 -- __glim_Vertex3d */
void __stdcall __glim_Vertex3d(GLdouble x, GLdouble y, GLdouble z)
{
    __GLfvProc fn = DISP_FV(0x228);
    GLfloat v[3];
    v[0] = (GLfloat)x;
    v[1] = (GLfloat)y;
    v[2] = (GLfloat)z;
    (*fn)(v);
}

/* 690a5810 -- __glim_Vertex3dv */
void __stdcall __glim_Vertex3dv(const GLdouble *p)
{
    __GLfvProc fn = DISP_FV(0x228);
    GLfloat v[3];
    v[0] = (GLfloat)p[0];
    v[1] = (GLfloat)p[1];
    v[2] = (GLfloat)p[2];
    (*fn)(v);
}

/* 690a5850 -- __glim_Vertex3i */
void __stdcall __glim_Vertex3i(GLint x, GLint y, GLint z)
{
    __GLfvProc fn = DISP_FV(0x228);
    GLfloat v[3];
    v[0] = (GLfloat)x;
    v[1] = (GLfloat)y;
    v[2] = (GLfloat)z;
    (*fn)(v);
}

/* 690a5890 -- __glim_Vertex3iv */
void __stdcall __glim_Vertex3iv(const GLint *p)
{
    __GLfvProc fn = DISP_FV(0x228);
    GLfloat v[3];
    v[0] = (GLfloat)p[0];
    v[1] = (GLfloat)p[1];
    v[2] = (GLfloat)p[2];
    (*fn)(v);
}

/* 690a5930 -- __glim_Vertex3sv (dispatch +0x228 is Vertex3fv): 3dfx GLCORE/S_VAPI.C shape, float
** temporaries first -- that is what hoists the last element's load above the first fstp */
void __stdcall __glim_Vertex3sv(const GLshort v[3])
{
    GLfloat x,y,z,fv[3];
    __GLfvProc vertexfv;
    __GL_SETUP();

    x = v[0];
    y = v[1];
    z = v[2];
    vertexfv = (__GLfvProc)gc->dispatchState->fn[0x228 / 4];
    fv[0] = x;
    fv[1] = y;
    fv[2] = z;
    (*vertexfv)(fv);
}

/* 690a59c0 -- __glim_Vertex4d */
void __stdcall __glim_Vertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    __GLfvProc fn = DISP_FV(0x248);
    GLfloat v[4];
    v[0] = (GLfloat)x;
    v[1] = (GLfloat)y;
    v[2] = (GLfloat)z;
    v[3] = (GLfloat)w;
    (*fn)(v);
}

/* 690a5a10 -- __glim_Vertex4dv */
void __stdcall __glim_Vertex4dv(const GLdouble *p)
{
    __GLfvProc fn = DISP_FV(0x248);
    GLfloat v[4];
    v[0] = (GLfloat)p[0];
    v[1] = (GLfloat)p[1];
    v[2] = (GLfloat)p[2];
    v[3] = (GLfloat)p[3];
    (*fn)(v);
}

/* 690a5a60 -- __glim_Vertex4i */
void __stdcall __glim_Vertex4i(GLint x, GLint y, GLint z, GLint w)
{
    __GLfvProc fn = DISP_FV(0x248);
    GLfloat v[4];
    v[0] = (GLfloat)x;
    v[1] = (GLfloat)y;
    v[2] = (GLfloat)z;
    v[3] = (GLfloat)w;
    (*fn)(v);
}

/* 690a5ab0 -- __glim_Vertex4iv */
void __stdcall __glim_Vertex4iv(const GLint *p)
{
    __GLfvProc fn = DISP_FV(0x248);
    GLfloat v[4];
    v[0] = (GLfloat)p[0];
    v[1] = (GLfloat)p[1];
    v[2] = (GLfloat)p[2];
    v[3] = (GLfloat)p[3];
    (*fn)(v);
}

/* 690a5b00 -- __glim_Vertex4s */
void __stdcall __glim_Vertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    GLfloat fv[4];
    __GLvertexfv vertex4fv;
    __GL_SETUP();

    vertex4fv = DISPATCH(gc, __GL_DISP_Vertex4fv, __GLvertexfv);
    fv[0] = x;
    fv[1] = y;
    fv[2] = z;
    fv[3] = w;
    (*vertex4fv)(fv);
}

/* 690a5b60 -- __glim_Vertex4sv (dispatch +0x248 is Vertex4fv): S_VAPI.C shape as above */
void __stdcall __glim_Vertex4sv(const GLshort v[4])
{
    GLfloat x,y,z,w,fv[4];
    __GLfvProc vertexfv;
    __GL_SETUP();

    x = v[0];
    y = v[1];
    z = v[2];
    w = v[3];
    vertexfv = (__GLfvProc)gc->dispatchState->fn[0x248 / 4];
    fv[0] = x;
    fv[1] = y;
    fv[2] = z;
    fv[3] = w;
    (*vertexfv)(fv);
}

/* 690a5700 -- __glim_Vertex2s */
void __stdcall __glim_Vertex2s(GLshort x, GLshort y)
{
    GLfloat fv[2];
    __GLvertex2fv vertex2fv;
    __GL_SETUP();

    vertex2fv = DISPATCH(gc, __GL_DISP_Vertex2fv, __GLvertex2fv);
    fv[0] = x;
    fv[1] = y;
    (*vertex2fv)(fv);
}

/* 690a58d0 -- __glim_Vertex3s */
void __stdcall __glim_Vertex3s(GLshort x, GLshort y, GLshort z)
{
    GLfloat fv[3];
    __GLvertexfv vertex3fv;
    __GL_SETUP();

    vertex3fv = DISPATCH(gc, __GL_DISP_Vertex3fv, __GLvertexfv);
    fv[0] = x;
    fv[1] = y;
    fv[2] = z;
    (*vertex3fv)(fv);
}

/* 690a54a0 -- __glim_Vertex3fv fast path: obj -> clip through the model-view's mvp xf3, clip
** check, then procs.v.  3dfx GLIDE3X/GLCORE/S_VAPI.C __glim_Vertex3fv: x, y, z read into locals
** first (VC6 then copies x, y through the FPU and spills z, as the original) */
void __stdcall __glim_Vertex3fv(const GLfloat v[3])
{
    __GL_SETUP();
    __GLvertexICD *vx;
    __GLfloat x, y, z;
    __GLtransformICD *tr;

    vx = gc->vertex.v0;
    tr = gc->transform.modelView;
    x = v[0];
    y = v[1];
    z = v[2];
    vx->obj.x = x;
    vx->obj.y = y;
    vx->obj.z = z;
    vx->obj.w = __GL_ONE_F;
    (*(__GLxformProc)tr->mvp.xf3)(&vx->clip, &vx->obj.x, &tr->mvp);
    vx->flagBits = 0x8000;
    vx->flagBits |= (*(__GLclipCheckProc)gc->procs.clipCheck3)(gc, vx);
    (*(__GLvertexProc)gc->procs.v)(gc, vx);
}

/* 690a55d0: __glim_Vertex2f -> the current Vertex2fv */
void __stdcall __glim_Vertex2f(GLfloat x, GLfloat y)
{
    ((void (__stdcall *)(const GLfloat *))__glCurrentGC()->dispatchState->fn[__GL_DISP_Vertex2fv])(&x);
}

/* 690a57a0: __glim_Vertex3f -> the current Vertex3fv */
void __stdcall __glim_Vertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    ((void (__stdcall *)(const GLfloat *))__glCurrentGC()->dispatchState->fn[__GL_DISP_Vertex3fv])(&x);
}

/* 690a5990: __glim_Vertex4f -> the current Vertex4fv */
void __stdcall __glim_Vertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    ((void (__stdcall *)(const GLfloat *))__glCurrentGC()->dispatchState->fn[__GL_DISP_Vertex4fv])(&x);
}

extern void __fastcall __glSaveN(__GLcontext *gc, __GLvertexICD *vx);     /* si_s_vertex.c */
extern void __fastcall __glSaveCI(__GLcontext *gc, __GLvertexICD *vx);
extern void __fastcall __glSaveC(__GLcontext *gc, __GLvertexICD *vx);
/* 690f3ed0, 690f3f10 -- s_vertex.c's save procs by the needs bits (16 entries here: the fog-coordinate bit) */
static __GLproc CISaveProcs[16] = {
    NULL,
    (__GLproc)__glSaveN,
    (__GLproc)__glSaveCI,
    NULL,
    (__GLproc)__glSaveT,
    (__GLproc)__glSaveNT,
    (__GLproc)__glSaveCIT,
    (__GLproc)__glSaveCIAll,
    (__GLproc)__glSaveF,
    (__GLproc)__glSaveNFog,
    (__GLproc)__glSaveCIF,
    (__GLproc)__glSaveNCIFog,
    (__GLproc)__glSaveTF,
    (__GLproc)__glSaveNTF,
    (__GLproc)__glSaveCITF,
    (__GLproc)__glSaveCIAllFog,
};
static __GLproc RGBSaveProcs[16] = {
    NULL,
    (__GLproc)__glSaveN,
    (__GLproc)__glSaveC,
    NULL,
    (__GLproc)__glSaveT,
    (__GLproc)__glSaveNT,
    (__GLproc)__glSaveCT,
    (__GLproc)__glSaveCAll,
    (__GLproc)__glSaveF,
    (__GLproc)__glSaveNFog,
    (__GLproc)__glSaveCFog,
    (__GLproc)__glSaveNCF,
    (__GLproc)__glSaveTF,
    (__GLproc)__glSaveNTF,
    (__GLproc)__glSaveCTF,
    (__GLproc)__glSaveCAllFog,
};

/*
** si_s_varray.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_varray.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690ae710 -- CopyVertex2s */
#pragma optimize("w", on)
void CopyVertex2s(__GLcontext *gc, const GLshort *vp, __GLvertexICD *v)
{
    v->obj.x = vp[0]; v->obj.y = vp[1]; v->obj.z = 0.0F; v->obj.w = 1.0F;
}

/* 690ae750 -- CopyVertex2i */
void __fastcall CopyVertex2i(__GLcontext *gc, const GLint *vp, __GLvertexICD *v)
{
    v->obj.x = vp[0]; v->obj.y = vp[1]; v->obj.z = 0.0F; v->obj.w = 1.0F;
}

/* 690ae780 -- CopyVertex2f */
void __fastcall CopyVertex2f(__GLcontext *gc, const GLfloat *vp, __GLvertexICD *v)
{
    v->obj.x = vp[0]; v->obj.y = vp[1]; v->obj.z = 0.0F; v->obj.w = 1.0F;
}

/* 690ae7b0 -- CopyVertex2d */
void __fastcall CopyVertex2d(__GLcontext *gc, const GLdouble *vp, __GLvertexICD *v)
{
    v->obj.x = vp[0]; v->obj.y = vp[1]; v->obj.z = 0.0F; v->obj.w = 1.0F;
}

/* 690ae910 -- CopyVertex4i */
void __fastcall CopyVertex4i(__GLcontext *gc, const GLint *vp, __GLvertexICD *v)
{
    v->obj.x = vp[0]; v->obj.y = vp[1]; v->obj.z = vp[2]; v->obj.w = vp[3];
}

/* 690ae970 -- CopyVertex4d */
void __fastcall CopyVertex4d(__GLcontext *gc, const GLdouble *vp, __GLvertexICD *v)
{
    v->obj.x = vp[0]; v->obj.y = vp[1]; v->obj.z = vp[2]; v->obj.w = vp[3];
}

/* 690ae9a0 -- CopyNormal3b */
void __fastcall CopyNormal3b(__GLcontext *gc, const GLbyte *np, __GLvertexICD *v)
{
    v->normal.x = np[0]; v->normal.y = np[1]; v->normal.z = np[2];
}

/* 690ae9e0 -- CopyNormal3s */
void __fastcall CopyNormal3s(__GLcontext *gc, const GLshort *np, __GLvertexICD *v)
{
    v->normal.x = np[0]; v->normal.y = np[1]; v->normal.z = np[2];
}

/* 690aea20 -- CopyNormal3i */
void __fastcall CopyNormal3i(__GLcontext *gc, const GLint *np, __GLvertexICD *v)
{
    v->normal.x = np[0]; v->normal.y = np[1]; v->normal.z = np[2];
}

/* 690aea80 -- CopyNormal3d */
void __fastcall CopyNormal3d(__GLcontext *gc, const GLdouble *np, __GLvertexICD *v)
{
    v->normal.x = np[0]; v->normal.y = np[1]; v->normal.z = np[2];
}

/* 690aeab0 -- CopyColor3b */
void __fastcall CopyColor3b(__GLcontext *gc, const GLbyte *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = __GL_B_TO_FLOAT(cp[0]);
    v->colors[__GL_FRONTFACE].g = __GL_B_TO_FLOAT(cp[1]);
    v->colors[__GL_FRONTFACE].b = __GL_B_TO_FLOAT(cp[2]);
    v->colors[__GL_FRONTFACE].a = 1.0F;
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}

/* 690aeb20 -- CopyColor3ub */
void __fastcall CopyColor3ub(__GLcontext *gc, const GLubyte *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = __GL_UB_TO_FLOAT(cp[0]);
    v->colors[__GL_FRONTFACE].g = __GL_UB_TO_FLOAT(cp[1]);
    v->colors[__GL_FRONTFACE].b = __GL_UB_TO_FLOAT(cp[2]);
    v->colors[__GL_FRONTFACE].a = 1.0F;
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}

/* 690aeb70 -- CopyColor3s */
void __fastcall CopyColor3s(__GLcontext *gc, const GLshort *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = __GL_S_TO_FLOAT(cp[0]);
    v->colors[__GL_FRONTFACE].g = __GL_S_TO_FLOAT(cp[1]);
    v->colors[__GL_FRONTFACE].b = __GL_S_TO_FLOAT(cp[2]);
    v->colors[__GL_FRONTFACE].a = 1.0F;
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}

/* 690aebe0 -- CopyColor3us */
void __fastcall CopyColor3us(__GLcontext *gc, const GLushort *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = __GL_US_TO_FLOAT(cp[0]);
    v->colors[__GL_FRONTFACE].g = __GL_US_TO_FLOAT(cp[1]);
    v->colors[__GL_FRONTFACE].b = __GL_US_TO_FLOAT(cp[2]);
    v->colors[__GL_FRONTFACE].a = 1.0F;
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}

/* 690aec40 -- CopyColor3i */
void __fastcall CopyColor3i(__GLcontext *gc, const GLint *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = __GL_I_TO_FLOAT(cp[0]);
    v->colors[__GL_FRONTFACE].g = __GL_I_TO_FLOAT(cp[1]);
    v->colors[__GL_FRONTFACE].b = __GL_I_TO_FLOAT(cp[2]);
    v->colors[__GL_FRONTFACE].a = 1.0F;
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}

/* 690aed40 -- CopyColor3d */
void __fastcall CopyColor3d(__GLcontext *gc, const GLdouble *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = cp[0];
    v->colors[__GL_FRONTFACE].g = cp[1];
    v->colors[__GL_FRONTFACE].b = cp[2];
    v->colors[__GL_FRONTFACE].a = 1.0F;
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}

/* 690aedf0 -- CopyColor4ub */
void __fastcall CopyColor4ub(__GLcontext *gc, const GLubyte *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = __GL_UB_TO_FLOAT(cp[0]);
    v->colors[__GL_FRONTFACE].g = __GL_UB_TO_FLOAT(cp[1]);
    v->colors[__GL_FRONTFACE].b = __GL_UB_TO_FLOAT(cp[2]);
    v->colors[__GL_FRONTFACE].a = __GL_UB_TO_FLOAT(cp[3]);
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}

/* 690aeed0 -- CopyColor4us */
void __fastcall CopyColor4us(__GLcontext *gc, const GLushort *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = __GL_US_TO_FLOAT(cp[0]);
    v->colors[__GL_FRONTFACE].g = __GL_US_TO_FLOAT(cp[1]);
    v->colors[__GL_FRONTFACE].b = __GL_US_TO_FLOAT(cp[2]);
    v->colors[__GL_FRONTFACE].a = __GL_US_TO_FLOAT(cp[3]);
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}

/* 690aef40 -- CopyColor4i */
void __fastcall CopyColor4i(__GLcontext *gc, const GLint *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = __GL_I_TO_FLOAT(cp[0]);
    v->colors[__GL_FRONTFACE].g = __GL_I_TO_FLOAT(cp[1]);
    v->colors[__GL_FRONTFACE].b = __GL_I_TO_FLOAT(cp[2]);
    v->colors[__GL_FRONTFACE].a = __GL_I_TO_FLOAT(cp[3]);
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}

/* 690af060 -- CopyColor4d */
void __fastcall CopyColor4d(__GLcontext *gc, const GLdouble *cp, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = cp[0];
    v->colors[__GL_FRONTFACE].g = cp[1];
    v->colors[__GL_FRONTFACE].b = cp[2];
    v->colors[__GL_FRONTFACE].a = cp[3];
    __glClampAndScaleColorf(gc, &v->colors[__GL_FRONTFACE], &v->colors[__GL_FRONTFACE].r);
}

/* 690af090 -- CopyIndexub */
void __fastcall CopyIndexub(__GLcontext *gc, const GLubyte *ip, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = ip[0];
}

/* 690af0b0 -- CopyIndexs */
void __fastcall CopyIndexs(__GLcontext *gc, const GLshort *ip, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = ip[0];
}

/* 690af0d0 -- CopyIndexi */
void __fastcall CopyIndexi(__GLcontext *gc, const GLint *ip, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = ip[0];
}

/* 690af0e0 -- CopyIndexf */
void __fastcall CopyIndexf(__GLcontext *gc, const GLfloat *ip, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = ip[0];
}

/* 690af0f0 -- CopyIndexd */
void __fastcall CopyIndexd(__GLcontext *gc, const GLdouble *ip, __GLvertexICD *v)
{
    v->colors[__GL_FRONTFACE].r = ip[0];
}
#pragma optimize("", on)

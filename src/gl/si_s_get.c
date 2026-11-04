/*
** si_s_get.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/s_get.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"

/* 690bbc90 -- __glim_GetClipPlane */
void __stdcall __glim_GetClipPlane(GLenum plane, GLdouble eqn[4])
{
    GLint index;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    index = plane - GL_CLIP_PLANE0;
    if ((index < 0) || (index >= gc->constants.numberOfClipPlanes)) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    eqn[0] = gc->state.transform.eyeClipPlanes[index].x;
    eqn[1] = gc->state.transform.eyeClipPlanes[index].y;
    eqn[2] = gc->state.transform.eyeClipPlanes[index].z;
    eqn[3] = gc->state.transform.eyeClipPlanes[index].w;
}

/* 690bb1e0 -- __glim_GetTexGenfv */
void __stdcall __glim_GetTexGenfv(GLenum coord, GLenum pname,
                              GLfloat v[])
{
    __GLtextureGenState* tcs;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    switch (coord) {
      case GL_S:
        tcs = &(&gc->state.texture.unit[gc->fx.activeTexture])->s;
        break;
      case GL_T:
        tcs = &(&gc->state.texture.unit[gc->fx.activeTexture])->t;
        break;
      case GL_R:
        tcs = &(&gc->state.texture.unit[gc->fx.activeTexture])->r;
        break;
      case GL_Q:
        tcs = &(&gc->state.texture.unit[gc->fx.activeTexture])->q;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    switch (pname) {
      case GL_TEXTURE_GEN_MODE:
        v[0] = tcs->mode;
        break;
      case GL_OBJECT_PLANE:
        v[0] = tcs->objectPlaneEquation.x;
        v[1] = tcs->objectPlaneEquation.y;
        v[2] = tcs->objectPlaneEquation.z;
        v[3] = tcs->objectPlaneEquation.w;
        break;
      case GL_EYE_PLANE:
        v[0] = tcs->eyePlaneEquation.x;
        v[1] = tcs->eyePlaneEquation.y;
        v[2] = tcs->eyePlaneEquation.z;
        v[3] = tcs->eyePlaneEquation.w;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690bb330 -- __glim_GetTexGendv */
void __stdcall __glim_GetTexGendv(GLenum coord, GLenum pname,
                              GLdouble v[])
{
    __GLtextureGenState* tcs;
    __GL_SETUP_NOT_IN_BEGIN();
    __GL_API_GET();

    switch (coord) {
      case GL_S:
        tcs = &(&gc->state.texture.unit[gc->fx.activeTexture])->s;
        break;
      case GL_T:
        tcs = &(&gc->state.texture.unit[gc->fx.activeTexture])->t;
        break;
      case GL_R:
        tcs = &(&gc->state.texture.unit[gc->fx.activeTexture])->r;
        break;
      case GL_Q:
        tcs = &(&gc->state.texture.unit[gc->fx.activeTexture])->q;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    switch (pname) {
      case GL_TEXTURE_GEN_MODE:
        v[0] = tcs->mode;
        break;
      case GL_OBJECT_PLANE:
        v[0] = tcs->objectPlaneEquation.x;
        v[1] = tcs->objectPlaneEquation.y;
        v[2] = tcs->objectPlaneEquation.z;
        v[3] = tcs->objectPlaneEquation.w;
        break;
      case GL_EYE_PLANE:
        v[0] = tcs->eyePlaneEquation.x;
        v[1] = tcs->eyePlaneEquation.y;
        v[2] = tcs->eyePlaneEquation.z;
        v[3] = tcs->eyePlaneEquation.w;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
}

/* 690bf1c0 -- __glim_GetString */
const GLubyte * __stdcall __glim_GetString(GLenum name)
{
    __GL_SETUP_NOT_IN_BEGIN2();
    __GL_API_GET();

    switch (name) {
      case GL_VENDOR:
        return (GLubyte*) gc->constants.vendor;
      case GL_RENDERER:
        return (GLubyte*) gc->constants.renderer;
      case GL_VERSION:
        return (GLubyte*) gc->constants.version;
      case GL_EXTENSIONS:
        return (GLubyte*) gc->constants.extensions;
      default:
        __glSetError(GL_INVALID_ENUM);
        return 0;
    }
}

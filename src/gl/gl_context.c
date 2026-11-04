/*
** gl_context.c -- s_contxt.c: __glCoreCreateContext and the rest of the context set-up and teardown, in address
** order.
*/
#include <string.h>
#include "wglpriv.h"
#include "glsi.h"
#include "siprotos.h"
#include "si_names.h"
#include "icdprotos.h"
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include "glsitypes.h"
#include "glshade.h"
#include "icdglob.h"
/* the ICD's init calls, in the SI's order: __glInitColorTables first (69083610, empty as in the 3dfx S_CTABLE.C), no
** vertex cache state */
/* the get strings, stored in .text by the original (6907e790..6907e7b4); 6907e7b4 is the generic
** (non-Glide) extension string */
static const char g_genericExtensions[] = "GL_EXT_abgr GL_EXT_bgra GL_EXT_packed_pixels GL_EXT_paletted_texture GL_EXT_vertex_array GL_SGI_compiled_vertex_array GL_SGI_cull_vertex GL_SGI_index_array_formats GL_SGI_index_func GL_SGI_index_material GL_SGI_index_texture GL_WIN_swap_hint ";
typedef void (__fastcall *__GLfree)(__GLcontext *gc, void *mem);
#define FREE(gc, p)     (*(__GLfree)(gc)->imports[3])(gc, (void *)(p))

/* 6907e440 -- __glEarlyInitContext: constants, the ICD procs, the allocated state arrays, the
** arena procs and the OG code generator's caches, blocks and constant pools */
void __fastcall __glEarlyInitContext(__GLcontext *gc)
{
    GLint numLights, i;
    GLubyte *buf;

    gc->constants.fviewportXAdjust = (__GLfloat) gc->constants.viewportXAdjust;
    gc->constants.fviewportYAdjust = (__GLfloat) gc->constants.viewportYAdjust;
    *(GLuint *)&gc->constants.one = 0x3f800000;
    *(GLuint *)&gc->constants.half = 0x3f000000;
    gc->exports[__GL_EXP_COPY_CONTEXT] = (GLuint)__glCopyContext;
    gc->exports[__GL_EXP_DISPATCH_EXEC] = (GLuint)__glDispatchExec;
    gc->exports[__GL_EXP_BEGIN_DISPATCH_OVERRIDE] = (GLuint)__glBeginDispatchOverride;
    gc->exports[__GL_EXP_END_DISPATCH_OVERRIDE] = (GLuint)__glEndDispatchOverride;
    gc->procs.pickColorMaterialProcs = (__GLproc)__glNop1;
    gc->procs.applyColor = (__GLproc)__glNop1;
    gc->polygon.shader.getModeFlags = (__GLproc)hwGetModeFlags;
    gc->vertex.getNeeds = (__GLproc)hwGetNeeds;
    gc->vertex.getFaceNeeds0 = (__GLproc)hwGetFaceNeeds0;
    gc->vertex.getFaceNeeds1 = (__GLproc)hwGetFaceNeeds1;
    gc->vertex.getMaterialNeeds = (__GLproc)hwGetMaterialNeeds;

    numLights = gc->constants.numberOfLights;
    gc->state.light.source = (struct __GLlightSourceStateRec *)__GL_CALLOC(gc, numLights, 0x74);
    gc->light.lutCache = NULL;
    gc->light.source = (struct __GLlightSourceICDRec *)__GL_CALLOC(gc, numLights, 0x100);
    gc->attributes.stack = __GL_CALLOC(gc, gc->constants.maxAttribStackDepth, 4);
    gc->attributes.clientStack = __GL_CALLOC(gc, gc->constants.maxClientAttribStackDepth, 4);
    gc->select.stack = (GLuint *)__GL_CALLOC(gc, gc->constants.maxNameStackDepth, 4);
    gc->polygon.shader.colors = (__GLcolor *)__GL_CALLOC(gc, 0x800, sizeof(__GLcolor));
    gc->polygon.shader.fbcolors = (__GLcolor *)__GL_CALLOC(gc, 0x800, sizeof(__GLcolor));
    gc->polygon.shader.stipplePat = (GLuint *)__GL_CALLOC(gc, 0x40, 4);

    __glEarlyInitTextureState(gc);
    __glEarlyInitTransformState(gc);

    gc->procs.newArena = (__GLproc)__glNewArena;
    gc->procs.deleteArena = (__GLproc)__glDeleteArena;
    gc->procs.arenaAlloc = (__GLproc)__glArenaAlloc;
    gc->procs.freeAll = (__GLproc)__glArenaFreeAll;

    gc->constants.ogCacheCount0 = 4;
    gc->constants.ogCacheBytes0 = 0x400;
    gc->constants.ogCacheCount1 = 4;
    gc->constants.ogCacheBytes1 = 0x100;
    gc->constants.ogCacheCount2 = 0x28;
    gc->constants.ogCacheBytes2 = 0x1000;
    gc->constants.ogScratchBytes = 0x800;
    gc->constants.ogCacheCount3 = 0x28;
    gc->constants.ogCacheBytes3 = 0x4000;
    gc->constants.ogCacheCount4 = 0x14;
    gc->constants.ogCacheBytes4 = 0x400;
    __glInitializeCodeCache(gc);

    gc->ogBlockPtr = &gc->ogBlocks[0];
    memset(gc->ogBlocks, 0, sizeof(gc->ogBlocks));
    buf = __GL_CALLOC(gc, 0x28, 0xc00);
    gc->ogBlocks[0].buffer = buf;
    for (i = 1; i < 0x28; i++) {
	gc->ogBlocks[i].buffer = buf + i * 0xc00;
    }

    gc->ogFloatPool.capacity = 1000;
    gc->ogFloatPool.count = 0;
    gc->ogFloatPool.data = __GL_CALLOC(gc, 1000, 4);
    gc->ogPool2.capacity = 1000;
    gc->ogPool2.count = 0;
    gc->ogPool2.data = __GL_CALLOC(gc, 1000, 8);
    __glInitDlistState(gc);
}

/* ---- s_contxt.c: rescale the colors kept in the state (after a change of the color scale) ---- */

/* 6907e6e0 -- __glContextSetColorScales (s_contxt.c): rescale the raster position / current colors, the light
** model ambient, every light's
** colors and both emissive colors with __glScaleColorf */
void __fastcall __glContextSetColorScales(__GLcontext *gc)
{
    GLint i;

    if (gc->modes.rgbMode) {
	__glScaleColorf(gc, &gc->state.current.rasterPos.colors[0], &gc->state.current.rasterPos.colors[0].r);
	__glScaleColorf(gc, &gc->state.current.color, &gc->state.current.color.r);
    }
    __glScaleColorf(gc, &gc->state.light.model.ambient, &gc->state.light.model.ambient.r);
    for (i = 0; i < gc->constants.numberOfLights; i++) {
	__glScaleColorf(gc, &((__GLlightSourceStateICD *)gc->state.light.source)[i].ambient,
			&((__GLlightSourceStateICD *)gc->state.light.source)[i].ambient.r);
	__glScaleColorf(gc, &((__GLlightSourceStateICD *)gc->state.light.source)[i].diffuse,
			&((__GLlightSourceStateICD *)gc->state.light.source)[i].diffuse.r);
	__glScaleColorf(gc, &((__GLlightSourceStateICD *)gc->state.light.source)[i].specular,
			&((__GLlightSourceStateICD *)gc->state.light.source)[i].specular.r);
    }
    __glScaleColorf(gc, &gc->state.light.front.emissive, &gc->state.light.front.emissive.r);
    __glScaleColorf(gc, &gc->state.light.back.emissive, &gc->state.light.back.emissive.r);
}

/* 690f3fd0 .. 690f400c -- s_contxt.c's default colours */
static GLfloat DefaultAmbient[4] = { 0.2F, 0.2F, 0.2F, 1.0F };
static GLfloat DefaultDiffuse[4] = { 0.8F, 0.8F, 0.8F, 1.0F };
static GLfloat DefaultBlack[4] = { 0.0F, 0.0F, 0.0F, 1.0F };
static GLfloat DefaultWhite[4] = { 1.0F, 1.0F, 1.0F, 1.0F };

/* 6907e8a7 -- s_contxt.c __glSoftResetContext */
void __fastcall __glSoftResetContext(__GLcontext *gc)
{
    __GLlightSourceStateICD *lss;
    __GLlightSourceMachineICD *lsm;
    __GLvertexICD *vx;
    GLint i, numLights;
    __GLfloat zero = __GL_ZERO_F;
    __GLfloat one = __GL_ONE_F;

    __glDetectCPU(gc);

    /* Setup default values for get strings */
    gc->constants.vendor = (const GLubyte *)"SGI";
    if (gc->constants.cpuCaps & __GL_CPU_MMX) {
	gc->constants.renderer = (const GLubyte *)"Generic/MMX";
    } else {
	gc->constants.renderer = (const GLubyte *)"Generic";
    }
    gc->constants.version = (const GLubyte *)"1.1.3 SGI";
    gc->constants.extensions = (const GLubyte *)g_genericExtensions;

    gc->constants.val255 = (__GLfloat) 255.0;
    gc->constants.val65535 = (__GLfloat) 65535.0;
    gc->constants.val4294965000 = (__GLfloat) 4294965000.0;
    gc->constants.oneOver255 = one / gc->constants.val255;
    gc->constants.oneOver65535 = one / gc->constants.val65535;
    gc->constants.oneOver4294965000 = one / gc->constants.val4294965000;

    if (gc->constants.alphaTestSize == 0) {
	gc->constants.alphaTestSize = 256;	/* A default */
    }
    gc->constants.alphaTableConv = (gc->constants.alphaTestSize - 1) /
	    gc->frontBuffer.alphaScale;

    gc->frontBuffer.oneOverRedScale = one / gc->frontBuffer.redScale;
    gc->frontBuffer.oneOverGreenScale = one / gc->frontBuffer.greenScale;
    gc->frontBuffer.oneOverBlueScale = one / gc->frontBuffer.blueScale;
    gc->frontBuffer.oneOverAlphaScale = one / gc->frontBuffer.alphaScale;

    gc->constants.oneOverRedScale = one / gc->constants.redScale;
    gc->constants.oneOverGreenScale = one / gc->constants.greenScale;
    gc->constants.oneOverBlueScale = one / gc->constants.blueScale;
    gc->constants.oneOverAlphaScale = one / gc->constants.alphaScale;

    gc->constants.oneOverDepthScale = one / gc->constants.depthScale;

    gc->constants.redRescale =
	gc->frontBuffer.redScale * gc->constants.oneOverRedScale;
    gc->constants.greenRescale =
	gc->frontBuffer.greenScale * gc->constants.oneOverGreenScale;
    gc->constants.blueRescale =
	gc->frontBuffer.blueScale * gc->constants.oneOverBlueScale;
    gc->constants.alphaRescale =
	gc->frontBuffer.alphaScale * gc->constants.oneOverAlphaScale;

    gc->constants.depthRescale =
	gc->depthBuffer.scale * gc->constants.oneOverDepthScale;

    /* Lookup table used in macro __GL_UB_TO_FLOAT */
    for (i = 0; i < 256; i++) {
	gc->constants.uByteToFloat[i] = i * gc->constants.oneOver255;
    }

    {
	/* Compute some fixed point viewport constants (on the stack here, not SGI's malloc'ed mem[4]: four
	** separate volatile locals give the original's slots) */
	volatile __GLfloat temp1, temp2, epsilon, besteps;

	temp1 = gc->constants.fviewportXAdjust;
	epsilon = one;
	besteps = one;
	for (;;) {
	    temp2 = temp1 + epsilon;
	    if (temp2 != temp1) {
		besteps = epsilon;
	    } else
		break;
	    epsilon = epsilon * 0.5f;
	}

	gc->constants.viewportEpsilon = besteps;
	/* in double: the original loads half first (the float form loads besteps and uses fsubr) */
	gc->constants.viewportAlmostHalf = (__GLfloat)((double)gc->constants.half - besteps);
    }

    numLights = gc->constants.numberOfLights;

    /* Misc machine state */
    __gl_beginMode = __GL_NEED_VALIDATE;
    gc->dirtyMask = 0xffffffff;
    gc->validateMask = ~0;
    gc->attributes.stackPointer = gc->attributes.stack;
    gc->attributes.clientStackPointer = gc->attributes.clientStack;
    gc->vertex.v0 = &gc->vertex.vbuf[0];

    vx = &gc->vertex.vbuf[0];
    for (i = 0; i < 16; i++, vx++) {
	vx->color = &vx->colors[0];
    }

    /* GL_LIGHTING_BIT state */
    gc->state.light.model.ambient.r = DefaultAmbient[0];
    gc->state.light.model.ambient.g = DefaultAmbient[1];
    gc->state.light.model.ambient.b = DefaultAmbient[2];
    gc->state.light.model.ambient.a = DefaultAmbient[3];
    gc->state.light.front.ambient.r = DefaultAmbient[0];
    gc->state.light.front.ambient.g = DefaultAmbient[1];
    gc->state.light.front.ambient.b = DefaultAmbient[2];
    gc->state.light.front.ambient.a = DefaultAmbient[3];
    gc->state.light.front.diffuse.r = DefaultDiffuse[0];
    gc->state.light.front.diffuse.g = DefaultDiffuse[1];
    gc->state.light.front.diffuse.b = DefaultDiffuse[2];
    gc->state.light.front.diffuse.a = DefaultDiffuse[3];
    gc->state.light.front.specular.r = DefaultBlack[0];
    gc->state.light.front.specular.g = DefaultBlack[1];
    gc->state.light.front.specular.b = DefaultBlack[2];
    gc->state.light.front.specular.a = DefaultBlack[3];
    gc->state.light.front.emissive.r = DefaultBlack[0];
    gc->state.light.front.emissive.g = DefaultBlack[1];
    gc->state.light.front.emissive.b = DefaultBlack[2];
    gc->state.light.front.emissive.a = DefaultBlack[3];
    gc->state.light.front.cmapa = 0;
    gc->state.light.front.cmaps = 1;
    gc->state.light.front.cmapd = 1;
    gc->state.light.back = gc->state.light.front;

    gc->light.front.specularExponent = -1;
    gc->light.front.specTable = NULL;
    gc->light.front.cache = NULL;
    gc->light.back.specularExponent = -1;
    gc->light.back.specTable = NULL;
    gc->light.back.cache = NULL;

    /* Initialize the individual lights */
    lss = (__GLlightSourceStateICD *)gc->state.light.source;
    lsm = (__GLlightSourceMachineICD *)gc->light.source;
    for (i = 0; i < numLights; i++, lss++, lsm++) {
	lss->ambient.r = DefaultBlack[0];
	lss->ambient.g = DefaultBlack[1];
	lss->ambient.b = DefaultBlack[2];
	lss->ambient.a = DefaultBlack[3];
	if (i == 0) {
	    lss->diffuse.r = DefaultWhite[0];
	    lss->diffuse.g = DefaultWhite[1];
	    lss->diffuse.b = DefaultWhite[2];
	    lss->diffuse.a = DefaultWhite[3];
	} else {
	    lss->diffuse.r = DefaultBlack[0];
	    lss->diffuse.g = DefaultBlack[1];
	    lss->diffuse.b = DefaultBlack[2];
	    lss->diffuse.a = DefaultBlack[3];
	}
	lss->specular = lss->diffuse;
	lss->position.z = one;
	lss->positionEye.z = one;
	lsm->position.z = one;
	lss->direction.z = -1;
	lsm->direction.z = -1;
	lss->spotLightCutOffAngle = 180;
	lss->constantAttenuation = one;
	lsm->spotTable = NULL;
	lsm->spotLightExponent = -1;
	lsm->cache = NULL;
    }
    gc->state.light.colorMaterialFace = GL_FRONT_AND_BACK;
    gc->state.light.indexMaterialFace = GL_FRONT_AND_BACK;
    gc->dirtyMask |= __GL_DIRTY_SHADEMODEL_ICD;
    gc->state.light.colorMaterialParam = GL_AMBIENT_AND_DIFFUSE;
    gc->state.light.indexMaterialParam = GL_INDEX_OFFSET;
    gc->state.light.shadingModel = GL_SMOOTH;

    /* GL_HINT_BIT state */
    gc->state.hints.perspectiveCorrection = GL_DONT_CARE;
    gc->state.hints.pointSmooth = GL_DONT_CARE;
    gc->state.hints.lineSmooth = GL_DONT_CARE;
    gc->state.hints.polygonSmooth = GL_DONT_CARE;
    gc->state.hints.fog = GL_DONT_CARE;

    /* GL_CURRENT_BIT state */
    gc->state.current.rasterPos.window.x = gc->constants.fviewportXAdjust;
    if (gc->constants.yInverted) {
	gc->state.current.rasterPos.window.y =
	    ((GLuint)gc->constants.height - gc->constants.viewportEpsilon) +
			gc->constants.fviewportYAdjust;
    } else {
	gc->state.current.rasterPos.window.y = gc->constants.fviewportYAdjust;
    }

    gc->state.current.rasterPos.clip.w = one;
    gc->state.current.rasterPos.texture[0].w = one;
    gc->state.current.rasterPos.color = &gc->state.current.rasterPos.colors[0];
    if (gc->modes.rgbMode) {
	gc->state.current.rasterPos.colors[0].r = DefaultWhite[0];
	gc->state.current.rasterPos.colors[0].g = DefaultWhite[1];
	gc->state.current.rasterPos.colors[0].b = DefaultWhite[2];
	gc->state.current.rasterPos.colors[0].a = DefaultWhite[3];
    } else {
	gc->state.current.rasterPos.colors[0].r = one;
    }
    gc->state.current.validRasterPos = GL_TRUE;
    gc->state.current.edgeTag = 0x1000;
    gc->state.current.fogCoord = zero;

    /* GL_FOG_BIT state */
    gc->state.fog.mode = GL_EXP;
    gc->state.fog.density = one;
    gc->state.fog.end = one;
    gc->state.fog.coordSource = GL_FRAGMENT_DEPTH_EXT;

    /* GL_POINT_BIT state */
    gc->state.point.requestedSize = one;
    gc->state.point.smoothSize = one;
    gc->state.point.aliasedSize = 1;
    gc->state.point.ext[0] = zero;
    gc->state.point.ext[1] = gc->constants.pointSizeMaximum;
    gc->state.point.ext[2] = one;
    gc->state.point.ext[3] = one;
    gc->state.point.ext[4] = zero;
    gc->state.point.ext[5] = zero;

    /* GL_LINE_BIT state */
    gc->state.line.requestedWidth = one;
    gc->state.line.smoothWidth = one;
    gc->state.line.aliasedWidth = 1;
    gc->state.line.stipple = 0xFFFF;
    gc->state.line.stippleRepeat = 1;

    /* GL_POLYGON_BIT state */
    gc->state.polygon.frontMode = GL_FILL;
    gc->state.polygon.backMode = GL_FILL;
    gc->state.polygon.cull = GL_BACK;
    gc->state.polygon.frontFaceDirection = GL_CCW;

    /* GL_POLYGON_STIPPLE_BIT state */
    for (i = 0; i < 4*32; i++) {
	gc->state.polygonStipple.stipple[i] = 0xFF;
    }
    for (i = 0; i < 32; i++) {
	gc->polygon.stipple[i] = 0xFFFFFFFF;
    }

    /* GL_STENCIL_BUFFER_BIT state */
    gc->state.stencil.testFunc = GL_ALWAYS;
    gc->state.stencil.mask = (1 << gc->modes.stencilBits) - 1;
    gc->state.stencil.fail = GL_KEEP;
    gc->state.stencil.depthFail = GL_KEEP;
    gc->state.stencil.depthPass = GL_KEEP;
    gc->state.stencil.writeMask = (1 << gc->modes.stencilBits) - 1;

    /* GL_DEPTH_BUFFER_BIT state */
    gc->state.depth.writeEnable = GL_TRUE;
    gc->state.depth.testFunc = GL_LESS;
    gc->state.depth.clear = 1.0;

    /* GL_COLOR_BUFFER_BIT state */
    gc->renderMode = GL_RENDER;
    gc->state.raster.alphaFunction = GL_ALWAYS;
    gc->state.raster.blendSrc = GL_ONE;
    gc->state.raster.blendDst = GL_ZERO;
    gc->state.raster.blendEquation = 0x8006;   /* GL_FUNC_ADD_EXT */
    gc->state.raster.indexFunction = GL_ALWAYS;
    gc->state.raster.logicOp = GL_COPY;
    gc->state.raster.rMask = GL_TRUE;
    gc->state.raster.gMask = GL_TRUE;
    gc->state.raster.bMask = GL_TRUE;
    gc->state.raster.aMask = GL_TRUE;
    if (gc->modes.doubleBufferMode) {
	gc->state.raster.drawBuffer = GL_BACK;
    } else {
	gc->state.raster.drawBuffer = GL_FRONT;
    }
    gc->state.raster.drawBufferReturn = gc->state.raster.drawBuffer;
    gc->state.current.userColor.r = one;
    gc->state.current.userColor.g = one;
    gc->state.current.userColor.b = one;
    gc->state.current.userColor.a = one;
    gc->state.current.userColorIndex = one;
    if (gc->modes.colorIndexMode) {
	gc->state.raster.writeMask = gc->frontBuffer.redMax;
	gc->state.current.color.r = gc->state.current.userColorIndex;
    } else {
	gc->state.current.color = gc->state.current.userColor;
    }
    gc->state.enables.general |= __GL_DITHER_ENABLE;

    gc->select.hit = GL_FALSE;
    gc->select.sp = gc->select.stack;

    gc->feedback.type = GL_2D;

    /*
    ** Initialize larger subsystems by calling their init codes.
    */
    __glInitColorTables(gc);
    __glInitEvaluatorState(gc);
    __glInitTextureState(gc);
    __glInitTransformState(gc);
    __glInitPixelState(gc);
    __glInitLUTCache(gc);
    __glInitVertexArrayState(gc);
}

/* 6907f130 -- __glDestroyContext (s_contxt.c) */
GLboolean __fastcall __glDestroyContext(__GLcontext *gc)
{
    __GLlightSourceMachineICD *lsm;
    GLint i;

    if (gc->ogPool2.data) {
	FREE(gc, gc->ogPool2.data);
    }
    if (gc->ogFloatPool.data) {
	FREE(gc, gc->ogFloatPool.data);
    }
    if (gc->ogBlocks[0].buffer) {
	FREE(gc, gc->ogBlocks[0].buffer);
    }
    __glReleaseCodeCache(gc);

    if (gc->attributes.stack) {
	__glFreeAttributeState(gc);
	FREE(gc, gc->attributes.stack);
    }

    for (i=0; i < gc->constants.maxClientAttribStackDepth; i++) {
	if (gc->attributes.clientStack[i]) {
	    FREE(gc, gc->attributes.clientStack[i]);
	}
    }
    if (gc->attributes.clientStack) {
	FREE(gc, gc->attributes.clientStack);
    }

    lsm = (__GLlightSourceMachineICD *)gc->light.source;
    for (i = 0; i < gc->constants.numberOfLights; i++, lsm++) {
	if (lsm->cache) {
	    __glFreeSpecLUT(gc, lsm->cache);
	}
    }
    if(gc->state.light.source) FREE(gc, gc->state.light.source);
    if(gc->light.source) FREE(gc, gc->light.source);
    if(gc->select.stack) FREE(gc, gc->select.stack);
    if(gc->state.transform.eyeClipPlanes) FREE(gc, gc->state.transform.eyeClipPlanes);
    if(gc->transform.modelViewStackAlloc) FREE(gc, gc->transform.modelViewStackAlloc);
    if(gc->transform.projectionStackAlloc) FREE(gc, gc->transform.projectionStackAlloc);
    for (i = 0; i < 2; i++) {
	if (gc->transform.textureStackAlloc[i]) FREE(gc, gc->transform.textureStackAlloc[i]);
    }
    if(gc->transform.clipTemp) FREE(gc, gc->transform.clipTemp);

    if (gc->polygon.shader.colors)
	FREE(gc, gc->polygon.shader.colors);
    if (gc->polygon.shader.fbcolors)
	FREE(gc, gc->polygon.shader.fbcolors);
    if (gc->polygon.shader.stipplePat)
	FREE(gc, gc->polygon.shader.stipplePat);

    if( gc->frontBuffer.alphaTestFuncTable ) {
	FREE(gc, gc->frontBuffer.alphaTestFuncTable);
    }

    /*
    ** Free other malloc'd data associated with the context
    */
    ((void (__fastcall *)(__GLcontext *))__glFreeColorTables)(gc);
    __glFreeIndexTestTable(gc);
    __glFreeEvaluatorState(gc);
    __glFreePixelState(gc);
    __glFreeVertexArrayState(gc);
    if (gc->dlist.dlistArray) __glFreeDlistState(gc);
    if (&gc->texture) __glFreeTextureState(gc);

    if (gc->light.front.cache) __glFreeSpecLUT(gc, gc->light.front.cache);
    if (gc->light.back.cache) __glFreeSpecLUT(gc, gc->light.back.cache);
    if (gc->light.lutCache) __glFreeLUTCache(gc);

    return GL_TRUE;
}

/************************************************************************/
/* SI functions (s_contxt.c / s_attrib.c / s_get.c) that the ICD exports as __stdcall entry points
*/

/* 6907f3d0 -- __glim_RenderMode */
GLint __stdcall __glim_RenderMode(GLenum mode)
{
    GLint rv;
    __GL_SETUP_NOT_IN_BEGIN2();

    switch (mode) {
      case GL_RENDER:
      case GL_FEEDBACK:
      case GL_SELECT:
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return 0;
    }

    __GL_API_STATE();

    /* Switch out of old render mode.  Get return value. */
    switch (gc->renderMode) {
      case GL_RENDER:
        rv = 0;
        break;
      case GL_FEEDBACK:
        rv = gc->feedback.overFlowed ? -1 :
            (GLint)(gc->feedback.result - gc->feedback.resultBase);
        break;
      case GL_SELECT:
        rv = gc->select.overFlowed ? -1 : gc->select.hits;
        break;
    }

    /* Switch to new render mode */
    gc->renderMode = mode;
    __GL_DELAY_VALIDATE(gc);
    switch (mode) {
      case GL_FEEDBACK:
        if (!gc->feedback.resultBase) {
            __glSetError(GL_INVALID_OPERATION);
            return rv;
        }
        gc->feedback.result = gc->feedback.resultBase;
        gc->feedback.overFlowed = GL_FALSE;
        break;
      case GL_SELECT:
        if (!gc->select.resultBase) {
            __glSetError(GL_INVALID_OPERATION);
            return rv;
        }
        gc->select.result = gc->select.resultBase;
        gc->select.overFlowed = GL_FALSE;
        gc->select.sp = gc->select.stack;
        gc->select.hit = GL_FALSE;
        gc->select.hits = 0;
        gc->select.z = 0;
        break;
    }
    return rv;
}

/*
** glctx.h -- the real __GLcontext of 3dfxOGL.dll 1.0.0.0761.
**
** SGI OpenGL Sample Implementation context in its MS ICD-kit (1.1 era) flavour, followed by the
** 3dfx hardware part ("hwcx").  Every member offset below was taken from the binary: the entry
** points that write the state, a per-offset census of the original's gc accesses and the SI 1.2.1 headers as a naming guide
** (ref/ogl-sample/gfx/lib/opengl/include/context.h and friends).  Unknown stretches are kept as
** `GLuint unk_<offset>[n]` so the structure stays byte exact; the GLCTX_CHECK() lines at the end
** make VC6 (/Zp4) verify the anchors at compile time.  No code reads or writes an unk_ member by
** name or offset (checked 2026-11-02, every function matching the original): only whole-record
** copies and clears (glPushAttrib, the context copy, memset) touch them, so they are padding.
**
** Status of the regions:  0x000..0x10c0 named (imports, eleven exports, mcFlags; the attribute
** block gc->state 0x58..0x8a8 pinned by the gl* entry points and glPushAttrib; constants /
** feedback / select / dlist in SI order; vertexArray with its array pointers), 0x10c0..0x241c
** named (dispatch states), procs 0x241c..0x27f4 named, attributes + vertex / light / texture /
** evaluator / transform / line / polygon / pixel / buffer machines and the buffers named, the OG
** workspace and fx core named, hwcx partly named (raw_* stretches).  The records the context points
** at are here too: light sources (state 0x74, machine 0x100), texture objects (generic 0xd4, sst
** 0x574), mipmap levels, device texture formats, colour tables, the texture env record (0x54).
*/
#ifndef __GLCTX_H__
#define __GLCTX_H__

typedef GLfloat __GLfloat;
typedef struct __GLcolorRec { __GLfloat r, g, b, a; } __GLcolor;
typedef struct __GLcoordRec { __GLfloat x, y, z, w; } __GLcoord;
typedef int (__fastcall *__GLproc)();       /* unspecified arguments; int like GCfn_any (callers use the result) */

/* ---- state (gc->state, 0x058..0x8a8, 0x850 bytes: the block glPushAttrib callocs and copies) ---- */

/* ---- the vertex (0xc0 bytes: gc->vertex.v0 = v + 0xc0 in the vertex procs) -------------------
** Pinned by the clip check (__glClipCheckAll: clip at 0x40, flagBits 0x10), the clip interpolation
** (ClipT2: window.w = 1/clip.w at 0x0c, texture[2] at 0x20 projected by clip.w) and the
** validateVertex procs (obj 0x50, colors[2] at 0x80 / 0x90, fog 0x18). */
typedef struct __GLvertexICDRec {
    __GLcoord   window;                     /* 0x00  x, y, z and w = 1/clip.w */
    GLuint      flagBits;                   /* 0x10  (>> 14 & 3) selects procs.validateVertex[] */
    GLuint      packedColor;                /* 0x14  *color packed to ARGB (procs.packVertexColor) */
    __GLfloat   fog;                        /* 0x18  (the sst code and the vertex-array cache vertices keep the
                                                 guard-band clip bits here) */
    __GLcolor  *color;                      /* 0x1c  -> colors[0] or colors[1] (read by the triangle fillers) */
    __GLcoord   texture[2];                 /* 0x20  two units */
    __GLcoord   clip;                       /* 0x40 */
    __GLcoord   eye;                        /* 0x50  eye.z is the GL_FRAGMENT_DEPTH_EXT fog distance */
    __GLcolor   colors[2];                  /* 0x60  front, back (the clip interpolators: r only in CI mode) */
    __GLcoord   obj;                        /* 0x80  glRasterPos stores (x, y, z, w) here, then clip = mvp * obj */
    __GLcoord   normal;                     /* 0x90 */
    __GLfloat   fogCoord;                   /* 0xa0  copied from state.current.fogCoord */
    __GLfloat   unk_a4[7];                  /* 0xa4 .. 0xc0 */
} __GLvertexICD;

typedef struct __GLcurrentStateRec {        /* 0x060 .. 0x1a8 (0x52 dwords) */
    __GLvertexICD rasterPos;                /* 0x060 .. 0x120  (p1 = &rasterPos in glRasterPos: clipCheck2, validateVertex) */
    GLboolean   validRasterPos;             /* 0x120 */
    GLubyte     pad121[3];
    GLuint      edgeTag;                    /* 0x124  glEdgeFlag */
    GLuint      unk_128[2];                 /* 0x128 */
    __GLcolor   userColor;                  /* 0x130  glColor* */
    __GLfloat   userColorIndex;             /* 0x140  glIndex* */
    GLuint      unk_144[3];                 /* 0x144 */
    __GLcolor   color;                      /* 0x150  the colour the lighting code reads */
    GLuint      packedColor;                /* 0x160  color as one 0xAARRGGBB dword (Glide vertex colour; read by generated code) */
    __GLcoord   normal;                     /* 0x164  glNormal3f writes x,y,z */
    GLuint      unk_174[3];                 /* 0x174 */
    __GLcoord   texture[2];                 /* 0x180  two texture units */
    __GLcoord  *activeTexture;              /* 0x1a0  glTexCoord* writes through this */
    __GLfloat   fogCoord;                   /* 0x1a4  EXT_fog_coord, stored as 1.0f - c (glFogCoordfEXT 690b45f0) */
} __GLcurrentState;

typedef struct __GLpointStateRec {          /* 0x1a8 */
    __GLfloat   requestedSize;              /* 0x1a8  glPointSize */
    __GLfloat   smoothSize;                 /* 0x1ac */
    GLint       aliasedSize;                /* 0x1b0 */
    __GLfloat   ext[6];                     /* 0x1b4  EXT_point_parameters (min, max, fade threshold, attenuation[3]) */
} __GLpointState;

typedef struct __GLlineStateRec {           /* 0x1cc */
    __GLfloat   requestedWidth;             /* 0x1cc  glLineWidth */
    __GLfloat   smoothWidth;                /* 0x1d0 */
    GLint       aliasedWidth;               /* 0x1d4 */
    GLushort    stipple;                    /* 0x1d8  glLineStipple */
    GLshort     stippleRepeat;              /* 0x1da */
} __GLlineState;

typedef struct __GLpolygonStateRec {        /* 0x1dc */
    GLenum      frontMode;                  /* 0x1dc  glPolygonMode */
    GLenum      backMode;                   /* 0x1e0 */
    GLenum      cull;                       /* 0x1e4  glCullFace */
    GLenum      frontFaceDirection;         /* 0x1e8  glFrontFace */
    __GLfloat   factor;                     /* 0x1ec  glPolygonOffset */
    __GLfloat   units;                      /* 0x1f0 */
} __GLpolygonState;

typedef struct __GLpolygonStippleStateRec { /* 0x1f4 */
    GLubyte     stipple[4 * 32];            /* 0x1f4 .. 0x274 */
} __GLpolygonStippleState;

typedef struct __GLpixelTransferModeRec {   /* 0x274  glPixelTransfer */
    __GLcolor   scale;                      /* 0x274 */
    __GLfloat   depthScale;                 /* 0x284  the older SI order (scale, depth scale, bias, depth bias):
                                                 glPixelTransfer (690bfcxx) stores DEPTH_SCALE here, ALPHA_BIAS at 0x294 */
    __GLcolor   bias;                       /* 0x288 */
    __GLfloat   depthBias;                  /* 0x298 */
    __GLfloat   zoomX;                      /* 0x29c  glPixelZoom */
    __GLfloat   zoomY;                      /* 0x2a0 */
    GLint       indexShift;                 /* 0x2a4 */
    GLint       indexOffset;                /* 0x2a8 */
    GLboolean   mapColor;                   /* 0x2ac */
    GLboolean   mapStencil;                 /* 0x2ad */
    GLubyte     pad2ae[2];
} __GLpixelTransferMode;

typedef struct __GLpixelMapStateRec {       /* 12 bytes, ten of them 0x2b0..0x328 (glPixelMap): the SI
                                               __GLpixelMapHead (glsi.h), whose base union this header does not
                                               declare; users cast the array to it */
    GLint       size, tableId;
    GLuint      base;
} __GLpixelMapState;

typedef struct __GLpixelPackModeRec {       /* 20 bytes (no 3D image fields in this kit) */
    GLboolean   swapEndian;                 /* +0 */
    GLboolean   lsbFirst;                   /* +1 */
    GLubyte     pad2[2];
    GLuint      lineLength;                 /* +4 */
    GLuint      skipLines;                  /* +8 */
    GLuint      skipPixels;                 /* +c */
    GLuint      alignment;                  /* +10 */
} __GLpixelPackMode;

typedef struct __GLpixelStateRec {          /* 0x274 */
    __GLpixelTransferMode transferMode;     /* 0x274 */
    __GLpixelMapState map[10];              /* 0x2b0 */
    __GLpixelPackMode packModes;            /* 0x328  glPixelStore (pack) */
    __GLpixelPackMode unpackModes;          /* 0x33c  glPixelStore (unpack) */
    GLuint      unk_350;                    /* 0x350 */
    GLenum      readBuffer;                 /* 0x354  glReadBuffer */
    GLenum      readBufferReturn;           /* 0x358 */
} __GLpixelState;

typedef struct __GLlightModelStateRec {     /* 0x370 */
    __GLcolor   ambient;                    /* 0x370 */
    GLboolean   localViewer;                /* 0x380 */
    GLboolean   twoSided;                   /* 0x381 */
    GLubyte     pad382[2];
} __GLlightModelState;

typedef struct __GLmaterialStateRec {       /* 0x58 bytes (SI has 0x50) */
    __GLcolor   ambient;                    /* +00 */
    __GLcolor   diffuse;                    /* +10 */
    __GLcolor   specular;                   /* +20 */
    __GLcolor   emissive;                   /* +30 */
    __GLfloat   specularExponent;           /* +40 */
    __GLfloat   cmapa;                      /* +44 */
    __GLfloat   cmaps;                      /* +48 */
    __GLfloat   cmapd;                      /* +4c */
    __GLfloat   indexOffset;                /* +50  glMaterial(GL_INDEX_OFFSET 0x0d13): EXT_index_material's CI lighting
                                               source (light.front/back.indexOffsetPtr) */
    __GLfloat   indexShift;                 /* +54  glMaterial(GL_INDEX_SHIFT 0x0d12) */
} __GLmaterialState;

/* one light's glLight state (0x74 bytes; state.light.source[] is calloc'd numberOfLights * 0x74, 6907e440) */
typedef struct __GLlightSourceStateRec {
    __GLcolor   ambient;                    /* +0x00 */
    __GLcolor   diffuse;                    /* +0x10 */
    __GLcolor   specular;                   /* +0x20 */
    __GLcoord   position;                   /* +0x30  as given */
    __GLcoord   positionEye;                /* +0x40  times the model-view (inverse-transpose xf4) */
    __GLcoord   direction;                  /* +0x50 */
    __GLfloat   spotLightExponent;          /* +0x60 */
    __GLfloat   spotLightCutOffAngle;       /* +0x64 */
    __GLfloat   constantAttenuation;        /* +0x68 */
    __GLfloat   linearAttenuation;          /* +0x6c */
    __GLfloat   quadraticAttenuation;       /* +0x70 */
} __GLlightSourceStateICD;

typedef struct __GLlightStateRec {          /* 0x35c */
    GLenum      colorMaterialFace;          /* 0x35c  glColorMaterial */
    GLenum      colorMaterialParam;         /* 0x360 */
    GLenum      indexMaterialFace;          /* 0x364  glIndexMaterialSGI face (GL_INDEX_MATERIAL_FACE_EXT), GL_FRONT_AND_BACK */
    GLenum      indexMaterialParam;         /* 0x368  GL_INDEX_OFFSET, the only mode (GL_INDEX_MATERIAL_PARAMETER_EXT) */
    GLenum      shadingModel;               /* 0x36c  glShadeModel */
    __GLlightModelState model;              /* 0x370 */
    __GLmaterialState front;                /* 0x384 */
    __GLmaterialState back;                 /* 0x3dc  glMaterial(GL_BACK) */
    __GLlightSourceStateICD *source;        /* 0x434  the light sources (allocated, numberOfLights * 0x74) */
} __GLlightState;

typedef struct __GLfogStateRec {            /* 0x438 */
    GLenum      mode;                       /* 0x438 */
    __GLcolor   color;                      /* 0x43c */
    __GLfloat   density;                    /* 0x44c */
    __GLfloat   start;                      /* 0x450 */
    __GLfloat   end;                        /* 0x454 */
    __GLfloat   oneOverEMinusS;             /* 0x458 */
    __GLfloat   index;                      /* 0x45c */
    __GLcolor   bufColor;                   /* 0x460 */
    GLubyte     r, g, b, i;                 /* 0x470 */
    GLenum      coordSource;                /* 0x474  GL_FOG_COORDINATE_SOURCE_EXT: GL_FOG_COORDINATE_EXT 0x8451 /
                                               GL_FRAGMENT_DEPTH_EXT 0x8452 (the default) */
} __GLfogState;

typedef struct __GLdepthStateRec {          /* 0x478 */
    GLenum      testFunc;                   /* 0x478  glDepthFunc */
    GLboolean   writeEnable;                /* 0x47c  glDepthMask */
    GLubyte     pad47d[3];
    GLdouble    clear;                      /* 0x480  glClearDepth */
} __GLdepthState;

typedef struct __GLaccumStateRec {          /* 0x488 */
    __GLcolor   clear;                      /* 0x488  glClearAccum */
} __GLaccumState;

typedef struct __GLstencilStateRec {        /* 0x498 */
    GLenum      testFunc;                   /* 0x498  glStencilFunc */
    GLshort     clear;                      /* 0x49c  glClearStencil */
    GLshort     reference;                  /* 0x49e */
    GLshort     mask;                       /* 0x4a0 */
    GLshort     writeMask;                  /* 0x4a2  glStencilMask */
    GLenum      fail;                       /* 0x4a4  glStencilOp */
    GLenum      depthFail;                  /* 0x4a8 */
    GLenum      depthPass;                  /* 0x4ac */
} __GLstencilState;

typedef struct __GLviewportRec {            /* 0x4b0 */
    GLint       x, y;                       /* 0x4b0  glViewport */
    GLsizei     width, height;              /* 0x4b8 */
    GLdouble    zNear;                      /* 0x4c0  glDepthRange */
    GLdouble    zFar;                       /* 0x4c8 */
    __GLcoord   scale;                      /* 0x4d0  x, y, z (w unused): window = ndc * scale + center */
    __GLcoord   center;                     /* 0x4e0  (not the SI's interleaved xScale, xCenter, ...: the
                                               vertex code reads x 0x4d0/0x4e0, y 0x4d4/0x4e4, z 0x4d8/0x4e8,
                                               and glDepthRange writes z = (f-n)/2, (n+f)/2) */
} __GLviewport;

typedef struct __GLtransformStateRec {      /* 0x4f0 */
    GLenum      matrixMode;                 /* 0x4f0  glMatrixMode */
    __GLcoord  *eyeClipPlanes;              /* 0x4f4  glClipPlane */
    __GLcoord   eyePos;                     /* 0x4f8  glCullParameterfvSGI */
    __GLcoord   eyePosObj;                  /* 0x508 */
    GLboolean   objEyeSpecified;            /* 0x518 */
    GLubyte     pad519[3];
} __GLtransformState;

typedef struct __GLenableStateRec {         /* 0x51c  glEnable / glDisable */
    GLuint      general;                    /* 0x51c */
    GLuint      texture[2];                 /* 0x520 */
    GLuint      lights;                     /* 0x528 */
    GLuint      clipPlanes;                 /* 0x52c */
    GLushort    eval1;                      /* 0x530 */
    GLushort    eval2;                      /* 0x532 */
} __GLenableState;

typedef struct __GLrasterStateRec {         /* 0x534 */
    GLenum      alphaFunction;              /* 0x534  glAlphaFunc */
    __GLfloat   alphaReference;             /* 0x538 */
    GLenum      blendSrc;                   /* 0x53c  glBlendFunc */
    GLenum      blendDst;                   /* 0x540 */
    GLenum      blendEquation;              /* 0x544  glBlendEquationEXT (GL_BLEND_EQUATION_EXT), GL_FUNC_ADD_EXT */
    GLenum      indexFunction;              /* 0x548  glIndexFuncSGI */
    __GLfloat   indexReference;             /* 0x54c */
    GLenum      logicOp;                    /* 0x550  glLogicOp */
    __GLcolor   clear;                      /* 0x554  glClearColor */
    __GLfloat   clearIndex;                 /* 0x564  glClearIndex */
    GLint       writeMask;                  /* 0x568  glIndexMask */
    GLboolean   rMask, gMask, bMask, aMask; /* 0x56c  glColorMask */
    GLenum      drawBuffer;                 /* 0x570  glDrawBuffer */
    GLenum      drawBufferReturn;           /* 0x574 */
} __GLrasterState;

typedef struct __GLhintStateRec {           /* 0x578  glHint */
    GLenum      perspectiveCorrection;      /* 0x578 */
    GLenum      pointSmooth;                /* 0x57c */
    GLenum      lineSmooth;                 /* 0x580 */
    GLenum      polygonSmooth;              /* 0x584 */
    GLenum      fog;                        /* 0x588 */
    GLenum      unk_58c;                    /* 0x58c */
    GLenum      ext590;                     /* 0x590  3dfx / PGI hints */
    GLenum      ext594;                     /* 0x594 */
} __GLhintState;

typedef struct __GLevaluatorGridRec {       /* glMapGrid: {start, finish, step, n} */
    __GLfloat   start;
    __GLfloat   finish;
    __GLfloat   step;
    GLint       n;
} __GLevaluatorGrid;

typedef struct __GLevaluatorStateRec {      /* 0x598 */
    __GLevaluatorGrid u1;                   /* 0x598  glMapGrid1 */
    __GLevaluatorGrid u2;                   /* 0x5a8  glMapGrid2 (u) */
    __GLevaluatorGrid v2;                   /* 0x5b8  glMapGrid2 (v) */
} __GLevaluatorState;

typedef struct __GLdlistStateRec {          /* 0x5c8 */
    GLuint      listBase;                   /* 0x5c8  glListBase */
} __GLdlistState;

typedef struct __GLtextureGenStateRec {     /* 36 bytes (SI) */
    GLenum      mode;
    __GLcoord   eyePlaneEquation;
    __GLcoord   objectPlaneEquation;
} __GLtextureGenState;

typedef struct __GLtextureEnvStateRec {     /* 0x54: SI mode + colour, then EXT/ARB_texture_env_combine */
    GLenum      mode;                       /* +0x00  glTexEnv(GL_TEXTURE_ENV_MODE) */
    __GLcolor   color;                      /* +0x04  GL_TEXTURE_ENV_COLOR */
    GLenum      combineRGB;                 /* +0x14 */
    GLenum      combineAlpha;               /* +0x18 */
    GLenum      sourceRGB[3];               /* +0x1c */
    GLenum      sourceAlpha[3];             /* +0x28 */
    GLenum      operandRGB[3];              /* +0x34 */
    GLenum      operandAlpha[3];            /* +0x40 */
    __GLfloat   rgbScale;                   /* +0x4c */
    __GLfloat   alphaScale;                 /* +0x50 */
} __GLtextureEnvState;

/* Per-unit texture state: 0x164 bytes, two units filling 0x5cc..0x894 (glDeleteTextures, __glim_DeleteTextures,
** steps 0x164 per unit and 0x2c per target from unit+0x90).  The active unit is in the 3dfx area
** (0x350b0), not here. */
typedef struct __GLtexObjParamICDRec {      /* 0x24 at texture +0x08: the object's own parameters (createTexture sets
                                               the defaults; the per-target copy in the state is __GLtexParamICD) */
    GLuint      hwKey;                      /* +0x00 */
    GLenum      sWrapMode, tWrapMode;       /* +0x04  GL_REPEAT at create */
    GLenum      minFilter, magFilter;       /* +0x0c  GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR */
    __GLcolor   borderColor;                /* +0x14 */
} __GLtexObjParamICD;
typedef struct __GLtexParamICDRec {         /* 0x2c: per-target copy of the bound object's params, name and priority
                                               (object +0x08 .. +0x34) */
    __GLtexObjParamICD params;              /* +0x00 */
    GLuint      name;                       /* +0x24 */
    __GLfloat   priority;                   /* +0x28 */
} __GLtexParamICD;

typedef struct __GLtextureUnitStateICDRec { /* 0x164 */
    __GLtextureGenState s, t, r, q;         /* +0x000  glTexGen (mode at unit+0, +0x24 ...) */
    __GLtexParamICD param[4];               /* +0x090  one per texture target */
    __GLtextureEnvState *env;               /* +0x140  numberOfTextureEnvs records (0x54 bytes) from imports.calloc at
                                                 texture init (6909c80d), indexed by target - GL_TEXTURE_ENV;
                                                 the software texture env (__glTextureBlend*) reads unit 0's */
    __GLfloat   scaleBias[2][4];            /* +0x144  [0] scale r, g, b, a (1), [1] bias (0): applied before the
                                                 texture colour table (the 3dfx ancestor's texture scale / bias); only
                                                 init and push / pop attrib touch them */
} __GLtextureUnitStateICD;

typedef struct __GLtextureStateRec {        /* 0x5cc */
    __GLtextureUnitStateICD unit[2];        /* 0x5cc, 0x730 */
} __GLtextureState;

typedef struct __GLscissorRec {             /* 0x894  glScissor */
    GLint       scissorX;                   /* 0x894 */
    GLint       scissorY;                   /* 0x898 */
    GLuint      scissorWidth;               /* 0x89c */
    GLuint      scissorHeight;              /* 0x8a0 */
} __GLscissor;

typedef struct __GLattributeRec {           /* 0x058 .. 0x8a8 (0x850 bytes) */
    GLuint      mask;                       /* 0x058  glPushAttrib mask of a stacked copy */
    GLuint      unk_5c;                     /* 0x05c */
    __GLcurrentState current;               /* 0x060 */
    __GLpointState point;                   /* 0x1a8 */
    __GLlineState line;                     /* 0x1cc */
    __GLpolygonState polygon;               /* 0x1dc */
    __GLpolygonStippleState polygonStipple; /* 0x1f4 */
    __GLpixelState pixel;                   /* 0x274 */
    __GLlightState light;                   /* 0x35c */
    __GLfogState fog;                       /* 0x438 */
    __GLdepthState depth;                   /* 0x478 */
    __GLaccumState accum;                   /* 0x488 */
    __GLstencilState stencil;               /* 0x498 */
    __GLviewport viewport;                  /* 0x4b0 */
    __GLtransformState transform;           /* 0x4f0 */
    __GLenableState enables;                /* 0x51c */
    __GLrasterState raster;                 /* 0x534 */
    __GLhintState hints;                    /* 0x578 */
    __GLevaluatorState evaluator;           /* 0x598 */
    __GLdlistState list;                    /* 0x5c8 */
    __GLtextureState texture;               /* 0x5cc */
    __GLscissor scissor;                    /* 0x894 */
    GLuint      unk_8a4;                    /* 0x8a4 */
} __GLattribute;

/* ---- modes (gc->modes, 0x8b4..0x90c): SI __GLcontextModes + one dword ------------------- */
typedef struct __GLcontextModesICDRec {
    GLboolean   rgbMode;                    /* 0x8b4 */
    GLboolean   colorIndexMode;             /* 0x8b5 */
    GLboolean   doubleBufferMode;           /* 0x8b6 */
    GLboolean   stereoMode;                 /* 0x8b7 */
    GLboolean   haveAccumBuffer;            /* 0x8b8 */
    GLboolean   haveDepthBuffer;            /* 0x8b9 */
    GLboolean   haveStencilBuffer;          /* 0x8ba */
    GLubyte     pad8bb;
    GLint       redBits, greenBits, blueBits, alphaBits;        /* 0x8bc */
    GLuint      redMask, greenMask, blueMask, alphaMask;        /* 0x8cc */
    GLint       rgbBits;                    /* 0x8dc */
    GLint       indexBits;                  /* 0x8e0 */
    GLint       accumRedBits, accumGreenBits, accumBlueBits, accumAlphaBits;   /* 0x8e4 */
    GLint       depthBits;                  /* 0x8f4 */
    GLint       stencilBits;                /* 0x8f8 */
    GLint       numAuxBuffers;              /* 0x8fc */
    GLint       level;                      /* 0x900 */
    GLint       pixmapMode;                 /* 0x904 */
    GLboolean   layerPlane;                 /* 0x908  DrvCreateLayerContext's iLayerPlane, 1 for the stand-alone
                                               binding (wglRcCreateGC): set -> procs go straight into the
                                               dispatch table, not into hwcx.immProcs */
    GLboolean   singleBuffered;             /* 0x909  !doubleBufferMode */
    GLubyte     pad90a[2];
} __GLcontextModesICD;

/* ---- constants (gc->constants, 0x90c..0xe44): SI order, 1.1 field set, two extra dwords ------ */
typedef struct __GLcontextConstantsICDRec {
    __GLfloat   half;                       /* 0x90c  (the most-read gc field: 160 uses) */
    __GLfloat   one;                        /* 0x910 */
    __GLfloat   val255;                     /* 0x914 */
    __GLfloat   oneOver255;                 /* 0x918 */
    __GLfloat   val65535;                   /* 0x91c */
    __GLfloat   oneOver65535;               /* 0x920 */
    __GLfloat   val4294965000;              /* 0x924 */
    __GLfloat   oneOver4294965000;          /* 0x928 */
    const GLubyte *vendor;                  /* 0x92c  glGetString */
    const GLubyte *renderer;                /* 0x930 */
    const GLubyte *version;                 /* 0x934 */
    const GLubyte *extensions;              /* 0x938 */
    GLint       numberOfLights;             /* 0x93c */
    GLint       numberOfClipPlanes;         /* 0x940 */
    GLint       numberOfTextures;           /* 0x944  texture targets (boundTexture[unit][0..n)) */
    GLint       numberOfTextureEnvs;        /* 0x948  1: glTexEnv checks target - GL_TEXTURE_ENV against it */
    GLint       maxViewportWidth;           /* 0x94c  set to the Glide screen size (glViewport clamps to it) */
    GLint       maxViewportHeight;          /* 0x950 */
    GLint       viewportXAdjust;            /* 0x954 */
    GLint       viewportYAdjust;            /* 0x958 */
    __GLfloat   fviewportXAdjust;           /* 0x95c */
    __GLfloat   fviewportYAdjust;           /* 0x960 */
    __GLfloat   viewportEpsilon;            /* 0x964 */
    __GLfloat   viewportAlmostHalf;         /* 0x968 */
    __GLfloat   redScale, blueScale, greenScale, alphaScale;                 /* 0x96c */
    __GLfloat   oneOverRedScale, oneOverBlueScale, oneOverGreenScale, oneOverAlphaScale; /* 0x97c */
    __GLfloat   redRescale, blueRescale, greenRescale, alphaRescale;         /* 0x98c */
    __GLfloat   depthScale;                 /* 0x99c */
    __GLfloat   oneOverDepthScale;          /* 0x9a0 */
    __GLfloat   depthRescale;               /* 0x9a4 */
    __GLfloat   texSScale;                  /* 0x9a8  s / t scale of the bound texture (tex obj +0x48 / +0x4c,  */
    __GLfloat   texTScale;                  /* 0x9ac  per active unit): vertex s, t are multiplied before Glide */
    GLint       width;                      /* 0x9b0  drawable size as last seen by notifyResize */
    GLint       height;                     /* 0x9b4 */
    GLint       alphaTestSize;              /* 0x9b8 */
    __GLfloat   alphaTableConv;             /* 0x9bc */
    GLint       maxTextureSize;             /* 0x9c0 */
    GLint       maxMipMapLevel;             /* 0x9c4 */
    GLint       subpixelBits;               /* 0x9c8 */
    GLint       maxListNesting;             /* 0x9cc */
    __GLfloat   pointSizeMinimum;           /* 0x9d0 */
    __GLfloat   pointSizeMaximum;           /* 0x9d4 */
    __GLfloat   pointSizeGranularity;       /* 0x9d8 */
    __GLfloat   lineWidthMinimum;           /* 0x9dc */
    __GLfloat   lineWidthMaximum;           /* 0x9e0 */
    __GLfloat   lineWidthGranularity;       /* 0x9e4 */
    GLint       maxEvalOrder;               /* 0x9e8 */
    GLint       maxPixelMapTable;           /* 0x9ec */
    GLint       maxAttribStackDepth;        /* 0x9f0 */
    GLint       maxClientAttribStackDepth;  /* 0x9f4 */
    GLint       maxNameStackDepth;          /* 0x9f8  glPushName */
    GLint       maxModelViewStackDepth;     /* 0x9fc */
    GLint       maxProjectionStackDepth;    /* 0xa00 */
    GLint       maxTextureStackDepth;       /* 0xa04 */
    __GLfloat   uByteToFloat[256];          /* 0xa08 .. 0xe08 */
    GLboolean   yInverted;                  /* 0xe08 */
    GLubyte     pade09[3];
    GLint       ySign;                      /* 0xe0c */
    GLuint      cpuCaps;                    /* 0xe10  SI order; |= the detected-CPU global __glCpuCaps (gl_xform.c): __GL_CPU_* below */
    GLint       ogCacheCount0, ogCacheBytes0;   /* 0xe14  OG caches (690e9890): {entries, bytes each}; 4, 0x400 */
    GLint       ogCacheCount1, ogCacheBytes1;   /* 0xe1c  4, 0x100 */
    GLint       ogCacheCount2, ogCacheBytes2;   /* 0xe24  0x28, 0x1000 */
    GLint       ogScratchBytes;             /* 0xe2c  0x800 */
    GLint       ogCacheCount3, ogCacheBytes3;   /* 0xe30  0x28, 0x4000 */
    GLint       ogCacheCount4, ogCacheBytes4;   /* 0xe38  0x14, 0x400 */
    GLint       maxTextureUnits;            /* 0xe40  GL_MAX_TEXTURE_UNITS_ARB */
} __GLcontextConstantsICD;

/* constants.cpuCaps bits (__glDetectCPU 6908278f; __GL_FORCE_MMX / __GL_FORCE_K3D / __GL_FORCE_KNI override the first
** three) */
#define __GL_CPU_MMX                0x01
#define __GL_CPU_3DNOW              0x02    /* AMD extended CPUID bit 31 (K3D) */
#define __GL_CPU_SSE                0x04    /* KNI, and the 3dfx display driver allows it (ExtEscape 0x3df4 / 0xfd4) */
#define __GL_CPU_AMD_MODEL6         0x08    /* the AMD K6 family by model */
#define __GL_CPU_AMD_MODEL7         0x10

/* EXT_fog_coord (not in the SDK's GL/gl.h) */
#define GL_FOG_COORDINATE_SOURCE_EXT        0x8450
#define GL_FOG_COORDINATE_EXT               0x8451
#define GL_FRAGMENT_DEPTH_EXT               0x8452
#define GL_FOG_COORDINATE_ARRAY_EXT         0x8457

typedef struct __GLfeedbackMachineRec {     /* 0xe44  glFeedbackBuffer */
    GLboolean   overFlowed;                 /* 0xe44 */
    GLubyte     pade45[3];
    GLfloat    *resultBase;                 /* 0xe48 */
    GLfloat    *result;                     /* 0xe4c */
    GLint       resultLength;               /* 0xe50 */
    GLenum      type;                       /* 0xe54 */
} __GLfeedbackMachine;

typedef struct __GLselectMachineRec {       /* 0xe58  glSelectBuffer / names */
    GLboolean   hit;                        /* 0xe58 */
    GLubyte     pade59[3];
    GLuint     *stack;                      /* 0xe5c */
    GLuint     *sp;                         /* 0xe60 */
    GLboolean   overFlowed;                 /* 0xe64 */
    GLubyte     pade65[3];
    GLuint     *resultBase;                 /* 0xe68 */
    GLuint     *result;                     /* 0xe6c */
    GLint       resultLength;               /* 0xe70 */
    GLint       hits;                       /* 0xe74 */
    GLuint     *z;                          /* 0xe78 */
} __GLselectMachine;

typedef struct __GLcompiledDlistRec {       /* 20 bytes (SI) */
    GLint       freeCount;                  /* +0x00 */
    GLuint      genericFlags;               /* +0x04  __GL_DLFLAG_*: 1 = the list holds a vertex, 0x100 material */
    GLuint      machineFlags;               /* +0x08 */
    struct __GLdlistOpICDRec *dlist;        /* +0x0c  the op chain (glsi.h __GLdlistOpICD) */
    struct __GLdlistOpICDRec *lastDlist;    /* +0x10  its tail, for appends */
} __GLcompiledDlist;

typedef struct __GLdlistMachineRec {        /* 0xe7c  (SI layout, byte for byte) */
    struct __GLdlistArrayRec *dlistArray;   /* 0xe7c  shared by wglShareLists (refcount) */
    struct __GLnamesArrayRec *namesArray;   /* 0xe80  glGenLists / glIsList / glDeleteLists (glsi.h __GLnamesArray) */
    void       *newArray;                   /* 0xe84 */
    void       *freeArray;                  /* 0xe88 */
    void       *optimizer;                  /* 0xe8c */
    void       *compiler;                   /* 0xe90 */
    void       *checkOp;                    /* 0xe94 */
    void       *initState;                  /* 0xe98 */
    void      **baseListExec;               /* 0xe9c */
    void      **listExec;                   /* 0xea0 */
    void      **machineListExec;            /* 0xea4 */
    void       *machineListFree;            /* 0xea8 */
    void       *machineMoreBlocks;          /* 0xeac */
    void       *machineListValidateExec;    /* 0xeb0 */
    GLint       nesting;                    /* 0xeb4  glCallList */
    GLenum      mode;                       /* 0xeb8 */
    GLuint      currentList;                /* 0xebc  glNewList */
    __GLcompiledDlist listData;             /* 0xec0 */
    struct __GLarenaRec *arena;             /* 0xed4  glsi.h __GLarena (procs.newArena) */
    void       *malloc;                     /* 0xed8 */
    void       *realloc;                    /* 0xedc */
    void       *free;                       /* 0xee0 */
} __GLdlistMachine;

typedef struct __GLarrayPointerICDRec {     /* 0x20 bytes: gl*Pointer writes all eight dwords */
    const GLubyte *pointer;                 /* +0x00  as given (bytes: element i is at pointer + i * stride) */
    GLint       size;                       /* +0x04 */
    GLenum      type;                       /* +0x08 */
    GLint       stride;                     /* +0x0c  effective (0 -> the element size from 690f46d0) */
    GLsizei     userStride;                 /* +0x10  as given */
    GLsizei     count;                      /* +0x14  gl*PointerEXT count (GL_*_ARRAY_COUNT_EXT); 0 from gl*Pointer */
    __GLproc    fetch;                      /* +0x18  per type/size table (690f4968 for vertex, ...) */
    __GLproc    fetch2;                     /* +0x1c  same table, 11 types further on */
} __GLarrayPointerICD;
/* the two texture coordinate arrays are interleaved field by field (offset = field * 8 + unit * 4):
** glTexCoordPointer indexes every field by clientActiveTexture, and the init loop steps 4 bytes */
typedef struct __GLtexArrayPointersICDRec {
    const GLubyte *pointer[2];              /* 0xf64 */
    GLint       size[2];                    /* 0xf6c */
    GLenum      type[2];                    /* 0xf74 */
    GLint       stride[2];                  /* 0xf7c */
    GLsizei     userStride[2];              /* 0xf84 */
    GLsizei     count[2];                   /* 0xf8c */
    __GLproc    fetch[2];                   /* 0xf94 */
    __GLproc    fetch2[2];                  /* 0xf9c */
} __GLtexArrayPointersICD;

typedef struct __GLvertArrayMachineRec {    /* 0xee4 .. 0x10c0 */
    __GLarrayPointerICD vertex;             /* 0xee4  glVertexPointer */
    __GLarrayPointerICD normal;             /* 0xf04  glNormalPointer */
    __GLarrayPointerICD color;              /* 0xf24  glColorPointer */
    __GLarrayPointerICD index;              /* 0xf44  glIndexPointer */
    __GLtexArrayPointersICD texture;        /* 0xf64  glTexCoordPointer (per unit, interleaved) */
    __GLarrayPointerICD edgeFlag;           /* 0xfa4  glEdgeFlagPointer */
    __GLarrayPointerICD fogCoord;           /* 0xfc4  glFogCoordPointerEXT (read by glArrayElement) */
    GLuint      formats;                    /* 0xfe4  packed type/size bits of the non-texture arrays (every gl*Pointer) */
    GLuint      enables;                    /* 0xfe8  glEnableClientState: component masks */
    GLuint      texFormats[2];              /* 0xfec  packed type/size bits per texture unit */
    GLuint      enableFlags;                /* 0xff4  glEnableClientState: one bit per array (0x10000 << unit for texcoords) */
    const GLvoid *interleavedPointer;       /* 0xff8  glInterleavedArrays pointer */
    GLuint      interleavedFormat;          /* 0xffc  glInterleavedArrays format; zeroed by every gl*Pointer */
    GLuint      flags;                      /* 0x1000  4 = locked (glLockArraysEXT); 1, 2 = cache state; 0x60 = draw
                                                 through glBegin / glArrayElement / glEnd */
    GLuint      fillFlags;                  /* 0x1004  OR-ed into every vertex flag word by the generated fill (od_vafill.c); never set: 0 */
    GLint       lockFirst;                  /* 0x1008 */
    GLsizei     lockCount;                  /* 0x100c */
    GLuint      cacheGranule;               /* 0x1010  0x1000: the vertex cache grows in these steps */
    GLuint      unk_1014;                   /* 0x1014 */
    GLuint      cacheBytes;                 /* 0x1018 */
    GLuint      cacheAlloc;                 /* 0x101c  imports.malloc / realloc result, freed by imports.free */
    __GLvertexICD *cacheBase;               /* 0x1020  cacheAlloc rounded up to 32 bytes */
    __GLvertexICD *vertexCache;             /* 0x1024  cacheBase - first: indexed by the absolute vertex number */
    GLuint      batchSize;                  /* 0x1028  36 at init (MAX_VERTEX_CACHE) */
    GLuint      unk_102c[(0x1044 - 0x102c) / 4];   /* 0x102c */
    __GLproc    drawArrays[10];             /* 0x1044  per primitive (glDrawArrays) */
    GLuint      lockPrimMask;               /* 0x106c  primitives that take the render-buffer lock (bit per mode) */
    GLuint      unk_1070[(0x1094 - 0x1070) / 4];
    __GLproc    fillRange;                  /* 0x1094  generated fill proc (hwGenVertexArrayFill), same entry as fillLockedRange */
    __GLproc    fillLockedRange;            /* 0x1098  (gc, first, first, count): fill the cache for the locked range */
    __GLproc    prepareCache;               /* 0x109c  (gc, 0, first, count, indices): size the vertex cache to the
                                                 index range and fill it; 69019d50 (sst) */
    GLuint      unk_10a0[2];
    __GLproc    fetchVertex;                /* 0x10a8  (gc, vertex): generated second entry of the fill proc; fetches
                                                 one cached vertex on demand unless its flags have 0x20 */
    GLuint      unk_10ac[(0x10c0 - 0x10ac) / 4];
} __GLvertArrayMachine;

/* vertexArray.enableFlags bits: one per array glEnableClientState turned on */
#define __GL_VA_NORMAL_ENABLE       0x00000001
#define __GL_VA_COLOR_ENABLE        0x00000002
#define __GL_VA_INDEX_ENABLE        0x00000008
#define __GL_VA_EDGEFLAG_ENABLE     0x00000010
#define __GL_VA_VERTEX_ENABLE       0x00000020
#define __GL_VA_FOGCOORD_ENABLE     0x00000040
#define __GL_VA_TEXCOORD_ENABLE(u)  (0x00010000 << (u))

/* vertexArray.flags bits */
#define __GL_VERTARRAY_CHANGED      0x00000002  /* an array pointer / enable changed since the cache was filled */
#define __GL_VERTARRAY_LOCKED       0x00000004  /* glLockArraysEXT is in effect */
#define __GL_VERTARRAY_CACHE_STALE  0x00020000  /* the locked vertex cache must be refilled (sst) */

/* glPushClientAttrib's record (s_attrib.c __GLclientAttribute, the ICD's 0x2c8 bytes: 690ba300 push,
** 690ba3e0 pop) */
typedef struct __GLclientAttributeICDRec {
    GLuint      mask;                       /* +0x000 */
    GLuint      unk_004[(0xb8 - 0x4) / 4];
    __GLpixelPackMode packModes;            /* +0x0b8 */
    __GLpixelPackMode unpackModes;          /* +0x0cc */
    GLuint      unk_0e0[(0xec - 0xe0) / 4];
    __GLvertArrayMachine vertexArray;       /* +0x0ec */
} __GLclientAttribute;

/* dl_table.c __GLdlistArray (0x14 bytes): only the reference count is touched outside dl_table */
typedef struct __GLdlistArrayRec {
    GLint       refcount;                   /* +0x00  contexts sharing the lists (wglShareLists) */
    GLuint      raw_04[4];
} __GLdlistArray;

/* ---- dispatch ---------------------------------------------------------------------------- */
#define __GL_TAB_ENTRIES 0x19a
typedef void (APIENTRY *__GLdispatchFn)();
typedef struct __GLdispatchTableRec { __GLdispatchFn fn[__GL_TAB_ENTRIES]; } __GLdispatchTable;   /* 0x668 bytes */

/* typed procs (the slots below whose signature is pinned by their callers) */
typedef __GLfloat (__fastcall *__GLfogVertexProc)(struct __GLcontextRec *gc, __GLvertexICD *vx);
typedef void (__fastcall *__GLbitmapProc)(struct __GLcontextRec *gc, GLsizei width, GLsizei height,
                                          GLfloat xOrig, GLfloat yOrig, GLfloat xMove, GLfloat yMove,
                                          const GLubyte *bitmap);
typedef GLboolean (__fastcall *__GLobtainSLockProc)(struct __GLcontextRec *gc);
/* procs.matrix[0..3], the SI's __GLmatrixProcs: (*__GL_MATRIX_MULT(gc))(res, a, b) */
typedef void (__fastcall *__GLmatrixCopyProc)(struct __GLmatrixICDRec *dst, const struct __GLmatrixICDRec *src);
typedef void (__fastcall *__GLmatrixMakeIdentityProc)(struct __GLmatrixICDRec *m);
typedef void (__fastcall *__GLmatrixMultProc)(struct __GLmatrixICDRec *res, const struct __GLmatrixICDRec *a,
                                              const struct __GLmatrixICDRec *b);
#define __GL_MATRIX_COPY(gc)             ((__GLmatrixCopyProc)(gc)->procs.matrix[0])          /* __glCopyMatrix 69079af0 */
#define __GL_MATRIX_INVERT_TRANSPOSE(gc) ((__GLmatrixCopyProc)(gc)->procs.matrix[1])          /* (inverse, src) 69079f60 */
#define __GL_MATRIX_MAKE_IDENTITY(gc)    ((__GLmatrixMakeIdentityProc)(gc)->procs.matrix[2])  /* __glMakeIdentity 69079b60 */
#define __GL_MATRIX_MULT(gc)             ((__GLmatrixMultProc)(gc)->procs.matrix[3])          /* __glMultMatrix 69079c50 */

/* ---- procs (gc->procs, 0x241c..0x27f4): SI __GLprocs order with the ICD's differences --------
** Anchors: the gc->procs stores in src/wgl/wgldev.c (wglDevInstallHWProcs), its HWP table and the
** entry points (glScissor -> applyScissor/computeClipBox, glColor -> applyColor, glBegin/glEnd ->
** beginPrim/endPrim, gl*Pixels -> drawPixels/copyPixels/readPixels, glBitmap -> bitmap). */
/* SI __GLspanProcs / __GLlineProcs / __GLpixelSpanProcs.  The span arrays have the SI's 15 entries:
** __glProcessSpan (690cde60) calls spanFuncs from 0x25ac and stippledSpanFuncs from 0x25e8 */
typedef struct __GLspanProcsICDRec {        /* 0x25ac */
    __GLproc    spanFuncs[15];              /* 0x25ac */
    __GLproc    stippledSpanFuncs[15];      /* 0x25e8 */
    GLint       n, m;                       /* 0x2624 */
    __GLproc    processSpan;                /* 0x262c */
} __GLspanProcsICD;
typedef struct __GLlineProcsICDRec {        /* 0x2630 */
    __GLproc    lineFuncs[16];
    __GLproc    stippledLineFuncs[16];      /* 0x2670 */
    GLint       n, m, l;                    /* 0x26b0 */
    __GLproc    processLine;                /* 0x26bc */
    __GLproc    wideLineRep;                /* 0x26c0 */
    __GLproc    wideStippledLineRep;        /* 0x26c4 */
    __GLproc    drawLine;                   /* 0x26c8 */
    __GLproc    drawStippledLine;           /* 0x26cc */
} __GLlineProcsICD;
typedef struct __GLpixelSpanProcsICDRec {   /* 0x26d0 */
    __GLproc    spanReadCI, spanReadCI2, spanReadRGBA, spanReadRGBA2;
    __GLproc    spanReadDepth, spanReadDepth2, spanReadStencil, spanReadStencil2;
    __GLproc    spanRenderCI, spanRenderCI2, spanRenderRGBA, spanRenderRGBA2;
    __GLproc    spanRenderDepth, spanRenderDepth2, spanRenderStencil, spanRenderStencil2;
} __GLpixelSpanProcsICD;

typedef struct __GLprocsICDRec {
    __GLproc    flush;                      /* 0x241c  wglDevFlush */
    __GLproc    finish;                     /* 0x2420  wglDevFinish */
    __GLproc    error;                      /* 0x2424 */
    __GLproc    waitIdle;                   /* 0x2428  wglDevFinishFront */
    __GLproc    validate;                   /* 0x242c  wglGlideValidate */
    __GLproc    revalidate;                 /* 0x2430  ICD addition (gc, arg), HW and SW both 6901a940: hardware state
                                               validation (690193b0), then re-pick store / bitmap / span / pixel
                                               procs from dirtyMaskSaved */
    __GLproc    pickBlendProcs;             /* 0x2434  installs 0x276c..0x277c */
    __GLproc    pickColorMaterialProcs;     /* 0x2438  applyColor, DTPixel */
    __GLproc    pickTextureProcs;           /* 0x243c */
    __GLproc    pickCalcTextureProcs;       /* 0x2440  calcTexture */
    __GLproc    pickFogProcs;               /* 0x2444  0x2760..0x2768 */
    __GLproc    pickTransformProcs;         /* 0x2448  push/pop/loadIdentity */
    __GLproc    pickPointProcs;             /* 0x244c */
    __GLproc    pickLineProcs;              /* 0x2450  line strip / loop vertex procs */
    __GLproc    pickTriangleProcs;          /* 0x2454  renderTriangle, fillTriangle, strip / quad procs */
    __GLproc    pickRenderBitmapProcs;      /* 0x2458 */
    __GLproc    pickPixelProcs;             /* 0x245c  read buffer, pixel modify flags */
    __GLproc    pickClipProcs;              /* 0x2460 */
    __GLproc    pickParameterClipProcs;     /* 0x2464 */
    __GLproc    pickBufferProcs;            /* 0x2468  wglGlideSetupDrawBuffers */
    __GLproc    pickStoreProcs;             /* 0x246c */
    __GLproc    pickSpanProcs;              /* 0x2470 */
    __GLproc    pickVertexProcs;            /* 0x2474  clip checks, validateVertex */
    __GLproc    pickVertexArrayProcs;       /* 0x2478  (no pickVertexCacheProcs in this kit) */
    __GLproc    pickMatrixProcs;            /* 0x247c */
    __GLproc    pickInvTransposeProcs;      /* 0x2480 */
    __GLproc    pickMvpMatrixProcs;         /* 0x2484 */
    __GLproc    classifyMatrix;             /* 0x2488  (gc, matrix) after a matrix change: sets matrix->elementClass
                                                 (sst: 69047d70) */
    __GLproc    pickAllProcsPhase1;         /* 0x248c */
    __GLproc    pickAllProcsPhase2;         /* 0x2490 */
    __GLproc    matrix[4];                  /* 0x2494  copy, invertTranspose, makeIdentity, mult (__GL_MATRIX_*(gc)) */
    __GLproc    pushMatrix;                 /* 0x24a4 */
    __GLproc    popMatrix;                  /* 0x24a8 */
    __GLproc    loadIdentity;               /* 0x24ac */
    __GLproc    computeInverseTranspose;    /* 0x24b0 */
    __GLproc    normalize;                  /* 0x24b4 */
    __GLproc    applyScissor;               /* 0x24b8  wglDevClipWindow (no scaleVector in this kit) */
    __GLproc    applyViewport;              /* 0x24bc */
    __GLproc    computeClipBox;             /* 0x24c0 */
    __GLproc    v;                          /* 0x24c4 */
    __GLproc    clipCheck2;                 /* 0x24c8  __glClipCheckAll2 (falls into 6907a810) */
    __GLproc    clipCheck3;                 /* 0x24cc  __glClipCheckAll3 */
    __GLproc    clipCheck4;                 /* 0x24d0  __glClipCheckAll4 */
    __GLproc    vertex;                     /* 0x24d4 */
    __GLproc    vertexPoints;               /* 0x24d8 */
    __GLproc    vertexLStrip;               /* 0x24dc */
    __GLproc    vertex2ndLines;             /* 0x24e0 */
    __GLproc    vertex2ndLLoop;             /* 0x24e4 */
    __GLproc    endLLoop;                   /* 0x24e8 */
    __GLproc    vertexTFan;                 /* 0x24ec */
    __GLproc    vertexEvenTStrip;           /* 0x24f0 */
    __GLproc    vertexOddTStrip;            /* 0x24f4 */
    __GLproc    vertex3rdTris;              /* 0x24f8 */
    __GLproc    vertex4thQStrip;            /* 0x24fc */
    __GLproc    vertex4thQuads;             /* 0x2500 */
    __GLproc    vertexPoly;                 /* 0x2504 */
    __GLproc    endPoly;                    /* 0x2508 */
    __GLproc    validateVertex[4];          /* 0x250c  indexed by (v->flagBits >> 14) & 3 */
    __GLproc    ec1;                        /* 0x251c */
    __GLproc    ec2;                        /* 0x2520 */
    __GLproc    calcTexture;                /* 0x2524 */
    __GLproc    calcTextureUnit[2];         /* 0x2528 */
    __GLproc    applyColor;                 /* 0x2530  glColor* */
    __GLproc    calcColor;                  /* 0x2534 */
    __GLproc    calcColor2;                 /* 0x2538 */
    __GLproc    calcRasterColor;            /* 0x253c */
    __GLproc    packVertexColor;            /* 0x2540  3dfx: v->color (+0x1c) -> packed ARGB at v+0x14 (hw 6904a3f0) */
    __GLproc    packColor;                  /* 0x2544  3dfx: (gc, const __GLcolor *) -> ARGB dword (hw 6904a510) */
    __GLproc    colorMaterial;              /* 0x2548  SI order from here: colorMaterial, changeMaterial, ... */
    __GLproc    changeMaterial;             /* 0x254c  (gc, ms, msm): five variants 69092720..69092ae0 */
    __GLproc    matValidate;                /* 0x2550  (gc): revalidate buffered vertices (vbuf0..v0, v1, v2+v3, ...) */
    __GLproc    DTPixel;                    /* 0x2554  from the table at 690f3f50, indexed by the depth function */
    __GLproc    beginPrim[10];              /* 0x2558  glBegin, indexed by the primitive */
    __GLproc    endPrim;                    /* 0x2580  glEnd */
    __GLproc    renderTriangle;             /* 0x2584 */
    __GLproc    renderTriangle2;            /* 0x2588 */
    __GLproc    fillTriangle;               /* 0x258c */
    __GLproc    fillTriangle2;              /* 0x2590 */
    __GLproc    drawIndexedTriangles;       /* 0x2594  ICD: (gc, indices, vertex cache, count - 2) an indexed GL_TRIANGLES
                                                 list (690b0190, sst DrawElements); picked by the sst (69063290 / 690619c0) */
    __GLproc    unk_2598;                   /* 0x2598 */
    __GLproc    clipTriangle;               /* 0x259c  (gc, a, b, c, orCodes): the triangle procs clip through it */
    __GLproc    clipPolygon;                /* 0x25a0  (gc, v, nv): __glClipPolygon 69096910 */
    __GLproc    polyClipParam;              /* 0x25a4  (dst, a, b, t): picked by __glGenericPickParameterClipProcs */
    __GLproc    rect;                       /* 0x25a8  (gc, x0, y0, x1, y1) floats on the stack: __glRect 690976a0 */
    __GLspanProcsICD span;                  /* 0x25ac */
    __GLlineProcsICD line;                  /* 0x2630 */
    __GLpixelSpanProcsICD pixel;            /* 0x26d0 */
    __GLproc    renderLine;                 /* 0x2710 */
    __GLproc    renderLine2;                /* 0x2714 */
    __GLproc    clipLine;                   /* 0x2718  (gc, v0, v1) */
    __GLproc    lineClipParam;              /* 0x271c */
    __GLproc    renderTrianglePolygonModeLine;   /* 0x2720  (gc, a, b, c) */
    __GLproc    rasterizeLine;              /* 0x2724 */
    __GLproc    renderPoint;                /* 0x2728  (gc, v) */
    __GLproc    renderPoint2;               /* 0x272c */
    __GLproc    renderTrianglePolygonModePoint;  /* 0x2730 */
    __GLbitmapProc bitmap;                  /* 0x2734  glBitmap: __glDrawBitmap 690976f0 */
    __GLproc    renderBitmap;               /* 0x2738 */
    __GLproc    calcLineRho[2];             /* 0x273c  per unit (690a4f20 reads gc->line.v0/v1) */
    __GLproc    calcPolygonRho[2];          /* 0x2744  per unit (690a4da0 reads the shade record) */
    __GLproc    texture[2];                 /* 0x274c  per unit: the texture object's textureFunc (+0xa4) */
    __GLproc    textureRaster[2];           /* 0x2754  same, or 690a4ff0 when getModeFlags & 0x1000000 */
    __GLproc    nonUnityQ;                  /* 0x275c  (gc, texcoord): a q != 1 was specified (wglDevNonUnityQ) */
    __GLproc    fogPoint;                   /* 0x2760  NULL when fog is off */
    __GLproc    fogColor;                   /* 0x2764 */
    __GLfogVertexProc fogVertex;            /* 0x2768  four variants (linear / exp / exp2 / generic) */
    __GLproc    blend;                      /* 0x276c */
    __GLproc    blendColor;                 /* 0x2770 */
    __GLproc    blendSrc;                   /* 0x2774  one variant per source factor */
    __GLproc    blendDst;                   /* 0x2778  one per destination factor */
    __GLproc    blendEquation;              /* 0x277c  690937d0 = GL_FUNC_ADD (dst += src), set by the blend picker */
    __GLproc    drawPixels;                 /* 0x2780  glDrawPixels */
    __GLproc    copyPixels;                 /* 0x2784  glCopyPixels */
    __GLproc    readPixels;                 /* 0x2788  glReadPixels */
    __GLproc    copyImage;                  /* 0x278c */
    __GLproc    storeTex;                   /* 0x2790 */
    __GLproc    fetchTex;                   /* 0x2794 */
    __GLproc    readImage;                  /* 0x2798 */
    __GLproc    pxStore;                    /* 0x279c */
    GLuint      unk_27a0[6];                /* 0x27a0 */
    __GLproc    store;                      /* 0x27b8  (compared with cfbStore) */
    __GLproc    cfbStore;                   /* 0x27bc  = drawBuffer->store (+0x84) */
    __GLproc    rasterPos2;                 /* 0x27c0  69093c30 */
    __GLproc    rasterPos3;                 /* 0x27c4  69093d90 */
    __GLproc    rasterPos4;                 /* 0x27c8  69093ef0 */
    __GLproc    newArena;                   /* 0x27cc  SI __GLmemoryManagerProcs: 690dd200 */
    __GLproc    deleteArena;                /* 0x27d0  690dd290 */
    __GLproc    arenaAlloc;                 /* 0x27d4  (arena, size) 690dd2e0 */
    __GLproc    freeAll;                    /* 0x27d8  690dd340 */
    GLuint      unk_27dc[3];                /* 0x27dc  (SI: dlist compiler / optimizer here) */
    __GLproc    convertPolygonStipple;      /* 0x27e8  69095c30: state.polygonStipple bytes -> polygonStipple dwords */
    __GLproc    colorTable;                 /* 0x27ec  glColorTableEXT back end (69082df0) */
    __GLproc    colorSubTable;              /* 0x27f0  glColorSubTableEXT back end (69082fb0) */
} __GLprocsICD;

/* ---- attribute stack + vertex machine (0x27f4..0x3440) ------------------------------------- */
typedef struct __GLattributeMachineRec {    /* 0x27f4  (SI: 16 bytes) */
    __GLattribute **stack;                  /* 0x27f4  glPushAttrib stack base */
    __GLclientAttribute **clientStack;      /* 0x27f8  glPushClientAttrib stack base */
    __GLattribute **stackPointer;           /* 0x27fc  GL_ATTRIB_STACK_DEPTH = stackPointer - stack */
    __GLclientAttribute **clientStackPointer;   /* 0x2800  GL_CLIENT_ATTRIB_STACK_DEPTH */
} __GLattributeMachine;

typedef struct __GLvertexMachineICDRec {    /* 0x2804 */
    __GLvertexICD *v0;                      /* 0x2804  the vertex procs advance this by 0xc0 */
    __GLvertexICD *v1;                      /* 0x2808 */
    __GLvertexICD *v2;                      /* 0x280c */
    __GLvertexICD *v3;                      /* 0x2810 */
    __GLvertexICD vbuf[16];                 /* 0x2814 .. 0x3414 */
    __GLvertexICD *provoking;               /* 0x3414 */
    /* ICD additions: recompute the mode flags and needs, return one of the words below */
    __GLproc    getNeeds;                   /* 0x3418  hwGetNeeds -> needs */
    __GLproc    getFaceNeeds0;              /* 0x341c  hwGetFaceNeeds0 -> faceNeeds[0] */
    __GLproc    getFaceNeeds1;              /* 0x3420  hwGetFaceNeeds1 */
    __GLproc    getMaterialNeeds;           /* 0x3424  hwGetMaterialNeeds */
    GLuint      needs;                      /* 0x3428  SI vertex machine order from here */
    GLuint      faceNeeds[2];               /* 0x342c */
    GLuint      materialNeeds;              /* 0x3434 */
} __GLvertexMachineICD;

/* ---- texture machine (part: 0x3530..0x3570) --------------------------------------------------
** glDeleteTextures replaces boundTexture[unit][target] by defaultTexture[target] when the deleted
** name matches (unit count 0x350b8 = TMUs, target count constants 0x944); glGen/Is/DeleteTextures
** use namesArray; textureEnabled[unit] is what the pick procs and the combine setup test. */
/* ---- texture objects (created by gc->texture.createTexture: 690c36d0 generic 0xd4 bytes, 6903b930
** sst 0x574 bytes with this as its head) and their mipmap levels (0x74 bytes each) -------------- */
/* a device texture format (the table g_device->chooseTexFormat 6904d740 returns from: SI __GLtextureFormat
** order with a 3dfx tail) */
typedef struct __GLtextureFormatICDRec {
    GLenum      internalFormat;             /* +0x00 */
    GLuint      unk_04;
    GLint       redSize, greenSize, blueSize, alphaSize;   /* +0x08 */
    GLint       luminanceSize, intensitySize;              /* +0x18 */
    GLint       bitsPerTexel;               /* +0x20 */
    GLenum      pixelFormat;                /* +0x24  the stored texels as a pixel span: format (__glInitTexImageStore
                                                 6909e410 / __glInitTexImageGet 6909e480) */
    GLenum      pixelType;                  /* +0x28  ... type (0 = the sst's direct 8888 -> 565 store, 6905e570) */
    GLint       alignment;                  /* +0x2c  ... row alignment */
    __GLproc    extractTexel;               /* +0x30 */
    __GLproc    extractTexelBorder;         /* +0x34 */
    __GLproc    storeSpan;                  /* +0x38  span modifier: pixels -> texels (__glGenericStoreTexImage 6908a5f0) */
    __GLproc    fetchSpan;                  /* +0x3c  span modifier: texels -> pixels (__glGenericFetchTexImage 6908a6c0) */
    GLuint      unk_40[2];
    GLint       grFormat;                   /* +0x48  GrTextureFormat_t for the Glide GrTexInfo (6903c770) */
} __GLtextureFormatICD;
typedef struct __GLmipMapLevelICDRec {      /* 0x74 (SI __GLmipMapLevel + ICD fields) */
    void       *buffer;                     /* +0x00 */
    void       *pixelBuffer;                /* +0x04  a framebuffer-format copy of the image (690a0dd0 / 690a0b90) */
    void       *compressedImage;            /* +0x08  compressed formats (FXT1 0x86b0 / 0x86b1, S3TC 0x83a1): the image,
                                                 compressedSize bytes (6903c770) */
    GLint       width, height;              /* +0x0c  with the border (GL_TEXTURE_WIDTH / HEIGHT) */
    GLint       depth, imageSize;           /* +0x14 */
    GLint       compressedSize;             /* +0x1c  GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB */
    GLint       width2, height2, depth2;    /* +0x20  without the border */
    __GLfloat   width2f, height2f, depth2f; /* +0x2c */
    GLint       widthLog2, heightLog2, depthLog2;   /* +0x38  (texel address = (row << widthLog2) + col) */
    GLint       border;                     /* +0x44  GL_TEXTURE_BORDER */
    GLint       requestedFormat;            /* +0x48  GL_TEXTURE_COMPONENTS (1 at create) */
    GLenum      baseFormat;                 /* +0x4c  e.g. GL_COLOR_INDEX */
    GLenum      internalFormat;             /* +0x50 */
    const __GLtextureFormatICD *texFormat;  /* +0x54 */
    GLint       redSize, greenSize, blueSize, alphaSize;   /* +0x58  GL_TEXTURE_RED_SIZE .. */
    GLint       luminanceSize, intensitySize;              /* +0x68 */
    __GLproc    extract;                    /* +0x70  texel fetch: (level, gc, 0, row, col, texel[4]) */
} __GLmipMapLevelICD;
/* a texture's colour table: SI __GLcolorTable in the older field order (s_ctable.c, 69082c20) */
typedef struct __GLcolorTableICDRec {       /* 0x34 */
    GLenum      target;                     /* +0x00 */
    GLubyte    *table;                      /* +0x04  the palette entries */
    GLenum      internalFormat;             /* +0x08 */
    GLenum      type;                       /* +0x0c */
    GLsizei     width;                      /* +0x10 */
    GLint       redSize, greenSize, blueSize, alphaSize;   /* +0x14 */
    GLint       luminanceSize, intensitySize;              /* +0x24 */
    GLenum      baseFormat;                 /* +0x2c */
    GLint       components;                 /* +0x30  1..4 (69082c20) */
} __GLcolorTableICD;
typedef struct __GLtextureICDRec {          /* 0xd4 (SI __GLtexture order; the sst object continues as __GLtextureFxICD) */
    GLint       refCount;                   /* +0x00  1 at create */
    struct __GLcontextRec *gc;              /* +0x04 */
    __GLtexObjParamICD params;              /* +0x08 */
    GLuint      name;                       /* +0x2c  (6909c660 returns &name: {name, priority}) */
    __GLfloat   priority;                   /* +0x30  1.0 */
    GLint       numLevels;                  /* +0x34 */
    __GLmipMapLevelICD **level;             /* +0x38  maxMipMapLevel pointers into one 0x74 * n block */
    __GLproc    free;                       /* +0x3c  (gc, tex): 690c3820 frees every level through releaseLevel */
    __GLproc    makeResident;               /* +0x40  (gc, tex, makeResidentArg, unit): 690c38e0; the sst one loads it into a TMU */
    __GLproc    unmakeResident;             /* +0x44  (gc, tex, unit): 690c38f0 */
    __GLfloat   sScale, tScale;             /* +0x48  copied to constants.texSScale / texTScale for the bound unit */
    GLint       dim;                        /* +0x50  1 or 2 */
    GLint       maxLevel;                   /* +0x54  level index clamp (690a4030) */
    __GLfloat   c;                          /* +0x58  minify / magnify switch-over lambda (rho <= c: magnify) */
    __GLcolorTableICD CT;                   /* +0x5c  CT.target = the texture target (GL_TEXTURE_1D / 2D),
                                                 CT.internalFormat GL_RGBA at create, CT.baseFormat (+0x88) = its own
                                                 palette's format */
    GLenum      baseFormat;                 /* +0x90  chooses the texel bytes produced (690a39b0, 690a4110) */
    __GLproc    createLevel;                /* +0x94  (gc, tex, level, internalformat, w, h, border, dim) */
    __GLproc    releaseLevel;               /* +0x98  (gc, tex, level) */
    __GLproc    copyTexImage;               /* +0x9c  (gc, spanInfo, tex, level): glTexImage / glTexSubImage */
    __GLproc    readTexImage;               /* +0xa0  (gc, spanInfo, tex, level): glCopyTexImage / glCopyTexSubImage */
    __GLproc    textureFunc;                /* +0xa4  copied to gc->procs.texture[unit] */
    __GLproc    env;                        /* +0xa8  (gc, color, texel) */
    __GLproc    magnify;                    /* +0xac  (tex, lod, s, t, texel) -- the 3dfx ancestor's order: texel in EDX */
    __GLproc    minify;                     /* +0xb0  (tex, lod, s, t, texel) */
    __GLproc    linear;                     /* +0xb4  (tex, level, s, t, texel): the level filters */
    __GLproc    nearest;                    /* +0xb8 */
    const struct __GLtextureFormatICDRec *texFormat;   /* +0xbc  the device format (g_device->chooseTexFormat) */
    GLuint      unk_c0;                     /* +0xc0 */
    GLint       targetIndex;                /* +0xc4  0..3 (createTexture's third argument) */
    GLuint      makeResidentArg;            /* +0xc8  makeResident's third argument: nothing sets it (0 from the
                                                 calloc) and every makeResident ignores it */
    GLuint      hwHandle[2];                /* +0xcc  per TMU slot (sst: its texture memory handle; one slot when the
                                                 TMUs share memory).  glGetTexParameteriv(GL_TEXTURE_RESIDENT)
                                                 returns &hwHandle[0] (original bug, kept) */
} __GLtextureICD;

/* the sst texture object (createTexture 6903b930: 0x574 bytes): __GLtextureICD and the 3dfx tail.
** Its memory comes from the shared texture memory manager (gc->texMgr) in 0x18-byte heap blocks. */
typedef struct __GLtexMemBlockICDRec {      /* 0x18 */
    struct __GLtextureFxICDRec *tex;        /* +0x00  owner */
    GLuint      start;                      /* +0x04  start address in texture memory */
    LPDIRECTDRAWSURFACE hwSurface;          /* +0x08  windowed: the block's DirectDraw surface, handed with the TMU
                                                 to grSurfaceSetTextureSurfaceExt before a download */
    GLuint      size;                       /* +0x0c */
    GLuint      lastUse;                    /* +0x10  gc->fx.swapCount when last bound */
    struct __GLtexMemBlockICDRec *next;     /* +0x14 */
} __GLtexMemBlockICD;
/* the shared texture memory manager (gc->texMgr; refcounted, shared by wglShareLists).  The software
** path's (690c3640: a copy of the template g_swTexMgrTemplate, icdglob.h) is the 0x0c-byte head (s_texmgr.c
** __GLtextureManager); the sst one (0x28 bytes, 6903b660 = g_device->initTextureManager) continues as __GLtexMgrFxICD */
typedef struct __GLtexMgrICDRec {
    GLint       refs;                       /* +0x00  contexts using it */
    __GLproc    free;                       /* +0x04  (gc): release (690c3680 sw / 6903b740 sst) */
    __GLproc    reset;                      /* +0x08  (gc, mgr): __glInvalidateAllTextures (690c36a0 sw: nothing)
                                                 / 6903b7a0 sst: free and rebuild the heaps */
} __GLtexMgrICD;
typedef struct __GLtexMgrFxICDRec {         /* 0x28 */
    GLint       refs;                       /* +0x00  as __GLtexMgrICD */
    __GLproc    free;                       /* +0x04 */
    __GLproc    reset;                      /* +0x08 */
    LPDIRECTDRAW ddraw;                     /* +0x0c  freed by the last user (wglDDrawFree) */
    struct __GLcontextRec *gc;              /* +0x10 */
    GLuint      end;                        /* +0x14  end of the texture memory */
    __GLtexMemBlockICD *heap[2];            /* +0x18  per TMU slot: the first block */
    __GLtexMemBlockICD *rover[2];           /* +0x20  per TMU slot: the block allocated last (the next search
                                                 starts after it) */
} __GLtexMgrFxICD;

/* g_tmuSharedMemory / __GL_TMU_SLOT (icdglob.h): with TEXUMA every per-TMU slot (the texture's hwHandle[] /
** mem[] / onTmu[], the manager's heap[] / rover[]) is slot 0 */

typedef struct __GLtextureFxICDRec {
    __GLtextureICD t;                       /* +0x000 */
    __GLtexMemBlockICD *mem[2];             /* +0x0d4  per TMU slot */
    GLint       memRequired;                /* +0x0dc  grTexTextureMemRequired */
    GLint       onTmu[2];                   /* +0x0e0  per TMU slot */
    __GLfloat   texScale[2];                /* +0x0e8  {s, t}: Glide's coordinate range (256, or 256 >> aspect on the
                                                 short side; -> hwcx.texScale) */
    GLint       texScaleExp[2];             /* +0x0f0  texScale bits - 0x3f800000 (-> hwcx.texScaleExp) */
    __GLfloat   texScaleInv[2];             /* +0x0f8  1/256 and t / 256 */
    GLint       wideAspect;                 /* +0x100  the aspect was beyond 8:1 (clamped to 3: the image is
                                                 replicated) */
    GLboolean   paletteLoaded;              /* +0x104 */
    GLubyte     pad105[3];
    void       *palette;                    /* +0x108  its own palette */
    GLint       smallLodLog2;               /* +0x10c  GrTexInfo: smallLodLog2 */
    GLint       largeLodLog2;               /* +0x110  .largeLodLog2 */
    GLint       aspectRatioLog2;            /* +0x114  .aspectRatioLog2 */
    GLint       format;                     /* +0x118  .format */
    void       *data;                       /* +0x11c  .data */
    GLuint      raw_120[(0x574 - 0x120) / 4];   /* +0x120 */
} __GLtextureFxICD;

typedef struct __GLtextureMachinePartICDRec {
    void       *boundTexture[2][4];         /* 0x3530  texture objects (name at +0x2c) */
    void       *defaultTexture[4];          /* 0x3550 */
    GLint       currentIndex[2];            /* 0x3560  per unit: the enabled target's index, -1 = none (6907c890) */
    GLboolean   textureEnabled[2];          /* 0x3568  per unit (indexed by unit; [1] gates the TMU1 combine) */
    GLubyte     pad356a[2];
    struct __GLnamesArrayRec *namesArray;   /* 0x356c  glGen / Is / DeleteTextures (glsi.h __GLnamesArray) */
} __GLtextureMachinePartICD;

/* ---- evaluator machine (0x3578..0x39e4): SI __GLevaluatorMachine, exactly (9 maps, order 40) ---- */
typedef struct __GLevaluator1ICDRec { GLint k, order; __GLfloat u1, u2; } __GLevaluator1ICD;            /* 0x10 */
typedef struct __GLevaluator2ICDRec { GLint k, majorOrder, minorOrder; __GLfloat u1, u2, v1, v2; } __GLevaluator2ICD; /* 0x1c */
typedef struct __GLevaluatorMachineICDRec {
    __GLevaluator1ICD eval1[9];             /* 0x3578 */
    __GLevaluator2ICD eval2[9];             /* 0x3608 */
    __GLfloat  *eval1Data[9];               /* 0x3704  glGetMap */
    __GLfloat  *eval2Data[9];               /* 0x3728 */
    __GLfloat   uvalue, vvalue;             /* 0x374c */
    __GLfloat   ucoeff[40], vcoeff[40];     /* 0x3754 */
    __GLfloat   ucoeffDeriv[40], vcoeffDeriv[40]; /* 0x3854 */
    GLint       uorder, vorder;             /* 0x39d4 */
    GLint       utype, vtype;               /* 0x39dc */
} __GLevaluatorMachineICD;

/* ---- transform machine (0x39e4..0x3ab4) ----------------------------------------------------
** __glDoLoadMatrix (apply loaded matrix): GL_MODELVIEW -> [gc+0x39e8], GL_PROJECTION -> [gc+0x39f4]
** and ++[gc+0x39fc], GL_TEXTURE -> [gc+0x3a08 + activeTexture*4]; wglDevCheckViewportInside
** compares the viewport against the clip box 0x3a20..0x3a2c and sets 0x3ab0. */
typedef struct __GLmatrixICDRec {           /* 0x60: SI __GLmatrix (a transform stack entry holds three: matrix,
                                               inverse transpose at +0x60, mvp at +0xc0) */
    __GLfloat   matrix[4][4];               /* +0x00 */
    GLuint      matrixType;                 /* +0x40  __GL_MT_*; >= IS2D: 690825e0 builds matrix2D */
    GLuint      elementClass;               /* +0x44  2 bits per element: 0 zero, 1 general, 3 one; 69047d70 sets
                                                 0x55555555 (all general), 69048c70 reads it; og_buildLazyProc's
                                                 cache key */
    GLushort    width, height;              /* +0x48  the viewport size an IS2DNRSC matrix was made for (SI GLshort) */
    __GLproc    xf2;                        /* +0x4c  (res, v[2], m) transform procs, picked by matrixType */
    __GLproc    xf3;                        /* +0x50  (res, v[3], m) */
    __GLproc    xf4;                        /* +0x54  (res, v[4], m) */
    __GLproc    unk_58[2];                  /* +0x58 */
} __GLmatrixICD;
typedef struct __GLtransformMachineICDRec {
    struct __GLtransformICDRec *modelViewStack;     /* 0x39e4 */
    struct __GLtransformICDRec *modelView;          /* 0x39e8 */
    GLuint      modelViewStackAlloc;                /* 0x39ec  imports.calloc result; the stack is it rounded up to 16 */
    struct __GLtransformICDRec *projectionStack;    /* 0x39f0 */
    struct __GLtransformICDRec *projection;         /* 0x39f4 */
    GLuint      projectionStackAlloc;               /* 0x39f8 */
    GLuint      projectionSequence;                 /* 0x39fc */
    struct __GLtransformICDRec *textureStack[2];    /* 0x3a00 */
    struct __GLtransformICDRec *texture[2];         /* 0x3a08 */
    GLuint      textureStackAlloc[2];               /* 0x3a10 */
    __GLvertexICD *clipTemp;                        /* 0x3a18  (planes * 2 + 12) clip vertices of 0xc0 bytes */
    __GLvertexICD *nextClipTemp;                    /* 0x3a1c */
    GLint       clipX0, clipY0, clipX1, clipY1;     /* 0x3a20  window clip box */
    GLint       minx, miny, maxx, maxy;             /* 0x3a30 */
    __GLfloat   fminx, fminy, fmaxx, fmaxy;         /* 0x3a40 */
    __GLmatrixICD matrix2D;                         /* 0x3a50  mvp * viewport (690825e0), for the 2-D fast paths */
    GLboolean   reasonableViewport;                 /* 0x3ab0  viewport inside the window */
    GLubyte     pad3ab1[3];
} __GLtransformMachineICD;

/* ---- 3dfx hardware context (0x35150..0x38528) ----------------------------------------------
** The Glide state flush (hwFlushGrState) walks two tables: bit i of grDirty marks grState[i] (a
** pointer to the record holding that call's arguments: 0 alpha blend, 1 depth func, 2 depth mode,
** 3 depth mask, 4..7 TMU0 filter/clamp/mipmap/combine, 8..11 TMU1, 12 color combine, 13 alpha
** combine, 14 render buffer, 15 cull, 16 alpha test func, 17 alpha ref, 18 depth bias, 19 color
** mask, 20..22 stencil func/op/mask (through the extension pointers), 23 dither, 24 fog colour,
** 25..27 fog table / mode); bit i of grDirtyExt marks grStateExt[i] (color / alpha / tex combine
** extensions), only when hasCombineExt.  The extension entry points are Glide __stdcall functions
** from grGetProcAddress (the generated code calls them through a __stdcall cast); the slot names
** are confirmed by the name strings hwInitGlideExtensions passes for each store.  Original-driver bug kept
** hwBeginSpritePoints passes all 10 arguments of grColorCombineExt (callee pops 40 bytes). */
/* ten identical 0x244-byte buffer records (0x36604 .. 0x37cac), one per big sst vertex / triangle
** routine (hwBuildImmProc, 69062310, 6905b650, 69047e30, 6902eaa0, 690312c0, 6902ad50, ...): the
** keyed code-page caches (__GLhwBufRecICD below). */
/* grState[] / grDirty bit indexes: the Glide call each record feeds (69029070) */
#define __GL_GR_ALPHA_BLEND         0   /* {src, dst, ...}: grAlphaBlendFunction(Ext) */
#define __GL_GR_DEPTH_FUNC          1
#define __GL_GR_DEPTH_MODE          2
#define __GL_GR_DEPTH_MASK          3
#define __GL_GR_TMU0_FILTER         4   /* {mag, min} grTexFilterMode(0, ...) */
#define __GL_GR_TMU0_CLAMP          5
#define __GL_GR_TMU0_MIPMAP         6
#define __GL_GR_TMU0_COMBINE        7   /* grTexCombine(0, ...) */
#define __GL_GR_TMU1_FILTER         8
#define __GL_GR_TMU1_CLAMP          9
#define __GL_GR_TMU1_MIPMAP        10
#define __GL_GR_TMU1_COMBINE       11
#define __GL_GR_COLOR_COMBINE      12   /* grColorCombine */
#define __GL_GR_ALPHA_COMBINE      13   /* grAlphaCombine */
#define __GL_GR_RENDER_BUFFER      14   /* grRenderBuffer */
#define __GL_GR_CULL               15
#define __GL_GR_ALPHA_TEST_FUNC    16
#define __GL_GR_ALPHA_TEST_REF     17   /* GrAlpha_t */
#define __GL_GR_DEPTH_BIAS         18
#define __GL_GR_COLOR_MASK         19   /* {rgb, ..., alpha at [4]}: grColorMask(Ext) */
#define __GL_GR_STENCIL_FUNC       20   /* through grStencilFuncExt */
#define __GL_GR_STENCIL_OP         21
#define __GL_GR_STENCIL_MASK       22
#define __GL_GR_DITHER             23   /* grDitherMode */
#define __GL_GR_FOG_COLOR          24   /* grFogColorValue */
#define __GL_GR_FOG                25   /* {mode, density, start, end}: the fog table is regenerated */
#define __GL_GR_FOG_MODE           26   /* grFogMode */
#define __GL_GR_FOG_TABLE          27   /* the GrFog_t table (no dirty bit of its own) */
/* grStateExt[] / grDirtyExt bit indexes (the combine extension records, 69029070) */
#define __GL_GRX_COLOR_COMBINE      0   /* grColorCombineExt */
#define __GL_GRX_ALPHA_COMBINE      1   /* grAlphaCombineExt */
#define __GL_GRX_TMU0_COLOR_COMBINE 2   /* grTexColorCombineExt(0, ...) */
#define __GL_GRX_TMU0_ALPHA_COMBINE 3   /* grTexAlphaCombineExt(0, ...) */
#define __GL_GRX_TMU1_COLOR_COMBINE 4
#define __GL_GRX_TMU1_ALPHA_COMBINE 5
#define __GL_GRX_TMU0_CONSTANT      6   /* grConstantColorValueExt(0, ...) */
#define __GL_GRX_TMU1_CONSTANT      7
/* store v in word (GRS_SETB: byte) k of the Glide state record grState[i] / grStateExt[i] and mark it
** dirty when it changes (the ancestor's __GL_GLIDE_SET); the caller's context is `gc` */
#define GRS_SET(i, k, v)                                                     \
    if (gc->hwcx.grState[i][k] != (v)) {                                    \
        gc->hwcx.grState[i][k] = (v);                                       \
        gc->hwcx.grDirty |= 1 << (i);                                       \
    }
#define GRS_SETB(i, k, v)                                                    \
    if (((char *)gc->hwcx.grState[i])[k] != (v)) {                          \
        ((char *)gc->hwcx.grState[i])[k] = (v);                             \
        gc->hwcx.grDirty |= 1 << (i);                                       \
    }
#define GRX_SET(i, k, v)                                                     \
    if (gc->hwcx.grStateExt[i][k] != (v)) {                                 \
        gc->hwcx.grStateExt[i][k] = (v);                                    \
        gc->hwcx.grDirtyExt |= 1 << (i);                                    \
    }
/* Each record is a keyed cache of generated code pages (sstbigpt-style, built by hwPageCacheInit): max entries
** of 0x18 bytes, kept in an LRU list (head = least recently used) and in a pointer array sorted by the key
** bytes (bsearch).  Users call alloc (the lookup) first and on a miss insert the key (hwPageCacheInsert) and
** generate the code into the entry's pages. */
typedef struct __GLhwBufEntryICDRec {       /* 0x18 */
    void       *key;                        /* +0x00  keySize bytes in the data arena */
    GLuint      valA;                       /* +0x04  its page in arena A */
    GLuint      valB;                       /* +0x08  its page in arena B */
    struct __GLhwBufRecICDRec *owner;       /* +0x0c */
    struct __GLhwBufEntryICDRec *prev;      /* +0x10  towards the head (older) */
    struct __GLhwBufEntryICDRec *next;      /* +0x14  towards the tail (newer) */
} __GLhwBufEntryICD;
typedef struct __GLhwBufRecICDRec {
    GLuint      addrA, addrB;               /* +0x00 */
    GLuint      keySize;                    /* +0x08 */
    GLint       count;                      /* +0x0c */
    GLint       max;                        /* +0x10 */
    __GLhwBufEntryICD *entries;             /* +0x14 */
    __GLhwBufEntryICD **sorted;             /* +0x18 */
    void       *arenaA, *arenaB, *data;     /* +0x1c */
    __GLhwBufEntryICD *head;                /* +0x28  least recently used */
    __GLhwBufEntryICD *tail;                /* +0x2c  most recently used */
    __GLproc    alloc;                      /* +0x30  (gc, rec, key, &blk): the lookup, hwPageCacheLookup; returns
                                                 the entry's page in arena A (B through blk), 0 when absent */
    GLuint      raw_34[(0x244 - 0x34) / 4];
} __GLhwBufRecICD;

/* Glide h5's struct cmdTransportInfo (fxglide.h) up to the end of what the COMMAND_TRANSPORT extension
** exposes: grCommandTransportInfoExt2() returns it (hwcx.cmdTransportInfo); the generated triangle code
** writes the FIFO directly through fifoPtr / fifoRoom */
typedef struct GrCmdTransportInfoRec {
    GLuint      triPacketHdr;               /* +0x00 */
    GLuint      cullStripHdr;               /* +0x04 */
    GLuint      paramMask;                  /* +0x08 */
    GLuint     *fifoPtr;                    /* +0x0c */
    GLuint      fifoRead;                   /* +0x10 */
    GLint       fifoRoom;                   /* +0x14 */
    GLint       autoBump;                   /* +0x18 */
    GLuint     *lastBump, *bumpPos;         /* +0x1c */
    GLuint      bumpSize;                   /* +0x24 */
    GLuint    **ptrLostContext;             /* +0x28 */
} GrCmdTransportInfo;

typedef struct __GLhwcxICDRec {
    GLuint      raw_35150[3];               /* 0x35150 */
    GLint       swLines;                    /* 0x3515c  set by the HW line picker when it falls back to software */
    GLint       swPoints;                   /* 0x35160  ditto, point picker */
    GLint       swTriangles;                /* 0x35164  ditto, triangle picker */
    GLint       swRendering;                /* 0x35168  everything in software (ForceSWRendering) */
    GLint       swRequested;                /* 0x3516c  -> swRendering at make-current */
    __GLcolorTableICD sharedPalette;        /* 0x35170  EXT_shared_texture_palette: its table is freed at destroy;
                                                 baseFormat (0x3519c) GL_RGB -> GR_TEXTABLE_PALETTE, else PALETTE_6666 */
    GLboolean   sharedPaletteLoaded;        /* 0x351a4  palette[] is in the hardware */
    GLubyte     pad351a5[3];
    GLuint      palette[256];               /* 0x351a8  the shared palette as grTexDownloadTable data */
    struct __GLtextureFxICDRec *paletteTexture;  /* 0x355a8  the texture whose own palette is in the hardware */
    void       *unitTexture[2];             /* 0x355ac  per-unit current texture cache */
    GLubyte    *scratch;                    /* 0x355b4  grown through imports.realloc */
    GLuint      scratchSize;                /* 0x355b8 */
    GLboolean   nonUnityQ;                  /* 0x355bc  set by wglDevNonUnityQ (21 tests) */
    GLboolean   flag_355bd, flag_355be, flag_355bf;
    GLuint      downloadAddress;            /* 0x355c0  grTexSource / grTexDownloadMipMap address */
    GLushort    gammaRamp[3][256];          /* 0x355c4 */
    GLuint      raw_35bc4[(0x35fc4 - 0x35bc4) / 4];
    char        extString[0x363c8 - 0x35fc4]; /* 0x35fc4  WGL extension string */
    GLint      *grState[28];                /* 0x363c8 */
    GLuint      grDirty;                    /* 0x36438 */
    GLint      *grStateExt[8];              /* 0x3643c */
    GLuint      grDirtyExt;                 /* 0x3645c */
    GLint       numTMU;                     /* 0x36460  grGet(GR_NUM_TMU) */
    const char *extensionStr;               /* 0x36464  grGetString(GR_EXTENSION) */
    GLuint      raw_36468;
    GLint       palette6666;                /* 0x3646c  PALETTE6666: GL_RGBA palettes go to the hardware */
    GrCmdTransportInfo *cmdTransportInfo;   /* 0x36470  from grCommandTransportInfoExt2() (Glide's own FIFO state;
                                               NULL = no direct FIFO writes) */
    const char *glideVersionStr;            /* 0x36474 */
    GLint       glideVerMajor;              /* 0x36478  grGetString(GR_VERSION), "%1d.%1d%1d.%2d.%4d" */
    GLuint      glideVerMinor;              /* 0x3647c  (compared unsigned) */
    GLint       glideVerMinorSub;           /* 0x36480 */
    GLint       glideVerRev;                /* 0x36484 */
    GLuint      glideVerBuild;              /* 0x36488  (compared unsigned) */
    __GLproc    grCommandTransportInfoExt2;     /* 0x3648c  (void) -> struct cmdTransportInfo * */
    __GLproc    grCommandTransportMakeRoomExt2; /* 0x36490  (blockSize, file, line) */
    GLint       hasTexFmtLarge;             /* 0x36494  TEXFMT; 0 -> maxTextureSize 256 / 9 levels */
    GLint       hasTexFmt32;                /* 0x36498  TEXFMT; 32-bit texel formats */
    GLint       hasTexFmtNapalm;            /* 0x3649c  TEXFMT; also gates the guard band (copied to 0x3514c) */
    GLint       hasCombineExt;              /* 0x364a0  gates the grStateExt walk */
    GLint       hasAlphaBlendExt;           /* 0x364a4  grAlphaBlendFunctionExt found */
    GLint       hasPalette6666;             /* 0x364a8  PALETTE6666 */
    GLint       hasTexUMA;                  /* 0x364ac  TEXUMA (grEnable(GR_TEXTURE_UMA_EXT)) */
    GLint       has32bpp;                   /* 0x364b0 */
    GLint       hasCmdTransport;            /* 0x364b4  COMMAND_TRANSPORT */
    GLint       hasSurfaceExt;              /* 0x364b8  SURFACE */
    GLint       hasGetGamma;                /* 0x364bc */
    GLint       hasTBuffer;                 /* 0x364c0 */
    GLint       hasGetRegistry;             /* 0x364c4  GETREGISTRY */
    GLint       hasFogCoord;                /* 0x364c8  FOGCOORD */
    __GLproc    grSstWinOpenExt;            /* 0x364cc */
    __GLproc    grStencilFuncExt;           /* 0x364d0  grState[20] */
    __GLproc    grStencilMaskExt;           /* 0x364d4  grState[22] */
    __GLproc    grStencilOpExt;             /* 0x364d8  grState[21] */
    __GLproc    grBufferClearExt;           /* 0x364dc */
    __GLproc    grColorMaskExt;             /* 0x364e0  grState[19], 4-channel path */
    __GLproc    grAlphaBlendFunctionExt;    /* 0x364e4  (hasAlphaBlendExt 0x364a4) */
    __GLproc    grColorCombineExt;          /* 0x364e8  grStateExt[0] */
    __GLproc    grTexColorCombineExt;       /* 0x364ec  grStateExt[2] */
    __GLproc    grAlphaCombineExt;          /* 0x364f0  grStateExt[1] */
    __GLproc    grTexAlphaCombineExt;       /* 0x364f4  grStateExt[3] */
    __GLproc    grConstantColorValueExt;    /* 0x364f8 */
    __GLproc    grGetGammaTableExt;         /* 0x364fc */
    __GLproc    grTBufferWriteMaskExt;      /* 0x36500 */
    __GLproc    grGetRegistryOrEnvironmentStringExt;   /* 0x36504  (name) -> char * */
    GLuint      glideContext;               /* 0x36508  GrContext_t */
    GLuint      raw_3650c;                  /* 0x3650c */
    GLuint      blendSrcHw;                 /* 0x36510  glBlendFunc: the Glide source factor (6901c840) */
    GLuint      blendDstHw;                 /* 0x36514  the Glide destination factor */
    GLuint      raw_36518[2];               /* 0x36518 */
    GLint       texScaleExp[2][2];          /* 0x36520  per TMU {s, t}: texScale bits - 0x3f800000 (tex obj +0xf0 / +0xf4):
                                               generated code scales by adding this to a float's exponent */
    __GLfloat   texScale[2][2];             /* 0x36530  per TMU {s, t}: Glide's coordinate range, 256 or 256 >> aspect
                                               for the short side (tex obj +0xe8 / +0xec); used by fmul / pfmul */
    GLint       blendEquationHw;            /* 0x36540  glBlendEquationEXT: 0 ADD, 1 SUBTRACT, 2 REVERSE_SUBTRACT */
    GLint       swapInterval;               /* 0x36544 */
    GLint       windowed;                   /* 0x36548  rendering to a Glide surface */
    GLint       resolution;                 /* 0x3654c  GrScreenResolution_t */
    GLuint      glideVerFlags;              /* 0x36550 */
    GLuint      raw_36554;                  /* 0x36554 */
    void       *stagingAlloc;               /* 0x36558  malloc(0x320) for four staging vertices (wglDevCreateContext) */
    __GLvertexICD *stagingBase;             /* 0x3655c  stagingAlloc rounded up to 32 bytes */
    __GLvertexICD *stagingVtx[4];           /* 0x36560  &stagingBase[i] */
    __GLvertexICD *immVertex;               /* 0x36570  the vertex the generated immediate-mode code builds (genCode[5]
                                                 fills its texture +0x20 / clip +0x40) */
    GLuint      hwPrimDesc;                 /* 0x36574  -> 8-byte entry of the primitive table at 690efde8 */
    GLint       hwPrim;                     /* 0x36578  primitive open on the hardware path, -1 = none */
    GLuint      primClass;                  /* 0x3657c  g_immProcClassByMode[prim] of the open glBegin: 0 points, 1 lines,
                                                 2 triangles, 3 quads; -1 at init (the immediate-proc class) */
    __GLproc    ogLazyProcs[16];            /* 0x36580  generated draw procs, compiled on first use: each starts as a
                                               stub (69048e60 ..) that runs the builder 69047e30(gc, n) and calls the
                                               slot again; generated code calls them through the absolute address.
                                               [9 + 3 * unit .. 11 + 3 * unit] (0x365a4, [2][3]): the per-unit
                                               texture-matrix procs */
    GLuint      raw_365c0;                  /* 0x365c0 */
    __GLproc    drawTri;                    /* 0x365c4  generated (bufRec[7] block +4) or 6902ac60 = grDrawTriangle */
    GLuint      genBlock_18;                /* 0x365c8  bufRec[7] block +0x18 */
    __GLproc    drawQuad;                   /* 0x365cc  generated (+8) or 6902ac70 = two grDrawTriangle */
    __GLproc    genProcs9[3];               /* 0x365d0  entry points of the bufRec[9] generated block */
    __GLproc    genProc8;                   /* 0x365dc  bufRec[8] generated entry */
    __GLproc    drawTriPackColor;           /* 0x365e0  generated (+0xc) or 6902abf0: pack each vertex colour, grDrawTriangle */
    GLuint      genBlock_10, genBlock_14;   /* 0x365e4  bufRec[7] block +0x10 / +0x14 */
    GLint       vtxCapacity;                /* 0x365ec  vertices the hardware vertex arrays hold (69019bc0) */
    GLubyte    *vtxBase;                    /* 0x365f0  field-major arrays: field k of every vertex starts at
                                                 vtxBase + 4 * k * vtxCapacity */
    void       *vtxAlloc;                   /* 0x365f4  the unaligned allocation behind vtxBase (69019bc0 regrows it
                                                 to (vtxCapacity + 1) * 32 bytes; destroyContext frees it) */
    void       *alloc_365f8;                /* 0x365f8  imports.malloc */
    GLboolean   depthFlag;                  /* 0x365fc */
    GLubyte     pad365fd[3];
    GLubyte *(__fastcall *buildImmProc)(struct __GLcontextRec *gc, GLuint ncomponents);
                                            /* 0x36600  builds the immediate-mode glVertex{2,3,4}fv routine (69020c90) */
    __GLhwBufRecICD bufRec[10];             /* 0x36604 .. 0x37cac */
    GLuint      raw_37cac[(0x38134 - 0x37cac) / 4];   /* 0x37cac */
    /* generated-code state: routines built by the x86 emitter (og_encode) */
    void       *codeBufRaw;                 /* 0x38134  calloc(0x1000, 2) (hwAllocCodeBuffers) */
    GLuint      codeBufPage;                /* 0x38138  codeBufRaw rounded up to 4 KB */
    void       *codeBuf2Raw;                /* 0x3813c  calloc(0x1000, 1): the second code buffer */
    GLuint      codeBuf2;                   /* 0x38140  codeBuf2Raw rounded up to 32 bytes */
    GLuint      genCode[9];                 /* 0x38144 .. 0x38168: generated routines, __GL_GEN_* below */
    void       *genTexCoord2f;              /* 0x38168  -> currentDispatchState.fn[105] */
    void       *genMultiTexCoord2fARB;      /* 0x3816c  -> fn[371] */
    void       *genColor4f;                 /* 0x38170  -> fn[30] */
    GLuint      genApplyColor;              /* 0x38174  -> procs.applyColor */
    GLuint      raw_38178[(0x38198 - 0x38178) / 4];
    GLubyte     inlineCode[0x40];           /* 0x38198  code stored in the context; its address is fn[130] */
    __GLproc    immProcs[5];                /* 0x381d8  HW hwLazyVertex2fv.. / SW __glim_Vertex2fv.. */
    void       *allocBase;                  /* 0x381ec  the unaligned malloc result the gc was carved from */
    GLuint      texMatrixType[2];           /* 0x381f0  per unit: the texture matrix type the hardware setup saw */
    GLuint      mvpMatrixType;              /* 0x381f8  the modelView mvp matrix type the hardware setup saw */
    __GLfloat   genConst[3];                /* 0x381fc  constants addressed by generated code */
    GLuint      raw_38208[(0x3850c - 0x38208) / 4];
    void       *growBufA;                   /* 0x3850c  grown in 0x1020 steps through imports.realloc */
    GLuint      growBufACap;                /* 0x38510  at least 0x400 */
    void       *growBufB;                   /* 0x38514 */
    GLuint      growBufBCap;                /* 0x38518 */
    GLuint      raw_3851c[(0x38528 - 0x3851c) / 4];
} __GLhwcxICD;
/* hwcx.genCode[] slots: the generated routines (called with `call [slot]` from other generated code) */
#define __GL_GEN_CLIP_EDGE          0   /* clip one edge against a plane (6903cb50 x87 / 6903dc40 SSE / 6903eb40 3DNow) */
#define __GL_GEN_INTERP             1   /* interpolate the new vertex from +0x40 (clip) on */
#define __GL_GEN_INTERP2            2   /* a 2-float interpolator (6903fba0; the 3DNow path's texture s, t too) */
#define __GL_GEN_CLIP_TRIANGLE      3   /* clip and draw a triangle */
#define __GL_GEN_CLIP_QUAD          4   /* a quad as two clipped triangles (69042180) */
#define __GL_GEN_IMM_TRANSFORM      5   /* immediate-mode vertex transform into hwcx.immVertex (690439e0) */
                                        /* [6]: called after a non-identity transform (69020c90) */

typedef struct __GLbufferHeadICDRec {       /* 0x1c: SI __GLbuffer */
    struct __GLcontextRec *gc;              /* +0x00 */
    struct __WGLwinSurfaceRec *drawableBuf; /* +0x04  sst: a 0x14-byte copy of the WGL buffer's surface and
                                                 rectangle (the __WGLwinSurface head: surf, rect only) */
    union {
        struct __WGLbufferRec *wglBuf;      /* +0x08  the WGL drawable buffer (SSTPICK.C buf.drawableBuf), set by
                                                 __glMakeCurrentBuffer */
        GLubyte *other;                     /* unused alias (SI "other") */
    };
    __GLproc    resize;                     /* +0x0c  called from notifyResize */
    __GLproc    makeCurrent;                /* +0x10 */
    __GLproc    loseCurrent;                /* +0x14 */
    __GLproc    freeBuf;                    /* +0x18  (SI: free) */
} __GLbufferHeadICD;
typedef __GLbufferHeadICD __GLbufferICD;     /* 0x1c, same header for every buffer */

/* ---- colour buffers: two 0xc4-byte records with the same access pattern (0x3e18, 0x3edc); a
** gc word at 0x3d38 is set to either address.  Inside still unnamed. */
typedef struct __GLcolorBufferICDRec {      /* 0xc4: SI __GLcolorBuffer + an index-test table, no fbtype */
    __GLbufferHeadICD buf;
    GLint       redMax, greenMax, blueMax;  /* +0x1c */
    GLboolean   needColorFragmentOps;       /* +0x28 */
    GLubyte     pad29[3];
    GLubyte    *alphaTestFuncTable;         /* +0x2c  RGBA alpha test: indexed by alpha * constants.alphaTableConv;
                                                 allocated, freed through imports.free */
    GLubyte    *indexTestFuncTable;         /* +0x30  ICD: the glIndexFuncSGI test table (colour-index mode only) */
    __GLfloat   redScale, greenScale, blueScale;          /* +0x34  the span procs clamp to these */
    GLint       iRedScale, iGreenScale, iBlueScale;       /* +0x40 */
    GLint       redShift, greenShift, blueShift, alphaShift; /* +0x4c */
    __GLfloat   alphaScale;                 /* +0x5c */
    GLint       iAlphaScale;                /* +0x60 */
    __GLfloat   oneOverRedScale, oneOverGreenScale, oneOverBlueScale, oneOverAlphaScale; /* +0x64 */
    GLuint      sourceMask, destMask;       /* +0x74 */
    __GLproc    pick;                       /* +0x7c  called with the gc by the validation */
    __GLproc    getDisplayMasks;            /* +0x80 */
    __GLproc    store;                      /* +0x84 */
    __GLproc    fetch;                      /* +0x88 */
    __GLproc    readColor;                  /* +0x8c */
    __GLproc    readSpan;                   /* +0x90 */
    __GLproc    returnSpan;                 /* +0x94 */
    __GLproc    storeSpan;                  /* +0x98 */
    __GLproc    storeStippledSpan;          /* +0x9c */
    __GLproc    storeLine;                  /* +0xa0 */
    __GLproc    storeStippledLine;          /* +0xa4 */
    __GLproc    fetchSpan;                  /* +0xa8 */
    __GLproc    fetchStippledSpan;          /* +0xac */
    __GLproc    fetchLine;                  /* +0xb0 */
    __GLproc    fetchStippledLine;          /* +0xb4 */
    __GLproc    clear;                      /* +0xb8  glClear */
    __GLproc    clearRect;                  /* +0xbc */
    void       *other2;                     /* +0xc0 */
} __GLcolorBufferICD;

/* ---- stencil / depth / accum buffers (0x3fa4..0x408c): SI layouts after the 0x1c-byte SI
** __GLbuffer header.  Offsets confirmed by the calls: stencil
** testFunc 0x3fdc / fetch 0x3fd8, depth store 0x4024 / fetch 0x4030, and glClear's per-buffer
** clear slots 0x3fec / 0x4028 / 0x4074. */
typedef struct __GLstencilBufferICDRec {    /* 0x4c */
    __GLbufferICD buf;
    GLboolean  *testFuncTable;              /* +0x1c */
    GLubyte    *failOpTable;                /* +0x20 */
    GLubyte    *depthFailOpTable;           /* +0x24 */
    GLubyte    *depthPassOpTable;           /* +0x28 */
    __GLproc    pick;                       /* +0x2c */
    __GLproc    store;                      /* +0x30 */
    __GLproc    fetch;                      /* +0x34 */
    GLboolean (__fastcall *testFunc)();     /* +0x38  (sfb, x, y) -> GLboolean (the callers test al) */
    __GLproc    failOp;                     /* +0x3c */
    __GLproc    passDepthFailOp;            /* +0x40 */
    __GLproc    depthPassOp;                /* +0x44 */
    __GLproc    clear;                      /* +0x48 */
} __GLstencilBufferICD;
typedef struct __GLdepthBufferICDRec {      /* 0x44 */
    __GLbufferICD buf;
    GLuint      writeMask;                  /* +0x1c */
    GLuint      scale;                      /* +0x20 */
    GLenum      testFunc;                   /* +0x24 */
    __GLfloat   minResolution;              /* +0x28 */
    __GLproc    fixZBuffer;                 /* +0x2c */
    __GLproc    pick;                       /* +0x30 */
    GLboolean (__fastcall *store)();        /* +0x34  (zfb, x, y, z) -> GLboolean (the callers test al) */
    __GLproc    clear;                      /* +0x38 */
    __GLproc    store2;                     /* +0x3c */
    __GLproc    fetch;                      /* +0x40 */
} __GLdepthBufferICD;
typedef struct __GLaccumBufferICDRec {      /* 0x58 */
    __GLbufferICD buf;
    __GLfloat   redScale, greenScale, blueScale, alphaScale;                         /* +0x1c */
    __GLfloat   oneOverRedScale, oneOverGreenScale, oneOverBlueScale, oneOverAlphaScale; /* +0x2c */
    __GLproc    pick;                       /* +0x3c */
    __GLproc    clear;                      /* +0x40 */
    __GLproc    accumulate;                 /* +0x44 */
    __GLproc    load;                       /* +0x48 */
    __GLproc    ret;                        /* +0x4c */
    __GLproc    mult;                       /* +0x50 */
    __GLproc    add;                        /* +0x54 */
} __GLaccumBufferICD;

/* ---- 3dfx fields at the end of the core part (0x350b0..0x35150) ------------------------------
** Names from the hand-written wgl code (wglDevMakeCurrent, the swap path, wglext3dfx) and from
** glDeleteTextures / the matrix code (activeTexture indexes transform.texture[]). */
typedef struct __GLfxCoreICDRec {
    GLint       activeTexture;              /* 0x350b0 */
    GLint       clientActiveTexture;        /* 0x350b4  glClientActiveTextureARB: unit & 1 */
    GLint       numTMUs;                    /* 0x350b8  grGet(GR_NUM_TMU) */
    GLint       tmuTexUnit[2];              /* 0x350bc  GL texture unit feeding TMU0 / TMU1 (indexes vertexArray.texture[]) */
    GLint       swapCount;                  /* 0x350c4 */
    GLuint      numSamples;                 /* 0x350c8  multisample (WGL_3DFX_multisample); unsigned: glEnable / glDisable test it with jbe */
    GLuint      flags;                      /* 0x350cc  bit 2 set at make-current */
    GLuint      primMode;                   /* 0x350d0  glBegin mode (69026fd0 and the generated glBegin store it) */
    GLuint      lastPrimMode;               /* 0x350d4  primMode at the last validate; a change sets flags |= 2 */
    GLenum      depthDirection;             /* 0x350d8  GL_LEQUAL, or GL_GEQUAL while the z-precision mode inverts
                                               the depth range (69067db0); with lastDepthFunc gives Glide's function */
    GLenum      lastDepthFunc;              /* 0x350dc  the last glDepthFunc argument (6901cca0), replayed on a switch */
    GLenum      offsetDepthFunc;            /* 0x350e0  lastDepthFunc at glPolygonOffset: decides the offset sign */
    __GLfloat   savedPolygonFactor;         /* 0x350e4  restored into state.polygon.factor */
    __GLfloat   savedPolygonUnits;          /* 0x350e8  restored into state.polygon.units */
    GLint       swapPending;                /* 0x350ec */
    GLint       depthClearPending;          /* 0x350f0  1 at make-current and from glOrtho while depthDirection is
                                                 GL_LEQUAL (z-precision mode): the next clear also resets depth */
    GLint       zPrecision16;               /* 0x350f4  FX_GL_16BPP_ZPRECISION ('0'/'1'/'2', default 1) */
    GLint       zPrecision32;               /* 0x350f8  FX_GL_32BPP_ZPRECISION */
    GLint       fullscreen;                 /* 0x350fc  1 = grSstWinOpen succeeded, 0 = windowed Glide surface;
                                               1 -> texture manager sized from grTexMaxAddress */
    GLint       guardBand;                  /* 0x35100  FX_GL_GUARDBAND_ENABLED (needs hasTexFmtNapalm) */
    __GLfloat   gbLeft;                     /* 0x35104  FX_GL_GBLEFT (also gets GBBOTTOM's default: original bug) */
    __GLfloat   gbTop;                      /* 0x35108  FX_GL_GBTOP */
    __GLfloat   gbRight;                    /* 0x3510c  FX_GL_GBRIGHT */
    __GLfloat   gbBottom;                   /* 0x35110  FX_GL_GBBOTTOM */
    __GLfloat   zcullFar;                   /* 0x35114  FX_GL_ZCULL_FAR (default 1.0) */
    __GLfloat   zcullNear;                  /* 0x35118  FX_GL_ZCULL_NEAR (default 1.0) */
    GLint       isScreenSaver;              /* 0x3511c  module name ends in .SCR */
    GLint       glideOpen;                  /* 0x35120  1 = fullscreen Glide window open */
    GLint       zcullEnable;                /* 0x35124  FX_GL_ZCULL_ENABLE, needs cpuCaps & 6 */
    GLint       superMode;                  /* 0x35128  FX_GL_SUPER (atoi) */
    GLint       objectKey;                  /* 0x3512c  FX_GL_OBJECT_KEY: the lockCount of the object glow worm marks */
    GLint       glowWorm;                   /* 0x35130  FX_GL_GLOW_WORM debug mode: 3 every object, else objectKey
                                                 matches; 4 also dumps c:\\swap%i_%i.txt at swap */
    GLuint      glowColor;                  /* 0x35134  FX_GL_GLOW_COLOR (default -1), read by generated code */
    GLint       force16bppTextures;         /* 0x35138  FX_GL_FORCE_16BPP_TEXTURES */
    GLuint      unk_3513c;
    GLint       hsrLevel;                   /* 0x35140  FX_GL_HSR '1'..'4' -> 4..1 */
    GLint       hsrEnable;                  /* 0x35144  FX_GL_HSR set: grSetNumPendingBuffers(FX_GL_MIN_VERTEX_STRIDE, default 5) */
    GLint       lastHsrMode;                /* 0x35148  -1 at make-current; a change re-picks the vertex procs (6904cfd9) */
    GLint       guardBandOk;                /* 0x3514c  hwcx.hasTexFmtNapalm copy: without it a fullscreen viewport is
                                                 clamped to the desktop size */
} __GLfxCoreICD;

/* ---- polygon shader (0x3bec..0x3d58): SI __GLshade --------------------------------------------
** The span procs loop gc->shader.length times over colors / fbcolors; cfb is set to &frontBuffer or
** &backBuffer.  ICD differences: one colour iterator, a 0x40-byte texture iterator, and a procedure
** where the SI has dfdx's neighbour (0x3d08, called with the gc ~100 times). */
typedef struct __GLcolorIteratorICDRec {
    __GLfloat   rLittle, gLittle, bLittle, aLittle;
    __GLfloat   rBig, gBig, bBig, aBig;
    __GLfloat   drdx, dgdx, dbdx, dadx;
    __GLfloat   drdy, dgdy, dbdy, dady;
} __GLcolorIteratorICD;
typedef struct __GLfragmentICDRec {         /* 0x30: one colour, one texture coordinate set */
    GLint       x, y;                       /* 0x3c2c */
    GLuint      z;                          /* 0x3c34 */
    __GLcolor   color;                      /* 0x3c38 */
    __GLfloat   texture[4];                 /* 0x3c48  s, t, qw, ? */
    __GLfloat   f;                          /* 0x3c58 */
} __GLfragmentICD;
typedef struct __GLshadeTailICDRec {
    GLint       dxLeftLittle, dxLeftBig;    /* 0x3bec  left edge walker */
    GLint       dxLeftFrac;                 /* 0x3bf4 */
    GLint       ixLeft, ixLeftFrac;         /* 0x3bf8 */
    GLint       dxRightLittle, dxRightBig;  /* 0x3c00  right edge walker */
    GLint       dxRightFrac;                /* 0x3c08 */
    GLint       ixRight, ixRightFrac;       /* 0x3c0c */
    __GLfloat   area;                       /* 0x3c14 */
    __GLfloat   dxAC, dxBC, dyAC, dyBC;     /* 0x3c18 */
    GLboolean   ccw;                        /* 0x3c28 */
    GLubyte     pad3c29[3];
    __GLfragmentICD frag;                   /* 0x3c2c */
    GLint       length;                     /* 0x3c5c  span length */
    __GLcolorIteratorICD colorIter;         /* 0x3c60 */
    GLint       zLittle, zBig;              /* 0x3ca0 */
    GLint       dzdx, dzdxBig;              /* 0x3ca8 */
    __GLfloat   dzdyf, dzdxf;               /* 0x3cb0 */
    __GLfloat   texIter[16];                /* 0x3cb8  texture iterator (ICD form, 4 x 4 floats) */
    __GLfloat   fLittle, fBig;              /* 0x3cf8  fog */
    __GLfloat   dfdy, dfdx;                 /* 0x3d00 */
    __GLproc    getModeFlags;               /* 0x3d08  ICD addition: hwGetModeFlags = compute (computeModeFlags), return modeFlags */
    GLuint      modeFlags;                  /* 0x3d0c */
    GLuint     *zbuf;                       /* 0x3d10 */
    GLint       zbufBig, zbufLittle;        /* 0x3d14 */
    GLubyte    *sbuf;                       /* 0x3d1c */
    GLint       sbufBig, sbufLittle;        /* 0x3d20 */
    __GLcolor  *colors;                     /* 0x3d28 */
    __GLcolor  *fbcolors;                   /* 0x3d2c */
    GLuint     *stipplePat;                 /* 0x3d30 */
    GLboolean   done;                       /* 0x3d34 */
    GLubyte     pad3d35[3];
    struct __GLcolorBufferICDRec *cfb;      /* 0x3d38  current colour buffer */
    __GLfloat   offsetZ;                    /* 0x3d3c */
    GLuint      startZLimit;                /* 0x3d40 */
    GLuint      endZLimit;                  /* 0x3d44 */
    __GLfloat   outBoundCount;              /* 0x3d48 */
    __GLfloat   rangeCount;                 /* 0x3d4c */
    __GLfloat   outCountBig;                /* 0x3d50 */
    __GLfloat   outCountLittle;             /* 0x3d54 */
} __GLshadeTailICD;
typedef struct __GLpolygonMachineICDRec {   /* 0x1f4: SI __GLpolygonMachine, exactly */
    GLuint      stipple[32];                /* 0x3b6c  internal stipple words */
    __GLshadeTailICD shader;                /* 0x3bec .. 0x3d58 */
    GLubyte     face[2];                    /* 0x3d58  face from the winding (__GL_CW / __GL_CCW) */
    GLubyte     mode[2];                    /* 0x3d5a  internal polygon mode per face */
    GLubyte     cullFace;                   /* 0x3d5c  passed to shader.getModeFlags */
    GLubyte     pad3d5d[3];
} __GLpolygonMachineICD;

/* ---- line machine (0x3ac4..0x3b6c): SI __GLlineMachine + __GLlineOptions, exactly ------------ */
typedef struct __GLlineMachineICDRec {
    GLint       stipplePosition;            /* 0x3ac4 */
    GLint       repeat;                     /* 0x3ac8 */
    GLboolean   notResetStipple;            /* 0x3acc */
    GLubyte     pad3acd[3];
    GLint       axis, numPixels;            /* 0x3ad0  options */
    __GLfloat   offset, length;             /* 0x3ad8 */
    GLint       xStart, yStart;             /* 0x3ae0 */
    GLint       xLittle, xBig, yLittle, yBig; /* 0x3ae8 */
    GLint       fraction, dfraction;        /* 0x3af8 */
    __GLfloat   curF, curR, curG, curB, curA, curS, curT, curQW; /* 0x3b00 */
    GLuint      curZ;                       /* 0x3b20 */
    __GLfloat   antiAliasPercent;           /* 0x3b24 */
    __GLfloat   f0;                         /* 0x3b28 */
    GLint       width;                      /* 0x3b2c */
    __GLvertexICD *v0, *v1;                 /* 0x3b30 */
    __GLfloat   realLength;                 /* 0x3b38 */
    __GLfloat   dldx, dldy;                 /* 0x3b3c */
    __GLfloat   dddx, dddy;                 /* 0x3b44 */
    __GLfloat   dlLittle, dlBig;            /* 0x3b4c */
    __GLfloat   ddLittle, ddBig;            /* 0x3b54 */
    __GLfloat   plength, pwidth;            /* 0x3b5c */
    __GLfloat   stippleOffset;              /* 0x3b64 */
    __GLfloat   oneOverStippleRepeat;       /* 0x3b68 */
} __GLlineMachineICD;

/* ---- pixel machine tail (0x3d60..0x3dd0): SI __GLpixelMachine from modifyRGBA on, exactly ------
** (no pixelModeFlags word and none of the 1.2 colour table / convolution / histogram state). */
typedef struct __GLpixelMachineICDRec {
    GLboolean   modifyRGBA, modifyCI, modifyDepth, modifyStencil;    /* 0x3d60 */
    GLboolean   fastRGBA;                   /* 0x3d64 */
    GLubyte     pad3d65[3];
    GLfloat     red0Mod, green0Mod, blue0Mod, alpha1Mod;            /* 0x3d68 */
    GLfloat    *redMap, *greenMap, *blueMap, *alphaMap;             /* 0x3d78 */
    GLfloat    *iMap;                       /* 0x3d88 */
    GLvoid     *iCurMap, *redCurMap, *greenCurMap, *blueCurMap, *alphaCurMap; /* 0x3d8c */
    GLboolean   rgbaCurrent;                /* 0x3da0 */
    GLubyte     pad3da1[3];
    GLfloat    *redModMap, *greenModMap, *blueModMap, *alphaModMap; /* 0x3da4 */
    GLboolean   iToICurrent;                /* 0x3db4 */
    GLubyte     pad3db5[3];
    GLfloat    *iToIMap;                    /* 0x3db8 */
    GLboolean   iToRGBACurrent;             /* 0x3dbc */
    GLubyte     pad3dbd[3];
    GLfloat    *iToRMap, *iToGMap, *iToBMap, *iToAMap;              /* 0x3dc0 */
} __GLpixelMachineICD;

/* ---- buffer machine (0x3dd0..0x3e10): SI __GLbufferMachine { doubleStore; __GLlockMachine } ----
** lockBuffers / unlockBuffers are called with renderBufferMask; lockCnt is stepped +-1 around
** every locked region (106 sites); sLockCnt is the byte wgl increments around a share. */
typedef struct __GLbufferMachineICDRec {
    GLboolean   doubleStore;                /* 0x3dd0 */
    GLubyte     pad3dd1[3];
    GLint       lockCnt;                    /* 0x3dd4 */
    __GLbufferICD *bufferToLock;            /* 0x3dd8 */
    GLuint      renderBufferMask;           /* 0x3ddc */
    GLuint      drawBufferMask;             /* 0x3de0 */
    GLuint      readBufferMask;             /* 0x3de4 */
    GLuint      accelBufferMask;            /* 0x3de8 */
    __GLproc    lockBuffers;                /* 0x3dec */
    __GLproc    unlockBuffers;              /* 0x3df0 */
    __GLproc    lockRenderBuffers;          /* 0x3df4 */
    __GLproc    unlockRenderBuffers;        /* 0x3df8 */
    __GLobtainSLockProc obtainSLock;        /* 0x3dfc */
    __GLproc    releaseSLock;               /* 0x3e00 */
    GLboolean   sLockCnt;                   /* 0x3e04 */
    GLubyte     pad3e05[3];
    __GLproc    lockDevice;                 /* 0x3e08 */
    __GLproc    unlockDevice;               /* 0x3e0c */
} __GLbufferMachineICD;

/* ---- light machine (0x3438..0x352c): SI __GLlightMachine order -------------------------------
** sources (0x3504) is the enabled-light list (linked through +0xe8); the lighting code tells the
** faces apart with `face == &gc->light.back`. */
typedef struct __GLmaterialMachineICDRec {   /* 0x64: SI __GLmaterialMachine plus ICD words */
    __GLcolor   sceneColor;                 /* +0x00  emissive + ambient * scene ambient (RGB lighting start) */
    __GLcolor   firstLightMatS;             /* +0x10  SI: the first light's material products */
    __GLcolor   firstLightMatD;             /* +0x20 */
    __GLcolor   firstLightMatA;             /* +0x30 */
    __GLfloat   specularExponent;           /* +0x40  -1 at init: the first use builds the table */
    __GLfloat  *specTable;                  /* +0x44  specular power lookup (indexed by n.h * scale) */
    __GLfloat   threshold, scale;           /* +0x48 */
    void       *cache;                      /* +0x50  the LUT cache entry, released by 690dd5f0 */
    __GLfloat   alpha;                      /* +0x54  copied into the lit vertex's colour alpha */
    __GLfloat   cmapd_minus_cmapa;          /* +0x58  CI lighting: material cmapd - cmapa */
    __GLfloat   cmaps_minus_cmapa;          /* +0x5c  cmaps - cmapa */
    __GLfloat  *indexOffsetPtr;             /* +0x60  CI lighting's index offset: &state.light.<face>.indexOffset
                                                 (glMaterial GL_INDEX_OFFSET), or &state.current.userColorIndex
                                                 for the faces GL_INDEX_MATERIAL_EXT names (69092150) */
} __GLmaterialMachineICD;
/* one light's machine state (0x100 bytes: gc->light.source[] is calloc'd numberOfLights * 0x100, 6907e440;
** the enabled ones are linked through next) */
typedef struct __GLlightSourcePerMaterialMachineICDRec {
    __GLcolor   ambient;                    /* +0x00  light * material products */
    __GLcolor   diffuse;                    /* +0x10 */
    __GLcolor   specular;                   /* +0x20 */
} __GLlightSourcePerMaterialMachineICD;
typedef struct __GLlightSourceICDRec {
    __GLlightSourcePerMaterialMachineICD front, back;   /* +0x00 */
    __GLlightSourceStateICD *state;         /* +0x60 */
    __GLfloat   constantAttenuation;        /* +0x64 */
    __GLfloat   linearAttenuation;          /* +0x68 */
    __GLfloat   quadraticAttenuation;       /* +0x6c */
    __GLfloat   spotLightExponent;          /* +0x70 */
    __GLcoord   position;                   /* +0x74 */
    __GLcoord   direction;                  /* +0x84 */
    __GLfloat   cosCutOffAngle;             /* +0x94 */
    __GLfloat   attenuation;                /* +0x98  1 / constantAttenuation */
    GLboolean   isSpot;                     /* +0x9c */
    GLubyte     pad9d[3];
    __GLcoord   hHat;                       /* +0xa0 */
    __GLcoord   unitVPpli;                  /* +0xb0 */
    __GLcoord   hHatModel;                  /* +0xc0  hHat in model space (__glValidateModelViewForLighting 69092c80) */
    __GLcoord   unitVPpliModel;             /* +0xd0  unitVPpli in model space */
    __GLfloat   sli;                        /* +0xe0 */
    __GLfloat   dli;                        /* +0xe4 */
    struct __GLlightSourceICDRec *next;     /* +0xe8  the enabled-light list (light.sources) */
    __GLfloat  *spotTable;                  /* +0xec */
    __GLfloat   threshold;                  /* +0xf0 */
    __GLfloat   scale;                      /* +0xf4 */
    struct __GLspecLUTEntryRec *cache;      /* +0xf8 */
    GLboolean   slowPath;                   /* +0xfc */
    GLubyte     padfd[3];
} __GLlightSourceMachineICD;
typedef struct __GLlightMachineICDRec {
    __GLlightSourceMachineICD *source;      /* 0x3438  the light source array */
    __GLmaterialMachineICD front;           /* 0x343c */
    __GLmaterialMachineICD back;            /* 0x34a0 */
    __GLlightSourceMachineICD *sources;     /* 0x3504  enabled sources, linked through next (+0xe8) */
    GLint       numEnabledLights;           /* 0x3508 */
    GLint       fastLightMode;              /* 0x350c */
    __GLmaterialState *cm;                  /* 0x3510  colour-material target */
    __GLmaterialMachineICD *cmm;            /* 0x3514 */
    struct __GLspecLUTCacheRec *lutCache;   /* 0x3518  specular lookup cache (glsi.h __GLspecLUTCache) */
    __GLcolor   baseColor;                  /* 0x351c */
} __GLlightMachineICD;

/* ---- optimized-geometry (OG) code generator workspace (0x4110..0x32f30) -----------------------
** og_new appends a 24-byte instruction record at ogInsnPtr; og_reset resets it to
** ogInsn[0]; og_fpk / og_mxk4 keep constant pools {data, count, capacity}.  Original
** behaviour kept: when a pool is full it callocs twice the size, sets count to 0 and neither copies
** nor frees the old buffer. */
typedef struct __GLogInsnICDRec { GLuint w[6]; } __GLogInsnICD;     /* 0x18 */
typedef struct __GLogPoolICDRec { void *data; GLuint count; GLuint capacity; } __GLogPoolICD;
typedef struct __GLogCacheICDRec {          /* 12 bytes: a generated-proc cache (3dfx OGCACHE.H __GLcodeCache;
                                               690e9890 / 690e9946) */
    void       *codeInfo;                   /* count __GLcodeCell (glsitypes.h) */
    void       *codeList;                   /* __GLcodeCell *: the MRU list, linked through cell->next */
    GLubyte    *codeSpace;                  /* count * bytes: the code */
} __GLogCacheICD;
typedef struct __GLogBlockICDRec {          /* 0x70 bytes */
    GLuint      raw[0x6c / 4];
    GLubyte    *buffer;                     /* +0x6c  0xc00 bytes; block 0's is the one allocation for all */
} __GLogBlockICD;

/* ---- the context --------------------------------------------------------------------------- */
typedef struct __GLcontextRec {
    /* imports (0x000, 9 entries) and exports (0x024, 11 entries): see the GC_imports_ / GC_exports_ macros */
    GLuint      imports[9];                 /* 0x000 */
    GLuint      exports[11];                /* 0x024  the SI's eight (destroyContext .. notifyResize, 0x024..0x040)
                                                 and three more procs at 0x044 / 0x048 / 0x04c (6907e6b0 / 6907e6c0 /
                                                 6907e6d0, stored by __glEarlyInitContext 6907e440) */
    GLuint      mcFlags;                    /* 0x050  (the SI's gcState slot) make-current flags (690790xx, wgldev): 1 validated
                                                 once, 2 initialised; 4 clear -> the viewport is taken from the window */
    GLuint      unk_54;                     /* 0x054 */
    __GLattribute state;                    /* 0x058 */
    GLint       beginMode;                  /* 0x8a8 */
    GLenum      renderMode;                 /* 0x8ac */
    GLint       error;                      /* 0x8b0 */
    __GLcontextModesICD modes;              /* 0x8b4 */
    __GLcontextConstantsICD constants;      /* 0x90c */
    __GLfeedbackMachine feedback;           /* 0xe44 */
    __GLselectMachine select;               /* 0xe58 */
    __GLdlistMachine dlist;                 /* 0xe7c */
    __GLvertArrayMachine vertexArray;       /* 0xee4 .. 0x10c0 */
    __GLdispatchTable *dispatchState;       /* 0x10c0  -> currentDispatchState (or the list-compile table) */
    GLuint      unk_10c4;                   /* 0x10c4 */
    __GLdispatchTable listCompState;        /* 0x10c8  copied from __glListCompState (gl_disp.c) */
    GLuint      unk_1730[2];                /* 0x1730 */
    __GLdispatchTable currentDispatchState; /* 0x1738  copied from __glImmedState (gl_disp.c); the gl* stubs jump through here */
    __GLdispatchTable savedDispatchState;   /* 0x1da0 */
    GLuint      validateMask;               /* 0x2408 */
    GLuint      dirtyMask;                  /* 0x240c */
    GLuint      dirtyMaskSaved;             /* 0x2410  (sst: |= dirtyMask, cleared after the flush) */
    struct __GLcolorBufferICDRec *drawBuffer;   /* 0x2414 */
    struct __GLcolorBufferICDRec *readBuffer;   /* 0x2418 */
    __GLprocsICD procs;                     /* 0x241c .. 0x27f4 */
    __GLattributeMachine attributes;        /* 0x27f4 */
    __GLvertexMachineICD vertex;            /* 0x2804 .. 0x3440 */
    __GLlightMachineICD light;              /* 0x3438 .. 0x352c */
    __GLtexMgrICD *texMgr;                  /* 0x352c  the shared texture memory manager (sst: __GLtexMgrFxICD) */
    __GLtextureMachinePartICD texture;      /* 0x3530 */
    GLuint      machines1_raw[1];           /* 0x3570  texture machine tail */
    __GLproc    createTexture;              /* 0x3574  (gc, name, target): a new texture object (6903b930 sst, 690c36d0) */
    __GLevaluatorMachineICD eval;           /* 0x3578 */
    __GLtransformMachineICD transform;      /* 0x39e4 */
    __GLcoord   cullEyeObj;                 /* 0x3ab4  GL_CULL_VERTEX_EXT eye position in object space (690813f0) */
    __GLlineMachineICD line;                /* 0x3ac4 */
    __GLpolygonMachineICD polygon;          /* 0x3b6c .. 0x3d60  SI __GLpolygonMachine */
    __GLpixelMachineICD pixel;              /* 0x3d60 */
    __GLbufferMachineICD buffers;           /* 0x3dd0 */
    struct __GLcolorBufferICDRec *front;    /* 0x3e10  draw buffer for GL_FRONT */
    struct __GLcolorBufferICDRec *back;     /* 0x3e14  draw buffer for GL_BACK */
    __GLcolorBufferICD frontBuffer;         /* 0x3e18 */
    __GLcolorBufferICD backBuffer;          /* 0x3edc */
    struct __GLcolorBufferICDRec *auxBuffer;          /* 0x3fa0 */
    __GLstencilBufferICD stencilBuffer;     /* 0x3fa4 */
    __GLdepthBufferICD depthBuffer;         /* 0x3ff0 */
    __GLaccumBufferICD accumBuffer;         /* 0x4034 */
    struct __WGLdrawableRec *drawablePrivate;         /* 0x408c */
    __GLvertexICD *vaCacheBase;             /* 0x4090  copy of vertexArray.cacheBase (both cache allocators) */
    GLuint      raw_4094[3];           /* 0x4094 */
    GLubyte     raw_40a0;              /* 0x40a0 */
    GLubyte     colorSet;                   /* 0x40a1  1 after every glColor* (no fixed-offset reader) */
    GLubyte     raw_40a2[2];
    GLuint      raw_40a4[(0x4110 - 0x40a4) / 4];   /* 0x40a4 */
    __GLogInsnICD ogInsn[8000];             /* 0x4110 .. 0x32f10 */
    __GLogInsnICD *ogInsnPtr;               /* 0x32f10  next free record */
    GLuint      unk_32f14;                  /* 0x32f14 */
    __GLogPoolICD ogFloatPool;              /* 0x32f18  og_fpk */
    __GLogPoolICD ogPool2;                  /* 0x32f24  og_mxk4 */
    GLuint      raw_32f30[3];               /* 0x32f30 */
    __GLogCacheICD ogCache[5];              /* 0x32f3c  sizes from constants.ogCache*; freed by 690e9a60 */
    void       *ogScratch;                  /* 0x32f78  constants.ogScratchBytes */
    GLuint      raw_32f7c[(0x33f2c - 0x32f7c) / 4];  /* 0x32f7c */
    __GLogBlockICD ogBlocks[0x28];          /* 0x33f2c  zeroed at init */
    __GLogBlockICD *ogBlockPtr;             /* 0x350ac  &ogBlocks[0] at init */
    __GLfxCoreICD fx;                       /* 0x350b0 .. 0x35150 (end of the core part) */
    __GLhwcxICD hwcx;                       /* 0x35150  3dfx part */
} __GLcontext;
/* gc->exports[] slots: the SI's __GLexports procs the ICD fills (__glCoreCreateContext 69078fa0, gl_context.c), then
** three ICD ones (__glEarlyInitContext) */
#define __GL_EXP_DESTROY_CONTEXT    0   /* export_DestroyContext 690792e0 (null_DestroyContext for the null gc) */
#define __GL_EXP_LOSE_CURRENT       1
#define __GL_EXP_MAKE_CURRENT       2   /* export_MakeCurrent 69079370 */
#define __GL_EXP_SHARE_CONTEXT      3
#define __GL_EXP_COPY_CONTEXT       4   /* __glCopyContext */
#define __GL_EXP_SWAP_BUFFERS       5   /* export_SwapBuffers 690792d0: procs.flush */
#define __GL_EXP_NOTIFY_DESTROY     6   /* __glNotifyDestroy 690792c0 */
#define __GL_EXP_NOTIFY_RESIZE      7   /* ChangeDrawableSize 69079180 */
#define __GL_EXP_DISPATCH_EXEC      8   /* __glDispatchExec 6907e6b0 */
#define __GL_EXP_BEGIN_DISPATCH_OVERRIDE 9  /* __glBeginDispatchOverride 6907e6c0 */
#define __GL_EXP_END_DISPATCH_OVERRIDE  10  /* __glEndDispatchOverride 6907e6d0 */

/* ---- compile-time anchors (VC6: a negative array size is an error; its constant folder only
** takes one-level member offsets, so nested anchors are checked inside their own structure) ---- */
#define GLCTX_CHECK(name, cond)  typedef char glctx_check_##name[(cond) ? 1 : -1]
#define GLCTX_FO(T, m)           ((size_t)&((T *)0)->m)
GLCTX_CHECK(size,       sizeof(__GLcontext) == 0x38528);
GLCTX_CHECK(state,      GLCTX_FO(__GLcontext, state) == 0x058);
GLCTX_CHECK(mc_flags,   GLCTX_FO(__GLcontext, mcFlags) == 0x050);
GLCTX_CHECK(begin_mode, GLCTX_FO(__GLcontext, beginMode) == 0x8a8);
GLCTX_CHECK(modes,      GLCTX_FO(__GLcontext, modes) == 0x8b4);
GLCTX_CHECK(constants,  GLCTX_FO(__GLcontext, constants) == 0x90c);
GLCTX_CHECK(feedback,   GLCTX_FO(__GLcontext, feedback) == 0xe44);
GLCTX_CHECK(select,     GLCTX_FO(__GLcontext, select) == 0xe58);
GLCTX_CHECK(dlist,      GLCTX_FO(__GLcontext, dlist) == 0xe7c);
GLCTX_CHECK(vert_array, GLCTX_FO(__GLcontext, vertexArray) == 0xee4);
GLCTX_CHECK(c_lights,   GLCTX_FO(__GLcontextConstantsICD, numberOfLights) == 0x93c - 0x90c);
GLCTX_CHECK(c_maxvp,    GLCTX_FO(__GLcontextConstantsICD, maxViewportWidth) == 0x94c - 0x90c);
GLCTX_CHECK(c_vpadj,    GLCTX_FO(__GLcontextConstantsICD, viewportXAdjust) == 0x954 - 0x90c);
GLCTX_CHECK(c_rescale,  GLCTX_FO(__GLcontextConstantsICD, redRescale) == 0x98c - 0x90c);
GLCTX_CHECK(c_width,    GLCTX_FO(__GLcontextConstantsICD, width) == 0x9b0 - 0x90c);
GLCTX_CHECK(c_names,    GLCTX_FO(__GLcontextConstantsICD, maxNameStackDepth) == 0x9f8 - 0x90c);
GLCTX_CHECK(c_ubyte,    GLCTX_FO(__GLcontextConstantsICD, uByteToFloat) == 0xa08 - 0x90c);
GLCTX_CHECK(c_yinv,     GLCTX_FO(__GLcontextConstantsICD, yInverted) == 0xe08 - 0x90c);
GLCTX_CHECK(sel_ovf,    GLCTX_FO(__GLselectMachine, overFlowed) == 0xe64 - 0xe58);
GLCTX_CHECK(dl_nesting, GLCTX_FO(__GLdlistMachine, nesting) == 0xeb4 - 0xe7c);
GLCTX_CHECK(va_edge,    GLCTX_FO(__GLvertArrayMachine, edgeFlag) == 0xfa4 - 0xee4);
GLCTX_CHECK(va_lock,    GLCTX_FO(__GLvertArrayMachine, flags) == 0x1000 - 0xee4);
GLCTX_CHECK(va_cache,   GLCTX_FO(__GLvertArrayMachine, vertexCache) == 0x1024 - 0xee4);
GLCTX_CHECK(va_draw,    GLCTX_FO(__GLvertArrayMachine, drawArrays) == 0x1044 - 0xee4);
GLCTX_CHECK(va_fill,    GLCTX_FO(__GLvertArrayMachine, fillLockedRange) == 0x1098 - 0xee4);
GLCTX_CHECK(va_size,    sizeof(__GLvertArrayMachine) == 0x10c0 - 0xee4);
GLCTX_CHECK(disp_state, GLCTX_FO(__GLcontext, dispatchState) == 0x10c0);
GLCTX_CHECK(listcomp,   GLCTX_FO(__GLcontext, listCompState) == 0x10c8);
GLCTX_CHECK(dispatch,   GLCTX_FO(__GLcontext, currentDispatchState) == 0x1738);
GLCTX_CHECK(saved_disp, GLCTX_FO(__GLcontext, savedDispatchState) == 0x1da0);
GLCTX_CHECK(valid_mask, GLCTX_FO(__GLcontext, validateMask) == 0x2408);
GLCTX_CHECK(dirty_mask, GLCTX_FO(__GLcontext, dirtyMask) == 0x240c);
GLCTX_CHECK(draw_buffer, GLCTX_FO(__GLcontext, drawBuffer) == 0x2414);
GLCTX_CHECK(procs,      GLCTX_FO(__GLcontext, procs) == 0x241c);
GLCTX_CHECK(attribs,    GLCTX_FO(__GLcontext, attributes) == 0x27f4);
GLCTX_CHECK(vertex_m,   GLCTX_FO(__GLcontext, vertex) == 0x2804);
GLCTX_CHECK(light_m,    GLCTX_FO(__GLcontext, light) == 0x3438);
GLCTX_CHECK(lm_back,    GLCTX_FO(__GLlightMachineICD, back) == 0x34a0 - 0x3438);
GLCTX_CHECK(mm_size,    sizeof(__GLmaterialMachineICD) == 0x64);
GLCTX_CHECK(tex_size,   sizeof(__GLtextureICD) == 0xd4);
GLCTX_CHECK(tex_name,   GLCTX_FO(__GLtextureICD, name) == 0x2c);
GLCTX_CHECK(tex_level,  GLCTX_FO(__GLtextureICD, level) == 0x38);
GLCTX_CHECK(tex_func,   GLCTX_FO(__GLtextureICD, textureFunc) == 0xa4);
GLCTX_CHECK(tex_c,      GLCTX_FO(__GLtextureICD, c) == 0x58);
GLCTX_CHECK(tex_ctfmt,  GLCTX_FO(__GLtextureICD, CT) + GLCTX_FO(__GLcolorTableICD, baseFormat) == 0x88);
GLCTX_CHECK(tex_mag,    GLCTX_FO(__GLtextureICD, magnify) == 0xac);
GLCTX_CHECK(tex_fmt,    GLCTX_FO(__GLtextureICD, texFormat) == 0xbc);
GLCTX_CHECK(tex_hw,     GLCTX_FO(__GLtextureICD, hwHandle) == 0xcc);
GLCTX_CHECK(texfx_mem,  GLCTX_FO(__GLtextureFxICD, mem) == 0xd4);
GLCTX_CHECK(texfx_sc,   GLCTX_FO(__GLtextureFxICD, texScale) == 0xe8);
GLCTX_CHECK(texfx_info, GLCTX_FO(__GLtextureFxICD, smallLodLog2) == 0x10c);
GLCTX_CHECK(texfx_size, sizeof(__GLtextureFxICD) == 0x574);
GLCTX_CHECK(texenv,     sizeof(__GLtextureEnvState) == 0x54);
GLCTX_CHECK(texparam,   sizeof(__GLtexParamICD) == 0x2c);
GLCTX_CHECK(ctable,     sizeof(__GLcolorTableICD) == 0x34);
GLCTX_CHECK(lss_size,   sizeof(__GLlightSourceStateICD) == 0x74);
GLCTX_CHECK(lsm_next,   GLCTX_FO(__GLlightSourceMachineICD, next) == 0xe8);
GLCTX_CHECK(lsm_size,   sizeof(__GLlightSourceMachineICD) == 0x100);
GLCTX_CHECK(mip_size,   sizeof(__GLmipMapLevelICD) == 0x74);
GLCTX_CHECK(mip_ext,    GLCTX_FO(__GLmipMapLevelICD, extract) == 0x70);
GLCTX_CHECK(mip_wlog2,  GLCTX_FO(__GLmipMapLevelICD, widthLog2) == 0x38);
GLCTX_CHECK(mip_border, GLCTX_FO(__GLmipMapLevelICD, border) == 0x44);
GLCTX_CHECK(mat_wh,     GLCTX_FO(__GLmatrixICD, width) == 0x48);
GLCTX_CHECK(mat_xf4,    GLCTX_FO(__GLmatrixICD, xf4) == 0x54);
GLCTX_CHECK(mat_size,   sizeof(__GLmatrixICD) == 0x60);
GLCTX_CHECK(lm_sources, GLCTX_FO(__GLlightMachineICD, sources) == 0x3504 - 0x3438);
GLCTX_CHECK(lm_size,    sizeof(__GLlightMachineICD) == 0x352c - 0x3438);
GLCTX_CHECK(texture_m,  GLCTX_FO(__GLcontext, texture) == 0x3530);
GLCTX_CHECK(tm_names,   GLCTX_FO(__GLtextureMachinePartICD, namesArray) == 0x356c - 0x3530);
GLCTX_CHECK(eval_m,     GLCTX_FO(__GLcontext, eval) == 0x3578);
GLCTX_CHECK(ev_data,    GLCTX_FO(__GLevaluatorMachineICD, eval1Data) == 0x3704 - 0x3578);
GLCTX_CHECK(ev_size,    sizeof(__GLevaluatorMachineICD) == 0x39e4 - 0x3578);
GLCTX_CHECK(transform_m, GLCTX_FO(__GLcontext, transform) == 0x39e4);
GLCTX_CHECK(line_m,     GLCTX_FO(__GLcontext, line) == 0x3ac4);
GLCTX_CHECK(ln_xstart,  GLCTX_FO(__GLlineMachineICD, xStart) == 0x3ae0 - 0x3ac4);
GLCTX_CHECK(ln_v0,      GLCTX_FO(__GLlineMachineICD, v0) == 0x3b30 - 0x3ac4);
GLCTX_CHECK(ln_size,    sizeof(__GLlineMachineICD) == 0x3b6c - 0x3ac4);
GLCTX_CHECK(polygon,    GLCTX_FO(__GLcontext, polygon) == 0x3b6c);
GLCTX_CHECK(shader,     GLCTX_FO(__GLpolygonMachineICD, shader) == 0x3bec - 0x3b6c);
GLCTX_CHECK(sh_frag,    GLCTX_FO(__GLshadeTailICD, frag) == 0x3c2c - 0x3bec);
GLCTX_CHECK(sh_len,     GLCTX_FO(__GLshadeTailICD, length) == 0x3c5c - 0x3bec);
GLCTX_CHECK(sh_z,       GLCTX_FO(__GLshadeTailICD, zLittle) == 0x3ca0 - 0x3bec);
GLCTX_CHECK(pr_line,    GLCTX_FO(__GLprocsICD, line) == 0x2630 - 0x241c);
GLCTX_CHECK(pr_pixel,   GLCTX_FO(__GLprocsICD, pixel) == 0x26d0 - 0x241c);
GLCTX_CHECK(pr_point,   GLCTX_FO(__GLprocsICD, renderPoint) == 0x2728 - 0x241c);
GLCTX_CHECK(pr_bitmap,  GLCTX_FO(__GLprocsICD, bitmap) == 0x2734 - 0x241c);
GLCTX_CHECK(pr_blend,   GLCTX_FO(__GLprocsICD, blend) == 0x276c - 0x241c);
GLCTX_CHECK(pr_store,   GLCTX_FO(__GLprocsICD, store) == 0x27b8 - 0x241c);
GLCTX_CHECK(pr_arena,   GLCTX_FO(__GLprocsICD, newArena) == 0x27cc - 0x241c);
GLCTX_CHECK(pr_mat,     GLCTX_FO(__GLprocsICD, matValidate) == 0x2550 - 0x241c);
GLCTX_CHECK(pr_draw,    GLCTX_FO(__GLprocsICD, drawPixels) == 0x2780 - 0x241c);
GLCTX_CHECK(sh_flags,   GLCTX_FO(__GLshadeTailICD, modeFlags) == 0x3d0c - 0x3bec);
GLCTX_CHECK(sh_cfb,     GLCTX_FO(__GLshadeTailICD, cfb) == 0x3d38 - 0x3bec);
GLCTX_CHECK(sh_size,    sizeof(__GLshadeTailICD) == 0x3d58 - 0x3bec);
GLCTX_CHECK(cullface,   GLCTX_FO(__GLpolygonMachineICD, cullFace) == 0x3d5c - 0x3b6c);
GLCTX_CHECK(pixel_m,    GLCTX_FO(__GLcontext, pixel) == 0x3d60);
GLCTX_CHECK(pm_itoi,    GLCTX_FO(__GLpixelMachineICD, iToICurrent) == 0x3db4 - 0x3d60);
GLCTX_CHECK(pm_size,    sizeof(__GLpixelMachineICD) == 0x3dd0 - 0x3d60);
GLCTX_CHECK(buffers_m,  GLCTX_FO(__GLcontext, buffers) == 0x3dd0);
GLCTX_CHECK(bm_size,    sizeof(__GLbufferMachineICD) == 0x40);
GLCTX_CHECK(front,      GLCTX_FO(__GLcontext, front) == 0x3e10);
GLCTX_CHECK(drawpriv,   GLCTX_FO(__GLcontext, drawablePrivate) == 0x408c);
GLCTX_CHECK(frontbuf,   GLCTX_FO(__GLcontext, frontBuffer) == 0x3e18);
GLCTX_CHECK(backbuf,    GLCTX_FO(__GLcontext, backBuffer) == 0x3edc);
GLCTX_CHECK(cb_scale,   GLCTX_FO(__GLcolorBufferICD, redScale) == 0x34);
GLCTX_CHECK(cb_pick,    GLCTX_FO(__GLcolorBufferICD, pick) == 0x7c);
GLCTX_CHECK(cb_clear,   GLCTX_FO(__GLcolorBufferICD, clear) == 0xb8);
GLCTX_CHECK(cb_size,    sizeof(__GLcolorBufferICD) == 0xc4);
GLCTX_CHECK(stencilbuf, GLCTX_FO(__GLcontext, stencilBuffer) == 0x3fa4);
GLCTX_CHECK(depthbuf,   GLCTX_FO(__GLcontext, depthBuffer) == 0x3ff0);
GLCTX_CHECK(accumbuf,   GLCTX_FO(__GLcontext, accumBuffer) == 0x4034);
GLCTX_CHECK(sb_size,    sizeof(__GLstencilBufferICD) == 0x4c);
GLCTX_CHECK(db_size,    sizeof(__GLdepthBufferICD) == 0x44);
GLCTX_CHECK(ab_size,    sizeof(__GLaccumBufferICD) == 0x58);
GLCTX_CHECK(tm_proj,    GLCTX_FO(__GLtransformMachineICD, projection) == 0x39f4 - 0x39e4);
GLCTX_CHECK(tm_tex,     GLCTX_FO(__GLtransformMachineICD, texture) == 0x3a08 - 0x39e4);
GLCTX_CHECK(tm_clip,    GLCTX_FO(__GLtransformMachineICD, clipX0) == 0x3a20 - 0x39e4);
GLCTX_CHECK(tm_rv,      GLCTX_FO(__GLtransformMachineICD, reasonableViewport) == 0x3ab0 - 0x39e4);
GLCTX_CHECK(vtx_size,   sizeof(__GLvertexICD) == 0xc0);
GLCTX_CHECK(vtx_clip,   GLCTX_FO(__GLvertexICD, clip) == 0x40);
GLCTX_CHECK(vtx_colors, GLCTX_FO(__GLvertexICD, colors) == 0x60);
GLCTX_CHECK(vtx_obj,    GLCTX_FO(__GLvertexICD, obj) == 0x80);
GLCTX_CHECK(rasterpos,  GLCTX_FO(__GLcurrentState, rasterPos) == 0x60 - 0x60);
GLCTX_CHECK(vm_vbuf,    GLCTX_FO(__GLvertexMachineICD, vbuf) == 0x2814 - 0x2804);
GLCTX_CHECK(vm_prov,    GLCTX_FO(__GLvertexMachineICD, provoking) == 0x3414 - 0x2804);
GLCTX_CHECK(p_validate, GLCTX_FO(__GLprocsICD, validate) == 0x242c - 0x241c);
GLCTX_CHECK(p_pickpt,   GLCTX_FO(__GLprocsICD, pickPointProcs) == 0x244c - 0x241c);
GLCTX_CHECK(p_picktri,  GLCTX_FO(__GLprocsICD, pickTriangleProcs) == 0x2454 - 0x241c);
GLCTX_CHECK(p_pickva,   GLCTX_FO(__GLprocsICD, pickVertexArrayProcs) == 0x2478 - 0x241c);
GLCTX_CHECK(p_pickmat,  GLCTX_FO(__GLprocsICD, pickMatrixProcs) == 0x247c - 0x241c);
GLCTX_CHECK(p_matrix,   GLCTX_FO(__GLprocsICD, matrix) == 0x2494 - 0x241c);
GLCTX_CHECK(p_pushmat,  GLCTX_FO(__GLprocsICD, pushMatrix) == 0x24a4 - 0x241c);
GLCTX_CHECK(p_scissor,  GLCTX_FO(__GLprocsICD, applyScissor) == 0x24b8 - 0x241c);
GLCTX_CHECK(p_clip2,    GLCTX_FO(__GLprocsICD, clipCheck2) == 0x24c8 - 0x241c);
GLCTX_CHECK(p_vertex,   GLCTX_FO(__GLprocsICD, vertex) == 0x24d4 - 0x241c);
GLCTX_CHECK(p_valvtx,   GLCTX_FO(__GLprocsICD, validateVertex) == 0x250c - 0x241c);
GLCTX_CHECK(p_ec1,      GLCTX_FO(__GLprocsICD, ec1) == 0x251c - 0x241c);
GLCTX_CHECK(p_applycol, GLCTX_FO(__GLprocsICD, applyColor) == 0x2530 - 0x241c);
GLCTX_CHECK(p_colmat,   GLCTX_FO(__GLprocsICD, colorMaterial) == 0x2548 - 0x241c);
GLCTX_CHECK(p_packvc,   GLCTX_FO(__GLprocsICD, packVertexColor) == 0x2540 - 0x241c);
GLCTX_CHECK(p_begin,    GLCTX_FO(__GLprocsICD, beginPrim) == 0x2558 - 0x241c);
GLCTX_CHECK(p_end,      GLCTX_FO(__GLprocsICD, endPrim) == 0x2580 - 0x241c);
GLCTX_CHECK(p_cliptri,  GLCTX_FO(__GLprocsICD, clipTriangle) == 0x259c - 0x241c);
GLCTX_CHECK(p_rect,     GLCTX_FO(__GLprocsICD, rect) == 0x25a8 - 0x241c);
GLCTX_CHECK(p_span,     GLCTX_FO(__GLprocsICD, span) == 0x25ac - 0x241c);
GLCTX_CHECK(p_bitmap,   GLCTX_FO(__GLprocsICD, bitmap) == 0x2734 - 0x241c);
GLCTX_CHECK(p_drawpix,  GLCTX_FO(__GLprocsICD, drawPixels) == 0x2780 - 0x241c);
GLCTX_CHECK(p_pxstore,  GLCTX_FO(__GLprocsICD, pxStore) == 0x279c - 0x241c);
GLCTX_CHECK(p_size,     sizeof(__GLprocsICD) == 0x27f4 - 0x241c);
GLCTX_CHECK(colorset,   GLCTX_FO(__GLcontext, colorSet) == 0x40a1);
GLCTX_CHECK(og_insn,    GLCTX_FO(__GLcontext, ogInsn) == 0x4110);
GLCTX_CHECK(og_ptr,     GLCTX_FO(__GLcontext, ogInsnPtr) == 0x32f10);
GLCTX_CHECK(og_pool2,   GLCTX_FO(__GLcontext, ogPool2) == 0x32f24);
GLCTX_CHECK(fxcore,     GLCTX_FO(__GLcontext, fx) == 0x350b0);
GLCTX_CHECK(fx_tmus,    GLCTX_FO(__GLfxCoreICD, numTMUs) == 0x350b8 - 0x350b0);
GLCTX_CHECK(fx_swpend,  GLCTX_FO(__GLfxCoreICD, swapPending) == 0x350ec - 0x350b0);
GLCTX_CHECK(fx_scr,     GLCTX_FO(__GLfxCoreICD, isScreenSaver) == 0x3511c - 0x350b0);
GLCTX_CHECK(fx_swmode,  GLCTX_FO(__GLfxCoreICD, glowWorm) == 0x35130 - 0x350b0);
GLCTX_CHECK(fx_gbok,    GLCTX_FO(__GLfxCoreICD, guardBandOk) == 0x3514c - 0x350b0);
GLCTX_CHECK(fx_poly,    GLCTX_FO(__GLfxCoreICD, savedPolygonFactor) == 0x350e4 - 0x350b0);
GLCTX_CHECK(fx_full,    GLCTX_FO(__GLfxCoreICD, fullscreen) == 0x350fc - 0x350b0);
GLCTX_CHECK(fx_zcull,   GLCTX_FO(__GLfxCoreICD, zcullEnable) == 0x35124 - 0x350b0);
GLCTX_CHECK(hwcx,       GLCTX_FO(__GLcontext, hwcx) == 0x35150);
GLCTX_CHECK(hw_state,   GLCTX_FO(__GLhwcxICD, grState) == 0x363c8 - 0x35150);
GLCTX_CHECK(hw_swlines, GLCTX_FO(__GLhwcxICD, swLines) == 0x3515c - 0x35150);
GLCTX_CHECK(hw_pal,     GLCTX_FO(__GLhwcxICD, palette) == 0x351a8 - 0x35150);
GLCTX_CHECK(hw_shpal,   GLCTX_FO(__GLhwcxICD, sharedPalette) == 0x35170 - 0x35150);
GLCTX_CHECK(hw_nuq,     GLCTX_FO(__GLhwcxICD, nonUnityQ) == 0x355bc - 0x35150);
GLCTX_CHECK(hw_gamma2,  GLCTX_FO(__GLhwcxICD, gammaRamp) == 0x355c4 - 0x35150);
GLCTX_CHECK(hw_extstr,  GLCTX_FO(__GLhwcxICD, extString) == 0x35fc4 - 0x35150);
GLCTX_CHECK(hw_dirty,   GLCTX_FO(__GLhwcxICD, grDirty) == 0x36438 - 0x35150);
GLCTX_CHECK(hw_dirty2,  GLCTX_FO(__GLhwcxICD, grDirtyExt) == 0x3645c - 0x35150);
GLCTX_CHECK(hw_ext,     GLCTX_FO(__GLhwcxICD, cmdTransportInfo) == 0x36470 - 0x35150);
GLCTX_CHECK(hw_tmu,     GLCTX_FO(__GLhwcxICD, numTMU) == 0x36460 - 0x35150);
GLCTX_CHECK(hw_ver,     GLCTX_FO(__GLhwcxICD, glideVerMajor) == 0x36478 - 0x35150);
GLCTX_CHECK(hw_fogc,    GLCTX_FO(__GLhwcxICD, hasFogCoord) == 0x364c8 - 0x35150);
GLCTX_CHECK(hw_reg,     GLCTX_FO(__GLhwcxICD, grGetRegistryOrEnvironmentStringExt) == 0x36504 - 0x35150);
GLCTX_CHECK(hw_comb,    GLCTX_FO(__GLhwcxICD, hasCombineExt) == 0x364a0 - 0x35150);
GLCTX_CHECK(hw_32bpp,   GLCTX_FO(__GLhwcxICD, has32bpp) == 0x364b0 - 0x35150);
GLCTX_CHECK(hw_winopen, GLCTX_FO(__GLhwcxICD, grSstWinOpenExt) == 0x364cc - 0x35150);
GLCTX_CHECK(hw_gamma,   GLCTX_FO(__GLhwcxICD, grGetGammaTableExt) == 0x364fc - 0x35150);
GLCTX_CHECK(hw_ctx,     GLCTX_FO(__GLhwcxICD, glideContext) == 0x36508 - 0x35150);
GLCTX_CHECK(hw_texsc,   GLCTX_FO(__GLhwcxICD, texScale) == 0x36530 - 0x35150);
GLCTX_CHECK(hw_lazy,    GLCTX_FO(__GLhwcxICD, ogLazyProcs) == 0x36580 - 0x35150);
GLCTX_CHECK(hw_drawtri, GLCTX_FO(__GLhwcxICD, drawTriPackColor) == 0x365e0 - 0x35150);
GLCTX_CHECK(hw_res,     GLCTX_FO(__GLhwcxICD, resolution) == 0x3654c - 0x35150);
GLCTX_CHECK(hw_vtx,     GLCTX_FO(__GLhwcxICD, vtxBase) == 0x365f0 - 0x35150);
GLCTX_CHECK(hw_depthf,  GLCTX_FO(__GLhwcxICD, depthFlag) == 0x365fc - 0x35150);
GLCTX_CHECK(hw_bufrec,  GLCTX_FO(__GLhwcxICD, bufRec) == 0x36604 - 0x35150);
GLCTX_CHECK(hw_brsize,  sizeof(__GLhwBufRecICD) == 0x244);
GLCTX_CHECK(hw_gencol,  GLCTX_FO(__GLhwcxICD, genColor4f) == 0x38170 - 0x35150);
GLCTX_CHECK(hw_inline,  GLCTX_FO(__GLhwcxICD, inlineCode) == 0x38198 - 0x35150);
GLCTX_CHECK(hw_alloc,   GLCTX_FO(__GLhwcxICD, allocBase) == 0x381ec - 0x35150);
GLCTX_CHECK(hw_growa,   GLCTX_FO(__GLhwcxICD, growBufA) == 0x3850c - 0x35150);
GLCTX_CHECK(hw_size,    sizeof(__GLhwcxICD) == 0x38528 - 0x35150);
/* gc->state (base 0x058) */
GLCTX_CHECK(current,    GLCTX_FO(__GLattribute, current) == 0x060 - 0x58);
GLCTX_CHECK(point,      GLCTX_FO(__GLattribute, point) == 0x1a8 - 0x58);
GLCTX_CHECK(line,       GLCTX_FO(__GLattribute, line) == 0x1cc - 0x58);
GLCTX_CHECK(polygon,    GLCTX_FO(__GLattribute, polygon) == 0x1dc - 0x58);
GLCTX_CHECK(stipple,    GLCTX_FO(__GLattribute, polygonStipple) == 0x1f4 - 0x58);
GLCTX_CHECK(pixel,      GLCTX_FO(__GLattribute, pixel) == 0x274 - 0x58);
GLCTX_CHECK(light,      GLCTX_FO(__GLattribute, light) == 0x35c - 0x58);
GLCTX_CHECK(fog,        GLCTX_FO(__GLattribute, fog) == 0x438 - 0x58);
GLCTX_CHECK(depth,      GLCTX_FO(__GLattribute, depth) == 0x478 - 0x58);
GLCTX_CHECK(accum,      GLCTX_FO(__GLattribute, accum) == 0x488 - 0x58);
GLCTX_CHECK(stencil,    GLCTX_FO(__GLattribute, stencil) == 0x498 - 0x58);
GLCTX_CHECK(viewport,   GLCTX_FO(__GLattribute, viewport) == 0x4b0 - 0x58);
GLCTX_CHECK(transform,  GLCTX_FO(__GLattribute, transform) == 0x4f0 - 0x58);
GLCTX_CHECK(enables,    GLCTX_FO(__GLattribute, enables) == 0x51c - 0x58);
GLCTX_CHECK(raster,     GLCTX_FO(__GLattribute, raster) == 0x534 - 0x58);
GLCTX_CHECK(hints,      GLCTX_FO(__GLattribute, hints) == 0x578 - 0x58);
GLCTX_CHECK(evaluator,  GLCTX_FO(__GLattribute, evaluator) == 0x598 - 0x58);
GLCTX_CHECK(list,       GLCTX_FO(__GLattribute, list) == 0x5c8 - 0x58);
GLCTX_CHECK(texture,    GLCTX_FO(__GLattribute, texture) == 0x5cc - 0x58);
GLCTX_CHECK(scissor,    GLCTX_FO(__GLattribute, scissor) == 0x894 - 0x58);
GLCTX_CHECK(state_size, sizeof(__GLattribute) == 0x8a8 - 0x58);
GLCTX_CHECK(cur_size,   sizeof(__GLcurrentState) == 0x1a8 - 0x60);
/* inside the sub-structures */
GLCTX_CHECK(valid_rp,   GLCTX_FO(__GLcurrentState, validRasterPos) == 0x120 - 0x60);
GLCTX_CHECK(user_color, GLCTX_FO(__GLcurrentState, userColor) == 0x130 - 0x60);
GLCTX_CHECK(color,      GLCTX_FO(__GLcurrentState, color) == 0x150 - 0x60);
GLCTX_CHECK(normal,     GLCTX_FO(__GLcurrentState, normal) == 0x164 - 0x60);
GLCTX_CHECK(texture0,   GLCTX_FO(__GLcurrentState, texture) == 0x180 - 0x60);
GLCTX_CHECK(act_tex,    GLCTX_FO(__GLcurrentState, activeTexture) == 0x1a0 - 0x60);
GLCTX_CHECK(tex_unit,   sizeof(__GLtextureUnitStateICD) == 0x164);
GLCTX_CHECK(span_m,     GLCTX_FO(__GLspanProcsICD, m) == 0x2628 - 0x25ac);
GLCTX_CHECK(pixel_dsc,  GLCTX_FO(__GLpixelTransferMode, depthScale) == 0x284 - 0x274);
GLCTX_CHECK(pixel_bias, GLCTX_FO(__GLpixelTransferMode, bias) == 0x288 - 0x274);
GLCTX_CHECK(pixel_zoom, GLCTX_FO(__GLpixelTransferMode, zoomX) == 0x29c - 0x274);
GLCTX_CHECK(pixel_pack, GLCTX_FO(__GLpixelState, packModes) == 0x328 - 0x274);
GLCTX_CHECK(read_buf,   GLCTX_FO(__GLpixelState, readBuffer) == 0x354 - 0x274);
GLCTX_CHECK(shade,      GLCTX_FO(__GLlightState, shadingModel) == 0x36c - 0x35c);
GLCTX_CHECK(front_mat,  GLCTX_FO(__GLlightState, front) == 0x384 - 0x35c);
GLCTX_CHECK(back_mat,   GLCTX_FO(__GLlightState, back) == 0x3dc - 0x35c);
GLCTX_CHECK(light_src,  GLCTX_FO(__GLlightState, source) == 0x434 - 0x35c);
GLCTX_CHECK(depth_clr,  GLCTX_FO(__GLdepthState, clear) == 0x480 - 0x478);
GLCTX_CHECK(st_fail,    GLCTX_FO(__GLstencilState, fail) == 0x4a4 - 0x498);
GLCTX_CHECK(vp_znear,   GLCTX_FO(__GLviewport, zNear) == 0x4c0 - 0x4b0);
GLCTX_CHECK(eval1,      GLCTX_FO(__GLenableState, eval1) == 0x530 - 0x51c);
GLCTX_CHECK(logic_op,   GLCTX_FO(__GLrasterState, logicOp) == 0x550 - 0x534);
GLCTX_CHECK(draw_buf,   GLCTX_FO(__GLrasterState, drawBuffer) == 0x570 - 0x534);
GLCTX_CHECK(mapgrid_v,  GLCTX_FO(__GLevaluatorState, v2) == 0x5b8 - 0x598);
GLCTX_CHECK(tex_unit,   sizeof(__GLtextureUnitStateICD) == 0x164);
GLCTX_CHECK(tex_env,    GLCTX_FO(__GLtextureUnitStateICD, env) == 0x70c - 0x5cc);
GLCTX_CHECK(tex_state,  sizeof(__GLtextureState) == 0x894 - 0x5cc);
GLCTX_CHECK(red_bits,   GLCTX_FO(__GLcontextModesICD, redBits) == 0x8bc - 0x8b4);
GLCTX_CHECK(depth_bits, GLCTX_FO(__GLcontextModesICD, depthBits) == 0x8f4 - 0x8b4);

#endif /* __GLCTX_H__ */

/*
** glsitypes.h -- SI types that several source files share, kept out of glsi.h so that only their users see them.
** Include after glsi.h.
*/
#ifndef GLSITYPES_H
#define GLSITYPES_H

/* dlistopt.h: a chalkboard for computing what the user wants us to do (the Material() merger in gl_dlopt.c,
** ApplyMatChange 690dfd70 in gl_dlist.c) */
typedef struct __GLmatChangeRec {
    GLenum faceName;
    GLint dirtyBits;
    __GLcolor ambient;
    __GLcolor diffuse;
    __GLcolor specular;
    __GLcolor emissive;
    __GLfloat shininess;
    __GLfloat cmapa, cmaps, cmapd;
} __GLmatChange;

/* s_accum.c: an accumulation buffer cell (the WGL accumulation buffer has 16-bit components; gl_accum.c, gl_store.c,
** __glReturnSpan 690c5510 in gl_store.c) */
typedef GLshort __GLaccumCellElement;
typedef struct __GLaccumCellRec { __GLaccumCellElement r, g, b, a; } __GLaccumCell;

/* g_lcomp.c: a bitmap's header, also the head of the glBitmap display-list record (0x20 bytes in the ICD: the SI
** fields + imageSize + one pad word; gl_lcomp3.c / gl_lexec.c compile / execute it, __glRenderBitmap 690977a0 in gl_bitmap.c and the
** fast bitmap path in gl_pxfast.c draw it) */
typedef struct __GLbitmapRec {
    GLsizei     width;
    GLsizei     height;
    __GLfloat   xorig;
    __GLfloat   yorig;
    __GLfloat   xmove;
    __GLfloat   ymove;
    GLint       imageSize;
    GLuint      pad_1c;
} __GLbitmap;

/* the {name, priority} tail (+0x24) of a target's __GLtexParamICD in the texture unit state: what
** __glLookUpTextureTexobjs 6909c660 returns (glTexParameter / glPrioritizeTextures write the priority,
** glGetTexParameter and glGet(GL_TEXTURE_BINDING_*) read them) */
typedef struct __GLtexNamePriICDRec {
    GLuint      name;
    __GLfloat   priority;
} __GLtexNamePriICD;

/* s_pgclip.c: a vertex with its window-clip outcode (__glCalc2DOutcode 69096a90; gl_pgclip.c clips polygons with
** them, gl_lnclip.c lines), and the drawable's ownership region they are clipped to */
typedef struct __GLclipVertexICDRec {
    __GLvertexICD v;
    GLuint      outcode;                    /* +0xc0 */
} __GLclipVertex;
typedef __WGLregionRect __GLregionRect;
/* the drawable's ownership region: a view of __WGLdrawable +0x1c0 (numRects, rects, boundingRect) */
typedef struct __GLdrawableRegionICDRec {
    GLint       numRects;
    __GLregionRect *rects;
    __GLregionRect boundingRect;
} __GLdrawableRegion;

/* 3dfx OGCACHE.H: one cell of a generated-code cache (__GLogCacheICD in glctx.h; __glInitOneCodeCache 690e9946
** builds them, __glAllocCodeSpace 690e9ad0 looks them up, og_util.c releases / invalidates them) */
typedef struct __GLcodeCellRec {            /* 0xfbc bytes */
    GLint       modeFlags;                  /* +0x000  the key; -1 = free */
    GLint       signature;                  /* +0x004 */
    GLubyte    *code;                       /* +0x008  this cell's part of cache->codeSpace */
    GLubyte     tr[0xfb8 - 0xc];            /* +0x00c  the name space handed back with the code */
    struct __GLcodeCellRec *next;           /* +0xfb8  next on the MRU list */
} __GLcodeCell;

/* px_paths.c: the span-modifier pick info the __glGenericPick* callers build on the stack for PickSpanModifiers
** 69086b40 (gl_pxpick.c) */
typedef struct __GLpixelSpanModInfoICDRec {
    GLint       srcData, dstData;           /* +0x00  __GL_PIXDATA_* */
    GLint       pixelPath;                  /* +0x08  __GL_PIXPATH_* */
    GLboolean   applyPixelTransfer;         /* +0x0c */
    GLubyte     pad0d[3];
    __GLproc    reader;                     /* +0x10  out: spanInfo->spanReader */
    __GLproc    render;                     /* +0x14  out: spanInfo->spanRender */
} __GLpixelSpanModInfo;
#define __GL_PIXDATA_FB         1
#define __GL_PIXDATA_MEM        2
#define __GL_PIXPATH_DRAWPIX    1
#define __GL_PIXPATH_READPIX    2
#define __GL_PIXPATH_COPYPIX    3
#define __GL_PIXPATH_READIMAGE  4
#define __GL_PIXPATH_COPYIMAGE  5

/* s_dlist.c: a generic display list (__glGenericAllocDlist 6907fa70) keeps in its __GLdlistICD.local the count of
** free functions (one more when it has a segment), then that many (freeFn, data) records, which
** __glGenericFreeDlist 6907f9c0 runs before it frees the segment */
typedef struct __GLDlistFreeFnsRec {
    void      (__fastcall *freeFn)(__GLcontext *gc, void *data);
    void       *data;
} __GLDlistFreeFns;
#define __GL_DLIST_FREECOUNT(dlist)     (*(GLuint *)(dlist)->local)
#define __GL_DLIST_FREEFNS(dlist)       ((__GLDlistFreeFns *)((GLuint *)(dlist)->local + 1))

/* texfmt.h: a texture internal format, the s_texfmt.c records (gl_texextract.c, returned by __glLookupTextureFormat
** 690a51b0 in gl_tex.c).  0x48 bytes in the ICD: this SI has no 3D extract procs.  The hardware's formats
** (__GLtextureFormatICD in glctx.h, sst_tex.c) are this record plus the Glide format. */
typedef void (__fastcall *__GLextractTexelProc)(__GLmipMapLevelICD *level, __GLtextureICD *tex,
                                                GLint img, GLint row, GLint col, struct __GLtexelRec *result);
typedef struct __GLtextureFormatSIRec {
    GLint internalFormat;
    GLint indexSize;
    GLint redSize;
    GLint greenSize;
    GLint blueSize;
    GLint alphaSize;
    GLint luminanceSize;
    GLint intensitySize;
    GLint bitsPerTexel;
    GLenum pxFormat;                /* the stored texels as a pixel span */
    GLenum pxType;
    GLint pxAlignment;
    __GLextractTexelProc extractTexel;
    __GLextractTexelProc extractTexelBorder;
    void *store;                    /* span modifiers and rect procs: NULL in every SI format */
    void *fetch;
    void *storeRect;
    void *fetchRect;
} __GLtextureFormatSI;
/* the built-in formats, in their table order (texfmt.h) */
enum {
    __GL_FORMAT_LUMINANCE8,
    __GL_FORMAT_LUMINANCE_ALPHA8,
    __GL_FORMAT_RGB8,
    __GL_FORMAT_RGB332,
    __GL_FORMAT_XRGB1555,
    __GL_FORMAT_RGB565,
    __GL_FORMAT_RGBA8,
    __GL_FORMAT_RGBA4,
    __GL_FORMAT_ARGB4,
    __GL_FORMAT_RGBA5551,
    __GL_FORMAT_ARGB1555,
    __GL_FORMAT_ALPHA8,
    __GL_FORMAT_INTENSITY8,
    __GL_FORMAT_COLOR_INDEX8,
    __GL_FORMAT_COLOR_INDEX16,
    __GL_FORMAT_VENDOR_SPECIFIC
};

#endif /* GLSITYPES_H */

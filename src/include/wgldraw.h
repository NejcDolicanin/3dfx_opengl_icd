/*
** wgldraw.h -- the window-system drawable of the WGL layer.
**
** This is the Microsoft ICD-kit flavour of SGI's __GLdrawablePrivate /
** __GLdrawableBuffer (ref/ogl-sample/gfx/include/gl/internal/glcore.h).  Field names
** follow SGI; offsets are the ones observed in 3dfxOGL.dll.  A buffer is 0x54 bytes
** (SGI: 0x50); the drawable private starts at offset 0x64 inside __WGLwindow.
*/
#ifndef __WGLDRAW_H__
#define __WGLDRAW_H__

#include <windows.h>
#include <ddraw.h>
#include <GL/gl.h>

struct __GLcontextRec;
struct __WGLwindowRec;

typedef struct __WGLdrawableRec __WGLdrawable;
typedef struct __WGLbufferRec   __WGLbuffer;
typedef struct __WGLmodesRec    __WGLmodes;

/* resize() result codes */
#define WGLBUF_FAILED     0
#define WGLBUF_OK         1
#define WGLBUF_FALLBACK   0x10     /* buffer went to the software (memory) fallback */

struct __WGLbufferRec {
    GLint   width;                  /* 0x00 */
    GLint   height;                 /* 0x04 */
    GLint   depth;                  /* 0x08  bits per element */
    void   *base;                   /* 0x0c  locked pointer */
    GLuint  size;                   /* 0x10  bytes allocated (memory buffers) */
    GLuint  elementSize;            /* 0x14  (depth-1)/8+1 */
    GLuint  elementSizeLog2;        /* 0x18 */
    GLint   outerWidth;             /* 0x1c  byteWidth / elementSize */
    GLint   byteWidth;              /* 0x20  pitch */
    void   *handle;                 /* 0x24  IDirectDrawSurface* / memory block / DCI info */
    GLboolean readlock;             /* 0x28  LFB lock flags of the hardware colour buffers (SSTPICK.C, wglglide.c) */
    GLboolean writelock;            /* 0x29 */
    GLubyte pad_2a[2];
    GLubyte (__fastcall *resize)(__WGLbuffer *buf, GLint x, GLint y, GLuint w, GLuint h,
                                 __WGLdrawable *dp, GLuint bufferMask);        /* 0x2c */
    void    (__fastcall *lock)(__WGLbuffer *buf, __WGLdrawable *dp);          /* 0x30 */
    void    (__fastcall *unlock)(__WGLbuffer *buf, __WGLdrawable *dp);        /* 0x34 */
    void    (__fastcall *fill)(__WGLbuffer *buf, __WGLdrawable *dp, GLuint val,
                               GLint x, GLint y, GLint w, GLint h);            /* 0x38 */
    void    (__fastcall *free)(__WGLbuffer *buf, __WGLdrawable *dp);          /* 0x3c */
    void   *private_;               /* 0x40  per-implementation data (see wglddraw.c / wgldib.c; windowed
                                       Glide drawables: __WGLwinSurface) */
    void    (__fastcall *freePrivate)(__WGLbuffer *buf, __WGLdrawable *dp);   /* 0x44  called by __glFreePrivate (6907de60) */
    void   *depthPrivate;           /* 0x48  depth buffer: its __WGLdepthPrivate (0x18 bytes) */
    void    (__fastcall *mainInit)(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth, void *fallbackInit); /* 0x4c */
    void    (__fastcall *fallbackInit)(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth);                /* 0x50 */
};                                  /* sizeof == 0x54 */

/* the depth buffer's private block (__WGLbuffer.depthPrivate, 0x18 bytes): the 3dfx ancestor's
** __GLdepthBuffer z-offset clear state (GLIDE3X INCLUDE/BUFFERS.H), kept with the drawable */
typedef struct __WGLdepthPrivateRec {
    GLuint    cnt;                  /* 0x00  added to every z */
    GLuint    scissorCnt;           /* 0x04 */
    GLint     numFracBits;          /* 0x08  z >> numFracBits */
    GLuint    zDepthMask;           /* 0x0c */
    GLuint    zClearInc;            /* 0x10  added to cnt per clear: 0x01000000 / 0xff000000 (0: no clear trick) */
    GLboolean fullZClear;           /* 0x14  the next clear must clear the whole buffer */
    GLboolean allowPartialZClear;   /* 0x15  clear a band per frame */
    GLboolean invertZRange;         /* 0x16 */
    GLubyte   pad17;
} __WGLdepthPrivate;               /* sizeof == 0x18 */

typedef struct __WGLregionRectRec { GLint x0, y0, x1, y1; } __WGLregionRect;

struct __WGLdrawableRec {
    __WGLmodes *modes;              /* 0x000 (window 0x064) */
    GLuint  width;                  /* 0x004 */
    GLuint  height;                 /* 0x008 */
    GLint   xOrigin;                /* 0x00c */
    GLint   yOrigin;                /* 0x010 */
    GLint   yInverted;              /* 0x014 */
    GLuint  accelBufferMask;        /* 0x018  bit per hw-accelerated buffer: 1 front,4 back,0x10 accum,0x20 depth,0x40 stencil */
    __WGLbuffer frontBuffer;        /* 0x01c */
    __WGLbuffer backBuffer;         /* 0x070 */
    __WGLbuffer accumBuffer;        /* 0x0c4 */
    __WGLbuffer depthBuffer;        /* 0x118 */
    __WGLbuffer stencilBuffer;      /* 0x16c */
    /* ownership region (clip list) */
    GLint   numRects;               /* 0x1c0 (window 0x224) */
    __WGLregionRect *rects;         /* 0x1c4 */
    __WGLregionRect boundingRect;   /* 0x1c8 */
    GLboolean regionValid;          /* 0x1d8 (window 0x23c)  set at creation, cleared on WM_DISPLAYCHANGE */
    GLboolean fullScreen;           /* 0x1d9 (window 0x23d) */
    GLubyte   pad_1da[2];
    LPCRITICAL_SECTION lock;        /* 0x1dc (window 0x240) */
    void *(__fastcall *malloc)(size_t);                      /* 0x1e0 */
    void *(__fastcall *calloc)(size_t, size_t);              /* 0x1e4 */
    void *(__fastcall *realloc)(void *, size_t);             /* 0x1e8 */
    void  (__fastcall *free)(void *);                        /* 0x1ec */
    void  (__fastcall *addSwapRect)(__WGLdrawable *dp, GLint x, GLint y, GLsizei w, GLsizei h); /* 0x1f0 */
    void  (__fastcall *setClipRect)(__WGLdrawable *dp, GLint x, GLint y, GLsizei w, GLsizei h); /* 0x1f4 */
    void  (__fastcall *updateClipRegion)(__WGLdrawable *dp);                                    /* 0x1f8 */
    void  (__fastcall *lockDP)(__WGLdrawable *dp, struct __GLcontextRec *gc);                   /* 0x1fc */
    void  (__fastcall *unlockDP)(__WGLdrawable *dp);                                            /* 0x200 */
    struct __WGLwindowRec *window;  /* 0x204  "other": back pointer to the owning window */
    void  (__fastcall *freeBuffers)(__WGLdrawable *dp);      /* 0x208 (window 0x26c) */
    struct __WGLwindowRectRec *windowRect;  /* 0x20c (window 0x270) a 0x14-byte calloc by wglGlideInitWindow
                                       (6904d4c0); the sst windowed path reads it; __glFreePrivate (6907de60)
                                       clears the pointer (does not free it) */
    void   *unk_210, *unk_214;
    GLint   childWindow;            /* 0x218 (window 0x27c) GetParent(hwnd) != NULL */
    LONG    refCount;               /* 0x21c (window 0x280) */
};                                  /* sizeof == 0x220 */

/* dp->windowRect: the window rectangle of a Glide drawable (0x14 bytes) */
typedef struct __WGLwindowRectRec {
    RECT    rect;                   /* 0x00  the window on the screen (windowed resize / makeCurrent) */
    GLint   swFallback;             /* 0x10  a buffer went to the software fallback at the last resize
                                       (wglDevNotifyResize sets it, 6901b8dd clears it); the windowed lock
                                       procs skip grFinish while it is set */
} __WGLwindowRect;

/* buf->private_ of a buffer of a windowed Glide drawable (6901b9e0 / 6901bba0 calloc 0x24 bytes); the gc
** buffer's drawableBuf is a record of the same head (69029800 copies surf / rect into it) */
typedef struct __WGLwinSurfaceRec {
    LPDIRECTDRAWSURFACE surf;       /* 0x00  the DirectDraw surface, also the Glide SURFACE extension's handle */
    RECT    rect;                   /* 0x04  -1 = no surface yet; the depth buffer's is {0, 0, w, h} */
    GLint   inVideo;                /* 0x14  1 = video memory (Glide can render to it), 0 = the system-memory
                                       fallback */
    GLuint  unk_18;
    void   *lock2, *unlock2;        /* 0x1c  depth buffer only */
} __WGLwinSurface;                  /* sizeof == 0x24 */

/* swap-hint region kept by the window (window+0x34), grown by addSwapRect */
typedef struct __WGLswapRegionRec {
    GLint  headerSize;              /* 0x00  = 0x20 */
    GLint  one;                     /* 0x04  = 1 */
    GLint  numRects;                /* 0x08 */
    GLint  byteSize;                /* 0x0c  numRects*0x10 */
    __WGLregionRect boundingRect;   /* 0x10 */
    __WGLregionRect rects[1];       /* 0x20 ... */
} __WGLswapRegion;

/* ---- wgldraw.c (generic) ---- */
void __fastcall wglDrawAddSwapRect(__WGLdrawable *dp, GLint x, GLint y, GLsizei w, GLsizei h);  /* 69009fc0 */
void __fastcall wglDrawSetClipRect(__WGLdrawable *dp, GLint x, GLint y, GLsizei w, GLsizei h);  /* 6900a0a0 */
void __fastcall wglDrawLockDP(__WGLdrawable *dp, struct __GLcontextRec *gc);                    /* 6900c9d0 */
void __fastcall wglDrawUnlockDP(__WGLdrawable *dp);                                             /* 6900ca50 */
GLubyte __fastcall wglDrawResize(__WGLdrawable *dp, GLint x, GLint y, GLuint w, GLuint h);      /* 6900cce0 */
void __fastcall wglDrawSetRegion(struct __WGLwindowRec *win, const RGNDATA *rgn);                        /* 6900cef0 */
void __fastcall wglDrawInitDDraw(struct __WGLwindowRec *w, const __WGLmodes *m);                /* 6900ca60 */
void __fastcall wglDrawInitDCI(struct __WGLwindowRec *w, const __WGLmodes *m);                  /* 6900cb50 */
void __fastcall wglDrawInitDIB(struct __WGLwindowRec *w, const __WGLmodes *m);                  /* 6900cc40 */
void __fastcall wglCSEnter(LPCRITICAL_SECTION cs);    /* 6900c9b0 */
void __fastcall wglCSLeave(LPCRITICAL_SECTION cs);    /* 6900c9c0 */
GLuint __fastcall wglLog2(GLuint v);                  /* 6907a5d0 */

/* ---- wglddraw.c ---- */
extern HMODULE            g_hDDraw;         /* 690f665c */
extern LPDIRECTDRAW       g_ddraw;          /* 690f6660 */
extern LPDIRECTDRAWSURFACE g_ddPrimary;     /* 690f6664 */
extern void              *g_ddPrimaryPtr;   /* 690f6668 */
extern int                g_ddRefs;         /* 690f666c */
LPDIRECTDRAW wglDDrawAllocate(void);                                     /* 6900f41f */
void  __fastcall wglDDrawFree(LPDIRECTDRAW dd);                          /* 6900f520 */
void  __fastcall wglDDrawError(const char *where, HRESULT hr);           /* 69011b7f */
void  __fastcall wglDDrawRestoreSurface(__WGLbuffer *buf, __WGLdrawable *dp);   /* 6900fa73 */
void  __fastcall wglDDrawInitPrimary(__WGLbuffer *buf, __WGLdrawable *dp);      /* 69010852 */
void  __fastcall wglDDrawInitColor(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth, void *fallbackInit);  /* 6900fe30 */
void  __fastcall wglDDrawInitDepth(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth, void *fallbackInit);  /* 690103b0 */
GLboolean __fastcall wglDDrawSwapBuffers(struct __WGLwindowRec *w);      /* 6900f59c */
GLboolean __fastcall wglDDrawCopyBackToFront(struct __WGLwindowRec *w);  /* 6900f720 */
void  __fastcall wglDDrawUpdateClipList(__WGLdrawable *dp);              /* 6900fceb */
void  __fastcall ddUnlock(__WGLbuffer *buf, __WGLdrawable *dp);                                 /* 690101ee */
void  __fastcall ddColorFill(__WGLbuffer *buf, __WGLdrawable *dp, GLuint val, GLint x, GLint y, GLint w, GLint h); /* 69010258 */

/* ---- wgldib.c ---- */
void  __fastcall wglMemInit(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth);                 /* 69010cb0 */
void  __fastcall wglDIBInit(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth);                 /* 69010f20 */
void  __fastcall wglDIBInitUnmanaged(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth);        /* 69011448 */
HDC   __fastcall wglDIBGetDC(__WGLbuffer *buf);                                                /* 69010f00 */
void  __fastcall wglDCIInit(__WGLbuffer *buf, __WGLdrawable *dp, GLint depth);                 /* 69011830 */
GLboolean wglDCIOpen(void);                                                                      /* 690115c8 */
GLboolean wglDCIClose(void);                                                                      /* 69011690 */
GLboolean __fastcall wglDCIDIBSwapBuffers(struct __WGLwindowRec *w);                           /* 69011762 */
void  __fastcall wglDCIUpdateClipList(struct __WGLwindowRec *w, HANDLE winWatch);              /* 69011a40 */

#endif /* __WGLDRAW_H__ */

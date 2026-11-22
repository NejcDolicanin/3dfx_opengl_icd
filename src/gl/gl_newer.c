/*
** gl_newer.c -- OpenGL 1.3, 1.4 and 1.5 on top of the 1.1 driver (not in the original).
**
** FX_GL_VERSION (g_glVersion: 11 .. 15) decides what is offered; with 1.1 nothing here is reachable.  The
** entry points have no dispatch slots of their own: they are plain functions handed out by wglGetProcAddress
** (g_newerProcs) and do their work through the current dispatch table, so display lists and the hardware /
** software split keep working.  What the 1.1 driver already has under an ARB / EXT name (multitexture, texture
** compression, fog coordinates, point parameters, the blend equation) gets its core name in wglGetProcAddressTable.
**
** Three kinds of feature (knownBugs.txt has the list per version):
**   hardware   glBlendFuncSeparate, GL_MIRRORED_REPEAT, GL_TEXTURE_LOD_BIAS, GL_MULTISAMPLE
**   CPU        transpose matrices, glMultiDraw*, glWindowPos*, GL_GENERATE_MIPMAP, buffer objects (1.5)
**   accepted   state that is kept for glGet / glIsEnabled but not drawn: cube maps, GL_CLAMP_TO_BORDER (drawn as
**              clamp to edge), sample coverage, DOT3 / SUBTRACT combine, the blend colour (nearest plain factor),
**              GL_MIN / GL_MAX blending (drawn as add), the secondary colour, depth textures (as luminance),
**              occlusion queries (1.5: everything counts as visible)
** The kept state is one set for the process, not per context.
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"
#include <glide.h>
#include <g3ext.h>
#include <string.h>
#include <stdlib.h>

#define DISP(name, type)    ((type)gc->currentDispatchState.fn[__GL_DISP_##name])
typedef void (APIENTRY *PF_e)(GLenum);
typedef void (APIENTRY *PF_ee)(GLenum, GLenum);
typedef void (APIENTRY *PF_eei)(GLenum, GLenum, GLint);
typedef void (APIENTRY *PF_ef)(GLenum, GLfloat);
typedef void (APIENTRY *PF_v)(void);
typedef void (APIENTRY *PF_fv)(const GLfloat *);
typedef void (APIENTRY *PF_dv)(const GLdouble *);
typedef void (APIENTRY *PF_efv)(GLenum, GLfloat *);
typedef void (APIENTRY *PF_eiv)(GLenum, GLint *);
typedef void (APIENTRY *PF_fff)(GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *PF_ortho)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
typedef void (APIENTRY *PF_drawArrays)(GLenum, GLint, GLsizei);
typedef void (APIENTRY *PF_drawElements)(GLenum, GLsizei, GLenum, const GLvoid *);
typedef void (APIENTRY *PF_texImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
typedef void (APIENTRY *PF_texImage1D)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
typedef void (APIENTRY *PF_getTexImage)(GLenum, GLint, GLenum, GLenum, GLvoid *);
typedef void (APIENTRY *PF_getTexLevel)(GLenum, GLint, GLenum, GLint *);

/* ---- the kept state ---- */
#define NE_SAMPLE_A2C       0x01
#define NE_SAMPLE_A2ONE     0x02
#define NE_SAMPLE_COVERAGE  0x04
#define NE_COLOR_SUM        0x08
#define NE_MULTISAMPLE      0x10
#define NE_SECONDARY_ARRAY  0x20        /* glEnableClientState(GL_SECONDARY_COLOR_ARRAY) */
#define NE_CUBE_UNIT0       0x100       /* << texture unit */
static GLuint    s_enables = NE_MULTISAMPLE;
static GLfloat   s_sampleCoverage = 1.0f;
static GLboolean s_sampleInvert;
static GLfloat   s_blendColor[4];
static GLboolean s_blendSeparate;       /* glBlendFuncSeparate gave alpha factors of its own */
static GLenum    s_blendSrcA = GL_ONE, s_blendDstA = GL_ZERO;
static GLfloat   s_secondary[4] = { 0, 0, 0, 1 };
static GLfloat   s_lodBias[__GL_NUM_TEXTURE_UNITS];
static GLenum    s_genMipHint = GL_DONT_CARE;
static GLboolean bufGet(GLenum sq, GLint *out);

/* ---- glEnable / glDisable / glIsEnabled --------------------------------------------------------------------------
** TRUE: the cap was one of the newer ones and is done.  GL_MULTISAMPLE is passed on as the 3dfx multisample cap
** (*cap changed, FALSE) so the T-buffer follows it. */
static GLuint newerEnableBit(__GLcontext *gc, GLenum cap)
{
    if (g_glVersion >= 13) {
        switch (cap) {
          case 0x809D: return NE_MULTISAMPLE;               /* GL_MULTISAMPLE */
          case 0x809E: return NE_SAMPLE_A2C;                /* GL_SAMPLE_ALPHA_TO_COVERAGE */
          case 0x809F: return NE_SAMPLE_A2ONE;              /* GL_SAMPLE_ALPHA_TO_ONE */
          case 0x80A0: return NE_SAMPLE_COVERAGE;           /* GL_SAMPLE_COVERAGE */
          case 0x8513: return NE_CUBE_UNIT0 << gc->fx.activeTexture;    /* GL_TEXTURE_CUBE_MAP: not drawn */
        }
    }
    if (g_glVersion >= 14 && cap == 0x8458)                 /* GL_COLOR_SUM: not drawn */
        return NE_COLOR_SUM;
    return 0;
}

GLboolean __fastcall __glEnableNewer(__GLcontext *gc, GLenum *cap, GLboolean on)
{
    GLuint bit = newerEnableBit(gc, *cap);

    if (bit == 0)
        return GL_FALSE;
    if (on) s_enables |= bit; else s_enables &= ~bit;
    if (bit == NE_MULTISAMPLE) {
        *cap = 0x86b2;                                      /* the 3dfx multisample cap of the hardware path */
        return GL_FALSE;
    }
    return GL_TRUE;
}

/* glEnableClientState / glDisableClientState(GL_SECONDARY_COLOR_ARRAY) (1.4): kept, the array is not read */
GLboolean __fastcall __glClientStateNewer(GLenum array, GLboolean on)
{
    if (g_glVersion < 14 || array != 0x845E)
        return GL_FALSE;
    if (on) s_enables |= NE_SECONDARY_ARRAY; else s_enables &= ~NE_SECONDARY_ARRAY;
    return GL_TRUE;
}

/* glHint(GL_GENERATE_MIPMAP_HINT) (1.4): kept; the box filter is the only one there is */
GLboolean __fastcall __glHintNewer(GLenum target, GLenum mode)
{
    if (g_glVersion < 14 || target != 0x8192)
        return GL_FALSE;
    s_genMipHint = mode;
    return GL_TRUE;
}

GLboolean __fastcall __glIsEnabledNewer(__GLcontext *gc, GLenum cap, GLboolean *out)
{
    GLuint bit = newerEnableBit(gc, cap);

    if (g_glVersion >= 14 && cap == 0x845E)                 /* GL_SECONDARY_COLOR_ARRAY */
        bit = NE_SECONDARY_ARRAY;

    if (bit == 0)
        return GL_FALSE;
    *out = (GLboolean)((s_enables & bit) != 0);
    return GL_TRUE;
}

/* ---- glGet: the values of the newer state.  Integers into iv (*ni of them) or floats into fv (*nf) ---- */
GLboolean __fastcall __glGetNewer(__GLcontext *gc, GLenum sq, GLint *iv, __GLfloat *fv, GLint *ni, GLint *nf)
{
    GLboolean b;
    GLfloat m[16];
    int i, j;

    if (__glIsEnabledNewer(gc, sq, &b)) {
        iv[0] = b; *ni = 1;
        return GL_TRUE;
    }
    if (g_glVersion >= 13) {
        switch (sq) {
          case 0x80A8: iv[0] = gc->fx.numSamples > 0; *ni = 1; return GL_TRUE;     /* GL_SAMPLE_BUFFERS */
          case 0x80A9: iv[0] = gc->fx.numSamples; *ni = 1; return GL_TRUE;         /* GL_SAMPLES */
          case 0x80AA: fv[0] = s_sampleCoverage; *nf = 1; return GL_TRUE;          /* GL_SAMPLE_COVERAGE_VALUE */
          case 0x80AB: iv[0] = s_sampleInvert; *ni = 1; return GL_TRUE;            /* GL_SAMPLE_COVERAGE_INVERT */
          case 0x851C: iv[0] = 16; *ni = 1; return GL_TRUE;        /* GL_MAX_CUBE_MAP_TEXTURE_SIZE: the minimum */
          case 0x8514: iv[0] = __glTexCubeBinding(gc); *ni = 1; return GL_TRUE;    /* GL_TEXTURE_BINDING_CUBE_MAP */
          case 0x84E3: case 0x84E4: case 0x84E5:                    /* GL_TRANSPOSE_MODELVIEW / PROJECTION /
                                                                       TEXTURE_MATRIX */
            DISP(GetFloatv, PF_efv)(sq == 0x84E3 ? GL_MODELVIEW_MATRIX : sq == 0x84E4 ? GL_PROJECTION_MATRIX :
                                    GL_TEXTURE_MATRIX, m);
            for (i = 0; i < 4; i++)
                for (j = 0; j < 4; j++)
                    fv[i * 4 + j] = m[j * 4 + i];
            *nf = 16;
            return GL_TRUE;
        }
    }
    if (g_glVersion >= 15 && bufGet(sq, iv)) {                      /* the buffer bindings */
        *ni = 1;
        return GL_TRUE;
    }
    if (g_glVersion >= 14) {
        switch (sq) {
          case 0x8005:                                              /* GL_BLEND_COLOR */
            for (i = 0; i < 4; i++) fv[i] = s_blendColor[i];
            *nf = 4;
            return GL_TRUE;
          case 0x80C9: iv[0] = gc->state.raster.blendSrc; *ni = 1; return GL_TRUE;             /* GL_BLEND_SRC_RGB */
          case 0x80C8: iv[0] = gc->state.raster.blendDst; *ni = 1; return GL_TRUE;             /* GL_BLEND_DST_RGB */
          case 0x80CB: iv[0] = s_blendSeparate ? s_blendSrcA : gc->state.raster.blendSrc; *ni = 1; return GL_TRUE;
          case 0x80CA: iv[0] = s_blendSeparate ? s_blendDstA : gc->state.raster.blendDst; *ni = 1; return GL_TRUE;
          case 0x84FD: fv[0] = 4.0f; *nf = 1; return GL_TRUE;       /* GL_MAX_TEXTURE_LOD_BIAS */
          case 0x8192: iv[0] = s_genMipHint; *ni = 1; return GL_TRUE;  /* GL_GENERATE_MIPMAP_HINT */
          case 0x8459:                                              /* GL_CURRENT_SECONDARY_COLOR */
            for (i = 0; i < 4; i++) fv[i] = s_secondary[i];
            *nf = 4;
            return GL_TRUE;
        }
    }
    return GL_FALSE;
}

/* ---- textures ---------------------------------------------------------------------------------------------------
** the wrap modes glTexParameter takes: the 1.1 ones, GL_CLAMP_TO_BORDER from 1.3 (drawn as clamp to edge: the TMU
** has no border colour), GL_MIRRORED_REPEAT from 1.4 (Glide's mirror clamp mode where it has one) */
GLboolean __fastcall __glWrapModeNewer(GLenum e)
{
    switch (e) {
      case GL_REPEAT: case GL_CLAMP: case 0x812F: return GL_TRUE;
      case 0x812D: return (GLboolean)(g_glVersion >= 13);
      case 0x8370: return (GLboolean)(g_glVersion >= 14);
    }
    return GL_FALSE;
}

/* Glide's clamp mode for GL_MIRRORED_REPEAT: GR_TEXTURECLAMP_MIRROR_EXT with the TEXMIRROR extension, else wrap */
GLint __fastcall hwTexMirrorMode(void)
{
    static GLint mode = -1;

    if (mode < 0) {
        const char *ext = grGetString(GR_EXTENSION);
        mode = (ext != NULL && strstr(ext, "TEXMIRROR") != NULL) ? GR_TEXTURECLAMP_MIRROR_EXT : GR_TEXTURECLAMP_WRAP;
    }
    return mode;
}

/* glTexEnv of what 1.3 / 1.4 add.  TRUE: done (or refused with an error).
**   GL_TEXTURE_FILTER_CONTROL / GL_TEXTURE_LOD_BIAS (1.4): Glide's LOD bias of the unit's TMU
**   GL_COMBINE_RGB / _ALPHA with GL_SUBTRACT or GL_DOT3_RGB(A) (1.3): the combine unit cannot do them; taken as
**   GL_REPLACE / GL_MODULATE so the call succeeds */
GLboolean __fastcall __glTexEnvNewer(GLenum target, GLenum pname, __GLfloat v)
{
    __GLcontext *gc = __glCurrentGC();
    GLenum e = (GLenum)v;

    if (target == 0x8500) {                                 /* GL_TEXTURE_FILTER_CONTROL */
        if (g_glVersion < 14 || pname != 0x8501) {          /* GL_TEXTURE_LOD_BIAS */
            __glSetError(GL_INVALID_ENUM);
            return GL_TRUE;
        }
        if (v > 4.0f) v = 4.0f;
        if (v < -4.0f) v = -4.0f;
        s_lodBias[gc->fx.activeTexture] = v;
        if (gc->fx.glideOpen && gc->hwcx.cmdTransportInfo != NULL)
            grTexLodBiasValue(gc->fx.tmuTexUnit[gc->fx.activeTexture], v);
        return GL_TRUE;
    }
    if (g_glVersion >= 13 && target == GL_TEXTURE_ENV && (pname == 0x8571 || pname == 0x8572)) {
        if (e == 0x84E7) {                                  /* GL_SUBTRACT */
            DISP(TexEnvi, PF_eei)(target, pname, GL_REPLACE);
            return GL_TRUE;
        }
        if (e == 0x86AE || e == 0x86AF) {                   /* GL_DOT3_RGB, GL_DOT3_RGBA */
            DISP(TexEnvi, PF_eei)(target, pname, GL_MODULATE);
            return GL_TRUE;
        }
    }
    /* texture env crossbar (1.4): GL_TEXTUREn as a combine source.  The unit's own texture is GL_TEXTURE.  Unit 1
    ** taking unit 0's texture gets GL_PREVIOUS, which is that texture whenever unit 0 passes its texture on
    ** unchanged; unit 0 cannot reach unit 1's texture (the TMUs are chained one way) and gets its own */
    if (g_glVersion >= 14 && target == GL_TEXTURE_ENV && (e == 0x84C0 || e == 0x84C1) &&
        ((pname >= 0x8580 && pname <= 0x8582) || (pname >= 0x8588 && pname <= 0x858A))) {  /* GL_SOURCEn_RGB / _ALPHA */
        GLuint unit = gc->fx.activeTexture;
        DISP(TexEnvi, PF_eei)(target, pname, (e - 0x84C0 == unit || unit == 0) ? GL_TEXTURE : 0x8578 /* GL_PREVIOUS */);
        return GL_TRUE;
    }
    return GL_FALSE;
}

/* the LOD biases again after Glide was opened anew (alt-tab): a fresh Glide context starts at 0 */
void __fastcall __glLodBiasRestore(__GLcontext *gc)
{
    GLint u;

    for (u = 0; u < gc->fx.numTMUs && u < __GL_NUM_TEXTURE_UNITS; u++)
        if (s_lodBias[u] != 0.0f)
            grTexLodBiasValue(gc->fx.tmuTexUnit[u], s_lodBias[u]);
}

/* glTexImage2D / glTexSubImage2D before the 1.1 code sees them.  TRUE: nothing more to do (a cube map face: the
** image is not kept).  From 1.4 a depth texture is stored as a luminance one (no shadow compare) */
GLboolean __fastcall __glTexImage2DNewer(GLenum target, GLint *components, GLenum *format)
{
    if (g_glVersion >= 13 && target >= 0x8515 && target <= 0x851B)     /* the six faces and the proxy */
        return GL_TRUE;
    if (g_glVersion >= 14) {
        if (*format == GL_DEPTH_COMPONENT)
            *format = GL_LUMINANCE;
        if (components != NULL && (*components == GL_DEPTH_COMPONENT || (*components >= 0x81A5 && *components <= 0x81A7)))
            *components = GL_LUMINANCE;
    }
    return GL_FALSE;
}

/* GL_GENERATE_MIPMAP (1.4): after level 0 of a 2D
** or 1D texture changed (glTexImage, glTexSubImage, glCopyTexImage, glCopyTexSubImage), the smaller levels are made
** from it with a box filter and loaded through glTexImage2D / 1D.  The level is read back as RGBA bytes, so every
** format works; a compressed texture is compressed again per level */
void __fastcall __glAutoMipmap(__GLcontext *gc, GLenum target)
{
    static const __GLpixelPackMode tight = { GL_FALSE, GL_FALSE, { 0, 0 }, 0, 0, 0, 1 };
    static GLboolean busy;
    __GLpixelPackMode savedPack, savedUnpack;
    GLint w = 0, h = 0, border = 0, ifmt = 0, level, nw, nh, x, y, c;
    GLubyte *a, *b, *t;

    if (busy || (target != GL_TEXTURE_2D && target != GL_TEXTURE_1D) || !__glTexWantsMipmaps(gc, target))
        return;
    DISP(GetTexLevelParameteriv, PF_getTexLevel)(target, 0, GL_TEXTURE_WIDTH, &w);
    if (target == GL_TEXTURE_1D)
        h = 1;
    else
        DISP(GetTexLevelParameteriv, PF_getTexLevel)(target, 0, GL_TEXTURE_HEIGHT, &h);
    DISP(GetTexLevelParameteriv, PF_getTexLevel)(target, 0, GL_TEXTURE_BORDER, &border);
    DISP(GetTexLevelParameteriv, PF_getTexLevel)(target, 0, GL_TEXTURE_INTERNAL_FORMAT, &ifmt);
    if (w <= 0 || h <= 0 || border != 0 || (w == 1 && h == 1))
        return;
    a = (GLubyte *)malloc((size_t)w * h * 4);
    b = (GLubyte *)malloc((size_t)w * h);                   /* (w/2) * (h/2) * 4 at most, w * 4 for a 1-high row */
    if (a == NULL || b == NULL) {
        free(a); free(b);
        return;
    }
    busy = GL_TRUE;
    savedPack = gc->state.pixel.packModes;
    savedUnpack = gc->state.pixel.unpackModes;
    gc->state.pixel.packModes = tight;
    gc->state.pixel.unpackModes = tight;
    DISP(GetTexImage, PF_getTexImage)(target, 0, GL_RGBA, GL_UNSIGNED_BYTE, a);
    for (level = 1; w > 1 || h > 1; level++) {
        nw = w > 1 ? w / 2 : 1;
        nh = h > 1 ? h / 2 : 1;
        for (y = 0; y < nh; y++)
            for (x = 0; x < nw; x++) {
                GLint x0 = w > 1 ? x * 2 : 0, x1 = w > 1 ? x * 2 + 1 : 0;
                GLint y0 = h > 1 ? y * 2 : 0, y1 = h > 1 ? y * 2 + 1 : 0;
                for (c = 0; c < 4; c++)
                    b[(y * nw + x) * 4 + c] = (GLubyte)((a[(y0 * w + x0) * 4 + c] + a[(y0 * w + x1) * 4 + c] +
                                                         a[(y1 * w + x0) * 4 + c] + a[(y1 * w + x1) * 4 + c] + 2) >> 2);
            }
        if (target == GL_TEXTURE_1D)
            DISP(TexImage1D, PF_texImage1D)(target, level, ifmt, nw, 0, GL_RGBA, GL_UNSIGNED_BYTE, b);
        else
            DISP(TexImage2D, PF_texImage2D)(target, level, ifmt, nw, nh, 0, GL_RGBA, GL_UNSIGNED_BYTE, b);
        t = a; a = b; b = t;
        w = nw; h = nh;
    }
    gc->state.pixel.packModes = savedPack;
    gc->state.pixel.unpackModes = savedUnpack;
    busy = GL_FALSE;
    free(a); free(b);
}

/* ---- blending ---------------------------------------------------------------------------------------------------
** the constant blend factors (1.4): the blender has no constant, so the factor becomes ONE or ZERO, whichever is
** nearer to the constant (its brightness for the colour factors).  TRUE: factor was one of them */
GLboolean __fastcall __glBlendConstFactor(GLenum factor, GLuint *out)
{
    GLfloat c;

    if (g_glVersion < 14 || factor < 0x8001 || factor > 0x8004)
        return GL_FALSE;
    if (factor <= 0x8002)                                   /* GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR */
        c = (s_blendColor[0] + s_blendColor[1] + s_blendColor[2]) / 3.0f;
    else                                                    /* GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA */
        c = s_blendColor[3];
    if (factor == 0x8002 || factor == 0x8004)
        c = 1.0f - c;
    *out = c >= 0.5f ? GR_BLEND_ONE : GR_BLEND_ZERO;
    return GL_TRUE;
}

/* glBlendFunc was called: the alpha factors follow the colour ones again */
void __fastcall __glBlendSeparateOff(void)
{
    s_blendSeparate = GL_FALSE;
}

/* the Glide alpha factors of glBlendFuncSeparate, for grAlphaBlendFunctionExt (hwFlushGrState) */
void __fastcall __glBlendSeparateAlpha(__GLcontext *gc, GLint *as, GLint *ad)
{
    GLuint s, d;

    if (!s_blendSeparate || !(gc->state.enables.general & __GL_BLEND_ENABLE))
        return;
    if (!hwBlendFactors(gc, s_blendSrcA, s_blendDstA, &s, &d))
        return;
    *as = s == GR_BLEND_SAME_COLOR_EXT ? GR_BLEND_SRC_ALPHA :
          s == GR_BLEND_ONE_MINUS_SAME_COLOR_EXT ? GR_BLEND_ONE_MINUS_SRC_ALPHA : (GLint)s;
    *ad = d == GR_BLEND_SAME_COLOR_EXT ? GR_BLEND_DST_ALPHA :
          d == GR_BLEND_ONE_MINUS_SAME_COLOR_EXT ? GR_BLEND_ONE_MINUS_DST_ALPHA : (GLint)d;
}

static void APIENTRY newer_BlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcA, GLenum dstA)
{
    __GLcontext *gc = __glCurrentGC();
    GLuint s, d;

    if (!hwBlendFactors(gc, srcA, dstA, &s, &d)) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    DISP(BlendFunc, PF_ee)(srcRGB, dstRGB);                 /* (clears the separate state) */
    if (srcA != srcRGB || dstA != dstRGB) {
        s_blendSeparate = GL_TRUE;
        s_blendSrcA = srcA;
        s_blendDstA = dstA;
        gc->hwcx.grDirty |= 1 << __GL_GR_ALPHA_BLEND;
        __GL_DELAY_VALIDATE(gc);
    }
}

static void APIENTRY newer_BlendColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
    __GLcontext *gc = __glCurrentGC();
    GLboolean sep = s_blendSeparate;

    s_blendColor[0] = r < 0 ? 0 : r > 1 ? 1 : r;
    s_blendColor[1] = g < 0 ? 0 : g > 1 ? 1 : g;
    s_blendColor[2] = b < 0 ? 0 : b > 1 ? 1 : b;
    s_blendColor[3] = a < 0 ? 0 : a > 1 ? 1 : a;
    /* a constant factor in use is worked out again */
    if ((gc->state.raster.blendSrc >= 0x8001 && gc->state.raster.blendSrc <= 0x8004) ||
        (gc->state.raster.blendDst >= 0x8001 && gc->state.raster.blendDst <= 0x8004)) {
        DISP(BlendFunc, PF_ee)(gc->state.raster.blendSrc, gc->state.raster.blendDst);
        s_blendSeparate = sep;
    }
}

/* ---- multisample ---- */
static void APIENTRY newer_SampleCoverage(GLclampf value, GLboolean invert)
{
    s_sampleCoverage = value < 0 ? 0 : value > 1 ? 1 : value;      /* kept; the T-buffer has no coverage mask */
    s_sampleInvert = (GLboolean)(invert != 0);
}

/* ---- transpose matrices (1.3) ---- */
static void APIENTRY newer_LoadTransposeMatrixf(const GLfloat *m)
{
    __GLcontext *gc = __glCurrentGC();
    GLfloat t[16];
    int i, j;
    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) t[i * 4 + j] = m[j * 4 + i];
    DISP(LoadMatrixf, PF_fv)(t);
}
static void APIENTRY newer_LoadTransposeMatrixd(const GLdouble *m)
{
    __GLcontext *gc = __glCurrentGC();
    GLdouble t[16];
    int i, j;
    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) t[i * 4 + j] = m[j * 4 + i];
    DISP(LoadMatrixd, PF_dv)(t);
}
static void APIENTRY newer_MultTransposeMatrixf(const GLfloat *m)
{
    __GLcontext *gc = __glCurrentGC();
    GLfloat t[16];
    int i, j;
    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) t[i * 4 + j] = m[j * 4 + i];
    DISP(MultMatrixf, PF_fv)(t);
}
static void APIENTRY newer_MultTransposeMatrixd(const GLdouble *m)
{
    __GLcontext *gc = __glCurrentGC();
    GLdouble t[16];
    int i, j;
    for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) t[i * 4 + j] = m[j * 4 + i];
    DISP(MultMatrixd, PF_dv)(t);
}

/* ---- compressed 3D images: there are none ---- */
static void APIENTRY newer_CompressedTexImage3D(GLenum target, GLint level, GLenum ifmt, GLsizei w, GLsizei h, GLsizei d,
                                               GLint border, GLsizei size, const GLvoid *data)
{
    __glSetError(GL_INVALID_ENUM);
}
static void APIENTRY newer_CompressedTexSubImage3D(GLenum target, GLint level, GLint x, GLint y, GLint z, GLsizei w,
                                                  GLsizei h, GLsizei d, GLenum format, GLsizei size, const GLvoid *data)
{
    __glSetError(GL_INVALID_ENUM);
}

/* ---- glMultiDraw* (1.4): the single calls in a loop ---- */
static void APIENTRY newer_MultiDrawArrays(GLenum mode, const GLint *first, const GLsizei *count, GLsizei n)
{
    __GLcontext *gc = __glCurrentGC();
    GLsizei i;
    for (i = 0; i < n; i++)
        if (count[i] > 0)
            DISP(DrawArrays, PF_drawArrays)(mode, first[i], count[i]);
}
static void APIENTRY newer_MultiDrawElements(GLenum mode, const GLsizei *count, GLenum type, const GLvoid **indices,
                                            GLsizei n)
{
    __GLcontext *gc = __glCurrentGC();
    GLsizei i;
    for (i = 0; i < n; i++)
        if (count[i] > 0)
            DISP(DrawElements, PF_drawElements)(mode, count[i], type, indices[i]);
}

/* ---- glPointParameteri / iv (1.4): the float ones ---- */
static void APIENTRY newer_PointParameteri(GLenum pname, GLint v)
{
    __GLcontext *gc = __glCurrentGC();
    DISP(PointParameterfEXT, PF_ef)(pname, (GLfloat)v);
}
static void APIENTRY newer_PointParameteriv(GLenum pname, const GLint *v)
{
    __GLcontext *gc = __glCurrentGC();
    DISP(PointParameterfEXT, PF_ef)(pname, (GLfloat)v[0]);
}

/* ---- the secondary colour (1.4): kept, not drawn (the chip cannot add a colour after texturing in one pass) ---- */
static void secondary(GLfloat r, GLfloat g, GLfloat b)
{
    s_secondary[0] = r; s_secondary[1] = g; s_secondary[2] = b;
}
#define SEC3(sfx, T, scale) \
    static void APIENTRY newer_SecondaryColor3##sfx(T r, T g, T b) \
        { secondary((GLfloat)(r * scale), (GLfloat)(g * scale), (GLfloat)(b * scale)); } \
    static void APIENTRY newer_SecondaryColor3##sfx##v(const T *v) \
        { secondary((GLfloat)(v[0] * scale), (GLfloat)(v[1] * scale), (GLfloat)(v[2] * scale)); }
SEC3(b,  GLbyte,   (1.0 / 127.0))
SEC3(d,  GLdouble, 1.0)
SEC3(f,  GLfloat,  1.0)
SEC3(i,  GLint,    (1.0 / 2147483647.0))
SEC3(s,  GLshort,  (1.0 / 32767.0))
SEC3(ub, GLubyte,  (1.0 / 255.0))
SEC3(ui, GLuint,   (1.0 / 4294967295.0))
SEC3(us, GLushort, (1.0 / 65535.0))
static void APIENTRY newer_SecondaryColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    if (size != 3) __glSetError(GL_INVALID_VALUE);
}

/* ---- glWindowPos* (1.4): the raster position in window coordinates.  Done with glRasterPos under an identity
** modelview and an orthographic projection of the viewport, so a position outside the viewport becomes invalid
** (the specification keeps it valid) ---- */
static void windowPos(GLfloat x, GLfloat y, GLfloat z)
{
    __GLcontext *gc = __glCurrentGC();
    GLint vp[4], mode;

    DISP(GetIntegerv, PF_eiv)(GL_VIEWPORT, vp);
    DISP(GetIntegerv, PF_eiv)(GL_MATRIX_MODE, &mode);
    DISP(MatrixMode, PF_e)(GL_PROJECTION);
    DISP(PushMatrix, PF_v)();
    DISP(LoadIdentity, PF_v)();
    DISP(Ortho, PF_ortho)(vp[0], vp[0] + vp[2], vp[1], vp[1] + vp[3], 0.0, -1.0);
    DISP(MatrixMode, PF_e)(GL_MODELVIEW);
    DISP(PushMatrix, PF_v)();
    DISP(LoadIdentity, PF_v)();
    DISP(RasterPos3f, PF_fff)(x, y, z < 0 ? 0 : z > 1 ? 1 : z);
    DISP(PopMatrix, PF_v)();
    DISP(MatrixMode, PF_e)(GL_PROJECTION);
    DISP(PopMatrix, PF_v)();
    DISP(MatrixMode, PF_e)((GLenum)mode);
}
#define WPOS(sfx, T) \
    static void APIENTRY newer_WindowPos2##sfx(T x, T y)          { windowPos((GLfloat)x, (GLfloat)y, 0); } \
    static void APIENTRY newer_WindowPos2##sfx##v(const T *v)     { windowPos((GLfloat)v[0], (GLfloat)v[1], 0); } \
    static void APIENTRY newer_WindowPos3##sfx(T x, T y, T z)     { windowPos((GLfloat)x, (GLfloat)y, (GLfloat)z); } \
    static void APIENTRY newer_WindowPos3##sfx##v(const T *v)     { windowPos((GLfloat)v[0], (GLfloat)v[1], (GLfloat)v[2]); }
WPOS(d, GLdouble)
WPOS(f, GLfloat)
WPOS(i, GLint)
WPOS(s, GLshort)

/* ==== OpenGL 1.5 ===================================================================================================
** Buffer objects: plain memory on the CPU.  The card has no vertex memory, so nothing gets faster; what a game gains
** is that its buffer-object path runs.  An array pointer set while a buffer is bound is the buffer's address plus
** the offset the game gave (__glBufArrayPointer, called by the gl*Pointer functions), so the array code below never
** knows.  Each array remembers its buffer and offset: when glBufferData moves the storage the pointers are set again.
** Not followed: glPushClientAttrib / glPopClientAttrib (the bindings are not saved). */
typedef struct {
    GLubyte  *data;
    GLint     size, capacity;
    GLenum    usage, access;
    GLboolean mapped, deleted, used;    /* used: was bound once (glIsBuffer) */
    GLint     refs;                     /* arrays that point into it: the storage outlives glDeleteBuffers */
} NewerBuffer;
#define BUF_SLOTS       (__GL_BUFSLOT_TEX0 + __GL_NUM_TEXTURE_UNITS)
#define BUF_MAX_NAME    65535
static NewerBuffer **s_buffers;         /* by name; NULL until the first buffer call */
static GLuint s_bufferSlots, s_bufferNext = 1;
static GLuint s_arrayBuffer, s_elementBuffer;
static struct { GLuint name; GLint offset; } s_bufSlot[BUF_SLOTS];

static NewerBuffer *bufLookup(GLuint name)
{
    return (name != 0 && name < s_bufferSlots) ? s_buffers[name] : NULL;
}
static NewerBuffer *bufCreate(GLuint name)
{
    if (name == 0 || name > BUF_MAX_NAME)
        return NULL;
    if (name >= s_bufferSlots) {
        GLuint n = s_bufferSlots ? s_bufferSlots : 64;
        NewerBuffer **t;
        while (n <= name) n *= 2;
        t = (NewerBuffer **)realloc(s_buffers, n * sizeof(*t));
        if (t == NULL) return NULL;
        memset(t + s_bufferSlots, 0, (n - s_bufferSlots) * sizeof(*t));
        s_buffers = t;
        s_bufferSlots = n;
    }
    if (s_buffers[name] == NULL) {
        s_buffers[name] = (NewerBuffer *)calloc(1, sizeof(NewerBuffer));
        if (s_buffers[name] != NULL) {
            s_buffers[name]->usage = 0x88E4;        /* GL_STATIC_DRAW */
            s_buffers[name]->access = 0x88BA;       /* GL_READ_WRITE */
        }
    }
    return s_buffers[name];
}
static void bufRelease(GLuint name)                 /* one array less points into it */
{
    NewerBuffer *b = bufLookup(name);
    if (b != NULL && --b->refs <= 0 && b->deleted) {
        free(b->data);
        free(b);
        s_buffers[name] = NULL;
    }
}
static NewerBuffer *bufTarget(GLenum target)        /* the buffer bound to target; sets the error */
{
    NewerBuffer *b;
    if (target != 0x8892 && target != 0x8893) {     /* GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER */
        __glSetError(GL_INVALID_ENUM);
        return NULL;
    }
    b = bufLookup(target == 0x8892 ? s_arrayBuffer : s_elementBuffer);
    if (b == NULL) __glSetError(GL_INVALID_OPERATION);
    return b;
}
/* the storage of buffer name moved: every array that lives in it gets its pointer again */
static void bufMoved(__GLcontext *gc, GLuint name, const NewerBuffer *b)
{
    GLint i, u;
    for (i = 0; i < BUF_SLOTS; i++) {
        const GLvoid *p = b->data + s_bufSlot[i].offset;
        if (s_bufSlot[i].name != name) continue;
        switch (i) {
          case __GL_BUFSLOT_VERTEX: gc->vertexArray.vertex.pointer = p;   gc->vertexArray.vertex.count = 0; break;
          case __GL_BUFSLOT_NORMAL: gc->vertexArray.normal.pointer = p;   gc->vertexArray.normal.count = 0; break;
          case __GL_BUFSLOT_COLOR:  gc->vertexArray.color.pointer = p;    gc->vertexArray.color.count = 0; break;
          case __GL_BUFSLOT_INDEX:  gc->vertexArray.index.pointer = p;    gc->vertexArray.index.count = 0; break;
          case __GL_BUFSLOT_EDGE:   gc->vertexArray.edgeFlag.pointer = p; gc->vertexArray.edgeFlag.count = 0; break;
          case __GL_BUFSLOT_FOG:    gc->vertexArray.fogCoord.pointer = p; gc->vertexArray.fogCoord.count = 0; break;
          case __GL_BUFSLOT_SECONDARY: break;
          default:
            u = i - __GL_BUFSLOT_TEX0;
            gc->vertexArray.texture.pointer[u] = p;
            gc->vertexArray.texture.count[u] = 0;
            break;
        }
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_VERTARRAY);
    }
}

/* gl*Pointer: the address to keep for the array.  Without a bound buffer it is the game's pointer */
const GLvoid *__fastcall __glBufArrayPointer(__GLcontext *gc, GLint slot, const GLvoid *pointer)
{
    NewerBuffer *b;

    if (s_buffers == NULL)                          /* no buffer object was ever made: OpenGL 1.1 .. 1.4 */
        return pointer;
    bufRelease(s_bufSlot[slot].name);
    s_bufSlot[slot].name = 0;
    b = bufLookup(s_arrayBuffer);
    if (b == NULL)
        return pointer;
    s_bufSlot[slot].name = s_arrayBuffer;
    s_bufSlot[slot].offset = (GLint)pointer;
    b->refs++;
    return b->data + (GLint)pointer;
}
/* glDrawElements: the index list, in the element buffer when one is bound */
const GLvoid *__fastcall __glBufElements(const GLvoid *indices)
{
    const NewerBuffer *b;
    if (s_elementBuffer == 0 || (b = bufLookup(s_elementBuffer)) == NULL)
        return indices;
    return b->data + (GLint)indices;
}
static GLint bufPointerSlot(__GLcontext *gc, GLenum pname)
{
    switch (pname) {
      case GL_VERTEX_ARRAY_POINTER:        return __GL_BUFSLOT_VERTEX;
      case GL_NORMAL_ARRAY_POINTER:        return __GL_BUFSLOT_NORMAL;
      case GL_COLOR_ARRAY_POINTER:         return __GL_BUFSLOT_COLOR;
      case GL_INDEX_ARRAY_POINTER:         return __GL_BUFSLOT_INDEX;
      case GL_EDGE_FLAG_ARRAY_POINTER:     return __GL_BUFSLOT_EDGE;
      case 0x8456:                         return __GL_BUFSLOT_FOG;         /* GL_FOG_COORDINATE_ARRAY_POINTER */
      case GL_TEXTURE_COORD_ARRAY_POINTER: return __GL_BUFSLOT_TEX0 + gc->fx.clientActiveTexture;
    }
    return -1;
}
/* glGetPointerv: TRUE when the array lives in a buffer; the answer is then its offset */
GLboolean __fastcall __glBufGetPointer(__GLcontext *gc, GLenum pname, void **params)
{
    GLint slot;
    if (s_buffers == NULL || (slot = bufPointerSlot(gc, pname)) < 0 || s_bufSlot[slot].name == 0)
        return GL_FALSE;
    *params = (void *)s_bufSlot[slot].offset;
    return GL_TRUE;
}
/* glGet: GL_ARRAY_BUFFER_BINDING, GL_ELEMENT_ARRAY_BUFFER_BINDING and the binding of each array */
static GLboolean bufGet(GLenum sq, GLint *out)
{
    __GLcontext *gc = __glCurrentGC();
    switch (sq) {
      case 0x8894: *out = s_arrayBuffer; return GL_TRUE;
      case 0x8895: *out = s_elementBuffer; return GL_TRUE;
      case 0x8896: *out = s_bufSlot[__GL_BUFSLOT_VERTEX].name; return GL_TRUE;
      case 0x8897: *out = s_bufSlot[__GL_BUFSLOT_NORMAL].name; return GL_TRUE;
      case 0x8898: *out = s_bufSlot[__GL_BUFSLOT_COLOR].name; return GL_TRUE;
      case 0x8899: *out = s_bufSlot[__GL_BUFSLOT_INDEX].name; return GL_TRUE;
      case 0x889A: *out = s_bufSlot[__GL_BUFSLOT_TEX0 + gc->fx.clientActiveTexture].name; return GL_TRUE;
      case 0x889B: *out = s_bufSlot[__GL_BUFSLOT_EDGE].name; return GL_TRUE;
      case 0x889C: *out = s_bufSlot[__GL_BUFSLOT_SECONDARY].name; return GL_TRUE;
      case 0x889D: *out = s_bufSlot[__GL_BUFSLOT_FOG].name; return GL_TRUE;
    }
    return GL_FALSE;
}

static void APIENTRY newer_GenBuffers(GLsizei n, GLuint *names)
{
    GLsizei i;
    if (n < 0) { __glSetError(GL_INVALID_VALUE); return; }
    for (i = 0; i < n; i++) {
        while (bufLookup(s_bufferNext) != NULL && s_bufferNext < BUF_MAX_NAME) s_bufferNext++;
        names[i] = bufCreate(s_bufferNext) != NULL ? s_bufferNext++ : 0;
        if (names[i] == 0) __glSetError(GL_OUT_OF_MEMORY);
    }
}
static void APIENTRY newer_DeleteBuffers(GLsizei n, const GLuint *names)
{
    GLsizei i;
    if (n < 0) { __glSetError(GL_INVALID_VALUE); return; }
    for (i = 0; i < n; i++) {
        NewerBuffer *b = bufLookup(names[i]);
        if (b == NULL || b->deleted) continue;
        if (s_arrayBuffer == names[i]) s_arrayBuffer = 0;
        if (s_elementBuffer == names[i]) s_elementBuffer = 0;
        b->deleted = GL_TRUE;
        b->refs++;                                  /* freed here unless an array still points into it */
        bufRelease(names[i]);
    }
}
static GLboolean APIENTRY newer_IsBuffer(GLuint name)
{
    const NewerBuffer *b = bufLookup(name);
    return (GLboolean)(b != NULL && b->used && !b->deleted);
}
static void APIENTRY newer_BindBuffer(GLenum target, GLuint name)
{
    NewerBuffer *b = NULL;
    if (target != 0x8892 && target != 0x8893) { __glSetError(GL_INVALID_ENUM); return; }
    if (name != 0) {
        b = bufLookup(name);
        if (b == NULL || b->deleted) b = (b == NULL) ? bufCreate(name) : NULL;
        if (b == NULL) { __glSetError(GL_OUT_OF_MEMORY); return; }
        b->used = GL_TRUE;
    }
    if (target == 0x8892) s_arrayBuffer = name; else s_elementBuffer = name;
}
static void APIENTRY newer_BufferData(GLenum target, GLint size, const GLvoid *data, GLenum usage)
{
    NewerBuffer *b = bufTarget(target);
    if (b == NULL) return;
    if (size < 0) { __glSetError(GL_INVALID_VALUE); return; }
    if (usage < 0x88E0 || usage > 0x88EA || (usage & 3) == 3) { __glSetError(GL_INVALID_ENUM); return; }
    if (size > b->capacity || b->data == NULL) {    /* a refill of the same size keeps its place */
        GLubyte *p = (GLubyte *)malloc(size ? size : 1);
        if (p == NULL) { __glSetError(GL_OUT_OF_MEMORY); return; }
        free(b->data);
        b->data = p;
        b->capacity = size;
        if (b->refs > 0)
            bufMoved(__glCurrentGC(), target == 0x8892 ? s_arrayBuffer : s_elementBuffer, b);
    }
    b->size = size;
    b->usage = usage;
    b->access = 0x88BA;
    b->mapped = GL_FALSE;
    if (data != NULL) memcpy(b->data, data, size);
}
static void APIENTRY newer_BufferSubData(GLenum target, GLint offset, GLint size, const GLvoid *data)
{
    NewerBuffer *b = bufTarget(target);
    if (b == NULL) return;
    if (offset < 0 || size < 0 || offset + size > b->size) { __glSetError(GL_INVALID_VALUE); return; }
    if (b->mapped) { __glSetError(GL_INVALID_OPERATION); return; }
    memcpy(b->data + offset, data, size);
}
static void APIENTRY newer_GetBufferSubData(GLenum target, GLint offset, GLint size, GLvoid *data)
{
    NewerBuffer *b = bufTarget(target);
    if (b == NULL) return;
    if (offset < 0 || size < 0 || offset + size > b->size) { __glSetError(GL_INVALID_VALUE); return; }
    if (b->mapped) { __glSetError(GL_INVALID_OPERATION); return; }
    memcpy(data, b->data + offset, size);
}
static GLvoid *APIENTRY newer_MapBuffer(GLenum target, GLenum access)
{
    NewerBuffer *b = bufTarget(target);
    if (b == NULL) return NULL;
    if (access < 0x88B8 || access > 0x88BA) { __glSetError(GL_INVALID_ENUM); return NULL; }
    if (b->mapped || b->data == NULL) { __glSetError(GL_INVALID_OPERATION); return NULL; }
    b->mapped = GL_TRUE;
    b->access = access;
    return b->data;
}
static GLboolean APIENTRY newer_UnmapBuffer(GLenum target)
{
    NewerBuffer *b = bufTarget(target);
    if (b == NULL) return GL_FALSE;
    if (!b->mapped) { __glSetError(GL_INVALID_OPERATION); return GL_FALSE; }
    b->mapped = GL_FALSE;
    b->access = 0x88BA;
    return GL_TRUE;
}
static void APIENTRY newer_GetBufferParameteriv(GLenum target, GLenum pname, GLint *v)
{
    const NewerBuffer *b = bufTarget(target);
    if (b == NULL) return;
    switch (pname) {
      case 0x8764: *v = b->size; break;             /* GL_BUFFER_SIZE */
      case 0x8765: *v = b->usage; break;            /* GL_BUFFER_USAGE */
      case 0x88BB: *v = b->access; break;           /* GL_BUFFER_ACCESS */
      case 0x88BC: *v = b->mapped; break;           /* GL_BUFFER_MAPPED */
      default: __glSetError(GL_INVALID_ENUM); break;
    }
}
static void APIENTRY newer_GetBufferPointerv(GLenum target, GLenum pname, GLvoid **v)
{
    const NewerBuffer *b = bufTarget(target);
    if (b == NULL) return;
    if (pname != 0x88BD) { __glSetError(GL_INVALID_ENUM); return; }     /* GL_BUFFER_MAP_POINTER */
    *v = b->mapped ? b->data : NULL;
}

/* ---- occlusion queries (1.5): accepted.  Nothing is counted: a finished query says every pixel of the viewport
** passed, so what a game tests this way is always drawn.  (The chip does count pixels, but Glide reads the counters
** of the first chip only, which in SLI sees every other band of lines; a small object could then read 0.) ---- */
static GLubyte *s_queries;              /* by name: 0 free, 1 generated, 2 used */
static GLuint s_querySlots, s_currentQuery, s_queryResult;

static void APIENTRY newer_GenQueries(GLsizei n, GLuint *names)
{
    GLsizei i;
    GLuint q = 1;
    if (n < 0) { __glSetError(GL_INVALID_VALUE); return; }
    for (i = 0; i < n; i++) {
        while (q < s_querySlots && s_queries[q] != 0) q++;
        if (q >= s_querySlots) {
            GLuint m = s_querySlots ? s_querySlots * 2 : 64;
            GLubyte *t = (GLubyte *)realloc(s_queries, m);
            if (t == NULL) { __glSetError(GL_OUT_OF_MEMORY); names[i] = 0; continue; }
            memset(t + s_querySlots, 0, m - s_querySlots);
            s_queries = t;
            s_querySlots = m;
        }
        s_queries[q] = 1;
        names[i] = q;
    }
}
static void APIENTRY newer_DeleteQueries(GLsizei n, const GLuint *names)
{
    GLsizei i;
    if (n < 0) { __glSetError(GL_INVALID_VALUE); return; }
    for (i = 0; i < n; i++)
        if (names[i] != 0 && names[i] < s_querySlots && names[i] != s_currentQuery)
            s_queries[names[i]] = 0;
}
static GLboolean APIENTRY newer_IsQuery(GLuint name)
{
    return (GLboolean)(name != 0 && name < s_querySlots && s_queries[name] == 2);
}
static void APIENTRY newer_BeginQuery(GLenum target, GLuint name)
{
    if (target != 0x8914) { __glSetError(GL_INVALID_ENUM); return; }    /* GL_SAMPLES_PASSED */
    if (name == 0 || name >= s_querySlots || s_queries[name] == 0 || s_currentQuery != 0) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }
    s_queries[name] = 2;
    s_currentQuery = name;
}
static void APIENTRY newer_EndQuery(GLenum target)
{
    __GLcontext *gc = __glCurrentGC();
    if (target != 0x8914) { __glSetError(GL_INVALID_ENUM); return; }
    if (s_currentQuery == 0) { __glSetError(GL_INVALID_OPERATION); return; }
    s_queryResult = (GLuint)(gc->state.viewport.width * gc->state.viewport.height);
    if (s_queryResult == 0) s_queryResult = 1;
    s_currentQuery = 0;
}
static void APIENTRY newer_GetQueryiv(GLenum target, GLenum pname, GLint *v)
{
    if (target != 0x8914) { __glSetError(GL_INVALID_ENUM); return; }
    switch (pname) {
      case 0x8864: *v = 32; break;                  /* GL_QUERY_COUNTER_BITS */
      case 0x8865: *v = s_currentQuery; break;      /* GL_CURRENT_QUERY */
      default: __glSetError(GL_INVALID_ENUM); break;
    }
}
static void APIENTRY newer_GetQueryObjectuiv(GLuint name, GLenum pname, GLuint *v)
{
    if (name == 0 || name >= s_querySlots || s_queries[name] == 0 || name == s_currentQuery) {
        __glSetError(GL_INVALID_OPERATION);
        return;
    }
    switch (pname) {
      case 0x8866: *v = s_queryResult; break;       /* GL_QUERY_RESULT */
      case 0x8867: *v = GL_TRUE; break;             /* GL_QUERY_RESULT_AVAILABLE */
      default: __glSetError(GL_INVALID_ENUM); break;
    }
}
static void APIENTRY newer_GetQueryObjectiv(GLuint name, GLenum pname, GLint *v)
{
    newer_GetQueryObjectuiv(name, pname, (GLuint *)v);
}

/* ---- the entry points wglGetProcAddress hands out, with the version that offers each ---- */
#define P(name, ver)        { "gl" #name, (PROC)newer_##name, ver }
#define SECP(sfx)           P(SecondaryColor3##sfx, 14), P(SecondaryColor3##sfx##v, 14)
#define WPOSP(sfx)          P(WindowPos2##sfx, 14), P(WindowPos2##sfx##v, 14), P(WindowPos3##sfx, 14), P(WindowPos3##sfx##v, 14)
const __GLnewerProc g_newerProcs[] = {
    { "glDrawRangeElements",    (PROC)__glim_DrawRangeElements, 12 },
    { "glDrawRangeElementsEXT", (PROC)__glim_DrawRangeElements, 12 },
    { "glTexImage3D",           (PROC)__glim_TexImage3D, 12 },          /* accepted, not drawn (gl_tex.c) */
    { "glTexSubImage3D",        (PROC)__glim_TexSubImage3D, 12 },
    { "glCopyTexSubImage3D",    (PROC)__glim_CopyTexSubImage3D, 12 },
    P(SampleCoverage, 13),
    P(LoadTransposeMatrixf, 13), P(LoadTransposeMatrixd, 13), P(MultTransposeMatrixf, 13), P(MultTransposeMatrixd, 13),
    P(CompressedTexImage3D, 13), P(CompressedTexSubImage3D, 13),
    P(BlendFuncSeparate, 14), P(BlendColor, 14),
    P(MultiDrawArrays, 14), P(MultiDrawElements, 14),
    P(PointParameteri, 14), P(PointParameteriv, 14),
    SECP(b), SECP(d), SECP(f), SECP(i), SECP(s), SECP(ub), SECP(ui), SECP(us),
    P(SecondaryColorPointer, 14),
    WPOSP(d), WPOSP(f), WPOSP(i), WPOSP(s),
    /* 1.5; wglGetProcAddressTable also answers these with "ARB" appended */
    P(GenBuffers, 15), P(DeleteBuffers, 15), P(IsBuffer, 15), P(BindBuffer, 15), P(BufferData, 15),
    P(BufferSubData, 15), P(GetBufferSubData, 15), P(MapBuffer, 15), P(UnmapBuffer, 15),
    P(GetBufferParameteriv, 15), P(GetBufferPointerv, 15),
    P(GenQueries, 15), P(DeleteQueries, 15), P(IsQuery, 15), P(BeginQuery, 15), P(EndQuery, 15),
    P(GetQueryiv, 15), P(GetQueryObjectiv, 15), P(GetQueryObjectuiv, 15),
};
const GLint g_newerProcCount = sizeof(g_newerProcs) / sizeof(g_newerProcs[0]);

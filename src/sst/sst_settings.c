/*
** sst_settings.c -- settings read from the registry / environment (FX_GL_*): guard band and Z-cull tweaks, depth
** precision, 16 bpp textures, performance, object key / glow; the glGetString strings and the fog tables.
*/
#include "wglpriv.h"
#include "og.h"
#include "glsi.h"
#include "glshade.h"
#include "icdglob.h"
#include "icdprotos.h"
#include <glide.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "si_names.h"
#include <math.h>

typedef char *(__stdcall *__GLregStrFn)(const char *name);
#define REGSTR(gc, name)    ((*(__GLregStrFn *)&(gc)->hwcx.grGetRegistryOrEnvironmentStringExt)(name))
/* the GETREGISTRY extension: a registry value or environment variable as a string, NULL when unset */
#define REG_OR_ENV_STRING(name) \
    ((char *)((const char *(__stdcall *)(const char *))gc->hwcx.grGetRegistryOrEnvironmentStringExt)(name))
extern float __fastcall hwFogTableIndexToW(int e);
extern void __fastcall hwDefaultGuardBand(GLuint *def, RECT *rect);

/* 690296b0 (size 194) -- hwLinearFogTable: linear fog table in eye-distance space (the FOGCOORD path): entry i
** covers w of index n - i; 255 * clamp(((65535 - min(w, 65535)) / 65535 - start) / (end - start)) */
void __fastcall hwLinearFogTable(GrFog_t *table, float start, float end)
{
    float tmp, scale, f;
    GLint n, i;

    if (end < start) {
        tmp = start;
        start = end;
        end = tmp;
    }
    n = grGetInteger(GR_FOG_TABLE_ENTRIES);
    scale = 1.0f / (end - start);
    i = 0;
    if (n > 0) {
        GLint j = n;
        do {
            f = (float)hwFogTableIndexToW(j);
        if (f > 65535.0f)
            f = 65535.0f;
        f = ((65535.0f - f) * 1.5259022e-05f - start) * scale;
        if (f > 1.0f)
            f = 1.0f;
        else if (f < 0.0f)
            f = 0.0f;
            table[i] = (GrFog_t)(f * 255.0f);
            i++;
            j--;
        } while (i < n);
    }
}

/* 69029780 -- hwFogTableIndexToW (Glide utility): the w of fog table entry e, 2^((e >> 2) + 3) / (8 - (e & 3)) */
float __fastcall hwFogTableIndexToW(int e)
{
  return (float)(pow(2.0, (double)(e >> 2) + 3.0)) / (8 - (e & 3));
}

/* 69029bc0 (size 346) -- hwDefaultGuardBand: default guard band (left, top, right, bottom multipliers) by desktop
** width */
void __fastcall hwDefaultGuardBand(GLuint *def, RECT *rect)
{
    GLfloat tab[7][4] = {
        { 14.0f, 14.0f, 14.0f, 14.0f },
        { 10.0f, 10.0f, 10.0f, 10.0f },
        {  8.0f,  8.0f,  8.0f,  8.0f },
        {  1.5f,  1.5f,  1.5f,  1.5f },
        {  1.0f,  1.0f,  1.0f,  1.0f },
        {  1.0f,  1.0f,  1.0f,  1.0f },
        {  1.0f,  1.0f,  1.0f,  1.0f },
    };
    GLfloat *out = (GLfloat *)def;
    GLint w = rect->right;
    GLint i;

    if (w > 1600)
        i = 6;
    else if (w > 1280)
        i = 5;
    else if (w > 1024)
        i = 4;
    else if (w > 800)
        i = 3;
    else if (w > 640)
        i = 2;
    else
        i = w > 512;
    out[0] = tab[i][0];
    out[1] = tab[i][1];
    out[2] = tab[i][2];
    out[3] = tab[i][3];
}

/* 69029dc1 (size 467) -- hwReadGuardBandTweaks: read the guard-band and Z-cull tweaks (FX_GL_* through Glide's
** registry / environment lookup).  Fix: FX_GL_GBBOTTOM's default goes into gbBottom (the original wrote gbLeft). */
void __fastcall hwReadGuardBandTweaks(__GLcontext *gc)
{
    GLuint def[4];                      /* left, top, right, bottom */
    RECT rect;
    char *s;

    s = REGSTR(gc, "FX_GL_GUARDBAND_ENABLED");
    if (s != NULL && *s != '0' && *s == '1')
        gc->fx.guardBand = 1;
    else
        gc->fx.guardBand = 0;
    if (gc->hwcx.hasTexFmtNapalm == 0)
        gc->fx.guardBand = 0;
    if (getenv("GL_DISABLE_GUARDBAND") != NULL)
        gc->fx.guardBand = 0;
    gc->fx.flags |= 0x20;
    if (gc->fx.guardBand == 1) {
        s = REGSTR(gc, "FX_GL_GBLEFT");
        GetWindowRect(GetDesktopWindow(), &rect);
        hwDefaultGuardBand(def, &rect);
        if (s != NULL)
            gc->fx.gbLeft = (GLfloat)atof(s);
        else
            *(GLuint *)&gc->fx.gbLeft = def[0];
        s = REGSTR(gc, "FX_GL_GBTOP");
        if (s != NULL)
            gc->fx.gbTop = (GLfloat)atof(s);
        else
            *(GLuint *)&gc->fx.gbTop = def[1];
        s = REGSTR(gc, "FX_GL_GBRIGHT");
        if (s != NULL)
            gc->fx.gbRight = (GLfloat)atof(s);
        else
            *(GLuint *)&gc->fx.gbRight = def[2];
        s = REGSTR(gc, "FX_GL_GBBOTTOM");
        if (s != NULL)
            gc->fx.gbBottom = (GLfloat)atof(s);
        else
            *(GLuint *)&gc->fx.gbBottom = def[3];
    }
    s = REGSTR(gc, "FX_GL_ZCULL_ENABLE");
    if (s != NULL && *s == '1')
        gc->fx.zcullEnable = 1;
    else
        gc->fx.zcullEnable = 0;
    if ((gc->constants.cpuCaps & 6) == 0)
        gc->fx.zcullEnable = 0;
    s = REGSTR(gc, "FX_GL_ZCULL_FAR");
    if (s != NULL)
        gc->fx.zcullFar = (GLfloat)atof(s);
    else
        gc->fx.zcullFar = 1.0f;
    s = REGSTR(gc, "FX_GL_ZCULL_NEAR");
    if (s != NULL)
        gc->fx.zcullNear = (GLfloat)atof(s);
    else
        gc->fx.zcullNear = 1.0f;
}

/* 69029fdf -- hwReadDepthPrecision: the 16 / 32 bit depth precision settings from the registry or environment
** ('0'..'2') */
void __fastcall hwReadDepthPrecision(__GLcontext *gc)
{
  char *s;

  gc->fx.zPrecision16 = 0;
  gc->fx.zPrecision32 = 0;
  s = REG_OR_ENV_STRING("FX_GL_16BPP_ZPRECISION");
  if (s != NULL) {
    if (*s == '0') gc->fx.zPrecision16 = 0;
    else if (*s == '1') gc->fx.zPrecision16 = 1;
    else if (*s == '2') gc->fx.zPrecision16 = 2;
  }
  else {
    gc->fx.zPrecision16 = 1;
  }
  s = REG_OR_ENV_STRING("FX_GL_32BPP_ZPRECISION");
  if (s != NULL) {
    if (*s == '0') gc->fx.zPrecision32 = 0;
    else if (*s == '1') gc->fx.zPrecision32 = 1;
    else if (*s == '2') gc->fx.zPrecision32 = 2;
  }
  else {
    gc->fx.zPrecision32 = 1;
  }
}

/* 6902a436 (size 841) -- hwBuildGetStrings: build the glGetString strings: vendor, version, renderer
** ("3Dfx/<hardware> [SLI]/<n> TMU(s)/<mem> MB[ SGRAM| SDRAM][/3DNow!][/KNI]/ICD|stand-alone (date)") and the GL and
** WGL extension strings */
void __fastcall hwBuildGetStrings(__GLcontext *gc)
{
    const char *hw;

    gc->constants.vendor = (const GLubyte *)"3Dfx Interactive Inc.";
    gc->constants.version = (const GLubyte *)"1.1.0";
    hw = grGetString(GR_HARDWARE);
    gc->constants.renderer = (const GLubyte *)__GL_MALLOC(gc, 0x100);
    if (gc->constants.renderer != NULL) {
        char *p = (char *)gc->constants.renderer;
        char *q, *w;
        const char *s;
        GLuint mem;

        p += sprintf(p, "3Dfx");
        p += sprintf(p, "/%s", hw);
        if (grGetInteger(GR_NUM_FB) > 1)
            p += sprintf(p, " SLI");
        p += sprintf(p, "/%d %s", grGetInteger(GR_NUM_TMU), grGetInteger(GR_NUM_TMU) == 1 ? "TMU" : "TMUs");
        if (grGetInteger(GR_MEMORY_UMA)) {
            mem = grGetInteger(GR_MEMORY_UMA);
            switch (grGetInteger(GR_GET_RESERVED_1)) {
              case 0:
                s = " SGRAM";
                break;
              case 1:
                s = " SDRAM";
                break;
              default:
                s = "";
                break;
            }
        } else {
            mem = grGetInteger(GR_MEMORY_FB);
            s = "";
        }
        if (mem > 0x400)
            mem >>= 20;
        p += sprintf(p, "/%d MB%s", mem, s);
        if (gc->constants.cpuCaps & 2)
            p += sprintf(p, "/3DNow!");
        if (gc->constants.cpuCaps & 4)
            p += sprintf(p, "/KNI");
        if (gc->modes.layerPlane)
            sprintf(p, "/stand-alone (Nov 21 2000)");
        else
            sprintf(p, "/ICD (Nov 21 2000)");

        q = (char *)gc->hwcx.raw_35bc4;
        w = gc->hwcx.extString;
        if (gc->fx.numTMUs > 1)
            q += sprintf(q, "GL_ARB_multitexture ");
        q += sprintf(q, "GL_EXT_abgr GL_EXT_bgra ");
        if (gc->hwcx.hasAlphaBlendExt)
            q += sprintf(q, "GL_EXT_blend_subtract GL_NV_blend_square ");
        q += sprintf(q, "GL_EXT_clip_volume_hint GL_EXT_compiled_vertex_array GL_EXT_packed_pixels "
                        "GL_EXT_point_parameters GL_EXT_stencil_wrap GL_EXT_texture_env_add "
                        "GL_EXT_vertex_array GL_SGIS_texture_edge_clamp ");
        if (gc->hwcx.hasCombineExt)
            q += sprintf(q, "GL_EXT_texture_env_combine ");
        if (gc->hwcx.hasTexFmtNapalm) {
            s = REGSTR(gc, "FX_GL_COMPRESS_LEGACY_TEXTURES");
            if (s == NULL || *s != '0')
                q += sprintf(q, "GL_S3_s3tc ");
            q += sprintf(q, "GL_EXT_texture_compression_s3tc ");
            q += sprintf(q, "GL_ARB_texture_compression ");
            q += sprintf(q, "GL_3DFX_texture_compression_FXT1 ");
        } else {
            q += sprintf(q, "GL_EXT_paletted_texture ");
            q += sprintf(q, "GL_EXT_shared_texture_palette ");
        }
        if (gc->fx.numTMUs > 1)
            q += sprintf(q, "GL_SGIS_multitexture ");
        if (gc->hwcx.hasFogCoord)
            q += sprintf(q, "GL_EXT_fog_coord ");
        w += sprintf(w, "WGL_ARB_pixel_format ");
        q += sprintf(q, "WGL_EXT_extensions_string ");
        w += sprintf(w, "WGL_EXT_extensions_string ");
        q += sprintf(q, "WGL_3DFX_gamma_control ");
        w += sprintf(w, "WGL_3DFX_gamma_control ");
        q += sprintf(q, "WGL_EXT_swap_control ");
        w += sprintf(w, "WGL_EXT_swap_control ");
        q += sprintf(q, "WGL_ARB_extensions_string ");
        w += sprintf(w, "WGL_ARB_extensions_string ");
        if (gc->hwcx.hasTBuffer) {
            q += sprintf(q, "GL_3DFX_multisample ");
            w += sprintf(w, "WGL_3DFX_multisample ");
        }
        sprintf(q, "");
        sprintf(w, "");
        gc->constants.extensions = (const GLubyte *)gc->hwcx.raw_35bc4;
    }
}

/* 6902a7ab -- hwReadForce16bppTextures: FX_GL_FORCE_16BPP_TEXTURES from the registry or environment: '1' forces 16 bpp
** textures, '2' 32 bpp; '0' or unset: by the rendering depth (hwTexelFormat) */
void __fastcall hwReadForce16bppTextures(__GLcontext *gc)
{
  char *s;

  s = REG_OR_ENV_STRING("FX_GL_FORCE_16BPP_TEXTURES");
  if (s != NULL && (*s == '1' || *s == '2')) gc->fx.force16bppTextures = *s - '0';
  else gc->fx.force16bppTextures = 0;
}

/* hwReadTrilinearMode: FX_GL_TRILINEAR_ENABLED from the registry or environment, how the *_MIPMAP_LINEAR filters
** are drawn: '1' LOD dithering, '2' split-TMU trilinear; '0' or unset: nearest mip level (bilinear).  Read once at
** Glide init (hwInitGlideExtensions): mode 2 makes the context single-texture, which the vertex layout, the
** extension string and the combine path depend on.  Without two TMUs in one texture memory it falls back to 1 */
void __fastcall hwReadTrilinearMode(__GLcontext *gc)
{
  const char *s;

  s = hwRegOrEnvString("FX_GL_TRILINEAR_ENABLED");
  if (s != NULL && (*s == '1' || *s == '2')) gc->fx.trilinearMode = *s - '0';
  else gc->fx.trilinearMode = 0;
  if (gc->fx.trilinearMode == 2 && (gc->hwcx.numTMU != 2 || !gc->hwcx.hasTexUMA))
      gc->fx.trilinearMode = 1;
  ICD_T("FX_GL_TRILINEAR_ENABLED mode (2 = split TMUs, one texture unit)", gc->fx.trilinearMode);
  ICD_T("  hardware TMUs", gc->hwcx.numTMU);
}

/* 6902a81e -- hwReadPerfSettings (size 232): FX_GL_SUPER, FX_GL_MIN_VERTEX_STRIDE (pending buffers) and FX_GL_HSR */
void __fastcall hwReadPerfSettings(__GLcontext *gc)
{
    char *s;
    GLint pending;

    s = REGSTR(gc, "FX_GL_SUPER");
    if (s != NULL)
        gc->fx.superMode = atoi(s);
    else
        gc->fx.superMode = 0;
    s = REGSTR(gc, "FX_GL_MIN_VERTEX_STRIDE");
    if (s != NULL)
        pending = atoi(s);
    else
        pending = 5;
    s = REGSTR(gc, "FX_GL_HSR");
    if (s != NULL) {
        if (*s == '0') {
            gc->fx.hsrEnable = 0;
            gc->fx.hsrLevel = 0;
        } else if (*s == '1') {
            gc->fx.hsrEnable = 1;
            gc->fx.hsrLevel = 4;
        } else if (*s == '2') {
            gc->fx.hsrEnable = 1;
            gc->fx.hsrLevel = 3;
        } else if (*s == '3') {
            gc->fx.hsrEnable = 1;
            gc->fx.hsrLevel = 2;
        } else if (*s == '4') {
            gc->fx.hsrEnable = 1;
            gc->fx.hsrLevel = 1;
        }
    } else {
        gc->fx.hsrLevel = 0;
        gc->fx.hsrEnable = 0;
    }
    if (gc->fx.hsrEnable == 1)
        grSetNumPendingBuffers(pending);
}

/* 6902a948 -- hwReadObjectKeyGlow: object key / glow colour / glow worm from the registry or environment */
void __fastcall hwReadObjectKeyGlow(__GLcontext *gc)
{
  char *s;

  s = REG_OR_ENV_STRING("FX_GL_OBJECT_KEY");
  if (s != NULL) {
    gc->fx.objectKey = atoi(s);
  }
  else {
    gc->fx.objectKey = 0;
  }
  s = REG_OR_ENV_STRING("FX_GL_GLOW_COLOR");
  if (s != NULL) {
    gc->fx.glowColor = atoi(s);
  }
  else {
    gc->fx.glowColor = 0xffffffff;
  }
  s = REG_OR_ENV_STRING("FX_GL_GLOW_WORM");
  if (s != NULL) {
    gc->fx.glowWorm = atoi(s);
  }
  else {
    gc->fx.glowWorm = 0;
  }
}

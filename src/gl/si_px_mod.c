/*
** si_px_mod.c -- ICD functions written from the SGI Sample Implementation source
** (ref/ogl-sample gfx/lib/opengl/glcore/px_mod.c).
*/
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"

/* 690e28f0 -- __glSpanModifyDepth */
void __fastcall __glSpanModifyDepth(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    __GLfloat dbias;
    __GLfloat dscale;
    const GLfloat *oldData;
    GLfloat *newData;
    GLfloat d;
    GLfloat one, zero;
    GLint i;
    GLint width;

    dbias = gc->state.pixel.transferMode.depthBias;
    dscale = gc->state.pixel.transferMode.depthScale;
    one = __GL_ONE_F;
    zero = __GL_ZERO_F;

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        d = *oldData++ * dscale + dbias;
        if (d < zero) d = zero;
        else if (d > one) d = one;
        *newData++ = d;
    }
}

/* 690e0680 -- __glBuildRGBAModifyTables */
void __fastcall __glBuildRGBAModifyTables(__GLcontext *gc, __GLpixelMachineICD *pm)
{
    GLfloat *redMap, *greenMap, *blueMap, *alphaMap;
    GLint rrsizeMinusOne, ggsizeMinusOne, 
          bbsizeMinusOne, aasizeMinusOne;
    __GLpixelMapHead *rrmap, *ggmap, 
                     *bbmap, *aamap;
    GLboolean mapColor;
    __GLfloat rbias, gbias, bbias, abias;
    GLint entry;
    __GLfloat rscale, gscale, bscale, ascale;
    GLint i;
    __GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;
    pm->rgbaCurrent = GL_TRUE;

    redMap = pm->redModMap;
    if (redMap == NULL) {
        /* First time allocation of these maps */
        redMap = pm->redModMap = (GLfloat*)
            ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->greenModMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->blueModMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->alphaModMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
    }
    greenMap = pm->greenModMap;
    blueMap = pm->blueModMap;
    alphaMap = pm->alphaModMap;

    rbias = gc->state.pixel.transferMode.bias.r;
    gbias = gc->state.pixel.transferMode.bias.g;
    bbias = gc->state.pixel.transferMode.bias.b;
    abias = gc->state.pixel.transferMode.bias.a;
    rscale = gc->state.pixel.transferMode.scale.r;
    gscale = gc->state.pixel.transferMode.scale.g;
    bscale = gc->state.pixel.transferMode.scale.b;
    ascale = gc->state.pixel.transferMode.scale.a;
    if (mapColor) {
        rrmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_R_TO_R];
        rrsizeMinusOne = rrmap->size - 1;
        ggmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_G_TO_G];
        ggsizeMinusOne = ggmap->size - 1;
        bbmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_B_TO_B];
        bbsizeMinusOne = bbmap->size - 1;
        aamap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_A_TO_A];
        aasizeMinusOne = aamap->size - 1;
    }

    for (i=0; i<256; i++) {
        alpha = red = green = blue = i / 255.0;

        red = red * rscale + rbias;
        green = green * gscale + gbias;
        blue = blue * bscale + bbias;
        alpha = alpha * ascale + abias;
        if (mapColor) {
            entry = (GLint)(red * rrsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > rrsizeMinusOne) entry = rrsizeMinusOne;
            red = rrmap->base.mapF[entry];

            entry = (GLint)(green * ggsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > ggsizeMinusOne) entry = ggsizeMinusOne;
            green = ggmap->base.mapF[entry];

            entry = (GLint)(blue * bbsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > bbsizeMinusOne) entry = bbsizeMinusOne;
            blue = bbmap->base.mapF[entry];

            entry = (GLint)(alpha * aasizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > aasizeMinusOne) entry = aasizeMinusOne;
            alpha = aamap->base.mapF[entry];
        } else {
            if (red > __GL_ONE_F) red = __GL_ONE_F;
            else if (red < __GL_ZERO_F) red = __GL_ZERO_F;
            if (green > __GL_ONE_F) green = __GL_ONE_F;
            else if (green < __GL_ZERO_F) green = __GL_ZERO_F;
            if (blue > __GL_ONE_F) blue = __GL_ONE_F;
            else if (blue < __GL_ZERO_F) blue = __GL_ZERO_F;
            if (alpha > __GL_ONE_F) alpha = __GL_ONE_F;
            else if (alpha < __GL_ZERO_F) alpha = __GL_ZERO_F;
        }

        redMap[i] = red * gc->frontBuffer.redScale;
        greenMap[i] = green * gc->frontBuffer.greenScale;
        blueMap[i] = blue * gc->frontBuffer.blueScale;
        alphaMap[i] = alpha * gc->frontBuffer.alphaScale;
    }
}

/* 690e09c0 -- __glBuildItoIModifyTables */
void __fastcall __glBuildItoIModifyTables(__GLcontext *gc, __GLpixelMachineICD *pm)
{
    GLint indexOffset, indexShift;
    __GLfloat indexScale;
    __GLpixelMapHead *iimap;
    GLint iimask;
    GLboolean mapColor;
    GLfloat *indexMap;
    GLint i;
    GLint entry;
    __GLfloat index;
    GLint mask;

    mapColor = gc->state.pixel.transferMode.mapColor;
    mask = gc->frontBuffer.redMax;
    pm->iToICurrent = GL_TRUE;

    indexMap = pm->iToIMap;
    if (indexMap == NULL) {
        indexMap = pm->iToIMap = (GLfloat*)
            ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
    }

    indexOffset = gc->state.pixel.transferMode.indexOffset;
    indexShift = gc->state.pixel.transferMode.indexShift;
    if (indexShift >= 0) {
        indexScale = (__GLfloat) ((GLuint) (1 << indexShift));
    } else {
        indexScale = __GL_ONE_F/(GLuint) (1 << (-indexShift));
    }

    if (mapColor) {
        iimap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_I_TO_I];
        iimask = iimap->size - 1;
    }

    for (i=0; i<256; i++) {
        index = i * indexScale + indexOffset;
        if (mapColor) {
            if (index < 0) {
                entry = (GLint)(index - __glHalf);
            } else {
                entry = (GLint)(index + __glHalf);
            }
            index = (__GLfloat) (iimap->base.mapI[entry & iimask]);
        }
        indexMap[i] = (GLfloat) (((GLint) index) & mask);
    }
}

/* 690e0ae0 -- __glBuildItoRGBAModifyTables */
void __fastcall __glBuildItoRGBAModifyTables(__GLcontext *gc, __GLpixelMachineICD *pm)
{
    GLint indexOffset, indexShift;
    __GLfloat indexScale;
    __GLpixelMapHead *irmap, *igmap, *ibmap, *iamap;
    GLint irmask, igmask, ibmask, iamask;
    GLfloat *redMap, *greenMap, *blueMap, *alphaMap;
    __GLfloat index;
    GLint entry;
    GLint i;

    pm->iToRGBACurrent = GL_TRUE;

    redMap = pm->iToRMap;
    if (redMap == NULL) {
        /* First time allocation of these maps */
        redMap = pm->iToRMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->iToGMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->iToBMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
        pm->iToAMap =
            (GLfloat*) ((void *(__fastcall *)(__GLcontext *, size_t))gc->imports[0])(gc, 256 * sizeof(GLfloat));
    }
    greenMap = pm->iToGMap;
    blueMap = pm->iToBMap;
    alphaMap = pm->iToAMap;

    indexOffset = gc->state.pixel.transferMode.indexOffset;
    indexShift = gc->state.pixel.transferMode.indexShift;
    if (indexShift >= 0) {
        indexScale = (__GLfloat) ((GLuint) (1 << indexShift));
    } else {
        indexScale = __GL_ONE_F/(GLuint) (1 << (-indexShift));
    }

    irmap = 
        &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_I_TO_R];
    irmask = irmap->size - 1;
    igmap = 
        &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_I_TO_G];
    igmask = igmap->size - 1;
    ibmap = 
        &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_I_TO_B];
    ibmask = ibmap->size - 1;
    iamap = 
        &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_I_TO_A];
    iamask = iamap->size - 1;

    for (i=0; i<256; i++) {
        index = i * indexScale + indexOffset;
        if (index < 0) {
            entry = (GLint)(index - __glHalf);
        } else {
            entry = (GLint)(index + __glHalf);
        }
        redMap[i] = irmap->base.mapF[entry & irmask] * 
                gc->frontBuffer.redScale;
        greenMap[i] = igmap->base.mapF[entry & igmask] *
                gc->frontBuffer.greenScale;
        blueMap[i] = ibmap->base.mapF[entry & ibmask] * 
                gc->frontBuffer.blueScale;
        alphaMap[i] = iamap->base.mapF[entry & iamask] *
                gc->frontBuffer.alphaScale;
    }
}

/* 690e16b0 -- __glSpanModifyRed */
void __fastcall __glSpanModifyRed(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                       const GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachineICD *pm;
    __GLfloat rbias;
    __GLfloat rscale;
    __GLpixelMapHead *rrmap;        /* the ICD leaves these uninitialised */
    GLint rrsizeMinusOne;
    GLboolean mapColor;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    pm = &(gc->pixel);
    green = pm->green0Mod;
    blue = pm->blue0Mod;
    alpha = pm->alpha1Mod;
    if (mapColor) {
        rbias = gc->state.pixel.transferMode.bias.r;
        rscale = gc->state.pixel.transferMode.scale.r;
        rrmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_R_TO_R];
        rrsizeMinusOne = rrmap->size - 1;
    } else {
        rbias = gc->state.pixel.transferMode.bias.r *
                gc->frontBuffer.redScale;
        rscale = gc->state.pixel.transferMode.scale.r *
                gc->frontBuffer.redScale;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        red = *oldData++ * rscale + rbias;
        if (mapColor) {
            entry = (GLint)(red * rrsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > rrsizeMinusOne) entry = rrsizeMinusOne;

            *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;
        } else {
            if (red > gc->frontBuffer.redScale) {
                red = gc->frontBuffer.redScale;
            } else if (red < __GL_ZERO_F) red = __GL_ZERO_F;

            *newData++ = red;
        }

        *newData++ = green;
        *newData++ = blue;
        *newData++ = alpha;
    }
}

/* 690e17f0 -- __glSpanModifyGreen */
void __fastcall __glSpanModifyGreen(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachineICD *pm;
    __GLfloat gbias;
    __GLfloat gscale;
    __GLpixelMapHead *ggmap;        /* the ICD leaves these uninitialised */
    GLint ggsizeMinusOne;
    GLboolean mapColor;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    pm = &(gc->pixel);
    red = pm->red0Mod;
    blue = pm->blue0Mod;
    alpha = pm->alpha1Mod;
    if (mapColor) {
        gbias = gc->state.pixel.transferMode.bias.g;
        gscale = gc->state.pixel.transferMode.scale.g;
        ggmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_G_TO_G];
        ggsizeMinusOne = ggmap->size - 1;
    } else {
        gbias = gc->state.pixel.transferMode.bias.g *
                gc->frontBuffer.greenScale;
        gscale = gc->state.pixel.transferMode.scale.g *
                gc->frontBuffer.greenScale;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        green = *oldData++ * gscale + gbias;
        *newData++ = red;
        if (mapColor) {
            entry = (GLint)(green * ggsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > ggsizeMinusOne) entry = ggsizeMinusOne;
            *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;
        } else {
            if (green > gc->frontBuffer.greenScale) {
                green = gc->frontBuffer.greenScale;
            } else if (green < __GL_ZERO_F) green = __GL_ZERO_F;

            *newData++ = green;
        }

        *newData++ = blue;
        *newData++ = alpha;
    }
}

/* 690e1930 -- __glSpanModifyBlue */
void __fastcall __glSpanModifyBlue(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                        const GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachineICD *pm;
    __GLfloat bbias;
    __GLfloat bscale;
    __GLpixelMapHead *bbmap;        /* the ICD leaves these uninitialised */
    GLint bbsizeMinusOne;
    GLboolean mapColor;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    pm = &(gc->pixel);
    red = pm->red0Mod;
    green = pm->green0Mod;
    alpha = pm->alpha1Mod;
    if (mapColor) {
        bbias = gc->state.pixel.transferMode.bias.b;
        bscale = gc->state.pixel.transferMode.scale.b;
        bbmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_B_TO_B];
        bbsizeMinusOne = bbmap->size - 1;
    } else {
        bbias = gc->state.pixel.transferMode.bias.b *
                gc->frontBuffer.blueScale;
        bscale = gc->state.pixel.transferMode.scale.b *
                gc->frontBuffer.blueScale;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        blue = *oldData++ * bscale + bbias;
        *newData++ = red;
        *newData++ = green;
        if (mapColor) {
            entry = (GLint)(blue * bbsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > bbsizeMinusOne) entry = bbsizeMinusOne;
            *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;
        } else {
            if (blue > gc->frontBuffer.blueScale) {
                blue = gc->frontBuffer.blueScale;
            } else if (blue < __GL_ZERO_F) blue = __GL_ZERO_F;

            *newData++ = blue;
        }

        *newData++ = alpha;
    }
}

/* 690e1a70 -- __glSpanModifyAlpha */
void __fastcall __glSpanModifyAlpha(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                         const GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachineICD *pm;
    __GLfloat abias;
    __GLfloat ascale;
    __GLpixelMapHead *aamap;        /* the ICD leaves these uninitialised */
    GLint aasizeMinusOne;
    GLboolean mapColor;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    pm = &(gc->pixel);
    red = pm->red0Mod;
    green = pm->green0Mod;
    blue = pm->blue0Mod;
    if (mapColor) {
        abias = gc->state.pixel.transferMode.bias.a;
        ascale = gc->state.pixel.transferMode.scale.a;
        aamap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_A_TO_A];
        aasizeMinusOne = aamap->size - 1;
    } else {
        abias = gc->state.pixel.transferMode.bias.a *
                gc->frontBuffer.alphaScale;
        ascale = gc->state.pixel.transferMode.scale.a *
                gc->frontBuffer.alphaScale;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        alpha = *oldData++ * ascale + abias;
        *newData++ = red;
        *newData++ = green;
        *newData++ = blue;
        if (mapColor) {
            entry = (GLint)(alpha * aasizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > aasizeMinusOne) entry = aasizeMinusOne;
            *newData++ = aamap->base.mapF[entry] * gc->frontBuffer.alphaScale;
        } else {
            if (alpha > gc->frontBuffer.alphaScale) {
                alpha = gc->frontBuffer.alphaScale;
            } else if (alpha < __GL_ZERO_F) alpha = __GL_ZERO_F;

            *newData++ = alpha;
        }
    }
}

/* 690e2970 -- __glSpanModifyStencil */
void __fastcall __glSpanModifyStencil(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                           const GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMapHead *ssmap;
    GLint ssmask;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLboolean mapStencil;
    GLint indexOffset, indexShift;

    indexOffset = gc->state.pixel.transferMode.indexOffset;
    indexShift = gc->state.pixel.transferMode.indexShift;
    mapStencil = gc->state.pixel.transferMode.mapStencil;
    if (mapStencil) {
        ssmap = &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_S_TO_S];
        ssmask = ssmap->size - 1;
    }

    /*
    ** Large positive shifts cause accuracy problems when casting to
    ** GLfloat, so shift beyond the reach of the mask when appropriate.
    */
    if (indexShift > gc->modes.stencilBits) {
        indexShift = indexShift % (sizeof(GLint)*8);
        if (indexShift > gc->modes.stencilBits)
            indexShift = gc->modes.stencilBits;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;

    /* Do a separate loop for each case to avoid tests inside the loop */
    if (indexShift >= 0) {
        if (mapStencil) {
            for (i=0; i<width; i++) {
                entry = ((GLint)*oldData++ << indexShift) + indexOffset;
                *newData++ = (GLfloat) (ssmap->base.mapI[entry & ssmask]);
            }
        } else /* mapping off */ {
            for (i=0; i<width; i++) {
                *newData++ = (GLfloat) (((GLint)*oldData++ << indexShift) + indexOffset);
            }
        }
    } else /* negative shift */ {
        if (mapStencil) {
            for (i=0; i<width; i++) {
                entry = ((GLint)*oldData++ >> -indexShift) + indexOffset;
                *newData++ = (GLfloat) (ssmap->base.mapI[entry & ssmask]);
            }
        } else /* mapping off */ {
            for (i=0; i<width; i++) {
                *newData++ = (GLfloat) (((GLint)*oldData++ >> -indexShift) + indexOffset);
            }
        }
    }
}

/* 690e0c90 -- __glSpanModifyRGBA */
void __fastcall __glSpanModifyRGBA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                        const GLvoid *inspan, GLvoid *outspan)
{
    __GLfloat rbias, gbias, bbias, abias;
    __GLfloat rscale, gscale, bscale, ascale;
    __GLpixelMapHead *rrmap, *ggmap, 
                     *bbmap, *aamap;
    GLint rrsizeMinusOne, ggsizeMinusOne, 
          bbsizeMinusOne, aasizeMinusOne;
    GLboolean mapColor;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    if (mapColor) {
        rbias = gc->state.pixel.transferMode.bias.r;
        gbias = gc->state.pixel.transferMode.bias.g;
        bbias = gc->state.pixel.transferMode.bias.b;
        abias = gc->state.pixel.transferMode.bias.a;
        rscale = gc->state.pixel.transferMode.scale.r;
        gscale = gc->state.pixel.transferMode.scale.g;
        bscale = gc->state.pixel.transferMode.scale.b;
        ascale = gc->state.pixel.transferMode.scale.a;

        rrmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_R_TO_R];
        rrsizeMinusOne = rrmap->size - 1;
        ggmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_G_TO_G];
        ggsizeMinusOne = ggmap->size - 1;
        bbmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_B_TO_B];
        bbsizeMinusOne = bbmap->size - 1;
        aamap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_A_TO_A];
        aasizeMinusOne = aamap->size - 1;
    } else {
        rbias = gc->state.pixel.transferMode.bias.r * 
                gc->frontBuffer.redScale;
        gbias = gc->state.pixel.transferMode.bias.g * 
                gc->frontBuffer.greenScale;
        bbias = gc->state.pixel.transferMode.bias.b *
                gc->frontBuffer.blueScale;
        abias = gc->state.pixel.transferMode.bias.a *
                gc->frontBuffer.alphaScale;
        rscale = gc->state.pixel.transferMode.scale.r *
                gc->frontBuffer.redScale;
        gscale = gc->state.pixel.transferMode.scale.g *
                gc->frontBuffer.greenScale;
        bscale = gc->state.pixel.transferMode.scale.b *
                gc->frontBuffer.blueScale;
        ascale = gc->state.pixel.transferMode.scale.a *
                gc->frontBuffer.alphaScale;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        red = *oldData++ * rscale + rbias;
        green = *oldData++ * gscale + gbias;
        blue = *oldData++ * bscale + bbias;
        alpha = *oldData++ * ascale + abias;
        if (mapColor) {
            entry = (GLint)(red * rrsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > rrsizeMinusOne) entry = rrsizeMinusOne;
            *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

            entry = (GLint)(green * ggsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > ggsizeMinusOne) entry = ggsizeMinusOne;
            *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;

            entry = (GLint)(blue * bbsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > bbsizeMinusOne) entry = bbsizeMinusOne;
            *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;

            entry = (GLint)(alpha * aasizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > aasizeMinusOne) entry = aasizeMinusOne;
            *newData++ = aamap->base.mapF[entry] * gc->frontBuffer.alphaScale;
        } else {
            if (red > gc->frontBuffer.redScale) {
                red = gc->frontBuffer.redScale;
            } else if (red < __GL_ZERO_F) red = __GL_ZERO_F;

            if (green > gc->frontBuffer.greenScale) {
                green = gc->frontBuffer.greenScale;
            } else if (green < __GL_ZERO_F) green = __GL_ZERO_F;

            if (blue > gc->frontBuffer.blueScale) {
                blue = gc->frontBuffer.blueScale;
            } else if (blue < __GL_ZERO_F) blue = __GL_ZERO_F;

            if (alpha > gc->frontBuffer.alphaScale) {
                alpha = gc->frontBuffer.alphaScale;
            } else if (alpha < __GL_ZERO_F) alpha = __GL_ZERO_F;

            *newData++ = red;
            *newData++ = green;
            *newData++ = blue;
            *newData++ = alpha;
        }
    }
}

/* 690e0ff0 -- __glSpanModifyABGR */
void __fastcall __glSpanModifyABGR(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                        const GLvoid *inspan, GLvoid *outspan)
{
    __GLfloat rbias, gbias, bbias, abias;
    __GLfloat rscale, gscale, bscale, ascale;
    __GLpixelMapHead *rrmap, *ggmap, 
                     *bbmap, *aamap;
    GLint rrsizeMinusOne, ggsizeMinusOne, 
          bbsizeMinusOne, aasizeMinusOne;
    GLboolean mapColor;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    if (mapColor) {
        rbias = gc->state.pixel.transferMode.bias.r;
        gbias = gc->state.pixel.transferMode.bias.g;
        bbias = gc->state.pixel.transferMode.bias.b;
        abias = gc->state.pixel.transferMode.bias.a;
        rscale = gc->state.pixel.transferMode.scale.r;
        gscale = gc->state.pixel.transferMode.scale.g;
        bscale = gc->state.pixel.transferMode.scale.b;
        ascale = gc->state.pixel.transferMode.scale.a;

        rrmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_R_TO_R];
        rrsizeMinusOne = rrmap->size - 1;
        ggmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_G_TO_G];
        ggsizeMinusOne = ggmap->size - 1;
        bbmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_B_TO_B];
        bbsizeMinusOne = bbmap->size - 1;
        aamap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_A_TO_A];
        aasizeMinusOne = aamap->size - 1;
    } else {
        rbias = gc->state.pixel.transferMode.bias.r * 
                gc->frontBuffer.redScale;
        gbias = gc->state.pixel.transferMode.bias.g * 
                gc->frontBuffer.greenScale;
        bbias = gc->state.pixel.transferMode.bias.b *
                gc->frontBuffer.blueScale;
        abias = gc->state.pixel.transferMode.bias.a *
                gc->frontBuffer.alphaScale;
        rscale = gc->state.pixel.transferMode.scale.r *
                gc->frontBuffer.redScale;
        gscale = gc->state.pixel.transferMode.scale.g *
                gc->frontBuffer.greenScale;
        bscale = gc->state.pixel.transferMode.scale.b *
                gc->frontBuffer.blueScale;
        ascale = gc->state.pixel.transferMode.scale.a *
                gc->frontBuffer.alphaScale;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        alpha = *oldData++ * ascale + abias;
        blue = *oldData++ * bscale + bbias;
        green = *oldData++ * gscale + gbias;
        red = *oldData++ * rscale + rbias;
        if (mapColor) {
            entry = (GLint)(red * rrsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > rrsizeMinusOne) entry = rrsizeMinusOne;
            *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

            entry = (GLint)(green * ggsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > ggsizeMinusOne) entry = ggsizeMinusOne;
            *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;

            entry = (GLint)(blue * bbsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > bbsizeMinusOne) entry = bbsizeMinusOne;
            *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;

            entry = (GLint)(alpha * aasizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > aasizeMinusOne) entry = aasizeMinusOne;
            *newData++ = aamap->base.mapF[entry] * gc->frontBuffer.alphaScale;
        } else {
            if (red > gc->frontBuffer.redScale) {
                red = gc->frontBuffer.redScale;
            } else if (red < __GL_ZERO_F) red = __GL_ZERO_F;

            if (green > gc->frontBuffer.greenScale) {
                green = gc->frontBuffer.greenScale;
            } else if (green < __GL_ZERO_F) green = __GL_ZERO_F;

            if (blue > gc->frontBuffer.blueScale) {
                blue = gc->frontBuffer.blueScale;
            } else if (blue < __GL_ZERO_F) blue = __GL_ZERO_F;

            if (alpha > gc->frontBuffer.alphaScale) {
                alpha = gc->frontBuffer.alphaScale;
            } else if (alpha < __GL_ZERO_F) alpha = __GL_ZERO_F;

            *newData++ = red;
            *newData++ = green;
            *newData++ = blue;
            *newData++ = alpha;
        }
    }
}

/* 690e1350 -- __glSpanModifyBGRA */
void __fastcall __glSpanModifyBGRA(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                        const GLvoid *inspan, GLvoid *outspan)
{
    __GLfloat rbias, gbias, bbias, abias;
    __GLfloat rscale, gscale, bscale, ascale;
    __GLpixelMapHead *rrmap, *ggmap, 
                     *bbmap, *aamap;
    GLint rrsizeMinusOne, ggsizeMinusOne, 
          bbsizeMinusOne, aasizeMinusOne;
    GLboolean mapColor;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    if (mapColor) {
        rbias = gc->state.pixel.transferMode.bias.r;
        gbias = gc->state.pixel.transferMode.bias.g;
        bbias = gc->state.pixel.transferMode.bias.b;
        abias = gc->state.pixel.transferMode.bias.a;
        rscale = gc->state.pixel.transferMode.scale.r;
        gscale = gc->state.pixel.transferMode.scale.g;
        bscale = gc->state.pixel.transferMode.scale.b;
        ascale = gc->state.pixel.transferMode.scale.a;

        rrmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_R_TO_R];
        rrsizeMinusOne = rrmap->size - 1;
        ggmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_G_TO_G];
        ggsizeMinusOne = ggmap->size - 1;
        bbmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_B_TO_B];
        bbsizeMinusOne = bbmap->size - 1;
        aamap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_A_TO_A];
        aasizeMinusOne = aamap->size - 1;
    } else {
        rbias = gc->state.pixel.transferMode.bias.r * 
                gc->frontBuffer.redScale;
        gbias = gc->state.pixel.transferMode.bias.g * 
                gc->frontBuffer.greenScale;
        bbias = gc->state.pixel.transferMode.bias.b *
                gc->frontBuffer.blueScale;
        abias = gc->state.pixel.transferMode.bias.a *
                gc->frontBuffer.alphaScale;
        rscale = gc->state.pixel.transferMode.scale.r *
                gc->frontBuffer.redScale;
        gscale = gc->state.pixel.transferMode.scale.g *
                gc->frontBuffer.greenScale;
        bscale = gc->state.pixel.transferMode.scale.b *
                gc->frontBuffer.blueScale;
        ascale = gc->state.pixel.transferMode.scale.a *
                gc->frontBuffer.alphaScale;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        blue = *oldData++ * bscale + bbias;
        green = *oldData++ * gscale + gbias;
        red = *oldData++ * rscale + rbias;
        alpha = *oldData++ * ascale + abias;
        if (mapColor) {
            entry = (GLint)(red * rrsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > rrsizeMinusOne) entry = rrsizeMinusOne;
            *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

            entry = (GLint)(green * ggsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > ggsizeMinusOne) entry = ggsizeMinusOne;
            *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;

            entry = (GLint)(blue * bbsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > bbsizeMinusOne) entry = bbsizeMinusOne;
            *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;

            entry = (GLint)(alpha * aasizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > aasizeMinusOne) entry = aasizeMinusOne;
            *newData++ = aamap->base.mapF[entry] * gc->frontBuffer.alphaScale;
        } else {
            if (red > gc->frontBuffer.redScale) {
                red = gc->frontBuffer.redScale;
            } else if (red < __GL_ZERO_F) red = __GL_ZERO_F;

            if (green > gc->frontBuffer.greenScale) {
                green = gc->frontBuffer.greenScale;
            } else if (green < __GL_ZERO_F) green = __GL_ZERO_F;

            if (blue > gc->frontBuffer.blueScale) {
                blue = gc->frontBuffer.blueScale;
            } else if (blue < __GL_ZERO_F) blue = __GL_ZERO_F;

            if (alpha > gc->frontBuffer.alphaScale) {
                alpha = gc->frontBuffer.alphaScale;
            } else if (alpha < __GL_ZERO_F) alpha = __GL_ZERO_F;

            *newData++ = red;
            *newData++ = green;
            *newData++ = blue;
            *newData++ = alpha;
        }
    }
}

/* 690e1bb0 -- __glSpanModifyRGB */
void __fastcall __glSpanModifyRGB(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                       const GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachineICD *pm;
    __GLfloat rbias, gbias, bbias;
    __GLfloat rscale, gscale, bscale;
    __GLpixelMapHead *rrmap, *ggmap, 
                     *bbmap;
    GLint rrsizeMinusOne, ggsizeMinusOne, 
          bbsizeMinusOne;
    GLboolean mapColor;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    pm = &(gc->pixel);
    mapColor = gc->state.pixel.transferMode.mapColor;

    alpha = pm->alpha1Mod;

    if (mapColor) {
        rbias = gc->state.pixel.transferMode.bias.r;
        gbias = gc->state.pixel.transferMode.bias.g;
        bbias = gc->state.pixel.transferMode.bias.b;
        rscale = gc->state.pixel.transferMode.scale.r;
        gscale = gc->state.pixel.transferMode.scale.g;
        bscale = gc->state.pixel.transferMode.scale.b;

        rrmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_R_TO_R];
        rrsizeMinusOne = rrmap->size - 1;
        ggmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_G_TO_G];
        ggsizeMinusOne = ggmap->size - 1;
        bbmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_B_TO_B];
        bbsizeMinusOne = bbmap->size - 1;
    } else {
        rbias = gc->state.pixel.transferMode.bias.r * 
                gc->frontBuffer.redScale;
        gbias = gc->state.pixel.transferMode.bias.g * 
                gc->frontBuffer.greenScale;
        bbias = gc->state.pixel.transferMode.bias.b *
                gc->frontBuffer.blueScale;
        rscale = gc->state.pixel.transferMode.scale.r *
                gc->frontBuffer.redScale;
        gscale = gc->state.pixel.transferMode.scale.g *
                gc->frontBuffer.greenScale;
        bscale = gc->state.pixel.transferMode.scale.b *
                gc->frontBuffer.blueScale;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        red = *oldData++ * rscale + rbias;
        green = *oldData++ * gscale + gbias;
        blue = *oldData++ * bscale + bbias;
        if (mapColor) {
            entry = (GLint)(red * rrsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > rrsizeMinusOne) entry = rrsizeMinusOne;
            *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

            entry = (GLint)(green * ggsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > ggsizeMinusOne) entry = ggsizeMinusOne;
            *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;

            entry = (GLint)(blue * bbsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > bbsizeMinusOne) entry = bbsizeMinusOne;
            *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;
        } else {
            if (red > gc->frontBuffer.redScale) {
                red = gc->frontBuffer.redScale;
            } else if (red < __GL_ZERO_F) red = __GL_ZERO_F;

            if (green > gc->frontBuffer.greenScale) {
                green = gc->frontBuffer.greenScale;
            } else if (green < __GL_ZERO_F) green = __GL_ZERO_F;

            if (blue > gc->frontBuffer.blueScale) {
                blue = gc->frontBuffer.blueScale;
            } else if (blue < __GL_ZERO_F) blue = __GL_ZERO_F;

            *newData++ = red;
            *newData++ = green;
            *newData++ = blue;
        }

        *newData++ = alpha;
    }
}

/* 690e1e50 -- __glSpanModifyBGR */
void __fastcall __glSpanModifyBGR(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                       const GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachineICD *pm;
    __GLfloat rbias, gbias, bbias;
    __GLfloat rscale, gscale, bscale;
    __GLpixelMapHead *rrmap, *ggmap, 
                     *bbmap;
    GLint rrsizeMinusOne, ggsizeMinusOne, 
          bbsizeMinusOne;
    GLboolean mapColor;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    pm = &(gc->pixel);
    mapColor = gc->state.pixel.transferMode.mapColor;

    alpha = pm->alpha1Mod;

    if (mapColor) {
        rbias = gc->state.pixel.transferMode.bias.r;
        gbias = gc->state.pixel.transferMode.bias.g;
        bbias = gc->state.pixel.transferMode.bias.b;
        rscale = gc->state.pixel.transferMode.scale.r;
        gscale = gc->state.pixel.transferMode.scale.g;
        bscale = gc->state.pixel.transferMode.scale.b;

        rrmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_R_TO_R];
        rrsizeMinusOne = rrmap->size - 1;
        ggmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_G_TO_G];
        ggsizeMinusOne = ggmap->size - 1;
        bbmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_B_TO_B];
        bbsizeMinusOne = bbmap->size - 1;
    } else {
        rbias = gc->state.pixel.transferMode.bias.r * 
                gc->frontBuffer.redScale;
        gbias = gc->state.pixel.transferMode.bias.g * 
                gc->frontBuffer.greenScale;
        bbias = gc->state.pixel.transferMode.bias.b *
                gc->frontBuffer.blueScale;
        rscale = gc->state.pixel.transferMode.scale.r *
                gc->frontBuffer.redScale;
        gscale = gc->state.pixel.transferMode.scale.g *
                gc->frontBuffer.greenScale;
        bscale = gc->state.pixel.transferMode.scale.b *
                gc->frontBuffer.blueScale;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        blue = *oldData++ * bscale + bbias;
        green = *oldData++ * gscale + gbias;
        red = *oldData++ * rscale + rbias;
        if (mapColor) {
            entry = (GLint)(red * rrsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > rrsizeMinusOne) entry = rrsizeMinusOne;
            *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

            entry = (GLint)(green * ggsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > ggsizeMinusOne) entry = ggsizeMinusOne;
            *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;

            entry = (GLint)(blue * bbsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > bbsizeMinusOne) entry = bbsizeMinusOne;
            *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;
        } else {
            if (red > gc->frontBuffer.redScale) {
                red = gc->frontBuffer.redScale;
            } else if (red < __GL_ZERO_F) red = __GL_ZERO_F;

            if (green > gc->frontBuffer.greenScale) {
                green = gc->frontBuffer.greenScale;
            } else if (green < __GL_ZERO_F) green = __GL_ZERO_F;

            if (blue > gc->frontBuffer.blueScale) {
                blue = gc->frontBuffer.blueScale;
            } else if (blue < __GL_ZERO_F) blue = __GL_ZERO_F;

            *newData++ = red;
            *newData++ = green;
            *newData++ = blue;
        }

        *newData++ = alpha;
    }
}

/* 690e20f0 -- __glSpanModifyLuminance */
void __fastcall __glSpanModifyLuminance(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                             const GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachineICD *pm;
    __GLfloat rbias, gbias, bbias;
    __GLfloat rscale, gscale, bscale;
    __GLpixelMapHead *rrmap, *ggmap, 
                     *bbmap;
    GLint rrsizeMinusOne, ggsizeMinusOne, 
          bbsizeMinusOne;
    GLboolean mapColor;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    pm = &(gc->pixel);
    mapColor = gc->state.pixel.transferMode.mapColor;

    alpha = pm->alpha1Mod;

    if (mapColor) {
        rbias = gc->state.pixel.transferMode.bias.r;
        gbias = gc->state.pixel.transferMode.bias.g;
        bbias = gc->state.pixel.transferMode.bias.b;
        rscale = gc->state.pixel.transferMode.scale.r;
        gscale = gc->state.pixel.transferMode.scale.g;
        bscale = gc->state.pixel.transferMode.scale.b;

        rrmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_R_TO_R];
        rrsizeMinusOne = rrmap->size - 1;
        ggmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_G_TO_G];
        ggsizeMinusOne = ggmap->size - 1;
        bbmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_B_TO_B];
        bbsizeMinusOne = bbmap->size - 1;
    } else {
        rbias = gc->state.pixel.transferMode.bias.r * 
                gc->frontBuffer.redScale;
        gbias = gc->state.pixel.transferMode.bias.g * 
                gc->frontBuffer.greenScale;
        bbias = gc->state.pixel.transferMode.bias.b *
                gc->frontBuffer.blueScale;
        rscale = gc->state.pixel.transferMode.scale.r *
                gc->frontBuffer.redScale;
        gscale = gc->state.pixel.transferMode.scale.g *
                gc->frontBuffer.greenScale;
        bscale = gc->state.pixel.transferMode.scale.b *
                gc->frontBuffer.blueScale;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        red = *oldData * rscale + rbias;
        green = *oldData * gscale + gbias;
        blue = *oldData++ * bscale + bbias;
        if (mapColor) {
            entry = (GLint)(red * rrsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > rrsizeMinusOne) entry = rrsizeMinusOne;
            *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

            entry = (GLint)(green * ggsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > ggsizeMinusOne) entry = ggsizeMinusOne;
            *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;

            entry = (GLint)(blue * bbsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > bbsizeMinusOne) entry = bbsizeMinusOne;
            *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;
        } else {
            if (red > gc->frontBuffer.redScale) {
                red = gc->frontBuffer.redScale;
            } else if (red < __GL_ZERO_F) red = __GL_ZERO_F;

            if (green > gc->frontBuffer.greenScale) {
                green = gc->frontBuffer.greenScale;
            } else if (green < __GL_ZERO_F) green = __GL_ZERO_F;

            if (blue > gc->frontBuffer.blueScale) {
                blue = gc->frontBuffer.blueScale;
            } else if (blue < __GL_ZERO_F) blue = __GL_ZERO_F;

            *newData++ = red;
            *newData++ = green;
            *newData++ = blue;
        }

        *newData++ = alpha;
    }
}

/* 690e2390 -- __glSpanModifyLuminanceAlpha */
void __fastcall __glSpanModifyLuminanceAlpha(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                                  const GLvoid *inspan, GLvoid *outspan)
{
    __GLfloat rbias, gbias, bbias, abias;
    __GLfloat rscale, gscale, bscale, ascale;
    __GLpixelMapHead *rrmap, *ggmap, 
                     *bbmap, *aamap;
    GLint rrsizeMinusOne, ggsizeMinusOne, 
          bbsizeMinusOne, aasizeMinusOne;
    GLboolean mapColor;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    if (mapColor) {
        rbias = gc->state.pixel.transferMode.bias.r;
        gbias = gc->state.pixel.transferMode.bias.g;
        bbias = gc->state.pixel.transferMode.bias.b;
        abias = gc->state.pixel.transferMode.bias.a;
        rscale = gc->state.pixel.transferMode.scale.r;
        gscale = gc->state.pixel.transferMode.scale.g;
        bscale = gc->state.pixel.transferMode.scale.b;
        ascale = gc->state.pixel.transferMode.scale.a;

        rrmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_R_TO_R];
        rrsizeMinusOne = rrmap->size - 1;
        ggmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_G_TO_G];
        ggsizeMinusOne = ggmap->size - 1;
        bbmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_B_TO_B];
        bbsizeMinusOne = bbmap->size - 1;
        aamap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_A_TO_A];
        aasizeMinusOne = aamap->size - 1;
    } else {
        rbias = gc->state.pixel.transferMode.bias.r * 
                gc->frontBuffer.redScale;
        gbias = gc->state.pixel.transferMode.bias.g * 
                gc->frontBuffer.greenScale;
        bbias = gc->state.pixel.transferMode.bias.b *
                gc->frontBuffer.blueScale;
        abias = gc->state.pixel.transferMode.bias.a *
                gc->frontBuffer.alphaScale;
        rscale = gc->state.pixel.transferMode.scale.r *
                gc->frontBuffer.redScale;
        gscale = gc->state.pixel.transferMode.scale.g *
                gc->frontBuffer.greenScale;
        bscale = gc->state.pixel.transferMode.scale.b *
                gc->frontBuffer.blueScale;
        ascale = gc->state.pixel.transferMode.scale.a *
                gc->frontBuffer.alphaScale;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        red = *oldData * rscale + rbias;
        green = *oldData * gscale + gbias;
        blue = *oldData++ * bscale + bbias;
        alpha = *oldData++ * ascale + abias;
        if (mapColor) {
            entry = (GLint)(red * rrsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > rrsizeMinusOne) entry = rrsizeMinusOne;
            *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

            entry = (GLint)(green * ggsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > ggsizeMinusOne) entry = ggsizeMinusOne;
            *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;

            entry = (GLint)(blue * bbsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > bbsizeMinusOne) entry = bbsizeMinusOne;
            *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;

            entry = (GLint)(alpha * aasizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > aasizeMinusOne) entry = aasizeMinusOne;
            *newData++ = aamap->base.mapF[entry] * gc->frontBuffer.alphaScale;
        } else {
            if (red > gc->frontBuffer.redScale) {
                red = gc->frontBuffer.redScale;
            } else if (red < __GL_ZERO_F) red = __GL_ZERO_F;

            if (green > gc->frontBuffer.greenScale) {
                green = gc->frontBuffer.greenScale;
            } else if (green < __GL_ZERO_F) green = __GL_ZERO_F;

            if (blue > gc->frontBuffer.blueScale) {
                blue = gc->frontBuffer.blueScale;
            } else if (blue < __GL_ZERO_F) blue = __GL_ZERO_F;

            if (alpha > gc->frontBuffer.alphaScale) {
                alpha = gc->frontBuffer.alphaScale;
            } else if (alpha < __GL_ZERO_F) alpha = __GL_ZERO_F;

            *newData++ = red;
            *newData++ = green;
            *newData++ = blue;
            *newData++ = alpha;
        }
    }
}

/* 690e26f0 -- __glSpanModifyRedAlpha */
void __fastcall __glSpanModifyRedAlpha(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                            const GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachineICD *pm;
    __GLfloat rbias, abias;
    __GLfloat rscale, ascale;
    __GLpixelMapHead *rrmap, *aamap;
    GLint rrsizeMinusOne, aasizeMinusOne;
    GLboolean mapColor;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    pm = &(gc->pixel);

    green = pm->green0Mod;
    blue = pm->blue0Mod;
    if (mapColor) {
        rbias = gc->state.pixel.transferMode.bias.r;
        abias = gc->state.pixel.transferMode.bias.a;
        rscale = gc->state.pixel.transferMode.scale.r;
        ascale = gc->state.pixel.transferMode.scale.a;

        rrmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_R_TO_R];
        rrsizeMinusOne = rrmap->size - 1;
        aamap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_A_TO_A];
        aasizeMinusOne = aamap->size - 1;
    } else {
        rbias = gc->state.pixel.transferMode.bias.r * 
                gc->frontBuffer.redScale;
        abias = gc->state.pixel.transferMode.bias.a *
                gc->frontBuffer.alphaScale;
        rscale = gc->state.pixel.transferMode.scale.r *
                gc->frontBuffer.redScale;
        ascale = gc->state.pixel.transferMode.scale.a *
                gc->frontBuffer.alphaScale;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        red = *oldData * rscale + rbias;
        alpha = *oldData++ * ascale + abias;
        if (mapColor) {
            entry = (GLint)(red * rrsizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > rrsizeMinusOne) entry = rrsizeMinusOne;
            *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

            *newData++ = green;
            *newData++ = blue;

            entry = (GLint)(alpha * aasizeMinusOne + __glHalf);
            if (entry < 0) entry = 0;
            else if (entry > aasizeMinusOne) entry = aasizeMinusOne;
            *newData++ = aamap->base.mapF[entry] * gc->frontBuffer.alphaScale;
        } else {
            if (red > gc->frontBuffer.redScale) {
                red = gc->frontBuffer.redScale;
            } else if (red < __GL_ZERO_F) red = __GL_ZERO_F;

            if (alpha > gc->frontBuffer.alphaScale) {
                alpha = gc->frontBuffer.alphaScale;
            } else if (alpha < __GL_ZERO_F) alpha = __GL_ZERO_F;

            *newData++ = red;
            *newData++ = green;
            *newData++ = blue;
            *newData++ = alpha;
        }
    }
}

/* 690e2af0 -- __glSpanModifyCI */
void __fastcall __glSpanModifyCI(__GLcontext *gc, __GLpixelSpanInfoICD *spanInfo,
                      const GLvoid *inspan, GLvoid *outspan)
{
    GLint indexOffset, indexShift;
    __GLpixelMapHead *iimap, *irmap, 
                     *igmap, *ibmap, 
                     *iamap;
    GLint iimask, irmask, igmask, 
          ibmask, iamask;
    GLfloat rscale, gscale, 
            bscale, ascale;
    GLboolean mapColor;
    const GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;

    mapColor = gc->state.pixel.transferMode.mapColor;

    indexOffset = gc->state.pixel.transferMode.indexOffset;
    indexShift = gc->state.pixel.transferMode.indexShift;
    if (spanInfo->dstFormat != GL_COLOR_INDEX) {
        irmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_I_TO_R];
        irmask = irmap->size - 1;
        igmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_I_TO_G];
        igmask = igmap->size - 1;
        ibmap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_I_TO_B];
        ibmask = ibmap->size - 1;
        iamap = 
            &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_I_TO_A];
        iamask = iamap->size - 1;
        rscale = gc->frontBuffer.redScale;
        gscale = gc->frontBuffer.greenScale;
        bscale = gc->frontBuffer.blueScale;
        ascale = gc->frontBuffer.alphaScale;
    } else {
        if (mapColor) {
            iimap = &((__GLpixelMapHead *)gc->state.pixel.map)[__GL_PIXEL_MAP_I_TO_I];
            iimask = iimap->size - 1;
        }
    }

    /*
    ** Large positive shifts cause accuracy problems when casting to
    ** GLfloat, so shift beyond the reach of the mask when appropriate.
    */
    if (indexShift > gc->modes.indexBits) {
        indexShift = indexShift % (sizeof(GLint)*8);
        if (indexShift > gc->modes.indexBits) indexShift = gc->modes.indexBits;
    }

    oldData = (const GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->width;
    for (i=0; i<width; i++) {
        if (indexShift >= 0) {
            entry = ((GLint)*oldData++ << indexShift) + indexOffset;
        } else {
            entry = ((GLint)*oldData++ >> -indexShift) + indexOffset;
        }
        if (spanInfo->dstFormat != GL_COLOR_INDEX) {
            *newData++ = irmap->base.mapF[entry & irmask] * rscale;
            *newData++ = igmap->base.mapF[entry & igmask] * gscale;
            *newData++ = ibmap->base.mapF[entry & ibmask] * bscale;
            *newData++ = iamap->base.mapF[entry & iamask] * ascale;
        } else if (mapColor) {
            *newData++ = (GLfloat) (iimap->base.mapI[entry & iimask]);
        } else {
            *newData++ = (GLfloat) entry;
        }
    }
}

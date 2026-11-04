/*
** wglfont.c -- wglUseFontBitmaps / wglUseFontOutlines (stand-alone implementations).
** Outline glyphs are parsed from GetGlyphOutline(GGO_NATIVE) TTPOLYGON data and
** tessellated with GLU loaded at run time.
*/
#include <string.h>
#include <math.h>
#ifndef _GDI32_
#define _GDI32_            /* we ARE the GL/WGL exporter: no dllimport on wgl* and gl* */
#endif
#include "wglpriv.h"

/* ------------------------------------------------------------- GLU loading */
HMODULE g_hGlu;                                   /* 690f6614 */
static const GLubyte *(APIENTRY *p_gluErrorString)(GLenum);                 /* 690f6644 */
static void *(APIENTRY *p_gluNewTess)(void);                                /* 690f662c */
static void  (APIENTRY *p_gluDeleteTess)(void *);                           /* 690f6640 */
static void  (APIENTRY *p_gluTessBeginPolygon)(void *, void *);             /* 690f6650 */
static void  (APIENTRY *p_gluTessBeginContour)(void *);                     /* 690f664c */
static void  (APIENTRY *p_gluTessVertex)(void *, GLdouble *, void *);       /* 690f660c */
static void  (APIENTRY *p_gluTessEndContour)(void *);                       /* 690f6634 */
static void  (APIENTRY *p_gluTessEndPolygon)(void *);                       /* 690f6630 */
static void  (APIENTRY *p_gluTessProperty)(void *, GLenum, GLdouble);       /* 690f6654 */
static void  (APIENTRY *p_gluTessNormal)(void *, GLdouble, GLdouble, GLdouble); /* 690f6618 */
static void  (APIENTRY *p_gluTessCallback)(void *, GLenum, void (*)());     /* 690f663c */

/* 6900e6dc -- load GLU32.DLL (or GLU.DLL when we are running as "opengl.dll") */
static BOOL wglLoadGLU(void)
{
    CHAR path[256];
    DWORD n;
    const char *lib;
    char *base;

    if (g_hGlu != NULL) return TRUE;
    lib = "GLU32.DLL";
    n = GetModuleFileNameA(g_hInstance, path, 0xff);
    if (n != 0) {
        base = path + n - 1;
        while (base > path && *base != '\\') base--;
        if (*base == '\\') base++;
        if (_stricmp(base, "opengl.dll") == 0) lib = "GLU.DLL";
    }
    g_hGlu = LoadLibraryA(lib);
    if (g_hGlu == NULL) return FALSE;
#define LOADGLU(var, name) if ((*(FARPROC *)&var = GetProcAddress(g_hGlu, name)) == NULL) return FALSE;
    LOADGLU(p_gluErrorString,      "gluErrorString");
    LOADGLU(p_gluNewTess,          "gluNewTess");
    LOADGLU(p_gluDeleteTess,       "gluDeleteTess");
    LOADGLU(p_gluTessBeginPolygon, "gluTessBeginPolygon");
    LOADGLU(p_gluTessBeginContour, "gluTessBeginContour");
    LOADGLU(p_gluTessVertex,       "gluTessVertex");
    LOADGLU(p_gluTessEndContour,   "gluTessEndContour");
    LOADGLU(p_gluTessEndPolygon,   "gluTessEndPolygon");
    LOADGLU(p_gluTessProperty,     "gluTessProperty");
    LOADGLU(p_gluTessNormal,       "gluTessNormal");
    LOADGLU(p_gluTessCallback,     "gluTessCallback");
#undef LOADGLU
    return TRUE;
}

/* ------------------------------------------------------- glyph outline buffers */
/*
** The outline of one glyph is flattened into g_outline[]: for each contour the number
** of points followed by x,y pairs (floats in em units * g_emScale); g_outline[0] is
** the contour count.  Combined vertices from the tessellator go into g_combined[].
*/
static float  g_emScale;            /* 690f6610  1/otmEMSquare */
static float *g_outline;            /* 690f6628 */
static GLuint g_outlineCap;         /* 690f6624 */
static GLuint g_outlineLen;         /* 690f6620 */
static float *g_combined;           /* 690f6648 */
static GLuint g_combinedCap;        /* 690f6638 */
static GLuint g_combinedLen;        /* 690f6608 */
static GLenum g_tessError;          /* 690f6604 */
static HFONT  g_scaledFont;         /* 690f661c */
static HGDIOBJ g_oldFont;           /* 690f6658 */
static const MAT2 g_identityMat = { {0,1}, {0,0}, {0,0}, {0,1} };   /* 690ee9c8 */

/* 6900edf0 -- wglOutlineInit: allocate g_outline[] (4000 floats, wglMalloc) and empty it; FALSE if that fails */
static BOOL wglOutlineInit(void)                       /* 6900edf0 */
{
    g_outlineCap = 4000;
    g_outline = (float *)wglMalloc(4000 * sizeof(float));
    if (g_outline == NULL) return FALSE;
    g_outlineLen = 0;
    return TRUE;
}
/* 6900ee20 -- wglCombinedInit: allocate g_combined[] (4000 floats, wglMalloc) and empty it; FALSE if that fails */
static BOOL wglCombinedInit(void)                      /* 6900ee20 */
{
    g_combinedCap = 4000;
    g_combined = (float *)wglMalloc(4000 * sizeof(float));
    if (g_combined == NULL) return FALSE;
    g_combinedLen = 0;
    return TRUE;
}
/* 6900ee50 -- wglOutlinePush: append v to g_outline[], growing it by 4000 floats (wglRealloc) when full; FALSE if
** the realloc fails (g_outlineCap is raised anyway) */
static BOOL __fastcall wglOutlinePush(float v)         /* 6900ee50 (value is the 3rd arg) */
{
    if (g_outlineLen >= g_outlineCap) {
        float *p;
        g_outlineCap += 4000;
        p = (float *)wglRealloc(g_outline, g_outlineCap * sizeof(float));
        if (p == NULL) return FALSE;
        g_outline = p;
    }
    g_outline[g_outlineLen] = v;
    g_outlineLen++;
    return TRUE;
}
/* 6900eeb0 -- wglCombinedPush: append v to g_combined[]; FALSE when it is full (unlike g_outline it never grows) */
static BOOL __fastcall wglCombinedPush(float v)        /* 6900eeb0 */
{
    if (g_combinedLen >= g_combinedCap) return FALSE;
    g_combined[g_combinedLen++] = v;
    return TRUE;
}
/* 6900eef0 -- wglOutlineFree: free g_outline[] if allocated and clear the pointer */
static void wglOutlineFree(void)  { if (g_outline)  { wglFree(g_outline);  g_outline  = NULL; } }   /* 6900eef0 */
/* 6900ef10 -- wglCombinedFree: free g_combined[] if allocated and clear the pointer */
static void wglCombinedFree(void) { if (g_combined) { wglFree(g_combined); g_combined = NULL; } }   /* 6900ef10 */

/* little-endian readers over the TTPOLYGON stream */
static int   __fastcall rdWord(const BYTE **p)  { int v = (*p)[1] * 0x100 + (*p)[0]; *p += 2; return v; }     /* 6900ef30 */
/* 6900ef50 -- rdDword: read a little-endian 32-bit int at *p and advance *p by 4 */
static int   __fastcall rdDword(const BYTE **p) { int v = (((*p)[3] * 0x100 + (*p)[2]) * 0x100 + (*p)[1]) * 0x100 + (*p)[0]; *p += 4; return v; } /* 6900ef50 */
/* 6900ef80 -- rdFixed: read a FIXED (fract WORD, then value WORD) at *p as a float times g_emScale */
static float __fastcall rdFixed(const BYTE **p)                                                                /* 6900ef80 */
{
    int lo = rdWord(p), hi = rdWord(p);
    return ((float)((hi << 16) | lo) * (1.0 / 65536.0)) * g_emScale;   /* FIXED -> float (double constant first) */
}

/* 6900ec90 -- subdivide a quadratic spline until flat enough (chordal deviation^2 < tol) */
static BOOL __fastcall wglSpline(float x0, float y0, float x1, float y1, float x2, float y2,
                                 GLint vertexCountIndex, float chordalDeviationSquared)
{
    float x01, y01, x12, y12, midPointX, midPointY, deltaX, deltaY;

    /* midpoint of the curve by de Casteljau */
    x01 = 0.5f * (x0 + x1);
    y01 = 0.5f * (y0 + y1);
    x12 = 0.5f * (x1 + x2);
    y12 = 0.5f * (y1 + y2);
    midPointX = 0.5f * (x01 + x12);
    midPointY = 0.5f * (y01 + y12);

    deltaX = midPointX - x1;
    deltaY = midPointY - y1;
    if (deltaX * deltaX + deltaY * deltaY > chordalDeviationSquared) {
        wglSpline(x0, y0, x01, y01, midPointX, midPointY, vertexCountIndex, chordalDeviationSquared);
        wglSpline(midPointX, midPointY, x12, y12, x2, y2, vertexCountIndex, chordalDeviationSquared);
    } else {
        /* the "pen" is already at (x0, y0) */
        if (!wglOutlinePush(x1) || !wglOutlinePush(y1) || !wglOutlinePush(x2) || !wglOutlinePush(y2))
            return FALSE;
        g_outline[vertexCountIndex] += 2.0f;
    }
    return TRUE;
}
/* 6900eaf0 -- TT_PRIM_LINE: wglPolyLine: add n to the point count at g_outline[contour], then append the n points
** read from *p (FIXED x, y scaled by g_emScale); FALSE if the outline buffer cannot grow */
static BOOL __fastcall wglPolyLine(const BYTE **p, GLint contour, unsigned short n)
{
    g_outline[contour] += (float)n;
    while (n-- != 0) {
        if (!wglOutlinePush(rdFixed(p))) return FALSE;
        if (!wglOutlinePush(rdFixed(p))) return FALSE;
    }
    return TRUE;
}
/* 6900eb70 -- TT_PRIM_QSPLINE: wglPolyQSpline: flatten the n-point quadratic B-spline at *p into the contour:
** each of its n-1 segments runs from the last outline point through the next control point to the midpoint of
** the following two controls (the last one to the final point; *p is wound back to re-read each shared control)
** and is subdivided by wglSpline to within deviation; FALSE if the outline buffer cannot grow */
static BOOL __fastcall wglPolyQSpline(const BYTE **p, GLint contour, unsigned short n, float deviation)
{
    unsigned short i;
    for (i = 0; i < n - 1; i++) {
        float x0 = g_outline[g_outlineLen - 2], y0 = g_outline[g_outlineLen - 1];
        float cx = rdFixed(p), cy = rdFixed(p), x1, y1;
        if (i == n - 2) {
            x1 = rdFixed(p); y1 = rdFixed(p);
        } else {
            x1 = (rdFixed(p) + cx) * 0.5f;
            y1 = (rdFixed(p) + cy) * 0.5f;
            *p -= 8;                       /* re-read the point as the next segment's control point */
        }
        if (!wglSpline(x0, y0, cx, cy, x1, y1, contour, deviation * deviation)) return FALSE;
    }
    return TRUE;
}
/* 6900eaa0 -- one TTPOLYCURVE: read its type and count, hand it to wglPolyLine / wglPolyQSpline; FALSE for others */
static BOOL __fastcall wglPolyCurve(const BYTE **p, GLint contour, float deviation)
{
    short type = (short)rdWord(p);
    unsigned short n = (unsigned short)rdWord(p);
    if (type == TT_PRIM_LINE)    return wglPolyLine(p, contour, n);
    if (type == TT_PRIM_QSPLINE) return wglPolyQSpline(p, contour, n, deviation);
    return FALSE;
}
/* 6900e9f0 -- one TTPOLYGONHEADER + its curves */
static BOOL __fastcall wglPolygon(const BYTE **p, float deviation)
{
    GLint contour = g_outlineLen;
    const BYTE *start = *p;
    int cb, type;
    if (!wglOutlinePush(0.0f)) return FALSE;          /* point count slot */
    cb = rdDword(p);
    type = rdDword(p);
    if (type != TT_POLYGON_TYPE) return FALSE;
    if (!wglOutlinePush(rdFixed(p))) return FALSE;     /* pfxStart */
    if (!wglOutlinePush(rdFixed(p))) return FALSE;
    g_outline[contour] += 1.0f;
    while (*p < start + cb) {
        if (!wglPolyCurve(p, contour, deviation)) return FALSE;
    }
    return TRUE;
}
/* 6900e990 -- whole glyph: outline[0] = number of contours */
static BOOL __fastcall wglParseGlyph(const BYTE *data, DWORD size, float deviation)
{
    const BYTE *p;
    BOOL status = FALSE;

    if (!wglOutlinePush(0.0f))                  /* loop count at g_outline[0] */
        goto done;
    p = data;
    while (p < data + size) {
        if (!wglPolygon(&p, deviation))
            goto done;
        g_outline[0] += 1.0f;                   /* increment loop count */
    }
    status = TRUE;
done:
    return status;
}

/* ------------------------------------------------------ tessellator callbacks */
/* 6900e8d0 -- tessVertex: GLU_TESS_VERTEX_DATA: glVertex3fv of the vertex's x, y and z (the polygon data's bits) */
static void CALLBACK tessVertex(void *v, void *z)                 /* 6900e8d0 */
{
    float xyz[3];
    xyz[0] = ((float *)v)[0]; xyz[1] = ((float *)v)[1]; xyz[2] = *(float *)&z;
    glVertex3fv(xyz);
}
/* 6900e980 -- tessError: GLU_ERROR: keep the error in g_tessError */
static void CALLBACK tessError(GLenum e) { g_tessError = e; }    /* 6900e980 */
/* 6900e900 -- tessCombine: GLU_TESS_COMBINE: append the new vertex to g_combined[] and return its last 3 floats */
static void CALLBACK tessCombine(GLdouble coords[3], void *d[4], GLfloat w[4], void **out)   /* 6900e900 */
{
    (void)d; (void)w;
    if (!wglCombinedPush((float)coords[0]) || !wglCombinedPush((float)coords[1]) || !wglCombinedPush((float)coords[2]))
        g_tessError = GL_OUT_OF_MEMORY;
    *out = g_combined + g_combinedLen - 3;
}

/* 6900e220 -- emit one glyph: WGL_FONT_LINES as line loops, WGL_FONT_POLYGONS through
** the GLU tessellator plus extruded sides when 'extrusion' != 0.  Returns TRUE on
** success.  3dfx's DrawGlyph (GLIDE3X/WGL/WGLFONTO.C, the Microsoft wglUseFontOutlines
** sample) with the ICD's buffer helpers.
*/
static BOOL __fastcall wglEmitGlyph(const BYTE *glyphBuf, DWORD glyphSize, float chordalDeviation,
                                    float extrusion, int format)
{
    int status = 0;
    float *p;
    DWORD loop;
    DWORD point;
    void *tess = NULL;

    /* the global buffer into which the outlines are placed */
    if (!wglOutlineInit())
	goto exit;

    /* convert the glyph outlines to a set of polyline loops */
    if (!wglParseGlyph(glyphBuf, glyphSize, chordalDeviation))
	goto exit;
    p = g_outline;

    if (format == WGL_FONT_LINES) {
	for (loop = (DWORD)*p++; loop; --loop) {
	    glBegin(GL_LINE_LOOP);
	    for (point = (DWORD)*p++; point; --point) {
		glVertex2fv(p);
		p += 2;
	    }
	    glEnd();
	}
	status = 1;
    } else if (format == WGL_FONT_POLYGONS) {
	double v[3];
	float *save_p = p;
	GLfloat z_value;

	if (!wglLoadGLU())
	    goto exit;
	if (!wglCombinedInit())
	    goto exit;
	if (!(tess = p_gluNewTess()))
	    goto exit;
	p_gluTessCallback(tess, 100100 /*GLU_BEGIN*/, (void (*)())glBegin);
	p_gluTessCallback(tess, 100107 /*GLU_TESS_VERTEX_DATA*/, (void (*)())tessVertex);
	p_gluTessCallback(tess, 100102 /*GLU_END*/, (void (*)())glEnd);
	p_gluTessCallback(tess, 100103 /*GLU_ERROR*/, (void (*)())tessError);
	p_gluTessCallback(tess, 100105 /*GLU_TESS_COMBINE*/, (void (*)())tessCombine);
	p_gluTessNormal(tess, 0.0F, 0.0F, 1.0F);

	g_tessError = 0;
	glNormal3f(0.0f, 0.0f, 1.0f);
	v[2] = 0.0;
	z_value = 0.0f;

	p_gluTessBeginPolygon(tess, (void *)*(int *)&z_value);
	for (loop = (DWORD)*p++; loop; --loop) {
	    p_gluTessBeginContour(tess);
	    for (point = (DWORD)*p++; point; --point) {
		v[0] = p[0];
		v[1] = p[1];
		p_gluTessVertex(tess, v, p);
		p += 2;
	    }
	    p_gluTessEndContour(tess);
	}
	p_gluTessEndPolygon(tess);

	status = !g_tessError;

	/* extrusion */
	if (extrusion) {
	    DWORD loops;
	    GLfloat thickness = (GLfloat)-extrusion;
	    float *vert, *vert2;
	    DWORD count;

	    p = save_p;
	    loops = (DWORD)*p++;

	    for (loop = 0; loop < loops; loop++) {
		GLfloat dx, dy, len;
		DWORD last;

		count = (DWORD)*p++;
		glBegin(GL_QUAD_STRIP);

		/* first and last vertex identical: don't draw the same quad twice */
		vert = p + (count - 1) * 2;
		last = (p[0] == vert[0] && p[1] == vert[1]) ? count - 1 : count;

		for (point = 0; point <= last; point++) {
		    vert  = p + 2 * (point % last);
		    vert2 = p + 2 * ((point + 1) % last);

		    dx = vert[0] - vert2[0];
		    dy = vert[1] - vert2[1];
		    len = (GLfloat)sqrt(dx * dx + dy * dy);

		    glNormal3f(dy / len, -dx / len, 0.0f);
		    glVertex3f((GLfloat)vert[0], (GLfloat)vert[1], thickness);
		    glVertex3f((GLfloat)vert[0], (GLfloat)vert[1], 0.0f);
		}

		glEnd();
		p += count * 2;
	    }

	    /* the back face */
	    p = save_p;
	    v[2] = thickness;
	    glNormal3f(0.0f, 0.0f, -1.0f);
	    p_gluTessNormal(tess, 0.0F, 0.0F, -1.0F);

	    p_gluTessBeginPolygon(tess, (void *)*(int *)&thickness);

	    for (loop = (DWORD)*p++; loop; --loop) {
		count = (DWORD)*p++;

		p_gluTessBeginContour(tess);

		for (point = 0; point < count; point++) {
		    vert = p + ((count - point - 1) << 1);
		    v[0] = vert[0];
		    v[1] = vert[1];
		    p_gluTessVertex(tess, v, vert);
		}
		p += count * 2;

		p_gluTessEndContour(tess);
	    }
	    p_gluTessEndPolygon(tess);
	}
    }

exit:
    wglOutlineFree();
    if (tess)
	p_gluDeleteTess(tess);
    wglCombinedFree();
    return status;
}

/* 6900e1d0 -- one glyph display list */
static BOOL __fastcall wglGlyphList(GLuint list, const BYTE *data, DWORD size, const GLYPHMETRICSFLOAT *gmf,
                                    float deviation, float extrusion, int format)
{
    BOOL ok;
    glNewList(list, GL_COMPILE);
    ok = wglEmitGlyph(data, size, deviation, extrusion, format);
    glTranslatef(gmf->gmfCellIncX, gmf->gmfCellIncY, 0.0f);
    glEndList();
    return ok;
}

/* 6900e090 -- create a copy of the DC's font scaled to its em square */
static HFONT __fastcall wglCreateEmFont(HDC hdc)
{
    UINT n;
    LPOUTLINETEXTMETRICA otm;
    LOGFONTA lf;
    LONG fontHeight, fontWidth, fontUnits;

    n = GetOutlineTextMetricsA(hdc, 0, NULL);
    if (n == 0) return NULL;
    otm = (LPOUTLINETEXTMETRICA)wglMalloc(n);
    if (otm == NULL) return NULL;
    otm->otmSize = n;
    if (GetOutlineTextMetricsA(hdc, n, otm) == 0) return NULL;
    fontHeight = otm->otmTextMetrics.tmHeight - otm->otmTextMetrics.tmInternalLeading;
    fontWidth = otm->otmTextMetrics.tmAveCharWidth;
    fontUnits = (LONG)otm->otmEMSquare;
    g_emScale = 1.0f / (FLOAT)fontUnits;
    lf.lfHeight = -((LONG)fontUnits);
    lf.lfWidth  = (LONG)((FLOAT)(fontWidth * fontUnits) / (FLOAT)fontHeight);
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight    = otm->otmTextMetrics.tmWeight;
    lf.lfItalic    = otm->otmTextMetrics.tmItalic;
    lf.lfUnderline = otm->otmTextMetrics.tmUnderlined;
    lf.lfStrikeOut = otm->otmTextMetrics.tmStruckOut;
    lf.lfCharSet   = otm->otmTextMetrics.tmCharSet;
    lf.lfOutPrecision  = OUT_OUTLINE_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality       = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = otm->otmTextMetrics.tmPitchAndFamily & 0xf0;
    strcpy(lf.lfFaceName, (char *)otm + (UINT_PTR)otm->otmpFaceName);
    g_scaledFont = CreateFontIndirectA(&lf);
    if (g_scaledFont == NULL) return NULL;
    wglFree(otm);
    return g_scaledFont;
}

/* 6900de90 -- wglUseFontOutlinesA: wglSAUseFontOutlinesA: clear pending GL errors, select a copy of the DC's font
** scaled to its em square, and for each of the count glyphs from first get its GGO_NATIVE outline, fill gmf[] (in
** em units; gmf is used unchecked) and compile list listBase + i (wglGlyphList, chordal deviation passed as
** g_emScale + deviation); then restore the font and return TRUE when GL raised no error.  The scaled font is never
** deleted, and the early FALSE returns leave it selected */
BOOL WINAPI wglSAUseFontOutlinesA(HDC hdc, DWORD first, DWORD count, DWORD listBase, FLOAT deviation,
                                  FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT gmf)
{
    GLYPHMETRICS gm;
    LPVOID buf;
    DWORD cap, size;
    UINT ch;

    while (glGetError() != GL_NO_ERROR) ;
    g_scaledFont = wglCreateEmFont(hdc);
    if (g_scaledFont == NULL) return FALSE;
    g_oldFont = SelectObject(hdc, g_scaledFont);
    if (g_oldFont == NULL) return FALSE;
    cap = 0x2800;
    buf = wglMalloc(cap);
    if (buf == NULL) return FALSE;
    for (ch = first; ch - first < count; ch++) {
        LPGLYPHMETRICSFLOAT m = &gmf[ch - first];
        size = GetGlyphOutlineA(hdc, ch, GGO_NATIVE, &gm, 0, NULL, &g_identityMat);
        if (size > cap) {
            wglFree(buf);
            cap = size;
            buf = wglMalloc(size);
            if (buf == NULL) return FALSE;
        }
        GetGlyphOutlineA(hdc, ch, GGO_NATIVE, &gm, cap, buf, &g_identityMat);
        m->gmfBlackBoxX = (float)gm.gmBlackBoxX * g_emScale;
        m->gmfBlackBoxY = (float)gm.gmBlackBoxY * g_emScale;
        m->gmfptGlyphOrigin.x = (float)gm.gmptGlyphOrigin.x * g_emScale;
        m->gmfptGlyphOrigin.y = (float)gm.gmptGlyphOrigin.y * g_emScale;
        m->gmfCellIncX = (float)gm.gmCellIncX * g_emScale;
        m->gmfCellIncY = (float)gm.gmCellIncY * g_emScale;
        if (!wglGlyphList(listBase + (ch - first), (const BYTE *)buf, size, m, g_emScale + deviation, extrusion, format)) {
            wglFree(buf);
            return FALSE;
        }
    }
    wglFree(buf);
    SelectObject(hdc, g_oldFont);
    if (glGetError() == GL_NO_ERROR) return TRUE;
    while (glGetError() != GL_NO_ERROR) ;
    return FALSE;
}

/* 6900de60 -- W variant just forwards (the original passes the A arguments through) */
BOOL WINAPI wglSAUseFontOutlinesW(HDC hdc, DWORD first, DWORD count, DWORD listBase, FLOAT deviation,
                                  FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT gmf)
{
    return wglSAUseFontOutlinesA(hdc, first, count, listBase, deviation, extrusion, format, gmf);
}

/* 6900f240 -- flip a 1-bpp glyph bitmap vertically (GDI top-down -> GL bottom-up), rows padded to 32 bits */
static void __fastcall wglFlipBitmap(int width, int height, const DWORD *src, DWORD *dst)
{
    int dwordsPerRow = (width + 0x1f) / 32, r, c;
    if (width > 0 && height > 0) {
        dst += (height - 1) * dwordsPerRow;
        for (r = 0; r < height; r++) {
            for (c = 0; c < dwordsPerRow; c++) dst[c] = src[c];
            dst -= dwordsPerRow;
            src += dwordsPerRow;
        }
    }
}

/* 6900eff6 -- wglUseFontBitmapsA: wglSAUseFontBitmapsA: for each of the count glyphs from first compile list
** listBase + i holding one glBitmap of its GGO_BITMAP image (flipped bottom-up; xorig +gmptGlyphOrigin.x, yorig
** blackbox height - origin y, advance gmCellIncX/Y; an empty glyph only advances); FALSE with
** ERROR_INVALID_PARAMETER when GetRasterizerCaps, an allocation or GetGlyphOutline fails, after deleting the lists
** from (ch + listBase), ch - first of them */
BOOL WINAPI wglSAUseFontBitmapsA(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    MAT2 mat;
    GLYPHMETRICS gm;
    RASTERIZER_STATUS rs;
    BOOL result = TRUE;
    UINT ch;
    DWORD size;
    LPVOID buf = NULL;
    LPVOID flipped = NULL;
    DWORD cap = 0;
    UINT bw, bh;
    int ox, oy, incx, incy;

    mat.eM11.value = 1; mat.eM11.fract = 0; mat.eM12.value = 0; mat.eM12.fract = 0;
    mat.eM21.value = 0; mat.eM21.fract = 0; mat.eM22.value = 1; mat.eM22.fract = 0;
    if (!GetRasterizerCaps(&rs, sizeof(rs))) {
        wglSystemError("wglUseFontBitmaps", ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    for (ch = first; ch < first + count; ch++) {
        glNewList(listBase + (ch - first), GL_COMPILE);
        size = GetGlyphOutlineA(hdc, ch, GGO_BITMAP, &gm, 0, buf, &mat);      /* (the buffer, with size 0) */
        if (size > cap) {
            if (buf) wglFree(buf);
            if (flipped) wglFree(flipped);
            cap = size + 0x80;
            buf = wglMalloc(cap);
            flipped = wglMalloc(cap);
            if (buf == NULL || flipped == NULL) {
                glEndList();
                result = FALSE;
                goto fail;
            }
        }
        if (GetGlyphOutlineA(hdc, ch, GGO_BITMAP, &gm, size, buf, &mat) == GDI_ERROR) {
            glEndList();
            result = FALSE;
            goto fail;
        }
        bw = gm.gmBlackBoxX; bh = gm.gmBlackBoxY;
        ox = gm.gmptGlyphOrigin.x; oy = gm.gmptGlyphOrigin.y;
        incx = gm.gmCellIncX; incy = gm.gmCellIncY;
        if (size == 0) {
            glBitmap(0, 0, 0.0f, 0.0f, (float)incx, (float)incy, NULL);
        } else {
            wglFlipBitmap(bw, bh, (const DWORD *)buf, (DWORD *)flipped);
            glBitmap(bw, bh, (float)ox, (float)(int)(bh - oy), (float)incx, (float)incy, (const GLubyte *)flipped);
        }
        glEndList();
    }
    goto done;
fail:
    wglSystemError("wglUseFontBitmaps", ERROR_INVALID_PARAMETER);
    glDeleteLists(ch + listBase, ch - first);     /* (sic: from the failing glyph's list on) */
done:
    if (buf) wglFree(buf);
    if (flipped) wglFree(flipped);
    return result;
}

/* 6900efb0 -- wglSAUseFontBitmapsW: just calls wglSAUseFontBitmapsA with the same arguments */
BOOL WINAPI wglSAUseFontBitmapsW(HDC hdc, DWORD first, DWORD count, DWORD listBase)
{
    return wglSAUseFontBitmapsA(hdc, first, count, listBase);
}

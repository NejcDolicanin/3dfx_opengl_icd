/*
** gltest.c -- a small OpenGL feature test for the 3dfx ICD (and any other driver, for comparison).
**
** It opens a fullscreen window, runs one page per feature group, checks what can be checked by reading values and
** pixels back, and writes everything to gltest.log next to the program.  Each page also shows its picture and its
** result lines on screen.
**
**   gltest [-w 1024] [-h 768] [-bpp 32] [-auto] [-window]
**     -w / -h / -bpp   display mode to switch to (default 1024x768x32)
**     -auto            show every page once, save it as gltest_NN.bmp, exit
**     -page n          with -auto: only page n
**     -window          no mode switch, a plain 1024x768 window (not the Glide path)
**   keys: left / right = page, space = repeat the page's checks, Esc = quit
**
** Adding a page: write a function void page_x(void) that draws and (while g_checking) calls check() / info(), and
** add it to g_pages[].  A page that needs a newer GL than the driver reports says so and draws nothing.
**
** Build: tools/gltest_build.sh  (Visual C++ 6, links opengl32 / gdi32 / user32 only).
*/
#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

/* ---- enums newer than the compiler's gl.h ---- */
#define GL_BGR_                         0x80E0
#define GL_BGRA_                        0x80E1
#define GL_UNSIGNED_BYTE_3_3_2_         0x8032
#define GL_UNSIGNED_SHORT_4_4_4_4_      0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1_      0x8034
#define GL_UNSIGNED_INT_8_8_8_8_        0x8035
#define GL_UNSIGNED_INT_10_10_10_2_     0x8036
#define GL_UNSIGNED_BYTE_2_3_3_REV_     0x8362
#define GL_UNSIGNED_SHORT_5_6_5_        0x8363
#define GL_UNSIGNED_SHORT_5_6_5_REV_    0x8364
#define GL_UNSIGNED_SHORT_4_4_4_4_REV_  0x8365
#define GL_UNSIGNED_SHORT_1_5_5_5_REV_  0x8366
#define GL_UNSIGNED_INT_8_8_8_8_REV_    0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV_ 0x8368
#define GL_RESCALE_NORMAL_              0x803A
#define GL_TEXTURE_3D_                  0x806F
#define GL_PROXY_TEXTURE_3D_            0x8070
#define GL_TEXTURE_DEPTH_               0x8071
#define GL_MAX_3D_TEXTURE_SIZE_         0x8073
#define GL_MAX_ELEMENTS_VERTICES_       0x80E8
#define GL_MAX_ELEMENTS_INDICES_        0x80E9
#define GL_TEXTURE_MIN_LOD_             0x813A
#define GL_TEXTURE_MAX_LOD_             0x813B
#define GL_TEXTURE_BASE_LEVEL_          0x813C
#define GL_TEXTURE_MAX_LEVEL_           0x813D
#define GL_LIGHT_MODEL_COLOR_CONTROL_   0x81F8
#define GL_SINGLE_COLOR_                0x81F9
#define GL_SEPARATE_SPECULAR_COLOR_     0x81FA
#define GL_ALIASED_POINT_SIZE_RANGE_    0x846D
#define GL_ALIASED_LINE_WIDTH_RANGE_    0x846E
#define GL_CLAMP_TO_EDGE_               0x812F

typedef void (APIENTRY *PFN_DrawRangeElements)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *);
typedef void (APIENTRY *PFN_TexImage3D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum,
                                        const GLvoid *);

/* ---- program state ---- */
static HWND   g_hwnd;
static HDC    g_hdc;
static HGLRC  g_hrc;
static int    g_w = 1024, g_h = 768, g_bpp = 32, g_auto, g_windowed;
static int    g_ver;                    /* major * 10 + minor of GL_VERSION */
static FILE  *g_log;
static int    g_checking;               /* the page runs its checks in this frame */
static int    g_pass, g_fail;
#define TEXT_LINES 40
#define TEXT_COLS  76
static char   g_text[TEXT_LINES][TEXT_COLS + 1];
static int    g_textLines;
static int    g_textDirty;
static GLuint g_textTex;

/* ---- log and result lines ---- */
static void line(const char *tag, const char *fmt, va_list ap)
{
    char buf[512];
    int n = sprintf(buf, "%s", tag);

    _vsnprintf(buf + n, sizeof(buf) - n - 1, fmt, ap);
    buf[sizeof(buf) - 1] = 0;
    if (g_log) { fprintf(g_log, "%s\n", buf); fflush(g_log); }
    if (g_textLines < TEXT_LINES) {
        strncpy(g_text[g_textLines], buf, TEXT_COLS);
        g_text[g_textLines][TEXT_COLS] = 0;
        g_textLines++;
        g_textDirty = 1;
    }
}
static void info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt); line("       ", fmt, ap); va_end(ap);
}
static int check(int ok, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt); line(ok ? "PASS   " : "FAIL   ", fmt, ap); va_end(ap);
    if (ok) g_pass++; else g_fail++;
    return ok;
}
/* the GL error since the last call, as text; "" when none (and clears the rest) */
static const char *glerr(void)
{
    static char s[32];
    GLenum e = glGetError();
    int guard = 0;

    while (glGetError() != GL_NO_ERROR && ++guard < 16) ;
    switch (e) {
      case GL_NO_ERROR:          return "";
      case GL_INVALID_ENUM:      return "GL_INVALID_ENUM";
      case GL_INVALID_VALUE:     return "GL_INVALID_VALUE";
      case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
      case GL_OUT_OF_MEMORY:     return "GL_OUT_OF_MEMORY";
    }
    sprintf(s, "error 0x%x", e);
    return s;
}
static int noerr(const char *what)
{
    const char *e = glerr();
    if (*e) { check(0, "%s: %s", what, e); return 0; }
    return 1;
}

/* ---- drawing helpers: pixel coordinates, origin bottom-left ---- */
static void ortho(void)
{
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0, g_w, 0, g_h, -100, 100);
    glMatrixMode(GL_MODELVIEW);  glLoadIdentity();
}
static void quad(float x, float y, float w, float h)
{
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(x, y);
    glTexCoord2f(1, 0); glVertex2f(x + w, y);
    glTexCoord2f(1, 1); glVertex2f(x + w, y + h);
    glTexCoord2f(0, 1); glVertex2f(x, y + h);
    glEnd();
}
static int g_finishBeforeRead = 1;      /* the feature pages wait for the card first; the read-back page does not */
static void readpx(int x, int y, GLubyte *rgba)
{
    rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0;
    if (g_finishBeforeRead) glFinish();
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
}
static int nearc(const GLubyte *a, int r, int g, int b, int tol)
{
    return abs(a[0] - r) <= tol && abs(a[1] - g) <= tol && abs(a[2] - b) <= tol;
}
/* colour tolerance of the frame buffer: 16 bpp has 5 / 6 bits and dithering */
static int fbtol(void) { return g_bpp <= 16 ? 40 : 12; }

static void *getproc(const char *name) { return (void *)wglGetProcAddress(name); }

/* ---- the text panel: GDI draws the lines into a DIB, which becomes a texture ---- */
#define PANEL_W 512
#define PANEL_H 512
static void drawText(void)
{
    if (g_textDirty) {
        BITMAPINFO bmi;
        void *bits = NULL;
        HDC mem = CreateCompatibleDC(NULL);
        HBITMAP bm;
        HFONT font;
        RECT r;
        int i;

        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = PANEL_W;
        bmi.bmiHeader.biHeight = PANEL_H;               /* bottom-up, as GL wants it */
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bm = CreateDIBSection(mem, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
        if (bm != NULL && bits != NULL) {
            HGDIOBJ oldBm = SelectObject(mem, bm), oldFont;
            font = CreateFontA(12, 0, 0, 0, FW_NORMAL, 0, 0, 0, ANSI_CHARSET, 0, 0, NONANTIALIASED_QUALITY,
                               FIXED_PITCH | FF_MODERN, "Courier New");
            oldFont = SelectObject(mem, font);
            r.left = 0; r.top = 0; r.right = PANEL_W; r.bottom = PANEL_H;
            FillRect(mem, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
            SetBkMode(mem, TRANSPARENT);
            for (i = 0; i < g_textLines; i++) {
                SetTextColor(mem, strncmp(g_text[i], "FAIL", 4) == 0 ? RGB(255, 80, 80) :
                                  strncmp(g_text[i], "PASS", 4) == 0 ? RGB(80, 255, 80) : RGB(220, 220, 220));
                TextOutA(mem, 2, 2 + i * 12, g_text[i], (int)strlen(g_text[i]));
            }
            GdiFlush();
            if (g_textTex == 0) glGenTextures(1, &g_textTex);
            glBindTexture(GL_TEXTURE_2D, g_textTex);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, PANEL_W, PANEL_H, 0, GL_BGRA_, GL_UNSIGNED_BYTE, bits);
            SelectObject(mem, oldFont); SelectObject(mem, oldBm);
            DeleteObject(font); DeleteObject(bm);
        }
        DeleteDC(mem);
        g_textDirty = 0;
        glerr();
    }
    if (g_textTex == 0) return;
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_textTex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor3f(1, 1, 1);
    quad((float)(g_w - PANEL_W), (float)(g_h - PANEL_H), PANEL_W, PANEL_H);
    glDisable(GL_TEXTURE_2D);
}

/* =====================================================================================================================
** page 1: what the driver says
*/
static void page_info(void)
{
    static const char *procs[] = { "glDrawRangeElements", "glDrawRangeElementsEXT", "glTexImage3D", "glTexSubImage3D",
        "glCopyTexSubImage3D", "glActiveTextureARB", "glActiveTexture", "glLockArraysEXT", "glBlendColor",
        "glBlendEquation", "glBlendFuncSeparate", "glMultiDrawArrays", "glGenBuffers", "glGenQueries" };
    const char *ext;
    char word[TEXT_COLS];
    int i, n;

    if (!g_checking) return;
    info("vendor   %s", glGetString(GL_VENDOR));
    info("renderer %s", glGetString(GL_RENDERER));
    info("version  %s  (taken as %d.%d)", glGetString(GL_VERSION), g_ver / 10, g_ver % 10);
    info("mode     %dx%d, %d bpp%s", g_w, g_h, g_bpp, g_windowed ? ", window" : "");
    {
        GLint r, g, b, a, z, s;
        glGetIntegerv(GL_RED_BITS, &r); glGetIntegerv(GL_GREEN_BITS, &g); glGetIntegerv(GL_BLUE_BITS, &b);
        glGetIntegerv(GL_ALPHA_BITS, &a); glGetIntegerv(GL_DEPTH_BITS, &z); glGetIntegerv(GL_STENCIL_BITS, &s);
        info("buffers  rgba %d %d %d %d, depth %d, stencil %d", r, g, b, a, z, s);
    }
    info("entry points:");
    for (i = 0; i < sizeof(procs) / sizeof(procs[0]); i++)
        info("  %-26s %s", procs[i], getproc(procs[i]) ? "yes" : "-");
    /* the extension string goes to the log in full, one name per line */
    ext = (const char *)glGetString(GL_EXTENSIONS);
    if (g_log && ext) {
        fprintf(g_log, "extensions:\n");
        while (*ext) {
            n = 0;
            while (*ext && *ext != ' ' && n < TEXT_COLS - 1) word[n++] = *ext++;
            word[n] = 0;
            while (*ext == ' ') ext++;
            if (n) fprintf(g_log, "  %s\n", word);
        }
    }
    glerr();
}

/* =====================================================================================================================
** page 2: reading pixels back.  glReadPixels must return what was drawn before it, with or without glFlush / glFinish.
** The other pages call glFinish before every read so that a driver that fails here still gets its features tested.
*/
static void page_readback(void)
{
    GLubyte px[4];
    int y = g_h - 140, i;

    ortho();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D);
    if (!g_checking) {
        glColor3f(1, 0, 0); quad(10, (float)y, 100, 100);
        glColor3f(0, 1, 0); quad(130, (float)y, 100, 100);
        glColor3f(0, 0, 1); quad(250, (float)y, 100, 100);
        return;
    }
    g_finishBeforeRead = 0;
    info("a quad is drawn and one pixel of it is read straight away");
    glColor3f(1, 0, 0); quad(10, (float)y, 100, 100);
    readpx(60, y + 50, px);
    check(nearc(px, 255, 0, 0, fbtol()), "read with nothing in between: red (%d %d %d)", px[0], px[1], px[2]);
    glColor3f(0, 1, 0); quad(130, (float)y, 100, 100);
    glFlush();
    readpx(180, y + 50, px);
    check(nearc(px, 0, 255, 0, fbtol()), "read after glFlush: green (%d %d %d)", px[0], px[1], px[2]);
    glColor3f(0, 0, 1); quad(250, (float)y, 100, 100);
    glFinish();
    readpx(300, y + 50, px);
    check(nearc(px, 0, 0, 255, fbtol()), "read after glFinish: blue (%d %d %d)", px[0], px[1], px[2]);
    /* many small quads, each read at once: counts how often the read is late */
    {
        int late = 0;
        for (i = 0; i < 32; i++) {
            float g = (i & 1) ? 1.0f : 0.0f;
            glColor3f(1 - g, g, 1);
            quad((float)(10 + i * 12), (float)(y - 40), 10, 10);
            readpx(15 + i * 12, y - 35, px);
            if (!nearc(px, (i & 1) ? 0 : 255, (i & 1) ? 255 : 0, 255, fbtol())) late++;
        }
        check(late == 0, "32 small quads, each read at once: %d reads did not show the quad", late);
    }
    g_finishBeforeRead = 1;
    readpx(60, y + 50, px);
    info("  the first quad read again at the end: %d %d %d", px[0], px[1], px[2]);
    /* the same with glFinish before each read, for every way of drawing: what was drawn last must be there */
    info("glFinish, then read, after each kind of drawing:");
    {
        static const GLfloat verts[4][2] = { {0, 0}, {40, 0}, {40, 40}, {0, 40} };
        static const GLuint idx[6] = { 0, 1, 2, 0, 2, 3 };
        static const GLfloat pos[4] = { 0, 0, 1, 0 }, white[4] = { 1, 1, 1, 1 }, black[4] = { 0, 0, 0, 1 };
        static GLubyte texel[4 * 4 * 4];
        static GLuint tex;
        int yy = y - 110, pass;

        for (i = 0; i < 16; i++) { texel[i * 4] = 255; texel[i * 4 + 1] = 128; texel[i * 4 + 2] = 0; texel[i * 4 + 3] = 255; }
        if (tex == 0) {
            glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, texel);
        }
        for (pass = 0; pass < 2; pass++) {              /* twice: the second round follows the lit quad */
            int x0 = 10 + pass * 260;
            glColor3f(0, 1, 1); quad((float)x0, (float)yy, 40, 40);
            readpx(x0 + 20, yy + 20, px);
            check(nearc(px, 0, 255, 255, fbtol()), "round %d: plain quad (%d %d %d)", pass + 1, px[0], px[1], px[2]);

            glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            quad((float)(x0 + 50), (float)yy, 40, 40);
            glDisable(GL_TEXTURE_2D);
            readpx(x0 + 70, yy + 20, px);
            check(nearc(px, 255, 128, 0, fbtol() + 20), "round %d: textured quad (%d %d %d)", pass + 1, px[0], px[1], px[2]);

            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(2, GL_FLOAT, 0, verts);
            glPushMatrix(); glTranslatef((float)(x0 + 100), (float)yy, 0);
            glColor3f(1, 0, 1);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, idx);
            glPopMatrix();
            readpx(x0 + 120, yy + 20, px);
            check(nearc(px, 255, 0, 255, fbtol()), "round %d: glDrawElements quad (%d %d %d)", pass + 1, px[0], px[1], px[2]);
            glPushMatrix(); glTranslatef((float)(x0 + 150), (float)yy, 0);
            glColor3f(1, 1, 0);
            glDrawArrays(GL_QUADS, 0, 4);
            glPopMatrix();
            glDisableClientState(GL_VERTEX_ARRAY);
            readpx(x0 + 170, yy + 20, px);
            check(nearc(px, 255, 255, 0, fbtol()), "round %d: glDrawArrays quad (%d %d %d)", pass + 1, px[0], px[1], px[2]);

            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);
            glLightfv(GL_LIGHT0, GL_POSITION, pos); glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
            glLightfv(GL_LIGHT0, GL_AMBIENT, black); glLightfv(GL_LIGHT0, GL_SPECULAR, black);
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, white); glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, black);
            glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
            glBegin(GL_QUADS); glNormal3f(0, 0, 1);
            glVertex2f((float)(x0 + 200), (float)yy); glVertex2f((float)(x0 + 240), (float)yy);
            glVertex2f((float)(x0 + 240), (float)(yy + 40)); glVertex2f((float)(x0 + 200), (float)(yy + 40));
            glEnd();
            glDisable(GL_LIGHTING);
            readpx(x0 + 220, yy + 20, px);
            check(nearc(px, 255, 255, 255, fbtol()), "round %d: lit quad (%d %d %d)", pass + 1, px[0], px[1], px[2]);
        }
    }
    glerr();
}

/* =====================================================================================================================
** page 3: OpenGL 1.2 state and queries
*/
static void page_queries12(void)
{
    GLint v[4];
    GLfloat f[4];
    PFN_TexImage3D tex3d = (PFN_TexImage3D)getproc("glTexImage3D");
    GLuint t;

    if (!g_checking) return;
    if (g_ver < 12) {
        info("the driver reports %d.%d: the 1.2 enums must be refused", g_ver / 10, g_ver % 10);
        glerr();
        glEnable(GL_RESCALE_NORMAL_);
        check(strcmp(glerr(), "GL_INVALID_ENUM") == 0, "glEnable(GL_RESCALE_NORMAL) gives GL_INVALID_ENUM");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_, 0);
        check(strcmp(glerr(), "GL_INVALID_ENUM") == 0, "GL_TEXTURE_MAX_LEVEL gives GL_INVALID_ENUM");
        check(getproc("glDrawRangeElements") == NULL, "glDrawRangeElements is not handed out");
        return;
    }
    glerr();
    v[0] = 0; glGetIntegerv(GL_MAX_ELEMENTS_VERTICES_, v);
    check(*glerr() == 0 && v[0] > 0, "GL_MAX_ELEMENTS_VERTICES = %d", v[0]);
    v[0] = 0; glGetIntegerv(GL_MAX_ELEMENTS_INDICES_, v);
    check(*glerr() == 0 && v[0] > 0, "GL_MAX_ELEMENTS_INDICES = %d", v[0]);
    f[0] = f[1] = 0; glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE_, f);
    check(*glerr() == 0 && f[1] >= 1.0f, "GL_ALIASED_POINT_SIZE_RANGE = %g .. %g", f[0], f[1]);
    f[0] = f[1] = 0; glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE_, f);
    check(*glerr() == 0 && f[1] >= 1.0f, "GL_ALIASED_LINE_WIDTH_RANGE = %g .. %g", f[0], f[1]);

    v[0] = 0; glGetIntegerv(GL_LIGHT_MODEL_COLOR_CONTROL_, v);
    check(*glerr() == 0 && v[0] == GL_SINGLE_COLOR_, "colour control starts as GL_SINGLE_COLOR (0x%x)", v[0]);
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_, GL_SEPARATE_SPECULAR_COLOR_);
    v[0] = 0; glGetIntegerv(GL_LIGHT_MODEL_COLOR_CONTROL_, v);
    check(*glerr() == 0 && v[0] == GL_SEPARATE_SPECULAR_COLOR_, "set to GL_SEPARATE_SPECULAR_COLOR reads back");
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_, 0x1234);
    check(strcmp(glerr(), "GL_INVALID_ENUM") == 0, "a bad colour control value gives GL_INVALID_ENUM");
    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL_, GL_SINGLE_COLOR_);

    glDisable(GL_NORMALIZE); glDisable(GL_RESCALE_NORMAL_); glerr();
    glEnable(GL_RESCALE_NORMAL_);
    check(*glerr() == 0 && glIsEnabled(GL_RESCALE_NORMAL_) && !glIsEnabled(GL_NORMALIZE),
          "GL_RESCALE_NORMAL on, GL_NORMALIZE still reads off");
    glEnable(GL_NORMALIZE); glDisable(GL_RESCALE_NORMAL_);
    check(!glIsEnabled(GL_RESCALE_NORMAL_) && glIsEnabled(GL_NORMALIZE), "and the other way round");
    glPushAttrib(GL_ENABLE_BIT); glDisable(GL_NORMALIZE); glEnable(GL_RESCALE_NORMAL_); glPopAttrib();
    check(!glIsEnabled(GL_RESCALE_NORMAL_) && glIsEnabled(GL_NORMALIZE), "glPushAttrib / glPopAttrib restore both");
    glDisable(GL_NORMALIZE);

    glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t); glerr();
    v[0] = -1; glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL_, v);
    v[1] = -1; glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_, v + 1);
    check(*glerr() == 0 && v[0] == 0 && v[1] == 1000, "base / max level start as 0 / 1000 (%d / %d)", v[0], v[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL_, 2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_, 5);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL_, v);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_, v + 1);
    check(*glerr() == 0 && v[0] == 2 && v[1] == 5, "base / max level 2 / 5 read back (%d / %d)", v[0], v[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_, -1);
    check(strcmp(glerr(), "GL_INVALID_VALUE") == 0, "a negative max level gives GL_INVALID_VALUE");
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD_, -2.0f);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD_, 4.0f);
    check(*glerr() == 0, "GL_TEXTURE_MIN_LOD / MAX_LOD are accepted");
    glDeleteTextures(1, &t);

    v[0] = 0; glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE_, v);
    check(*glerr() == 0 && v[0] >= 16, "GL_MAX_3D_TEXTURE_SIZE = %d", v[0]);
    if (check(tex3d != NULL, "glTexImage3D is handed out")) {
        static GLubyte vox[8 * 8 * 8 * 4];
        tex3d(GL_PROXY_TEXTURE_3D_, 0, GL_RGBA, 16, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        v[0] = 0; glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D_, 0, GL_TEXTURE_WIDTH, v);
        v[1] = 0; glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D_, 0, GL_TEXTURE_DEPTH_, v + 1);
        check(*glerr() == 0 && v[0] == 16 && v[1] == 16, "proxy 16x16x16 is accepted (%d, depth %d)", v[0], v[1]);
        tex3d(GL_PROXY_TEXTURE_3D_, 0, GL_RGBA, 4096, 4096, 4096, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        v[0] = -1; glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D_, 0, GL_TEXTURE_WIDTH, v);
        check(*glerr() == 0, "proxy 4096x4096x4096 gives no error (width %d; 0 = too large for the driver)", v[0]);
        tex3d(GL_TEXTURE_3D_, 0, GL_RGBA, 8, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, vox);
        check(*glerr() == 0, "glTexImage3D 8x8x8 gives no error");
        glEnable(GL_TEXTURE_3D_);
        check(*glerr() == 0 && glIsEnabled(GL_TEXTURE_3D_), "GL_TEXTURE_3D can be enabled");
        glDisable(GL_TEXTURE_3D_);
        info("  (3D textures are accepted by the 3dfx ICD but not drawn)");
    }
    glerr();
}

/* =====================================================================================================================
** page 4: packed pixel types.  One 8x8 pattern, uploaded in every type; every quad must look the same.
*/
#define PAT 8
static GLubyte g_pat[PAT * PAT * 4];            /* RGBA bytes: colour bars, the top half with alpha 0 */
static void makePattern(void)
{
    static const GLubyte bars[8][3] = { {255,0,0}, {0,255,0}, {0,0,255}, {255,255,255},
                                        {255,255,0}, {0,255,255}, {255,0,255}, {0,0,0} };
    int x, y;
    for (y = 0; y < PAT; y++)
        for (x = 0; x < PAT; x++) {
            GLubyte *p = g_pat + (y * PAT + x) * 4;
            p[0] = bars[x][0]; p[1] = bars[x][1]; p[2] = bars[x][2];
            p[3] = (GLubyte)(y < PAT / 2 ? 255 : 0);
        }
}
#define FLD(c, max)     (((GLuint)(c) * (max) + 127u) / 255u)
/* one element of the given type from the components in the order the format names them */
static GLuint packElem(GLenum type, const GLubyte *c)
{
    switch (type) {
      case GL_UNSIGNED_BYTE_3_3_2_:         return (FLD(c[0], 7) << 5) | (FLD(c[1], 7) << 2) | FLD(c[2], 3);
      case GL_UNSIGNED_BYTE_2_3_3_REV_:     return FLD(c[0], 7) | (FLD(c[1], 7) << 3) | (FLD(c[2], 3) << 6);
      case GL_UNSIGNED_SHORT_5_6_5_:        return (FLD(c[0], 31) << 11) | (FLD(c[1], 63) << 5) | FLD(c[2], 31);
      case GL_UNSIGNED_SHORT_5_6_5_REV_:    return FLD(c[0], 31) | (FLD(c[1], 63) << 5) | (FLD(c[2], 31) << 11);
      case GL_UNSIGNED_SHORT_4_4_4_4_:      return (FLD(c[0], 15) << 12) | (FLD(c[1], 15) << 8) | (FLD(c[2], 15) << 4) | FLD(c[3], 15);
      case GL_UNSIGNED_SHORT_4_4_4_4_REV_:  return FLD(c[0], 15) | (FLD(c[1], 15) << 4) | (FLD(c[2], 15) << 8) | (FLD(c[3], 15) << 12);
      case GL_UNSIGNED_SHORT_5_5_5_1_:      return (FLD(c[0], 31) << 11) | (FLD(c[1], 31) << 6) | (FLD(c[2], 31) << 1) | FLD(c[3], 1);
      case GL_UNSIGNED_SHORT_1_5_5_5_REV_:  return FLD(c[0], 31) | (FLD(c[1], 31) << 5) | (FLD(c[2], 31) << 10) | (FLD(c[3], 1) << 15);
      case GL_UNSIGNED_INT_8_8_8_8_:        return ((GLuint)c[0] << 24) | ((GLuint)c[1] << 16) | ((GLuint)c[2] << 8) | c[3];
      case GL_UNSIGNED_INT_8_8_8_8_REV_:    return c[0] | ((GLuint)c[1] << 8) | ((GLuint)c[2] << 16) | ((GLuint)c[3] << 24);
      case GL_UNSIGNED_INT_10_10_10_2_:     return (FLD(c[0], 1023) << 22) | (FLD(c[1], 1023) << 12) | (FLD(c[2], 1023) << 2) | FLD(c[3], 3);
      case GL_UNSIGNED_INT_2_10_10_10_REV_: return FLD(c[0], 1023) | (FLD(c[1], 1023) << 10) | (FLD(c[2], 1023) << 20) | (FLD(c[3], 3) << 30);
    }
    return 0;
}
typedef struct { const char *name; GLenum format, type; int esize, minver; } PackedCase;
static const PackedCase g_packed[] = {
    { "RGBA  UNSIGNED_BYTE (reference)", GL_RGBA,  GL_UNSIGNED_BYTE,               0, 11 },
    { "BGRA  UNSIGNED_BYTE",             GL_BGRA_, GL_UNSIGNED_BYTE,               0, 11 },
    { "RGB   3_3_2",                     GL_RGB,   GL_UNSIGNED_BYTE_3_3_2_,        1, 11 },
    { "RGBA  4_4_4_4",                   GL_RGBA,  GL_UNSIGNED_SHORT_4_4_4_4_,     2, 11 },
    { "RGBA  5_5_5_1",                   GL_RGBA,  GL_UNSIGNED_SHORT_5_5_5_1_,     2, 11 },
    { "RGBA  8_8_8_8",                   GL_RGBA,  GL_UNSIGNED_INT_8_8_8_8_,       4, 11 },
    { "RGBA  10_10_10_2",                GL_RGBA,  GL_UNSIGNED_INT_10_10_10_2_,    4, 11 },
    { "RGB   5_6_5",                     GL_RGB,   GL_UNSIGNED_SHORT_5_6_5_,       2, 12 },
    { "RGB   5_6_5_REV",                 GL_RGB,   GL_UNSIGNED_SHORT_5_6_5_REV_,   2, 12 },
    { "RGB   2_3_3_REV",                 GL_RGB,   GL_UNSIGNED_BYTE_2_3_3_REV_,    1, 12 },
    { "RGBA  4_4_4_4_REV",               GL_RGBA,  GL_UNSIGNED_SHORT_4_4_4_4_REV_, 2, 12 },
    { "BGRA  4_4_4_4_REV",               GL_BGRA_, GL_UNSIGNED_SHORT_4_4_4_4_REV_, 2, 12 },
    { "RGBA  1_5_5_5_REV",               GL_RGBA,  GL_UNSIGNED_SHORT_1_5_5_5_REV_, 2, 12 },
    { "BGRA  1_5_5_5_REV",               GL_BGRA_, GL_UNSIGNED_SHORT_1_5_5_5_REV_, 2, 12 },
    { "RGBA  8_8_8_8_REV",               GL_RGBA,  GL_UNSIGNED_INT_8_8_8_8_REV_,   4, 12 },
    { "BGRA  8_8_8_8_REV",               GL_BGRA_, GL_UNSIGNED_INT_8_8_8_8_REV_,   4, 12 },
    { "RGBA  2_10_10_10_REV",            GL_RGBA,  GL_UNSIGNED_INT_2_10_10_10_REV_,4, 12 },
};
#define NPACKED (sizeof(g_packed) / sizeof(g_packed[0]))
static GLuint g_packedTex[NPACKED];
static int    g_packedOk[NPACKED];              /* uploaded without error */

/* the pattern in one case's encoding; returns bytes per row */
static void encodePattern(const PackedCase *pc, GLubyte *out)
{
    int i, n = PAT * PAT;
    for (i = 0; i < n; i++) {
        const GLubyte *s = g_pat + i * 4;
        GLubyte c[4];
        GLuint v;
        if (pc->format == GL_BGRA_) { c[0] = s[2]; c[1] = s[1]; c[2] = s[0]; c[3] = s[3]; }
        else                        { c[0] = s[0]; c[1] = s[1]; c[2] = s[2]; c[3] = s[3]; }
        if (pc->esize == 0) { memcpy(out + i * 4, c, 4); continue; }
        v = packElem(pc->type, c);
        if (pc->esize == 1)      out[i] = (GLubyte)v;
        else if (pc->esize == 2) ((GLushort *)out)[i] = (GLushort)v;
        else                     ((GLuint *)out)[i] = v;
    }
}
static void page_packed(void)
{
    static GLubyte enc[PAT * PAT * 4], back[PAT * PAT * 4];
    GLubyte px[4];
    int i, x, y, cols = 4, cell = 120, size = 96, shown = 0;

    ortho();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    if (g_checking) {
        makePattern();
        info("one 8x8 pattern in each type; all quads must look alike");
        for (i = 0; i < NPACKED; i++) {
            const PackedCase *pc = &g_packed[i];
            int bad = 0, tol = 40, k;

            g_packedOk[i] = 0;
            if (pc->minver > g_ver) continue;
            if (g_packedTex[i] == 0) glGenTextures(1, &g_packedTex[i]);
            glBindTexture(GL_TEXTURE_2D, g_packedTex[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            encodePattern(pc, enc);
            glerr();
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PAT, PAT, 0, pc->format, pc->type, enc);
            if (!noerr(pc->name)) continue;
            g_packedOk[i] = 1;
            /* read the texture back as bytes: colours within the coarsest field's step */
            memset(back, 0x55, sizeof(back));
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, back);
            if (*glerr()) { check(0, "%s: glGetTexImage failed", pc->name); continue; }
            for (k = 0; k < PAT * PAT; k++)
                if (abs(back[k * 4] - g_pat[k * 4]) > tol || abs(back[k * 4 + 1] - g_pat[k * 4 + 1]) > tol ||
                    abs(back[k * 4 + 2] - g_pat[k * 4 + 2]) > tol)
                    bad++;
            check(bad == 0, "%-32s texels wrong: %d of 64", pc->name, bad);
        }
        if (g_ver >= 12) {      /* a sub-image with padded rows: 5 texels of 5_6_5 are 10 bytes, rows 12 apart */
            static GLushort sub[3 * 6];
            static GLubyte black[PAT * PAT * 4];
            GLuint t;
            for (i = 0; i < 18; i++) sub[i] = (GLushort)((i % 6) < 5 ? 0xffff : 0x0000);
            glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PAT, PAT, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 5, 3, GL_RGB, GL_UNSIGNED_SHORT_5_6_5_, sub);
            memset(back, 0x55, sizeof(back));
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, back);
            check(*glerr() == 0 && back[(1 * PAT + 1) * 4] > 200 && back[(3 * PAT + 5) * 4] > 200 &&
                  back[(1 * PAT + 6) * 4] < 50 && back[(4 * PAT + 1) * 4] < 50 && back[0] < 50,
                  "glTexSubImage2D 5x3 of 5_6_5 with padded rows lands in the right texels");
            glDeleteTextures(1, &t);
        }
    }
    glEnable(GL_TEXTURE_2D);
    glColor3f(1, 1, 1);
    for (i = 0; i < NPACKED; i++) {
        if (!g_packedOk[i]) continue;
        x = 10 + (shown % cols) * cell;
        y = g_h - 20 - size - (shown / cols) * cell;
        glBindTexture(GL_TEXTURE_2D, g_packedTex[i]);
        quad((float)x, (float)y, (float)size, (float)size);
        if (g_checking) {       /* the red and the white bar on the screen */
            int bw = size / PAT, ok;
            readpx(x + bw / 2, y + size / 4, px);
            ok = nearc(px, 255, 0, 0, fbtol() + 40);
            readpx(x + 3 * bw + bw / 2, y + size / 4, px);
            ok = ok && nearc(px, 255, 255, 255, fbtol() + 40);
            check(ok, "%-32s drawn: red and white bars", g_packed[i].name);
        }
        shown++;
    }
    glDisable(GL_TEXTURE_2D);
    if (g_checking && g_ver >= 12) {    /* read a strip of the first quad as 5_6_5 and as bytes */
        GLushort s565[4];
        GLubyte b[4 * 4];
        int bw = size / PAT, k, bad = 0;
        x = 10; y = g_h - 20 - size + size / 4;
        glReadBuffer(GL_BACK);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        memset(s565, 0, sizeof(s565)); memset(b, 0, sizeof(b));
        for (k = 0; k < 4; k++) {
            glReadPixels(x + k * bw + bw / 2, y, 1, 1, GL_RGB, GL_UNSIGNED_SHORT_5_6_5_, &s565[k]);
            glReadPixels(x + k * bw + bw / 2, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &b[k * 4]);
        }
        if (*glerr()) check(0, "glReadPixels as 5_6_5 gives an error");
        else {
            for (k = 0; k < 4; k++) {
                int r = ((s565[k] >> 11) & 31) * 255 / 31, g = ((s565[k] >> 5) & 63) * 255 / 63, bl = (s565[k] & 31) * 255 / 31;
                if (abs(r - b[k * 4]) > 12 || abs(g - b[k * 4 + 1]) > 12 || abs(bl - b[k * 4 + 2]) > 12) bad++;
            }
            check(bad == 0, "glReadPixels as 5_6_5 matches the byte read (%d of 4 differ)", bad);
        }
    }
    glerr();
}

/* =====================================================================================================================
** page 5: texture base / max level.  Every mip level has its own colour.
*/
static const GLubyte g_levelColor[6][3] = { {255,0,0}, {0,255,0}, {0,0,255}, {255,255,0}, {255,0,255}, {0,255,255} };
static void solidLevel(int level, int size, const GLubyte *rgb)
{
    static GLubyte img[32 * 32 * 4];
    int i;
    for (i = 0; i < size * size; i++) { img[i * 4] = rgb[0]; img[i * 4 + 1] = rgb[1]; img[i * 4 + 2] = rgb[2]; img[i * 4 + 3] = 255; }
    glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
}
static int levelOf(const GLubyte *px)
{
    int i;
    for (i = 0; i < 6; i++)
        if (nearc(px, g_levelColor[i][0], g_levelColor[i][1], g_levelColor[i][2], fbtol() + 30)) return i;
    return -1;
}
static void page_levels(void)
{
    static GLuint full, partial;
    static const GLubyte orange[3] = { 255, 128, 0 };
    GLubyte px[4];
    int i, y = g_h - 90;

    ortho();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    if (full == 0) {
        glGenTextures(1, &full); glBindTexture(GL_TEXTURE_2D, full);
        for (i = 0; i < 6; i++) solidLevel(i, 32 >> i, g_levelColor[i]);
        glGenTextures(1, &partial); glBindTexture(GL_TEXTURE_2D, partial);
        solidLevel(0, 32, orange);                      /* level 0 only: incomplete with a mipmap filter */
    }
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, full);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glColor3f(0.5f, 0.5f, 0.5f);
    if (g_checking) info("levels: 0 red, 1 green, 2 blue, 3 yellow, 4 magenta, 5 cyan");

    /* row 1: the plain chain at 32, 12, 6 and 3 pixels (which level the driver picks is only reported) */
    quad(10, (float)y, 32, 32); quad(60, (float)y, 12, 12); quad(90, (float)y, 6, 6); quad(110, (float)y, 3, 3);
    if (g_checking) {
        int a, b, c;
        readpx(26, y + 16, px); a = levelOf(px);
        readpx(66, y + 6, px);  b = levelOf(px);
        readpx(93, y + 3, px);  c = levelOf(px);
        check(a == 0, "plain chain at 32 px shows level 0 (got %d)", a);
        info("  at 12 px level %d, at 6 px level %d (the driver's own choice)", b, c);
    }
    if (g_ver < 12) {
        if (g_checking) info("the driver reports %d.%d: base / max level not tested", g_ver / 10, g_ver % 10);
        glDisable(GL_TEXTURE_2D);
        return;
    }
    /* row 2: max level 0 -> level 0 at every size */
    y -= 60;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_, 0);
    quad(10, (float)y, 32, 32); quad(60, (float)y, 6, 6); quad(90, (float)y, 3, 3);
    if (g_checking) {
        int a, b;
        readpx(63, y + 3, px); a = levelOf(px);
        readpx(91, y + 1, px); b = levelOf(px);
        check(a == 0 && b == 0, "GL_TEXTURE_MAX_LEVEL 0: 6 px and 3 px quads show level 0 (got %d, %d)", a, b);
    }
    /* row 3: max level 1 -> never beyond level 1 */
    y -= 60;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_, 1);
    quad(10, (float)y, 32, 32); quad(60, (float)y, 4, 4); quad(90, (float)y, 2, 2);
    if (g_checking) {
        int a, b;
        readpx(62, y + 2, px); a = levelOf(px);
        readpx(91, y + 1, px); b = levelOf(px);
        check(a == 1 && b == 1, "GL_TEXTURE_MAX_LEVEL 1: 4 px and 2 px quads show level 1 (got %d, %d)", a, b);
    }
    /* row 4: base level 1 -> level 1 even magnified; base 2, max 2 -> level 2 */
    y -= 80;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_, 1000);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL_, 1);
    quad(10, (float)y, 64, 64);
    if (g_checking) {
        int a;
        readpx(42, y + 32, px); a = levelOf(px);
        check(a == 1, "GL_TEXTURE_BASE_LEVEL 1: a 64 px quad shows level 1 (got %d)", a);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL_, 2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_, 2);
    quad(90, (float)y, 64, 64); quad(170, (float)y, 2, 2);
    if (g_checking) {
        int a, b;
        readpx(122, y + 32, px); a = levelOf(px);
        readpx(171, y + 1, px);  b = levelOf(px);
        check(a == 2 && b == 2, "base 2, max 2: 64 px and 2 px quads show level 2 (got %d, %d)", a, b);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL_, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_, 1000);
    /* row 5: level 0 only.  Mipmap filter: incomplete, drawn untextured (grey); with max level 0: orange */
    y -= 60;
    glBindTexture(GL_TEXTURE_2D, partial);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_, 1000);
    quad(10, (float)y, 32, 32);
    if (g_checking) {
        readpx(26, y + 16, px);
        /* (not a pass / fail: the 3dfx ICD draws an incomplete texture's level 0 on the card, a strict driver
           draws the polygon untextured) */
        info("  level 0 only, mipmap filter, no max level: %s (%d %d %d)",
             nearc(px, 255, 128, 0, fbtol() + 30) ? "drawn textured" : "drawn untextured", px[0], px[1], px[2]);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL_, 0);
    quad(60, (float)y, 32, 32); quad(110, (float)y, 6, 6);
    if (g_checking) {
        int ok;
        readpx(76, y + 16, px); ok = nearc(px, 255, 128, 0, fbtol() + 30);
        readpx(113, y + 3, px); ok = ok && nearc(px, 255, 128, 0, fbtol() + 30);
        check(ok, "the same with GL_TEXTURE_MAX_LEVEL 0: complete, textured at 32 px and 6 px");
    }
    glDisable(GL_TEXTURE_2D);
    glerr();
}

/* =====================================================================================================================
** page 6: GL_RESCALE_NORMAL.  A lit quad under a uniform scale of 2: unscaled normals give half the light.
*/
static void litQuad(float x, float y, float size)
{
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(2, 2, 2);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex2f(0, 0); glVertex2f(size / 2, 0); glVertex2f(size / 2, size / 2); glVertex2f(0, size / 2);
    glEnd();
    glPopMatrix();
}
static void page_rescale(void)
{
    static const GLfloat pos[4] = { 0, 0, 1, 0 }, white[4] = { 1, 1, 1, 1 }, black[4] = { 0, 0, 0, 1 },
                         half[4] = { 0.5f, 0.5f, 0.5f, 1 };
    GLubyte a[4], b[4], c[4];
    int y = g_h - 140;

    ortho();
    glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, black);
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
    glLightfv(GL_LIGHT0, GL_AMBIENT, black);
    glLightfv(GL_LIGHT0, GL_SPECULAR, black);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, half);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, black);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
    glDisable(GL_NORMALIZE);
    if (g_ver >= 12) glDisable(GL_RESCALE_NORMAL_);
    glerr();
    litQuad(10, (float)y, 100);                 /* neither: normal length 0.5 -> quarter grey */
    glEnable(GL_NORMALIZE);
    litQuad(130, (float)y, 100);                /* GL_NORMALIZE: half grey */
    glDisable(GL_NORMALIZE);
    if (g_ver >= 12) {
        glEnable(GL_RESCALE_NORMAL_);
        litQuad(250, (float)y, 100);            /* GL_RESCALE_NORMAL: half grey too */
        glDisable(GL_RESCALE_NORMAL_);
    }
    glDisable(GL_LIGHTING);
    if (g_checking) {
        info("a lit grey quad under glScalef(2): plain, GL_NORMALIZE, GL_RESCALE_NORMAL");
        readpx(60, y + 50, a); readpx(180, y + 50, b); readpx(300, y + 50, c);
        info("  plain %d, normalize %d, rescale %d (grey levels)", a[0], b[0], c[0]);
        check(b[0] > a[0] + 30, "GL_NORMALIZE is brighter than no normalisation");
        if (g_ver >= 12)
            check(abs(c[0] - b[0]) <= fbtol(), "GL_RESCALE_NORMAL gives the same as GL_NORMALIZE");
        else
            info("the driver reports %d.%d: GL_RESCALE_NORMAL not tested", g_ver / 10, g_ver % 10);
    }
    glerr();
}

/* =====================================================================================================================
** page 7: glDrawRangeElements
*/
static void page_drawrange(void)
{
    static const GLfloat verts[4][2] = { {10, 0}, {110, 0}, {110, 100}, {10, 100} };
    static const GLuint idx[6] = { 0, 1, 2, 0, 2, 3 };
    static const GLushort idx16[6] = { 0, 1, 2, 0, 2, 3 };
    PFN_DrawRangeElements dre = (PFN_DrawRangeElements)getproc("glDrawRangeElements");
    GLubyte px[4];
    int y = g_h - 140;

    ortho();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D);
    if (dre == NULL) {
        if (g_checking) info("glDrawRangeElements is not handed out (driver reports %d.%d)", g_ver / 10, g_ver % 10);
        return;
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, verts);
    glPushMatrix(); glTranslatef(0, (float)y, 0);
    glColor3f(0, 1, 0);
    glerr();
    dre(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_INT, idx);
    glTranslatef(120, 0, 0);
    glColor3f(0, 0, 1);
    dre(GL_TRIANGLES, 0, 3, 6, GL_UNSIGNED_SHORT, idx16);
    glPopMatrix();
    if (g_checking) {
        check(*glerr() == 0, "glDrawRangeElements gives no error");
        readpx(60, y + 50, px);
        check(nearc(px, 0, 255, 0, fbtol()), "the GLuint-indexed quad is drawn (%d %d %d)", px[0], px[1], px[2]);
        readpx(180, y + 50, px);
        check(nearc(px, 0, 0, 255, fbtol()), "the GLushort-indexed quad is drawn (%d %d %d)", px[0], px[1], px[2]);
        dre(GL_TRIANGLES, 3, 0, 6, GL_UNSIGNED_INT, idx);
        check(strcmp(glerr(), "GL_INVALID_VALUE") == 0, "end < start gives GL_INVALID_VALUE");
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glerr();
}

/* =====================================================================================================================
** OpenGL 1.3
*/
#define GL_TEXTURE0_                    0x84C0
#define GL_TEXTURE1_                    0x84C1
#define GL_ACTIVE_TEXTURE_              0x84E0
#define GL_MAX_TEXTURE_UNITS_           0x84E2
#define GL_TRANSPOSE_MODELVIEW_MATRIX_  0x84E3
#define GL_MULTISAMPLE_                 0x809D
#define GL_SAMPLE_BUFFERS_              0x80A8
#define GL_SAMPLES_                     0x80A9
#define GL_SAMPLE_COVERAGE_VALUE_       0x80AA
#define GL_TEXTURE_CUBE_MAP_            0x8513
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_   0x851C
#define GL_CLAMP_TO_BORDER_             0x812D
#define GL_COMBINE_                     0x8570
#define GL_COMBINE_RGB_                 0x8571
#define GL_SUBTRACT_                    0x84E7
#define GL_DOT3_RGB_                    0x86AE
#define GL_COMPRESSED_RGB_              0x84ED
#define GL_TEXTURE_COMPRESSED_          0x86A1
typedef void (APIENTRY *PFN_e)(GLenum);
typedef void (APIENTRY *PFN_eff)(GLenum, GLfloat, GLfloat);
typedef void (APIENTRY *PFN_fv)(const GLfloat *);
typedef void (APIENTRY *PFN_fb)(GLclampf, GLboolean);

static void solidTex(GLuint *tex, GLint ifmt, int r, int g, int b)
{
    static GLubyte img[16 * 16 * 4];
    int i;
    for (i = 0; i < 256; i++) { img[i * 4] = (GLubyte)r; img[i * 4 + 1] = (GLubyte)g; img[i * 4 + 2] = (GLubyte)b; img[i * 4 + 3] = 255; }
    if (*tex == 0) glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, ifmt, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
}

static void page_gl13(void)
{
    static const char *procs[] = { "glActiveTexture", "glClientActiveTexture", "glMultiTexCoord2f", "glMultiTexCoord4fv",
        "glCompressedTexImage2D", "glGetCompressedTexImage", "glSampleCoverage", "glLoadTransposeMatrixf",
        "glMultTransposeMatrixd" };
    static GLuint texA, texB, texC;
    PFN_e activeTexture = (PFN_e)getproc("glActiveTexture");
    PFN_eff multiTexCoord2f = (PFN_eff)getproc("glMultiTexCoord2f");
    GLubyte px[4];
    GLint v[4], units = 1;
    GLfloat f[16];
    int i, missing = 0, y = g_h - 140;

    ortho();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND);
    if (g_ver < 13) {
        if (g_checking) {
            info("the driver reports %d.%d: 1.3 must not be offered", g_ver / 10, g_ver % 10);
            check(activeTexture == NULL, "glActiveTexture (core name) is not handed out");
            glerr();
            glEnable(GL_MULTISAMPLE_);
            check(strcmp(glerr(), "GL_INVALID_ENUM") == 0, "glEnable(GL_MULTISAMPLE) gives GL_INVALID_ENUM");
        }
        return;
    }
    if (g_checking) {
        PFN_fv loadT = (PFN_fv)getproc("glLoadTransposeMatrixf");
        PFN_fb sampleCoverage = (PFN_fb)getproc("glSampleCoverage");
        static const GLfloat rowMajor[16] = { 1, 0, 0, 5,  0, 1, 0, 6,  0, 0, 1, 7,  0, 0, 0, 1 };

        for (i = 0; i < sizeof(procs) / sizeof(procs[0]); i++)
            if (getproc(procs[i]) == NULL) { missing++; info("  missing: %s", procs[i]); }
        check(missing == 0, "the 1.3 entry points are handed out (%d missing)", missing);
        glerr();
        if (loadT != NULL) {
            glMatrixMode(GL_MODELVIEW); glPushMatrix();
            loadT(rowMajor);
            glGetFloatv(GL_MODELVIEW_MATRIX, f);
            check(f[12] == 5 && f[13] == 6 && f[14] == 7, "glLoadTransposeMatrixf: translation lands in elements 12..14");
            memset(f, 0, sizeof(f));
            glGetFloatv(GL_TRANSPOSE_MODELVIEW_MATRIX_, f);
            check(*glerr() == 0 && f[3] == 5 && f[7] == 6 && f[11] == 7, "GL_TRANSPOSE_MODELVIEW_MATRIX gives it back");
            glPopMatrix();
        }
        v[0] = v[1] = -1;
        glGetIntegerv(GL_SAMPLE_BUFFERS_, v); glGetIntegerv(GL_SAMPLES_, v + 1);
        check(*glerr() == 0 && v[0] >= 0 && v[1] >= 0, "GL_SAMPLE_BUFFERS = %d, GL_SAMPLES = %d", v[0], v[1]);
        check(glIsEnabled(GL_MULTISAMPLE_) && *glerr() == 0, "GL_MULTISAMPLE starts enabled");
        if (sampleCoverage != NULL) {
            sampleCoverage(0.5f, GL_TRUE);
            f[0] = 0; glGetFloatv(GL_SAMPLE_COVERAGE_VALUE_, f);
            check(*glerr() == 0 && f[0] == 0.5f, "glSampleCoverage value reads back (%g)", f[0]);
            sampleCoverage(1.0f, GL_FALSE);
        }
        v[0] = 0; glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_, v);
        check(*glerr() == 0 && v[0] >= 16, "GL_MAX_CUBE_MAP_TEXTURE_SIZE = %d", v[0]);
        glEnable(GL_TEXTURE_CUBE_MAP_);
        check(*glerr() == 0 && glIsEnabled(GL_TEXTURE_CUBE_MAP_), "GL_TEXTURE_CUBE_MAP can be enabled");
        glDisable(GL_TEXTURE_CUBE_MAP_);
        solidTex(&texC, GL_RGBA, 255, 0, 255);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_);
        v[0] = 0; glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, v);
        check(*glerr() == 0 && v[0] == GL_CLAMP_TO_BORDER_, "GL_CLAMP_TO_BORDER is accepted and reads back");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_);
        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_, GL_SUBTRACT_);
        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_, GL_DOT3_RGB_);
        check(*glerr() == 0, "combine modes GL_SUBTRACT and GL_DOT3_RGB are accepted");
        glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB_, GL_MODULATE);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glerr();
    }
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_, &units);
    glerr();
    /* quad 1: a texture given as GL_COMPRESSED_RGB */
    solidTex(&texC, GL_COMPRESSED_RGB_, 255, 0, 255);
    if (g_checking) {
        const char *e = glerr();
        v[0] = 0; glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_, v);
        check(*e == 0, "a GL_COMPRESSED_RGB texture loads (%s)", *e ? e : v[0] ? "stored compressed" : "stored uncompressed");
        glerr();
    }
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor3f(1, 1, 1);
    quad(10, (float)y, 100, 100);
    if (g_checking) {
        readpx(60, y + 50, px);
        check(nearc(px, 255, 0, 255, fbtol() + 40), "and draws magenta (%d %d %d)", px[0], px[1], px[2]);
    }
    /* quad 2: two texture units through the core names: yellow modulated by cyan is green */
    if (activeTexture != NULL && multiTexCoord2f != NULL && units >= 2) {
        solidTex(&texA, GL_RGBA, 255, 255, 0);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        activeTexture(GL_TEXTURE1_);
        if (g_checking) {
            v[0] = 0; glGetIntegerv(GL_ACTIVE_TEXTURE_, v);
            check(v[0] == GL_TEXTURE1_, "glActiveTexture(GL_TEXTURE1) reads back");
        }
        glEnable(GL_TEXTURE_2D);
        solidTex(&texB, GL_RGBA, 0, 255, 255);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glBegin(GL_QUADS);
        multiTexCoord2f(GL_TEXTURE0_, 0, 0); multiTexCoord2f(GL_TEXTURE1_, 0, 0); glVertex2f(130, (float)y);
        multiTexCoord2f(GL_TEXTURE0_, 1, 0); multiTexCoord2f(GL_TEXTURE1_, 1, 0); glVertex2f(230, (float)y);
        multiTexCoord2f(GL_TEXTURE0_, 1, 1); multiTexCoord2f(GL_TEXTURE1_, 1, 1); glVertex2f(230, (float)(y + 100));
        multiTexCoord2f(GL_TEXTURE0_, 0, 1); multiTexCoord2f(GL_TEXTURE1_, 0, 1); glVertex2f(130, (float)(y + 100));
        glEnd();
        glDisable(GL_TEXTURE_2D);
        activeTexture(GL_TEXTURE0_);
        if (g_checking) {
            readpx(180, y + 50, px);
            check(nearc(px, 0, 255, 0, fbtol() + 20), "two units through glMultiTexCoord2f: green (%d %d %d)", px[0], px[1], px[2]);
        }
    } else if (g_checking) {
        info("  one texture unit: the two-unit quad is not drawn");
    }
    glDisable(GL_TEXTURE_2D);
    glerr();
}

/* =====================================================================================================================
** OpenGL 1.4
*/
#define GL_BLEND_DST_RGB_               0x80C8
#define GL_BLEND_SRC_RGB_               0x80C9
#define GL_BLEND_DST_ALPHA_             0x80CA
#define GL_BLEND_SRC_ALPHA_             0x80CB
#define GL_BLEND_COLOR_                 0x8005
#define GL_FUNC_ADD_                    0x8006
#define GL_MIN_                         0x8007
#define GL_MIRRORED_REPEAT_             0x8370
#define GL_TEXTURE_FILTER_CONTROL_      0x8500
#define GL_TEXTURE_LOD_BIAS_            0x8501
#define GL_GENERATE_MIPMAP_             0x8191
#define GL_COLOR_SUM_                   0x8458
#define GL_CURRENT_SECONDARY_COLOR_     0x8459
#define GL_DEPTH_COMPONENT_             0x1902
typedef void (APIENTRY *PFN_eeee)(GLenum, GLenum, GLenum, GLenum);
typedef void (APIENTRY *PFN_ffff)(GLclampf, GLclampf, GLclampf, GLclampf);
typedef void (APIENTRY *PFN_fff)(GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *PFN_ii)(GLint, GLint);
typedef void (APIENTRY *PFN_multiDrawArrays)(GLenum, const GLint *, const GLsizei *, GLsizei);

static void page_gl14(void)
{
    static const char *procs[] = { "glBlendFuncSeparate", "glBlendColor", "glBlendEquation", "glFogCoordf",
        "glFogCoordPointer", "glPointParameterf", "glPointParameteri", "glMultiDrawArrays", "glMultiDrawElements",
        "glWindowPos2i", "glWindowPos3fv", "glSecondaryColor3f", "glSecondaryColor3ubv" };
    static GLuint texHalf, texLevels, texGen;
    static GLubyte half[4 * 4 * 4], cyan[8 * 8 * 4];
    static const GLfloat twoQuads[8][2] = { {0, 0}, {40, 0}, {40, 40}, {0, 40},  {60, 0}, {100, 0}, {100, 40}, {60, 40} };
    static const GLint firsts[2] = { 0, 4 };
    static const GLsizei counts[2] = { 4, 4 };
    PFN_eeee blendFuncSeparate = (PFN_eeee)getproc("glBlendFuncSeparate");
    PFN_ffff blendColor = (PFN_ffff)getproc("glBlendColor");
    PFN_e blendEquation = (PFN_e)getproc("glBlendEquation");
    PFN_multiDrawArrays multiDrawArrays = (PFN_multiDrawArrays)getproc("glMultiDrawArrays");
    PFN_ii windowPos2i = (PFN_ii)getproc("glWindowPos2i");
    PFN_fff secondaryColor3f = (PFN_fff)getproc("glSecondaryColor3f");
    GLubyte px[4];
    GLint v[4];
    GLfloat f[4];
    int i, missing = 0, y = g_h - 80;

    ortho();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND);
    if (g_ver < 14) {
        if (g_checking) {
            info("the driver reports %d.%d: 1.4 must not be offered", g_ver / 10, g_ver % 10);
            check(blendFuncSeparate == NULL, "glBlendFuncSeparate is not handed out");
            check(windowPos2i == NULL, "glWindowPos2i is not handed out");
        }
        return;
    }
    if (g_checking) {
        for (i = 0; i < sizeof(procs) / sizeof(procs[0]); i++)
            if (getproc(procs[i]) == NULL) { missing++; info("  missing: %s", procs[i]); }
        check(missing == 0, "the 1.4 entry points are handed out (%d missing)", missing);
        glerr();
        if (blendFuncSeparate != NULL) {
            blendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
            glGetIntegerv(GL_BLEND_SRC_RGB_, v); glGetIntegerv(GL_BLEND_DST_RGB_, v + 1);
            glGetIntegerv(GL_BLEND_SRC_ALPHA_, v + 2); glGetIntegerv(GL_BLEND_DST_ALPHA_, v + 3);
            check(*glerr() == 0 && v[0] == GL_SRC_ALPHA && v[1] == GL_ONE_MINUS_SRC_ALPHA && v[2] == GL_ONE && v[3] == GL_ZERO,
                  "glBlendFuncSeparate reads back");
            glBlendFunc(GL_ONE, GL_ZERO);
        }
        if (blendColor != NULL) {
            blendColor(0.25f, 0.5f, 0.75f, 1.0f);
            f[0] = f[1] = f[2] = f[3] = 0; glGetFloatv(GL_BLEND_COLOR_, f);
            check(*glerr() == 0 && f[0] == 0.25f && f[1] == 0.5f && f[2] == 0.75f && f[3] == 1.0f, "glBlendColor reads back");
        }
        if (blendEquation != NULL) {
            blendEquation(GL_MIN_);
            check(*glerr() == 0, "glBlendEquation(GL_MIN) is accepted");
            blendEquation(GL_FUNC_ADD_);
        }
        if (secondaryColor3f != NULL) {
            secondaryColor3f(0.5f, 0.25f, 1.0f);
            f[0] = f[1] = f[2] = 0; glGetFloatv(GL_CURRENT_SECONDARY_COLOR_, f);
            check(*glerr() == 0 && f[0] == 0.5f && f[1] == 0.25f && f[2] == 1.0f, "glSecondaryColor3f reads back");
            glEnable(GL_COLOR_SUM_);
            check(*glerr() == 0 && glIsEnabled(GL_COLOR_SUM_), "GL_COLOR_SUM can be enabled");
            glDisable(GL_COLOR_SUM_);
        }
        {   /* a depth texture */
            static GLubyte d[16];
            GLuint t;
            glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT_, 4, 4, 0, GL_DEPTH_COMPONENT_, GL_UNSIGNED_BYTE, d);
            check(*glerr() == 0, "a GL_DEPTH_COMPONENT texture is accepted");
            glDeleteTextures(1, &t);
        }
        /* GL_GENERATE_MIPMAP: level 0 half white, half black; the 1x1 level must be grey */
        {
            static GLubyte img[16 * 16 * 4], one[4];
            for (i = 0; i < 256; i++) {
                GLubyte c = (GLubyte)((i % 16) < 8 ? 255 : 0);
                img[i * 4] = img[i * 4 + 1] = img[i * 4 + 2] = c; img[i * 4 + 3] = 255;
            }
            if (texGen == 0) glGenTextures(1, &texGen);
            glBindTexture(GL_TEXTURE_2D, texGen);
            glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_, GL_TRUE);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4); glPixelStorei(GL_PACK_ALIGNMENT, 4);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
            v[0] = 0; glGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_WIDTH, v);
            one[0] = one[1] = one[2] = 0;
            glGetTexImage(GL_TEXTURE_2D, 4, GL_RGBA, GL_UNSIGNED_BYTE, one);
            check(*glerr() == 0 && v[0] == 8 && abs(one[0] - 127) <= 24,
                  "GL_GENERATE_MIPMAP: level 1 is 8 wide (%d), the 1x1 level is grey (%d)", v[0], one[0]);            /* the same after glCopyTexImage2D: the levels must follow the new level 0 */
            glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 8, 8, 0);
            v[0] = 0; glGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_WIDTH, v);
            v[1] = -1; glGetTexLevelParameteriv(GL_TEXTURE_2D, 4, GL_TEXTURE_WIDTH, v + 1);
            check(*glerr() == 0 && v[0] == 4, "and after glCopyTexImage2D of 8x8: level 1 is 4 wide (%d)", v[0]);
        }
        glHint(0x8192 /* GL_GENERATE_MIPMAP_HINT */, GL_NICEST);
        check(*glerr() == 0, "glHint(GL_GENERATE_MIPMAP_HINT) is accepted");
        glEnableClientState(0x845E /* GL_SECONDARY_COLOR_ARRAY */);
        check(*glerr() == 0 && glIsEnabled(0x845E), "GL_SECONDARY_COLOR_ARRAY can be enabled");
        glDisableClientState(0x845E);
        glTexEnvi(GL_TEXTURE_ENV, 0x8580 /* GL_SOURCE0_RGB */, GL_TEXTURE0_);
        check(*glerr() == 0, "GL_TEXTURE0 as a combine source (crossbar) is accepted");
        glTexEnvi(GL_TEXTURE_ENV, 0x8580, GL_TEXTURE);
        glerr();

    }
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glColor3f(1, 1, 1);
    /* row 1: mirrored repeat.  Left half of the texture red, right half green; s runs 1..2 */
    if (texHalf == 0) {
        for (i = 0; i < 16; i++) {
            half[i * 4] = (GLubyte)((i % 4) < 2 ? 255 : 0); half[i * 4 + 1] = (GLubyte)((i % 4) < 2 ? 0 : 255);
            half[i * 4 + 2] = 0; half[i * 4 + 3] = 255;
        }
        glGenTextures(1, &texHalf); glBindTexture(GL_TEXTURE_2D, texHalf);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, half);
    }
    glBindTexture(GL_TEXTURE_2D, texHalf);
    for (i = 0; i < 2; i++) {
        float x = (float)(10 + i * 120);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, i == 0 ? GL_REPEAT : GL_MIRRORED_REPEAT_);
        glBegin(GL_QUADS);
        glTexCoord2f(1, 0); glVertex2f(x, (float)y);
        glTexCoord2f(2, 0); glVertex2f(x + 100, (float)y);
        glTexCoord2f(2, 1); glVertex2f(x + 100, (float)(y + 40));
        glTexCoord2f(1, 1); glVertex2f(x, (float)(y + 40));
        glEnd();
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    if (g_checking) {
        GLubyte q[4];
        readpx(30, y + 20, px); readpx(150, y + 20, q);
        check(*glerr() == 0 && nearc(px, 255, 0, 0, fbtol() + 30) && nearc(q, 0, 255, 0, fbtol() + 30),
              "GL_MIRRORED_REPEAT: repeat starts red, mirror starts green (%d %d %d / %d %d %d)",
              px[0], px[1], px[2], q[0], q[1], q[2]);
    }
    /* row 2: LOD bias on the coloured mip chain */
    y -= 60;
    if (texLevels == 0) {
        glGenTextures(1, &texLevels); glBindTexture(GL_TEXTURE_2D, texLevels);
        for (i = 0; i < 6; i++) solidLevel(i, 32 >> i, g_levelColor[i]);
    }
    glBindTexture(GL_TEXTURE_2D, texLevels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    quad(10, (float)y, 32, 32);
    glTexEnvf(GL_TEXTURE_FILTER_CONTROL_, GL_TEXTURE_LOD_BIAS_, 2.0f);
    quad(60, (float)y, 32, 32);
    glTexEnvf(GL_TEXTURE_FILTER_CONTROL_, GL_TEXTURE_LOD_BIAS_, 0.0f);
    if (g_checking) {
        int a, b;
        const char *e = glerr();
        readpx(26, y + 16, px); a = levelOf(px);
        readpx(76, y + 16, px); b = levelOf(px);
        check(*e == 0 && a == 0 && b >= 1, "GL_TEXTURE_LOD_BIAS 2: a 32 px quad goes from level %d to level %d", a, b);
    }
    glDisable(GL_TEXTURE_2D);
    /* row 3: glMultiDrawArrays, two quads */
    y -= 60;
    if (multiDrawArrays != NULL) {
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, twoQuads);
        glPushMatrix(); glTranslatef(10, (float)y, 0);
        glColor3f(1, 0.5f, 0);
        multiDrawArrays(GL_QUADS, firsts, counts, 2);
        glPopMatrix();
        glDisableClientState(GL_VERTEX_ARRAY);
        if (g_checking) {
            GLubyte q[4];
            const char *e = glerr();
            readpx(30, y + 20, px); readpx(90, y + 20, q);
            check(*e == 0 && nearc(px, 255, 128, 0, fbtol() + 10) && nearc(q, 255, 128, 0, fbtol() + 10),
                  "glMultiDrawArrays draws both quads (%d %d %d / %d %d %d)", px[0], px[1], px[2], q[0], q[1], q[2]);
        }
    }
    /* row 4: glWindowPos + glDrawPixels */
    y -= 40;
    if (windowPos2i != NULL) {
        for (i = 0; i < 64; i++) { cyan[i * 4] = 0; cyan[i * 4 + 1] = 255; cyan[i * 4 + 2] = 255; cyan[i * 4 + 3] = 255; }
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glTranslatef(500, 500, 0);     /* must not matter */
        windowPos2i(10, y);
        glPopMatrix();
        glDrawPixels(8, 8, GL_RGBA, GL_UNSIGNED_BYTE, cyan);
        if (g_checking) {
            const char *e = glerr();
            f[0] = f[1] = -1; glGetFloatv(GL_CURRENT_RASTER_POSITION, f);
            readpx(14, y + 4, px);
            check(*e == 0 && f[0] == 10.0f && f[1] == (GLfloat)y && nearc(px, 0, 255, 255, fbtol() + 10),
                  "glWindowPos2i + glDrawPixels: position %g,%g, pixel %d %d %d", f[0], f[1], px[0], px[1], px[2]);
        }
    }
    glerr();
}

/* =====================================================================================================================
** page 10: OpenGL 1.5: buffer objects, occlusion queries
*/
typedef void (APIENTRY *PFN_genNames)(GLsizei, GLuint *);
typedef void (APIENTRY *PFN_delNames)(GLsizei, const GLuint *);
typedef GLboolean (APIENTRY *PFN_isName)(GLuint);
typedef void (APIENTRY *PFN_bindBuffer)(GLenum, GLuint);
typedef void (APIENTRY *PFN_bufferData)(GLenum, int, const GLvoid *, GLenum);
typedef void (APIENTRY *PFN_bufferSubData)(GLenum, int, int, const GLvoid *);
typedef void (APIENTRY *PFN_getBufferSubData)(GLenum, int, int, GLvoid *);
typedef GLvoid *(APIENTRY *PFN_mapBuffer)(GLenum, GLenum);
typedef GLboolean (APIENTRY *PFN_unmapBuffer)(GLenum);
typedef void (APIENTRY *PFN_getBufferParam)(GLenum, GLenum, GLint *);
typedef void (APIENTRY *PFN_beginQuery)(GLenum, GLuint);
typedef void (APIENTRY *PFN_endQuery)(GLenum);
typedef void (APIENTRY *PFN_getQueryiv)(GLenum, GLenum, GLint *);
typedef void (APIENTRY *PFN_getQueryObjectuiv)(GLuint, GLenum, GLuint *);
#define GL_ARRAY_BUFFER_15          0x8892
#define GL_ELEMENT_ARRAY_BUFFER_15  0x8893
typedef struct { GLfloat x, y; GLubyte r, g, b, a; } BufVert;
static void bufQuad(BufVert *v, float x, float y, float w, int r, int g, int b)
{
    int i;
    v[0].x = x; v[0].y = y; v[1].x = x + w; v[1].y = y; v[2].x = x + w; v[2].y = y + w; v[3].x = x; v[3].y = y + w;
    for (i = 0; i < 4; i++) { v[i].r = (GLubyte)r; v[i].g = (GLubyte)g; v[i].b = (GLubyte)b; v[i].a = 255; }
}
static void page_gl15(void)
{
    static const char *procs[] = { "glGenBuffers", "glDeleteBuffers", "glIsBuffer", "glBindBuffer", "glBufferData",
        "glBufferSubData", "glGetBufferSubData", "glMapBuffer", "glUnmapBuffer", "glGetBufferParameteriv",
        "glGetBufferPointerv", "glGenQueries", "glDeleteQueries", "glIsQuery", "glBeginQuery", "glEndQuery",
        "glGetQueryiv", "glGetQueryObjectiv", "glGetQueryObjectuiv", "glBindBufferARB", "glBufferDataARB" };
    static const GLushort quadIdx[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    PFN_genNames genBuffers = (PFN_genNames)getproc("glGenBuffers"), genQueries = (PFN_genNames)getproc("glGenQueries");
    PFN_delNames deleteBuffers = (PFN_delNames)getproc("glDeleteBuffers"), deleteQueries = (PFN_delNames)getproc("glDeleteQueries");
    PFN_isName isBuffer = (PFN_isName)getproc("glIsBuffer");
    PFN_bindBuffer bindBuffer = (PFN_bindBuffer)getproc("glBindBuffer");
    PFN_bufferData bufferData = (PFN_bufferData)getproc("glBufferData");
    PFN_bufferSubData bufferSubData = (PFN_bufferSubData)getproc("glBufferSubData");
    PFN_getBufferSubData getBufferSubData = (PFN_getBufferSubData)getproc("glGetBufferSubData");
    PFN_mapBuffer mapBuffer = (PFN_mapBuffer)getproc("glMapBuffer");
    PFN_unmapBuffer unmapBuffer = (PFN_unmapBuffer)getproc("glUnmapBuffer");
    PFN_getBufferParam getBufferParameteriv = (PFN_getBufferParam)getproc("glGetBufferParameteriv");
    PFN_beginQuery beginQuery = (PFN_beginQuery)getproc("glBeginQuery");
    PFN_endQuery endQuery = (PFN_endQuery)getproc("glEndQuery");
    PFN_getQueryiv getQueryiv = (PFN_getQueryiv)getproc("glGetQueryiv");
    PFN_getQueryObjectuiv getQueryObjectuiv = (PFN_getQueryObjectuiv)getproc("glGetQueryObjectuiv");
    BufVert verts[8], back[4], *map;
    GLuint buf[2] = { 0, 0 }, query = 0, samples;
    GLubyte px[4], q[4];
    GLint v[4];
    GLvoid *ptr;
    const char *e;
    int i, missing = 0, y = g_h - 80, ok;

    ortho();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDisable(GL_TEXTURE_2D);
    if (g_ver < 15) {
        if (g_checking) {
            info("the driver reports %d.%d: 1.5 must not be offered", g_ver / 10, g_ver % 10);
            check(bindBuffer == NULL, "glBindBuffer is not handed out");
            check(beginQuery == NULL, "glBeginQuery is not handed out");
        }
        return;
    }
    for (i = 0; i < sizeof(procs) / sizeof(procs[0]); i++)
        if (getproc(procs[i]) == NULL) { missing++; if (g_checking) info("  missing: %s", procs[i]); }
    if (g_checking) check(missing == 0, "the 1.5 entry points are handed out (%d missing)", missing);
    glerr();
    if (missing) return;

    /* ---- buffer objects: state ---- */
    genBuffers(2, buf);
    bindBuffer(GL_ARRAY_BUFFER_15, buf[0]);
    bufQuad(verts, 10, (float)y, 40, 255, 0, 0);
    bufQuad(verts + 4, 60, (float)y, 40, 0, 255, 0);
    bufferData(GL_ARRAY_BUFFER_15, 4 * sizeof(BufVert), verts, 0x88E4 /* GL_STATIC_DRAW */);
    if (g_checking) {
        e = glerr();
        v[0] = v[1] = v[2] = -1;
        glGetIntegerv(0x8894 /* GL_ARRAY_BUFFER_BINDING */, v);
        getBufferParameteriv(GL_ARRAY_BUFFER_15, 0x8764 /* GL_BUFFER_SIZE */, v + 1);
        getBufferParameteriv(GL_ARRAY_BUFFER_15, 0x8765 /* GL_BUFFER_USAGE */, v + 2);
        check(*e == 0 && buf[0] != 0 && buf[1] != 0 && buf[0] != buf[1] && isBuffer(buf[0]) && !isBuffer(buf[1]) &&
              v[0] == (GLint)buf[0] && v[1] == 4 * sizeof(BufVert) && v[2] == 0x88E4,
              "glGenBuffers / glBindBuffer / glBufferData: binding %d, size %d, usage 0x%x %s", v[0], v[1], v[2], e);
        memset(back, 0, sizeof(back));
        bufferSubData(GL_ARRAY_BUFFER_15, 2 * sizeof(BufVert), sizeof(BufVert), verts + 2);
        getBufferSubData(GL_ARRAY_BUFFER_15, 0, sizeof(back), back);
        check(noerr("glBufferSubData / glGetBufferSubData") && memcmp(back, verts, sizeof(back)) == 0,
              "glGetBufferSubData returns what glBufferData stored");
        /* written through the mapping, read back with glGetBufferSubData.  (What the mapping holds on entry is only
        ** reported: wine's 32-bit layer hands out a copy it does not fill) */
        map = (BufVert *)mapBuffer(GL_ARRAY_BUFFER_15, 0x88BA /* GL_READ_WRITE */);
        v[0] = 0; getBufferParameteriv(GL_ARRAY_BUFFER_15, 0x88BC /* GL_BUFFER_MAPPED */, v);
        ok = map != NULL && v[0] == 1;
        if (map != NULL) {
            info("  the mapping %s the buffer's data on entry", memcmp(map, verts, 4 * sizeof(BufVert)) == 0 ? "holds" : "does NOT hold");
            memcpy(map, verts, 4 * sizeof(BufVert));
            map[1].g = 77;
            ok = unmapBuffer(GL_ARRAY_BUFFER_15) && ok;
            memset(back, 0, sizeof(back));
            getBufferSubData(GL_ARRAY_BUFFER_15, 0, sizeof(back), back);
            ok = ok && back[1].g == 77 && back[3].x == verts[3].x;
            bufferSubData(GL_ARRAY_BUFFER_15, 0, 4 * sizeof(BufVert), verts);
        }
        e = glerr();
        check(ok && *e == 0, "glMapBuffer: what is written through it is in the buffer after glUnmapBuffer %s", e);
        bufferSubData(GL_ARRAY_BUFFER_15, 3 * sizeof(BufVert), 2 * sizeof(BufVert), verts);
        check(glGetError() == GL_INVALID_VALUE, "glBufferSubData past the end: GL_INVALID_VALUE");
        glerr();
    }

    /* ---- row 1: glDrawArrays out of the buffer; the pointers are offsets ---- */
    glEnableClientState(GL_VERTEX_ARRAY); glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, sizeof(BufVert), (const GLvoid *)0);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(BufVert), (const GLvoid *)8);
    glDrawArrays(GL_QUADS, 0, 4);
    if (g_checking) {
        e = glerr();
        readpx(30, y + 20, px);
        check(*e == 0 && nearc(px, 255, 0, 0, fbtol()), "glDrawArrays from a buffer object: red (%d %d %d) %s",
              px[0], px[1], px[2], e);
        v[0] = -1; ptr = (GLvoid *)-1;
        glGetIntegerv(0x8898 /* GL_COLOR_ARRAY_BUFFER_BINDING */, v);
        glGetPointerv(GL_COLOR_ARRAY_POINTER, &ptr);
        check(v[0] == (GLint)buf[0], "GL_COLOR_ARRAY_BUFFER_BINDING is the buffer (%d)", v[0]);
        info("  glGetPointerv of that array: %d (the offset, 8, is right)", (int)ptr);
        glerr();
    }
    /* the buffer grows after the pointers were set: both quads must come out */
    bufferData(GL_ARRAY_BUFFER_15, 64 * 1024, NULL, 0x88E8 /* GL_DYNAMIC_DRAW */);
    bufferSubData(GL_ARRAY_BUFFER_15, 0, 8 * sizeof(BufVert), verts);
    bindBuffer(GL_ARRAY_BUFFER_15, 0);                  /* the arrays keep their buffer */
    glDrawArrays(GL_QUADS, 0, 8);
    if (g_checking) {
        e = glerr();
        readpx(30, y + 20, px); readpx(80, y + 20, q);
        check(*e == 0 && nearc(px, 255, 0, 0, fbtol()) && nearc(q, 0, 255, 0, fbtol()),
              "after glBufferData made it larger: red and green (%d %d %d / %d %d %d) %s",
              px[0], px[1], px[2], q[0], q[1], q[2], e);
    }

    /* ---- row 2: glDrawElements with the indices in an element buffer ---- */
    y -= 60;
    bindBuffer(GL_ARRAY_BUFFER_15, buf[0]);
    bufQuad(verts, 10, (float)y, 40, 0, 0, 255);
    bufQuad(verts + 4, 60, (float)y, 40, 255, 255, 0);
    bufferSubData(GL_ARRAY_BUFFER_15, 0, 8 * sizeof(BufVert), verts);
    bindBuffer(GL_ELEMENT_ARRAY_BUFFER_15, buf[1]);
    bufferData(GL_ELEMENT_ARRAY_BUFFER_15, sizeof(quadIdx), quadIdx, 0x88E4);
    glDrawElements(GL_QUADS, 4, GL_UNSIGNED_SHORT, (const GLvoid *)(4 * sizeof(GLushort)));   /* the second quad */
    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, (const GLvoid *)0);                 /* the first one */
    if (g_checking) {
        e = glerr();
        readpx(30, y + 20, px); readpx(80, y + 20, q);
        check(*e == 0 && nearc(px, 0, 0, 255, fbtol()) && nearc(q, 255, 255, 0, fbtol()),
              "glDrawElements from an element buffer: blue and yellow (%d %d %d / %d %d %d) %s",
              px[0], px[1], px[2], q[0], q[1], q[2], e);
    }
    bindBuffer(GL_ELEMENT_ARRAY_BUFFER_15, 0);
    bindBuffer(GL_ARRAY_BUFFER_15, 0);

    /* ---- row 3: back to plain client arrays ---- */
    y -= 60;
    bufQuad(verts, 10, (float)y, 40, 255, 0, 255);
    glVertexPointer(2, GL_FLOAT, sizeof(BufVert), &verts[0].x);
    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(BufVert), &verts[0].r);
    glDrawElements(GL_QUADS, 4, GL_UNSIGNED_SHORT, quadIdx);
    if (g_checking) {
        e = glerr();
        readpx(30, y + 20, px);
        v[0] = -1; glGetIntegerv(0x8896 /* GL_VERTEX_ARRAY_BUFFER_BINDING */, v);
        check(*e == 0 && v[0] == 0 && nearc(px, 255, 0, 255, fbtol()),
              "client arrays again after the buffers are unbound: magenta (%d %d %d) %s", px[0], px[1], px[2], e);
    }
    glDisableClientState(GL_VERTEX_ARRAY); glDisableClientState(GL_COLOR_ARRAY);
    deleteBuffers(2, buf);
    if (g_checking)
        check(*glerr() == 0 && !isBuffer(buf[0]), "glDeleteBuffers");

    /* ---- row 4: an occlusion query around a visible quad ---- */
    y -= 60;
    genQueries(1, &query);
    beginQuery(0x8914 /* GL_SAMPLES_PASSED */, query);
    glColor3f(0, 1, 1);
    quad(10, (float)y, 40, 40);
    endQuery(0x8914);
    if (g_checking) {
        e = glerr();
        v[0] = -1; samples = 0; v[1] = 0;
        getQueryiv(0x8914, 0x8864 /* GL_QUERY_COUNTER_BITS */, v);
        getQueryObjectuiv(query, 0x8867 /* GL_QUERY_RESULT_AVAILABLE */, (GLuint *)&v[1]);
        getQueryObjectuiv(query, 0x8866 /* GL_QUERY_RESULT */, &samples);
        check(*e == 0 && *glerr() == 0 && query != 0 && samples > 0,
              "occlusion query of a visible 40 x 40 quad: %u samples (counter bits %d) %s", samples, v[0], e);
        /* a quad behind a nearer one: 0 from a driver that counts, not 0 from one that only accepts queries */
        glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS);
        glClear(GL_DEPTH_BUFFER_BIT);
        glBegin(GL_QUADS); glVertex3f(60, (float)y, 0.5f); glVertex3f(100, (float)y, 0.5f);
        glVertex3f(100, (float)y + 40, 0.5f); glVertex3f(60, (float)y + 40, 0.5f); glEnd();
        beginQuery(0x8914, query);
        glBegin(GL_QUADS); glVertex3f(60, (float)y, -0.5f); glVertex3f(100, (float)y, -0.5f);
        glVertex3f(100, (float)y + 40, -0.5f); glVertex3f(60, (float)y + 40, -0.5f); glEnd();
        endQuery(0x8914);
        glDisable(GL_DEPTH_TEST);
        samples = 12345; getQueryObjectuiv(query, 0x8866, &samples);
        info("  a hidden quad: %u samples (0 = really counted; the 3dfx ICD says visible)", samples);
        /* shadow compare functions other than LEQUAL / GEQUAL are accepted */
        glTexParameteri(GL_TEXTURE_2D, 0x884D /* GL_TEXTURE_COMPARE_FUNC */, GL_GREATER);
        check(*glerr() == 0, "GL_TEXTURE_COMPARE_FUNC takes GL_GREATER (1.5 shadow functions)");
        glTexParameteri(GL_TEXTURE_2D, 0x884D, GL_LEQUAL);
    }
    deleteQueries(1, &query);
    glerr();
}

/* ---- the pages ---- */
typedef struct { const char *title; void (*draw)(void); } Page;
static const Page g_pages[] = {
    { "driver",                         page_info },
    { "reading pixels back",            page_readback },
    { "OpenGL 1.2: state and queries",  page_queries12 },
    { "OpenGL 1.2: packed pixel types", page_packed },
    { "OpenGL 1.2: texture base / max level", page_levels },
    { "OpenGL 1.2: rescale normal",     page_rescale },
    { "OpenGL 1.2: glDrawRangeElements", page_drawrange },
    { "OpenGL 1.3",                     page_gl13 },
    { "OpenGL 1.4",                     page_gl14 },
    { "OpenGL 1.5",                     page_gl15 },
};
#define NPAGES ((int)(sizeof(g_pages) / sizeof(g_pages[0])))
static int g_page, g_runChecks = 1;
static int g_only;                      /* -page n: with -auto, only that page (1-based) */

static void saveBmp(int n)
{
    char name[64];
    BITMAPFILEHEADER fh;
    BITMAPINFOHEADER ih;
    int row = (g_w * 3 + 3) & ~3, x, y;
    GLubyte *rgba = (GLubyte *)malloc(g_w * g_h * 4), *out = (GLubyte *)calloc(row, g_h);
    FILE *f;

    if (rgba == NULL || out == NULL) { free(rgba); free(out); return; }
    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, g_w, g_h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    for (y = 0; y < g_h; y++)
        for (x = 0; x < g_w; x++) {
            out[y * row + x * 3]     = rgba[(y * g_w + x) * 4 + 2];
            out[y * row + x * 3 + 1] = rgba[(y * g_w + x) * 4 + 1];
            out[y * row + x * 3 + 2] = rgba[(y * g_w + x) * 4];
        }
    sprintf(name, "gltest_%02d.bmp", n);
    f = fopen(name, "wb");
    if (f != NULL) {
        memset(&fh, 0, sizeof(fh)); memset(&ih, 0, sizeof(ih));
        fh.bfType = 0x4d42; fh.bfOffBits = sizeof(fh) + sizeof(ih); fh.bfSize = fh.bfOffBits + row * g_h;
        ih.biSize = sizeof(ih); ih.biWidth = g_w; ih.biHeight = g_h; ih.biPlanes = 1; ih.biBitCount = 24;
        fwrite(&fh, sizeof(fh), 1, f); fwrite(&ih, sizeof(ih), 1, f); fwrite(out, row, g_h, f);
        fclose(f);
    }
    free(rgba); free(out);
    glerr();
}

static void frame(void)
{
    glViewport(0, 0, g_w, g_h);
    glDrawBuffer(GL_BACK);
    glClearColor(0.15f, 0.15f, 0.2f, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (g_runChecks) {
        g_textLines = 0;
        g_checking = 1;
        if (g_log) fprintf(g_log, "\n== page %d: %s\n", g_page + 1, g_pages[g_page].title);
        info("page %d of %d: %s", g_page + 1, NPAGES, g_pages[g_page].title);
    }
    g_pages[g_page].draw();
    if (g_runChecks) {
        info("left / right: page   space: again   Esc: quit");
        g_checking = 0;
        g_runChecks = 0;
    }
    ortho();
    drawText();
    glFinish();
    {   /* is the text panel in the frame buffer?  Its background is black, the clear colour is not */
        static int told[16];
        GLubyte px[4];
        int tell = g_log && g_page < 16 && !told[g_page], missing;
        readpx(g_w - 6, g_h - PANEL_H + 6, px);
        missing = !(px[0] < 20 && px[1] < 20 && px[2] < 20);
        if (tell) {
            told[g_page] = 1;
            fprintf(g_log, "       (text panel read back from the frame: %s, %d %d %d)\n",
                    missing ? "MISSING" : "there", px[0], px[1], px[2]);
        }
        if (tell && missing) {      /* late, or never?  And is it on the screen after the swap? */
            Sleep(300);
            readpx(g_w - 6, g_h - PANEL_H + 6, px);
            fprintf(g_log, "       (the same read 300 ms later: %d %d %d)\n", px[0], px[1], px[2]);
            if (g_auto) saveBmp(g_page + 1);
            SwapBuffers(g_hdc);
            glFinish();
            glReadBuffer(GL_FRONT);
            glReadPixels(g_w - 6, g_h - PANEL_H + 6, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
            fprintf(g_log, "       (from the front buffer after the swap: %d %d %d)\n", px[0], px[1], px[2]);
            glReadBuffer(GL_BACK);
            return;
        }
    }
    if (g_auto) saveBmp(g_page + 1);
    SwapBuffers(g_hdc);
}

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
      case WM_KEYDOWN:
        if (wp == VK_ESCAPE) PostQuitMessage(0);
        else if (wp == VK_RIGHT) { g_page = (g_page + 1) % NPAGES; g_runChecks = 1; }
        else if (wp == VK_LEFT)  { g_page = (g_page + NPAGES - 1) % NPAGES; g_runChecks = 1; }
        else if (wp == VK_SPACE) g_runChecks = 1;
        return 0;
      case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    WNDCLASSA wc;
    PIXELFORMATDESCRIPTOR pfd;
    DEVMODEA dm;
    MSG msg;
    const char *v;
    int pf, i, quit = 0;

    (void)prev; (void)show;
    for (i = 1; i < __argc; i++) {
        if (!strcmp(__argv[i], "-w") && i + 1 < __argc) g_w = atoi(__argv[++i]);
        else if (!strcmp(__argv[i], "-h") && i + 1 < __argc) g_h = atoi(__argv[++i]);
        else if (!strcmp(__argv[i], "-bpp") && i + 1 < __argc) g_bpp = atoi(__argv[++i]);
        else if (!strcmp(__argv[i], "-auto")) g_auto = 1;
        else if (!strcmp(__argv[i], "-window")) g_windowed = 1;
        else if (!strcmp(__argv[i], "-page") && i + 1 < __argc) g_only = atoi(__argv[++i]);
    }
    (void)cmd;
    g_log = fopen("gltest.log", "w");
    if (!g_windowed) {
        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        dm.dmPelsWidth = g_w; dm.dmPelsHeight = g_h; dm.dmBitsPerPel = g_bpp;
        dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
        if (ChangeDisplaySettingsA(&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
            if (g_log) fprintf(g_log, "could not switch to %dx%dx%d, using the desktop mode\n", g_w, g_h, g_bpp);
            g_w = GetSystemMetrics(SM_CXSCREEN); g_h = GetSystemMetrics(SM_CYSCREEN);
        }
    }
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_OWNDC; wc.lpfnWndProc = wndProc; wc.hInstance = inst; wc.lpszClassName = "gltest";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);
    if (g_windowed)
        g_hwnd = CreateWindowA("gltest", "gltest", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 20, 20, g_w + 8, g_h + 27, NULL,
                               NULL, inst, NULL);
    else
        g_hwnd = CreateWindowExA(WS_EX_TOPMOST, "gltest", "gltest", WS_POPUP | WS_VISIBLE, 0, 0, g_w, g_h, NULL, NULL,
                                 inst, NULL);
    if (g_windowed) { RECT r; GetClientRect(g_hwnd, &r); g_w = r.right; g_h = r.bottom; }
    g_hdc = GetDC(g_hwnd);
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd); pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = (BYTE)g_bpp;
    pfd.cDepthBits = (BYTE)(g_bpp > 16 ? 24 : 16);
    pf = ChoosePixelFormat(g_hdc, &pfd);
    if (pf == 0 || !SetPixelFormat(g_hdc, pf, &pfd) || (g_hrc = wglCreateContext(g_hdc)) == NULL ||
        !wglMakeCurrent(g_hdc, g_hrc)) {
        if (g_log) { fprintf(g_log, "no OpenGL context (pixel format %d, error %lu)\n", pf, GetLastError()); fclose(g_log); }
        ChangeDisplaySettingsA(NULL, 0);
        MessageBoxA(NULL, "Could not create an OpenGL context.", "gltest", MB_OK);
        return 1;
    }
    DescribePixelFormat(g_hdc, pf, sizeof(pfd), &pfd);
    g_bpp = pfd.cColorBits;
    v = (const char *)glGetString(GL_VERSION);
    if (v != NULL && v[0] >= '0' && v[0] <= '9' && v[1] == '.' && v[2] >= '0' && v[2] <= '9')
        g_ver = (v[0] - '0') * 10 + (v[2] - '0');
    else
        g_ver = 11;
    if (g_log) fprintf(g_log, "gltest: %s / %s / %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER), v ? v : "?");

    if (g_auto) {
        for (g_page = 0; g_page < NPAGES; g_page++) {
            if (g_only && g_page + 1 != g_only) continue;
            g_runChecks = 1;
            frame();
            Sleep(700);
            while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) DispatchMessageA(&msg);
        }
    } else {
        while (!quit) {
            while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) quit = 1;
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
            frame();
            Sleep(30);
        }
    }
    if (g_log) {
        fprintf(g_log, "\n== %d passed, %d failed\n", g_pass, g_fail);
        fclose(g_log);
    }
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(g_hrc);
    ReleaseDC(g_hwnd, g_hdc);
    DestroyWindow(g_hwnd);
    if (!g_windowed) ChangeDisplaySettingsA(NULL, 0);
    return g_fail != 0;
}

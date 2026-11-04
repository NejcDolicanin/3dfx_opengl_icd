/*
** glstubs.c -- the gl* entry points exported by 3dfxOGL.dll and the extension entry points
** handed out by wglGetProcAddress.  Every one is the same 4-instruction stub in the
** original (69001338..69009640):
**     mov eax, fs:[18h]                    ; TEB self pointer
**     add eax, [g_tebOffContext]           ; + offset of the TLS slot holding the current gc
**     mov eax, [eax]                       ; gc
**     jmp dword ptr [eax + 1738h + slot*4] ; gc->dispatch[slot]
*/
#define _GDI32_
#include <windows.h>
#include <GL/gl.h>
extern DWORD g_tebOffContext;      /* 690f65d8 */

#ifdef _MSC_VER
#define GL_BODY(slot) { __asm mov eax, fs:[18h] __asm add eax, g_tebOffContext __asm mov eax, [eax] \
                        __asm jmp dword ptr [eax + 0x1738 + (slot) * 4] }
#define GL_STUB __declspec(naked)
#else
#define GL_BODY(slot) { }        /* syntax-check placeholder */
#define GL_STUB
#endif

GL_STUB void APIENTRY glNewList(GLuint list, GLenum mode) GL_BODY(1)   /* 69006390 */
GL_STUB void APIENTRY glEndList(void) GL_BODY(2)   /* 690063b0 */
GL_STUB void APIENTRY glCallList(GLuint list) GL_BODY(3)   /* 690063d0 */
GL_STUB void APIENTRY glCallLists(GLsizei n, GLenum type, const GLvoid *lists) GL_BODY(4)   /* 690063f0 */
GL_STUB void APIENTRY glDeleteLists(GLuint list, GLsizei range) GL_BODY(5)   /* 69006410 */
GL_STUB GLuint APIENTRY glGenLists(GLsizei range) GL_BODY(6)   /* 69006430 */
GL_STUB void APIENTRY glListBase(GLuint base) GL_BODY(7)   /* 69006450 */
GL_STUB void APIENTRY glBegin(GLenum mode) GL_BODY(8)   /* 69006470 */
GL_STUB void APIENTRY glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) GL_BODY(9)   /* 69006490 */
GL_STUB void APIENTRY glColor3b(GLbyte red, GLbyte green, GLbyte blue) GL_BODY(10)   /* 690064b0 */
GL_STUB void APIENTRY glColor3bv(const GLbyte *v) GL_BODY(11)   /* 690064d0 */
GL_STUB void APIENTRY glColor3d(GLdouble red, GLdouble green, GLdouble blue) GL_BODY(12)   /* 690064f0 */
GL_STUB void APIENTRY glColor3dv(const GLdouble *v) GL_BODY(13)   /* 69006510 */
GL_STUB void APIENTRY glColor3f(GLfloat red, GLfloat green, GLfloat blue) GL_BODY(14)   /* 69006530 */
GL_STUB void APIENTRY glColor3fv(const GLfloat *v) GL_BODY(15)   /* 69006550 */
GL_STUB void APIENTRY glColor3i(GLint red, GLint green, GLint blue) GL_BODY(16)   /* 69006570 */
GL_STUB void APIENTRY glColor3iv(const GLint *v) GL_BODY(17)   /* 69006590 */
GL_STUB void APIENTRY glColor3s(GLshort red, GLshort green, GLshort blue) GL_BODY(18)   /* 690065b0 */
GL_STUB void APIENTRY glColor3sv(const GLshort *v) GL_BODY(19)   /* 690065d0 */
GL_STUB void APIENTRY glColor3ub(GLubyte red, GLubyte green, GLubyte blue) GL_BODY(20)   /* 690065f0 */
GL_STUB void APIENTRY glColor3ubv(const GLubyte *v) GL_BODY(21)   /* 69006610 */
GL_STUB void APIENTRY glColor3ui(GLuint red, GLuint green, GLuint blue) GL_BODY(22)   /* 69006630 */
GL_STUB void APIENTRY glColor3uiv(const GLuint *v) GL_BODY(23)   /* 69006650 */
GL_STUB void APIENTRY glColor3us(GLushort red, GLushort green, GLushort blue) GL_BODY(24)   /* 69006670 */
GL_STUB void APIENTRY glColor3usv(const GLushort *v) GL_BODY(25)   /* 69006690 */
GL_STUB void APIENTRY glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha) GL_BODY(26)   /* 690066b0 */
GL_STUB void APIENTRY glColor4bv(const GLbyte *v) GL_BODY(27)   /* 690066d0 */
GL_STUB void APIENTRY glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) GL_BODY(28)   /* 690066f0 */
GL_STUB void APIENTRY glColor4dv(const GLdouble *v) GL_BODY(29)   /* 69006710 */
GL_STUB void APIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) GL_BODY(30)   /* 69006730 */
GL_STUB void APIENTRY glColor4fv(const GLfloat *v) GL_BODY(31)   /* 69006750 */
GL_STUB void APIENTRY glColor4i(GLint red, GLint green, GLint blue, GLint alpha) GL_BODY(32)   /* 69006770 */
GL_STUB void APIENTRY glColor4iv(const GLint *v) GL_BODY(33)   /* 69006790 */
GL_STUB void APIENTRY glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha) GL_BODY(34)   /* 690067b0 */
GL_STUB void APIENTRY glColor4sv(const GLshort *v) GL_BODY(35)   /* 690067d0 */
GL_STUB void APIENTRY glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) GL_BODY(36)   /* 690067f0 */
GL_STUB void APIENTRY glColor4ubv(const GLubyte *v) GL_BODY(37)   /* 69006810 */
GL_STUB void APIENTRY glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha) GL_BODY(38)   /* 69006830 */
GL_STUB void APIENTRY glColor4uiv(const GLuint *v) GL_BODY(39)   /* 69006850 */
GL_STUB void APIENTRY glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha) GL_BODY(40)   /* 69006870 */
GL_STUB void APIENTRY glColor4usv(const GLushort *v) GL_BODY(41)   /* 69006890 */
GL_STUB void APIENTRY glEdgeFlag(GLboolean flag) GL_BODY(42)   /* 690068b0 */
GL_STUB void APIENTRY glEdgeFlagv(const GLboolean *flag) GL_BODY(43)   /* 690068d0 */
GL_STUB void APIENTRY glEnd(void) GL_BODY(44)   /* 690068f0 */
GL_STUB void APIENTRY glIndexd(GLdouble c) GL_BODY(45)   /* 69006910 */
GL_STUB void APIENTRY glIndexdv(const GLdouble *c) GL_BODY(46)   /* 69006930 */
GL_STUB void APIENTRY glIndexf(GLfloat c) GL_BODY(47)   /* 69006950 */
GL_STUB void APIENTRY glIndexfv(const GLfloat *c) GL_BODY(48)   /* 69006970 */
GL_STUB void APIENTRY glIndexi(GLint c) GL_BODY(49)   /* 69006990 */
GL_STUB void APIENTRY glIndexiv(const GLint *c) GL_BODY(50)   /* 690069b0 */
GL_STUB void APIENTRY glIndexs(GLshort c) GL_BODY(51)   /* 690069d0 */
GL_STUB void APIENTRY glIndexsv(const GLshort *c) GL_BODY(52)   /* 690069f0 */
GL_STUB void APIENTRY glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz) GL_BODY(53)   /* 69006a10 */
GL_STUB void APIENTRY glNormal3bv(const GLbyte *v) GL_BODY(54)   /* 69006a30 */
GL_STUB void APIENTRY glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz) GL_BODY(55)   /* 69006a50 */
GL_STUB void APIENTRY glNormal3dv(const GLdouble *v) GL_BODY(56)   /* 69006a70 */
GL_STUB void APIENTRY glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) GL_BODY(57)   /* 69006a90 */
GL_STUB void APIENTRY glNormal3fv(const GLfloat *v) GL_BODY(58)   /* 69006ab0 */
GL_STUB void APIENTRY glNormal3i(GLint nx, GLint ny, GLint nz) GL_BODY(59)   /* 69006ad0 */
GL_STUB void APIENTRY glNormal3iv(const GLint *v) GL_BODY(60)   /* 69006af0 */
GL_STUB void APIENTRY glNormal3s(GLshort nx, GLshort ny, GLshort nz) GL_BODY(61)   /* 69006b10 */
GL_STUB void APIENTRY glNormal3sv(const GLshort *v) GL_BODY(62)   /* 69006b30 */
GL_STUB void APIENTRY glRasterPos2d(GLdouble x, GLdouble y) GL_BODY(63)   /* 69006b50 */
GL_STUB void APIENTRY glRasterPos2dv(const GLdouble *v) GL_BODY(64)   /* 69006b70 */
GL_STUB void APIENTRY glRasterPos2f(GLfloat x, GLfloat y) GL_BODY(65)   /* 69006b90 */
GL_STUB void APIENTRY glRasterPos2fv(const GLfloat *v) GL_BODY(66)   /* 69006bb0 */
GL_STUB void APIENTRY glRasterPos2i(GLint x, GLint y) GL_BODY(67)   /* 69006bd0 */
GL_STUB void APIENTRY glRasterPos2iv(const GLint *v) GL_BODY(68)   /* 69006bf0 */
GL_STUB void APIENTRY glRasterPos2s(GLshort x, GLshort y) GL_BODY(69)   /* 69006c10 */
GL_STUB void APIENTRY glRasterPos2sv(const GLshort *v) GL_BODY(70)   /* 69006c30 */
GL_STUB void APIENTRY glRasterPos3d(GLdouble x, GLdouble y, GLdouble z) GL_BODY(71)   /* 69006c50 */
GL_STUB void APIENTRY glRasterPos3dv(const GLdouble *v) GL_BODY(72)   /* 69006c70 */
GL_STUB void APIENTRY glRasterPos3f(GLfloat x, GLfloat y, GLfloat z) GL_BODY(73)   /* 69006c90 */
GL_STUB void APIENTRY glRasterPos3fv(const GLfloat *v) GL_BODY(74)   /* 69006cb0 */
GL_STUB void APIENTRY glRasterPos3i(GLint x, GLint y, GLint z) GL_BODY(75)   /* 69006cd0 */
GL_STUB void APIENTRY glRasterPos3iv(const GLint *v) GL_BODY(76)   /* 69006cf0 */
GL_STUB void APIENTRY glRasterPos3s(GLshort x, GLshort y, GLshort z) GL_BODY(77)   /* 69006d10 */
GL_STUB void APIENTRY glRasterPos3sv(const GLshort *v) GL_BODY(78)   /* 69006d30 */
GL_STUB void APIENTRY glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) GL_BODY(79)   /* 69006d50 */
GL_STUB void APIENTRY glRasterPos4dv(const GLdouble *v) GL_BODY(80)   /* 69006d70 */
GL_STUB void APIENTRY glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) GL_BODY(81)   /* 69006d90 */
GL_STUB void APIENTRY glRasterPos4fv(const GLfloat *v) GL_BODY(82)   /* 69006db0 */
GL_STUB void APIENTRY glRasterPos4i(GLint x, GLint y, GLint z, GLint w) GL_BODY(83)   /* 69006dd0 */
GL_STUB void APIENTRY glRasterPos4iv(const GLint *v) GL_BODY(84)   /* 69006df0 */
GL_STUB void APIENTRY glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w) GL_BODY(85)   /* 69006e10 */
GL_STUB void APIENTRY glRasterPos4sv(const GLshort *v) GL_BODY(86)   /* 69006e30 */
GL_STUB void APIENTRY glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) GL_BODY(87)   /* 69006e50 */
GL_STUB void APIENTRY glRectdv(const GLdouble *v1, const GLdouble *v2) GL_BODY(88)   /* 69006e70 */
GL_STUB void APIENTRY glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) GL_BODY(89)   /* 69006e90 */
GL_STUB void APIENTRY glRectfv(const GLfloat *v1, const GLfloat *v2) GL_BODY(90)   /* 69006eb0 */
GL_STUB void APIENTRY glRecti(GLint x1, GLint y1, GLint x2, GLint y2) GL_BODY(91)   /* 69006ed0 */
GL_STUB void APIENTRY glRectiv(const GLint *v1, const GLint *v2) GL_BODY(92)   /* 69006ef0 */
GL_STUB void APIENTRY glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2) GL_BODY(93)   /* 69006f10 */
GL_STUB void APIENTRY glRectsv(const GLshort *v1, const GLshort *v2) GL_BODY(94)   /* 69006f30 */
GL_STUB void APIENTRY glTexCoord1d(GLdouble s) GL_BODY(95)   /* 69006f50 */
GL_STUB void APIENTRY glTexCoord1dv(const GLdouble *v) GL_BODY(96)   /* 69006f70 */
GL_STUB void APIENTRY glTexCoord1f(GLfloat s) GL_BODY(97)   /* 69006f90 */
GL_STUB void APIENTRY glTexCoord1fv(const GLfloat *v) GL_BODY(98)   /* 69006fb0 */
GL_STUB void APIENTRY glTexCoord1i(GLint s) GL_BODY(99)   /* 69006fd0 */
GL_STUB void APIENTRY glTexCoord1iv(const GLint *v) GL_BODY(100)   /* 69006ff0 */
GL_STUB void APIENTRY glTexCoord1s(GLshort s) GL_BODY(101)   /* 69007010 */
GL_STUB void APIENTRY glTexCoord1sv(const GLshort *v) GL_BODY(102)   /* 69007030 */
GL_STUB void APIENTRY glTexCoord2d(GLdouble s, GLdouble t) GL_BODY(103)   /* 69007050 */
GL_STUB void APIENTRY glTexCoord2dv(const GLdouble *v) GL_BODY(104)   /* 69007070 */
GL_STUB void APIENTRY glTexCoord2f(GLfloat s, GLfloat t) GL_BODY(105)   /* 69007090 */
GL_STUB void APIENTRY glTexCoord2fv(const GLfloat *v) GL_BODY(106)   /* 690070b0 */
GL_STUB void APIENTRY glTexCoord2i(GLint s, GLint t) GL_BODY(107)   /* 690070d0 */
GL_STUB void APIENTRY glTexCoord2iv(const GLint *v) GL_BODY(108)   /* 690070f0 */
GL_STUB void APIENTRY glTexCoord2s(GLshort s, GLshort t) GL_BODY(109)   /* 69007110 */
GL_STUB void APIENTRY glTexCoord2sv(const GLshort *v) GL_BODY(110)   /* 69007130 */
GL_STUB void APIENTRY glTexCoord3d(GLdouble s, GLdouble t, GLdouble r) GL_BODY(111)   /* 69007150 */
GL_STUB void APIENTRY glTexCoord3dv(const GLdouble *v) GL_BODY(112)   /* 69007170 */
GL_STUB void APIENTRY glTexCoord3f(GLfloat s, GLfloat t, GLfloat r) GL_BODY(113)   /* 69007190 */
GL_STUB void APIENTRY glTexCoord3fv(const GLfloat *v) GL_BODY(114)   /* 690071b0 */
GL_STUB void APIENTRY glTexCoord3i(GLint s, GLint t, GLint r) GL_BODY(115)   /* 690071d0 */
GL_STUB void APIENTRY glTexCoord3iv(const GLint *v) GL_BODY(116)   /* 690071f0 */
GL_STUB void APIENTRY glTexCoord3s(GLshort s, GLshort t, GLshort r) GL_BODY(117)   /* 69007210 */
GL_STUB void APIENTRY glTexCoord3sv(const GLshort *v) GL_BODY(118)   /* 69007230 */
GL_STUB void APIENTRY glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q) GL_BODY(119)   /* 69007250 */
GL_STUB void APIENTRY glTexCoord4dv(const GLdouble *v) GL_BODY(120)   /* 69007270 */
GL_STUB void APIENTRY glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q) GL_BODY(121)   /* 69007290 */
GL_STUB void APIENTRY glTexCoord4fv(const GLfloat *v) GL_BODY(122)   /* 690072b0 */
GL_STUB void APIENTRY glTexCoord4i(GLint s, GLint t, GLint r, GLint q) GL_BODY(123)   /* 690072d0 */
GL_STUB void APIENTRY glTexCoord4iv(const GLint *v) GL_BODY(124)   /* 690072f0 */
GL_STUB void APIENTRY glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q) GL_BODY(125)   /* 69007310 */
GL_STUB void APIENTRY glTexCoord4sv(const GLshort *v) GL_BODY(126)   /* 69007330 */
GL_STUB void APIENTRY glVertex2d(GLdouble x, GLdouble y) GL_BODY(127)   /* 69007350 */
GL_STUB void APIENTRY glVertex2dv(const GLdouble *v) GL_BODY(128)   /* 69007370 */
GL_STUB void APIENTRY glVertex2f(GLfloat x, GLfloat y) GL_BODY(129)   /* 69007390 */
GL_STUB void APIENTRY glVertex2fv(const GLfloat *v) GL_BODY(130)   /* 690073b0 */
GL_STUB void APIENTRY glVertex2i(GLint x, GLint y) GL_BODY(131)   /* 690073d0 */
GL_STUB void APIENTRY glVertex2iv(const GLint *v) GL_BODY(132)   /* 690073f0 */
GL_STUB void APIENTRY glVertex2s(GLshort x, GLshort y) GL_BODY(133)   /* 69007410 */
GL_STUB void APIENTRY glVertex2sv(const GLshort *v) GL_BODY(134)   /* 69007430 */
GL_STUB void APIENTRY glVertex3d(GLdouble x, GLdouble y, GLdouble z) GL_BODY(135)   /* 69007450 */
GL_STUB void APIENTRY glVertex3dv(const GLdouble *v) GL_BODY(136)   /* 69007470 */
GL_STUB void APIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z) GL_BODY(137)   /* 69007490 */
GL_STUB void APIENTRY glVertex3fv(const GLfloat *v) GL_BODY(138)   /* 690074b0 */
GL_STUB void APIENTRY glVertex3i(GLint x, GLint y, GLint z) GL_BODY(139)   /* 690074d0 */
GL_STUB void APIENTRY glVertex3iv(const GLint *v) GL_BODY(140)   /* 690074f0 */
GL_STUB void APIENTRY glVertex3s(GLshort x, GLshort y, GLshort z) GL_BODY(141)   /* 69007510 */
GL_STUB void APIENTRY glVertex3sv(const GLshort *v) GL_BODY(142)   /* 69007530 */
GL_STUB void APIENTRY glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) GL_BODY(143)   /* 69007550 */
GL_STUB void APIENTRY glVertex4dv(const GLdouble *v) GL_BODY(144)   /* 69007570 */
GL_STUB void APIENTRY glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) GL_BODY(145)   /* 69007590 */
GL_STUB void APIENTRY glVertex4fv(const GLfloat *v) GL_BODY(146)   /* 690075b0 */
GL_STUB void APIENTRY glVertex4i(GLint x, GLint y, GLint z, GLint w) GL_BODY(147)   /* 690075d0 */
GL_STUB void APIENTRY glVertex4iv(const GLint *v) GL_BODY(148)   /* 690075f0 */
GL_STUB void APIENTRY glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w) GL_BODY(149)   /* 69007610 */
GL_STUB void APIENTRY glVertex4sv(const GLshort *v) GL_BODY(150)   /* 69007630 */
GL_STUB void APIENTRY glClipPlane(GLenum plane, const GLdouble *equation) GL_BODY(151)   /* 69007650 */
GL_STUB void APIENTRY glColorMaterial(GLenum face, GLenum mode) GL_BODY(152)   /* 69007670 */
GL_STUB void APIENTRY glCullFace(GLenum mode) GL_BODY(153)   /* 69007690 */
GL_STUB void APIENTRY glFogf(GLenum pname, GLfloat param) GL_BODY(154)   /* 690076b0 */
GL_STUB void APIENTRY glFogfv(GLenum pname, const GLfloat *params) GL_BODY(155)   /* 690076d0 */
GL_STUB void APIENTRY glFogi(GLenum pname, GLint param) GL_BODY(156)   /* 690076f0 */
GL_STUB void APIENTRY glFogiv(GLenum pname, const GLint *params) GL_BODY(157)   /* 69007710 */
GL_STUB void APIENTRY glFrontFace(GLenum mode) GL_BODY(158)   /* 69007730 */
GL_STUB void APIENTRY glHint(GLenum target, GLenum mode) GL_BODY(159)   /* 69007750 */
GL_STUB void APIENTRY glLightf(GLenum light, GLenum pname, GLfloat param) GL_BODY(160)   /* 69007770 */
GL_STUB void APIENTRY glLightfv(GLenum light, GLenum pname, const GLfloat *params) GL_BODY(161)   /* 69007790 */
GL_STUB void APIENTRY glLighti(GLenum light, GLenum pname, GLint param) GL_BODY(162)   /* 690077b0 */
GL_STUB void APIENTRY glLightiv(GLenum light, GLenum pname, const GLint *params) GL_BODY(163)   /* 690077d0 */
GL_STUB void APIENTRY glLightModelf(GLenum pname, GLfloat param) GL_BODY(164)   /* 690077f0 */
GL_STUB void APIENTRY glLightModelfv(GLenum pname, const GLfloat *params) GL_BODY(165)   /* 69007810 */
GL_STUB void APIENTRY glLightModeli(GLenum pname, GLint param) GL_BODY(166)   /* 69007830 */
GL_STUB void APIENTRY glLightModeliv(GLenum pname, const GLint *params) GL_BODY(167)   /* 69007850 */
GL_STUB void APIENTRY glLineStipple(GLint factor, GLushort pattern) GL_BODY(168)   /* 69007870 */
GL_STUB void APIENTRY glLineWidth(GLfloat width) GL_BODY(169)   /* 69007890 */
GL_STUB void APIENTRY glMaterialf(GLenum face, GLenum pname, GLfloat param) GL_BODY(170)   /* 690078b0 */
GL_STUB void APIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) GL_BODY(171)   /* 690078d0 */
GL_STUB void APIENTRY glMateriali(GLenum face, GLenum pname, GLint param) GL_BODY(172)   /* 690078f0 */
GL_STUB void APIENTRY glMaterialiv(GLenum face, GLenum pname, const GLint *params) GL_BODY(173)   /* 69007910 */
GL_STUB void APIENTRY glPointSize(GLfloat size) GL_BODY(174)   /* 69007930 */
GL_STUB void APIENTRY glPolygonMode(GLenum face, GLenum mode) GL_BODY(175)   /* 69007950 */
GL_STUB void APIENTRY glPolygonStipple(const GLubyte *mask) GL_BODY(176)   /* 69007970 */
GL_STUB void APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height) GL_BODY(177)   /* 69007990 */
GL_STUB void APIENTRY glShadeModel(GLenum mode) GL_BODY(178)   /* 690079b0 */
GL_STUB void APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param) GL_BODY(179)   /* 690079d0 */
GL_STUB void APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) GL_BODY(180)   /* 690079f0 */
GL_STUB void APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param) GL_BODY(181)   /* 69007a10 */
GL_STUB void APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint *params) GL_BODY(182)   /* 69007a30 */
GL_STUB void APIENTRY glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels) GL_BODY(183)   /* 69007a50 */
GL_STUB void APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) GL_BODY(184)   /* 69007a70 */
GL_STUB void APIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param) GL_BODY(185)   /* 69007a90 */
GL_STUB void APIENTRY glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params) GL_BODY(186)   /* 69007ab0 */
GL_STUB void APIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param) GL_BODY(187)   /* 69007ad0 */
GL_STUB void APIENTRY glTexEnviv(GLenum target, GLenum pname, const GLint *params) GL_BODY(188)   /* 69007af0 */
GL_STUB void APIENTRY glTexGend(GLenum coord, GLenum pname, GLdouble param) GL_BODY(189)   /* 69007b10 */
GL_STUB void APIENTRY glTexGendv(GLenum coord, GLenum pname, const GLdouble *params) GL_BODY(190)   /* 69007b30 */
GL_STUB void APIENTRY glTexGenf(GLenum coord, GLenum pname, GLfloat param) GL_BODY(191)   /* 69007b50 */
GL_STUB void APIENTRY glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params) GL_BODY(192)   /* 69007b70 */
GL_STUB void APIENTRY glTexGeni(GLenum coord, GLenum pname, GLint param) GL_BODY(193)   /* 69007b90 */
GL_STUB void APIENTRY glTexGeniv(GLenum coord, GLenum pname, const GLint *params) GL_BODY(194)   /* 69007bb0 */
GL_STUB void APIENTRY glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer) GL_BODY(195)   /* 69007bd0 */
GL_STUB void APIENTRY glSelectBuffer(GLsizei size, GLuint *buffer) GL_BODY(196)   /* 69007bf0 */
GL_STUB GLint APIENTRY glRenderMode(GLenum mode) GL_BODY(197)   /* 69007c10 */
GL_STUB void APIENTRY glInitNames(void) GL_BODY(198)   /* 69007c30 */
GL_STUB void APIENTRY glLoadName(GLuint name) GL_BODY(199)   /* 69007c50 */
GL_STUB void APIENTRY glPassThrough(GLfloat token) GL_BODY(200)   /* 69007c70 */
GL_STUB void APIENTRY glPopName(void) GL_BODY(201)   /* 69007c90 */
GL_STUB void APIENTRY glPushName(GLuint name) GL_BODY(202)   /* 69007cb0 */
GL_STUB void APIENTRY glDrawBuffer(GLenum mode) GL_BODY(203)   /* 69007cd0 */
GL_STUB void APIENTRY glClear(GLbitfield mask) GL_BODY(204)   /* 69007cf0 */
GL_STUB void APIENTRY glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) GL_BODY(205)   /* 69007d10 */
GL_STUB void APIENTRY glClearIndex(GLfloat c) GL_BODY(206)   /* 69007d30 */
GL_STUB void APIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) GL_BODY(207)   /* 69007d50 */
GL_STUB void APIENTRY glClearStencil(GLint s) GL_BODY(208)   /* 69007d70 */
GL_STUB void APIENTRY glClearDepth(GLclampd depth) GL_BODY(209)   /* 69007d90 */
GL_STUB void APIENTRY glStencilMask(GLuint mask) GL_BODY(210)   /* 69007db0 */
GL_STUB void APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) GL_BODY(211)   /* 69007dd0 */
GL_STUB void APIENTRY glDepthMask(GLboolean flag) GL_BODY(212)   /* 69007df0 */
GL_STUB void APIENTRY glIndexMask(GLuint mask) GL_BODY(213)   /* 69007e10 */
GL_STUB void APIENTRY glAccum(GLenum op, GLfloat value) GL_BODY(214)   /* 69007e30 */
GL_STUB void APIENTRY glDisable(GLenum cap) GL_BODY(215)   /* 69007e50 */
GL_STUB void APIENTRY glEnable(GLenum cap) GL_BODY(216)   /* 69007e70 */
GL_STUB void APIENTRY glFinish(void) GL_BODY(217)   /* 69007e90 */
GL_STUB void APIENTRY glFlush(void) GL_BODY(218)   /* 69007eb0 */
GL_STUB void APIENTRY glPopAttrib(void) GL_BODY(219)   /* 69007ed0 */
GL_STUB void APIENTRY glPushAttrib(GLbitfield mask) GL_BODY(220)   /* 69007ef0 */
GL_STUB void APIENTRY glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points) GL_BODY(221)   /* 69007f10 */
GL_STUB void APIENTRY glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points) GL_BODY(222)   /* 69007f30 */
GL_STUB void APIENTRY glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points) GL_BODY(223)   /* 69007f50 */
GL_STUB void APIENTRY glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points) GL_BODY(224)   /* 69007f70 */
GL_STUB void APIENTRY glMapGrid1d(GLint un, GLdouble u1, GLdouble u2) GL_BODY(225)   /* 69007f90 */
GL_STUB void APIENTRY glMapGrid1f(GLint un, GLfloat u1, GLfloat u2) GL_BODY(226)   /* 69007fb0 */
GL_STUB void APIENTRY glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) GL_BODY(227)   /* 69007fd0 */
GL_STUB void APIENTRY glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) GL_BODY(228)   /* 69007ff0 */
GL_STUB void APIENTRY glEvalCoord1d(GLdouble u) GL_BODY(229)   /* 69008010 */
GL_STUB void APIENTRY glEvalCoord1dv(const GLdouble *u) GL_BODY(230)   /* 69008030 */
GL_STUB void APIENTRY glEvalCoord1f(GLfloat u) GL_BODY(231)   /* 69008050 */
GL_STUB void APIENTRY glEvalCoord1fv(const GLfloat *u) GL_BODY(232)   /* 69008070 */
GL_STUB void APIENTRY glEvalCoord2d(GLdouble u, GLdouble v) GL_BODY(233)   /* 69008090 */
GL_STUB void APIENTRY glEvalCoord2dv(const GLdouble *u) GL_BODY(234)   /* 690080b0 */
GL_STUB void APIENTRY glEvalCoord2f(GLfloat u, GLfloat v) GL_BODY(235)   /* 690080d0 */
GL_STUB void APIENTRY glEvalCoord2fv(const GLfloat *u) GL_BODY(236)   /* 690080f0 */
GL_STUB void APIENTRY glEvalMesh1(GLenum mode, GLint i1, GLint i2) GL_BODY(237)   /* 69008110 */
GL_STUB void APIENTRY glEvalPoint1(GLint i) GL_BODY(238)   /* 69008130 */
GL_STUB void APIENTRY glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) GL_BODY(239)   /* 69008150 */
GL_STUB void APIENTRY glEvalPoint2(GLint i, GLint j) GL_BODY(240)   /* 69008170 */
GL_STUB void APIENTRY glAlphaFunc(GLenum func, GLclampf ref) GL_BODY(241)   /* 69008190 */
GL_STUB void APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor) GL_BODY(242)   /* 690081b0 */
GL_STUB void APIENTRY glLogicOp(GLenum opcode) GL_BODY(243)   /* 690081d0 */
GL_STUB void APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask) GL_BODY(244)   /* 690081f0 */
GL_STUB void APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) GL_BODY(245)   /* 69008210 */
GL_STUB void APIENTRY glDepthFunc(GLenum func) GL_BODY(246)   /* 69008230 */
GL_STUB void APIENTRY glPixelZoom(GLfloat xfactor, GLfloat yfactor) GL_BODY(247)   /* 69008250 */
GL_STUB void APIENTRY glPixelTransferf(GLenum pname, GLfloat param) GL_BODY(248)   /* 69008270 */
GL_STUB void APIENTRY glPixelTransferi(GLenum pname, GLint param) GL_BODY(249)   /* 69008290 */
GL_STUB void APIENTRY glPixelStoref(GLenum pname, GLfloat param) GL_BODY(250)   /* 690082b0 */
GL_STUB void APIENTRY glPixelStorei(GLenum pname, GLint param) GL_BODY(251)   /* 690082d0 */
GL_STUB void APIENTRY glPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat *values) GL_BODY(252)   /* 690082f0 */
GL_STUB void APIENTRY glPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint *values) GL_BODY(253)   /* 69008310 */
GL_STUB void APIENTRY glPixelMapusv(GLenum map, GLsizei mapsize, const GLushort *values) GL_BODY(254)   /* 69008330 */
GL_STUB void APIENTRY glReadBuffer(GLenum mode) GL_BODY(255)   /* 69008350 */
GL_STUB void APIENTRY glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type) GL_BODY(256)   /* 69008370 */
GL_STUB void APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) GL_BODY(257)   /* 69008390 */
GL_STUB void APIENTRY glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) GL_BODY(258)   /* 690083b0 */
GL_STUB void APIENTRY glGetBooleanv(GLenum pname, GLboolean *params) GL_BODY(259)   /* 690083d0 */
GL_STUB void APIENTRY glGetClipPlane(GLenum plane, GLdouble *equation) GL_BODY(260)   /* 690083f0 */
GL_STUB void APIENTRY glGetDoublev(GLenum pname, GLdouble *params) GL_BODY(261)   /* 69008410 */
GL_STUB GLenum APIENTRY glGetError(void) GL_BODY(262)   /* 69008430 */
GL_STUB void APIENTRY glGetFloatv(GLenum pname, GLfloat *params) GL_BODY(263)   /* 69008450 */
GL_STUB void APIENTRY glGetIntegerv(GLenum pname, GLint *params) GL_BODY(264)   /* 69008470 */
GL_STUB void APIENTRY glGetLightfv(GLenum light, GLenum pname, GLfloat *params) GL_BODY(265)   /* 69008490 */
GL_STUB void APIENTRY glGetLightiv(GLenum light, GLenum pname, GLint *params) GL_BODY(266)   /* 690084b0 */
GL_STUB void APIENTRY glGetMapdv(GLenum target, GLenum query, GLdouble *v) GL_BODY(267)   /* 690084d0 */
GL_STUB void APIENTRY glGetMapfv(GLenum target, GLenum query, GLfloat *v) GL_BODY(268)   /* 690084f0 */
GL_STUB void APIENTRY glGetMapiv(GLenum target, GLenum query, GLint *v) GL_BODY(269)   /* 69008510 */
GL_STUB void APIENTRY glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params) GL_BODY(270)   /* 69008530 */
GL_STUB void APIENTRY glGetMaterialiv(GLenum face, GLenum pname, GLint *params) GL_BODY(271)   /* 69008550 */
GL_STUB void APIENTRY glGetPixelMapfv(GLenum map, GLfloat *values) GL_BODY(272)   /* 69008570 */
GL_STUB void APIENTRY glGetPixelMapuiv(GLenum map, GLuint *values) GL_BODY(273)   /* 69008590 */
GL_STUB void APIENTRY glGetPixelMapusv(GLenum map, GLushort *values) GL_BODY(274)   /* 690085b0 */
GL_STUB void APIENTRY glGetPolygonStipple(GLubyte *mask) GL_BODY(275)   /* 690085d0 */
GL_STUB const GLubyte * APIENTRY glGetString(GLenum name) GL_BODY(276)   /* 690085f0 */
GL_STUB void APIENTRY glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params) GL_BODY(277)   /* 69008610 */
GL_STUB void APIENTRY glGetTexEnviv(GLenum target, GLenum pname, GLint *params) GL_BODY(278)   /* 69008630 */
GL_STUB void APIENTRY glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params) GL_BODY(279)   /* 69008650 */
GL_STUB void APIENTRY glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params) GL_BODY(280)   /* 69008670 */
GL_STUB void APIENTRY glGetTexGeniv(GLenum coord, GLenum pname, GLint *params) GL_BODY(281)   /* 69008690 */
GL_STUB void APIENTRY glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels) GL_BODY(282)   /* 690086b0 */
GL_STUB void APIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) GL_BODY(283)   /* 690086d0 */
GL_STUB void APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint *params) GL_BODY(284)   /* 690086f0 */
GL_STUB void APIENTRY glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params) GL_BODY(285)   /* 69008710 */
GL_STUB void APIENTRY glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params) GL_BODY(286)   /* 69008730 */
GL_STUB GLboolean APIENTRY glIsEnabled(GLenum cap) GL_BODY(287)   /* 69008750 */
GL_STUB GLboolean APIENTRY glIsList(GLuint list) GL_BODY(288)   /* 69008770 */
GL_STUB void APIENTRY glDepthRange(GLclampd zNear, GLclampd zFar) GL_BODY(289)   /* 69008790 */
GL_STUB void APIENTRY glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) GL_BODY(290)   /* 690087b0 */
GL_STUB void APIENTRY glLoadIdentity(void) GL_BODY(291)   /* 690087d0 */
GL_STUB void APIENTRY glLoadMatrixf(const GLfloat *m) GL_BODY(292)   /* 690087f0 */
GL_STUB void APIENTRY glLoadMatrixd(const GLdouble *m) GL_BODY(293)   /* 69008810 */
GL_STUB void APIENTRY glMatrixMode(GLenum mode) GL_BODY(294)   /* 69008830 */
GL_STUB void APIENTRY glMultMatrixf(const GLfloat *m) GL_BODY(295)   /* 69008850 */
GL_STUB void APIENTRY glMultMatrixd(const GLdouble *m) GL_BODY(296)   /* 69008870 */
GL_STUB void APIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) GL_BODY(297)   /* 69008890 */
GL_STUB void APIENTRY glPopMatrix(void) GL_BODY(298)   /* 690088b0 */
GL_STUB void APIENTRY glPushMatrix(void) GL_BODY(299)   /* 690088d0 */
GL_STUB void APIENTRY glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z) GL_BODY(300)   /* 690088f0 */
GL_STUB void APIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) GL_BODY(301)   /* 69008910 */
GL_STUB void APIENTRY glScaled(GLdouble x, GLdouble y, GLdouble z) GL_BODY(302)   /* 69008930 */
GL_STUB void APIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z) GL_BODY(303)   /* 69008950 */
GL_STUB void APIENTRY glTranslated(GLdouble x, GLdouble y, GLdouble z) GL_BODY(304)   /* 69008970 */
GL_STUB void APIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z) GL_BODY(305)   /* 69008990 */
GL_STUB void APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height) GL_BODY(306)   /* 690089b0 */
GL_STUB void APIENTRY glArrayElementEXT(void) GL_BODY(337)   /* 690089d0 */
GL_STUB void APIENTRY glColorPointerEXT(void) GL_BODY(338)   /* 690089f0 */
GL_STUB void APIENTRY glDrawArraysEXT(void) GL_BODY(339)   /* 69008a10 */
GL_STUB void APIENTRY glEdgeFlagPointerEXT(void) GL_BODY(340)   /* 69008a30 */
GL_STUB void APIENTRY glGetPointervEXT(void) GL_BODY(341)   /* 69008a50 */
GL_STUB void APIENTRY glIndexPointerEXT(void) GL_BODY(342)   /* 69008a70 */
GL_STUB void APIENTRY glNormalPointerEXT(void) GL_BODY(343)   /* 69008a90 */
GL_STUB void APIENTRY glTexCoordPointerEXT(void) GL_BODY(344)   /* 69008ab0 */
GL_STUB void APIENTRY glVertexPointerEXT(void) GL_BODY(345)   /* 69008ad0 */
GL_STUB void APIENTRY glColorSubTableEXT(void) GL_BODY(346)   /* 69008af0 */
GL_STUB void APIENTRY glColorTableEXT(void) GL_BODY(347)   /* 69008b10 */
GL_STUB void APIENTRY glCopyColorTableEXT(void) GL_BODY(348)   /* 69008b30 */
GL_STUB void APIENTRY glGetColorTableEXT(void) GL_BODY(349)   /* 69008b50 */
GL_STUB void APIENTRY glGetColorTableParameterfvEXT(void) GL_BODY(350)   /* 69008b70 */
GL_STUB void APIENTRY glGetColorTableParameterivEXT(void) GL_BODY(351)   /* 69008b90 */
GL_STUB void APIENTRY glArrayElement(GLint i) GL_BODY(307)   /* 69008bb0 */
GL_STUB void APIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) GL_BODY(309)   /* 69008bd0 */
GL_STUB void APIENTRY glDisableClientState(GLenum array) GL_BODY(310)   /* 69008bf0 */
GL_STUB void APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count) GL_BODY(311)   /* 69008c10 */
GL_STUB void APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) GL_BODY(312)   /* 69008c30 */
GL_STUB void APIENTRY glEdgeFlagPointer(GLsizei stride, const GLvoid *pointer) GL_BODY(313)   /* 69008c50 */
GL_STUB void APIENTRY glEnableClientState(GLenum array) GL_BODY(314)   /* 69008c70 */
GL_STUB void APIENTRY glGetPointerv(GLenum pname, GLvoid* *params) GL_BODY(330)   /* 69008c90 */
GL_STUB void APIENTRY glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer) GL_BODY(315)   /* 69008cb0 */
GL_STUB void APIENTRY glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer) GL_BODY(318)   /* 69008cd0 */
GL_STUB void APIENTRY glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer) GL_BODY(319)   /* 69008cf0 */
GL_STUB void APIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) GL_BODY(321)   /* 69008d10 */
GL_STUB void APIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) GL_BODY(322)   /* 69008d30 */
GL_STUB void APIENTRY glPolygonOffset(GLfloat factor, GLfloat units) GL_BODY(320)   /* 69008d50 */
GL_STUB void APIENTRY glCopyTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border) GL_BODY(324)   /* 69008d70 */
GL_STUB void APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) GL_BODY(325)   /* 69008d90 */
GL_STUB void APIENTRY glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) GL_BODY(326)   /* 69008db0 */
GL_STUB void APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) GL_BODY(327)   /* 69008dd0 */
GL_STUB void APIENTRY glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels) GL_BODY(333)   /* 69008df0 */
GL_STUB void APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) GL_BODY(334)   /* 69008e10 */
GL_STUB GLboolean APIENTRY glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences) GL_BODY(323)   /* 69008e30 */
GL_STUB void APIENTRY glBindTexture(GLenum target, GLuint texture) GL_BODY(308)   /* 69008e50 */
GL_STUB void APIENTRY glDeleteTextures(GLsizei n, const GLuint *textures) GL_BODY(328)   /* 69008e70 */
GL_STUB void APIENTRY glGenTextures(GLsizei n, GLuint *textures) GL_BODY(329)   /* 69008e90 */
GL_STUB GLboolean APIENTRY glIsTexture(GLuint texture) GL_BODY(331)   /* 69008eb0 */
GL_STUB void APIENTRY glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities) GL_BODY(332)   /* 69008ed0 */
GL_STUB void APIENTRY glIndexub(GLubyte c) GL_BODY(316)   /* 69008ef0 */
GL_STUB void APIENTRY glIndexubv(const GLubyte *c) GL_BODY(317)   /* 69008f10 */
GL_STUB void APIENTRY glPopClientAttrib(void) GL_BODY(335)   /* 69008f30 */
GL_STUB void APIENTRY glPushClientAttrib(GLbitfield mask) GL_BODY(336)   /* 69008f50 */
GL_STUB void APIENTRY glLockArraysEXT(void) GL_BODY(352)   /* 69008f70 */
GL_STUB void APIENTRY glUnlockArraysEXT(void) GL_BODY(353)   /* 69008f90 */
GL_STUB void APIENTRY glCullParameterdvSGI(void) GL_BODY(354)   /* 69008fb0 */
GL_STUB void APIENTRY glCullParameterfvSGI(void) GL_BODY(355)   /* 69008fd0 */
GL_STUB void APIENTRY glIndexFuncSGI(void) GL_BODY(356)   /* 69008ff0 */
GL_STUB void APIENTRY glIndexMaterialSGI(void) GL_BODY(357)   /* 69009010 */
GL_STUB void APIENTRY glAddSwapHintRectWIN(void) GL_BODY(358)   /* 69009030 */
GL_STUB void APIENTRY glActiveTextureARB(void) GL_BODY(359)   /* 69009050 */
GL_STUB void APIENTRY glClientActiveTextureARB(void) GL_BODY(360)   /* 69009070 */
GL_STUB void APIENTRY glMultiTexCoord1dARB(void) GL_BODY(361)   /* 69009090 */
GL_STUB void APIENTRY glMultiTexCoord1dvARB(void) GL_BODY(362)   /* 690090b0 */
GL_STUB void APIENTRY glMultiTexCoord1fARB(void) GL_BODY(363)   /* 690090d0 */
GL_STUB void APIENTRY glMultiTexCoord1fvARB(void) GL_BODY(364)   /* 690090f0 */
GL_STUB void APIENTRY glMultiTexCoord1iARB(void) GL_BODY(365)   /* 69009110 */
GL_STUB void APIENTRY glMultiTexCoord1ivARB(void) GL_BODY(366)   /* 69009130 */
GL_STUB void APIENTRY glMultiTexCoord1sARB(void) GL_BODY(367)   /* 69009150 */
GL_STUB void APIENTRY glMultiTexCoord1svARB(void) GL_BODY(368)   /* 69009170 */
GL_STUB void APIENTRY glMultiTexCoord2dARB(void) GL_BODY(369)   /* 69009190 */
GL_STUB void APIENTRY glMultiTexCoord2dvARB(void) GL_BODY(370)   /* 690091b0 */
GL_STUB void APIENTRY glMultiTexCoord2fARB(void) GL_BODY(371)   /* 690091d0 */
GL_STUB void APIENTRY glMultiTexCoord2fvARB(void) GL_BODY(372)   /* 690091f0 */
GL_STUB void APIENTRY glMultiTexCoord2iARB(void) GL_BODY(373)   /* 69009210 */
GL_STUB void APIENTRY glMultiTexCoord2ivARB(void) GL_BODY(374)   /* 69009230 */
GL_STUB void APIENTRY glMultiTexCoord2sARB(void) GL_BODY(375)   /* 69009250 */
GL_STUB void APIENTRY glMultiTexCoord2svARB(void) GL_BODY(376)   /* 69009270 */
GL_STUB void APIENTRY glMultiTexCoord3dARB(void) GL_BODY(377)   /* 69009290 */
GL_STUB void APIENTRY glMultiTexCoord3dvARB(void) GL_BODY(378)   /* 690092b0 */
GL_STUB void APIENTRY glMultiTexCoord3fARB(void) GL_BODY(379)   /* 690092d0 */
GL_STUB void APIENTRY glMultiTexCoord3fvARB(void) GL_BODY(380)   /* 690092f0 */
GL_STUB void APIENTRY glMultiTexCoord3iARB(void) GL_BODY(381)   /* 69009310 */
GL_STUB void APIENTRY glMultiTexCoord3ivARB(void) GL_BODY(382)   /* 69009330 */
GL_STUB void APIENTRY glMultiTexCoord3sARB(void) GL_BODY(383)   /* 69009350 */
GL_STUB void APIENTRY glMultiTexCoord3svARB(void) GL_BODY(384)   /* 69009370 */
GL_STUB void APIENTRY glMultiTexCoord4dARB(void) GL_BODY(385)   /* 69009390 */
GL_STUB void APIENTRY glMultiTexCoord4dvARB(void) GL_BODY(386)   /* 690093b0 */
GL_STUB void APIENTRY glMultiTexCoord4fARB(void) GL_BODY(387)   /* 690093d0 */
GL_STUB void APIENTRY glMultiTexCoord4fvARB(void) GL_BODY(388)   /* 690093f0 */
GL_STUB void APIENTRY glMultiTexCoord4iARB(void) GL_BODY(389)   /* 69009410 */
GL_STUB void APIENTRY glMultiTexCoord4ivARB(void) GL_BODY(390)   /* 69009430 */
GL_STUB void APIENTRY glMultiTexCoord4sARB(void) GL_BODY(391)   /* 69009450 */
GL_STUB void APIENTRY glMultiTexCoord4svARB(void) GL_BODY(392)   /* 69009470 */
GL_STUB void APIENTRY glPointParameterfEXT(void) GL_BODY(393)   /* 69009490 */
GL_STUB void APIENTRY glPointParameterfvEXT(void) GL_BODY(394)   /* 690094b0 */
GL_STUB void APIENTRY glBlendEquationEXT(void) GL_BODY(395)   /* 690094d0 */
GL_STUB void APIENTRY glCompressedTexImage1DARB(void) GL_BODY(396)   /* 690094f0 */
GL_STUB void APIENTRY glCompressedTexImage2DARB(void) GL_BODY(397)   /* 69009510 */
GL_STUB void APIENTRY glCompressedTexSubImage1DARB(void) GL_BODY(398)   /* 69009530 */
GL_STUB void APIENTRY glCompressedTexSubImage2DARB(void) GL_BODY(399)   /* 69009550 */
GL_STUB void APIENTRY glGetCompressedTexImageARB(void) GL_BODY(400)   /* 69009570 */
GL_STUB void APIENTRY glTBufferMask3DFX(GLuint mask) GL_BODY(401)   /* 69009590 */
GL_STUB void APIENTRY glFogCoordfEXT(void) GL_BODY(402)   /* 690095b0 */
GL_STUB void APIENTRY glFogCoorddEXT(void) GL_BODY(403)   /* 690095d0 */
GL_STUB void APIENTRY glFogCoordfvEXT(void) GL_BODY(404)   /* 690095f0 */
GL_STUB void APIENTRY glFogCoorddvEXT(void) GL_BODY(405)   /* 69009610 */
GL_STUB void APIENTRY glFogCoordPointerEXT(void) GL_BODY(406)   /* 69009630 */
GL_STUB void APIENTRY gl_stub_690b3c2a(void) GL_BODY(2)   /* 690b3c2a */

/* 690ee000 -- wglGetProcAddress table: {name, entry point}, 0x72 entries */
const char *WINAPI wglGetExtensionsStringEXT(void);
const char *WINAPI wglGetExtensionsStringARB(HDC);
GLboolean WINAPI wglSwapIntervalEXT(int interval);
int  WINAPI wglGetSwapIntervalEXT(void);
BOOL WINAPI wglSetDeviceGammaRamp3DFX(HDC, LPVOID);
BOOL WINAPI wglGetDeviceGammaRamp3DFX(HDC, LPVOID);
BOOL WINAPI wglQueryMultisample3DFX(int);
BOOL WINAPI wglSetMultisample3DFX(int);
BOOL WINAPI wglGetPixelFormatAttribivARB(HDC, int, int, UINT, const int *, int *);
BOOL WINAPI wglGetPixelFormatAttribfvARB(HDC, int, int, UINT, const int *, FLOAT *);
BOOL WINAPI wglChoosePixelFormatARB(HDC, const int *, const FLOAT *, UINT, int *, UINT *);
struct { const char *name; PROC fn; } g_extProcTable[0x72] = {   /* writable: 69028bd7 clears the T-buffer entries */
    { "glArrayElementEXT", (PROC)glArrayElementEXT },
    { "glDrawArraysEXT", (PROC)glDrawArraysEXT },
    { "glVertexPointerEXT", (PROC)glVertexPointerEXT },
    { "glNormalPointerEXT", (PROC)glNormalPointerEXT },
    { "glColorPointerEXT", (PROC)glColorPointerEXT },
    { "glIndexPointerEXT", (PROC)glIndexPointerEXT },
    { "glTexCoordPointerEXT", (PROC)glTexCoordPointerEXT },
    { "glEdgeFlagPointerEXT", (PROC)glEdgeFlagPointerEXT },
    { "glGetPointervEXT", (PROC)glGetPointervEXT },
    { "glColorSubTableEXT", (PROC)glColorSubTableEXT },
    { "glColorTableEXT", (PROC)glColorTableEXT },
    { "glCopyColorTableEXT", (PROC)glCopyColorTableEXT },
    { "glGetColorTableEXT", (PROC)glGetColorTableEXT },
    { "glGetColorTableParameterfvEXT", (PROC)glGetColorTableParameterfvEXT },
    { "glGetColorTableParameterivEXT", (PROC)glGetColorTableParameterivEXT },
    { "glLockArraysEXT", (PROC)glLockArraysEXT },
    { "glUnlockArraysEXT", (PROC)glUnlockArraysEXT },
    { "glCullParameterdvSGI", (PROC)glCullParameterdvSGI },
    { "glCullParameterfvSGI", (PROC)glCullParameterfvSGI },
    { "glIndexFuncSGI", (PROC)glIndexFuncSGI },
    { "glIndexMaterialSGI", (PROC)glIndexMaterialSGI },
    { "glAddSwapHintRectWIN", (PROC)glAddSwapHintRectWIN },
    { "wglGetExtensionsStringEXT", (PROC)wglGetExtensionsStringEXT },
    { "wglGetExtensionsStringARB", (PROC)wglGetExtensionsStringARB },
    { "wglSwapIntervalEXT", (PROC)wglSwapIntervalEXT },
    { "wglGetSwapIntervalEXT", (PROC)wglGetSwapIntervalEXT },
    { "wglSetDeviceGammaRamp3DFX", (PROC)wglSetDeviceGammaRamp3DFX },
    { "wglGetDeviceGammaRamp3DFX", (PROC)wglGetDeviceGammaRamp3DFX },
    { "wglQueryMultisample3DFX", (PROC)wglQueryMultisample3DFX },
    { "wglSetMultisample3DFX", (PROC)wglSetMultisample3DFX },
    { "glTBufferMask3DFX", (PROC)glTBufferMask3DFX },
    { "glActiveTextureARB", (PROC)glActiveTextureARB },
    { "glClientActiveTextureARB", (PROC)glClientActiveTextureARB },
    { "glMultiTexCoord1dARB", (PROC)glMultiTexCoord1dARB },
    { "glMultiTexCoord1dvARB", (PROC)glMultiTexCoord1dvARB },
    { "glMultiTexCoord1fARB", (PROC)glMultiTexCoord1fARB },
    { "glMultiTexCoord1fvARB", (PROC)glMultiTexCoord1fvARB },
    { "glMultiTexCoord1iARB", (PROC)glMultiTexCoord1iARB },
    { "glMultiTexCoord1ivARB", (PROC)glMultiTexCoord1ivARB },
    { "glMultiTexCoord1sARB", (PROC)glMultiTexCoord1sARB },
    { "glMultiTexCoord1svARB", (PROC)glMultiTexCoord1svARB },
    { "glMultiTexCoord2dARB", (PROC)glMultiTexCoord2dARB },
    { "glMultiTexCoord2dvARB", (PROC)glMultiTexCoord2dvARB },
    { "glMultiTexCoord2fARB", (PROC)glMultiTexCoord2fARB },
    { "glMultiTexCoord2fvARB", (PROC)glMultiTexCoord2fvARB },
    { "glMultiTexCoord2iARB", (PROC)glMultiTexCoord2iARB },
    { "glMultiTexCoord2ivARB", (PROC)glMultiTexCoord2ivARB },
    { "glMultiTexCoord2sARB", (PROC)glMultiTexCoord2sARB },
    { "glMultiTexCoord2svARB", (PROC)glMultiTexCoord2svARB },
    { "glMultiTexCoord3dARB", (PROC)glMultiTexCoord3dARB },
    { "glMultiTexCoord3dvARB", (PROC)glMultiTexCoord3dvARB },
    { "glMultiTexCoord3fARB", (PROC)glMultiTexCoord3fARB },
    { "glMultiTexCoord3fvARB", (PROC)glMultiTexCoord3fvARB },
    { "glMultiTexCoord3iARB", (PROC)glMultiTexCoord3iARB },
    { "glMultiTexCoord3ivARB", (PROC)glMultiTexCoord3ivARB },
    { "glMultiTexCoord3sARB", (PROC)glMultiTexCoord3sARB },
    { "glMultiTexCoord3svARB", (PROC)glMultiTexCoord3svARB },
    { "glMultiTexCoord4dARB", (PROC)glMultiTexCoord4dARB },
    { "glMultiTexCoord4dvARB", (PROC)glMultiTexCoord4dvARB },
    { "glMultiTexCoord4fARB", (PROC)glMultiTexCoord4fARB },
    { "glMultiTexCoord4fvARB", (PROC)glMultiTexCoord4fvARB },
    { "glMultiTexCoord4iARB", (PROC)glMultiTexCoord4iARB },
    { "glMultiTexCoord4ivARB", (PROC)glMultiTexCoord4ivARB },
    { "glMultiTexCoord4sARB", (PROC)glMultiTexCoord4sARB },
    { "glMultiTexCoord4svARB", (PROC)glMultiTexCoord4svARB },
    { "glSelectTextureSGIS", (PROC)glActiveTextureARB },
    { "glMTexCoord1dSGIS", (PROC)glMultiTexCoord1dARB },
    { "glMTexCoord1dvSGIS", (PROC)glMultiTexCoord1dvARB },
    { "glMTexCoord1fSGIS", (PROC)glMultiTexCoord1fARB },
    { "glMTexCoord1fvSGIS", (PROC)glMultiTexCoord1fvARB },
    { "glMTexCoord1iSGIS", (PROC)glMultiTexCoord1iARB },
    { "glMTexCoord1ivSGIS", (PROC)glMultiTexCoord1ivARB },
    { "glMTexCoord1sSGIS", (PROC)glMultiTexCoord1sARB },
    { "glMTexCoord1svSGIS", (PROC)glMultiTexCoord1svARB },
    { "glMTexCoord2dSGIS", (PROC)glMultiTexCoord2dARB },
    { "glMTexCoord2dvSGIS", (PROC)glMultiTexCoord2dvARB },
    { "glMTexCoord2fSGIS", (PROC)glMultiTexCoord2fARB },
    { "glMTexCoord2fvSGIS", (PROC)glMultiTexCoord2fvARB },
    { "glMTexCoord2iSGIS", (PROC)glMultiTexCoord2iARB },
    { "glMTexCoord2ivSGIS", (PROC)glMultiTexCoord2ivARB },
    { "glMTexCoord2sSGIS", (PROC)glMultiTexCoord2sARB },
    { "glMTexCoord2svSGIS", (PROC)glMultiTexCoord2svARB },
    { "glMTexCoord3dSGIS", (PROC)glMultiTexCoord3dARB },
    { "glMTexCoord3dvSGIS", (PROC)glMultiTexCoord3dvARB },
    { "glMTexCoord3fSGIS", (PROC)glMultiTexCoord3fARB },
    { "glMTexCoord3fvSGIS", (PROC)glMultiTexCoord3fvARB },
    { "glMTexCoord3iSGIS", (PROC)glMultiTexCoord3iARB },
    { "glMTexCoord3ivSGIS", (PROC)glMultiTexCoord3ivARB },
    { "glMTexCoord3sSGIS", (PROC)glMultiTexCoord3sARB },
    { "glMTexCoord3svSGIS", (PROC)glMultiTexCoord3svARB },
    { "glMTexCoord4dSGIS", (PROC)glMultiTexCoord4dARB },
    { "glMTexCoord4dvSGIS", (PROC)glMultiTexCoord4dvARB },
    { "glMTexCoord4fSGIS", (PROC)glMultiTexCoord4fARB },
    { "glMTexCoord4fvSGIS", (PROC)glMultiTexCoord4fvARB },
    { "glMTexCoord4iSGIS", (PROC)glMultiTexCoord4iARB },
    { "glMTexCoord4ivSGIS", (PROC)glMultiTexCoord4ivARB },
    { "glMTexCoord4sSGIS", (PROC)glMultiTexCoord4sARB },
    { "glMTexCoord4svSGIS", (PROC)glMultiTexCoord4svARB },
    { "glPointParameterfEXT", (PROC)glPointParameterfEXT },
    { "glPointParameterfvEXT", (PROC)glPointParameterfvEXT },
    { "glBlendEquationEXT", (PROC)glBlendEquationEXT },
    { "wglGetPixelFormatAttribivARB", (PROC)wglGetPixelFormatAttribivARB },
    { "wglGetPixelFormatAttribfvARB", (PROC)wglGetPixelFormatAttribfvARB },
    { "wglChoosePixelFormatARB", (PROC)wglChoosePixelFormatARB },
    { "glCompressedTexImage1DARB", (PROC)glCompressedTexImage1DARB },
    { "glCompressedTexImage2DARB", (PROC)glCompressedTexImage2DARB },
    { "glCompressedTexSubImage1DARB", (PROC)glCompressedTexSubImage1DARB },
    { "glCompressedTexSubImage2DARB", (PROC)glCompressedTexSubImage2DARB },
    { "glGetCompressedTexImageARB", (PROC)glGetCompressedTexImageARB },
    { "glFogCoordfEXT", (PROC)glFogCoordfEXT },
    { "glFogCoorddEXT", (PROC)glFogCoorddEXT },
    { "glFogCoordfvEXT", (PROC)glFogCoordfvEXT },
    { "glFogCoorddvEXT", (PROC)glFogCoorddvEXT },
    { "glFogCoordPointerEXT", (PROC)glFogCoordPointerEXT },
};
/* 69001334 -- the table's entry count, a constant in .text that sst_grstate.c's T-buffer loop reads from memory
** (wglstand.c's lookup uses the literal 0x72) */
const GLint g_extProcCount = sizeof(g_extProcTable) / sizeof(g_extProcTable[0]);

/* 690ee3c4 -- the 0x150 gl* names re-bound by name in stand-alone mode */
const char *g_glEntryNames[0x150] = {
    "glNewList", "glEndList", "glCallList", "glCallLists",
    "glDeleteLists", "glGenLists", "glListBase", "glBegin",
    "glBitmap", "glColor3b", "glColor3bv", "glColor3d",
    "glColor3dv", "glColor3f", "glColor3fv", "glColor3i",
    "glColor3iv", "glColor3s", "glColor3sv", "glColor3ub",
    "glColor3ubv", "glColor3ui", "glColor3uiv", "glColor3us",
    "glColor3usv", "glColor4b", "glColor4bv", "glColor4d",
    "glColor4dv", "glColor4f", "glColor4fv", "glColor4i",
    "glColor4iv", "glColor4s", "glColor4sv", "glColor4ub",
    "glColor4ubv", "glColor4ui", "glColor4uiv", "glColor4us",
    "glColor4usv", "glEdgeFlag", "glEdgeFlagv", "glEnd",
    "glIndexd", "glIndexdv", "glIndexf", "glIndexfv",
    "glIndexi", "glIndexiv", "glIndexs", "glIndexsv",
    "glNormal3b", "glNormal3bv", "glNormal3d", "glNormal3dv",
    "glNormal3f", "glNormal3fv", "glNormal3i", "glNormal3iv",
    "glNormal3s", "glNormal3sv", "glRasterPos2d", "glRasterPos2dv",
    "glRasterPos2f", "glRasterPos2fv", "glRasterPos2i", "glRasterPos2iv",
    "glRasterPos2s", "glRasterPos2sv", "glRasterPos3d", "glRasterPos3dv",
    "glRasterPos3f", "glRasterPos3fv", "glRasterPos3i", "glRasterPos3iv",
    "glRasterPos3s", "glRasterPos3sv", "glRasterPos4d", "glRasterPos4dv",
    "glRasterPos4f", "glRasterPos4fv", "glRasterPos4i", "glRasterPos4iv",
    "glRasterPos4s", "glRasterPos4sv", "glRectd", "glRectdv",
    "glRectf", "glRectfv", "glRecti", "glRectiv",
    "glRects", "glRectsv", "glTexCoord1d", "glTexCoord1dv",
    "glTexCoord1f", "glTexCoord1fv", "glTexCoord1i", "glTexCoord1iv",
    "glTexCoord1s", "glTexCoord1sv", "glTexCoord2d", "glTexCoord2dv",
    "glTexCoord2f", "glTexCoord2fv", "glTexCoord2i", "glTexCoord2iv",
    "glTexCoord2s", "glTexCoord2sv", "glTexCoord3d", "glTexCoord3dv",
    "glTexCoord3f", "glTexCoord3fv", "glTexCoord3i", "glTexCoord3iv",
    "glTexCoord3s", "glTexCoord3sv", "glTexCoord4d", "glTexCoord4dv",
    "glTexCoord4f", "glTexCoord4fv", "glTexCoord4i", "glTexCoord4iv",
    "glTexCoord4s", "glTexCoord4sv", "glVertex2d", "glVertex2dv",
    "glVertex2f", "glVertex2fv", "glVertex2i", "glVertex2iv",
    "glVertex2s", "glVertex2sv", "glVertex3d", "glVertex3dv",
    "glVertex3f", "glVertex3fv", "glVertex3i", "glVertex3iv",
    "glVertex3s", "glVertex3sv", "glVertex4d", "glVertex4dv",
    "glVertex4f", "glVertex4fv", "glVertex4i", "glVertex4iv",
    "glVertex4s", "glVertex4sv", "glClipPlane", "glColorMaterial",
    "glCullFace", "glFogf", "glFogfv", "glFogi",
    "glFogiv", "glFrontFace", "glHint", "glLightf",
    "glLightfv", "glLighti", "glLightiv", "glLightModelf",
    "glLightModelfv", "glLightModeli", "glLightModeliv", "glLineStipple",
    "glLineWidth", "glMaterialf", "glMaterialfv", "glMateriali",
    "glMaterialiv", "glPointSize", "glPolygonMode", "glPolygonStipple",
    "glScissor", "glShadeModel", "glTexParameterf", "glTexParameterfv",
    "glTexParameteri", "glTexParameteriv", "glTexImage1D", "glTexImage2D",
    "glTexEnvf", "glTexEnvfv", "glTexEnvi", "glTexEnviv",
    "glTexGend", "glTexGendv", "glTexGenf", "glTexGenfv",
    "glTexGeni", "glTexGeniv", "glFeedbackBuffer", "glSelectBuffer",
    "glRenderMode", "glInitNames", "glLoadName", "glPassThrough",
    "glPopName", "glPushName", "glDrawBuffer", "glClear",
    "glClearAccum", "glClearIndex", "glClearColor", "glClearStencil",
    "glClearDepth", "glStencilMask", "glColorMask", "glDepthMask",
    "glIndexMask", "glAccum", "glDisable", "glEnable",
    "glFinish", "glFlush", "glPopAttrib", "glPushAttrib",
    "glMap1d", "glMap1f", "glMap2d", "glMap2f",
    "glMapGrid1d", "glMapGrid1f", "glMapGrid2d", "glMapGrid2f",
    "glEvalCoord1d", "glEvalCoord1dv", "glEvalCoord1f", "glEvalCoord1fv",
    "glEvalCoord2d", "glEvalCoord2dv", "glEvalCoord2f", "glEvalCoord2fv",
    "glEvalMesh1", "glEvalPoint1", "glEvalMesh2", "glEvalPoint2",
    "glAlphaFunc", "glBlendFunc", "glLogicOp", "glStencilFunc",
    "glStencilOp", "glDepthFunc", "glPixelZoom", "glPixelTransferf",
    "glPixelTransferi", "glPixelStoref", "glPixelStorei", "glPixelMapfv",
    "glPixelMapuiv", "glPixelMapusv", "glReadBuffer", "glCopyPixels",
    "glReadPixels", "glDrawPixels", "glGetBooleanv", "glGetClipPlane",
    "glGetDoublev", "glGetError", "glGetFloatv", "glGetIntegerv",
    "glGetLightfv", "glGetLightiv", "glGetMapdv", "glGetMapfv",
    "glGetMapiv", "glGetMaterialfv", "glGetMaterialiv", "glGetPixelMapfv",
    "glGetPixelMapuiv", "glGetPixelMapusv", "glGetPolygonStipple", "glGetString",
    "glGetTexEnvfv", "glGetTexEnviv", "glGetTexGendv", "glGetTexGenfv",
    "glGetTexGeniv", "glGetTexImage", "glGetTexParameterfv", "glGetTexParameteriv",
    "glGetTexLevelParameterfv", "glGetTexLevelParameteriv", "glIsEnabled", "glIsList",
    "glDepthRange", "glFrustum", "glLoadIdentity", "glLoadMatrixf",
    "glLoadMatrixd", "glMatrixMode", "glMultMatrixf", "glMultMatrixd",
    "glOrtho", "glPopMatrix", "glPushMatrix", "glRotated",
    "glRotatef", "glScaled", "glScalef", "glTranslated",
    "glTranslatef", "glViewport", "glArrayElement", "glBindTexture",
    "glColorPointer", "glDisableClientState", "glDrawArrays", "glDrawElements",
    "glEdgeFlagPointer", "glEnableClientState", "glIndexPointer", "glIndexub",
    "glIndexubv", "glInterleavedArrays", "glNormalPointer", "glPolygonOffset",
    "glTexCoordPointer", "glVertexPointer", "glAreTexturesResident", "glCopyTexImage1D",
    "glCopyTexImage2D", "glCopyTexSubImage1D", "glCopyTexSubImage2D", "glDeleteTextures",
    "glGenTextures", "glGetPointerv", "glIsTexture", "glPrioritizeTextures",
    "glTexSubImage1D", "glTexSubImage2D", "glPopClientAttrib", "glPushClientAttrib",
};

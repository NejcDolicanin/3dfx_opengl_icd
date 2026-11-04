/*
** sst_vtxcopy.c -- og generator pieces: the FIFO vertex-field copies (one vertex's fields, selected by the
** packet format bits `flags`, from [src + field] through `tmp` into the packet at [edi + *off]), the texture
** coordinate copies into the vertex [ebp], and one row of an inlined matrix transform.
*/
#include "wglpriv.h"
#include "og.h"
#include <glide.h>
#include "glsi.h"
#include "icdprotos.h"

/* the packet format bits = the setup unit's parameter mask (SST_SETUP_RGB | _A, _Z, _Wfbi, _W0, _ST0, _W1, _ST1) */
#define FMT_COLOR       3                           /* r, g, b, a (or the packed colour) */
#define FMT_Z           4
#define FMT_W           8
#define FMT_Q0          0x10                        /* TMU0 q */
#define FMT_ST0         0x20                        /* TMU0 s, t */
#define FMT_Q1          0x40
#define FMT_ST1         0x80

/* offset of a __GLvertexICD field (the generated code addresses vertices by register) */
/* texture[u].x / .y / .w (s, t, q) of a vertex; the original's expression shapes (u is a TMU's unit) */
#define TEX_S(u)        (((u) + 2) * 0x10)                  /* VOFS(texture[0].x) + u * 0x10 */
#define TEX_T(u)        ((u) * 0x10 + VOFS(texture[0].y))
#define TEX_Q(u)        ((u) * 0x10 + VOFS(texture[0].w))

#define PKT             MEMR(*off, edi)                                     /* the packet slot [edi + *off] */

/* field at src + o -> the next packet word */
#define COPY(o) \
    (EMIT(og_mov(gc, tmp, MEMB((o), src))), \
     EMIT(og_mov(gc, PKT, tmp)), \
     *off += 4)

/* 69030c20 -- hwCopyVertexFloat: the packet words of the vertex [src] with its float colour */
GLubyte *__fastcall hwCopyVertexFloat(__GLcontext *gc, GLubyte *pc, OgOperand *src, GLint *off,
                                 GLubyte flags, OgOperand *tmp)
{
    COPY(VOFS(window.x));
    COPY(VOFS(window.y));
    if (flags & FMT_COLOR) {
        COPY(VOFS(colors[0].r));
        COPY(VOFS(colors[0].g));
        COPY(VOFS(colors[0].b));
        COPY(VOFS(colors[0].a));
    }
    if (flags & FMT_Z)
        COPY(VOFS(window.z));
    if (flags & FMT_W)
        COPY(VOFS(window.w));
    if (flags & FMT_Q0)
        COPY(TEX_Q(gc->fx.tmuTexUnit[0]));
    if (flags & FMT_ST0) {
        COPY(TEX_S(gc->fx.tmuTexUnit[0]));
        COPY(TEX_T(gc->fx.tmuTexUnit[0]));
    }
    if (flags & FMT_Q1)
        COPY(TEX_Q(gc->fx.tmuTexUnit[1]));
    if (flags & FMT_ST1) {
        COPY(TEX_S(gc->fx.tmuTexUnit[1]));
        COPY(TEX_T(gc->fx.tmuTexUnit[1]));
    }
    return pc;
}

/* 690306e0 -- hwCopyVertexPacked: the same with the packed colour, from `csrc` (the provoking vertex)
   unless smooth */
GLubyte *__fastcall hwCopyVertexPacked(__GLcontext *gc, GLubyte *pc, OgOperand *src, GLint *off,
                                 GLubyte flags, OgOperand *tmp, OgOperand *csrc)
{
    COPY(VOFS(window.x));
    COPY(VOFS(window.y));
    if (flags & FMT_COLOR) {
        if (gc->state.light.shadingModel == GL_SMOOTH)
            EMIT(og_mov(gc, tmp, MEMB(VOFS(packedColor), src)));
        else
            EMIT(og_mov(gc, tmp, MEMB(VOFS(packedColor), csrc)));
        EMIT(og_mov(gc, PKT, tmp));
        *off += 4;
    }
    if (flags & FMT_Z)
        COPY(VOFS(window.z));
    if (flags & FMT_W)
        COPY(VOFS(window.w));
    if (flags & FMT_Q0)
        COPY(TEX_Q(gc->fx.tmuTexUnit[0]));
    if (flags & FMT_ST0) {
        COPY(TEX_S(gc->fx.tmuTexUnit[0]));
        COPY(TEX_T(gc->fx.tmuTexUnit[0]));
    }
    if (flags & FMT_Q1)
        COPY(TEX_Q(gc->fx.tmuTexUnit[1]));
    if (flags & FMT_ST1) {
        COPY(TEX_S(gc->fx.tmuTexUnit[1]));
        COPY(TEX_T(gc->fx.tmuTexUnit[1]));
    }
    return pc;
}

/* field o of the hardware vertex numbered [src] in the field-major vertex arrays (vtxBase + o * vtxCapacity) */
#define HWCOPY(o) \
    (EMIT(og_mov(gc, tmp, og_mem(gc, IMM((GLuint)gc->hwcx.vtxBase + (o) * gc->hwcx.vtxCapacity), NULL, src, 4))), \
     EMIT(og_mov(gc, PKT, tmp)), \
     *off += 4)

/* 69030120 -- hwCopyVertexIndexed: the same for an indexed hardware vertex (src = its index) */
GLubyte *__fastcall hwCopyVertexIndexed(__GLcontext *gc, GLubyte *pc, OgOperand *src, GLint *off,
                                 GLubyte flags, OgOperand *tmp, OgOperand *unused)
{
    HWCOPY(VOFS(window.x));
    HWCOPY(VOFS(window.y));
    if (flags & FMT_COLOR)
        HWCOPY(VOFS(packedColor));
    if (flags & FMT_Z)
        HWCOPY(VOFS(window.z));
    if (flags & FMT_W)
        HWCOPY(VOFS(window.w));
    if (flags & FMT_Q0)
        HWCOPY(TEX_Q(gc->fx.tmuTexUnit[0]));
    if (flags & FMT_ST0) {
        HWCOPY(TEX_S(gc->fx.tmuTexUnit[0]));
        HWCOPY(TEX_T(gc->fx.tmuTexUnit[0]));
    }
    if (flags & FMT_Q1)
        HWCOPY(TEX_Q(gc->fx.tmuTexUnit[1]));
    if (flags & FMT_ST1) {
        HWCOPY(TEX_S(gc->fx.tmuTexUnit[1]));
        HWCOPY(TEX_T(gc->fx.tmuTexUnit[1]));
    }
    return pc;
}

/* texture[i].x / .y / .w of the vertex in ebp, by the unit number i */
#define VX_S(i)         (VOFS(texture[0].x) + (i) * 0x10)
#define VX_T(i)         (VOFS(texture[0].y) + (i) * 0x10)
#define VX_Q(i)         (VOFS(texture[0].w) + (i) * 0x10)

/* 69021da0 -- hwGenTexCoords: per enabled unit: texgen units copy the current texcoord into the vertex
   [ebp], call the texgen routine (__glCalcMixedTexture) and add the TMU's scale exponents; the others
   og_texCoordXform */
GLubyte *__fastcall hwGenTexCoords(__GLcontext *gc, GLubyte *pc)
{
    GLint i;

    for (i = 0; i < gc->fx.numTMUs; i++) {
        if (!gc->texture.textureEnabled[i])
            continue;
        if (gc->state.enables.texture[i] & (__GL_TEXTURE_GEN_S_ENABLE | __GL_TEXTURE_GEN_T_ENABLE |
                                            __GL_TEXTURE_GEN_R_ENABLE | __GL_TEXTURE_GEN_Q_ENABLE)) {
            EMIT(og_mov(gc, R(eax), MEM(&gc->state.current.texture[i].x)));
            EMIT(og_mov(gc, R(ebx), MEM(&gc->state.current.texture[i].y)));
            EMIT(og_mov(gc, R(ecx), MEM(&gc->state.current.texture[i].w)));
            EMIT(og_mov(gc, MEMR(VX_S(i), ebp), R(eax)));
            EMIT(og_mov(gc, MEMR(VX_T(i), ebp), R(ebx)));
            EMIT(og_mov(gc, MEMR(VX_Q(i), ebp), R(ecx)));
            EMIT(og_push(gc, R(edx)));
            EMIT(og_mov(gc, R(eax), IMM(i)));
            EMIT(og_push(gc, R(eax)));
            EMIT(og_mov(gc, R(edx), R(ebp)));
            EMIT(og_mov(gc, R(ecx), IMM(gc)));
            pc = og_callRel32(gc, pc, (GLuint)__glSSTCalcMixedTextureUnit);
            EMIT(og_pop(gc, R(edx)));
            EMIT(og_mov(gc, R(eax), MEMR(VX_S(i), ebp)));
            EMIT(og_mov(gc, R(ebx), MEMR(VX_T(i), ebp)));
            EMIT(og_add(gc, R(eax), MEM(&gc->hwcx.texScaleExp[i][0])));
            EMIT(og_add(gc, R(ebx), MEM(&gc->hwcx.texScaleExp[i][1])));
            EMIT(og_mov(gc, MEMR(VX_S(i), ebp), R(eax)));
            EMIT(og_mov(gc, MEMR(VX_T(i), ebp), R(ebx)));
        } else
            pc = og_texCoordXform(gc, pc, i);
    }
    return pc;
}

/* 69020ac0 -- hwCopyTexCoords: both units: the current texture coordinate plus the TMU scale exponents
   into the vertex [ebp] */
GLubyte *__fastcall hwCopyTexCoords(__GLcontext *gc, GLubyte *pc)
{
    int i;

    for (i = 0; i < 2; i++) {
        EMIT(og_mov(gc, R(ebx), MEM(&gc->state.current.texture[i].x)));
        EMIT(og_mov(gc, R(edx), MEM(&gc->state.current.texture[i].y)));
        EMIT(og_mov(gc, R(esi), MEM(&gc->hwcx.texScaleExp[i][0])));
        EMIT(og_mov(gc, R(edi), MEM(&gc->hwcx.texScaleExp[i][1])));
        EMIT(og_add(gc, R(ebx), R(esi)));
        EMIT(og_add(gc, R(edx), R(edi)));
        EMIT(og_mov(gc, MEMR(VX_S(i), ebp), R(ebx)));
        EMIT(og_mov(gc, MEMR(VX_T(i), ebp), R(edx)));
    }
    return pc;
}

/* 69048c70 -- og_genMatrixRow: st0 = row `row` of the matrix [ecx] dotted with the vector [eax]: m->+0x44
   holds 2 bits per element (0 zero, 3 one), `mask` 2 bits per vector component; terms known to be zero are
   skipped, a 1 drops the multiply.  The loop runs on the original's four induction variables (`row`
   becomes the matrix-bit shift).
   Original quirk kept: an all-zero row loads its 0.0 from `row`'s own stack slot, an address in this
   generator's frame baked into the generated code; when the routine runs that slot holds whatever is
   there then (the right 0.0 only by chance). */
GLubyte *__fastcall og_genMatrixRow(__GLcontext *gc, GLubyte *pc, GLint row, GLuint *m, GLuint mask)
{
    GLuint vs, vo, mo;
    GLint terms;

    terms = 0;
    for (vo = 0, mo = row * 4, vs = 0, row *= 2; vs < 8; vs += 2, vo += 4, row += 8, mo += 16) {
        GLuint me = (m[0x11] >> row) & 3;
        GLuint ve = (mask >> vs) & 3;
        if (me && ve) {
            if (me == 3)
                EMIT(og_fld(gc, MEMR(vo, eax)));
            else if (ve == 3)
                EMIT(og_fld(gc, MEMR(mo, ecx)));
            else {
                EMIT(og_fld(gc, MEMR(mo, ecx)));
                EMIT(og_fmul(gc, MEMR(vo, eax)));
            }
            terms++;
        }
    }
    if (terms > 0) {
        for (; terms > 1; terms--)
            EMIT(og_faddp(gc, R(st1), R(st0)));
        return pc;
    }
    row = 0;
    return og_encode(gc, pc, og_fld(gc, MEM(&row)));
}

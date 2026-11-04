/*
** gl_image.c -- s_image.c: __glFillImage3D, the client image copy into the packed internal
** format (display lists, glPolygonStipple, the texture paths).  The ICD's SI predates the 3D
** unpack modes: an image is height rows, there are no image-height / skip-images settings, and
** the modes are gc->state.pixel.unpackModes.
**
** Then the rest of s_image.c.
*/
#include <string.h>
#include "wglpriv.h"
#include "glsi.h"
#include "si_names.h"
#include "icdprotos.h"
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include "glsitypes.h"
#include "glshade.h"

/* the bit tables: __glMsbToLsbTable[256] is followed by LowBitsMask[9], then HighBitsMask[9] */
/* 690f4568 .. 690f467d -- s_image.c's tables: each byte with its bits reversed, and the low / high n bits set */
GLubyte __glMsbToLsbTable[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

static GLubyte LowBitsMask[9] = {
    0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff,
};

static GLubyte HighBitsMask[9] = {
    0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff,
};

/* 69095d00 -- __glFillImage3D */
void __fastcall __glFillImage3D(__GLcontext *gc,
		     GLsizei width, GLsizei height, GLsizei depth,
		     GLenum format, GLenum type,
		     const GLvoid *userdata, GLubyte *newimage)
{
    GLint components;
    GLint element_size;
    GLint rowsize;
    GLint imagesize;
    GLint padding;
    GLint line_length = gc->state.pixel.unpackModes.lineLength;
    GLint alignment = gc->state.pixel.unpackModes.alignment;
    GLint skip_pixels = gc->state.pixel.unpackModes.skipPixels;
    GLint skip_lines = gc->state.pixel.unpackModes.skipLines;
    GLint groups_per_line;
    GLint group_size;
    GLint lsb_first = gc->state.pixel.unpackModes.lsbFirst;
    GLint swap_bytes = gc->state.pixel.unpackModes.swapEndian;
    GLint elements_per_line;
    const GLubyte *start;
    const GLubyte *rowStart;
    const GLubyte *src;
    GLubyte *dst;
    GLint h, i, j, k;

    components = __glElementsPerGroup(format, type);
    if (line_length > 0) {
	groups_per_line = line_length;
    } else {
	groups_per_line = width;
    }

    /* All formats except GL_BITMAP fall out trivially */
    if (type == GL_BITMAP) {
	GLint elements_left;
	GLint bit_offset;
	GLint current_byte;
	GLint next_byte;
	GLint high_bit_mask;
	GLint low_bit_mask;

	rowsize = (groups_per_line * components + 7) / 8;
	padding = (rowsize % alignment);
	if (padding) {
	    rowsize += alignment - padding;
	}
	start = ((const GLubyte*) userdata) + skip_lines * rowsize +
		(skip_pixels * components / 8);
	bit_offset = (skip_pixels * components) % 8;
	high_bit_mask = LowBitsMask[8-bit_offset];
	low_bit_mask = HighBitsMask[bit_offset];
	elements_per_line = width * components;
	dst = newimage;
	for (i = 0; i < height; i++) {
	    elements_left = elements_per_line;
	    src = start;
	    while (elements_left) {
		/* First retrieve low bits from current byte */
		if (lsb_first) {
		    current_byte = __glMsbToLsbTable[src[0]];
		} else {
		    current_byte = src[0];
		}
		if (bit_offset) {
		    /* Need to read next byte to finish current byte */
		    if (elements_left > (8 - bit_offset)) {
			if (lsb_first) {
			    next_byte = __glMsbToLsbTable[src[1]];
			} else {
			    next_byte = src[1];
			}
			current_byte =
			    ((current_byte & high_bit_mask) << bit_offset) |
			    ((next_byte & low_bit_mask) >> (8 - bit_offset));
		    } else {
			current_byte =
			    ((current_byte & high_bit_mask) << bit_offset);
		    }
		}
		if (elements_left >= 8) {
		    *dst = (GLubyte) current_byte;
		    elements_left -= 8;
		} else {
		    *dst = (GLubyte)
			(current_byte & HighBitsMask[elements_left]);
		    elements_left = 0;
		}
		dst++;
		src++;
	    }
	    start += rowsize;
	}
    } else {
	element_size = (GLint)__glBytesPerElement(type);
	group_size = element_size * components;
	if (element_size == 1) swap_bytes = 0;

	rowsize = groups_per_line * group_size;
	padding = (rowsize % alignment);
	if (padding) {
	    rowsize += alignment - padding;
	}

	imagesize = height * rowsize;

	start = ((const GLubyte*) userdata) +
		skip_lines * rowsize +
		skip_pixels * group_size;
	dst = newimage;
	elements_per_line = width * components;

	if (swap_bytes) {
	    for (h = 0; h < depth; h++) {
		rowStart = start;
		for (i = 0; i < height; i++) {
		    src = rowStart;
		    for (j = 0; j < elements_per_line; j++) {
			for (k = 1; k <= element_size; k++) {
			    dst[k-1] = src[element_size - k];
			}
			dst += element_size;
			src += element_size;
		    }
		    rowStart += rowsize;
		}
		start += imagesize;
	    }
	} else {
	    for (h = 0; h < depth; h++) {
		if (rowsize == elements_per_line * element_size) {
		    /* Ha!  This is mondo easy! */
		    __GL_MEMCOPY(dst, start, height * rowsize);
		    dst += height * rowsize;
		} else {
		    src = start;
		    for (i = 0; i < height; i++) {
			__GL_MEMCOPY(dst, src, elements_per_line*element_size);
			dst += elements_per_line * element_size;
			src += rowsize;
		    }
		}
		start += imagesize;
	    }
	}
    }
}

/* ---- s_image.c ---------------------------------------------------------------------------------- */
/* 69095c90 -- __glImageSize (s_image.c, the older SI: no depth) */
GLint __fastcall __glImageSize(GLsizei width, GLsizei height, GLenum format, GLenum type)
{
    GLint bytes_per_row;
    GLint components;

    components = __glElementsPerGroup(format, type);
    if (type == GL_BITMAP) {
	bytes_per_row = (width + 7) >> 3;
    } else {
	bytes_per_row = __glBytesPerElement(type) * width;
    }
    return bytes_per_row * components * height;
}

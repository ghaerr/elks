#ifndef ALREADY_INCLUDED_OUTPUT
#define ALREADY_INCLUDED_OUTPUT
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 2002 - 2005 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software, alter it and re-
 * distribute it freely for any non-commercial, non-profit purpose subject to
 * the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software in a
 *   product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *   be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any distribution.
 *
 *   4. The names of this software and/or it's copyright holders may not be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 */

#include "buf.h"
#include "flags.h"
#include <stdio.h>

struct output_ctx {
    unsigned char bitbuf;
    unsigned char bitcount;
    int pos;
    int start;
    struct buf *buf;
    int flags_proto;
};

void output_ctx_init(struct output_ctx *ctx,    /* IN/OUT */
                     int flags_proto, /* IN */
                     struct buf *out);       /* IN/OUT */

unsigned int output_get_pos(struct output_ctx *ctx);    /* IN */

void output_byte(struct output_ctx *ctx,        /* IN/OUT */
                 unsigned char byte);   /* IN */

void output_word(struct output_ctx *ctx,        /* IN/OUT */
                 unsigned short int word);      /* IN */

void output_bits_flush(struct output_ctx *ctx,  /* IN/OUT */
                       int add_marker_bit);     /* IN */

int output_bits_alignment(struct output_ctx *ctx);      /* IN */

void output_bits(struct output_ctx *ctx,        /* IN/OUT */
                 int count,     /* IN */
                 int val);      /* IN */

void output_gamma_code(struct output_ctx *ctx,  /* IN/OUT */
                       int code);       /* IN */
#ifdef __cplusplus
}
#endif
#endif

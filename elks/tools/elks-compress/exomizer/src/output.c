/*
 * Copyright (c) 2002 - 2007 Magnus Lind.
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

#include <stdlib.h>
#include "log.h"
#include "output.h"

static void bitbuf_bit(struct output_ctx *ctx, int bit)
{
    if (ctx->flags_proto & PFLAG_BITS_ORDER_BE)
    {
        /* ror (new) */
        ctx->bitbuf >>= 1;
        if (bit)
        {
            ctx->bitbuf |= 0x80;
        }
        if (++ctx->bitcount == 8)
        {
            output_bits_flush(ctx, 0);
        }
    }
    else
    {
        /* rol (old) */
        ctx->bitbuf <<= 1;
        if (bit)
        {
            ctx->bitbuf |= 0x01;
        }
        if (++ctx->bitcount == 8)
        {
            output_bits_flush(ctx, 0);
        }
    }
}

void output_ctx_init(struct output_ctx *ctx,    /* IN/OUT */
                     int flags_proto, /* IN */
                     struct buf *out)/* IN/OUT */
{
    ctx->bitbuf = 0;
    ctx->bitcount = 0;
    ctx->pos = buf_size(out);
    ctx->buf = out;
    ctx->flags_proto = flags_proto;
}

unsigned int output_get_pos(struct output_ctx *ctx)     /* IN */
{
    return ctx->pos;
}

void output_byte(struct output_ctx *ctx,        /* IN/OUT */
                 unsigned char byte)    /* IN */
{
    /*LOG(LOG_DUMP, ("output_byte: $%02X\n", byte)); */
    if(ctx->pos < buf_size(ctx->buf))
    {
        char *p;
        p = buf_data(ctx->buf);
        p[ctx->pos] = byte;
    }
    else
    {
        while(ctx->pos > buf_size(ctx->buf))
        {
            buf_append_char(ctx->buf, '\0');
        }
        buf_append_char(ctx->buf, byte);
    }
    ++(ctx->pos);
}

void output_word(struct output_ctx *ctx,        /* IN/OUT */
                 unsigned short int word)       /* IN */
{
    output_byte(ctx, (unsigned char) (word & 0xff));
    output_byte(ctx, (unsigned char) (word >> 8));
}


void output_bits_flush(struct output_ctx *ctx,  /* IN/OUT */
                       int add_marker_bit)      /* IN */
{
    if (add_marker_bit)
    {
        if (ctx->flags_proto & PFLAG_BITS_ORDER_BE)
        {
            ctx->bitbuf |= (0x80 >> ctx->bitcount);
        }
        else
        {
            ctx->bitbuf |= (0x01 << ctx->bitcount);
        }
        ++ctx->bitcount;
    }
    if (ctx->bitcount > 0)
    {
        /* flush the bitbuf */
        output_byte(ctx, ctx->bitbuf);
        LOG(LOG_DUMP, ("bitstream flushed 0x%02X\n", ctx->bitbuf));
        /* reset it */
        ctx->bitbuf = 0;
        ctx->bitcount = 0;
    }
}

int output_bits_alignment(struct output_ctx *ctx)
{
    int alignment = (8 - ctx->bitcount) & 7;
    LOG(LOG_DUMP, ("bitbuf 0x%02X aligned %d\n", ctx->bitbuf, alignment));
    return alignment;
}

static void output_bits_int(struct output_ctx *ctx,        /* IN/OUT */
                            int count,     /* IN */
                            int val)       /* IN */
{
    if (ctx->flags_proto & PFLAG_BITS_COPY_GT_7)
    {
        while (count > 7)
        {
            /* at least 8 bits or more are left */
            output_byte(ctx, (unsigned char)(val & 0xFF));
            count -= 8;
            val >>= 8;
        }
    }

    while (count-- > 0)
    {
        bitbuf_bit(ctx, val & 1);
        val >>= 1;
    }
}

void output_bits(struct output_ctx *ctx,        /* IN/OUT */
                 int count,     /* IN */
                 int val)       /* IN */
{
    LOG(LOG_DUMP, ("output bits: count = %d, val = %d\n", count, val));
    output_bits_int(ctx, count, val);
}

void output_gamma_code(struct output_ctx *ctx,  /* IN/OUT */
                       int code)        /* IN */
{
    LOG(LOG_DUMP, ("output gamma: code = %d\n", code));
    output_bits_int(ctx, 1, 1);
    while (code-- > 0)
    {
        output_bits_int(ctx, 1, 0);
    }
}

/*
 * Copyright (c) 2002 - 2005 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software in a
 *   product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *   be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any source distribution.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "exodec.h"
#include "log.h"

static int bitbuf_rotate(struct dec_ctx *ctx, int carry)
{
    int carry_out;
    if (ctx->flags_proto & PFLAG_BITS_ORDER_BE)
    {
        /* rol (new) */
        carry_out = (ctx->bitbuf & 0x80) != 0;
        ctx->bitbuf <<= 1;
        if (carry)
        {
            ctx->bitbuf |= 0x01;
        }
    }
    else
    {
        /* ror (old) */
        carry_out = ctx->bitbuf & 0x01;
        ctx->bitbuf >>= 1;
        if (carry)
        {
            ctx->bitbuf |= 0x80;
        }
    }
    return carry_out;
}

static char *get(struct buf *buf)
{
    return buf_data(buf);
}

static int
get_byte(struct dec_ctx *ctx)
{
    int c;
    if(ctx->inpos == ctx->inend)
    {
        LOG(LOG_ERROR, ("unexpected end of input data\n"));
        exit(1);
    }
    c = ctx->inbuf[ctx->inpos++];
    ctx->bits_read += 8;

    return c;
}

static int
get_bits(struct dec_ctx *ctx, int count)
{
    int byte_copy = 0;
    int val;

    val = 0;
    if (ctx->flags_proto & PFLAG_BITS_COPY_GT_7)
    {
        while (count > 7)
        {
            byte_copy = count >> 3;
            count &= 7;
        }
    }

    /*printf("get_bits: count = %d", count);*/
    while(count-- > 0)
    {
        int carry = bitbuf_rotate(ctx, 0);
        if (ctx->bitbuf == 0)
        {
            ctx->bitbuf = get_byte(ctx);
            ctx->bits_read -= 8;
            carry = bitbuf_rotate(ctx, 1);
        }
        val <<= 1;
        val |= carry;

        /*printf("bit read %d\n", val &1);*/
        ctx->bits_read++;
    }
    /*printf(" val = %d\n", val);*/

    while (byte_copy-- > 0)
    {
        val <<= 8;
        val |= get_byte(ctx);
    }

    return val;
}

static int
get_gamma_code(struct dec_ctx *ctx)
{
    int gamma_code;
    /* get bitnum index */
    gamma_code = 0;
    while(get_bits(ctx, 1) == 0)
    {
        ++gamma_code;
    }
    return gamma_code;
}

static int
get_cooked_code_phase2(struct dec_ctx *ctx, int index)
{
    int base;
    struct dec_table *tp;
    tp = &ctx->t;

    base = tp->table_lo[index] | (tp->table_hi[index] << 8);
    return base + get_bits(ctx, tp->table_bi[index]);
}

static
void
table_init(struct dec_ctx *ctx, struct dec_table *tp) /* IN/OUT */
{
    int i, end;
    unsigned int a = 0;
    unsigned int b = 0;

    tp->table_bit[0] = 2;
    tp->table_bit[1] = 4;
    tp->table_bit[2] = 4;
    if (ctx->flags_proto & PFLAG_4_OFFSET_TABLES)
    {
        end = 68;
        tp->table_bit[3] = 4;

        tp->table_off[0] = 64;
        tp->table_off[1] = 48;
        tp->table_off[2] = 32;
        tp->table_off[3] = 16;
    }
    else
    {
        end = 52;
        tp->table_off[0] = 48;
        tp->table_off[1] = 32;
        tp->table_off[2] = 16;
    }

    for(i = 0; i < end; ++i)
    {
        if(i & 0xF)
        {
            a += 1 << b;
        } else
        {
            a = 1;
        }

        tp->table_lo[i] = a & 0xFF;
        tp->table_hi[i] = a >> 8;

        if (ctx->flags_proto & PFLAG_BITS_COPY_GT_7)
        {
            b = get_bits(ctx, 3);
            b |= get_bits(ctx, 1) << 3;
        }
        else
        {
            b = get_bits(ctx, 4);
        }
        tp->table_bi[i] = b;
    }
}

void
dec_ctx_init(struct dec_ctx *ctx,
             struct buf *enc_in, /* optional */
             struct buf *inbuf, struct buf *outbuf, int flags_proto)
{
    struct buf *enc = enc_in;

    if (enc_in == NULL)
    {
        enc = inbuf;
    }
    ctx->bits_read = 0;
    ctx->inbuf = buf_data(enc);
    ctx->inend = buf_size(enc);
    ctx->inpos = 0;
    ctx->flags_proto = flags_proto;

    ctx->outbuf = outbuf;

    if (flags_proto & PFLAG_BITS_ALIGN_START)
    {
        ctx->bitbuf = 0;
    }
    else
    {
        /* init bitbuf */
        ctx->bitbuf = get_byte(ctx);
    }

    /* init tables */
    table_init(ctx, &ctx->t);
    if (enc_in != NULL)
    {
        /* switch to inbuf */
        ctx->inbuf = buf_data(inbuf);
        ctx->inend = buf_size(inbuf);
        ctx->inpos = 0;

        if (flags_proto & PFLAG_BITS_ALIGN_START)
        {
            ctx->bitbuf = 0;
        }
        else
        {
            /* init bitbuf */
            ctx->bitbuf = get_byte(ctx);
        }
    }
}

void
dec_ctx_table_dump(struct dec_ctx *ctx, struct buf *enc_out)
{
    if (enc_out != NULL)
    {
        int i, j, offset_tables= 3;
        if (ctx->flags_proto & PFLAG_4_OFFSET_TABLES)
        {
            offset_tables = 4;
        }

        buf_clear(enc_out);
        for(i = 0; i < 16; ++i)
        {
            buf_printf(enc_out, "%X", ctx->t.table_bi[i]);
        }
        for(j = 0; j < offset_tables; ++j)
        {
            int start;
            int end;
            buf_append_char(enc_out, ',');
            start = ctx->t.table_off[j];
            end = start + (1 << ctx->t.table_bit[j]);
            for(i = start; i < end; ++i)
            {
                buf_printf(enc_out, "%X", ctx->t.table_bi[i]);
            }
        }
    }
}

void dec_ctx_free(struct dec_ctx *ctx)
{
}

void dec_ctx_decrunch(struct dec_ctx *ctx)
{
    int bits = ctx->bits_read;
    int val;
    int i;
    int len;
    int literal = 1;
    int offset = 0;
    int src = 0;
    int treshold = (ctx->flags_proto & PFLAG_4_OFFSET_TABLES)? 4: 3;
    int reuse_offset_state = 1;

    if (ctx->flags_proto & PFLAG_IMPL_1LITERAL)
    {
        goto literal_start;
    }

    for(;;)
    {
        int reuse_offset;
        reuse_offset_state <<= 1;
        reuse_offset_state |= literal;

        literal = 0;
        bits = ctx->bits_read;
        LOG(LOG_DEBUG, ("[%02X]",ctx->bitbuf));
        if(get_bits(ctx, 1))
        {
        literal_start:
            /* literal */
            len = 1;

            LOG(LOG_DEBUG, ("[%d] literal $%02X\n",
                            buf_size(ctx->outbuf),
                            ctx->inbuf[ctx->inpos]));

            literal = 1;

            goto literal;
        }

        val = get_gamma_code(ctx);
        if(val == 16)
        {
            /* done */
            break;
        }
        if(val == 17)
        {
            len = get_bits(ctx, 16);
            literal = 1;

            LOG(LOG_DEBUG, ("[%d] literal copy len %d\n",
                            buf_size(ctx->outbuf), len));

            goto literal;
        }

        len = get_cooked_code_phase2(ctx, val);

        reuse_offset = 0;
        if ((ctx->flags_proto & PFLAG_REUSE_OFFSET) != 0 &&
            (reuse_offset_state & 3) == 1)
        {
            /* offset reuse state, read a bit */
            reuse_offset = get_bits(ctx, 1);
            LOG(LOG_DEBUG, ("[%d] offset reuse bit = %d, latest = %d\n",
                            buf_size(ctx->outbuf), reuse_offset,
                            offset));
        }
        if (!reuse_offset)
        {
            i = (len > treshold ? treshold : len) - 1;
            val = ctx->t.table_off[i] + get_bits(ctx, ctx->t.table_bit[i]);
            offset = get_cooked_code_phase2(ctx, val);
        }
        LOG(LOG_DEBUG, ("[%d] sequence offset = %d, len = %d\n",
                        buf_size(ctx->outbuf), offset, len));

        src = buf_size(ctx->outbuf) - offset;

    literal:
        do {
            if(literal)
            {
                val = get_byte(ctx);
            }
            else
            {
                val = get(ctx->outbuf)[src++];
            }
            buf_append_char(ctx->outbuf, val);
        } while (--len > 0);

        if (ctx->flags_proto & PFLAG_4_OFFSET_TABLES)
        {
            LOG(LOG_DEBUG, ("bits read for this iteration %d, total %d.\n",
                            ctx->bits_read - bits, ctx->bits_read - 280));
        }
        else
        {
            LOG(LOG_DEBUG, ("bits read for this iteration %d, total %d.\n",
                            ctx->bits_read - bits, ctx->bits_read - 216));
        }
    }
}

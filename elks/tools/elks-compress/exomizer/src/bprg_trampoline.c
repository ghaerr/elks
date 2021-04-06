/*
 * Copyright (c) 2003 Magnus Lind.
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

#include "bprg.h"
#include "log.h"

#include <stdlib.h>

struct trampoline {
    int init;
    int start;
    int size;
};

static int
trampoline_cb_line_mutate(const unsigned char *in, /* IN */
                          unsigned char *mem, /* IN/OUT */
                          unsigned short *pos, /* IN/OUT */
                          void *priv) /* IN/OUT */
{
    unsigned short start;
    unsigned short i;

    struct trampoline *t;

    t = priv;
    if(!t->init)
    {
        /* move pointer ahead */
        *pos = t->start + t->size;
        t->init = 1;
    }
    start = *pos;
    i = 0;

    /* skip link */
    i += 2;
    in += 2;
    /* copy number */
    mem[start + i++] = *(in++);
    mem[start + i++] = *(in++);
    /* copy line including terminating '\0' */
    while((mem[start + i++] = *(in++)) != '\0');

    /* set link properly */
    mem[start] = start + i;
    mem[start + 1] = (start + i) >> 8;

    *pos = start + i;
    /* success */
    return 0;
}

void
bprg_trampoline_add(struct bprg_ctx *ctx,
                    int *start, int *var, int *end,
                    int flags)
{
    struct trampoline t;
    int tmp;
    int endpos;
    int i;

    if(start == NULL)
    {
        tmp = ctx->basic_start;
        start = &tmp;
    }

    t.init = 0;
    t.start = *start;

    /* start */
    t.size = 8;

    /* var */
    t.size += 8;

    if(end != NULL)
    {
        t.size += 8;
    }
    t.size += 7;
    if(flags & TRAMPOLINE_FLAG_REGEN)
    {
        t.size += 3;
    }
    if(flags & TRAMPOLINE_FLAG_C264_COLOR_REGEN)
    {
        t.size += 3;
    }
    bprg_lines_mutate(ctx, trampoline_cb_line_mutate, &t);

    endpos = ctx->start + ctx->len;
    if(var != NULL && endpos > *var)
    {
        LOG(LOG_ERROR, ("error, basic ends at $%04X, "
                        "must be >= basic vars at $%04X\n",
                        endpos,
                        *var));
        exit(1);
    }
    ctx->start = *start;
    ctx->basic_start = ctx->start + t.size;
    ctx->len = endpos - ctx->start;

    i = ctx->start;
    /* lda #<start */
    ctx->mem[i++] = 0xA9;
    ctx->mem[i++] = ctx->basic_start & 0xFF;
    /* sta $2B */
    ctx->mem[i++] = 0x85;
    ctx->mem[i++] = 0x2B;
    /* lda #<start */
    ctx->mem[i++] = 0xA9;
    ctx->mem[i++] = ctx->basic_start >> 8;
    /* sta $2C */
    ctx->mem[i++] = 0x85;
    ctx->mem[i++] = 0x2C;

    if(var != NULL)
    {
        endpos = *var;
    }
    /* lda #<endpos */
    ctx->mem[i++] = 0xA9;
    ctx->mem[i++] = endpos & 0xFF;
    /* sta $2D */
    ctx->mem[i++] = 0x85;
    ctx->mem[i++] = 0x2D;
    /* lda #<endpos */
    ctx->mem[i++] = 0xA9;
    ctx->mem[i++] = endpos >> 8;
    /* sta $2E */
    ctx->mem[i++] = 0x85;
    ctx->mem[i++] = 0x2E;

    if(end != NULL)
    {
        /* lda #<end */
        ctx->mem[i++] = 0xA9;
        ctx->mem[i++] = *end & 0xFF;
        /* sta $37 */
        ctx->mem[i++] = 0x85;
        ctx->mem[i++] = 0x37;
        /* lda #>end */
        ctx->mem[i++] = 0xA9;
        ctx->mem[i++] = *end >> 8;
        /* sta $38 */
        ctx->mem[i++] = 0x85;
        ctx->mem[i++] = 0x38;
    }
    if(flags & TRAMPOLINE_FLAG_C264)
    {
        /* jsr $8BBE */
        ctx->mem[i++] = 0x20;
        ctx->mem[i++] = 0xBE;
        ctx->mem[i++] = 0x8B;

        if(flags & TRAMPOLINE_FLAG_REGEN)
        {
            /* jsr $8818 */
            ctx->mem[i++] = 0x20;
            ctx->mem[i++] = 0x18;
            ctx->mem[i++] = 0x88;
        }

        if(flags & TRAMPOLINE_FLAG_C264_COLOR_REGEN)
        {
            /* jsr $F3B5 */
            ctx->mem[i++] = 0x20;
            ctx->mem[i++] = 0xB5;
            ctx->mem[i++] = 0xF3;
        }

        /* jmp $8BDC */
        ctx->mem[i++] = 0x4C;
        ctx->mem[i++] = 0xDC;
        ctx->mem[i++] = 0x8B;
    } else /* c64 trampoline */
    {
        /* jsr $A659 */
        ctx->mem[i++] = 0x20;
        ctx->mem[i++] = 0x59;
        ctx->mem[i++] = 0xA6;

        if(flags & TRAMPOLINE_FLAG_REGEN)
        {
            /* jsr $A533 */
            ctx->mem[i++] = 0x20;
            ctx->mem[i++] = 0x33;
            ctx->mem[i++] = 0xA5;
        }
        /* jmp $A7AE */
        ctx->mem[i++] = 0x4C;
        ctx->mem[i++] = 0xAE;
        ctx->mem[i++] = 0xA7;
    }
    ctx->mem[i++] = 0x00;
}

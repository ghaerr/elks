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

#include <stdio.h>
#include <stdlib.h>
#include "bprg.h"
#include "log.h"

FILE *
open_file(const char *name, int *load_addr);

void
bprg_init(struct bprg_ctx *ctx, const char *prg_name)
{
    FILE *in;
    int load;
    int len;

    ctx->len = 0;
    memset(ctx->mem, 0, sizeof(*ctx->mem));

    in = open_file(prg_name, &load);
    if (in == NULL)
    {
        LOG(LOG_ERROR,
            (" can't open file \"%s\" for input\n", prg_name));
        exit(1);
    }

    ctx->start = load;
    ctx->basic_start = load;

    LOG(LOG_VERBOSE, ("load start $%04X\n", load));
    len = fread(ctx->mem + load, 1, sizeof(ctx->mem) - load, in);
    LOG(LOG_VERBOSE, ("load end $%04X\n", load + len));

    fclose(in);
    LOG(LOG_NORMAL, ("read %d bytes (%d blocks) from file "
                     "\"%s\",$%04X,$%04X.\n",
                     len, (len + 255) / 254, prg_name, load, load + len));



    /* this call rebuilds broken links */
    bprg_lines_mutate(ctx, NULL, NULL);
}

void
bprg_save(struct bprg_ctx *ctx, const char *prg_name)
{
    FILE *out;
    out = fopen(prg_name,"wb");

    fputc(ctx->start & 255, out);
    fputc(ctx->start >> 8, out);

    fwrite(ctx->mem + ctx->start, 1, ctx->len, out);

    fclose(out);

    LOG(LOG_NORMAL, ("wrote %d bytes (%d blocks) to file "
                     "\"%s\",$%04X,$%04X.\n",
                     ctx->len, (ctx->len + 255) / 254, prg_name,
                     ctx->start, ctx->start + ctx->len));

}

void
bprg_free(struct bprg_ctx *ctx)
{
}

void
bprg_get_iterator(struct bprg_ctx *ctx, struct bprg_iterator *i)
{
    i->mem = ctx->mem;
    i->pos = ctx->basic_start;
}

int
bprg_iterator_next(struct bprg_iterator *i, struct brow **b)
{
    if (i->mem[i->pos + 1] == '\0')
    {
        return 0;
    }
    if(b != NULL)
    {
        *b = (struct brow*)(i->mem + i->pos);
    }

    /* set pos to next line */
    i->pos = i->mem[i->pos] | (i->mem[i->pos + 1] << 8);

    return 1;
}

static int clone_cb_line_mutate(const unsigned char *in, /* IN */
                           unsigned char *mem, /* IN/OUT */
                           unsigned short *pos, /* IN/OUT */
                           void *priv) /* IN/OUT */
{
    unsigned short start;
    unsigned short i;

    start = *pos;
    i = 0;

    /* skip link */
    i += 2; in += 2;
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
bprg_lines_mutate(struct bprg_ctx *ctx,
                  cb_line_mutate *f,
                  void *priv)
{
    unsigned short pos;
    unsigned short len;
    static unsigned char mem[65536];
    unsigned char *p;

    memset(mem, 0, sizeof(mem));

    pos = ctx->start;
    p = ctx->mem + pos;

    /* copy trampoline */
    while (pos < ctx->basic_start)
    {
        mem[pos++] = *(p++);
    }

    while (p[1] != '\0')
    {
        /* call callback function */
        if(f == NULL || f(p, mem, &pos, priv) != 0)
        {
            /* it was not interested, call plain clone */
            clone_cb_line_mutate(p, mem, &pos, priv);
        }
        /* skip link and line */
        p += 4;
        /* eat line including terminating '\0' */
        while(*(p++) != 0);
    }
    mem[pos++] = '\0';
    mem[pos++] = '\0';
    len = pos - ctx->start;

    /* copy local stuff to context */
    memcpy(ctx->mem, mem, sizeof(mem));
    ctx->len = len;

    LOG(LOG_DUMP, ("program length %hu\n", ctx->len));
}

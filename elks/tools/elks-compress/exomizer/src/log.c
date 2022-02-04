/*
 * Copyright (c) 2002, 2003 Magnus Lind.
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
 * This file is a part of the Exomizer v1.1 release
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "log.h"

#ifdef WIN32
#define vsnprintf _vsnprintf
#endif

struct log_output {
    enum log_level min;
    enum log_level max;
    FILE *stream;
    int isatty;
    log_formatter_f *f;
};

struct log_ctx {
    enum log_level level;
    int out_len;
    struct log_output *out;
    int buf_len;
    char *buf;
};

struct log_ctx *G_log_ctx = NULL;
enum log_level G_log_level = LOG_MIN;
enum log_level G_log_log_level = 0;
int G_log_tty_only = 0;

struct log_ctx *log_new(void)
{
    struct log_ctx *ctx;

    ctx = malloc(sizeof(*ctx));
    if (ctx == NULL)
    {
        fprintf(stderr,
                "fatal error, can't allocate memory for log context\n");
        exit(1);
    }
    ctx->level = LOG_NORMAL;
    ctx->out_len = 0;
    ctx->out = NULL;
    ctx->buf_len = 0;
    ctx->buf = NULL;

    return ctx;
}

/* log_delete closes all added output streams
 * and files except for stdout and stderr
 */
void log_delete(struct log_ctx *ctx)
{
    int i;

    for (i = 0; i < ctx->out_len; ++i)
    {
        FILE *file = ctx->out[i].stream;
        if (file != stderr && file != stdout)
        {
            fclose(file);
        }
    }
    free(ctx->out);
    free(ctx->buf);
    free(ctx);
}

void log_set_level(struct log_ctx *ctx, /* IN/OUT */
                   enum log_level level)        /* IN */
{
    ctx->level = level;
}

void log_add_output_stream(struct log_ctx *ctx, /* IN/OUT */
                           enum log_level min,  /* IN */
                           enum log_level max,  /* IN */
                           log_formatter_f * default_f, /* IN */
                           FILE * out_stream)   /* IN */
{
    struct log_output *out;

    ctx->out_len += 1;
    ctx->out = realloc(ctx->out, ctx->out_len * sizeof(*(ctx->out)));
    if (ctx->out == NULL)
    {
        fprintf(stderr,
                "fatal error, can't allocate memory for log output\n");
        exit(1);
    }
    out = &(ctx->out[ctx->out_len - 1]);
    out->min = min;
    out->max = max;
    out->stream = out_stream;
    out->isatty = isatty(fileno(out_stream));
    out->f = default_f;
}

void raw_log_formatter(FILE * out,      /* IN */
                       enum log_level level,    /* IN */
                       const char *context,     /* IN */
                       const char *log) /* IN */
{
    fprintf(out, "%s", log);
    fflush(out);
}

void log_vlog(struct log_ctx *ctx,      /* IN */
              enum log_level level,     /* IN */
              const char *context,      /* IN */
              log_formatter_f * f,      /* IN */
              const char *printf_str,   /* IN */
              va_list argp)
{
    int len;
    int i;

    if (ctx->level < level)
    {
        /* don't log this */
        return;
    }

    len = 0;
    do
    {
        if (len >= ctx->buf_len)
        {
            ctx->buf_len = len + 1024;
            ctx->buf = realloc(ctx->buf, ctx->buf_len);
            if (ctx->buf == NULL)
            {
                fprintf(stderr,
                        "fatal error, can't allocate memory for log log\n");
                exit(1);
            }
        }
        len = vsnprintf(ctx->buf, ctx->buf_len, printf_str, argp);
    }

    while (len >= ctx->buf_len);

    for (i = 0; i < ctx->out_len; ++i)
    {
        struct log_output *o = &ctx->out[i];
        log_formatter_f *of = f;

        if (level >= o->min && level <= o->max &&
            (G_log_tty_only == 0 || o->isatty != 0))
        {
            /* generate log for this output */
            if (of == NULL)
            {
                of = o->f;
            }
            if (of != NULL)
            {
                of(o->stream, level, context, ctx->buf);
            } else
            {
                fprintf(o->stream, "%s\n", ctx->buf);
                fflush(o->stream);
            }
        }
    }
}

void log_log_default(const char *printf_str,    /* IN */
                     ...)
{
    va_list argp;
    va_start(argp, printf_str);
    log_vlog(G_log_ctx, G_log_log_level,
             NULL, raw_log_formatter, printf_str, argp);
}

void log_log(struct log_ctx *ctx,       /* IN */
             enum log_level level,      /* IN */
             const char *context,       /* IN */
             log_formatter_f * f,       /* IN */
             const char *printf_str,    /* IN */
             ...)
{
    va_list argp;
    va_start(argp, printf_str);
    log_vlog(ctx, level, context, f, printf_str, argp);
}

void hex_dump(int level, unsigned char *p, int len)
{
    int i;
    int j;
    for(i = 0; i < len;)
    {
        LOG(level, ("%02x", p[i]));
        ++i;
        if(i == len || (i & 15) == 0)
        {
            LOG(level, ("\t\""));
            for(j = (i - 1) & ~15; j < i; ++j)
            {
                unsigned char c = p[j];
                if(!isprint(c))
                {
                    c = '.';
                }
                LOG(level, ("%c", c));
            }
            LOG(level, ("\"\n"));
        }
        else
        {
            LOG(level, (","));
        }
    }
}

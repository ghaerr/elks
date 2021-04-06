/*
 * Copyright (c) 2002 2005 Magnus Lind.
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"

void buf_init(struct buf *b)
{
    b->data = NULL;
    b->size = 0;
    b->capacity = 0;
}

void buf_free(struct buf *b)
{
    if (b->capacity == -1)
    {
        fprintf(stderr, "error, can't free a buf view\n");
        exit(1);
    }
    if (b->data != NULL)
    {
        free(b->data);
        b->data = NULL;
    }
    b->size = 0;
    b->capacity = 0;
}

void buf_new(struct buf **bp)
{
    struct buf *b;

    b = malloc(sizeof(struct buf));
    if (b == NULL)
    {
        fprintf(stderr, "error, can't allocate memory\n");
        exit(1);
    }
    b->data = NULL;
    b->size = 0;
    b->capacity = 0;

    *bp = b;
}

void buf_delete(struct buf **bp)
{
    struct buf *b;

    b = *bp;
    buf_free(b);
    free(b);
    b = NULL;
    *bp = b;
}

int buf_size(const struct buf *b)
{
    return b->size;
}

int buf_capacity(const struct buf *b)
{
    return b->capacity;
}

void *buf_data(const struct buf *b)
{
    return b->data;
}

void *buf_reserve(struct buf *b, int new_capacity)
{
    int capacity = b->capacity;
    if (capacity == -1)
    {
        fprintf(stderr, "error, can't reserve capacity for a buf view\n");
        exit(1);
    }
    if (capacity == 0)
    {
        capacity = 1;
    }
    while (capacity < new_capacity)
    {
        capacity <<= 1;
    }
    if (capacity > b->capacity)
    {
        b->data = realloc(b->data, capacity);
        if (b->data == NULL)
        {
            fprintf(stderr, "error, can't reallocate memory\n");
            exit(1);
        }
        b->capacity = capacity;
    }
    return b->data;
}

void buf_clear(struct buf *b)
{
    buf_replace(b, 0, b->size, NULL, 0);
}

void buf_remove(struct buf *b, int b_off, int b_n)
{
    buf_replace(b, b_off, b_n, NULL, 0);
}

void *buf_insert(struct buf *b, int b_off, const void *m, int m_n)
{
    return buf_replace(b, b_off, 0, m, m_n);
}

void *buf_append(struct buf *b, const void *m, int m_n)
{
    return buf_replace(b, b->size, 0, m, m_n);
}

void buf_append_char(struct buf *b, char c)
{
    buf_replace(b, b->size, 0, &c, 1);
}

void buf_append_str(struct buf *b, const char *str)
{
    buf_replace(b, b->size, 0, str, strlen(str));
}

void *buf_replace(struct buf *b, int b_off, int b_n, const void *m, int m_n)
{
    int new_size;
    int rest_off;
    int rest_n;
    if (b->capacity == -1)
    {
        fprintf(stderr, "error, can't modify a buf view\n");
        exit(1);
    }
    if (b_off < 0)
    {
        b_off += b->size + 1;
    }
    if (b_n == -1)
    {
        b_n = b->size - b_off;
    }
    if(b_off < 0 || b_off > b->size)
    {
        fprintf(stderr, "error, b_off %d must be within [0 - %d].\n",
                b_off, b->size);
        exit(1);
    }
    if(b_n < 0 || b_n > b->size - b_off)
    {
        fprintf(stderr,
                "error, b_n %d must be within [0 and %d] for b_off %d.\n",
                b_n, b->size - b_off, b_off);
        exit(1);
    }
    if (m_n < 0)
    {
        fprintf(stderr, "error, m_n %d must be >= 0.\n", m_n);
        exit(1);
    }
    new_size = b->size - b_n + m_n;
    if (new_size > b->capacity)
    {
        buf_reserve(b, new_size);
    }

    rest_off = b_off + b_n;
    rest_n = b->size - rest_off;
    if (rest_n > 0)
    {
        memmove((char*)b->data + b_off + m_n,
                (char*)b->data + rest_off, rest_n);
    }
    if (m_n > 0)
    {
        if (m != NULL)
        {
            memcpy((char*)b->data + b_off, m, m_n);
        }
    }
    b->size = new_size;
    return (char*)b->data + b_off;
}

static void fskip(FILE *f, int off)
{
    int skip, remaining;
    for (remaining = off; remaining > 0; remaining -= skip)
    {
        char buf[2048];
        if (remaining == 0)
        {
            /* done */
            break;
        }
        skip = remaining < 2048 ? remaining : 2048;
        if (fread(buf, 1, skip, f) != skip)
        {
            if (feof(f))
            {
                fprintf(stderr,
                        "Error: EOF occured while skipping %d bytes.\n",
                        off);
            }
            else
            {
                fprintf(stderr,
                        "Error: Error occured while skipping %d bytes.\n",
                        off);
            }
            exit(-1);
        }
    }
}

void *buf_freplace(struct buf *b, int b_off, int b_n,
                   FILE *f, int f_off, int f_n)
{
    char buf[2048];
    int read;
    int len;
    int pos;

    if (f == NULL)
    {
        fprintf(stderr, "Error: f must not be NULL.\n");
        exit(-1);
    }
    if (b->capacity == -1)
    {
        fprintf(stderr, "error, can't modify a buf view\n");
        exit(1);
    }
    if (f_off < 0)
    {
        int f_size;
        if(fseek(f, 0, SEEK_END))
        {
            fprintf(stderr, "Error: can't seek to EOF in file.\n");
            exit(-1);
        }
        f_size = ftell(f);
        if(fseek(f, 0, SEEK_SET))
        {
            fprintf(stderr, "Error: can't seek to start in file.\n");
            exit(-1);
        }
        if (f_off < 0)
        {
            f_off += f_size + 1;
        }
        if (f_n == -1)
        {
            f_n = f_size - f_off;
        }
        if(f_off < 0 || f_off > f_size)
        {
            fprintf(stderr, "Error, f_off %d must be within [0 - %d].\n",
                    f_off, f_size);
            exit(1);
        }
        if (f_n < 0 || f_n > f_size - f_off)
        {
            fprintf(stderr,
                    "Error, f_n %d must be within [0 and %d] for f_off %d.\n",
                    f_n, f_size - f_off, f_off);
            exit(1);
        }
    }

    /* skip to f_off */
    fskip(f, f_off);
    if (ferror(f))
    {
        /* An error occured. */
        fprintf(stderr, "Error, failed to skip %d bytes in file.\n", f_off);
        exit(-1);
    }

    len = (f_n == -1 || f_n > 2048) ? 2048 : f_n;
    pos = b_off;
    while ((read = fread(buf, 1, len, f)) == len)
    {
        buf_replace(b, pos, b_n, buf, read);
        b_n = 0;
        pos += len;
        if (f_n != -1)
        {
            f_n -= len;
            len = f_n > 2048 ? 2048 : f_n;
        }
    }
    if (ferror(f))
    {
        /* A read error occured. */
        fprintf(stderr, "Error, failed to read from file.\n");
    }
    if (read > 0)
    {
        buf_replace(b, pos, b_n, buf, read);
    }
    return (char*)b->data + pos;
}

const struct buf *buf_view(struct buf *v,
                           const struct buf *b, int b_off, int b_n)
{
    if (b_off < 0)
    {
        b_off += b->size + 1;
    }
    if (b_n == -1)
    {
        b_n = b->size - b_off;
    }
    if(b_off < 0 || b_off > b->size)
    {
        fprintf(stderr, "error, b_off %d must be within [0 - %d].\n",
                b_off, b->size);
        exit(1);
    }
    if(b_n < 0 || b_n > b->size - b_off)
    {
        fprintf(stderr, "error, b_n %d must be within [0 - %d].\n",
                b_n, b->size - b_off);
        exit(1);
    }
    if (b->data != NULL)
    {
        v->data = (char*)b->data + b_off;
    }
    else
    {
        v->data = NULL;
    }
    v->size = b_n;
    v->capacity = -1;

    return v;
}

void buf_printf(struct buf *b, const char *format, ...)
{
    int pos;
    int printed;
    va_list args;

    if (b->capacity == -1)
    {
        fprintf(stderr, "error, can't printf to a buf view\n");
        exit(1);
    }

    pos = b->size;

    va_start(args, format);
    printed = vsnprintf((char*)buf_data(b) + pos, b->capacity - pos,
                        format, args);
    va_end(args);

    if (printed >= b->capacity - pos)
    {
        va_list args2;

        buf_reserve(b, pos + printed + 1);

        va_start(args2, format);
        printed = vsnprintf((char*)buf_data(b) + pos, b->capacity - pos,
                            format, args2);
        va_end(args2);
    }
    b->size += printed;
}

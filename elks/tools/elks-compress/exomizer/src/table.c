/*
 * Copyright (c) 2020 Magnus Lind.
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

#include "table.h"
#include "buf.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static void str_free(void *a)
{
    char **str = a;
    free(*str);
}

static void row_free(void *a)
{
    struct vec *vec = a;
    vec_free(vec, str_free);
}

static void col_free(void *a)
{
    struct table_col *col = a;
    free(col->title);
    free(col->value_fmt);
}

void table_init(struct table_ctx *table, int cell_padding)
{
    vec_init(&table->cols, sizeof (struct table_col));
    vec_init(&table->rows, sizeof (struct vec));
    table->cell_padding = cell_padding;
}

void table_free(struct table_ctx *table)
{
    vec_free(&table->cols, col_free);
    vec_free(&table->rows, row_free);
}

void table_col_add(struct table_ctx *table,
                   const char *title,
                   const char *value_fmt,
                   int value_align_right)
{
    struct table_col *col = vec_push(&table->cols, NULL);
    col->title = strdup(title);
    col->value_fmt = strdup(value_fmt);
    col->align_fmt = value_align_right ? "%*s|%*s%*s" : "%*s|%*s%-*s";
    col->width = strlen(title);
}

void table_row_add(struct table_ctx *table, ...)
{
    struct vec_iterator col_i;
    struct table_col *col;
    struct buf buf = STATIC_BUF_INIT;
    struct vec *row;
    const char *val;
    va_list argp;

    buf_init(&buf);
    va_start(argp, table);
    row = vec_push(&table->rows, NULL);
    vec_init(row, sizeof (char *));

    vec_get_iterator(&table->cols, &col_i);
    while ((col = vec_iterator_next(&col_i)) != NULL)
    {
        int width;
        const char *dup;
        val = va_arg(argp, const char *);

        buf_printf(&buf, col->value_fmt, val);
        dup = strdup((char*)buf_data(&buf));
        vec_push(row, &dup);

        width = buf_size(&buf);
        if (col->width < width)
        {
            /* adjust the width */
            col->width = width;
        }
        buf_clear(&buf);
    }

    va_end(argp);
}

void table_header_print(struct table_ctx *table, struct buf *target)
{
    struct vec_iterator col_i;
    struct table_col *col;
    int pre = table->cell_padding;
    int post = 0;

    vec_get_iterator(&table->cols, &col_i);
    while ((col = vec_iterator_next(&col_i)) != NULL)
    {
        buf_printf(target, "%*s|%*s%-*s", post, "", pre, "",
                   col->width, col->title);
        post = table->cell_padding;
    }
    buf_printf(target, "%*s|\n", post, "");
}

void table_sep_print(struct table_ctx *table, struct buf *target)
{
    struct vec_iterator col_i;
    struct table_col *col;
    vec_get_iterator(&table->cols, &col_i);
    while ((col = vec_iterator_next(&col_i)) != NULL)
    {
        int i;
        int width = col->width + 2 * table->cell_padding;
        buf_printf(target, "|");
        for (i = 0; i < width; ++i)
        {
            buf_printf(target, "-");
        }
    }
    buf_printf(target, "|\n");
}

void table_rows_print(struct table_ctx *table, struct buf *target)
{
    struct vec_iterator col_i;
    struct table_col *col;
    struct vec_iterator row_i;
    struct vec *row;

    vec_get_iterator(&table->rows, &row_i);
    while ((row = vec_iterator_next(&row_i)) != NULL)
    {
        struct vec_iterator cell_i;
        char *cell;
        int pre = table->cell_padding;
        int post = 0;

        vec_get_iterator(&table->cols, &col_i);
        vec_get_iterator(row, &cell_i);
        while ((col = vec_iterator_next(&col_i)) != NULL)
        {
            cell = *(char**)vec_iterator_next(&cell_i);
            buf_printf(target, col->align_fmt, post, "", pre, "",
                       col->width, cell);
            post = table->cell_padding;
        }
        buf_printf(target, "%*s|\n", post, "");
    }
}

void table_buf_print(struct table_ctx *table, struct buf *target)
{
    table_header_print(table, target);
    table_sep_print(table, target);
    table_rows_print(table, target);
}

void table_fprint(FILE *f, struct table_ctx *table)
{
    struct buf buf = STATIC_BUF_INIT;
    table_buf_print(table, &buf);
    fprintf(f, "%s", (char*)buf_data(&buf));
    buf_free(&buf);
}

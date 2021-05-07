#ifndef ALREADY_INCLUDED_TABLE
#define ALREADY_INCLUDED_TABLE
#ifdef __cplusplus
extern "C" {
#endif

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

#include "vec.h"

#define ALIGN_LEFT  0
#define ALIGN_RIGHT 1

#define STATIC_TABLE_INIT(CELL_PADDING) \
    {STATIC_VEC_INIT(sizeof struct table_col), \
     STATIC_VEC_INIT(sizeof struct vec), \
     (CELL_PADDING)}

struct table_ctx {
    struct vec cols;
    struct vec rows;
    int cell_padding;
};

struct table_col {
    char *title;
    char *value_fmt;
    const char *align_fmt;
    int width;
};

void table_init(struct table_ctx *table, int cell_padding);
void table_free(struct table_ctx *table);

void table_col_add(struct table_ctx *table,
                   const char *column_title,
                   const char *value_fmt,
                   int value_align_right);

/* the value list must consist of const char * pointers match
 * the number of columns added */
void table_row_add(struct table_ctx *table, ...);

void table_print(FILE *f, struct table_ctx *table);
void table_buf_print(struct table_ctx *table, struct buf *target);

#ifdef __cplusplus
}
#endif
#endif

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

#include "perf.h"
#include "table.h"

static int size_cycles_asc_cmp(const void *a, const void *b)
{
    const struct measurement *p1 = a;
    const struct measurement *p2 = b;
    int i = p1->size - p2->size;
    if (i == 0)
    {
        i = p1->cycles - p2->cycles;
    }
    return i;
}

static int cycles_size_asc_cmp(const void *a, const void *b)
{
    const struct measurement *p1 = a;
    const struct measurement *p2 = b;
    int i = p1->cycles - p2->cycles;
    if (i == 0)
    {
        i = p1->size - p2->size;
    }
    return i;
}

void perf_init(struct perf_ctx *perf)
{
    vec_init(&perf->measurements, sizeof (struct measurement));
}
void perf_free(struct perf_ctx *perf)
{
    vec_free(&perf->measurements, NULL);
}

void perf_add(struct perf_ctx *perf, const char *name,
              int in_size, int out_size, int cycles)
{
    float reduced = 100.0 * (out_size - in_size) / out_size;
    float cb_out = (float)cycles / out_size;
    float bkc_out = 1000 * out_size / (float)cycles;
    struct measurement *m = vec_push(&perf->measurements, NULL);
    m->name = name;
    m->size = in_size;
    m->reduced = reduced;
    m->cycles = cycles;
    m->cb_out = cb_out;
    m->bkc_out = bkc_out;
    m->hidden = 0;
}

void perf_sort_size_cycles(struct perf_ctx *perf, int pareto_hide)
{
    struct vec_iterator i;
    struct measurement *m;
    int cycles = -1;
    vec_sort(&perf->measurements, size_cycles_asc_cmp);
    vec_get_iterator(&perf->measurements, &i);
    while ((m = vec_iterator_next(&i)) != NULL)
    {
        if (cycles == -1 || cycles > m->cycles)
        {
            cycles = m->cycles;
            m->hidden = 0;
        }
        else if (pareto_hide)
        {
            m->hidden = 1;
        }
    }
}

void perf_sort_cycles_size(struct perf_ctx *perf, int pareto_hide)
{
    struct vec_iterator i;
    struct measurement *m;
    int size = -1;
    vec_sort(&perf->measurements, cycles_size_asc_cmp);
    vec_get_iterator(&perf->measurements, &i);
    while ((m = vec_iterator_next(&i)) != NULL)
    {
        if (size == -1 || size > m->size)
        {
            size = m->size;
            m->hidden = 0;
        }
        else if (pareto_hide)
        {
            m->hidden = 1;
        }
    }
}

void perf_buf_print(struct perf_ctx *perf, struct buf *target)
{
    struct table_ctx table;
    struct vec_iterator i;
    struct measurement *m;

    table_init(&table, 1);
    table_col_add(&table, "File name", "%s", 0);
    table_col_add(&table, "Size", "%d", 1);
    table_col_add(&table, "Reduced", "%.2f%%", 1);
    table_col_add(&table, "Cycles", "%d", 1);
    table_col_add(&table, "C/B", "%.2f", 1);
    table_col_add(&table, "B/kC", "%.2f", 1);

    vec_get_iterator(&perf->measurements, &i);
    while ((m = vec_iterator_next(&i)) != NULL)
    {
        if (!m->hidden)
        {
            table_row_add(&table, m->name, m->size, m->reduced,
                          m->cycles, m->cb_out, m->bkc_out);
        }
    }

    table_buf_print(&table, target);

    table_free(&table);
}

void perf_fprint(FILE *f, struct perf_ctx *perf)
{
    struct buf buf = STATIC_BUF_INIT;
    perf_buf_print(perf, &buf);
    fprintf(f, "%s", (char*)buf_data(&buf));
    buf_free(&buf);
}

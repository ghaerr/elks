#ifndef ALREADY_INCLUDED_PERF
#define ALREADY_INCLUDED_PERF
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

#include "callback.h"
#include "vec.h"
#include "buf.h"

struct measurement
{
    const char *name;
    int size;
    float reduced;
    int cycles;
    float cb_out;
    float bkc_out;
    int hidden;
};

#define STATIC_PERF_INIT {STATIC_VEC_INIT(sizeof(struct measurement))}

struct perf_ctx
{
    struct vec measurements;
};

void perf_init(struct perf_ctx *perf);
void perf_free(struct perf_ctx *perf);

void perf_add(struct perf_ctx *perf, const char *name,
              int in_size, int out_size, int cycles);

void perf_sort_size_cycles(struct perf_ctx *perf, int pareto_hide);
void perf_sort_cycles_size(struct perf_ctx *perf, int pareto_hide);

void perf_buf_print(struct perf_ctx *perf, struct buf *target);

void perf_fprint(FILE *f, struct perf_ctx *perf);

#ifdef __cplusplus
}
#endif
#endif

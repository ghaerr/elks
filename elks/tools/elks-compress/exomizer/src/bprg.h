#ifndef ALREADY_INCLUDED_BPRG
#define ALREADY_INCLUDED_BPRG
#ifdef __cplusplus
extern "C" {
#endif

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

#include "vec.h"

struct bprg_ctx {
    unsigned short len;
    unsigned short start;
    unsigned short basic_start;
    unsigned char mem[65536];
};

struct bprg_iterator {
    const unsigned char *mem;
    unsigned short pos;
};

typedef int cb_line_mutate(const unsigned char *in, /* IN */
                           unsigned char *mem, /* IN/OUT */
                           unsigned short *pos, /* IN/OUT */
                           void *priv); /* IN/OUT */

void
bprg_init(struct bprg_ctx *ctx, const char *prg_name);

void
bprg_save(struct bprg_ctx *ctx, const char *prg_name);

void
bprg_free(struct bprg_ctx *ctx);

void
bprg_get_iterator(struct bprg_ctx *ctx, struct bprg_iterator *i);

struct brow {
    unsigned char row[1]; /* Flexible array member */
};

int
bprg_iterator_next(struct bprg_iterator *i, struct brow **b);

void
bprg_lines_mutate(struct bprg_ctx *ctx,
                  cb_line_mutate *f,
                  void *priv);

/* higher level functions */
void
bprg_renumber(struct bprg_ctx *ctx,
              int start,
              int step,
              int mode);

void
bprg_link_patch(struct bprg_ctx *ctx);

void
bprg_rem_remove(struct bprg_ctx *ctx);

#define TRAMPOLINE_FLAG_C264 1
#define TRAMPOLINE_FLAG_REGEN 2
#define TRAMPOLINE_FLAG_C264_COLOR_REGEN 4

void
bprg_trampoline_add(struct bprg_ctx *ctx,
                    int *start, int *var, int *end,
                    int flags);
#ifdef __cplusplus
}
#endif
#endif

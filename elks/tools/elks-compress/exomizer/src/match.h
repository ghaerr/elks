#ifndef ALREADY_INCLUDED_MATCH
#define ALREADY_INCLUDED_MATCH
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 2002 - 2005, 2013 Magnus Lind.
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

#include "chunkpool.h"
#include "buf.h"
#include "vec.h"

struct match {
    unsigned int offset;
    unsigned short int len;
    const struct match *next;
};

struct pre_calc {
    struct match_node *single;
    const struct match *cache;
};

struct match_ctx {
    struct chunkpool m_pool;
    struct pre_calc *info;
    unsigned short int *rle;
    unsigned short int *rle_r;
    const unsigned char *buf;
    const unsigned char *noread_buf;
    int len;
    int max_offset;
    int max_len;
};

void match_ctx_init(struct match_ctx *ctx,          /* IN/OUT */
                    const struct buf *inbuf,   /* IN */
                    const struct buf *noread_inbuf,   /* IN */
                    int max_len,            /* IN */
                    int max_offset,         /* IN */
                    int favor_speed);       /* IN */

void match_ctx_free(struct match_ctx *ctx);     /* IN/OUT */

/* this needs to be called with the indexes in
 * reverse order */
const struct match *matches_get(const struct match_ctx *ctx, /* IN/OUT */
                                int index);     /* IN */

void match_delete(struct match_ctx *ctx,        /* IN/OUT */
                  struct match *mp);   /* IN */

struct match_cache_enum {
    const struct match_ctx *ctx;
    struct match tmp1;
    struct match tmp2;
    int pos;
};

typedef const struct match *match_enum_next_f(void *match_enum_data);

void match_cache_get_enum(struct match_ctx *ctx,       /* IN */
                          struct match_cache_enum *mpce);     /* IN/OUT */

const struct match *match_cache_enum_get_next(void *match_cache_enum); /* IN */

struct match_concat_enum {
    struct vec_iterator enum_iterator;
    match_enum_next_f *next_f;
    void *enum_current;
};

void match_concat_get_enum(match_enum_next_f *next_f,
                           struct vec *data_vec,
                           struct match_concat_enum *mpcce);

const struct match *match_concat_enum_get_next(void *match_concat_enum); /*IN*/

#ifdef __cplusplus
}
#endif
#endif

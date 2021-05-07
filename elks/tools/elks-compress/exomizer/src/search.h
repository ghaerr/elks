#ifndef ALREADY_INCLUDED_SEARCH
#define ALREADY_INCLUDED_SEARCH
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 2002 - 2005 Magnus Lind.
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

#include "match.h"
#include "output.h"

struct encode_int_bucket {
    unsigned int start;
    unsigned int end;
};

struct encode_match_buckets {
    struct encode_int_bucket len;
    struct encode_int_bucket offset;
};

struct search_node {
    int index;
    struct match match;
    unsigned int total_offset;
    float total_score;
    struct search_node *prev;
    unsigned int latest_offset;
};

struct encode_match_data {
    struct output_ctx *out;
    void *priv;
};

/* example of what may be used for priv data
 * field in the encode_match_data struct */
typedef float encode_int_f(int val, void *priv,
                           struct output_ctx *out,
                           struct encode_int_bucket *eibp);       /* IN */

struct encode_match_priv {
    int flags_proto;
    int flags_notrait;
    int lit_num;
    int seq_num;
    int rle_num;
    float lit_bits;
    float seq_bits;
    float rle_bits;

    encode_int_f *offset_f;
    encode_int_f *len_f;
    void *offset_f_priv;
    void *len_f_priv;

    struct output_ctx *out;
};

/* end of example */

typedef float encode_match_f(const struct match *mp,
                             struct encode_match_data *emd,     /* IN */
                             unsigned int prev_offset,
                             struct encode_match_buckets *embp);    /* OUT */

void search_node_dump(const struct search_node *snp);        /* IN */

/* Dont forget to free the (*result) after calling */
void search_buffer(struct match_ctx *ctx,       /* IN */
                   encode_match_f * f,  /* IN */
                   struct encode_match_data *emd,       /* IN */
                   int flags_proto,             /* IN */
                   int flags_notrait,           /* IN */
                   int max_sequence_length,     /* IN */
                   int greedy,   /* IN */
                   struct search_node **result);       /* IN */

struct match_snp_enum {
    const struct search_node *startp;
    const struct search_node *currp;
};

void match_snp_get_enum(const struct search_node *snp,   /* IN */
                        struct match_snp_enum *snpe); /* IN/OUT */

const struct match *match_snp_enum_get_next(void *matchp_snp_enum);

#ifdef __cplusplus
}
#endif
#endif

/*
 * Copyright (c) 2002 - 2018 Magnus Lind.
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "log.h"
#include "search.h"
#include "progress.h"

static void update_snp(struct search_node *snp,
                       float total_score,
                       unsigned int total_offset,
                       struct search_node *prev,
                       struct match *match,
                       int flags_proto)
{
    unsigned int latest_offset = 0;
    snp->total_score = total_score;
    snp->total_offset = total_offset;
    snp->prev = prev;
    snp->match = *match;

    if ((flags_proto & PFLAG_REUSE_OFFSET) != 0)
    {
        if (match->offset == 0)
        {
            /* we are a literal */
            struct match *prev_match = &prev->match;
            if (prev_match->offset > 0)
            {
                /* and the previous was a sequence, set the offset */
                latest_offset = prev_match->offset;
            }
        }
    }
    snp->latest_offset = latest_offset;
}

void search_buffer(struct match_ctx *ctx,       /* IN */
                   encode_match_f * f,  /* IN */
                   struct encode_match_data *emd,       /* IN */
                   int flags_proto,             /* IN */
                   int flags_notrait,           /* IN */
                   int max_sequence_length,     /* IN */
                   int greedy,   /* IN */
                   struct search_node **result)/* OUT */
{
    struct progress prog;
    struct search_node *sn_arr;
    const struct match *mp = NULL;
    struct search_node *snp;
    struct search_node *best_copy_snp;
    int best_copy_len;

    struct search_node *best_rle_snp;

    int use_literal_sequences = !(flags_notrait & TFLAG_LIT_SEQ);
    int skip_len0123_mirrors = flags_notrait & TFLAG_LEN0123_SEQ_MIRRORS;
    int len = ctx->len + 1;

    if (skip_len0123_mirrors)
    {
        if (flags_proto & PFLAG_4_OFFSET_TABLES)
        {
            skip_len0123_mirrors = 4;
        }
        else
        {
            skip_len0123_mirrors = 3;
        }
    }

    progress_init(&prog, "finding.shortest.path.",len, 0);

    sn_arr = malloc(len * sizeof(struct search_node));
    memset(sn_arr, 0, len * sizeof(struct search_node));

    --len;
    snp = &sn_arr[len];
    snp->index = len;
    snp->match.offset = 0;
    snp->match.len = 0;
    snp->total_offset = 0;
    snp->total_score = 0;
    snp->prev = NULL;
    snp->latest_offset = 0;

    best_copy_snp = snp;
    best_copy_len = 0;

    best_rle_snp = NULL;

    /* think twice about changing this code,
     * it works the way it is. The last time
     * I examined this code I was certain it was
     * broken and broke it myself, trying to fix it. */
    for (;;)
    {
        float prev_score;
        float latest_offset_sum;

        if (use_literal_sequences)
        {
            /* check if we can do even better with copy */
            snp = &sn_arr[len];
            if((snp->match.offset != 0 || snp->match.len != 1) &&
               (best_copy_snp->total_score+best_copy_len * 8.0 -
                snp->total_score > 0.0 ||
                best_copy_len > max_sequence_length))
            {
                /* found a better copy endpoint */
                LOG(LOG_DEBUG,
                    ("best copy start moved to index %d\n", snp->index));
                best_copy_snp = snp;
                best_copy_len = 0;
            } else
            {
                float copy_score = best_copy_len * 8.0 + (1.0 + 17.0 + 17.0);
                float total_copy_score = best_copy_snp->total_score +
                                         copy_score;

                LOG(LOG_DEBUG,
                    ("total score %0.1f, copy total score %0.1f\n",
                     snp->total_score, total_copy_score));

                if (snp->total_score > total_copy_score &&
                    best_copy_len <= max_sequence_length &&
                    !(skip_len0123_mirrors &&
                      /* must be < 2 due to PBIT_IMPL_1LITERAL adjustment */
                      best_copy_len > 255 && (best_copy_len & 255) < 2))
                {
                    struct match local_m;
                    /* here it is good to just copy instead of crunch */

                    LOG(LOG_DEBUG,
                        ("copy index %d, len %d, total %0.1f, copy %0.1f\n",
                         snp->index, best_copy_len,
                         snp->total_score, total_copy_score));

                    local_m.len = best_copy_len;
                    local_m.offset = 0;
                    local_m.next = NULL;

                    update_snp(snp,
                               total_copy_score,
                               best_copy_snp->total_offset,
                               best_copy_snp,
                               &local_m,
                               flags_proto);
                }
            }
            /* end of copy optimization */
        }

        /* check if we can do rle */
        snp = &sn_arr[len];
        if(best_rle_snp == NULL ||
           snp->index + max_sequence_length < best_rle_snp->index ||
           snp->index + ctx->rle_r[snp->index] < best_rle_snp->index)
        {
            /* best_rle_snp can't be reached by rle from snp, reset it*/
            if(ctx->rle[snp->index] > 0)
            {
                best_rle_snp = snp;
                LOG(LOG_DEBUG, ("resetting best_rle at index %d, len %d\n",
                                 snp->index, ctx->rle[snp->index]));
            }
            else
            {
                best_rle_snp = NULL;
            }
        }
        else if(ctx->rle[snp->index] > 0 &&
                snp->index + ctx->rle_r[snp->index] >= best_rle_snp->index)
        {
            float best_rle_score;
            float total_best_rle_score;
            float snp_rle_score;
            float total_snp_rle_score;
            struct match rle_m;

            LOG(LOG_DEBUG, ("challenger len %d, index %d, "
                             "ruling len %d, index %d\n",
                             ctx->rle_r[snp->index], snp->index,
                             ctx->rle_r[best_rle_snp->index],
                             best_rle_snp->index));

            /* snp and best_rle_snp is the same rle area,
             * let's see which is best */
            rle_m.len = ctx->rle[best_rle_snp->index];
            rle_m.offset = 1;
            best_rle_score = f(&rle_m, emd, best_rle_snp->latest_offset, NULL);
            total_best_rle_score = best_rle_snp->total_score +
                best_rle_score;

            rle_m.len = ctx->rle[snp->index];
            rle_m.offset = 1;
            snp_rle_score = f(&rle_m, emd, snp->latest_offset, NULL);
            total_snp_rle_score = snp->total_score + snp_rle_score;

            if(total_snp_rle_score <= total_best_rle_score)
            {
                /* yes, the snp is a better rle than best_rle_snp */
                LOG(LOG_DEBUG, ("prospect len %d, index %d, (%0.1f+%0.1f) "
                                 "ruling len %d, index %d (%0.1f+%0.1f)\n",
                                 ctx->rle[snp->index], snp->index,
                                 snp->total_score, snp_rle_score,
                                 ctx->rle[best_rle_snp->index],
                                 best_rle_snp->index,
                                 best_rle_snp->total_score, best_rle_score));
                best_rle_snp = snp;
                LOG(LOG_DEBUG, ("setting current best_rle: "
                                 "index %d, len %d\n",
                                 snp->index, rle_m.len));
            }
        }
        if(best_rle_snp != NULL && best_rle_snp != snp)
        {
            float rle_score;
            float total_rle_score;
            /* check if rle is better */
            struct match local_m;
            local_m.len = best_rle_snp->index - snp->index;
            local_m.offset = 1;

            rle_score = f(&local_m, emd, best_rle_snp->latest_offset, NULL);
            total_rle_score = best_rle_snp->total_score + rle_score;

            LOG(LOG_DEBUG, ("comparing index %d (%0.1f) with "
                             "rle index %d, len %d, total score %0.1f %0.1f\n",
                             snp->index, snp->total_score,
                             best_rle_snp->index, local_m.len,
                             best_rle_snp->total_score, rle_score));

            if(snp->total_score > total_rle_score)
            {
                /*here it is good to do rle instead of crunch */
                LOG(LOG_DEBUG,
                    ("rle index %d, len %d, total %0.1f, rle %0.1f\n",
                     snp->index, local_m.len,
                     snp->total_score, total_rle_score));

                update_snp(snp,
                           total_rle_score,
                           best_rle_snp->total_offset + 1,
                           best_rle_snp,
                           &local_m,
                           flags_proto);
            }
        }
        /* end of rle optimization */

        if (len == 0)
        {
            break;
        }
        mp = matches_get(ctx, len - 1);
        LOG(LOG_DUMP,
            ("matches for index %d with total score %0.1f\n",
             len - 1, snp->total_score));

        prev_score = sn_arr[len].total_score;
        latest_offset_sum = sn_arr[len].total_offset;
        while (mp != NULL)
        {
            const struct match *next;
            int end_len;
            struct match tmp;
            int bucket_len_start;
            float score;
            struct search_node *prev_snp;

            next = mp->next;
            end_len = 1;
            tmp = *mp;
            tmp.next = NULL;
            bucket_len_start = 0;
            prev_snp = &sn_arr[len];

            for(tmp.len = mp->len; tmp.len >= end_len; --(tmp.len))
            {
                float total_score;
                unsigned int total_offset;
                struct encode_match_buckets match_buckets = {{0, 0}, {0, 0}};

                LOG(LOG_DUMP, ("mp[%d, %d], tmp[%d, %d]\n",
                               mp->offset, mp->len,
                               tmp.offset, tmp.len));
                if (bucket_len_start == 0 ||
                    tmp.len < 4 ||
                    tmp.len < bucket_len_start ||
                    (skip_len0123_mirrors && tmp.len > 255 &&
                     (tmp.len & 255) < skip_len0123_mirrors))
                {
                    score = f(&tmp, emd, prev_snp->latest_offset,
                              &match_buckets);
                    bucket_len_start = match_buckets.len.start;
                }

                total_score = prev_score + score;
                total_offset = latest_offset_sum + tmp.offset;
                snp = &sn_arr[len - tmp.len];

                LOG(LOG_DUMP,
                    ("[%05d] cmp [%05d, %05d score %.1f + %.1f] with %.1f",
                     len, tmp.offset, tmp.len,
                     prev_score, score, snp->total_score));

                if (total_score < 100000000.0 &&
                    (snp->match.len == 0 ||
                     total_score < snp->total_score ||
                     (total_score == snp->total_score &&
                      total_offset < snp->total_offset &&
                      (greedy ||
                       (snp->match.len == 1 && snp->match.offset > 8) ||
                       tmp.offset > 48 ||
                       tmp.len > 15))))
                {
                    LOG(LOG_DUMP, (", replaced"));
                    snp->index = len - tmp.len;

                    update_snp(snp,
                               total_score,
                               total_offset,
                               prev_snp,
                               &tmp,
                               flags_proto);
                }
                LOG(LOG_DUMP, ("\n"));
            }
            LOG(LOG_DUMP, ("tmp.len %d, ctx->rle[%d] %d\n",
                           tmp.len, len - tmp.len,
                           ctx->rle[len - tmp.len]));

            mp = next;
        }

        /* slow way to get to the next node for cur */
        --len;
        ++best_copy_len;

        progress_bump(&prog, len);
    }
    if(len > 0 && mp == NULL)
    {
        LOG(LOG_ERROR, ("No matches at len %d.\n", len));
    }

    progress_free(&prog);

    *result = sn_arr;
}

void match_snp_get_enum(const struct search_node *snp, /* IN */
                        struct match_snp_enum *snpe)  /* IN/OUT */
{
    snpe->startp = snp;
    snpe->currp = snp;
}

const struct match *match_snp_enum_get_next(void *match_snp_enum)
{
    struct match_snp_enum *snpe = match_snp_enum;
    const struct match *val;

    if (snpe->currp == NULL)
    {
    restart:
        val = NULL;
        snpe->currp = snpe->startp;
    }
    else
    {
        val = &snpe->currp->match;
        if (val->len == 0)
        {
            /* restart here too */
            goto restart;
        }
        snpe->currp = snpe->currp->prev;
    }
    return val;
}

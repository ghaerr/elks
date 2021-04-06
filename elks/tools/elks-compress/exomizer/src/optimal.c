/*
 * Copyright (c) 2002 - 2007 Magnus Lind.
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "log.h"
#include "search.h"
#include "radix.h"
#include "chunkpool.h"
#include "optimal.h"

struct interval_node {
    int start;
    int score;
    struct interval_node *next;
    signed char prefix;
    signed char bits;
    signed char depth;
    signed char flags;
};

static
void
interval_node_init(struct interval_node *inp, int start, int depth, int flags)
{
    inp->start = start;
    inp->flags = flags;
    inp->depth = depth;
    inp->bits = 0;
    inp->prefix = flags >= 0 ? flags : depth + 1;
    inp->score = -1;
    inp->next = NULL;
}

static struct interval_node *interval_node_clone(struct interval_node *inp)
{
    struct interval_node *inp2 = NULL;

    if(inp != NULL)
    {
        inp2 = malloc(sizeof(struct interval_node));
        if (inp2 == NULL)
        {
            LOG(LOG_ERROR, ("out of memory error in file %s, line %d\n",
                            __FILE__, __LINE__));
            exit(0);
        }
        /* copy contents */
        *inp2 = *inp;
        inp2->next = interval_node_clone(inp->next);
    }

    return inp2;
}

static void interval_node_delete(struct interval_node *inp)
{
    struct interval_node *inp2;
    while (inp != NULL)
    {
        inp2 = inp;
        inp = inp->next;
        free(inp2);
    }
}

static void interval_node_dump(int level, struct interval_node *inp)
{
    int end;

    end = 0;
    while (inp != NULL)
    {
        end = inp->start + (1 << inp->bits);
        LOG(level, ("%X", inp->bits));
        inp = inp->next;
    }
    LOG(level, ("[eol@%d]\n", end));
}

float optimal_encode_int(int arg, void *priv, struct output_ctx *out,
                         struct encode_int_bucket *eibp)
{
    struct interval_node *inp;
    int end;

    float val;

    inp = priv;
    val = 100000000.0;
    end = 0;
    while (inp != NULL)
    {
        end = inp->start + (1 << inp->bits);
        if (arg >= inp->start && arg < end)
        {
            break;
        }
        inp = inp->next;
    }
    if (inp != NULL)
    {
        val = (float) (inp->prefix + inp->bits);
        if (eibp != NULL)
        {
            eibp->start = inp->start;
            eibp->end = end;
        }
    } else
    {
        val += (float) (arg - end);
        if (eibp != NULL)
        {
            eibp->start = 0;
            eibp->end = 0;
        }
    }
    LOG(LOG_DUMP, ("encoding %d to %0.1f bits\n", arg, val));

    if (out != NULL)
    {
        output_bits(out, inp->bits, arg - inp->start);
        if (inp->flags < 0)
        {
            LOG(LOG_DUMP, ("gamma prefix code = %d\n", inp->depth));
            output_gamma_code(out, inp->depth);
        } else
        {
            LOG(LOG_DUMP, ("flat prefix %d bits\n", inp->depth));
            output_bits(out, inp->prefix, inp->depth);
        }
    }

    return val;
}

float optimal_encode(const struct match *mp,
                     struct encode_match_data *emd,
                     unsigned int prev_offset,
                     struct encode_match_buckets *embp)
{
    struct interval_node **offset;
    float bits;
    struct encode_match_priv *data;
    struct encode_int_bucket *eib_len = NULL;
    struct encode_int_bucket *eib_offset = NULL;
    if (embp != NULL)
    {
        eib_len = &embp->len;
        eib_offset = &embp->offset;
    }

    data = emd->priv;
    offset = data->offset_f_priv;

    bits = 0.0;
    if (mp->len > 255 && (data->flags_notrait & TFLAG_LEN0123_SEQ_MIRRORS) &&
        (mp->len & 255) < ((data->flags_proto & PFLAG_4_OFFSET_TABLES)
                           ? 4 : 3))
    {
        bits += 100000000.0;
    }
    if (mp->offset == 0)
    {
        bits += 9.0f * mp->len;
        data->lit_num += mp->len;
        data->lit_bits += bits;
    } else
    {
        bits += 1.0;
        if (mp->offset != prev_offset)
        {
            switch (mp->len)
            {
            case 0:
                LOG(LOG_ERROR, ("bad len\n"));
                exit(1);
                break;
            case 1:
                if (data->flags_notrait & TFLAG_LEN1_SEQ)
                {
                    bits += 100000000.0;
                }
                else
                {
                    bits += data->offset_f(mp->offset, offset[0], emd->out,
                                           eib_offset);
                }
                break;
            case 2:
                bits += data->offset_f(mp->offset, offset[1], emd->out,
                                       eib_offset);
                break;
            case 3:
                if (data->flags_proto & PFLAG_4_OFFSET_TABLES)
                {
                    bits += data->offset_f(mp->offset, offset[2], emd->out,
                                           eib_offset);
                    break;
                }
            default:
                bits += data->offset_f(mp->offset, offset[7], emd->out,
                                       eib_offset);
                break;
            }
        }
        if (prev_offset > 0)
        {
            /* reuse previous offset state cost */
            bits += 1.0;
            if (emd->out != NULL)
            {
                output_bits(emd->out, 1, mp->offset == prev_offset);
            }
        }
        bits += data->len_f(mp->len, data->len_f_priv, emd->out, eib_len);
        if (bits > (9.0 * mp->len))
        {
            /* lets make literals out of it */
            data->lit_num += 1;
            data->lit_bits += bits;
        } else
        {
            if (mp->offset == 1)
            {
                data->rle_num += 1;
                data->rle_bits += bits;
            } else
            {
                data->seq_num += 1;
                data->seq_bits += bits;
            }
        }
    }
    if (embp != NULL)
    {
        if (eib_len->start + eib_len->end == 0 ||
            eib_offset->start + eib_offset->end == 0)
        {
            eib_len->start = 0;
            eib_len->end = 0;
            eib_offset->start = 0;
            eib_offset->end = 0;
        }
    }
    return bits;
}

struct optimize_arg {
    struct radix_root cache;
    int *stats;
    int *stats2;
    int max_depth;
    int flags;
    struct chunkpool in_pool;
};

#define CACHE_KEY(START, DEPTH) ((int)((START)*32+DEPTH))

static struct interval_node *optimize1(struct optimize_arg *arg, int start,
                                       int depth, int init)
{
    struct interval_node in;
    struct interval_node *best_inp;
    int key;
    int end, i;
    int start_count, end_count;

    LOG(LOG_DUMP, ("IN start %d, depth %d\n", start, depth));

    do
    {
        best_inp = NULL;
        if (arg->stats[start] == 0)
        {
            break;
        }
        key = CACHE_KEY(start, depth);
        best_inp = radix_node_get(&arg->cache, key);
        if (best_inp != NULL)
        {
            break;
        }

        interval_node_init(&in, start, depth, arg->flags);

        for (i = 0; i < 16; ++i)
        {
            in.next = NULL;
            in.bits = i;
            end = start + (1 << i);

            start_count = end_count = 0;
            if (start < 1000000)
            {
                start_count = arg->stats[start];
                if (end < 1000000)
                {
                    end_count = arg->stats[end];
                }
            }

            in.score = (start_count - end_count) *
                (in.prefix + in.bits);

            /* one index below */
            LOG(LOG_DUMP, ("interval score: [%d<<%d[%d\n",
                           start, i, in.score));
            if (end_count > 0)
            {
                int penalty;
                /* we're not done, now choose between using
                 * more bits, go deeper or skip the rest */
                if (depth + 1 < arg->max_depth)
                {
                    /* we can go deeper, let's try that */
                    in.next = optimize1(arg, end, depth + 1, i);
                }
                /* get the penalty for skipping */
                penalty = 100000000;
                if (arg->stats2 != NULL)
                {
                    penalty = arg->stats2[end];
                }
                if (in.next != NULL && in.next->score < penalty)
                {
                    penalty = in.next->score;
                }
                in.score += penalty;
            }
            if (best_inp == NULL || in.score < best_inp->score)
            {
                /* it's the new best in town, use it */
                if (best_inp == NULL)
                {
                    /* allocate if null */
                    best_inp = chunkpool_malloc(&arg->in_pool);
                }
                *best_inp = in;
            }
        }
        if (best_inp != NULL)
        {
            radix_node_set(&arg->cache, key, best_inp);
        }
    }
    while (0);

    if(IS_LOGGABLE(LOG_DUMP))
    {
        LOG(LOG_DUMP, ("OUT depth %d: ", depth));
        interval_node_dump(LOG_DUMP, best_inp);
    }
    return best_inp;
}

static struct interval_node *optimize(int stats[], int stats2[],
                                      int max_depth, int flags)
{
    struct optimize_arg arg;

    struct interval_node *inp;

    arg.stats = stats;
    arg.stats2 = stats2;

    arg.max_depth = max_depth;
    arg.flags = flags;

    chunkpool_init(&arg.in_pool, sizeof(struct interval_node));

    radix_tree_init(&arg.cache);

    inp = optimize1(&arg, 1, 0, 0);

    /* use normal malloc for the winner */
    inp = interval_node_clone(inp);

    /* cleanup */
    radix_tree_free(&arg.cache, NULL, NULL);
    chunkpool_free(&arg.in_pool);

    return inp;
}

static void export_helper(struct interval_node *np, int depth,
                          struct buf *target)
{
    while(np != NULL)
    {
        buf_printf(target, "%X", np->bits);
        np = np->next;
        --depth;
    }
    while(depth-- > 0)
    {
        buf_printf(target, "0");
    }
}


void optimal_encoding_export(struct encode_match_data *emd,
                             struct buf *target)
{
    struct interval_node **offsets;
    struct encode_match_priv *data;

    buf_clear(target);
    data = emd->priv;
    offsets = data->offset_f_priv;
    export_helper(data->len_f_priv, 16, target);
    buf_append_char(target, ',');
    export_helper(offsets[0], 4, target);
    buf_append_char(target, ',');
    export_helper(offsets[1], 16, target);
    if (data->flags_proto & PFLAG_4_OFFSET_TABLES)
    {
        buf_append_char(target, ',');
        export_helper(offsets[2], 16, target);
    }
    buf_append_char(target, ',');
    export_helper(offsets[7], 16, target);
}

static void import_helper(struct interval_node **npp,
                          const char **encodingp,
                          int flags)
{
    int c;
    int start = 1;
    int depth = 0;
    const char *encoding;

    encoding = *encodingp;
    while((c = *(encoding++)) != '\0')
    {
        char buf[2] = {0, 0};
        char *dummy;
        int bits;
        struct interval_node *np;

        if(c == ',')
        {
            break;
        }

        buf[0] = c;
        bits = strtol(buf, &dummy, 16);

        LOG(LOG_DUMP, ("got bits %d\n", bits));

        np = malloc(sizeof(struct interval_node));
        interval_node_init(np, start, depth, flags);
        np->bits = bits;

        ++depth;
        start += 1 << bits;

        *npp = np;
        npp = &(np->next);
    }
    *encodingp = encoding;
}

void optimal_encoding_import(struct encode_match_data *emd,
                             const char *encoding)
{
    struct encode_match_priv *data;
    struct interval_node **npp, **offsets;

    LOG(LOG_DEBUG, ("importing encoding: %s\n", encoding));

    data = emd->priv;

    optimal_free(emd);
    optimal_init(emd, data->flags_notrait, data->flags_proto);

    data = emd->priv;
    offsets = data->offset_f_priv;

    /* lengths */
    npp = (void*)&data->len_f_priv;
    import_helper(npp, &encoding, -1);

    /* offsets, len = 1 */
    npp = &offsets[0];
    import_helper(npp, &encoding, 2);

    /* offsets, len = 2 */
    npp = &offsets[1];
    import_helper(npp, &encoding, 4);

    if (data->flags_proto & PFLAG_4_OFFSET_TABLES)
    {
        /* offsets, len = 3 */
        npp = &offsets[2];
        import_helper(npp, &encoding, 4);
    }

    /* offsets, len >= 3 */
    npp = &offsets[7];
    import_helper(npp, &encoding, 4);

    LOG(LOG_DEBUG, ("imported encoding: "));
    optimal_dump(LOG_DEBUG, emd);
}

void optimal_init(struct encode_match_data *emd,/* IN/OUT */
                  int flags_notrait,      /* IN */
                  int flags_proto)      /* IN */
{
    struct encode_match_priv *data;
    struct interval_node **inpp;

    emd->priv = malloc(sizeof(struct encode_match_priv));
    data = emd->priv;

    memset(data, 0, sizeof(struct encode_match_priv));

    data->offset_f = optimal_encode_int;
    data->len_f = optimal_encode_int;
    inpp = malloc(sizeof(struct interval_node *) * 8);
    inpp[0] = NULL;
    inpp[1] = NULL;
    inpp[2] = NULL;
    inpp[3] = NULL;
    inpp[4] = NULL;
    inpp[5] = NULL;
    inpp[6] = NULL;
    inpp[7] = NULL;
    data->offset_f_priv = inpp;
    data->len_f_priv = NULL;
    data->flags_notrait = flags_notrait;
    data->flags_proto = flags_proto;
}

void optimal_free(struct encode_match_data *emd)        /* IN */
{
    struct encode_match_priv *data;
    struct interval_node **inpp;
    struct interval_node *inp;

    data = emd->priv;

    inpp = data->offset_f_priv;
    if (inpp != NULL)
    {
        interval_node_delete(inpp[0]);
        interval_node_delete(inpp[1]);
        interval_node_delete(inpp[2]);
        interval_node_delete(inpp[3]);
        interval_node_delete(inpp[4]);
        interval_node_delete(inpp[5]);
        interval_node_delete(inpp[6]);
        interval_node_delete(inpp[7]);
    }
    free(inpp);

    inp = data->len_f_priv;
    interval_node_delete(inp);

    data->offset_f_priv = NULL;
    data->len_f_priv = NULL;
}

void freq_stats_dump(int level, int arr[])
{
    int i;
    for (i = 0; i < 32; ++i)
    {
        LOG(level, ("%d, ", arr[i] - arr[i + 1]));
    }
    LOG(level, ("\n"));
}

void freq_stats_dump_raw(int level, int arr[])
{
    int i;
    for (i = 0; i < 32; ++i)
    {
        LOG(level, ("%d, ", arr[i]));
    }
    LOG(level, ("\n"));
}

void optimal_optimize(struct encode_match_data *emd,    /* IN/OUT */
                      match_enum_next_f *enum_next_f,   /* IN */
                      void *matchp_enum)        /* IN */
{
    struct encode_match_priv *data;
    const struct match *mp;
    struct interval_node **offset;
    static int offset_arr[8][1000000];
    static int offset_parr[8][1000000];
    static int len_arr[1000000];
    int treshold;

    int i, j;

    data = emd->priv;

    memset(offset_arr, 0, sizeof(offset_arr));
    memset(offset_parr, 0, sizeof(offset_parr));
    memset(len_arr, 0, sizeof(len_arr));

    offset = data->offset_f_priv;

    /* first the lens */
    while ((mp = enum_next_f(matchp_enum)) != NULL)
    {
        if (mp->offset > 0)
        {
            len_arr[mp->len] += 1;
            if(len_arr[mp->len] < 0)
            {
                LOG(LOG_ERROR, ("len counter wrapped!\n"));
            }
        }
    }

    for (i = 65534; i >= 0; --i)
    {
        len_arr[i] += len_arr[i + 1];
        if(len_arr[i] < 0)
        {
            LOG(LOG_ERROR, ("len counter wrapped!\n"));
        }
    }

    data->len_f_priv = optimize(len_arr, NULL, 16, -1);

    /* then the offsets */
    while ((mp = enum_next_f(matchp_enum)) != NULL)
    {
        if (mp->offset > 0)
        {
            treshold = mp->len * 9;
            treshold -= 1 + (int) optimal_encode_int(mp->len,
                                                     data->len_f_priv,
                                                     NULL, NULL);
            switch (mp->len)
            {
            case 0:
                LOG(LOG_ERROR, ("bad len\n"));
                exit(0);
                break;
            case 1:
                offset_parr[0][mp->offset] += treshold;
                offset_arr[0][mp->offset] += 1;
                if(offset_arr[0][mp->offset] < 0)
                {
                    LOG(LOG_ERROR, ("offset0 counter wrapped!\n"));
                }
                break;
            case 2:
                offset_parr[1][mp->offset] += treshold;
                offset_arr[1][mp->offset] += 1;
                if(offset_arr[1][mp->offset] < 0)
                {
                    LOG(LOG_ERROR, ("offset1 counter wrapped!\n"));
                }
                break;
            case 3:
                if (data->flags_proto & PFLAG_4_OFFSET_TABLES)
                {
                    offset_parr[2][mp->offset] += treshold;
                    offset_arr[2][mp->offset] += 1;
                    if(offset_arr[2][mp->offset] < 0)
                    {
                        LOG(LOG_ERROR, ("offset2 counter wrapped!\n"));
                    }
                    break;
                }
            default:
                offset_parr[7][mp->offset] += treshold;
                offset_arr[7][mp->offset] += 1;
                if(offset_arr[7][mp->offset] < 0)
                {
                    LOG(LOG_ERROR, ("offset7 counter wrapped!\n"));
                }
                break;
            }
        }
    }

    for (i = 999998; i >= 0; --i)
    {
        for (j = 0; j < 8; ++j)
        {
            offset_arr[j][i] += offset_arr[j][i + 1];
            offset_parr[j][i] += offset_parr[j][i + 1];
        }
    }

    offset[0] = optimize(offset_arr[0], offset_parr[0], 1 << 2, 2);
    offset[1] = optimize(offset_arr[1], offset_parr[1], 1 << 4, 4);
    offset[2] = optimize(offset_arr[2], offset_parr[2], 1 << 4, 4);
    offset[3] = optimize(offset_arr[3], offset_parr[3], 1 << 4, 4);
    offset[4] = optimize(offset_arr[4], offset_parr[4], 1 << 4, 4);
    offset[5] = optimize(offset_arr[5], offset_parr[5], 1 << 4, 4);
    offset[6] = optimize(offset_arr[6], offset_parr[6], 1 << 4, 4);
    offset[7] = optimize(offset_arr[7], offset_parr[7], 1 << 4, 4);

    if(IS_LOGGABLE(LOG_DEBUG))
    {
        optimal_dump(LOG_DEBUG, emd);
    }
}

void optimal_dump(int level, struct encode_match_data *emd)
{
    struct encode_match_priv *data;
    struct interval_node **offset;
    struct interval_node *len;

    data = emd->priv;

    offset = data->offset_f_priv;
    len = data->len_f_priv;

    LOG(level, ("lens:             "));
    interval_node_dump(level, len);

    LOG(level, ("offsets (len =1): "));
    interval_node_dump(level, offset[0]);

    LOG(level, ("offsets (len =2): "));
    interval_node_dump(level, offset[1]);

    if (data->flags_proto & PFLAG_4_OFFSET_TABLES)
    {
        LOG(level, ("offsets (len =3): "));
        interval_node_dump(level, offset[2]);
    }
    LOG(level, ("offsets (len =8): "));
    interval_node_dump(level, offset[7]);
}

static void interval_out(struct output_ctx *out, struct interval_node *inp1,
                         int size, int flags_proto)
{
    unsigned char buffer[256];
    unsigned char count;
    struct interval_node *inp;

    count = 0;

    memset(buffer, 0, sizeof(buffer));
    inp = inp1;
    while (inp != NULL)
    {
        ++count;
        LOG(LOG_DUMP, ("bits %d, lo %d, hi %d\n",
                       inp->bits, inp->start & 0xFF, inp->start >> 8));
        buffer[sizeof(buffer) - count] = inp->bits;
        inp = inp->next;
    }

    while (size > 0)
    {
        int b;
        b = buffer[sizeof(buffer) - size];
        LOG(LOG_DUMP, ("outputting nibble %d\n", b));
        if (flags_proto & PFLAG_BITS_COPY_GT_7)
        {
            output_bits(out, 1, b >> 3);
            output_bits(out, 3, b & 7);
        }
        else
        {
            output_bits(out, 4, b);
        }
        size--;
    }
}

void optimal_out(struct output_ctx *out,        /* IN/OUT */
                 struct encode_match_data *emd) /* IN */
{
    struct encode_match_priv *data;
    struct interval_node **offset;
    struct interval_node *len;

    data = emd->priv;

    offset = data->offset_f_priv;
    len = data->len_f_priv;

    interval_out(out, offset[0], 4, data->flags_proto);
    interval_out(out, offset[1], 16, data->flags_proto);
    if (data->flags_proto & PFLAG_4_OFFSET_TABLES)
    {
        interval_out(out, offset[2], 16, data->flags_proto);
    }
    interval_out(out, offset[7], 16, data->flags_proto);
    interval_out(out, len, 16, data->flags_proto);
}

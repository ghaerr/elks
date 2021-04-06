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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "match.h"
#include "chunkpool.h"
#include "radix.h"
#include "buf.h"
#include "progress.h"

struct match_node {
    int index;
    struct match_node *next;
};

static const struct match *matches_calc(struct match_ctx *ctx, /* IN/OUT */
                                        int index,            /* IN */
                                        int favor_speed);     /* IN */

struct match *match_new(struct match_ctx *ctx, /* IN/OUT */
                        struct match **mpp,
                        int len,
                        int offset)
{
    struct match *m = chunkpool_malloc(&ctx->m_pool);

    if(len == 0)
    {
        LOG(LOG_ERROR, ("tried to allocate len0 match.\n"));
        *(volatile int*)0;
    }
    if(len > ctx->max_len)
    {
        len = ctx->max_len;
    }

    m->len = len;
    m->offset = offset;

    /* insert new node in list */
    m->next = *mpp;
    *mpp = m;

    return m;
}

#define IS_READABLE(NOREAD, OFFSET) \
    ((NOREAD) == NULL || (NOREAD)[OFFSET] == 0)

#define READ_BUF(IN, NOREAD, OFFSET) \
    ((NOREAD) != NULL && (NOREAD)[OFFSET] != 0 ? -1 : IN[OFFSET])

void match_ctx_init(struct match_ctx *ctx,         /* IN/OUT */
                    const struct buf *inbuf,  /* IN */
                    const struct buf *noread_inbuf,  /* IN */
                    int max_len,           /* IN */
                    int max_offset,        /* IN */
                    int favor_speed) /* IN */
{
    struct match_node *np;
    struct progress prog;

    int buf_len = buf_size(inbuf);
    const unsigned char *buf = buf_data(inbuf);
    const unsigned char *noread_buf = NULL;
    char *rle_map;

    int c, i;
    int val;

    if (noread_inbuf != NULL)
    {
        noread_buf = buf_data(noread_inbuf);
    }

    ctx->info = calloc(buf_len + 1, sizeof(*ctx->info));
    ctx->rle = calloc(buf_len + 1, sizeof(*ctx->rle));
    ctx->rle_r = calloc(buf_len + 1, sizeof(*ctx->rle_r));

    chunkpool_init(&ctx->m_pool, sizeof(struct match));

    ctx->max_offset = max_offset;
    ctx->max_len = max_len;

    ctx->buf = buf;
    ctx->len = buf_len;
    ctx->noread_buf = noread_buf;

    val = READ_BUF(buf, noread_buf, 0);
    for (i = 1; i < buf_len; ++i)
    {
        if (val != -1 && buf[i] == val)
        {
            int len = ctx->rle[i - 1] + 1;
            if(len > ctx->max_len)
            {
                len = ctx->max_len;
            }
            ctx->rle[i] = len;
        } else
        {
            ctx->rle[i] = 0;
        }
        val = READ_BUF(buf, noread_buf, i);
    }

    val = READ_BUF(buf, noread_buf, 0);
    for (i = buf_len - 2; i >= 0; --i)
    {
        if (val != -1 && buf[i] == val)
        {
            int len = ctx->rle_r[i + 1] + 1;
            if(len > ctx->max_len)
            {
                len = ctx->max_len;
            }
            ctx->rle_r[i] = len;
        } else
        {
            ctx->rle_r[i] = 0;
        }
        val = READ_BUF(buf, noread_buf, i);
    }

    /* add extra nodes to rle sequences */
    rle_map = malloc(65536);
    for(c = 0; c < 256; ++c)
    {
        struct match_node *prev_np;
        struct match_node *trailing_np;
        unsigned short int rle_len;

        /* for each possible rle char */
        memset(rle_map, 0, 65536);
        prev_np = NULL;
        trailing_np = NULL;
        for (i = 0; i < buf_len; ++i)
        {
            /* must be the correct char */
            if(buf[i] != c)
            {
                continue;
            }

            rle_len = ctx->rle[i];
            if(!rle_map[rle_len] && ctx->rle_r[i] > 16)
            {
                /* no previous lengths and not our primary length*/
                continue;
            }

            if (favor_speed &&
                ctx->rle_r[i] != 0 && ctx->rle[i] != 0)
            {
                continue;
            }

            np = chunkpool_malloc(&ctx->m_pool);
            np->index = i;
            np->next = NULL;
            rle_map[rle_len] = 1;

            LOG(LOG_DUMP, ("0) c = %d, added np idx %d -> %d\n", c, i, 0));

            /* if we have a previous entry, let's chain it together */
            if(prev_np != NULL)
            {
                LOG(LOG_DUMP, ("1) c = %d, pointed np idx %d -> %d\n",
                                c, prev_np->index, i));
                prev_np->next = np;

                if (IS_READABLE(noread_buf, prev_np->index))
                {
                    trailing_np = prev_np;
                }
            }

            if (trailing_np != NULL && IS_READABLE(noread_buf, np->index))
            {
                while(trailing_np != prev_np)
                {
                    struct match_node *tmp = trailing_np->next;
                    trailing_np->next = np;
                    trailing_np = tmp;
                }
                trailing_np = NULL;
            }

            ctx->info[i].single = np;
            prev_np = np;
        }
        while(trailing_np != NULL)
        {
            struct match_node *tmp = trailing_np->next;
            trailing_np->next = NULL;
            trailing_np = tmp;
        }

        memset(rle_map, 0, 65536);
        prev_np = NULL;
        for (i = buf_len - 1; i >= 0; --i)
        {
            /* must be the correct char */
            if(buf[i] != c)
            {
                continue;
            }

            rle_len = ctx->rle_r[i];
            np = ctx->info[i].single;
            if(np == NULL)
            {
                if(rle_map[rle_len] && prev_np != NULL && rle_len > 0)
                {
                    np = chunkpool_malloc(&ctx->m_pool);
                    np->index = i;
                    np->next = prev_np;
                    ctx->info[i].single = np;

                    LOG(LOG_DEBUG, ("2) c = %d, added np idx %d -> %d\n",
                                    c, i, prev_np->index));
                }
            }
            else if (IS_READABLE(noread_buf, np->index))
            {
                prev_np = np;
            }

            if(ctx->rle_r[i] > 0)
            {
                continue;
            }
            rle_len = ctx->rle[i] + 1;
            rle_map[rle_len] = 1;
        }
    }
    free(rle_map);

    progress_init(&prog, "building.directed.acyclic.graph.", buf_len - 1, 0);

    for (i = buf_len - 1; i >= 0; --i)
    {
        const struct match *matches;

        /* let's populate the cache */
        matches = matches_calc(ctx, i, favor_speed);

        /* add to cache */
        ctx->info[i].cache = matches;

        progress_bump(&prog, i);
    }

    progress_free(&prog);
}

void match_ctx_free(struct match_ctx *ctx)      /* IN/OUT */
{
    chunkpool_free(&ctx->m_pool);
    free(ctx->info);
    free(ctx->rle);
    free(ctx->rle_r);
}

void dump_matches(int level, const struct match *mp)
{
    if (mp == NULL)
    {
        LOG(level, (" (NULL)\n"));
    } else
    {
        if(mp->offset > 0)
        {
            LOG(level, (" offset %d, len %d\n", mp->offset, mp->len));
        }
        if (mp->next != NULL)
        {
            dump_matches(level, mp->next);
        }
    }
}

const struct match *matches_get(const struct match_ctx *ctx, /* IN/OUT */
                                int index)     /* IN */
{
    return ctx->info[index].cache;
}

/* this needs to be called with the indexes in
 * reverse order */
const struct match *matches_calc(struct match_ctx *ctx,        /* IN/OUT */
                                 int index,            /* IN */
                                 int favor_speed)      /* IN */
{
    const unsigned char *buf;
    const unsigned char *noread_buf;

    struct match *matches;
    struct match *mp;
    const struct match_node *np;

    buf = ctx->buf;
    noread_buf = ctx->noread_buf;
    matches = NULL;

    LOG(LOG_DUMP, ("index %d, char '%c', rle %d, rle_r %d\n",
                   index, buf[index], ctx->rle[index],
                   ctx->rle_r[index]));

    /* proces the literal match and add it to matches */
    mp = match_new(ctx, &matches, 1, 0);

    /* get possible match */
    np = ctx->info[index].single;
    if(np != NULL)
    {
        np = np->next;
    }
    for (; np != NULL; np = np->next)
    {
        int mp_len;
        int len;
        int pos;
        int offset;

        /* limit according to max offset */
        if(np->index > index + ctx->max_offset)
        {
            break;
        }

        LOG(LOG_DUMP, ("find lengths for index %d to index %d\n",
                        index, np->index));

        /* get match len */
        mp_len = mp->offset > 0 ? mp->len : 0;
        LOG(LOG_DUMP, ("0) comparing with current best [%d] off %d len %d\n",
                        index, mp->offset, mp_len));

        offset = np->index - index;
        len = mp_len;
        pos = index + 1 - len;
        /* Compare the first <previous len> bytes backwards. We can
         * skip some comparisons by increasing by the rle count. We
         * don't need to compare the first byte, hence > 1 instead of
         * > 0 */
        while(len > 1 && buf[pos] == READ_BUF(buf, noread_buf, pos + offset))
        {
            int offset1 = ctx->rle_r[pos];
            int offset2 = ctx->rle_r[pos + offset];
            int offset = offset1 < offset2 ? offset1 : offset2;

            LOG(LOG_DUMP, ("1) compared sucesssfully [%d] %d %d\n",
                            index, pos, pos + offset));

            len -= 1 + offset;
            pos += 1 + offset;
        }
        if(len > 1)
        {
            /* sequence length too short, skip this match */
            continue;
        }

        if(offset < 17)
        {
            /* allocate match struct and add it to matches */
            mp = match_new(ctx, &matches, 1, offset);
        }

        /* Here we know that the current match is atleast as long as
         * the previous one. let's compare further. */
        len = mp_len;
        pos = index - len;
        while(len <= ctx->max_len &&
              pos >= 0 && buf[pos] == READ_BUF(buf, noread_buf, pos + offset))
        {
            LOG(LOG_DUMP, ("2) compared sucesssfully [%d] %d %d\n",
                            index, pos, pos + offset));
            ++len;
            --pos;
        }
        if(len > mp_len || (!favor_speed && len == mp_len))
        {
            /* allocate match struct and add it to matches */
            mp = match_new(ctx, &matches, index - pos, offset);
        }
        if (len > ctx->max_len)
        {
            break;
        }
        if(pos < 0)
        {
            /* we have reached the eof, no better matches can be found */
            break;
        }
    }
    LOG(LOG_DEBUG, ("adding matches for index %d to cache\n", index));
    dump_matches(LOG_DEBUG, matches);

    return matches;
}

static int match_keep_this(const struct match *mp)
{
    int val = 1;
    /* if we want to ignore this match then return true else false */
    if(mp->len == 1)
    {
        if(mp->offset > 34)
        {
            val = 0;
        }
    }
    return val;
}

static void match_cache_peek(const struct match_ctx *ctx,
                             int pos,
                             const struct match **litpp,
                             const struct match **seqpp,
                             struct match *lit_tmp,
                             struct match *val_tmp)
{
    const struct match *litp, *seqp, *val;

    seqp = NULL;
    litp = NULL;
    if(pos >= 0)
    {
        val = matches_get(ctx, pos);
        litp = val;
        while(litp->offset != 0)
        {
            litp = litp->next;
        }

        /* inject extra rle match */
        if(ctx->rle_r[pos] > 0 && ctx->rle[pos + 1] > 0)
        {
            val_tmp->offset = 1;
            val_tmp->len = ctx->rle[pos + 1];
            val_tmp->next = (struct match *)val;
            val = val_tmp;
            LOG(LOG_DEBUG, ("injecting rle val(%d,%d)\n",
                            val->len, val->offset));
        }

        while(val != NULL)
        {
            if(val->offset != 0)
            {
                if(match_keep_this(val))
                {
                    if(seqp == NULL || val->len > seqp->len ||
                       (val->len == seqp->len && val->offset < seqp->offset))
                    {
                        seqp = val;
                    }
                }
                if(litp->offset == 0 || litp->offset > val->offset)
                {
                    LOG(LOG_DEBUG, ("val(%d,%d)", val->len, val->offset));
                    if(lit_tmp != NULL)
                    {
                        int diff;
                        struct match tmp2;
                        tmp2 = *val;
                        tmp2.len = 1;
                        diff = ctx->rle[pos + val->offset];
                        if(tmp2.offset > diff)
                        {
                            tmp2.offset -= diff;
                        }
                        else
                        {
                            tmp2.offset = 1;
                        }
                        LOG(LOG_DEBUG, ("=> litp(%d,%d)",
                                        tmp2.len, tmp2.offset));
                        if(match_keep_this(&tmp2))
                        {
                            LOG(LOG_DEBUG, (", keeping"));
                            *lit_tmp = tmp2;
                            litp = lit_tmp;
                        }
                    }
                    LOG(LOG_DEBUG, ("\n"));
                }
            }
            val = val->next;
        }
    }

    if(litpp != NULL) *litpp = litp;
    if(seqpp != NULL) *seqpp = seqp;
}

void match_cache_get_enum(struct match_ctx *ctx,       /* IN */
                           struct match_cache_enum *mpce)      /* IN/OUT */
{
    mpce->ctx = ctx;
    mpce->pos = ctx->len - 1;
}

const struct match *match_cache_enum_get_next(void *match_cache_enum)
{
    const struct match *val, *lit, *seq;
    struct match_cache_enum *mpce;

    mpce = match_cache_enum;

    match_cache_peek(mpce->ctx, mpce->pos, &lit, &seq,
                     &mpce->tmp1, &mpce->tmp2);

    val = lit;
    if(lit == NULL)
    {
        /* the end, reset enum and return NULL */
        mpce->pos = mpce->ctx->len - 1;
    }
    else
    {
        if(seq != NULL)
        {
            struct match t1;
            struct match t2;
            const struct match *next;
            match_cache_peek(mpce->ctx, mpce->pos - 1, NULL, &next, &t1, &t2);
            if(next == NULL ||
               (seq->len >= next->len + ((mpce->pos & 1) && next->len < 3)))
            {
                /* nope, next is not better, use this sequence */
                val = seq;
            }
        }
    }
    if(val != NULL)
    {
        LOG(LOG_DEBUG, ("Using len %05d, offset, %05d\n", val->len, val->offset));
        mpce->pos -= val->len;
    }
    return val;
}

void match_concat_get_enum(match_enum_next_f *next_f,
                           struct vec *data_vec,
                           struct match_concat_enum *mpcce)
{
    vec_get_iterator(data_vec, &mpcce->enum_iterator);
    mpcce->next_f = next_f;
    mpcce->enum_current = vec_iterator_next(&mpcce->enum_iterator);
}

const struct match *match_concat_enum_get_next(void *match_concat_enum) /*IN*/
{
    struct match_concat_enum *e = match_concat_enum;
    const struct match *mp = NULL;
restart:
    if (e->enum_current == NULL)
    {
        e->enum_current = vec_iterator_next(&e->enum_iterator);
    }
    else
    {
        mp = e->next_f(e->enum_current);
        if (mp == NULL)
        {
            e->enum_current = vec_iterator_next(&e->enum_iterator);
            goto restart;
        }
    }
    return mp;
}

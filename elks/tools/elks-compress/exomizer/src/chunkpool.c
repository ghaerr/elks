/*
 * Copyright (c) 2003 - 2005, 2015 Magnus Lind.
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
#include "log.h"
#include <stdlib.h>
#include <string.h>

void
chunkpool_init(struct chunkpool *ctx, int item_size)
{
    ctx->item_size = item_size;
    ctx->item_end = (0x1fffff / item_size) * item_size;
    ctx->item_pos = ctx->item_end;
    ctx->current_chunk = NULL;
    vec_init(&ctx->used_chunks, sizeof(void*));
    ctx->alloc_count = 0;
}

static void chunk_free(void *chunks, int item_pos, int item_size, cb_free *f)
{
    if (chunks != NULL && f != NULL)
    {
        do
        {
            item_pos -= item_size;
            f((char*)chunks + item_pos);
        }
        while(item_pos > 0);
    }
}

void
chunkpool_free2(struct chunkpool *ctx, cb_free *f)
{
    void **chunkp;
    struct vec_iterator i;
    if (ctx->current_chunk != NULL)
    {
        chunk_free(ctx->current_chunk, ctx->item_pos, ctx->item_size, f);
        free(ctx->current_chunk);
    }
    vec_get_iterator(&ctx->used_chunks, &i);
    while ((chunkp = vec_iterator_next(&i)) != NULL)
    {
        chunk_free(*chunkp, ctx->item_end, ctx->item_size, f);
        free(*chunkp);
    }

    ctx->item_size = -1;
    ctx->item_end = -1;
    ctx->item_pos = -1;
    ctx->current_chunk = NULL;
    vec_free(&ctx->used_chunks, NULL);
}

void
chunkpool_free(struct chunkpool *ctx)
{
    chunkpool_free2(ctx, NULL);
}

void *
chunkpool_malloc(struct chunkpool *ctx)
{
    void *p;
    if(ctx->item_pos == ctx->item_end)
    {
	void *m;
	m = malloc(ctx->item_end);
        LOG(LOG_DEBUG, ("allocating new chunk %p\n", m));
	if (m == NULL)
	{
	    LOG(LOG_ERROR, ("out of memory error in file %s, line %d\n",
			    __FILE__, __LINE__));
	    LOG(LOG_ERROR, ("alloced %d items of size %d\n",
                            ctx->alloc_count, ctx->item_size));
	    exit(1);
	}
	vec_push(&ctx->used_chunks, &ctx->current_chunk);
        ctx->current_chunk = m;
	ctx->item_pos = 0;
    }
    p = (char*)ctx->current_chunk + ctx->item_pos;
    ctx->item_pos += ctx->item_size;
    ++ctx->alloc_count;
    return p;
}

void *
chunkpool_calloc(struct chunkpool *ctx)
{
    void *p = chunkpool_malloc(ctx);
    memset(p, 0, ctx->item_size);
    return p;
}

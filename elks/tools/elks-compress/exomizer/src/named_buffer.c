/*
 * Copyright (c) 2005 Magnus Lind.
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

#include "named_buffer.h"
#include "log.h"
#include "chunkpool.h"
#include "buf_io.h"

#include <stdlib.h>

void named_buffer_init(struct named_buffer *nb)
{
    map_init(&nb->map);
    chunkpool_init(&nb->buf, sizeof(struct buf));
}

void named_buffer_free(struct named_buffer *nb)
{
    typedef void cb_free(void *a);

    chunkpool_free2(&nb->buf, (cb_free*)buf_free);
    map_free(&nb->map);
}

void named_buffer_clear(struct named_buffer *nb)
{
    named_buffer_free(nb);
    named_buffer_init(nb);
}

void named_buffer_copy(struct named_buffer *nb,
                       const struct named_buffer *source)
{
    struct map_iterator i;
    const struct map_entry *e;

    for(map_get_iterator(&source->map, &i);
        (e = map_iterator_next(&i)) != NULL;)
    {
        /* don't allocate copies of the entries */
        map_put(&nb->map, e->key, e->value);
    }
}

struct buf *new_named_buffer(struct named_buffer *nb, const char *name)
{
    struct buf *mp;

    mp = chunkpool_malloc(&nb->buf);
    /* name is already strdup:ped */
    if(map_put(&nb->map, name, mp) != NULL)
    {
        /* found */
        LOG(LOG_ERROR, ("buffer already exists.\n"));
        exit(1);
    }
    buf_init(mp);
    return mp;
}

struct buf *get_named_buffer(struct named_buffer *nb, const char *name)
{
    struct buf *mp;

    mp = map_get(&nb->map, name);
    if(mp == NULL)
    {
        mp = new_named_buffer(nb, name);
    }

    return mp;
}

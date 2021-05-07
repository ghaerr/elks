/*
 * Copyright (c) 2006 Magnus Lind.
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

#include "map.h"
#include "log.h"
#include <stdlib.h>

static int map_entry_cmp(const void *a, const void *b)
{
    struct map_entry *entry_a;
    struct map_entry *entry_b;
    int val;

    entry_a = (struct map_entry*)a;
    entry_b = (struct map_entry*)b;

    val = strcmp(entry_a->key, entry_b->key);

    return val;
}

void map_init(struct map *m)
{
    vec_init(&m->vec, sizeof(struct map_entry));
}
void map_clear(struct map *m)
{
    vec_clear(&m->vec, NULL);
}

void map_free(struct map *m)
{
    vec_free(&m->vec, NULL);
}

void *map_put(struct map *m, const char *key, void *value)
{
    struct map_entry e;
    int pos;
    void *prev_value = NULL;

    e.key = key;
    e.value = value;
    pos = vec_find(&m->vec, map_entry_cmp, &e);
    if(pos == -1)
    {
        /* error, find failed */
        LOG(LOG_ERROR, ("map_put: vec_find() internal error\n"));
        exit(1);
    }
    if(pos >= 0)
    {
        /* previous value exists, get value then set */
        struct map_entry *prev_e;
        prev_e = vec_get(&m->vec, pos);
        prev_value = prev_e->value;
        vec_set(&m->vec, pos, &e);
    }
    else
    {
        /* no value exists, insert */
        vec_insert(&m->vec, -(pos + 2), &e);
    }
    return prev_value;
}

static struct map_entry *get(const struct map *m, const char *key)
{
    struct map_entry e;
    int pos;
    struct map_entry *out = NULL;

    e.key = key;
    pos = vec_find(&m->vec, map_entry_cmp, &e);
    if(pos == -1)
    {
        /* error, find failed */
        LOG(LOG_ERROR, ("map_put: vec_find() internal error\n"));
        exit(1);
    }
    if(pos >= 0)
    {
        out = vec_get(&m->vec, pos);
    }
    return out;
}

int map_contains_key(const struct map *m, const char *key)
{
    return get(m, key) != NULL;
}

void *map_get(const struct map *m, const char *key)
{
    struct map_entry *e = get(m, key);
    void *value = NULL;
    if(e != NULL)
    {
        value = e->value;
    }
    return value;
}

void map_put_all(struct map *m, const struct map *source)
{
    struct map_iterator i;
    const struct map_entry *e;
    for(map_get_iterator(source, &i); (e = map_iterator_next(&i)) != NULL;)
    {
        map_put(m, e->key, e->value);
    }
}

int map_contains(const struct map *m1, const struct map *m2, cb_cmp *f)
{
    struct map_iterator i;
    const struct map_entry *e;

    for(map_get_iterator(m2, &i); (e = map_iterator_next(&i)) != NULL;)
    {
        int pos;
        pos = vec_find(&m1->vec, map_entry_cmp, e);
        if(pos == -1)
        {
            /* error, find failed */
            LOG(LOG_ERROR, ("map_put: vec_find() internal error\n"));
            exit(1);
        }
        if(pos < 0)
        {
            /* not found, break */
            break;
        }
        else if(f != NULL)
        {
            struct map_entry *e2;
            /* compare the values too */
            e2 = vec_get(&m1->vec, pos);
            if(f(e2->value, e->value))
            {
                /* values not equal, break */
                break;
            }
        }
    }
    return e == NULL;
}

int map_equals(const struct map *m1, const struct map *m2, cb_cmp *f)
{
    return map_contains(m1, m2, f) && map_contains(m2, m1, f);
}

void map_get_iterator(const struct map *m, struct map_iterator *i)
{
    vec_get_iterator(&m->vec, &i->vec);
}

const struct map_entry *map_iterator_next(struct map_iterator *i)
{
    return vec_iterator_next(&i->vec);
}

#ifndef ALREADY_INCLUDED_MAP
#define ALREADY_INCLUDED_MAP
#ifdef __cplusplus
extern "C" {
#endif

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

#include "vec.h"
#include "callback.h"

struct map_entry {
    const char *key;
    void *value;
};

#define STATIC_MAP_INIT {STATIC_VEC_INIT(sizeof(struct map_entry))}

struct map {
    struct vec vec;
};

struct map_iterator {
    struct vec_iterator vec;
};

void map_init(struct map *m);
void map_clear(struct map *m);
void map_free(struct map *m);

void *map_put(struct map *m, const char *key, void *value);
void *map_get(const struct map *m, const char *key);
int map_contains_key(const struct map *m, const char *key);
void map_put_all(struct map *m, const struct map *source);

int map_contains(const struct map *m1, const struct map *m2, cb_cmp *f);

/**
 * If f is NULL, only the keys will be compared.
 * returns -1 on error, 1 on equality and 0 otherwise,
 **/
int map_equals(const struct map *m1, const struct map *m2, cb_cmp *f);

void map_get_iterator(const struct map *p, struct map_iterator *i);
const struct map_entry *map_iterator_next(struct map_iterator *i);

#ifdef __cplusplus
}
#endif
#endif

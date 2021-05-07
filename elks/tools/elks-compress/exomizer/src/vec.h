#ifndef ALREADY_INCLUDED_VEC
#define ALREADY_INCLUDED_VEC
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 2003 - 2005 Magnus Lind.
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

#include "callback.h"
#include "buf.h"
#include <string.h>
#include <stdio.h>

#define STATIC_VEC_INIT(EL_SIZE) {(EL_SIZE), STATIC_BUF_INIT, 1}

struct vec {
    size_t elsize;
    struct buf buf;
    int flags;
};

struct vec_iterator {
    const struct vec *vec;
    int pos;
};

void vec_init(struct vec *p, size_t elsize);
void vec_clear(struct vec *p, cb_free * f);
void vec_free(struct vec *p, cb_free * f);

int vec_size(const struct vec *p);

/**
 * Returns a pointer to the item at the given index or NULL if the
 * index is out of bounds.
 **/
void *vec_get(const struct vec *p, int index);

/**
 * Returns a pointer to the set item or NULL if the index is out of
 * bounds.
 **/
void *vec_set(struct vec *p, int index, const void *in);

/**
 * Returns a pointer to the inserted item or NULL if the index is out of
 * bounds.
 **/
void *vec_insert(struct vec *p, int index, const void *in);
void vec_remove(struct vec *p, int index);

void *vec_push(struct vec *p, const void *in);

/**
 * Gets the position where the key is stored in the vector. The vector
 * needs to be sorted for this function to work. Returns the position,
 * -1 on error or a negative number that can be converted to where
 * it should have been if it had been inserted. insert_pos = -(val + 2)
 **/
int vec_find(const struct vec *p, cb_cmp * f, const void *key);

/**
 * Gets a pointer to the element that the key points to.
 * Returns a pointer that may be NULL if not found.
 **/
void *vec_find2(const struct vec *p, cb_cmp * f, const void *key);

/**
 * Inserts the in element in its correct position in a sorted vector.
 * returns 1 if insertion is successful, 0 if element is already
 * present or -1 on error. If out is not NULL it will be
 * dereferenced and set to the inserted or present element.
 **/
int vec_insert_uniq(struct vec *p, cb_cmp * f, const void *in, void **out);
void vec_sort(struct vec *p, cb_cmp * f);

/**
 * Gets a restarting iterator for the given vector.
 **/
void vec_get_iterator(const struct vec *p, struct vec_iterator *i);

/**
 * Gets a pointer to the next item from a resterting iterator. Returns
 * NULL when all items has been enumerated. Will restart from the
 * beginning if called again after returning NULL.
 **/
void *vec_iterator_next(struct vec_iterator *i);

int vec_equals(const struct vec *a, const struct vec *b, cb_cmp *equals);
void vec_fprint(FILE *, const struct vec *a, cb_fprint *fprint);

#ifdef __cplusplus
}
#endif
#endif

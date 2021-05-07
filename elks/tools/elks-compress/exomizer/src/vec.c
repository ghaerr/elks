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

#include "vec.h"
#include <stdlib.h>

#define VEC_FLAG_SORTED 1

void vec_init(struct vec *p, size_t elsize)
{
    p->elsize = elsize;
    buf_init(&p->buf);
    p->flags = VEC_FLAG_SORTED;
}

void vec_clear(struct vec *p, cb_free * f)
{
    struct vec_iterator i;
    const void *d;

    vec_get_iterator(p, &i);

    if (f != NULL)
    {
        while ((d = vec_iterator_next(&i)) != NULL)
        {
            f((void*)d);
        }
    }
    buf_clear(&p->buf);
    p->flags = VEC_FLAG_SORTED;

}

void vec_free(struct vec *p, cb_free * f)
{
    vec_clear(p, f);
    buf_free(&p->buf);
}

int vec_size(const struct vec *p)
{
    int size;
    size = buf_size(&p->buf) / p->elsize;
    return size;
}

void *vec_get(const struct vec *p, int index)
{
    char *buf = NULL;

    if(index >= 0 && index < vec_size(p))
    {
        buf = (char *) buf_data(&p->buf);
        buf += index * p->elsize;
    }

    return (void *)buf;
}

void *vec_set(struct vec *p, int index, const void *in)
{
    void *buf = NULL;

    if(index >= 0 && index < vec_size(p))
    {
        buf = buf_replace(&p->buf, index * p->elsize, p->elsize,
                          in, p->elsize);
    }

    return buf;
}

void *vec_insert(struct vec *p, int index, const void *in)
{
    char *buf = NULL;

    if(index >= 0 && index <= vec_size(p))
    {
        buf = buf_replace(&p->buf, index * p->elsize, 0, in, p->elsize);
    }
    return buf;
}

void vec_remove(struct vec *p, int index)
{
    if(index >= 0 && index < vec_size(p))
    {
        buf_replace(&p->buf, index * p->elsize, p->elsize, NULL, 0);
    }
}

void *vec_push(struct vec *p, const void *in)
{
    void *out;
    out = buf_append(&p->buf, in, p->elsize);
    p->flags &= ~VEC_FLAG_SORTED;

    return out;
}

int vec_find(const struct vec *p, cb_cmp * f, const void *in)
{
    int lo;

    lo = -1;
    if(p->flags & VEC_FLAG_SORTED)
    {
        int hi;
        lo = 0;
        hi = vec_size(p) - 1;
        while(lo <= hi)
        {
            int next;
            int val;

            next = (lo + hi) / 2;
            val = f(in, vec_get(p, next));
            if(val == 0)
            {
                /* match */
                lo = -(next + 2);
                break;
            }
            else if(val < 0)
            {
                hi = next - 1;
            }
            else
            {
                lo = next + 1;
            }
        }
    }
    return -(lo + 2);
}

void *vec_find2(const struct vec *p, cb_cmp * f, const void *key)
{
    void *out = NULL;
    int pos = vec_find(p, f, key);
    if(pos >= 0)
    {
        out = vec_get(p, pos);
    }
    return out;
}

int vec_insert_uniq(struct vec *p, cb_cmp * f, const void *in, void **outp)
{
    int val = 0;
    void *out;

    val = vec_find(p, f, in);
    if(val != -1)
    {
        if(val < 0)
        {
            /* not there */
            out = vec_insert(p, -(val + 2), in);
            val = 1;
        }
        else
        {
            out = vec_get(p, val);
            val = 0;
        }

        if(outp != NULL)
        {
            *outp = out;
        }
    }

    return val;
}

void vec_sort(struct vec *p, cb_cmp * f)
{
    qsort(buf_data(&p->buf), vec_size(p), p->elsize, f);
    p->flags |= VEC_FLAG_SORTED;
}


void vec_get_iterator(const struct vec *p, struct vec_iterator *i)
{
    i->vec = p;
    i->pos = 0;
}

void *vec_iterator_next(struct vec_iterator *i)
{
    void *out;
    int size = vec_size(i->vec);
    if (i->pos >= size)
    {
        i->pos = 0;
        return NULL;
    }
    out = vec_get(i->vec, i->pos);
    i->pos += 1;
    return out;
}

void vec_fprint(FILE *f, const struct vec *a, cb_fprint *fprint)
{
    struct vec_iterator i;
    int *e;
    char *glue = "[";

    vec_get_iterator(a, &i);
    while((e = vec_iterator_next(&i)) != NULL)
    {
        fprintf(f, "%s", glue);
        fprint(f, e);
        glue = ", ";
    }
    fprintf(f, "]");
}

int vec_equals(const struct vec *a, const struct vec *b, cb_cmp *cmp)
{
    struct vec_iterator ia;
    struct vec_iterator ib;
    void *ea;
    void *eb;
    int equal = 1;

    vec_get_iterator(a, &ia);
    vec_get_iterator(b, &ib);

    while(equal)
    {
        ea = vec_iterator_next(&ia);
        eb = vec_iterator_next(&ib);

        if(ea == NULL && eb == NULL)
        {
            break;
        }

        if((ea == NULL) ^ (eb == NULL))
        {
            equal = 0;
            break;
        }
        equal = !cmp(ea, eb);
    }
    return equal;
}

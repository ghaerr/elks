#ifndef ALREADY_INCLUDED_RADIX
#define ALREADY_INCLUDED_RADIX
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 2002, 2003 Magnus Lind.
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

struct radix_root {
    int depth;
    struct radix_node *root;
    struct chunkpool mem;
};

typedef void free_callback(void *data, void *priv);

/* *f will be called even for null pointers */
void radix_tree_free(struct radix_root *rr,     /* IN */
                     free_callback * f, /* IN */
                     void *priv);       /* IN */

void radix_tree_init(struct radix_root *rr);    /* IN */

void radix_node_set(struct radix_root *rr,      /* IN */
                    unsigned int index, /* IN */
                    void *data);        /* IN */

void *radix_node_get(struct radix_root *rr,     /* IN */
                     unsigned int index);       /* IN */

#ifdef __cplusplus
}
#endif
#endif

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "radix.h"
#include "chunkpool.h"

#define RADIX_TREE_NODE_RADIX 8U
#define RADIX_TREE_NODE_MASK  ((1U << RADIX_TREE_NODE_RADIX) - 1U)

struct radix_node {
    struct radix_node *rn;
};

void radix_tree_init(struct radix_root *rr)     /* IN */
{
    rr->depth = 0;
    rr->root = NULL;

    chunkpool_init(&rr->mem, (1 << RADIX_TREE_NODE_RADIX) * sizeof(void *));
}

static
void radix_tree_free_helper(int depth, struct radix_node *rnp,
                            free_callback * f,      /* IN */
                            void *priv) /* IN */
{
    int i;
    do
    {
        if (depth == 0)
        {
            /* do something to the data pointer? */
            if (f != NULL)
            {
                f(rnp, priv);
            }
            break;
        }
        if (rnp == NULL)
        {
            /* tree not grown here */
            break;
        }

        for (i = RADIX_TREE_NODE_MASK; i >= 0; --i)
        {
            radix_tree_free_helper(depth - 1, rnp[i].rn, f, priv);
            rnp[i].rn = NULL;
        }
    }
    while (0);
}

void radix_tree_free(struct radix_root *rr,     /* IN */
                     free_callback * f, /* IN */
                     void *priv)        /* IN */
{
    radix_tree_free_helper(rr->depth, rr->root, f, priv);
    rr->depth = 0;
    rr->root = NULL;
    chunkpool_free(&rr->mem);
}

void radix_node_set(struct radix_root *rrp,    /* IN */
                    unsigned int index, /* IN */
                    void *data) /* IN */
{
    struct radix_node *rnp;
    struct radix_node **rnpp;
    unsigned int mask;
    int depth;

    mask = ~0U << (RADIX_TREE_NODE_RADIX * rrp->depth);
    while (index & mask)
    {
        /*LOG(LOG_DUMP, ("calloc called\n")); */
        /* not deep enough, let's deepen the tree */
	rnp = chunkpool_calloc(&rrp->mem);

        rnp[0].rn = rrp->root;
        rrp->root = rnp;
        rrp->depth += 1;

        mask = ~0U << (RADIX_TREE_NODE_RADIX * rrp->depth);
    }

    /* go down */
    rnpp = &rrp->root;
    for (depth = rrp->depth - 1; depth >= 0; --depth)
    {
        unsigned int node_index;

        if (*rnpp == NULL)
        {
            /*LOG(LOG_DUMP, ("calloc called\n")); */
            /* tree is not grown in this interval */
	    *rnpp = chunkpool_calloc(&rrp->mem);
        }
        node_index = ((index >> (RADIX_TREE_NODE_RADIX * depth)) &
                      RADIX_TREE_NODE_MASK);

        rnpp = &((*rnpp)[node_index].rn);
    }
    *rnpp = data;
}

void *radix_node_get(struct radix_root *rr,     /* IN */
                     unsigned int index)        /* IN */
{
    struct radix_node *rnp;
    unsigned short int depth;

    /* go down */
    rnp = rr->root;
    for (depth = rr->depth - 1; depth < 0xffff; --depth)
    {
        unsigned short int node_index;

        if (rnp == NULL)
        {
            /* tree is not grown in this interval */
            break;
        }
        node_index = ((index >> (RADIX_TREE_NODE_RADIX * depth)) &
                      RADIX_TREE_NODE_MASK);

        rnp = rnp[node_index].rn;
    }
    return rnp;
}

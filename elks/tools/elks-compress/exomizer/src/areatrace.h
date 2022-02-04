#ifndef ALREADY_INCLUDED_AREATRACE
#define ALREADY_INCLUDED_AREATRACE
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 2018 Magnus Lind.
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

#include "int.h"
#include "vec.h"

struct areatrace
{
    struct vec areas;
};

void areatrace_init(struct areatrace *at);

void areatrace_free(struct areatrace *at);

/*
 * Updates the given area trace with the given memory access.
 */
void areatrace_access(struct areatrace *at /* IN/OUT */,
                      u16 address /* IN */);

/*
 * Merges overlapping areas in the given area trace.
 * Areas can have one byte overlap when it is still undecided which of the
 * areas that is growing/shrinking by the access pattern.
 */
void areatrace_merge_overlapping(struct areatrace *at /* IN/OUT */);

/*
 * Gets the largest area from the given area trace.
 */
void areatrace_get_largest(const struct areatrace *at, /* IN */
                           int *startp /* OUT */,
                           int *endp); /* OUT */

#ifdef __cplusplus
}
#endif
#endif

#ifndef NAMED_BUFFER_INCLUDED
#define NAMED_BUFFER_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

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

#include "buf.h"
#include "chunkpool.h"
#include "map.h"

struct named_buffer {
    struct map map;
    struct chunkpool buf;
};

void named_buffer_init(struct named_buffer *nb);
void named_buffer_free(struct named_buffer *nb);
void named_buffer_clear(struct named_buffer *nb);

void named_buffer_copy(struct named_buffer *nb,
                       const struct named_buffer *source);

struct buf *new_named_buffer(struct named_buffer *nb, const char *name);
struct buf *get_named_buffer(struct named_buffer *nb, const char *name);

#ifdef __cplusplus
}
#endif
#endif

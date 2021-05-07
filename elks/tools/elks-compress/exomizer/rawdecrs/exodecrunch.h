#ifndef EXO_DECRUNCH_ALREADY_INCLUDED
#define EXO_DECRUNCH_ALREADY_INCLUDED

/*
 * Copyright (c) 2005 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented * you must not
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
 */

/**
 * This decompressor decompresses files that have been compressed
 * using the raw sub-sub command with the -P39 (default) setting of
 * the raw command.
 */
#define MAX_OFFSET 65535

typedef int exo_read_crunched_byte(void *data);

struct exo_decrunch_ctx *
exo_decrunch_new(unsigned short int max_offset,
                 exo_read_crunched_byte *read_byte,
                 void *read_data);

int
exo_read_decrunched_byte(struct exo_decrunch_ctx *ctx);

void
exo_decrunch_delete(struct exo_decrunch_ctx *ctx);

#endif /* EXO_DECRUNCH_ALREADY_INCLUDED */

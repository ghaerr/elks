#ifndef ALREADY_INCLUDED_OPTIMAL
#define ALREADY_INCLUDED_OPTIMAL
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 2002 - 2005 Magnus Lind.
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

#include "search.h"
#include "output.h"

float optimal_encode(const struct match *mp,   /* IN */
                     struct encode_match_data *emp,     /* IN */
                     unsigned int prev_offset, /* IN */
                     struct encode_match_buckets *embp);/* OUT */

void optimal_init(struct encode_match_data *emp,        /* IN/OUT */
                  int flags_notrait,    /* IN */
                  int flags_proto);     /* IN */

void optimal_free(struct encode_match_data *emd);       /* IN */

void optimal_optimize(struct encode_match_data *emd,    /* IN/OUT */
                      match_enum_next_f *enum_next_f,   /* IN */
                      void *priv);      /* IN */

void optimal_encoding_import(struct encode_match_data *emd,     /* IN/OUT */
                             const char *encoding);     /* IN */

void optimal_encoding_export(struct encode_match_data *emd,     /* IN */
                             struct buf *export);    /* OUT */

void optimal_dump(int level, struct encode_match_data *emp);       /* IN */

void optimal_out(struct output_ctx *out,        /* IN/OUT */
                 struct encode_match_data *emd);        /* IN */

#ifdef __cplusplus
}
#endif
#endif

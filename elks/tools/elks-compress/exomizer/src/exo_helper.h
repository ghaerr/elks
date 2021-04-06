#ifndef EXO_HELPER_ALREADY_INCLUDED
#define EXO_HELPER_ALREADY_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Copyright (c) 2005, 2013, 2015 Magnus Lind.
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

#include "log.h"
#include "buf.h"
#include "search.h"
#include "optimal.h"
#include "flags.h"

#define DECRUNCH_OPTIONS_DEFAULT {NULL, PFLAG_BITS_ORDER_BE |    \
                                  PFLAG_BITS_COPY_GT_7 | \
                                  PFLAG_IMPL_1LITERAL |  \
                                  PFLAG_REUSE_OFFSET,    \
                                  1, 0}

#define CRUNCH_OPTIONS_DEFAULT {NULL, 100, 65535, 65535, 0, 1, \
                                PFLAG_BITS_ORDER_BE |            \
                                PFLAG_BITS_COPY_GT_7 |           \
                                PFLAG_IMPL_1LITERAL |            \
                                PFLAG_REUSE_OFFSET,              \
                                0, 0, 0, NULL}

struct common_flags
{
    struct crunch_options *options;
    const char *outfile;
};

#define STATIC_CRUNCH_INFO_INIT {0, 0, 0}
struct crunch_info
{
    int traits_used;
    int max_len;
    int max_offset;
    int needed_safety_offset;
};

#define CRUNCH_FLAGS "cCe:Em:M:p:P:T:o:N:qBv"
#define BASE_FLAGS "o:qBv"

void print_crunch_flags(enum log_level level, const char *default_outfile);

void print_base_flags(enum log_level level, const char *default_outfile);

void print_crunch_info(enum log_level level, struct crunch_info *info);

typedef void print_usage_f(const char *appl, enum log_level level,
                           const char *default_outfile);

void handle_crunch_flags(int flag_char, /* IN */
                         const char *flag_arg, /* IN */
                         print_usage_f *print_usage, /* IN */
                         const char *appl, /* IN */
                         struct common_flags *options); /* OUT */

void handle_base_flags(int flag_char, /* IN */
                       const char *flag_arg, /* IN */
                       print_usage_f *print_usage, /* IN */
                       const char *appl, /* IN */
                       const char **default_outfilep); /* OUT */

struct crunch_options
{
    const char *imported_encoding;
    int max_passes;
    int max_len;
    int max_offset;
    int favor_speed;
    int output_header;
    int flags_proto;
    int flags_notrait;
    /* 0 backward, 1 forward */
    int direction_forward;
    int write_reverse;
    const char *noread_filename;
};

void print_license(void);

struct io_bufs
{
    struct buf in;
    int in_off;
    struct buf out;
    struct crunch_info info;
};

void crunch_multi(struct vec *io_bufs,
                  struct buf *noread_in,
                  struct buf *enc_buf,
                  const struct crunch_options *options, /* IN */
                  struct crunch_info *merged_info); /* OUT */

void crunch(struct buf *inbuf,
            int in_off,
            struct buf *noread_inbuf,
            struct buf *outbuf,
            const struct crunch_options *options, /* IN */
            struct crunch_info *info); /* OUT */

struct decrunch_options
{
    const char *imported_encoding;
    /* see crunch_options flags field */
    int flags_proto;
    /* 0 backward, 1 forward */
    int direction_forward;
    int write_reverse;
};

void decrunch(int level,
              struct buf *inbuf,
              int in_off,
              struct buf *outbuf,
              struct decrunch_options *dopts);

void reverse_buffer(char *start, int len);

#ifdef __cplusplus
}
#endif
#endif

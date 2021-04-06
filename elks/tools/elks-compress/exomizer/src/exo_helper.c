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
#include "output.h"
#include "buf.h"
#include "buf_io.h"
#include "match.h"
#include "search.h"
#include "optimal.h"
#include "exodec.h"
#include "exo_helper.h"
#include "exo_util.h"
#include "getflag.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const struct crunch_options default_options = CRUNCH_OPTIONS_DEFAULT;

void do_output_backwards(struct match_ctx *ctx,
                         struct search_node *snp,
                         struct encode_match_data *emd,
                         const struct crunch_options *options,
                         struct buf *outbuf,
                         struct crunch_info *infop)
{
    int pos;
    int pos_diff;
    int max_diff;
    int diff;
    int traits_used = 0;
    int max_len = 0;
    int max_offset = 0;
    struct output_ctx *old;
    struct output_ctx out;
    struct search_node *initial_snp;
    int initial_len;
    int alignment = 0;
    int measure_alignment;
    int len0123skip = 3;

    old = emd->out;
    emd->out = &out;

    initial_len = buf_size(outbuf);
    initial_snp = snp;
    measure_alignment = options->flags_proto & PFLAG_BITS_ALIGN_START;
    if (options->flags_proto & PFLAG_4_OFFSET_TABLES)
    {
         len0123skip = 4;
    }
    for (;;)
    {
        buf_remove(outbuf, initial_len, -1);
        snp = initial_snp;
        output_ctx_init(&out, options->flags_proto, outbuf);

        output_bits(&out, alignment, 0);

        pos = output_get_pos(&out);

        pos_diff = pos;
        max_diff = 0;

        if (snp != NULL)
        {
            LOG(LOG_DUMP, ("pos $%04X\n", out.pos));

            output_gamma_code(&out, 16);
            output_bits(&out, 1, 0); /* 1 bit out */

            diff = output_get_pos(&out) - pos_diff;
            if(diff > max_diff)
            {
                max_diff = diff;
            }

            LOG(LOG_DUMP, ("pos $%04X\n", out.pos));
            LOG(LOG_DUMP, ("------------\n"));
        }
        while (snp != NULL)
        {
            const struct match *mp;

            mp = &snp->match;
            if (mp != NULL && mp->len > 0)
            {
                if (mp->offset == 0)
                {
                    int splitLitSeq =
                        snp->prev->match.len == 0 &&
                        (options->flags_proto & PFLAG_IMPL_1LITERAL);
                    int i = 0;
                    if (mp->len > 1)
                    {
                        int len = mp->len;
                        if (splitLitSeq)
                        {
                            --len;
                        }
                        for(; i < len; ++i)
                        {
                            output_byte(&out, ctx->buf[snp->index + i]);
                        }
                        output_bits(&out, 16, len);
                        output_gamma_code(&out, 17);
                        output_bits(&out, 1, 0);
                        /* literal sequence */
                        LOG(LOG_DUMP, ("[%d] literal copy len %d\n", out.pos,
                                       len));
                        traits_used |= TFLAG_LIT_SEQ;
                        if (len > max_len)
                        {
                            max_len = len;
                        }
                    }
                    if (i < mp->len)
                    {
                        /* literal */
                        LOG(LOG_DUMP, ("[%d] literal $%02X\n", out.pos,
                                       ctx->buf[snp->index + i]));
                        output_byte(&out, ctx->buf[snp->index + i]);
                        if (!splitLitSeq)
                        {
                            output_bits(&out, 1, 1);
                        }
                    }
                } else
                {
                    unsigned int latest_offset = snp->prev->latest_offset;
                    if (latest_offset > 0)
                    {
                        LOG(LOG_DUMP,
                            ("[%d] offset reuse bit = %d, latest = %d\n",
                             out.pos, mp->offset == latest_offset,
                             latest_offset));
                    }
                    LOG(LOG_DUMP, ("[%d] sequence offset = %d, len = %d\n",
                                   out.pos, mp->offset, mp->len));
                    optimal_encode(mp, emd, latest_offset, NULL);
                    output_bits(&out, 1, 0);
                    if (mp->len == 1)
                    {
                        traits_used |= TFLAG_LEN1_SEQ;
                    }
                    else
                    {
                        int lo = mp->len & 255;
                        int hi = mp->len & ~255;
                        if (hi > 0 && lo < len0123skip)
                        {
                            traits_used |= TFLAG_LEN0123_SEQ_MIRRORS;
                        }
                    }
                    if (mp->offset > max_offset)
                    {
                        max_offset = mp->offset;
                    }
                    if (mp->len > max_len)
                    {
                        max_len = mp->len;
                    }
                }

                pos_diff += mp->len;
                diff = output_get_pos(&out) - pos_diff;
                if(diff > max_diff)
                {
                    max_diff = diff;
                }
            }
            LOG(LOG_DUMP, ("------------\n"));
            snp = snp->prev;
        }

        LOG(LOG_DUMP, ("pos $%04X\n", out.pos));
        if (options->output_header)
        {
            /* output header here */
            optimal_out(&out, emd);
            LOG(LOG_DUMP, ("pos $%04X\n", out.pos));
        }

        if (!measure_alignment)
        {
            break;
        }
        alignment = output_bits_alignment(&out);
        measure_alignment = 0;
    }
    output_bits_flush(&out, !(options->flags_proto & PFLAG_BITS_ALIGN_START));

    emd->out = old;

    if(infop != NULL)
    {
        infop->traits_used = traits_used;
        infop->max_len = max_len;
        infop->max_offset = max_offset;
        infop->needed_safety_offset = max_diff;
    }
}

/**
 * Reads an exported encoding into the given enc_buf for access in
 * forward direction. If read from file the file is read forward but
 * reversed if needs_reversing != 0.
 */
static void read_encoding_to_buf(const char *imported_enc,
                                 int flags_proto,
                                 int needs_reversing,
                                 struct buf *enc_buf)
{
    if (imported_enc[0] == '@')
    {
        ++imported_enc;
        read_file(imported_enc, enc_buf);
    }
    else
    {
        struct encode_match_data emd;
        struct crunch_options options = default_options;

        options.flags_proto = flags_proto;
        options.output_header = 1;
        options.imported_encoding = imported_enc;
        emd.out = NULL;

        optimal_init(&emd, options.flags_notrait, options.flags_proto);
        optimal_encoding_import(&emd, options.imported_encoding);
        do_output_backwards(NULL, NULL, &emd, &options, enc_buf, NULL);
        /* enc_buf is in backward direction */
        optimal_free(&emd);
        needs_reversing = 1;
    }

    if (needs_reversing)
    {
        reverse_buffer(buf_data(enc_buf), buf_size(enc_buf));
    }
}

static void read_encoding_to_emd(struct encode_match_data *emd,
                                 const struct crunch_options *options)
{
    struct buf enc_buf = STATIC_BUF_INIT;
    const char *imported_enc = options->imported_encoding;

    if (imported_enc[0] == '@')
    {
        struct dec_ctx ctx;
        read_encoding_to_buf(imported_enc, options->flags_proto,
                             !options->direction_forward ^
                             (options->write_reverse != 0), &enc_buf);
        /* enc_buf is in direction forward which is expected by dec_ctx */
        dec_ctx_init(&ctx, &enc_buf, &enc_buf, NULL, options->flags_proto);

        buf_clear(&enc_buf);
        dec_ctx_table_dump(&ctx, &enc_buf);
        dec_ctx_free(&ctx);

        imported_enc = buf_data(&enc_buf);
    }

    LOG(LOG_NORMAL, (" Using imported encoding\n Enc: %s\n", imported_enc));
    optimal_encoding_import(emd, imported_enc);

    buf_free(&enc_buf);
}

static struct search_node**
do_compress_backwards(struct match_ctx *ctxp, int ctx_count,
                      struct encode_match_data *emd,
                      const struct crunch_options *options,
                      struct buf *enc) /* IN */
{
    struct vec snpev = STATIC_VEC_INIT(sizeof(struct match_snp_enum));
    struct match_concat_enum mpcce;
    struct search_node **snpp;
    int pass, i, last_waltz = 0;
    float size;
    float old_size;
    char prev_enc[100];

    snpp = calloc(ctx_count, sizeof(struct search_node *));

    pass = 1;
    prev_enc[0] = '\0';

    LOG(LOG_NORMAL, (" pass %d: ", pass));
    if(options->imported_encoding != NULL)
    {
        read_encoding_to_emd(emd, options);
        if (options->max_passes == 1)
        {
            /* only one pass, use the greedy variant of search_buffer */
            ++pass;
        }
    }
    else
    {
        struct vec mpcev = STATIC_VEC_INIT(sizeof(struct match_cache_enum));
        LOG(LOG_NORMAL, ("optimizing ..\n"));
        for (i = 0; i < ctx_count; ++i)
        {
            struct match_cache_enum *mp_enum = vec_push(&mpcev, NULL);
            match_cache_get_enum(ctxp + i, mp_enum);
        }
        match_concat_get_enum(match_cache_enum_get_next, &mpcev, &mpcce);
        optimal_optimize(emd, match_concat_enum_get_next, &mpcce);
        vec_free(&mpcev, NULL);
    }
    optimal_encoding_export(emd, enc);
    strcpy(prev_enc, buf_data(enc));

    old_size = 100000000.0;

    for (;;)
    {
    last_waltz:
        size = 0.0;
        for (i = 0; i < ctx_count; ++i)
        {
            if (snpp[i] != NULL)
            {
                free(snpp[i]);
                snpp[i] = NULL;
            }

            search_buffer(ctxp + i, optimal_encode, emd,
                          options->flags_proto,
                          options->flags_notrait,
                          options->max_len,
                          !(pass & 1), &snpp[i]);
            if (snpp[i] == NULL)
            {
                LOG(LOG_ERROR, ("error: search_buffer() returned NULL\n"));
                exit(1);
            }
            size += snpp[i]->total_score;
        }
        LOG(LOG_NORMAL, ("  size %0.1f bits ~%d bytes\n",
                         size, (((int) size) + 7) >> 3));
        if (last_waltz)
        {
            break;
        }

        ++pass;
        if (size >= old_size)
        {
            last_waltz = 1;
            goto last_waltz;
        }
        old_size = size;

        if(pass > options->max_passes)
        {
            break;
        }

        optimal_free(emd);
        optimal_init(emd, options->flags_notrait, options->flags_proto);

        LOG(LOG_NORMAL, (" pass %d: optimizing ..\n", pass));

        for (i = 0; i < ctx_count; ++i)
        {
            struct match_snp_enum *mp_enum = vec_push(&snpev, NULL);
            match_snp_get_enum(snpp[i], mp_enum);
        }
        match_concat_get_enum(match_snp_enum_get_next, &snpev, &mpcce);
        optimal_optimize(emd, match_concat_enum_get_next, &mpcce);
        vec_clear(&snpev, NULL);

        optimal_encoding_export(emd, enc);
        if (strcmp(buf_data(enc), prev_enc) == 0)
        {
            break;
        }
        strcpy(prev_enc, buf_data(enc));
    }

    vec_free(&snpev, NULL);

    return snpp;
}

void crunch_multi(struct vec *io_bufs,
                  struct buf *noread_in,
                  struct buf *enc_buf,
                  const struct crunch_options *options, /* IN */
                  struct crunch_info *infop) /* OUT */
{
    struct match_ctx *ctxp;
    struct encode_match_data emd;
    struct search_node **snpp;
    struct crunch_info merged_info = STATIC_CRUNCH_INFO_INIT;
    struct buf exported_enc = STATIC_BUF_INIT;
    int buf_count = vec_size(io_bufs);
    int outlen = 0;
    int inlen = 0;
    int i;
    int *outpos;

    outpos = malloc(sizeof(int) * buf_count);

    ctxp = malloc(sizeof(struct match_ctx) * buf_count);
    if(options == NULL)
    {
        options = &default_options;
    }

    LOG(LOG_NORMAL,
        ("\nPhase 1: Preprocessing file%s"
         "\n---------------------------%s\n",
         (buf_count == 1 ? "" : "s"), (buf_count == 1 ? "" : "-")));
    for (i = 0; i < buf_count; ++i)
    {
        struct buf nor_view;
        struct io_bufs *io = vec_get(io_bufs, i);
        struct buf *in = &io->in;
        const struct buf *nor = NULL;

        if (noread_in != NULL)
        {
            int growth_req = buf_size(in) + io->in_off - buf_size(noread_in);
            if (growth_req > 0)
            {
                memset(buf_append(noread_in, NULL, growth_req), 0, growth_req);
            }
            nor = buf_view(&nor_view, noread_in, io->in_off, buf_size(in));
        }

        if (options->direction_forward == 1)
        {
            struct buf *out = &io->out;
            reverse_buffer(buf_data(in), buf_size(in));
            outpos[i] = buf_size(out);
        }
        inlen += buf_size(in);
        match_ctx_init(ctxp + i, in, nor, options->max_len,
                       options->max_offset, options->favor_speed);
    }
    LOG(LOG_NORMAL, (" Length of indata: %d bytes.\n", inlen));
    LOG(LOG_NORMAL, (" Preprocessing file%s, done.\n",
                     (buf_count == 1 ? "" : "s")));

    emd.out = NULL;
    optimal_init(&emd, options->flags_notrait, options->flags_proto);

    LOG(LOG_NORMAL,
        ("\nPhase 2: Calculating encoding"
         "\n-----------------------------\n"));
    snpp = do_compress_backwards(ctxp, buf_count, &emd, options, &exported_enc);

    LOG(LOG_NORMAL, (" Calculating encoding, done.\n"));

    LOG(LOG_NORMAL,
        ("\nPhase 3: Generating output file%s"
         "\n-------------------------------%s\n",
         (buf_count == 1 ? "" : "s"), (buf_count == 1 ? "" : "-")));
    LOG(LOG_NORMAL, (" Enc: %s\n", (char*)buf_data(&exported_enc)));

    if (enc_buf != NULL)
    {
        struct crunch_options enc_opts = *options;
        enc_opts.output_header = 1;
        do_output_backwards(NULL, NULL, &emd, &enc_opts, enc_buf, NULL);

        if (options->direction_forward == 1)
        {
            reverse_buffer(buf_data(enc_buf), buf_size(enc_buf));
        }
    }

    for (i = 0; i < buf_count; ++i)
    {
        struct io_bufs *io = vec_get(io_bufs, i);
        const struct buf *in = &io->in;
        struct buf *out = &io->out;
        struct crunch_info *info = &io->info;

        outlen -= buf_size(out);
        do_output_backwards(ctxp + i, snpp[i], &emd, options, out, info);
        outlen += buf_size(out);

        if (options->direction_forward == 1)
        {
            reverse_buffer(buf_data(in), buf_size(in));
            reverse_buffer((char*)buf_data(out) + outpos[i],
                           buf_size(out) - outpos[i]);
        }

        merged_info.traits_used |= info->traits_used;
        if (merged_info.max_len < info->max_len)
        {
            merged_info.max_len = info->max_len;
        }
        if (merged_info.max_offset < info->max_offset)
        {
            merged_info.max_offset = info->max_offset;
        }
        if (merged_info.needed_safety_offset < info->needed_safety_offset)
        {
            merged_info.needed_safety_offset = info->needed_safety_offset;
        }
    }
    LOG(LOG_NORMAL, (" Length of crunched data: %d bytes.\n", outlen));
    LOG(LOG_BRIEF, (" Crunched data reduced %d bytes (%0.2f%%)\n",
                    inlen - outlen, 100.0 * (inlen - outlen) / inlen));

    optimal_free(&emd);
    for (i = 0; i < buf_count; ++i)
    {
        free(snpp[i]);
        match_ctx_free(ctxp + i);
    }
    free(snpp);
    free(ctxp);
    buf_free(&exported_enc);
    free(outpos);

    if(infop != NULL)
    {
        *infop = merged_info;
    }
}

void reverse_buffer(char *start, int len)
{
    char *end = start + len - 1;
    char tmp;

    while (start < end)
    {
        tmp = *start;
        *start = *end;
        *end = tmp;

        ++start;
        --end;
    }
}

void crunch(struct buf *inbuf,
            int in_off,
            struct buf *noread_inbuf,
            struct buf *outbuf,
            const struct crunch_options *options, /* IN */
            struct crunch_info *info) /* OUT */
{
    struct vec io_bufs = STATIC_VEC_INIT(sizeof(struct io_bufs));

    struct io_bufs *io = vec_push(&io_bufs, NULL);
    io->in = *inbuf;
    io->in_off = in_off;
    io->out = *outbuf;
    crunch_multi(&io_bufs, noread_inbuf, NULL, options, info);
    *inbuf = io->in;
    *outbuf = io->out;

    vec_free(&io_bufs, NULL);
}

void decrunch(int level,
              struct buf *inbuf,
              int in_off,
              struct buf *outbuf,
              struct decrunch_options *dopts)
{
    struct dec_ctx ctx;
    struct buf enc_buf = STATIC_BUF_INIT;
    struct buf *encp;
    int outpos;

    if (dopts->direction_forward == 0)
    {
        reverse_buffer(buf_data(inbuf), buf_size(inbuf));
    }
    outpos = buf_size(outbuf);

    encp = NULL;
    if(dopts->imported_encoding != NULL)
    {
        read_encoding_to_buf(dopts->imported_encoding, dopts->flags_proto,
                             !dopts->direction_forward ^
                             (dopts->write_reverse != 0), &enc_buf);
        /* enc_buf is in direction forward which is expected by dec_ctx */
        encp = &enc_buf;
    }

    dec_ctx_init(&ctx, encp, inbuf, outbuf, dopts->flags_proto);

    buf_clear(&enc_buf);
    dec_ctx_table_dump(&ctx, &enc_buf);
    LOG(level, (" Enc: %s\n", (char*)buf_data(&enc_buf)));

    buf_free(&enc_buf);
    dec_ctx_decrunch(&ctx);
    dec_ctx_free(&ctx);

    if (dopts->direction_forward == 0)
    {
        reverse_buffer(buf_data(inbuf), buf_size(inbuf));
        reverse_buffer((char*)buf_data(outbuf) + outpos,
                       buf_size(outbuf) - outpos);
    }
}

void print_license(void)
{
    LOG(LOG_WARNING,
        ("----------------------------------------------------------------------------\n"
         "Exomizer v3.1.0 Copyright (c) 2002-2020 Magnus Lind. (magli143@gmail.com)\n"
         "----------------------------------------------------------------------------\n"));
    LOG(LOG_WARNING,
        ("This software is provided 'as-is', without any express or implied warranty.\n"
         "In no event will the authors be held liable for any damages arising from\n"
         "the use of this software.\n"
         "Permission is granted to anyone to use this software, alter it and re-\n"
         "distribute it freely for any non-commercial, non-profit purpose subject to\n"
         "the following restrictions:\n\n"));
    LOG(LOG_WARNING,
        ("   1. The origin of this software must not be misrepresented; you must not\n"
         "   claim that you wrote the original software. If you use this software in a\n"
         "   product, an acknowledgment in the product documentation would be\n"
         "   appreciated but is not required.\n"
         "   2. Altered source versions must be plainly marked as such, and must not\n"
         "   be misrepresented as being the original software.\n"
         "   3. This notice may not be removed or altered from any distribution.\n"));
    LOG(LOG_WARNING,
        ("   4. The names of this software and/or it's copyright holders may not be\n"
         "   used to endorse or promote products derived from this software without\n"
         "   specific prior written permission.\n"
         "----------------------------------------------------------------------------\n"
         "The files processed and/or generated by using this software are not covered\n"
         "nor affected by this license in any way.\n"));
}

void print_base_flags(enum log_level level, const char *default_outfile)
{
    LOG(level,
        ("  -o <outfile>  sets the outfile name, default is \"%s\"\n",
         default_outfile));
    LOG(level,
        ("  -q            quiet mode, disables all display output\n"
         "  -B            brief mode, disables most display output\n"
         "  -v            displays version and the usage license\n"
         "  --            treats all following arguments as non-options\n"
         "  -?            displays this help screen\n"));
}

void print_crunch_flags(enum log_level level, const char *default_outfile)
{
    LOG(level,
        ("  -c            compatibility mode, disables the use of literal sequences\n"
         "  -C            favor compression speed over ratio\n"
         "  -e <encoding> uses the given encoding for crunching\n"
         "  -E            don't write the encoding to the outfile\n"));
    LOG(level,
        ("  -m <offset>   sets the maximum sequence offset, default is 65535\n"
         "  -M <length>   sets the maximum sequence length, default is 65535\n"
         "  -p <passes>   limits the number of optimization passes, default is 100\n"
         "  -T <options>  bitfield that controls bit stream traits. [0-7]\n"
         "  -P <options>  bitfield that controls bit stream format. [0-63]\n"
         "  -N <nr_file>  controls addresses that are not to be read.\n"));
    print_base_flags(level, default_outfile);
}

void print_crunch_info(enum log_level level, struct crunch_info *info)
{
        LOG(level, ("  Literal sequences are %sused",
                    info->traits_used & TFLAG_LIT_SEQ ? "" : "not "));
        LOG(level, (", length 1 sequences are %sused",
                    info->traits_used & TFLAG_LEN1_SEQ ? "" : "not "));
        LOG(level, (",\n  length 0123 mirrors are %sused",
                    info->traits_used & TFLAG_LEN0123_SEQ_MIRRORS ?
                    "" : "not "));
        LOG(level, (", max length used is %d", info->max_len));
        LOG(level, (",\n  max offset used is %d", info->max_offset));
        LOG(level, (" and the safety offset is %d.\n",
                    info->needed_safety_offset));
		/*printf("Safety offset %d\n", info->needed_safety_offset);*/
}

void handle_base_flags(int flag_char, /* IN */
                       const char *flag_arg, /* IN */
                       print_usage_f *print_usage, /* IN */
                       const char *appl, /* IN */
                       const char **default_outfilep) /* IN */
{
    switch(flag_char)
    {
    case 'o':
        *default_outfilep = flag_arg;
        break;
    case 'q':
        LOG_SET_LEVEL(LOG_WARNING);
        break;
    case 'B':
        LOG_SET_LEVEL(LOG_BRIEF);
        break;
    case 'v':
        print_license();
        exit(0);
    default:
        if (flagflag != '?')
        {
            LOG(LOG_ERROR,
                ("error, invalid option \"-%c\"", flagflag));
            if (flagarg != NULL)
            {
                LOG(LOG_ERROR, (" with argument \"%s\"", flagarg));
            }
            LOG(LOG_ERROR, ("\n"));
        }
        print_usage(appl, LOG_WARNING, *default_outfilep);
        exit(0);
    }
}

void handle_crunch_flags(int flag_char, /* IN */
                         const char *flag_arg, /* IN */
                         print_usage_f *print_usage, /* IN */
                         const char *appl, /* IN */
                         struct common_flags *flags) /* OUT */
{
    struct crunch_options *options = flags->options;
    switch(flag_char)
    {
    case 'c':
        options->flags_notrait |= TFLAG_LIT_SEQ;
        break;
    case 'C':
        options->favor_speed = 1;
        break;
    case 'e':
        options->imported_encoding = flag_arg;
        break;
    case 'E':
        options->output_header = 0;
        break;
    case 'm':
        if (str_to_int(flag_arg, &options->max_offset, NULL) != 0 ||
            options->max_offset < 0 || options->max_offset > 65535)
        {
            LOG(LOG_ERROR,
                ("Error: invalid offset for -m option, "
                 "must be in the range of [0 - 65535]\n"));
            print_usage(appl, LOG_NORMAL, flags->outfile);
            exit(1);
        }
        break;
    case 'M':
        if (str_to_int(flag_arg, &options->max_len, NULL) != 0 ||
            options->max_len < 0 || options->max_len >= 65536)
        {
            LOG(LOG_ERROR,
                ("Error: invalid offset for -n option, "
                 "must be in the range of [0 - 65535]\n"));
            print_usage(appl, LOG_NORMAL, flags->outfile);
            exit(1);
        }
        break;
    case 'p':
        if (str_to_int(flag_arg, &options->max_passes, NULL) != 0 ||
            options->max_passes < 1 || options->max_passes > 100)
        {
            LOG(LOG_ERROR,
                ("Error: invalid value for -p option, "
                 "must be in the range of [1 - 100]\n"));
            print_usage(appl, LOG_NORMAL, flags->outfile);
            exit(1);
        }
        break;
    case 'T':
        if (str_to_int(flag_arg, &options->flags_notrait, NULL) != 0 ||
            options->flags_notrait < 0 || options->flags_notrait > 7)
        {
            LOG(LOG_ERROR,
                ("Error: invalid value for -T option, "
                 "must be in the range of [0 - 7]\n"));
            print_usage(appl, LOG_NORMAL, flags->outfile);
            exit(1);
        }
        break;
    case 'P':
        do
        {
            int op = 0;
            int flags_proto;
            if (*flag_arg == '+')
            {
                op = 1;
                ++flag_arg;
            }
            else if (*flag_arg == '-')
            {
                op = 2;
                ++flag_arg;
            }
            if (str_to_int(flag_arg, &flags_proto, &flag_arg) != 0 ||
                flags_proto < 0 || flags_proto > 63)
            {
                LOG(LOG_ERROR,
                    ("Error: invalid value for -P option, "
                     "must be in the range of [0 - 63]\n"));
                print_usage(appl, LOG_NORMAL, flags->outfile);
                exit(1);
            }
            if (op == 1)
            {
                options->flags_proto |= flags_proto;
            }
            else if (op == 2)
            {
                options->flags_proto &= ~flags_proto;
            }
            else
            {
                options->flags_proto = flags_proto;
            }
        }
        while (*flag_arg != '\0');
        break;
    case 'N':
        options->noread_filename = flag_arg;
        break;
    default:
        handle_base_flags(flag_char, flag_arg, print_usage,
                          appl, &flags->outfile);
    }
}

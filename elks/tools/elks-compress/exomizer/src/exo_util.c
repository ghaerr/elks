/*
 * Copyright (c) 2008 - 2018 Magnus Lind.
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

#include "exo_util.h"
#include "log.h"
#include "vec.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define ATARI_XEX_MAGIC_LEN 2
#define ORIC_TAP_MAGIC_LEN 3
#define APPLESINGLE_MAGIC_LEN 8

static unsigned char atari_xex_magic[] = {0xff, 0xff};
static unsigned char oric_tap_magic[] = {0x16, 0x16, 0x16};
static unsigned char applesingle_magic[] = {0, 5, 0x16, 0, 0, 2, 0, 0};

#define OPEN_UNPROVIDED INT_MIN
#define OPEN_DEFAULT (INT_MIN + 1)
struct open_info
{
    int is_raw;
    int load_addr;
    int offset;
    int length;
};

int find_sys(const unsigned char *buf, int sys_token, int *stub_lenp)
{
    int outstart = -1;
    int state = 1;

    /* skip link and line number */
    int i = 4;
    /* exit loop at line end */
    while(i < 1000 && buf[i] != '\0')
    {
        unsigned char *sys_end;
        int c = buf[i];
        switch(state)
        {
            /* look for and consume sys token */
        case 1:
            if((sys_token == -1 &&
                (c == 0x9e /* cbm */ ||
                 c == 0x8c /* apple 2*/ ||
                 c == 0xbf /* oric 1*/)) ||
               c == sys_token)
            {
                state = 2;
            }
            break;
            /* skip spaces and left parenthesis, if any */
        case 2:
            if(strchr(" (", c) != NULL) break;
            state = 3;
            /* convert string number to int */
        case 3:
            outstart = strtol((char*)(buf + i), (void*)&sys_end, 10);
            if((buf + i) == sys_end)
            {
                /* we got nothing */
                outstart = -1;
            }
            state = 4;
            break;
        case 4:
            break;
        }
        ++i;
    }
    if (stub_lenp != NULL)
    {
        /* skip zero byte + next zero link word */
        *stub_lenp = i + 3;
    }

    LOG(LOG_DEBUG, ("state when leaving: %d.\n", state));
    return outstart;
}

static int get_byte(FILE *in)
{
    int byte = fgetc(in);
    if(byte == EOF)
    {
        LOG(LOG_ERROR, ("Error: unexpected end of file."));
        fclose(in);
        exit(1);
    }
    return byte;
}

static int get_le_word(FILE *in)
{
    int word = get_byte(in);
    word |= get_byte(in) << 8;
    return word;
}

static int get_be_word(FILE *in)
{
    int word = get_byte(in) << 8;
    word |= get_byte(in);
    return word;
}

static unsigned int get_be_dword(FILE *in)
{
    int word = get_byte(in) << 24;
    word |= get_byte(in) << 16;
    word |= get_byte(in) << 8;
    word |= get_byte(in);
    return word;
}

/**
 * Normalizes and validates given offsets and lengths and seeks to the
 * normalized offset. Offsets are relative current position of the in file
 * unless they are negative, then they are relative to eof of in.
 * after searching to the resulting offset the length is normalised.
 * if it is negative it is added to the remaining length of the file.
 */
static void seek_and_normalize_offset_and_len(FILE *in, struct open_info *info)
{
    int file_pos;
    int file_len;
    int offset;
    int remaining;
    int length;

    file_pos = ftell(in);
    /* get the real length of the file and validate the offset*/
    if(fseek(in, 0, SEEK_END))
    {
        LOG(LOG_ERROR, ("Error: can't seek to EOF.\n"));
        fclose(in);
        exit(1);
    }
    file_len = ftell(in);
    offset = info->offset;
    if (offset == OPEN_UNPROVIDED || offset == OPEN_DEFAULT)
    {
        offset = 0;
    }
    if (offset < 0)
    {
        offset += file_len;
    }
    else
    {
        offset += file_pos;
    }
    if (offset < file_pos || offset > file_len)
    {
        /* bad offset */
        LOG(LOG_ERROR, ("Error: offset %d (%d) out of range.\n",
                        info->offset, offset));
        fclose(in);
        exit(1);
    }
    if(fseek(in, offset, SEEK_SET))
    {
        LOG(LOG_ERROR, ("Error: seek to offset %d (%d) failed.\n",
                        info->offset, offset));
        fclose(in);
        exit(1);
    }
    remaining = file_len - offset;
    length = info->length;
    if (length == OPEN_UNPROVIDED || length == OPEN_DEFAULT)
    {
        length = remaining;;
    }
    if (length < 0)
    {
        length += remaining;;
    }
    if (length < 0 || length > remaining)
    {
        /* bad offset */
        LOG(LOG_ERROR, ("Error: length %d (%d) out of range.\n",
                        info->length, length));
        fclose(in);
        exit(1);
    }

    info->offset = offset;
    info->length = length;
}

/**
 * Opens the given file and sets the fields in the open_info according to what
 * is peovided by the file name suffix. If any of the fields load_addr, offset
 * or length is unprovided or defaulted by the file name suffix then it will be
 * set to OPEN_UNPROVIDED or OPEN_DEFAULT.
 * if the file name suffix indicates that it is a raw file (@<addr>) then the
 * field is_raw will be set to 1.
 */
static
FILE *
open_file(const char *name, struct open_info *open_info)
{
    FILE *in;
    int is_raw = 0;
    int tries;
    char *tries_arr[3];
    int load = OPEN_UNPROVIDED;
    int offset = OPEN_UNPROVIDED;
    int length = OPEN_UNPROVIDED;

    for (tries = 0;; ++tries)
    {
        char *load_str;
        char *at_str;

        in = fopen(name, "rb");
        if (in != NULL || is_raw == 1 || tries == 3)
        {
            /* We have succeded in opening the file.
             * There's no address suffix. */
            break;
        }

        /* hmm, let's see if the user is trying to relocate it */
        load_str = strrchr(name, ',');
        at_str = strrchr(name, '@');
        if(at_str != NULL && (load_str == NULL || at_str > load_str))
        {
            is_raw = 1;
            load_str = at_str;
        }

        if (load_str == NULL)
        {
            /* nope, */
            break;
        }

        *load_str = '\0';
        ++load_str;

        /* relocation was requested */
        tries_arr[tries] = load_str;
    }
    if (in == NULL)
    {
        LOG(LOG_ERROR,
            (" can't open file \"%s\" for input\n", name));
        exit(1);
    }

    if (--tries >= 0)
    {
        char *p = tries_arr[tries];
        load = OPEN_DEFAULT;
        if (p[0] != '\0' && str_to_int(p, &load, NULL) != 0)
        {
            /* we fail */
            LOG(LOG_ERROR, (" can't parse load address from \"%s\"\n", p));
            exit(1);
        }
    }
    if (--tries >= 0)
    {
        char *p = tries_arr[tries];
        offset = OPEN_DEFAULT;
        if (p[0] != '\0' && str_to_int(p, &offset, NULL) != 0)
        {
            /* we fail */
            LOG(LOG_ERROR, (" can't parse offset from \"%s\"\n", p));
            exit(1);
        }
    }
    if (--tries >= 0)
    {
        char *p = tries_arr[tries];
        length = OPEN_DEFAULT;
        if (p[0] != '\0')
        {
            if (str_to_int(p, &length, NULL) != 0)
            {
                /* we fail */
                LOG(LOG_ERROR, (" can't parse length from \"%s\"\n", p));
                exit(1);
            }
        }
    }

    if (open_info != NULL)
    {
        open_info->is_raw = is_raw;
        open_info->load_addr = load;
        open_info->offset = offset;
        open_info->length = length;
    }
    return in;
}

static int is_matching_header(unsigned char *buf1,
                              unsigned char *buf2,
                              int len)
{
    int i;
    for (i = 0; i < len; ++i)
    {
        if (buf1[i] != buf2[i])
        {
            return 0;
        }
    }
    return 1;
}

/**
 * Detects the file type by reading a few bytes at the start and then
 * check for byte patterns. Won't detect RAW, default to PRG.
 */
static enum file_type detect_type(FILE *in)
{
    /* read the beginning of the file */
    unsigned char buf[8] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
    /* Default to PRG */
    enum file_type type = PRG;

    if (fread(buf, 1, 8, in) != 8 &&
        ferror(in))
    {
        LOG(LOG_ERROR, ("Error: failed to read from file.\n"));
        fclose(in);
        exit(1);
    }
    if(fseek(in, 0, SEEK_SET))
    {
        LOG(LOG_ERROR, ("Error: can't seek to file start.\n"));
        fclose(in);
        exit(1);
    }

    if(is_matching_header(buf, applesingle_magic, APPLESINGLE_MAGIC_LEN))
    {
        type = APPLESINGLE;
    }
    else if(is_matching_header(buf, atari_xex_magic, ATARI_XEX_MAGIC_LEN))
    {
        type = ATARI_XEX;
    }
    else if(is_matching_header(buf, oric_tap_magic, ORIC_TAP_MAGIC_LEN))
    {
        type = ORIC_TAP;
    }
    return type;
}

/* Handles xex files */
static void load_xex(unsigned char mem[65536], FILE *in,
                     struct load_info *info)
{
    int run = -1;
    int jsr = -1;
    int min = 65536, max = 0;
    unsigned char buf[ATARI_XEX_MAGIC_LEN];

    if (fread(buf, 1, ATARI_XEX_MAGIC_LEN, in) != ATARI_XEX_MAGIC_LEN)
    {
        LOG(LOG_ERROR,
            ("Error: failed to read Atari XEX header from file.\n"));
        fclose(in);
        exit(1);
    }
    if (!is_matching_header(buf, atari_xex_magic, ATARI_XEX_MAGIC_LEN))
    {
        LOG(LOG_ERROR, ("Error: not a valid Atari xex-header."));
        fclose(in);
        exit(1);
    }

    goto initial_state;
    for(;;)
    {
        int start, end, len;

        start = fgetc(in);
        if(start == EOF) break;
        ungetc(start, in);

        start = get_le_word(in);
        if(start == 0xffff)
        {
        initial_state:
            /* allowed optional header */
            start = get_le_word(in);
        }
        end = get_le_word(in);
        if(start > 0xffff || end > 0xffff || end < start)
        {
            LOG(LOG_ERROR, ("Error: corrupt data in xex-file."));
            fclose(in);
            exit(1);
        }
        if(start == 0x2e2 && end == 0x2e3)
        {
            /* init vector */
            jsr = get_le_word(in);
            LOG(LOG_VERBOSE, ("Found xex initad $%04X.\n", jsr));
            continue;
        }
        if(start == 0x2e0 && end == 0x2e1)
        {
            /* run vector */
            run = get_le_word(in);
            LOG(LOG_VERBOSE, ("Found xex runad $%04X.\n", run));
            continue;
        }
        ++end;
        jsr = -1;
        if(start < min) min = start;
        if(end > max) max = end;

        len = fread(mem + start, 1, end - start, in);
        if(len != end - start)
        {
            LOG(LOG_ERROR, ("Error: unexpected end of xex-file.\n"));
            fclose(in);
            exit(1);
        }
        LOG(LOG_VERBOSE, (" xex chunk loading from $%04X to $%04X\n",
                          start, end));
    }

    if(run == -1 && jsr != -1) run = jsr;

    info->start = min;
    info->end = max;
    info->basic_var_start = -1;
    info->run = -1;
    if(run != -1)
    {
        info->run = run;
    }
}

/* Handles tap files */
static void load_oric_tap(unsigned char mem[65536], FILE *in,
                          struct load_info *load_info)
{
    int c;
    int autostart;
    int start, end, len;
    unsigned char buf[ORIC_TAP_MAGIC_LEN];

    /* read oric tap header */
    if (fread(buf, 1, ORIC_TAP_MAGIC_LEN, in) != ORIC_TAP_MAGIC_LEN)
    {
        LOG(LOG_ERROR, ("Error: failed to read Oric TAP header from file.\n"));
        fclose(in);
        exit(1);
    }
    if (!is_matching_header(buf, oric_tap_magic, ORIC_TAP_MAGIC_LEN))
    {
        LOG(LOG_ERROR, ("Error: not a valid Oric tap-header."));
        fclose(in);
        exit(1);
    }

    /* There can be optionally more 0x16 bytes */
    while((c = get_byte(in)) == 0x16);

    /* next byte must be 0x24 */
    if(c != 0x24)
    {
        LOG(LOG_ERROR, ("Error: bad sync byte after lead-in in Oric tap-file "
                        "header, got $%02X but expected $24\n", c));
        fclose(in);
        exit(1);
    }

    /* now we are in sync, lets be lenient */
    get_byte(in); /* should be 0x0 */
    get_byte(in); /* should be 0x0 */
    get_byte(in); /* should be 0x0 or 0x80 */
    autostart = (get_byte(in) != 0);  /* should be 0x0, 0x80 or 0xc7 */
    end = get_be_word(in) + 1; /* the header end address is inclusive */
    start = get_be_word(in);
    get_byte(in); /* should be 0x0 */
    /* read optional file name */
    while(get_byte(in) != 0x0);

    /* read the data */
    len = fread(mem + start, 1, end - start, in);
    if(len != end - start)
    {
        LOG(LOG_BRIEF, ("Warning: Oric tap-file contains %d byte(s) data "
                        "less than expected.\n", end - start - len));
        end = start + len;
    }
    LOG(LOG_VERBOSE, (" Oric tap-file loading from $%04X to $%04X\n",
                      start, end));

    /* fill in the fields */
    if (load_info != NULL)
    {
        load_info->start = start;
        load_info->end = end;
        load_info->run = -1;
        load_info->basic_var_start = -1;
        if(autostart)
        {
            load_info->run = start;
        }
        if (load_info->basic_txt_start >= start &&
            load_info->basic_txt_start < end)
        {
            load_info->basic_var_start = end - 1;
        }
    }
}

struct as_descr
{
    unsigned int id;
    unsigned int offset;
    unsigned int length;
};

static int cb_cmp_as_descr_id(const void *a, const void *b)
{
    const struct as_descr *ap = a;
    const struct as_descr *bp = b;

    int result = 0;
    if (ap->id < bp->id)
    {
        result = -1;
    }
    else if (ap->id > bp->id)
    {
        result = 1;
    }
    return result;
}

/**
 * Gets the load address from a PRODOS entry in an AppleSingle file.
 * Also validates that the file type is binary or Applesoft basic.
 */
static int load_applesingle_prodos_entry(FILE *in,
                                         struct as_descr *prodos,
                                         int *file_type)
{
    int type;
    int load_addr;
    if (prodos->length != 8)
    {
        LOG(LOG_ERROR, ("Error: invalid length of prodos entry %d.\n",
                        prodos->length));
        fclose(in);
        exit(1);
    }
    if (fseek(in, prodos->offset, SEEK_SET) != 0)
    {
        LOG(LOG_ERROR,
            ("Error: failed to seek to prodos entry at offset %d.\n",
             prodos->offset));
        fclose(in);
        exit(1);
    }
    get_be_word(in); /* access */
    type = get_be_word(in); /* file type */
    if (type == 0xff)
    {
        /* system file that loads to $2000 */
        load_addr = 0x2000;
    }
    else
    if (type == 6 || type == 0xfc)
    {
        /* binary, Applesoft basic */
        load_addr = get_be_dword(in); /* aux file type */
    }
    else
    {
        /* unsupported file type*/
        LOG(LOG_ERROR,
            ("Error: unsupported value $%X for PRODOS filetype.\n", type));
        fclose(in);
        exit(1);
    }
    if (load_addr < 0 || load_addr > 0xffff)
    {
        /* not binary, nor Applesoft basic */
        LOG(LOG_ERROR,
            ("Error: unexpected value $%X for PRODOS aux filetype.\n",
             type));
        fclose(in);
        exit(1);
    }
    if (file_type != NULL)
    {
        *file_type = type;
    }
    return load_addr;
}

/* Handles apple single files */
static void load_applesingle(unsigned char mem[65536], FILE *in,
                             struct load_info *load_info)
{
    int desc_size;
    int i;
    unsigned char buf[16];
    struct vec descrs;
    struct as_descr *prodosp;
    struct as_descr *datap;
    struct as_descr key;
    int load_addr;
    int file_type;
    int length;

    /* read oric tap header */
    if (fread(buf, 1, APPLESINGLE_MAGIC_LEN, in) != APPLESINGLE_MAGIC_LEN)
    {
        LOG(LOG_ERROR,
            ("Error: failed to read applesingle header from file.\n"));
        fclose(in);
        exit(1);
    }
    if (!is_matching_header(buf, applesingle_magic, APPLESINGLE_MAGIC_LEN))
    {
        LOG(LOG_ERROR, ("Error: not a valid AppleSingle-header."));
        fclose(in);
        exit(1);
    }

    /* skip filler */
    if (fread(buf, 1, 16, in) != 16)
    {
        LOG(LOG_ERROR, ("Error: unexpected end of AppleSingle file."));
        fclose(in);
        exit(1);
    }

    /* Read the number of entr descriptors that follow */
    desc_size = get_be_word(in);

    vec_init(&descrs, sizeof(struct as_descr));

    for (i = 0; i < desc_size; ++i)
    {
        struct as_descr descr;
        descr.id = get_be_dword(in);
        descr.offset = get_be_dword(in);
        descr.length = get_be_dword(in);
        if (!vec_insert_uniq(&descrs, cb_cmp_as_descr_id, &descr, NULL))
        {
            LOG(LOG_ERROR, ("Error: duplicate descriptor for id %d\n.",
                            descr.id));
            fclose(in);
            exit(1);
        }
    }

    /* find the PRODOS descriptor */
    key.id = 11;
    prodosp = vec_find2(&descrs, cb_cmp_as_descr_id, &key);
    if (prodosp == NULL)
    {
        LOG(LOG_ERROR,
            ("Error: unable to find descr 11 in AppleSingle file\n."));
        fclose(in);
        exit(1);
    }

    /* find the data descriptor */
    key.id = 1;
    datap = vec_find2(&descrs, cb_cmp_as_descr_id, &key);
    if (datap == NULL)
    {
        LOG(LOG_ERROR,
            ("Error: unable to find descr 1 in AppleSingle file\n."));
        fclose(in);
        exit(1);
    }

    load_addr = load_applesingle_prodos_entry(in, prodosp, &file_type);

    if (fseek(in, datap->offset, SEEK_SET) != 0)
    {
        LOG(LOG_ERROR,
            ("Error: failed to seek to data entry at offset %d.\n",
             prodosp->offset));
        fclose(in);
        exit(1);
    }
    length = datap->length;
    if (length > 65536 - load_addr)
    {
        /* truncating length to available buffer space */
        length = 65536 - load_addr;
    }
    if (fread(mem + load_addr, 1, datap->length, in) != datap->length)
    {
        LOG(LOG_ERROR, ("Error: unexpected end of AppleSingle file."));
        fclose(in);
        exit(1);
    }

    if (load_info != NULL)
    {
        load_info->start = load_addr;
        load_info->end = load_addr + datap->length;
        load_info->run = -1;
        load_info->basic_var_start = -1;
        if (file_type != 0xfc)
        {
            /* We're not a basic file */
            load_info->run = load_addr;
        }
        if (file_type == 0xff)
        {
            /* We're a system file, not plain bin or basic */
            load_info->type = APPLESINGLE_SYS;
        }
        if (load_info->basic_txt_start >= load_info->start &&
            load_info->basic_txt_start < load_info->end)
        {
            load_info->basic_var_start = load_info->end;
        }
    }

    vec_free(&descrs, NULL);
}

/**
 * Requires that open_info->load_addr is set to a proper value
 * (not OPEN_UNPROVIDED nor OPEN_DEFAULT).
 */
static void load_raw(unsigned char mem[65536], FILE *in,
                     struct open_info *open_info,
                     struct load_info *load_info)
{
    int len;

    if (open_info->load_addr == OPEN_UNPROVIDED ||
        open_info->load_addr == OPEN_DEFAULT)
    {
        LOG(LOG_ERROR,
            ("Error: No load address given for raw file."));
        fclose(in);
        exit(1);
    }

    seek_and_normalize_offset_and_len(in, open_info);
    len = fread(mem + open_info->load_addr, 1, open_info->length, in);

    if (load_info != NULL)
    {
        load_info->start = open_info->load_addr;
        load_info->end = open_info->load_addr + len;
        load_info->basic_var_start = -1;
        load_info->run = -1;
        if(load_info->basic_txt_start >= load_info->start &&
           load_info->basic_txt_start < load_info->end)
        {
            load_info->basic_var_start = load_info->end;
        }
    }
}

static int try_load_bbc_inf(unsigned char mem[65536],
                            FILE *in,
                            const char *filename,
                            struct open_info *open_info,
                            struct load_info *load_info)
{
    FILE *inf;
    char *p;
    unsigned int load;
    unsigned int run;

    /* try to open shadowing .inf file */
    struct buf name_buf = STATIC_BUF_INIT;
    buf_printf(&name_buf, "%s.inf", filename);
    p = buf_data(&name_buf);
    inf = fopen(p, "rb");
    if (inf == NULL)
    {
        /* failed to open */
        return 0;
    }
    /* parse .inf file here and populate load_info */
    if (fscanf(inf, "%*s %x %x", &load, &run) != 2)
    {
        LOG(LOG_ERROR,
            ("Error: Failed to parse BBCIm/BBCXfer inf from \"%s\".", p));
        fclose(inf);
        fclose(in);
        exit(1);
    }
    fclose(inf);
    load &= 0xffff;
    run &= 0xffff;
    LOG(LOG_DEBUG, ("BBC inf: load %06X, run %06X.", load, run));

    /* read data file here */
    open_info->load_addr = load;
    load_raw(mem, in, open_info, load_info);
    if (load_info != NULL)
    {
        /* set run address after load_raw since it clears it */
        load_info->run = run;
    }

    /* success */
    buf_free(&name_buf);
    return 1;
}

void load_located(const char *filename, unsigned char mem[65536],
                  struct load_info *info)
{
    struct open_info open_info;
    FILE *in;
    enum file_type type;
    int load_addr;

    in = open_file(filename, &open_info);
    if (open_info.is_raw)
    {
        type = RAW;
    }
    else if (open_info.load_addr != OPEN_UNPROVIDED)
    {
        /* relocaded prg */
        type = PRG;
    }
    else if (try_load_bbc_inf(mem, in, filename, &open_info, info))
    {
        type = BBC_INF;
    }
    else
    {
        type = detect_type(in);
    }
    if (info != NULL)
    {
        info->type = type;
    }
    switch (type)
    {
    case ATARI_XEX:
        /* file is an xex file */
        load_xex(mem, in, info);
        break;
    case ORIC_TAP:
        /* file is an oric tap file */
        load_oric_tap(mem, in, info);
        break;
    case APPLESINGLE:
        /* file is an AppleSingle file */
        load_applesingle(mem, in, info);
        break;
    case BBC_INF:
        /* already loaded by try_load_bbc_inf(...) */
        break;
    case PRG:
        load_addr = get_le_word(in);
        if (open_info.load_addr == OPEN_UNPROVIDED ||
            open_info.load_addr == OPEN_DEFAULT)
        {
            open_info.load_addr = load_addr;
        }
        /* no break on purpose */
    case RAW:
        if (open_info.load_addr == OPEN_UNPROVIDED ||
            open_info.load_addr == OPEN_DEFAULT)
        {
            LOG(LOG_ERROR,
                ("Error: No load address given for raw file \"%s\".\n",
                 filename));
            fclose(in);
            exit(1);
        }

        /* file is a located raw file or a prg file
         * with it's header already read */
        load_raw(mem, in, &open_info, info);
        break;
    default:
        LOG(LOG_ERROR, ("Error: unknown file type for file \"%s\".\n",
                        filename));
        fclose(in);
        exit(1);
        break;
    }
    fclose(in);

    LOG(LOG_BRIEF,
        (" Reading \"%s\", loading from $%04X to $%04X.\n",
         filename, info->start, info->end));
}

int str_to_int(const char *str, int *value, const char **strp)
{
    int status = 0;
    do {
        char *str_end;
        long lval;

        /* base 0 is auto detect */
        int base = 0;

        if (*str == '\0')
        {
            /* no string to parse */
            status = 1;
            break;
        }

        if (*str == '$' || *str == '&')
        {
            /* a $ or & prefix specifies base 16 */
            ++str;
            base = 16;
        }
        if (*str == '%')
        {
            /* a % prefix specifies base 2 */
            ++str;
            base = 2;
        }

        lval = strtol(str, &str_end, base);
        if (str == str_end)
        {
            /* found nothing to parse */
            status = 1;
            break;
        }

        if(strp == NULL && *str_end != '\0')
        {
            /* there is garbage in the string */
            status = 1;
            break;
        }

        if(value != NULL)
        {
            /* all is well, set the out parameter */
            *value = (int)lval;
        }

        if(strp != NULL)
        {
            *strp = str_end;
        }

    } while(0);

    return status;
}

const char *fixup_appl(char *appl)
{
    char *applp;

    /* strip pathprefix from appl */
    applp = strrchr(appl, '\\');
    if (applp != NULL)
    {
        appl = applp + 1;
    }
    applp = strrchr(appl, '/');
    if (applp != NULL)
    {
        appl = applp + 1;
    }
    /* strip possible exe suffix */
    applp = appl + strlen(appl) - 4;
    if(strcmp(applp, ".exe") == 0 || strcmp(applp, ".EXE") == 0)
    {
        *applp = '\0';
    }
    return appl;
}

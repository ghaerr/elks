/*
 * Copyright (c) 2003 Magnus Lind.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "getflag.h"
#include "bprg.h"

int
read_int_sequence(const char *str, int num, int value_vector[])
{
    int i = 0;

    for(;;)
    {
        int value;
        int base = 0;
        char *str_end;
        if(*str == '$')
        {
            ++str;
            base = 16;
        }
        LOG(LOG_DEBUG, ("strtol %s\n", str));
        value = strtol(str, &str_end, base);
        if(str == str_end)
        {
            /* conversion failed somehow */
            i = -i - 1;
            break;
        }
        LOG(LOG_DEBUG, ("vv[%d] = %d\n", i, value));

        value_vector[i++] = value;

        str = str_end;
        if(*str == '\0')
        {
            /* end of sequence */
            break;
        }
        if(i >= num)
        {
            /* too many entries in sequence, value_vector is too small */
            i = -i;
            break;
        }
        if(*str != ',')
        {
            /* garbage characters in sequence */
            i = -i;
            break;
        }
        ++str;
    }
    LOG(LOG_DEBUG, ("returning %d\n", i));
    return i;
}

FILE *
open_file(const char *name, int *load_addr)
{
    FILE * in;
    int relocated = 0;
    int reloc = 0;
    int load;

    do {
        char *load_str;

        in = fopen(name, "rb");
        if (in != NULL)
        {
            /* we have succeded in opening the file */
            break;
        }

        /* hmm, let's see if the user is trying to relocate it */
        load_str = strrchr(name, ',');
        if (load_str == NULL)
        {
            /* we fail */
            break;
        }

        *load_str = '\0';
        ++load_str;
        relocated = 1;

        /* relocation was requested */
        if (read_int_sequence(load_str, 1, &reloc) < 0)
        {
            /* we fail */
            LOG(LOG_ERROR,
                (" can't parse load address from \"%s\"\n", load_str));
            exit(1);
        }

        in = fopen(name, "rb");

    } while (0);
    if (in == NULL)
    {
        LOG(LOG_ERROR,
            (" can't open file \"%s\" for input\n", name));
        exit(1);
    }

    /* set the load address */
    load = fgetc(in);
    load |= fgetc(in) << 8;

    if(load_addr != NULL)
    {
        *load_addr = load;
        if (relocated)
        {
            *load_addr = reloc;
        }
    }
    LOG(LOG_DEBUG, ("opened file \"%s\" for load at $%04X.\n",
                    name, *load_addr));
    return in;
}

void print_license(void)
{
    LOG(LOG_WARNING,
        ("----------------------------------------------------------------------------\n"
         "Exobasic v1.0b2, Copyright (c) 2003 Magnus Lind. (magli143@gmail.com)\n"
         "----------------------------------------------------------------------------\n"));
    LOG(LOG_WARNING,
        ("This software is provided 'as-is', without any express or implied warranty.\n"
         "In no event will the authors be held liable for any damages arising from\n"
         "the use of this software.\n"
         "Permission is granted to anyone to use this software, alter it and re-\n"
         "distribute it freely for any non-commercial, non-profit purpose subject to\n"
         "the following restrictions:\n\n"));
    LOG(LOG_WARNING,
        ("   1.  The origin of this software must not be misrepresented; you must not\n"
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
         "nor infected by this license in any way.\n"));
}


void print_usage(const char *appl, enum log_level level)
{
    const char *applp;

    /* strip pathprefix from appl */
    applp = strrchr(appl, '\\');
    if (applp != NULL)
    {
        appl = applp + 1;
    }                           /* done */
    applp = strrchr(appl, '/');
    if (applp != NULL)
    {
        appl = applp + 1;
    }
    /* done */
    LOG(level, ("usage: %s [option]... infile[,<address>]...\n", appl));
    LOG(level,
        ("  -n <start>,[<increment>]\n"
         "               renumbers the basic program. line numbers will start at <start>\n"
         "               and be incremented with <increment>. can't be combined with -N\n"
         "  -N           crunch optimized renumbering. basic program will be runnable\n"
         "               but not editable after this. can't be combined with -n\n"
         "  -p           save file with crunch optimized (broken) line links. if\n"));
    LOG(level,
        ("               combined with -t, the trampoline will repair the links before\n"
         "               it runs the program.\n"
         "  -r           remove remarks and unneccessary spaces\n"
         "  -t           generate c64 trampoline, the trampoline routine will be\n"
         "               located at the beginning of the outfile and is started by\n"
         "               jmp:ing to the first address it loads to.\n"
         "  -c           trampoline will also recreate the stack color table, must be\n"
         "               combined with -t and -4.\n"));
    LOG(level,
        ("  -4           generate c16/plus4 trampoline instead of c64, must be combined\n"
         "               with -t.\n"
         "  --           treat all args to the right as non-options\n"
         "  -?           displays this help screen\n"
         "  -v           displays version and the usage license\n"
         " The name of the outfile(s) will be the name of the infile(s) with a suffix of\n"
         " \".out.prg\" added.\n"));
}

int main(int argc, char *argv[])
{
    int renargs[2] = {-1, 5};
    int renflags = 0;

    int patch_links = 0;
    int remarks = 0;
    int renumber = 0;
    int reNumber = 0;
    int trampoline = 0;
    int c264_color = 0;
    int c264 = 0;
    int *t_start = NULL;
    int *t_var = NULL;
    int *t_end = NULL;

    int c, infilec;
    char **infilev;

    /* init logging */
    LOG_INIT_CONSOLE(LOG_NORMAL);

    LOG(LOG_DUMP, ("flagind %d\n", flagind));
    while ((c = getflag(argc, argv, "s:n:Nprt4cv")) != -1)
    {
        LOG(LOG_DUMP, (" flagind %d flagopt '%c'\n", flagind, c));
        switch (c)
        {
        case 'r':
            LOG(LOG_DEBUG, ("option -r: remove remarks and spaces\n"));
            remarks = 1;
            break;
        case 'p':
            LOG(LOG_DEBUG, ("option -p: setting link patch mode\n"));
            patch_links = 1;
            break;
        case 'N':
            reNumber = 1;
            renflags = 1;
            renargs[0] = 0;
            renargs[1] = 1;
            LOG(LOG_DEBUG, ("option -N: brutal renumber\n"));
            break;
        case 'n':
            if (read_int_sequence(flagarg, 2, renargs) < 0 ||
                renargs[0] < 0 || renargs[0] >= 63999 ||
                renargs[1] < 0 || renargs[1] >= 63999)
            {
                LOG(LOG_ERROR,
                    ("error: invalid number for -n option, "
                     "must be in the range of [0 - 63999]\n"));
                print_usage(argv[0], LOG_ERROR);
                exit(1);
            }
            LOG(LOG_DEBUG, ("option -n: nice renumber, "
                            "start with %d, increment %d\n",
                            renargs[0],renargs[1]));
            renumber = 1;
            break;
        case 't':
            LOG(LOG_DEBUG, ("option -t: adding trampoline\n"));
            trampoline = 1;
            break;
        case 'c':
            LOG(LOG_DEBUG, ("option -c: adding c264 color table regen.\n"));
            c264_color = 1;
            break;
        case '4':
            LOG(LOG_DEBUG, ("option -4: using c264 trampoline\n"));
            c264 = 1;
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
            print_usage(argv[0], LOG_WARNING);
            exit(0);
        }
    }
#if 0
    LOG(LOG_DEBUG, ("flagind %d\n", flagind));
    for (c = 0; c < argc; ++c)
    {
        if (c == flagind)
        {
            LOG(LOG_DEBUG, ("-----------------------\n"));
        }
        LOG(LOG_DEBUG, ("argv[%d] = \"%s\"\n", c, argv[c]));
    }
    exit(1);
#endif
    if(renumber && reNumber)
    {
        LOG(LOG_ERROR, ("error: the -N and -n options can't be combined.\n"));
        print_usage(argv[0], LOG_ERROR);
        exit(1);
    }
    if(c264 && !trampoline)
    {
        LOG(LOG_ERROR,
            ("error: the -4 option must be combined with -t.\n"));
        print_usage(argv[0], LOG_ERROR);
        exit(1);
    }
    if(c264_color && !c264)
    {
        LOG(LOG_ERROR,
            ("error: the -c option must be combined with -t and -4.\n"));
        print_usage(argv[0], LOG_ERROR);
        exit(1);
    }

    infilev = argv + flagind;
    infilec = argc - flagind;

    if(infilec == 0)
    {
        LOG(LOG_ERROR,
            ("error: at least one infile has to be specified.\n"));
        print_usage(argv[0], LOG_ERROR);
        exit(1);
    }

    for(c = 0; c < infilec; ++c)
    {
        struct bprg_ctx ctx;
        char buf[1000];

        bprg_init(&ctx, infilev[c]);

        if(remarks)
        {
            LOG(LOG_NORMAL, (" - removing remarks and spaces"));
            bprg_rem_remove(&ctx);
            LOG(LOG_NORMAL, (", done.\n"));
        }

        if(renumber || reNumber)
        {
            LOG(LOG_NORMAL, (" - renumbering basic lines"));
            bprg_renumber(&ctx, renargs[0], renargs[1], renflags);
            LOG(LOG_NORMAL, (", done.\n"));
        }

        if(trampoline)
        {
            int flags;

            LOG(LOG_NORMAL, (" - adding trampoline @ %04X",
                             ctx.start));
            flags = 0;
            flags |= patch_links ? TRAMPOLINE_FLAG_REGEN: 0;
            flags |= c264 ? TRAMPOLINE_FLAG_C264: 0;
            flags |= c264_color ? TRAMPOLINE_FLAG_C264_COLOR_REGEN: 0;

            if(flags & TRAMPOLINE_FLAG_C264)
            {
                LOG(LOG_NORMAL, (" for c16/plus4"));
            }
            if(flags & TRAMPOLINE_FLAG_REGEN)
            {
                LOG(LOG_NORMAL, (" that recreates links"));
            }
            if(flags & TRAMPOLINE_FLAG_C264_COLOR_REGEN)
            {
                LOG(LOG_NORMAL, (" and color table"));
            }

            bprg_trampoline_add(&ctx, t_start, t_var, t_end, flags);
            LOG(LOG_NORMAL, (", done.\n"));
        }
        if(patch_links)
        {
            LOG(LOG_NORMAL, (" - clobbering basic line links"));
            bprg_link_patch(&ctx);
            LOG(LOG_NORMAL, (", done.\n"));
        }

        strcpy(buf, infilev[c]);
        strcat(buf, ".out.prg");

        bprg_save(&ctx, buf);

        bprg_free(&ctx);
    }

    return 0;
}

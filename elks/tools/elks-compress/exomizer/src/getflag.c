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

#include <string.h>
#include <stdio.h>

int flagind = 1;
int flagflag = '?';
const char *flagarg = NULL;

static void reverse(char **buf, int pos1, int pos2)
{
    char **buf1;
    char **buf2;
    char *tmp;

    buf1 = buf + pos1;
    buf2 = buf + pos2 - 1;

    while (buf1 < buf2)
    {
        tmp = *buf1;
        *buf1 = *buf2;
        *buf2 = tmp;

        ++buf1;
        --buf2;
    }
}


int getflag(int argc, char **argv, const char *flags)
{
    int argstart, flagstart, c;
    const char *flagp;

    c = -1;
    flagarg = NULL;
    argstart = flagind;
    flagstart = argc;

    /* skip over non-flags */
    while (flagind < argc && argv[flagind][0] != '-')
    {
        ++flagind;
    }
    if (flagind == argc)
    {
        /* no more args */
        flagind = argstart;
        return c;
    }
    /* we have an arg to work with */
    do
    {
        flagstart = flagind;
        if (argv[flagind][1] == '-' && argv[flagind][2] == '\0')
        {
            /* stop parsing at '--' flag */
            break;
        }
        c = flagflag = argv[flagind][1];
        if (c == ':' || c == '\0')
        {
            /* this is an illegal flag */
            c = '?';
            break;
        }
        /* flag with arg */
        if (argv[flagind][2] != '\0')
        {
            /* flag-arg in same argv[] */
            flagarg = argv[flagind] + 2;
        }
        flagp = strchr(flags, c);
        if (flagp == NULL)
        {
            /* this is an unknown flag */
            c = '?';
            break;
        }

        if (flagp[1] != ':')
        {
            /* We expect no argument */
            if (flagarg != NULL)
            {
                /* error, a simple flag with an argument */
                c = '?';
            }
            break;
        }

        /* We expect an argument */
        if (flagarg != NULL)
        {
            /* ... and have one */
            break;
        }

        /* flag-arg is in the next argv[] */
        if (flagind + 1 == argc)
        {
            /* auahh, no flag-arg */
            flagstart = argstart;
            c = ':';
            break;
        }

        flagarg = argv[++flagind];
    } while (0);
    /* skip to next arg */
    ++flagind;

    if (flagstart < flagind && argstart < flagstart)
    {
        /* we have found some args
         * we have also skipped over some non-args
         * shuffle the non-flag arg to the end of argv */
        reverse(argv, argstart, flagstart);
        reverse(argv, flagstart, flagind);
        reverse(argv, argstart, flagind);
    }
    flagind = argstart + flagind - flagstart;

    return c;
}

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

#include "progress.h"
#include "log.h"
#include <string.h>

#define BAR_LENGTH 64

void progress_init(struct progress *p, char *msg, int start, int end)
{
    if(start > end)
    {
        p->factor = (float)BAR_LENGTH / (end - start);
        p->offset = -start;
    }
    else
    {
        p->factor = (float)BAR_LENGTH / (start - end);
        p->offset = start;
    }
    p->last = -1;
    if(msg == NULL)
    {
        msg = "progressing_";
    }
    p->msg = msg;

    LOG(LOG_DEBUG, ("start %d, end %d, pfactor %f, poffset %d\n",
                    start, end, p->factor, p->offset));
}

void progress_bump(struct progress *p, int pos)
{
    int fraction = ((pos + p->offset) * p->factor) + 0.5;
    while(fraction > p->last)
    {
        if(p->last == -1)
        {
            LOG_TTY(LOG_NORMAL, ("  %*s]\r [", BAR_LENGTH, ""));
        }
        else
        {
            LOG_TTY(LOG_NORMAL, ("%c", p->msg[p->last % strlen(p->msg)]));
        }
        p->last += 1;
    }
}

void progress_free(struct progress *p)
{
    LOG_TTY(LOG_NORMAL, ("\n"));
}

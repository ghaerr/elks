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

#include <stdlib.h>
#include "bprg.h"
#include "log.h"

static int
linkpatch_cb_line_mutate(const unsigned char *in, /* IN */
                         unsigned char *mem, /* IN/OUT */
                         unsigned short *pos, /* IN/OUT */
                         void *priv) /* IN/OUT */
{
    unsigned short start;
    unsigned short i;

    start = *pos;
    i = 0;

    /* skip link */
    i += 2; in += 2;
    /* copy number */
    mem[start + i++] = *(in++);
    mem[start + i++] = *(in++);
    /* copy line including terminating '\0' */
    while((mem[start + i++] = *(in++)) != '\0');

    /* the purpose of this whole "if" structure is to improve the
     * crunching of the basic program by changing the basic line
     * link pointers to something that crunches better. This is
     * possible since the basic normally recreates the links when
     * loading the basic program anyhow. However if we decrunch
     * the basic program this will not happen. But there is still
     * a nice subroutine that does the job for us that we can call
     * from the basic start trampoline */

    /* link patching begins here */
    if(mem[start + 2] == '\0' && mem[start + 4] != '\0')
    {
        /* mask 1, length 3 offset 3*/
        mem[start + 0] = mem[start + 3];
        mem[start + 1] = mem[start + 4];

    } else if(mem[start + 3] == '\0' && mem[start + 5] != '\0')
    {
        /* mask 2, length 3 offset 4*/
        mem[start + 0] = mem[start + 4];
        mem[start + 1] = mem[start + 5];

    } else if(mem[start + 3] != '\0')
    {
        /* mask 3, length 2 offset 2*/
        mem[start + 0] = mem[start + 2];
        mem[start + 1] = mem[start + 3];

    } else if(mem[start + 4] != '\0')
    {
        /* mask 4, length 2 offset 3*/
        mem[start + 0] = mem[start + 3];
        mem[start + 1] = mem[start + 4];
    } else
    {
        /* if we ever get here I have made a big mistake in
           assuming that the test (mem[start + 4] != '\0') always is true
           since empty basic lines are forbidden */
        LOG(LOG_ERROR, ("Error, can't patch link (should be impossible)"));
        exit(1);
    }
    /* link patching ends here */

    *pos = start + i;
    /* success */
    return 0;
}

void
bprg_link_patch(struct bprg_ctx *ctx)
{
    bprg_lines_mutate(ctx, linkpatch_cb_line_mutate, NULL);
    LOG(LOG_VERBOSE, ("program length %hu\n", ctx->len));
}


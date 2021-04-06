/*
 * Copyright (c) 2004 Magnus Lind.
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

#include "pc.h"
#include "log.h"
#include <stdlib.h>

struct pc {
    struct expr *pc1;
    int pc2;
};

static struct expr unset_value;
static struct pc p = {&unset_value, 0};

void pc_dump(int level)
{
}

void pc_set(int pc)
{
    p.pc1 = NULL;
    p.pc2 = pc;
}

void pc_set_expr(struct expr *pc)
{
    p.pc1 = pc;
    p.pc2 = 0;
}

struct expr *pc_get(void)
{
    struct expr *old_pc1;

    if(p.pc1 == &unset_value)
    {
        LOG(LOG_ERROR, ("PC must be set by a .org(pc) call.\n"));
        exit(1);
    }
    if(p.pc1 == NULL || p.pc2 != 0)
    {
        old_pc1 = p.pc1;
        p.pc1 = new_expr_number(p.pc2);
        p.pc2 = 0;
        if(old_pc1 != NULL)
        {
            p.pc1 = new_expr_op2(PLUS, p.pc1, old_pc1);
        }
    }

    return p.pc1;
}

void pc_add(int offset)
{
    if(p.pc1 != &unset_value)
    {
        p.pc2 += offset;
    }
}

void pc_add_expr(struct expr *pc)
{
    struct expr *old_pc1;

    if(p.pc1 != &unset_value)
    {
        old_pc1 = p.pc1;
        p.pc1 = pc;
        if(old_pc1 != NULL)
        {
            p.pc1 = new_expr_op2(PLUS, p.pc1, old_pc1);
        }
    }
}

void pc_unset(void)
{
    pc_set_expr(&unset_value);
}

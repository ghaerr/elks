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

#include "expr.h"
#include "chunkpool.h"
#include "log.h"

#include <stdlib.h>

static struct chunkpool s_expr_pool;

void expr_init()
{
    chunkpool_init(&s_expr_pool, sizeof(struct expr));
}

void expr_free()
{
    chunkpool_free(&s_expr_pool);
}

void expr_dump(int level, struct expr *e)
{
    switch(e->expr_op)
    {
    case SYMBOL:
        LOG(level, ("expr %p symref %s\n", (void*)e, e->type.symref));
        break;
    case NUMBER:
        LOG(level, ("expr %p number %d\n", (void*)e, e->type.number));
        break;
    case vNEG:
        LOG(level, ("expr %p unary op %d, referring to %p\n",
                    (void*)e, e->expr_op, (void*)e->type.arg1));
    case LNOT:
        LOG(level, ("expr %p unary op %d, referring to %p\n",
                    (void*)e, e->expr_op, (void*)e->type.arg1));
        break;
    default:
        LOG(level, ("expr %p binary op %d, arg1 %p, arg2 %p\n",
                    (void*)e, e->expr_op, (void*)e->type.arg1,
                    (void*)e->expr_arg2));

    }
}

struct expr *new_expr_op1(i16 op, struct expr *arg)
{
    struct expr *val;

    if(op != vNEG && op != LNOT)
    {
        /* error, invalid unary operator  */
        LOG(LOG_ERROR, ("%d not allowed as unary operator\n", op));
        exit(1);
    }

    val = chunkpool_malloc(&s_expr_pool);
    val->expr_op = op;
    val->type.arg1 = arg;

    expr_dump(LOG_DEBUG, val);

    return val;
}

struct expr *new_expr_op2(i16 op, struct expr *arg1, struct expr *arg2)
{
    struct expr *val;

    if(op == vNEG ||
       op == LNOT ||
       op == NUMBER ||
       op == SYMBOL)
    {
        /* error, invalid binary operator  */
        printf("op %d, vNEG %d, NUMBER %d, SYMBOL %d\n", op, vNEG, NUMBER, SYMBOL);
        LOG(LOG_ERROR, ("%d not allowed as binary operator\n", op));
        exit(1);
    }

    val = chunkpool_malloc(&s_expr_pool);
    val->expr_op = op;
    val->type.arg1 = arg1;
    val->expr_arg2 = arg2;

    expr_dump(LOG_DEBUG, val);

    return val;
}

struct expr *new_expr_symref(const char *symbol)
{
    struct expr *val;

    val = chunkpool_malloc(&s_expr_pool);
    val->expr_op = SYMBOL;
    val->type.symref = symbol;

    expr_dump(LOG_DEBUG, val);

    return val;
}

struct expr *new_expr_number(i32 number)
{
    struct expr *val;

    LOG(LOG_DEBUG, ("creating new number %d\n", number));

    val = chunkpool_malloc(&s_expr_pool);
    val->expr_op = NUMBER;
    val->type.number = number;

    expr_dump(LOG_DEBUG, val);

    return val;
}

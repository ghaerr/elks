#ifndef ALREADY_INCLUDED_EXPR
#define ALREADY_INCLUDED_EXPR
#ifdef __cplusplus
extern "C" {
#endif

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

#include "int.h"
#include "asm.tab.h"

union expr_type
{
    i32 number;
    const char *symref;
    struct expr *arg1;
};

struct expr
{
    union expr_type type;
    struct expr *expr_arg2;
    i16 expr_op;
};

void expr_init(void);
void expr_free(void);

struct expr *new_expr_op1(i16 op, struct expr *arg);
struct expr *new_expr_op2(i16 op, struct expr *arg1, struct expr *arg2);
struct expr *new_expr_symref(const char *symbol);
struct expr *new_expr_number(i32 number);
void expr_dump(int level, struct expr *e);

#ifdef __cplusplus
}
#endif
#endif

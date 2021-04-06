#ifndef INCLUDED_PARSE
#define INCLUDED_PARSE
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
#include "vec.h"
#include "buf.h"
#include "expr.h"
#include "map.h"
#include "named_buffer.h"
#include "chunkpool.h"
#include "pc.h"

#define ATOM_TYPE_OP_ARG_NONE    0	/* uses u.op */
#define ATOM_TYPE_OP_ARG_U8      1	/* uses u.op */
#define ATOM_TYPE_OP_ARG_U16     2	/* uses u.op */
#define ATOM_TYPE_OP_ARG_I8      3	/* uses u.op */
#define ATOM_TYPE_OP_ARG_UI8     4	/* uses u.op */
#define ATOM_TYPE_EXPRS		12	/* uses u.exprs */
#define ATOM_TYPE_WORD_EXPRS	10	/* uses u.exprs */
#define ATOM_TYPE_BYTE_EXPRS	11	/* uses u.exprs */
#define ATOM_TYPE_RES		13	/* uses u.res */
#define ATOM_TYPE_BUFFER	14	/* uses u.buffer */

struct op
{
    struct expr *arg;
    u8 code;
};

struct res
{
    struct expr *length;
    struct expr *value;
};

struct buffer
{
    const char *name;
    i32 length;
    i32 skip;
};

struct atom
{
    u8 type;
    union
    {
        struct op op;
        struct vec *exprs;
        struct buffer buffer;
        struct res res;
    } u;
};

extern int push_state_skip;
extern int push_state_macro;
extern int push_state_init;
extern int num_lines;

void parse_init(void);
void parse_free(void);

void set_initial_symbol(const char *symbol, i32 value);
void set_initial_symbol_soft(const char *symbol, i32 value);
void initial_symbol_dump(int level, const char *symbol);

struct buf *new_initial_named_buffer(const char *name);

int assemble(struct buf *source, struct buf *dest);

/* start of internal functions */

struct atom *new_op(u8 op_code, u8 op_size, struct expr *arg);
struct atom *new_op0(u8 op_code);

struct atom *new_exprs(struct expr *arg);
struct atom *exprs_add(struct atom *atom, struct expr *arg);
struct atom *exprs_to_byte_exprs(struct atom *atom);
struct atom *exprs_to_word_exprs(struct atom *atom);

struct atom *new_res(struct expr *len, struct expr *value);
struct atom *new_incbin(const char *name,
                        struct expr *skip, struct expr *len);

struct expr *new_is_defined(const char *symbol);
struct expr *new_expr_inclen(const char *name);
struct expr *new_expr_incword(const char *name,
                              struct expr *skip);

void new_symbol_expr(const char *symbol, struct expr *arg);
void new_symbol_expr_guess(const char *symbol, struct expr *arg);

/* returns NULL if found, not otherwise, expp may be NULL. */
const char *find_symref(const char *symbol,
                        struct expr **expp);

int resolve_symbol(const char *symbol, int *has_valuep, i32 *valuep);
void symbol_dump_resolved(int level, const char *symbol);

void new_label(const char *label);
void set_org(struct expr *arg);
void push_if_state(struct expr *arg);
void push_macro_state(const char *name);
void macro_append(const char *text);
void asm_error(const char *msg);
void asm_echo(const char *msg, struct atom *atom);
void asm_include(const char *msg);

void output_atoms(struct buf *out, struct vec *mem);
void asm_src_buffer_push(struct buf *buf);

int assembleSinglePass(struct buf *source, struct buf *dest);

#ifdef __cplusplus
}
#endif
#endif

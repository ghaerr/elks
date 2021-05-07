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

#include "parse.h"
#include "asm.tab.h"
#include "log.h"
#include "chunkpool.h"
#include "named_buffer.h"
#include "pc.h"
#include "map.h"

#include <stdlib.h>

struct parse {
    /* pools */
    struct chunkpool atom_pool;
    struct chunkpool vec_pool;
    /* one pass data only */
    struct map sym_table;
    struct named_buffer named_buffer;
    const char *macro_name;
    /* history of the multi pass */
    struct map *guesses;
    /* initial arguments */
    struct map initial_symbols;
    struct named_buffer initial_named_buffer;
};

static struct parse s;

void scanner_init(void);
void scanner_free(void);

void parse_init(void)
{
    expr_init();
    scanner_init();
    chunkpool_init(&s.atom_pool, sizeof(struct atom));
    chunkpool_init(&s.vec_pool, sizeof(struct vec));
    map_init(&s.sym_table);
    named_buffer_init(&s.named_buffer);

    map_init(&s.initial_symbols);
    named_buffer_init(&s.initial_named_buffer);
}

static void parse_reset(void)
{
    map_clear(&s.sym_table);
    named_buffer_clear(&s.named_buffer);
}

static void free_vec_pool(struct vec *v)
{
    vec_free(v, NULL);
}

void parse_free(void)
{
    named_buffer_free(&s.initial_named_buffer);
    map_free(&s.initial_symbols);

    named_buffer_free(&s.named_buffer);
    chunkpool_free(&s.atom_pool);
    chunkpool_free2(&s.vec_pool, (cb_free*)free_vec_pool);
    named_buffer_free(&s.named_buffer);
    map_free(&s.sym_table);
    scanner_free();
    expr_free();
}

int is_valid_i8(i32 value)
{
    return (value >= -128 && value <= 127);
}

int is_valid_u8(i32 value)
{
    return (value >= 0 && value <= 255);
}

int is_valid_ui8(i32 value)
{
    return (value >= -128 && value <= 255);
}

int is_valid_u16(i32 value)
{
    return (value >= 0 && value <= 65535);
}

int is_valid_ui16(i32 value)
{
    return (value >= -32768 && value <= 65535);
}

struct expr *new_is_defined(const char *symbol)
{
    int expr_val = (map_get(&s.sym_table, symbol) != NULL);
    return new_expr_number(expr_val);
}

void new_symbol_expr(const char *symbol, struct expr *arg)
{
    if(map_put(&s.sym_table, symbol, arg) != NULL)
    {
        /* error, symbol redefinition not allowed */
        LOG(LOG_ERROR, ("not allowed to redefine symbol %s\n", symbol));
        exit(1);
    }
}

void new_symbol_expr_guess(const char *symbol, struct expr *arg)
{
    /* Set a soft symbol only if it is not set */
    if(map_get(s.guesses, symbol) == NULL)
    {
        map_put(s.guesses, symbol, arg);
    }
}

const char *find_symref(const char *symbol,
                        struct expr **expp)
{
    struct expr *exp;
    const char *p;

    exp = NULL;
    p = NULL;
    if(s.guesses != NULL)
    {
        exp = map_get(s.guesses, symbol);
    }
    if(exp == NULL)
    {
        exp = map_get(&s.sym_table, symbol);
        if(exp == NULL)
        {
            static char buf[1024];
            /* error, symbol not found */
            sprintf(buf, "symbol %s not found", symbol);
            p = buf;
            LOG(LOG_DEBUG, ("%s\n", p));
            return p;
        }
    }

    if(expp != NULL)
    {
        *expp = exp;
    }

    return p;
}

void new_label(const char *label)
{
    struct expr *e = pc_get();
    new_symbol_expr(label, e);
}

static void dump_sym_table(int level, struct map *m)
{
    struct map_iterator i;
    const struct map_entry *e;

    for(map_get_iterator(m, &i); (e = map_iterator_next(&i)) != NULL;)
    {
        LOG(level, ("sym_table: %s ", e->key));
        expr_dump(level, e->value);
    }
}

void set_initial_symbol(const char *symbol, i32 value)
{
    struct expr *e = new_expr_number(value);
    map_put(&s.initial_symbols, symbol, e);
}

void set_initial_symbol_soft(const char *symbol, i32 value)
{
    if (!map_contains_key(&s.initial_symbols, symbol))
    {
        struct expr *e = new_expr_number(value);
        map_put(&s.initial_symbols, symbol, e);
    }
}

struct buf *new_initial_named_buffer(const char *name)
{
    return new_named_buffer(&s.initial_named_buffer, name);
}

static const char *resolve_expr2(struct expr *e, i32 *valp)
{
    struct expr *e2;
    i32 value;
    i32 value2;
    const char *p;

    p = NULL;
    LOG(LOG_DEBUG, ("resolve_expr: "));

    expr_dump(LOG_DEBUG, e);

    switch (e->expr_op)
    {
    case NUMBER:
        /* we are already resolved */
        value = e->type.number;
        break;
    case vNEG:
        p = resolve_expr2(e->type.arg1, &value);
        if(p != NULL) break;
        value = -value;
        break;
    case LNOT:
        p = resolve_expr2(e->type.arg1, &value);
        if(p != NULL) break;
        value = !value;
        break;
    case SYMBOL:
        e2 = NULL;
        p = find_symref(e->type.symref, &e2);
        if(p != NULL) break;
        if(e2 == NULL)
        {
            static char buf[1024];
            /* error, symbol not found */
            sprintf(buf, "symbol %s has no value.", e->type.symref);
            p = buf;
            LOG(LOG_DEBUG, ("%s\n", p));
            break;
        }
        p = resolve_expr2(e2, &value);
        break;
    default:
        LOG(LOG_DEBUG, ("binary op %d\n", e->expr_op));

        p = resolve_expr2(e->type.arg1, &value);
        if(p != NULL) break;

        /* short circuit the logical operators */
        if(e->expr_op == LOR)
        {
            value = (value != 0);
            if(value) break;
        }
        else if(e->expr_op == LAND)
        {
            value = (value != 0);
            if(!value) break;
        }

        p = resolve_expr2(e->expr_arg2, &value2);
        if(p != NULL) break;

        switch(e->expr_op)
        {
        case MINUS:
            value -= value2;
            break;
        case PLUS:
            value += value2;
            break;
        case MULT:
            value *= value2;
            break;
        case DIV:
            value /= value2;
            break;
        case MOD:
            value %= value2;
            break;
        case LT:
            value = (value < value2);
            break;
        case GT:
            value = (value > value2);
            break;
        case EQ:
            value = (value == value2);
            break;
        case NEQ:
            value = (value != value2);
            break;
        case LOR:
            value = (value2 != 0);
            break;
        case LAND:
            value = (value2 != 0);
            break;
        default:
            LOG(LOG_ERROR, ("unsupported op %d\n", e->expr_op));
            exit(1);
        }
    }
    if(p == NULL)
    {
        if(e->expr_op != NUMBER)
        {
            /* shortcut future recursion */
            e->expr_op = NUMBER;
            e->type.number = value;
        }
        if(valp != NULL)
        {
            *valp = value;
        }
    }

    return p;
}

static i32 resolve_expr(struct expr *e)
{
    i32 val;
    const char *p;

    p = resolve_expr2(e, &val);
    if(p != NULL)
    {
        LOG(LOG_ERROR, ("%s\n", p));
        exit(1);
    }
    return val;
}

struct expr *new_expr_inclen(const char *name)
{
    long length;
    struct buf *in;
    struct expr *expr;

    in = get_named_buffer(&s.named_buffer, name);
    length = buf_size(in);

    expr = new_expr_number((i32)length);
    return expr;
}

struct expr *new_expr_incword(const char *name,
                              struct expr *skip)
{
    i32 word;
    i32 offset;
    long length;
    struct buf *in;
    struct expr *expr;
    unsigned char *p;

    offset = resolve_expr(skip);
    in = get_named_buffer(&s.named_buffer, name);
    length = buf_size(in);
    if(offset < 0)
    {
        offset += length;
    }
    if(offset < 0 || offset > length - 2)
    {
        LOG(LOG_ERROR,
            ("Can't read word from offset %d in file \"%s\".\n",
             offset, name));
        exit(1);
    }
    p = buf_data(in);
    p += offset;
    word = *p++;
    word |= *p++ << 8;

    expr = new_expr_number(word);
    return expr;
}

void set_org(struct expr *arg)
{
    /* org assembler directive */
    pc_set_expr(arg);
    LOG(LOG_DEBUG, ("setting .org to ???\n"));
    return;
}

void push_macro_state(const char *name)
{
    s.macro_name = name;
    push_state_macro = 1;
    new_named_buffer(&s.named_buffer, name);
}

void macro_append(const char *text)
{
    struct buf *mb;

    LOG(LOG_DEBUG, ("appending >>%s<< to macro\n", text));

    mb = get_named_buffer(&s.named_buffer, s.macro_name);
    buf_append(mb, text, strlen(text));
}

void push_if_state(struct expr *arg)
{
    int val;
    LOG(LOG_DEBUG, ("resolving if expression\n"));
    val = resolve_expr(arg);
    LOG(LOG_DEBUG, ("if expr resolved to %d\n", val));
    if(val)
    {
        push_state_init = 1;
    }
    else
    {
        push_state_skip = 1;
    }
}

struct atom *new_op(u8 op_code, u8 atom_op_type,
                    struct expr *op_arg)
{
    struct atom *atom;

    atom = chunkpool_malloc(&s.atom_pool);
    atom->type = atom_op_type;
    atom->u.op.code = op_code;
    atom->u.op.arg = op_arg;

    switch(atom_op_type)
    {
    case ATOM_TYPE_OP_ARG_NONE:
        pc_add(1);
        break;
    case ATOM_TYPE_OP_ARG_U8:
        pc_add(2);
        break;
    case ATOM_TYPE_OP_ARG_U16:
        pc_add(3);
        break;
    case ATOM_TYPE_OP_ARG_I8:
        pc_add(2);
        atom->u.op.arg = new_expr_op2(MINUS, atom->u.op.arg, pc_get());
        break;
    case ATOM_TYPE_OP_ARG_UI8:
        pc_add(2);
        break;
    default:
        LOG(LOG_ERROR, ("invalid op arg range %d\n", atom_op_type));
        exit(1);
    }
    pc_dump(LOG_DEBUG);

    return atom;
}

struct atom *new_op0(u8 op_code)
{
    struct atom *atom;
    atom = new_op(op_code, ATOM_TYPE_OP_ARG_NONE, NULL);
    return atom;
}

struct atom *new_exprs(struct expr *arg)
{
    struct atom *atom;

    atom = chunkpool_malloc(&s.atom_pool);
    atom->type = ATOM_TYPE_EXPRS;
    atom->u.exprs = chunkpool_malloc(&s.vec_pool);
    vec_init(atom->u.exprs, sizeof(struct expr*));
    exprs_add(atom, arg);
    return atom;
}

struct atom *exprs_add(struct atom *atom, struct expr *arg)
{
    if(atom->type != ATOM_TYPE_EXPRS)
    {
        LOG(LOG_ERROR, ("can't add expr to atom of type %d\n", atom->type));
        exit(1);
    }
    vec_push(atom->u.exprs, &arg);
    return atom;
}

struct atom *exprs_to_byte_exprs(struct atom *atom)
{
    if(atom->type != ATOM_TYPE_EXPRS)
    {
        LOG(LOG_ERROR, ("can't convert atom of type %d to byte exprs.\n",
                        atom->type));
        exit(1);
    }
    atom->type = ATOM_TYPE_BYTE_EXPRS;

    pc_add(vec_size(atom->u.exprs));
    return atom;
}

struct atom *exprs_to_word_exprs(struct atom *atom)
{
    if(atom->type != ATOM_TYPE_EXPRS)
    {
        LOG(LOG_ERROR, ("can't convert exprs of type %d to word exprs.\n",
                        atom->type));
        exit(1);
    }
    atom->type = ATOM_TYPE_WORD_EXPRS;

    pc_add(vec_size(atom->u.exprs) * 2);
    return atom;
}

struct atom *new_res(struct expr *len, struct expr *value)
{
    struct atom *atom;

    atom = chunkpool_malloc(&s.atom_pool);
    atom->type = ATOM_TYPE_RES;
    atom->u.res.length = len;
    atom->u.res.value = value;

    pc_add_expr(len);
    return atom;
}

struct atom *new_incbin(const char *name,
                        struct expr *skip, struct expr *len)
{
    struct atom *atom;
    long length;
    i32 len32;
    i32 skip32;
    struct buf *in;

    /* find out how long the file is */
    in = get_named_buffer(&s.named_buffer, name);
    length = buf_size(in);

    skip32 = 0;
    if(skip != NULL)
    {
        skip32 = resolve_expr(skip);
    }
    if(skip32 < 0)
    {
        skip32 += length;
    }
    if(skip32 < 0 || skip32 > length)
    {
        LOG(LOG_ERROR,
            ("Can't read from offset %d in file \"%s\".\n", skip32, name));
        exit(1);
    }
    length -= skip32;

    len32 = 0;
    if(len != NULL)
    {
        len32 = resolve_expr(len);
    }
    if(len32 < 0)
    {
        len32 += length;
    }
    if(len32 < 0 || len32 > length)
    {
        LOG(LOG_ERROR,
            ("Can't read %d bytes from offset %d from file \"%s\".\n",
             len32, skip32, name));
        exit(1);
    }

    atom = chunkpool_malloc(&s.atom_pool);
    atom->type = ATOM_TYPE_BUFFER;
    atom->u.buffer.name = name;
    atom->u.buffer.length = len32;
    atom->u.buffer.skip = skip32;

    if(len != NULL)
    {
        pc_add(len32);
    }
    return atom;
}


void asm_error(const char *msg)
{
    LOG(LOG_ERROR, ("Error: %s\n", msg));
    exit(1);
}

void asm_echo(const char *msg, struct atom *atom)
{
    struct vec_iterator i;
    struct expr **exprp;
    int count = 0;
    i32 e[10];

    if(atom != NULL)
    {
        if(atom->type != ATOM_TYPE_EXPRS || vec_size(atom->u.exprs) > 10)
        {
            LOG(LOG_ERROR, ("echo arguments must be a string followed by none "
                            "or at most ten expressions.\n"));
            exit(1);
        }

        vec_get_iterator(atom->u.exprs, &i);
        while((exprp = vec_iterator_next(&i)) != NULL)
        {
            e[count++] = resolve_expr(*exprp);
        }
    }
    for(; count < 10; ++count)
    {
        e[count] = 0;
    }
    fprintf(stdout, msg, e[0], e[1], e[2], e[3],
            e[4], e[5], e[6], e[7], e[8], e[9]);
    fprintf(stdout, "\n");
}

void asm_include(const char *msg)
{
    struct buf *src;

    src = get_named_buffer(&s.named_buffer, msg);
    asm_src_buffer_push(src);
}

void initial_symbol_dump(int level, const char *symbol)
{
    i32 value;
    struct expr *expr;

    expr = map_get(&s.initial_symbols, symbol);
    if(expr != NULL)
    {
        value = resolve_expr(expr);
        LOG(level, ("symbol \"%s\" resolves to %d ($%04X)\n",
                    symbol, value, value));
    }
    else
    {
        if(map_contains_key(&s.initial_symbols, symbol))
        {
            LOG(level, ("symbol \"%s\" defined but has no value\n", symbol));
        }
        else
        {
            LOG(level, ("symbol \"%s\" not found\n", symbol));
        }
    }
}

int resolve_symbol(const char *symbol, int *has_valuep, i32 *valuep)
{
    int found = 0;
    int has_value = 0;
    i32 value = 0;
    struct expr *e = NULL;
    const char *p;

    p = find_symref(symbol, &e);
    if(p == NULL)
    {
        if(e != NULL)
        {
            value = resolve_expr(e);
            has_value = 1;
        }
        found = 1;
    }
    if(found)
    {
        if(has_valuep != NULL)
        {
            *has_valuep = has_value;
        }
        if(has_value && valuep != NULL)
        {
            *valuep = value;
        }
    }
    return found;
}

void symbol_dump_resolved(int level, const char *symbol)
{
    int has_value;
    i32 value;

    if(resolve_symbol(symbol, &has_value, &value))
    {
        if(has_value)
        {
            LOG(level, ("symbol \"%s\" resolves to %d ($%04X)\n",
                        symbol, value, value));
        }
        else
        {
            LOG(level, ("symbol \"%s\" is defined but has no value\n",
                        symbol));
        }
    }
    else
    {
        LOG(level, ("symbol \"%s\" not found\n", symbol));
    }
}

void output_atoms(struct buf *out, struct vec *atoms)
{
    struct vec_iterator i;
    struct vec_iterator i2;
    struct atom **atomp;
    struct atom *atom;
    struct expr **exprp;
    struct expr *expr;
    struct buf *in;
    const char *p;
    i32 value;
    i32 value2;

    dump_sym_table(LOG_DEBUG, &s.sym_table);

    vec_get_iterator(atoms, &i);
    while((atomp = vec_iterator_next(&i)) != NULL)
    {
        atom = *atomp;

        LOG(LOG_DEBUG, ("yadda\n"));

        switch(atom->type)
        {
        case ATOM_TYPE_OP_ARG_NONE:
            LOG(LOG_DEBUG, ("output: $%02X\n", atom->u.op.code));
            buf_append_char(out, atom->u.op.code);
            break;
        case ATOM_TYPE_OP_ARG_U8:
            /* op with argument */
            value = resolve_expr(atom->u.op.arg);
            if(!is_valid_u8(value))
            {
                LOG(LOG_ERROR, ("value %d out of range for op $%02X @%p\n",
                                value, atom->u.op.code, (void*)atom));
                exit(1);
            }
            LOG(LOG_DEBUG, ("output: $%02X $%02X\n",
                            atom->u.op.code, value & 255));
            buf_append_char(out, atom->u.op.code);
            buf_append_char(out, value);
            break;
        case ATOM_TYPE_OP_ARG_I8:
            /* op with argument */
            value = resolve_expr(atom->u.op.arg);
            if(!is_valid_i8(value))
            {
                LOG(LOG_ERROR, ("value %d out of range for op $%02X @%p\n",
                                value, atom->u.op.code, (void*)atom));
                exit(1);
            }
            LOG(LOG_DEBUG, ("output: $%02X $%02X\n",
                            atom->u.op.code, value & 255));
            buf_append_char(out, atom->u.op.code);
            buf_append_char(out, value);
            break;
        case ATOM_TYPE_OP_ARG_UI8:
            /* op with argument */
            value = resolve_expr(atom->u.op.arg);
            if(!is_valid_ui8(value))
            {
                LOG(LOG_ERROR, ("value %d out of range for op $%02X @%p\n",
                                value, atom->u.op.code, (void*)atom));
                exit(1);
            }
            LOG(LOG_DEBUG, ("output: $%02X $%02X\n",
                            atom->u.op.code, value & 255));
            buf_append_char(out, atom->u.op.code);
            buf_append_char(out, value);
            break;
        case ATOM_TYPE_OP_ARG_U16:
            /* op with argument */
            value = resolve_expr(atom->u.op.arg);
            if(!is_valid_u16(value))
            {
                LOG(LOG_ERROR, ("value %d out of range for op $%02X @%p\n",
                                value, atom->u.op.code, (void*)atom));
                exit(1);
            }
            value2 = value / 256;
            value = value % 256;
            LOG(LOG_DEBUG, ("output: $%02X $%02X $%02X\n",
                            atom->u.op.code,
                            value, value2));
            buf_append_char(out, atom->u.op.code);
            buf_append_char(out, value);
            buf_append_char(out, value2);
            break;
        case ATOM_TYPE_RES:
            /* reserve memory statement */
            value = resolve_expr(atom->u.res.length);
            if(!is_valid_u16(value))
            {
                LOG(LOG_ERROR, ("length %d for .res(length, value) "
                                "is out of range\n", value));
                exit(1);
            }
            value2 = resolve_expr(atom->u.res.value);
            if(!is_valid_ui8(value2))
            {
                LOG(LOG_ERROR, ("value %d for .res(length, value) "
                                "is out of range\n", value));
                exit(1);
            }
            LOG(LOG_DEBUG, ("output: .RES %d, %d\n", value, value2));
            while(--value >= 0)
            {
                buf_append_char(out, value2);
            }
            break;
        case ATOM_TYPE_BUFFER:
            /* include binary file statement */
            value = atom->u.buffer.skip;
            if(!is_valid_u16(value))
            {
                LOG(LOG_ERROR, ("value %d for .res(length, value) "
                                "is out of range\n", value));
                exit(1);
            }
            value2 = atom->u.buffer.length;
            if(!is_valid_u16(value2))
            {
                LOG(LOG_ERROR, ("length %d for .incbin(name, skip, length) "
                                "is out of range\n", value2));
                exit(1);
            }
            LOG(LOG_DEBUG, ("output: .INCBIN \"%s\", %d, %d\n",
                            atom->u.buffer.name, value, value2));
            in = get_named_buffer(&s.named_buffer, atom->u.buffer.name);
            p = buf_data(in);
            p += value;
            while(--value2 >= 0)
            {
                buf_append_char(out, *p++);
            }
            break;
        case ATOM_TYPE_WORD_EXPRS:
            vec_get_iterator(atom->u.exprs, &i2);
            while((exprp = vec_iterator_next(&i2)) != NULL)
            {
                expr = *exprp;
                value = resolve_expr(expr);
                if(!is_valid_ui16(value))
                {
                    LOG(LOG_ERROR, ("value %d for .word(value, ...) "
                                    "is out of range\n", value));
                }
                value2 = value / 256;
                value = value % 256;
                buf_append_char(out, value);
                buf_append_char(out, value2);
            }
            LOG(LOG_DEBUG, ("output: %d words\n", vec_size(atom->u.exprs)));
            break;
        case ATOM_TYPE_BYTE_EXPRS:
            vec_get_iterator(atom->u.exprs, &i2);
            while((exprp = vec_iterator_next(&i2)) != NULL)
            {
                expr = *exprp;
                value = resolve_expr(expr);
                if(!is_valid_ui8(value))
                {
                    LOG(LOG_ERROR, ("value %d for .byte(value, ...) "
                                    "is out of range\n", value));
                }
                buf_append_char(out, value);
            }
            LOG(LOG_DEBUG, ("output: %d bytes\n", vec_size(atom->u.exprs)));
            break;
        default:
            LOG(LOG_ERROR, ("invalid atom_type %d @%p\n",
                            atom->type, (void*)atom));
            exit(1);
        }
    }
}

static int expr_cmp_cb(const void *a, const void *b)
{
    int result = 0;
    i32 e1v = resolve_expr((struct expr*)a);
    i32 e2v = resolve_expr((struct expr*)b);

    if(e1v < e2v)
    {
        result = -1;
    }
    else if(e1v > e2v)
    {
        result = 1;
    }
    return result;
}

static int loopDetect(struct vec *guesses_history)
{
    int result = 0;
    struct vec_iterator i;
    struct map *m;
    for(vec_get_iterator(guesses_history, &i);
        (m = vec_iterator_next(&i)) != NULL;)
    {
        if(map_equals(m, s.guesses, expr_cmp_cb))
        {
            result = 1;
            break;
        }
    }
    return result;
}

static int wasFinalPass(void)
{
    int result = 1;
    struct map_iterator i;
    struct map_entry *me;
    for(map_get_iterator(s.guesses, &i);
        (me = (struct map_entry*)map_iterator_next(&i)) != NULL;)
    {
        struct expr *guess_expr;
        struct expr *sym_expr;

        LOG(LOG_VERBOSE, ("Checking guessed symbol %s: ", me->key));
        /* Was this guessed symbol used in this pass? */
        if((sym_expr = map_get(&s.sym_table, me->key)) == NULL)
        {
            /* No, skip it */
            continue;
        }
        guess_expr = me->value;
        LOG(LOG_VERBOSE, ("expected %d, actual %d\n",
             resolve_expr(guess_expr),
             resolve_expr(sym_expr)));

        if(expr_cmp_cb(me->value, sym_expr) != 0)
        {
            /* Not the same, not the last pass.
             * copy the actual value from the sym table */
            me->value = sym_expr;
            result = 0;
        }
    }
    return result;
}

int assemble(struct buf *source, struct buf *dest)
{
    struct vec guesses_history;
    struct map guesses_storage;
    int dest_pos;
    int result;

    dump_sym_table(LOG_DEBUG, &s.initial_symbols);

    vec_init(&guesses_history, sizeof(struct map));
    s.guesses = NULL;
    dest_pos = buf_size(dest);
    for(;;)
    {

        map_put_all(&s.sym_table, &s.initial_symbols);
        named_buffer_copy(&s.named_buffer, &s.initial_named_buffer);
        map_init(&guesses_storage);

        if(s.guesses != NULL)
        {
            /* copy updated guesses from latest pass */
            map_put_all(&guesses_storage, s.guesses);
        }
        s.guesses = &guesses_storage;

        result = assembleSinglePass(source, dest);
        if(result != 0)
        {
            /* the assemble pass failed */
            break;
        }

        /* check if any guessed symbols was wrong and update them
         * to their actual value */
        if(wasFinalPass())
        {
            /* The assemble pass succeeded without any wrong guesses,
             * we're done */
            break;
        }
        if(loopDetect(&guesses_history))
        {
            /* More passes would only get us into a loop */
            LOG(LOG_VERBOSE, ("Aborting due to loop.\n"));
            result = -1;
            break;
        }

        LOG(LOG_VERBOSE, ("Trying another pass.\n"));

        /* allocate storage for the guesses in the history vector */
        s.guesses = vec_push(&guesses_history, s.guesses);

        parse_reset();
        buf_remove(dest, dest_pos, -1);
    }
    map_free(&guesses_storage);
    vec_free(&guesses_history, (cb_free*)map_free);
    s.guesses = NULL;
    return result;
}

/*
 * Copyright (c) 2003, 2013 Magnus Lind.
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
#include <stdlib.h>
#include "bprg.h"
#include "vec.h"
#include "log.h"

/* data */
#define TOK_DATA  '\x83'

/* line numbers */
#define TOK_GOTO  '\x89'
#define TOK_RUN   '\x8A'
#define TOK_GOSUB '\x8D'
#define TOK_REM   '\x8F'
#define TOK_LIST  '\x9B'
#define TOK_TO    '\xA4'
#define TOK_THEN  '\xA7'
#define TOK_GO    '\xCB'

#define TOKSTR_NUMBER "\x89\x8A\x8D\x8F\x9B\xA7\xCB"

struct goto_fixup {
    unsigned char *where;
    int old_line;
    int from_line;
};

struct goto_target {
    int old_line;
    int new_line;
};

struct renumber_ctx {
    struct vec fixups;
    struct vec targets;
    struct vec_iterator i;
    struct goto_fixup *fixup;
    int line;
    int step;
    int mode;
};

static int goto_target_cb_cmp(const void *a, const  void *b)
{
    int val = 0;
    struct goto_target *a1 = (struct goto_target*)a;
    struct goto_target *b1 = (struct goto_target*)b;

    if (a1->old_line < b1->old_line)
    {
        val = -1;
    } else if (a1->old_line > b1->old_line)
    {
        val = 1;
    }
    return val;
}

static void
goto_fixup_add(struct bprg_ctx *ctx,
               struct renumber_ctx *rctx,
               int line,
               char **p)
{
    struct goto_fixup fixup;
    struct goto_target target;

    unsigned short number;
    char *n;
    char *n1;

    n = *p;
    do {

        /* eat spaces */
        while(*n == ' ') ++n;

        n1 = n;
        if(*n == '\0') break;

        number = strtol(n, &n1, 10);

        if (n == n1)
        {
            if(n[-1] != TOK_THEN)
            {
                /* error, should not happen */
                LOG(LOG_VERBOSE,
                    ("can't convert $%02X at line %d \"%s\" to line number\n",
                     *n, line, n));
            }
            break;
        }

        /* register the goto into the fixup vec */
        fixup.where = (unsigned char *)n;
        fixup.old_line = number;
        fixup.from_line = line;

        LOG(LOG_VERBOSE, ("adding fixup line %d\n", line));
        vec_push(&rctx->fixups, &fixup);

        target.old_line = number;
        target.new_line = -1;

        vec_insert_uniq(&rctx->targets, goto_target_cb_cmp, &target, NULL);

        /* always add a reverse mapping is not optimal
         * but an optimal approach would include graph traversal.. */
        target.old_line = line;
        target.new_line = -1;

        vec_insert_uniq(&rctx->targets, goto_target_cb_cmp, &target, NULL);

        n = n1;

        /* eat spaces */
        while(*n == ' ') ++n;

        if(*n != ',') break;

        /* *n == ',' here
    int max_diff;
    int diff;
    int traits_used = 0; */
        ++n;

    } while (1);

    *p = n;
}

static void
goto_scan(struct bprg_ctx *ctx,
          struct renumber_ctx *rctx)
{
    struct bprg_iterator i;
    struct brow *b;

    bprg_get_iterator(ctx, &i);

    while(bprg_iterator_next(&i, &b))
    {
        const char *accept[3] = {TOKSTR_NUMBER "\"", "\"", ""};
        int line;

        char *p;
        int quote = 0;

        LOG(LOG_DUMP,
            ("scanning line \"%s\"\n", b->row + 4));

        p = (char*)b->row;

        line = (unsigned char)p[2] | ((unsigned char)p[3] << 8);

        /* skip link and number */
        p += 4;

        while ((p = strpbrk(p, accept[quote])) != NULL)
        {
            char tok = *(p++);
            switch (tok) {
            case '\"':
                /* toggle quote mode */
                quote ^= 1;
                break;
            case TOK_REM:
                /* skip this line, the rest is an rem statement */
                LOG(LOG_DUMP, ("skipping rem\n"));
                quote = 2;
                break;
            case TOK_GO:
                while (*p == ' ') ++p;
                if(*p != TOK_TO)
                {
                    LOG(LOG_WARNING, ("found GO without TO, skipping\n"));
                    break;
                }
                ++p;
                goto_fixup_add(ctx, rctx, line, &p);
                break;
            case TOK_GOSUB:
                LOG(LOG_DUMP, ("found gosub on line %d\n", line));
            default:
                /* we found a basic token that uses line numbers */
                goto_fixup_add(ctx, rctx, line, &p);
                break;
            }
        }
    }
}

static int
renumber1_cb_line_mutate(const unsigned char *in, /* IN */
                         unsigned char *mem, /* IN/OUT */
                         unsigned short *pos, /* IN/OUT */
                         void *priv) /* IN/OUT */
{
    struct renumber_ctx *rctx;
    struct goto_target target_key;
    struct goto_target *target;
    int start;
    int i;
    int line;

    rctx = priv;

    /* prepare target key */
    target_key.old_line = in[2] | (in[3] << 8);
    /* update targets vector with the new line number */
    target = vec_find2(&rctx->targets, goto_target_cb_cmp, &target_key);

    line = 0;
    if(target != NULL || rctx->mode == 0)
    {
        line = rctx->line;
    }
    if(target != NULL)
    {
        target->new_line = line;
    }

    LOG(LOG_DUMP, ("renumbering line %d to %d (target %p)\n",
                   target_key.old_line, line, (void*)target));

    if(target != NULL || rctx->mode == 0)
    {
        /* update line number for next line */
        rctx->line += rctx->step;
    }

    start = *pos;
    i = 0;

    /* copy link */
    mem[start + i++] = *(in++);
    mem[start + i++] = *(in++);
    /* skip number */
    i += 2; in += 2;
    /* copy line including terminating '\0' */
    while((mem[start + i++] = *in++) != '\0');
    *pos = start + i;

    /* the actual renumbering of this row */
    mem[start + 2] = line;
    mem[start + 3] = line >> 8;

    return 0;
}

static int
renumber2_cb_line_mutate(const unsigned char *in, /* IN */
                         unsigned char *mem, /* IN/OUT */
                         unsigned short *pos, /* IN/OUT */
                         void *priv) /* IN/OUT */
{
    struct renumber_ctx *rctx;
    int start;
    int i;
    unsigned char c;

    rctx = priv;


    start = *pos;
    i = 0;

    /* skip link */
    i += 2;
    in += 2;
    /* copy number */
    mem[start + i++] = *(in++);
    mem[start + i++] = *(in++);
    /* copy line including terminating '\0' */
    do {
        if (rctx->fixup != NULL && rctx->fixup->where == in)
        {
            /* hoaa, fixup reference */
            struct goto_target target_key;
            struct goto_target *target;

            LOG(LOG_DUMP, ("found fixup goto %u at %p\n",
                           rctx->fixup->old_line, (void*)in));

            target_key.old_line = rctx->fixup->old_line;
            target = vec_find2(&rctx->targets,
                               goto_target_cb_cmp,
                               &target_key);
            if(target == NULL)
            {
                LOG(LOG_ERROR, ("found fixup has no target \n"));
                exit(1);
            }
            if(target->new_line == -1)
            {
                LOG(LOG_WARNING,
                    ("warning at line %d: nonexisting line %d "
                     "is renumbered to %d\n",
                     rctx->fixup->from_line,
                     target->old_line,
                     target->new_line));
            }

            /* write new line number */
            i += sprintf((char*)mem + start + i, "%d", target->new_line);
            /* skip old in input */
            strtol((char*)in, (void*)&in, 10);

            /* set where to next fixup */
            rctx->fixup = vec_iterator_next(&rctx->i);
        }
        /* copy byte normally */
        c = *(in++);
        mem[start + i++] = c;
    } while(c != '\0');

    /* set link properly */
    mem[start] = start + i;
    mem[start + 1] = (start + i) >> 8;

    *pos = start + i;
    /* success */
    return 0;
}

void
bprg_renumber(struct bprg_ctx *ctx,
              int start,
              int step,
              int mode)
{
    struct renumber_ctx rctx;

    vec_init(&rctx.fixups, sizeof(struct goto_fixup));
    vec_init(&rctx.targets, sizeof(struct goto_target));

    goto_scan(ctx, &rctx);

    rctx.line = start;
    rctx.step = step;
    rctx.mode = mode;

    /* just renumber the lines */
    bprg_lines_mutate(ctx, renumber1_cb_line_mutate, &rctx);

    vec_get_iterator(&rctx.fixups, &rctx.i);
    rctx.fixup = vec_iterator_next(&rctx.i);

    /* now fixup line references */
    bprg_lines_mutate(ctx, renumber2_cb_line_mutate, &rctx);

    vec_free(&rctx.targets, NULL);
    vec_free(&rctx.targets, NULL);
}

static int
rem_cb_line_mutate(const unsigned char *in, /* IN */
                   unsigned char *mem, /* IN/OUT */
                   unsigned short *pos, /* IN/OUT */
                   void *priv) /* IN/OUT */
{
    int start;
    int i;
    char c;
    int quote;
    int data;
    int line;

    struct renumber_ctx *rctx;
    rctx = priv;

    start = *pos;
    i = 0;

    /* skip link */
    i += 2;
    in += 2;
    /* copy number */
    line = *(in++);
    line |= (*(in++) << 8);
    mem[start + i++] = line;
    mem[start + i++] = (line >> 8);

    /* copy line including terminating '\0' */
    quote = 0;
    data = 0;
    do {
        c = *(in++);
        switch(c) {
        case ' ':
            if(quote) break;

            /* in data statements we can only ignore unquoted spaces
             * if  they come immediately after a comma or the DATA token*/
            if(data &&
               mem[start + i - 1] != ',' &&
               (char)mem[start + i - 1] != TOK_DATA)
            {
                break;
            }

            /* skip this byte */
            continue;
            break;
        case ':':
            if(quote) break;
            data = 0;
            break;
        case '\"':
            quote ^= 1;
            break;
        case TOK_DATA:
            data = 1;
            break;
        case TOK_REM:
            if(quote) break;
            if(i == 4)
            {
                /* rem at the beginning of a line */
                struct goto_target target_key;
                struct goto_target *target;
                target_key.old_line = line;
                target = vec_find2(&rctx->targets,
                                   goto_target_cb_cmp,
                                   &target_key);
                if(target == NULL)
                {
                    /* skip this line entirely */
                    return 0;
                }
                /* line is a got target, we can't remove it but
                 * we can remove the rem text */
                mem[start + i++] = c;

            } else if(i > 4 && mem[start + i - 1] == ':')
            {
                --i;
            }
            c = '\0';
            break;
        }
        mem[start + i++] = c;

    } while(c != '\0');

    /* set link properly */
    mem[start] = start + i;
    mem[start + 1] = (start + i) >> 8;

    *pos = start + i;
    /* success */
    return 0;
}

void
bprg_rem_remove(struct bprg_ctx *ctx)
{
    struct renumber_ctx rctx;

    vec_init(&rctx.fixups, sizeof(struct goto_fixup));
    vec_init(&rctx.targets, sizeof(struct goto_target));

    goto_scan(ctx, &rctx);

    bprg_lines_mutate(ctx, rem_cb_line_mutate, &rctx);

    vec_free(&rctx.targets, NULL);
    vec_free(&rctx.fixups, NULL);
}

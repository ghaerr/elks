#ifndef ALREADY_INCLUDED_LOG
#define ALREADY_INCLUDED_LOG
#ifdef __cplusplus
extern "C" {
#endif

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

#include <stdio.h>
#include <stdarg.h>

#ifndef __GNUC__
#define  __attribute__(x)  /*NOTHING*/
#endif

enum log_level {
    LOG_MIN = -99,
    LOG_FATAL = -40,
    LOG_ERROR = -30,
    LOG_WARNING = -20,
    LOG_TERSE = -15,
    LOG_BRIEF = -10,
    LOG_NORMAL = 0,
    LOG_VERBOSE = 10,
    LOG_TRACE = 20,
    LOG_DEBUG = 30,
    LOG_DUMP = 40,
    LOG_MAX = 99
};

typedef void log_formatter_f(FILE * out,        /* IN */
                             enum log_level level,      /* IN */
                             const char *context,       /* IN */
                             const char *);     /* IN */

/*
 * this log output function adds nothing
 */
void raw_log_formatter(FILE * out,      /* IN */
                       enum log_level level,    /* IN */
                       const char *context,     /* IN */
                       const char *log);        /* IN */


struct log_output;

struct log_ctx;

struct log_ctx *log_new(void);

/* log_delete closes all added output streams
 * and files except for stdout and stderr
 */
void log_delete(struct log_ctx *ctx);

void log_set_level(struct log_ctx *ctx, /* IN/OUT */
                   enum log_level level);       /* IN */

void log_add_output_stream(struct log_ctx *ctx, /* IN/OUT */
                           enum log_level min,  /* IN */
                           enum log_level max,  /* IN */
                           log_formatter_f * default_f, /* IN */
                           FILE * out_stream);  /* IN */

void log_vlog(struct log_ctx *ctx,      /* IN */
              enum log_level level,     /* IN */
              const char *context,      /* IN */
              log_formatter_f * f,      /* IN */
              const char *printf_str,   /* IN */
              va_list argp);


void log_log_default(const char *printf_str,    /* IN */
                     ...)
    __attribute__((format(printf,1,2)));

/* some helper macros */

extern struct log_ctx *G_log_ctx;
extern enum log_level G_log_level;
extern enum log_level G_log_log_level;
extern int G_log_tty_only;

#define LOG_SET_LEVEL(L) \
do { \
    log_set_level(G_log_ctx, (L)); \
    G_log_level = (L); \
} while(0)

#define LOG_INIT(L) \
do { \
    G_log_ctx = log_new(); \
    log_set_level(G_log_ctx, (L)); \
    G_log_level = (L); \
} while(0)

#define LOG_INIT_CONSOLE(X) \
do { \
    G_log_ctx = log_new(); \
    log_set_level(G_log_ctx, (X)); \
    G_log_level = (X); \
    log_add_output_stream(G_log_ctx, LOG_WARNING, LOG_MAX, NULL, stdout); \
    log_add_output_stream(G_log_ctx, LOG_MIN, LOG_WARNING - 1, NULL, stderr); \
} while(0)

#define LOG_FREE log_delete(G_log_ctx)

#define IS_LOGGABLE(L) (G_log_level >= (L))

#define LOG(L, M) \
do { \
    if(IS_LOGGABLE(L)) { \
        G_log_log_level = (L); \
        G_log_tty_only = 0; \
        log_log_default M; \
    } \
} while(0)

#define LOG_TTY(L, M) \
do { \
    if(IS_LOGGABLE(L)) { \
        G_log_log_level = (L); \
        G_log_tty_only = 1; \
        log_log_default M; \
    } \
} while(0)

void hex_dump(int level, unsigned char *p, int len);

#ifdef __cplusplus
}
#endif
#endif

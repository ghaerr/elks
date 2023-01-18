/* Copyright (C) 1995,1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 *
 * Copyright (C) 2022 TK Chia <@tkchia@mastodon.social>
 */

#include <errno.h>

#define MAXONEXIT 32            /* C90 requires 32 */

typedef void (*vfuncp) (void);

static int atexit_count;
static vfuncp atexit_table[MAXONEXIT];

int atexit(vfuncp ptr)
{
   if (atexit_count >= MAXONEXIT) {
      errno = ENOMEM;
      return -1;
   }
   if (ptr)
      atexit_table[atexit_count++] = ptr;
   return 0;
}

/* NOTE: ensure this priority value is higher (100) than
 * stdio's destructor stdio_close_all (90), do not destroy
 * stdio streams before running atexit( ) termination functions
 */
#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
__attribute__((destructor(100)))
static void atexit_exit_all(void)
{
   int count = atexit_count - 1;

   /* In reverse order */
   for (; count >= 0; count--)
      atexit_table[count]();
}

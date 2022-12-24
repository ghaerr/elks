/* Copyright (C) 1995,1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 *
 * Copyright (C) 2022 TK Chia <@tkchia@mastodon.social>
 */

#include <errno.h>

#define MAXONEXIT 32            /* C90 requires 32 */

typedef void (*vfuncp) (void);

static int __on_exit_count = 0;
static vfuncp __on_exit_table[MAXONEXIT];

int atexit (vfuncp ptr)
{
   if (__on_exit_count >= MAXONEXIT)
   {
      errno = ENOMEM;
      return -1;
   }
   if (ptr)
   {
      __on_exit_table[__on_exit_count++] = ptr;
   }
   return 0;
}

/* NOTE: ensure this priority value is higher than stdio's destructor's ---
 * do not destroy stdio streams before running atexit( ) termination
 * functions */
__attribute__((destructor(100))) static void
__do_exit (void)
{
   int count = __on_exit_count-1;

   /* In reverse order */
   for (; count >= 0; count--)
   {
      __on_exit_table[count]();
   }
}

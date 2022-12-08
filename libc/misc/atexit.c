/* Copyright (C) 1995,1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 *
 * Copyright (C) 2022 TK Chia <@tkchia@mastodon.social>
 */

/*
 * This deals with both the atexit and on_exit function calls
 * 
 * Note calls installed with atexit are called with the same args as on_exit
 * fuctions; the void* is given the NULL value.
 * 
 */

#include <errno.h>
#include <stdbool.h>

#define MAXONEXIT 42		/* C90 requires 32 */

typedef void (*vfuncp) (void);

extern vfuncp _cleanup;
extern void __do_exit (int);

static int __on_exit_count = 0;
static vfuncp __on_exit_table[MAXONEXIT];

int atexit (vfuncp ptr)
{
   if( __on_exit_count < 0 || __on_exit_count >= MAXONEXIT)
   {
      errno = ENOMEM;
      return -1;
   }
   _cleanup = __do_exit;
   if( ptr )
   {
      __on_exit_table[__on_exit_count] = ptr;
      __on_exit_count++;
   }
   return 0;
}

void __do_exit (int rv)
{
   register int count = __on_exit_count-1;
   register vfuncp ptr;
   __on_exit_count = -1;		/* ensure no more will be added */
   _cleanup = 0;			/* Calling exit won't re-do this */

   /* In reverse order */
   for (; count >= 0; count--)
   {
      ptr = __on_exit_table[count];
      (*ptr) ();
   }
}

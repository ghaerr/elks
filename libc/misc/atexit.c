/* Copyright (C) 1995,1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

/*
 * This deals with both the atexit and on_exit function calls
 *
 * Note calls installed with atexit are called with the same args as on_exit
 * fuctions; the void* is given the NULL value.
 *
 */

#include <errno.h>

/* ATEXIT.H */
#define MAXONEXIT 20		/* AIUI Posix requires 10 */

typedef void (*vfuncp) ();

extern vfuncp _cleanup;
extern void __do_exit();

extern struct exit_table
{
   vfuncp called;
   void *argument;
}
__on_exit_table[MAXONEXIT];

extern int __on_exit_count;

/* End ATEXIT.H */

int atexit (vfuncp ptr)
{
   if (__on_exit_count < 0 || __on_exit_count >= MAXONEXIT)
   {
      errno = ENOMEM;
      return -1;
   }
   _cleanup = __do_exit;
   if (ptr )
   {
      __on_exit_table[__on_exit_count].called = ptr;
      __on_exit_table[__on_exit_count].argument = 0;
      __on_exit_count++;
   }
   return 0;
}

#ifdef L_on_exit
int
on_exit(ptr, arg)
vfuncp ptr;
void *arg;
{
   if (__on_exit_count < 0 || __on_exit_count >= MAXONEXIT)
   {
      errno = ENOMEM;
      return -1;
   }
   __cleanup = __do_exit;
   if (ptr )
   {
      __on_exit_table[__on_exit_count].called = ptr;
      __on_exit_table[__on_exit_count].argument = arg;
      __on_exit_count++;
   }
   return 0;
}

#endif

int   __on_exit_count = 0;
struct exit_table __on_exit_table[MAXONEXIT];

void __do_exit (int rv)
{
   register int   count = __on_exit_count-1;
   register vfuncp ptr;
   __on_exit_count = -1;		/* ensure no more will be added */
   _cleanup = 0;			/* Calling exit won't re-do this */

   /* In reverse order */
   for (; count >= 0; count--)
   {
      ptr = __on_exit_table[count].called;
      (*ptr) (rv, __on_exit_table[count].argument);
   }
}

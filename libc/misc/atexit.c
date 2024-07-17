/* Copyright (C) 1995,1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 *
 * Copyright (C) 2022 TK Chia <@tkchia@mastodon.social>
 */

#include <stdlib.h>
#include <errno.h>
#include <sys/rtinit.h>
#include <sys/linksym.h>

#define MAXONEXIT 32            /* C90 requires 32 */

typedef void (*vfuncp) (void);

static int atexit_count;
static vfuncp atexit_table[MAXONEXIT];

/* atexit functions run prior to stdio destructors */
#pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
static DESTRUCTOR(__atexit_fini, _INIT_PRI_ATEXIT);
static void __atexit_fini(void)
{
   int count = atexit_count - 1;

   /* In reverse order */
   for (; count >= 0; count--)
      atexit_table[count]();
}

int atexit(void (*function)(void))
{
   if (atexit_count >= MAXONEXIT) {
      errno = ENOMEM;
      return -1;
   }
   if (function)
      atexit_table[atexit_count++] = function;
   return 0;
}

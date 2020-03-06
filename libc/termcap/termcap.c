/* Work-alike for termcap, plus extra features.
   Copyright (C) 1985, 1986, 1993 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Emacs config.h may rename various library functions such as malloc.  */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <termcap.h>
#include <stdlib.h>
#include <stdio.h>

void
tprint(const char *cap)
{
  char *x = tgetstr (cap, 0);
  register char *y;

  printf ("%s: ", cap);
  if (x)
    {
      for (y = x; *y; y++)
	if (*y <= ' ' || *y == 0177)
	  printf ("\\%0o", *y);
	else
	  putchar (*y);
      free (x);
    }
  else
    printf ("none");
  putchar ('\n');
}


int
main(int argc, char *argv[])
{
  char *term;
  char *buf;

  term = argv[1];
  printf ("TERM: %s\n", term);

  buf = (char *) tgetent (0, term);
  if ((int) buf <= 0)
    {
      printf ("No entry.\n");
      return 0;
    }

  printf ("Entry: %s\n", buf);

  tprint ("cm");
  tprint ("AL");

  printf ("co: %d\n", tgetnum ("co"));
  printf ("am: %d\n", tgetflag ("am"));

  return 0;
}

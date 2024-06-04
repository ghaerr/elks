/* Merge parameters into a termcap entry string.
   Copyright (C) 1985, 1987, 1993 Free Software Foundation, Inc.

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

#include <sys/types.h>
#include "t.h"

/* Emacs config.h may rename various library functions such as malloc.  */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* not HAVE_CONFIG_H */

/* Assuming STRING is the value of a termcap string entry
   containing `%' constructs to expand parameters,
   merge in parameter values and store result in block OUTSTRING points to.
   LEN is the length of OUTSTRING.  If more space is needed,
   a block is allocated with `malloc'.

   The value returned is the address of the resulting string.
   This may be OUTSTRING or may be the address of a block got with `malloc'.
   In the latter case, the caller must free the block.

   The fourth and following args to tparam serve as the parameter values.  */

/* VARARGS 2 */
char *
tparam (char *string, char *outstring, int len, int arg0, int arg1, int arg2, int arg3)
{
#ifdef NO_ARG_ARRAY
  int arg[4];
  arg[0] = arg0;
  arg[1] = arg1;
  arg[2] = arg2;
  arg[3] = arg3;
  return termcap_tparam1(string, outstring, len, NULL, NULL, arg);
#else
  return termcap_tparam1(string, outstring, len, NULL, NULL, &arg0);
#endif
}

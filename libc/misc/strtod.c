#ifndef __HAS_NO_FLOATS__

/*
 * strtod.c - This file is part of the libc-8086 package for ELKS,
 * Copyright (C) 1995, 1996 Nat Friedman <ndf@linux.mit.edu>.
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Apr 2022 made working by Greg Haerr
 */
#include <stdlib.h>
#include <ctype.h>

double
strtod(const char *nptr, char **endptr)
{
  double number;
  double fp_part, fp_divisor;
  int negative = 0;

  /* advance beyond any leading whitespace */
  while (isspace(*nptr))
    nptr++;

  /* check for optional '+' or '-' */
  if (*nptr=='-')
    {
      nptr++;
      negative=1;
    }
  else
    if (*nptr=='+')
      nptr++;

  number=0;
  while (isdigit(*nptr))
    {
      number=number*10+(*nptr-'0');
      nptr++;
    }

  if (*nptr=='.')
    {
      nptr++;
      fp_part = 0;
      fp_divisor = 10.0;
      while (isdigit(*nptr))
	{
	  fp_part += (*nptr++ - '0') / fp_divisor;
	  fp_divisor *= 10.0;
	}
      number+=fp_part;
    }

  if (*nptr=='e' || *nptr=='E')
    {
      int exponent = 0;
      int exp_negative = 0;

      nptr++;
      if (*nptr=='-')
	{
	  nptr++;
	  exp_negative=1;
	}
      else
	if (*nptr=='+')
	  nptr++;

      while (isdigit(*nptr))
	{
	  exponent = exponent * 10 + (*nptr++ - '0');
	}
      while (exponent)
        {
          if (exp_negative)
	    number/=10;
          else
	    number*=10;
          exponent--;
        }
    }

  if (endptr)
	*endptr = (char *)nptr;

  return (negative ? -number:number);
}
#endif

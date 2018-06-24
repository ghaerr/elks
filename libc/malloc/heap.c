/* Copyright (C) 1995,1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

#include <errno.h>
#include <unistd.h>

//-----------------------------------------------------------------------------

void * sbrk (intptr_t increment)
{
   void * new_brk;
   if (_sbrk (increment, &new_brk))
	   return (void *) -1;

   return new_brk;
}

//-----------------------------------------------------------------------------

// TODO: simplify

int brk (void * addr)
{
   return _brk (addr);
}

//-----------------------------------------------------------------------------

/* Copyright (C) 1995,1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

#include <errno.h>
#include <unistd.h>

/****************************************************************************/

#ifdef L_errno
int errno = 0;	/* libc error value */
#endif

/****************************************************************************/

#ifdef __AS386_16__

#ifdef L___brk_addr
#asm
.data
export brk_addr
brk_addr: .word __end	! This holds the current return for sbrk(0)
.text
#endasm
#endif

/****************************************************************************/

#ifdef L_sbrk
int sbrk(brk_off)
int brk_off;
{
#asm
  mov	bx,sp
#if !__FIRST_ARG_IN_AX__
  mov	ax,[bx+2]	! Fetch the requested value
#endif
  test	ax,ax
  jnz	has_change
  mov	ax,[brk_addr]	! Simple one, read current - can`t fail.
  jmp	eof

has_change:
  js	go_down
  add	ax,[brk_addr]	! Goin up!
  jc	Enomem
  sub	bx,#511		! Safety space 512 bytes
  cmp	bx,ax		! Too close ?
  jb	Enomem

sbrk_ok:
#if !defined(__MSDOS__) && !defined(__STANDALONE__)
  push	ax 		! MSDOS `kernel` doesn`t care
  call	___brk		! Tell the kernel
  test	ax,ax
  pop	ax		! ASSUME ___brk doesn`t alter stack!
  jnz	Enomem		! Ugh! kernel didn`t like the idea!
#endif
  xchg	[brk_addr],ax	! Save away new val
  jmp	eof		! Return it
go_down:
  add	ax,[brk_addr]
  jnc	Enomem
  cmp	ax,#__end
  jae	sbrk_ok

Enomem:
  mov	ax,#12		! This should be ENOMEM not a magic.
  mov	[_errno],ax
  mov	ax,#-1
eof:
#endasm
}
#endif

/****************************************************************************/

#ifdef L_brk
int
brk(new_brk)
char * new_brk;
{
#asm
  mov	bx,sp
#if !__FIRST_ARG_IN_AX__
  mov	ax,[bx+2]	! Fetch the requested value
#endif
  sub	bx,#512		! Safety space 512 bytes
  cmp	bx,ax		! Too close ?
  jb	Enomem
  cmp	ax,#__end
  jae	brk_ok
Enomem:
  mov	ax,#12		! This should be ENOMEM not a magic.
  mov	[_errno],ax
  mov	ax,#-1
  ret
brk_ok:
#if !defined(__MSDOS__) && !defined(__STANDALONE__)
  push	ax
  call	___brk		! Tell the kernel
  test	ax,ax
  pop	bx		! ASSUME ___brk doesn`t alter stack!
  jnz	Enomem		! Ugh! kernel didn`t like the idea!
  mov	[brk_addr],bx	! Save away new val
#else
  mov	[brk_addr],ax	! MSDOS `kernel` doesn`t care
  mov	ax,#0
#endif
#endasm
}
#endif

#endif

//-----------------------------------------------------------------------------

char * __brk_addr = 0;	/* This holds the current return for sbrk(0) */

__brk_addr_init()
{
   if( __brk_addr == 0 && (__brk_addr = _brk(0)) == 0 )
   {
      errno = ENOMEM;
      return -1;
   }
   return 0;
}

//-----------------------------------------------------------------------------

void * sbrk (intptr_t increment)
{
   char * new_brk;
   if( __brk_addr_init() ) return (char*)-1;
   if( increment == 0 ) return __brk_addr;

   new_brk = __brk_addr + increment;
   __brk_addr = _brk(new_brk);
   if( __brk_addr != new_brk )
   {
      errno = ENOMEM;
      return (char*)-1;
   }
   return __brk_addr - increment;
}

//-----------------------------------------------------------------------------

int brk (void * addr)
{
   if( __brk_addr_init() ) return -1;

   __brk_addr = _brk(addr);
   if( __brk_addr == addr ) return 0;
   errno = ENOMEM;
   return -1;
}

//-----------------------------------------------------------------------------

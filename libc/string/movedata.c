#ifdef L_movedata
#include <string.h>

/* NB There isn't any C version of this function ... */

#ifdef BCC_AX_ASM
void
__movedata(srcseg, srcoff, destseg, destoff, len)
unsigned int srcseg, srcoff, destseg, destoff, len;
{
#asm
  push	bp
  mov	bp,sp
  push	si
  push	di
  push	ds
#ifdef PARANOID
  push	es
  cld
#endif

  ! sti			! Are we _really_ paranoid ?

#if !__FIRST_ARG_IN_AX__
  mov	ds,[bp+4]	! Careful, [bp+xx] is SS based.
  mov	si,[bp+6]
  mov	es,[bp+8]
  mov	di,[bp+10]
  mov	cx,[bp+12]
#else
  mov	ds,ax
  mov	si,[bp+4]
  mov	es,[bp+6]
  mov	di,[bp+8]
  mov	cx,[bp+10]
#endif

  ; Would it me a good idea to normalise the pointers ?
  ; How about allowing for overlapping moves ?

  shr	cx,#1	; Do this faster by doing a mov word
  rep
  movsw
  adc	cx,cx	; Retrieve the leftover 1 bit from cflag.
  rep
  movsb

  ! cli			! Are we _really_ paranoid ?

#ifdef PARANOID
  pop	es
#endif
  pop	ds
  pop	di
  pop	si
  pop	bp
#endasm
}
#endif

#endif

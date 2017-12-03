
#define __ASSEMBLY__

#asm
#include "minix.v"

! Must match minix.c ...
#define LOADSEG   (0x1000)

! Must match ELKS
#define ELKS_INITSEG	(0x0100)
#define ELKS_SYSSEG	(0x1000)

org minix_helper

  push	cs
  pop	ds
  xor	ax,ax
  mov	es,ax
  mov	ss,ax
  mov	sp,ax

  mov	cx,#$200		! Move 512 words
  mov	si,ax			! Current load address.
  mov	di,#minix_helper	! To the correct address.
  rep
   movsw

  mov	ds,ax
  jmpi	code,0

msg_p2:
  .asciz	"\r\nLoading ELKS kernel\r\n"

msg_p3:
  .asciz	"Starting ...\r\n"

aint_elks:
  .asciz	"Not an ELKS image!"

elks_name:
  .asciz	"linux"
  .byte		0,0,0,0,0,0,0,0,0

dispmsg:		! SI now has pointer to a message
  lodsb
  cmp	al,#0
  jz	EOS
  mov	bx,#7
  mov	ah,#$E		! Can't use $13 cause that's AT+ only!
  int	$10
  jmp	dispmsg
EOS:
  ret

code:
  mov	si,#msg_p2
  call	dispmsg

  mov	ax,minix_dinode		! In the same directory.
  mov	minix_inode,ax

  mov	cx,#14
  mov	si,#elks_name
  mov	di,#minix_bootfile
  rep
   movsb

  call	minix__loadprog
				! Ok, now loaded "boot/linux" (or so)
  mov	si,#msg_p3
  call	dispmsg

  call	kill_motor		! For kernels without a floppy driver.
!
  mov	ax,#LOADSEG
  mov	ds,ax

  mov	ax,$1E6			! Check for ELKS magic number
  cmp	ax,#$4C45
  jnz	not_elks
  mov	ax,$1E8
  cmp	ax,#$534B
  jz	boot_it
not_elks:
  xor	ax,ax
  mov	ds,ax
  mov	si,#aint_elks
  call	dispmsg
  xor	ax,ax
  int	$16
  jmpi	$0,$FFFF

boot_it:
  mov	ax,#ELKS_INITSEG
  mov	es,ax

  mov	bl,497			! Fetch number of setup sects.
  xor	bh,bh
  inc	bx
  mov	ax,500			! Fetch system size
  mov	cl,#5
  add	ax,#31
  shr	ax,cl
  mov	dx,ax

looping:			! Put the setup where it belongs
  call	copy_sect
  dec	bx
  jnz	looping

  mov	ax,#ELKS_SYSSEG
  mov	es,ax

looping2:			! Put the body code in the right place.
  call	copy_sect
  dec	dx
  jnz	looping2

  ! Ok, everything should be where it belongs call it.
  mov	ax,#ELKS_INITSEG
  mov	ds,ax
  mov	es,ax
  mov	ss,ax
  mov	sp,#0x4000-12
  jmpi	0,ELKS_INITSEG+$20

copy_sect:
  mov	cx,#256
  xor	si,si
  xor	di,di
  rep
   movsw

  mov	ax,ds
  add	ax,#32
  mov	ds,ax
  mov	ax,es
  add	ax,#32
  mov	es,ax
  ret

kill_motor:
  push	dx
  mov	dx,#0x3f2
  xor	al, al
  outb
  pop	dx
  ret

#endasm


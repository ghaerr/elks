/*
 * This bootblock loads the linux-8086 executable in the file 'boot'
 * from the root directory of a minix filesystem.
 *
 * Copyright (C) 1990-1998 Robert de Bath, distributed under the GPL Version 2
 * Based on minix filesystem definitions.
 *
 * TODO:
 *       Alter nogood() to do a mov sp,... so the helper program can override
 *       the panic message.
 */

#include <a.out.h>
#include "minix.h"

/* #define DOTS		/* define to have dots printed */
/* #define HARDDISK	/* Define for hard disk version */
/* #define TRY_FLOPPY	/* To do trial reads to find floppy size */

/* #define MIN_SPACE	   */

#define zone_shift 0	/* for any < 32M (!= 0 not supported yet, if ever) */
#define seg_at(k) ((k)*64)
#define seg_of(p) ((unsigned int)p >>4)
#define BOOTSEG	  (0x07c0)
#define LOADSEG   (0x1000)
#define ORGADDR   (0x0500)

#ifdef HARDDISK
#define get_now()
#endif

#ifdef zone_shift
#if zone_shift == 0
#define load_zone load_block
#endif
#else
static short     zone_shift;
#endif

#asm
BOOTADDR = 0x7c00

.text
! Apparently on startup the only things we can assume are that we start at
! `start` (ABS addr $07C00) and the boot sector is in the segment.

! So first set CS=DS=ES=SS=0
! The we move this to $0500 and put the stack at the top of the first 64k.
! The directory 'file' is loaded $1500 and scanned there.
! The final executable will be loaded in the 2nd 64k chunk.
!
org ORGADDR	! The lowest available address.
start:
#ifndef MIN_SPACE
  include sysboot16.s

org start	! The lowest available address, again.
  j	skip_vars

org dos_sysid
  .ascii "MINIXFS BOOT (C) 1990-1999,  Robert de Bath"

  org codestart
#endif

! A few variables we need to know the positions of for patching, so export
! them and as86_encaps will make some variables. Put them here at the start
! so they're in the same place for both Floppy and harddisk versions as they
! will be used by helper programs.

export	inode			! Inode to search
inode:
_inode: .word	1		! ROOT_INODE

#ifndef MIN_SPACE
export	dinode			! Inode of directory file was found in.
dinode:
_dinode: .word	1		! ROOT_INODE
#endif

export	bootfile		! File to boot, make this whatever you like,
bootfile:			! 'boot' is good, 'linux' too.
_bootfile:
   .ascii	"boot"
   .byte	0,0,0,0,0,0,0,0,0,0

skip_vars:
#ifdef HARDDISK
  mov	bx,[si+8]	! Fetch the linear address of part from DS:SI
  mov	dh,[si+10]	! DL is drive number
#endif

  xor	ax,ax		! All segments are zero, first 64k of mem.
  mov	ds,ax
  mov	es,ax
  mov	ss,ax
  mov	sp,ax

#ifndef HARDDISK
loopy:
  mov	ax,#$0203		! Read 3 sectors, code + superblock.
  mov	bx,#start		! Where this _should_ be
  mov	cx,#$0001		! From sector 1
  xor	dx,dx			! Of the floppy drive head zero
  int	$13
  jc	loopy
#else

  mov	cx,#$100	! Move 256 words
  mov	si,#BOOTADDR	! From default BB
  mov	di,#ORGADDR	! To the correct address.
  rep
   movsw

  xchg	dl,dh
  mov	[bootpart],bx	! Save the partition sector offset (and drive)
  mov	[bootpart+2],dx

  ! Read next 2 sectors of hd.
  xor	dx,dx
  mov	cx,#1
  mov	bx,#ORGADDR+$200
  mov	al,#2

  call	load_sect
#endif

  jmpi	code,#0

#endasm

/* /* */
/****************************************************************************/
/* Section cdef */
/****************************************************************************/

/* The name of the file and inode to start */
extern char bootfile[];
extern inode_nr inode;
extern inode_nr dinode;

/* For multi-sector reads */
extern sect_nr lastsect;
extern sect_nr firstsect;
extern unsigned loadaddr;
extern unsigned loadcount;

/* Keep track of zones to load */
extern zone_nr * next_zone;
extern zone_nr * end_zone;
extern zone_nr   indirect;

/* Where to load zones */
extern unsigned ldaddr;

/* Directory reading */
extern dir_struct * dirptr;
extern unsigned flength;
extern unsigned dir_32;

#ifndef HARDDISK
/* The 'shape' of the floppy - intuit from superblock or try to read max */
extern unsigned n_sectors;
#endif

extern struct super_block b_super;
extern d_inode            b_inode[INODES_PER_BLOCK];
extern zone_nr            b_zone[NR_INDIRECTS];
extern dir_struct         directory[];

/* /* */
/****************************************************************************/
/* Section adef */
/****************************************************************************/

#asm
.text

#ifdef HARDDISK
bootpart:   .long	0
#else
_loadcount: .word	0
_firstsect: .word	0
_loadaddr:  .word	0
_lastsect:  .word	0
#endif

  block start+0x400
_b_super:	.blkb 512

#ifndef MIN_SPACE
export helper
helper:		.blkb 1024
export helper_end
helper_end:
#endif

_b_inode:	.blkb 1024
_b_zone:	.blkb 1024

#ifdef MIN_SPACE
temp_space:	.blkb 512
#endif
probe_buf:
_directory:	.blkb 32768
  endb

#endasm

/* /* */
/****************************************************************************/
/* Section nogood */
/****************************************************************************/
/* #if defined(HARDDISK) && !defined(SKIPBOOT) */
#ifndef SKIPBOOT
static
nogood()
{
#asm
  mov	si,#fail_fs
min_nextc:
  lodsb
  cmp	al,#0
  jz	min_eos
  mov	bx,#7
  mov	ah,#$E		! Can't use $13 cause that's AT+ only!
  int	$10
  jmp	min_nextc
min_eos:		! Wait for a key then reboot
  xor	ax,ax
  int	$16
  jmpi	$0,$FFFF	! Reboot.

fail_fs:
  .byte		13,10
#if defined(HARDDISK)
  .asciz	"Initial boot failed, press return to reboot\r\n"
#else
  .asciz	"Boot failed:"
#endif
#endasm
}

#else

static
nogood()
{
/* This didn't work, chain the boot sector of the HD */
#asm
  push	cs
  pop	es
hcode:
  mov	ax,#$0201		! Read 1 sector
  mov	bx,#BOOTADDR		! In the boot area
  mov	cx,#$0001		! From sector 1
  mov	dx,#$0080		! Of the hard drive head zero
  int	$13
  jc	hcode			! Keep trying forever!
  jmpi	BOOTADDR,0
#endasm
}
#endif

/* /* */
/****************************************************************************/
/* Section hd_sect */
/****************************************************************************/
#ifdef HARDDISK
#asm
!
! Load AL sectors from linear sector DX:CX into location ES:BX
! Linear sector zero is at [bootpart]
! This loads one sector at a time, but that's OK cause even in the _very_
! worst case it'll take no more that 5 seconds to load a 16 bit executable.
!
load_sect:
  add	cx,[bootpart]
  adc	dx,[bootpart+2]
moresect:
  cmp	al,#0
  jnz	onesect
  clc
  ret

! Load one sector...
onesect:
  push	ax	! Save lots
  push	di
  push	si
  push	cx	! Drive and sector.
  push	dx

  push	es	! Load location
  push	bx

  push	cx	! Drive and sector again.
  push	dx

  ! Fetch drive 'shape'
  mov	ah,#8
  mov	dl,dh
  int	$13	! DX:CX = drive specification
  jc	_nogood

  and	cx,#$3F	! Get sector count => DI
  mov	di,cx

  xor	dl,dl	! Get head count  => SI
  xchg	dl,dh
  inc	dx
  mov	si,dx

  pop	dx	! Get back drive and sector
  pop	ax

  mov	bl,dh	! Save drive
  xor	dh,dh

  div	di	! DX=sector, AX=track number
  mov	cx,dx
  inc	cl	! CL=sector number

  xor	dx,dx
  div	si	! DX=head, AX=cylinder

  mov	dh,dl
  mov	dl,bl	! DX for int 1302

  xchg	al,ah
  ror	al,#1
  ror	al,#1
  or	cx,ax	! CX for int 1302

  pop	bx	! ES:BX for int 1302
  pop	es

  mov	di,#5	! Lots of retries for a hd
retry:
  mov	ax,#$0201
  int	$13
  jnc	got_hd_sect

  xor	ax,ax	! Reset between each try.
  int	$13

  dec	di
  jnz	retry
  br	_nogood

got_hd_sect:
  pop	dx
  pop	cx
  pop	si
  pop	di
  pop	ax

  dec	al
  add	cx,#1
  adc	dh,#0
  add	bh,#2
  jmp	moresect
#endasm
#endif

/****************************************************************************/
/* This is the end of the parts that MUST be in the first sector            */
/* From here down the functions can safely be in any order.                 */
/****************************************************************************/

/* /* */
/****************************************************************************/
/* Section fd_block */
/****************************************************************************/
#ifndef HARDDISK
static
load_block(address, blkno)
unsigned address, blkno;
{
   register sect_nr sectno;
   if(blkno == 0) { zero_block(address); return; }
#ifdef DOTS
   prt_dot();
#endif

   sectno = (sect_nr)blkno * 2;
   load_sect(address,    sectno);
   load_sect(address+32, sectno+1);
}
#endif

/****************************************************************************/
/* Section fd_bpb */
/****************************************************************************/
#ifndef HARDDISK
#asm
_set_bpb:
#ifdef MIN_SPACE
bios_tabl=temp_space		! Temp space.
bios_disk=temp_space+4		!
#else
bios_tabl=dosfs_stat		! Temp space.
bios_disk=dosfs_stat+4		!
#endif

#ifndef __CALLER_SAVES__
  push	si
  push	di
#endif

  mov	di,#bios_disk
  mov	bx,#0x78
! 0:bx is parameter table address

  push	ds
  push	di

  mov	si,[bx]
  mov	ax,[bx+2]
  mov	[bios_tabl],si
  mov	[bios_tabl+2],ax
  push	ax

  pop	ds
! ds:si is source

! copy 12 bytes
  mov	cl,#6
  cld
  rep
  movsw

  pop	di
  pop	ds
  mov	ax,[_n_sectors]
  movb	4[di],al		! patch sector count

  mov	[bx],di
  mov	2[bx],es

#ifndef __CALLER_SAVES__
  pop	si
  pop	di
#endif
  ret

_unset_bpb:
! 0:0x78 is parameter table address

  mov	ax,[bios_tabl]
  mov	[0x78],ax
  mov	ax,[bios_tabl+2]
  mov	[0x78+2],ax
  ret

#endasm
#endif

/****************************************************************************/
/* Section fd_get_now */
/****************************************************************************/
#ifndef HARDDISK
static
get_now()
{
#asm
  mov	si,#5
retry_get:
  xor	dx,dx
  mov	cx,[_firstsect]
  shr	ch,#1
  adc	dh,#0
  mov	es,[_loadaddr]
  xor	bx,bx
  mov	ax,[_loadcount]
  test	ax,ax
  jz	no_load
  mov	ah,#2
  int	$13		! Try fetch
  jnc	no_load
  xor	ax,ax		! Bad, retry.
  int	$13
  dec	si
  jnz	retry_get
  br	_nogood
no_load:
  xor	ax,ax
  mov	[_loadcount],ax
#endasm
}
#endif

/****************************************************************************/
/* Section fd_probe */
/****************************************************************************/
#ifndef HARDDISK
#ifdef TRY_FLOPPY
#asm
!-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
!
! These are the number of sectors per track that will be scanned for.
! For 3.5 inch floppies 36 is 2.88 Mb, 18 is 1.44Mb, 21 is 1.68Mb on
! a 1.44Mb floppy drive. 15 and 9 are for 5.25 inch floppies.

disksizes: .byte 36,21,18,15,9

! It seems that there is no BIOS call to get the number of sectors.  Guess
! 36 sectors if sector 36 can be read, 18 sectors if sector 18 can be read,
! 15 if sector 15 can be read.  Otherwise guess 9.

_probe_sectors:
	mov	si,#disksizes		! table of sizes to try

probe_loop:
	lodsb
	cbw				! extend to word
	mov	_n_sectors, ax
	cmp	al,#9
	je	got_sectors		! if all else fails, try 9
	xchg	ax, cx			! cx = track and sector
	xor	dx, dx			! drive 0, head 0
	mov	bx,#probe_buf 		! address after setup (es = cs)
	mov	ax,#0x0201		! service 2, 1 sector
	int	0x13
	jc	probe_loop		! try next value
got_sectors:

	ret
#endasm
#else
probe_sectors()
{
   /* Guess the number of sectors based on the size of the file system */
   if( (n_sectors = b_super.s_nzones / 40) > 11 ) n_sectors /= 2;
}
#endif
#endif

/****************************************************************************/
/* Section fd_sect */
/****************************************************************************/
#ifndef HARDDISK
static
load_sect(address, sectno)
unsigned address;
sect_nr sectno;
{
   register sect_nr nsect;

   nsect   =  sectno%n_sectors +1;
   sectno /= n_sectors;
   nsect  |= (sectno<<8);

   if( loadcount )
   {
      lastsect++;
      if( ( address & 4095 ) && nsect == lastsect )
      {
         loadcount++;
         return;
      }
      get_now();
   }

   lastsect  = firstsect = nsect;
   loadaddr  = address;
   loadcount = 1;
}
#endif

/****************************************************************************/
/* Section fd_zeroblk */
/****************************************************************************/
#ifndef HARDDISK
static
zero_block(address)
{
#asm
#if __FIRST_ARG_IN_AX__
  mov	es,ax
#else
  mov	bx,sp
  mov	es,[bx+2]
#endif
  push	di
  mov	cx,#512
  xor	ax,ax
  mov	di,ax
  rep
   stosw
  pop	di
#endasm
}
#endif

/****************************************************************************/
/* Section hd_block */
/****************************************************************************/
#ifdef HARDDISK
/*----------------------------------*/
/* Hard disk block driver           */
/*----------------------------------*/

#asm
_load_block:
  push	bp
  mov	bp,sp
#if __FIRST_ARG_IN_AX__
  ! Fetch load location
  mov	es,ax

  ! Test for block zero
  mov	ax,4[bp]
#else
  ! Fetch load location
  mov	ax,[bp+4]
  mov	es,ax

  ! Test for block zero
  mov	ax,6[bp]
#endif
  test	ax,ax
  jne 	real_block

  ! Iff block zero, zap memory
  push	di
  mov	cx,#512
  xor	ax,ax
  mov	di,ax
  rep
   stosw
  pop	di

func_exit:
  mov	sp,bp
  pop	bp
  ret

real_block:
#ifdef DOTS
  push	ax
  call	_prt_dot
  pop	ax
#endif

! Load a real block.
  mov	cx,ax
  xor	dx,dx
  shl	cx,#1
  rcl	dx,#1

  xor	bx,bx
  mov	al,#2
  call	load_sect

  j	func_exit
#endasm
#endif

/****************************************************************************/
/* Section main */
/****************************************************************************/
#asm
code:
  call	_loadprog
  call	_runprog
  br	_nogood

#endasm

/****************************************************************************/
/* Section prt_dots */
/****************************************************************************/
#ifdef DOTS
#asm
_prt_crlf:
  mov   al,#13
  call	outch
  mov	al,#10
  j	outch
_prt_dot:
  mov   al,#'.
outch:
  mov	ah,#$0E
  mov	bx,#7
  int	$10
  ret
#endasm
#endif

/****************************************************************************/
/* Section end_1 */
/****************************************************************************/
#if defined(HARDDISK) || !defined(MIN_SPACE)
#asm
end_of_part1:
#ifdef HARDDISK
  if *>start+0x1FE	! Leave space for magic
#else
  if *>start+0x200
#endif
     fail! Part 1 too large!
  endif
  .blkb	0x200+start-*
#endasm
#endif

/****************************************************************************/
/* Section prog_load */
/****************************************************************************/

loadprog()
{
#ifdef DOTS
   prt_dot();
#endif
   if( b_super.s_magic == SUPER_MAGIC2 )
      dir_32 = 1;
   else if( b_super.s_magic == SUPER_MAGIC )
      dir_32 = 0;
   else
      nogood();

#ifdef zone_shift
   if( zone_shift != b_super.s_log_zone_size) nogood();
#else
   zone_shift = b_super.s_log_zone_size;
#endif

#ifndef HARDDISK
   probe_sectors();

   /* if( (n_sectors = b_super.s_nzones / 40) > 11 ) n_sectors /= 2; */

   set_bpb();
#endif

try_again:;
   inode--;
   load_block(seg_of(b_inode), inode/INODES_PER_BLOCK
                          + b_super.s_imap_blocks
                          + b_super.s_zmap_blocks
                          + 2);
   get_now();

   ldaddr = LOADSEG;		/* Load at 64k mark */

   {
      register d_inode * i_ptr;
      i_ptr = b_inode + inode%INODES_PER_BLOCK;
      next_zone = i_ptr->i_zone;
      flength   = i_ptr->i_size;
      if( (i_ptr->i_mode & I_TYPE) == I_DIRECTORY )
      {
	 ldaddr = seg_of(directory);
#ifndef MIN_SPACE
	 dinode  = inode+1;	/* Remember current directory */
#endif
	 inode  = 0;		/* Mark - we've no _file_ inode yet */
      }
   }

   end_zone = next_zone+NR_DZONE_NUM;
   load_zone(seg_of(b_zone), (indirect = next_zone[NR_DZONE_NUM]));
   get_now();

   for(;;)
   {
      if( next_zone >= end_zone )
      {
         if( indirect != 0 )
         {
            next_zone = b_zone;
            end_zone  = next_zone + NR_INDIRECTS;
            indirect  = 0;
            continue;
         }
	 break;
      }
      load_zone(ldaddr, *next_zone);
      next_zone++;
      ldaddr += (seg_at(1) << zone_shift);
   }
   get_now();

#ifdef DOTS
   prt_crlf();
#endif
   if(!inode)
   {
      dirptr = directory;
      while(flength > 0)
      {
register char * s = bootfile;
register char * p = dirptr->d_name;

	 if( dirptr->d_inum )
	 {
	    for(;;)
	    {
	       if( *s == '\0')
	       {
		  if(*p == '\0')
		  {
		     inode = dirptr->d_inum;
		     goto try_again;
		  }
		  break;
	       }
	       if( *s++ != *p++ ) break;
	    }
	 }
	 flength -= 16;
	 dirptr++;
	 if( dir_32 )
	 {
	    flength -= 16;
	    dirptr++;
	 }
      }
      nogood();
   }
#ifndef HARDDISK
   unset_bpb();
#endif
}

/****************************************************************************/
/* Section prog_run */
/****************************************************************************/
static
runprog()
{
/* It all worked, run the loaded executable */
#asm
#ifdef HARDDISK
  mov	dx,[bootpart+2]
  xchg	dh,dl		! DX => hard drive
  push	[bootpart]	! CX => partition offset
  xor	si,si
#else
  xor	dx,dx		! DX=0 => floppy drive
  push	dx		! CX=0 => partition offset = 0
  mov	si,[_n_sectors]	! Save for monitor.out
#endif

  mov	bx,#LOADSEG
  mov	ds,bx		! DS = loadaddress
  xor	di,di		! Zero
  mov	ax,[di]
  cmp	ax,#0x0301	! Right magic ?
  jnz	binfile		! Yuk ... assume .SYS
  inc	bx
  inc	bx		! bx = initial CS
  mov	ax,[di+2]
  and	ax,#$20		! Is it split I/D ?
  jz	impure		! No ...
  mov	cl,#4
  mov	ax,[di+8]
  shr	ax,cl
impure:
  pop	cx
  add	ax,bx
  mov	ss,ax
  mov	sp,[di+24]	! Chmem value
  mov	ds,ax
binfile:

  push	bx
  push	di		!  jmpi	0,#LOADSEG+2
  retf
#endasm
}

/****************************************************************************/
/* Section sys_libs */
/****************************************************************************/
#asm
! These functions are pulled from the C library.
libstuff:
imodu:
	xor	dx,dx
	div	bx
	mov	ax,dx		! instruction queue full so xchg slower
	ret
idiv_u:
	xor	dx,dx
	div	bx
	ret
#ifndef zone_shift
isl:
islu:
	mov	cl,bl
	shl	ax,cl
	ret
#endif
libend:
#endasm

/****************************************************************************/
/* Section sys_vars */
/****************************************************************************/
#asm
#ifdef MIN_SPACE
  block	temp_space+64
#endif
vars:
#ifndef HARDDISK
_n_sectors:	.blkw 1
#endif
_next_zone:	.blkw 1
_end_zone:	.blkw 1
_indirect:	.blkw 1
_ldaddr:	.blkw 1
_dirptr:	.blkw 1
_flength:	.blkw 1
_dir_32:	.blkw 1
varend:
#ifdef MIN_SPACE
  endb
#endif
#endasm

/****************************************************************************/
/* Section end_2 */
/****************************************************************************/
#asm
end_of_prog:
  if *>start+0x400
     fail! Part 2 too large!
  endif

  if end_of_prog<start+0x201
  .blkb	0x400+start-*
  else
  .blkb	0x3FF+start-*
  .byte 0xFF
  endif

#endasm

/****************************************************************************/

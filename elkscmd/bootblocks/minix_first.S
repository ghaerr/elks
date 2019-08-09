//------------------------------------------------------------------------------
// MINIX Boot Block (first block of the filesystem)
// Sourced from DEV86 bootblocks
// Reworked for ELKS to load directly the kernel without any helper
// Sector 1 : boot sector and primitives
//------------------------------------------------------------------------------

#include <autoconf.h>

#define BOOTADDR  0x7C00
#define LOADSEG   0x1000

// Must match ELKS
#define ELKS_INITSEG (0x0100)
#define ELKS_SYSSEG  (0x1000)

	.code16

	.text

// Boot block is the first one in the MINIX filesystem
// First part of the boot block (1K) is the boot sector (512 B)
// Loaded by the BIOS at address 0x7C00 (32K - 1K)

sector_1:

	// Get the memory size

	int $0x12   // in KB
	mov $6,%cl  // to paragraphs
	shl %cl,%ax

	// Move to the latest rounded 64K
	// To avoid unaligned buffer for BIOS INT 13h

	sub $0x1000,%ax
	and $0xF000,%ax
	mov %ax,%es

	xor %ax,%ax
	mov %ax,%ds
	mov $BOOTADDR,%si
	xor %di,%di
	mov $256,%cx  // 256 words = 512 B
	cld
	rep
	movsw

	// Rebase CS DS ES SS to work in the 64K segment

	mov %es,%ax
	mov %ax,%ds
	mov %ax,%ss  // automatic CLI for next instruction
	xor %sp,%sp

	push %ax
	mov $_next1,%ax
	push %ax
	retf

_next1:

	mov $msg_head,%bx
	call _puts

	// Load the second sector of the boot block

	mov $0x0201,%ax    // read 1 sector
	mov $sector_2,%bx  // destination
	mov $2,%cx         // track 0 - from sector 2 (base 1)
	xor %dx,%dx        // drive 0 - head 0
	int $0x13          // BIOS disk services
	jnc _next2

	mov $msg_load,%bx
	call _puts

// void reboot ()

	.global reboot

reboot:

	mov $msg_reboot,%bx
	call _puts
	xor %ax,%ax  // wait for key
	int $0x16
	int $0x19
	jmp reboot

_next2:

	.extern main

	/*
	// Not necessary as fixed format
	err = drive_get (0);
	if (err) {
		puts ("drive_get:");
		word_hex (err);
		puts ("\r\n");
		break;
		}
	*/


	call main
	jmp reboot

//------------------------------------------------------------------------------

msg_head:
	.asciz "\r\nMINIX boot\r\n"

msg_load:
	.asciz "Sector 2!\r\n"

msg_reboot:
	.asciz "Press key\r\n\r\n"

//------------------------------------------------------------------------------

_putc:

	mov $7,%bx    // page 0 - color 7
	mov $0xE,%ah  // teletype write
	int $0x10     // BIOS video services
	ret

_puts:

	mov (%bx),%al
	or %al,%al
	jz puts_exit
	mov %bx,%dx
	call _putc
	mov %dx,%bx
	inc %bx
	jmp _puts

puts_exit:
	ret

// void puts (const char * s)

	.global puts

puts:

	mov %sp,%bx
	mov 2(%bx),%bx
	call _puts
	ret

//------------------------------------------------------------------------------

/*
hex_tab:
	.asciz "0123456789ABCDEF"

_quad_hex:

	and $0x0F,%al
	xor %bh,%bh
	mov %al,%bl
	mov hex_tab(%bx),%al
	call _putc
	ret

_byte_hex:

	mov %al,%dl

	mov $4,%cl
	shr %cl,%al
	call _quad_hex

	mov %dl,%al
	call _quad_hex

	ret

_word_hex:

	push %ax
	mov %ah,%al
	call _byte_hex
	pop %ax
	call _byte_hex
	ret

// void word_hex (word_t w)

	.global word_hex

word_hex:

	mov %sp,%bx
	mov 2(%bx),%ax
	call _word_hex
	ret
*/

//------------------------------------------------------------------------------

// int seg_data ()

	.global seg_data

seg_data:

	mov %ds,%ax
	ret

//------------------------------------------------------------------------------

// int drive_reset (int drive)
/*
	.global drive_reset

drive_reset:
	mov %sp,%bx
	mov 2(%bx),%dx  // DL = drive (0 based)
	xor %ah,%ah
	int $0x13
	mov %ah,%al     // AH = status
	xor %ah,%ah
	ret
*/

//------------------------------------------------------------------------------

// Disk geometry (CHS)

	.global sect_max
	.global head_max
	.global track_max

// #undef CONFIG_IMG_FD1680
// #undef CONFIG_IMG_FD1200
// #undef CONFIG_IMG_FD720
// #undef CONFIG_IMG_FD360

sect_max:
#ifdef CONFIG_IMG_FD1440
	.word 18
#endif
#ifdef CONFIG_IMG_FD720
	.word 9
#endif

head_max:
#if defined(CONFIG_IMG_FD1440) || defined(CONFIG_IMG_FD720)
	.word 2
#endif

track_max:
#if defined(CONFIG_IMG_FD1440) || defined(CONFIG_IMG_FD720)
	.word 80
#endif

//------------------------------------------------------------------------------

// int drive_get (int drive)
/*
	.global drive_get

drive_get:

	mov %sp,%bx
	mov 2(%bx),%dx  // DL = drive ( base 0)

	push %es
	push %si

	mov $8,%ah
	int $0x13

	pop %si
	pop %es

	jc dg_error

	mov %ch,%al  // maximum track (base 0)
	mov $0,%ah
	inc %al
	mov %ax,track_max

	mov %cl,%al  // maximum sector (base 1)
	and $0x3F,%al
	mov %ax,sect_max

	mov %dh,%al  // maximum head (base 0)
	inc %al
	mov %ax,head_max

	xor %ax,%ax
	jmp dg_exit

dg_error:

	mov %ah,%al
	mov $0,%ah

dg_exit:

	ret
*/

//------------------------------------------------------------------------------

// int disk_read (int drive, int sect, int count, byte_t * buf, int seg)

	.global disk_read

disk_read:

	push %bp
	mov %sp,%bp
	sub $8,%sp

// -2(%bp) = relative sector
// -4(%bp) = relative head
// -6(%bp) = relative track
// -8(%bp) = relative count

	// Compute the CHS from absolute sector

	mov 6(%bp),%ax   // absolute sector
	xor %dx,%dx
	divw sect_max
	mov %dx,-2(%bp)
	xor %dx,%dx
	divw head_max
	mov %dx,-4(%bp)
	mov %ax,-6(%bp)

	// Read loop

dr_loop:

	// Compute remaining sectors in current track

	mov -2(%bp),%ax
	add 8(%bp),%ax    // absolute count
	cmp sect_max,%ax
	ja dr_over
	mov 8(%bp),%ax
	jmp dr_next1

dr_over:

	mov sect_max,%ax
	sub -2(%bp),%ax

dr_next1:

	mov %ax,-8(%bp)  // relative count

	mov 4(%bp),%dl   // drive number
	mov -4(%bp),%dh  // head number
	mov -2(%bp),%cl  // first sector number
	inc %cl          // base 1 for sector number
	mov -6(%bp),%ch  // cylinder number

	push %es
	mov 10(%bp),%bx
	mov 12(%bp),%es
	mov $0x02,%ah    // BIOS read disk
	int $0x13
	pop %es

	jc dr_error

	// Update absolute count

	mov -8(%bp),%ax
	sub %ax,8(%bp)
	jz dr_end

	// Move to next head

	movw $0,-2(%bp)
	mov -4(%bp),%ax
	inc %ax
	mov %ax,-4(%bp)

	cmp head_max,%ax
	jnz dr_next2

	// Move to next track

	movw $0,-4(%bp)
	mov -6(%bp),%ax
	inc %ax
	mov %ax,-6(%bp)

dr_next2:

	// Advance in buffer

	mov -8(%bp),%ax
	mov $5,%cl
	shl %cl,%ax
	add %ax,12(%bp)

	jmp dr_loop

dr_error:
	mov %ah,%al
	mov $0,%ah
	jmp dr_exit

dr_end:
	xor %ax,%ax

dr_exit:

	mov %bp,%sp
	pop %bp
	ret

//------------------------------------------------------------------------------

// int strcmp (const char *s, const char * d)

	.global strcmp

strcmp:

	mov %sp,%bx
	push %di
	push %si

	mov 2(%bx),%di
	mov 4(%bx),%si

_sc_loop:

	lodsb
	scasb
	jne _sc_diff
	test %al,%al
	jne _sc_loop
	xor %ax,%ax
	jmp _sc_exit

_sc_diff:
	cmc
	sbb %ax,%ax
	or $1,%al

_sc_exit:
	pop %si
	pop %di
	ret

//------------------------------------------------------------------------------

// void run_prog ()

	.global run_prog

run_prog:

/*
static
run_prog()
{
// It all worked, run the loaded executable
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
*/

	push %ds
	mov $LOADSEG,%ax
	mov %ax,%ds

	mov 0x01E6,%ax  // check for ELKS magic number
	cmp $0x4C45,%ax
	jnz not_elks
	mov 0x01E8,%ax
	cmp $0x534B,%ax
	jz  boot_it

not_elks:

	pop %ds
	mov $msg_elks,%bx
	call _puts
	jmp reboot

boot_it:

	pop %ds
	mov $LOADSEG,%ax
	mov %ax,%ds
	mov $ELKS_INITSEG,%ax
	mov %ax,%es

	mov 497,%bl  // fetch number of setup sectors
	xor %bh,%bh
	inc %bx
	mov 500,%ax  // fetch system executable size
	mov $5,%cl
	add $31,%ax
	shr %cl,%ax
	mov %ax,%dx

// Put the setup where it belongs

looping:
	call copy_sect
	dec %bx
	jnz looping

	mov	$ELKS_SYSSEG,%ax
	mov %ax,%es

// Put the system code at the right place

looping2:
	call copy_sect
	dec %dx
	jnz looping2

// Ok, everything should be where it belongs call it

	mov $ELKS_INITSEG,%ax
	mov %ax,%ds
	mov %ax,%es
	mov %ax,%ss
	mov $0x4000-12,%sp
	ljmp $ELKS_INITSEG+0x20,$0

copy_sect:
	mov $256,%cx
	xor %si,%si
	xor %di,%di
	rep
	movsw

	mov %ds,%ax
	add $32,%ax
	mov %ax,%ds
	mov %es,%ax
	add $32,%ax
	mov %ax,%es
	ret

msg_elks:
	.asciz "Not ELKS!\r\n"

//------------------------------------------------------------------------------

// Magic at end of first sector
// to mark it as bootable for BIOS

	.org 0x1FE
	.word 0xAA55

// Second sector of the boot block

	.org 0x200

sector_2:

//------------------------------------------------------------------------------

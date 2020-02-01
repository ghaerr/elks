//------------------------------------------------------------------------------
// MINIX Boot Block (first block of the filesystem)
// Sourced from DEV86 boot blocks
// Reworked for ELKS to load directly the kernel without any helper
// Sector 1 : boot sector and primitives
//------------------------------------------------------------------------------

#include <autoconf.h>

#include "boot_err.h"

#define BOOTADDR  0x7C00
#define LOADSEG   0x1000

// Must match ELKS
#define ELKS_INITSEG (0x0100)
#define ELKS_SYSSEG  (0x1000)

	.code16

	.text

// Boot block is the first one in the MINIX filesystem
// First part of the boot block (1K) is the boot sector (512 B)
// Loaded by the BIOS at 0:7C00h (32K - 1K) with DL = boot drive

entry:

	// Get the memory size
	// TODO: optional BIOS INT 12h

	int $0x12   // in KB
	mov $6,%cl  // to paragraphs
	shl %cl,%ax

	// Move to the highest & aligned 64K

	// Some BIOSes are intolerant to unaligned buffer for INT 13h
	// so the 64K is the safest possible alignement in all cases

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

	// Copy the current BIOS floppy parameter table
	// so that we could modify it locally

	mov $0x78,%bx  // 0:78h (INT vector 1Eh)
	lds (%bx),%si  // ds:si = BIOS original table
	mov $floppy_table,%di
	mov $6,%cx     // 12 bytes (actually 11 in the table)
	//cld
	rep
	movsw

	// Change BIOS table pointer to our copy

	mov %ax,%ds
	mov $floppy_table,%di
	mov %di,(%bx)
	mov %es,2(%bx)

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

	// Print header

	mov $msg_boot,%bx
	call _puts

	// Save boot drive as provided by BIOS

	mov %dl,drive_num  // DX preserved above

	// Set sector count in floppy parameter table

	mov $floppy_table,%bx
	mov sect_max,%al
	mov %al,4(%bx)

	// Reset the floppy drive

	//mov drive_num,%dl
	xor %ah,%ah
	int $0x13

	// Load the second sector of the boot block

	// TODO: move the retry to the 'disk_read' function
	// and reuse the same function for all reading

	mov $3,%si         // number of tries = 3

_loop1:

	mov $0x0201,%ax    // read 1 sector
	mov $payload,%bx  // destination (ES already set)
	mov $2,%cx         // track 0 - from sector 2 (base 1)
	xor %dh,%dh        // head 0
	mov drive_num,%dl  // boot driver
	int $0x13          // BIOS disk services
	jnc _next2

	dec %si
	jnz _loop1

	mov $ERR_PAYLOAD,%al
	jmp _except

_next2:

	.extern load_prog

	/*
	// Not necessary as fixed format
	// TODO: move it to the "target probe" payload

	err = drive_get (0);
	if (err) {
		puts ("drive_get:");
		word_hex (err);
		puts ("\r\n");
		break;
		}
	*/

	call load_prog
	mov $ERR_NO_SYSTEM,%al
	jmp _except

//------------------------------------------------------------------------------

// We use an exception-like basic mechanism
// to save the space required by error returning

// void except (int code)

	.global except

except:
_except:
	// AX = exception code
	xor %sp,%sp
	add $'0',%al  // first version with single digit
	call _putc
	mov $msg_error,%bx
	call _puts

	.global _reboot

_reboot:

	mov $msg_reboot,%bx
	call _puts

	xor %ax,%ax  // wait for key
	int $0x16

	int $0x19    // BIOS bootstrap
	jmp _reboot

//------------------------------------------------------------------------------

	.global _putc

_putc:

	mov $7,%bx    // page 0 - color 7
	mov $0xE,%ah  // teletype write
	int $0x10     // BIOS video services
	ret

	.global puts

puts:
	xchg %ax,%bx
	// fall through

	.global _puts

_puts:

	mov (%bx),%al
	or %al,%al
	jnz 1f
	ret
1:	mov %bx,%cx
	call _putc
	mov %cx,%bx
	inc %bx
	jmp _puts

puts_exit:
	ret

//------------------------------------------------------------------------------

/*
_half_hex:

	and $0x0F,%al
	add $'0',%al
	cmp $'9',%al
	jle hh_next
	add $('A'-'9'),%al

hh_next:
	jmp _putc

_byte_hex:

	mov %al,%dl

	mov $4,%cl
	shr %cl,%al
	call _half_hex

	mov %dl,%al
	call _half_hex

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

// void disk_read (int sect, int count, byte_t * buf, int seg)

	.global disk_read

disk_read:

	push %cx
	push %dx
	push %bp
	mov %sp,%bp
	sub $8,%sp

// 8(%bp) = segment
// 4(%bp) = offset
// 2(%bp) = logical count
// -2(%bp) = relative sector
// -4(%bp) = relative head
// -6(%bp) = relative track
// -8(%bp) = relative count

	// Compute the CHS from absolute sector

	// AX = absolute sector
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
	add 2(%bp),%ax    // absolute count
	cmp sect_max,%ax
	ja dr_over
	mov 2(%bp),%ax
	jmp dr_next1

dr_over:

	mov sect_max,%ax
	sub -2(%bp),%ax

dr_next1:

	mov %ax,-8(%bp)  // relative count

	mov drive_num,%dl
	mov -4(%bp),%dh  // head number
	mov -2(%bp),%cl  // first sector number
	inc %cl          // base 1 for sector number
	mov -6(%bp),%ch  // cylinder number

	push %es
	mov 4(%bp),%bx
	mov 8(%bp),%es
	mov $0x02,%ah    // BIOS read disk
	int $0x13
	pop %es

	jnc dr_cont

	// PATCH: failure trace

	mov $'*',%al
	call _putc

	// TODO: reset drive on error and retry
	//mov drive_num,%dl
	//xor %ah,%ah
	//int $0x13
	//mov -8(%bp),%ax
	//jmp dr_try

	// TODO: use BIOS returned error
	//mov %ah,%al
	mov $ERR_DISK_READ,%al
	jmp _except

dr_cont:

	// PATCH: success trace

	mov $'.',%al
	call _putc

	// Update absolute count

	mov -8(%bp),%ax
	sub %ax,2(%bp)
	jz dr_exit

	// Move to next head

	movw $0,-2(%bp)
	mov -4(%bp),%ax
	inc %ax
	mov %ax,-4(%bp)

	cmp head_max,%ax
	jnz dr_next2

	// Move to next track

	movw $0,-4(%bp)
	incw -6(%bp)

dr_next2:

	// Advance in buffer

	// PATCH: don't move ES, only BX
	// This limits the reading size to 64K

	mov -8(%bp),%ax
	mov $9,%cl
	shl %cl,%ax
	add %ax,4(%bp)

	jmp dr_loop

dr_exit:

	mov %bp,%sp
	pop %bp
	pop %dx
	pop %cx
	ret $2

//------------------------------------------------------------------------------

// void run_prog ()

	.global run_prog

// TODO: to be moved to the MINIX specific payload

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
	mov $ERR_BAD_SYSTEM,%al
	jmp _except

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

//------------------------------------------------------------------------------

msg_boot:
	.asciz "Boot sector\r\n"

msg_error:
	.asciz "!\r\n"

msg_reboot:
	.asciz "Press key\r\n"

//------------------------------------------------------------------------------

// Boot drive as provided by BIOS on entry

	.global drive_num

drive_num:
	.byte 0

// Local copy of BIOS floppy parameter table

floppy_table:
	.word 0,0,0,0,0,0

// Disk geometry (CHS)

	.global sect_max
	.global head_max
	.global track_max

// #undef CONFIG_IMG_FD1680
// #undef CONFIG_IMG_FD1200

sect_max:
#if defined(CONFIG_IMG_FD360) || defined(CONFIG_IMG_FD720)
	.word 9
#endif
#if defined(CONFIG_IMG_FD1200)
	.word 15
#endif
#if defined(CONFIG_IMG_FD1440)
	.word 18
#endif
#if defined(CONFIG_IMG_FD1680)
	.word 21
#endif
#if defined(CONFIG_IMG_HD)
	.word CONFIG_IMG_SECT
#endif

head_max:
#if defined(CONFIG_IMG_FD360) || defined(CONFIG_IMG_FD720) || defined(CONFIG_IMG_FD1200) || defined(CONFIG_IMG_FD1440) || defined(CONFIG_IMG_FD1680)
	.word 2
#endif
#if defined(CONFIG_IMG_HD)
	.word CONFIG_IMG_HEAD
#endif

// TODO: number of tracks is not used

track_max:
#if defined(CONFIG_IMG_FD360)
	.word 40
#endif
#if  defined(CONFIG_IMG_FD720) || defined(CONFIG_IMG_FD1440)
	.word 80
#endif
#if defined(CONFIG_IMG_HD)
	.word CONFIG_IMG_TRACK
#endif

// Magic at end of first sector
// to mark it as bootable for BIOS

	.org 0x1FE
	.word 0xAA55

// Here comes the first sector of the payload
// that is specific to the filesystem

	.org 0x200

payload:

//------------------------------------------------------------------------------

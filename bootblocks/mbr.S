// This is a 'Master Boot Record' following the MSDOS 'standards'.
// This BB successfully boots MSDOS, Windows or Linux in CHS.
//
// Copyright GPL2, Robert de Bath, 1996-2008.
// Updated for ELKS and ia16-elf-as by Greg Haerr April 2020
//
// Lowest available is $0500, MSDOS appears to use $0600 ... I wonder why?

	.arch	i8086, nojumps
	.code16
	.text

#define MBRKEY		// Option to choose the boot record based on keystroke (107)
#define wait_ticks	50	// ticks to wait for key before booting

#define OURSEG		0x60	// (=0x600) we copy ourselves here, then set DS to it
#define BOOTADDR	0x7c00	// BIOS loads us here
#define DISKBUF		0x7c00	// addressable as ES:DISKBUF

#define partition_start	0x1BE		// addressable as DS:partition_start
#define partition_end	0x1FE
#define partition_size	0x10
#define magic_word	DISKBUF+0x1FE	// addressable as ES:magic_word

entry:

	cli			// Assume _nothing_! (needed for NT 4)
	mov	$OURSEG,%ax	// Prepare to copy ourselves to OURSEG
	mov	%ax,%es		// ES = OURSEG
	mov	%ax,%ss		// SS = OURSEG
	xor	%sp,%sp		// Stack just below OURSEG

	xor	%di,%di
	push	%di		// Push 0
	mov	%di,%ds		// DS = 0 for copy
	mov	$BOOTADDR,%si
	mov	$256,%cx	// Copy from 0:BOOTADDR to OURSEG:0
	cld
	rep
	movsw

	push	%es		// DS = OURSEG
	pop	%ds
	pop	%es		// ES = 0

	push	%ss		// Push OURSEG
	mov	$cont,%cx	// Push offset of cont
	push	%cx
	retf			// Jump to OURSEG:cont
cont:
	mov	%dh,drive_num	// Save BIOS drive number
	sti			// Let the interrupts back in.

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Display a pre-boot message, timeout on keypress, load
	call	disp_message
#ifdef MBRKEY
	call 	key_wait
#endif

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//

// Now check the partition table, must use DS:SI as pointer cause that's what the
// partition boot blocks expect.

// Normal active partition check, (Order: 1,2,3,4)
	mov	$partition_start,%si
check_active:
	cmpb	$0x80,(%si)	// Flag for activated partition
	jz	found_active
try_next_part:			// Try next partition
	add	$partition_size,%si
	cmp	$partition_end,%si
	jnz	check_active

	jmp	no_partition

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Active partition found, boot it.
found_active:
	mov	$3,%di		// Max retries
retry:

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
do_CHS:

	mov	(%si),%dx	// dh = drive head, dl = 0x80 ie HD drive 0
	mov	2(%si),%cx	// cx = sector & cylinder encoded for int $0x13
	mov	$DISKBUF,%bx	// Pointer to start of BB.
	mov	$0x0201,%ax	// Read 1 sector
	int	$0x13		// Disk read.
	jnc	sector_loaded

// Error, reset and retry
retry_error:
	xor	%ax,%ax
	int	$0x13		// Disk reset

	dec	%di
	jnz	retry		// Try again

	mov	$disk_read_error,%si
	jmp	no_boot		// Sorry it ain't gonna work.

sector_loaded:
	mov	$magic_word,%di	// Check end of sector 
	cmpw	$0xAA55,%es:(%di) // for magic 0xAA55 word
	jnz	try_next_part	// No? Try next partition.

	mov	drive_num,%dh	// Restore DH drive number for VBR
	mov	%si,%bp		// LILO says some BBs use bp rather than si

	ljmp	$0,$BOOTADDR	// Jump to VBR boot sector

no_partition:
	mov	$no_bootpart,%si
no_boot:			// SI now has pointer to error message
	call	puts


// Fatal errors ...........
#ifdef MBRKEY
	mov	$crlf,%si
	call	puts
	jmp	key_pause
#else
	mov	$press_key,%si
	call	puts
keyboot:			// Wait for a key then reboot
	xor	%ax,ax
	int	$0x16
	ljmp	$0xFFFF,$0	// Reboot.
#endif

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// 
disp_message:
	mov	$banner,%si

// Display message uses SI,AX,BX
puts:				// This is smaller than using $13
	lodsb
	cmp	$0,%al
	jz	.EOS
	mov	$7,%bx		// Page 0, color 7
	mov	$0xE,%ah	// Teletype write
	int	$0x10		// BIOS video services
	jmp	puts
.EOS:
	ret

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//

#ifndef MBRKEY
press_key:
	.asciz	"\r\nPress return:"
#endif

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//

disk_read_error:
	.asciz	"Disk read error"
no_bootpart:
	.asciz	"No bootable partition"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
// Choose the partition based on a pressed key ...

#ifdef MBRKEY
key_wait:
	mov	$Prompt,%si
	call	puts
	call	wait_key
	jnz	key_pause

	mov	$Unprompt,%si	// Nothing has happened, return.
	call	puts
	ret

key_pause:
	mov	$Pause,%si
	call	puts

key_tick:
	call	wait_key
	jz	key_tick
	jmp	Got_key

wait_key:
	mov	$wait_ticks,%di	// Wait for ticks

next_loop:
	mov	$1,%ah
	int	$0x16
	jnz	done_wait
	mov	$0,%ah
	int	$0x1A		// Get current tick
	cmp	%si,%dx		// If changed DEC our counter.
	jz	next_loop
	mov	%dx,%si
	dec	%di
	jnz	next_loop

done_wait:
	ret

Got_key:
	mov	$0,%ah		// Clear the kbd buffer.
	int	$0x16
	cmp	$0x20,%al
	jz	key_tick

	push	%ax
	mov	%al,Showkey
	mov	$Showkey,%si
	call	puts
	pop	%ax

	cmp	$'F',%al	// f -> floppy boot
	jz	is_floppy
	cmp	$'f',%al
	jz	is_floppy

	cmp	$'1',%al	// 1 .. 4 -> hard disk partition
	jb	key_pause
	cmp	$'4',%al
	ja	key_pause

	and	$0x7,%ax	// calc hd partition address
	dec	%ax
	mov	$4,%cl
	shl	%cl,%ax
	add	$partition_start,%ax
	mov	%ax,%si

	orb	$0x80,(%si)	// force active flag for partition
	jmp	found_active

is_floppy:
	mov	$floppy_part,%si // replace table with floppy partition
	jmp	found_active

Prompt:
	.asciz	"\rMBR: "
Unprompt:
	.asciz	"\r    \r"
Pause:
	.asciz	"\rMBR F1234> "
Showkey:
	.ascii	" "
crlf:
	.asciz	"\r\n"
floppy_part:
	.word	0,1,0,0,0,0,0,0		// faked floppy partition table

#endif

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//

	.global	banner
banner:
	.asciz	"Welcome to ELKS MBR Boot Manager\r\n"

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//
drive_num:
	.byte	0		// saved BIOS drive number

	.global	end_of_code
end_of_code:

	.org	0x1B8	// Modern Standard MBR locations (see Wiki Master Boot Record)
	.long	0		// 0x1B8 = 32-bit disk signature
	.word	0		// 0x1BC = 0 (0x5A5A if copy-protected)

	.org	0x1BE	// MBR partition table (left out)

	.org	0x1FE	// Boot signature
	.word	0xAA55
//THE END

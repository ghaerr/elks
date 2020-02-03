//------------------------------------------------------------------------------
// ELKS boot sector
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

// Loaded by the BIOS at 0:7C00h (32K - 1K) with DL = boot drive number
//
// Allow the new BIOS floppy parameter table to clobber the first few bytes
// of the boot sector after we are done with them

entry:
floppy_table:
	jmp entry1

	.org entry+3,0x90

// Parameter block structure, hopefully compatible enough with Microsoft's
// BPB (see the _Microsoft FAT Specification_) to be used on a FAT12 or
// FAT16 disk
//
// Fields marked (-) only have a meaning for FAT12 or FAT16, and are not
// used by other filesystem types

#if defined(CONFIG_IMG_FD360) || defined(CONFIG_IMG_FD720)
	.set SECT_MAX,9
#elif defined(CONFIG_IMG_FD1200)
	.set SECT_MAX,15
#elif defined(CONFIG_IMG_FD1440)
	.set SECT_MAX,18
#elif defined(CONFIG_IMG_FD1680)
	.set SECT_MAX,12
#elif defined(CONFIG_IMG_HD)
	.set SECT_MAX,CONFIG_IMG_SECT
#endif
#if defined(CONFIG_IMG_FD360) || defined(CONFIG_IMG_FD720) \
    || defined(CONFIG_IMG_FD1200) || defined(CONFIG_IMG_FD1440) \
    || defined(CONFIG_IMG_FD1680)
	.set HEAD_MAX,2
#elif defined(CONFIG_IMG_HEAD)
	.set HEAD_MAX,CONFIG_IMG_HEAD
#endif
#if defined(CONFIG_IMG_FD360)
	.set TRACK_MAX,40
#elif defined(CONFIG_IMG_FD720) || defined(CONFIG_IMG_FD1440)
	.set TRACK_MAX,80
#elif defined(CONFIG_IMG_HD)
	.set TRACK_MAX,CONFIG_IMG_TRACK
#endif

	.global sect_max
	.global head_max
	.global tot_sects

// Boot drive as provided by BIOS on entry
// Allow this to overlap with the OEM name, which the boot code does not use

	.global drive_num

drive_num:

	.ascii "eLkS"			// OEM name --- stash information on
	.byte 0				// ELKS parameters' location and
	.byte elks_parms_start		// size here
	.byte elks_parms_end-elks_parms_start
	.byte 0
	.word 0x200			// Bytes per sector
	.byte 0				// Sectors per cluster (-)
	.word 0				// Reserved sectors (-)
	.byte 0				// No. of file allocation tables (-)
	.word 0				// No. of root directory entries (-)
tot_sects:
	.word SECT_MAX*HEAD_MAX*TRACK_MAX // Total sector count, 16-bit
	.byte 0				// Media descriptor (-)
	.word 0				// Size of one file alloc. table (-)
sect_max:
	.word SECT_MAX			// Sectors per track
head_max:
	.word HEAD_MAX			// No. of heads
	.word 0				// No. of hidden sectors

// Extra disk parameters for the ELKS bootloader

elks_parms_start:
payload_logical_sect:
	.word 1				// Starting logical sector of OS-
					// -specific payload
payload_sect_count:
	.byte 1				// No. of sectors in payload
elks_parms_end:

entry1:
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

	xor %di,%di
	mov %di,%ds
	mov $BOOTADDR,%si
	mov $256,%cx  // 256 words = 512 B
	cld
	rep
	movsw

	// Copy the current BIOS floppy parameter table
	// so that we could modify it locally

	mov $0x78,%bx  // 0:78h (INT vector 1Eh)
	lds (%bx),%si  // ds:si = BIOS original table
.if floppy_table == entry
	xor %di,%di
.else
	mov $floppy_table,%di
.endif
	push %di
	mov $6,%cl     // 12 bytes (actually 11 in the table) and CH = 0
	//cld
	rep
	movsw

	// Change BIOS table pointer to our copy

	mov %cx,%ds    // CX = 0
	popw (%bx)
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

	mov sect_max,%al
	mov %al,floppy_table + 4

	// Load the payload sectors, usually the second sector of the boot block

	mov payload_logical_sect,%ax
	mov payload_sect_count,%dl
	xor %dh,%dh
	mov $payload,%cx
	push %ss
	mov $ERR_PAYLOAD,%bl
	call _disk_read

	call payload

	mov $ERR_NO_SYSTEM,%al
	// fall through to _except

//------------------------------------------------------------------------------

// We use an exception-like basic mechanism
// to save the space required by error returning

// void except (char code)

	.global except

except:
_except:
	// AL = exception code
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
	jz 1f
	mov %bx,%cx
	call _putc
	mov %cx,%bx
	inc %bx
	jmp _puts
1:  ret

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
	mov $ERR_DISK_READ,%bl

_disk_read:
	// BL = error code to display on failure

	push %cx
	push %dx
	push %bp
	mov %sp,%bp
	sub $8,%sp
	mov $3,%bh
	push %bx

// 8(%bp) = segment
// 4(%bp) = offset
// 2(%bp) = logical count
// -2(%bp) = relative sector
// -4(%bp) = relative head
// -6(%bp) = relative track
// -8(%bp) = relative count
// -9(%bp) = retries left
// -10(%bp) = failure error code

	// Compute the CHS from logical sector

	// AX = logical sector
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
	add 2(%bp),%ax
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

	decb -9(%bp)
	jz dr_error
	mov drive_num,%dl
	xor %ah,%ah
	int $0x13
	jmp dr_loop

dr_error:
	// TODO: use BIOS returned error
	//mov %ah,%al
	pop %ax
	jmp _except

dr_cont:

	// PATCH: success trace

	mov $'.',%al
	call _putc

	// Update logical count

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
	// Limitation: reading size <= 64K

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

// Magic at end of first sector
// to mark it as bootable for BIOS

	.org 0x1FE
	.word 0xAA55

// Here comes the first sector of the payload
// that is specific to the filesystem

	.org 0x200

payload:

//------------------------------------------------------------------------------

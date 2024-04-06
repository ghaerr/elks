//------------------------------------------------------------------------------
// ELKS boot sector
//------------------------------------------------------------------------------

// If BOOT_FAT is undefined, this assembles to a filesystem-independent boot
// sector which loads a stage 2 bootloader on the second disk sector
//
// If BOOT_FAT is defined, this assembles to a single-sector bootloader that
// reads and runs /linux on a FAT12/FAT16 filesystem

#include <linuxmt/config.h>
#include <linuxmt/boot.h>

#include "boot_err.h"

// configuration settings for boot code

#ifdef CONFIG_ARCH_PC98
// Japanese NEC PC98

#define BOOTCS
#if defined(CONFIG_IMG_FD1232)
#define SECTOR_SIZE	1024
#else
#define SECTOR_SIZE	512
#endif
#define MEMTOPSEG	0xA000		// 640K memory

#else
// IBM PC

#define BOOTADDR	0x7C00
#define BIOS_DRVNUM			// use boot drive number from BIOS
#define SECTOR_SIZE	512
#endif

#define LOADSEG       DEF_INITSEG

// Whether to try to do whole-track reads, or read one sector at a time
//
// Whole-track reads are disabled for the FAT bootloader because we are a
// bit short of code space :-(
//
// Whole-track reads are also disabled for the NEC PC98 platform for now;
// it seems that, instead of a DDPT, we need to frob the PC98's F2DD_POINTER
// (http://www.webtech.co.jp/company/doc/undocumented_mem/memsys.txt)
// mechanism for configuring the "end of track" value; we will need to
// somehow figure out this mechanism and test it :-( :-(
#if !defined BOOT_FAT && !defined CONFIG_ARCH_PC98
#   define FAST_READ
#else
#   undef FAST_READ
#endif

// Macro to restore the original (i.e. BIOS's) Diskette Drive Parameter Table
//
// Note: this may clobber BX and DS, and reset SP!  Use it only upon an error
// or just before handing over to /linux
.macro RESTORE_DDPT
#ifdef FAST_READ
	mov $-8,%sp
	pop %bx
	pop %ds
	popw (%bx)
	popw 2(%bx)
#endif
.endm

	.arch	i8086, nojumps
	.code16

	.text

// Loaded by the BIOS at 0:7C00h (32K - 1K) or 7C0h:0
// with DL = boot drive number

entry:

// Allow the new BIOS floppy parameter table to clobber the first few bytes
// of the boot sector after we are done with them

floppy_table:

#ifdef BOOT_FAT
	jmp entry1
	.org 3, 0x90

#include "boot_sect_fat.h"

	FAT_BPB

entry1:
#endif

	// Get the memory size
#ifdef MEMTOPSEG
	mov $MEMTOPSEG,%ax
#else
	int $0x12   // in KB
	mov $6,%cl  // to paragraphs
	shl %cl,%ax
#endif

	// Move to the highest & aligned 64K

	// Some BIOSes are intolerant to unaligned buffer for INT 13h
	// so the 64K is the safest possible alignement in all cases

	sub $0x1000,%ax
	and $0xF000,%ax
	mov %ax,%es
	mov %ax,%ss   // automatic CLI for next instruction
	xor %sp,%sp

	xor %di,%di
#ifdef BOOTCS
	mov %cs,%ax
	mov %ax,%ds
	xor %si,%si
#else
	mov %di,%ds		// DS:SI = 0:BOOTADDR
	mov $BOOTADDR,%si
#endif
	mov $SECTOR_SIZE/2,%cx  // sector size in words
	rep
	movsw

#ifdef FAST_READ
	// Copy the current BIOS floppy parameter table
	// so that we could modify it locally
	//
	// Keep a pointer to the original table so that RESTORE_DDPT can
	// restore it later

	mov $0x78,%bx  // 0:78h (INT vector 1Eh)
	lds (%bx),%si  // ds:si = BIOS original table
	push %ds
	push %si
	push %cx       // push address 0:78h (cx = 0)
	push %bx
.if floppy_table == entry
	xor %di,%di
.else
	mov $floppy_table,%di
.endif
	push %di
	mov $7,%cl     // Usually the table is 11 bytes, but Ralf Brown's
		       // Interrupt List says that the IBM SurePath BIOS
		       // uses a 14-byte table
		       // CH = 0 at this point
	//cld
	rep
	movsw

	// Change BIOS table pointer to our copy

	mov %cx,%ds    // CX = 0
	popw (%bx)
	mov %es,2(%bx)
#endif

	// Rebase CS DS to work in the 64K segment

	push %es
	pop %ds

	push %ss
	mov $_next1,%cl  // CH = 0
	push %cx
	retf

_next1:

	// Save boot drive as provided by BIOS

#ifdef BIOS_DRVNUM
	mov %dl,drive_num  // DX preserved above
#endif

	// Print header

#ifdef CONFIG_ARCH_PC98
	mov $0x0C,%ah
	int $0x18
#endif
	mov $msg_boot,%bx
	call _puts

#ifdef FAST_READ
	// Set sector count in floppy parameter table

	mov sect_max,%al
	mov %al,floppy_table + 4
#endif

#ifndef BOOT_FAT
	// Load the second sector of the boot block

	mov $1,%ax
	mov %ax,%dx
	mov $payload,%cx
	push %ss
	mov $ERR_PAYLOAD,%bl
	call _disk_read

	call payload
#else
	// Load the file /linux from the FAT12/16 filesystem's root directory

	FAT_LOAD_AND_RUN_KERNEL
#endif

no_system:
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
	jmp _reboot1

	.global _reboot

_reboot:
	mov $msg_reboot,%bx

_reboot1:
	call _puts

	xor %ax,%ax  // wait for key
#ifdef CONFIG_ARCH_PC98
	int $0x18
#else
	int $0x16
#endif
	ljmpw $0xffff,$0  // do a cold(er) boot

//------------------------------------------------------------------------------

	.global _putc

_putc:

#ifdef CONFIG_ARCH_PC98
	mov $tvram_x,%si
	mov (%si),%di
	xor %ah,%ah
	push %es
	mov $0xa000,%dx
	mov %dx,%es
	mov %ax,%es:(%di)
	pop %es
	inc %di
	inc %di
	mov %di,(%si)
#else
	push %bx
	mov $7,%bx    // page 0 - color 7
	mov $0xE,%ah  // teletype write
	int $0x10     // BIOS video services
	pop %bx
#endif
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
	call _putc
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
	// AX = sect
	// DX = count
	// CX = buf
	// seg pushed on stack
	mov $ERR_DISK_READ,%bl

_disk_read:
	// BL = error code in case of failure

	mov $5,%bh
	push %bp
	mov %sp,%bp
	push %ax
	push %dx
	push %cx
	push %bx

// word 4(%bp) = segment
// word -2(%bp) = logical sector number
// word -4(%bp) = logical count
// word -6(%bp) = buffer offset
// byte -7(%bp) = no. of retries left
// byte -8(%bp) = failure error code

dr_loop:
	// Compute the CHS from logical sector
	// AX = logical sector
	xchg %ax,%bx
	mov sect_max,%al
	mulb head_max
	xchg %ax,%bx
	xor %dx,%dx
	add sect_offset,%ax
	adc sect_offset+2,%dx
	div %bx
#ifdef CONFIG_ARCH_PC98
	mov %ax,%cx      // cylinder number for PC_98
#else
	mov %al,%ch      // CH = low 8 bits of physical cylinder number
	ror %ah
	ror %ah
	mov %ah,%cl      // stash higher 2 bits of cylinder number in CL
#endif

	xchg %ax,%dx
	xor %dx,%dx
#ifndef BOOT_FAT
	mov sect_max,%bl
	xor %bh,%bh
#else
	mov sect_max,%bx // BX = sectors per track
#endif
	div %bx          // DL = physical sector number - 1
#ifndef CONFIG_ARCH_PC98
	or %dl,%cl       // stash {physical sector number - 1} in CL
	inc %cx          // base 1 for sector number
#endif
	mov %al,%dh      // DH = physical head number

#ifdef FAST_READ
	// Compute number of sectors to read
	// First limit the sector count to the end of the track

	sub %dl,%bl
	mov -4(%bp),%ax
	cmp %bx,%ax
	jna dr_over
	xchg %ax,%bx

	// Also make sure the read does not overshoot the end of ES
	//
	// And, make sure we do not read more than 0x7F sectors at one go
	// (this slightly simplifies the advance-in-buffer calculations
	// later on)

dr_over:
	mov -6+1(%bp),%bl
	neg %bl
	shr %bl
	jnz dr_over2
	mov $0x7F,%bl
dr_over2:
	cmp %bl,%al      // BL ranges from 1 to 0x7F inclusive
	jna dr_over3
	xchg %ax,%bx     // AL = number of sectors to read for this round

dr_over3:
#endif
#ifdef BIOS_DRVNUM
	mov drive_num,%dl
#endif

dr_try:
#ifdef FAST_READ
	push %ax
#endif
	push %cx
	push %dx
	push %es
	mov 4(%bp),%es

#ifdef CONFIG_ARCH_PC98
	push %bp
	mov -6(%bp),%ax
	mov %ax,%bp
	push %es
	xor %ax,%ax
	mov %ax,%es
	mov $0x584,%bx
	mov %es:(%bx),%al // Physical Device Address
	mov %al,drive_num
	pop %es
	mov $0xD6,%ah    // Read Data
	mov $SECTOR_SIZE,%bx
	test $0x10,%al
	jz pc98_1b
#if defined(CONFIG_IMG_FD1232)
	mov $0x03,%ch    // 1024 Bytes per sector
	inc %dl          // sector number for PC_98
#elif defined(CONFIG_IMG_FD1440)
	mov $0x02,%ch    // 512 Bytes per sector
	inc %dl          // sector number for PC_98
#endif
pc98_1b:
	int $0x1B
	pop %bp
#else

	mov -6(%bp),%bx
#ifdef FAST_READ
	mov $0x02,%ah    // BIOS read disk
#else
	mov $0x0201,%ax
#endif
	int $0x13
#endif
	pop %es
	jnc dr_cont

#ifndef CONFIG_ARCH_PC98
	xor %ah,%ah       // BIOS reset disk
	int $0x13
#endif
	// PATCH: failure trace

	mov $'*',%al
	call _putc

	decb -7(%bp)
	pop %dx
	pop %cx
#ifdef FAST_READ
	pop %ax
	mov $1,%al       // if retry needed, try reading just _one_ sector...
#endif
	jnz dr_try

	// TODO: use BIOS returned error
	pop %ax
	jmp _except

dr_cont:
	// Reset retry count

	movb $5,-7(%bp)

	// PATCH: success trace

	mov $'.',%al
	call _putc

	pop %dx
	pop %cx
#ifdef FAST_READ
	pop %ax

	// Update logical sector number and logical count
	// AL = number of sectors just read, which is at most 0x7F

	cbtw
	sub %ax,-4(%bp)
	jz dr_exit
	add %ax,-2(%bp)

	// Advance in buffer; advance BX, then advance ES if BX overflows

	shl %al          // cannot overflow :-)
	add %al,-6+1(%bp)
#else
	decw -4(%bp)
	jz dr_exit
	incw -2(%bp)
	addb $SECTOR_SIZE>>8,-6+1(%bp)	// increment by sector size
#endif
	jnc dr_next
	addb $0x10,4+1(%bp)

dr_next:
	mov -2(%bp),%ax
	jmp dr_loop

dr_exit:
	mov %bp,%sp
	pop %bp
	ret $2

//------------------------------------------------------------------------------

#ifndef BOOT_FAT
// void run_prog ()

	.global run_prog

// TODO: to be moved to the MINIX specific payload

run_prog:

	mov drive_num,%dl
	xor %dh,%dh
	mov sect_offset,%cx
	mov sect_offset+2,%bx
	mov $LOADSEG,%ax
	mov %ax,%ds

	mov elks_magic,%ax  // check for ELKS magic number
	cmp $0x4C45,%ax
	jnz not_elks
	mov elks_magic+2,%ax
	cmp $0x534B,%ax
	jz  boot_it

not_elks:

	push %cs
	pop %ds
	mov $ERR_BAD_SYSTEM,%al
	jmp _except

boot_it:

	// Signify that /linux was loaded as 1 blob
	orb $(EF_AS_BLOB|EF_BIOS_DEV_NUM),elks_flags
	mov %dx,root_dev
	mov %cx,part_offset  // save sector offset of booted partition
	mov %bx,part_offset+2

	RESTORE_DDPT

	mov $LOADSEG,%ax
	mov %ax,%ds
	mov %ax,%es
	ljmp $LOADSEG+0x20,$0
#endif

//------------------------------------------------------------------------------

msg_boot:
	.asciz "ELKS"

msg_error:
	.ascii "!\r\n"
	// fall through

msg_reboot:
#ifdef CONFIG_ARCH_PC98
	.byte 0
tvram_x:
	.word 0
#else
	.asciz "Press key\r\n"
#endif

	.global end_of_code
end_of_code:
//------------------------------------------------------------------------------

#if defined(CONFIG_IMG_FD1232)
	.org SECTOR_SIZE/2-2
	.word 0xAA55 // boot signature need to be at 0x1FE and 0x1FF
#endif

// ELKS disk parameter structure
// For future expansion, fields should be added at the _front_ of structure

#ifdef BOOT_FAT
	// FAT boot sectors use older EPB for now
	.org SECTOR_SIZE-9	// 0x1F7 on IBM PC
elks_parms_start:

#else
	.org SECTOR_SIZE-0x0D	// 0x1F3 on IBM PC
elks_parms_start:

	.global sect_offset
sect_offset:
	.long 0                 // Partition start sector of MINIX boot block
#endif

// Disk geometry (CHS)

	.global sect_max
	.global head_max
	.global track_max

// TODO: number of tracks is not used

track_max:
#if defined(CONFIG_IMG_FD360)
	.word 40
#elif defined(CONFIG_IMG_FD720) || defined(CONFIG_IMG_FD1200) \
      || defined(CONFIG_IMG_FD1440) || defined(CONFIG_IMG_FD1680) \
	  || defined(CONFIG_IMG_FD2880)
	.word 80
#elif defined(CONFIG_IMG_FD1232)
	.word 77
#elif defined(CONFIG_IMG_HD)
	.word CONFIG_IMG_CYL
#else
	.warning "Unknown number of disk tracks!"
	.word 0
#endif

#ifndef BOOT_FAT
sect_max:
#endif
#if defined(CONFIG_IMG_FD360) || defined(CONFIG_IMG_FD720)
	.byte 9
#elif defined(CONFIG_IMG_FD1200)
	.byte 15
#elif defined(CONFIG_IMG_FD1232)
	.byte 8
#elif defined(CONFIG_IMG_FD1440)
	.byte 18
#elif defined(CONFIG_IMG_FD1680)
	.byte 21
#elif defined(CONFIG_IMG_FD2880)
	.byte 36
#elif defined(CONFIG_IMG_HD)
	.byte CONFIG_IMG_SECT
#else
	.warning "Unknown number of disk sectors per track!"
	.byte 0
#endif

#ifndef BOOT_FAT
head_max:
#endif
#if defined(CONFIG_IMG_FD360) || defined(CONFIG_IMG_FD720) \
    || defined(CONFIG_IMG_FD1200) || defined(CONFIG_IMG_FD1440) \
    || defined(CONFIG_IMG_FD1680) || defined(CONFIG_IMG_FD2880) \
	|| defined(CONFIG_IMG_FD1232)
	.byte 2
#elif defined(CONFIG_IMG_HD)
	.byte CONFIG_IMG_HEAD
#else
	.warning "Unknown number of disk sides/heads!"
	.byte 0
#endif

// Marker to indicate presence and size of ELKS parameter structure

	.byte . - elks_parms_start
	.ascii "eL"
	.org SECTOR_SIZE-2	// 0x1FE on IBM PC

//------------------------------------------------------------------------------

// Boot drive as provided by BIOS on entry
// Allow this variable to overlap the "eL" marker (above)

	.global drive_num

	.set drive_num, . - 1

// Magic at end of first sector
// to mark it as bootable for BIOS

	.word 0xAA55

#ifndef BOOT_FAT
// Here comes the first sector of the payload
// that is specific to the filesystem

	.org SECTOR_SIZE

payload:
#endif

//------------------------------------------------------------------------------

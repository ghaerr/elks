/*
 *	PC98 Disk BIOS INT 0x1B Interface
 *	Dec 2021 by @tyama501, based on bios13.S
 */

#include <linuxmt/config.h>

	.code16

	.text
	.global	call_bios

call_bios:
	push %bp
	mov  %sp,%bp

// Things we want to save - direction flag BP ES

	pushf
	push %es
	push %si
	push %di

// DS also in SS

	mov 4(%bp),%bx

//	Load the register block from the table	

	mov 2(%bx),%ax
	mov 6(%bx),%cx
	mov 8(%bx),%dx
	mov 10(%bx),%si
	mov 12(%bx),%di
	mov 14(%bx),%bp
	mov 16(%bx),%es
	push 18(%bx)        // DS in stack
	push 20(%bx)
	popf
	mov 4(%bx),%bx      // Load BX
//
//	Stack now holds the call value for DS
//
	pop %ds             // DS desired

// ***** DS is now wrong we cannot load from the array again *****

//	Do a disk interrupt.

	cmp $0x84,%ah
	jz sense

	push %bx
	push %cx
	push %dx
	push %bp

	xchg %ch,%cl     // cylinder number for PC_98
	xchg %ch,%dl     // sector number for PC_98
	xchg %bx,%bp


	push %ax
	mov %ch,%al
	and $0x80,%al
	cmp $0x80,%al
	jnz fd

hd:
	pop %ax
	mov $0x200,%bx   // 512Bytes
	cmp $2,%al
	jnz ch_hd
	shl %bx          // 1024Bytes
ch_hd:
	mov %ch,%al
	and $0x0F,%al
	or  $0xA0,%al    // Physical Device Address
	push %dx
	and $0xC0,%dl
	mov %dl,%ch      // MSBits of cylinder number
	pop %dx
	and $0x3F,%dl
	dec %dl
	jmp call_1B

fd:
	pop %ax
#ifdef CONFIG_IMG_FD1232
	mov $0x400,%bx   // 1024Bytes
	mov %ch,%al
	and $0x0F,%al
	or  $0x90,%al    // Physical Device Address
	mov $0x03,%ch    // 1024Bytes per sector
#else
	mov $0x200,%bx   // 512Bytes
	cmp $2,%al
	jnz ch_1440
	shl %bx          // 1024Bytes
ch_1440:
	mov %ch,%al
	and $0x0F,%al
	or  $0x30,%al    // Physical Device Address
	mov $0x02,%ch    // 512Bytes per sector
#endif

call_1B:
	int $0x1B

	pop %bp
	pop %dx
	pop %cx
	pop %bx
	jmp result

sense:
	mov %dl,%al
	and $0x0F,%al
	or  $0xA0,%al    // Physical Device Address
	int $0x1B

result:

//	Now recover the results
//	Make some breathing room

	pushf
	push %ds
	push %bx

//	Stack is now returned FL, DS, BX
//	Recover our DS segment

	push	%ss
	pop	%ds

// ***** We can now use the bios data table again *****

	mov %sp,%bx
	mov 18(%bx),%bx      // Load BX with table address

	mov %ax,2(%bx)       // Save the old AX
	pop 4(%bx)          // Save the old BX
	mov %cx,6(%bx)
	mov %dx,8(%bx)
	mov %si,10(%bx)
	mov %di,12(%bx)
	mov %bp,14(%bx)
	mov %es,16(%bx)
	pop 18(%bx)         // Save the old DS
	pop 20(%bx)         // Pop the returned flags off

//	Restore things we must save

	pop %di
	pop %si
	pop %es
	popf
	pop %bp
	mov	20(%bx),%ax	// Return CARRY value
	and	$1,%ax
	ret

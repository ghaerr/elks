/*
 *	16bit PC BIOS interface library
 *
 *	FIXME: Only supports IRQ 0x13 for now. Needs to do 0x10 later!
 *
 */

#include <linuxmt/config.h>

/*
 *	Quick drop into assembler for this one.
 */

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

#ifdef CONFIG_ARCH_PC98
	push %ax
	push %bx
	push %cx
	push %dx
	push %bp

	xchg %ch,%cl     // cylinder number for PC_98
	xchg %ch,%dl     // sector number for PC_98
	xchg %bx,%bp

#if defined(CONFIG_IMG_FD1440)
	mov $0x200,%bx   // 512Bytes
	mov $0x02,%ch    // 512Bytes per sector
	mov $0xB0,%al    // Physical Device Address
#else
	mov $0x400,%bx   // 1024Bytes
	mov $0x03,%ch    // 1024Bytes per sector
	mov $0x90,%al    // Physical Device Address
#endif
	int $0x1B

	pop %bp
	pop %dx
	pop %cx
	pop %bx
	pop %ax
#else
	int $0x13
#endif

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

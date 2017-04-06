!
! int __save_flags(void)
!
! Low level IRQ control.

	.globl	___save_flags
	.text

___save_flags:
	pushf
	pop	ax
	ret

! void restore_flags(unsigned int)
!
! this version is smaller than the functionally equivalent C version
! at 7 bytes vs. 21 or thereabouts :-) --Alastair Bridgewater
!
! Further reduced to 5 bytes  --Juan Perez

	.globl	_restore_flags

_restore_flags:
	pop	ax
	popf
	pushf
	jmp	ax

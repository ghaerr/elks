! This is the C run-time start-off routine. Its job is to take the
! arguments as put on the stack by EXEC, and to parse them and set them
! up the way _main expects them.

.text
.extern _main, _exit, _etext, _monstartup
.globl crtso, _environ
.globl begtext, begdata, begbss

begtext:

crtso:		mov	bx,sp
		mov	cx,(bx)
		add	bx,*2
		mov	ax,cx
		inc	ax
		shl	ax,#1
		add	ax,bx
		mov	_environ,ax	! save envp in environ
		push	ax		! push environ
		push	bx		! push argv
		push	cx		! push argc
		mov	ax,#_etext	! monstartup(2, etext)
		push	ax
		mov	ax,#2
		push	ax
		call	_monstartup	! do call of monstartup
		pop	bx		! remove arguments
		pop	bx
		call	_main
		add	sp,*6
		push	ax		! push exit status
		call	_exit

! a special version of _exit gets linked because _monstartup is referenced

.data

begdata:	.zerow 8		! food for null pointer bugs

_environ:	.word 0

.bss

begbss:

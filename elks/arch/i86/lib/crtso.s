| This is the C run-time start-off routine.  It's job is to take the
| arguments as put on the stack by EXEC, and to parse them and set them up the
| way _main expects them.

.text
.extern _main, _exit
.globl crtso, _environ
.globl begtext, begdata, begbss
begtext:
crtso:
		sub	bp,bp	| clear for backtrace of core files
		mov	bx,sp
		mov	cx,(bx)
		add	bx,*2
		mov	ax,cx
		inc	ax
		shl	ax,#1
		add	ax,bx
		mov	_environ,ax	| save envp in environ
		push	ax	| push environ
		push	bx	| push argv
		push	cx	| push argc
		call	_main
		add	sp,*6
		push	ax	| push exit status
		call	_exit

.data
begdata:
		.zerow 8	| food for null pointer bugs
_environ:	.word 0
.bss
begbss:

.define _send,_receive,_sendrec

| See ../h/com.h for C definitions
SEND = 1
RECEIVE = 2
BOTH = 3
SYSVEC = 32

|*========================================================================*
|                           send and receive                              *
|*========================================================================*
| send(), receive(), sendrec() save all registers but ax, bx, cx, dx and flags.
.globl _send, _receive, _sendrec
_send:
	pop dx			| return addr
	pop ax			| dest-src
	pop bx			| message pointer
	sub sp,*4
	push dx
	mov cx,*SEND		| send(dest, ptr)
	int SYSVEC		| trap to the kernel
	ret

_receive:
	pop dx
	pop ax
	pop bx
	sub sp,*4
	push dx
	mov cx,*RECEIVE		| receive(src, ptr)
	int SYSVEC		| trap to the kernel
	ret

_sendrec:
	pop dx
	pop ax
	pop bx
	sub sp,*4
	push dx
	mov cx,*BOTH		| sendrec(srcdest, ptr)
	int SYSVEC		| trap to the kernel
	ret

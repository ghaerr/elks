.extern _main
.globl begtext, begdata, begbss
.globl crtso, _data_org, _exit
.text

begtext:

crtso:
	j .0
	.zerow 13		! stack for inital IRET when common I&D
				! also padding to make INIT_SP same as
				! for separate I&D

.0:	mov sp,_stackpt
	call _main

.1:	jmp .1			! this will never be executed

_exit:	jmp _exit		! this will never be executed either

.data

begdata:

_data_org:			! fs needs to know where build stuffed table
.word 0xDADA			! magic number for build
.word 8				! CLICK_SHIFT to check - must match h/const.h
.word 0,0,0			! used by FS only for sizes of init
				! stack for separate I&D follows
.word 0,0,0			! for ip:ss:f pushed by debugger traps
.word 0,0,0			! for cs:ds:ret adr in save()
				! this was missing - a bug as late as V1.3c
				! for ds for new restart() as well
.word 0,0,0			! for ip:ss:f built by restart()
				! so INIT_SP in const.h must be 0x1C

.bss

begbss:

.extern _stackpt

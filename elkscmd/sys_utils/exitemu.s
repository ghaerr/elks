# Program to exit dosemu when it is running ELKS.
# by David Murn
export	_main
_main:
  mov ax,#0xffff
  int 0xe6
.data
.bss

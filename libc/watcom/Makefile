# Makefile of libc/watcom module

COMPILER ?= watcom
LIB ?= out.lib

include $(TOPDIR)/libc/$(COMPILER).inc

all:
	$(MAKE) -C syscall
	$(MAKE) -C asm

clean:
	$(MAKE) -C syscall clean
	$(MAKE) -C asm clean

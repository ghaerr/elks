# OpenWatcom Makefile for ELKS C Library 

include watcom.inc

#NOTBUILT = \
	asm	    \
	debug	\
	gcc	    \
	math	\
	crt0.S	\

SUBDIRS =	\
	watcom	\
	ctype	\
	error	\
	getent	\
	malloc	\
	misc	\
	net	    \
	regex	\
	stdio	\
	string	\
	termcap	\
	termios	\
	time	\
	# end of list

.PHONY: all
all:
	rm -f */*.lib watcom/*/*.lib
	$(MAKE) -C system -f out.mk COMPILER=watcom LIB=out.lib
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR COMPILER=watcom LIB=out.lib || exit 1; done
	rm -f c86/out.lib
	wlib -l=libc$(MODEL).lst -c -n -q -b -fo libc$(MODEL).lib */*.lib watcom/*/*.lib

.PHONY: clean
clean:
	rm -f system/*.ocj system/*.osj system/*.omj system/*.olj system/*.lib libc*.lst
	$(MAKE) -C watcom/syscall clean
	$(MAKE) -C watcom/asm clean
	for DIR in $(SUBDIRS); do rm -f $$DIR/*.ocj $$DIR/*.osj $$DIR/*.omj $$DIR/*.olj $$DIR/*.lib || exit 1; done
	rm -f libc*.lib

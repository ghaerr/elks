# C86 Makefile for ELKS C Library 

include c86.inc

#NOTBUILT = \
	asm	    \
	debug	\
	gcc	    \
	math	\
	crt0.S	\

SUBDIRS =	\
	c86	    \
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
	$(MAKE) -C system -f out.mk COMPILER=c86 LIB=out.lib
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR COMPILER=c86 LIB=out.lib || exit 1; done
	$(AR) $(ARFLAGS_SUB) libc86.a */*.lib

.PHONY: clean
clean:
	rm -f */*.i */*.as */*.ocj */*.lst */*.lib
	for DIR in $(SUBDIRS); do rm -f $$DIR/*.ocj $$DIR/*.lib || exit 1; done
	rm -f libc86.a

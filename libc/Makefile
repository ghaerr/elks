# Makefile of ELKS C Library

ifndef TOPDIR
$(error TOPDIR is not defined)
endif

include $(TOPDIR)/Make.defs

include Makefile.inc

# Defines

SUBDIRS = \
	asm \
	error \
	gcc \
	getent \
	malloc \
	misc \
	regex \
	stdio \
	string \
	system \
	termcap \
	termios \
	time \
	# end of list

#SUBDIRS=list

# Rules

.PHONY: all $(SUBDIRS)

all: $(LIBC) crt0.o

$(LIBC):: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ all
	sleep 1

#crt0.s: crt0.S
crt0.o: crt0.S

.PHONY: clean

clean:
	for DIR in $(SUBDIRS); do $(MAKE) -C $$DIR clean || exit 1; done
	rm -f *.o libc.a

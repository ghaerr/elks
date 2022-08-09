BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

HOSTCC = gcc
HOSTCFLAGS = -O3

###############################################################################

PRGS = testsym
OBJS = testsym.o syms.o
#LDFLAGS += -maout-heap=12000

all: nm86 $(PRGS)

nm86: nm86.c syms.c
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $^

testsym: $(OBJS)
	$(LD) $(LDFLAGS) -mno-post-link -o $@ $(OBJS) $(LDLIBS)
	elf2elks --symtab --heap 12000 $@
	./nm86 $@ > $@.map
	cp $@ $(TOPDIR)/elkscmd/rootfs_template/root

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(PRGS) *.o nm86

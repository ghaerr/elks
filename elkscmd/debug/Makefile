BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

HOSTCC = gcc
HOSTCFLAGS = -O3

###############################################################################

PRGS = testsym disasm
LIBOBJS += syms.o stacktrace.o printreg.o
#OBJS += ulltostr.o
CFLAGS += -fno-optimize-sibling-calls
CFLAGS += -fno-omit-frame-pointer
#CFLAGS += -finstrument-functions
#LDFLAGS += -maout-heap=12000

all: nm86 $(PRGS)

nm86: nm86.c syms.c
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $^

testsym: testsym.o $(LIBOBJS)
	$(LD) $(LDFLAGS) -mno-post-link -o $@ $^ $(LDLIBS)
	elf2elks --symtab --heap 12000 $@
	./nm86 $@ > $@.map
	cp $@ $(TOPDIR)/elkscmd/rootfs_template/root

disasm: dis.o disasm.o $(LIBOBJS)
	$(LD) $(LDFLAGS) -mno-post-link -o $@ $^ $(LDLIBS)
	elf2elks --symtab --heap 0xffff $@
	cp -p $@ $(TOPDIR)/elkscmd/rootfs_template/root
	-mkdir $(TOPDIR)/elkscmd/rootfs_template/lib
	cp -p $(TOPDIR)/elks/arch/i86/boot/system.sym $(TOPDIR)/elkscmd/rootfs_template/lib

disasm.o: disasm.c
	$(CC) $(CFLAGS) -fno-instrument-functions -c -o $*.o $<

ulltostr.o: ulltostr.c
	$(CC) $(CFLAGS) -fno-instrument-functions -c -o $*.o $<

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(PRGS) *.o nm86 testsym.map

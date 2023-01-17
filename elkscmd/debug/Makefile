BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

HOSTCC = gcc
HOSTCFLAGS = -O3

###############################################################################

PRGS = testsym disasm opcodes
LIBOBJS += instrument.o syms.o
LIBOBJS += stacktrace.o printreg.o

# disable tail call optimization for better stack traces
CFLAGS += -fno-optimize-sibling-calls
# always push BP in function prologue for better stack traces
CFLAGS += -fno-omit-frame-pointer
# optional instrumentation for function tracing
CFLAGS += -finstrument-functions-simple
# include symbol table in executable and heap to read it
LDFLAGS += -maout-symtab -maout-heap=12000

all: nm86 $(PRGS)

nm86: nm86.c syms.c
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $^

testsym: testsym.o $(LIBOBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	#elf2elks --symtab --heap 12000 $@
	./nm86 $@ > $@.map
	cp $@ $(TOPDIR)/elkscmd/rootfs_template/root

disasm: dis.o disasm.o $(LIBOBJS)
	$(LD) $(LDFLAGS) -maout-heap=0xffff -o $@ $^ $(LDLIBS)
	cp -p $@ $(TOPDIR)/elkscmd/rootfs_template/root
	-mkdir $(TOPDIR)/elkscmd/rootfs_template/lib
	cp -p $(TOPDIR)/elks/arch/i86/boot/system.sym $(TOPDIR)/elkscmd/rootfs_template/lib

opcodes: opcodes.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ $(TOPDIR)/elkscmd/rootfs_template/root

disasm.o: disasm.c
	$(CC) $(CFLAGS) -fno-instrument-functions -fno-instrument-functions-simple -c -o $*.o $<

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(PRGS) *.o nm86 testsym.map

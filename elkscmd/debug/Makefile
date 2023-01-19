BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS = testsym disasm opcodes
INSTRUMENTOBJS = instrument.o syms.o shared.o shared-asm.o
DISASMOBJS = dis.o disasm.o syms.o shared-asm.o
TESTOBJS   = testsym.o stacktrace.o printreg.o $(INSTRUMENTOBJS)

# disable tail call optimization for better stack traces
CFLAGS += -fno-optimize-sibling-calls
# always push BP in function prologue for better stack traces
CFLAGS += -fno-omit-frame-pointer
# optional instrumentation for function tracing
CFLAGS += -finstrument-functions-simple
# include symbol table in executable and heap to read it
LDFLAGS += -maout-symtab -maout-heap=12000

# added after CFLAGS for non-instrumented .c files in this directory
NOINSTFLAGS = -fno-instrument-functions -fno-instrument-functions-simple

HOSTPRGS = nm86

all: $(HOSTPRGS) $(PRGS)

nm86: nm86.c syms.c
	$(HOSTCC) $(HOSTCFLAGS) -o $@ $^

testsym: $(TESTOBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	./nm86 $@ > $@.map
	cp $@ $(TOPDIR)/elkscmd/rootfs_template/root

disasm: $(DISASMOBJS)
	$(LD) $(LDFLAGS) -maout-heap=0xffff -o $@ $^ $(LDLIBS)
	cp -p $@ $(TOPDIR)/elkscmd/rootfs_template/root
	-mkdir $(TOPDIR)/elkscmd/rootfs_template/lib
	cp -p $(TOPDIR)/elks/arch/i86/boot/system.sym $(TOPDIR)/elkscmd/rootfs_template/lib

opcodes: opcodes.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ $(TOPDIR)/elkscmd/rootfs_template/root

disasm.o: disasm.c
	$(CC) $(CFLAGS) $(NOINSTFLAGS) -c -o $*.o $<

dis.o: dis.c
	$(CC) $(CFLAGS) $(NOINSTFLAGS) -c -o $*.o $<

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(PRGS) $(HOSTPRGS) *.o *.map

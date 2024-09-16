BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS = testsym disasm nm opcodes

# disable tail call optimization for better stack traces
CFLAGS += -fno-optimize-sibling-calls
# always push BP in function prologue for better stack traces
CFLAGS += -fno-omit-frame-pointer
# optional instrumentation for function tracing
CFLAGS += -finstrument-functions-simple
# include symbol table in executable
LDFLAGS += -maout-symtab

# added after CFLAGS for non-instrumented .c files in this directory
#NOINSTFLAGS = -fno-instrument-functions -fno-instrument-functions-simple

HOSTPRGS = nm86 hostdisasm
HOSTCFLAGS += -I. -I$(TOPDIR)/elks/include
SYMS_C = $(TOPDIR)/libc/debug/syms.c

all: $(PRGS) system.sym

host: $(HOSTPRGS)

testsym: testsym.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
#	./nm86 $@ > $@.map

disasm: dis.o disasm.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

nm: nm86.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

system.sym:
	cp -p $(TOPDIR)/elks/arch/i86/boot/system.sym $(TOPDIR)/elkscmd/debug

opcodes: opcodes.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
#	cp $@ $(TOPDIR)/elkscmd/rootfs_template/root

disasm.o: disasm.c
	$(CC) $(CFLAGS) $(NOINSTFLAGS) -c -o $*.o $<

dis.o: dis.c
	$(CC) $(CFLAGS) $(NOINSTFLAGS) -c -o $*.o $<

nm86.o: nm86.c
	$(CC) $(CFLAGS) $(NOINSTFLAGS) -c -o $*.o $<

nm86: nm86.c $(SYMS_C)
	$(HOSTCC) $(HOSTCFLAGS) -D__far= -o $@ $^

hostdisasm: dis.c disasm.c $(SYMS_C)
	$(HOSTCC) $(HOSTCFLAGS) -D__far= -Wno-int-to-void-pointer-cast -Wno-format -o $@ $^

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	$(RM) $(PRGS) $(HOSTPRGS) *.o *.map system/sym

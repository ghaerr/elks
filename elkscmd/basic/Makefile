BASEDIR=..

include $(BASEDIR)/Makefile-rules

###############################################################################

CFLAGS += -Wno-maybe-uninitialized

###############################################################################

PRGS = basic
OBJS = basic.o host.o
#MAPFILE = -Wl,-Map=basic.map

# uncomment following four lines for --ftrace function tracing
#CFLAGS += -finstrument-functions-simple
#CFLAGS += -fno-optimize-sibling-calls -fno-omit-frame-pointer
#LDFLAGS += -maout-symtab -maout-heap=12000
#OBJS += ../debug/instrument.o ../debug/syms.o ../debug/shared.o ../debug/shared-asm.o

ifeq ($(CONFIG_ARCH_IBMPC), y)
HOST_SPECIFIC = host-ibmpc.o asm-ibmpc.o
endif

ifeq ($(CONFIG_ARCH_PC98), y)
HOST_SPECIFIC = host-pc98.o asm-pc98.o
endif

ifeq ($(CONFIG_ARCH_8018X), y)
HOST_SPECIFIC = host-8018x.o
endif

ifndef HOST_SPECIFIC
HOST_SPECIFIC = host-stubs.o
endif

OBJS += $(HOST_SPECIFIC)

all: $(PRGS)

$(PRGS): $(OBJS) basic.h host.h
	$(LD) -o $@ $(LDFLAGS) $(MAPFILE) $(OBJS) $(LDLIBS)

HOSTPRGS = hostbasic
.PHONY: host
host: $(HOSTPRGS)

HOSTSRC = basic.c host.c host-stubs.c
HOSTSRC += ../../libc/misc/ecvt.c
HOSTSRC += ../../libc/misc/dtostr.c

hostbasic: $(HOSTSRC)
	$(HOSTCC) $(HOSTCFLAGS) $(HOSTSRC) -o $@ -lm

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	$(RM) $(PRGS) $(HOSTPRGS) *.o *.map

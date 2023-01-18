BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

CFLAGS += -Wno-maybe-uninitialized

HOSTCC = gcc
HOST_CFLAGS = -O3

###############################################################################

PRGS = basic
OBJS = basic.o host.o
#MAPFILE = -Wl,-Map=basic.map

# uncomment following four lines for --ftrace function tracing
#CFLAGS += -finstrument-functions-simple
#CFLAGS += -fno-optimize-sibling-calls -fno-omit-frame-pointer
#LDFLAGS += -maout-symtab -maout-heap=12000
#OBJS += ../debug/instrument.o ../debug/syms.o ../debug/stacktrace.o ../debug/printreg.o

ifeq ($(CONFIG_ARCH_IBMPC), y)
OBJS += host-stubs.o
endif

ifeq ($(CONFIG_ARCH_8018X), y)
OBJS += host-8018x.o
endif

ifeq ($(CONFIG_ARCH_PC98), y)
OBJS += host-pc98.o intC5-pc98.o
endif

all: $(PRGS)

basic: $(OBJS) basic.h host.h
	$(LD) $(LDFLAGS) $(MAPFILE) -o basic $(OBJS) $(LDLIBS)

HOSTSRC = basic.c host.c host-stubs.c
HOSTSRC += ../../libc/misc/ecvt.c
HOSTSRC += ../../libc/misc/dtostr.c
hostbasic: $(HOSTSRC) host.h basic.h
	$(HOSTCC) $(HOST_CFLAGS) $(HOSTSRC) -o $@

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(PRGS) *.o hostbasic

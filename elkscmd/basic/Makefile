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

all: $(PRGS)

basic: basic.o host.o basic.h host.h
	$(LD) $(LDFLAGS) -o basic basic.o host.o $(LDLIBS)

HOSTSRC = basic.c host.c
HOSTSRC += ../../libc/misc/ecvt.c
HOSTSRC += ../../libc/stdio/__fp_print_func.c
hostbasic: $(HOSTSRC) host.h basic.h
	$(HOSTCC) $(HOST_CFLAGS) $(HOSTSRC) -o $@

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(PRGS) *.o hostbasic

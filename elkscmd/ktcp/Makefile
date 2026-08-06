# Makefile for ktcp
BASEDIR=..
include $(BASEDIR)/Makefile-rules

SHELL		= /bin/sh

CFILES		= ktcp.c slip.c ip.c icmp.c tcp.c tcp_cb.c tcp_output.c \
		  timer.c tcpdev.c netconf.c vjhc.c deveth.c arp.c udp.c dhcp.c

OBJS		= $(CFILES:.c=.o)

##############################################################################

all:	ktcp

ktcp:	$(OBJS)
	# heap=0xffff means "all the segment that is left" (elks/fs/exec.c:302), so
	# the heap tracks BSS automatically instead of silently eating the margin
	# when a buffer grows. Every connection control block comes out of it.
	$(LD) $(LDFLAGS) -maout-heap=0xffff -maout-stack=3072  -o ktcp $(OBJS) $(LDLIBS)

install: ktcp
	$(INSTALL) ktcp $(DESTDIR)/bin

clean: 
	$(RM) *.o ktcp

dep:
	makedepend $(CFILES)

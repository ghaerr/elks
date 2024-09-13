# Makefile for ktcp
BASEDIR=..
include $(BASEDIR)/Make.defs

SHELL		= /bin/sh

CFILES		= ktcp.c slip.c ip.c icmp.c tcp.c tcp_cb.c tcp_output.c \
		  timer.c tcpdev.c netconf.c vjhc.c deveth.c arp.c hexdump.c

OBJS		= $(CFILES:.c=.o)

##############################################################################

all:	ktcp

ktcp:	$(OBJS)
	$(LD) $(LDFLAGS) -maout-heap=32768 -maout-stack=3072  -o ktcp $(OBJS) $(LDLIBS)

install: ktcp
	$(INSTALL) ktcp $(DESTDIR)/bin

clean: 
	rm -f *~ *.o ktcp core 

dep:
	makedepend $(CFILES)

#######
# EOF #
#######

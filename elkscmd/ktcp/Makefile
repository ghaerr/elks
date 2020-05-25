# Makefile for ktcp
BASEDIR=..
include $(BASEDIR)/Make.defs

SHELL		= /bin/sh

CFILES		= ktcp.c slip.c ip.c icmp.c tcp.c tcp_cb.c tcp_output.c \
		  timer.c tcpdev.c netconf.c vjhc.c deveth.c arp.c

OBJS		= $(CFILES:.c=.o)

##############################################################################

all:	ktcp

ktcp:	$(OBJS)
	$(LD) $(LDFLAGS) -maout-chmem=0x8000 -o ktcp $(OBJS) $(LDLIBS)

lint:
	@for FILE in *.c ; do \
		echo '===>' "$${FILE}" ; \
		splint -weak -D__KERNEL__ $(LOCALFLAGS) "$${FILE}" \
			2>&1 > "$${FILE}.lint" ; \
	done

install: ktcp
	$(INSTALL) ktcp $(DESTDIR)/bin
	$(INSTALL) resolv.conf $(DESTDIR)/etc

clean: 
	rm -f *~ *.o ktcp core 

dep:
	makedepend $(CFILES)

#######
# EOF #
#######

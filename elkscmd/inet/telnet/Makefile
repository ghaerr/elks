# Makefile for telnet

BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

SRCS = telnet.c ttn_conf.c
OBJS = $(SRCS:.c=.o)

all:	telnet

telnet:	$(OBJS) $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(TINYPRINTF) $(LDLIBS)

install: telnet
	$(INSTALL) telnet $(DESTDIR)/bin

clean:
	rm -f $(OBJS) telnet

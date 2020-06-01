# Makefile for telnet

BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

SRCS = telnetd.c telnet.c
OBJS = $(SRCS:.c=.o) 
LDFLAGS += -maout-heap=1024 -maout-stack=1024

all:	telnetd

telnetd: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

install: telnetd
	$(INSTALL) telnetd $(DESTDIR)/bin

clean:
	rm -f $(OBJS) telnetd


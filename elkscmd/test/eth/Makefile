# Makefile for ethernet driver test

BASEDIR=../..

include $(BASEDIR)/Make.defs


CFILES		= eth.c

OBJS		= $(CFILES:.c=.o)

include $(BASEDIR)/Make.rules


all:	eth

eth:	$(OBJS)
	$(LD) $(LDFLAGS) -o eth eth.o $(LDLIBS)

install: eth
	$(INSTALL) eth $(DESTDIR)/bin

clean:
	rm -f *.o eth

dep:
	makedepend $(CFILES)

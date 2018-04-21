# Makefile for ethernet driver test

BASEDIR=../..

include $(BASEDIR)/Make.defs


CFILES		= eth.c

OBJS		= $(CFILES:.c=.o)

include $(BASEDIR)/Make.rules


all:	eth

eth:	$(OBJS)


install: eth
	sudo install eth $(TARGET_MNT)/bin

clean: 
	rm -f *.o eth

dep:
	makedepend $(CFILES)

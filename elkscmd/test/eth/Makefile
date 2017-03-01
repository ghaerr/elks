# Makefile for ethernet driver test

BASEDIR=../..

include $(BASEDIR)/Make.defs


CFILES		= eth.c

OBJS		= $(CFILES:.c=.o)

include $(BASEDIR)/Make.rules


all:	eth

eth:	$(OBJS)


smin_rfs:

min_rfs:

rfs:

max_rfs:

net_rfs:


install: all
	cp -p eth $(TARGET_MNT)/bin

clean: 
	rm -f *.o eth

dep:
	makedepend $(CFILES)

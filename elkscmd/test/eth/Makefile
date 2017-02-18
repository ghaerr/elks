# Makefile for eth

BASEDIR=../..

include $(BASEDIR)/Make.defs


CFILES		= eth.c

OBJS		= $(CFILES:.c=.o)

include $(BASEDIR)/Make.rules


all:	eth

eth:	$(OBJS)

min_rfs:

smin_rfs:

max_rfs: install

rfs: install

net_rfs: install

install: eth
	cp -p eth $(TARGET_MNT)/bin/eth

clean: 
	rm -f *.o eth

dep:
	makedepend $(CFILES)

# Makefile for pseudo tty test

BASEDIR=../..

include $(BASEDIR)/Make.defs


CFILES		= testprog.c

OBJS		= $(CFILES:.c=.o)

include $(BASEDIR)/Make.rules


all:	testprog

eth:	$(OBJS)


smin_rfs: install

min_rfs:

rfs: install

max_rfs: install

net_rfs: install


install: all
	cp -p testprog $(TARGET_MNT)/bin

clean: 
	rm -f *.o testprog

dep:
	makedepend $(CFILES)

# Makefile for pseudo tty test

BASEDIR=../..

include $(BASEDIR)/Make.defs


CFILES		= ptyshell.c

OBJS		= $(CFILES:.c=.o)

include $(BASEDIR)/Make.rules


all:	ptyshell

eth:	$(OBJS)


smin_rfs: install

min_rfs:

rfs: install

max_rfs: install

net_rfs: install


install: all
	cp -p ptyshell $(TARGET_MNT)/bin

clean: 
	rm -f *.o ptyshell

dep:
	makedepend $(CFILES)

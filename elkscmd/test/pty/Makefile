# Makefile for pseudo tty test

BASEDIR=../..

include $(BASEDIR)/Make.defs


CFILES		= ptyshell.c

OBJS		= $(CFILES:.c=.o)

include $(BASEDIR)/Make.rules


all:	ptyshell

ptyshell: $(OBJS)
	$(LD) $(LDFLAGS) -o ptyshell ptyshell.o $(LDLIBS)

install: ptyshell
	sudo install ptyshell $(TARGET_MNT)/bin

clean: 
	rm -f *.o ptyshell

dep:
	makedepend $(CFILES)

# Makefile for urlget

include ../../Make.defs

CC=bcc

CFLAGS=	-O 
#CFLAGS=  -D__LINUX__

LDFLAGS=-i

SRC=	net.c urlget.c

all:	urlget

urlget:	$(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o urlget $(SRC)

install: all
	cp -p urlget $(TARGET_MNT)/bin

clean:
	rm -f urlget *.o


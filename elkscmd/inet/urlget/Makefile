# Makefile for urlget

BASEDIR=../..

include $(BASEDIR)/Make.defs

#CFLAGS=-O 

#LDFLAGS=-i

SRC=	net.c urlget.c

all:	urlget

urlget:	$(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o urlget $(SRC)

install: all
	cp -p urlget $(TARGET_MNT)/bin

clean:
	rm -f urlget *.o


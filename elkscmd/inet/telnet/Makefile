# Makefile for telnet

include ../../Make.defs

CC=bcc

CFLAGS=	-O 
#CFLAGS=  -D__LINUX__ 

LDFLAGS=-i

SRC=	ttn.c ttn_conf.c

all:	ttn

ttn:	$(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o telnet $(SRC)

install: all
	cp -p telnet $(TARGET_MNT)/bin

clean:
	rm -f telnet


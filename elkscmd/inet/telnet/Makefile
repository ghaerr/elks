# Makefile for telnet

BASEDIR=../..

include $(BASEDIR)/Make.defs

#LDFLAGS=-i

SRC=	ttn.c ttn_conf.c

all:	ttn

ttn:	$(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o telnet $(SRC)

install: all
	cp -p telnet $(TARGET_MNT)/bin

clean:
	rm -f telnet


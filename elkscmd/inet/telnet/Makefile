# Makefile for telnet

CC=bcc

CFLAGS=	-O 
#CFLAGS=  -D__LINUX__ 

LDFLAGS=-i

SRC=	ttn.c ttn_conf.c

all:	ttn

ttn:	$(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o telnet $(SRC)

clean:
	rm -f telnet


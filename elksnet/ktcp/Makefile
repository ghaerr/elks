# Makefile for ktcp

SHELL = /bin/sh
CC = bcc

BASE_CFLAGS = -c -g 
#BASE_CFLAGS = -c -O

CFLAGS = -D__KERNEL__ $(BASE_CFLAGS)

CFILES = \
	ktcp.c \
	slip.c \
	ip.c \
	icmp.c \
	tcp.c \
	tcp_cb.c \
	tcp_output.c \
	timer.c \
	tcpdev.c


OBJ = $(CFILES:.c=.o)

all: ktcp

.c.o:
	$(CC) $(CFLAGS) -o $@ $<

ktcp: $(OBJ)
	$(CC) $(OPT) -o ktcp $(OBJ) $(BASELIBS)

clean: 
	rm -f *\.o ktcp core

dep:
	makedepend $(CFILES)



# Makefile for ktcp

SHELL		= /bin/sh

CC		= bcc

INCDIR		= `pwd`/../../elks/include

#BASE_CFLAGS	= -c -g -I$(INCDIR)
BASE_CFLAGS	= -c -O -I$(INCDIR)

CFLAGS		= -D__KERNEL__ $(BASE_CFLAGS)

CFILES		= ktcp.c slip.c ip.c icmp.c tcp.c tcp_cb.c tcp_output.c \
		  timer.c tcpdev.c netconf.c

OBJ		= $(CFILES:.c=.o)

all:	ktcp

.c.o:
	$(CC) $(CFLAGS) -o $@ $<

ktcp:	$(OBJ)
	$(CC) $(OPT) -o ktcp $(OBJ) $(BASELIBS)

clean: 
	rm -f *\.o ktcp core

dep:
	makedepend $(CFILES)



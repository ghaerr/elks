# Makefile for httpd

SHELL = /bin/sh
CC = bcc

BASE_CFLAGS = -c -g 
#BASE_CFLAGS = -c -O

#CFLAGS = -D__LINUX__ $(BASE_CFLAGS)
CFLAGS = $(BASE_CFLAGS)

CFILES = \
	httpd.c \

OBJ = $(CFILES:.c=.o)

all: httpd

.c.o:
	$(CC) $(CFLAGS) -o $@ $<

httpd: $(OBJ)
	$(CC) $(OPT) -o httpd $(OBJ) $(BASELIBS)

clean: 
	rm -f *\.o httpd core

dep:
	makedepend $(CFILES)




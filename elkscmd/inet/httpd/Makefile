# Makefile for httpd

include ../../Make.defs

SHELL = /bin/sh
CC = bcc

BASE_CFLAGS = -c -O
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

install: all
	mkdir $(TARGET_MNT)/usr/lib/httpd
	cp -f -p sample_index.html $(TARGET_MNT)/usr/lib/httpd/index.html
	cp -p httpd $(TARGET_MNT)/sbin

clean: 
	rm -f *\.o httpd core

dep:
	makedepend $(CFILES)




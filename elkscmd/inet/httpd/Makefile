# Makefile for httpd

BASEDIR=../..

include $(BASEDIR)/Make.defs

SHELL = /bin/sh

#CFLAGS = -c -O

CFILES = httpd.c

OBJ = $(CFILES:.c=.o)

all: httpd

#.c.o:
#	$(CC) $(CFLAGS) -o $@ $<

httpd: $(OBJ)
	$(CC) $(OPT) -o httpd $(OBJ) $(BASELIBS)

install: all
	mkdir $(TARGET_MNT)/usr/lib/httpd
	cp -f -p sample_index.html $(TARGET_MNT)/usr/lib/httpd/index.html
	cp -p httpd $(TARGET_MNT)/bin

clean: 
	rm -f core httpd $(OBJ)

dep:
	makedepend $(CFILES)

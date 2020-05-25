# Makefile for bc
#
# A makefile for bc.  This is part of the bc/sbc distribution.
#
# $Id$
###############################################################################
#
# Include standard packaging commands.

BASEDIR 	= ..
include $(BASEDIR)/Make.defs

###############################################################################
#
#  Make sure these have the correct directories for your machine.
#
#  LIBDIR and BINDIR are where bc and libmath.b will be put.

PREFIX =
LIBDIR = $(PREFIX)/lib
BINDIR = $(PREFIX)/bin

# Programs definitions for use by make.

SHELL = /bin/sh

YACC = yacc

LEX = flex -I -8

LOCALFLAGS=-D_POSIX_SOURCE

# Try compiling this program using the `stdcall' calling convention...
CFLBASE += -mrtd

# For ELKS, bc needs more data segment space than the kernel-given default.
LDFLAGS += -maout-chmem=46000

OFILES = scan.o util.o main.o number.o storage.o load.o execute.o 

SUBDIRS = Examples Test

all: fbc bc

bc: $& config.h bc.o $(OFILES) global.o
	$(LD) -o bc $(LDFLAGS) bc.o $(OFILES) global.o $(LDLIBS)

sbc: sbc.o $(OFILES) global.o
	$(LD) -o sbc $(LDFLAGS) sbc.o $(OFILES) global.o $(LDLIBS)

math.h: libmath.b
	$(MAKE) -$(MAKEFLAGS) fbc
	elksemu ./fbc -c libmath.b </dev/null >math.h
	./fix_math.h
	rm -f ./fbc

fbc: $(OFILES) bc.o
	echo \"\" > math.h
	$(CC) -c $(CFLAGS) global.c
	$(LD) -o fbc $(LDFLAGS) bc.o $(OFILES) global.o $(LDLIBS)

install: bc fbc
	$(INSTALL) bc $(DESTDIR)/bin
#	$(INSTALL) fbc $(DESTDIR)/bin

clean:
	rm -f *.o *.bak core math.h bc fbc sbc bc.c sbc.c scan.c y.tab.h

scan.c: scan.l
	$(LEX) scan.l
	mv lex.yy.c scan.c

y.tab.h bc.c: bc.y
	@echo "expect 1 shift/reduce conflict"
	$(YACC) -d bc.y
	mv y.tab.c bc.c

sbc.c: sbc.y
	$(YACC) -d sbc.y
	mv y.tab.c sbc.c

global.o: bcdefs.h global.h math.h
bc.o:	bcdefs.h global.h 
execute.o: bcdefs.h global.h
load.o: bcdefs.h global.h 
main.o: bcdefs.h global.h version.h
number.o: bcdefs.h
sbc.o: bcdefs.h global.h 
scan.o: y.tab.h bcdefs.h global.h
storage.o: bcdefs.h global.h
util.o: bcdefs.h global.h version.h

bcdefs.h: number.h const.h config.h
	touch bcdefs.h

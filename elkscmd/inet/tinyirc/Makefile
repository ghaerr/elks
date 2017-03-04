# tinyirc makefile
# by Nathan Laredo
#
# I don't wish to assert any rights (copyright) over this makefile
# but please give me credit if you use my code.
#
SERVER = irc.freenode.net
PORT = 7000
#

BASEDIR=../..
include $(BASEDIR)/Make.defs

all: elks

default:
	## Please use "make target"
	## where target is one of the following
	##
	## aix    hpux    gnu    posix    elks   generic    debug
	##
	## If you have trouble with the input line, try a different target

debug:
	$(MAKE) tinyirc CFLAGS=-g LDFLAGS=-g CC=gcc LIBS=-ltermcap

generic:
	$(MAKE) tinyirc CFLAGS=-O LDFLAGS=-s LIBS=-ltermcap

elks:
	$(MAKE) tinyirc CFLAGS="-0 -O -ansi -DPOSIX -DELKS -I$(INC_DIR)" LDFLAGS= LIBS= CC=bcc

aix:
	$(MAKE) tinyirccv CFLAGS="-O -D_AIX_" \
		LDFLAGS=-s LIBS=-lcurses CC=bsdcc

posix:
	$(MAKE) tinyirc CFLAGS="-O2 -m486 -DPOSIX" LDFLAGS="-s" LIBS=-ltermcap

gnu:
	$(MAKE) tinyirc CFLAGS="-O2 -pipe -DPOSIX -Wall -Wunused -Wformat" \
		LDFLAGS=-s LIBS=-ltermcap CC=gcc

hpux:
	$(MAKE) tinyirccv LDFLAGS=-s LIBS=-lcurses

ntest:
	$(MAKE) tinyirccv CFLAGS="-O -I/usr/include/ncurses -DPOSIX" \
		LDFLAGS="-L/usr/local/lib" LIBS=-lncurses CC=gcc

ctest:
	$(MAKE) tinyirccv CFLAGS=-O LDFLAGS=-s LIBS=-lcurses

clean:
	rm -f tinyirc.o tinyirccv.o tinyirc

DEFINES = -DDEFAULTSERVER=\"$(SERVER)\" -DDEFAULTPORT=$(PORT)

tinyirc: tinyirc.o
	$(CC) $(LDFLAGS) -o tinyirc tinyirc.o $(LIBS)

tinyirccv: tinyirccv.o
	$(CC) $(LDFLAGS) -o tinyirc tinyirccv.o $(LIBS)

tinyirc.o: tinyirc.c Makefile
	$(CC) $(CFLAGS) $(DEFINES) -c tinyirc.c -o tinyirc.o

tinyirccv.o: tinyirccv.c Makefile
	$(CC) $(CFLAGS) $(DEFINES) -c tinyirccv.c -o tinyirccv.o

install:

smin_rfs:

min_rfs:

rfs:

max_rfs:

net_rfs:

#EOF

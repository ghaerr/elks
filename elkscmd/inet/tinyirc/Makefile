# tinyirc makefile
# by Nathan Laredo
#
# I don't wish to assert any rights (copyright) over this makefile
# but please give me credit if you use my code.
#
# chat.freenode.net=
SERVER = 162.213.39.42
PORT = 8000
#

BASEDIR=../..
include $(BASEDIR)/Make.defs

LOCALFLAGS=-DPOSIX -DELKS

#
# Rules
#

include $(BASEDIR)/Make.rules

all: tinyirc

LOCALDEFS = -DDEFAULTSERVER=\"$(SERVER)\" -DDEFAULTPORT=$(PORT)

tinyirc: tinyirc.o
	$(LD) $(LDFLAGS) -o tinyirc tinyirc.o $(LDLIBS)

tinyircd: tinyircd.o
	$(LD) $(LDFLAGS) -o tinyircd tinyircd.o $(LDLIBS)

tinyirccv: tinyirccv.o
	$(CC) $(LDFLAGS) -o tinyirc tinyirccv.o $(LIBS)

tinyirc.o: tinyirc.c Makefile
	$(CC) $(CFLAGS) $(LOCALDEFS) -c tinyirc.c -o tinyirc.o

tinyirccv.o: tinyirccv.c Makefile
	$(CC) $(CFLAGS) $(LOCALDEFS) -c tinyirccv.c -o tinyirccv.o

install: tinyirc
	$(INSTALL) tinyirc $(DESTDIR)/bin

clean:
	rm -f core *.o tinyirc

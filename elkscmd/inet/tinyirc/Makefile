# Makefile for tinyirc

BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

# configurable options
# chat.freenode.net=
SERVER = 162.213.39.42
PORT = 8000

all: tinyirc

LOCALFLAGS = -DPOSIX -DELKS -DDEFAULTSERVER=\"$(SERVER)\" -DDEFAULTPORT=$(PORT)

tinyirc: tinyirc.o
	$(LD) $(LDFLAGS) -o $@ -maout-heap=8192 $^ $(LDLIBS)

tinyircd: tinyircd.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

install: tinyirc
	$(INSTALL) tinyirc $(DESTDIR)/bin

clean:
	rm -f *.o tinyirc

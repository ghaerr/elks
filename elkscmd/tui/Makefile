BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS = fm matrix cons ttyinfo sl

#PRGS_HOST =

CFLAGS += -DELKS=1
TUILIB = tty.o runes.o unikey.o

all: $(PRGS) $(PRGS_HOST)

fm: fm.o strverscmp.o realpath.o strlcpy.o strlcat.o curses.o curses2.o $(TUILIB) v7malloc.o
	$(LD) $(LDFLAGS) -maout-heap=0xffff -o $@ $^ $(LDLIBS)

matrix: matrix.o tty-cp437.o kcp437.o tty.o runes.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

cons: cons.o tty-cp437.o kcp437.o tty.o runes.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

ttyinfo: ttyinfo.o $(TUILIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

sl: sl.o curses.o tty.o unikey.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(PRGS) $(PRGS_HOST) *~ *.o

BASEDIR=..

include $(BASEDIR)/Makefile-rules

###############################################################################

CFLAGS += -DELKS=1

###############################################################################

PRGS = fm matrix cons ttyinfo sl ttyclock ttypong ttytetris invaders

#PRGS_HOST =

TUILIB = tty.o runes.o unikey.o
MALLOC=dmalloc.o

all: $(PRGS) $(PRGS_HOST)

fm: fm.o strverscmp.o realpath.o strlcpy.o strlcat.o curses.o curses2.o $(TUILIB) $(MALLOC)
	$(LD) $(LDFLAGS) -maout-heap=0xffff -o $@ $^ $(LDLIBS)

matrix: matrix.o tty-cp437.o kcp437.o tty.o runes.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

cons: cons.o tty-cp437.o kcp437.o tty.o runes.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

ttyinfo: ttyinfo.o $(TUILIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

sl: sl.o curses.o tty.o unikey.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

ttyclock: ttyclock.o curses.o curses2.o curses3.o tty.o unikey.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

ttypong: ttypong.o curses.o curses2.o tty.o unikey.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

ttytetris: ttytetris.o tetris-frame.o tetris-shapes.o tetris-util.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

invaders: invaders.o curses.o curses2.o tty.o unikey.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	$(RM) $(PRGS) $(PRGS_HOST) *.o

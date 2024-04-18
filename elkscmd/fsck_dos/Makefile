BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

LOCALCFLAGS = -Dlint -DELKS=1
# remove next line when FAT32 supported
LOCALCFLAGS += -Wno-shift-count-overflow

PRGS = fsck-dos

all: $(PRGS)

install: all
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

main.o: main.c
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c -o $*.o $<

boot.o: boot.c
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c -o $*.o $<

check.o: check.c
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c -o $*.o $<

dir.o: dir.c
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c -o $*.o $<

fat.o: fat.c
	$(CC) $(LOCALCFLAGS) $(CFLAGS) -c -o $*.o $<

fsck-dos: main.o boot.o check.o dir.o fat.o
	$(LD) $(LDFLAGS) -maout-heap=0xffff -o $@ $^ $(LDLIBS)

clean:
	rm -f *.o $(PRGS)

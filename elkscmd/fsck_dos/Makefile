BASEDIR=..

include $(BASEDIR)/Makefile-rules

###############################################################################

LOCALCFLAGS = -Dlint -DELKS=1 -D__huge=
# remove next line when FAT32 supported
LOCALCFLAGS += -Wno-shift-count-overflow

###############################################################################

PRGS = fsck-dos
ifdef WATCOM
PRGS += fsck-dos.os2
endif

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

fsck-dos.os2:
	ewcc -Dlint=1 -DELKS=1 boot.c check.c dir.c fat.c main.c mem.c
	ewlink -o $@ boot.obj check.obj dir.obj fat.obj main.obj mem.obj
	cp $@ $(TOPDIR)/elkscmd/rootfs_template/bin

clean:
	$(RM) *.o $(PRGS) *.obj *.os2
	$(RM) $(TOPDIR)/elkscmd/rootfs_template/bin/fsck-dos.os2

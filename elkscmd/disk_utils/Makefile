BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PROGS=fdisk ramdisk mkfs fsck partype

SPROGS=mkfs

all: $(PROGS)

install_sibo: all
	$(INSTALL) $(SPROGS) $(DESTDIR)/sbin

install: all
	$(INSTALL) $(PROGS) $(DESTDIR)/sbin

fsck: fsck.o
	$(LD) $(LDFLAGS) -o fsck fsck.o $(LDLIBS)

fdisk: fdisk.o
	$(LD) $(LDFLAGS) -o fdisk fdisk.o $(LDLIBS)

mkfs: mkfs.o
	$(LD) $(LDFLAGS) -o mkfs mkfs.o $(LDLIBS)

partype: partype.o
	$(LD) $(LDFLAGS) -o partype partype.o $(LDLIBS)

ramdisk: ramdisk.o
	$(LD) $(LDFLAGS) -o ramdisk ramdisk.o $(LDLIBS)

clean:
	rm -f *.o $(FORMATMOD) core $(PROGS)

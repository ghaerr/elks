BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS = ramdisk mkfs fsck partype fdisk

SPRGS=mkfs

all: $(PRGS)

install_sibo: all
	$(INSTALL) $(SPRGS) $(DESTDIR)/bin

install: all
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

fsck: fsck.o
	$(LD) $(LDFLAGS) -o fsck -maout-heap=0xffff fsck.o $(LDLIBS)

fdisk: fdisk.o
	$(LD) $(LDFLAGS) -o fdisk fdisk.o $(LDLIBS)

mkfs: mkfs.o
	$(LD) $(LDFLAGS) -o mkfs mkfs.o $(LDLIBS)

partype: partype.o
	$(LD) $(LDFLAGS) -o partype partype.o $(LDLIBS)

ramdisk: ramdisk.o
	$(LD) $(LDFLAGS) -o ramdisk ramdisk.o $(LDLIBS)

clean:
	rm -f *.o $(FORMATMOD) core $(PRGS)

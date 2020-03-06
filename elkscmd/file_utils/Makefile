BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

OUTS = grep ln ls mkdir mkfifo mknod more mv rm rmdir sync touch
OUTS_BE += cat chgrp chmod chown cmp cp dd

PRGS = $(OUTS)
ifeq "$(CONFIG_APP_FILE_UTILS)" "y"
	PRGS += $(OUTS_BE)
endif

all: $(PRGS)

cat: cat.o
	$(LD) $(LDFLAGS) -o cat cat.o $(LDLIBS)

chgrp: chgrp.o
	$(LD) $(LDFLAGS) -o chgrp chgrp.o $(LDLIBS)

chmod: chmod.o
	$(LD) $(LDFLAGS) -o chmod chmod.o $(LDLIBS)

chown: chown.o
	$(LD) $(LDFLAGS) -o chown chown.o $(LDLIBS)

cmp: cmp.o
	$(LD) $(LDFLAGS) -o cmp cmp.o $(LDLIBS)

cp: cp.o
	$(LD) $(LDFLAGS) -o cp cp.o $(LDLIBS)

dd: dd.o
	$(LD) $(LDFLAGS) -o dd dd.o $(LDLIBS)

grep: grep.o
	$(LD) $(LDFLAGS) -o grep grep.o $(LDLIBS)

l: l.o
	$(LD) $(LDFLAGS) -o l l.o $(LDLIBS)

ln: ln.o
	$(LD) $(LDFLAGS) -o ln ln.o $(LDLIBS)

ls: ls.o
	$(LD) $(LDFLAGS) -o ls ls.o $(LDLIBS)

mkdir: mkdir.o
	$(LD) $(LDFLAGS) -o mkdir mkdir.o $(LDLIBS)

mkfifo: mkfifo.o
	$(LD) $(LDFLAGS) -o mkfifo mkfifo.o $(LDLIBS)

mknod: mknod.o
	$(LD) $(LDFLAGS) -o mknod mknod.o $(LDLIBS)

more: more.o
	$(LD) $(LDFLAGS) -o more more.o $(LDLIBS)

mv: mv.o
	$(LD) $(LDFLAGS) -o mv mv.o $(LDLIBS)

rm: rm.o
	$(LD) $(LDFLAGS) -o rm rm.o $(LDLIBS)

rmdir: rmdir.o
	$(LD) $(LDFLAGS) -o rmdir rmdir.o $(LDLIBS)

sync: sync.o
	$(LD) $(LDFLAGS) -o sync sync.o $(LDLIBS)

touch: touch.o
	$(LD) $(LDFLAGS) -o touch touch.o $(LDLIBS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(OUTS) $(OUTS_BE) *.o

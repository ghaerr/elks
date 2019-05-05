BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

LOCALFLAGS=-D_POSIX_SOURCE

PRGS =	banner cksum cut decomp16 du fgrep grep proto sum uniq wc

NETPRGS = decomp16 du grep wc

all: $(PRGS)

banner: banner.o
	$(LD) $(LDFLAGS) -o banner banner.o $(LDLIBS)

cksum: cksum.o
	$(LD) $(LDFLAGS) -o cksum cksum.o $(LDLIBS)

cut: cut.o
	$(LD) $(LDFLAGS) -o cut cut.o $(LDLIBS)

decomp16: decomp16.o
	$(LD) $(LDFLAGS) -o decomp16 decomp16.o $(LDLIBS)

du: du.o
	$(LD) $(LDFLAGS) -o du du.o $(LDLIBS)

fgrep: fgrep.o
	$(LD) $(LDFLAGS) -o fgrep fgrep.o $(LDLIBS)

grep: grep.o
	$(LD) $(LDFLAGS) -o grep grep.o $(LDLIBS)

proto: proto.o
	$(LD) $(LDFLAGS) -o proto proto.o $(LDLIBS)

sum: sum.o
	$(LD) $(LDFLAGS) -o sum sum.o $(LDLIBS)

uniq: uniq.o
	$(LD) $(LDFLAGS) -o uniq uniq.o $(LDLIBS)

wc: wc.o
	$(LD) $(LDFLAGS) -o wc wc.o $(LDLIBS)

install: $(PRGS)
	install $(PRGS) $(TARGET_MNT)/bin

clean:
	rm -f $(PRGS) *.o

BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

LOCALFLAGS=-D_POSIX_SOURCE

PRGS = banner decomp16 fgrep grep proto sum uniq wc cksum cut du

all: $(PRGS)

#banner: LDFLAGS += -maout-symtab
banner: banner.o
	$(LD) $(LDFLAGS) -o banner banner.o $(LDLIBS)

cksum: cksum.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o cksum cksum.o $(TINYPRINTF) $(LDLIBS)

cut: cut.o
	$(LD) $(LDFLAGS) -o cut cut.o $(LDLIBS)

decomp16: decomp16.o
	$(LD) $(LDFLAGS) -o decomp16 decomp16.o $(LDLIBS)

du: du.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o du du.o $(TINYPRINTF) $(LDLIBS)

fgrep: fgrep.o
	$(LD) $(LDFLAGS) -o fgrep fgrep.o $(LDLIBS)

grep: grep.o
	$(LD) $(LDFLAGS) -o grep grep.o $(LDLIBS)

proto: proto.o
	$(LD) $(LDFLAGS) -o proto proto.o $(LDLIBS)

sum: sum.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o sum sum.o $(TINYPRINTF) $(LDLIBS)

uniq: uniq.o
	$(LD) $(LDFLAGS) -o uniq uniq.o $(LDLIBS)

wc: wc.o
	$(LD) $(LDFLAGS) -o wc wc.o $(LDLIBS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(PRGS) *.o

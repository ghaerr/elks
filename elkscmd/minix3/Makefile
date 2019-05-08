BASEDIR=..

LOCALFLAGS=

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=cal diff file find head sed sort tail tee

#
# Rules
#

cal: cal.o
	$(LD) $(LDFLAGS) -o cal cal.o $(LDLIBS)

diff: diff.o
	$(LD) $(LDFLAGS) -o diff diff.o $(LDLIBS)

file: file.o
	$(LD) $(LDFLAGS) -o file file.o $(LDLIBS)

find: find.o
	$(LD) $(LDFLAGS) -o find find.o $(LDLIBS)

head: head.o
	$(LD) $(LDFLAGS) -o head head.o $(LDLIBS)

sed: sed.o
	$(LD) $(LDFLAGS) -o sed sed.o $(LDLIBS)

sort: sort.o
	$(LD) $(LDFLAGS) -o sort sort.o $(LDLIBS)

tail: tail.o
	$(LD) $(LDFLAGS) -o tail tail.o $(LDLIBS)

tee: tee.o
	$(LD) $(LDFLAGS) -o tee tee.o $(LDLIBS)


include $(BASEDIR)/Make.rules

all: $(PRGS)


install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f core *.o $(PRGS)

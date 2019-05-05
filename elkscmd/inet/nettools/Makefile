BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=netstat nslookup

LOCALFLAGS=-I$(ELKSCMD_DIR)

all: $(PRGS)

install: $(PRGS)
	install $(PRGS) $(TARGET_MNT)/bin

netstat: netstat.o
	$(LD) $(LDFLAGS) netstat.o -o netstat $(LDLIBS)

nslookup: nslookup.o
	$(LD) $(LDFLAGS) nslookup.o -o nslookup $(LDLIBS)

clean:
	rm -f core *.o $(PRGS)

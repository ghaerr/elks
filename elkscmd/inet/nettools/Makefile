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
	sudo install $(PRGS) $(TARGET_MNT)/bin

netstat: netstat.o
	$(CC) $(CFLAGS) $(LDFLAGS) netstat.o -o netstat

nslookup: nslookup.o
	$(CC) $(CFLAGS) $(LDFLAGS) nslookup.o -o nslookup

clean:
	rm -f core *.o $(PRGS)

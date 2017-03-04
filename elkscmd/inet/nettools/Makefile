BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=netstat nslookup

SPRGS=

CFLAGS=-0 -O -ansi -I$(ELKSCMD_DIR) -I$(ELKS_DIR)/include

all: $(PRGS)

max_rfs: install

rfs:

net_rfs: install

min_rfs:

smin_rfs:

install_sibo: all
	cp -p $(SPRGS) $(TARGET_MNT)/bin

install: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

netstat: netstat.o
	$(CC) $(CFLAGS) $(LDFLAGS) netstat.o -o netstat

nslookup: nslookup.o
	$(CC) $(CFLAGS) $(LDFLAGS) nslookup.o -o nslookup

clean:
	rm -f core *.o $(PRGS) $(SPRGS)

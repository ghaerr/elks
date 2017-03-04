BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

STD_USR_BIN    = ed tar compress uncompress zcat miniterm

include $(BASEDIR)/Make.rules

HOSTCC = gcc
HOST_CFLAGS = -O2

###############################################################################

PRGS=ed tar compress compress.host uncompress zcat miniterm

all: $(PRGS)

compress: compress.c Makefile
	$(CC) $(CFLAGS) -H0xe000 -s $@.c -o $@

compress.host: compress.c
	$(HOSTCC) $(HOST_CFLAGS) -s compress.c -o $@

uncompress: compress
	ln -sf compress uncompress

zcat: compress
	ln -sf compress zcat

max_rfs: all
	cp -p $(STD_USR_BIN) $(TARGET_MNT)/usr/bin

rfs: all
	cp -p $(STD_USR_BIN) $(TARGET_MNT)/usr/bin

net_rfs: rfs

min_rfs:

smin_rfs:

clean:
	rm -f core $(PRGS) *~ *.o


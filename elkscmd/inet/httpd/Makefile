BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=httpd

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
	mkdir -p $(TARGET_MNT)/usr/lib/httpd
	cp -f -p sample_index.html $(TARGET_MNT)/usr/lib/httpd/index.html

netstat: httpd.o
	$(CC) $(CFLAGS) $(LDFLAGS) httpd.o -o httpd

clean:
	rm -f core *.o $(PRGS) $(SPRGS)

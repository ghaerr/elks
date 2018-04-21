BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=httpd

SPRGS=

LOCALFLAGS=-I$(ELKSCMD_DIR)

all: $(PRGS)

install_sibo: all
	cp -p $(SPRGS) $(TARGET_MNT)/bin

install: $(PRGS)
	sudo install $(PRGS) $(TARGET_MNT)/bin
	sudo mkdir -p $(TARGET_MNT)/usr/lib/httpd
	sudo install sample_index.html $(TARGET_MNT)/usr/lib/httpd/index.html

netstat: httpd.o
	$(CC) $(CFLAGS) $(LDFLAGS) httpd.o -o httpd

clean:
	rm -f core *.o $(PRGS) $(SPRGS)

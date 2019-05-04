BASEDIR=../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRG=httpd

LOCALFLAGS=-I$(ELKSCMD_DIR)

all: $(PRG)

$(PRG): httpd.o
	$(LD) $(LDFLAGS) -o $(PRG) httpd.o $(LDLIBS)

install: $(PRG)
	$(SUDO) install $(PRG) $(TARGET_MNT)/bin
	$(SUDO) mkdir -p $(TARGET_MNT)/var/www
	$(SUDO) install sample_index.html $(TARGET_MNT)/var/www/index.html

clean:
	rm -f *.o $(PRG)

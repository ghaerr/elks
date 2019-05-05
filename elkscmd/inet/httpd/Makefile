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
	install $(PRG) $(TARGET_MNT)/bin
	mkdir -p $(TARGET_MNT)/var/www
	install sample_index.html $(TARGET_MNT)/var/www/index.html

clean:
	rm -f *.o $(PRG)

BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

DIRS=nettools httpd telnet tinyirc urlget telnetd

all:
	for i in $(DIRS); do make -C $$i all || exit 1 ; done

install:
	for i in $(DIRS); do make -C $$i install || exit 1; done

clean:
	for i in $(DIRS); do make -C $$i clean || exit 1; done

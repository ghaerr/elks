BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

# All subdirectories to build & clean

SUBDIRS = \
    httpd \
    nettools \
    telnet \
    telnetd \
    tinyirc \
    urlget \
    # EOL

all:
	for i in $(SUBDIRS); do $(MAKE) -C $$i all || exit 1 ; done

install:
	for i in $(SUBDIRS); do $(MAKE) -C $$i install || exit 1; done

clean:
	for i in $(SUBDIRS); do $(MAKE) -C $$i clean || exit 1; done

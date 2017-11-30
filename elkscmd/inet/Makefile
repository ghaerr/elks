BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

DIRS=nettools httpd telnet tinyirc urlget telnetd

DIRS2=

all:
	for i in $(DIRS); do make -C $$i all || exit 1 ; done

max_rfs:
	for i in $(DIRS); do make -C $$i max_rfs install ; done

rfs:

net_rfs:
	for i in $(DIRS); do make -C $$i net_rfs install ; done

min_rfs:

smin_rfs:

install: all
	for i in $(DIRS); do make -C $$i install ; done

clean:
	for i in $(DIRS) $(DIRS2); do make -C $$i clean ; done

BASEDIR=..

include $(BASEDIR)/Make.defs

DIRS=httpd telnet urlget

DIRS2=nettools

all:
	@for i in $(DIRS); do make -C $$i all ; done

rfs:

net_rfs: install
	@for i in $(DIRS2); do make -C $$i net_rfs ; done

min_rfs:

smin_rfs:

install:
	@for i in $(DIRS); do make -C $$i install ; done

clean:
	@for i in $(DIRS) $(DIRS2); do make -C $$i clean ; done

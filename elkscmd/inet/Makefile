include ../Make.defs

DIRS=httpd telnet urlget
DIRS2=nettools

all:
	@for i in $(DIRS); do \
	(cd $$i; make) ; \
	done

rfs:

net_rfs: install
	@for i in $(DIRS2); do \
	(cd $$i; make net_rfs) ; \
	done

min_rfs:

smin_rfs:

install:
	@for i in $(DIRS); do \
	(cd $$i; make install) ; \
	done

clean:
	@for i in $(DIRS) $(DIRS2); do \
	(cd $$i; make clean) ; \
	done

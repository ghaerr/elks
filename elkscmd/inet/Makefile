include ../Make.defs

DIRS=httpd telnet

all:
	@for i in $(DIRS); do \
	(cd $$i; make) ; \
	done

rfs: install

min_rfs:

smin_rfs:

install:
	@for i in $(DIRS); do \
	(cd $$i; make install) ; \
	done

clean:
	@for i in $(DIRS); do \
	(cd $$i; make clean) ; \
	done

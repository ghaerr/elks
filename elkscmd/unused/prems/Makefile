BASEDIR=..

include $(BASEDIR)/Makefile-rules

###############################################################################

###############################################################################

PRGS =	pres/pres

NETPRGS =

all:
	$(MAKE) -C prem all
	$(MAKE) -C pres all

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	$(MAKE) -C pres clean
	$(MAKE) -C prem clean

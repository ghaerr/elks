BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=lpfilter

all: $(PRGS)

install: $(PRGS)
	install $(PRGS) $(DESTDIR)/bin

clean:
	rm -f $(PRGS)

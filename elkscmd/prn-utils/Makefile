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
	$(SUDO) install $(PRGS) $(TARGET_MNT)/bin

clean:
	rm -f $(PRGS)

BASEDIR=..

LOCALFLAGS=

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=cal diff file find head sed sort tail tee

#
# Rules
#

include $(BASEDIR)/Make.rules

all: $(PRGS)

install: $(PRGS)
	sudo install $(PRGS) $(TARGET_MNT)/bin

clean:
	rm -f core *.o $(PRGS)

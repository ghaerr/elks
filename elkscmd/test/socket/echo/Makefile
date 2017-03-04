BASEDIR=../../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=echoserver echoclient

all: $(PRGS)

smin_rfs:

min_rfs:

rfs:

max_rfs: install

net_rfs: install

install: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

clean:
	rm -f core *.o $(PRGS)

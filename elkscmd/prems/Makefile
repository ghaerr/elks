BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS =	pres/pres

NETPRGS =

all:
	make -C prem all
	make -C pres all

max_rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

rfs: all

net_rfs: all

min_rfs: all

smin_rfs: all

clean:
	make -C pres clean
	make -C prem clean

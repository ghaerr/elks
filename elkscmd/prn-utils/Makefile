BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=lpfilter

MIN_PRGS=

all: $(PRGS)

max_rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

min_rfs: all
#	cp -p $(MIN_PRGS) $(TARGET_MNT)/bin

clean:
	rm -f $(PRGS)

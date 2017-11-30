BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

all: xvi

xvi: xvi.o
	$(CC) $(LDFLAGS) xvi.o -o xvi

max_rfs: xvi
	cp -p xvi $(TARGET_MNT)/bin$

rfs: xvi
	cp -p xvi $(TARGET_MNT)/bin$
	
min_rfs: xvi

clean:
	rm -f core xvi *.o

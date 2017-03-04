BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

CFLAGS= $(CFLBASE)
LFLAGS= -s

all: xvi

xvi: xvi.o
	$(CC) $(LFLAGS) xvi.o -o xvi

max_rfs: xvi
	cp -p xvi $(TARGET_MNT)/bin$

rfs: xvi
	cp -p xvi $(TARGET_MNT)/bin$
	
min_rfs: xvi

clean:
	rm -f core xvi *.o

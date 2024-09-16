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

install: xvi
	$(INSTALL) xvi $(DESTDIR)/bin

clean:
	$(RM) core xvi *.o

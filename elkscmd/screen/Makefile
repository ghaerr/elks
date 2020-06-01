# The following options can be set:
#
# -DLOADAV    -- your system maintains a load average like 4.3 BSD does
#                (an array of three doubles called _avenrun; it is read
#                from /dev/kmem; _avenrun is taken from the namelist of
#                /vmunix).
# -DSUNLOADAV -- the load average maintained by the kernel is in the
#                Sun format (three longs).  Set this in addition to
#                LOADAV.
# -DGETTTYENT -- your system has the new format /etc/ttys (like 4.3 BSD)
#                and the getttyent(3) library functions.
#
# -DUSEBCOPY  -- use the bcopy() from the system's C-library.  If this
#                is set, bcopy must support overlapping source and
#                destination.  If USEBCOPY is not set, screen uses its
#                own version of bcopy.
#
# You should install as set-uid with owner root, so that it can read/write
# /etc/utmp, read /dev/kmem, and chown/chmod the allocated pseudo-ttys.

##################################################################################

# Makefile for screen

BASEDIR=..

include $(BASEDIR)/Make.defs

OPTIONS	= -DELKS # -DUSEBCOPY # -DLOADAV -DSUNLOADAV -DGETTTYENT -DUSEBCOPY

LOCALFLAGS = -Wno-implicit-int \
		-Wno-unused-function \
		-Wno-unused-variable \
		-Wno-incompatible-pointer-types \
		-Wno-char-subscripts \
		$(OPTIONS)
		
LOCALFLAGS += -maout-heap=0x7000


###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

all: screen

screen:	screen.o ansi.o 
	$(CC) $(CFLAGS) -s -o screen screen.o ansi.o

screen.o: screen.c screen.h
	$(CC) $(CFLAGS) -c screen.c

ansi.o: ansi.c screen.h
	$(CC) $(CFLAGS) -c ansi.c

clean:
	rm -f screen *.o core *~

install: screen
	$(INSTALL) screen $(DESTDIR)/bin/screen


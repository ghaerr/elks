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

OPTIONS= -DLOADAV -DSUNLOADAV -DGETTTYENT -DUSEBCOPY

CFILES= screen.c ansi.c
OFILES= screen.o ansi.o

all:	screen

screen: $(OFILES)
	$(CC) $(CFLAGS) -o screen $(OFILES) -ltermcap

screen.o: screen.c screen.h
	$(CC) $(OPTIONS) $(CFLAGS) -c screen.c

ansi.o: ansi.c screen.h
	$(CC) $(CFLAGS) -c ansi.c

BASEDIR=..

LOCALFLAGS=-D_POSIX_SOURCE

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=env install lp pwdauth remsync synctree tget
RFS_PRGS=env install lp pwdauth tget

# lpd mt	# Do not compile.

NETPRGS=env

all: $(PRGS)

lpd: lpd.o ../lib/mktemp.o

../lib/mktemp.o:
	make -C ../lib mktemp.o

max_rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

rfs: all
	cp -p $(RFS_PRGS) $(TARGET_MNT)/bin

net_rfs: all
	cp -p $(NETPRGS) $(TARGET_MNT)/bin

min_rfs: all

smin_rfs: all

clean:
	rm -f core *.o $(PRGS)

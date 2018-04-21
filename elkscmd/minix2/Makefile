BASEDIR=..

LOCALFLAGS=-D_POSIX_SOURCE

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=env lp pwdauth remsync synctree tget
RFS_PRGS=env install lp pwdauth tget

# install   # Same name as phony target install
# lpd mt	# Do not compile.

NETPRGS=env

all: $(PRGS)

lpd: lpd.o ../lib/mktemp.o

../lib/mktemp.o:
	make -C ../lib mktemp.o

install: $(PRGS)
	sudo install $(PRGS) $(TARGET_MNT)/bin

clean:
	rm -f core *.o $(PRGS)

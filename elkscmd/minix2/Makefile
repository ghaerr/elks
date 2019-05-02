BASEDIR=..

LOCALFLAGS=-D_POSIX_SOURCE

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=env lp pwdauth remsync synctree tget

env: env.o
	$(LD) $(LDFLAGS) -o env env.o $(LDLIBS)

pwdauth: pwdauth.o
	$(LD) $(LDFLAGS) -o pwdauth pwdauth.o $(LDLIBS)

lp: lp.o
	$(LD) $(LDFLAGS) -o lp lp.o $(LDLIBS)

remsync: remsync.o
	$(LD) $(LDFLAGS) -o remsync remsync.o $(LDLIBS)

synctree: synctree.o
	$(LD) $(LDFLAGS) -o synctree synctree.o $(LDLIBS)

tget: tget.o
	$(LD) $(LDFLAGS) -o tget tget.o $(LDLIBS)

# install   # Same name as phony target install
# lpd mt	# Do not compile.

NETPRGS=env

all: $(PRGS)

lpd: lpd.o ../lib/mktemp.o
	$(LD) $(LDFLAGS) -o lpd lpd.o ../lib/mktemp.o $(LDLIBS)

../lib/mktemp.o:
	make -C ../lib mktemp.o

install: $(PRGS)
	$(SUDO) install $(PRGS) $(TARGET_MNT)/bin

clean:
	rm -f core *.o $(PRGS)

BASEDIR=..

LOCALFLAGS=-D_POSIX_SOURCE

include $(BASEDIR)/Make.defs

PRGS=env install lp pwdauth tget remsync synctree lpd

# mt # Doesn't compile.

NETPRGS=env

all: $(PRGS)

lpd: lpd.o ../lib/mktemp.o

../lib/mktemp.o:
	make -C ../lib mktemp.o

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

net_rfs: all
	cp -p $(NETPRGS) $(TARGET_MNT)/bin

min_rfs: all

smin_rfs: all

clean:
	rm -f core *.o $(PRGS)

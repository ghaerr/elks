BASEDIR=..

include $(BASEDIR)/Make.defs

CFLAGS	= $(CFLBASE) -D_POSIX_SOURCE

PRGS =	env install lp pwdauth  tget lpd # remsync synctree mt

NETPRGS = env

all: $(PRGS)

lpd: lpd.o ../lib/mktemp.o

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

net_rfs: all
	cp -p $(NETPRGS) $(TARGET_MNT)/bin

min_rfs: all

smin_rfs: all

clean:
	rm -f $(PRGS) *.o

include ../Make.defs

CFLAGS	= $(CFLBASE) -D_POSIX_SOURCE

PRGS =	env install lp pwdauth remsync synctree tget lpd # mt

all: $(PRGS)

lpd: lpd.o ../lib/mktemp.o

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

min_rfs: all

clean:
	rm -f $(PRGS) *.o

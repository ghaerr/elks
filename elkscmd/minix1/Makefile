include ../Make.defs

CFLAGS	= $(CFLBASE) -D_POSIX_SOURCE

xCC=gcc
xCFLAGS=-D_POSIX_SOURCE -D_PROTOTYPE\(a,b\)=

PRGS =	banner \
	cksum \
	cut \
	decomp16 \
	du \
	fgrep \
	grep \
	proto \
	sum \
	uniq \
	wc

all: $(PRGS)

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

min_rfs: all

clean:
	rm -f $(PRGS)

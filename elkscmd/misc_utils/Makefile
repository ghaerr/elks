BASEDIR = ..

include $(BASEDIR)/Make.defs

PRGS=ed tar

all: $(PRGS)

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

net_rfs: rfs

min_rfs: rfs

smin_rfs:

clean:
	rm -f $(PRGS)

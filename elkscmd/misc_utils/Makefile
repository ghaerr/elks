include ../Make.defs

PRGS=ed tar

all: $(PRGS)

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

min_rfs: rfs

clean:
	rm -f $(PRGS)

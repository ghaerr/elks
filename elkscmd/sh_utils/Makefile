include ../Make.defs

PRGS=basename date dirname echo false printenv pwd true whoami yes \
	# logname uname

MIN_PRGS=date pwd printenv

all: $(PRGS)

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

min_rfs: all
	cp -p $(MIN_PRGS) $(TARGET_MNT)/bin

clean:
	rm -f $(PRGS)

BASEDIR = ..

include $(BASEDIR)/Make.defs

PRGS=basename date dirname echo false printenv pwd true which whoami yes \
	logname tr xargs mesg write stty test # uname

NETPRGS=date echo false true yes stty test pwd

MIN_PRGS=date pwd printenv

write: write.o ../sys_utils/utent.o

all: $(PRGS)

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

net_rfs: all
	cp -p $(NETPRGS) $(TARGET_MNT)/bin


min_rfs: all
	cp -p $(MIN_PRGS) $(TARGET_MNT)/bin

smin_rfs:

clean:
	rm -f $(PRGS)

BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=basename date dirname echo false printenv pwd true which whoami yes \
	logname tr xargs mesg stty test

# uname write	# Do not compile

NETPRGS=date echo false true yes stty test pwd

MIN_PRGS=date pwd printenv test

write: write.o ../sys_utils/utent.o

all: $(PRGS)

max_rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

net_rfs: all
	cp -p $(NETPRGS) $(TARGET_MNT)/bin


min_rfs: all
	cp -p $(MIN_PRGS) $(TARGET_MNT)/bin

smin_rfs:

clean:
	rm -f core $(PRGS)

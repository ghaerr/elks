BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= false pwd true uname
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	= date echo printenv
MIN_USR_LIB	= 
MIN_USR_MAN	= 
MIN_USR_SBIN	= 

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	= 
NET_USR_LIB	= 
NET_USR_MAN	= 
NET_USR_SBIN	= 

STD_BIN 	= logname mesg test
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= dirname tr which whoami write xargs yes
STD_USR_LIB	= 
STD_USR_MAN	= 
STD_USR_SBIN	= stty

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=basename date dirname echo false printenv pwd true which whoami yes \
	logname tr xargs mesg stty test

# uname write	# Do not compile

NETPRGS=date echo false true yes stty test pwd

MIN_PRGS=date pwd printenv test

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
	rm -f core $(PRGS)

BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= getty init kill login mount passwd ps reboot umount
MIN_LIB 	= 
MIN_SBIN	= clock exitemu
MIN_USR_BIN	= 
MIN_USR_LIB	= 
MIN_USR_SBIN	= 

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	= 
NET_USR_LIB	= 
NET_USR_SBIN	= 

STD_BIN 	= who
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= man
STD_USR_LIB	= 
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=getty kill man mount reboot umount clock exitemu who \
	init login passwd ps

NETPRGS=init getty login kill mount umount passwd reboot ps who

MIN_PRGS=init getty login kill mount reboot umount clock exitemu ps

SMIN_PRGS=mount umount

all: $(PRGS)

rfs: all
	cp -p $(PRGS) $(TARGET_MNT)/bin
	
net_rfs: all
	cp -p $(NETPRGS) $(TARGET_MNT)/bin

min_rfs: all
	cp -p $(MIN_PRGS) $(TARGET_MNT)/bin

smin_rfs: all
	cp -p $(SMIN_PRGS) $(TARGET_MNT)/bin

clean:
	rm -f *.o core $(PRGS)

passwd: getpass.o passwd.o putpwent.o


utent.o: utent.c
	$(CC) $(CFLAGS) -c utent.c

# Trim stack space of core programs.
# The -H option is the maximum size the data segment can be.
# If the code is modified, this value has to be carefully trimmed
# to fit the programs requirements. At the moment it is data + bss +
# 1024 bytes at least for even the smallest programs. Binaries which
# call malloc will need much more than this.

getty: getty.c
	$(CC) $(CFLAGS) $(LDFLAGS) getty.c -o getty -H 2000

new_init: init.c utent.o
	$(CC) $(CFLAGS) $(LDFLAGS) init.c utent.o -o init

init: init.c 
	$(CC) $(CFLAGS) $(LDFLAGS) init.c -o init -H 2000

login: getpass.o login.o utent.o
	$(CC) $(CFLAGS) $(LDFLAGS) getpass.o login.o utent.o -o login -H 2000

BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= getty init kill login mount passwd ps swapon meminfo reboot umount
MIN_LIB 	= 
MIN_SBIN	= clock exitemu getty init reboot
MIN_USR_BIN	= meminfo passwd
MIN_USR_LIB	= 
MIN_USR_MAN	= 
MIN_USR_SBIN	= knl

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	= 
NET_USR_LIB	= 
NET_USR_MAN	= 
NET_USR_SBIN	= 

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= man who
STD_USR_LIB	= 
STD_USR_MAN	= clock.8 getty.1 kill.1 knl.8 login.1 man.1 mount.8 \
		  passwd.1 ps.1 reboot.8 umount.8 who.1
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

PRGS		= getty kill man mount reboot umount clock exitemu who ps \
		  init login passwd knl meminfo

NETPRGS=init getty login kill mount umount passwd reboot ps swapon meminfo who

MIN_PRGS=init getty login kill mount reboot umount clock exitemu ps swapon meminfo

SMIN_PRGS	= init mount umount

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
	$(CC) $(CFLAGS) $(LDFLAGS) getty.c -o getty -H 1800

meminfo: meminfo.c
	$(CC) $(CFLAGS) $(LDFLAGS) meminfo.c -o meminfo -H 100

swapon: swapon.c
	$(CC) $(CFLAGS) $(LDFLAGS) swapon.c -o swapon -H 100

new_init: init.c utent.o
	$(CC) $(CFLAGS) $(LDFLAGS) init.c utent.o -o init

init: init.c 
	$(CC) $(CFLAGS) $(LDFLAGS) init.c -o init -H 2000

rdev: rdev.c 
	$(CC) $(CFLAGS) $(LDFLAGS) rdev.c -o rdev

login: getpass.o login.o utent.o
	$(CC) $(CFLAGS) $(LDFLAGS) getpass.o login.o utent.o -o login -H 2000

BASEDIR=..

include $(BASEDIR)/Make.defs

LOCALFLAGS =

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS		= init getty login kill mount umount passwd reboot shutdown ps \
		  swapon meminfo who clock exitemu knl man poweroff

NETPRGS		= init getty login kill mount umount passwd reboot shutdown ps \
		  swapon meminfo who poweroff

MIN_PRGS	= init getty login kill mount umount passwd ps clock \
		  exitemu meminfo

SMIN_PRGS	= init mount umount

all: $(PRGS)

install: $(PRGS)
	sudo install $(PRGS) $(TARGET_MNT)/bin

clean:
	rm -f *.o core $(PRGS)

passwd: getpass.o passwd.o putpwent.o


# Trim stack space of core programs.
# The -H option is the maximum size the data segment can be.
# If the code is modified, this value has to be carefully trimmed
# to fit the programs requirements. At the moment it is data + bss +
# 1024 bytes at least for even the smallest programs. Binaries which
# call malloc will need much more than this.

getty: getty.c
	$(CC) $(CFLAGS) $(LDFLAGS) getty.c -o getty -H 0x1800

meminfo: meminfo.c
	$(CC) $(CFLAGS) $(LDFLAGS) meminfo.c -o meminfo -H 0x100

swapon: swapon.c
	$(CC) $(CFLAGS) $(LDFLAGS) swapon.c -o swapon -H 0x100

init: init.c 
	$(CC) $(CFLAGS) $(LDFLAGS) init.c -o init -H 0x8000

rdev: rdev.c 
	$(CC) $(CFLAGS) $(LDFLAGS) rdev.c -o rdev

login: getpass.o login.o
	$(CC) $(CFLAGS) $(LDFLAGS) getpass.o login.o -o login -H 0x8000

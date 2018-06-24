BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

PRGS		= init getty login kill mount umount passwd reboot shutdown ps \
		  swapon meminfo who knl man poweroff

CFLAGS += -DDEBUG

# clock disabled because direct I/O port access
# exitemu disabled because it calls INT directly to DOSEMU

init: init.o
	$(LD) $(LDFLAGS) -o init init.o $(LDLIBS)

getty: getty.o
	$(LD) $(LDFLAGS) -o getty getty.o $(LDLIBS)

login: getpass.o login.o
	$(LD) $(LDFLAGS) -o login getpass.o login.o $(LDLIBS)

kill: kill.o
	$(LD) $(LDFLAGS) -o kill kill.o $(LDLIBS)

mount: mount.o
	$(LD) $(LDFLAGS) -o mount mount.o $(LDLIBS)

umount: umount.o
	$(LD) $(LDFLAGS) -o umount umount.o $(LDLIBS)

passwd: getpass.o passwd.o putpwent.o
	$(LD) $(LDFLAGS) -o passwd getpass.o passwd.o putpwent.o $(LDLIBS)

reboot: reboot.o
	$(LD) $(LDFLAGS) -o reboot reboot.o $(LDLIBS)

shutdown: shutdown.o
	$(LD) $(LDFLAGS) -o shutdown shutdown.o $(LDLIBS)

ps: ps.o
	$(LD) $(LDFLAGS) -o ps ps.o $(LDLIBS)

swapon: swapon.o
	$(LD) $(LDFLAGS) -o swapon swapon.o $(LDLIBS)

meminfo: meminfo.o
	$(LD) $(LDFLAGS) -o meminfo meminfo.o $(LDLIBS)

who: who.o
	$(LD) $(LDFLAGS) -o who who.o $(LDLIBS)

clock: clock.o
	$(LD) $(LDFLAGS) -o clock clock.o $(LDLIBS)

exitemu: exitemu.o
	$(LD) $(LDFLAGS) -o exitemu exitemu.o $(LDLIBS)

knl: knl.o
	$(LD) $(LDFLAGS) -o knl knl.o $(LDLIBS)

man: man.o
	$(LD) $(LDFLAGS) -o man man.o $(LDLIBS)

poweroff: poweroff.o
	$(LD) $(LDFLAGS) -o poweroff poweroff.o $(LDLIBS)

all: $(PRGS)

install: $(PRGS)
	sudo install $(PRGS) $(TARGET_MNT)/bin

clean:
	rm -f *.o core $(PRGS)

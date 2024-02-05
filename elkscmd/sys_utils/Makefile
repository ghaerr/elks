BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

include $(BASEDIR)/Make.rules

###############################################################################

ELKS_LIB=$(ELKS_DIR)/arch/i86/lib

# clock enabled and has direct I/O port access
# knl, insmod removed as useless
# exitemu disabled because it calls INT directly to DOSEMU
PRGS = \
	init \
	min_init \
	getty \
	login \
	kill \
	mount \
	umount \
	passwd \
	reboot \
	shutdown \
	ps \
	meminfo \
	who \
	man \
	poweroff \
	chmem \
	clock \
	unreal16 \
	beep \
	mouse \
	sercat \
	console \
	makeboot \
	# EOL

all: $(PRGS)

init: init.o
	$(LD) $(LDFLAGS) -maout-heap=4096 -maout-stack=1024 -o init init.o $(LDLIBS)

min_init: min_init.o
	$(LD) $(LDFLAGS) -maout-heap=256 -maout-stack=256 -o min_init min_init.o $(LDLIBS)

getty: getty.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -maout-heap=1024 -maout-stack=1024 -o getty getty.o $(TINYPRINTF) $(LDLIBS)

login: login.o
	$(LD) $(LDFLAGS) -maout-stack=2048 -o login login.o $(LDLIBS)

kill: kill.o
	$(LD) $(LDFLAGS) -o kill kill.o $(LDLIBS)

mount: mount.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o mount mount.o $(TINYPRINTF) $(LDLIBS)

umount: umount.o
	$(LD) $(LDFLAGS) -o umount umount.o $(LDLIBS)

passwd: passwd.o
	$(LD) $(LDFLAGS) -o passwd passwd.o $(LDLIBS)

reboot: reboot.o
	$(LD) $(LDFLAGS) -o reboot reboot.o $(LDLIBS)

shutdown: shutdown.o
	$(LD) $(LDFLAGS) -o shutdown shutdown.o $(LDLIBS)

ps: ps.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -maout-heap=1024 -maout-stack=2048 -o ps ps.o $(TINYPRINTF) $(LDLIBS)

meminfo: meminfo.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -maout-heap=1 -maout-stack=512 -o meminfo meminfo.o $(TINYPRINTF) $(LDLIBS)

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

chmem: chmem.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o chmem chmem.o $(TINYPRINTF) $(LDLIBS)

mouse: mouse.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o mouse mouse.o $(TINYPRINTF) $(LDLIBS)

sercat: sercat.o
	$(LD) $(LDFLAGS) -o sercat sercat.o $(LDLIBS)

console: console.o
	$(LD) $(LDFLAGS) -o console console.o $(LDLIBS)

makeboot: makeboot.o $(TINYPRINTF)
	$(LD) $(LDFLAGS) -o makeboot makeboot.o $(TINYPRINTF) $(LDLIBS)

unreal.o: $(ELKS_LIB)/unreal.S
	$(LD) -melks-libc -mcmodel=small -c $(ELKS_LIB)/unreal.S -o unreal.o

beep: beep.o
	$(LD) $(LDFLAGS) -o beep beep.o $(LDLIBS)

unreal16: unreal16.o unreal.o
	$(LD) -melks-libc -mcmodel=small -c unreal16.S -o unreal16.o
	$(LD) -melks-libc -mcmodel=small -nostdlib -o unreal16 unreal16.o unreal.o

all: $(PRGS)

install: $(PRGS)
	$(INSTALL) $(PRGS) $(DESTDIR)/bin

clean:
	rm -f *.o core $(PRGS)

# Microwindows and Nano-X Makefile for ELKS
#
# originally from v0.86 10/29/99 g haerr
#

BASEDIR = ..
include $(BASEDIR)/Make.defs

LOCALFLAGS = -DELKS=1 -DUNIX=1 -DDEBUG=1 -Dconst= -Dvolatile= -I.
MATHLIB =

## mouse and kbd drivers
#DRIVERS += drivers/mou_ser.o
#DRIVERS += drivers/mou_ser_qemu.o
DRIVERS += drivers/mou_null.o
DRIVERS += drivers/kbd_tty.o

# The following line links the nano-X application with the server.
# This is required if no network is present, or for speed or debugging.
LINK_APP_INTO_SERVER=1

NANOXFILES = nanox/srvmain.o nanox/srvfunc.o nanox/srvutil.o nanox/srvevent.o
CLNTFILES = nanox/client.o
NETFILES = nanox/srvnet.o
STUBFILES = nanox/stubs.o

# nano-X demo programs 
NANOXDEMO =  demos/demo.o
NANOXDEMO += demos/landmine.o
NANOXDEMO += demos/world.o				#world needs world map - does not fit on full3
NANOXDEMO += demos/nclock.o
NANOXDEMO += demos/nterm.o				#works if pseudo tty enabled in ELKS menuconfig
NANOXDEMO += demos/demo2.o
NANOXDEMO += demos/demo-vga.o			#low-level vga routines test

ifdef LINK_APP_INTO_SERVER
CFLAGS += -DNONETWORK=1
NANOXFILES += $(STUBFILES)
ALL += nano-X
else
xNANOXDEMO =demos/demo2.o
NANOXFILES += $(NETFILES)
DEMOLIBS += libnano-X.a
ALL += libnano-X.a nano-Xd demo
endif

SERVFILES = engine/devdraw.o engine/devmouse.o engine/devkbd.o engine/devclip1.o \
	engine/devpal1.o engine/devpal2.o engine/devpal4.o

## higher speed asm VGA driver
#DRIVERS += drivers/scr_bios.o drivers/elkplan4.o drivers/romfont.o drivers/elksutilasm.o drivers/mempl4.o
## portable C VGA driver
DRIVERS += drivers/scr_bios.o drivers/vgaplan4.o drivers/romfont.o drivers/elksutilasm.o drivers/mempl4.o
## hercules driver
#DRIVERS += drivers/scr_herc.o drivers/romfont.o drivers/elksutilasm.o

SERVFILES += $(DRIVERS)

all: $(ALL)

clean:
	rm -f *.o *.a nano-X core
	rm -f drivers/*.o
	rm -f bin/*
	rm -f nanox/*.o
	rm -f engine/*.o
	rm -f demos/*.o

libnano-X.a: $(CLNTFILES)
	$(AR) rc libnano-X.a $(CLNTFILES)

nano-X: $(SERVFILES) $(NANOXFILES) $(NANOXDEMO)
	$(LD) $(LDFLAGS) demos/demo-vga.o $(DRIVERS) $(LIBS) -o bin/demo-vga $(MATHLIB) $(LDLIBS)
	$(LD) $(LDFLAGS) $(NANOXFILES) demos/demo.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/demo $(MATHLIB) $(LDLIBS)
	$(LD) $(LDFLAGS) $(NANOXFILES) demos/demo2.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/demo2 $(MATHLIB) $(LDLIBS)
	$(LD) $(LDFLAGS) $(NANOXFILES) demos/world.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/world $(MATHLIB) $(LDLIBS)
	$(LD) $(LDFLAGS) $(NANOXFILES) demos/landmine.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/landmine $(MATHLIB) $(LDLIBS)
	$(LD) $(LDFLAGS) $(NANOXFILES) demos/nterm.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/nterm $(MATHLIB) $(LDLIBS)
	$(LD) $(LDFLAGS) $(NANOXFILES) demos/nclock.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/nclock $(MATHLIB) $(LDLIBS)
	cp demos/world.map bin/world.map

nano-Xd: $(SERVFILES) $(NANOXFILES)
	$(LD) $(LDFLAGS) $(NANOXFILES) $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/nano-X $(MATHLIB) $(LDFLAGS)

#only a few demos work in client/server mode
demo: $(NANOXDEMO) libnano-X.a
#	$(LD) $(LDFLAGS) $(NANOXDEMO) $(DEMOLIBS) -o bin/demo $(LDFLAGS)
	$(LD) $(LDFLAGS) demos/demo.o $(DEMOLIBS) -o bin/demo $(MATHLIB) $(LDFLAGS)
	$(LD) $(LDFLAGS) demos/landmine.o $(DEMOLIBS) -o bin/landmine $(MATHLIB) $(LDFLAGS)
#	$(LD) $(LDFLAGS) demos/nterm.o $(DEMOLIBS) -o bin/nterm $(MATHLIB)	 $(LDFLAGS)
#	$(LD) $(LDFLAGS) demos/nclock.o $(DEMOLIBS) -o bin/nclock $(MATHLIB)	 $(LDFLAGS)
	$(LD) $(LDFLAGS) demos/demo2.o $(DEMOLIBS) -o bin/demo2 $(MATHLIB) $(LDFLAGS)
#	$(LD) $(LDFLAGS) demos/world.o $(DEMOLIBS) -o bin/world $(MATHLIB) $(LDFLAGS)
#	cp demos/world.map bin/world.map
#	cp demos/demo.sh bin/demo.sh

.PHONY: install

install:
	install bin/demo $(DESTDIR)/bin/demo

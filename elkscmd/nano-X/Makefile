# 10/29/1999 g haerr
# Microwindows and Nano-X Makefile for ELKS
#

BASEDIR = ..
include $(BASEDIR)/Make.defs

LOCALFLAGS = -DELKS=1 -DUNIX=1 -DDEBUG=1 -Dconst= -Dvolatile= -I.
MATHLIB =

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
NANOXDEMO += demo-vga.o							#low-level vga routines test

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
DRIVERS += drivers/scr_bios.o drivers/elkplan4.o drivers/romfont.o drivers/elksutil.o drivers/mempl4.o
## portable C VGA driver
#DRIVERS += drivers/scr_bios.o drivers/vgaplan4.o drivers/romfont.o drivers/elksutil.o drivers/mempl4.o
## hercules driver
#DRIVERS += drivers/scr_herc.o drivers/romfont.o drivers/elksutil.o

## mouse and kbd drivers
#DRIVERS += drivers/mou_ser.o
#DRIVERS += drivers/mou_ser_qemu.o
DRIVERS += drivers/mou_null.o
DRIVERS += drivers/kbd_tty.o

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
	$(CC) $(CFLAGS) $(NANOXFILES) demos/demo.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/demo $(MATHLIB)
	$(CC) $(CFLAGS) $(NANOXFILES) demos/landmine.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/landmine $(MATHLIB)
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nterm.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/nterm $(MATHLIB)	
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nclock.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/nclock $(MATHLIB)	
	$(CC) $(CFLAGS) $(NANOXFILES) demos/demo2.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/demo2 $(MATHLIB)
	$(CC) $(CFLAGS) $(NANOXFILES) demos/world.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/world $(MATHLIB)
	cp demos/world.map bin/world.map
	$(CC) $(CFLAGS) demo-vga.o $(DRIVERS) $(LIBS) -o bin/demo-vga $(MATHLIB)

nano-Xd: $(SERVFILES) $(NANOXFILES)
	$(CC) $(CFLAGS) $(NANOXFILES) $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/nano-X $(MATHLIB)

#only a few demos work in client/server mode	
demo: $(NANOXDEMO) libnano-X.a
#	$(CC) $(CFLAGS) $(NANOXDEMO) $(DEMOLIBS) -o bin/demo
	$(CC) $(CFLAGS) demos/demo.o $(DEMOLIBS) -o bin/demo $(MATHLIB)
	$(CC) $(CFLAGS) demos/landmine.o $(DEMOLIBS) -o bin/landmine $(MATHLIB)
#	$(CC) $(CFLAGS) demos/nterm.o $(DEMOLIBS) -o bin/nterm $(MATHLIB)	
#	$(CC) $(CFLAGS) demos/nclock.o $(DEMOLIBS) -o bin/nclock $(MATHLIB)	
	$(CC) $(CFLAGS) demos/demo2.o $(DEMOLIBS) -o bin/demo2 $(MATHLIB)
#	$(CC) $(CFLAGS) demos/world.o $(DEMOLIBS) -o bin/world $(MATHLIB)
#	cp demos/world.map bin/world.map
	cp demos/demo.sh bin/demo.sh	

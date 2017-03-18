# 10/29/1999 g haerr
# Micro-Windows and Nano-X Makefile for Linux and ELKS
#
# This package can be configured to run on Linux (MIPS or x86) or ELKS.
# On Linux, we've got drivers for Linux 2.x framebuffers
# or you can use svgalib for all VGA hardware can be used.
# In addition, a gpm or direct serial mouse driver can be configured.
# On ELKS, the bios screen driver and serial mouse driver are always used.
#
# Either Micro-Windows and/or Nano-X can be built.
# Micro-Windows contains just one demo, while nano-X has several.
# See MWINDEMO= and NANOXDEMO=
#
# For MSDOS makes, see mcmwin.mak and mcnanox.mak
#

# export variables for sublevel makefiles
export HOSTCC HOSTCFLAGS CC CFLAGS AS LD AR STRIP LINUX ELKS HAVEMSFONTS

# build target platform: linux or elks
# note: ELKS can't build client/server nano-X, nor widget lib
ELKS=1

# build nano-X
NANOX=1

# uncomment the following line to link the nano-X application
# with the server.  This is required for ELKS, if no network is present,
# or for speed or debugging.  This affects the nano-X server only.
LINK_APP_INTO_SERVER=1

# Window move algorithms, change for tradeoff between cpu speed and looks
# UPDATEREGIONS paints in update clipping region only for better look and feel
# ERASEMOVE repaints only backgrounds while window dragging, quicker
# XORMOVE moves windows only after button up, quickest (should set for ELKS)
UPDATEREGIONS=1
ERASEMOVE=1
xXORMOVE=1

# vgalib library location
LIBVGA=-lvga

# for recursive makes
TOPDIR := $(shell pwd)

ifdef ELKS
CC = bcc -0 -ansi -Dconst= -Dvolatile=
INC = -I$(TOPDIR) -I/usr/bcc/include -I/home/georg/elks-dev/OS-build/elks/include
CFLAGS = -DELKS=1 -DUNIX=1 -DDEBUG=1 $(INC) -O
MATHLIB =
xSERMOUSE=1
QEMUMOUSE=1
xLINK_APP_INTO_SERVER=1
endif

ifdef NANOX
NANOXFILES = nanox/srvmain.o nanox/srvfunc.o nanox/srvutil.o nanox/srvevent.o
CLNTFILES = nanox/client.o
NETFILES = nanox/srvnet.o
STUBFILES = nanox/stubs.o
# nano-X demo programs 
NANOXDEMO  =demos/nanox/demo.o
NANOXDEMO +=demos/nanox/landmine.o
#world needs world map - does not fit on full3
NANOXDEMO +=demos/nanox/world.o
NANOXDEMO +=demos/nanox/nclock.o
#works if pseudo tty enabled in ELKS menuconfig
NANOXDEMO +=demos/nanox/nterm.o
#demo2 makes blue box only
NANOXDEMO +=demos/nanox/demo2.o

xNANOXDEMO=demos/nanox/info.o

ifdef LINK_APP_INTO_SERVER
CFLAGS += -DNONETWORK=1
xNANOXFILES += $(NANOXDEMO) $(STUBFILES)
NANOXFILES += $(STUBFILES)
ALL += nano-X
else
NANOXFILES += $(NETFILES)
DEMOLIBS += libnano-X.a
ALL += libnano-X.a nano-X demo
endif

endif

SERVFILES = engine/devdraw.o engine/devmouse.o engine/devkbd.o engine/devclip.o\
	engine/devrgn.o engine/devpal1.o engine/devpal2.o engine/devpal4.o

ifdef ELKS
# higher speed asm VGA driver
SERVFILES += drivers/scr_bios.o drivers/elkplan4.o drivers/romfont.o\
	drivers/elksutil.o drivers/mempl4.o
# portable C VGA driver
xSERVFILES += drivers/scr_bios.o drivers/vgaplan4.o drivers/romfont.o\
	drivers/elksutil.o drivers/mempl4.o
# hercules driver
xSERVFILES += drivers/scr_herc.o drivers/romfont.o drivers/elksutil.o

#SERVFILES += drivers/mou_ser.o
SERVFILES += drivers/mou_ser_qemu.o
#SERVFILES += drivers/mou_null.o
SERVFILES += drivers/kbd_tty.o
endif

ifdef ERASEMOVE
CFLAGS += -DERASEMOVE=1
endif

ifdef XORMOVE
CFLAGS += -DXORMOVE=1
endif

ifdef UPDATEREGIONS
CFLAGS += -DUPDATEREGIONS=1
endif

all: $(ALL)

clean:
	rm -f *.o *.a nano-X microwin demo test core
	rm -f drivers/*.o
	rm -f bin/*
	rm -f nanox/*.o
	rm -f engine/*.o
	rm -f demos/nanox/*.o

realclean: clean
	$(MAKE) -C fonts realclean
	$(MAKE) -C bmp realclean

libnano-X.a: $(CLNTFILES)
	$(AR) rcs libnano-X.a $(CLNTFILES)

fonts/libfonts.a:
	$(MAKE) -C fonts

nano-X: $(SERVFILES) $(NANOXFILES) $(NANOXDEMO)
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nanox/demo.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/demo $(MATHLIB)
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nanox/landmine.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/landmine $(MATHLIB)
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nanox/nterm.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/nterm $(MATHLIB)	
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nanox/nclock.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/nclock $(MATHLIB)	
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nanox/demo2.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/demo2 $(MATHLIB)
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nanox/world.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/world $(MATHLIB)
	cp demos/nanox/world.map bin/world.map

demo: $(NANOXDEMO) libnano-X.a
	$(CC) $(CFLAGS) $(NANOXDEMO) $(DEMOLIBS) -o demo
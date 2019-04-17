# 10/29/1999 g haerr
# Microwindows and Nano-X Makefile for ELKS
#
# This package can be configured to run on Linux (MIPS or x86) or ELKS.
# On Linux, we've got drivers for Linux 2.x framebuffers
# or you can use svgalib for all VGA hardware can be used.
# In addition, a gpm or direct serial mouse driver can be configured.
# On ELKS, the bios screen driver and serial mouse driver are always used.
#
TOPDIR := ../..

# gcc-ia16 compiler, comment out for BCC
GCC=1

# The following line links the nano-X application with the server.
# This is required for ELKS, if no network is present,
# or for speed or debugging.  This affects the nano-X server only.
# note: ELKS can't build client/server nano-X
LINK_APP_INTO_SERVER=1

## compiler options
ifdef GCC
  CC = ia16-elf-gcc -ffreestanding -fno-inline -melks -mcmodel=small -mno-segment-relocation-stuff -mtune=i8086 -Os
  AS = ia16-elf-as
  LD = ia16-elf-ld
  LDFLAGS = -T $(TOPDIR)/elks/elks-small.ld $(TOPDIR)/libc/crt0.o
  LDLIBS = $(TOPDIR)/libc/libc.a $(TOPDIR)/cross/lib/gcc/ia16-elf/6.3.0/libgcc.a
  #LDLIBS = '-(' $(TOPDIR)/libc/libc.a '-)'
  INC = -I. -I../../libc/include -I../../elks/include -I../../include
else # BCC
  CC = bcc -0 -ansi -Dconst= -Dvolatile=
  AS = as86
  INC = -I. -I/usr/bcc/include -I../../elks/include -I../../include
endif
CFLAGS = -DELKS=1 -DUNIX=1 -DDEBUG=1 $(INC)
#CFLAGS += -DHAVEBLIT=1
MATHLIB =

NANOXFILES = nanox/srvmain.o nanox/srvfunc.o nanox/srvutil.o nanox/srvevent.o
CLNTFILES = nanox/client.o
NETFILES = nanox/srvnet.o
STUBFILES = nanox/stubs.o

## nano-X demo programs 
NANOXDEMO =
NANOXDEMO += demos/nanox/landmine.o
# world needs world map - does not fit on full3
NANOXDEMO += demos/nanox/world.o
NANOXDEMO += demos/nanox/nclock.o
# works if pseudo tty enabled in ELKS menuconfig
NANOXDEMO += demos/nanox/nterm.o
NANOXDEMO += demos/nanox/demo.o
NANOXDEMO += demos/nanox/demo2.o
#NANOXDEMO += demos/nanox/info.o

ifdef LINK_APP_INTO_SERVER
CFLAGS += -DNONETWORK=1
NANOXFILES += $(STUBFILES)
ALL += nano-X
else
NANOXFILES += $(NETFILES)
DEMOLIBS += libnano-X.a
ALL += libnano-X.a nano-Xd demo
endif

SERVFILES = engine/devdraw.o engine/devmouse.o engine/devkbd.o engine/devclip1.o\
	engine/devpal1.o engine/devpal2.o engine/devpal4.o

## screen driver
SERVFILES += drivers/scr_bios.o drivers/romfont.o drivers/mempl4.o
ifdef GCC
  # portable C VGA driver
  SERVFILES += drivers/vgaplan4.o drivers/elksutilasm.o
  # hercules driver
  #SERVFILES += drivers/scr_herc.o drivers/elksutilasm.o
else # BCC
  # higher speed asm VGA driver
  SERVFILES += drivers/elkplan4.o drivers/elksutil.o
endif

## mouse driver
# serial mouse
SERVFILES += drivers/mou_ser.o
# qemu mouse
#SERVFILES += drivers/mou_ser_qemu.o
# no mouse
#SERVFILES += drivers/mou_null.o

## tty driver
# tty keyboard
SERVFILES += drivers/kbd_tty.o

all: $(ALL)

clean:
	rm -f drivers/*.o
	rm -f bin/*
	rm -f nanox/*.o
	rm -f engine/*.o
	rm -f demos/nanox/*.o

libnano-X.a: $(CLNTFILES)
	$(AR) rcs libnano-X.a $(CLNTFILES)

nano-X: $(SERVFILES) $(NANOXFILES) $(NANOXDEMO)
ifdef GCC
	$(LD) $(LDFLAGS) $(NANOXFILES) demos/nanox/nterm.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/nterm $(LDLIBS)
	$(LD) $(LDFLAGS) $(NANOXFILES) demos/nanox/world.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/world $(LDLIBS)
	$(LD) $(LDFLAGS) $(NANOXFILES) demos/nanox/demo.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/demo $(LDLIBS)
	$(LD) $(LDFLAGS) $(NANOXFILES) demos/nanox/demo2.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/demo2 $(LDLIBS)
# nclock missing gmtime
	$(LD) $(LDFLAGS) $(NANOXFILES) demos/nanox/nclock.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/nclock $(LDLIBS)
# landmine missing rand/srand
	$(LD) $(LDFLAGS) $(NANOXFILES) demos/nanox/landmine.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/landmine $(LDLIBS)
else
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nanox/landmine.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/landmine $(MATHLIB)
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nanox/nterm.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/nterm $(MATHLIB)	
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nanox/nclock.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/nclock $(MATHLIB)	
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nanox/world.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/world $(MATHLIB)
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nanox/demo2.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/demo2 $(MATHLIB)
	$(CC) $(CFLAGS) $(NANOXFILES) demos/nanox/demo.o $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/demo $(MATHLIB)
endif
	cp demos/nanox/world.map bin/world.map

nano-Xd: $(SERVFILES) $(NANOXFILES)
	$(CC) $(CFLAGS) $(NANOXFILES) $(SERVFILES) $(NANOLIBS) $(LIBS) -o bin/nano-X $(MATHLIB)

#only a few demos work in client/server mode	
demo: $(NANOXDEMO) libnano-X.a
#	$(CC) $(CFLAGS) $(NANOXDEMO) $(DEMOLIBS) -o bin/demo
	$(CC) $(CFLAGS) demos/nanox/demo.o $(DEMOLIBS) -o bin/demo $(MATHLIB)
	$(CC) $(CFLAGS) demos/nanox/landmine.o $(DEMOLIBS) -o bin/landmine $(MATHLIB)
#	$(CC) $(CFLAGS) demos/nanox/nterm.o $(DEMOLIBS) -o bin/nterm $(MATHLIB)	
#	$(CC) $(CFLAGS) demos/nanox/nclock.o $(DEMOLIBS) -o bin/nclock $(MATHLIB)	
	$(CC) $(CFLAGS) demos/nanox/demo2.o $(DEMOLIBS) -o bin/demo2 $(MATHLIB)
#	$(CC) $(CFLAGS) demos/nanox/world.o $(DEMOLIBS) -o bin/world $(MATHLIB)
#	cp demos/nanox/world.map bin/world.map
	cp demos/demo.sh bin/demo.sh	

# Microwindows and Nano-X Makefile for ELKS
# originally from v0.86 10/29/99 g haerr

BASEDIR = ..
include $(BASEDIR)/Makefile-rules

# Uncomment the following line to link nano-X applications with the server.
#LINK_APP_INTO_SERVER=1

# remove any -mregparmcall as ASM files need cdecl calling convention
CFLAGS := $(filter-out -mregparmcall, $(CFLAGS))
CFLAGS += -DELKS=1 -DUNIX=1 -DNDEBUG=1 -I.
AR = ia16-elf-ar

# mouse and kbd drivers
ifeq ($(CONFIG_ARCH_PC98), y)
DRIVERS += drivers/mou_pc98.o drivers/kbd_tty.o
DRIVERS += drivers/scr_pc98.o drivers/vgaplan4_pc98.o
DRIVERS += drivers/ramfont.o drivers/X6x13.o
DRIVERS += drivers/mempl4.o drivers/elksutilasm.o
endif

ifeq ($(CONFIG_ARCH_IBMPC), y)
DRIVERS += drivers/mou_ser.o drivers/kbd_tty.o
DRIVERS += drivers/mempl4.o drivers/elksutilasm.o

ifeq ($(CONFIG_HW_VGA), y)
DRIVERS += drivers/scr_bios.o drivers/vgaplan4.o
DRIVERS += drivers/romfont.o drivers/elksutilasm.o
else
DRIVERS += drivers/scr_cga.o drivers/vgaplan4_cga.o
DRIVERS += drivers/ramfont.o drivers/X6x13.o
endif
endif

# high speed C VGA driver (BCC)
#DRIVERS += drivers/elkplan4.o

# high speed asm VGA driver (MASM)
#DRIVERS += drivers/asmplan4.o

# hercules driver
#DRIVERS += drivers/scr_herc.o

# high speed asm VGA driver (BCC)
#DRIVERS += drivers/elkplan4.o

# hercules driver
#DRIVERS += drivers/scr_herc.o

# Nano-X client and server build
NANOX = nanox/srvmain.o nanox/srvfunc.o nanox/srvutil.o nanox/srvevent.o
ENGINE = engine/devdraw.o engine/devmouse.o engine/devkbd.o engine/devclip1.o \
	engine/devpal1.o engine/devpal2.o engine/devpal4.o

ALL = libnano-X.a nxdemos

ifdef LINK_APP_INTO_SERVER
CFLAGS += -DNONETWORK=1
CLIENT = $(NANOX) $(ENGINE) $(DRIVERS) nanox/stubs.o
else
CLIENT = nanox/client.o
SERVER = $(NANOX) $(ENGINE) $(DRIVERS) nanox/srvnet.o
ALL += bin/nano-X
endif

all: $(ALL)

libnano-X.a: $(CLIENT)
	$(AR) rc libnano-X.a $(CLIENT)

bin/nano-X: $(SERVER)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

nxdemos: libnano-X.a
	$(MAKE) -C demos

clean:
	$(RM) *.o *.a bin/nano-X drivers/*.o nanox/*.o engine/*.o
	$(MAKE) -C demos clean

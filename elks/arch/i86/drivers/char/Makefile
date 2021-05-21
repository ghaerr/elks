# Makefile for the ELKS character devices.
#
#########################################################################
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
# Relative path to the base directory.

BASEDIR 	= ../../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= 

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	=

NOINDENT	= 

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

ifeq ($(CONFIG_ARCH_8018X), y)
	OBJS  = init.o mem.o ntty.o meta.o tcpdev.o pty.o
	OBJS += console-headless.o kbd-poll.o conio-8018x.o
endif

ifeq ($(CONFIG_ARCH_IBMPC), y)
	OBJS  = init.o mem.o ntty.o meta.o tcpdev.o pty.o
	OBJS += serial.o serfast.o lp.o

ifdef CONFIG_CHAR_DEV_CGATEXT
	OBJS += cgatext.o
endif

ifdef CONFIG_CONSOLE_DIRECT
OBJS += console-direct.o bell-8254.o
endif

ifdef CONFIG_CONSOLE_BIOS
OBJS += console-bios.o console-bios-asm.o bell-8254.o
endif

ifdef CONFIG_CONSOLE_HEADLESS
OBJS += console-headless.o
endif

ifdef CONFIG_KEYBOARD_SCANCODE
OBJS += kbd-scancode.o
else
OBJS += kbd-poll.o conio-bios.o
endif

endif # CONFIG_ARCH_IBMPC

#########################################################################
# Commands.

all:	chr_drv.a

chr_drv.a: $(OBJS)
	$(AR) rcs chr_drv.a $(OBJS)

KeyMaps/keymaps.h:
	$(MAKE) -C KeyMaps keymaps.h

dircon.o: console.c

bioscon.o: console.c

#########################################################################
# Standard commands.

distdir:
	mkdir $(DISTDIR)/arch/i86/drivers/char/KeyMaps
	${MAKE} -C KeyMaps distdir
	cp -pf CGAsnow Changes NEW_NOTES $(DISTDIR)/arch/i86/drivers/char
	cp -pf NOTES README_CONSOLE $(DISTDIR)/arch/i86/drivers/char
	cp -pf Makefile config.in $(DISTDIR)/arch/i86/drivers/char
	cp -pf *.S *.h *.c $(DISTDIR)/arch/i86/drivers/char

#########################################################################
### Dependencies:

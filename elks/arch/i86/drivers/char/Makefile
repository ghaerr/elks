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

USEBCC 		= Y

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

OBJS  = init.o mem.o ntty.o tcpdev.o pty.o lp.o

ifeq ($(CONFIG_ETH), y)
    OBJS += eth.o
endif

ifeq ($(CONFIG_ARCH_8018X), y)
	CONIO = 8018x
	SERIAL = template
endif

ifeq ($(CONFIG_ARCH_IBMPC), y)
	CONIO = bios
	SERIAL = 8250
endif

ifeq ($(CONFIG_ARCH_PC98), y)
	SERIAL = pc98
endif

ifeq ($(CONFIG_ARCH_SWAN), y)
	CONIO = swan
	SERIAL = swan
endif

ifeq ($(CONFIG_ARCH_SOLO86), y)
	CONIO = solo86
endif

ifdef CONFIG_ARCH_PC98
ifdef CONFIG_CONSOLE_DIRECT
OBJS += console-direct-pc98.o
endif
else
ifdef CONFIG_ARCH_SWAN
ifdef CONFIG_CONSOLE_DIRECT
OBJS += console-direct-swan.o console-font-4x8-swan.o console-font-8x8-swan.o
endif
else
ifdef CONFIG_CONSOLE_DIRECT
OBJS += console-direct.o bell-8254.o
ifdef CONFIG_CONSOLE_DUAL
OBJS += crtc-6845.o
endif
endif
endif
endif

ifdef CONFIG_CONSOLE_BIOS
OBJS += console-bios.o console-bios-asm.o bell-8254.o
endif

ifdef CONFIG_CONSOLE_8018X
OBJS += console-serial-8018x.o
endif

ifdef CONFIG_ARCH_PC98
ifdef CONFIG_CONSOLE_HEADLESS
OBJS += console-headless.o
endif
OBJS += conio-pc98.o conio-pc98-asm.o kbd-poll-pc98.o
endif

ifdef CONFIG_ARCH_SWAN
ifdef CONFIG_CONSOLE_HEADLESS
OBJS += console-headless.o
endif
OBJS += kbd-poll.o bell-swan.o conio-swan.o
endif

ifdef CONFIG_ARCH_IBMPC
ifdef CONFIG_CONSOLE_HEADLESS
OBJS += console-headless.o
endif

ifdef CONFIG_KEYBOARD_SCANCODE
OBJS += kbd-scancode.o
else
OBJS += kbd-poll.o conio-$(CONIO).o
endif
endif

ifdef CONFIG_ARCH_SOLO86
ifdef CONFIG_CONSOLE_HEADLESS
OBJS += console-headless.o
endif

OBJS += kbd-poll.o conio-$(CONIO).o
endif

ifdef CONFIG_CHAR_DEV_RS
	OBJS += serial-$(SERIAL).o
	ifdef CONFIG_ARCH_IBMPC
		OBJS += serfast.o
	endif
endif

ifdef CONFIG_CHAR_DEV_CGATEXT
	OBJS += cgatext.o
endif

#########################################################################
# Commands.

all:	chr_drv.a

chr_drv.a: $(OBJS)
	$(AR) rcs chr_drv.a $(OBJS)

KeyMaps/keymaps.h:
	$(MAKE) -C KeyMaps keymaps.h

console-direct.o: console.c

console-direct-pc98.o: console.c

console-bios.o: console.c

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

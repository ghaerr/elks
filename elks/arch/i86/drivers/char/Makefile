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

OBJS  = init.o ntty.o
OBJS += pty.o                               # CONFIG_PSEUDO_TTY
OBJS += mem.o                               # CONFIG_CHAR_DEV_MEM
OBJS += lp.o                                # CONFIG_CHAR_DEV_LP
OBJS += eth.o                               # CONFIG_ETH
OBJS += tcpdev.o                            # CONFIG_INET
OBJS += audio-sb.o audio-mad.o              # CONFIG_AUDIO_SB
OBJS += mouse-ps2.o                         # CONFIG_MOUSE_PS2

#
# Platform-dependent console, keyboard and serial
#
ifdef CONFIG_ARCH_IBMPC
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
OBJS += crtc-6845.o                         # CONFIG_CONSOLE_DUAL
OBJS += audio-8254.o                        # CONFIG_AUDIO
OBJS += serial-8250.o serfast.o             # CONFIG_CHAR_DEV_RS
endif

ifdef CONFIG_ARCH_PC98
  ifdef CONFIG_CONSOLE_DIRECT
    OBJS += console-direct-pc98.o
  endif
OBJS += conio-pc98.o conio-pc98-asm.o kbd-poll-pc98.o
OBJS += serial-pc98.o
endif

ifdef CONFIG_ARCH_SOLO86
OBJS += console-headless.o conio-solo86.o kbd-poll.o
endif

ifdef CONFIG_CONSOLE_8018X
OBJS += console-serial-8018x.o conio-8018x.o
endif

ifdef CONFIG_CONSOLE_NECV25
OBJS += conio-necv25.o
OBJS += console-serial-necv25.o serfast.o
endif

ifdef CONFIG_ARCH_SWAN
  ifdef CONFIG_CONSOLE_DIRECT
    OBJS += console-direct-swan.o console-font-4x8-swan.o console-font-8x8-swan.o
  endif
OBJS += kbd-poll.o conio-swan.o bell-swan.o
OBJS += serial-swan.o
endif

#########################################################################
# Commands.

all:	chr_drv.a

chr_drv.a: $(OBJS)
	$(AR) rcs chr_drv.a $(OBJS)

KeyMaps/keymaps.h:
	$(MAKE) -C KeyMaps keymaps.h

console-direct.o: console.c direct-ibmpc.c direct-amstrad.c

console-direct-pc98.o: console.c

console-bios.o: console.c

#########################################################################
# Standard commands.

#########################################################################
### Dependencies:

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

DISTFILES	= CGAsnow Changes NEW_NOTES NOTES

NOINDENT	= bell.c bioscon.c
# console.c sibo_con.c

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

ifeq ($(CONFIG_ARCH_SIBO), y)
OBJS  = init.o con_asm.o font.o sibo_key.o key_asm.o mem.o \
	sibo_con.o meta.o ntty.o ser_asm.o pty.o
else
OBJS  = serial.o lp.o xt_key.o init.o dircon.o mem.o \
	ntty.o meta.o tcpdev.o pty.o bell.o

ifdef CONFIG_CONSOLE_BIOS
OBJS += bioscon.o bioscon-low.o
endif

endif # CONFIG_ARCH_SIBO

#########################################################################
# Commands.

all:	chr_drv.a

chr_drv.a: $(OBJS)
	$(AR) rcs chr_drv.a $(OBJS)

KeyMaps/keymaps.h:
	$(MAKE) -C KeyMaps keymaps.h

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

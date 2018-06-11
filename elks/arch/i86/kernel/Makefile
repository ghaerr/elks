# Makefile for the Linux/MT-kernel.
#
#########################################################################
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
# Relative path to base directory.

BASEDIR 	= ../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

CLEANDEP	= 

CLEANME 	= entry.c entry.S entry.s

DEPEND  	= 

DISTFILES	= syscall.dat

NOINDENT	= bios16.S irq.c irqtab.S printreg.S process.c strace.h \
		  system.c timer.c

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

ifeq ($(CONFIG_ARCH_SIBO), y)
OBJS  = strace.o printreg.o system.o irq.o ../sibo/irqtab.o process.o \
		entry.o signal.o timer.o
else
OBJS  = strace.o printreg.o system.o irq.o irqtab.o process.o bios16.o \
		entry.o signal.o timer.o
endif

#########################################################################
# Commands.

irqtab.s: irqtab.S
irqtab.o: irqtab.s

bios16.s: bios16.S
bios16.o: bios16.s

printreg.s: printreg.S
printreg.o: printreg.s

entry.s: entry.S
entry.o: entry.s

all:	akernel.a

akernel.a: $(OBJS)
	ar rcs akernel.a $(OBJS)

entry.S: syscall.dat mkentry.sh
	sh mkentry.sh > entry.tmp
	mv entry.tmp entry.S

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile mkentry.sh $(DISTDIR)/arch/i86/kernel
	cp -pf syscall.dat $(DISTDIR)/arch/i86/kernel
	cp -pf *.c $(DISTDIR)/arch/i86/kernel

#########################################################################
### Dependencies:

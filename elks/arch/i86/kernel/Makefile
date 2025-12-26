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

CLEANME 	= 

DEPEND  	= 

DISTFILES	= 

NOINDENT	= irqtab.S process.c strace.h system.c timer.c

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS  = strace.o irq.o irqtab.o process.o \
		entry.o signal.o timer.o softirq.o

ifeq ($(CONFIG_ARCH_IBMPC), y)
OBJS += irq-8259.o reset-ibm.o timer-8254.o divzero.o nmi.o
endif

ifeq ($(CONFIG_ARCH_PC98), y)
OBJS += irq-8259.o reset-pc98.o timer-8254.o divzero.o nmi.o
endif

ifeq ($(CONFIG_ARCH_SOLO86), y)
OBJS += irq-solo86.o reset-stubs.o timer-8254.o divzero.o nmi.o
endif

ifeq ($(CONFIG_ARCH_8018X), y)
OBJS += irq-8018x.o reset-stubs.o timer-8018x.o
endif

ifeq ($(CONFIG_ARCH_NECV25), y)
OBJS += irq-necv25.o reset-stubs.o timer-necv25.o
endif

ifeq ($(CONFIG_ARCH_SWAN), y)
OBJS += irq-swan.o reset-swan.o timer-swan.o system-swan.o divzero.o nmi.o
else
OBJS += system.o
endif

#########################################################################
# Commands.

all:	akernel.a

akernel.a: $(OBJS)
	$(AR) rcs akernel.a $(OBJS)

entry.S: syscall.dat mkentry.sh
	sh mkentry.sh > entry.tmp
	mv entry.tmp entry.S

strace.o: strace.c
	$(CC) $(CFLAGS) -fno-optimize-sibling-calls -c -o strace.o strace.c

#
# Special compilations apparently needed for startup failure using emu86 w/ROM
# FIXME Apparent stack trashing but not fully verified
irq.o: irq.c
	$(CC) $(CFLAGS) -fno-defer-pop -c -o irq.o irq.c

#irq-8259.o: irq-8259.c
#	$(CC) $(CFLAGS) -fno-defer-pop -c -o irq-8259.o irq-8259.c

#timer-8254.o: timer-8254.c
#	$(CC) $(CFLAGS) -fno-defer-pop -c -o timer-8254.o timer-8254.c

#########################################################################
# Standard commands.

#########################################################################
### Dependencies:

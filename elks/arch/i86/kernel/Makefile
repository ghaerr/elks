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

ELKSDIR 	= ../../..

#########################################################################
# Include standard commands.

include $(ELKSDIR)/Makefile-rules

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

all:	akernel.a

akernel.a: $(OBJS)
	ar rcs akernel.a $(OBJS)

entry.c: syscall.dat mkentry.sh
	sh mkentry.sh > entry.tmp
	mv entry.tmp entry.c

#########################################################################
# Standard commands.

clean:
	rm -f core *~ *.o *.a *.tmp keyboard.s entry.c
	for i in *.c;do rm -f `basename $$i .c`.s;done

distdir:
	cp -pf Makefile mkentry.sh $(DISTDIR)/arch/i86/kernel
	cp -pf syscall.dat $(DISTDIR)/arch/i86/kernel
	cp -pf *.c $(DISTDIR)/arch/i86/kernel

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > make.tmp
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> make.tmp
	mv make.tmp Makefile

#########################################################################
### Dependencies:

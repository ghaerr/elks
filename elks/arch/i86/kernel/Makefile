# Makefile for the Linux/MT-kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
#
# Note that the standard Makefile rules and defines have been moved to
# the file below.

ELKSDIR		= ../../..

include $(ELKSDIR)/Makefile-rules

#########################################################################

ifeq ($(CONFIG_ARCH_SIBO), y)
OBJS  = strace.o printreg.o system.o irq.o ../sibo/irqtab.o process.o \
		entry.o signal.o timer.o
else
OBJS  = strace.o printreg.o system.o irq.o irqtab.o process.o bios16.o \
		entry.o signal.o timer.o
endif

all:	akernel.a

akernel.a: $(OBJS)
	ar rcs akernel.a $(OBJS)

entry.c: syscall.dat mkentry.sh
	sh mkentry.sh > tmp.o
	mv tmp.o entry.c

clean:
	rm -f core *~ *.o *.a tmp_make keyboard.s entry.c
	for i in *.c;do rm -f `basename $$i .c`.s;done

distdir:
	cp -pf Makefile mkentry.sh $(TOPDIR)/$(DISTDIR)/arch/i86/kernel
	cp -pf syscall.dat $(TOPDIR)/$(DISTDIR)/arch/i86/kernel
	cp -pf *.c $(TOPDIR)/$(DISTDIR)/arch/i86/kernel

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

#########################################################################
### Dependencies:

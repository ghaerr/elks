# Makefile for the ELKS character devices.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
#
# Note that the standard Makefile rules and defines have been moved to
# the file below.

ELKSDIR		= ../../../..

include $(ELKSDIR)/Makefile-rules

#########################################################################

ifeq ($(CONFIG_ARCH_SIBO), y)
OBJS  = init.o con_asm.o font.o sibo_key.o key_asm.o sibo_con.o \
	mem.o meta.o ntty.o ser_asm.o pty.o
else
OBJS  = bioscon.o serial.o lp.o xt_key.o init.o dircon.o ntty.o mem.o meta.o \
	tcpdev.o pty.o bell.o # clist.o tty.o
endif

all:	chr_dev.a

chr_drv.a: $(OBJS)
	ar rcs chr_drv.a $(OBJS)
	sync

clean:
	rm -f core *~ *.o *.a tmp_make
	for i in *.c;do rm -f `basename $$i .c`.s;done
	for i in *.S;do rm -f `basename $$i .S`.s;done

distdir:
	cp -pf CGAsnow Changes NEW_NOTES $(TOPDIR)/$(DISTDIR)/arch/i86/drivers/char
	cp -pf NOTES README_CONSOLE $(TOPDIR)/$(DISTDIR)/arch/i86/drivers/char
	cp -pf Makefile Config.in $(TOPDIR)/$(DISTDIR)/arch/i86/drivers/char
	cp -pf *.S *.h *.c $(TOPDIR)/$(DISTDIR)/arch/i86/drivers/char

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

font.s: font.S

font.o: font.s

key_asm.s: key_asm.S

key_asm.o: key_asm.s

con_asm.s: con_asm.S

con_asm.o: con_asm.s

ser_asm.s: ser_asm.S

ser_asm.o: ser_asm.s

#########################################################################
### Dependencies:

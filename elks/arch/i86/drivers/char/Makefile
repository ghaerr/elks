#
# Makefile for the ELKS character devices.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

AR	=ar
AS	=as86 
LD	=ld86 
LDFLAGS	=-0 -s -x
CC	=bcc
CFLAGS	=-0 -I../../../../include $(CFLBASE)
CPP	=bcc -E -nostdinc -I../../../../include
CC_PROTO = gcc -I../../../../include -M

.c.s:
	$(CC) $(CFLAGS) \
	-0 -S -o $*.s $<
.s.o:
	$(AS) -0 -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-0 -c -o $*.o $<

OBJS  = bioscon.o serial.o lp.o xt_key.o init.o dircon.o ntty.o mem.o meta.o \
	pty.o # clist.o tty.o

chr_drv.a: $(OBJS)
	ar rcs chr_drv.a $(OBJS)
	sync

clean:
	rm -f core *~ *.o *.a tmp_make keyboard.s
	for i in *.c;do rm -f `basename $$i .c`.s;done

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

### Dependencies:

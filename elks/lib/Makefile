#
# Makefile for the Linux/MT-kernel.
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
CFLAGS	=$(CFLBASE) -I../include 
CPP	=bcc -E -nostdinc -I../include
CC_PROTO =gcc -I../include -M

.c.s:
	$(CC) $(CFLAGS) \
	-0 -S -o $*.s $<
.s.o:
	$(AS) -0 -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-0 -c -o $*.o $<

OBJS  = chqueue.o string.o

lib.a: $(OBJS)
	ar rcs lib.a $(OBJS)
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
chqueue.s chqueue.o: chqueue.c ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/wait.h \
 ../include/linuxmt/chqueue.h ../include/linuxmt/debug.h \
 ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/arch/posix_types.h \
 ../include/arch/irq.h
string.s string.o: string.c ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/linuxmt/string.h \
 ../include/arch/string.h

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
CFLAGS	=$(CFLBASE) -Wall -I../../../include
CPP	=bcc -E -nostdinc -I../../../include
CC_PROTO =gcc -I../../../include -M

.c.s:
	$(CC) $(CFLAGS) \
	-0 -S -o $*.s $<
.s.o:
	$(AS) -0 -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-0 -c -o $*.o $<

OBJS  = init.o segment.o malloc.o user.o

mm.a: $(OBJS)
	ar rcs mm.a $(OBJS)
	sync

clean:
	rm -f core *.o *.a tmp_make keyboard.s
	for i in *.c;do rm -f `basename $$i .c`.s;done

distdir:
	cp -pf Makefile $(TOPDIR)/$(DISTDIR)/arch/i86/mm
	cp -pf *.c $(TOPDIR)/$(DISTDIR)/arch/i86/mm

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

### Dependencies:
init.s init.o: init.c ../../../include/linuxmt/types.h \
 ../../../include/arch/types.h ../../../include/linuxmt/posix_types.h \
 ../../../include/linuxmt/config.h ../../../include/linuxmt/autoconf.h \
 ../../../include/arch/posix_types.h ../../../include/arch/irq.h \
 ../../../include/arch/system.h ../../../include/arch/segment.h
malloc.s malloc.o: malloc.c ../../../include/linuxmt/types.h \
 ../../../include/arch/types.h ../../../include/linuxmt/posix_types.h \
 ../../../include/linuxmt/config.h ../../../include/linuxmt/autoconf.h \
 ../../../include/arch/posix_types.h ../../../include/arch/irq.h \
 ../../../include/linuxmt/sched.h ../../../include/linuxmt/fs.h \
 ../../../include/linuxmt/wait.h ../../../include/linuxmt/vfs.h \
 ../../../include/arch/statfs.h ../../../include/linuxmt/kdev_t.h \
 ../../../include/linuxmt/ioctl.h ../../../include/arch/ioctl.h \
 ../../../include/linuxmt/pipe_fs_i.h ../../../include/linuxmt/net.h \
 ../../../include/linuxmt/socket.h ../../../include/linuxmt/uio.h \
 ../../../include/arch/bitops.h ../../../include/linuxmt/time.h \
 ../../../include/linuxmt/signal.h ../../../include/linuxmt/ntty.h \
 ../../../include/arch/param.h ../../../include/linuxmt/timex.h \
 ../../../include/linuxmt/mm.h ../../../include/linuxmt/errno.h \
 ../../../include/arch/errno.h ../../../include/linuxmt/kernel.h \
 ../../../include/linuxmt/string.h ../../../include/arch/string.h
segment.s segment.o: segment.c ../../../include/arch/segment.h
user.s user.o: user.c ../../../include/linuxmt/types.h \
 ../../../include/arch/types.h ../../../include/linuxmt/posix_types.h \
 ../../../include/linuxmt/config.h ../../../include/linuxmt/autoconf.h \
 ../../../include/arch/posix_types.h ../../../include/arch/irq.h \
 ../../../include/linuxmt/sched.h ../../../include/linuxmt/fs.h \
 ../../../include/linuxmt/wait.h ../../../include/linuxmt/vfs.h \
 ../../../include/arch/statfs.h ../../../include/linuxmt/kdev_t.h \
 ../../../include/linuxmt/ioctl.h ../../../include/arch/ioctl.h \
 ../../../include/linuxmt/pipe_fs_i.h ../../../include/linuxmt/net.h \
 ../../../include/linuxmt/socket.h ../../../include/linuxmt/uio.h \
 ../../../include/arch/bitops.h ../../../include/linuxmt/time.h \
 ../../../include/linuxmt/signal.h ../../../include/linuxmt/ntty.h \
 ../../../include/arch/param.h ../../../include/linuxmt/timex.h \
 ../../../include/arch/segment.h ../../../include/linuxmt/mm.h \
 ../../../include/linuxmt/errno.h ../../../include/arch/errno.h \
 ../../../include/linuxmt/kernel.h ../../../include/linuxmt/string.h \
 ../../../include/arch/string.h

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

OBJS  = socket.o protocols.o ipv4/af_inet.o unix/af_unix.o

net.a: $(OBJS)
	ar rcs net.a $(OBJS)

clean:
	rm -f core *~ *.o *.a tmp_make
	for i in *.c;do rm -f `basename $$i .c`.s;done

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

### Dependencies:
protocols.s protocols.o: protocols.c ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/net.h ../include/linuxmt/wait.h \
 ../include/linuxmt/socket.h ../include/linuxmt/uio.h \
 ../include/linuxmt/fs.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/arch/bitops.h
socket.s socket.o: socket.c ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/errno.h \
 ../include/arch/errno.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/linuxmt/net.h \
 ../include/linuxmt/wait.h ../include/linuxmt/fs.h \
 ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/arch/bitops.h \
 ../include/linuxmt/mm.h ../include/linuxmt/sched.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/arch/param.h ../include/linuxmt/timex.h \
 ../include/linuxmt/kernel.h ../include/linuxmt/string.h \
 ../include/arch/string.h ../include/linuxmt/fcntl.h \
 ../include/linuxmt/stat.h ../include/arch/stat.h

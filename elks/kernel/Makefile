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

OBJS  = sched.o printk.o sleepwake.o dma.o version.o sys.o stubs.o fork.o \
	exit.o time.o signal.o

kernel.a: $(OBJS)
	ar rcs kernel.a $(OBJS)

clean:
	rm -f core *~ *.o *.a tmp_make keyboard.s
	for i in *.c;do rm -f `basename $$i .c`.s;done

distdir:
	cp -pf Makefile $(TOPDIR)/$(DISTDIR)/kernel
	cp -pf *.c $(TOPDIR)/$(DISTDIR)/kernel

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

### Dependencies:
dma.s dma.o: dma.c ../include/linuxmt/config.h ../include/linuxmt/autoconf.h
exit.s exit.o: exit.c ../include/linuxmt/sched.h ../include/linuxmt/fs.h \
 ../include/linuxmt/wait.h ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/linuxmt/config.h ../include/linuxmt/autoconf.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/arch/bitops.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/linuxmt/ntty.h ../include/arch/param.h \
 ../include/linuxmt/timex.h ../include/linuxmt/errno.h \
 ../include/arch/errno.h
fork.s fork.o: fork.c ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/errno.h \
 ../include/arch/errno.h ../include/linuxmt/sched.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/linuxmt/net.h \
 ../include/linuxmt/socket.h ../include/linuxmt/uio.h \
 ../include/arch/bitops.h ../include/linuxmt/time.h \
 ../include/linuxmt/signal.h ../include/linuxmt/ntty.h \
 ../include/arch/param.h ../include/linuxmt/timex.h \
 ../include/linuxmt/debug.h
memdumpk.s memdumpk.o: memdumpk.c ../include/linuxmt/kernel.h
printk.s printk.o: printk.c ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/linuxmt/fcntl.h \
 ../include/linuxmt/sched.h ../include/linuxmt/fs.h \
 ../include/linuxmt/wait.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/linuxmt/net.h \
 ../include/linuxmt/socket.h ../include/linuxmt/uio.h \
 ../include/arch/bitops.h ../include/linuxmt/time.h \
 ../include/linuxmt/signal.h ../include/linuxmt/ntty.h \
 ../include/arch/param.h ../include/linuxmt/timex.h
sched.s sched.o: sched.c ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/linuxmt/sched.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/arch/bitops.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/linuxmt/ntty.h ../include/arch/param.h \
 ../include/linuxmt/timex.h ../include/linuxmt/timer.h
signal.s signal.o: signal.c ../include/linuxmt/debug.h \
 ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/linuxmt/sched.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/arch/bitops.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/linuxmt/ntty.h ../include/arch/param.h \
 ../include/linuxmt/timex.h ../include/linuxmt/kernel.h \
 ../include/linuxmt/errno.h ../include/arch/errno.h \
 ../include/linuxmt/mm.h ../include/linuxmt/string.h \
 ../include/arch/string.h ../include/arch/segment.h
sleepwake.s sleepwake.o: sleepwake.c ../include/arch/irq.h ../include/arch/types.h \
 ../include/linuxmt/types.h ../include/linuxmt/posix_types.h \
 ../include/linuxmt/config.h ../include/linuxmt/autoconf.h \
 ../include/arch/posix_types.h ../include/linuxmt/sched.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/arch/bitops.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/linuxmt/ntty.h ../include/arch/param.h \
 ../include/linuxmt/timex.h
stubs.s stubs.o: stubs.c ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/linuxmt/time.h
sys.s sys.o: sys.c ../include/linuxmt/errno.h ../include/arch/errno.h \
 ../include/linuxmt/sched.h ../include/linuxmt/fs.h \
 ../include/linuxmt/wait.h ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/linuxmt/config.h ../include/linuxmt/autoconf.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/arch/bitops.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/linuxmt/ntty.h ../include/arch/param.h \
 ../include/linuxmt/timex.h ../include/linuxmt/kernel.h \
 ../include/linuxmt/utsname.h ../include/linuxmt/string.h \
 ../include/arch/string.h ../include/linuxmt/stat.h \
 ../include/arch/stat.h ../include/linuxmt/mm.h \
 ../include/arch/segment.h ../include/arch/io.h
syscall.s syscall.o: syscall.c ../include/linuxmt/errno.h \
 ../include/arch/errno.h
time.s time.o: time.c ../include/arch/types.h ../include/linuxmt/time.h \
 ../include/linuxmt/timex.h ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/errno.h \
 ../include/arch/errno.h ../include/linuxmt/mm.h \
 ../include/linuxmt/sched.h ../include/linuxmt/fs.h \
 ../include/linuxmt/wait.h ../include/linuxmt/types.h \
 ../include/linuxmt/posix_types.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/linuxmt/net.h \
 ../include/linuxmt/socket.h ../include/linuxmt/uio.h \
 ../include/arch/bitops.h ../include/linuxmt/signal.h \
 ../include/linuxmt/ntty.h ../include/arch/param.h \
 ../include/linuxmt/kernel.h ../include/linuxmt/string.h \
 ../include/arch/string.h ../include/arch/system.h \
 ../include/arch/segment.h
version.s version.o: version.c ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/linuxmt/config.h ../include/linuxmt/autoconf.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/utsname.h ../include/linuxmt/version.h \
 ../include/linuxmt/compile.h

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

OBJS  = super.o bufops.o devices.o fcntl.o stat.o \
	inode.o file_table.o buffer.o block_dev.o \
	namei.o ioctl.o filesystems.o open.o \
	read_write.o readdir.o exec.o select.o pipe.o

fs.a: $(OBJS)
	ar rcs fs.a $(OBJS)

clean:
	rm -f core *~ *.o *.a tmp_make keyboard.s
	for i in *.c;do rm -f `basename $$i .c`.s;done

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile
block_dev.s block_dev.o: block_dev.c ../include/linuxmt/errno.h \
 ../include/arch/errno.h ../include/linuxmt/sched.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/linuxmt/net.h \
 ../include/linuxmt/config.h ../include/linuxmt/autoconf.h \
 ../include/linuxmt/socket.h ../include/linuxmt/uio.h \
 ../include/arch/bitops.h ../include/linuxmt/time.h \
 ../include/linuxmt/signal.h ../include/arch/param.h \
 ../include/linuxmt/timex.h ../include/linuxmt/kernel.h \
 ../include/linuxmt/fcntl.h ../include/linuxmt/mm.h \
 ../include/linuxmt/string.h ../include/arch/string.h \
 ../include/arch/segment.h ../include/arch/system.h
buffer.s buffer.o: buffer.c ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/linuxmt/sched.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/arch/bitops.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/arch/param.h ../include/linuxmt/timex.h \
 ../include/linuxmt/kernel.h ../include/linuxmt/major.h \
 ../include/linuxmt/string.h ../include/arch/string.h \
 ../include/linuxmt/errno.h ../include/arch/errno.h \
 ../include/linuxmt/debug.h ../include/arch/system.h \
 ../include/arch/segment.h ../include/arch/io.h
bufops.s bufops.o: bufops.c ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/arch/bitops.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h
devices.s devices.o: devices.c ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/arch/bitops.h \
 ../include/linuxmt/major.h ../include/linuxmt/string.h \
 ../include/arch/string.h ../include/linuxmt/sched.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/arch/param.h ../include/linuxmt/timex.h \
 ../include/linuxmt/stat.h ../include/arch/stat.h \
 ../include/linuxmt/fcntl.h ../include/linuxmt/errno.h \
 ../include/arch/errno.h
exec.s exec.o: exec.c ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/linuxmt/utime.h \
 ../include/linuxmt/errno.h ../include/arch/errno.h \
 ../include/linuxmt/fcntl.h ../include/linuxmt/stat.h \
 ../include/arch/stat.h ../include/linuxmt/string.h \
 ../include/arch/string.h ../include/linuxmt/sched.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/arch/bitops.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/arch/param.h ../include/linuxmt/timex.h \
 ../include/linuxmt/kernel.h ../include/linuxmt/tty.h \
 ../include/linuxmt/mm.h ../include/linuxmt/minix.h \
 ../include/linuxmt/debug.h ../include/arch/segment.h
fcntl.s fcntl.o: fcntl.c ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/arch/segment.h \
 ../include/linuxmt/sched.h ../include/linuxmt/fs.h \
 ../include/linuxmt/wait.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/linuxmt/net.h \
 ../include/linuxmt/config.h ../include/linuxmt/autoconf.h \
 ../include/linuxmt/socket.h ../include/linuxmt/uio.h \
 ../include/arch/bitops.h ../include/linuxmt/time.h \
 ../include/linuxmt/signal.h ../include/arch/param.h \
 ../include/linuxmt/timex.h ../include/linuxmt/kernel.h \
 ../include/linuxmt/errno.h ../include/arch/errno.h \
 ../include/linuxmt/stat.h ../include/arch/stat.h \
 ../include/linuxmt/fcntl.h ../include/linuxmt/string.h \
 ../include/arch/string.h
file_table.s file_table.o: file_table.c ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/config.h ../include/linuxmt/autoconf.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/arch/bitops.h \
 ../include/linuxmt/string.h ../include/arch/string.h \
 ../include/linuxmt/mm.h ../include/linuxmt/sched.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/arch/param.h ../include/linuxmt/timex.h \
 ../include/linuxmt/errno.h ../include/arch/errno.h \
 ../include/linuxmt/kernel.h
filesystems.s filesystems.o: filesystems.c ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/arch/bitops.h \
 ../include/linuxmt/minix_fs.h ../include/linuxmt/romfs_fs.h \
 ../include/linuxmt/elksfs_fs.h ../include/linuxmt/major.h
inode.s inode.o: inode.c ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/stat.h ../include/arch/stat.h \
 ../include/linuxmt/sched.h ../include/linuxmt/fs.h \
 ../include/linuxmt/wait.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/linuxmt/net.h \
 ../include/linuxmt/socket.h ../include/linuxmt/uio.h \
 ../include/arch/bitops.h ../include/linuxmt/time.h \
 ../include/linuxmt/signal.h ../include/arch/param.h \
 ../include/linuxmt/timex.h ../include/linuxmt/kernel.h \
 ../include/linuxmt/mm.h ../include/linuxmt/errno.h \
 ../include/arch/errno.h ../include/linuxmt/string.h \
 ../include/arch/string.h ../include/linuxmt/debug.h \
 ../include/arch/system.h
ioctl.s ioctl.o: ioctl.c ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/arch/segment.h \
 ../include/linuxmt/sched.h ../include/linuxmt/fs.h \
 ../include/linuxmt/wait.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/linuxmt/net.h \
 ../include/linuxmt/config.h ../include/linuxmt/autoconf.h \
 ../include/linuxmt/socket.h ../include/linuxmt/uio.h \
 ../include/arch/bitops.h ../include/linuxmt/time.h \
 ../include/linuxmt/signal.h ../include/arch/param.h \
 ../include/linuxmt/timex.h ../include/linuxmt/mm.h \
 ../include/linuxmt/errno.h ../include/arch/errno.h \
 ../include/linuxmt/kernel.h ../include/linuxmt/string.h \
 ../include/arch/string.h ../include/linuxmt/stat.h \
 ../include/arch/stat.h ../include/linuxmt/termios.h \
 ../include/linuxmt/fcntl.h
namei.s namei.o: namei.c ../include/arch/segment.h ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/errno.h ../include/arch/errno.h \
 ../include/linuxmt/sched.h ../include/linuxmt/fs.h \
 ../include/linuxmt/wait.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/linuxmt/net.h \
 ../include/linuxmt/config.h ../include/linuxmt/autoconf.h \
 ../include/linuxmt/socket.h ../include/linuxmt/uio.h \
 ../include/arch/bitops.h ../include/linuxmt/time.h \
 ../include/linuxmt/signal.h ../include/arch/param.h \
 ../include/linuxmt/timex.h ../include/linuxmt/kernel.h \
 ../include/linuxmt/string.h ../include/arch/string.h \
 ../include/linuxmt/fcntl.h ../include/linuxmt/stat.h \
 ../include/arch/stat.h ../include/linuxmt/mm.h \
 ../include/linuxmt/debug.h
open.s open.o: open.c ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/linuxmt/utime.h \
 ../include/linuxmt/errno.h ../include/arch/errno.h \
 ../include/linuxmt/fcntl.h ../include/linuxmt/stat.h \
 ../include/arch/stat.h ../include/linuxmt/string.h \
 ../include/arch/string.h ../include/linuxmt/sched.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/arch/bitops.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/arch/param.h ../include/linuxmt/timex.h \
 ../include/linuxmt/kernel.h ../include/linuxmt/tty.h \
 ../include/linuxmt/mm.h ../include/arch/segment.h
pipe.s pipe.o: pipe.c ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/sched.h ../include/linuxmt/fs.h \
 ../include/linuxmt/wait.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/linuxmt/net.h \
 ../include/linuxmt/socket.h ../include/linuxmt/uio.h \
 ../include/arch/bitops.h ../include/linuxmt/time.h \
 ../include/linuxmt/signal.h ../include/arch/param.h \
 ../include/linuxmt/timex.h ../include/linuxmt/kernel.h \
 ../include/linuxmt/major.h ../include/linuxmt/stat.h \
 ../include/arch/stat.h ../include/linuxmt/errno.h \
 ../include/arch/errno.h ../include/linuxmt/fcntl.h \
 ../include/linuxmt/string.h ../include/arch/string.h \
 ../include/linuxmt/locks.h ../include/linuxmt/mm.h \
 ../include/linuxmt/debug.h ../include/arch/system.h \
 ../include/arch/segment.h
read_write.s read_write.o: read_write.c ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/errno.h ../include/arch/errno.h \
 ../include/linuxmt/stat.h ../include/arch/stat.h \
 ../include/linuxmt/kernel.h ../include/linuxmt/sched.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/arch/bitops.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/arch/param.h ../include/linuxmt/timex.h \
 ../include/linuxmt/fcntl.h ../include/linuxmt/mm.h \
 ../include/linuxmt/string.h ../include/arch/string.h \
 ../include/arch/segment.h ../include/linuxmt/debug.h
readdir.s readdir.o: readdir.c ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/errno.h ../include/arch/errno.h \
 ../include/linuxmt/stat.h ../include/arch/stat.h \
 ../include/linuxmt/kernel.h ../include/linuxmt/sched.h \
 ../include/linuxmt/fs.h ../include/linuxmt/wait.h \
 ../include/linuxmt/vfs.h ../include/arch/statfs.h \
 ../include/linuxmt/kdev_t.h ../include/linuxmt/ioctl.h \
 ../include/arch/ioctl.h ../include/linuxmt/pipe_fs_i.h \
 ../include/linuxmt/net.h ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/socket.h \
 ../include/linuxmt/uio.h ../include/arch/bitops.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/arch/param.h ../include/linuxmt/timex.h \
 ../include/linuxmt/mm.h ../include/linuxmt/string.h \
 ../include/arch/string.h ../include/linuxmt/debug.h \
 ../include/arch/segment.h
select.s select.o: select.c ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/time.h ../include/linuxmt/fs.h \
 ../include/linuxmt/wait.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/linuxmt/net.h \
 ../include/linuxmt/socket.h ../include/linuxmt/uio.h \
 ../include/arch/bitops.h ../include/linuxmt/kernel.h \
 ../include/linuxmt/sched.h ../include/linuxmt/signal.h \
 ../include/arch/param.h ../include/linuxmt/timex.h \
 ../include/linuxmt/string.h ../include/arch/string.h \
 ../include/linuxmt/stat.h ../include/arch/stat.h \
 ../include/linuxmt/errno.h ../include/arch/errno.h \
 ../include/linuxmt/mm.h ../include/arch/segment.h \
 ../include/arch/system.h
stat.s stat.o: stat.c ../include/linuxmt/types.h ../include/arch/types.h \
 ../include/linuxmt/posix_types.h ../include/arch/posix_types.h \
 ../include/arch/irq.h ../include/linuxmt/errno.h \
 ../include/arch/errno.h ../include/linuxmt/string.h \
 ../include/arch/string.h ../include/linuxmt/stat.h \
 ../include/arch/stat.h ../include/linuxmt/fs.h \
 ../include/linuxmt/wait.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/linuxmt/net.h \
 ../include/linuxmt/config.h ../include/linuxmt/autoconf.h \
 ../include/linuxmt/socket.h ../include/linuxmt/uio.h \
 ../include/arch/bitops.h ../include/linuxmt/sched.h \
 ../include/linuxmt/time.h ../include/linuxmt/signal.h \
 ../include/arch/param.h ../include/linuxmt/timex.h \
 ../include/linuxmt/kernel.h ../include/linuxmt/mm.h \
 ../include/arch/segment.h
super.s super.o: super.c ../include/linuxmt/config.h \
 ../include/linuxmt/autoconf.h ../include/linuxmt/types.h \
 ../include/arch/types.h ../include/linuxmt/posix_types.h \
 ../include/arch/posix_types.h ../include/arch/irq.h \
 ../include/linuxmt/sched.h ../include/linuxmt/fs.h \
 ../include/linuxmt/wait.h ../include/linuxmt/vfs.h \
 ../include/arch/statfs.h ../include/linuxmt/kdev_t.h \
 ../include/linuxmt/ioctl.h ../include/arch/ioctl.h \
 ../include/linuxmt/pipe_fs_i.h ../include/linuxmt/net.h \
 ../include/linuxmt/socket.h ../include/linuxmt/uio.h \
 ../include/arch/bitops.h ../include/linuxmt/time.h \
 ../include/linuxmt/signal.h ../include/arch/param.h \
 ../include/linuxmt/timex.h ../include/linuxmt/kernel.h \
 ../include/linuxmt/major.h ../include/linuxmt/stat.h \
 ../include/arch/stat.h ../include/linuxmt/errno.h \
 ../include/arch/errno.h ../include/linuxmt/string.h \
 ../include/arch/string.h ../include/linuxmt/locks.h \
 ../include/linuxmt/mm.h ../include/linuxmt/debug.h \
 ../include/arch/system.h ../include/arch/segment.h

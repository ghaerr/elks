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
CFLAGS	=$(CFLBASE) -I../../include 
CPP	=bcc -E -nostdinc -I../../include
CC_PROTO =gcc -I../../include -M

.c.s:
	$(CC) $(CFLAGS) \
	-0 -S -o $*.s $<
.s.o:
	$(AS) -0 -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-0 -c -o $*.o $<

OBJS  = bitmap.o dir.o file.o namei.o symlink.o truncate.o inode.o

elksfs.a: $(OBJS)
	ar rcs elksfs.a $(OBJS)

clean:
	rm -f core *~ *.o *.a tmp_make
	for i in *.c;do rm -f `basename $$i .c`.s;done

distdir:
	cp -pf Makefile $(TOPDIR)/$(DISTDIR)/fs/elksfs
	cp -pf *.c $(TOPDIR)/$(DISTDIR)/fs/elksfs

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

### Dependencies
bitmap.s bitmap.o: bitmap.c ../../include/linuxmt/types.h \
 ../../include/arch/types.h ../../include/linuxmt/posix_types.h \
 ../../include/linuxmt/config.h ../../include/linuxmt/autoconf.h \
 ../../include/arch/posix_types.h ../../include/arch/irq.h \
 ../../include/linuxmt/sched.h ../../include/linuxmt/fs.h \
 ../../include/linuxmt/wait.h ../../include/linuxmt/vfs.h \
 ../../include/arch/statfs.h ../../include/linuxmt/kdev_t.h \
 ../../include/linuxmt/ioctl.h ../../include/arch/ioctl.h \
 ../../include/linuxmt/pipe_fs_i.h ../../include/linuxmt/net.h \
 ../../include/linuxmt/socket.h ../../include/linuxmt/uio.h \
 ../../include/arch/bitops.h ../../include/linuxmt/time.h \
 ../../include/linuxmt/signal.h ../../include/linuxmt/ntty.h \
 ../../include/arch/param.h ../../include/linuxmt/timex.h \
 ../../include/linuxmt/elksfs_fs.h ../../include/linuxmt/stat.h \
 ../../include/arch/stat.h ../../include/linuxmt/kernel.h \
 ../../include/linuxmt/string.h ../../include/arch/string.h
dir.s dir.o: dir.c ../../include/linuxmt/types.h ../../include/arch/types.h \
 ../../include/linuxmt/posix_types.h ../../include/linuxmt/config.h \
 ../../include/linuxmt/autoconf.h ../../include/arch/posix_types.h \
 ../../include/arch/irq.h ../../include/linuxmt/string.h \
 ../../include/arch/string.h ../../include/linuxmt/errno.h \
 ../../include/arch/errno.h ../../include/linuxmt/fs.h \
 ../../include/linuxmt/wait.h ../../include/linuxmt/vfs.h \
 ../../include/arch/statfs.h ../../include/linuxmt/kdev_t.h \
 ../../include/linuxmt/ioctl.h ../../include/arch/ioctl.h \
 ../../include/linuxmt/pipe_fs_i.h ../../include/linuxmt/net.h \
 ../../include/linuxmt/socket.h ../../include/linuxmt/uio.h \
 ../../include/arch/bitops.h ../../include/linuxmt/elksfs_fs.h \
 ../../include/linuxmt/stat.h ../../include/arch/stat.h \
 ../../include/arch/segment.h
file.s file.o: file.c ../../include/linuxmt/types.h \
 ../../include/arch/types.h ../../include/linuxmt/posix_types.h \
 ../../include/linuxmt/config.h ../../include/linuxmt/autoconf.h \
 ../../include/arch/posix_types.h ../../include/arch/irq.h \
 ../../include/linuxmt/sched.h ../../include/linuxmt/fs.h \
 ../../include/linuxmt/wait.h ../../include/linuxmt/vfs.h \
 ../../include/arch/statfs.h ../../include/linuxmt/kdev_t.h \
 ../../include/linuxmt/ioctl.h ../../include/arch/ioctl.h \
 ../../include/linuxmt/pipe_fs_i.h ../../include/linuxmt/net.h \
 ../../include/linuxmt/socket.h ../../include/linuxmt/uio.h \
 ../../include/arch/bitops.h ../../include/linuxmt/time.h \
 ../../include/linuxmt/signal.h ../../include/linuxmt/ntty.h \
 ../../include/arch/param.h ../../include/linuxmt/timex.h \
 ../../include/linuxmt/elksfs_fs.h ../../include/linuxmt/kernel.h \
 ../../include/linuxmt/errno.h ../../include/arch/errno.h \
 ../../include/linuxmt/fcntl.h ../../include/linuxmt/stat.h \
 ../../include/arch/stat.h ../../include/linuxmt/mm.h \
 ../../include/linuxmt/string.h ../../include/arch/string.h \
 ../../include/linuxmt/debug.h ../../include/arch/segment.h \
 ../../include/arch/system.h
inode.s inode.o: inode.c ../../include/linuxmt/types.h \
 ../../include/arch/types.h ../../include/linuxmt/posix_types.h \
 ../../include/linuxmt/config.h ../../include/linuxmt/autoconf.h \
 ../../include/arch/posix_types.h ../../include/arch/irq.h \
 ../../include/linuxmt/sched.h ../../include/linuxmt/fs.h \
 ../../include/linuxmt/wait.h ../../include/linuxmt/vfs.h \
 ../../include/arch/statfs.h ../../include/linuxmt/kdev_t.h \
 ../../include/linuxmt/ioctl.h ../../include/arch/ioctl.h \
 ../../include/linuxmt/pipe_fs_i.h ../../include/linuxmt/net.h \
 ../../include/linuxmt/socket.h ../../include/linuxmt/uio.h \
 ../../include/arch/bitops.h ../../include/linuxmt/time.h \
 ../../include/linuxmt/signal.h ../../include/linuxmt/ntty.h \
 ../../include/arch/param.h ../../include/linuxmt/timex.h \
 ../../include/linuxmt/elksfs_fs.h ../../include/linuxmt/kernel.h \
 ../../include/linuxmt/mm.h ../../include/linuxmt/errno.h \
 ../../include/arch/errno.h ../../include/linuxmt/string.h \
 ../../include/arch/string.h ../../include/linuxmt/stat.h \
 ../../include/arch/stat.h ../../include/linuxmt/debug.h \
 ../../include/arch/system.h ../../include/arch/segment.h
namei.s namei.o: namei.c ../../include/linuxmt/types.h \
 ../../include/arch/types.h ../../include/linuxmt/posix_types.h \
 ../../include/linuxmt/config.h ../../include/linuxmt/autoconf.h \
 ../../include/arch/posix_types.h ../../include/arch/irq.h \
 ../../include/linuxmt/sched.h ../../include/linuxmt/fs.h \
 ../../include/linuxmt/wait.h ../../include/linuxmt/vfs.h \
 ../../include/arch/statfs.h ../../include/linuxmt/kdev_t.h \
 ../../include/linuxmt/ioctl.h ../../include/arch/ioctl.h \
 ../../include/linuxmt/pipe_fs_i.h ../../include/linuxmt/net.h \
 ../../include/linuxmt/socket.h ../../include/linuxmt/uio.h \
 ../../include/arch/bitops.h ../../include/linuxmt/time.h \
 ../../include/linuxmt/signal.h ../../include/linuxmt/ntty.h \
 ../../include/arch/param.h ../../include/linuxmt/timex.h \
 ../../include/linuxmt/elksfs_fs.h ../../include/linuxmt/kernel.h \
 ../../include/linuxmt/string.h ../../include/arch/string.h \
 ../../include/linuxmt/stat.h ../../include/arch/stat.h \
 ../../include/linuxmt/fcntl.h ../../include/linuxmt/errno.h \
 ../../include/arch/errno.h ../../include/linuxmt/debug.h \
 ../../include/arch/segment.h
symlink.s symlink.o: symlink.c ../../include/linuxmt/types.h \
 ../../include/arch/types.h ../../include/linuxmt/posix_types.h \
 ../../include/linuxmt/config.h ../../include/linuxmt/autoconf.h \
 ../../include/arch/posix_types.h ../../include/arch/irq.h \
 ../../include/linuxmt/errno.h ../../include/arch/errno.h \
 ../../include/linuxmt/sched.h ../../include/linuxmt/fs.h \
 ../../include/linuxmt/wait.h ../../include/linuxmt/vfs.h \
 ../../include/arch/statfs.h ../../include/linuxmt/kdev_t.h \
 ../../include/linuxmt/ioctl.h ../../include/arch/ioctl.h \
 ../../include/linuxmt/pipe_fs_i.h ../../include/linuxmt/net.h \
 ../../include/linuxmt/socket.h ../../include/linuxmt/uio.h \
 ../../include/arch/bitops.h ../../include/linuxmt/time.h \
 ../../include/linuxmt/signal.h ../../include/linuxmt/ntty.h \
 ../../include/arch/param.h ../../include/linuxmt/timex.h \
 ../../include/linuxmt/elksfs_fs.h ../../include/linuxmt/stat.h \
 ../../include/arch/stat.h ../../include/arch/segment.h
truncate.s truncate.o: truncate.c ../../include/linuxmt/types.h \
 ../../include/arch/types.h ../../include/linuxmt/posix_types.h \
 ../../include/linuxmt/config.h ../../include/linuxmt/autoconf.h \
 ../../include/arch/posix_types.h ../../include/arch/irq.h \
 ../../include/linuxmt/errno.h ../../include/arch/errno.h \
 ../../include/linuxmt/sched.h ../../include/linuxmt/fs.h \
 ../../include/linuxmt/wait.h ../../include/linuxmt/vfs.h \
 ../../include/arch/statfs.h ../../include/linuxmt/kdev_t.h \
 ../../include/linuxmt/ioctl.h ../../include/arch/ioctl.h \
 ../../include/linuxmt/pipe_fs_i.h ../../include/linuxmt/net.h \
 ../../include/linuxmt/socket.h ../../include/linuxmt/uio.h \
 ../../include/arch/bitops.h ../../include/linuxmt/time.h \
 ../../include/linuxmt/signal.h ../../include/linuxmt/ntty.h \
 ../../include/arch/param.h ../../include/linuxmt/timex.h \
 ../../include/linuxmt/elksfs_fs.h ../../include/linuxmt/stat.h \
 ../../include/arch/stat.h ../../include/linuxmt/fcntl.h

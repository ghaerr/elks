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

BASEDIR 	= ..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC  	= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	= 

NOINDENT	= 

#########################################################################
# Include the standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS  = super.o bufops.o devices.o fcntl.o stat.o inode.o file_table.o \
	buffer.o block_dev.o namei.o ioctl.o filesystems.o open.o \
	read_write.o readdir.o exec.o select.o pipe.o

#########################################################################
# Commands.

all:	fs.a

fs.a:	$(OBJS)
	ar rcs fs.a $(OBJS)

#########################################################################
# Standard commands.

distdir:
	cp -pf Config.in Makefile *.c $(DISTDIR)/fs

#########################################################################
### Dependencies:

block_dev.o: block_dev.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/wait.h /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/ioctl.h \
 /X/elks/include/arch/ioctl.h /X/elks/include/linuxmt/pipe_fs_i.h \
 /X/elks/include/linuxmt/net.h /X/elks/include/linuxmt/socket.h \
 /X/elks/include/linuxmt/uio.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/fcntl.h /X/elks/include/linuxmt/locks.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/arch/segment.h


buffer.o: buffer.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/vfs.h /X/elks/include/arch/statfs.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/locks.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/arch/segment.h /X/elks/include/arch/io.h


bufops.o: bufops.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/vfs.h /X/elks/include/arch/statfs.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h


devices.o: devices.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/wait.h /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/ioctl.h \
 /X/elks/include/arch/ioctl.h /X/elks/include/linuxmt/pipe_fs_i.h \
 /X/elks/include/linuxmt/net.h /X/elks/include/linuxmt/socket.h \
 /X/elks/include/linuxmt/uio.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/stat.h \
 /X/elks/include/arch/stat.h /X/elks/include/linuxmt/fcntl.h \
 /X/elks/include/linuxmt/errno.h


exec.o: exec.c /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/fcntl.h /X/elks/include/linuxmt/stat.h \
 /X/elks/include/arch/stat.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/minix.h \
 /X/elks/include/linuxmt/dll.h /X/elks/include/linuxmt/msdos.h \
 /X/elks/include/linuxmt/init.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/arch/segment.h


fcntl.o: fcntl.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/arch/segment.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/wait.h /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/ioctl.h \
 /X/elks/include/arch/ioctl.h /X/elks/include/linuxmt/pipe_fs_i.h \
 /X/elks/include/linuxmt/net.h /X/elks/include/linuxmt/socket.h \
 /X/elks/include/linuxmt/uio.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/stat.h \
 /X/elks/include/arch/stat.h /X/elks/include/linuxmt/fcntl.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h


file_table.o: file_table.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/wait.h /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/ioctl.h \
 /X/elks/include/arch/ioctl.h /X/elks/include/linuxmt/pipe_fs_i.h \
 /X/elks/include/linuxmt/net.h /X/elks/include/linuxmt/socket.h \
 /X/elks/include/linuxmt/uio.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/time.h /X/elks/include/linuxmt/signal.h \
 /X/elks/include/linuxmt/ntty.h /X/elks/include/linuxmt/termios.h \
 /X/elks/include/linuxmt/chqueue.h /X/elks/include/arch/system.h \
 /X/elks/include/linuxmt/timex.h /X/elks/include/arch/param.h \
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/kernel.h


filesystems.o: filesystems.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/wait.h /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/ioctl.h \
 /X/elks/include/arch/ioctl.h /X/elks/include/linuxmt/pipe_fs_i.h \
 /X/elks/include/linuxmt/net.h /X/elks/include/linuxmt/socket.h \
 /X/elks/include/linuxmt/uio.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h \
 /X/elks/include/linuxmt/minix_fs.h /X/elks/include/linuxmt/romfs_fs.h \
 /X/elks/include/linuxmt/elksfs_fs.h


inode.o: inode.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/stat.h \
 /X/elks/include/arch/stat.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/vfs.h /X/elks/include/arch/statfs.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/linuxmt/debug.h


ioctl.o: ioctl.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/ioctl.h \
 /X/elks/include/arch/ioctl.h /X/elks/include/arch/segment.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/wait.h /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/pipe_fs_i.h \
 /X/elks/include/linuxmt/net.h /X/elks/include/linuxmt/socket.h \
 /X/elks/include/linuxmt/uio.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/mm.h \
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/linuxmt/stat.h /X/elks/include/arch/stat.h \
 /X/elks/include/linuxmt/fcntl.h


namei.o: namei.c /X/elks/include/arch/segment.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/config.h /X/elks/include/linuxmt/autoconf.h \
 /X/elks/include/linuxmt/kdev_t.h /X/elks/include/linuxmt/major.h \
 /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/arch/system.h \
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/vfs.h /X/elks/include/arch/statfs.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/linuxmt/timex.h /X/elks/include/arch/param.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/linuxmt/fcntl.h \
 /X/elks/include/linuxmt/stat.h /X/elks/include/arch/stat.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/debug.h


open.o: open.c /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/fcntl.h /X/elks/include/linuxmt/stat.h \
 /X/elks/include/arch/stat.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/utime.h \
 /X/elks/include/arch/segment.h


pipe.o: pipe.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/vfs.h /X/elks/include/arch/statfs.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/stat.h /X/elks/include/arch/stat.h \
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/fcntl.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/linuxmt/locks.h /X/elks/include/linuxmt/mm.h \
 /X/elks/include/linuxmt/debug.h /X/elks/include/arch/segment.h


read_write.o: read_write.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/stat.h /X/elks/include/arch/stat.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/vfs.h /X/elks/include/arch/statfs.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/fcntl.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/arch/segment.h \
 /X/elks/include/linuxmt/debug.h


readdir.o: readdir.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/stat.h /X/elks/include/arch/stat.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/vfs.h /X/elks/include/arch/statfs.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/mm.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/linuxmt/debug.h /X/elks/include/arch/segment.h


select.o: select.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/vfs.h /X/elks/include/arch/statfs.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/mm.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/linuxmt/stat.h \
 /X/elks/include/arch/stat.h /X/elks/include/arch/segment.h


stat.o: stat.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/linuxmt/stat.h /X/elks/include/arch/stat.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/vfs.h /X/elks/include/arch/statfs.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/arch/segment.h


super.o: super.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/vfs.h /X/elks/include/arch/statfs.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/stat.h /X/elks/include/arch/stat.h \
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/linuxmt/locks.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/arch/segment.h



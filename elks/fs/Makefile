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

block_dev.o: block_dev.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/locks.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


buffer.o: buffer.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/locks.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h \
 /home/rhw/CVS/Source/X/elks/include/arch/io.h


bufops.o: bufops.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h


devices.o: devices.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h


exec.o: exec.c /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/dll.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/msdos.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/init.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


fcntl.o: fcntl.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h


file_table.o: file_table.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h


filesystems.o: filesystems.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/major.h


inode.o: inode.c /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h


ioctl.o: ioctl.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fcntl.h


namei.o: namei.c /home/rhw/CVS/Source/X/elks/include/arch/segment.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h


open.o: open.c /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/utime.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


pipe.o: pipe.c /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/locks.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


read_write.o: read_write.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h


readdir.o: readdir.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


select.o: select.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


stat.o: stat.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


super.o: super.c /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/locks.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h



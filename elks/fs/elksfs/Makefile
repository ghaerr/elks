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

BASEDIR 	= ../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

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

OBJS  = bitmap.o dir.o file.o namei.o symlink.o truncate.o inode.o

#########################################################################
# Commands.

all:	elksfs.a

elksfs.a:	$(OBJS)
	ar rcs elksfs.a $(OBJS)

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile *.c $(DISTDIR)/fs/elksfs

#########################################################################
### Dependencies:

bitmap.o: bitmap.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/string.h


dir.o: dir.c /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/segment.h


file.o: file.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/segment.h


inode.o: inode.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/segment.h


namei.o: namei.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/segment.h


symlink.o: symlink.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/segment.h


truncate.o: truncate.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/vfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/statfs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kdev_t.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/ioctl.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/pipe_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/net.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/socket.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/uio.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/bitops.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_i.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/minix_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs_sb.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/elksfs_fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fcntl.h



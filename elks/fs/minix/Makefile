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
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS  = bitmap.o dir.o file.o namei.o symlink.o truncate.o inode.o

#########################################################################
# Commands.

all:	minixfs.a

minixfs.a: $(OBJS)
	ar rcs minixfs.a $(OBJS)

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile *.c $(DISTDIR)/fs/minix

#########################################################################
### Dependencies:

bitmap.o: bitmap.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h


dir.o: dir.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


file.o: file.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


inode.o: inode.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/locks.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


namei.o: namei.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


symlink.o: symlink.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
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
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


truncate.o: truncate.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
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
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/minix_fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fcntl.h



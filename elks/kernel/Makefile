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

USEBCC 		= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	= 

NOINDENT	= memdumpk.c stubs.c

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to compile.

OBJS  = sched.o printk.o sleepwake.o dma.o version.o sys.o stubs.o fork.o \
	exit.o time.o signal.o

#########################################################################
# Commands:

all:	kernel.a

kernel.a: $(OBJS)
	ar rcs kernel.a $(OBJS)

version.o: version.c $(BASEDIR)/include/linuxmt/compiler-generated.h

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile *.c $(DISTDIR)/kernel

#########################################################################
### Dependencies:

dma.o: dma.c /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h


exit.o: exit.c /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h


fork.o: fork.c /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h


memdumpk.o: memdumpk.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h


printk.o: printk.c /home/rhw/CVS/Source/X/elks/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h


sched.o: sched.c /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timer.h


signal.o: signal.c /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


sleepwake.o: sleepwake.c \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h


stubs.o: stubs.c


sys.o: sys.c /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/utsname.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h \
 /home/rhw/CVS/Source/X/elks/include/arch/io.h


syscall.o: syscall.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h


time.o: time.c /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


version.o: version.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/compiler-generated.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/utsname.h



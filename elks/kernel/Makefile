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

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile *.c $(DISTDIR)/kernel

#########################################################################
### Dependencies:

dma.o: dma.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h


exit.o: exit.c /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/config.h /X/elks/include/linuxmt/autoconf.h \
 /X/elks/include/linuxmt/kdev_t.h /X/elks/include/linuxmt/major.h \
 /X/elks/include/linuxmt/posix_types.h \
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
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h


fork.o: fork.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/mm.h \
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
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h


memdumpk.o: memdumpk.c /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/config.h /X/elks/include/linuxmt/autoconf.h \
 /X/elks/include/linuxmt/kdev_t.h /X/elks/include/linuxmt/major.h \
 /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/mm.h \
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
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h


printk.o: printk.c /X/elks/include/linuxmt/fcntl.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/config.h /X/elks/include/linuxmt/autoconf.h \
 /X/elks/include/linuxmt/kdev_t.h /X/elks/include/linuxmt/major.h \
 /X/elks/include/linuxmt/posix_types.h \
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
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/arch/segment.h


sched.o: sched.c /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/config.h /X/elks/include/linuxmt/autoconf.h \
 /X/elks/include/linuxmt/kdev_t.h /X/elks/include/linuxmt/major.h \
 /X/elks/include/linuxmt/posix_types.h \
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
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/timer.h


signal.o: signal.c /X/elks/include/linuxmt/debug.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/config.h /X/elks/include/linuxmt/autoconf.h \
 /X/elks/include/linuxmt/kdev_t.h /X/elks/include/linuxmt/major.h \
 /X/elks/include/linuxmt/posix_types.h \
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
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/mm.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/arch/segment.h


sleepwake.o: sleepwake.c /X/elks/include/arch/irq.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/config.h /X/elks/include/linuxmt/autoconf.h \
 /X/elks/include/linuxmt/kdev_t.h /X/elks/include/linuxmt/major.h \
 /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/asm.h \
 /X/elks/include/arch/segment.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/sched.h \
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
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h


stubs.o: stubs.c


sys.o: sys.c /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/types.h \
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
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/utsname.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/linuxmt/stat.h \
 /X/elks/include/arch/stat.h /X/elks/include/linuxmt/mm.h \
 /X/elks/include/arch/segment.h /X/elks/include/arch/io.h


syscall.o: syscall.c /X/elks/include/linuxmt/errno.h


time.o: time.c /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/time.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/linuxmt/config.h /X/elks/include/linuxmt/autoconf.h \
 /X/elks/include/linuxmt/kdev_t.h /X/elks/include/linuxmt/major.h \
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/mm.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/linuxmt/posix_types.h \
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
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/arch/param.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/arch/segment.h


version.o: version.c /X/elks/include/linuxmt/compiler-generated.h \
 /X/elks/include/linuxmt/config.h /X/elks/include/linuxmt/autoconf.h \
 /X/elks/include/linuxmt/kdev_t.h /X/elks/include/linuxmt/major.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/utsname.h



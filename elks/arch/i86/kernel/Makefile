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

ELKSDIR 	= ../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

ELKSSTD 	= Y

CLEANDEP	= 

CLEANME 	= entry.c

DEPEND		= 

DISTFILES	= syscall.dat

#########################################################################
# Include standard commands.

include $(ELKSDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

ifeq ($(CONFIG_ARCH_SIBO), y)
OBJS  = strace.o printreg.o system.o irq.o ../sibo/irqtab.o process.o \
		entry.o signal.o timer.o
else
OBJS  = strace.o printreg.o system.o irq.o irqtab.o process.o bios16.o \
		entry.o signal.o timer.o
endif

#########################################################################
# Commands.

all:	akernel.a

akernel.a: $(OBJS)
	ar rcs akernel.a $(OBJS)

entry.c: syscall.dat mkentry.sh
	sh mkentry.sh > entry.tmp
	mv entry.tmp entry.c

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile mkentry.sh $(DISTDIR)/arch/i86/kernel
	cp -pf syscall.dat $(DISTDIR)/arch/i86/kernel
	cp -pf *.c $(DISTDIR)/arch/i86/kernel

#########################################################################
### Dependencies:
bios16.o: bios16.c /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h /usr/src/elks/include/arch/segment.h \
 /usr/src/elks/include/linuxmt/biosparm.h

irq.o: irq.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/sched.h \
 /usr/src/elks/include/linuxmt/fs.h \
 /usr/src/elks/include/linuxmt/wait.h \
 /usr/src/elks/include/linuxmt/vfs.h \
 /usr/src/elks/include/arch/statfs.h \
 /usr/src/elks/include/linuxmt/kdev_t.h \
 /usr/src/elks/include/linuxmt/ioctl.h \
 /usr/src/elks/include/arch/ioctl.h \
 /usr/src/elks/include/linuxmt/pipe_fs_i.h \
 /usr/src/elks/include/linuxmt/net.h \
 /usr/src/elks/include/linuxmt/socket.h \
 /usr/src/elks/include/linuxmt/uio.h \
 /usr/src/elks/include/arch/bitops.h \
 /usr/src/elks/include/linuxmt/romfs_fs_i.h \
 /usr/src/elks/include/linuxmt/minix_fs_sb.h \
 /usr/src/elks/include/linuxmt/romfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/elksfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/time.h \
 /usr/src/elks/include/linuxmt/signal.h \
 /usr/src/elks/include/linuxmt/ntty.h \
 /usr/src/elks/include/linuxmt/termios.h \
 /usr/src/elks/include/linuxmt/chqueue.h \
 /usr/src/elks/include/arch/system.h \
 /usr/src/elks/include/arch/param.h \
 /usr/src/elks/include/linuxmt/timex.h \
 /usr/src/elks/include/linuxmt/timer.h \
 /usr/src/elks/include/linuxmt/hdreg.h /usr/src/elks/include/arch/io.h \
 /usr/src/elks/include/arch/keyboard.h

irqtab.o: irqtab.c /usr/src/elks/include/arch/irq.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h

printreg.o: printreg.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h

process.o: process.c /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h \
 /usr/src/elks/include/linuxmt/debug.h \
 /usr/src/elks/include/linuxmt/sched.h \
 /usr/src/elks/include/linuxmt/fs.h \
 /usr/src/elks/include/linuxmt/wait.h \
 /usr/src/elks/include/linuxmt/vfs.h \
 /usr/src/elks/include/arch/statfs.h \
 /usr/src/elks/include/linuxmt/kdev_t.h \
 /usr/src/elks/include/linuxmt/ioctl.h \
 /usr/src/elks/include/arch/ioctl.h \
 /usr/src/elks/include/linuxmt/pipe_fs_i.h \
 /usr/src/elks/include/linuxmt/net.h \
 /usr/src/elks/include/linuxmt/socket.h \
 /usr/src/elks/include/linuxmt/uio.h \
 /usr/src/elks/include/arch/bitops.h \
 /usr/src/elks/include/linuxmt/romfs_fs_i.h \
 /usr/src/elks/include/linuxmt/minix_fs_sb.h \
 /usr/src/elks/include/linuxmt/romfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/elksfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/time.h \
 /usr/src/elks/include/linuxmt/signal.h \
 /usr/src/elks/include/linuxmt/ntty.h \
 /usr/src/elks/include/linuxmt/termios.h \
 /usr/src/elks/include/linuxmt/chqueue.h \
 /usr/src/elks/include/arch/system.h \
 /usr/src/elks/include/arch/param.h \
 /usr/src/elks/include/linuxmt/timex.h

signal.o: signal.c /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h \
 /usr/src/elks/include/linuxmt/sched.h \
 /usr/src/elks/include/linuxmt/fs.h \
 /usr/src/elks/include/linuxmt/wait.h \
 /usr/src/elks/include/linuxmt/vfs.h \
 /usr/src/elks/include/arch/statfs.h \
 /usr/src/elks/include/linuxmt/kdev_t.h \
 /usr/src/elks/include/linuxmt/ioctl.h \
 /usr/src/elks/include/arch/ioctl.h \
 /usr/src/elks/include/linuxmt/pipe_fs_i.h \
 /usr/src/elks/include/linuxmt/net.h \
 /usr/src/elks/include/linuxmt/socket.h \
 /usr/src/elks/include/linuxmt/uio.h \
 /usr/src/elks/include/arch/bitops.h \
 /usr/src/elks/include/linuxmt/romfs_fs_i.h \
 /usr/src/elks/include/linuxmt/minix_fs_sb.h \
 /usr/src/elks/include/linuxmt/romfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/elksfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/time.h \
 /usr/src/elks/include/linuxmt/signal.h \
 /usr/src/elks/include/linuxmt/ntty.h \
 /usr/src/elks/include/linuxmt/termios.h \
 /usr/src/elks/include/linuxmt/chqueue.h \
 /usr/src/elks/include/arch/system.h \
 /usr/src/elks/include/arch/param.h \
 /usr/src/elks/include/linuxmt/timex.h \
 /usr/src/elks/include/linuxmt/mm.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/kernel.h \
 /usr/src/elks/include/linuxmt/string.h \
 /usr/src/elks/include/arch/string.h \
 /usr/src/elks/include/linuxmt/debug.h \
 /usr/src/elks/include/arch/segment.h

strace.o: strace.c /usr/src/elks/include/linuxmt/autoconf.h

system.o: system.c /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h \
 /usr/src/elks/include/linuxmt/kernel.h \
 /usr/src/elks/include/linuxmt/wait.h \
 /usr/src/elks/include/linuxmt/sched.h \
 /usr/src/elks/include/linuxmt/fs.h \
 /usr/src/elks/include/linuxmt/vfs.h \
 /usr/src/elks/include/arch/statfs.h \
 /usr/src/elks/include/linuxmt/kdev_t.h \
 /usr/src/elks/include/linuxmt/ioctl.h \
 /usr/src/elks/include/arch/ioctl.h \
 /usr/src/elks/include/linuxmt/pipe_fs_i.h \
 /usr/src/elks/include/linuxmt/net.h \
 /usr/src/elks/include/linuxmt/socket.h \
 /usr/src/elks/include/linuxmt/uio.h \
 /usr/src/elks/include/arch/bitops.h \
 /usr/src/elks/include/linuxmt/romfs_fs_i.h \
 /usr/src/elks/include/linuxmt/minix_fs_sb.h \
 /usr/src/elks/include/linuxmt/romfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/elksfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/time.h \
 /usr/src/elks/include/linuxmt/signal.h \
 /usr/src/elks/include/linuxmt/ntty.h \
 /usr/src/elks/include/linuxmt/termios.h \
 /usr/src/elks/include/linuxmt/chqueue.h \
 /usr/src/elks/include/arch/system.h \
 /usr/src/elks/include/arch/param.h \
 /usr/src/elks/include/linuxmt/timex.h \
 /usr/src/elks/include/arch/segment.h

timer.o: timer.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/linuxmt/timer.h \
 /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h


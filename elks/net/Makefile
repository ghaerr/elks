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

ELKSDIR 	= ..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

ELKSSTD 	= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND		= 

DISTFILES	= 

#########################################################################
# Include standard commands.

include $(ELKSDIR)/Makefile-rules

#########################################################################
# Objects to compile.

OBJS  = socket.o protocols.o ipv4/af_inet.o nano/af_nano.o unix/af_unix.o

#########################################################################
# Rules.

all:	net.a

net.a: $(OBJS)
	ar rcs net.a $(OBJS)

#########################################################################
# Standard commands.

distdir:
	cp -pf Config.in Makefile *.c $(DISTDIR)/net
	mkdir $(DISTDIR)/net/ipv4
	cp -pf ipv4/Makefile ipv4/*.c ipv4/*.h $(DISTDIR)/net/ipv4
	mkdir $(DISTDIR)/net/nano
	cp -pf nano/Makefile nano/*.c nano/*.h $(DISTDIR)/net/nano
	mkdir $(DISTDIR)/net/unix
	cp -pf unix/Makefile unix/*.c unix/*.h $(DISTDIR)/net/unix

#########################################################################
### Dependencies:
protocols.o: protocols.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h /usr/src/elks/include/linuxmt/net.h \
 /usr/src/elks/include/linuxmt/wait.h \
 /usr/src/elks/include/linuxmt/socket.h \
 /usr/src/elks/include/linuxmt/uio.h \
 /usr/src/elks/include/linuxmt/fs.h \
 /usr/src/elks/include/linuxmt/vfs.h \
 /usr/src/elks/include/arch/statfs.h \
 /usr/src/elks/include/linuxmt/kdev_t.h \
 /usr/src/elks/include/linuxmt/ioctl.h \
 /usr/src/elks/include/arch/ioctl.h \
 /usr/src/elks/include/linuxmt/pipe_fs_i.h \
 /usr/src/elks/include/arch/bitops.h \
 /usr/src/elks/include/linuxmt/romfs_fs_i.h \
 /usr/src/elks/include/linuxmt/minix_fs_sb.h \
 /usr/src/elks/include/linuxmt/romfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/elksfs_fs_sb.h

socket.o: socket.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/socket.h \
 /usr/src/elks/include/linuxmt/uio.h \
 /usr/src/elks/include/linuxmt/net.h \
 /usr/src/elks/include/linuxmt/wait.h \
 /usr/src/elks/include/linuxmt/fs.h \
 /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h /usr/src/elks/include/linuxmt/vfs.h \
 /usr/src/elks/include/arch/statfs.h \
 /usr/src/elks/include/linuxmt/kdev_t.h \
 /usr/src/elks/include/linuxmt/ioctl.h \
 /usr/src/elks/include/arch/ioctl.h \
 /usr/src/elks/include/linuxmt/pipe_fs_i.h \
 /usr/src/elks/include/arch/bitops.h \
 /usr/src/elks/include/linuxmt/romfs_fs_i.h \
 /usr/src/elks/include/linuxmt/minix_fs_sb.h \
 /usr/src/elks/include/linuxmt/romfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/elksfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/mm.h \
 /usr/src/elks/include/linuxmt/sched.h \
 /usr/src/elks/include/linuxmt/time.h \
 /usr/src/elks/include/linuxmt/signal.h \
 /usr/src/elks/include/linuxmt/ntty.h \
 /usr/src/elks/include/linuxmt/termios.h \
 /usr/src/elks/include/linuxmt/chqueue.h \
 /usr/src/elks/include/arch/system.h \
 /usr/src/elks/include/arch/param.h \
 /usr/src/elks/include/linuxmt/timex.h \
 /usr/src/elks/include/linuxmt/kernel.h \
 /usr/src/elks/include/linuxmt/string.h \
 /usr/src/elks/include/arch/string.h \
 /usr/src/elks/include/linuxmt/fcntl.h \
 /usr/src/elks/include/linuxmt/stat.h \
 /usr/src/elks/include/arch/stat.h \
 /usr/src/elks/include/arch/segment.h


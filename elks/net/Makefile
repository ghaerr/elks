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

NOINDENT	= 

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

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

protocols.o: protocols.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/wait.h /X/elks/include/linuxmt/socket.h \
 /X/elks/include/linuxmt/uio.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/vfs.h /X/elks/include/arch/statfs.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h


socket.o: socket.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/linuxmt/net.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/ioctl.h \
 /X/elks/include/arch/ioctl.h /X/elks/include/linuxmt/pipe_fs_i.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/mm.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/linuxmt/fcntl.h /X/elks/include/linuxmt/stat.h \
 /X/elks/include/arch/stat.h /X/elks/include/arch/segment.h



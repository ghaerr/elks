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

BASEDIR 	= ../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	= 

NOINDENT	= segment.c user.c

#########################################################################
# Include the standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS  = init.o segment.o malloc.o user.o

#########################################################################
# Commands.

all:	mm.a

mm.a: $(OBJS)
	ar rcs mm.a $(OBJS)
	sync

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile *.c $(DISTDIR)/arch/i86/mm

#########################################################################
### Dependencies:

init.o: init.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


malloc.o: malloc.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mem.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h


segment.o: segment.c \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h


user.o: user.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
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
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h



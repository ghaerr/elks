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

OBJS  = inode.o

#########################################################################
# Commands.

all:	romfs.a

romfs.a: $(OBJS)
	ar rcs romfs.a $(OBJS)

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile *.c $(DISTDIR)/fs/romfs

#########################################################################
### Dependencies:

inode.o: inode.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/romfs_fs.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/locks.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/stat.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/border.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/debug.h



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

ELKSDIR 	= ../..

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
inode.o: inode.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/romfs_fs.h \
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
 /usr/src/elks/include/linuxmt/locks.h \
 /usr/src/elks/include/linuxmt/stat.h \
 /usr/src/elks/include/arch/stat.h /usr/src/elks/include/arch/border.h \
 /usr/src/elks/include/linuxmt/debug.h


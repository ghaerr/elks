# Makefile for the ELKS blocks devices.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
# Relative path to base directory.

BASEDIR 	= ../../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	= 

NOINDENT	= directhd.c

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS  = genhd.o doshd.o ll_rw_blk.o rd.o floppy.o directhd.o init.o \
	sibo_ssd.o ssd_asm.o

#########################################################################
# Commands.

all:	blk_drv.a

blk_drv.a: $(OBJS)
	ar rcs blk_drv.a $(OBJS)

ssd_asm.o: ssd_asm.s

ssd_asm.s: ssd_asm.S

#########################################################################
# Standard commands.

distdir:
	cp -pf Config.in Makefile $(DISTDIR)/arch/i86/drivers/block
	cp -pf *.S *.h *.c $(DISTDIR)/arch/i86/drivers/block

#########################################################################
### Dependencies:

directhd.o: directhd.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/hdreg.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/genhd.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
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
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/directhd.h


doshd.o: doshd.c /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/genhd.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/hdreg.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/biosparm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/bioshd.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h blk.h


floppy.o: floppy.c /home/rhw/CVS/Source/X/elks/include/linuxmt/sched.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fdreg.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fd.h \
 /home/rhw/CVS/Source/X/elks/include/arch/dma.h \
 /home/rhw/CVS/Source/X/elks/include/arch/io.h \
 /home/rhw/CVS/Source/X/elks/include/arch/segment.h


genhd.o: genhd.c /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/genhd.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h


init.o: init.c /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/genhd.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/system.h


ll_rw_blk.o: ll_rw_blk.c \
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
 /home/rhw/CVS/Source/X/elks/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks/include/arch/io.h blk.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/genhd.h


rd.o: rd.c /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/rd.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h


sibo_ssd.o: sibo_ssd.c \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h


ssd.o: ssd.c /home/rhw/CVS/Source/X/elks/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks/include/arch/asm.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks/include/linuxmt/errno.h



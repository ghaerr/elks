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

directhd.o: directhd.c /X/elks/include/linuxmt/hdreg.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/genhd.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/config.h /X/elks/include/linuxmt/autoconf.h \
 /X/elks/include/linuxmt/kdev_t.h \
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
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/time.h /X/elks/include/linuxmt/signal.h \
 /X/elks/include/linuxmt/ntty.h /X/elks/include/linuxmt/termios.h \
 /X/elks/include/linuxmt/chqueue.h /X/elks/include/arch/system.h \
 /X/elks/include/linuxmt/timex.h /X/elks/include/arch/param.h \
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/directhd.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/arch/segment.h


doshd.o: doshd.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/kernel.h \
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
 /X/elks/include/linuxmt/genhd.h /X/elks/include/linuxmt/hdreg.h \
 /X/elks/include/linuxmt/biosparm.h /X/elks/include/linuxmt/bioshd.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/arch/segment.h blk.h /X/elks/include/linuxmt/locks.h


floppy.o: floppy.c /X/elks/include/linuxmt/sched.h \
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
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/linuxmt/fdreg.h /X/elks/include/linuxmt/fd.h \
 /X/elks/include/arch/dma.h /X/elks/include/arch/io.h \
 /X/elks/include/arch/segment.h


genhd.o: genhd.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/vfs.h /X/elks/include/arch/statfs.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h \
 /X/elks/include/linuxmt/genhd.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/string.h /X/elks/include/arch/string.h \
 /X/elks/include/arch/system.h


init.o: init.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/vfs.h /X/elks/include/arch/statfs.h \
 /X/elks/include/linuxmt/ioctl.h /X/elks/include/arch/ioctl.h \
 /X/elks/include/linuxmt/pipe_fs_i.h /X/elks/include/linuxmt/net.h \
 /X/elks/include/linuxmt/socket.h /X/elks/include/linuxmt/uio.h \
 /X/elks/include/arch/bitops.h /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h \
 /X/elks/include/linuxmt/genhd.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/init.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/arch/system.h


ll_rw_blk.o: ll_rw_blk.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
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
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/linuxmt/init.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/arch/io.h blk.h /X/elks/include/linuxmt/locks.h \
 /X/elks/include/linuxmt/genhd.h


rd.o: rd.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/rd.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/linuxmt/errno.h


sibo_ssd.o: sibo_ssd.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/linuxmt/errno.h


ssd.o: ssd.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/linuxmt/errno.h



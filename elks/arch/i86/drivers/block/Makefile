# Makefile for the ELKS blocks devices.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
# Relative path to base directory.

ELKSDIR 	= ../../../..

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
directhd.o: directhd.c /usr/src/elks/include/linuxmt/hdreg.h \
 /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/genhd.h \
 /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h /usr/src/elks/include/linuxmt/fs.h \
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
 /usr/src/elks/include/linuxmt/string.h \
 /usr/src/elks/include/arch/string.h \
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
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/kernel.h \
 /usr/src/elks/include/arch/segment.h \
 /usr/src/elks/include/linuxmt/directhd.h

doshd.o: doshd.c /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h \
 /usr/src/elks/include/linuxmt/kernel.h \
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
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/genhd.h \
 /usr/src/elks/include/linuxmt/hdreg.h \
 /usr/src/elks/include/linuxmt/biosparm.h \
 /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/bioshd.h \
 /usr/src/elks/include/linuxmt/string.h \
 /usr/src/elks/include/arch/string.h \
 /usr/src/elks/include/linuxmt/mm.h \
 /usr/src/elks/include/arch/segment.h blk.h

floppy.o: floppy.c /usr/src/elks/include/linuxmt/sched.h \
 /usr/src/elks/include/linuxmt/fs.h \
 /usr/src/elks/include/linuxmt/wait.h \
 /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h /usr/src/elks/include/linuxmt/vfs.h \
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
 /usr/src/elks/include/linuxmt/kernel.h \
 /usr/src/elks/include/linuxmt/mm.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/string.h \
 /usr/src/elks/include/arch/string.h \
 /usr/src/elks/include/linuxmt/fdreg.h \
 /usr/src/elks/include/linuxmt/fd.h /usr/src/elks/include/arch/dma.h \
 /usr/src/elks/include/arch/io.h /usr/src/elks/include/arch/segment.h

genhd.o: genhd.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/linuxmt/fs.h \
 /usr/src/elks/include/linuxmt/wait.h \
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
 /usr/src/elks/include/linuxmt/net.h \
 /usr/src/elks/include/linuxmt/socket.h \
 /usr/src/elks/include/linuxmt/uio.h \
 /usr/src/elks/include/arch/bitops.h \
 /usr/src/elks/include/linuxmt/romfs_fs_i.h \
 /usr/src/elks/include/linuxmt/minix_fs_sb.h \
 /usr/src/elks/include/linuxmt/romfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/elksfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/genhd.h \
 /usr/src/elks/include/linuxmt/kernel.h \
 /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/string.h \
 /usr/src/elks/include/arch/string.h \
 /usr/src/elks/include/arch/system.h

init.o: init.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/linuxmt/fs.h \
 /usr/src/elks/include/linuxmt/wait.h \
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
 /usr/src/elks/include/linuxmt/net.h \
 /usr/src/elks/include/linuxmt/socket.h \
 /usr/src/elks/include/linuxmt/uio.h \
 /usr/src/elks/include/arch/bitops.h \
 /usr/src/elks/include/linuxmt/romfs_fs_i.h \
 /usr/src/elks/include/linuxmt/minix_fs_sb.h \
 /usr/src/elks/include/linuxmt/romfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/elksfs_fs_sb.h \
 /usr/src/elks/include/linuxmt/genhd.h \
 /usr/src/elks/include/linuxmt/kernel.h \
 /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/string.h \
 /usr/src/elks/include/arch/string.h \
 /usr/src/elks/include/arch/system.h

ll_rw_blk.o: ll_rw_blk.c /usr/src/elks/include/linuxmt/types.h \
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
 /usr/src/elks/include/linuxmt/kernel.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/string.h \
 /usr/src/elks/include/arch/string.h \
 /usr/src/elks/include/linuxmt/mm.h /usr/src/elks/include/arch/io.h \
 blk.h /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/genhd.h

rd.o: rd.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/linuxmt/rd.h \
 /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/kernel.h \
 /usr/src/elks/include/linuxmt/debug.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h

sibo_ssd.o: sibo_ssd.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/kernel.h \
 /usr/src/elks/include/linuxmt/debug.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h

ssd.o: ssd.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/kernel.h \
 /usr/src/elks/include/linuxmt/debug.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h


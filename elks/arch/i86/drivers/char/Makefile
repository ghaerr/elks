# Makefile for the ELKS character devices.
#
#########################################################################
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
# Relative path to the base directory.

ELKSDIR 	= ../../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

ELKSSTD 	= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND		= 

DISTFILES	= CGAsnow Changes NEW_NOTES NOTES

#########################################################################
# Include standard commands.

include $(ELKSDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

ifeq ($(CONFIG_ARCH_SIBO), y)
OBJS  = init.o con_asm.o font.o sibo_key.o key_asm.o sibo_con.o \
	mem.o meta.o ntty.o ser_asm.o pty.o
else
OBJS  = bioscon.o serial.o lp.o xt_key.o init.o dircon.o ntty.o \
	mem.o meta.o tcpdev.o pty.o bell.o # clist.o tty.o
endif

#########################################################################
# Commands.

all:	chr_drv.a

chr_drv.a: $(OBJS)
	ar rcs chr_drv.a $(OBJS)
	sync

con_asm.s: con_asm.S

con_asm.o: con_asm.s

font.s: font.S

font.o: font.s

key_asm.s: key_asm.S

key_asm.o: key_asm.s

ser_asm.s: ser_asm.S

ser_asm.o: ser_asm.s

#########################################################################
# Standard commands.

distdir:
	mkdir $(DISTDIR)/arch/i86/drivers/char/KeyMaps
	make -C KeyMaps distdir
	cp -pf CGAsnow Changes NEW_NOTES $(DISTDIR)/arch/i86/drivers/char
	cp -pf NOTES README_CONSOLE $(DISTDIR)/arch/i86/drivers/char
	cp -pf Makefile Config.in $(DISTDIR)/arch/i86/drivers/char
	cp -pf *.S *.h *.c $(DISTDIR)/arch/i86/drivers/char

#########################################################################
### Dependencies:
bell.o: bell.c

bioscon.o: bioscon.c /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/fcntl.h \
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
 /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/sched.h \
 /usr/src/elks/include/linuxmt/time.h \
 /usr/src/elks/include/linuxmt/signal.h \
 /usr/src/elks/include/linuxmt/ntty.h \
 /usr/src/elks/include/linuxmt/termios.h \
 /usr/src/elks/include/linuxmt/chqueue.h \
 /usr/src/elks/include/arch/system.h \
 /usr/src/elks/include/arch/param.h \
 /usr/src/elks/include/linuxmt/timex.h

console.o: console.c /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/fcntl.h \
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
 /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/sched.h \
 /usr/src/elks/include/linuxmt/time.h \
 /usr/src/elks/include/linuxmt/signal.h \
 /usr/src/elks/include/linuxmt/ntty.h \
 /usr/src/elks/include/linuxmt/termios.h \
 /usr/src/elks/include/linuxmt/chqueue.h \
 /usr/src/elks/include/arch/system.h \
 /usr/src/elks/include/arch/param.h \
 /usr/src/elks/include/linuxmt/timex.h console.h

dircon.o: dircon.c /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/fcntl.h \
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
 /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/sched.h \
 /usr/src/elks/include/linuxmt/time.h \
 /usr/src/elks/include/linuxmt/signal.h \
 /usr/src/elks/include/linuxmt/ntty.h \
 /usr/src/elks/include/linuxmt/termios.h \
 /usr/src/elks/include/linuxmt/chqueue.h \
 /usr/src/elks/include/arch/system.h \
 /usr/src/elks/include/arch/param.h \
 /usr/src/elks/include/linuxmt/timex.h

init.o: init.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h

keyboard.o: keyboard.c /usr/src/elks/include/linuxmt/sched.h \
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
 /usr/src/elks/include/linuxmt/timex.h /usr/src/elks/include/arch/io.h \
 /usr/src/elks/include/arch/keyboard.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/fcntl.h console.h

lp.o: lp.c /usr/src/elks/include/linuxmt/types.h \
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
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/lp.h

mem.o: mem.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/linuxmt/major.h \
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
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h /usr/src/elks/include/linuxmt/mm.h \
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
 /usr/src/elks/include/linuxmt/debug.h \
 /usr/src/elks/include/linuxmt/mem.h \
 /usr/src/elks/include/arch/segment.h

meta.o: meta.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h

ntty.o: ntty.c /usr/src/elks/include/linuxmt/types.h \
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
 /usr/src/elks/include/linuxmt/fcntl.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h /usr/src/elks/include/linuxmt/mm.h \
 /usr/src/elks/include/linuxmt/kernel.h \
 /usr/src/elks/include/linuxmt/string.h \
 /usr/src/elks/include/arch/string.h \
 /usr/src/elks/include/linuxmt/major.h

pty.o: pty.c /usr/src/elks/include/linuxmt/types.h \
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
 /usr/src/elks/include/linuxmt/fcntl.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h /usr/src/elks/include/linuxmt/mm.h \
 /usr/src/elks/include/linuxmt/kernel.h \
 /usr/src/elks/include/linuxmt/string.h \
 /usr/src/elks/include/arch/string.h \
 /usr/src/elks/include/linuxmt/major.h

serial.o: serial.c /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h /usr/src/elks/include/linuxmt/wait.h \
 /usr/src/elks/include/linuxmt/chqueue.h \
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
 /usr/src/elks/include/arch/system.h \
 /usr/src/elks/include/arch/param.h \
 /usr/src/elks/include/linuxmt/timex.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/serial_reg.h \
 /usr/src/elks/include/linuxmt/debug.h

sibo_con.o: sibo_con.c /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/fcntl.h \
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
 /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/sched.h \
 /usr/src/elks/include/linuxmt/time.h \
 /usr/src/elks/include/linuxmt/signal.h \
 /usr/src/elks/include/linuxmt/ntty.h \
 /usr/src/elks/include/linuxmt/termios.h \
 /usr/src/elks/include/linuxmt/chqueue.h \
 /usr/src/elks/include/arch/system.h \
 /usr/src/elks/include/arch/param.h \
 /usr/src/elks/include/linuxmt/timex.h

sibo_key.o: sibo_key.c /usr/src/elks/include/linuxmt/sched.h \
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
 /usr/src/elks/include/linuxmt/timex.h /usr/src/elks/include/arch/io.h \
 /usr/src/elks/include/arch/keyboard.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/fcntl.h

tcpdev.o: tcpdev.c /usr/src/elks/include/linuxmt/types.h \
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
 /usr/src/elks/include/linuxmt/debug.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/major.h \
 /usr/src/elks/include/linuxmt/fcntl.h \
 /usr/src/elks/include/linuxmt/tcpdev.h \
 /usr/src/elks/include/linuxmt/in.h

xt_key.o: xt_key.c /usr/src/elks/include/linuxmt/sched.h \
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
 /usr/src/elks/include/linuxmt/timex.h /usr/src/elks/include/arch/io.h \
 /usr/src/elks/include/arch/keyboard.h \
 /usr/src/elks/include/linuxmt/errno.h \
 /usr/src/elks/include/arch/errno.h \
 /usr/src/elks/include/linuxmt/fcntl.h KeyMaps/keys-be.h \
 KeyMaps/keys-de.h KeyMaps/keys-dv.h KeyMaps/keys-es.h \
 KeyMaps/keys-fr.h KeyMaps/keys-se.h KeyMaps/keys-uk.h \
 KeyMaps/keys-us.h


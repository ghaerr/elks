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

BASEDIR 	= ../../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	= CGAsnow Changes NEW_NOTES NOTES

NOINDENT	= bell.c bioscon.c
# console.c sibo_con.c

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

ifeq ($(CONFIG_ARCH_SIBO), y)
OBJS  = init.o common.o con_asm.o font.o sibo_key.o key_asm.o mem.o \
	sibo_con.o meta.o ntty.o ser_asm.o pty.o
else
OBJS  = bioscon.o common.o serial.o lp.o xt_key.o init.o dircon.o mem.o \
	ntty.o meta.o tcpdev.o pty.o bell.o	# clist.o tty.o
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

KeyMaps/keymaps.h:
	make -C KeyMaps keymaps.h

#########################################################################
# Standard commands.

distdir:
	mkdir $(DISTDIR)/arch/i86/drivers/char/KeyMaps
	${MAKE} -C KeyMaps distdir
	cp -pf CGAsnow Changes NEW_NOTES $(DISTDIR)/arch/i86/drivers/char
	cp -pf NOTES README_CONSOLE $(DISTDIR)/arch/i86/drivers/char
	cp -pf Makefile Config.in $(DISTDIR)/arch/i86/drivers/char
	cp -pf *.S *.h *.c $(DISTDIR)/arch/i86/drivers/char

#########################################################################
### Dependencies:

bell.o: bell.c /X/elks/include/arch/io.h


bioscon.o: bioscon.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/fcntl.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/wait.h /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/ioctl.h \
 /X/elks/include/arch/ioctl.h /X/elks/include/linuxmt/pipe_fs_i.h \
 /X/elks/include/linuxmt/net.h /X/elks/include/linuxmt/socket.h \
 /X/elks/include/linuxmt/uio.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h


common.o: common.c common.h


console.o: console.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/fcntl.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/wait.h /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/ioctl.h \
 /X/elks/include/arch/ioctl.h /X/elks/include/linuxmt/pipe_fs_i.h \
 /X/elks/include/linuxmt/net.h /X/elks/include/linuxmt/socket.h \
 /X/elks/include/linuxmt/uio.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/time.h /X/elks/include/linuxmt/signal.h \
 /X/elks/include/linuxmt/ntty.h /X/elks/include/linuxmt/termios.h \
 /X/elks/include/linuxmt/chqueue.h /X/elks/include/arch/system.h \
 /X/elks/include/linuxmt/timex.h /X/elks/include/arch/param.h \
 console.h


dircon.o: dircon.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/fcntl.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/wait.h /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/ioctl.h \
 /X/elks/include/arch/ioctl.h /X/elks/include/linuxmt/pipe_fs_i.h \
 /X/elks/include/linuxmt/net.h /X/elks/include/linuxmt/socket.h \
 /X/elks/include/linuxmt/uio.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/mm.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/arch/io.h \
 /X/elks/include/arch/keyboard.h console.h


init.o: init.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/init.h


keyboard.o: keyboard.c /X/elks/include/linuxmt/sched.h \
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
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/fcntl.h /X/elks/include/arch/io.h \
 /X/elks/include/arch/keyboard.h


lp.o: lp.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/lp.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/sched.h \
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
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/arch/io.h


mem.o: mem.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
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
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/mm.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/linuxmt/mem.h /X/elks/include/arch/segment.h


meta.o: meta.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h


ntty.o: ntty.c /X/elks/include/linuxmt/types.h \
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
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/fcntl.h \
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/mm.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/linuxmt/init.h


pty.o: pty.c /X/elks/include/linuxmt/types.h \
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
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/fcntl.h \
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/mm.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/linuxmt/debug.h


serial.o: serial.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/chqueue.h /X/elks/include/linuxmt/sched.h \
 /X/elks/include/linuxmt/fs.h /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/ioctl.h \
 /X/elks/include/arch/ioctl.h /X/elks/include/linuxmt/pipe_fs_i.h \
 /X/elks/include/linuxmt/net.h /X/elks/include/linuxmt/socket.h \
 /X/elks/include/linuxmt/uio.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/arch/system.h \
 /X/elks/include/linuxmt/timex.h /X/elks/include/arch/param.h \
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/mm.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/linuxmt/serial_reg.h \
 /X/elks/include/linuxmt/debug.h /X/elks/include/arch/io.h


sibo_con.o: sibo_con.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/fcntl.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/wait.h /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/ioctl.h \
 /X/elks/include/arch/ioctl.h /X/elks/include/linuxmt/pipe_fs_i.h \
 /X/elks/include/linuxmt/net.h /X/elks/include/linuxmt/socket.h \
 /X/elks/include/linuxmt/uio.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h


sibo_key.o: sibo_key.c /X/elks/include/linuxmt/sched.h \
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
 /X/elks/include/arch/param.h /X/elks/include/arch/io.h \
 /X/elks/include/arch/keyboard.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/fcntl.h


tcpdev.o: tcpdev.c /X/elks/include/linuxmt/kernel.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/config.h /X/elks/include/linuxmt/autoconf.h \
 /X/elks/include/linuxmt/kdev_t.h /X/elks/include/linuxmt/major.h \
 /X/elks/include/linuxmt/posix_types.h \
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
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/linuxmt/errno.h /X/elks/include/linuxmt/fcntl.h \
 /X/elks/include/linuxmt/mm.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h /X/elks/include/linuxmt/tcpdev.h \
 /X/elks/include/linuxmt/in.h /X/elks/include/arch/segment.h


xt_key.o: xt_key.c /X/elks/include/linuxmt/sched.h \
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
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/fcntl.h /X/elks/include/arch/io.h \
 /X/elks/include/arch/keyboard.h KeyMaps/keymaps.h KeyMaps/keys-be.h \
 KeyMaps/keys-de.h KeyMaps/keys-dv.h KeyMaps/keys-es.h \
 KeyMaps/keys-se.h KeyMaps/keys-uk.h KeyMaps/keys-us.h



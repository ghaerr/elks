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

bell.o: bell.c


bioscon.o: bioscon.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fcntl.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h


common.o: common.c common.h


console.o: console.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fcntl.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h console.h


dircon.o: dircon.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fcntl.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h console.h


init.o: init.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h


keyboard.o: keyboard.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/io.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/keyboard.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fcntl.h console.h


lp.o: lp.c /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/lp.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h


mem.o: mem.c /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/major.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/mem.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/segment.h


meta.o: meta.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h


ntty.o: ntty.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/major.h


pty.o: pty.c /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/mm.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/string.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/major.h


serial.o: serial.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fs.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/serial_reg.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/debug.h


sibo_con.o: sibo_con.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fcntl.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h


sibo_key.o: sibo_key.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/io.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/keyboard.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fcntl.h


tcpdev.o: tcpdev.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/kernel.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/debug.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/major.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fcntl.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/tcpdev.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/in.h


xt_key.o: xt_key.c \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/sched.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fs.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/wait.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h \
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
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/time.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/signal.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/ntty.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/termios.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/chqueue.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/system.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/timex.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/param.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/io.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/keyboard.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/errno.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/fcntl.h \
 KeyMaps/keymaps.h KeyMaps/keys-be.h KeyMaps/keys-de.h \
 KeyMaps/keys-dv.h KeyMaps/keys-es.h KeyMaps/keys-se.h \
 KeyMaps/keys-uk.h KeyMaps/keys-us.h



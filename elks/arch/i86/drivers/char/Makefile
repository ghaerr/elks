#
# Makefile for the ELKS character devices.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

AR	=ar
AS	=as86 
LD	=ld86 
LDFLAGS	=-0 -s -x
CC	=bcc
CFLAGS	=-0 -nostdinc -I../../../../include $(CFLBASE)
CPP	=bcc -E -nostdinc -I../../../../include
CC_PROTO = gcc -I../../../../include -M

.s.o:
	$(AS) -0 -o $*.o $<
.S.s:
	$(CC) $(CFLAGS) -P -o $*.s $<
.c.o:
	$(CC) $(CFLAGS) -0 -c -o $*.o $<

ifeq ($(CONFIG_ARCH_SIBO), y)
OBJS  = init.o con_asm.o font.o sibo_key.o key_asm.o sibo_con.o \
	mem.o meta.o ntty.o ser_asm.o
else
OBJS  = bioscon.o serial.o lp.o xt_key.o init.o dircon.o ntty.o mem.o meta.o \
	tcpdev.o pty.o bell.o # clist.o tty.o
endif

chr_drv.a: $(OBJS)
	ar rcs chr_drv.a $(OBJS)
	sync

clean:
	rm -f core *~ *.o *.a tmp_make
	for i in *.c;do rm -f `basename $$i .c`.s;done
	for i in *.S;do rm -f `basename $$i .S`.s;done

distdir:
	cp -pf CGAsnow Changes NEW_NOTES $(TOPDIR)/$(DISTDIR)/arch/i86/drivers/char
	cp -pf NOTES README_CONSOLE $(TOPDIR)/$(DISTDIR)/arch/i86/drivers/char
	cp -pf Makefile Config.in $(TOPDIR)/$(DISTDIR)/arch/i86/drivers/char
	cp -pf *.S *.h *.c $(TOPDIR)/$(DISTDIR)/arch/i86/drivers/char

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

font.s: font.S

font.o: font.s

key_asm.s: key_asm.S

key_asm.o: key_asm.s

con_asm.s: con_asm.S

con_asm.o: con_asm.s

ser_asm.s: ser_asm.S

ser_asm.o: ser_asm.s


### Dependencies:
bell.s bell.o: bell.c
bioscon.s bioscon.o: bioscon.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/fcntl.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/wait.h ../../../../include/linuxmt/vfs.h \
 ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/major.h \
 ../../../../include/linuxmt/sched.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h \
 ../../../../include/linuxmt/ntty.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h
console.s console.o: console.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/fcntl.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/wait.h ../../../../include/linuxmt/vfs.h \
 ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/major.h \
 ../../../../include/linuxmt/sched.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h \
 ../../../../include/linuxmt/ntty.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/chqueue.h console.h
dircon.s dircon.o: dircon.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/fcntl.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/wait.h ../../../../include/linuxmt/vfs.h \
 ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/major.h \
 ../../../../include/linuxmt/sched.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h \
 ../../../../include/linuxmt/ntty.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/chqueue.h
init.s init.o: init.c ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/linuxmt/types.h ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h
keyboard.s keyboard.o: keyboard.c ../../../../include/linuxmt/sched.h \
 ../../../../include/linuxmt/fs.h ../../../../include/linuxmt/wait.h \
 ../../../../include/linuxmt/types.h ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/vfs.h ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h \
 ../../../../include/linuxmt/ntty.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h ../../../../include/arch/io.h \
 ../../../../include/arch/keyboard.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/fcntl.h \
 ../../../../include/linuxmt/chqueue.h console.h
lp.s lp.o: lp.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/sched.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/wait.h ../../../../include/linuxmt/vfs.h \
 ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h \
 ../../../../include/linuxmt/ntty.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/major.h ../../../../include/linuxmt/lp.h
mem.s mem.o: mem.c ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/linuxmt/major.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/wait.h \
 ../../../../include/linuxmt/types.h ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/vfs.h ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/mm.h ../../../../include/linuxmt/sched.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h \
 ../../../../include/linuxmt/ntty.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/kernel.h \
 ../../../../include/linuxmt/string.h \
 ../../../../include/arch/string.h ../../../../include/linuxmt/debug.h \
 ../../../../include/linuxmt/mem.h ../../../../include/arch/segment.h
meta.s meta.o: meta.c ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h
ntty.s ntty.o: ntty.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/sched.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/wait.h ../../../../include/linuxmt/vfs.h \
 ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h \
 ../../../../include/linuxmt/ntty.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/fcntl.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/mm.h ../../../../include/linuxmt/kernel.h \
 ../../../../include/linuxmt/string.h \
 ../../../../include/arch/string.h \
 ../../../../include/linuxmt/termios.h \
 ../../../../include/linuxmt/chqueue.h \
 ../../../../include/linuxmt/major.h
pty.s pty.o: pty.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/sched.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/wait.h ../../../../include/linuxmt/vfs.h \
 ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h \
 ../../../../include/linuxmt/ntty.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/fcntl.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/mm.h ../../../../include/linuxmt/kernel.h \
 ../../../../include/linuxmt/string.h \
 ../../../../include/arch/string.h \
 ../../../../include/linuxmt/termios.h \
 ../../../../include/linuxmt/chqueue.h \
 ../../../../include/linuxmt/major.h
serial.s serial.o: serial.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/wait.h \
 ../../../../include/linuxmt/chqueue.h \
 ../../../../include/linuxmt/sched.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/vfs.h ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h \
 ../../../../include/linuxmt/ntty.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/serial_reg.h \
 ../../../../include/linuxmt/debug.h \
 ../../../../include/linuxmt/termios.h
sibo_con.s sibo_con.o: sibo_con.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/fcntl.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/wait.h ../../../../include/linuxmt/vfs.h \
 ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/major.h \
 ../../../../include/linuxmt/sched.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h \
 ../../../../include/linuxmt/ntty.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/chqueue.h
sibo_key.s sibo_key.o: sibo_key.c ../../../../include/linuxmt/sched.h \
 ../../../../include/linuxmt/fs.h ../../../../include/linuxmt/wait.h \
 ../../../../include/linuxmt/types.h ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/vfs.h ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h \
 ../../../../include/linuxmt/ntty.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h ../../../../include/arch/io.h \
 ../../../../include/arch/keyboard.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/fcntl.h \
 ../../../../include/linuxmt/chqueue.h
tcpdev.s tcpdev.o: tcpdev.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/sched.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/wait.h ../../../../include/linuxmt/vfs.h \
 ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h \
 ../../../../include/linuxmt/ntty.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/debug.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/major.h \
 ../../../../include/linuxmt/fcntl.h \
 ../../../../include/linuxmt/tcpdev.h ../../../../include/linuxmt/in.h
xt_key.s xt_key.o: xt_key.c ../../../../include/linuxmt/sched.h \
 ../../../../include/linuxmt/fs.h ../../../../include/linuxmt/wait.h \
 ../../../../include/linuxmt/types.h ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/vfs.h ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h \
 ../../../../include/linuxmt/ntty.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h ../../../../include/arch/io.h \
 ../../../../include/arch/keyboard.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/fcntl.h \
 ../../../../include/linuxmt/chqueue.h

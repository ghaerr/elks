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
CFLAGS	=-0 -I../../../../include $(CFLBASE)
CPP	=bcc -E -nostdinc -I../../../../include
CC_PROTO = gcc -I../../../../include -M

.c.s:
	$(CC) $(CFLAGS) \
	-0 -S -o $*.s $<
.s.o:
	$(AS) -0 -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-0 -c -o $*.o $<

OBJS  = bioscon.o serial.o lp.o xt_key.o init.o dircon.o ntty.o mem.o # clist.o tty.o

chr_drv.a: $(OBJS)
	ar rcs chr_drv.a $(OBJS)
	sync

clean:
	rm -f core *~ *.o *.a tmp_make keyboard.s
	for i in *.c;do rm -f `basename $$i .c`.s;done

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

### Dependencies:
bioscon.s bioscon.o: bioscon.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
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
 ../../../../include/linuxmt/signal.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/chqueue.h \
 ../../../../include/linuxmt/ntty.h
dircon.s dircon.o: dircon.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
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
 ../../../../include/linuxmt/signal.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/chqueue.h \
 ../../../../include/linuxmt/ntty.h
init.s init.o: init.c ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/linuxmt/types.h ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h
lp.s lp.o: lp.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
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
 ../../../../include/linuxmt/signal.h ../../../../include/arch/param.h \
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
 ../../../../include/linuxmt/signal.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/kernel.h \
 ../../../../include/linuxmt/string.h \
 ../../../../include/arch/string.h ../../../../include/linuxmt/debug.h \
 ../../../../include/linuxmt/mem.h ../../../../include/arch/segment.h
ntty.s ntty.o: ntty.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
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
 ../../../../include/linuxmt/signal.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/fcntl.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/mm.h ../../../../include/linuxmt/kernel.h \
 ../../../../include/linuxmt/string.h \
 ../../../../include/arch/string.h \
 ../../../../include/linuxmt/termios.h \
 ../../../../include/linuxmt/chqueue.h \
 ../../../../include/linuxmt/ntty.h \
 ../../../../include/linuxmt/major.h
serial.s serial.o: serial.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/wait.h \
 ../../../../include/linuxmt/chqueue.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/linuxmt/sched.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/vfs.h ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/serial_reg.h \
 ../../../../include/linuxmt/ntty.h \
 ../../../../include/linuxmt/debug.h
xt_key.s xt_key.o: xt_key.c ../../../../include/linuxmt/sched.h \
 ../../../../include/linuxmt/fs.h ../../../../include/linuxmt/wait.h \
 ../../../../include/linuxmt/types.h ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/vfs.h ../../../../include/arch/statfs.h \
 ../../../../include/linuxmt/kdev_t.h \
 ../../../../include/linuxmt/ioctl.h ../../../../include/arch/ioctl.h \
 ../../../../include/linuxmt/pipe_fs_i.h \
 ../../../../include/linuxmt/net.h \
 ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/linuxmt/socket.h \
 ../../../../include/linuxmt/uio.h ../../../../include/arch/bitops.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h ../../../../include/arch/io.h \
 ../../../../include/arch/keyboard.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/fcntl.h \
 ../../../../include/linuxmt/chqueue.h \
 ../../../../include/linuxmt/ntty.h

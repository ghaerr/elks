#
# Makefile for the Linux/MT-kernel.
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
CFLAGS	=-Wall $(CFLBASE) -I../../../include
CPP	=bcc -E -nostdinc -I../../../include
CC_PROTO =gcc -I../../../include -M

.c.s:
	$(CC) $(CFLAGS) \
	-0 -S -o $*.s $<
.s.o:
	$(AS) -0 -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-0 -c -o $*.o $<

OBJS  = strace.o printreg.o system.o irq.o irqtab.o process.o bios16.o entry.o # signal.o

akernel.a: $(OBJS)
	ar rcs akernel.a $(OBJS)

entry.c: syscall.dat mkentry.sh
	sh mkentry.sh > tmp.o
	mv tmp.o entry.c

clean:
	rm -f core *~ *.o *.a tmp_make keyboard.s entry.c
	for i in *.c;do rm -f `basename $$i .c`.s;done

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

### Dependencies:
bios16.s bios16.o: bios16.c ../../../include/linuxmt/types.h \
 ../../../include/arch/types.h ../../../include/linuxmt/posix_types.h \
 ../../../include/arch/posix_types.h ../../../include/arch/irq.h \
 ../../../include/arch/segment.h ../../../include/linuxmt/biosparm.h
entry.s entry.o: entry.c
irq.s irq.o: irq.c ../../../include/linuxmt/config.h \
 ../../../include/linuxmt/autoconf.h ../../../include/linuxmt/types.h \
 ../../../include/arch/types.h ../../../include/linuxmt/posix_types.h \
 ../../../include/arch/posix_types.h ../../../include/arch/irq.h \
 ../../../include/linuxmt/errno.h ../../../include/arch/errno.h \
 ../../../include/linuxmt/sched.h ../../../include/linuxmt/fs.h \
 ../../../include/linuxmt/wait.h ../../../include/linuxmt/vfs.h \
 ../../../include/arch/statfs.h ../../../include/linuxmt/kdev_t.h \
 ../../../include/linuxmt/ioctl.h ../../../include/arch/ioctl.h \
 ../../../include/linuxmt/pipe_fs_i.h ../../../include/linuxmt/net.h \
 ../../../include/linuxmt/socket.h ../../../include/linuxmt/uio.h \
 ../../../include/arch/bitops.h ../../../include/linuxmt/time.h \
 ../../../include/linuxmt/signal.h ../../../include/arch/param.h \
 ../../../include/linuxmt/timex.h ../../../include/arch/system.h \
 ../../../include/arch/io.h ../../../include/arch/keyboard.h
irqtab.s irqtab.o: irqtab.c ../../../include/arch/irq.h
printreg.s printreg.o: printreg.c ../../../include/linuxmt/config.h \
 ../../../include/linuxmt/autoconf.h
process.s process.o: process.c ../../../include/linuxmt/types.h \
 ../../../include/arch/types.h ../../../include/linuxmt/posix_types.h \
 ../../../include/arch/posix_types.h ../../../include/arch/irq.h \
 ../../../include/linuxmt/sched.h ../../../include/linuxmt/fs.h \
 ../../../include/linuxmt/wait.h ../../../include/linuxmt/vfs.h \
 ../../../include/arch/statfs.h ../../../include/linuxmt/kdev_t.h \
 ../../../include/linuxmt/ioctl.h ../../../include/arch/ioctl.h \
 ../../../include/linuxmt/pipe_fs_i.h ../../../include/linuxmt/net.h \
 ../../../include/linuxmt/config.h ../../../include/linuxmt/autoconf.h \
 ../../../include/linuxmt/socket.h ../../../include/linuxmt/uio.h \
 ../../../include/arch/bitops.h ../../../include/linuxmt/time.h \
 ../../../include/linuxmt/signal.h ../../../include/arch/param.h \
 ../../../include/linuxmt/timex.h
signal.s signal.o: signal.c ../../../include/linuxmt/types.h \
 ../../../include/arch/types.h ../../../include/linuxmt/posix_types.h \
 ../../../include/arch/posix_types.h ../../../include/arch/irq.h \
 ../../../include/linuxmt/config.h ../../../include/linuxmt/autoconf.h \
 ../../../include/linuxmt/sched.h ../../../include/linuxmt/fs.h \
 ../../../include/linuxmt/wait.h ../../../include/linuxmt/vfs.h \
 ../../../include/arch/statfs.h ../../../include/linuxmt/kdev_t.h \
 ../../../include/linuxmt/ioctl.h ../../../include/arch/ioctl.h \
 ../../../include/linuxmt/pipe_fs_i.h ../../../include/linuxmt/net.h \
 ../../../include/linuxmt/socket.h ../../../include/linuxmt/uio.h \
 ../../../include/arch/bitops.h ../../../include/linuxmt/time.h \
 ../../../include/linuxmt/signal.h ../../../include/arch/param.h \
 ../../../include/linuxmt/timex.h ../../../include/linuxmt/mm.h \
 ../../../include/linuxmt/errno.h ../../../include/arch/errno.h \
 ../../../include/linuxmt/kernel.h ../../../include/linuxmt/string.h \
 ../../../include/arch/string.h ../../../include/arch/segment.h
strace.s strace.o: strace.c ../../../include/linuxmt/autoconf.h
system.s system.o: system.c ../../../include/linuxmt/types.h \
 ../../../include/arch/types.h ../../../include/linuxmt/posix_types.h \
 ../../../include/arch/posix_types.h ../../../include/arch/irq.h \
 ../../../include/linuxmt/kernel.h ../../../include/linuxmt/wait.h \
 ../../../include/linuxmt/sched.h ../../../include/linuxmt/fs.h \
 ../../../include/linuxmt/vfs.h ../../../include/arch/statfs.h \
 ../../../include/linuxmt/kdev_t.h ../../../include/linuxmt/ioctl.h \
 ../../../include/arch/ioctl.h ../../../include/linuxmt/pipe_fs_i.h \
 ../../../include/linuxmt/net.h ../../../include/linuxmt/config.h \
 ../../../include/linuxmt/autoconf.h ../../../include/linuxmt/socket.h \
 ../../../include/linuxmt/uio.h ../../../include/arch/bitops.h \
 ../../../include/linuxmt/time.h ../../../include/linuxmt/signal.h \
 ../../../include/arch/param.h ../../../include/linuxmt/timex.h \
 ../../../include/arch/segment.h

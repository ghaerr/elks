#
# Makefile for the ELKS blocks devices.
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
CFLAGS	=-0 -I../../../../include  $(CFLBASE)
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

OBJS  = genhd.o doshd.o ll_rw_blk.o rd.o floppy.o directhd.o #mscdex.o

blk_drv.a: $(OBJS)
	ar rcs blk_drv.a $(OBJS)

clean:
	rm -f core *~ *.o *.a tmp_make keyboard.s
	for i in *.c;do rm -f `basename $$i .c`.s;done

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

### Dependencies:
directhd.s directhd.o: directhd.c ../../../../include/linuxmt/hdreg.h \
 ../../../../include/linuxmt/major.h \
 ../../../../include/linuxmt/genhd.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/wait.h \
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
 ../../../../include/linuxmt/string.h \
 ../../../../include/arch/string.h ../../../../include/linuxmt/mm.h \
 ../../../../include/linuxmt/sched.h \
 ../../../../include/linuxmt/time.h \
 ../../../../include/linuxmt/signal.h ../../../../include/arch/param.h \
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/kernel.h \
 ../../../../include/linuxmt/directhd.h blk.h
doshd.s doshd.o: doshd.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/kernel.h \
 ../../../../include/linuxmt/sched.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/wait.h ../../../../include/linuxmt/vfs.h \
 ../../../../include/arch/statfs.h \
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
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/genhd.h \
 ../../../../include/linuxmt/hdreg.h \
 ../../../../include/linuxmt/biosparm.h \
 ../../../../include/linuxmt/major.h \
 ../../../../include/linuxmt/bioshd.h \
 ../../../../include/linuxmt/string.h \
 ../../../../include/arch/string.h ../../../../include/linuxmt/mm.h \
 ../../../../include/arch/segment.h ../../../../include/arch/system.h \
 blk.h
floppy.s floppy.o: floppy.c ../../../../include/linuxmt/sched.h \
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
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/kernel.h ../../../../include/linuxmt/mm.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/string.h \
 ../../../../include/arch/string.h ../../../../include/linuxmt/fdreg.h \
 ../../../../include/linuxmt/fd.h ../../../../include/arch/dma.h \
 ../../../../include/arch/io.h ../../../../include/arch/system.h \
 ../../../../include/arch/segment.h
genhd.s genhd.o: genhd.c ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/linuxmt/fs.h ../../../../include/linuxmt/wait.h \
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
 ../../../../include/linuxmt/genhd.h \
 ../../../../include/linuxmt/kernel.h \
 ../../../../include/linuxmt/major.h \
 ../../../../include/linuxmt/string.h \
 ../../../../include/arch/string.h ../../../../include/arch/system.h
ll_rw_blk.s ll_rw_blk.o: ll_rw_blk.c ../../../../include/linuxmt/types.h \
 ../../../../include/arch/types.h \
 ../../../../include/linuxmt/posix_types.h \
 ../../../../include/arch/posix_types.h ../../../../include/arch/irq.h \
 ../../../../include/linuxmt/sched.h ../../../../include/linuxmt/fs.h \
 ../../../../include/linuxmt/wait.h ../../../../include/linuxmt/vfs.h \
 ../../../../include/arch/statfs.h \
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
 ../../../../include/linuxmt/timex.h \
 ../../../../include/linuxmt/kernel.h \
 ../../../../include/linuxmt/errno.h ../../../../include/arch/errno.h \
 ../../../../include/linuxmt/string.h \
 ../../../../include/arch/string.h ../../../../include/linuxmt/mm.h \
 ../../../../include/arch/system.h ../../../../include/arch/io.h blk.h \
 ../../../../include/linuxmt/major.h \
 ../../../../include/linuxmt/genhd.h
rd.s rd.o: rd.c ../../../../include/linuxmt/config.h \
 ../../../../include/linuxmt/autoconf.h \
 ../../../../include/linuxmt/rd.h ../../../../include/linuxmt/major.h \
 ../../../../include/linuxmt/kernel.h \
 ../../../../include/linuxmt/debug.h /usr/include/errno.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/linux/errno.h /usr/include/asm/errno.h

# Makefile for the Linux/MT-kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
#
# Note that the standard Makefile rules and defines have been moved to
# the file below.

ELKSDIR		= ..

include $(ELKSDIR)/Makefile-rules

#########################################################################

include $(ELKSDIR)/.config

OBJS  = super.o bufops.o devices.o fcntl.o stat.o inode.o file_table.o \
	buffer.o block_dev.o namei.o ioctl.o filesystems.o open.o \
	read_write.o readdir.o exec.o select.o pipe.o

all:	fs.a

fs.a: $(OBJS)
	ar rcs fs.a $(OBJS)

clean:
	rm -f core *~ *.o *.a tmp_make keyboard.s
	for i in *.c;do rm -f `basename $$i .c`.s;done

distdir:
	cp -pf Config.in Makefile $(TOPDIR)/$(DISTDIR)/fs
	cp -pf *.c $(TOPDIR)/$(DISTDIR)/fs

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

#########################################################################
### Dependencies:

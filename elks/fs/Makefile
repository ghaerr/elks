# Makefile for the Linux/MT-kernel.
#
#########################################################################
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
# Relative path to base directory.

ELKSDIR 	= ..

#########################################################################
# Include standard commands.

include $(ELKSDIR)/Makefile-rules

#########################################################################
# What to compile.

include $(ELKSDIR)/.config

#########################################################################
# Objects to be compiled.

OBJS  = super.o bufops.o devices.o fcntl.o stat.o inode.o file_table.o \
	buffer.o block_dev.o namei.o ioctl.o filesystems.o open.o \
	read_write.o readdir.o exec.o select.o pipe.o

#########################################################################
# Commands.

all:	fs.a

fs.a:	$(OBJS)
	ar rcs fs.a $(OBJS)

#########################################################################
# Standard commands.

clean:
	rm -f core *~ *.o *.a *.tmp keyboard.s
	for i in *.c;do rm -f `basename $$i .c`.s;done

distdir:
	cp -pf Config.in Makefile *.c $(DISTDIR)/fs

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > make.tmp
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> make.tmp
	mv make.tmp Makefile

#########################################################################
### Dependencies

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

ELKSDIR 	= ../..

#########################################################################
# Include standard commands.

include $(ELKSDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS  = bitmap.o dir.o file.o namei.o symlink.o truncate.o inode.o

#########################################################################
# Commands.

all:	minixfs.a

minixfs.a: $(OBJS)
	ar rcs minixfs.a $(OBJS)

#########################################################################
# Standard commands.

clean:
	rm -f core *~ *.o *.a *.tmp keyboard.s
	for i in *.c;do rm -f `basename $$i .c`.s;done

distdir:
	cp -pf Makefile *.c $(DISTDIR)/fs/minix

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > make.tmp
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> make.tmp
	mv make.tmp Makefile

#########################################################################
### Dependencies:

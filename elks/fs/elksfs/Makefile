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

ELKSDIR		= ../..

include $(ELKSDIR)/Makefile-rules

#########################################################################

OBJS  = bitmap.o dir.o file.o namei.o symlink.o truncate.o inode.o

all:	elksfs.a

elksfs.a: $(OBJS)
	ar rcs elksfs.a $(OBJS)

clean:
	rm -f core *~ *.o *.a tmp_make
	for i in *.c;do rm -f `basename $$i .c`.s;done

distdir:
	cp -pf Makefile $(TOPDIR)/$(DISTDIR)/fs/elksfs
	cp -pf *.c $(TOPDIR)/$(DISTDIR)/fs/elksfs

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

#########################################################################
### Dependencies:

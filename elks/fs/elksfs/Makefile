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
# Standard commands.

AR		= ar
AS		= as86 
LD		= ld86 
LDFLAGS 	= -0 -s -x
CC		= bcc
CFLAGS		= $(CFLBASE) -I$(ELKSDIR)/include 
CPP		= bcc -E -nostdinc -I$(ELKSDIR)/include
CC_PROTO	= gcc -I$(ELKSDIR)/include -M

#########################################################################
# Standard prototypes.

.c.s:
	$(CC) $(CFLAGS) \
	-0 -S -o $*.s $<
.s.o:
	$(AS) -0 -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-0 -c -o $*.o $<

#########################################################################
# Objects to be compiled.

OBJS  = bitmap.o dir.o file.o namei.o symlink.o truncate.o inode.o

#########################################################################
# Commands.

all:	elksfs.a

elksfs.a:	$(OBJS)
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

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
# What to compile.

include $(ELKSDIR)/.config

#########################################################################
# Objects to be compiled.

OBJS  = super.o bufops.o devices.o fcntl.o stat.o \
	inode.o file_table.o buffer.o block_dev.o \
	namei.o ioctl.o filesystems.o open.o \
	read_write.o readdir.o exec.o select.o pipe.o

#########################################################################
# Commands.

all:	fs.a

fs.a:	$(OBJS)
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
### Dependencies

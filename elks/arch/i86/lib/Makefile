# Makefile for bcc library - 8086 assembler routines
#
#########################################################################
# Relative path to base directory.

ELKSDIR 	= ../../..

#########################################################################
# Standard commands.

AR		= ar
AS		= bcc
ASFLAGS 	= -0 -c
CC		= bcc
CFLAGS 		= -0 -D__KERNEL__ -I$(ELKSDIR)/include

CC_PROTO	= gcc -I$(ELKSDIR)/include -M

#########################################################################
# Standard prototypes.

.c.s:
	$(CC) $(CFLAGS) -S -o $*.s $<

.s.o:
	$(AS) $(ASFLAGS) -o $*.o $<

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

#.S.o:
#	gcc -I $(ELKSDIR)/include -E -traditional -o $*.s $<
#	$(AS) -0 -c -o $*.o $<

.S.s:
	gcc -I $(ELKSDIR)/include -E -traditional -o $*.s $<

#########################################################################
# Objects to be compiled.

OBJS		= $(IOBJS) $(JOBJS) $(LLOBJS) $(LXOBJS)

# compiler support for integer arithmetic

IOBJS		=idiv.o idivu.o imod.o imodu.o imul.o isl.o isr.o isru.o

# miscellaneous

JOBJS		=inport.o inportb.o outport.o outportb.o \
		 peekb.o peekw.o peekd.o pokeb.o pokew.o poked.o bitops.o \
		 memmove.o string.o fmemset.o border.o
#		 ntohl.o ntohs.o

# compiler support for long arithmetic on little-endian (normal) longs

LLOBJS		=laddl.o landl.o lcmpl.o lcoml.o ldecl.o ldivl.o ldivul.o \
		 leorl.o lincl.o lmodl.o lmodul.o lmull.o lnegl.o lorl.o \
		 lsll.o lsrl.o lsrul.o lsubl.o ltstl.o setupw.o setupb.o

# compiler support for long arithmetic on all longs

LXOBJS		=ldivmod.o

#########################################################################
# Commands.

all:	lib86.a

lib86.a: $(OBJS)
	ar rcs lib86.a $(OBJS)
	sync

clean:
	rm -f core *~ *.o *.a tmp_make keyboard.s
	for i in *.c;do rm -f `basename $$i .c`.s;done
	rm -f setupb.s setupw.s

distdir:
	cp -pf Makefile $(TOPDIR)/$(DISTDIR)/arch/i86/lib
	cp -pf *.c *.S *.s $(TOPDIR)/$(DISTDIR)/arch/i86/lib

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

setupb.s: setupb.S

setupw.s: setupw.S

#########################################################################
### Dependencies:

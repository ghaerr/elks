# Makefile for bcc library - 8086 assembler routines
#
#########################################################################
#
# Note that the standard Makefile rules and defines have been moved to
# the file below.

ELKSDIR		= ../../..

include $(ELKSDIR)/Makefile-rules

#########################################################################

OBJS		= $(IOBJS) $(JOBJS) $(LLOBJS) $(LXOBJS)

# compiler support for integer arithmetic
IOBJS		= idiv.o idivu.o imod.o imodu.o imul.o isl.o isr.o isru.o

# miscellaneous
JOBJS		= inport.o inportb.o outport.o outportb.o \
		  peekb.o peekw.o peekd.o pokeb.o pokew.o poked.o \
		  bitops.o memmove.o string.o fmemset.o border.o
#		  ntohl.o ntohs.o

# compiler support for long arithmetic on little-endian (normal) longs
LLOBJS		= laddl.o landl.o lcmpl.o lcoml.o ldecl.o ldivl.o \
		  ldivul.o leorl.o lincl.o lmodl.o lmodul.o lmull.o \
		  lnegl.o lorl.o lsll.o lsrl.o lsrul.o lsubl.o ltstl.o \
		  setupw.o setupb.o

# compiler support for long arithmetic on all longs
LXOBJS		= ldivmod.o

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

### Dependencies:

# Makefile for bcc library - 8086 assembler routines

AR		=ar
AS		=bcc -0 -c
CC		=bcc -0  
CC_PROTO=gcc -I../../../include -M

OBJS		= $(IOBJS) $(JOBJS) $(LLOBJS) $(LXOBJS)

# compiler support for integer arithmetic
IOBJS		=idiv.o idivu.o imod.o imodu.o imul.o isl.o isr.o isru.o

# miscellaneous
JOBJS		=inport.o inportb.o outport.o outportb.o \
		 peekb.o peekw.o pokeb.o pokew.o bitops.o memmove.o \
		 string.o fmemset.o
#		 ntohl.o ntohs.o

# compiler support for long arithmetic on little-endian (normal) longs
LLOBJS		=laddl.o landl.o lcmpl.o lcoml.o ldecl.o ldivl.o ldivul.o \
		 leorl.o lincl.o lmodl.o lmodul.o lmull.o lnegl.o lorl.o \
		 lsll.o lsrl.o lsrul.o lsubl.o ltstl.o setupw.o setupb.o

# compiler support for long arithmetic on all longs
LXOBJS		=ldivmod.o

CFLAGS 		=-D__KERNEL__ -I../../../include

.c.s:
	$(CC) $(CFLAGS) \
	-0 -S -o $*.s $<
.s.o:
	$(AS) -0 -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-0 -c -o $*.o $<

.S.o:
	gcc -I ../../../include -E -traditional -o $*.s $<
	$(AS) -0 -c -o $*.o $*.s

lib86.a: $(OBJS)
	ar rcs lib86.a $(OBJS)
	sync

clean:
	rm -f core *~ *.o *.a tmp_make keyboard.s
	for i in *.c;do rm -f `basename $$i .c`.s;done
	rm -f setupb.s setupw.s

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

### Dependencies:

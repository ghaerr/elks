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
	$(AS) -0 -o $*.o $<
.S.s:
	$(CC) $(CFLAGS) -P -o $*.s $<
.c.o:
	$(CC) $(CFLAGS) -0 -c -o $*.o $<

OBJS  = genhd.o doshd.o ll_rw_blk.o rd.o floppy.o directhd.o init.o sibo_ssd.o ssd_asm.o

blk_drv.a: $(OBJS)
	ar rcs blk_drv.a $(OBJS)

clean:
	rm -f core *~ *.o *.a tmp_make keyboard.s
	for i in *.c;do rm -f `basename $$i .c`.s;done
	for i in *.S;do rm -f `basename $$i .S`.s;done

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

ssd_asm.o: ssd_asm.s

ssd_asm.s: ssd_asm.S

### Dependencies:

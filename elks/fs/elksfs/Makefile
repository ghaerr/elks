#
# Makefile for the Linux/MT-kernel.
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
CFLAGS	=$(CFLBASE) -I../../include 
CPP	=bcc -E -nostdinc -I../../include
CC_PROTO =gcc -I../../include -M

.c.s:
	$(CC) $(CFLAGS) \
	-0 -S -o $*.s $<
.s.o:
	$(AS) -0 -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-0 -c -o $*.o $<

OBJS  = bitmap.o dir.o file.o namei.o symlink.o truncate.o inode.o

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

### Dependencies

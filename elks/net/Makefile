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
	$(CC) $(CFLAGS) -0 -S -o $*.s $<

.s.o:
	$(AS) -0 -c -o $*.o $<

.c.o:
	$(CC) $(CFLAGS) -0 -c -o $*.o $<

#########################################################################
# Objects to compile.

OBJS  = socket.o protocols.o ipv4/af_inet.o unix/af_unix.o nano/af_nano.o

#########################################################################
# Rules.

all:	net.a

net.a: $(OBJS)
	ar rcs net.a $(OBJS)

clean:
	rm -f core *~ *.o *.a tmp_make */*.o
	for i in *.c;do rm -f `basename $$i .c`.s;done

distdir:
	cp -pf Config.in Makefile $(TOPDIR)/$(DISTDIR)/net
	cp -pf *.c $(TOPDIR)/$(DISTDIR)/net
	(mkdir $(TOPDIR)/$(DISTDIR)/net/nano)
	(mkdir $(TOPDIR)/$(DISTDIR)/net/unix)
	(mkdir $(TOPDIR)/$(DISTDIR)/net/ipv4)
	cp -pf Makefile nano/*.c nano/*.h $(TOPDIR)/$(DISTDIR)/net/nano
	cp -pf Makefile unix/*.c unix/*.h $(TOPDIR)/$(DISTDIR)/net/unix
	cp -pf Makefile ipv4/*.c ipv4/*.h $(TOPDIR)/$(DISTDIR)/net/ipv4

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

#########################################################################
### Dependencies:

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
# Include standard commands.

include $(ELKSDIR)/Makefile-rules

#########################################################################
# Objects to compile.

OBJS  = socket.o protocols.o ipv4/af_inet.o nano/af_nano.o unix/af_unix.o

#########################################################################
# Rules.

all:	net.a

net.a: $(OBJS)
	ar rcs net.a $(OBJS)

#########################################################################
# Standard commands.

clean:
	rm -f core *~ *.o *.a tmp_make */*.o
	for i in *.c;do rm -f `basename $$i .c`.s;done

distdir:
	cp -pf Config.in Makefile *.c $(DISTDIR)/net
	mkdir $(DISTDIR)/net/ipv4
	cp -pf ipv4/Makefile ipv4/*.c ipv4/*.h $(DISTDIR)/net/ipv4
	mkdir $(DISTDIR)/net/nano
	cp -pf nano/Makefile nano/*.c nano/*.h $(DISTDIR)/net/nano
	mkdir $(DISTDIR)/net/unix
	cp -pf unix/Makefile unix/*.c unix/*.h $(DISTDIR)/net/unix

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

#########################################################################
### Dependencies:

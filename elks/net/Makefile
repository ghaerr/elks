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

ELKSDIR		= ..

include $(ELKSDIR)/Makefile-rules

#########################################################################

OBJS  = socket.o protocols.o ipv4/af_inet.o unix/af_unix.o nano/af_nano.o

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
	cp -pf nano/*.c nano/*.h $(TOPDIR)/$(DISTDIR)/net/nano
	cp -pf unix/*.c unix/*.h $(TOPDIR)/$(DISTDIR)/net/unix

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

### Dependencies:

# Makefile for the ELKS 286pmode extender.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
#
# Note that the standard Makefile rules and defines have been moved to
# the file below.

ELKSDIR		= ../../..

include $(ELKSDIR)/Makefile-rules

#########################################################################
# crt0.o should be first on this next line.

OBJS  = crt0.o main.o irq.o

pmode286: $(OBJS)
	$(LD) -0 -i $(OBJS) ../lib/lib86.a -M -o pmode286 > ../../../pmode286.map

clean:
	rm -f core *~ *.o *.a tmp_make pmode286 pmode286.map
	for i in *.c;do rm -f `basename $$i .c`.s;done

distdir:
	cp -pf Makefile Config.in $(TOPDIR)/$(DISTDIR)/arch/i86/286pmode
	cp -pf crt0.s descriptor.h irq.s $(TOPDIR)/$(DISTDIR)/arch/i86/286pmode
	cp -pf main.c tss.h $(TOPDIR)/$(DISTDIR)/arch/i86/286pmode

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CC_PROTO) $$i;done) >> tmp_make
	mv tmp_make Makefile

### Dependencies:

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
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

ELKSSTD 	= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND		= 

DISTFILES	= 

#########################################################################
# Include standard commands.

include $(ELKSDIR)/Makefile-rules

#########################################################################
# Objects to compile.

OBJS  = chqueue.o string.o

#########################################################################
# Commands:

all:	lib.a

lib.a:	$(OBJS)
	ar rcs lib.a $(OBJS)
	sync

#########################################################################
# Standard rules.

distdir:
	cp -pf Makefile *.c $(DISTDIR)/lib

#########################################################################
### Dependencies:
chqueue.o: chqueue.c /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/linuxmt/wait.h \
 /usr/src/elks/include/linuxmt/chqueue.h \
 /usr/src/elks/include/linuxmt/debug.h \
 /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h

string.o: string.c /usr/src/elks/include/linuxmt/types.h \
 /usr/src/elks/include/arch/types.h \
 /usr/src/elks/include/linuxmt/posix_types.h \
 /usr/src/elks/include/linuxmt/config.h \
 /usr/src/elks/include/linuxmt/autoconf.h \
 /usr/src/elks/include/arch/posix_types.h \
 /usr/src/elks/include/arch/irq.h \
 /usr/src/elks/include/linuxmt/string.h \
 /usr/src/elks/include/arch/string.h


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

BASEDIR 	= ..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	= 

NOINDENT	= 

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

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

chqueue.o: chqueue.c /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/wait.h \
 /X/elks/include/linuxmt/chqueue.h /X/elks/include/linuxmt/debug.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h


string.o: string.c /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h



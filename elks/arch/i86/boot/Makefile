# Note that the standard Makefile rules and defines have been moved to
# the file below.

BASEDIR 	= ../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

CLEANDEP	= 

CLEANME 	= Image bootsect nbImage nbImage.rd netbootsect setup \
		  system system.app

DEPEND  	= 

DISTFILES	= empty system.afl system.rsz tux.pic

NOINDENT	= crt1.c

#########################################################################
# Include the standard rules.

include $(BASEDIR)/Makefile-rules

#########################################################################
### Dependencies:

crt1.o: crt1.c /X/elks/include/arch/segment.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/arch/types.h \
 /X/elks/include/linuxmt/config.h /X/elks/include/linuxmt/autoconf.h \
 /X/elks/include/linuxmt/kdev_t.h /X/elks/include/linuxmt/major.h \
 /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h



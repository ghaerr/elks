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

crt1.o: crt1.c /home/rhw/CVS/Source/X/elks-test/include/arch/segment.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/config.h \
 /home/rhw/CVS/Source/X/elks-test/include/linuxmt/autoconf.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/posix_types.h \
 /home/rhw/CVS/Source/X/elks-test/include/arch/irq.h



# Basic Makefile, created on Sunday, 09 September 2001 at 09:50:03 BST.
#
#########################################################################
#
# Note that the standard Makefile rules and defines have been moved to
# the file below.

ELKSDIR		= ../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

ELKSSTD 	= Y

CLEANDEP	= 

CLEANME 	= Image bootsect nbImage nbImage.rd netbootsect setup \
		  system system.app

DEPEND		= 

DISTFILES	= empty system.afl system.rsz tux.pic

#########################################################################
# Include the standard rules.

include $(ELKSDIR)/Makefile-rules

#########################################################################
### Dependencies:
crt1.o: crt1.c /usr/src/elks/include/arch/segment.h


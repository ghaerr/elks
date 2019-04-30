# Minix fsck Makefile
#
#########################################################################
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
#########################################################################
# Relative path to base directory.

BASEDIR 	= ../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= N

CLEANDEP	=

CLEANME 	= ../bin/mfsck

DEPEND  	=

DISTFILES	=

NOINDENT	=

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS  = mfsck.o

#########################################################################
# Commands.

all:	../bin/mfsck

../bin/mfsck: $(OBJS) mfsck.h bitops.h
	$(CC) -o ../bin/mfsck $(OBJS)

#########################################################################
### Dependencies:

# Objdump a.out header dump tool
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

CLEANME 	= ../bin/objdump86

DEPEND  	=

DISTFILES	=

NOINDENT	=

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS=objdump86.o

#########################################################################
# Commands.

all:	../bin/objdump86

../bin/objdump86: $(OBJS)
	$(CC) -o ../bin/objdump86 $(CFLAGS) $(OBJS)

#########################################################################
### Dependencies:

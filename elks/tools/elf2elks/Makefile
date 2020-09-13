# Convert ELF output file from gold linker to ELKS a.out
# Requires libelf to be installed
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

CLEANME 	= ../bin/elf2elks

DEPEND  	=

DISTFILES	=

NOINDENT	=

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS=elf2elks.o

#########################################################################
# Commands.

all:	../bin/elf2elks

../bin/elf2elks: $(OBJS)
	$(CC) -o ../bin/elf2elks $(CFLAGS) $(OBJS) -lelf

#########################################################################
### Dependencies:

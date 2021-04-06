# Minix filesystem manipulation tool
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

CLEANME 	= ../bin/elks-compress ../bin/exomizer

DEPEND  	=

DISTFILES	=

NOINDENT	=

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

SRCS=elks-compress.c
OBJS=$(SRCS:.c=.o)

#########################################################################
# Commands.

all:	../bin/elks-compress ../bin/exomizer

../bin/elks-compress: minix.h $(OBJS)
	$(CC) -o ../bin/elks-compress $(CFLAGS) $(OBJS)

../bin/exomizer:
	make -C exomizer/src
	cp -p exomizer/src/exomizer ../bin/exomizer

#########################################################################
### Dependencies:

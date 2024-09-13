# fixomf - Mark symbols as weak reference in Watcom OBJ files
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

CLEANME 	= ../bin/fixomf

DEPEND  	=

DISTFILES	=

NOINDENT	=

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

SRCS=fcenable.c records.c mem.c makepath.c split2.c

OBJS=$(SRCS:.c=.o)

#########################################################################
# Commands.

all:	../bin/fixomf

CFLAGS += -D__UNIX__ -O3 -Wno-pragma-pack

../bin/fixomf: $(OBJS)
	$(CC) -o ../bin/fixomf $(CFLAGS) $(OBJS)

#########################################################################
### Dependencies:

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

USEBCC 	= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND		= 

DISTFILES	= 

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

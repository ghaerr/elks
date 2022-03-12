# eman - display ELKS manual pages on host
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

CLEANME 	= ../bin/eman

DEPEND  	=

DISTFILES	=

NOINDENT	=

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

SRCS=$(TOPDIR)/elkscmd/sys_utils/man.c

#OBJS=$(SRCS:.c=.o)

#########################################################################
# Commands.

all:	../bin/eman

../bin/eman: $(SRCS)
	$(CC) -D_PATH_MANPAGES=\"$(TOPDIR)/elkscmd/man\" -o ../bin/eman $(CFLAGS) $(SRCS)

#########################################################################
### Dependencies:

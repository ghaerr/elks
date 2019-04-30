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

CLEANME 	= ../bin/mfs

DEPEND  	=

DISTFILES	=

NOINDENT	=

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

SRCS=dir.c genfs.c iname.c inode.c main.c reader.c super.c utils.c writer.c
OBJS=$(SRCS:.c=.o)

#########################################################################
# Commands.

all:	../bin/mfs

../bin/mfs: minix_fs.h bitops.h protos.h $(OBJS)
	$(CC) -o ../bin/mfs $(CFLAGS) $(OBJS)

#########################################################################
### Dependencies:

# Makefile for bcc library - 8086 assembler routines
#
#########################################################################
# Relative path to base directory.

BASEDIR 	= ../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		=

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	= 

NOINDENT	=

#########################################################################
# Include the standard rules.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

# Precompiled assembly

SRCS1 = \
	fmemory.S \
	peekpoke.S \
	string.S \
	bios16.S \
	unreal.S \
	printreg.S \
	atomic.S \
	# end of list

OBJS1 = $(SRCS1:.S=.o)

# Non-precompiled assembly

SRCS2 = \
	border.s \
	# end of list

OBJS2 = $(SRCS2:.s=.o)

# C sources

SRCS3 = \
	bitops.c \
	# end of list

OBJS3 = $(SRCS3:.c=.o)

OBJS		= $(OBJS1) $(OBJS2) $(OBJS3)


#########################################################################
# Commands.

all:	lib86.a

lib86.a: $(OBJS)
	$(AR) rcs lib86.a $(OBJS)

$(TMPS1): $(SRCS1)
$(OBJS1): $(TMPS1)
$(OBJS2): $(SRCS2)
$(OBJS3): $(SRCS3)

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile $(SRCS1) $(SRCS2) $(SRCS3) $(DISTDIR)/arch/i86/lib

#########################################################################
### Dependencies:

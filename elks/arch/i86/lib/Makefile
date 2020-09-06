# Makefile for bcc library - 8086 assembler routines
#
#########################################################################
# Relative path to base directory.

BASEDIR 	= ../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

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
	fmemcpyb.S fmemcpyw.S \
	fmemcmpb.S fmemcmpw.S \
	setupb.S setupw.S \
	string.S \
	ia16-far-call-thunks.S \
#	farlist.S \
	# end of list

OBJS1 = $(SRCS1:.S=.o)

# Non-precompiled assembly

SRCS2 = \
	peekb.s peekw.s pokeb.s pokew.s \
	fmemsetb.s fmemsetw.s \
	border.s \
	ldivmod.s \
	divmodsi3.s \
	# end of list

OBJS2 = $(SRCS2:.s=.o)

# C sources

SRCS3 = \
	bitops.c \
	# end of list

OBJS3 = $(SRCS3:.c=.o)

OBJS		= $(OBJS1) $(OBJS2) $(OBJS3)
#OBJS		= $(OBJS1) $(OBJS2) $(OBJS3) $(JOBJS)

OBJS_MEM = \
	peekb.o \
	peekw.o \
	pokeb.o \
	pokew.o \
	fmemsetb.o \
	fmemsetw.o \
	fmemcpyb.o \
	fmemcpyw.o \
	fmemcmpb.o \
	fmemcmpw.o \

# miscellaneous
# TODO: move them to the C library

JOBJS       = inport.o inportb.o outport.o outportb.o irqflag.o \
              memcmp.o

#########################################################################
# Commands.

all:	lib86.a libmem.a

lib86.a: $(OBJS)
	$(AR) rcs lib86.a $(OBJS)

libmem.a: $(OBJS_MEM)
	$(AR) rcs $@ $^

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

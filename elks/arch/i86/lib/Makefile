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

SRCS1= \
	fmemcpyb.S fmemcpyw.S \
	fmemcmpb.S fmemcmpw.S \
	setupb.S setupw.S \
	string.S \
	farlist.S seglist.S \
	# end of list

OBJS1=$(SRCS1:.S=.o)

# Non-precompiled assembly

SRCS2= \
	peekb.s peekw.s pokeb.s pokew.s \
	fmemsetb.s fmemsetw.s \
	border.s \
	ldivmod.s \
	# end of list

OBJS2=$(SRCS2:.s=.o)

# C sources

SRCS3= \
	bitops.c \
	# end of list

OBJS3=$(SRCS3:.c=.o)

ifeq ($(USEIA16), y)
OBJS		= $(OBJS1) $(OBJS2) $(OBJS3) divmodsi3.o
else
OBJS		= $(OBJS1) $(OBJS2) $(OBJS3) $(IOBJS) $(JOBJS) $(LLOBJS)
endif

# compiler support for integer arithmetic

IOBJS		=idiv.o idivu.o imod.o imodu.o imul.o isl.o isr.o isru.o min.o ctypefn.o 

# miscellaneous

JOBJS       = inport.o inportb.o outport.o outportb.o irqflag.o \
              memcmp.o
#		 ntohl.o ntohs.o

# compiler support for long arithmetic on little-endian (normal) longs

LLOBJS  	=laddl.o landl.o lcmpl.o lcoml.o ldecl.o ldivl.o ldivul.o \
		 leorl.o lincl.o lmodl.o lmodul.o lmull.o lnegl.o lorl.o \
		 lsll.o lsrl.o lsrul.o lsubl.o ltstl.o

#########################################################################
# Commands.

all:	lib86.a

lib86.a: $(OBJS)
	$(AR) $(ARFLAGS) lib86.a $(OBJS)

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile $(SRCS1) $(SRCS2) $(SRCS3) $(DISTDIR)/arch/i86/lib

#########################################################################
### Dependencies:

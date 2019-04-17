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

USEBCC  	= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	= 

NOINDENT	= 

#########################################################################
# Include the standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS  = buffer.o super.o bufops.o devices.o fcntl.o stat.o inode.o file_table.o \
	block_dev.o namei.o ioctl.o filesystems.o open.o read_write.o \
	readdir.o exec.o select.o pipe.o

#########################################################################
# Commands.

all:	fs.a

fs.a:	$(OBJS)
	$(AR) $(ARFLAGS) fs.a $(OBJS)

#########################################################################
# Standard commands.

distdir:
	cp -pf config.in Makefile *.c $(DISTDIR)/fs

#########################################################################
### Dependencies:

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

NOINDENT	= memmove.c

#########################################################################
# Include the standard rules.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

OBJS		= $(IOBJS) $(JOBJS) $(LLOBJS) $(LXOBJS)

# compiler support for integer arithmetic

IOBJS		=idiv.o idivu.o imod.o imodu.o imul.o isl.o isr.o isru.o

# miscellaneous

JOBJS		=inport.o inportb.o outport.o outportb.o \
		 peekb.o peekw.o peekd.o pokeb.o pokew.o poked.o bitops.o \
		 memmove.o string.o fmemset.o border.o
#		 ntohl.o ntohs.o

# compiler support for long arithmetic on little-endian (normal) longs

LLOBJS  	=laddl.o landl.o lcmpl.o lcoml.o ldecl.o ldivl.o ldivul.o \
		 leorl.o lincl.o lmodl.o lmodul.o lmull.o lnegl.o lorl.o \
		 lsll.o lsrl.o lsrul.o lsubl.o ltstl.o setupw.o setupb.o

# compiler support for long arithmetic on all longs

LXOBJS  	=ldivmod.o

#########################################################################
# Commands.

all:	lib86.a

lib86.a: $(OBJS)
	ar rcs lib86.a $(OBJS)
	sync

setupb.s: setupb.S

setupw.s: setupw.S

#########################################################################
# Standard commands.

distdir:
	cp -pf Makefile *.c *.S *.s $(DISTDIR)/arch/i86/lib

#########################################################################
### Dependencies:

bitops.o: bitops.c /X/elks/include/arch/bitops.h \
 /X/elks/include/arch/types.h /X/elks/include/arch/irq.h \
 /X/elks/include/linuxmt/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/asm.h \
 /X/elks/include/linuxmt/kernel.h


memmove.o: memmove.c /X/elks/include/linuxmt/mm.h \
 /X/elks/include/linuxmt/sched.h /X/elks/include/linuxmt/types.h \
 /X/elks/include/arch/types.h /X/elks/include/linuxmt/config.h \
 /X/elks/include/linuxmt/autoconf.h /X/elks/include/linuxmt/kdev_t.h \
 /X/elks/include/linuxmt/major.h /X/elks/include/linuxmt/posix_types.h \
 /X/elks/include/arch/posix_types.h /X/elks/include/arch/irq.h \
 /X/elks/include/arch/asm.h /X/elks/include/linuxmt/fs.h \
 /X/elks/include/linuxmt/wait.h /X/elks/include/linuxmt/vfs.h \
 /X/elks/include/arch/statfs.h /X/elks/include/linuxmt/ioctl.h \
 /X/elks/include/arch/ioctl.h /X/elks/include/linuxmt/pipe_fs_i.h \
 /X/elks/include/linuxmt/net.h /X/elks/include/linuxmt/socket.h \
 /X/elks/include/linuxmt/uio.h /X/elks/include/arch/bitops.h \
 /X/elks/include/linuxmt/romfs_fs_i.h \
 /X/elks/include/linuxmt/minix_fs_sb.h \
 /X/elks/include/linuxmt/romfs_fs_sb.h \
 /X/elks/include/linuxmt/elksfs_fs_sb.h /X/elks/include/linuxmt/time.h \
 /X/elks/include/linuxmt/signal.h /X/elks/include/linuxmt/ntty.h \
 /X/elks/include/linuxmt/termios.h /X/elks/include/linuxmt/chqueue.h \
 /X/elks/include/arch/system.h /X/elks/include/linuxmt/timex.h \
 /X/elks/include/arch/param.h /X/elks/include/linuxmt/errno.h \
 /X/elks/include/linuxmt/kernel.h /X/elks/include/linuxmt/string.h \
 /X/elks/include/arch/string.h



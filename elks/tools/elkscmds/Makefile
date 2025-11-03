# elkscmds - ELKS commands for host use
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

CLEANME 	= ../bin/eman ../bin/hd ../bin/disasm ../bin/nm86

DEPEND  	=

DISTFILES	=

NOINDENT	=

#########################################################################
# Include standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Objects to be compiled.

EMAN =      $(TOPDIR)/elkscmd/sys_utils/man.c
HD =        $(TOPDIR)/elkscmd/misc_utils/hd.c
DEBUG_DIR = $(TOPDIR)/elkscmd/debug
DISASM =    $(DEBUG_DIR)/disasm.c $(DEBUG_DIR)/dis.c $(TOPDIR)/libc/debug/syms.c
NM86  =     $(DEBUG_DIR)/nm86.c $(TOPDIR)/libc/debug/syms.c

#########################################################################
# Commands.

all: $(CLEANME)

../bin/eman: $(EMAN)
	$(CC) -o $@ $(CFLAGS) -D_PATH_MANPAGES=\"$(TOPDIR)/elkscmd/man\" $^

../bin/hd: $(HD)
	$(CC) -o $@ $(CFLAGS) -D__far= $^

../bin/disasm: $(DISASM)
	$(CC) -o $@ $(CFLAGS) -I. -D__far= -Wno-int-to-void-pointer-cast -Wno-format $(DISASM)

../bin/nm86: $(NM86)
	$(CC) -o $@ $(CFLAGS) -I. -D__far= $^

#########################################################################
### Dependencies:

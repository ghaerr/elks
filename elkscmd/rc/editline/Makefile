##  $Revision$
##
##  Unix makefile for editline library.
##
###############################################################################
##
## Include standard packaging commands.

MIN_BIN 	= 
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	= 
MIN_USR_LIB	= 
MIN_USR_SBIN	= 

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	= 
NET_USR_LIB	= 
NET_USR_SBIN	= 

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= 
STD_USR_LIB	= 
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################
##
##  Set your options:
##	-DANSI_ARROWS		ANSI arrows keys work like emacs.
##	-DHAVE_STDLIB		Have <stdlib.h>.
##	-DHAVE_TCGETATTR	Have tcgetattr(), tcsetattr().
##	-DHIDE			Make static functions static (non debug).
##	-DHIST_SIZE=n		History size.
##	-DNEED_STRDUP		Don't have strdup().
##	-DUNIQUE_HISTORY	Don't save command if same as last one.
##	-DUSE_DIRENT		Use <dirent.h>, not <sys/dir.h>?
##	-DUSE_TERMCAP		Use the termcap library for terminal size
##				see LDFLAGS, below, if you set this.
##	-DNEED_PERROR		Don't have perror() (used in testit)
DEFS	= -DANSI_ARROWS -DHAVE_TCGETATTR -DHIDE -DUSE_DIRENT -DSYS_UNIX -DUNIQUE_HISTORY -DHIDE #-DUSE_TERMCAP

##  Set your C compiler:
WARN	= -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wwrite-strings \
	  -Wunused -Wcomment -Wswitch
CC	= gcc -ansi $(WARN)
#CC	= cc
CFLAGS	= $(DEFS) -O2 -m486 -fomit-frame-pointer 

##  If you have -DUSE_TERMCAP, set this as appropriate:
#LDFLAGS = -ltermlib
#LDFLAGS = -ltermcap

##  Set ranlib as appropriate:
RANLIB	= ranlib
#RANLIB	= echo

##  End of configuration.

SOURCES	= editline.c complete.c sysunix.c
OBJECTS	= editline.o complete.o sysunix.o
SHARFILES =	README Makefile editline.3 editline.h unix.h editline.c \
		complete.c sysunix.c testit.c \
		Make.os9 os9.h sysos9.c

all:		libedit.a

testit:		testit.c libedit.a
	$(CC) $(CFLAGS) -o testit testit.c libedit.a $(LDFLAGS)

shar:		$(SHARFILES)
	shar $(SHARFILES) >shar

clean:
	rm -f *.[oa] testit foo core tags lint lint.all a.out shar

lint:		testit
	lint -a -b -u -x $(DEFS) $(SOURCES) testit.c >lint.all
	sed -e '/warning: function prototype not in scope/d' \
		-e '/warning: old style argument declaration/'d \
		-e '/mix of old and new style function declaration/'d \
		<lint.all >lint

libedit.a:	$(OBJECTS)
	@rm -f $@
	ar r $@ $(OBJECTS)
	$(RANLIB) $@

$(OBJECTS):	editline.h

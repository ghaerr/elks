# Makefile for rc.

BASEDIR=..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= 
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	= 
MIN_USR_LIB	= 
MIN_USR_MAN	= 
MIN_USR_SBIN	= 

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	= 
NET_USR_LIB	= 
NET_USR_MAN	= 
NET_USR_SBIN	= 

STD_BIN 	= rc
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= 
STD_USR_LIB	= 
STD_USR_MAN	= rc.1
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################
#
# Please check the configuration parameters in config.h (and if you want
# to make sure, the definitions in proto.h) to make sure they are correct
# for your system.

SHELL=/bin/sh

# Uncomment this line if you have defined the NOEXECVE macro in config.h
EXECVE=execve.o

# Define this macro if you wish to extend rc via locally-defined builtins.
# An interface is provided in addon.[ch]. Note that the author does not
# endorse any such extensions, rather hopes that this way rc will become
# useful to more people.
#ADDON=addon.o

# Use an ANSI compiler (or at least one that groks prototypes and void *):
#CC=bcc
#CFLAGS=-O2 -m486 -fomit-frame-pointer -DSYSVR4
#CFLAGS=
#LDFLAGS=-static

# You may substitute "bison -y" for yacc. (You want to choose the one that
# makes a smaller y.tab.c. Also see the README about Sun's yacc.)
YACC=yacc

OBJS=$(ADDON) builtins.o except.o exec.o $(EXECVE) fn.o footobar.o getopt.o \
	glob.o glom.o hash.o heredoc.o input.o lex.o list.o main.o match.o \
	nalloc.o open.o print.o redir.o sigmsgs.o signal.o status.o tree.o \
	utils.o var.o version.o wait.o walk.o which.o y.tab.o libedit.a

# If rc is compiled with READLINE defined, you must supply the correct
# arguments to ld on this line. Typically this would be something like:
#
#	$(CC) -o $@ $(OBJS) -lreadline -ltermcap

all: rc

rc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

sigmsgs.c: mksignal
	sh mksignal /usr/include/sys/signal.h

y.tab.c: parse.y
	$(YACC) -d parse.y

config.h: config.h-dist
	cp config.h-dist config.h

trip: rc
	./rc -p < trip.rc

clean: force
	rm -f core *.o *.tab.* rc

history: force
	make -C history CC="$(CC)" $(HISTORYMAKEFLAGS)

force:

# dependencies:

$(OBJS): config.h
sigmsgs.h: sigmsgs.c
lex.o y.tab.o: y.tab.c
builtins.c fn.c status.c hash.c: sigmsgs.h

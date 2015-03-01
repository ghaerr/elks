# Makefile for Berkeley yacc.

BASEDIR = ..
    
include ../Make.defs

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

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= yacc
STD_USR_LIB	= 
STD_USR_MAN	= 
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

BINDIR	      = /usr/bin

HDRS	      = defs.h

CFLAGS	      = $(CFLBASE) -DNDEBUG -D_POSIX_SOURCE

LDFLAGS	      =

LIBS	      =

LINKER	      = $(CC)

MAKEFILE      = Makefile

OBJS	      = closure.o \
		error.o \
		lalr.o \
		lr0.o \
		main.o \
		mkpar.o \
		output.o \
		reader.o \
		skeleton.o \
		symtab.o \
		verbose.o \
		warshall.o \
		../lib/mktemp.o

PRINT	      = pr -f -l88

PROGRAM	      = yacc

SRCS	      = closure.c \
		error.c \
		lalr.c \
		lr0.c \
		main.c \
		mkpar.c \
		output.c \
		reader.c \
		skeleton.c \
		symtab.c \
		verbose.c \
		warshall.c

all:		$(PROGRAM)

$(PROGRAM):     $(OBJS) $(LIBS)
		$(LINKER) $(LDFLAGS) -o $(PROGRAM) $(OBJS) $(LIBS)

max_rfs: install

rfs: install

min_rfs:

net_rfs:

smin_rfs: all

install: $(PROGRAM)
	cp -p $(PROGRAM) $(TARGET_MNT)/usr/bin/$(PROGRAM)

clean:;		rm -f $(OBJS) $(PROGRAM) core

#depend:;	@mkmf -f $(MAKEFILE) PROGRAM=$(PROGRAM) DEST=$(DEST)
#
#index:;	@ctags -wx $(HDRS) $(SRCS)
#
#listing:;	@$(PRINT) Makefile $(HDRS) $(SRCS) | lpr
#
#lint:;		@lint $(SRCS)
#
#program:        $(PROGRAM)
#
#tags:           $(HDRS) $(SRCS); @ctags $(HDRS) $(SRCS)

###
closure.o: defs.h
error.o: defs.h
lalr.o: defs.h
lr0.o: defs.h
main.o: defs.h
mkpar.o: defs.h
output.o: defs.h
reader.o: defs.h
skeleton.o: defs.h
symtab.o: defs.h
verbose.o: defs.h
warshall.o: defs.h

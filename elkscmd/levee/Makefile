# makefile for Levee

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

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= lev
STD_USR_LIB	= 
STD_USR_MAN	= 
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

CFLAGS = -O -0 -DSIZE=16000L

xCC=gcc
xCFLAGS=-g


OBJS = blockio.o display.o editcor.o exec.o find.o \
    unixcall.o globals.o insert.o main.o misc.o \
    modify.o move.o ucsd.o undo.o wildargs.o

all: lev

lev: $(OBJS)
	$(CC) $(LDFLAGS) -o lev $(OBJS)

rfs: install

net_rfs: install

min_rfs: install

smin_rfs:

install: lev
	cp -p lev $(TARGET_MNT)/bin/vi

clean:
	rm -f *.o lev

# Dependencies

blockio.o    : levee.h extern.h blockio.c
display.o    : levee.h extern.h termcap.i display.c
editcor.o    : levee.h extern.h editcor.c
exec.o       : levee.h extern.h exec.c
find.o       : levee.h extern.h grep.h find.c
globals.o    : levee.h globals.c
insert.o     : levee.h extern.h insert.c
main.o       : levee.h extern.h main.c
misc.o       : levee.h extern.h misc.c
modify.o     : levee.h extern.h grep.h modify.c
move.o       : levee.h extern.h move.c
rmxcall.o    : levee.h rmxcall.c
ucsd.o       : levee.h extern.h ucsd.c
undo.o       : levee.h extern.h undo.c

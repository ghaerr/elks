#########################################################################
# Relative path to base directory.

BASEDIR 	= ../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= N

CLEANDEP	= 

CLEANME 	= local-curses.h lxdialog

DEPEND  	= local-curses.h

DISTFILES	= BIG.FAT.WARNING

NOINDENT	= 

#########################################################################
# Include the standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Where are the curses libraries?

ifeq (/usr/include/ncurses/ncurses.h, $(wildcard /usr/include/ncurses/ncurses.h))
	CURSES_INC = -I/usr/include/ncurses
	CURSES_LOC = "<ncurses.h>"
else
ifeq (/usr/include/ncurses/curses.h, $(wildcard /usr/include/ncurses/curses.h))
	CURSES_INC = -I/usr/include/ncurses
	CURSES_LOC = "<ncurses/curses.h>"
else
ifeq (/usr/include/ncurses.h, $(wildcard /usr/include/ncurses.h))
	CURSES_INC =
	CURSES_LOC = "<ncurses.h>"
else
	CURSES_INC =
	CURSES_LOC = "<curses.h>"
endif
endif
endif

#########################################################################
# Local commands.

CC = gcc
CPP = gcc -E
OPTIM = -O2 -Wall -fomit-frame-pointer

CFLAGS = $(OPTIM) -DLOCALE $(CURSES_INC)
LDFLAGS = -s -L .
LDLIBS = -lncurses

#########################################################################
# What do we compile?

OBJS = checklist.o menubox.o textbox.o yesno.o inputbox.o \
       util.o lxdialog.o msgbox.o
SRCS = $(OBJS:.o=.c)

#########################################################################
# Available commands.

all:	ncurses lxdialog

lxdialog: $(OBJS)

#########################################################################
# Write a custom header for curses.

local-curses.h:
	printf > local-curses.h '%s\n' \
		'/* Include curses as available on this system.' \
		' * Automatically created during compilation.' \
		' * DO NOT CHANGE.' \
		' */' ''
	printf '#include %s\n\n' $(CURSES_LOC) >> local-curses.h

#########################################################################
# Check that ncurses is available.

ncurses: local-curses.h
	@x=`find /lib/ /usr/lib/ /usr/local/lib/ -maxdepth 1 -name 'libncurses.*'` ;\
	if [ ! "$$x" ]; then \
		echo -e "\007" ;\
		echo ">> Unable to find the Ncurses libraries." ;\
		echo ">>" ;\
		echo ">> You must have Ncurses installed in order" ;\
		echo ">> to use 'make menuconfig'" ;\
		echo ;\
		exit 1 ;\
	fi

#########################################################################
### Dependencies:

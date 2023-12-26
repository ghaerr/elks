#########################################################################
# Where are the curses libraries?

ifeq ($(shell pkg-config --exists ncurses ; echo $$?), 0)
	CURSES_INC := $(shell pkg-config --cflags-only-I ncurses)
	CURSES_LOC := "<ncurses.h>"
	CURSES_LIBS := $(shell pkg-config --libs ncurses)
else ifeq (/usr/include/ncurses/ncurses.h, $(wildcard /usr/include/ncurses/ncurses.h))
	CURSES_INC = -I/usr/include/ncurses
	CURSES_LOC = "<ncurses.h>"
else ifeq (/usr/include/ncurses/curses.h, $(wildcard /usr/include/ncurses/curses.h))
	CURSES_INC = -I/usr/include/ncurses
	CURSES_LOC = "<ncurses/curses.h>"
else ifeq (/usr/include/ncurses.h, $(wildcard /usr/include/ncurses.h))
	CURSES_INC =
	CURSES_LOC = "<ncurses.h>"
else
	CURSES_INC =
	CURSES_LOC = "<curses.h>"
endif
ifeq ($(CURSES_LIBS), )
	CURSES_LIBS = -lncurses
endif

#########################################################################
# Local commands.

CC = gcc
CFLAGS = -std=c99 -O2 -Wall -pedantic -DLOCALE $(CURSES_INC)

LDFLAGS = -s
LDLIBS = $(CURSES_LIBS)

#########################################################################
# What do we compile?

PGM = lxdialog

OBJS = checklist.o menubox.o textbox.o yesno.o inputbox.o \
       util.o lxdialog.o msgbox.o

SRCS = $(OBJS:.o=.c)

#########################################################################
# Available commands.

.PHONY: all clean

all: $(PGM)

clean:
	rm -fv $(OBJS) $(PGM) local-curses.h

$(OBJS): local-curses.h

$(PGM): $(OBJS)

#########################################################################
# Check that ncurses is available.
# Write a custom header for curses.

local-curses.h:
	@if ! pkg-config --exists ncurses && \
		! find /lib/ /lib64/ /usr/lib/ /usr/lib64/ /usr/local/lib/ /usr/local/lib64/ /Library/Developer/CommandLineTools/SDKs/MacOSX13.3.sdk/usr/lib/ -maxdepth 2 -name 'libncurses.*' | grep . ; then \
		echo -e "\007" ;\
		echo ">> Unable to find the Ncurses libraries." ;\
		echo ">>" ;\
		echo ">> You must have Ncurses installed in order" ;\
		echo ">> to use 'make menuconfig'" ;\
		echo ;\
		exit 1 ;\
	fi
	printf > local-curses.h '%s\n' \
		'/* Include curses as available on this system.' \
		' * Automatically created during compilation.' \
		' * DO NOT CHANGE.' \
		' */' ''
	printf '#include %s\n\n' $(CURSES_LOC) >> local-curses.h

#########################################################################

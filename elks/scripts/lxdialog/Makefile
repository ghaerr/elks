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

checklist.o: checklist.c dialog.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/ctype.h /usr/include/stdlib.h \
 /usr/include/alloca.h /usr/include/string.h local-curses.h \
 /usr/include/ncurses.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/ncurses/unctrl.h \
 /usr/include/ncurses/curses.h


inputbox.o: inputbox.c dialog.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/ctype.h /usr/include/stdlib.h \
 /usr/include/alloca.h /usr/include/string.h local-curses.h \
 /usr/include/ncurses.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/ncurses/unctrl.h \
 /usr/include/ncurses/curses.h


lxdialog.o: lxdialog.c dialog.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/ctype.h /usr/include/stdlib.h \
 /usr/include/alloca.h /usr/include/string.h local-curses.h \
 /usr/include/ncurses.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/ncurses/unctrl.h \
 /usr/include/ncurses/curses.h


menubox.o: menubox.c dialog.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/ctype.h /usr/include/stdlib.h \
 /usr/include/alloca.h /usr/include/string.h local-curses.h \
 /usr/include/ncurses.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/ncurses/unctrl.h \
 /usr/include/ncurses/curses.h


msgbox.o: msgbox.c dialog.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/ctype.h /usr/include/stdlib.h \
 /usr/include/alloca.h /usr/include/string.h local-curses.h \
 /usr/include/ncurses.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/ncurses/unctrl.h \
 /usr/include/ncurses/curses.h


textbox.o: textbox.c dialog.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/ctype.h /usr/include/stdlib.h \
 /usr/include/alloca.h /usr/include/string.h local-curses.h \
 /usr/include/ncurses.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/ncurses/unctrl.h \
 /usr/include/ncurses/curses.h


util.o: util.c dialog.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/ctype.h /usr/include/stdlib.h \
 /usr/include/alloca.h /usr/include/string.h local-curses.h \
 /usr/include/ncurses.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/ncurses/unctrl.h \
 /usr/include/ncurses/curses.h colors.h


yesno.o: yesno.c dialog.h /usr/include/sys/types.h \
 /usr/include/features.h /usr/include/sys/cdefs.h \
 /usr/include/gnu/stubs.h /usr/include/bits/types.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h \
 /usr/include/time.h /usr/include/endian.h /usr/include/bits/endian.h \
 /usr/include/sys/select.h /usr/include/bits/select.h \
 /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h \
 /usr/include/fcntl.h /usr/include/bits/fcntl.h /usr/include/unistd.h \
 /usr/include/bits/posix_opt.h /usr/include/bits/confname.h \
 /usr/include/getopt.h /usr/include/ctype.h /usr/include/stdlib.h \
 /usr/include/alloca.h /usr/include/string.h local-curses.h \
 /usr/include/ncurses.h /usr/include/stdio.h \
 /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h \
 /usr/include/libio.h /usr/include/_G_config.h \
 /usr/include/bits/stdio_lim.h /usr/include/ncurses/unctrl.h \
 /usr/include/ncurses/curses.h



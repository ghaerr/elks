CC = gcc
CPP = gcc -E
OPTIM = -O2 -Wall -fomit-frame-pointer

CFLAGS = $(OPTIM) -DLOCALE 
LDFLAGS = -s -L .
LDLIBS = -lncurses

ifeq (/usr/include/ncurses/ncurses.h, $(wildcard /usr/include/ncurses/ncurses.h))
        CFLAGS += -I/usr/include/ncurses -DCURSES_LOC="<ncurses.h>"
else
ifeq (/usr/include/ncurses/curses.h, $(wildcard /usr/include/ncurses/curses.h))
        CFLAGS += -I/usr/include/ncurses -DCURSES_LOC="<ncurses/curses.h>"
else
ifeq (/usr/include/ncurses.h, $(wildcard /usr/include/ncurses.h))
        CFLAGS += -DCURSES_LOC="<ncurses.h>"
else
	CFLAGS += -DCURSES_LOC="<curses.h>"
endif
endif
endif


OBJS = checklist.o menubox.o textbox.o yesno.o inputbox.o \
       util.o lxdialog.o msgbox.o
SRCS = $(OBJS:.o=.c)


all: ncurses lxdialog

lxdialog: $(OBJS)

ncurses:
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

clean:
	rm -f core *.o *~ lxdialog

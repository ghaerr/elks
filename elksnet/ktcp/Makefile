# Makefile for ktcp
BASEDIR=..
include $(BASEDIR)/Make.defs

SHELL		= /bin/sh

CC		= bcc

#BASE_CFLAGS	= -g
BASE_CFLAGS	= -ansi -O -0

LOCALFLAGS	= -D__KERNEL__

CFLAGS		= $(BASE_CFLAGS) $(LOCALFLAGS)

CFILES		= ktcp.c slip.c ip.c icmp.c tcp.c tcp_cb.c tcp_output.c \
		  timer.c tcpdev.c netconf.c vjhc.c

OBJ		= $(CFILES:.c=.o)

##############################################################################

all:	ktcp

ktcp:	$(OBJ)
	$(CC) $(OPT) -s -o ktcp $(OBJ) $(BASELIBS)

lint:
	@for FILE in *.c ; do \
		echo '===>' "$${FILE}" ; \
		splint -weak -D__KERNEL__ $(LOCALFLAGS) "$${FILE}" \
			2>&1 > "$${FILE}.lint" ; \
	done

clean: 
	rm -f *~ *.o ktcp core 

dep:
	makedepend $(CFILES)

#######
# EOF #
#######

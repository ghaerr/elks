# Makefile for ktcp
BASEDIR=..
include $(BASEDIR)/Make.defs

SHELL		= /bin/sh

CC		= bcc

#BASE_CFLAGS	= -g
BASE_CFLAGS	= -ansi -O -0

LOCALFLAGS	= 

CFLAGS		= $(BASE_CFLAGS) $(LOCALFLAGS) -I$(ELKS_DIR)/include

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

max_rfs: all
#        cp -p ktcp $(TARGET_MNT)/bin

rfs: all
#        cp -p ktcp $(TARGET_MNT)/bin

net_rfs: all
#        cp -p ktcp $(TARGET_MNT)/bin

min_rfs:

smin_rfs:


clean: 
	rm -f *~ *.o ktcp core 

dep:
	makedepend $(CFILES)

#######
# EOF #
#######

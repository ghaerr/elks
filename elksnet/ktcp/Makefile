# Makefile for ktcp

SHELL		= /bin/sh

CC		= bcc

#BASE_CFLAGS	= -c -g
BASE_CFLAGS	= -c -O 

LOCALFLAGS	= -D__KERNEL__ -I../../elks-test/include -ansi

CFLAGS		= $(BASE_CFLAGS) $(LOCALFLAGS)

CFILES		= ktcp.c slip.c ip.c icmp.c tcp.c tcp_cb.c tcp_output.c \
		  timer.c tcpdev.c netconf.c

OBJ		= $(CFILES:.c=.o)

##############################################################################

all:	ktcp

.c.o:
	$(CC) $(CFLAGS) -o $@ $<

ktcp:	$(OBJ)
	$(CC) $(OPT) -o ktcp $(OBJ) $(BASELIBS)

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

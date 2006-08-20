# Makefile for m4

BASEDIR = ..

include $(BASEDIR)/Make.defs

LOCALFLAGS =

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
STD_USR_BIN	= 
STD_USR_LIB	= 
STD_USR_MAN	= 
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

OBJS=	eval.o expr.o look.o main.o misc.o serv.o

CLEANFILES= $(OBJS)

all:	m4

m4:	$(OBJS)
	$(CC) $(CFLAGS) -o m4 $(OBJS) $(LIBS)

rfs: install

min_rfs: install

net_rfs: install

smin_rfs:

install: m4
	cp -p m4 $(TARGET_MNT)/usr/bin
	compress < m4.1 > $(TARGET_MNT)/usr/man/man1/m4.1.Z

test: m4
	m4 < test.m4 > test_host_m4.txt
	elksemu ./m4 < test.m4 > test_elks_m4.txt
	diff -u test_host_m4.txt test_elks_m4.txt

clean:
	rm -f core m4 $(CLEANFILES)


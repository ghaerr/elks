BASEDIR=../../..

include $(BASEDIR)/Make.defs

###############################################################################
#
# Include standard packaging commands.

MIN_BIN 	= 
MIN_LIB 	= 
MIN_SBIN	= 
MIN_USR_BIN	= 
MIN_USR_LIB	= 
MIN_USR_SBIN	= 

NET_BIN 	= 
NET_LIB 	= 
NET_SBIN	= 
NET_USR_BIN	= 
NET_USR_LIB	= 
NET_USR_SBIN	= 

STD_BIN 	= 
STD_LIB 	= 
STD_SBIN	= 
STD_USR_BIN	= 
STD_USR_LIB	= 
STD_USR_SBIN	= echoserver_un echoserver_in echoclient_un echoclient_in

include $(BASEDIR)/Make.rules

###############################################################################

PRGS=echoserver_un echoserver_in echoclient_un echoclient_in

all: $(PRGS)


smin_rfs:

min_rfs:

rfs:

max_rfs:

net_rfs:


install: all
	cp -p $(PRGS) $(TARGET_MNT)/bin

echoserver_un: echoserver_un.o

echoclient_un: echoclient_un.o

echoserver_in: echoserver_in.o

echoclient_in: echoclient_in.o


clean:
	rm -f core *.o $(PRGS)

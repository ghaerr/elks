###############################################################################
#
# Include standard packaging commands.

BASEDIR 	= ../..

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
STD_USR_SBIN	= 

include $(BASEDIR)/Make.rules

###############################################################################

CC=gcc -g -O

all: history

history: history.c
	$(CC) -o history history.c
	@echo 'Now install "history" as -, --, -p and --p in your bin directory'
	@echo '(for example, by making them links to the same file)'

clean:
	rm -rf core history

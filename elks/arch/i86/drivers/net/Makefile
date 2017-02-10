# Note that the standard Makefile rules and defines have been moved to
# the file below.

BASEDIR 	= ../../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	= 

NOINDENT	= 

#########################################################################
# Include the standard rules.

include $(BASEDIR)/Makefile-rules

#########################################################################
### Dependencies:

ifeq ($(CONFIG_ETH), y)
OBJS = net_drv.o ne2k-main.o
else
OBJS = net_drv.o
endif

all: net_drv.a

net_drv.a: $(OBJS)
	ar rcs net_drv.a $(OBJS)

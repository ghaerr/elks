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
# Specific rules.

OBJS = net_drv.o
ifeq ($(CONFIG_ETH_NE2K), y)
OBJS += ne2k-mac.o ne2k-main.o
endif

all: net_drv.a

net_drv.a: $(OBJS)
	$(AR) $(ARFLAGS) net_drv.a $(OBJS)

#########################################################################
### Dependencies:

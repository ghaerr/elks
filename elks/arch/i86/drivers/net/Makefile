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
OBJS += ne2k-asm.o ne2k.o
endif
ifeq ($(CONFIG_ETH_WD), y)
OBJS += wd.o
endif
ifeq ($(CONFIG_ETH_EL3), y)
OBJS += el3-asm.o el3.o
endif

all: net_drv.a

net_drv.a: $(OBJS)
	$(AR) rcs net_drv.a $(OBJS)

#########################################################################
### Dependencies:

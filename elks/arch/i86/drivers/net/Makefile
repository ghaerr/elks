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
# Objects to be compiled.

OBJS =
OBJS += ne2k-asm.o ne2k.o           # CONFIG_ETH_NE2K
OBJS += wd.o                        # CONFIG_ETH_WD
OBJS += el3-asm.o el3.o             # CONFIG_ETH_EL3
OBJS += ultra.o                     # CONFIG_ETH_ULTRA
OBJS += pcnet.o pcnet-asm.o         # CONFIG_ETH_PCNET

all: net_drv.a

net_drv.a: $(OBJS)
	$(AR) rcs net_drv.a $(OBJS)

#########################################################################
### Dependencies:

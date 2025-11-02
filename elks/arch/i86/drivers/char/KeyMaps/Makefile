#
#
BASEDIR 	= ../../../../..

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
# Specify rules specific to this directory.

config.in:
	./mkcfg

keymaps.h: config.in

#########################################################################
# Standard commands.

#########################################################################
### Dependencies:

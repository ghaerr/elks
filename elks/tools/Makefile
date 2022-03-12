# Note that the standard Makefile rules and defines have been moved to
# the file below.

BASEDIR 	= ..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= N

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	= 

NOINDENT	= 

#########################################################################
# Include the standard rules.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Commands.
all:
	$(MAKE) -C mfs all
	$(MAKE) -C mfsck all
	$(MAKE) -C mkromfs all
	$(MAKE) -C setboot all
	$(MAKE) -C objdump86 all
	$(MAKE) -C elf2elks all
	$(MAKE) -C elks-compress all
	$(MAKE) -C eman all


#########################################################################
### Dependencies:

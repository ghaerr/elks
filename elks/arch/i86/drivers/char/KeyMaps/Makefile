# Note that the standard Makefile rules and defines have been moved to
# the file below.

BASEDIR 	= ../../../../..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

CLEANDEP	= 

CLEANME 	= Config.in keymaps.h

DEPEND  	= 

DISTFILES	= 

NOINDENT	= 

#########################################################################
# Include the standard rules.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Specify rules specific to this directory.

Config.in:
	./mkcfg

keymaps.h: Config.in

#########################################################################
# Specify distribution rule.

distdir:
	cp -pf Makefile mkcfg keys-*.h $(DISTDIR)/arch/i86/drivers/char/KeyMaps

#########################################################################
### Dependencies:

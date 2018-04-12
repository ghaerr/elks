#########################################################################
# Relative path to base directory.

BASEDIR 	= ..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= N

CLEANDEP	= 

CLEANME 	=

DEPEND  	= 

DISTFILES	= Configure Menuconfig elksspec index mkMake renvar \
		  setdir vars

NOINDENT	= 

#########################################################################
# Include the standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################

distdir:
	cp -pf Makefile Configure Menuconfig $(DISTDIR)/scripts
	cp -pf README.Menuconfig $(DISTDIR)/scripts
	cp -pf *.c *.h *.sh *.cc $(DISTDIR)/scripts
	mkdir $(DISTDIR)/scripts/lxdialog
	cp -pf lxdialog/Makefile $(DISTDIR)/scripts/lxdialog
	cp -pf lxdialog/BIG.FAT.WARNING $(DISTDIR)/scripts/lxdialog
	cp -pf lxdialog/*.c lxdialog/*.h $(DISTDIR)/scripts/lxdialog

#########################################################################
### Dependencies:

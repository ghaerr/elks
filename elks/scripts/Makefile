#########################################################################
# Relative path to base directory.

BASEDIR 	= ..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= N

CLEANDEP	= 

CLEANME 	= kconfig.tk tkparse

DEPEND  	= 

DISTFILES	= Configure Menuconfig elksspec index mkMake renvar \
		  setdir vars

NOINDENT	= 

#########################################################################
# Include the standard commands.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Local commands.

HOSTCFLAGS:=$(HOSTCFLAGS) -Wall
HEADER=header.tk
TAIL=tail.tk

# Previous versions always remade kconfig.tk because they always depended
# on soundscript.  This runs fairly fast, and I can't find all the
# Config.in files to depend on anyways.  So I'll force it to remake.

kconfig.tk: dummy

kconfig.tk: ${TOPDIR}/Makefile ${TOPDIR}/arch/${ARCH}/config.in \
		tkparse ${HEADER} ${TAIL}
	@if [ -f /usr/local/bin/wish ];	then \
		echo '#!'"/usr/local/bin/wish -f" > kconfig.tk; \
	else \
		echo '#!'"/usr/bin/wish -f" > kconfig.tk; \
	fi
	cat ${HEADER} >> kconfig.tk
	./tkparse < ../arch/${ARCH}/config.in >> kconfig.tk
	echo "set defaults \"arch/${ARCH}/defconfig\"" >> kconfig.tk
	cat ${TAIL} >> kconfig.tk
	chmod 755 kconfig.tk

tkparse: tkparse.o tkcond.o tkgen.o
	${HOSTCC} -o tkparse tkparse.o tkcond.o tkgen.o

tkparse.o: tkparse.c tkparse.h

tkcond.o: tkcond.c tkparse.h

tkgen.o: tkgen.c tkparse.h

tkparse.o tkcond.o tkgen.o:
	$(HOSTCC) $(HOSTCFLAGS) -c -o $@ $(@:.o=.c)

#########################################################################

distdir:
	cp -pf Makefile Configure Menuconfig $(DISTDIR)/scripts
	cp -pf README.Menuconfig $(DISTDIR)/scripts
	cp -pf *.c *.h *.tk *.sh *.cc $(DISTDIR)/scripts
	mkdir $(DISTDIR)/scripts/lxdialog
	cp -pf lxdialog/Makefile $(DISTDIR)/scripts/lxdialog
	cp -pf lxdialog/BIG.FAT.WARNING $(DISTDIR)/scripts/lxdialog
	cp -pf lxdialog/*.c lxdialog/*.h $(DISTDIR)/scripts/lxdialog

#########################################################################
### Dependencies:

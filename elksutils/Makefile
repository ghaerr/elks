# Makefile to make all programs in the ELKSutils package.
#
###############################################################################
# Compile everything present.

all:
	@for DIR in */ ; do \
	    if [ -f "$$DIR"/Makefile ]; then \
		make -C "$$DIR" all ;
	    fi ; \
	done

###############################################################################
# Clean everything up.

clean:
	@for DIR in */ ; do \
	    if [ -f "$$DIR"/Makefile ]; then \
		make -C "$$DIR" clean ;
	    fi ; \
	done

###############################################################################
# Compile named subsystems. Add entries as appropriate.

hdinst:
	@make -C hdinst all

###############################################################################
# EOF.

# Makefile to make all programs in the ELKSnet package.
#
###############################################################################
# Compile everything present.

all:
	@for DIR in */ ; do \
	    if [ "$$DIR" != "CVS/" ]; then \
		make -C "$$DIR" all ;
	    fi ; \
	done

###############################################################################
# Compile named subsystems. Add entries as appropriate.

ktcp:
	@make -C ktcp all

screen:
	@make -C screen all

###############################################################################
# EOF.

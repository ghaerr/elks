# Makefile to make all programs in the ELKSnet package.
#
###############################################################################
# Compile everything present.

DIRS = ktcp # screen

all: $(DIRS)
	@for DIR in $(DIRS) ; do \
	    make -C "$$DIR" all || exit 1; \
	done

###############################################################################
# Clean everything up.

clean:
	@for DIR in $(DIRS) ; do \
	    make -C "$$DIR" clean || exit 1; \
	done

###############################################################################
# Compile named subsystems. Add entries as appropriate.

ktcp:
	@make -C ktcp all

screen:
	@make -C screen all

###############################################################################
# EOF.

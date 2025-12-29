# Note that the standard Makefile rules and defines have been moved to
# the file below.

BASEDIR 	= ..

#########################################################################
# Define the variables required by the standard rules - see the standard
# rules file (below) for details of these variables.

USEBCC 		= Y

CLEANDEP	= 

CLEANME 	= 

DEPEND  	= 

DISTFILES	= 

NOINDENT	= main.c

#########################################################################
# Include the standard rules.

include $(BASEDIR)/Makefile-rules

#########################################################################
# Commands:

all:    main.o

main.o: main.c
	$(CC) $(CFLAGS) -fno-optimize-sibling-calls -fno-defer-pop -c -o main.o main.c


#########################################################################
### Dependencies:

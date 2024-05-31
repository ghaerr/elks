# Makefile of /libc/gcc module

# This module implements the infrastructure used by GCC-generated code for
# calling static initialization functions (constructors) & termination
# functions (destructors).  These may be registered by the application or by
# library routines, & are meant to be called by libc before entry to main( )
# and after exit( ) respectively.
#
# The gccint info page for GCC 6.3.0 says:
#
#	"Each list [of constructors or destructors] always begins with an
#	 ignored function pointer (which may hold 0, -1, or a count of the
#	 function pointers after it, depending on the environment).  This is
#	 followed by a series of zero or more function pointers to
#	 constructors (or destructors), followed by a function pointer
#	 containing zero. ...
#
#	"The best way to handle static constructors works only for object
#	 file formats which provide arbitrarily-named sections.  A section is
#	 set aside for a list of constructors, and another for a list of
#	 destructors.  Traditionally these are called `.ctors' and `.dtors'.
#	 ..."
#
# C functions can be marked as constructors or destructors in GCC using
# `__attribute__((constructor( )))' or `__attribute__((destructor( )))'.
#
# (Older versions of elks-libc used bcc's "autorun" mechanism to register
# constructors.  See github.com/jbruchon/dev86/blob/master/libc/Pre_main .)
#
# (Also, this module used to contain implementations of low-level routines
# __ashlsi3, __udivsi3, etc. used by GCC generated code.  It is now feasible
# to link programs with the real libgcc, so these routines are no longer
# needed.)	-- tkchia 20221204

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

# SRCS = divmodsi3.s ldivmod.s ashlsi3.s
SRCS = do-global-ctors.S do-global-dtors.S
OBJS = $(SRCS:.S=.o)

all: $(LIB)

$(LIB): $(OBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(OBJS)

clean:
	$(RM) *.[aod]

$(OBJS): $(SRCS)

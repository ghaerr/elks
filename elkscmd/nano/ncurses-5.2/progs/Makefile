# Generated automatically from Makefile.in by configure.
# $Id: Makefile.in,v 1.51 2000/10/14 22:19:14 tom Exp $
##############################################################################
# Copyright (c) 1998-2000 Free Software Foundation, Inc.                     #
#                                                                            #
# Permission is hereby granted, free of charge, to any person obtaining a    #
# copy of this software and associated documentation files (the "Software"), #
# to deal in the Software without restriction, including without limitation  #
# the rights to use, copy, modify, merge, publish, distribute, distribute    #
# with modifications, sublicense, and/or sell copies of the Software, and to #
# permit persons to whom the Software is furnished to do so, subject to the  #
# following conditions:                                                      #
#                                                                            #
# The above copyright notice and this permission notice shall be included in #
# all copies or substantial portions of the Software.                        #
#                                                                            #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR #
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   #
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    #
# THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      #
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    #
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        #
# DEALINGS IN THE SOFTWARE.                                                  #
#                                                                            #
# Except as contained in this notice, the name(s) of the above copyright     #
# holders shall not be used in advertising or otherwise to promote the sale, #
# use or other dealings in this Software without prior written               #
# authorization.                                                             #
##############################################################################
#
# Author: Thomas E. Dickey <dickey@clark.net> 1996,1997
#
# Makefile for ncurses source code.
#
# This makes the ncurses utility programs.
#
# The variable 'srcdir' refers to the source-distribution, and can be set with
# the configure script by "--srcdir=DIR".
#
# The rules are organized to produce the libraries for the configured models,
# and the programs with the configured default model.

# turn off _all_ suffix rules; we'll generate our own
.SUFFIXES:

SHELL		= /bin/sh
THIS		= Makefile

CF_MFLAGS 	= 

x		= 

MODEL		= ../objects
DESTDIR		= 
srcdir		= .
prefix		= /usr/local
exec_prefix	= ${prefix}
bindir		= ${exec_prefix}/bin
libdir		= ${exec_prefix}/lib
includedir	= ${prefix}/include
datadir		= ${prefix}/share

LIBTOOL		= 

INSTALL		= /usr/bin/install -c
INSTALL_PROGRAM	= ${INSTALL}
transform	= s,x,x,

AWK		= awk
LN_S		= ln -s

CC		= gcc
CPP		= gcc -E
CFLAGS		= -O2 

INCDIR		= $(srcdir)/../include
CPPFLAGS	= -I../progs -I$(srcdir)  -DNDEBUG -I. -I../include -I$(includedir) \
		  -DHAVE_CONFIG_H

CCFLAGS		= $(CPPFLAGS) $(CFLAGS)

CFLAGS_LIBTOOL	= $(CCFLAGS)
CFLAGS_NORMAL	= $(CCFLAGS)
CFLAGS_DEBUG	= $(CCFLAGS) -g -DTRACE
CFLAGS_PROFILE	= $(CCFLAGS) -pg
CFLAGS_SHARED	= $(CCFLAGS) unknown

CFLAGS_DEFAULT	= $(CFLAGS_NORMAL)

LD		= ld
LINK		=  $(LIBTOOL) $(CC)
LDFLAGS		=  \
		-L../lib -L$(libdir) -lncurses     

LDFLAGS_LIBTOOL	= $(LDFLAGS)
LDFLAGS_NORMAL	= $(LDFLAGS)
LDFLAGS_DEBUG	= $(LDFLAGS) -g
LDFLAGS_PROFILE	= $(LDFLAGS) -pg
LDFLAGS_SHARED	= $(LDFLAGS) 

LDFLAGS_DEFAULT	= $(LDFLAGS_NORMAL)

LINT		= 
LINT_OPTS	= 
LINT_LIBS	= -lncurses 

AUTO_SRC = \
	termsort.c \
	transform.h

PROGS = tic$x toe$x infocmp$x clear$x tput$x tset$x

TESTPROGS = mvcur$x tctest$x hardscroll$x hashmap$x

# Default library, for linking applications
DEPS_CURSES = ../lib/libncurses.a

################################################################################
all:		$(AUTO_SRC) $(PROGS)

sources:	$(AUTO_SRC)

install: 	$(AUTO_SRC) install.progs
uninstall: uninstall.progs

# this line simplifies the configure-script
libs \
install.libs \
uninstall.libs:

TRANSFORM = sed 's/$x$$//'|sed '$(transform)'|sed 's/$$/$x/'

actual_captoinfo = `echo captoinfo$x| $(TRANSFORM)`
actual_clear     = `echo clear$x|     $(TRANSFORM)`
actual_infocmp   = `echo infocmp$x|   $(TRANSFORM)`
actual_infotocap = `echo infotocap$x| $(TRANSFORM)`
actual_init      = `echo init$x|      $(TRANSFORM)`
actual_reset     = `echo reset$x|     $(TRANSFORM)`
actual_tic       = `echo tic$x|       $(TRANSFORM)`
actual_toe       = `echo toe$x|       $(TRANSFORM)`
actual_tput      = `echo tput$x|      $(TRANSFORM)`
actual_tset      = `echo tset$x|      $(TRANSFORM)`

transform.h :
	echo "#define PROG_CAPTOINFO \"$(actual_captoinfo)\"" >$@
	echo "#define PROG_INFOTOCAP \"$(actual_infotocap)\"" >>$@
	echo "#define PROG_RESET     \"$(actual_reset)\""     >>$@
	echo "#define PROG_INIT      \"$(actual_init)\""      >>$@

install.progs: $(AUTO_SRC) $(PROGS) $(DESTDIR)$(bindir)
	$(LIBTOOL) $(INSTALL_PROGRAM) tic$x     $(DESTDIR)$(bindir)/$(actual_tic)
	$(LIBTOOL) $(INSTALL_PROGRAM) toe$x     $(DESTDIR)$(bindir)/$(actual_toe)
	$(LIBTOOL) $(INSTALL_PROGRAM) infocmp$x $(DESTDIR)$(bindir)/$(actual_infocmp)
	$(LIBTOOL) $(INSTALL_PROGRAM) clear$x   $(DESTDIR)$(bindir)/$(actual_clear)
	$(LIBTOOL) $(INSTALL_PROGRAM) tput$x    $(DESTDIR)$(bindir)/$(actual_tput)
	$(LIBTOOL) $(INSTALL_PROGRAM) tset$x    $(DESTDIR)$(bindir)/$(actual_tset)
	@echo "linking $(actual_captoinfo) to $(actual_tic)"
	-@rm -f $(DESTDIR)$(bindir)/$(actual_captoinfo)
	(cd $(DESTDIR)$(bindir) && $(LN_S) $(actual_tic) $(actual_captoinfo))
	@echo "linking $(actual_infotocap) to $(actual_tic)"
	-@rm -f $(DESTDIR)$(bindir)/$(actual_infotocap)
	(cd $(DESTDIR)$(bindir) && $(LN_S) $(actual_tic) $(actual_infotocap))
	@echo "linking $(actual_reset) to $(actual_tset)"
	-@rm -f $(DESTDIR)$(bindir)/$(actual_reset)
	(cd $(DESTDIR)$(bindir) && $(LN_S) $(actual_tset) $(actual_reset))

uninstall.progs:
	-@$(LIBTOOL) rm -f $(DESTDIR)$(bindir)/$(actual_tic)
	-@$(LIBTOOL) rm -f $(DESTDIR)$(bindir)/$(actual_toe)
	-@$(LIBTOOL) rm -f $(DESTDIR)$(bindir)/$(actual_infocmp)
	-@$(LIBTOOL) rm -f $(DESTDIR)$(bindir)/$(actual_clear)
	-@$(LIBTOOL) rm -f $(DESTDIR)$(bindir)/$(actual_tput)
	-@$(LIBTOOL) rm -f $(DESTDIR)$(bindir)/$(actual_tset)
	-@rm -f $(DESTDIR)$(bindir)/$(actual_captoinfo)
	-@rm -f $(DESTDIR)$(bindir)/$(actual_infotocap)
	-@rm -f $(DESTDIR)$(bindir)/$(actual_reset)

$(DESTDIR)$(bindir) :
	$(srcdir)/../mkinstalldirs $@

#
# Utilities normally built by make all start here
#

DEPS_TIC = \
	$(MODEL)/tic.o \
	$(MODEL)/dump_entry.o

tic$x: $(DEPS_TIC) $(DEPS_CURSES) transform.h
	 $(LINK) $(DEPS_TIC) $(LDFLAGS_DEFAULT) -o $@

DEPS_TOE = \
	$(MODEL)/toe.o \
	$(MODEL)/dump_entry.o

toe$x: $(DEPS_TOE) $(DEPS_CURSES)
	 $(LINK) $(DEPS_TOE) $(LDFLAGS_DEFAULT) -o $@

DEPS_CLEAR = \
	$(MODEL)/clear.o

clear$x: $(DEPS_CLEAR) $(DEPS_CURSES)
	 $(LINK) $(DEPS_CLEAR) $(LDFLAGS_DEFAULT) -o $@

DEPS_TPUT = \
	$(MODEL)/tput.o

tput$x: $(DEPS_TPUT) $(DEPS_CURSES) transform.h
	 $(LINK) $(DEPS_TPUT) $(LDFLAGS_DEFAULT) -o $@

DEPS_INFOCMP = \
	$(MODEL)/infocmp.o \
	$(MODEL)/dump_entry.o

infocmp$x: $(DEPS_INFOCMP) $(DEPS_CURSES)
	 $(LINK) $(DEPS_INFOCMP) $(LDFLAGS_DEFAULT) -o $@

DEPS_TSET = \
	$(MODEL)/tset.o \
	$(MODEL)/dump_entry.o

tset$x: $(DEPS_TSET) $(DEPS_CURSES) transform.h
	 $(LINK) $(DEPS_TSET) $(LDFLAGS_DEFAULT) -o $@

termsort.c: $(srcdir)/MKtermsort.sh
	sh -c "$(srcdir)/MKtermsort.sh $(AWK) $(srcdir)/../include/Caps" >$@

#
# Utility productions start here
#

tags:
	ctags *.[ch]

#TAGS:
#	etags *.[ch]

mostlyclean ::
	-rm -f core tags TAGS *~ *.ln *.atac trace
	-rm -f $(TESTPROGS)

clean :: mostlyclean
	-rm -f $(AUTO_SRC)
	-rm -f $(PROGS)
	-rm -rf .libs

distclean :: clean
	-rm -f Makefile

realclean :: distclean

# These rules are used to allow "make -n" to work on a clean directory-tree
../include/hashsize.h \
../include/parametrized.h \
../include/term.h :
	cd ../include; $(MAKE) $(CF_MFLAGS)
$(DEPS_CURSES) :
	cd ../ncurses; $(MAKE) $(CF_MFLAGS)

lint:
	$(LINT) $(LINT_OPTS) $(CPPFLAGS) $(srcdir)/clear.c                          $(LINT_LIBS)
	$(LINT) $(LINT_OPTS) $(CPPFLAGS) $(srcdir)/infocmp.c $(srcdir)/dump_entry.c $(LINT_LIBS)
	$(LINT) $(LINT_OPTS) $(CPPFLAGS) $(srcdir)/tic.c     $(srcdir)/dump_entry.c $(LINT_LIBS)
	$(LINT) $(LINT_OPTS) $(CPPFLAGS) $(srcdir)/toe.c     $(srcdir)/dump_entry.c $(LINT_LIBS)
	$(LINT) $(LINT_OPTS) $(CPPFLAGS) $(srcdir)/tput.c                           $(LINT_LIBS)
	$(LINT) $(LINT_OPTS) $(CPPFLAGS) $(srcdir)/tset.c    $(srcdir)/dump_entry.c $(LINT_LIBS)

###############################################################################
# The remainder of this file is automatically generated during configuration
###############################################################################

# generated by mk-0th.awk

.SUFFIXES: .c .h .i
.c.i :
	$(CPP) $(CPPFLAGS) $< >$@
.h.i :
	$(CPP) $(CPPFLAGS) $< >$@

C_SRC = \
	$(srcdir)/clear.c \
	$(srcdir)/tic.c \
	$(srcdir)/toe.c \
	$(srcdir)/dump_entry.c \
	$(srcdir)/infocmp.c \
	$(srcdir)/tput.c \
	$(srcdir)/tset.c

lintlib :
	@echo no action needed

# generated by mk-1st.awk

NORMAL_OBJS = \
	../objects/clear.o \
	../objects/tic.o \
	../objects/toe.o \
	../objects/dump_entry.o \
	../objects/infocmp.o \
	../objects/tput.o \
	../objects/tset.o

$(NORMAL_OBJS) : ../include/ncurses_cfg.h ./progs.priv.h

mostlyclean::
	-rm -f $(NORMAL_OBJS)

clean ::
	-rm -f $(NORMAL_OBJS)

# generated by mk-2nd.awk


../objects/clear.o :	$(srcdir)/clear.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../progs/clear.c

../objects/tic.o :	$(srcdir)/tic.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			$(srcdir)/dump_entry.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../progs/tic.c

../objects/toe.o :	$(srcdir)/toe.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			$(srcdir)/dump_entry.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../progs/toe.c

../objects/dump_entry.o :	$(srcdir)/dump_entry.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			$(srcdir)/dump_entry.h \
			../include/parametrized.h \
			$(INCDIR)/capdefaults.c \
			termsort.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../progs/dump_entry.c

../objects/infocmp.o :	$(srcdir)/infocmp.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			$(srcdir)/dump_entry.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../progs/infocmp.c

../objects/tput.o :	$(srcdir)/tput.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../progs/tput.c

../objects/tset.o :	$(srcdir)/tset.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../progs/tset.c


# generated by mk-1st.awk

DEBUG_OBJS = \
	../obj_g/clear.o \
	../obj_g/tic.o \
	../obj_g/toe.o \
	../obj_g/dump_entry.o \
	../obj_g/infocmp.o \
	../obj_g/tput.o \
	../obj_g/tset.o

$(DEBUG_OBJS) : ../include/ncurses_cfg.h ./progs.priv.h

mostlyclean::
	-rm -f $(DEBUG_OBJS)

clean ::
	-rm -f $(DEBUG_OBJS)

# generated by mk-2nd.awk


../obj_g/clear.o :	$(srcdir)/clear.c \
			../include/term.h
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../progs/clear.c

../obj_g/tic.o :	$(srcdir)/tic.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			$(srcdir)/dump_entry.h
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../progs/tic.c

../obj_g/toe.o :	$(srcdir)/toe.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			$(srcdir)/dump_entry.h
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../progs/toe.c

../obj_g/dump_entry.o :	$(srcdir)/dump_entry.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			$(srcdir)/dump_entry.h \
			../include/parametrized.h \
			$(INCDIR)/capdefaults.c \
			termsort.c
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../progs/dump_entry.c

../obj_g/infocmp.o :	$(srcdir)/infocmp.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			$(srcdir)/dump_entry.h
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../progs/infocmp.c

../obj_g/tput.o :	$(srcdir)/tput.c \
			../include/term.h
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../progs/tput.c

../obj_g/tset.o :	$(srcdir)/tset.c \
			../include/term.h
	cd ../obj_g; $(LIBTOOL) $(CC) $(CFLAGS_DEBUG) -c ../progs/tset.c


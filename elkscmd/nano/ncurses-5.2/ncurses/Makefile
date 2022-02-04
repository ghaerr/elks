# Generated automatically from Makefile.in by configure.
# $Id: Makefile.in,v 1.71 2000/10/14 22:55:35 tom Exp $
##############################################################################
# Copyright (c) 1998,1999,2000 Free Software Foundation, Inc.                #
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
# This makes the following:
#	programs
#	includes
#	libraries (normal/debug/profile/shared)
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

x		= 

CF_MFLAGS 	= 


MODEL		= normal
DESTDIR		= 
srcdir		= .
prefix		= /usr/local
exec_prefix	= ${prefix}
libdir		= ${exec_prefix}/lib
includedir	= ${prefix}/include
datadir		= ${prefix}/share

LIBTOOL		= 

INSTALL		= /usr/bin/install -c
INSTALL_LIB	= /usr/bin/install -c -m 644
INSTALL_DATA	= ${INSTALL} -m 644

AR		= ar
AR_OPTS		= rv
AWK		= awk
LD		= ld
LN_S		= ln -s

#CC		= gcc
#CPP		= gcc -E
#CFLAGS		= -O3 
#RANLIB		= ranlib

include ../../Make.defs
CFLAGS += -fno-inline

INCDIR		= $(srcdir)/../include
CPPFLAGS	= -I../ncurses -I$(srcdir)  -DNDEBUG -I. -I../include -I$(includedir) \
		  -DHAVE_CONFIG_H

CCFLAGS		= $(CPPFLAGS) $(CFLAGS)

HOSTCC		= gcc
HOSTCCFLAGS	= -O2  $(CPPFLAGS)
HOSTLDFLAGS	=  

CFLAGS_LIBTOOL	= $(CCFLAGS)
CFLAGS_NORMAL	= $(CCFLAGS)
CFLAGS_DEBUG	= $(CCFLAGS) -g -DTRACE
CFLAGS_PROFILE	= $(CCFLAGS) -pg
CFLAGS_SHARED	= $(CCFLAGS) unknown

CFLAGS_DEFAULT	= $(CFLAGS_NORMAL)

LINK		= $(LIBTOOL) $(CC)
LDFLAGS		=   

SHLIB_DIRS	= -L../lib -L$(libdir)
SHLIB_LIST	= $(SHLIB_DIRS) 
TINFO_LIST	= $(SHLIB_DIRS) 

MK_SHARED_LIB	= echo unknown

NCURSES_MAJOR	= 5
NCURSES_MINOR	= 2
REL_VERSION	= 5.2
ABI_VERSION	= 5

LIBRARIES	=  ../lib/libncurses.a

LINT		= 
LINT_OPTS	= 
LINT_LIBS	= -lncurses 

FALLBACK_LIST	= ansi

AUTO_SRC = \
	../include/nomacros.h \
	./comp_captab.c \
	./expanded.c \
	./fallback.c \
	init_keytry.h \
	./lib_keyname.c \
	./lib_gen.c \
	./codes.c \
	./names.c \
	./unctrl.c

TEST_DEPS	= ../lib/libncurses.a
TEST_ARGS	= -L../lib -lncurses
TEST_LDFLAGS	=  $(TEST_ARGS)    

TEST_PROGS = \
	captoinfo$x \
	hardscroll$x \
	hashmap$x \
	lib_mvcur$x

base	= $(srcdir)/base
serial	= $(srcdir)/tty
tinfo	= $(srcdir)/tinfo
trace	= $(srcdir)/trace

################################################################################
all \
libs ::		$(AUTO_SRC) ../lib $(LIBRARIES)

sources:	$(AUTO_SRC)

$(DESTDIR)$(libdir) :
	$(srcdir)/../mkinstalldirs $@

../lib : ; mkdir $@

./fallback.c : $(tinfo)/MKfallback.sh
#	sh $(tinfo)/MKfallback.sh $(FALLBACK_LIST) >$@
	cp fallback.ansi.c fallback.c

./lib_gen.c : $(base)/MKlib_gen.sh ../include/curses.h
	sh $(base)/MKlib_gen.sh "$(CPP)" "$(AWK)" <../include/curses.h >$@

../include/nomacros.h : $(base)/MKlib_gen.sh ../include/curses.h
	-sh $(base)/MKlib_gen.sh "$(CPP)" "$(AWK)" <../include/curses.h | \
		fgrep undef >$@

init_keytry.h: make_keys$x $(tinfo)/keys.list
	./make_keys $(tinfo)/keys.list > $@

make_keys$x : \
		$(tinfo)/make_keys.c \
		./names.c
	$(HOSTCC) -o $@ $(HOSTCCFLAGS) $(tinfo)/make_keys.c $(HOSTLDFLAGS)

make_hash$x : \
		$(tinfo)/comp_hash.c \
		../include/hashsize.h
	$(HOSTCC) -o $@ $(HOSTCCFLAGS) -DMAIN_PROGRAM $(tinfo)/comp_hash.c $(HOSTLDFLAGS)

./expanded.c : $(serial)/MKexpanded.sh
	sh $(serial)/MKexpanded.sh "$(CPP)" $(CPPFLAGS) > $@

./comp_captab.c: \
		make_hash$x \
		../include/hashsize.h \
		$(tinfo)/MKcaptab.awk
	sh $(tinfo)/MKcaptab.awk $(AWK) $(srcdir)/../include/Caps > $@

./lib_keyname.c: $(tinfo)/keys.list $(base)/MKkeyname.awk
	$(AWK) -f $(base)/MKkeyname.awk $(tinfo)/keys.list > $@

./names.c ./codes.c: $(tinfo)/MKnames.awk
	$(AWK) -f $(tinfo)/MKnames.awk $(srcdir)/../include/Caps
	cat namehdr boolnames boolfnames numnames numfnames strnames strfnames nameftr >./names.c
	cat namehdr boolcodes numcodes strcodes codeftr >./codes.c
	-rm -f namehdr nameftr codeftr boolnames boolfnames boolcodes numnames numfnames numcodes strnames strfnames strcodes

./unctrl.c: $(base)/MKunctrl.awk
	echo | $(AWK) -f $(base)/MKunctrl.awk >$@

tags:
	ctags *.[ch]

#TAGS:
#	etags *.[ch]

mostlyclean ::
	-rm -f core tags TAGS *~ *.ln *.atac trace
	-rm -f $(TEST_PROGS)

clean :: mostlyclean
	-rm -f $(AUTO_SRC)
	-rm -f make_keys$x
	-rm -f make_hash$x
	-rm -rf .libs

distclean :: clean
	-rm -f Makefile

realclean :: distclean

# These rules are used to allow "make -n" to work on a clean directory-tree
../include/hashsize.h \
../include/parametrized.h \
../include/term.h :
	cd ../include; $(MAKE) $(CF_MFLAGS)

# These rules build test-programs for the modules that have test-drivers
test_progs : $(TEST_PROGS)

captoinfo$x : $(tinfo)/captoinfo.c $(TEST_DEPS)
	 $(CC) -o $@ $(CFLAGS_DEFAULT) -DMAIN $(tinfo)/captoinfo.c $(TEST_LDFLAGS)

hardscroll$x : $(serial)/hardscroll.c $(TEST_DEPS)
	 $(CC) -o $@ $(CFLAGS_DEFAULT) -DSCROLLDEBUG $(serial)/hardscroll.c $(TEST_LDFLAGS)

hashmap$x : $(serial)/hashmap.c $(TEST_DEPS)
	 $(CC) -o $@ $(CFLAGS_DEFAULT) -DHASHDEBUG $(serial)/hashmap.c $(TEST_LDFLAGS)

lib_mvcur$x : $(serial)/lib_mvcur.c $(TEST_DEPS) \
		../objects/dump_entry.o
	 $(CC) -o $@ $(CFLAGS_DEFAULT) -DNCURSES_TEST -I$(serial)/../../progs $(serial)/lib_mvcur.c ../objects/dump_entry.o $(TEST_LDFLAGS)

../objects/dump_entry.o:
	cd ../progs && $(MAKE) ../objects/dump_entry.o

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
	$(serial)/hardscroll.c \
	$(serial)/hashmap.c \
	$(base)/lib_addch.c \
	$(base)/lib_addstr.c \
	$(base)/lib_beep.c \
	$(base)/lib_bkgd.c \
	$(base)/lib_box.c \
	$(base)/lib_chgat.c \
	$(base)/lib_clear.c \
	$(base)/lib_clearok.c \
	$(base)/lib_clrbot.c \
	$(base)/lib_clreol.c \
	$(base)/lib_color.c \
	$(base)/lib_colorset.c \
	$(base)/lib_delch.c \
	$(base)/lib_delwin.c \
	$(base)/lib_echo.c \
	$(base)/lib_endwin.c \
	$(base)/lib_erase.c \
	$(base)/lib_flash.c \
	./lib_gen.c \
	$(base)/lib_getch.c \
	$(base)/lib_getstr.c \
	$(base)/lib_hline.c \
	$(base)/lib_immedok.c \
	$(base)/lib_inchstr.c \
	$(base)/lib_initscr.c \
	$(base)/lib_insch.c \
	$(base)/lib_insdel.c \
	$(base)/lib_insstr.c \
	$(base)/lib_instr.c \
	$(base)/lib_isendwin.c \
	$(base)/lib_leaveok.c \
	$(base)/lib_mouse.c \
	$(base)/lib_move.c \
	$(serial)/lib_mvcur.c \
	$(base)/lib_mvwin.c \
	$(base)/lib_newterm.c \
	$(base)/lib_newwin.c \
	$(base)/lib_nl.c \
	$(base)/lib_overlay.c \
	$(base)/lib_pad.c \
	$(base)/lib_printw.c \
	$(base)/lib_redrawln.c \
	$(base)/lib_refresh.c \
	$(base)/lib_restart.c \
	$(base)/lib_scanw.c \
	$(base)/lib_screen.c \
	$(base)/lib_scroll.c \
	$(base)/lib_scrollok.c \
	$(base)/lib_scrreg.c \
	$(base)/lib_set_term.c \
	$(base)/lib_slk.c \
	$(base)/lib_slkatr_set.c \
	$(base)/lib_slkatrof.c \
	$(base)/lib_slkatron.c \
	$(base)/lib_slkatrset.c \
	$(base)/lib_slkattr.c \
	$(base)/lib_slkclear.c \
	$(base)/lib_slkcolor.c \
	$(base)/lib_slkinit.c \
	$(base)/lib_slklab.c \
	$(base)/lib_slkrefr.c \
	$(base)/lib_slkset.c \
	$(base)/lib_slktouch.c \
	$(base)/lib_touch.c \
	$(trace)/lib_traceatr.c \
	$(trace)/lib_tracedmp.c \
	$(trace)/lib_tracemse.c \
	$(serial)/lib_tstp.c \
	$(base)/lib_ungetch.c \
	$(serial)/lib_vidattr.c \
	$(base)/lib_vline.c \
	$(base)/lib_wattroff.c \
	$(base)/lib_wattron.c \
	$(base)/lib_winch.c \
	$(base)/lib_window.c \
	$(base)/nc_panel.c \
	$(base)/safe_sprintf.c \
	$(serial)/tty_update.c \
	$(base)/memmove.c \
	$(base)/sigaction.c \
	$(base)/vsscanf.c \
	$(base)/define_key.c \
	./expanded.c \
	$(base)/keybound.c \
	$(base)/keyok.c \
	$(base)/lib_dft_fgbg.c \
	$(base)/lib_freeall.c \
	$(tinfo)/lib_print.c \
	$(base)/resizeterm.c \
	$(trace)/trace_xnames.c \
	$(base)/tries.c \
	$(base)/version.c \
	$(base)/wresize.c \
	$(tinfo)/access.c \
	$(tinfo)/add_tries.c \
	$(tinfo)/alloc_entry.c \
	$(tinfo)/alloc_ttype.c \
	$(tinfo)/captoinfo.c \
	./codes.c \
	./comp_captab.c \
	$(tinfo)/comp_error.c \
	$(tinfo)/comp_expand.c \
	$(tinfo)/comp_hash.c \
	$(tinfo)/comp_parse.c \
	$(tinfo)/comp_scan.c \
	$(tinfo)/doalloc.c \
	./fallback.c \
	$(tinfo)/free_ttype.c \
	$(tinfo)/getenv_num.c \
	$(tinfo)/home_terminfo.c \
	$(tinfo)/init_keytry.c \
	$(tinfo)/lib_acs.c \
	$(tinfo)/lib_baudrate.c \
	$(tinfo)/lib_cur_term.c \
	$(tinfo)/lib_data.c \
	$(tinfo)/lib_has_cap.c \
	$(tinfo)/lib_kernel.c \
	./lib_keyname.c \
	$(tinfo)/lib_longname.c \
	$(tinfo)/lib_napms.c \
	$(tinfo)/lib_options.c \
	$(tinfo)/lib_raw.c \
	$(tinfo)/lib_setup.c \
	$(tinfo)/lib_termcap.c \
	$(tinfo)/lib_termname.c \
	$(tinfo)/lib_tgoto.c \
	$(tinfo)/lib_ti.c \
	$(tinfo)/lib_tparm.c \
	$(tinfo)/lib_tputs.c \
	$(trace)/lib_trace.c \
	$(trace)/lib_tracebits.c \
	$(trace)/lib_tracechr.c \
	$(tinfo)/lib_ttyflags.c \
	$(serial)/lib_twait.c \
	$(tinfo)/name_match.c \
	./names.c \
	$(tinfo)/parse_entry.c \
	$(tinfo)/read_entry.c \
	$(tinfo)/read_termcap.c \
	$(tinfo)/setbuf.c \
	$(tinfo)/strings.c \
	$(trace)/trace_buf.c \
	$(trace)/trace_tries.c \
	./unctrl.c \
	$(tinfo)/write_entry.c

# Producing llib-lncurses is time-consuming, so there's no direct-dependency for
# it in the lintlib rule.  We'll only remove in the cleanest setup.
clean ::
	rm -f llib-lncurses.*

realclean ::
	rm -f llib-lncurses

llib-lncurses : $(C_SRC)
	cproto -a -l -DLINT $(CPPFLAGS) $(C_SRC) >$@

lintlib :
	$(srcdir)/../misc/makellib ncurses $(CPPFLAGS)
lint :
	$(LINT) $(LINT_OPTS) $(CPPFLAGS) $(C_SRC) $(LINT_LIBS)

# generated by mk-1st.awk

NORMAL_OBJS = \
	../objects/hardscroll.o \
	../objects/hashmap.o \
	../objects/lib_addch.o \
	../objects/lib_addstr.o \
	../objects/lib_beep.o \
	../objects/lib_bkgd.o \
	../objects/lib_box.o \
	../objects/lib_chgat.o \
	../objects/lib_clear.o \
	../objects/lib_clearok.o \
	../objects/lib_clrbot.o \
	../objects/lib_clreol.o \
	../objects/lib_color.o \
	../objects/lib_colorset.o \
	../objects/lib_delch.o \
	../objects/lib_delwin.o \
	../objects/lib_echo.o \
	../objects/lib_endwin.o \
	../objects/lib_erase.o \
	../objects/lib_flash.o \
	../objects/lib_gen.o \
	../objects/lib_getch.o \
	../objects/lib_getstr.o \
	../objects/lib_hline.o \
	../objects/lib_immedok.o \
	../objects/lib_inchstr.o \
	../objects/lib_initscr.o \
	../objects/lib_insch.o \
	../objects/lib_insdel.o \
	../objects/lib_insstr.o \
	../objects/lib_instr.o \
	../objects/lib_isendwin.o \
	../objects/lib_leaveok.o \
	../objects/lib_mouse.o \
	../objects/lib_move.o \
	../objects/lib_mvcur.o \
	../objects/lib_mvwin.o \
	../objects/lib_newterm.o \
	../objects/lib_newwin.o \
	../objects/lib_nl.o \
	../objects/lib_overlay.o \
	../objects/lib_pad.o \
	../objects/lib_printw.o \
	../objects/lib_redrawln.o \
	../objects/lib_refresh.o \
	../objects/lib_restart.o \
	../objects/lib_scanw.o \
	../objects/lib_screen.o \
	../objects/lib_scroll.o \
	../objects/lib_scrollok.o \
	../objects/lib_scrreg.o \
	../objects/lib_set_term.o \
	../objects/lib_slk.o \
	../objects/lib_slkatr_set.o \
	../objects/lib_slkatrof.o \
	../objects/lib_slkatron.o \
	../objects/lib_slkatrset.o \
	../objects/lib_slkattr.o \
	../objects/lib_slkclear.o \
	../objects/lib_slkcolor.o \
	../objects/lib_slkinit.o \
	../objects/lib_slklab.o \
	../objects/lib_slkrefr.o \
	../objects/lib_slkset.o \
	../objects/lib_slktouch.o \
	../objects/lib_touch.o \
	../objects/lib_tstp.o \
	../objects/lib_ungetch.o \
	../objects/lib_vidattr.o \
	../objects/lib_vline.o \
	../objects/lib_wattroff.o \
	../objects/lib_wattron.o \
	../objects/lib_winch.o \
	../objects/lib_window.o \
	../objects/nc_panel.o \
	../objects/safe_sprintf.o \
	../objects/tty_update.o \
	../objects/memmove.o \
	../objects/sigaction.o \
	../objects/vsscanf.o \
	../objects/access.o \
	../objects/add_tries.o \
	../objects/alloc_entry.o \
	../objects/alloc_ttype.o \
	../objects/captoinfo.o \
	../objects/codes.o \
	../objects/comp_captab.o \
	../objects/comp_error.o \
	../objects/comp_expand.o \
	../objects/comp_hash.o \
	../objects/comp_parse.o \
	../objects/comp_scan.o \
	../objects/doalloc.o \
	../objects/fallback.o \
	../objects/free_ttype.o \
	../objects/getenv_num.o \
	../objects/home_terminfo.o \
	../objects/init_keytry.o \
	../objects/lib_acs.o \
	../objects/lib_baudrate.o \
	../objects/lib_cur_term.o \
	../objects/lib_data.o \
	../objects/lib_has_cap.o \
	../objects/lib_kernel.o \
	../objects/lib_keyname.o \
	../objects/lib_longname.o \
	../objects/lib_napms.o \
	../objects/lib_options.o \
	../objects/lib_raw.o \
	../objects/lib_setup.o \
	../objects/lib_termcap.o \
	../objects/lib_termname.o \
	../objects/lib_tgoto.o \
	../objects/lib_ti.o \
	../objects/lib_tparm.o \
	../objects/lib_tputs.o \
	../objects/lib_trace.o \
	../objects/lib_ttyflags.o \
	../objects/lib_twait.o \
	../objects/name_match.o \
	../objects/names.o \
	../objects/parse_entry.o \
	../objects/read_entry.o \
	../objects/read_termcap.o \
	../objects/setbuf.o \
	../objects/strings.o \
	../objects/unctrl.o \
	../objects/write_entry.o

$(NORMAL_OBJS) : ../include/ncurses_cfg.h ./curses.priv.h

../lib/libncurses.a : $(NORMAL_OBJS)
	$(AR) $(AR_OPTS) $@ $?
	$(RANLIB) $@

install \
install.libs \
install.ncurses :: $(DESTDIR)$(libdir) ../lib/libncurses.a
	@echo installing ../lib/libncurses.a as $(DESTDIR)$(libdir)/libncurses.a
	$(INSTALL_DATA) ../lib/libncurses.a $(DESTDIR)$(libdir)/libncurses.a
	@echo linking libcurses.a to libncurses.a
	-@rm -f $(DESTDIR)$(libdir)/libcurses.a
	(cd $(DESTDIR)$(libdir) && $(LN_S) libncurses.a libcurses.a)
	$(RANLIB) $(DESTDIR)$(libdir)/libncurses.a

uninstall \
uninstall.libs \
uninstall.ncurses ::
	@echo uninstalling $(DESTDIR)$(libdir)/libncurses.a
	-@rm -f $(DESTDIR)$(libdir)/libncurses.a
	@echo linking libcurses.a to libncurses.a
	-@rm -f $(DESTDIR)$(libdir)/libcurses.a

clean ::
	-rm -f ../lib/libncurses.a

mostlyclean::
	-rm -f $(NORMAL_OBJS)

# generated by mk-2nd.awk


../objects/hardscroll.o :	$(serial)/hardscroll.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/hardscroll.c

../objects/hashmap.o :	$(serial)/hashmap.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/hashmap.c

../objects/lib_addch.o :	$(base)/lib_addch.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_addch.c

../objects/lib_addstr.o :	$(base)/lib_addstr.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_addstr.c

../objects/lib_beep.o :	$(base)/lib_beep.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_beep.c

../objects/lib_bkgd.o :	$(base)/lib_bkgd.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_bkgd.c

../objects/lib_box.o :	$(base)/lib_box.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_box.c

../objects/lib_chgat.o :	$(base)/lib_chgat.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_chgat.c

../objects/lib_clear.o :	$(base)/lib_clear.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_clear.c

../objects/lib_clearok.o :	$(base)/lib_clearok.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_clearok.c

../objects/lib_clrbot.o :	$(base)/lib_clrbot.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_clrbot.c

../objects/lib_clreol.o :	$(base)/lib_clreol.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_clreol.c

../objects/lib_color.o :	$(base)/lib_color.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_color.c

../objects/lib_colorset.o :	$(base)/lib_colorset.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_colorset.c

../objects/lib_delch.o :	$(base)/lib_delch.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_delch.c

../objects/lib_delwin.o :	$(base)/lib_delwin.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_delwin.c

../objects/lib_echo.o :	$(base)/lib_echo.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_echo.c

../objects/lib_endwin.o :	$(base)/lib_endwin.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_endwin.c

../objects/lib_erase.o :	$(base)/lib_erase.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_erase.c

../objects/lib_flash.o :	$(base)/lib_flash.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_flash.c

../objects/lib_gen.o :	./lib_gen.c \
			../include/curses.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/lib_gen.c

../objects/lib_getch.o :	$(base)/lib_getch.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_getch.c

../objects/lib_getstr.o :	$(base)/lib_getstr.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_getstr.c

../objects/lib_hline.o :	$(base)/lib_hline.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_hline.c

../objects/lib_immedok.o :	$(base)/lib_immedok.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_immedok.c

../objects/lib_inchstr.o :	$(base)/lib_inchstr.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_inchstr.c

../objects/lib_initscr.o :	$(base)/lib_initscr.c \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_initscr.c

../objects/lib_insch.o :	$(base)/lib_insch.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_insch.c

../objects/lib_insdel.o :	$(base)/lib_insdel.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_insdel.c

../objects/lib_insstr.o :	$(base)/lib_insstr.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_insstr.c

../objects/lib_instr.o :	$(base)/lib_instr.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_instr.c

../objects/lib_isendwin.o :	$(base)/lib_isendwin.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_isendwin.c

../objects/lib_leaveok.o :	$(base)/lib_leaveok.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_leaveok.c

../objects/lib_mouse.o :	$(base)/lib_mouse.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_mouse.c

../objects/lib_move.o :	$(base)/lib_move.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_move.c

../objects/lib_mvcur.o :	$(serial)/lib_mvcur.c \
			../include/term.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/lib_mvcur.c

../objects/lib_mvwin.o :	$(base)/lib_mvwin.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_mvwin.c

../objects/lib_newterm.o :	$(base)/lib_newterm.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_newterm.c

../objects/lib_newwin.o :	$(base)/lib_newwin.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_newwin.c

../objects/lib_nl.o :	$(base)/lib_nl.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_nl.c

../objects/lib_overlay.o :	$(base)/lib_overlay.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_overlay.c

../objects/lib_pad.o :	$(base)/lib_pad.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_pad.c

../objects/lib_printw.o :	$(base)/lib_printw.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_printw.c

../objects/lib_redrawln.o :	$(base)/lib_redrawln.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_redrawln.c

../objects/lib_refresh.o :	$(base)/lib_refresh.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_refresh.c

../objects/lib_restart.o :	$(base)/lib_restart.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_restart.c

../objects/lib_scanw.o :	$(base)/lib_scanw.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_scanw.c

../objects/lib_screen.o :	$(base)/lib_screen.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_screen.c

../objects/lib_scroll.o :	$(base)/lib_scroll.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_scroll.c

../objects/lib_scrollok.o :	$(base)/lib_scrollok.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_scrollok.c

../objects/lib_scrreg.o :	$(base)/lib_scrreg.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_scrreg.c

../objects/lib_set_term.o :	$(base)/lib_set_term.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_set_term.c

../objects/lib_slk.o :	$(base)/lib_slk.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slk.c

../objects/lib_slkatr_set.o :	$(base)/lib_slkatr_set.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkatr_set.c

../objects/lib_slkatrof.o :	$(base)/lib_slkatrof.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkatrof.c

../objects/lib_slkatron.o :	$(base)/lib_slkatron.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkatron.c

../objects/lib_slkatrset.o :	$(base)/lib_slkatrset.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkatrset.c

../objects/lib_slkattr.o :	$(base)/lib_slkattr.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkattr.c

../objects/lib_slkclear.o :	$(base)/lib_slkclear.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkclear.c

../objects/lib_slkcolor.o :	$(base)/lib_slkcolor.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkcolor.c

../objects/lib_slkinit.o :	$(base)/lib_slkinit.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkinit.c

../objects/lib_slklab.o :	$(base)/lib_slklab.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slklab.c

../objects/lib_slkrefr.o :	$(base)/lib_slkrefr.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkrefr.c

../objects/lib_slkset.o :	$(base)/lib_slkset.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slkset.c

../objects/lib_slktouch.o :	$(base)/lib_slktouch.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_slktouch.c

../objects/lib_touch.o :	$(base)/lib_touch.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_touch.c

../objects/lib_tstp.o :	$(serial)/lib_tstp.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/lib_tstp.c

../objects/lib_ungetch.o :	$(base)/lib_ungetch.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_ungetch.c

../objects/lib_vidattr.o :	$(serial)/lib_vidattr.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/lib_vidattr.c

../objects/lib_vline.o :	$(base)/lib_vline.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_vline.c

../objects/lib_wattroff.o :	$(base)/lib_wattroff.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_wattroff.c

../objects/lib_wattron.o :	$(base)/lib_wattron.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_wattron.c

../objects/lib_winch.o :	$(base)/lib_winch.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_winch.c

../objects/lib_window.o :	$(base)/lib_window.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/lib_window.c

../objects/nc_panel.o :	$(base)/nc_panel.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/nc_panel.c

../objects/safe_sprintf.o :	$(base)/safe_sprintf.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/safe_sprintf.c

../objects/tty_update.o :	$(serial)/tty_update.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/tty_update.c

../objects/memmove.o :	$(base)/memmove.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/memmove.c

../objects/sigaction.o :	$(base)/sigaction.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/sigaction.c

../objects/vsscanf.o :	$(base)/vsscanf.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(base)/vsscanf.c

../objects/access.o :	$(tinfo)/access.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/access.c

../objects/add_tries.o :	$(tinfo)/add_tries.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/add_tries.c

../objects/alloc_entry.o :	$(tinfo)/alloc_entry.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/alloc_entry.c

../objects/alloc_ttype.o :	$(tinfo)/alloc_ttype.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/alloc_ttype.c

../objects/captoinfo.o :	$(tinfo)/captoinfo.c \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/captoinfo.c

../objects/codes.o :	./codes.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/codes.c

../objects/comp_captab.o :	./comp_captab.c \
			$(INCDIR)/tic.h \
			../include/term.h \
			../include/hashsize.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/comp_captab.c

../objects/comp_error.o :	$(tinfo)/comp_error.c \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_error.c

../objects/comp_expand.o :	$(tinfo)/comp_expand.c \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_expand.c

../objects/comp_hash.o :	$(tinfo)/comp_hash.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			../include/hashsize.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_hash.c

../objects/comp_parse.o :	$(tinfo)/comp_parse.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_parse.c

../objects/comp_scan.o :	$(tinfo)/comp_scan.c \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/comp_scan.c

../objects/doalloc.o :	$(tinfo)/doalloc.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/doalloc.c

../objects/fallback.o :	./fallback.c \
			../include/term.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/fallback.c

../objects/free_ttype.o :	$(tinfo)/free_ttype.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/free_ttype.c

../objects/getenv_num.o :	$(tinfo)/getenv_num.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/getenv_num.c

../objects/home_terminfo.o :	$(tinfo)/home_terminfo.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/home_terminfo.c

../objects/init_keytry.o :	$(tinfo)/init_keytry.c \
			../include/term.h \
			$(INCDIR)/tic.h \
			init_keytry.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/init_keytry.c

../objects/lib_acs.o :	$(tinfo)/lib_acs.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_acs.c

../objects/lib_baudrate.o :	$(tinfo)/lib_baudrate.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_baudrate.c

../objects/lib_cur_term.o :	$(tinfo)/lib_cur_term.c \
			../include/term.h \
			$(INCDIR)/term_entry.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_cur_term.c

../objects/lib_data.o :	$(tinfo)/lib_data.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_data.c

../objects/lib_has_cap.o :	$(tinfo)/lib_has_cap.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_has_cap.c

../objects/lib_kernel.o :	$(tinfo)/lib_kernel.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_kernel.c

../objects/lib_keyname.o :	./lib_keyname.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/lib_keyname.c

../objects/lib_longname.o :	$(tinfo)/lib_longname.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_longname.c

../objects/lib_napms.o :	$(tinfo)/lib_napms.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_napms.c

../objects/lib_options.o :	$(tinfo)/lib_options.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_options.c

../objects/lib_raw.o :	$(tinfo)/lib_raw.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_raw.c

../objects/lib_setup.o :	$(tinfo)/lib_setup.c \
			../include/term.h \
			$(INCDIR)/term_entry.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_setup.c

../objects/lib_termcap.o :	$(tinfo)/lib_termcap.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h \
			$(INCDIR)/capdefaults.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_termcap.c

../objects/lib_termname.o :	$(tinfo)/lib_termname.c \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_termname.c

../objects/lib_tgoto.o :	$(tinfo)/lib_tgoto.c \
			../include/term.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_tgoto.c

../objects/lib_ti.o :	$(tinfo)/lib_ti.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_ti.c

../objects/lib_tparm.o :	$(tinfo)/lib_tparm.c \
			../include/term.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_tparm.c

../objects/lib_tputs.o :	$(tinfo)/lib_tputs.c \
			../include/term.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_tputs.c

../objects/lib_trace.o :	$(trace)/lib_trace.c \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(trace)/lib_trace.c

../objects/lib_ttyflags.o :	$(tinfo)/lib_ttyflags.c \
			../include/term.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/lib_ttyflags.c

../objects/lib_twait.o :	$(serial)/lib_twait.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(serial)/lib_twait.c

../objects/name_match.o :	$(tinfo)/name_match.c \
			../include/term.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/name_match.c

../objects/names.o :	./names.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/names.c

../objects/parse_entry.o :	$(tinfo)/parse_entry.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h \
			../include/parametrized.h \
			$(INCDIR)/capdefaults.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/parse_entry.c

../objects/read_entry.o :	$(tinfo)/read_entry.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/read_entry.c

../objects/read_termcap.o :	$(tinfo)/read_termcap.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/read_termcap.c

../objects/setbuf.o :	$(tinfo)/setbuf.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/setbuf.c

../objects/strings.o :	$(tinfo)/strings.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/strings.c

../objects/unctrl.o :	./unctrl.c
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/unctrl.c

../objects/write_entry.o :	$(tinfo)/write_entry.c \
			../include/term.h \
			$(INCDIR)/term_entry.h \
			$(INCDIR)/tic.h
	cd ../objects; $(LIBTOOL) $(CC) $(CFLAGS_NORMAL) -c ../ncurses/$(tinfo)/write_entry.c


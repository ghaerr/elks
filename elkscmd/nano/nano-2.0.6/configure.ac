# Configuration for GNU nano - a small and user-friendly text editor
#
# Copyright (C) 1999, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.
#
# GNU nano is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# GNU nano is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU nano; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301,
# USA.
#
# $Id: configure.ac,v 1.160.2.6 2007/04/26 12:00:33 dolorous Exp $

AC_INIT([GNU nano], [2.0.6], [nano-devel@gnu.org], [nano])
AC_CONFIG_SRCDIR([src/nano.c])
AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADERS([config.h])

AC_PREREQ(2.54)

dnl Checks for programs.

AC_GNU_SOURCE
AC_PROG_CC
AC_PROG_LN_S
AC_ISC_POSIX
AC_SYS_LARGEFILE

dnl Internationalization macros.

AM_GNU_GETTEXT_VERSION(0.11.5)
AM_GNU_GETTEXT([external], [need-ngettext])
AM_CONDITIONAL(USE_NLS, test x$USE_NLS = xyes)

dnl Data location.

pkgdatadir=${datadir}/${PACKAGE}
AC_DEFINE_DIR([PKGDATADIR], [pkgdatadir], [Where data are placed to.])

dnl Checks for header files.

AC_HEADER_STDC
AC_CHECK_HEADERS(getopt.h libintl.h limits.h regex.h sys/param.h wchar.h wctype.h)

dnl Checks for options.

AC_ARG_ENABLE(debug,
[  --enable-debug          Enable debugging (disabled by default)],
[if test x$enableval = xyes; then
    AC_DEFINE(DEBUG, 1, [Define this to enable debug messages and assert warnings.]) debug_support=yes
fi])

if test x$debug_support != xyes; then
    AC_DEFINE(NDEBUG, 1, [Shut up assert warnings :-)])
fi

AC_ARG_ENABLE(extra,
[  --enable-extra          Enable extra features, currently only easter eggs],
[if test x$enableval = xyes; then
    AC_DEFINE(NANO_EXTRA, 1, [Define this to enable extra stuff.]) extra_support=yes
fi])

AC_ARG_ENABLE(tiny,
[  --enable-tiny           Disable features for the sake of size],
[if test x$enableval = xyes; then
    AC_DEFINE(NANO_TINY, 1, [Define this to make the nano executable as small as possible.]) tiny_support=yes
    AC_DEFINE(DISABLE_BROWSER, 1, [Define this to disable the built-in file browser.])
    AC_DEFINE(DISABLE_HELP, 1, [Define this to disable the help text display.])
    AC_DEFINE(DISABLE_JUSTIFY, 1, [Define this to disable the justify routines.])
    AC_DEFINE(DISABLE_MOUSE, 1, [Define this to disable the mouse functions.])
    AC_DEFINE(DISABLE_OPERATINGDIR, 1, [Define this to disable the setting of the operating directory (chroot of sorts).])
    AC_DEFINE(DISABLE_SPELLER, 1, [Define this to disable the spell checker functions.])
    AC_DEFINE(DISABLE_TABCOMP, 1, [Define to disable the tab completion functions for files and search strings.])
    AC_DEFINE(DISABLE_WRAPPING, 1, [Define this to disable all text wrapping.])
fi])

AC_ARG_ENABLE(browser,
[  --disable-browser       Disable built-in file browser],
[if test x$enableval != xyes; then
    AC_DEFINE(DISABLE_BROWSER, 1, [Define this to disable the built-in file browser.])
fi])

AC_ARG_ENABLE(help,
[  --disable-help          Disable help functions],
[if test x$enableval != xyes; then
    AC_DEFINE(DISABLE_HELP, 1, [Define this to disable the help text display.])
fi])

AC_ARG_ENABLE(justify,
[  --disable-justify       Disable justify/unjustify functions],
[if test x$enableval != xyes; then
    AC_DEFINE(DISABLE_JUSTIFY, 1, [Define this to disable the justify routines.])
fi])

AC_ARG_ENABLE(mouse,
[  --disable-mouse         Disable mouse support (and -m flag)],
[if test x$enableval != xyes; then
    AC_DEFINE(DISABLE_MOUSE, 1, [Define this to disable the mouse support.])
fi])

AC_ARG_ENABLE(operatingdir,
[  --disable-operatingdir  Disable setting of operating directory (chroot of sorts)],
[if test x$enableval != xyes; then
    AC_DEFINE(DISABLE_OPERATINGDIR, 1, [Define this to disable the setting of the operating directory (chroot of sorts).])
fi])

AC_ARG_ENABLE(speller,
[  --disable-speller       Disable spell checker functions],
[if test x$enableval != xyes; then
    AC_DEFINE(DISABLE_SPELLER, 1, [Define this to disable the spell checker functions.])
fi])

AC_ARG_ENABLE(tabcomp,
[  --disable-tabcomp       Disable tab completion functions],
[if test x$enableval != xyes; then
    AC_DEFINE(DISABLE_TABCOMP, 1, [Define to disable the tab completion functions for files and search strings.])
fi])

AC_ARG_ENABLE(wrapping,
[  --disable-wrapping      Disable all wrapping of text (and -w flag)],
[if test x$enableval != xyes; then
    AC_DEFINE(DISABLE_WRAPPING, 1, [Define this to disable all text wrapping.])
fi])

AC_ARG_ENABLE(wrapping-as-root,
[  --disable-wrapping-as-root  Disable wrapping of text as root by default],
[if test x$enableval != xyes; then
    AC_DEFINE(DISABLE_ROOTWRAPPING, 1, [Define this to disable text wrapping as root by default.])
fi])

AC_ARG_ENABLE(color,
[  --enable-color          Enable color and syntax highlighting],
[if test x$enableval = xyes; then
    if test x$ac_cv_header_regex_h = xyes; then
	AC_DEFINE(ENABLE_NANORC, 1, [Define this to use .nanorc files.]) nanorc_support=yes
	AC_DEFINE(ENABLE_COLOR, 1, [Define this to have syntax highlighting, requires regex.h and ENABLE_NANORC too!]) color_support=yes
    else
	AC_MSG_ERROR([
*** The header file regex.h was not found.  If you wish to use color
*** support this header file is required.  Please either install C
*** libraries that include the regex.h file or do not call the configure
*** script with --enable-color.])
    fi
fi])

AC_ARG_ENABLE(multibuffer,
[  --enable-multibuffer    Enable multiple file buffers],
[if test x$enableval = xyes; then
    AC_DEFINE(ENABLE_MULTIBUFFER, 1, [Define this to enable multiple file buffers.]) multibuffer_support=yes
fi])

AC_ARG_ENABLE(nanorc,
[  --enable-nanorc         Enable use of .nanorc files],
[if test x$enableval = xyes; then
    AC_DEFINE(ENABLE_NANORC, 1, [Define this to use .nanorc files.]) nanorc_support=yes
fi])

AC_ARG_ENABLE(all,
[  --enable-all            Enable ALL extra nano functionality (kitchen sink option)],
[if test x$enableval = xyes; then
    AC_DEFINE(ENABLE_NANORC, 1, [Define this to use .nanorc files.]) nanorc_support=yes
    AC_DEFINE(ENABLE_COLOR, 1, [Define this to have syntax highlighting, requires regex.h and ENABLE_NANORC too!]) color_support=yes
    AC_DEFINE(ENABLE_MULTIBUFFER, 1, [Define this to enable multiple file buffers.]) multibuffer_support=yes
    AC_DEFINE(NANO_EXTRA, 1, [Define this to enable extra stuff.]) extra_support=yes
fi])

AC_MSG_CHECKING([whether to enable UTF-8 support])
AC_ARG_ENABLE(utf8,
[  --enable-utf8           Enable UTF-8 support],
[if test x$enableval = xyes; then
    enable_utf8=yes
else
    enable_utf8=no
fi],
[enable_utf8=auto])
AC_MSG_RESULT($enable_utf8)

AC_MSG_CHECKING([whether to use slang])
CURSES_LIB_NAME=""
AC_ARG_WITH(slang,
[  --with-slang[=DIR]      Use the slang library instead of curses],
[    case "$with_slang" in
    no)
	AC_MSG_RESULT(no)
	;;
    *)
	AC_MSG_RESULT(yes)

	if test x$with_slang != xyes; then
	    # Add additional search path
	    LDFLAGS="-L$with_slang/lib $LDFLAGS"
	    CPPFLAGS="-I$with_slang/include $CPPFLAGS"
	fi

	if test x$enable_utf8 != xno; then
	    AC_CHECK_HEADER(slcurses.h,
		AC_MSG_CHECKING([for SLutf8_enable in -lslang])
		_libs=$LIBS
		LIBS="$LIBS -lslang"
		AC_TRY_RUN([
#include <slcurses.h>
int main(void)
{
    SLutf8_enable(1);
    return 0;
}],
		    [AC_MSG_RESULT(yes)
		    AC_DEFINE(USE_SLANG, 1, [Define to use the slang wrappers for curses instead of native curses.]) slang_support=yes
		    CURSES_LIB_WIDE=yes
		    if test x$with_slang != xyes; then
			CURSES_LIB="-L${with_slang}/lib -lslang"
		    else
			CURSES_LIB="-lslang"
		    fi
		    CURSES_LIB_NAME=slang],
		    [AC_MSG_RESULT(no)

		    # We might need the term library
		    for termlib in ncurses curses termcap terminfo termlib; do
			AC_CHECK_LIB([${termlib}], tputs, [tcap="-l$termlib"])
			test -n "$tcap" && break
		    done

		    AC_MSG_CHECKING([for SLutf8_enable in -lslang $tcap])
		    LIBS="$LIBS $tcap"
		    AC_TRY_RUN([
#include <slcurses.h>
int main(void)
{
    SLutf8_enable(1);
    return 0;
}],
			[AC_MSG_RESULT(yes)
			AC_DEFINE(USE_SLANG, 1, [Define to use the slang wrappers for curses instead of native curses.]) slang_support=yes
			CURSES_LIB_WIDE=yes
			if test x$with_slang != xyes; then
			    CURSES_LIB="-L${with_slang}/lib -lslang $tcap"
			else
			    CURSES_LIB="-lslang $tcap"
			fi
			CURSES_LIB_NAME=slang],
			[AC_MSG_RESULT(no)

			# We might need the math library
			AC_MSG_CHECKING([for SLutf8_enable in -lslang $tcap -lm])
			LIBS="$LIBS -lm"
			AC_TRY_RUN([
#include <slcurses.h>
int main(void)
{
    SLutf8_enable(1);
    return 0;
}],
			    [AC_MSG_RESULT(yes)
			    AC_DEFINE(USE_SLANG, 1, [Define to use the slang wrappers for curses instead of native curses.]) slang_support=yes
			    CURSES_LIB_WIDE=yes
			    if test x$with_slang != xyes; then
				CURSES_LIB="-L${with_slang}/lib -lslang $tcap -lm"
			    else
				CURSES_LIB="-lslang $tcap -lm"
			    fi
			    CURSES_LIB_NAME=slang],
			    [AC_MSG_RESULT(no)],
    AC_MSG_WARN([*** Can not use slang when cross-compiling]))],
    AC_MSG_WARN([*** Can not use slang when cross-compiling]))],
    AC_MSG_WARN([*** Can not use slang when cross-compiling])),
	    AC_MSG_ERROR([
*** The header file slcurses.h was not found.  If you wish to use
*** slang support this header file is required.  Please either
*** install a version of slang that includes the slcurses.h file or
*** do not call the configure script with --with-slang.]))
	fi

	if eval "test x$CURSES_LIB_NAME = x"; then
	    # Reset libs if the above slang tests failed
	    if test x$enable_utf8 != xno; then
		LIBS=$_libs
	    fi

	    AC_CHECK_HEADER(slcurses.h,
		AC_MSG_CHECKING([for SLtt_initialize in -lslang])
		_libs=$LIBS
		LIBS="$LIBS -lslang"
		AC_TRY_RUN([
#include <slcurses.h>
int main(void)
{
    SLtt_initialize(NULL);
    return 0;
}],
		    [AC_MSG_RESULT(yes)
		    AC_DEFINE(USE_SLANG, 1, [Define to use the slang wrappers for curses instead of native curses.]) slang_support=yes
		    if test x$with_slang != xyes; then
			CURSES_LIB="-L${with_slang}/lib -lslang"
		    else
			CURSES_LIB="-lslang"
		    fi
		    CURSES_LIB_NAME=slang],
		    [AC_MSG_RESULT(no)

		    # We might need the term library
		    for termlib in ncurses curses termcap terminfo termlib; do
			AC_CHECK_LIB([${termlib}], tputs, [tcap="-l$termlib"])
			test -n "$tcap" && break
		    done

		    AC_MSG_CHECKING([for SLtt_initialize in -lslang $tcap])
		    LIBS="$LIBS $tcap"
		    AC_TRY_RUN([
#include <slcurses.h>
int main(void)
{
    SLtt_initialize(NULL);
    return 0;
}],
			[AC_MSG_RESULT(yes)
			AC_DEFINE(USE_SLANG, 1, [Define to use the slang wrappers for curses instead of native curses.]) slang_support=yes
			if test x$with_slang != xyes; then
			    CURSES_LIB="-L${with_slang}/lib -lslang $tcap"
			else
			    CURSES_LIB="-lslang $tcap"
			fi
			CURSES_LIB_NAME=slang],
			[AC_MSG_RESULT(no)

			# We might need the math library
			AC_MSG_CHECKING([for SLtt_initialize in -lslang $tcap -lm])
			LIBS="$LIBS -lm"
			AC_TRY_RUN([
#include <slcurses.h>
int main(void)
{
    SLtt_initialize(NULL);
    return 0;
}],
			    [AC_MSG_RESULT(yes)
			    AC_DEFINE(USE_SLANG, 1, [Define to use the slang wrappers for curses instead of native curses.]) slang_support=yes
			    if test x$with_slang != xyes; then
				CURSES_LIB="-L${with_slang}/lib -lslang $tcap -lm"
			    else
				CURSES_LIB="-lslang $tcap -lm"
			    fi
			    CURSES_LIB_NAME=slang],
			    [AC_MSG_RESULT(no)],
    AC_MSG_WARN([*** Can not use slang when cross-compiling]))],
    AC_MSG_WARN([*** Can not use slang when cross-compiling]))],
    AC_MSG_WARN([*** Can not use slang when cross-compiling])),
	    AC_MSG_ERROR([
*** The header file slcurses.h was not found.  If you wish to use
*** slang support this header file is required.  Please either
*** install a version of slang that includes the slcurses.h file or
*** do not call the configure script with --with-slang.]))
	fi

	if test "${_libs+set}" = "set"; then
	    LIBS=$_libs
	fi

	if test x$with_slang != xyes; then
	    LDFLAGS=${_ldflags}
	fi
	;;
    esac], [AC_MSG_RESULT(no)])

AM_CONDITIONAL(USE_COLOR, test x$color_support = xyes)
AM_CONDITIONAL(USE_NANORC, test x$nanorc_support = xyes)

dnl Checks for functions.

AC_CHECK_FUNCS(getdelim getline isblank strcasecmp strcasestr strncasecmp strnlen vsnprintf)

if test x$enable_utf8 != xno; then
    AC_CHECK_FUNCS(iswalnum iswblank iswpunct iswspace mblen mbstowcs mbtowc wctomb wcwidth)
fi

if test x$ac_cv_func_vsnprintf = xno; then
    AM_PATH_GLIB_2_0(2.0.0,,
	AC_MSG_ERROR([
*** vsnprintf() not found.  GLIB 2.x not found either.  You should
*** install the GLIB 2.x library which can be found at
*** ftp://ftp.gtk.org/.]),
	glib)
fi

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(getopt_long)

dnl Checks for libraries.

if eval "test x$CURSES_LIB_NAME = x"; then
    AC_CHECK_HEADERS(ncurses.h)

    if test x$enable_utf8 != xno; then
	AC_CHECK_LIB(ncursesw, get_wch, [CURSES_LIB="-lncursesw" CURSES_LIB_NAME=ncursesw CURSES_LIB_WIDE=yes])
    fi

    if eval "test x$CURSES_LIB_NAME = x"; then
	AC_CHECK_LIB(ncurses, initscr, [CURSES_LIB="-lncurses" CURSES_LIB_NAME=ncurses])
    fi
fi

if eval "test x$CURSES_LIB_NAME = x"; then
    AC_CHECK_HEADERS(curses.h)

    if test x$enable_utf8 != xno; then
	AC_CHECK_LIB(curses, get_wch, [CURSES_LIB="-lcurses" CURSES_LIB_NAME=curses CURSES_LIB_WIDE=yes])
    fi

    if eval "test x$CURSES_LIB_NAME = x"; then
	 AC_CHECK_LIB(curses, initscr, [CURSES_LIB="-lcurses" CURSES_LIB_NAME=curses])
    fi
fi

if eval "test x$CURSES_LIB_NAME = x"; then
    AC_MSG_WARN([
*** No curses lib available.  Consider getting the official ncurses
*** distribution from ftp://ftp.gnu.org/pub/gnu/ncurses if you get
*** errors compiling nano.])
else
    AC_MSG_RESULT([Using $CURSES_LIB_NAME as the curses library])
fi

AC_CHECK_LIB([$CURSES_LIB_NAME], use_default_colors, AC_DEFINE(HAVE_USE_DEFAULT_COLORS, 1, [Define this if your curses library has the use_default_colors() command.]))

dnl Parse any configure options.

LIBS="$LIBS $CURSES_LIB"

AC_SUBST(CURSES_LIB)

if test "x$GLIB_CFLAGS" != "x"; then
    CFLAGS="$CFLAGS $GLIB_CFLAGS"
fi
if test "x$GLIB_LIBS" != "x"; then
    LDFLAGS="$LDFLAGS $GLIB_LIBS"
fi

if test x$enable_utf8 != xno && \
    test x$CURSES_LIB_WIDE = xyes && \
    test x$ac_cv_func_iswalnum = xyes && \
    test x$ac_cv_func_iswpunct = xyes && \
    (test x$ac_cv_func_iswblank = xyes || test x$ac_cv_func_iswspace = xyes) && \
    test x$ac_cv_func_mblen = xyes && \
    test x$ac_cv_func_mbstowcs = xyes && \
    test x$ac_cv_func_mbtowc = xyes && \
    test x$ac_cv_func_wctomb = xyes && \
    test x$ac_cv_func_wcwidth = xyes; then
	AC_DEFINE(ENABLE_UTF8, 1, [Define this if your system has sufficient UTF-8 support (a wide curses library, iswalnum(), iswpunct(), iswblank() or iswspace(), mblen(), mbstowcs(), mbtowc(), wctomb(), and wcwidth()).])
else
    if test x$enable_utf8 = xyes; then
	AC_MSG_ERROR([
*** UTF-8 support was requested, but insufficient UTF-8 support was
*** detected in your curses and/or C libraries.  Please verify that your
*** slang was built with UTF-8 support or your curses was built with
*** wide character support, and that your C library was built with wide
*** character support.])
    elif test x$enable_utf8 != xno; then
	AC_MSG_WARN([
*** Insufficient UTF-8 support was detected in your curses and/or C
*** libraries.  If you want UTF-8 support, please verify that your slang
*** was built with UTF-8 support or your curses was built with wide
*** character support, and that your C library was built with wide
*** character support.])
    fi
fi

AC_CONFIG_FILES([
Makefile
doc/Makefile
doc/nanorc.sample
doc/man/Makefile
doc/man/fr/Makefile
doc/syntax/Makefile
doc/texinfo/Makefile
m4/Makefile
po/Makefile.in
src/Makefile
nano.spec
])

AC_OUTPUT

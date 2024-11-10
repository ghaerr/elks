dnl Copyright 1997,2001-2003 Alain Knaff.
dnl This file is part of mtools.
dnl
dnl Mtools is free software: you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation, either version 3 of the License, or
dnl (at your option) any later version.
dnl
dnl Mtools is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
dnl
dnl Check for declaration of sys_errlist in one of stdio.h and errno.h.
dnl Declaration of sys_errlist on BSD4.4 interferes with our declaration.
dnl Reported by Keith Bostic.
AC_DEFUN([CF_SYS_ERRLIST],
[
AC_MSG_CHECKING([declaration of sys_errlist])
AC_CACHE_VAL(cf_cv_dcl_sys_errlist,[
    AC_TRY_COMPILE([
#include <stdio.h>
#include <sys/types.h>
#include <errno.h> ],
    [char *c = (char *) *sys_errlist],
    [cf_cv_dcl_sys_errlist=yes],
    [cf_cv_dcl_sys_errlist=no])])
AC_MSG_RESULT($cf_cv_dcl_sys_errlist)
test $cf_cv_dcl_sys_errlist = no || AC_DEFINE([DECL_SYS_ERRLIST],1,[Define when sys_errlist is defined in the standard include files])
])dnl
dnl

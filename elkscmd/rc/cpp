#!/bin/sh

# @(#) cpp.sh 1.3 92/01/15 21:53:22

# Unprototypeing preprocessor for pre-ANSI C compilers.  On some systems,
# this script can be as simple as:
#
#	/lib/cpp "$@" | unproto
#
# However, some cc(1) drivers specify output file names on the
# preprocessor command line, so this shell script must be prepared to
# intercept them.  Depending on the driver program, the cpp options may
# even go before or after the file name argument(s). The script below
# tries to tackle all these cases.
#
# You may want to add -Ipath_to_stdarg.h_file, -Dvoid=, -Dvolatile=, 
# and even -D__STDC__.

## (This is what I used while testing with the SunOS C compiler.
## Also, I added "-Qpath ." to CFLAGS so that cpp would be
## run out of the current directory. --- Byron)
cpp_args="-I/u/byron/lib/sun4 -Dconst= -Dvolatile="

while :
do
	case $1 in
	"")	break;;
	-*)	cpp_args="$cpp_args $1";;
	 *)	cpp_args="$cpp_args $1"
		case $2 in
		""|-*)	;;
		    *)	exec 1> $2 || exit 1; shift;;
		esac;;
	esac
	shift
done

/lib/cpp $cpp_args | unproto

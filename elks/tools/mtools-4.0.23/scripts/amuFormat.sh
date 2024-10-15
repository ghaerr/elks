#!/bin/sh
# Copyright 2004 Feuz Stefan.
# Copyright 2007 Adam Tkac.
# This file is part of mtools.
#
# Mtools is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Mtools is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
#
#  amuFormat.sh  Formats various types and sizes of PC-Cards, according to the
#  AMU-specification
#
#  parameters:   $1:   Card Type: The Card Type is written as disk/volume-label
#                      to the boot-record
#                      The string should have a length of max. 11 characters.
#
#                $2:   Drive character (b:, c:)
#
#  10-12-2003    lct   created
#
vers=1.4

#echo "debug: $0,$1,$2,$3,$4"

#
# main()
#
if [ $# -ne 2 ] ; then
	echo "Usage: amuFormat.sh <Card Type> <drive>" >&2
	echo "<Card Type> has to be defined in amuFormat.sh itself" >&2
	echo "<drive> has to be defined in mtools.conf" >&2
	exit 1
fi

echo "amuFormat $vers started..."

drive="$2"

case "$1" in
8MBCARD-FW)
	## using the f: or g: drive for fat12 formatting...
	## see mtools.conf file...
	case "$2" in
	[bB]:) drive="f:" ;;
	[cC]:) drive="g:" ;;
	*) echo "Drive $2 not supported."; exit 1 ;;
	esac
	cylinders=245 heads=2 cluster_size=8
	;;
32MBCARD-FW)
	#from amu_toolkit_0_6:
	#mformat -t489 -h4 -c4 -n32 -H32 -r32 -vPC-CARD -M512 -N0000 c:
	cylinders=489 heads=4 cluster_size=4
	;;
64MBCARD-FW)
	echo "***** WARNING: untested on AvHMU, exiting *****"
	exit 1
	cylinders=245 heads=2 cluster_size=8
	;;
1GBCARD-FW)
	# from amu_toolkit_0_6:
	#mformat -t2327 -h16 -c64 -n63 -H63 -r32 -v AMU-CARD -M512 -N 0000 c:
	echo "***** WARNING: untested on AvHMU *****"
	cylinders=2327 heads=16 cluster_size=64
	;;
64MBCARDSAN)
	# from amu_toolkit_0_6:
	#mformat -t489 -h8 -c4 -n32 -H32 -r32 -v AMU-CARD -M512 -N 0000 c:
	cylinders=489 heads=8 cluster_size=4
	;;
#
# insert new cards here...
#
*)
	echo "Card not supported."
	exit 1
	;;
esac

echo "Formatting card in slot $2 as $1"

## initialise partition table
mpartition -I "$drive"

# write a partition table
mpartition -c -t$cylinders -h$heads -s32 -b32 "$drive"

## write boot-record, two FATs and a root-directory
mformat -c$cluster_size -v "$1" "$drive"

minfo "$2"
mdir  "$2"

echo "done."

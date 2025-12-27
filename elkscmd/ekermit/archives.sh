#!/bin/bash
# E-Kermit installation script
# Frank da Cruz 26 May 2021

v=18  # This version (1.8)
p=17  # Previous version (1.7)

cd ~/ek$v || exit 1
pwd

#Source and documentation files to be archived
s="COPYING main.c unix.h archives.sh \
 debug.h kermit.c makefile unixio.c cdefs.h kermit.h platform.h"

#Differences from last release
t="COPYING kermit.h platform.h unix.h cdefs.h debug.h \
 main.c kermit.c unixio.c makefile"
rm -f ek$v.diff
for i in $t; do diff -cb ../ek$p/$i $i >> ek$v.diff; done

d="ek$p.diff ek$v.diff"

#New file not in previous version
n="AAREADME.TXT"

#Result files - Make all new ones
z="ek$v.tar ek$v.tar.gz ek$v.zip"
rm -f $z
rm -f *.~*~

#Set source file attributes
chmod 664 $n $s $d || exit 2
chmod +x archives.sh || exit 3
chgrp kermit $n $s $d || exit 4

echo CREATING ek$v.zip
#Create Zip archive with CRLF line terminators
zip ek${v}.zip $n $s $d > /dev/null || exit 5
echo CREATING ek$v.tar
#Create tar archive in native Unix format
tar cvf ek$v.tar $n $s $d > /dev/null || exit 6
echo CREATING ek$v.tar.gz
gzip ek$v.tar || exit 7
tar cvf ek$v.tar $n $s $d > /dev/null || exit 8

chmod 664 ek$v.zip
chmod 664 ek$v.tar
chmod 664 ek$v.tar.gz

chgrp kermit ek$v.zip
chgrp kermit ek$v.tar
chgrp kermit ek$v.tar.gz

rm -f zi*
echo TAR ARCHIVE:
tar tvf ek$v.tar
echo
echo ZIP ARCIVE:
unzip -l ek$v.zip
echo
echo DONE

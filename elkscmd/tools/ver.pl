#!/usr/bin/perl
#Get the version from the kernel makefile

while (<>) {
        /^\s*VERSION\s*=\s*(\d+)/	&& ($ver = $1);
        /^\s*PATCHLEVEL\s*=\s*(\d+)/	&& ($pat = $1);
        /^\s*SUBLEVEL\s*=\s*(\d+)/	&& ($sub = $1);
        /^\s*EXTRAVERSION\s*=\s*(.*)/	&& ($ext = $1);
}

print "\nELKS ${ver}.${pat}.${sub}${ext}\n\n";

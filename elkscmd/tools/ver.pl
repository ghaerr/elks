#!/usr/bin/perl
#Get the version from the kernel makefile
open(MAKEFILE, $ARGV[0]);

while (<MAKEFILE>) {
        if (/^\s*VERSION\s*=\s*([0-9]{1,4})/) {
                $ver = $1;
        }
        if (/^\s*PATCHLEVEL\s*=\s*([0-9]{1,4})/) {
                $pat = $1;
        }
        if (/^\s*SUBLEVEL\s*=\s*([0-9]{1,4})/) {
                $sub = $1;
        }
}
print "\nELKS ";
print $ver, ".", $pat, ".", $sub;
print "\n\n";

#ifndef __GETOPT_H
#define __GETOPT_H

/* Copyright (C) 1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

extern char *optarg;
extern int opterr;
extern int optind;

int getopt(int argc, char *const *argv, const char *shortopts);

#endif /* __GETOPT_H */

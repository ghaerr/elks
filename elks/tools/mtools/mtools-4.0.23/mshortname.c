/*  Copyright 2010 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 *
 * mshortname.c
 * Change MSDOS file attribute flags
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "mainloop.h"

static int print_short_name(direntry_t *entry, MainParam_t *mp UNUSEDP)
{
	fprintShortPwd(stdout, entry);
	putchar('\n');
	return GOT_ONE;
}


static void usage(int ret) NORETURN;
static void usage(int ret)
{
	fprintf(stderr, "Mtools version %s, dated %s\n", 
		mversion, mdate);
	fprintf(stderr, 
		"Usage: %s msdosfile [msdosfiles...]\n",
		progname);
	exit(ret);
}

void mshortname(int argc, char **argv, int type UNUSEDP) NORETURN;
void mshortname(int argc, char **argv, int type UNUSEDP)
{
	struct MainParam_t mp;
	int c;

	if(helpFlag(argc, argv))
		usage(0);
	while ((c = getopt(argc, argv, "i:h")) != EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg);
				break;
			case 'h':
				usage(0);
			case '?':
				usage(1);
		}
	}

	if(optind == argc) {
		usage(0);
	}

	if (optind >= argc)
		usage(1);

	init_mp(&mp);
	mp.callback = print_short_name;
	mp.arg = NULL;
	mp.lookupflags = ACCEPT_PLAIN | ACCEPT_DIR;
	exit(main_loop(&mp, argv + optind, argc - optind));
}

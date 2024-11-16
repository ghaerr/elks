/*  Copyright 1986-1992 Emmet P. Gray.
 *  Copyright 1996-1998,2000-2002,2007,2009 Alain Knaff.
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
 * mattrib.c
 * Change MSDOS file attribute flags
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "mainloop.h"

typedef struct Arg_t {
	unsigned char add;
	unsigned char remove;
	struct MainParam_t mp;
	int recursive;
	int doPrintName;
} Arg_t;

static int attrib_file(direntry_t *entry, MainParam_t *mp)
{
	Arg_t *arg=(Arg_t *) mp->arg;

	if(entry->entry != -3) {
		/* if not root directory, change it */
		entry->dir.attr = (entry->dir.attr & arg->remove) | arg->add;
		dir_write(entry);
	}
	return GOT_ONE;
}

static int replay_attrib(direntry_t *entry, MainParam_t *mp UNUSEDP)
{
	if ( (IS_ARCHIVE(entry) && IS_DIR(entry)) ||
		 (!IS_ARCHIVE(entry) && !IS_DIR(entry)) ||
		 IS_SYSTEM(entry) || IS_HIDDEN(entry)) {

		printf("mattrib ");

		if (IS_ARCHIVE(entry) && IS_DIR(entry)) {
			printf("+a ");
		}

		if (!IS_ARCHIVE(entry) && !IS_DIR(entry)) {
			printf("-a ");
		}

		if (IS_SYSTEM(entry)) {
			printf("+s ");
		}

		if (IS_HIDDEN(entry)) {
			printf("+h ");
		}

		fprintPwd(stdout, entry, 1);
		printf("\n");
	}
	return GOT_ONE;
}



static int view_attrib(direntry_t *entry, MainParam_t *mp UNUSEDP)
{
	printf("  ");
	if(IS_ARCHIVE(entry))
		putchar('A');
	else
		putchar(' ');
	fputs("  ",stdout);
	if(IS_SYSTEM(entry))
		putchar('S');
	else
		putchar(' ');
	if(IS_HIDDEN(entry))
		putchar('H');
	else
		putchar(' ');
	if(IS_READONLY(entry))
		putchar('R');
	else
		putchar(' ');
	printf("     ");
	fprintPwd(stdout, entry, 0);
	printf("\n");
	return GOT_ONE;
}


static int concise_view_attrib(direntry_t *entry, MainParam_t *mp)
{
	Arg_t *arg=(Arg_t *) mp->arg;

	if(IS_ARCHIVE(entry))
		putchar('A');
	if(IS_DIR(entry))
		putchar('D');	
	if(IS_SYSTEM(entry))
		putchar('S');
	if(IS_HIDDEN(entry))
		putchar('H');
	if(IS_READONLY(entry))
		putchar('R');
	if(arg->doPrintName) {
		putchar(' ');
		fprintPwd(stdout, entry, 0);
	}
	putchar('\n');
	return GOT_ONE;
}

static int recursive_attrib(direntry_t *entry, MainParam_t *mp)
{
	mp->callback(entry, mp);
	return mp->loop(mp->File, mp, "*");
}


static void usage(int ret) NORETURN;
static void usage(int ret)
{
	fprintf(stderr, "Mtools version %s, dated %s\n", 
		mversion, mdate);
	fprintf(stderr, 
		"Usage: %s [-p] [-a|+a] [-h|+h] [-r|+r] [-s|+s] msdosfile [msdosfiles...]\n",
		progname);
	exit(ret);
}

static int letterToCode(int letter)
{
	switch (toupper(letter)) {
		case 'A':
			return ATTR_ARCHIVE;
		case 'H':
			return ATTR_HIDDEN;
		case 'R':
			return ATTR_READONLY;
		case 'S':
			return ATTR_SYSTEM;
		default:
			usage(1);
	}
}

void mattrib(int argc, char **argv, int type UNUSEDP) NORETURN;
void mattrib(int argc, char **argv, int type UNUSEDP)
{
	Arg_t arg;
	int view;
	int c;
	int concise;
	int replay;
	char *ptr;
	int wantUsage;

	arg.add = 0;
	arg.remove = 0xff;
	arg.recursive = 0;
	arg.doPrintName = 1;
	view = 0;
	concise = 0;
	replay = 0;
	wantUsage = 0;

	if(helpFlag(argc, argv))
		usage(0);
	while ((c = getopt(argc, argv, "i:/ahrsAHRSXp")) != EOF) {
		switch (c) {
			case 'h':
				wantUsage = 1;
				/* FALL THROUGH */
			default:
				arg.remove &= ~letterToCode(c);
				break;
			case 'i':
				set_cmd_line_image(optarg);
				break;
			case 'p':
				replay = 1;
				break;
			case '/':
				arg.recursive = 1;
				break;
			case 'X':
				concise = 1;
				break;
			case '?':
				usage(1);
		}
	}

	if(optind == argc && wantUsage) {
		usage(0);
	}

	for(;optind < argc;optind++) {
		switch(argv[optind][0]) {
			case '+':
				for(ptr = argv[optind] + 1; *ptr; ptr++)
					arg.add |= letterToCode(*ptr);
				continue;
			case '-':
				for(ptr = argv[optind] + 1; *ptr; ptr++)
					arg.remove &= ~letterToCode(*ptr);
				continue;
		}
		break;
	}

	if(arg.remove == 0xff && !arg.add)
		view = 1;

	if (optind >= argc)
		usage(1);

	init_mp(&arg.mp);
	if(view){
		if(concise) {
			arg.mp.callback = concise_view_attrib;
			arg.doPrintName = (argc - optind > 1 ||
					   arg.recursive ||
					   strpbrk(argv[optind], "*[?") != 0);
		} else if (replay) {
			arg.mp.callback = replay_attrib;
		} else
			arg.mp.callback = view_attrib;
		arg.mp.openflags = O_RDONLY;
	} else {
		arg.mp.callback = attrib_file;
		arg.mp.openflags = O_RDWR;
	}

	if(arg.recursive)
		arg.mp.dirCallback = recursive_attrib;

	arg.mp.arg = (void *) &arg;
	arg.mp.lookupflags = ACCEPT_PLAIN | ACCEPT_DIR;
	if(arg.recursive)
		arg.mp.lookupflags |= DO_OPEN_DIRS | NO_DOTS;
	exit(main_loop(&arg.mp, argv + optind, argc - optind));
}

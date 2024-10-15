/*  Copyright 1997,2000-2002,2009 Alain Knaff.
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
 * mdu.c:
 * Display the space occupied by an MSDOS directory
 */

#include "sysincludes.h"
#include "msdos.h"
#include "vfat.h"
#include "mtools.h"
#include "file.h"
#include "mainloop.h"
#include "fs.h"
#include "codepage.h"


typedef struct Arg_t {
	int all;
	int inDir;
	int summary;
	struct Arg_t *parent;
	char *target;
	char *path;
	unsigned int blocks;
	MainParam_t mp;
} Arg_t;

static void usage(int ret) NORETURN;
static void usage(int ret)
{
		fprintf(stderr, "Mtools version %s, dated %s\n",
			mversion, mdate);
		fprintf(stderr, "Usage: %s: msdosdirectory\n",
			progname);
		exit(ret);
}

static int file_mdu(direntry_t *entry, MainParam_t *mp)
{
	unsigned int blocks;
	Arg_t * arg = (Arg_t *) (mp->arg);

	blocks = countBlocks(entry->Dir,getStart(entry->Dir, &entry->dir));
	if(arg->all || !arg->inDir) {
		fprintPwd(stdout, entry,0);
		printf(" %d\n", blocks);
	}
	arg->blocks += blocks;
	return GOT_ONE;
}


static int dir_mdu(direntry_t *entry, MainParam_t *mp)
{
	Arg_t *parentArg = (Arg_t *) (mp->arg);
	Arg_t arg;
	int ret;
	
	arg = *parentArg;
	arg.mp.arg = (void *) &arg;
	arg.parent = parentArg;
	arg.inDir = 1;

	/* account for the space occupied by the directory itself */
	if(!isRootDir(entry->Dir)) {
		arg.blocks = countBlocks(entry->Dir,
					 getStart(entry->Dir, &entry->dir));
	} else {
		arg.blocks = 0;
	}

	/* recursion */
	ret = mp->loop(mp->File, &arg.mp, "*");
	if(!arg.summary || !parentArg->inDir) {
		fprintPwd(stdout, entry,0);
		printf(" %d\n", arg.blocks);
	}
	arg.parent->blocks += arg.blocks;
	return ret;
}

void mdu(int argc, char **argv, int type UNUSEDP) NORETURN;
void mdu(int argc, char **argv, int type UNUSEDP)
{
	Arg_t arg;
	int c;

	arg.all = 0;
	arg.inDir = 0;
	arg.summary = 0;
	if(helpFlag(argc, argv))
		usage(0);
	while ((c = getopt(argc, argv, "i:ash")) != EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg);
				break;
			case 'a':
				arg.all = 1;
				break;
			case 's':
				arg.summary = 1;
				break;
			case 'h':
				usage(0);
			case '?':
				usage(1);
		}
	}

	if (optind >= argc)
		usage(1);

	if(arg.summary && arg.all) {
		fprintf(stderr,"-a and -s options are mutually exclusive\n");
		usage(1);
	}

	init_mp(&arg.mp);
	arg.mp.callback = file_mdu;
	arg.mp.openflags = O_RDONLY;
	arg.mp.dirCallback = dir_mdu;

	arg.mp.arg = (void *) &arg;
	arg.mp.lookupflags = ACCEPT_PLAIN | ACCEPT_DIR | DO_OPEN_DIRS | NO_DOTS;
	exit(main_loop(&arg.mp, argv + optind, argc - optind));
}

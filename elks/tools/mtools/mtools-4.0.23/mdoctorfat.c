/*  Copyright 1999,2001,2002,2009 Alain Knaff.
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
 * Test program for doctoring the fat
 */


#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "vfat.h"
#include "mainloop.h"
#include "plain_io.h"
#include "nameclash.h"
#include "file.h"
#include "fs.h"
#include "fsP.h"

typedef struct Arg_t {
	char *target;
	MainParam_t mp;
	ClashHandling_t ch;
	Stream_t *sourcefile;
	uint32_t fat;
	int markbad;
	int setsize;
	unsigned long size;
	Fs_t *Fs;
} Arg_t;

static int dos_doctorfat(direntry_t *entry, MainParam_t *mp)
{
	Fs_t *Fs = getFs(mp->File);
	Arg_t *arg=(Arg_t *) mp->arg;
	
	if(!arg->markbad && entry->entry != -3) {
		/* if not root directory, change it */
		set_word(entry->dir.start, arg->fat & 0xffff);
		set_word(entry->dir.startHi, arg->fat >> 16);
		if(arg->setsize)
			set_dword(entry->dir.size, arg->size);
		dir_write(entry);		
	}
	arg->Fs = Fs; 
	return GOT_ONE;
}

static int unix_doctorfat(MainParam_t *mp UNUSEDP)
{
	fprintf(stderr,"File does not reside on a Dos fs\n");
	return ERROR_ONE;
}

static void usage(int ret) NORETURN;
static void usage(int ret)
{
	fprintf(stderr,
		"Mtools version %s, dated %s\n", mversion, mdate);
	fprintf(stderr,
		"Usage: [-b] %s file fat\n", progname);
	exit(ret);
}

void mdoctorfat(int argc, char **argv, int mtype UNUSEDP) NORETURN;
void mdoctorfat(int argc, char **argv, int mtype UNUSEDP)
{
	Arg_t arg;
	int c, ret;
	unsigned int address;
	unsigned int begin, end;
	char *number, *eptr;
	int i;
	unsigned int offset;
	
	/* get command line options */

	init_clash_handling(& arg.ch);

	offset = 0;

	arg.markbad = 0;
	arg.setsize = 0;

	/* get command line options */
	if(helpFlag(argc, argv))
		usage(0);
	while ((c = getopt(argc, argv, "i:bo:s:h")) != EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg);
				break;
			case 'b':
				arg.markbad = 1;
				break;
			case 'o':
				offset = strtoui(optarg,0,0);
				break;
			case 's':
				arg.setsize=1;
				arg.size = strtoul(optarg,0,0);
				break;
			case 'h':
				usage(0);
			case '?':
				usage(1);
		}
	}

	if (argc - optind < 2)
		usage(1);


	/* only 1 file to copy... */
	init_mp(&arg.mp);
	arg.mp.arg = (void *) &arg;
		
	arg.mp.callback = dos_doctorfat;
	arg.mp.unixcallback = unix_doctorfat;
	
	arg.mp.lookupflags = ACCEPT_PLAIN | ACCEPT_DIR | DO_OPEN;
	arg.mp.openflags = O_RDWR;
	arg.fat = strtoui(argv[optind+1], 0, 0) + offset;
	ret=main_loop(&arg.mp, argv + optind, 1);
	if(ret)
		exit(ret);
	address = 0;
	for(i=optind+1; i < argc; i++) {
		unsigned int j;
		number = argv[i];
		if (*number == '<') {
			number++;
		}
		begin = strtoui(number, &eptr, 0);
		if (eptr && *eptr == '-') {
			number = eptr+1;
			end = strtoui(number, &eptr, 0);
		} else {
			end = begin;
		}
		if (eptr == number) {
			fprintf(stderr, "Not a number: %s\n", number);
			exit(-1);
		}

		if (eptr && *eptr == '>') {
			eptr++;
		}
		if (eptr && *eptr) {
			fprintf(stderr, "Not a number: %s\n", eptr);
			exit(-1);
		}

		for (j=begin; j <= end; j++) {
			if(arg.markbad) {
				arg.Fs->fat_encode(arg.Fs, j+offset, arg.Fs->last_fat ^ 6 ^ 8);
			} else {
				if(address) {
					arg.Fs->fat_encode(arg.Fs, address, j+offset);
				}
				address = j+offset;
			}
		}
	}

	if (address && !arg.markbad) {
		arg.Fs->fat_encode(arg.Fs, address, arg.Fs->end_fat);
	}

	exit(ret);
}

/*  Copyright 1996-1998,2000-2002,2005,2007-2009 Alain Knaff.
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
 * mmove.c
 * Renames/moves an MSDOS file
 *
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

/*
 * Preserve the file modification times after the fclose()
 */

typedef struct Arg_t {
	const char *fromname;
	int verbose;
	MainParam_t mp;

	direntry_t *entry;
	ClashHandling_t ch;
} Arg_t;


/*
 * Open the named file for read, create the cluster chain, return the
 * directory structure or NULL on error.
 */
static int renameit(dos_name_t *dosname,
		    char *longname UNUSEDP,
		    void *arg0,
		    direntry_t *targetEntry)
{
	Arg_t *arg = (Arg_t *) arg0;
	int fat;

	targetEntry->dir = arg->entry->dir;
	dosnameToDirentry(dosname, &targetEntry->dir);

	if(IS_DIR(targetEntry)) {
		direntry_t *movedEntry;

		/* get old direntry. It is important that we do this
		 * on the actual direntry which is stored in the file,
		 * and not on a copy, because we will modify it, and the
		 * modification should be visible at file 
		 * de-allocation time */
		movedEntry = getDirentry(arg->mp.File);
		if(movedEntry->Dir != targetEntry->Dir) {
			/* we are indeed moving it to a new directory */
			direntry_t subEntry;
			Stream_t *oldDir;
			/* we have a directory here. Change its parent link */
			
			initializeDirentry(&subEntry, arg->mp.File);

			switch(vfat_lookup(&subEntry, "..", 2, ACCEPT_DIR,
					   NULL, 0, NULL, 0)) {
			    case -1:
				fprintf(stderr,
					" Directory has no parent entry\n");
				break;
			    case -2:
				return ERROR_ONE;
			    case 0:
				GET_DATA(targetEntry->Dir, 0, 0, 0, &fat);
				if (fat == fat32RootCluster(targetEntry->Dir)) {
				    fat = 0;
				}

				subEntry.dir.start[1] = (fat >> 8) & 0xff;
				subEntry.dir.start[0] = fat & 0xff;
				dir_write(&subEntry);
				if(arg->verbose){
					fprintf(stderr,
						"Easy, isn't it? I wonder why DOS can't do this.\n");
				}
				break;
			}

			wipeEntry(movedEntry);
			
			/* free the old parent, allocate the new one. */
			oldDir = movedEntry->Dir;
			*movedEntry = *targetEntry;
			COPY(targetEntry->Dir);
			FREE(&oldDir);
			return 0;
		}
	}

	/* wipe out original entry */
	wipeEntry(arg->mp.direntry);
	return 0;
}



static int rename_file(direntry_t *entry, MainParam_t *mp)
/* rename a messy DOS file to another messy DOS file */
{
	int result;
	Stream_t *targetDir;
	char *shortname;
	const char *longname;

	Arg_t * arg = (Arg_t *) (mp->arg);

	arg->entry = entry;
	targetDir = mp->targetDir;

	if (targetDir == entry->Dir){
		arg->ch.ignore_entry = -1;
		arg->ch.source = entry->entry;
		arg->ch.source_entry = entry->entry;
	} else {
		arg->ch.ignore_entry = -1;
		arg->ch.source = -2;
	}

	longname = mpPickTargetName(mp);
	shortname = 0;
	result = mwrite_one(targetDir, longname, shortname,
			    renameit, (void *)arg, &arg->ch);
	if(result == 1)
		return GOT_ONE;
	else
		return ERROR_ONE;
}


static int rename_directory(direntry_t *entry, MainParam_t *mp)
{
	int ret;

	/* moves a DOS dir */
	if(isSubdirOf(mp->targetDir, mp->File)) {
		fprintf(stderr, "Cannot move directory ");
		fprintPwd(stderr, entry,0);
		fprintf(stderr, " into one of its own subdirectories (");
		fprintPwd(stderr, getDirentry(mp->targetDir),0);
		fprintf(stderr, ")\n");
		return ERROR_ONE;
	}

	if(entry->entry == -3) {
		fprintf(stderr, "Cannot move a root directory: ");
		fprintPwd(stderr, entry,0);
		return ERROR_ONE;
	}

	ret = rename_file(entry, mp);
	if(ret & ERROR_ONE)
		return ret;
	
	return ret;
}

static int rename_oldsyntax(direntry_t *entry, MainParam_t *mp)
{
	int result;
	Stream_t *targetDir;
	const char *shortname, *longname;

	Arg_t * arg = (Arg_t *) (mp->arg);
	arg->entry = entry;
	targetDir = entry->Dir;

	arg->ch.ignore_entry = -1;
	arg->ch.source = entry->entry;
	arg->ch.source_entry = entry->entry;

#if 0
	if(!strcasecmp(mp->shortname, arg->fromname)){
		longname = mp->longname;
		shortname = mp->targetName;
	} else {
#endif
		longname = mp->targetName;
		shortname = 0;
#if 0
	}
#endif
	result = mwrite_one(targetDir, longname, shortname,
			    renameit, (void *)arg, &arg->ch);
	if(result == 1)
		return GOT_ONE;
	else
		return ERROR_ONE;
}


static void usage(int ret) NORETURN;
static void usage(int ret)
{
	fprintf(stderr,
		"Mtools version %s, dated %s\n", mversion, mdate);
	fprintf(stderr,
		"Usage: %s [-vV] [-D clash_option] file targetfile\n", progname);
	fprintf(stderr,
		"       %s [-vV] [-D clash_option] file [files...] target_directory\n", 
		progname);
	exit(ret);
}

void mmove(int argc, char **argv, int oldsyntax) NORETURN;
void mmove(int argc, char **argv, int oldsyntax)
{
	Arg_t arg;
	int c;
	char shortname[12*4+1];
	char longname[4*MAX_VNAMELEN+1];
	char def_drive;
	int i;

	/* get command line options */

	init_clash_handling(& arg.ch);

	/* get command line options */
	arg.verbose = 0;
	if(helpFlag(argc, argv))
		usage(0);
	while ((c = getopt(argc, argv, "i:vD:oh")) != EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg);
				break;
			case 'v':	/* dummy option for mcopy */
				arg.verbose = 1;
				break;
			case 'o':
				handle_clash_options(&arg.ch, c);
				break;
			case 'D':
				if(handle_clash_options(&arg.ch, *optarg))
					usage(1);
				break;
			case 'h':
				usage(0);
			case '?':
				usage(1);
			default:
				break;
		}
	}

	if (argc - optind < 2)
		usage(1);

	init_mp(&arg.mp);		
	arg.mp.arg = (void *) &arg;
	arg.mp.openflags = O_RDWR;

	/* look for a default drive */
	def_drive = '\0';
	for(i=optind; i<argc; i++)
		if(argv[i][0] && argv[i][1] == ':' ){
			if(!def_drive)
				def_drive = ch_toupper(argv[i][0]);
			else if(def_drive != ch_toupper(argv[i][0])){
				fprintf(stderr,
					"Cannot move files across different drives\n");
				exit(1);
			}
		}

	if(def_drive)
		*(arg.mp.mcwd) = def_drive;

	if (oldsyntax && (argc - optind != 2 || strpbrk(":/", argv[argc-1])))
		oldsyntax = 0;

	arg.mp.lookupflags =
	  ACCEPT_PLAIN | ACCEPT_DIR | DO_OPEN_DIRS | NO_DOTS | NO_UNIX;

	if (!oldsyntax){
		target_lookup(&arg.mp, argv[argc-1]);
		arg.mp.callback = rename_file;
		arg.mp.dirCallback = rename_directory;
	} else {
		/* do not look up the target; it will be the same dir as the
		 * source */
		arg.fromname = argv[optind];
		if(arg.fromname[0] && arg.fromname[1] == ':')
			arg.fromname += 2;
		arg.fromname = _basename(arg.fromname);
		arg.mp.targetName = strdup(argv[argc-1]);
		arg.mp.callback = rename_oldsyntax;
	}


	arg.mp.longname.data = longname;
	arg.mp.longname.len = sizeof(longname);
	longname[0]='\0';

	arg.mp.shortname.data = shortname;
	arg.mp.shortname.len = sizeof(shortname);
	shortname[0]='\0';

	exit(main_loop(&arg.mp, argv + optind, argc - optind - 1));
}

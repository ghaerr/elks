/*  Copyright 1986-1992 Emmet P. Gray.
 *  Copyright 1994,1996-2002,2007-2009 Alain Knaff.
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
 * mcopy.c
 * Copy an MSDOS files to and from Unix
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

static void set_mtime(const char *target, time_t mtime)
{
	if (target && strcmp(target, "-") && mtime != 0L) {
#ifdef HAVE_UTIMES
		struct timeval tv[2];	
		tv[0].tv_sec = mtime;
		tv[0].tv_usec = 0;
		tv[1].tv_sec = mtime;
		tv[1].tv_usec = 0;
		utimes((char *)target, tv);
#else
#ifdef HAVE_UTIME
		struct utimbuf utbuf;

		utbuf.actime = mtime;
		utbuf.modtime = mtime;
		utime(target, &utbuf);
#endif
#endif
	}
	return;
}

typedef struct Arg_t {
	int recursive;
	int preserveAttributes;
	int preserveTime;
	unsigned char attr;
	char *path;
	int textmode;
	int needfilter;
	int nowarn;
	int verbose;
	int type;
	int convertCharset;
	MainParam_t mp;
	ClashHandling_t ch;
	int noClobber;
} Arg_t;

static int _unix_write(MainParam_t *mp, int needfilter, const char *unixFile);

/* Write the Unix file */
static int unix_write(MainParam_t *mp, int needfilter)
{
	Arg_t *arg=(Arg_t *) mp->arg;

	if(arg->type)
		return _unix_write(mp, needfilter, "-");
	else {
		char *unixFile = mpBuildUnixFilename(mp);
		int ret;
		if(!unixFile) {
			printOom();
			return ERROR_ONE;
		}
		ret = _unix_write(mp, needfilter, unixFile);
		free(unixFile);
		return ret;
	}
}


/* Write the Unix file */
static int _unix_write(MainParam_t *mp, int needfilter, const char *unixFile)
{
	Arg_t *arg=(Arg_t *) mp->arg;
	time_t mtime;
	Stream_t *File=mp->File;
	Stream_t *Target, *Source;
	struct MT_STAT stbuf;
	int ret;
	char errmsg[80];

	File->Class->get_data(File, &mtime, 0, 0, 0);

	if (!arg->preserveTime)
		mtime = 0L;

	/* if we are creating a file, check whether it already exists */
	if(!arg->type) {
		if (!arg->nowarn && !access(unixFile, 0)){
			if(arg->noClobber) {
				fprintf(stderr, "File \"%s\" exists. To overwrite, try again, and explicitly specify target directory\n",unixFile);
				return ERROR_ONE;
			}

			/* sanity checking */
			if (!MT_STAT(unixFile, &stbuf)) {
				struct MT_STAT srcStbuf;
				int sFd; /* Source file descriptor */
				if(!S_ISREG(stbuf.st_mode)) {
					fprintf(stderr,"\"%s\" is not a regular file\n",
						unixFile);
				
					return ERROR_ONE;
				}
				sFd = get_fd(File);
				if(sFd == -1) {
					fprintf(stderr, "Not ok Unix file ==> good\n");
				}
				if((!MT_FSTAT(sFd, &srcStbuf)) &&
				   stbuf.st_dev == srcStbuf.st_dev &&
				   stbuf.st_ino == srcStbuf.st_ino) {
					fprintf(stderr, "Attempt to copy file on itself\n");
					return ERROR_ONE;
				}
			}

			if( ask_confirmation("File \"%s\" exists, overwrite (y/n) ? ",
					     unixFile)) {
				return ERROR_ONE;
			}
			
		}
	}

	if(!arg->type && arg->verbose) {
		fprintf(stderr,"Copying ");
		mpPrintFilename(stderr,mp);
		fprintf(stderr,"\n");
	}
	
	if(got_signal) {
		return ERROR_ONE;
	}

	if ((Target = SimpleFileOpen(0, 0, unixFile,
				     O_WRONLY | O_CREAT | O_TRUNC,
				     errmsg, 0, 0, 0))) {
		ret = 0;
		if(needfilter && arg->textmode){
			Source = open_filter(COPY(File),arg->convertCharset);
			if (!Source)
				ret = -1;
		} else
			Source = COPY(File);

		if (ret == 0 )
			ret = copyfile(Source, Target);
		FREE(&Source);
		FREE(&Target);
		if(ret <= -1){
			if(!arg->type)
				unlink(unixFile);
			return ERROR_ONE;
		}
		if(!arg->type)
			set_mtime(unixFile, mtime);
		return GOT_ONE;
	} else {
		fprintf(stderr,"%s\n", errmsg);
		return ERROR_ONE;
	}
}

static int makeUnixDir(char *filename)
{
	if(!mkdir(filename
#ifndef OS_mingw32msvc
	          , 0777
#endif
	         ))
		return 0;
	if(errno == EEXIST) {
		struct MT_STAT buf;
		if(MT_STAT(filename, &buf) < 0)
			return -1;
		if(S_ISDIR(buf.st_mode))
			return 0;
		errno = ENOTDIR;
	}
	return -1;
}

/* Copy a directory to Unix */
static int unix_copydir(direntry_t *entry, MainParam_t *mp)
{
	Arg_t *arg=(Arg_t *) mp->arg;
	time_t mtime;
	Stream_t *File=mp->File;
	int ret;
	char *unixFile;

	if (!arg->recursive && mp->basenameHasWildcard)
		return 0;

	File->Class->get_data(File, &mtime, 0, 0, 0);	
	if (!arg->preserveTime)
		mtime = 0L;
	if(!arg->type && arg->verbose) {
		fprintf(stderr,"Copying ");
		fprintPwd(stderr, entry,0);
		fprintf(stderr, "\n");
	}
	if(got_signal)
		return ERROR_ONE;
	unixFile = mpBuildUnixFilename(mp);
	if(!unixFile) {
		printOom();
		return ERROR_ONE;
	}
	if(arg->type || !*mpPickTargetName(mp) || !makeUnixDir(unixFile)) {
		Arg_t newArg;

		newArg = *arg;
		newArg.mp.arg = (void *) &newArg;
		newArg.mp.unixTarget = unixFile;
		newArg.mp.targetName = 0;
		newArg.mp.basenameHasWildcard = 1;

		ret = mp->loop(File, &newArg.mp, "*");
		set_mtime(unixFile, mtime);
		free(unixFile);
		return ret | GOT_ONE;		
	} else {
		perror("mkdir");
		fprintf(stderr, 
			"Failure to make directory %s\n", 
			unixFile);
		free(unixFile);
		return ERROR_ONE;
	}
}

static  int dos_to_unix(direntry_t *entry UNUSEDP, MainParam_t *mp)
{
	return unix_write(mp, 1);
}


static  int unix_to_unix(MainParam_t *mp)
{
	return unix_write(mp, 0);
}


static int directory_dos_to_unix(direntry_t *entry, MainParam_t *mp)
{
	return unix_copydir(entry, mp);
}

/*
 * Open the named file for read, create the cluster chain, return the
 * directory structure or NULL on error.
 */
static int writeit(struct dos_name_t *dosname,
		   char *longname,
		   void *arg0,
		   direntry_t *entry)
{
	Stream_t *Target;
	time_t now;
	int type, fat, ret;
	time_t date;
	mt_size_t filesize, newsize;
	Arg_t *arg = (Arg_t *) arg0;



	if (arg->mp.File->Class->get_data(arg->mp.File,
									  & date, &filesize, &type, 0) < 0 ){
		fprintf(stderr, "Can't stat source file\n");
		return -1;
	}

	if(fileTooBig(filesize)) {
		fprintf(stderr, "File \"%s\" too big\n", longname);
		return 1;
	}

	if (type){
		if (arg->verbose)
			fprintf(stderr, "\"%s\" is a directory\n", longname);
		return -1;
	}

	/*if (!arg->single || arg->recursive)*/
	if(arg->verbose)
		fprintf(stderr,"Copying %s\n", longname);
	if(got_signal)
		return -1;

	/* will it fit? */
	if (!getfreeMinBytes(arg->mp.targetDir, filesize))
		return -1;
	
	/* preserve mod time? */
	if (arg->preserveTime)
		now = date;
	else
		getTimeNow(&now);

	mk_entry(dosname, arg->attr, 1, 0, now, &entry->dir);

	Target = OpenFileByDirentry(entry);
	if(!Target){
		fprintf(stderr,"Could not open Target\n");
		exit(1);
	}
	if (arg->needfilter & arg->textmode)
		Target = open_filter(Target,arg->convertCharset);



	ret = copyfile(arg->mp.File, Target);
	GET_DATA(Target, 0, &newsize, 0, &fat);
	FREE(&Target);
	if (arg->needfilter & arg->textmode)
	    newsize++; /* ugly hack: we gathered the size before the Ctrl-Z
			* was written.  Increment it manually */
	if(ret < 0 ){
		fat_free(arg->mp.targetDir, fat);
		return -1;
	} else {
		mk_entry(dosname, arg->attr, fat, truncBytes32(newsize),
				 now, &entry->dir);
		return 0;
	}
}



static int dos_write(direntry_t *entry, MainParam_t *mp, int needfilter)
/* write a messy dos file to another messy dos file */
{
	int result;
	Arg_t * arg = (Arg_t *) (mp->arg);
	const char *targetName = mpPickTargetName(mp);

	if(entry && arg->preserveAttributes)
		arg->attr = entry->dir.attr;
	else
		arg->attr = ATTR_ARCHIVE;

	arg->needfilter = needfilter;
	if (entry && mp->targetDir == entry->Dir){
		arg->ch.ignore_entry = -1;
		arg->ch.source = entry->entry;
	} else {
		arg->ch.ignore_entry = -1;
		arg->ch.source = -2;
	}
	result = mwrite_one(mp->targetDir, targetName, 0,
			    writeit, (void *)arg, &arg->ch);
	if(result == 1)
		return GOT_ONE;
	else
		return ERROR_ONE;
}

static Stream_t *subDir(Stream_t *parent, const char *filename)
{
	direntry_t entry;
	initializeDirentry(&entry, parent);

	switch(vfat_lookup(&entry, filename, -1, ACCEPT_DIR, 0, 0, 0, 0)) {
	    case 0:
		return OpenFileByDirentry(&entry);
	    case -1:
		return NULL;
	    default: /* IO Error */
		return NULL;
	}
}

static int dos_copydir(direntry_t *entry, MainParam_t *mp)
/* copyes a directory to Dos */
{
	Arg_t * arg = (Arg_t *) (mp->arg);
	Arg_t newArg;
	time_t now;
	time_t date;
	int ret;
	const char *targetName = mpPickTargetName(mp);

	if (!arg->recursive && mp->basenameHasWildcard)
		return 0;

	if(entry && isSubdirOf(mp->targetDir, mp->File)) {
		fprintf(stderr, "Cannot recursively copy directory ");
		fprintPwd(stderr, entry,0);
		fprintf(stderr, " into one of its own subdirectories ");
		fprintPwd(stderr, getDirentry(mp->targetDir),0);
		fprintf(stderr, "\n");
		return ERROR_ONE;
	}

	if (arg->mp.File->Class->get_data(arg->mp.File,
					  & date, 0, 0, 0) < 0 ){
		fprintf(stderr, "Can't stat source file\n");
		return ERROR_ONE;
	}

	if(!arg->type && arg->verbose)
		fprintf(stderr,"Copying %s\n", mpGetBasename(mp));

	if(entry && arg->preserveAttributes)
		arg->attr = entry->dir.attr;
	else
		arg->attr = 0;

	if (entry && (mp->targetDir == entry->Dir)){
		arg->ch.ignore_entry = -1;
		arg->ch.source = entry->entry;
	} else {
		arg->ch.ignore_entry = -1;
		arg->ch.source = -2;
	}

	/* preserve mod time? */
	if (arg->preserveTime)
		now = date;
	else
		getTimeNow(&now);

	newArg = *arg;
	newArg.mp.arg = &newArg;
	newArg.mp.targetName = 0;
	newArg.mp.basenameHasWildcard = 1;
	if(*targetName) {
		/* maybe the directory already exist. Use it */
		newArg.mp.targetDir = subDir(mp->targetDir, targetName);
		if(!newArg.mp.targetDir)
			newArg.mp.targetDir = createDir(mp->targetDir, 
							targetName,
							&arg->ch, arg->attr, 
							now);
	} else
		newArg.mp.targetDir = mp->targetDir;

	if(!newArg.mp.targetDir)
		return ERROR_ONE;

	ret = mp->loop(mp->File, &newArg.mp, "*");
	if(*targetName)
		FREE(&newArg.mp.targetDir);
	return ret | GOT_ONE;
}


static int dos_to_dos(direntry_t *entry, MainParam_t *mp)
{
	return dos_write(entry, mp, 0);
}

static int unix_to_dos(MainParam_t *mp)
{
	return dos_write(0, mp, 1);
}

static void usage(int ret) NORETURN;
static void usage(int ret)
{
	fprintf(stderr,
		"Mtools version %s, dated %s\n", mversion, mdate);
	fprintf(stderr,
		"Usage: %s [-spatnmQVBT] [-D clash_option] sourcefile targetfile\n", progname);
	fprintf(stderr,
		"       %s [-spatnmQVBT] [-D clash_option] sourcefile [sourcefiles...] targetdirectory\n", 
		progname);
	exit(ret);
}

void mcopy(int argc, char **argv, int mtype) NORETURN;
void mcopy(int argc, char **argv, int mtype)
{
	Arg_t arg;
	int c, fastquit;
	

	/* get command line options */

	init_clash_handling(& arg.ch);

	/* get command line options */
	arg.recursive = 0;
	arg.preserveTime = 0;
	arg.preserveAttributes = 0;
	arg.nowarn = 0;
	arg.textmode = 0;
	arg.verbose = 0;
	arg.convertCharset = 0;
	arg.type = mtype;
	fastquit = 0;
	if(helpFlag(argc, argv))
		usage(0);
	while ((c = getopt(argc, argv, "i:abB/sptTnmvQD:oh")) != EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg);
				break;
			case 's':
			case '/':
				arg.recursive = 1;
				break;
			case 'p':
				arg.preserveAttributes = 1;
				break;
			case 'T':
				arg.convertCharset = 1;
			case 'a':
			case 't':
				arg.textmode = 1;
				break;
			case 'n':
				arg.nowarn = 1;
				break;
			case 'm':
				arg.preserveTime = 1;
				break;
			case 'v':
				arg.verbose = 1;
				break;
			case 'Q':
				fastquit = 1;
				break;
			case 'B':
			case 'b':
				batchmode = 1;
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

	if (argc - optind < 1)
		usage(1);

	init_mp(&arg.mp);
	arg.mp.lookupflags = ACCEPT_PLAIN | ACCEPT_DIR | DO_OPEN | NO_DOTS;
	arg.mp.fast_quit = fastquit;
	arg.mp.arg = (void *) &arg;
	arg.mp.openflags = O_RDONLY;
	arg.noClobber = 0;

	/* last parameter is "-", use mtype mode */
	if(!mtype && !strcmp(argv[argc-1], "-")) {
		arg.type = mtype = 1;
		argc--;
	}

	if(mtype){
		/* Mtype = copying to stdout */
		arg.mp.targetName = strdup("-");
		arg.mp.unixTarget = strdup("");
		arg.mp.callback = dos_to_unix;
		arg.mp.dirCallback = unix_copydir;
		arg.mp.unixcallback = unix_to_unix;		
	} else {
		const char *target;
		if (argc - optind == 1) {
			/* copying to the current directory */
			target = ".";
			arg.noClobber = 1;
		} else {
			/* target is the last item mentioned */
			argc--;
			target = argv[argc];
		}

		target_lookup(&arg.mp, target);
		if(!arg.mp.targetDir && !arg.mp.unixTarget) {
			fprintf(stderr,"Bad target %s\n", target);
			exit(1);
		}

		/* callback functions */
		if(arg.mp.unixTarget) {
			arg.mp.callback = dos_to_unix;
			arg.mp.dirCallback = directory_dos_to_unix;
			arg.mp.unixcallback = unix_to_unix;
		} else {
			arg.mp.dirCallback = dos_copydir;
			arg.mp.callback = dos_to_dos;
			arg.mp.unixcallback = unix_to_dos;
		}
	}

	exit(main_loop(&arg.mp, argv + optind, argc - optind));
}

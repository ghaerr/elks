/*  Copyright 1997-2002,2005-2009 Alain Knaff.
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
 * mainloop.c
 * Iterating over all the command line parameters, and matching patterns
 * where needed
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "vfat.h"
#include "fs.h"
#include "mainloop.h"
#include "plain_io.h"
#include "file.h"
#include "file_name.h"


/* Fix the info in the MCWD file to be a proper directory name.
 * Always has a leading separator.  Never has a trailing separator
 * (unless it is the path itself).  */

static const char *fix_mcwd(char *ans)
{
	FILE *fp;
	char *s;
	char buf[MAX_PATH];

	fp = open_mcwd("r");
	if(!fp || !fgets(buf, MAX_PATH, fp)) {
		if(fp)
			fclose(fp);
		ans[0] = get_default_drive();
		strcpy(ans+1, ":/");
		return ans;
	}

	buf[strlen(buf) -1] = '\0';
	fclose(fp);
					/* drive letter present? */
	s = buf;
	if (buf[0] && buf[1] == ':') {
		memcpy(ans, buf, 2);
		ans[2] = '\0';
		s = &buf[2];
	} else {
		ans[0] = get_default_drive();
		strcpy(ans+1, ":");
	}
			/* add a leading separator */
	if (*s != '/' && *s != '\\') {
		strcat(ans, "/");
		strcat(ans, s);
	} else
		strcat(ans, s);

#if 0
					/* translate to upper case */
	for (s = ans; *s; ++s) {
		*s = ch_toupper(*s);
		if (*s == '\\')
			*s = '/';
	}
#endif
					/* if only drive, colon, & separator */
	if (strlen(ans) == 3)
		return(ans);
					/* zap the trailing separator */
	if (*--s == '/')
		*s = '\0';
	return ans;
}

int unix_dir_loop(Stream_t *Stream, MainParam_t *mp); 
int unix_loop(Stream_t *Stream UNUSEDP, MainParam_t *mp, char *arg,
	      int follow_dir_link);

static int _unix_loop(Stream_t *Dir, MainParam_t *mp,
		      const char *filename UNUSEDP)
{
	return unix_dir_loop(Dir, mp);
}

int unix_loop(Stream_t *Stream UNUSEDP, MainParam_t *mp,
	      char *arg, int follow_dir_link)
{
	int ret;
	int isdir=0;
	int unixNameLength;

	mp->File = NULL;
	mp->direntry = NULL;
	unixNameLength = strlen(arg);
	if(unixNameLength > 1 && arg[unixNameLength-1] == '/') {
	    /* names ending in slash, and having at least two characters */
	    char *name = strdup(arg);
	    name[unixNameLength-1]='\0';
	    mp->unixSourceName = name;
	} else {
	    mp->unixSourceName = arg;
	}
	/*	mp->dir.attr = ATTR_ARCHIVE;*/
	mp->loop = _unix_loop;
	if((mp->lookupflags & DO_OPEN)){
		mp->File = SimpleFileOpen(0, 0, arg, O_RDONLY, 0, 0, 0, 0);
		if(!mp->File){
			perror(arg);
#if 0
			tmp = _basename(arg);
			strncpy(mp->filename, tmp, VBUFSIZE);
			mp->filename[VBUFSIZE-1] = '\0';
#endif
			return ERROR_ONE;
		}
		GET_DATA(mp->File, 0, 0, &isdir, 0);
		if(isdir) {
#if !defined(__EMX__) && !defined(OS_mingw32msvc)
			struct MT_STAT buf;
#endif

			FREE(&mp->File);
#if !defined(__EMX__) && !defined(OS_mingw32msvc)
			if(!follow_dir_link &&
			   MT_LSTAT(arg, &buf) == 0 &&
			   S_ISLNK(buf.st_mode)) {
				/* skip links to directories in order to avoid
				 * infinite loops */
				fprintf(stderr,
					"skipping directory symlink %s\n",
					arg);
				return 0;				
			}
#endif
			if(! (mp->lookupflags & ACCEPT_DIR))
				return 0;
			mp->File = OpenDir(arg);
		}
	}

	if(isdir)
		ret = mp->dirCallback(0, mp);
	else
		ret = mp->unixcallback(mp);
	FREE(&mp->File);
	return ret;
}


int isSpecial(const char *name)
{
	if(name[0] == '\0')
		return 1;
	if(!strcmp(name,"."))
		return 1;
	if(!strcmp(name,".."))
		return 1;
	return 0;			
}

#ifdef HAVE_WCHAR_H
int isSpecialW(const wchar_t *name)
{
	if(name[0] == '\0')
		return 1;
	if(!wcscmp(name,L"."))
		return 1;
	if(!wcscmp(name,L".."))
		return 1;
	return 0;			
}
#endif

static int checkForDot(int lookupflags, const wchar_t *name)
{
	return (lookupflags & NO_DOTS) && isSpecialW(name);
}


typedef struct lookupState_t {
	Stream_t *container;
	int nbContainers;
	Stream_t *Dir;
	int nbDirs;
	const char *filename;
} lookupState_t;

static int isUniqueTarget(const char *name)
{
	return name && strcmp(name, "-");
}

static int handle_leaf(direntry_t *direntry, MainParam_t *mp,
		       lookupState_t *lookupState)
{
	Stream_t *MyFile=0;
	int ret;

	if(got_signal)
		return ERROR_ONE;
	if(lookupState) {
		/* we are looking for a "target" file */
		switch(lookupState->nbDirs) {
			case 0: /* no directory yet, open it */
				lookupState->Dir = OpenFileByDirentry(direntry);
				lookupState->nbDirs++;
				/* dump the container, we have
				 * better now */
				FREE(&lookupState->container);
				return 0;
			case 1: /* we have already a directory */
				FREE(&lookupState->Dir);
				fprintf(stderr,"Ambiguous\n");
				return STOP_NOW | ERROR_ONE;
			default:
				return STOP_NOW | ERROR_ONE;
		}
	}

	mp->direntry = direntry;
	if(IS_DIR(direntry)) {
		if(mp->lookupflags & (DO_OPEN | DO_OPEN_DIRS))
			MyFile = mp->File = OpenFileByDirentry(direntry);
		ret = mp->dirCallback(direntry, mp);
	} else {
		if(mp->lookupflags & DO_OPEN)
			MyFile = mp->File = OpenFileByDirentry(direntry);
		ret = mp->callback(direntry, mp);
	}
	FREE(&MyFile);
	if(isUniqueTarget(mp->targetName))
		ret |= STOP_NOW;
	return ret;
}

static int _dos_loop(Stream_t *Dir, MainParam_t *mp, const char *filename)
{	
	Stream_t *MyFile=0;
	direntry_t entry;
	int ret;
	int r;

	ret = 0;
	r=0;
	initializeDirentry(&entry, Dir);
	while(!got_signal &&
	      (r=vfat_lookup(&entry, filename, -1,
			     mp->lookupflags,
			     mp->shortname.data, mp->shortname.len,
			     mp->longname.data, mp->longname.len)) == 0 ){
		mp->File = NULL;
		if(!checkForDot(mp->lookupflags,entry.name)) {
			MyFile = 0;
			if((mp->lookupflags & DO_OPEN) ||
			   (IS_DIR(&entry) &&
			    (mp->lookupflags & DO_OPEN_DIRS))) {
				MyFile = mp->File = OpenFileByDirentry(&entry);
			}
			if(got_signal)
				break;
			mp->direntry = &entry;
			if(IS_DIR(&entry))
				ret |= mp->dirCallback(&entry,mp);
			else
				ret |= mp->callback(&entry, mp);
			FREE(&MyFile);
		}
		if (fat_error(Dir))
			ret |= ERROR_ONE;
		if(mp->fast_quit && (ret & ERROR_ONE))
			break;
	}
	if (r == -2)
	    return ERROR_ONE;
	if(got_signal)
		ret |= ERROR_ONE;
	return ret;
}

static int recurs_dos_loop(MainParam_t *mp, const char *filename0,
			   const char *filename1,
			   lookupState_t *lookupState)
{
	/* Dir is de-allocated by the same entity which allocated it */
	const char *ptr;
	direntry_t entry;
	int length;
	int lookupflags;
	int ret;
	int have_one;
	int doing_mcwd;
	int r;

	while(1) {
		/* strip dots and / */
		if(!strncmp(filename0,"./", 2)) {
			filename0 += 2;
			continue;
		}
		if(!strcmp(filename0,".") && filename1) {
			filename0 ++;
			continue;
		}
		if(filename0[0] == '/') {
			filename0++;
			continue;
		}
		if(!filename0[0]) {
			if(!filename1)
				break;
			filename0 = filename1;
			filename1 = 0;
			continue;
		}
		break;
	}

	if(!strncmp(filename0,"../", 3) ||
	   (!strcmp(filename0, "..") && filename1)) {
		/* up one level */
		mp->File = getDirentry(mp->File)->Dir;
		return recurs_dos_loop(mp, filename0+2, filename1, lookupState);
	}

	doing_mcwd = !!filename1;

	ptr = strchr(filename0, '/');
	if(!ptr) {			
		length = strlen(filename0);		
		ptr = filename1;
		filename1 = 0;
	} else {
		length = ptr - filename0;
		ptr++;
	}
	if(!ptr) {
		if(mp->lookupflags & OPEN_PARENT) {
			mp->targetName = filename0;
			ret = handle_leaf(getDirentry(mp->File), mp,
					  lookupState);
			mp->targetName = 0;
			return ret;
		}
		
		if(!strcmp(filename0, ".") || !filename0[0]) {
			return handle_leaf(getDirentry(mp->File),
					   mp, lookupState);
		}

		if(!strcmp(filename0, "..")) {
			return handle_leaf(getParent(getDirentry(mp->File)), mp,
					   lookupState);
		}

		lookupflags = mp->lookupflags;
		
		if(lookupState) {
			lookupState->filename = filename0;
			if(lookupState->nbContainers + lookupState->nbDirs > 0){
				/* we have already one target, don't bother
				 * with this one. */
				FREE(&lookupState->container);
			} else {
				/* no match yet.  Remember this container for
				 * later use */
				lookupState->container = COPY(mp->File);
			}
			lookupState->nbContainers++;
		}
	} else
		lookupflags = ACCEPT_DIR | DO_OPEN | NO_DOTS;

	ret = 0;
	r = 0;
	have_one = 0;
	initializeDirentry(&entry, mp->File);
	while(!(ret & STOP_NOW) &&
	      !got_signal &&
	      (r=vfat_lookup(&entry, filename0, length,
			     lookupflags | NO_MSG,
			     mp->shortname.data, mp->shortname.len,
			     mp->longname.data, mp->longname.len)) == 0 ){
		if(checkForDot(lookupflags, entry.name))
			/* while following the path, ignore the
			 * special entries if they were not
			 * explicitly given */
			continue;
		have_one = 1;
		if(ptr) {
			Stream_t *SubDir;
			SubDir = mp->File = OpenFileByDirentry(&entry);
			ret |= recurs_dos_loop(mp, ptr, filename1, lookupState);
			FREE(&SubDir);
		} else {
			ret |= handle_leaf(&entry, mp, lookupState);
			if(isUniqueTarget(mp->targetName))
				return ret | STOP_NOW;
		}
		if(doing_mcwd)
			break;
	}
	if (r == -2)
		return ERROR_ONE;
	if(got_signal)
		return ret | ERROR_ONE;
	if(doing_mcwd && !have_one)
		return NO_CWD;
	return ret;
}

static int common_dos_loop(MainParam_t *mp, const char *pathname,
			   lookupState_t *lookupState, int open_mode)

{
	Stream_t *RootDir;
	const char *cwd;
	char drive;

	int ret;
	mp->loop = _dos_loop;
	
	drive='\0';
	cwd = "";
	if(*pathname && pathname[1] == ':') {
		drive = ch_toupper(*pathname);
		pathname += 2;
		if(mp->mcwd[0] == drive)
			cwd = mp->mcwd+2;
	} else if(mp->mcwd[0]) {
		drive = mp->mcwd[0];
		cwd = mp->mcwd+2;
	} else {
		drive = get_default_drive();
	}

	if(*pathname=='/') /* absolute path name */
		cwd = "";

	RootDir = mp->File = open_root_dir(drive, open_mode, NULL);
	if(!mp->File)
		return ERROR_ONE;

	ret = recurs_dos_loop(mp, cwd, pathname, lookupState);
	if(ret & NO_CWD) {
		/* no CWD */
		*mp->mcwd = '\0';
		unlink_mcwd();
		ret = recurs_dos_loop(mp, "", pathname, lookupState);
	}
	FREE(&RootDir);
	return ret;
}

static int dos_loop(MainParam_t *mp, const char *arg)
{
	return common_dos_loop(mp, arg, 0, mp->openflags);
}


static int dos_target_lookup(MainParam_t *mp, const char *arg)
{
	lookupState_t lookupState;
	int ret;
	int lookupflags;

	lookupState.nbDirs = 0;
	lookupState.Dir = 0;
	lookupState.nbContainers = 0;
	lookupState.container = 0;

	lookupflags = mp->lookupflags;
	mp->lookupflags = DO_OPEN | ACCEPT_DIR;
	ret = common_dos_loop(mp, arg, &lookupState, O_RDWR);
	mp->lookupflags = lookupflags;
	if(ret & ERROR_ONE)
		return ret;

	if(lookupState.nbDirs) {
		mp->targetName = 0;
		mp->targetDir = lookupState.Dir;
		FREE(&lookupState.container); /* container no longer needed */
		return ret;
	}

	switch(lookupState.nbContainers) {
		case 0:
			/* no match */
			fprintf(stderr,"%s: no match for target\n", arg);
			return MISSED_ONE;
		case 1:
			mp->targetName = strdup(lookupState.filename);
			mp->targetDir = lookupState.container;
			return ret;
		default:
			/* too much */
			fprintf(stderr, "Ambiguous %s\n", arg);
			return ERROR_ONE;			
	}
}

static int unix_target_lookup(MainParam_t *mp, const char *arg)
{
	char *ptr;
	mp->unixTarget = strdup(arg);
	/* try complete filename */
	if(access(mp->unixTarget, F_OK) == 0)
		return GOT_ONE;
	ptr = strrchr(mp->unixTarget, '/');
	if(!ptr) {
		mp->targetName = mp->unixTarget;
		mp->unixTarget = strdup(".");
		return GOT_ONE;
	} else {
		*ptr = '\0';
		mp->targetName = ptr+1;
		return GOT_ONE;
	}
}

int target_lookup(MainParam_t *mp, const char *arg)
{
	if((mp->lookupflags & NO_UNIX) || (arg[0] && arg[1] == ':' ))
		return dos_target_lookup(mp, arg);
	else
		return unix_target_lookup(mp, arg);
}

int main_loop(MainParam_t *mp, char **argv, int argc)
{
	int i;
	int ret, Bret;
	
	Bret = 0;

	if(argc != 1 && mp->targetName) {
		fprintf(stderr,
			"Several file names given, but last argument (%s) not a directory\n", mp->targetName);
	}

	for (i = 0; i < argc; i++) {
		if ( got_signal )
			break;
		mp->originalArg = argv[i];
		mp->basenameHasWildcard = strpbrk(_basename(mp->originalArg),
						  "*[?") != 0;
		if (mp->unixcallback && (!argv[i][0]
#ifdef OS_mingw32msvc
/* On Windows, support only the command-line image drive. */
                                         || argv[i][0] != ':'
#endif
                                         || argv[i][1] != ':' ))
			ret = unix_loop(0, mp, argv[i], 1);
		else
			ret = dos_loop(mp, argv[i]);
		
		if (! (ret & (GOT_ONE | ERROR_ONE)) ) {
			/* one argument was unmatched */
			fprintf(stderr, "%s: File \"%s\" not found\n",
				progname, argv[i]);
			ret |= ERROR_ONE;
		}
		Bret |= ret;
		if(mp->fast_quit && (Bret & (MISSED_ONE | ERROR_ONE)))
			break;
	}
	FREE(&mp->targetDir);
	if(Bret & ERROR_ONE)
		return 1;
	if ((Bret & GOT_ONE) && ( Bret & MISSED_ONE))
		return 2;
	if (Bret & MISSED_ONE)
		return 1;
	return 0;
}

static int dispatchToFile(direntry_t *entry, MainParam_t *mp)
{
	if(entry)
		return mp->callback(entry, mp);
	else
		return mp->unixcallback(mp);
}


void init_mp(MainParam_t *mp)
{
	fix_mcwd(mp->mcwd);
	mp->openflags = O_RDONLY;
	mp->targetName = 0;
	mp->targetDir = 0;
	mp->unixTarget = 0;
	mp->dirCallback = dispatchToFile;
	mp->unixcallback = NULL;
	mp->shortname.data = mp->longname.data = 0;
	mp->shortname.len = mp->longname.len = 0;
	mp->File = 0;
	mp->fast_quit = 0;
}

const char *mpGetBasename(MainParam_t *mp)
{
	if(mp->direntry) {
		wchar_to_native(mp->direntry->name, mp->targetBuffer,
				MAX_VNAMELEN+1, sizeof(mp->targetBuffer));
		return mp->targetBuffer;
	} else
		return _basename(mp->unixSourceName);
}

void mpPrintFilename(FILE *fp, MainParam_t *mp)
{
	if(mp->direntry)
		fprintPwd(fp, mp->direntry, 0);
	else
		fprintf(fp,"%s",mp->originalArg);
}

const char *mpPickTargetName(MainParam_t *mp)
{
	/* picks the target name: either the one explicitly given by the
	 * user, or the same as the source */
	if(mp->targetName)
		return mp->targetName;
	else
		return mpGetBasename(mp);
}

char *mpBuildUnixFilename(MainParam_t *mp)
{
	const char *target;
	char *ret;
	char *tmp;

	target = mpPickTargetName(mp);
	ret = malloc(strlen(mp->unixTarget) + 2 + strlen(target));
	if(!ret)
		return 0;
	strcpy(ret, mp->unixTarget);
	if(*target) {
#if 1 /* fix for 'mcopy -n x:file existingfile' -- H. Lermen 980816 */
		if(!mp->targetName && !mp->targetDir) {
			struct MT_STAT buf;
			if (!MT_STAT(ret, &buf) && !S_ISDIR(buf.st_mode))
				return ret;
		}
#endif
		strcat(ret, "/");
		if(!strcmp(target, ".")) {
		  target="DOT";
		} else if(!strcmp(target, "..")) {
		  target="DOTDOT";
		}
		while( (tmp=strchr(target, '/')) ) {
		  strncat(ret, target, tmp-target);
		  strcat(ret, "\\");
		  target=tmp+1;
		}
		strcat(ret, target);
	}
	return ret;
}

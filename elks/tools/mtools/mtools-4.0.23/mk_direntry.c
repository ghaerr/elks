/*  Copyright 1995-1998,2000-2003,2005,2007-2009 Alain Knaff.
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
 * mk_direntry.c
 * Make new directory entries, and handles name clashes
 *
 */

/*
 * This file is used by those commands that need to create new directory entries
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "vfat.h"
#include "nameclash.h"
#include "fs.h"
#include "stream.h"
#include "mainloop.h"
#include "file_name.h"

/**
 * Converts input to shortname
 * @param un unix name (in Unix charset)
 * 
 * @return 1 if name had to be mangled
 */
static __inline__ int convert_to_shortname(doscp_t *cp, ClashHandling_t *ch,
					   const char *un, dos_name_t *dn)
{
	int mangled;

	/* Then do conversion to dn */
	ch->name_converter(cp, un, 0, &mangled, dn);
	dn->sentinel = '\0';
	if (dn->base[0] == '\xE5')
		dn->base[0] = '\x05';
	return mangled;
}

static __inline__ void chomp(char *line)
{
	int l = strlen(line);
	while(l > 0 && (line[l-1] == '\n' || line[l-1] == '\r')) {
		line[--l] = '\0';
	}
}

/**
 * Asks for an alternative new name for a file, in case of a clash
 */
static __inline__ int ask_rename(doscp_t *cp, ClashHandling_t *ch,
				 dos_name_t *shortname,
				 char *longname,
				 int isprimary)
{
	int mangled;

	/* TODO: Would be nice to suggest "autorenamed" version of name, press 
	 * <Return> to get it.
	 */
#if 0
	fprintf(stderr,"Entering ask_rename, isprimary=%d.\n", isprimary);
#endif

	if(!opentty(0))
		return 0;

	mangled = 0;
	do {
		char tname[4*MAX_VNAMELEN+1];
		fprintf(stderr, "New %s name for \"%s\": ",
			isprimary ? "primary" : "secondary", longname);
		fflush(stderr);
		if (! fgets(tname, 4*MAX_VNAMELEN+1, opentty(0)))
			return 0;
		chomp(tname);
		if (isprimary)
			strcpy(longname, tname);
		else
			mangled = convert_to_shortname(cp, 
						       ch, tname, shortname);
	} while (mangled & 1);
	return 1;
}

/**
 * This function determines the action to be taken in case there is a problem
 * with target name (clash, illegal characters, or reserved)
 * The decision either comes from the default (ch), or the user will be
 * prompted if there is no default
 */
static __inline__ clash_action ask_namematch(doscp_t *cp,
					     dos_name_t *dosname,
					     char *longname,
					     int isprimary, 
					     ClashHandling_t *ch,
					     int no_overwrite,
					     int reason)
{
	/* User's answer letter (from keyboard). Only first letter is used,
	 * but we allocate space for 10 in order to account for extra garbage
	 * that user may enter
	 */
	char ans[10];

	/**
	 * Return value: action to be taken
	 */
	clash_action a;

	/**
	 * Should this decision be made permanent (do no longer ask same
	 * question)
	 */
	int perm;

	/**
	 * Buffer for shortname
	 */
	char name_buffer[4*13];

	/**
	 * Name to be printed
	 */
	char *name;

#define EXISTS 0
#define RESERVED 1
#define ILLEGALS 2

	static const char *reasons[]= {
		"already exists",
		"is reserved",
		"contains illegal character(s)"};

	a = ch->action[isprimary];

	if(a == NAMEMATCH_NONE && !opentty(1)) {
		/* no default, and no tty either . Skip the troublesome file */
		return NAMEMATCH_SKIP;
	}

	if (!isprimary)
		name = unix_normalize(cp, name_buffer,
				      dosname, sizeof(*dosname));
	else
		name = longname;

	perm = 0;
	while (a == NAMEMATCH_NONE) {
		fprintf(stderr, "%s file name \"%s\" %s.\n",
			isprimary ? "Long" : "Short", name, reasons[reason]);
		fprintf(stderr,
			"a)utorename A)utorename-all r)ename R)ename-all ");
		if(!no_overwrite)
			fprintf(stderr,"o)verwrite O)verwrite-all");
		fprintf(stderr,
			"\ns)kip S)kip-all q)uit (aArR");
		if(!no_overwrite)
			fprintf(stderr,"oO");
		fprintf(stderr,"sSq): ");
		fflush(stderr);
		fflush(opentty(1));
		if (mtools_raw_tty) {
			int rep;
			rep = fgetc(opentty(1));			
			fputs("\n", stderr);
			if(rep == EOF)
				ans[0] = 'q';
			else
				ans[0] = rep;
		} else {
			if(fgets(ans, 9, opentty(0)) == NULL)
				ans[0] = 'q';
		}
		perm = isupper((unsigned char)ans[0]);
		switch(tolower((unsigned char)ans[0])) {
			case 'a':
				a = NAMEMATCH_AUTORENAME;
				break;
			case 'r':
				if(isprimary)
					a = NAMEMATCH_PRENAME;
				else
					a = NAMEMATCH_RENAME;
				break;
			case 'o':
				if(no_overwrite)
					continue;
				a = NAMEMATCH_OVERWRITE;
				break;
			case 's':
				a = NAMEMATCH_SKIP;
				break;
			case 'q':
				perm = 0;
				a = NAMEMATCH_QUIT;
				break;
			default:
				perm = 0;
		}
	}

	/* Keep track of this action in case this file collides again */
	ch->action[isprimary]  = a;
	if (perm)
		ch->namematch_default[isprimary] = a;

	/* if we were asked to overwrite be careful. We can't set the action
	 * to overwrite, else we get won't get a chance to specify another
	 * action, should overwrite fail. Indeed, we'll be caught in an
	 * infinite loop because overwrite will fail the same way for the
	 * second time */
	if(a == NAMEMATCH_OVERWRITE)
		ch->action[isprimary] = NAMEMATCH_NONE;
	return a;
}

/*
 * Processes a name match
 *  dosname short dosname (ignored if is_primary)
 *
 *
 * Returns:
 * 2 if file is to be overwritten
 * 1 if file was renamed
 * 0 if it was skipped
 *
 * If a short name is involved, handle conversion between the 11-character
 * fixed-length record DOS name and a literal null-terminated name (e.g.
 * "COMMAND  COM" (no null) <-> "COMMAND.COM" (null terminated)).
 *
 * Also, immediately copy the original name so that messages can use it.
 */
static __inline__ clash_action process_namematch(doscp_t *cp,
						 dos_name_t *dosname,
						 char *longname,
						 int isprimary,
						 ClashHandling_t *ch,
						 int no_overwrite,
						 int reason)
{
	clash_action action;

#if 0
	fprintf(stderr,
		"process_namematch: name=%s, default_action=%d, ask=%d.\n",
		name, default_action, ch->ask);
#endif

	action = ask_namematch(cp, dosname, longname,
			       isprimary, ch, no_overwrite, reason);

	switch(action){
	case NAMEMATCH_QUIT:
		got_signal = 1;
		return NAMEMATCH_SKIP;
	case NAMEMATCH_SKIP:
		return NAMEMATCH_SKIP;
	case NAMEMATCH_RENAME:
	case NAMEMATCH_PRENAME:
		/* We need to rename the file now.  This means we must pass
		 * back through the loop, a) ensuring there isn't a potential
		 * new name collision, and b) finding a big enough VSE.
		 * Change the name, so that it won't collide again.
		 */
		ask_rename(cp, ch, dosname, longname, isprimary);
		return action;
	case NAMEMATCH_AUTORENAME:
		/* Very similar to NAMEMATCH_RENAME, except that we need to
		 * first generate the name.
		 * TODO: Remember previous name so we don't
		 * keep trying the same one.
		 */
		if (isprimary) {
			autorename_long(longname, 1);
			return NAMEMATCH_PRENAME;
		} else {
			autorename_short(dosname, 1);
			return NAMEMATCH_RENAME;
		}
	case NAMEMATCH_OVERWRITE:
		if(no_overwrite)
			return NAMEMATCH_SKIP;
		else
			return NAMEMATCH_OVERWRITE;
	default:
		return NAMEMATCH_NONE;
	}
}

static int contains_illegals(const char *string, const char *illegals,
			     int len)
{
	for(; *string && len--; string++)
		if((*string < ' ' && *string != '\005' && !(*string & 0x80)) ||
		   strchr(illegals, *string))
			return 1;
	return 0;
}

static int is_reserved(char *ans, int islong)
{
	unsigned int i;
	static const char *dev3[] = {"CON", "AUX", "PRN", "NUL", "   "};
	static const char *dev4[] = {"COM", "LPT" };

	for (i = 0; i < sizeof(dev3)/sizeof(*dev3); i++)
		if (!strncasecmp(ans, dev3[i], 3) &&
		    ((islong && !ans[3]) ||
		     (!islong && !strncmp(ans+3,"     ",5))))
			return 1;

	for (i = 0; i < sizeof(dev4)/sizeof(*dev4); i++)
		if (!strncasecmp(ans, dev4[i], 3) &&
		    (ans[3] >= '1' && ans[3] <= '4') &&
		    ((islong && !ans[4]) ||
		     (!islong && !strncmp(ans+4,"    ",4))))
			return 1;
	
	return 0;
}

static __inline__ clash_action get_slots(Stream_t *Dir,
					 dos_name_t *dosname,
					 char *longname,
					 struct scan_state *ssp,
					 ClashHandling_t *ch)
{
	int error;
	clash_action ret;
	int match_pos=0;
	direntry_t entry;
	int isprimary;
	int no_overwrite;
	int reason;
	int pessimisticShortRename;
	doscp_t *cp = GET_DOSCONVERT(Dir);

	pessimisticShortRename = (ch->action[0] == NAMEMATCH_AUTORENAME);

	entry.Dir = Dir;
	no_overwrite = 1;
	if((is_reserved(longname,1)) ||
	   longname[strspn(longname,". ")] == '\0'){
		reason = RESERVED;
		isprimary = 1;
	} else if(contains_illegals(longname,long_illegals,1024)) {
		reason = ILLEGALS;
		isprimary = 1;
	} else if(is_reserved(dosname->base,0)) {
		reason = RESERVED;
		ch->use_longname = 1;
		isprimary = 0;
	} else if(!ch->is_label &&
		  contains_illegals(dosname->base,short_illegals,11)) {
		reason = ILLEGALS;
		ch->use_longname = 1;
		isprimary = 0;
	} else {
		reason = EXISTS;
		switch (lookupForInsert(Dir,
					&entry,
					dosname, longname, ssp,
					ch->ignore_entry,
					ch->source_entry,
					pessimisticShortRename &&
					ch->use_longname,
					ch->use_longname)) {
			case -1:
				return NAMEMATCH_ERROR;
				
			case 0:
				return NAMEMATCH_SKIP;
				/* Single-file error error or skip request */
				
			case 5:
				return NAMEMATCH_GREW;
				/* Grew directory, try again */
				
			case 6:
				return NAMEMATCH_SUCCESS; /* Success */
		}	
		match_pos = -2;
		if (ssp->longmatch > -1) {
			/* Primary Long Name Match */
#ifdef debug
			fprintf(stderr,
				"Got longmatch=%d for name %s.\n",
				longmatch, longname);
#endif			
			match_pos = ssp->longmatch;
			isprimary = 1;
		} else if ((ch->use_longname & 1) && (ssp->shortmatch != -1)) {
			/* Secondary Short Name Match */
#ifdef debug
			fprintf(stderr,
				"Got secondary short name match for name %s.\n",
				longname);
#endif

			match_pos = ssp->shortmatch;
			isprimary = 0;
		} else if (ssp->shortmatch >= 0) {
			/* Primary Short Name Match */
#ifdef debug
			fprintf(stderr,
				"Got primary short name match for name %s.\n",
				longname);
#endif
			match_pos = ssp->shortmatch;
			isprimary = 1;
		} else
			return NAMEMATCH_RENAME;

		if(match_pos > -1) {
			entry.entry = match_pos;
			dir_read(&entry, &error);
			if (error)
			    return NAMEMATCH_ERROR;
			/* if we can't overwrite, don't propose it */
			no_overwrite = (match_pos == ch->source || IS_DIR(&entry));
		}
	}
	ret = process_namematch(cp, dosname, longname,
				isprimary, ch, no_overwrite, reason);
	
	if (ret == NAMEMATCH_OVERWRITE && match_pos > -1){
		if((entry.dir.attr & 0x5) &&
		   (ask_confirmation("file is read only, overwrite anyway (y/n) ? ")))
			return NAMEMATCH_RENAME;
		/* Free up the file to be overwritten */
		if(fatFreeWithDirentry(&entry))
			return NAMEMATCH_ERROR;
		
#if 0
		if(isprimary &&
		   match_pos - ssp->match_free + 1 >= ssp->size_needed){
			/* reuse old entry and old short name for overwrite */
			ssp->free_start = match_pos - ssp->size_needed + 1;
			ssp->free_size = ssp->size_needed;
			ssp->slot = match_pos;
			ssp->got_slots = 1;
			strncpy(dosname, dir.name, 3);
			strncpy(dosname + 8, dir.ext, 3);
			return ret;
		} else
#endif
			{
			wipeEntry(&entry);
			return NAMEMATCH_RENAME;
		}
	}

	return ret;
}


static __inline__ int write_slots(Stream_t *Dir,
				  dos_name_t *dosname,
				  char *longname,
				  struct scan_state *ssp,
				  write_data_callback *cb,
				  void *arg,
				  int Case)
{
	direntry_t entry;

	/* write the file */
	if (fat_error(Dir))
		return 0;

	entry.Dir = Dir;
	entry.entry = ssp->slot;
	native_to_wchar(longname, entry.name, MAX_VNAMELEN, 0, 0);
	entry.name[MAX_VNAMELEN]='\0';
	entry.dir.Case = Case & (EXTCASE | BASECASE);
	if (cb(dosname, longname, arg, &entry) >= 0) {
		if ((ssp->size_needed > 1) &&
		    (ssp->free_end - ssp->free_start >= ssp->size_needed)) {
			ssp->slot = write_vfat(Dir, dosname, longname,
					       ssp->free_start, &entry);
		} else {
			ssp->size_needed = 1;
			write_vfat(Dir, dosname, 0,
				   ssp->free_start, &entry);
		}
		/* clear_vses(Dir, ssp->free_start + ssp->size_needed,
		   ssp->free_end); */
	} else
		return 0;

	return 1;	/* Successfully wrote the file */
}

static void stripspaces(char *name)
{
	char *p,*non_space;

	non_space = name;
	for(p=name; *p; p++)
		if (*p != ' ')
			non_space = p;
	if(name[0])
		non_space[1] = '\0';
}


static int _mwrite_one(Stream_t *Dir,
		       char *argname,
		       char *shortname,
		       write_data_callback *cb,
		       void *arg,
		       ClashHandling_t *ch)
{
	char longname[VBUFSIZE];
	const char *dstname;
	dos_name_t dosname;
	int expanded;
	struct scan_state scan;
	clash_action ret;
	doscp_t *cp = GET_DOSCONVERT(Dir);

	expanded = 0;

	if(isSpecial(argname)) {
		fprintf(stderr, "Cannot create entry named . or ..\n");
		return -1;
	}

	if(ch->name_converter == dos_name) {
		if(shortname)
			stripspaces(shortname);
		if(argname)
			stripspaces(argname);
	}

	if(shortname){
		convert_to_shortname(cp, ch, shortname, &dosname);
		if(ch->use_longname & 1){
			/* short name mangled, treat it as a long name */
			argname = shortname;
			shortname = 0;
		}
	}

	if (argname[0] && (argname[1] == ':')) {
		/* Skip drive letter */
		dstname = argname + 2;
	} else {
		dstname = argname;
	}

	/* Copy original argument dstname to working value longname */
	strncpy(longname, dstname, VBUFSIZE-1);

	if(shortname) {
		ch->use_longname =
			convert_to_shortname(cp, ch, shortname, &dosname);
		if(strcmp(shortname, longname))
			ch->use_longname |= 1;
	} else {
		ch->use_longname =
			convert_to_shortname(cp, ch, longname, &dosname);
	}

	ch->action[0] = ch->namematch_default[0];
	ch->action[1] = ch->namematch_default[1];

	while (1) {
		switch((ret=get_slots(Dir, &dosname, longname, &scan, ch))){
			case NAMEMATCH_ERROR:
				return -1;	/* Non-file-specific error,
						 * quit */
				
			case NAMEMATCH_SKIP:
				return -1;	/* Skip file (user request or
						 * error) */

			case NAMEMATCH_PRENAME:
				ch->use_longname =
					convert_to_shortname(cp, ch,
							     longname,
							     &dosname);
				continue;
			case NAMEMATCH_RENAME:
				continue;	/* Renamed file, loop again */

			case NAMEMATCH_GREW:
				/* No collision, and not enough slots.
				 * Try to grow the directory
				 */
				if (expanded) {	/* Already tried this
						 * once, no good */
					fprintf(stderr,
						"%s: No directory slots\n",
						progname);
					return -1;
				}
				expanded = 1;
				
				if (dir_grow(Dir, scan.max_entry))
					return -1;
				continue;
			case NAMEMATCH_OVERWRITE:
			case NAMEMATCH_SUCCESS:
				return write_slots(Dir, &dosname, longname,
						   &scan, cb, arg,
						   ch->use_longname);
			default:
				fprintf(stderr,
					"Internal error: clash_action=%d\n",
					ret);
				return -1;
		}

	}
}

int mwrite_one(Stream_t *Dir,
	       const char *_argname,
	       const char *_shortname,
	       write_data_callback *cb,
	       void *arg,
	       ClashHandling_t *ch)
{
	char *argname;
	char *shortname;
	int ret;

	if(_argname)
		argname = strdup(_argname);
	else
		argname = 0;
	if(_shortname)
		shortname = strdup(_shortname);
	else
		shortname = 0;
	ret = _mwrite_one(Dir, argname, shortname, cb, arg, ch);
	if(argname)
		free(argname);
	if(shortname)
		free(shortname);
	return ret;
}

void init_clash_handling(ClashHandling_t *ch)
{
	ch->ignore_entry = -1;
	ch->source_entry = -2;
	ch->nowarn = 0;	/*Don't ask, just do default action if name collision */
	ch->namematch_default[0] = NAMEMATCH_AUTORENAME;
	ch->namematch_default[1] = NAMEMATCH_NONE;
	ch->name_converter = dos_name; /* changed by mlabel */
	ch->source = -2;
	ch->is_label = 0;
}

int handle_clash_options(ClashHandling_t *ch, char c)
{
	int isprimary;
	if(isupper(c))
		isprimary = 0;
	else
		isprimary = 1;
	c = ch_tolower(c);
	switch(c) {
		case 'o':
			/* Overwrite if primary name matches */
			ch->namematch_default[isprimary] = NAMEMATCH_OVERWRITE;
			return 0;
		case 'r':
				/* Rename primary name interactively */
			ch->namematch_default[isprimary] = NAMEMATCH_RENAME;
			return 0;
		case 's':
			/* Skip file if primary name collides */
			ch->namematch_default[isprimary] = NAMEMATCH_SKIP;
			return 0;
		case 'm':
			ch->namematch_default[isprimary] = NAMEMATCH_NONE;
			return 0;
		case 'a':
			ch->namematch_default[isprimary] = NAMEMATCH_AUTORENAME;
			return 0;
		default:
			return -1;
	}
}

void dosnameToDirentry(const struct dos_name_t *dn, struct directory *dir) {
	strncpy(dir->name, dn->base, 8);
	strncpy(dir->ext, dn->ext, 3);
}

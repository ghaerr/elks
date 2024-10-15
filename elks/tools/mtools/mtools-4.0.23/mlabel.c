/*  Copyright 1986-1992 Emmet P. Gray.
 *  Copyright 1996-1998,2000-2002,2005,2007-2009 Alain Knaff.
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
 * mlabel.c
 * Make an MSDOS volume label
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mainloop.h"
#include "vfat.h"
#include "mtools.h"
#include "nameclash.h"
#include "file_name.h"

static void _label_name(doscp_t *cp, const char *filename, int verbose UNUSEDP,
			int *mangled, dos_name_t *ans, int preserve_case)
{
	int len;
	int i;
	int have_lower, have_upper;
	wchar_t wbuffer[12];

	memset(ans, ' ', sizeof(*ans)-1);
	ans->sentinel = '\0';
	len = native_to_wchar(filename, wbuffer, 11, 0, 0);
	if(len > 11){
		*mangled = 1;
		len = 11;
	} else
		*mangled = 0;

	have_lower = have_upper = 0;
	for(i=0; i<len; i++){
		if(islower(wbuffer[i]))
			have_lower = 1;
		if(isupper(wbuffer[i]))
			have_upper = 1;
		if(!preserve_case)
			wbuffer[i] = ch_towupper(wbuffer[i]);
		if(
#ifdef HAVE_WCHAR_H
		   wcschr(L"^+=/[]:,?*\\<>|\".", wbuffer[i])
#else
		   strchr("^+=/[]:,?*\\<>|\".", wbuffer[i])
#endif
		   ){
			*mangled = 1;
			wbuffer[i] = '~';
		}
	}
	if (have_lower && have_upper)
		*mangled = 1;
	wchar_to_dos(cp, wbuffer, ans->base, len, mangled);
}

void label_name_uc(doscp_t *cp, const char *filename, int verbose,
		   int *mangled, dos_name_t *ans)
{
	_label_name(cp, filename, verbose, mangled, ans, 0);
}

void label_name_pc(doscp_t *cp, const char *filename, int verbose,
		   int *mangled, dos_name_t *ans)
{
	_label_name(cp, filename, verbose, mangled, ans, 1);
}

int labelit(struct dos_name_t *dosname,
	    char *longname UNUSEDP,
	    void *arg0 UNUSEDP,
	    direntry_t *entry)
{
	time_t now;

	/* find out current time */
	getTimeNow(&now);
	mk_entry(dosname, 0x8, 0, 0, now, &entry->dir);
	return 0;
}

static void usage(int ret) NORETURN;
static void usage(int ret)
{
	fprintf(stderr, "Mtools version %s, dated %s\n",
		mversion, mdate);
	fprintf(stderr, "Usage: %s [-vscVn] [-N serial] drive:\n", progname);
	exit(ret);
}


void mlabel(int argc, char **argv, int type UNUSEDP) NORETURN;
void mlabel(int argc, char **argv, int type UNUSEDP)
{

	const char *newLabel="";
	int verbose, clear, interactive, show;
	direntry_t entry;
	int result=0;
	char longname[VBUFSIZE];
	char shortname[45];
	ClashHandling_t ch;
	struct MainParam_t mp;
	Stream_t *RootDir;
	int c;
	int mangled;
	enum { SER_NONE, SER_RANDOM, SER_SET }  set_serial = SER_NONE;
	unsigned long serial = 0;
	int need_write_boot = 0;
	int have_boot = 0;
	char *eptr;
	union bootsector boot;
	Stream_t *Fs=0;
	int r;
	struct label_blk_t *labelBlock;
	int isRo=0;
	int *isRop=NULL;
	char drive;

	init_clash_handling(&ch);
	ch.name_converter = label_name_uc;
	ch.ignore_entry = -2;
	ch.is_label = 1;

	verbose = 0;
	clear = 0;
	show = 0;

	if(helpFlag(argc, argv))
		usage(0);
	while ((c = getopt(argc, argv, "i:vcsnN:h")) != EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg);
				break;
			case 'v':
				verbose = 1;
				break;
			case 'c':
				clear = 1;
				break;
			case 's':
				show = 1;
				break;
			case 'n':
				set_serial = SER_RANDOM;
				init_random();
				serial=random();
				break;
			case 'N':
				set_serial = SER_SET;
				serial = strtoul(optarg, &eptr, 16);
				if(*eptr) {
					fprintf(stderr,
						"%s not a valid serial number\n",
						optarg);
					exit(1);
				}
				break;
			case 'h':
				usage(0);
			default:
				usage(1);
			}
	}

	if (argc - optind > 1)
		usage(1);
	if(argc - optind == 1) {
	    if(!argv[optind][0] || argv[optind][1] != ':')
		usage(1);
	    drive = ch_toupper(argv[argc -1][0]);
	    newLabel = argv[optind]+2;
	} else {
	    drive = get_default_drive();
	}

	init_mp(&mp);
	if(strlen(newLabel) > VBUFSIZE) {
		fprintf(stderr, "Label too long\n");
		FREE(&RootDir);
		exit(1);
	}

	interactive = !show && !clear &&!newLabel[0] &&
		(set_serial == SER_NONE);
	if(!clear && !newLabel[0]) {
		isRop = &isRo;
	}
	if(clear && newLabel[0]) {
		/* Clear and new label specified both */
		fprintf(stderr, "Both clear and new label specified\n");
		FREE(&RootDir);
		exit(1);
	}		
	RootDir = open_root_dir(drive, isRop ? 0 : O_RDWR, isRop);
	if(isRo) {
		show = 1;
		interactive = 0;
	}	
	if(!RootDir) {
		fprintf(stderr, "%s: Cannot initialize drive\n", argv[0]);
		exit(1);
	}

	initializeDirentry(&entry, RootDir);
	r=vfat_lookup(&entry, 0, 0, ACCEPT_LABEL | MATCH_ANY,
		      shortname, sizeof(shortname),
		      longname, sizeof(longname));
	if (r == -2) {
		FREE(&RootDir);
		exit(1);
	}

	if(show || interactive){
		if(isNotFound(&entry))
			printf(" Volume has no label\n");
		else if (*longname)
			printf(" Volume label is %s (abbr=%s)\n",
			       longname, shortname);
		else
			printf(" Volume label is %s\n",  shortname);

	}

	/* ask for new label */
	if(interactive){
		saved_sig_state ss; 
		newLabel = longname;
		allow_interrupts(&ss);
		fprintf(stderr,"Enter the new volume label : ");
		if(fgets(longname, VBUFSIZE, stdin) == NULL) {
			fprintf(stderr, "\n");
			if(errno == EINTR) {
				FREE(&RootDir);
				exit(1);
			}
			longname[0] = '\0';
		}
		if(longname[0])
			longname[strlen(newLabel)-1] = '\0';
	}

	if(strlen(newLabel) > 11) {
		fprintf(stderr,"New label too long\n");
		FREE(&RootDir);
		exit(1);
	}

	if((!show || newLabel[0]) && !isNotFound(&entry)){
		/* if we have a label, wipe it out before putting new one */
		if(interactive && newLabel[0] == '\0')
			if(ask_confirmation("Delete volume label (y/n): ")){
				FREE(&RootDir);
				exit(0);
			}
		entry.dir.attr = 0; /* for old mlabel */
		wipeEntry(&entry);
	}

	if (newLabel[0] != '\0') {
		ch.ignore_entry = 1;
		result = mwrite_one(RootDir,newLabel,0,labelit,NULL,&ch) ?
		  0 : 1;
	}

	have_boot = 0;
	if( (!show || newLabel[0]) || set_serial != SER_NONE) {
		Fs = GetFs(RootDir);
		have_boot = (force_read(Fs,boot.characters,0,sizeof(boot)) ==
			     sizeof(boot));
	}

	if(WORD_S(fatlen)) {
	    labelBlock = &boot.boot.ext.old.labelBlock;
	} else {
	    labelBlock = &boot.boot.ext.fat32.labelBlock;
	}

	if(!show || newLabel[0]){
		dos_name_t dosname;
		const char *shrtLabel;
		doscp_t *cp;
		if(!newLabel[0])
			shrtLabel = "NO NAME    ";
		else
			shrtLabel = newLabel;
		cp = GET_DOSCONVERT(Fs);
		label_name_pc(cp, shrtLabel, verbose, &mangled, &dosname);

		if(have_boot && boot.boot.descr >= 0xf0 && has_BPB4) {
			strncpy(labelBlock->label, dosname.base, 8);
			strncpy(labelBlock->label+8, dosname.ext, 3);
			need_write_boot = 1;

		}
	}

	if((set_serial != SER_NONE) & have_boot) {
		if(have_boot && boot.boot.descr >= 0xf0 && has_BPB4) {
			set_dword(labelBlock->serial, serial);	
			need_write_boot = 1;
		}
	}

	if(need_write_boot) {
		force_write(Fs, (char *)&boot, 0, sizeof(boot));
		/* If this is fat 32, write backup boot sector too */
		if(!WORD_S(fatlen)) {
			int backupBoot = WORD_S(ext.fat32.backupBoot);
			force_write(Fs, (char *)&boot, 
				    backupBoot * WORD_S(secsiz),
				    sizeof(boot));
		}
	}

	FREE(&RootDir);
	exit(result);
}

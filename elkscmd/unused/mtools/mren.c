/*
 * Rename an existing MSDOS file
 *
 * Emmet P. Gray			US Army, HQ III Corps & Fort Hood
 * ...!uunet!uiucuxc!fthood!egray	Attn: AFZF-DE-ENV
 * 					Directorate of Engineering & Housing
 * 					Environmental Management Office
 * 					Fort Hood, TX 76544-5057
 */

#include <stdio.h>
#include "msdos.h"

int fd;				/* the file descriptor for the floppy */
int dir_start;			/* starting sector for directory */
int dir_len;			/* length of directory (in sectors) */
int dir_entries;		/* number of directory entries */
int dir_chain[25];		/* chain of sectors in directory */
int clus_size;			/* cluster size (in sectors) */
int fat_len;			/* length of FAT table (in sectors) */
int num_clus;			/* number of available clusters */
unsigned char *fatbuf;		/* the File Allocation Table */
char *mcwd;			/* the Current Working Directory */

main(argc, argv)
int argc;
char *argv[];
{
	int entry, ismatch, nogo, fargn, verbose, got_it;
	char *filename, *newfile, *fixname(), *strncpy(), *unixname();
	char *getpath(), *pathname, tname[9], text[4], *getname(), *target;
	char *new, ans[10], *temp, *strcpy();
	void exit(), writedir(), free();
	struct directory *dir, *search();

	if (init(2)) {
		fprintf(stderr, "mren: Cannot initialize diskette\n");
		exit(1);
	}
	fargn = 1;
	verbose = 0;
	if (argc > 1) {
		if (!strcmp(argv[1], "-v")) {
			fargn = 2;
			verbose = 1;
		}
	}
	if (argc != fargn+2) {
		fprintf(stderr, "Usage: mren [-v] sourcefile targetfile\n");
		exit(1);
	}
	filename = getname(argv[fargn]);
	pathname = getpath(argv[fargn]);
	if (subdir(pathname))
		exit(1);

	temp = getname(argv[fargn+1]);
	target = fixname(argv[fargn+1], verbose);

	strncpy(tname, target, 8);
	strncpy(text, target+8, 3);
	tname[8] = '\0';
	text[3] = '\0';

	new = unixname(tname, text);
	nogo = 0;
					/* the name supplied may be altered */
	if (strcmp(temp, new) && verbose) {
		while (!nogo) {
			printf("Do you accept \"%s\" as the new file name (y/n) ? ", new);
			gets(ans);
			if (ans[0] == 'y' || ans[0] == 'Y')
				break;
			if (ans[0] == 'n' || ans[0] == 'N')
				nogo = 1;
		}
	}
	if (nogo)
		exit(0);
					/* see if exists and do it */
	ismatch = 0;
	for (entry=0; entry<dir_entries; entry++) {
		dir = search(entry);
					/* if empty */
		if (dir->name[0] == 0x0)
			break;
					/* if erased */
		if (dir->name[0] == 0xe5)
			continue;
					/* if volume label */
		if (dir->attr == 0x08)
			continue;
					/* you may rename a directory */
		strncpy(tname, (char *) dir->name, 8);
		strncpy(text, (char *) dir->ext, 3);
		tname[8] = '\0';
		text[3] = '\0';

		newfile = unixname(tname, text);

					/* if the new name already exists */
		if (!strcmp(new, newfile)) {
			fprintf(stderr, "mren: File \"%s\" already exists\n", new);
			exit(1);
		}
					/* if the old name exists */
		if (!strcmp(filename, newfile)) {
			ismatch = 1;
			got_it = entry;
		}
		free(newfile);
	}
	if (!ismatch) {
		fprintf(stderr, "mren: File \"%s\" not found\n", filename);
		exit(1);
	}
					/* so go ahead and do it */
	dir = search(got_it);
	strncpy((char *) dir->name, target, 8);
	strncpy((char *) dir->ext, target+8, 3);
	writedir(got_it, dir);

	close(fd);
	exit(0);
}

/*
 * Delete a MSDOS file
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
	int i, ismatch, entry, start, nogo, verbose, fargn;
	char *filename, *newfile, text[4], tname[9], *getname(), *unixname();
	char *strncpy(), *getpath(), *pathname, ans[10];
	void exit(), zapit(), writefat(), writedir(), free();
	struct directory *dir, *search();

	if (init(2)) {
		fprintf(stderr, "mdel: Cannot initialize diskette\n");
		exit(1);
	}

	if (argc < 2) {
		fprintf(stderr, "Usage: mdel [-v] msdosfile [msdosfiles...]\n");
		exit(1);
	}
	if (!strcmp(argv[1], "-v")) {
		verbose = 1;
		fargn = 2;
	}
	else {
		verbose = 0;
		fargn = 1;
	}
	for (i=fargn; i<argc; i++) {
		filename = getname(argv[i]);
		pathname = getpath(argv[i]);
		if (subdir(pathname)) {
			free(filename);
			free(pathname);
			continue;
		}
		nogo = 0;
		ismatch = 0;
		for (entry=0; entry<dir_entries; entry++) {
			dir = search(entry);
					/* if empty */
			if (dir->name[0] == 0x0)
				break;
					/* if erased */
			if (dir->name[0] == 0xe5)
				continue;
					/* if dir or volume lable */
			if ((dir->attr & 0x10) || (dir->attr & 0x08))
				continue;

			strncpy(tname, (char *) dir->name, 8);
			strncpy(text, (char *) dir->ext, 3);
			tname[8] = '\0';
			text[3] = '\0';

			newfile = unixname(tname, text);
					/* see it if matches the pattern */
			if (match(newfile, filename)) {
				if (verbose)
					printf("Removing %s\n", newfile);
				ismatch = 1;
				if (dir->attr & 0x01) {
					while (!nogo) {
						printf("mdel: \"%s\" is read only, erase anyway (y/n) ? ", newfile);
						gets(ans);
						if (ans[0] == 'y' || ans[0] == 'Y')
							break;
						if (ans[0] == 'n' || ans[0] == 'N')
							nogo = 1;
					}
					if (nogo) {
						free(newfile);
						continue;
					}
				}
				start = dir->start[1]*0x100 + dir->start[0];
				zapit(start);
				dir->name[0] = 0xe5;
				writedir(entry, dir);
			}
			free(newfile);
		}
		if (!ismatch)
			fprintf(stderr, "mdel: File \"%s\" not found\n", filename);
		free(filename);
		free(pathname);
	}
					/* update the FAT sectors */
	writefat();
	close(fd);
	exit(0);
}

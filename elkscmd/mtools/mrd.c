/*
 * Delete a MSDOS sub directory
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
	int ismatch, entry, start;
	char *filename, *newfile, text[4], tname[9], *getname();
	char *strncpy(), *pathname, *getpath(), *unixname();
	void exit(), zapit(), writefat(), writedir(), free();
	struct directory *dir, *search();

	if (init(2)) {
		fprintf(stderr, "mrd: Cannot initialize diskette\n");
		exit(1);
	}
					/* only 1 directory ! */
	if (argc != 2) {
		fprintf(stderr, "Usage: mrd mdsosdirectory\n");
		exit(1);
	}

	filename = getname(argv[1]);
	pathname = getpath(argv[1]);
	if (subdir(pathname))
		exit(1);

	ismatch = 0;
	for (entry=0; entry<dir_entries; entry++) {
		dir = search(entry);
					/* if empty */
		if (dir->name[0] == 0x0)
			break;
					/* if erased */
		if (dir->name[0] == 0xe5)
			continue;
					/* if not dir */
		if (!(dir->attr & 0x10))
			continue;

		strncpy(tname, (char *) dir->name, 8);
		strncpy(text, (char *) dir->ext, 3);
		tname[8] = '\0';
		text[3] = '\0';

		newfile = unixname(tname, text);
		if (!strcmp(newfile, filename)) {
			start = dir->start[1]*0x100 + dir->start[0];
			if (!isempty(start)) {
				fprintf(stderr, "mrd: Directory \"%s\" is not empty\n", filename);
				exit(1);
			}
			if (!start) {
				fprintf(stderr, "mrd: Can't remove root directory\n");
				exit(1);
			}
			zapit(start);
			dir->name[0] = 0xe5;
			writedir(entry, dir);
			ismatch = 1;
		}
		free(newfile);
	}
	if (!ismatch) {
		fprintf(stderr, "mrd: Directory \"%s\" not found\n", filename);
		exit(1);
	}
					/* update the FAT sectors */
	writefat();
	close(fd);
	exit(0);
}

/*
 * See if directory is empty.  Returns 1 if empty, 0 if not.  Can't use
 * subdir() and search() as it would clobber the globals.
 */

int
isempty(fat)
int fat;
{
	register int i;
	int next, buflen, sector;
	unsigned char tbuf[CLSTRBUF];
	void perror(), exit(), move();

	while (1) {
		sector = (fat-2)*clus_size + dir_start + dir_len;
		move(sector);
		buflen = clus_size * MSECSIZ;
		if (read(fd, (char *) tbuf, buflen) != buflen) {
			perror("isempty: read");
			exit(1);
		}
					/* check first character of name */
		for (i=0; i<MSECSIZ; i+=MDIRSIZ) {
			if (tbuf[i] == '.')
				continue;
			if (tbuf[i] != 0x0 && tbuf[i] != 0xe5)
				return(0);
		}
					/* get next cluster number */
		next = getfat(fat);
		if (next == -1) {
			fprintf(stderr, "isempty: FAT problem\n");
			exit(1);
		}
					/* end of cluster chain */
		if (next >= 0xff8)
			break;
		fat = next;
	}
	return(1);
}

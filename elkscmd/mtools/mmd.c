/*
 * Make a MSDOS sub directory
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
	int entry, slot, fat, dot, fargn, verbose;
	char *filename, *newfile, *fixname(), *strncpy();
	char *getpath(), *pathname, tname[9], text[4], *fixed, *unixname();
	void exit(), putcluster(), writefat(), writedir(), free();
	struct directory *dir, *search(), *mk_entry();
	long time(), now;

	if (init(2)) {
		fprintf(stderr, "mmd: Cannot initialize diskette\n");
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
					/* only 1 directory ! */
	if (argc != fargn+1) {
		fprintf(stderr, "Usage: mmd [-v] msdosdirectory\n");
		exit(1);
	}
	fixed = fixname(argv[fargn], verbose);

	strncpy(tname, fixed, 8);
	strncpy(text, fixed+8, 3);
	tname[8] = '\0';
	text[3] = '\0';

	filename = unixname(tname, text);
	pathname = getpath(argv[fargn]);

	if (subdir(pathname))
		exit(1);
					/* see if exists and get slot */
	slot = -1;
	dot = 0;
	for (entry=0; entry<dir_entries; entry++) {
		dir = search(entry);
					/* if empty */
		if (dir->name[0] == 0x0) {
			if (slot < 0)
				slot = entry;
			break;
		}
					/* if erased */
		if (dir->name[0] == 0xe5) {
			if (slot < 0)
				slot = entry;
			continue;
		}
					/* if not a directory */
		if (!(dir->attr & 0x10))
			continue;

		strncpy(tname, (char *) dir->name, 8);
		strncpy(text, (char *) dir->ext, 3);
		tname[8] = '\0';
		text[3] = '\0';

		newfile = unixname(tname, text);
					/* save the 'dot' directory info */
		if (!strcmp(".", newfile))
			dot = dir->start[1]*0x100 + dir->start[0];

		if (!strcmp(filename, newfile)) {
			fprintf(stderr, "mmd: Directory \"%s\" already exists\n", filename);
			exit(1);
		}
		free(newfile);
	}
					/* no '.' entry means root directory */
	if (dot == 0 && slot < 0) {
		fprintf(stderr, "mmd: No directory slots\n");
		exit(1);
	}
					/* make the directory grow */
	if (dot && slot < 0) {
		if (grow(dot)) {
			fprintf(stderr, "mmd: Disk full\n");
			exit(1);
		}
					/* first slot in 'new' directory */
		slot = entry;
	}
	if ((fat = nextfat(0)) == -1) {
		fprintf(stderr, "mmd: Disk full\n");
		exit(1);
	}
					/* make directory entry */
	time(&now);
	dir = mk_entry(fixed, 0x10, fat, 0L, now);
	writedir(slot, dir);
					/* write the cluster */
	putfat(fat, 0xfff);
	putcluster(fat, dot);
					/* write FAT sectors */
	writefat();
	close(fd);
	exit(0);
}

/*
 * Write a blank directory 'template' to the cluster starting at 'dot'.
 */

void
putcluster(dot, dot_dot)
int dot, dot_dot;
{
	int buflen, start;
	static struct directory dirs[32];
	struct directory *mk_entry();
	void exit(), perror(), move();
	long time(), now;

	start = (dot - 2)*clus_size + dir_start + dir_len;
	move(start);

	buflen = clus_size * MSECSIZ;
					/* make the '.' and '..' entries */	
	time(&now);
	dirs[0] = *mk_entry(".          ", 0x10, dot, 0L, now);
	dirs[1] = *mk_entry("..         ", 0x10, dot_dot, 0L, now);

	if (write(fd, (char *) &dirs[0], buflen) != buflen) {
		perror("putcluster: write");
		exit(1);
	}
	return;
}

/*
 * Returns next free cluster or -1 if none are available.
 */

int
nextfat(last)
int last;
{
	register int i;

	for (i=last+1; i<num_clus+2; i++) {
		if (!getfat(i))
			return(i);
	}
	return(-1);
}

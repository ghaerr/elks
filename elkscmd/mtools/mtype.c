/*
 * Display contents of a MSDOS file
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

long size;
long current;
int stripmode = 0;
int textmode = 0;

main(argc, argv)
int argc;
char *argv[];
{
	extern int optind;
	extern char *optarg;
	int fat, i, ismatch, entry, c, oops;
	char *filename, *newfile, text[4], tname[9], *getname(), *unixname();
	char *strncpy(), *pathname, *getpath();
	void readit(), free();
	struct directory *dir, *search();

	if (init(0)) {
		fprintf(stderr, "mtype: Cannot initialize diskette\n");
		exit(1);
	}
					/* get command line options */
	oops = 0;
	while ((c = getopt(argc, argv, "st")) != EOF) {
		switch(c) {
			case 's':
				stripmode = 1;
				break;
			case 't':
				textmode = 1;
				break;
			default:
				oops = 1;
				break;
		}
	}

	if (oops || (argc - optind) < 1) {
		fprintf(stderr, "Usage: mtype [-st] msdosfile [msdosfiles...]\n");
		exit(1);
	}

	for (i=optind; i<argc; i++) {
		filename = getname(argv[i]);
		pathname = getpath(argv[i]);
		if (subdir(pathname)) {
			free(filename);
			free(pathname);
			continue;
		}
		ismatch = 0;
		for (entry=0; entry<dir_entries; entry++) {
			dir = search(entry);
					/* if empty */
			if (dir->name[0] == 0x0)
				break;
					/* if erased */
			if (dir->name[0] == 0xe5)
				continue;
					/* if dir or volume label */
			if ((dir->attr & 0x10) || (dir->attr & 0x08))
				continue;

			strncpy(tname, (char *) dir->name, 8);
			strncpy(text, (char *) dir->ext, 3);
			tname[8] = '\0';
			text[3] = '\0';

			newfile = unixname(tname, text);
					/* see it if matches the pattern */
			if (match(newfile, filename)) {
				fat = dir->start[1]*0x100 + dir->start[0];
				size = dir->size[2]*0x10000L + dir->size[1]*0x100 + dir->size[0];
				readit(fat);
				ismatch = 1;
			}
			free(newfile);
		}
		if (!ismatch)
			fprintf(stderr, "mtype: File \"%s\" not found\n", filename);
		free(filename);
		free(pathname);
	}
	close(fd);
	exit(0);
}

/*
 * Decode the FAT chain given the begining FAT entry.
 */

void
readit(fat)
int fat;
{
	void getcluster();

	current = 0L;
	while (1) {
		getcluster(fat);
					/* get next cluster number */
		fat = getfat(fat);
		if (fat == -1) {
			fprintf(stderr, "mtype: FAT problem\n");
			exit(1);
		}
					/* end of cluster chain */
		if (fat >= 0xff8)
			break;
	}
	return;
}

/*
 * Read the named cluster, output to the stdout.
 */

void
getcluster(num)
int num;
{
	register int i;
	int buflen, start;
	unsigned char buf[CLSTRBUF];
	void exit(), perror(), move();

	start = (num - 2)*clus_size + dir_start + dir_len;
	move(start);

	buflen = clus_size * MSECSIZ;
	if (read(fd, (char *) buf, buflen) != buflen) {
		perror("getcluster: read");
		exit(1);
	}
					/* stop at size not EOF marker */
	for (i=0; i<buflen; i++) {
		current++;
		if (current > size) 
			break;
		if (textmode && buf[i] == '\r')
			continue;
		if (textmode && current == size && buf[i] == 0x1a)
			continue;
		if (stripmode)
			putchar(buf[i] & 0x7f);
		else
			putchar(buf[i]);
	}
	return;
}

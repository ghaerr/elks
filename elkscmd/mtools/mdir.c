/*
 * Display a MSDOS directory
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
char *mcwd;			/* the current working directory */

main(argc, argv)
int argc;
char *argv[];
{
	int i, entry, files, blocks, fargn, wide, faked;
	long size;
	char name[9], ext[4], *date, *time, *convdate(), *convtime();
	char *strncpy(), newpath[MAX_PATH], *getname(), *getpath(), *pathname;
	char *newfile, *filename, *unixname(), volume[12], *sep;
	char *strcpy(), *strcat(), newname[MAX_PATH], *strncat();
	void exit(), reset_dir(), free();
	struct directory *dir, *search();

	if (init(0)) {
		fprintf(stderr, "mdir: Cannot initialize diskette\n");
		exit(1);
	}
					/* find the volume label */
	reset_dir();
	volume[0] = '\0';
	for (entry=0; entry<dir_entries; entry++) {
		dir = search(entry);
					/* if empty */
		if (dir->name[0] == 0x0)
			break;
					/* if not volume label */
		if (!(dir->attr & 0x08))
			continue;

		strncpy(volume, (char *) dir->name, 8);
		volume[8] = '\0';
		strncat(volume, (char *) dir->ext, 3);
		volume[11] = '\0';
		break;
	}
	if (volume[0] == '\0')
		printf(" Volume in drive has no label\n");
	else
		printf(" Volume in drive is %s\n", volume);
	fargn = 1;
	wide = 0;
					/* first argument number */
	if (argc > 1) {
		if (!strcmp(argv[1], "-w")) {
			wide = 1;
			fargn = 2;
		}
	}
					/* fake an argument */
	faked = 0;
	if (argc == fargn) {
		faked++;
		argc++;
	}
	files = 0;
	for (i=fargn; i<argc; i++) {
		if (faked) {
			filename = getname(".");
			pathname = getpath(".");
		}
		else {
			filename = getname(argv[i]);
			pathname = getpath(argv[i]);
		}
					/* move to first guess subdirectory */
					/* required by isdir() */
		if (subdir(pathname)) {
			free(filename);
			free(pathname);
			continue;
		}
					/* is filename really a subdirectory? */
		if (isdir(filename)) {
			strcpy(newpath, pathname);
			if (strcmp(pathname,"/") && strcmp(pathname, "\\")) {
				if (*pathname != '\0')
					strcat(newpath, "/");
			}
			strcat(newpath, filename);
					/* move to real subdirectory */
			if (subdir(newpath)) {
				free(filename);
				free(pathname);
				continue;
			}
			strcpy(newname, "*");
		}
		else {
			strcpy(newpath, pathname);
			strcpy(newname, filename);
		}

		if (*filename == '\0')
			strcpy(newname, "*");

		if (*newpath == '/' || *newpath == '\\')
			printf(" Directory for %s\n\n", newpath);
		else if (!strcmp(newpath, "."))
			printf(" Directory for %s\n\n", mcwd);
		else {
			if (strlen(mcwd) == 1 || !strlen(newpath))
				sep = "";
			else
				sep = "/";
			printf(" Directory for %s%s%s\n\n", mcwd, sep, newpath);
		}
		for (entry=0; entry<dir_entries; entry++) {
			dir = search(entry);
					/* if empty */
			if (dir->name[0] == 0x0)
				break;
					/* if erased */
			if (dir->name[0] == 0xe5)
				continue;
					/* if a volume label */
			if (dir->attr & 0x08)
				continue;

			strncpy(name, (char *) dir->name, 8);
			strncpy(ext, (char *) dir->ext, 3);
			name[8] = '\0';
			ext[3] = '\0';

			newfile = unixname(name, ext);
			if (!match(newfile, newname)) {
				free(newfile);
				continue;
			}
			free(newfile);

			files++;
			if (wide && files != 1) {
				if (!((files-1) % 5))
					putchar('\n');
			}
			date = convdate(dir->date[1], dir->date[0]);
			time = convtime(dir->time[1], dir->time[0]);
			size = dir->size[2]*0x10000L + dir->size[1]*0x100 + dir->size[0];
					/* is a subdirectory */
			if (dir->attr & 0x10) {
				if (wide)
					printf("%-9.9s%-6.6s", name, ext);
				else
					printf("%8s %3s  <DIR>     %s  %s\n", name, ext, date, time);
				continue;
			}
			if (wide)
				printf("%-9.9s%-6.6s", name, ext);
			else
				printf("%8s %3s %8d   %s  %s\n", name, ext, size, date, time);
		}
		if (argc > 2)
			putchar('\n');

		free(filename);
		free(pathname);
	}

	blocks = getfree() * MSECSIZ;
	if (!files)
		printf("File \"%s\" not found\n", newname);
	else
		printf("     %3d File(s)     %6ld bytes free\n", files, blocks);
	close(fd);
	exit(0);
}

/*
 * Get the amount of free space on the diskette
 */

int
getfree()
{
	register int i, total;

	total = 0;
	for (i=2; i<num_clus+2; i++) {
					/* if getfat returns zero */
		if (!getfat(i))
			total += clus_size;
	}
	return(total);
}

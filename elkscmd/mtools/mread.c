/*
 * Read (copy) a MSDOS file to Unix
 *
 * Emmet P. Gray			US Army, HQ III Corps & Fort Hood
 * ...!uunet!uiucuxc!fthood!egray	Attn: AFZF-DE-ENV
 * 					Directorate of Engineering & Housing
 * 					Environmental Management Office
 * 					Fort Hood, TX 76544-5057
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "msdos.h"

#define LOWERCASE

int fd;				/* the file descriptor for the floppy */
int dir_start;			/* starting sector for directory */
int dir_len;			/* length of directory (in sectors) */
int dir_entries;		/* number of directory entries */
int dir_chain[25];		/* chain of sectors in directory */
int clus_size;			/* cluster size (in sectors) */
int fat_len;			/* length of FAT table (in sectors) */
int num_clus;			/* number of available clusters */
unsigned char *fatbuf;		/* the File Allocation Table */
char *buf;			/* The input buffer */
long bufsiz;			/* It's size */
int maxcontig;			/* In clusters */
char *mcwd;			/* the Current Working Directory */

extern union bootblock bb;
long size;
long current;
int textmode = 0;
int nowarn = 0;

main(argc, argv)
int argc;
char *argv[];
{
	extern int optind;
	extern char *optarg;
	int fat, i, ismatch, entry, single, c, oops, mod_time;
	long mod_date, convstamp();
	char *filename, *newfile, text[4], tname[9], *getname(), *unixname();
	char *strncpy(), *pathname, *getpath(), *target, tmp[MAX_PATH];
	char *strcat(), *strcpy();
	void perror(), exit(), free();
	struct directory *dir, *search();
	struct stat stbuf;

	if (init(0)) {
		fprintf(stderr, "mread: Cannot initialize diskette\n");
		exit(1);
	}
					/* get command line options */
	oops = 0;
	mod_time = 0;
	while ((c = getopt(argc, argv, "tnm")) != EOF) {
		switch(c) {
			case 't':
				textmode = 1;
				break;
			case 'n':
				nowarn = 1;
				break;
			case 'm':
				mod_time = 1;
				break;
			default:
				oops = 1;
				break;
		}
	}

	if (oops || (argc - optind) < 2) {
		fprintf(stderr, "Usage: mread [-tn] msdosfile unixfile\n");
		fprintf(stderr, "    or mread [-tn] msdosfile [msdosfiles...] unixdirectory\n");
		exit(1);
	}
					/* only 1 file to copy... */
	single = 1;
	target = argv[argc-1];
					/* ...unless last arg is a directory */
	if (!stat(target, &stbuf)) {
		if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
			single = 0;	
	}
					/* too many arguments */
	if (single && (argc - optind) != 2) {
		fprintf(stderr,
			"mread: too many arguments or target dir omitted\n");
		exit(1);
	}

	bufsiz = NSECT(bb.sb) * NTRACK(bb.sb) ;
	maxcontig = (bufsiz += clus_size - (--bufsiz) % clus_size)/clus_size;

	if ( (buf = (char *)malloc(bufsiz *= MSECSIZ)) == NULL ) {
		fprintf(stderr,"Could not allocate input buffer\n");
		exit(1) ;
	}

	for (i=optind; i<argc-1; i++) {
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
					/* if dir or volume lable */
			if ((dir->attr & 0x10) || (dir->attr & 0x08))
				continue;

			strncpy(tname, (char *) dir->name, 8);
			strncpy(text, (char *) dir->ext, 3);
			tname[8] = '\0';
			text[3] = '\0';

			newfile = unixname(tname, text);
			fat = dir->start[1]*0x100 + dir->start[0];
			size = dir->size[2]*0x10000L + dir->size[1]*0x100 + dir->size[0];
			if (mod_time)
				mod_date = convstamp(dir->time, dir->date);
			else
				mod_date = 0L;

					/* if single file */
			if (single) {
				if (!strcmp(newfile, filename)) {
					readit(fat, target, mod_date);
					ismatch = 1;
					break;
				}
			}
					/* if multiple files */
			else {
				if (match(newfile, filename)) {
					printf("Copying %s\n", newfile);
					strcpy(tmp, target);
					strcat(tmp, "/");
					strcat(tmp, newfile);
					readit(fat, tmp, mod_date);
					ismatch = 1;
				}
			}
			free(newfile);
		}
		if (!ismatch)
			fprintf(stderr, "mread: File \"%s\" not found\n", filename);
		free(filename);
		free(pathname);
	}
	close(fd);
	exit(0);
}

/*
 * Decode the FAT chain given the begining FAT entry, open the named Unix
 * file for write.
 */

static int readpos;

int
readit(fat, target, mod_date)
int fat;
char *target;
long mod_date;
{
	void getclusters() ;
	char ans[10];
	void exit();
	int curfat;
	FILE *fp;
	struct stat stbuf;
	struct utimbuf {
		time_t actime;
		time_t modtime;
	} utbuf;

#ifdef LOWERCASE
	char *c;
	c = target;
	while (*c) {
		if (isupper(*c))
			*c = tolower(*c);
		c++;
	}
#endif /* LOWERCASE */

	if (!nowarn) {
		if (!access(target, 0)) {
			while (1) {
				printf("File \"%s\" exists, overwrite (y/n) ? ", target);
				gets(ans);
				if (ans[0] == 'n' || ans[0] == 'N')
					return;
				if (ans[0] == 'y' || ans[0] == 'Y')
					break;
			}
					/* sanity checking */
			if (!stat(target, &stbuf)) {
				if ((stbuf.st_mode & S_IFREG) != S_IFREG) {
					fprintf(stderr, "mread: \"%s\" is not a regular file\n", target);
					return;
				}
			}
		}
	}

	if (!(fp = fopen(target, "w"))) {
		fprintf(stderr, "mread: Can't open \"%s\" for write\n", target);
		return;
	}

	current = 0L;
	readpos = -1;
	for (;;) {
		/*
		 * Find chain of contiguous clusters
		 * curfat -> last in chain + 1
		 */
		for (curfat=fat;
		    ++curfat < fat + maxcontig &&
		    getfat(curfat-1)==curfat;) ;

		getclusters(fat, curfat, fp);
					/* get next cluster number */
		fat = getfat(--curfat);
		if (fat == -1) {
			fprintf(stderr, "mread: FAT problem\n");
			exit(1);
		}
					/* end of cluster chain */
		if (fat >= 0xff8)
			break;
	}
	if (fclose(fp)) {
		fprintf(stderr,"Error closing %s\n",target) ;
		perror("close?") ;
		exit(1) ;
	}
					/* preserve mod times ? */
	if (mod_date != 0L) {
		utbuf.actime = mod_date;
		utbuf.modtime = mod_date;
		utime(target, &utbuf);
	}
	return;
}

/*
 * Read the named cluster, write to the Unix file descriptor.
 */

void
getclusters(start, end, fp)
FILE *fp;
{
	register int i;
	int buflen, blk;
	void exit(), perror(), move();

	blk = (start - 2)*clus_size + dir_start + dir_len;
	if (blk != readpos)
		move(blk);

	buflen = (end-start) * clus_size * MSECSIZ;

	if (read(fd, (char *) buf, buflen) != buflen) {
		perror("getcluster: read");
		exit(1);
	}
	readpos = blk + (end-start)*clus_size;
					/* stop at size not EOF marker */
	if (textmode) {
		for (i=0; i<buflen; i++) {
			current++;
			if (current > size) 
				break;
			if ( buf[i] == '\r')
				continue;
			if ( current == size && buf[i] == 0x1a)
				continue;
			putc((char) buf[i], fp);
		}
	}
	else {
		buflen = (size > buflen) ? buflen : size;
		if (fwrite(buf,buflen,1,fp)!=1) {
			perror("getclusters: fwrite") ;
			exit(1) ;
		}
		size -= buflen ;
	}
		
	return;
}

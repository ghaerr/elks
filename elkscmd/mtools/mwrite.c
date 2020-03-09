/*
 * Write (copy) a Unix file to MSDOS
 *
 * Emmet P. Gray			US Army, HQ III Corps & Fort Hood
 * ...!uunet!uiucuxc!fthood!egray	Attn: AFZF-DE-ENV
 * 					Directorate of Engineering & Housing
 * 					Environmental Management Office
 * 					Fort Hood, TX 76544-5057
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "msdos.h"

int fd;				/* the file descriptor for the floppy */
int dir_start;			/* starting sector for directory */
int dir_len;			/* length of directory (in sectors) */
int dir_entries;		/* number of directory entries */
int dir_chain[25];		/* chain of sectors in directory */
int clus_size;			/* cluster size (in sectors) */
unsigned long clus_len;		/* The cluster lenght in bytes */
int fat_len;			/* length of FAT table (in sectors) */
int num_clus;			/* number of available clusters */
unsigned char *fatbuf;		/* the File Allocation Table */
char *mcwd;			/* the Current Working Directory */
static char *inbuf;		/* The input buffer */
static char *outbuf;		/* The output buffer */
int bufsiz;			/* Buffer size */
int maxcontig;			/* Max contiguous clusters per write call */
long size;			/* Size of DOS file */

int full = 0;
int textmode = 0;
int nowarn = 0;
int need_nl = 0;

extern union bootblock bb;
void exit(), zapit(), writefat(), writedir(), free(), perror(), move();

main(argc, argv)
int argc;
char *argv[];
{
	extern int optind;
	extern char *optarg;
	int i, entry, ismatch, nogo, slot, start, dot, single;
	int root, c, oops, verbose, first, mod_time;
	char *filename, *newfile, tname[9], text[4], *fixname(), *getname();
	char *unixname(), ans[10], *strncpy(), *pathname, *getpath(), *fixed;
	char tmp[MAX_PATH], *target, *strcat(), *strcpy();
	struct directory *dir, *search(), *writeit();

	if (init(2)) {
		fprintf(stderr, "mwrite: Cannot initialize diskette\n");
		exit(1);
	}
					/* get command line options */
	oops = 0;
	verbose = 0;
	mod_time = 0;
	while ((c = getopt(argc, argv, "tnvm")) != EOF) {
		switch(c) {
			case 't':
				textmode = 1;
				break;
			case 'n':
				nowarn = 1;
				break;
			case 'v':
				verbose = 1;
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
		fprintf(stderr, "Usage: mwrite [-tnv] unixfile msdosfile\n");
		fprintf(stderr, "    or mwrite [-tnv] unixfile [unixfiles...] msdosdirectory\n");
		exit(1);
	}
	root = 0;
	if (!strcmp(argv[argc-1], "/") || !strcmp(argv[argc-1], "\\"))
		root = 1;

	filename = getname(argv[argc-1]);
	pathname = getpath(argv[argc-1]);
					/* test if path is ok first */
	if (subdir(pathname))
		exit(1);
					/* test if last argv is a dir */
	if (isdir(filename) || root) {
		if (!strlen(pathname)) {
					/* don't alter the presence or */
					/* absence of a leading separator */
			strcpy(tmp, filename);
		}
		else {
			strcpy(tmp, pathname);
			strcat(tmp, "/");
			strcat(tmp, filename);
		}
					/* subdir is not recursive */
		subdir(tmp);
		single = 0;
	}
	else {
		single = 1;
					/* too many arguments */
		if ((argc - optind) != 2) {
			fprintf(stderr, "mwrite: too many arguments or destination directory omitted\n");
			exit(1);
		}
	}


	clus_len = clus_size * MSECSIZ;
	/* Round cylinder up to nearest cluster multiple */
	bufsiz = NTRACK(bb.sb) * NSECT(bb.sb);
	maxcontig = (bufsiz += clus_size - (--bufsiz) % clus_size) / clus_size;
	bufsiz *= MSECSIZ;

	if ( (outbuf = (char *)malloc(bufsiz)) == NULL ||
	    ( inbuf = (char *)malloc(bufsiz)) == NULL) {
		fprintf(stderr,
			"mwrite: Cannot allocate I/O buffers\n");
		perror("malloc") ;
		exit(1);
	}

	for (i=optind; i<argc-1; i++) {
		if (single)
			fixed = fixname(argv[argc-1], verbose);
		else
			fixed = fixname(argv[i], verbose);

		strncpy(tname, fixed, 8);
		strncpy(text, fixed+8, 3);
		tname[8] = '\0';
		text[3] = '\0';

		target = unixname(tname, text);
					/* see if exists and get slot */
		ismatch = 0;
		slot = -1;
		dot = 0;
		nogo = 0;
		first = 1;
		for (entry=0; entry<dir_entries; entry++) {
			dir = search(entry);
					/* save the '.' entry info */
			if (first) {
				first = 0;
				if ((dir->attr & 0x10) && dir->name[0] == '.') {
					dot = dir->start[1]*0x100 + dir->start[0];
					continue;
				}
			}
					/* is empty */
			if (dir->name[0] == 0x0) {
				if (slot < 0)
					slot = entry;
				break;
			}
					/* is erased */
			if (dir->name[0] == 0xe5) {
				if (slot < 0)
					slot = entry;
				continue;
			}
					/* is dir or volume lable */
			if ((dir->attr & 0x10) || (dir->attr & 0x08))
				continue;

			strncpy(tname, (char *) dir->name, 8);
			strncpy(text, (char *) dir->ext, 3);
			tname[8] = '\0';
			text[3] = '\0';

			newfile = unixname(tname, text);
					/* if file exists, delete it first */
			if (!strcmp(target, newfile)) {
				ismatch = 1;
				start = dir->start[1]*0x100 + dir->start[0];
				if (nowarn) {
					zapit(start);
					dir->name[0] = 0xe5;
					writedir(entry, dir);
					if (slot < 0)
						slot = entry;
				} else {
					while (1) {
						printf("File \"%s\" exists, overwrite (y/n) ? ", target);
						gets(ans);
						if (ans[0] == 'n' || ans[0] == 'N') {
							nogo = 1;
							break;
						}
						if (ans[0] == 'y' || ans[0] == 'Y') {
							zapit(start);
							dir->name[0] = 0xe5;
							writedir(entry, dir);
							if (slot < 0)
								slot = entry;
							break;
						}
					}
				}
			}
			free(newfile);
			if (ismatch)
				break;
		}
		if (nogo) {		/* chickened out... */
			free(fixed);
			free(target);
			continue;
		}
					/* no '.' entry means root directory */
		if (dot == 0 && slot < 0) {
			fprintf(stderr, "mwrite: No directory slots\n");
			exit(1);
		}
					/* make the directory grow */
		if (dot && slot < 0) {
			if (grow(dot)) {
				fprintf(stderr, "mwrite: Disk full\n");
				exit(1);
			}
					/* first entry in 'new' directory */
			slot = entry;
		}
		if (!single)
			printf("Copying %s\n", target);
					/* write the file */
		if (dir = writeit(fixed, argv[i], verbose, mod_time))
			writedir(slot, dir);

		free(fixed);
		free(target);

		if (full) {
			fprintf(stderr, "mwrite: Disk Full\n");
			break;
		}
		if (single)
			break;
	}
					/* write FAT sectors */
	writefat();
	close(fd);
	exit(0);
}

/*
 * Open the named file for write, create the cluster chain, return the
 * directory structure or NULL on error.
 */

struct directory *
writeit(fixed, path, verbose, mod_time)
char *fixed, *path;
int verbose, mod_time;
{
	FILE *fp;
	int fat, firstfat, oldfat, curfat, chain;
	long time(), now;
	struct directory *dir, *mk_entry();
	struct stat stbuf;

	if (stat(path, &stbuf) < 0) {
		fprintf(stderr, "mwrite: Can't stat \"%s\"\n", path);
		return(NULL);
	}

	if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
		if (verbose)
			fprintf(stderr, "mwrite: \"%s\" is a directory\n", path);
		return(NULL);
	}

	if ((stbuf.st_mode & S_IFREG) != S_IFREG) {
		fprintf(stderr, "mwrite: \"%s\" is not a regular file\n", path);
		return(NULL);
	}
					/* preserve mod time? */
	if (mod_time)
		now = stbuf.st_mtime;
	else
		time(&now);

	if (!(fp = fopen(path, "r"))) {
		fprintf(stderr, "mwrite: Can't open \"%s\" for read\n", path);
		return(NULL);
	}
#ifdef HAVE_SETBUFFER
	setbuffer(fp,inbuf,bufsiz) ;
#endif

	fat = oldfat = firstfat = nextfat(size=0);
	for (;;) {
		if (fat == -1) {
			full = 1;
			if (size)
				zapit(firstfat) ;
			return(NULL) ;
		}

		/*
		 * grab a bunch of contiguous FAT slots
		 * curfat -> 1 + last grabbed slot.
		 * FIX ME!
		 * someone should try to read and cache a cylinder on
		 * first write access, do write to memory until a new
		 * cylinder is requested.
		 * The overhead is higher,  but should be more robust under
		 * fragmentation.
		 * In mread this may even be a win!  (Also see comments
		 * in putclusters() )
		 */
		for (curfat=fat;
		    ++curfat < fat + maxcontig  &&
		    nextfat(curfat-1) == curfat;) ;

		if ((oldfat=putclusters(oldfat,fat, curfat, fp)) == 0)
			break ;

		fat = nextfat(oldfat);
	}
	fclose(fp);
	dir = mk_entry(fixed, 0x20, firstfat, size, now);
	return(dir);
}

/*
 * Write to the cluster chain from the named Unix file descriptor.
 * N.B. all the clusters in the chain are contiguous.
 */


static int writepos = -1;

int
putclusters(chain,start,end,fp)
FILE *fp;
{
	static int blk;
	int c, nclust, current, eof=0;
	int buflen = ( end - start ) * MSECSIZ ;
	register char *tbuf=outbuf;

	blk = (start - 2)*clus_size + dir_start + dir_len;

	if (textmode) { /* '\n' to '\r\n' translation */
		current = 0;
		if (need_nl) {
			tbuf[current++] = '\n';
			need_nl = 0;
		}
		while (current < buflen) {
			if ((c = fgetc(fp)) == EOF) {
					/* put a file EOF marker */
				tbuf[current++] = 0x1a;
				++eof;
				break;
			}
			if (c == '\n') {
				tbuf[current++] = '\r';
				/* if at the end of the buffer */
				if (current == buflen) {
					need_nl++;
					break;
				}
			}
			tbuf[current++] = c;
		}
	}
	else {
		/*
		 * FIX ME!
		 * The kernel guarantees to satisfy REGULAR file
		 * read requests unless EOF,
		 * This code will break on pipes,  sockets,  etc.
		 * To fix one should do something akin to the
		 * "atomic" I/O of Berkeley multiprocess dump,  which loops
		 * on read/write requests until EOF or enough data has been
		 * gathered.  If you want to do this please change
		 * all instances of  read/write with Read/Write and
		 * add an offset parameter to allow arbitrary sorting/queuing
		 * etc,  behind the back of the DOS code.  This will
		 * make the overall code much cleaner,  and we
		 * can put Read/Write in a sysdep.c where they belong.
		 * Also one may want to memory map the input file.  Avoiding
		 * redundant copying to user space,  this is for perfectionists
		 * as the floppy is much slower than UNIX disks,  so gains
		 * here are small.
		 */
		if ( (current = fread(outbuf, 1, buflen, fp)) < 0) {
			perror("putcluster: fread");
			exit(1);
		}
		if ( current != buflen )
			++eof;
	}

	size += current;

	if (current == 0) {
		putfat(chain,0xfff) ;
		return(0) ;
	}

	putfat(chain,start) ;

	/*
	 * chain the clusters, we are about to overwrite
	 * making sure to terminate the chain.
	 */
	for (end=start; current > clus_len ; ++end) {
		putfat(end,end+1);
		current -= clus_len ;
	}
	putfat(end, 0xfff) ;

	if ( blk != writepos )
		move(blk);

	nclust=(end-start)+1;
	writepos = eof ? -1 : blk + nclust*clus_size;
	buflen = nclust * clus_size * MSECSIZ;

	if (write(fd, outbuf, buflen) != buflen) {
		perror("putclusters: write");
		exit(1);
	}

	return(eof ? 0: end) ;
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

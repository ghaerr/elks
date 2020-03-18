/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * The "tar" built-in command.
 */

#include "mutils.h"

#include <sys/types.h>
#include <sys/stat.h>


/*
 * Tar file format.
 */
#define TBLOCK 512
#define NAMSIZ 100

union hblock {
	char dummy[TBLOCK];
	struct header {
		char name[NAMSIZ];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[12];
		char chksum[8];
		char linkflag;
		char linkname[NAMSIZ];
		char extno[4];
		char extotal[4];
		char efsize[12];
	} dbuf;
} dblock;


static	BOOL	inheader;
static	BOOL	badheader;
static	BOOL	badwrite;
static	BOOL	extracting;
static	BOOL	warnedroot;
static	BOOL	eof;
static	BOOL	verbose;
static	long	datacc;
static	int	outfd;
static	char	outname[NAMSIZ];

static	void	doheader();
static	void	dodata();
static	void	createpath(char *name, mode_t mode);
static	long	getoctal();

int main(int argc, char **argv)
{
	char	*str;
	char	*devname;
	char	*cp;
	int	devfd;
	int	cc;
	long	incc;
	int	blocksize;
	BOOL	listflag;
	BOOL	fileflag;
	char	buf[8192];

	if (argc < 2) {
		fprintf(stderr, "Too few arguments for tar\n");
		exit(1);
	}

	extracting = FALSE;
	listflag = FALSE;
	fileflag = FALSE;
	verbose = FALSE;
	badwrite = FALSE;
	badheader = FALSE;
	warnedroot = FALSE;
	eof = FALSE;
	inheader = TRUE;
	incc = 0;
	datacc = 0;
	outfd = -1;
	blocksize = sizeof(buf);

	for (str = argv[1]; *str; str++) {
		switch (*str) {
			case 'f':	fileflag = TRUE; break;
			case 't':	listflag = TRUE; break;
			case 'x':	extracting = TRUE; break;
			case 'v':	verbose = TRUE; break;

			case 'c':
			case 'a':
				fprintf(stderr, "Writing is not supported\n");
				exit(1);

			default:
				fprintf(stderr, "Unknown tar flag\n");
				exit(1);
		}
	}

	if (!fileflag) {
		fprintf(stderr, "The 'f' flag must be specified\n");
		exit(1);
	}

	if (argc < 3) {
		fprintf(stderr, "Missing input name\n");
		exit(1);
	}
	devname = argv[2];

	if (extracting + listflag != 1) {
		fprintf(stderr, "Exactly one of 'x' or 't' must be specified\n");
		exit(1);
	}

	devfd = open(devname, 0);
	if (devfd < 0) {
		perror(devname);
		exit(1);
	}

	while (TRUE) {
		if ((incc == 0) && !eof) {
			while (incc < blocksize) {
				cc = read(devfd, &buf[incc], blocksize - incc);
				if (cc < 0) {
					perror(devname);
					goto done;
				}

				if (cc == 0)
					break;

				incc += cc;
			}
			cp = buf;
		}

		if (inheader) {
			if ((incc == 0) || eof)
				goto done;

			if (incc < TBLOCK) {
				fprintf(stderr, "Short block for header\n");
				goto done;
			}

			doheader((struct header *) cp);

			cp += TBLOCK;
			incc -= TBLOCK;

			continue;
		}

		cc = incc;
		if (cc > datacc)
			cc = datacc;

		dodata(cp, cc);

		if (cc % TBLOCK)
			cc += TBLOCK - (cc % TBLOCK);

		cp += cc;
		incc -= cc;
	}

done:
	close(devfd);
	exit(0);
}


static void
doheader(hp)
	struct	header	*hp;
{
	mode_t mode;
	int	uid;
	int	gid;
	long	size;
	time_t	mtime;
	char	*name;
	int	cc;
	BOOL	hardlink;
	BOOL	softlink;

	/*
	 * If the block is completely empty, then this is the end of the
	 * archive file.  If the name is null, then just skip this header.
	 */
	name = hp->name;
	if (*name == '\0') {
		for (cc = TBLOCK; cc > 0; cc--) {
			if (*name++)
				return;
		}

		eof = TRUE;
		return;
	}

	mode = getoctal(hp->mode, sizeof(hp->mode));
	uid = getoctal(hp->uid, sizeof(hp->uid));
	gid = getoctal(hp->gid, sizeof(hp->gid));
	size = getoctal(hp->size, sizeof(hp->size));
	mtime = getoctal(hp->mtime, sizeof(hp->mtime));
	//int chksum = getoctal(hp->chksum, sizeof(hp->chksum));

	if ((uid < 0) || (gid < 0) || (size < 0)) {
		if (!badheader)
			fprintf(stderr, "Bad tar header, skipping\n");
		badheader = TRUE;
		return;
	}

	badheader = FALSE;
	badwrite = FALSE;

	hardlink = ((hp->linkflag == 1) || (hp->linkflag == '1'));
	softlink = ((hp->linkflag == 2) || (hp->linkflag == '2'));

	if (name[strlen(name) - 1] == '/')
		mode |= S_IFDIR;
	else if ((mode & S_IFMT) == 0)
		mode |= S_IFREG;

	if (*name == '/') {
		while (*name == '/')
			name++;

		if (!warnedroot)
			fprintf(stderr, "Absolute paths detected, removing leading slashes\n");
		warnedroot = TRUE;
	}

	if (!extracting) {
		if (verbose)
                    {
			printf("%s %3d/%-d %9ld %s %s ", modestring(mode),
				uid, gid, size, timestring(mtime), name);
                    }
		else
			printf("%s", name);

		if (hardlink)
			printf(" (link to \"%s\")", hp->linkname);
		else if (softlink)
			printf(" (symlink to \"%s\")", hp->linkname);
		else if (S_ISREG(mode)) {
			inheader = (size == 0);
			datacc = size;
		}

		printf("\n");
		return;
	}

	if (verbose)
		printf("x %s\n", name);


	if (hardlink) {
		if (link(hp->linkname, name) < 0)
			perror(name);
		return;
	}

	if (softlink) {
#ifdef	S_ISLNK
		if (symlink(hp->linkname, name) < 0)
			perror(name);
#else
		fprintf(stderr, "Cannot create symbolic links\n");
#endif
		return;
	}

	if (S_ISDIR(mode)) {
		createpath(name, mode);
		return;
	}

	createpath(name, 0777);

	inheader = (size == 0);
	datacc = size;

	outfd = creat(name, mode);
	if (outfd < 0) {
		perror(name);
		badwrite = TRUE;
		return;
	}

	if (size == 0) {
		close(outfd);
		outfd = -1;
	}
}



/*
 * Handle a data block of some specified size.
 */
static void
dodata(cp, count)
	char	*cp;
{
	int	cc;

	datacc -= count;
	if (datacc <= 0)
		inheader = TRUE;

	if (badwrite || !extracting)
		return;

	while (count > 0) {
		cc = write(outfd, cp, count);
		if (cc < 0) {
			perror(outname);
			close(outfd);
			outfd = -1;
			badwrite = TRUE;
			return;
		}
		count -= cc;
	}

	if (datacc <= 0) {
		if (close(outfd))
			perror(outname);
		outfd = -1;
	}
}


/*
 * Attempt to create the directories along the specified path, except for
 * the final component.  The mode is given for the final directory only,
 * while all previous ones get default protections.  Errors are not reported
 * here, as failures to restore files can be reported later.
 */
static void
createpath(char *name, mode_t mode)
{
	char	*cp;
	char	*cpold;
	char	buf[NAMSIZ];

	strcpy(buf, name);

	cp = strchr(buf, '/');

	while (cp) {
		cpold = cp;
		cp = strchr(cp + 1, '/');

		*cpold = '\0';
		if (mkdir(buf, cp ? 0777 : mode) == 0)
			printf("Directory \"%s\" created\n", buf);

		*cpold = '/';
	}
}


/*
 * Read an octal value in a field of the specified width, with optional
 * spaces on both sides of the number and with an optional null character
 * at the end.  Returns -1 on an illegal format.
 */
static long
getoctal(cp, len)
	char	*cp;
{
	long	val;

	while ((len > 0) && (*cp == ' ')) {
		cp++;
		len--;
	}

	if ((len == 0) || !isoctal(*cp))
		return -1;

	val = 0;
	while ((len > 0) && isoctal(*cp)) {
		val = val * 8 + *cp++ - '0';
                len--;
	}

	while ((len > 0) && (*cp == ' ')) {
		cp++;
		len--;
	}

	if ((len > 0) && *cp)
		return -1;

	return val;
}

#define BUF_SIZE 1024 

typedef	struct	chunk	CHUNK;
#define	CHUNKINITSIZE	4
struct	chunk	{
	CHUNK	*next;
	char	data[CHUNKINITSIZE];	/* actually of varying length */
};


static	CHUNK *	chunklist;



/*
 * Return the standard ls-like mode string from a file mode.
 * This is static and so is overwritten on each call.
 */
char *
modestring(mode_t mode)
{
	static	char	buf[12];

	strcpy(buf, "----------");

	/*
	 * Fill in the file type.
	 */
	if (S_ISDIR(mode))
		buf[0] = 'd';
	if (S_ISCHR(mode))
		buf[0] = 'c';
	if (S_ISBLK(mode))
		buf[0] = 'b';
	if (S_ISFIFO(mode))
		buf[0] = 'p';
#ifdef	S_ISLNK
	if (S_ISLNK(mode))
		buf[0] = 'l';
#endif
#ifdef	S_ISSOCK
	if (S_ISSOCK(mode))
		buf[0] = 's';
#endif

	/*
	 * Now fill in the normal file permissions.
	 */
	if (mode & S_IRUSR)
		buf[1] = 'r';
	if (mode & S_IWUSR)
		buf[2] = 'w';
	if (mode & S_IXUSR)
		buf[3] = 'x';
	if (mode & S_IRGRP)
		buf[4] = 'r';
	if (mode & S_IWGRP)
		buf[5] = 'w';
	if (mode & S_IXGRP)
		buf[6] = 'x';
	if (mode & S_IROTH)
		buf[7] = 'r';
	if (mode & S_IWOTH)
		buf[8] = 'w';
	if (mode & S_IXOTH)
		buf[9] = 'x';

	/*
	 * Finally fill in magic stuff like suid and sticky text.
	 */
	if (mode & S_ISUID)
		buf[3] = ((mode & S_IXUSR) ? 's' : 'S');
	if (mode & S_ISGID)
		buf[6] = ((mode & S_IXGRP) ? 's' : 'S');
	if (mode & S_ISVTX)
		buf[9] = ((mode & S_IXOTH) ? 't' : 'T');

	return buf;
}

/*
 * Get the time to be used for a file.
 * This is down to the minute for new files, but only the date for old files.
 * The string is returned from a static buffer, and so is overwritten for
 * each call.
 */
char *
timestring(time_t t)
{
	time_t		now;
	char		*str;
	static	char	buf[26];

	time(&now);

	str = ctime(&t);

	strcpy(buf, &str[4]);
	buf[12] = '\0';

	if ((t > now) || (t < now - 365*24*60L*60)) {
		strcpy(&buf[7], &str[20]);
		buf[11] = '\0';
	}

	return buf;
}

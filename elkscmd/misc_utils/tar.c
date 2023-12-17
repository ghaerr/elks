/* Apr 2022 ported by Greg Haerr from Research UNIX v7 with BSD addons*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * All advertising materials mentioning features or use of this software must
 * display the following acknowledgement:
 *
 * This product includes software developed or owned by Caldera International, Inc.
 *
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA INTERNATIONAL,
 * INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL CALDERA
 * INTERNATIONAL, INC. BE LIABLE FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <utime.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/time.h>

#define ELKS            1
#define DO_REPLACE      0   /* =1 for tar 'r' option, requires awk */

typedef unsigned long daddr_t;
daddr_t	bsrch();
daddr_t	lookup();
#define TBLOCK	512
#define NBLOCK	20
#define NAMSIZ	100
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
	} dbuf;
} dblock, tbuf[NBLOCK];

struct linkbuf {
	ino_t	inum;
	dev_t	devnum;
	int	count;
	char	pathname[NAMSIZ];
	struct	linkbuf *nextp;
} *ihead;

struct stat stbuf;

int	rflag, xflag, vflag, tflag, mt, cflag, mflag;
int	term, chksum, wflag, recno, first, linkerrok;
int	hflag, oflag, pflag;
int	nblock = 1;

daddr_t	low;
daddr_t	high;

FILE	*tfile;
char	tname[] = "/tmp/tarXXXXXX";

char	*usefile;
char	magtape[]	= "/dev/mt1";

void putfile();
int readtape();
int writetape();

void
doselect(pairp, st)
int *pairp;
struct stat *st;
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n>=0 && (st->st_mode&*ap++)==0)
		ap++;
	printf("%c", *ap);
}

#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000
int	m1[] = { 1, ROWN, 'r', '-' };
int	m2[] = { 1, WOWN, 'w', '-' };
int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int	m4[] = { 1, RGRP, 'r', '-' };
int	m5[] = { 1, WGRP, 'w', '-' };
int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int	m7[] = { 1, ROTH, 'r', '-' };
int	m8[] = { 1, WOTH, 'w', '-' };
int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

void
pmode(st)
register struct stat *st;
{
	register int **mp;

	for (mp = &m[0]; mp < &m[9];)
		doselect(*mp++, st);
}

done(n)
{
	unlink(tname);
	exit(n);
}

void
usage()
{
	fprintf(stderr, "tar: usage  tar -{txu}[cvfblmhop] [tarfile] [blocksize] file1 file2...\n");
	done(1);
}

void
longt(st)
register struct stat *st;
{
	register char *cp;

	pmode(st);
	printf("%3d/%1d", st->st_uid, st->st_gid);
	printf("%7ld", st->st_size);
	cp = ctime(&st->st_mtime);
	printf(" %-12.12s %-4.4s ", cp+4, cp+20);
}

response()
{
	char c;

	c = getchar();
	if (c != '\n')
		while (getchar() != '\n');
	else c = 'n';
	return(c);
}

checkw(c, name)
char *name;
{
	if (wflag) {
		printf("%c ", c);
		if (vflag)
			longt(&stbuf);
		printf("%s: ", name);
		if (response() == 'y'){
			return(1);
		}
		return(0);
	}
	return(1);
}

checkupdate(arg)
char	*arg;
{
	long	mtime;
	daddr_t seekp;
	char name[100];

	fseek(tfile, 0L, SEEK_SET);
	for (;;) {
		if ((seekp = lookup(arg)) < 0)
			return(1);
		fseek(tfile, seekp, 0);
		fscanf(tfile, "%s %lo", name, &mtime);
		if (stbuf.st_mtime > mtime)
			return(1);
		else
			return(0);
	}
}

void
putempty()
{
	char *cp;
	char buf[TBLOCK];

	for (cp = buf; cp < &buf[TBLOCK]; )
		*cp++ = '\0';
	writetape(buf);
}

void
flushtape()
{
	write(mt, tbuf, TBLOCK*nblock);
}

void
dorep(argv)
	char *argv[];
{
	register char *cp, *cp2;
	char wdir[PATH_MAX], tempdir[PATH_MAX], *parent;

	if (!cflag) {
#if DO_REPLACE
		getdir();
		do {
			passtape();
			if (term)
				done(0);
			getdir();
		} while (!endtape());
		backtape();
		if (tfile != NULL) {
			char buf[200];

			(void)sprintf(buf,
"sort +0 -1 +1nr %s -o %s; awk '$1 != prev {print; prev=$1}' %s >%sX; mv %sX %s",
				tname, tname, tname, tname, tname, tname);
			fflush(tfile);
			system(buf);
			freopen(tname, "r", tfile);
			fstat(fileno(tfile), &stbuf);
			high = stbuf.st_size;
		}
#else
        printf("tar: replace option not supported\n");
        done(4);
#endif
	}

	getcwd(wdir, sizeof(wdir));
	while (*argv && ! term) {
		cp2 = *argv;
		if (!strcmp(cp2, "-C") && argv[1]) {
			argv++;
			if (chdir(*argv) < 0) {
				fprintf(stderr, "tar: can't change directories to ");
				perror(*argv);
			} else
				getcwd(wdir, sizeof(wdir));
			argv++;
			continue;
		}
		parent = wdir;
		for (cp = *argv; *cp; cp++)
			if (*cp == '/')
				cp2 = cp;
		if (cp2 != *argv) {
			*cp2 = '\0';
			if (chdir(*argv) < 0) {
				fprintf(stderr, "tar: can't change directories to ");
				perror(*argv);
				continue;
			}
			parent = getcwd(tempdir, sizeof(tempdir));
			*cp2 = '/';
			cp2++;
		}
		putfile(*argv++, cp2, parent);
		if (chdir(wdir) < 0) {
			fprintf(stderr, "tar: cannot change back?: ");
			perror(wdir);
		}
	}
	putempty();
	putempty();
	flushtape();
	if (linkerrok == 0)
		return;
	for (; ihead != NULL; ihead = ihead->nextp) {
		if (ihead->count == 0)
			continue;
		fprintf(stderr, "tar: missing links to %s\n", ihead->pathname);
	}
}

void
backtape()
{
	lseek(mt, (long) -TBLOCK, 1);
	if (recno >= nblock) {
		recno = nblock - 1;
		if (read(mt, tbuf, TBLOCK*nblock) < 0) {
			fprintf(stderr, "Tar: read error after seek\n");
			done(4);
		}
		lseek(mt, (long) -TBLOCK, 1);
	}
}

endtape()
{
	if (dblock.dbuf.name[0] == '\0') {
                backtape();
		return(1);
	}
	else
		return(0);
}

checksum()
{
	register i;
	register char *cp;

	for (cp = dblock.dbuf.chksum; cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
		*cp = ' ';
	i = 0;
	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		i += *cp;
	return(i);
}

void
getdir()
{
	register struct stat *sp;
	int i;

	readtape( (char *) &dblock);
	if (dblock.dbuf.name[0] == '\0')
		return;
	sp = &stbuf;
	sscanf(dblock.dbuf.mode, "%o", &i);
	sp->st_mode = i;
	sscanf(dblock.dbuf.uid, "%o", &i);
	sp->st_uid = i;
	sscanf(dblock.dbuf.gid, "%o", &i);
	sp->st_gid = i;
	sscanf(dblock.dbuf.size, "%lo", &sp->st_size);
	sscanf(dblock.dbuf.mtime, "%lo", &sp->st_mtime);
	sscanf(dblock.dbuf.chksum, "%o", &chksum);
	if (chksum != checksum()) {
		fprintf(stderr, "directory checksum error\n");
		done(2);
	}
	if (tfile != NULL)
		fprintf(tfile, "%s %s\n", dblock.dbuf.name, dblock.dbuf.mtime);
}

void
passtape()
{
	long blocks;
	char buf[TBLOCK];

	if (dblock.dbuf.linkflag == '1')
		return;
	blocks = stbuf.st_size;
	blocks += TBLOCK-1;
	blocks /= TBLOCK;

	while (blocks-- > 0)
		readtape(buf);
}

void
copy(to, from)
register char *to, *from;
{
	register i;

	i = TBLOCK;
	do {
		*to++ = *from++;
	} while (--i);
}

writetape(buffer)
char *buffer;
{
	first = 1;
	if (nblock == 0)
		nblock = 1;
	if (recno >= nblock) {
		if (write(mt, tbuf, TBLOCK*nblock) < 0) {
			fprintf(stderr, "Tar: write error %d\n", errno);
			done(2);
		}
		recno = 0;
	}
	copy(&tbuf[recno++], buffer);
	if (recno >= nblock) {
		if (write(mt, tbuf, TBLOCK*nblock) < 0) {
			fprintf(stderr, "Tar: write error %d\n", errno);
			done(2);
		}
		recno = 0;
	}
	return(TBLOCK);
}

void
tomodes(sp)
register struct stat *sp;
{
	register char *cp;

	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		*cp = '\0';
	sprintf(dblock.dbuf.mode, "%6o ", sp->st_mode & 07777);
	sprintf(dblock.dbuf.uid, "%6o ", sp->st_uid);
	sprintf(dblock.dbuf.gid, "%6o ", sp->st_gid);
	sprintf(dblock.dbuf.size, "%11lo ", sp->st_size);
	sprintf(dblock.dbuf.mtime, "%11lo ", sp->st_mtime);
}

void
putfile(longname, shortname, parent)
	char *longname;
	char *shortname;
	char *parent;
{
	int infile = 0;
	long blocks;
	register char *cp;
	struct direct *dp;
	DIR *dirp;
	register int i;
	long l;
	char newparent[NAMSIZ+64];
	char buf[TBLOCK];

	if (!hflag)
		i = lstat(shortname, &stbuf);
	else
		i = stat(shortname, &stbuf);
	if (i < 0) {
		fprintf(stderr, "tar: ");
		perror(longname);
		return;
	}
	if (tfile != NULL && checkupdate(longname) == 0)
		return;
	if (checkw('r', longname) == 0)
		return;
	//if (Fflag && checkf(shortname, stbuf.st_mode, Fflag) == 0)
		//return;

	switch (stbuf.st_mode & S_IFMT) {
	case S_IFDIR:
		for (i = 0, cp = buf; (*cp++ = longname[i++]) != '\0';)
			;
		*--cp = '/';
		*++cp = 0  ;
		if (!oflag) {
			if ((cp - buf) >= NAMSIZ) {
				fprintf(stderr, "tar: %s: file name too long\n",
				    longname);
				return;
			}
			stbuf.st_size = 0;
			tomodes(&stbuf);
			strcpy(dblock.dbuf.name,buf);
			(void)sprintf(dblock.dbuf.chksum, "%6o", checksum());
			(void) writetape((char *)&dblock);
		}
		(void)sprintf(newparent, "%s/%s", parent, shortname);
		if (chdir(shortname) < 0) {
			perror(shortname);
			return;
		}
		if ((dirp = opendir(".")) == NULL) {
			fprintf(stderr, "tar: %s: directory read error\n",
			    longname);
			if (chdir(parent) < 0) {
				fprintf(stderr, "tar: cannot change back?: ");
				perror(parent);
			}
			return;
		}
		while ((dp = readdir(dirp)) != NULL && !term) {
			if (!strcmp(".", dp->d_name) ||
			    !strcmp("..", dp->d_name))
				continue;
			strcpy(cp, dp->d_name);
			l = telldir(dirp);
			closedir(dirp);
			putfile(buf, cp, newparent);
			dirp = opendir(".");
			seekdir(dirp, l);
		}
		closedir(dirp);
		if (chdir(parent) < 0) {
			fprintf(stderr, "tar: cannot change back?: ");
			perror(parent);
		}
		break;

	case S_IFLNK:
		tomodes(&stbuf);
		if (strlen(longname) >= NAMSIZ) {
			fprintf(stderr, "tar: %s: file name too long\n",
			    longname);
			return;
		}
		strcpy(dblock.dbuf.name, longname);
		if (stbuf.st_size + 1 >= NAMSIZ) {
			fprintf(stderr, "tar: %s: symbolic link too long\n",
			    longname);
			return;
		}
		i = readlink(shortname, dblock.dbuf.linkname, NAMSIZ - 1);
		if (i < 0) {
			fprintf(stderr, "tar: can't read symbolic link ");
			perror(longname);
			return;
		}
		dblock.dbuf.linkname[i] = '\0';
		dblock.dbuf.linkflag = '2';
		if (vflag)
			fprintf(stderr, "a %s symbolic link to %s\n",
			    longname, dblock.dbuf.linkname);
		(void)sprintf(dblock.dbuf.size, "%11lo", 0L);
		(void)sprintf(dblock.dbuf.chksum, "%6o", checksum());
		(void) writetape((char *)&dblock);
		break;

	case S_IFREG:
		if ((infile = open(shortname, 0)) < 0) {
			fprintf(stderr, "tar: ");
			perror(longname);
			return;
		}
		tomodes(&stbuf);
		if (strlen(longname) >= NAMSIZ) {
			fprintf(stderr, "tar: %s: file name too long\n",
			    longname);
			close(infile);
			return;
		}
		strcpy(dblock.dbuf.name, longname);
		if (stbuf.st_nlink > 1) {
			struct linkbuf *lp;
			int found = 0;

			for (lp = ihead; lp != NULL; lp = lp->nextp)
				if (lp->inum == stbuf.st_ino &&
				    lp->devnum == stbuf.st_dev) {
					found++;
					break;
				}
			if (found) {
				strcpy(dblock.dbuf.linkname, lp->pathname);
				dblock.dbuf.linkflag = '1';
				(void)sprintf(dblock.dbuf.chksum, "%6o", checksum());
				(void) writetape( (char *) &dblock);
				if (vflag)
					fprintf(stderr, "a %s link to %s\n",
					    longname, lp->pathname);
				lp->count--;
				close(infile);
				return;
			}
			lp = (struct linkbuf *) malloc(sizeof(*lp));
			if (lp != NULL) {
				lp->nextp = ihead;
				ihead = lp;
				lp->inum = stbuf.st_ino;
				lp->devnum = stbuf.st_dev;
				lp->count = stbuf.st_nlink - 1;
				strcpy(lp->pathname, longname);
			}
		}
        blocks = (stbuf.st_size + (TBLOCK-1)) / TBLOCK;
        if (vflag) {
            fprintf(stderr, "a %s ", longname);
            fprintf(stderr, "%ld blocks\n", blocks);
        }
        sprintf(dblock.dbuf.chksum, "%6o", checksum());
        writetape( (char *) &dblock);

        while ((i = read(infile, buf, TBLOCK)) > 0 && blocks > 0) {
            writetape(buf);
            blocks--;
        }
        close(infile);
		if (i < 0) {
			fprintf(stderr, "tar: Read error on ");
			perror(longname);
        } else if (blocks != 0 || i != 0)
            fprintf(stderr, "%s: file changed size\n", longname);
        while (blocks-- >  0)
            putempty();
		break;

	default:
		fprintf(stderr, "tar: %s is not a file. Not dumped\n",
		    longname);
		break;
	}
}

prefix(s1, s2)
register char *s1, *s2;
{
	while (*s1)
		if (*s1++ != *s2++)
			return(0);
	if (*s2)
		return(*s2 == '/');
	return(1);
}

/*
 * Make all directories needed by `name'.  If `name' is itself
 * a directory on the tar tape (indicated by a trailing '/'),
 * return 1; else 0.
 */
checkdir(name)
    register char *name;
{
    register char *cp;

    /*
     * Quick check for existence of directory.
     */
    if ((cp = rindex(name, '/')) == 0)
        return (0);
    *cp = '\0';
    if (access(name, 0) == 0) { /* already exists */
        *cp = '/';
        return (cp[1] == '\0'); /* return (lastchar == '/') */
    }
    *cp = '/';

    /*
     * No luck, try to make all directories in path.
     */
    for (cp = name; *cp; cp++) {
        if (*cp != '/')
            continue;
        *cp = '\0';
        if (access(name, 0) < 0) {
            if (mkdir(name, 0777 & ~umask(0)) < 0) {
                perror(name);
                *cp = '/';
                return (0);
            }
            chown(name, stbuf.st_uid, stbuf.st_gid);
            if (pflag && cp[1] == '\0') /* dir on the tape */
                chmod(name, stbuf.st_mode & 07777);
        }
        *cp = '/';
    }
    return (cp[-1]=='/');
}


void
doxtract(argv)
char	*argv[];
{
	long blocks, bytes;
	char **cp;
	int ofile;
	char buf[TBLOCK];

	for (;;) {
		getdir();
		if (endtape())
			break;

		if (*argv == 0)
			goto gotit;

		for (cp = argv; *cp; cp++)
			if (prefix(*cp, dblock.dbuf.name))
				goto gotit;
		passtape();
		continue;

gotit:
		if (checkw('x', dblock.dbuf.name) == 0) {
			passtape();
			continue;
		}

        if (checkdir(dblock.dbuf.name)) {   /* have a directory */
            //if (mflag == 0)
                //dodirtimes(&dblock);
            continue;
        }

        if (dblock.dbuf.linkflag == '2') {  /* symlink */
            /*
             * only unlink non directories or empty
             * directories
             */
            if (rmdir(dblock.dbuf.name) < 0) {
                if (errno == ENOTDIR)
                    unlink(dblock.dbuf.name);
            }
            if (symlink(dblock.dbuf.linkname, dblock.dbuf.name)<0) {
                fprintf(stderr, "tar: %s: symbolic link failed: ",
                    dblock.dbuf.name);
                perror("");
                continue;
            }
            if (vflag)
                fprintf(stderr, "x %s symbolic link to %s\n",
                    dblock.dbuf.name, dblock.dbuf.linkname);
            continue;
        }
        if (dblock.dbuf.linkflag == '1') {  /* regular link */
            /*
             * only unlink non directories or empty
             * directories
             */
            if (rmdir(dblock.dbuf.name) < 0) {
                if (errno == ENOTDIR)
                    unlink(dblock.dbuf.name);
            }
            if (link(dblock.dbuf.linkname, dblock.dbuf.name) < 0) {
                fprintf(stderr, "tar: can't link %s to %s: ",
                    dblock.dbuf.name, dblock.dbuf.linkname);
                perror("");
                continue;
            }
            if (vflag)
                fprintf(stderr, "%s linked to %s\n",
                    dblock.dbuf.name, dblock.dbuf.linkname);
            continue;
        }

		if ((ofile = creat(dblock.dbuf.name, stbuf.st_mode & 07777)) < 0) {
			fprintf(stderr, "tar: %s - cannot create\n", dblock.dbuf.name);
			passtape();
			continue;
		}

		chown(dblock.dbuf.name, stbuf.st_uid, stbuf.st_gid);

		blocks = ((bytes = stbuf.st_size) + TBLOCK-1)/TBLOCK;
		if (vflag)
			fprintf(stderr, "x %s, %ld bytes, %ld blocks\n", dblock.dbuf.name, bytes, blocks);
		while (blocks-- > 0) {
			readtape(buf);
			if (bytes > TBLOCK) {
				if (write(ofile, buf, TBLOCK) < 0) {
					fprintf(stderr, "tar: %s: HELP - extract write error\n", dblock.dbuf.name);
					done(2);
				}
			} else
				if (write(ofile, buf, (int) bytes) < 0) {
					fprintf(stderr, "tar: %s: HELP - extract write error\n", dblock.dbuf.name);
					done(2);
				}
			bytes -= TBLOCK;
		}
		close(ofile);
		if (mflag == 0) {
			struct utimbuf ut;

			ut.actime = time(NULL);
			ut.modtime = stbuf.st_mtime;
			utime(dblock.dbuf.name, &ut);
		}
	}
}

void
dotable()
{
	for (;;) {
		getdir();
		if (endtape())
			break;
		if (vflag)
			longt(&stbuf);
		printf("%s", dblock.dbuf.name);
		if (dblock.dbuf.linkflag == '1')
			printf(" linked to %s", dblock.dbuf.linkname);
		if (dblock.dbuf.linkflag == '2')
			printf(" symbolic link to %s", dblock.dbuf.linkname);
		printf("\n");
		passtape();
	}
}

void onintr(int sig)
{
	signal(SIGINT, SIG_IGN);
	term++;
}

void onquit(int sig)
{
	signal(SIGQUIT, SIG_IGN);
	term++;
}

void onhup(int sig)
{
	signal(SIGHUP, SIG_IGN);
	term++;
}

void onterm(int sig)
{
	signal(SIGTERM, SIG_IGN);
	term++;
}

#define	N	200
int	njab;
daddr_t
lookup(s)
char *s;
{
	register i;
	daddr_t a;

	for(i=0; s[i]; i++)
		if(s[i] == ' ')
			break;
	a = bsrch(s, i, low, high);
	return(a);
}

cmp(b, s, n)
char *b, *s;
{
	register i;

	if(b[0] != '\n')
		exit(2);
	for(i=0; i<n; i++) {
		if(b[i+1] > s[i])
			return(-1);
		if(b[i+1] < s[i])
			return(1);
	}
	return(b[i+1] == ' '? 0 : -1);
}

daddr_t
bsrch(s, n, l, h)
daddr_t l, h;
char *s;
{
	register i, j;
	char b[N];
	daddr_t m, m1;

	njab = 0;

loop:
	if(l >= h)
		return(-1L);
	m = l + (h-l)/2 - N/2;
	if(m < l)
		m = l;
	fseek(tfile, m, 0);
	fread(b, 1, N, tfile);
	njab++;
	for(i=0; i<N; i++) {
		if(b[i] == '\n')
			break;
		m++;
	}
	if(m >= h)
		return(-1L);
	m1 = m;
	j = i;
	for(i++; i<N; i++) {
		m1++;
		if(b[i] == '\n')
			break;
	}
	i = cmp(b+j, s, n);
	if(i < 0) {
		h = m;
		goto loop;
	}
	if(i > 0) {
		l = m1;
		goto loop;
	}
	return(m);
}

readtape(buffer)
char *buffer;
{
	int i, j;
	int n, off = 0, total = 0;

	if (recno >= nblock || first == 0) {
		if (first == 0 && nblock == 0)
			j = NBLOCK;
		else
			j = nblock;
		n = TBLOCK * j;
again:
		if ((i = read(mt, (char *)tbuf+off, n)) < 0) {
			fprintf(stderr, "Tar: read error, %d\n", errno);
			done(3);
		}
#if ELKS
		/* keep reading as pipe reads may only return 80 characters */
		total += i;
		if (i != 0 && (i % TBLOCK) != 0) {
			off += i;
			n -= i;
			if (n != 0) goto again;
		}
		i = total;
#endif
		if (first == 0) {
			if ((i % TBLOCK) != 0) {
				fprintf(stderr, "Tar: blocksize error\n");
				done(3);
			}
			i /= TBLOCK;
			if (rflag && i != 1) {
				fprintf(stderr, "Tar: Cannot update blocked tapes (yet)\n");
				done(4);
			}
			if (i != nblock && i != 1) {
				fprintf(stderr, "Tar: blocksize = %d\n", i);
				nblock = i;
			}
		}
		recno = 0;
	}
	first = 1;
	copy(buffer, &tbuf[recno++]);
	return(TBLOCK);
}

main(argc, argv)
int	argc;
char	*argv[];
{
	char *cp;

	if (argc < 2)
		usage();

	tfile = NULL;
	usefile =  magtape;
	argv[argc] = 0;
	argv++;
	for (cp = *argv++; *cp; cp++) 
		switch(*cp) {
		case 'f':
			usefile = *argv++;
			if (nblock == 1)
				nblock = 0;
			break;
		case 'c':
			cflag++;
			rflag++;
			break;
		case 'u':
			mktemp(tname);
			if ((tfile = fopen(tname, "w")) == NULL) {
				fprintf(stderr, "Tar: cannot create temporary file (%s)\n", tname);
				done(1);
			}
			fprintf(tfile, "!!!!!/!/!/!/!/!/!/! 000\n");
			/* FALL THROUGH */
		case 'r':
			rflag++;
			if (nblock != 1 && cflag == 0) {
noupdate:
				fprintf(stderr, "Tar: Blocked tapes cannot be updated (yet)\n");
				done(1);
			}
			break;
		case 'v':
			vflag++;
			break;
		case 'w':
			wflag++;
			break;
		case 'x':
			xflag++;
			break;
		case 't':
			tflag++;
			break;
		case 'm':
			mflag++;
			break;
		case 'h':
			hflag++;
			break;
		case 'o':
			oflag++;
			break;
		case 'p':
			pflag++;
			break;
		case '-':
			break;
		case '0':
		case '1':
			magtape[7] = *cp;
			usefile = magtape;
			break;
		case 'b':
			nblock = atoi(*argv++);
			if (nblock > NBLOCK || nblock <= 0) {
				fprintf(stderr, "Invalid blocksize. (Max %d)\n", NBLOCK);
				done(1);
			}
			if (rflag && !cflag)
				goto noupdate;
			break;
		case 'l':
			linkerrok++;
			break;
		default:
			fprintf(stderr, "tar: %c: unknown option\n", *cp);
			usage();
		}

	if (rflag) {
		if (cflag && tfile != NULL)
			usage();
		if (signal(SIGINT, SIG_IGN) != SIG_IGN)
			signal(SIGINT, onintr);
		if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
			signal(SIGHUP, onhup);
		if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
			signal(SIGQUIT, onquit);
/*
		if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
			signal(SIGTERM, onterm);
*/
		if (strcmp(usefile, "-") == 0) {
			if (cflag == 0) {
				fprintf(stderr, "Can only create standard output archives\n");
				done(1);
			}
			mt = dup(1);
			nblock = 1;
		}
		else if ((mt = open(usefile, 2)) < 0) {
			if (cflag == 0 || (mt =  creat(usefile, 0666)) < 0) {
				fprintf(stderr, "tar: cannot open %s\n", usefile);
				done(1);
			}
		}
		if (cflag == 0 && nblock == 0)
			nblock = 1;
		dorep(argv);
	} else if (xflag)  {
		if (strcmp(usefile, "-") == 0) {
			mt = dup(0);
			nblock = 1;
		}
		else if ((mt = open(usefile, 0)) < 0) {
			fprintf(stderr, "tar: cannot open %s\n", usefile);
			done(1);
		}
		doxtract(argv);
	}
	else if (tflag) {
		if (strcmp(usefile, "-") == 0) {
			mt = dup(0);
			nblock = 1;
		}
		else if ((mt = open(usefile, 0)) < 0) {
			fprintf(stderr, "tar: cannot open %s\n", usefile);
			done(1);
		}
		dotable();
	}
	else
		usage();
	done(0);
}

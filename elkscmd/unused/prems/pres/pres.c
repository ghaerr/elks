/*
 * compression/decompression (c) RL 1990-2015
 *
 * A library (prem/malprem) and a cli frontend to it (pres)
 *
 * Usage: pres [-dcfFqii] [file ...]
 * Flags:
 *      -d:         Decompress instead of compressing
 *
 *      -c:         Output to stdout, without modifying file(s)
 *
 *      -f:         (Re)create the output file even if it already exists
 *                  If no -f and stdin is connected to a terminal
 *                  then a question will be asked, otherwise
 *                  the file will be recreated
 *
 *      -F:         Create the output file even if there is no saving
 *                  due to compression
 *
 *      -q:         Quiet working, except for errors
 *
 *      -i:         Continue decompression despite errors,
 *                  if given twice then ignore "end of data"
 *
 * Input:
 *      file ...:   The file(s) to compress, if none, stdin will be compressed
 *
 * Output:
 *      file.W:     Compressed version with preserved modes and owner
 *      or stdout   (if input is stdin)
 *
 *      If file names are given, replaces the files with a compressed version,
 *      adding .W suffix to the names, if compression produced a saving
 * Algorithm:
 *      LZFG (Ziv-Lempel modification by Fiala & Greene, 1989)
 *
 * $Header: pres.c,v 1.2 90/12/18 19:29:13 pro Exp $
 * $Log:	pres.c,v $
 * Revision 1.2  90/12/18  19:29:13  pro
 * Changed the order of memory freeing
 * otherwise it filled up with multiple file arguments.
 *
 * Revision 1.1  90/12/18  16:21:20  pro
 * Initial revision
 *
 */

#ifndef lint
static char rcsid[] = "$Header: pres.c,v 1.2 90/12/18 19:29:13 pro Exp $";
#else
#define void int
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <alloc.h>
#include "prem.h"
#include "pres.h"

#define MAXFNAMEL 256

#define ARGVAL() (*++(*argv) || (--argc && *++argv))


int wcat_flg = 0;   /* Output to stdout, no messages */
int quiet = 0;      /* no compression report */
int force = 0;
int ignore = 0;     /* try unpacking despite errors */
char ofname [MAXFNAMEL];
int exit_stat = 0;

int (*bgnd_flag)();

long int enk; /* input data length */
long int elk; /* compressed length */


/*
 * Exit with diagnostics
 */
int writeerr(void)
{
    perror(ofname);
    unlink(ofname);
    exit(1);
}


/*
 * Compressing input to output (one file)
 */
int pres(void)
{
    register int c;
    register LITER * d, * fin;
    LITER * enb, * elb;
    int enlon;  /* input block length */
    int ellon;  /* output block length */
    unsigned int b_n;    /* current block number */

    if ((enb=(LITER *)malloc((unsigned int)BLON)) == NULL
     || (elb=(LITER *)malloc((unsigned int)BLON)) == NULL ){
        fprintf( stderr, "pres: No memory\n");
        exit( 1);
    }
    enk = elk = 0;
    for (;;){
        d = enb;
        b_n = enk / BLON; /* the future block number */
        /* fill the output buffer */
        while (d - enb < BLON && (c=getchar()) != EOF ){
            ++enk;
            *d++ = c; /* filling the output buffer */
        }
        /* got eof on input or full buffer */
        /* actual compression */
        if ((ellon=b_prem( enb, (enlon=d-enb), elb )) > 0 ){
            /* success */
            /* the magic byte */
            c = BMARK | (enlon<BLON ? LASTA : 0) | PREMITA;
            fin = (d=elb) + ellon; /* output buffer to output */
        } else { /* did not compress */
            /* the magic byte */
            c = BMARK | (enlon<BLON ? LASTA : 0);
            fin = (d=enb) + enlon; /* input buffer to output */
        }
        /* time to do the output */
        /* the magic byte */
        putchar(c);
        putchar(~c);
        /* block number */
        putchar(b_n&0377);
        putchar((b_n>>8)&0377);
        elk += 4;
        /* possibly the block length */
        if (enlon < BLON){
            putchar(enlon & 0377);
            putchar((enlon >> 8) & 0377);
            elk += 2;
        }
        elk += fin - d;
        while (d < fin ) /* output the block */
            putchar( *d++);
        if (enlon != BLON ) break; /* incomplete block means last one */
    }
    free((char *)elb);
    free((char *)enb);
    /*
     * tell the result
     */
    if (wcat_flg == 0 && !quiet) {
        fprintf(stderr, "Compression: %d%%",
                enk > 0 ?
                ((100 * (enk - elk)) / enk) :
                0);
    }
    /* exit(2) if no compression */
    if (elk > enk) exit_stat = 2;
    fflush(stdout);
    if (ferror(stdout)) writeerr();
}

/*
 * Decompression input to output
 */

/* Decompression input function */
int en_l(void)
{
	return getchar();
}

/* the number of the found (by serch() below) block */
static unsigned int it_blk;
/* the found block length */
static int it_lon;

/* Looking for the block beginning, returns the header byte or EOF */
static int serch(void)
{
    register int i, j;
    int msg = 0;

    i = en_l();
    for (;; ){
        while (i != EOF && (i & ~(LASTA|PREMITA)) != BMARK ) i = en_l();
        if (i == EOF ) return( EOF ); /* real EOF */
        /* something found */
        if (i == ((~(j=en_l()))&0377) ){
            if ((j=en_l()) == EOF ) return( EOF);
            it_blk = j;
            if ((j=en_l()) == EOF ) return( EOF);
            it_blk += j<<8; /* block number */
            if (i & LASTA ){
                if ((j=en_l()) == EOF ) return( EOF);
                it_lon = j;
                if ((j=en_l()) == EOF ) return( EOF);
                it_lon += j<<8; /* block length */
                if (it_lon < BLON ) return( i);
            } else {
                it_lon = BLON;
                return( i);
            }
        }
        /* error! */
        if (!msg ){
            ++msg;
            fprintf( stderr, " *** Bad block header, ");
            fprintf( stderr, ignore ?
                             "continuing *** " :
                             "aborting ***\n");
        }
        if (ignore ){
                i = j;
                continue;
        }
        /* !ignore */
        break;
    }
    return( EOF);
}

static int kopir(LITER *b, unsigned int lon);

/*
 * Decompress one file
 */
int unpres(void)
{
    register LITER * d, * fin;
    register int i, j, k;
    LITER * elb;
    int (*stato)();
    unsigned int oblk; /* expected block number */
    unsigned int b_blk; /* expected number after the decompressed one */
    int lon; /* decompressed block length */

    if ((elb=(LITER *)malloc((unsigned int)BLON)) == NULL ){
        fprintf( stderr, "unpres: No memory\n");
        exit( 1);
    }
    b_blk = oblk = 0;
    i = serch();
    for (;; ){
        if (i == EOF ){ /* EOF or non-ignored error */
            fprintf( stderr, "\
 *** End of input data, finishing ***\n");
            exit_stat = 2;
            break;
        }
        if (it_blk != oblk ){  /* skipped blocks? */
            fprintf( stderr, " *** instead of block %u got %u, ",
                                              oblk,  it_blk);
            fprintf( stderr, ignore ?
                             "continuing *** " :
                             "aborting ***\n");
            if (!ignore ){
                exit_stat = 2;
                break;
            }
        }
        stato = (i & PREMITA) ? b_malprem : kopir;
        if ((*stato)( elb, it_lon ) ){
            fprintf( stderr, " *** Found incorrect data, ");
            fprintf( stderr, ignore ?
                             "continuing *** " :
                             "aborting ***\n");
            if (!ignore ){
                exit_stat = 2;
                break;
            } else {
                i = serch();
                continue;
            }
        }
        oblk = it_blk + 1;
        lon = it_lon;
        if (lon == BLON ){ /* not the last block */
            if ((i=serch()) == EOF    /* end ??? */
             || it_blk != oblk )  /* out of order */
                    continue; /* do not trust this block */
        } else { /* looks like the last one */
            if (ignore && (i=serch()) != EOF ){
                /* not last, look further! */
                fprintf( stderr, "\
 *** There is more data after the end marker, ");
                fprintf( stderr, ignore > 1 ?
                                 "continuing *** " :
                                 "but stopping here ***\n");
                if (ignore > 1 )
                    continue; /* ignoring the "last" block indication */
            }
        }
        if ((j=oblk-b_blk-1) != 0 ){
            /* the next block number in order, but here there was a skip */
            if (fseek( stdout, (long)oblk*(long)BLON, 0 ) < 0 )
                /* can not seek (pipe) */
                if (j > 0 && j <= 10 ) /* skipped blocks
                                          are not too many */
                    while (j-- ) /* fill with garbage */
                        for (k=BLON; k--; ) putchar( 'U');
        }
        b_blk = oblk;
        for (fin=(d=elb)+lon; d<fin; putchar( *d++ ) ) ;
        if (lon < BLON ) break; /* this was the last block */
    }
    free((char *)elb);
    fflush( stdout);
    if (ferror(stdout)) writeerr();
}

/* Handling an uncompressed block */
static int kopir(LITER *b, unsigned int lon)
{
    register int kod;

    while (lon--) {
        if ((kod=en_l()) == EOF) return -1;
        *b++ = kod;
    }
    return 0;
}


void copystat(char *ifname, char *ofname)
{
    struct stat statbuf;
    int mode;
    time_t timep[2];

    fclose(stdout);
    if (stat(ifname, &statbuf)) { /* check the input file state */
	perror(ifname);
	return;
    }
    if ((statbuf.st_mode & S_IFMT/*0170000*/) != S_IFREG/*0100000*/) {
	if (quiet)
		fprintf(stderr, "%s: ", ifname);
        fprintf(stderr, " -- not a file: skipped");
	exit_stat = 1;
    } else if (statbuf.st_nlink > 1) {
	if (quiet)
		fprintf(stderr, "%s: ", ifname);
        fprintf(stderr, " -- there are %d other hard links: skipped",
                statbuf.st_nlink - 1);
        exit_stat = 1;
    } else if (exit_stat == 2 && (!force)) { /* No compression: remove file.W */
        fprintf(stderr, " -- file unchanged");
    } else {                    /* ***** successful compression ***** */
        exit_stat = 0;
        mode = statbuf.st_mode & 07777;
        if (chmod(ofname, mode)) /* copy the modes,*/
            perror(ofname);
        chown(ofname, statbuf.st_uid, statbuf.st_gid); /* owner,*/
        timep[0] = statbuf.st_atime;
        timep[1] = statbuf.st_mtime;
        utime(ofname, timep);   /* timestamps */
        if (unlink(ifname))     /* remove the input file */
            perror(ifname);
        if (!quiet)
                fprintf(stderr, " -- replaced by %s", ofname);
        /* Success */
        return;
    }
    /* Too bad -- some of the checks failed */
    if (unlink(ofname))
        perror(ofname);
}

/*
 * Return 1 means foreground with stderr connected to a terminal
 */
int foreground(void)
{
        if (bgnd_flag == SIG_IGN) { /* background? */
                return 0;
        } else {                        /* foreground */
                if (isatty(2)) {         /* and stderr is a terminal */
			return 1;
		} else {
			return 0;
		}
	}
}

int onintr(void)
{
    unlink(ofname);
    exit(1);
}

int main(register int argc, char **argv)
{
    int do_decomp = 0;
    int overwrite = 0;  /* Do not recreate existing file(s) */
    char tempname[MAXFNAMEL];
    char **filelist, **fileptr;
    char *cp;
    struct stat statbuf;
    extern int onintr();


    if ((bgnd_flag = signal (SIGINT, SIG_IGN)) != SIG_IGN)
	signal(SIGINT, onintr);

    filelist = fileptr = (char **)(malloc(argc * sizeof(*argv)));
    *filelist = NULL;

    if ((cp = strrchr(argv[0], '/')) != 0) {
	cp++;
    } else {
	cp = argv[0];
    }
    if (strcmp(cp, "unpres") == 0) {
        do_decomp = 1;
    } else if (strcmp(cp, "wcat") == 0) {
        do_decomp = 1;
        wcat_flg = 1;
    }

    /* Argument parsing
     * All flags are optional
     */
    for (argc--, argv++; argc > 0; argc--, argv++) {
        if (**argv == '-') {    /* a flag */
            while (*++(*argv)) { /* parse all flags in this arg */
                switch (**argv) {
		    case 'd':
			do_decomp = 1;
			break;
		    case 'f':
			overwrite = 1;
			break;
		    case 'c':
                        wcat_flg = 1;
			break;
		    case 'q':
			quiet = 1;
			break;
		    case 'F':
			force = 1;
			break;
                    case 'i':
                        ++ignore;
                        break;
                    default:
                        fprintf(stderr, "Unknown flag: '%c'; ", **argv);
			goto usage;
		}
	    }
	}
        else {          /* Input file name */
            *fileptr++ = *argv; /* fill the input file list */
	    *fileptr = NULL;
	}
    }

    if (*filelist != NULL) {
	for (fileptr = filelist; *fileptr; fileptr++) {
	    exit_stat = 0;
            if (do_decomp != 0) {                       /* DECOMPRESSION */
                /* Check for the .W suffix */
                if (strcmp(*fileptr + strlen(*fileptr) - 2, ".W") != 0) {
                    /* No .W: add it */
                    strncpy(tempname, *fileptr, MAXFNAMEL-3);
                    tempname[MAXFNAMEL-3]=0;
                    strcat(tempname, ".W");
                    *fileptr = tempname;
                }
                /* Open the input file */
                if ((freopen(*fileptr, "r", stdin)) == NULL) {
                        perror(*fileptr); continue;
                }
                /* Generate the output file name */
                strncpy(ofname, *fileptr, MAXFNAMEL-1);
                ofname[MAXFNAMEL-1]=0;
                ofname[strlen(ofname) - 2] = '\0';  /* Cut off .W */
            } else {                                    /* COMPRESSION */
                if (strcmp(*fileptr + strlen(*fileptr) - 2, ".W") == 0) {
                    fprintf(stderr, "%s: .W already present -- skipping\n",
                            *fileptr);
                    continue;
                }
                /* Open the output file */
                if ((freopen(*fileptr, "r", stdin)) == NULL) {
                    perror(*fileptr); continue;
                }
                stat(*fileptr, &statbuf);
                /* Build the output name */
                strncpy(ofname, *fileptr, MAXFNAMEL-3);
                ofname[MAXFNAMEL-3]=0;
#ifdef MAXFNAMEC          /* short names */
                if ((cp = strrchr(ofname,'/')) != NULL)    cp++;
                else                                    cp = ofname;
                if (strlen(cp) > MAXFNAMEC-2) {
                    fprintf(stderr,"%s: too long name, no space for .W\n",cp);
                    continue;
                }
#endif  /* MAXFNAMEC */          /* long names are allowed */
                strcat(ofname, ".W");
            }
            /* check the presence of the output file */
            if (overwrite == 0 && wcat_flg == 0) {
                if (stat(ofname, &statbuf) == 0) {
                    char response[2];
                    response[0] = 'n';
                    fprintf(stderr, "%s already exists;", ofname);
                    if (foreground()) {
                        fprintf(stderr," recreate (y or n)? ");
                        fflush(stderr);
                        read(2, response, 2);
                        while (response[1] != '\n') {
                            if (read(2, response+1, 1) < 0){
                                perror("stderr"); break;
                            }
                        }
                    }
                    if (response[0] != 'y'
                     && response[0] != 'Y' ) {
                        fprintf(stderr, "\tskipped\n");
                        continue;
                    }
                }
            }
            if (wcat_flg == 0) {         /* Open the output file */
                if (freopen(ofname, "w", stdout) == NULL) {
                    perror(ofname);
                    continue;
                }
                if (!quiet)
                        fprintf(stderr, "%s: ", *fileptr);
            }

            /* the actual compressing/decompressing */
            if (do_decomp == 0) pres();
            else                unpres();
            if (wcat_flg == 0) {
                copystat(*fileptr, ofname); /* preserve the state */
                if (exit_stat || (!quiet))
                        putc('\n', stderr);
            }
        }
    } else {            /* stdio */
        if (do_decomp == 0) {
                pres();
                if (!quiet)
                        putc('\n', stderr);
        } else {
            unpres();
        }
    }
    exit(exit_stat);

usage:
    fprintf(stderr,"Usage: pres [-dfFqcii] [file ...]\n");
    exit(1);
}

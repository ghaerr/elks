/*
 * FLPARC - recording/reading data on removable data, with compression
 * (c) RL 1990-2015
 *
 * $Header: flparc.c,v 1.2 91/01/24 14:11:35 pro Exp $
 * $Log:	flparc.c,v $
 * Revision 1.2  91/01/24  14:11:35  pro
 * message correction
 *
 * Revision 1.1  90/12/18  16:20:43  pro
 * Initial revision
 *
 */

#ifndef lint
char rcsid[] = "$Header: flparc.c,v 1.2 91/01/24 14:11:35 pro Exp $";
#else
#define void int
#endif

#include "prem.h" /* To compress/decompress */
#include "pres.h" /* Data format */
#include <ediag.h>
#include <alloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define B_LON   512 /* I/O block length */
#define DEFFIL "/etc/default/tar" /* defaults file */

char *afn = 0;    /* Archive file name */
int dlon = 0;     /* Media size */
int premado = -2; /* Compression/recording flag */
int fd;           /* Archive file descriptor */
int bkf = 0;      /* Blockfactor */
unsigned char *b; /* Blocking buffer */
int bi = 0;       /* Index in the blocking buffer */
unsigned int bimaks; /* Size of the blocking buffer */
long skp = 0;     /* Pointer in the archive file */
long num = 0;     /* Number of the bytes input for compression */
int  flp = -1;    /* Number of the filled media */
int verb = 0;     /* Tell the current compression ratio */
int ignore = 0;   /* Ignore read errors at decompression */
int exit_stat;    /* Exit status */

/*** Prototypes ***/
void pres(void);
void unpres(void);

/* Prompt and media change */
void shan_fl(void)
{
    char c, d;

    ++flp;
    if ( premado && verb && flp > 0 ){
	fprintf( stderr, ediag(
	    " Currently compressed %d%%\n",
	    " Nuntempa premado %d%%\n"),
 (100*(num-((long)flp*dlon*B_LON)))/num );
/* note this works up to ca 21MB of size reduction (overflow of 100*(long)) */
    }
    fprintf( stderr, ediag(
	"\
 Insert medium #%d and press <ENTER> or `n'<ENTER> to abort ",
	"\
 Instalu diskon #%d kaj premu <ENIGU> or `n'<ENIGU> por fini "),
                     flp+1 );
    read( 2, &c, 1 );
    d = c;
    while ( c != '\n' )
	read( 2, &c, 1 );
    if ( d == 'n' || d == 'N' ){
	fprintf( stderr, ediag(
	    " Aborted by user\n",
	    " Finita fare de uzanto\n") );
        exit( exit_stat );
    }
    skp = 0;
}

/* Blocking buffer flush */
void ell_fin(void)
{
    if ( skp/B_LON >= dlon ) shan_fl(); /* Change the media */
    lseek( fd, skp, 0 );
    if ( write( fd, b, bimaks ) < bimaks ){ /* The whole buffer */
	fprintf( stderr, ediag(
	    " Write error! Block %d\n",
	    " Skriberaro! Bloko %d\n"), skp/B_LON );
	    exit_stat = 5;
    }
    if ( num > 0 && verb ) fprintf( stderr, ediag(
	" Compressed: %d%%\n",
	" Kunpremite: %d%%\n"),
	(100*(num-((long)flp*dlon*B_LON+skp+bi)))/num );
/* note this works up to ca 21MB of size reduction (overflow of 100*(long)) */
    skp = 0;
    bi = 0;
}

/* Writing with compression */
void rikor(void)
{
    shan_fl(); /* ask to insert the first diskette */
    pres();
    ell_fin(); /* clean the output buffer */
}

/* Reading with decompression */
void leg(void)
{
    bi = bimaks;
    shan_fl(); /* ask to insert the first diskette */
    unpres();
}

/* Read defaults from a file */
void tdefa(char c)
{
    /* c = logical number of the media type ('0'-'9' or 'f') */
    FILE *deffil;
    static char buf[BUFSIZ];
    static char archfn[100]; /* For the archive file name */
    register char *p;
    char *estas(char*, char);

    if ( (deffil=fopen(DEFFIL,"r")) == NULL ) return; /* no such file */
    do{
        if ( fgets( buf, BUFSIZ, deffil ) == NULL ){
            fclose( deffil );
            return; /* not found */
        }
    } while ( !(p=estas( buf, c )) ); /* unsuitable line in the buffer */
    fclose( deffil );
    /* found a line, p points to the archive file name */
    /* set the missing defaults */
    if ( !afn ){                   /* beginning of the name */
        afn = archfn;
        while ( *p > ' ' ) *afn++ = *p++;  /* copy the name */
        *afn = 0;
        afn = archfn;
    } else while ( *p > ' ' ) ++p;         /* skip the name */
    while ( *p && *p <= ' ') ++p;  /* skip the delimiter */
    if ( !bkf ) bkf = atoi( p );   /* beginning of the blocking factor */
    while ( *p > ' ' ) ++p;        /* skip the blocking factor */
    while ( *p && *p <= ' ') ++p;  /* skip the delimiter */
    if ( !dlon ) dlon = atoi( p ); /* beginning of the media size (KB!) */
}

char *estas(char *b, char c) /* look for a pattern */
{
    static char bbb[] = "archiveX=";

    bbb[7] = c;
    if (strncmp(b, bbb, 9)) return 0;
    return(b + 9);
}

/* Output a byte with multiblock buffering */
static void elll(int l)
{
    if ( bi >= bimaks ){
	if ( skp/B_LON >= dlon ) shan_fl(); /* Change the media */
	lseek( fd, skp, 0 );
	if ( write( fd, b, bimaks ) < bimaks ){
	    fprintf( stderr, ediag(
		" Write error! Block %d\n",
		" Skriberaro! Bloko %d\n"), skp/B_LON );
	    exit_stat = 5;
	    if ( !ignore ) exit( exit_stat );
        }
	skp += bimaks;
	bi = 0;
    }
    b[bi++] = l;
}

/* Read one byte for decompression */
int en_l(void)
{
    if ( bi >= bimaks ){
	if ( skp/B_LON >= dlon ) shan_fl(); /* Change the media */
	lseek( fd, skp, 0 );
	if ( read( fd, b, bimaks ) < bimaks ){
	    fprintf( stderr, ediag(
		" Read error! Block %d\n",
		" Legeraro! Bloko %d\n"), skp/B_LON );
	    exit_stat = 5;
	    if ( !ignore ) exit( exit_stat );
	}
	skp += bimaks;
	bi = 0;
    }
    return( b[bi++] );
}

/*
 * Compression
 */
void pres(void)
{
    register int c;
    register LITER * d, * fin;
    LITER * enb, * elb;
    int enlon;  /* input block length */
    int ellon;  /* output block length */
    unsigned int b_n;    /* current block number */

    if ( (enb=(LITER *)malloc((unsigned int)BLON)) == NULL
     || (elb=(LITER *)malloc((unsigned int)BLON)) == NULL ){
        fprintf( stderr, ediag(
            "pres: no memory\n",
            "pres: ne suficxas memoro\n")  );
        exit_stat = 5;
        return;
    }
    for ( ;; ){
        d = enb;
        b_n = num / BLON; /* the future block number */
        /* fill the input buffer */
        while ( d - enb < BLON && (c=getchar()) != EOF ){
            ++num;
            *d++ = c; /* fill the input buffer */
        }
        /* EOF or buffer is full */
        /* the actual compression */
        if ( (ellon=b_prem( enb, (enlon=d-enb), elb )) > 0 ){
            /* success */
            /* the magic byte */
            c = BMARK | (enlon<BLON ? LASTA : 0) | PREMITA;
            fin = (d=elb) + ellon; /* output buffer to output */
        } else { /* not compressible */
            /* the magic byte */
            c = BMARK | (enlon<BLON ? LASTA : 0);
            fin = (d=enb) + enlon; /* input buffer to output */
        }
        /* output time */
        /* the magic byte */
        elll( c );
        elll( ~c );
        /* the block number */
        elll( b_n&0377 );
        elll( (b_n>>8)&0377 );
        /* possibly block length */
        if ( enlon<BLON ){
            elll( enlon&0377 );
            elll( (enlon>>8)&0377 );
        }
        while ( d < fin ) /* output the block */
            elll( *d++ );
        if ( enlon != BLON ) break; /* it was the last one */
    }
    free( (char *)enb );
    free( (char *)elb );
}

/*
 * Decompression
 */

/* the number of the found block */
static unsigned int it_blk;
/* its length */
static int it_lon;

/* Find the beginning of a block, returns either the header byte or EOF */
static int serch()
{
    register int i, j;
    int msg = 0;

    i = en_l();
    for ( ;; ){
        while ( (i & ~(LASTA|PREMITA)) != BMARK ) i = en_l();
        /* found something */
        if ( i == ((~(j=en_l()))&0377) ){
            it_blk = en_l();
            it_blk += en_l()<<8; /* block number */
            if ( i & LASTA ){
                it_lon = en_l();
                it_lon += en_l()<<8; /* block length */
                if ( it_lon < BLON ) return( i );
            } else {
                it_lon = BLON;
                return( i );
            }
        }
        /* wrong! */
        if ( !msg ){
            ++msg;
            fprintf( stderr, ediag(
                " *** Bad block header, ",
                " *** Maltauxga blokpriskribo, ") );
            fprintf( stderr, ignore ?
                             ediag(
                             "search continued *** ",
                             "sercxo dauxras *** ") :
                             ediag(
                             "aborted ***\n",
                             "fino ***\n") );
        }
        if ( ignore ){
                i = j;
                continue;
        }
        /* !ignore */
        break;
    }
    return( EOF );
}

static int kopir();

/*
 * Decompress a single file
 */
void unpres(void)
{
    register LITER * d, * fin;
    register int i, j, k;
    LITER * elb;
    int (*stato)();
    unsigned int oblk; /* expected block number */
    unsigned int b_blk; /* expected block number after the decompressed one */
    int lon; /* decompressed block length */

    if ( (elb=(LITER *)malloc((unsigned int)BLON)) == NULL ){
        fprintf( stderr, ediag(
            "unpres: no memory\n",
            "unpres: ne suficxas memoro\n")  );
        exit_stat = 5;
        return;
    }
    b_blk = oblk = 0;
    i = serch();
    for ( ;; ){
        if ( i == EOF ){ /* not ignored error */
            fprintf( stderr, ediag(
" *** Erroneous data, finished ***\n",
" *** Datumeraro, fino ***\n") );
            exit_stat = 5;
            break;
        }
        if ( it_blk != oblk ){  /* some blocks are missing */
            fprintf( stderr, ediag(
            " *** Expected block # %u, really # %u, ",
            " *** Anstataux bloko %u venis %u, "),
                              oblk,  it_blk );
            fprintf( stderr, ignore ?
                             ediag(
                             "continued *** ",
                             "dauxrigas plu *** ") :
                             ediag(
                             "aborted ***\n",
                             "fino ***\n") );
            if ( !ignore ){
                exit_stat = 5;
                break;
            }
        }
        stato = (i & PREMITA) ? b_malprem : kopir;
        if ( (*stato)( elb, it_lon ) ){
            fprintf( stderr, ediag(
            " *** Wrong data found, ",
            " *** Maltauxgaj datumoj, ") );
            fprintf( stderr, ignore ?
                             ediag(
                             "continued *** ",
                             "dauxrigas plu *** ") :
                             ediag(
                             "aborted ***\n",
                             "fino ***\n") );
            if ( !ignore ){
                exit_stat = 5;
                break;
            } else {
                i = serch();
                continue;
            }
        }
        oblk = it_blk + 1;
        lon = it_lon;
        if ( lon == BLON ){ /* not the last block */
            if ( (i=serch()) == EOF    /* error */
             || it_blk != oblk )  /* out of order */
                    continue; /* don't trust this block */
        } else { /* looks like the last block */
            if ( ignore ){
                /* not the last, we may need to look further! */
                fprintf( stderr, ediag(
" *** There are some data after END-OF-DATA, ",
" *** Datumoj ekzistas post la indiko de la fino, ") );
                fprintf( stderr, ignore > 1 ?
                                 ediag(
                                 "continued *** ",
                                 "dauxrigas plu *** ") :
                                 ediag(
                                 "ignored ***\n",
                                 "sed ignoritas ***\n")  );
                if ( ignore > 1 ){
                    i = serch();
                    continue; /* ignore the "last block" impression */
                }
            }
        }
        if ( (j=oblk-b_blk-1) != 0 ){
            /* the next block number in order, but here there was a skip */
            if ( fseek( stdout, (long)oblk*(long)BLON, 0 ) < 0 )
                /* can not seek (pipe) */
                if ( j > 0 && j <= 10 ) /* skipped blocks
                                          are not too many */
                    while ( j-- ) /* fill with garbage */
                        for ( k=BLON; k--; ) putchar( 'U' );
        }
        b_blk = oblk;
        for ( fin=(d=elb)+lon; d<fin; putchar( *d++ ) ) ;
        if ( lon < BLON ) break; /* this was the last block */
    }
    free( (char *)elb );
    fflush( stdout );
    if ( ferror(stdout) ){
        perror( "stdout" );
        exit_stat = 5;
    }
}

/* Handling a non-compressed block */
static int kopir(register LITER *b, register unsigned int lon)
{
    register int kod;

    while ( lon-- ){
        kod = en_l();
        *b++ = kod;
    }
    return(0);
}

int main(int argc, char **argv)
{
    register int i;
    register char c;

    if (argc < 2){
	fprintf(stderr, ediag( "\
   Backup stdin onto removable media with data compression,\n\
	   restore to stdout; (c) RL 1990-2015\n",
				"\
       Datumskribo al sxangxeblaj diskoj, kunpremante,\n\
      relego malkunpremante al stdout; (c) RL 1990-2015\n" ) );
Uzo:    fprintf( stderr, ediag( "\
 Usage: flparc [c|x][0-9iivfbk] [arch_file] [blocking] [size_KB]\n",
                                "\
 Uzo: flparc [c|x][0-9iivfbk] [arkivdosiero] [blokumopo] [grandeco_KB]\n" )
	  );
	exit(1);
    }
    argc--, argv++, i = 1;
    while ( c = *(*argv)++ ){ /* process all flags */
	switch( c ){
	    case 'x':
		premado += 2; /* must become 0 */
		break;
	    case 'c':
		premado += 3; /* must become 1 */
		break;
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
		tdefa( c ); /* Figure out the defaults for such an argument */
		break;
	    case 'i':
                ++ignore; /* can become 1 or 2 */
		break;
	    case 'v':
		verb = 1;
		break;
	    case 'f':
		if ( i >= argc ){
		    fprintf( stderr, ediag(
			" archive file name is needed\n",
			" mankas arkiva dosiero\n") );
		    goto Uzo;
		}
		afn = argv[i++];
		tdefa( 'f' ); /* Figure out defaults */
		break;
	    case 'b':
		if ( i >= argc ){
		    fprintf( stderr, ediag(
			" blocking factor is needed\n",
			" mankas blokumopo\n") );
		    goto Uzo;
		}
		bkf = atoi( argv[i++] );
		break;
	    case 'k':
		if ( i >= argc ){
		    fprintf( stderr, ediag(
			" media size is needed\n",
			" mankas grandeco de la disko(j)\n") );
		    goto Uzo;
		}
		dlon = atoi( argv[i++] ); /* here in KB ! */
		break;
	    default:
		fprintf( stderr, ediag(
		    " unknown flag: `%c'\n",
		    " nekonata flago: `%c'\n"), c );
		goto Uzo;
	}
    }
    if ( premado < 0 ){
	fprintf( stderr, ediag(
	    " flag `x' or `c' is needed!\n",
	    " mankas `x' aux `c'!\n") );
	goto Uzo;
    }
    if ( premado > 1 ){
	fprintf( stderr, ediag(
	    " should be only one flag `x' or `c'!\n",
	    " `x' aux `c' devas esti sola!\n") );
	goto Uzo;
    }
    if ( !(afn&&dlon&&bkf) ){ /* Not enough parameters */
	tdefa( '0' ); /* look in the defaults file */
	if ( !(afn&&dlon&&bkf) ){ /* no such file */
	    fprintf( stderr, ediag(
		"\
 Insufficient parameters! (default file `%s' does not exist)\n",
		"\
 Nesuficxaj parametroj! (mankas silentelekta dosiero `%s')\n"), DEFFIL );
	    goto Uzo;
	}
    }
    if ( (fd=open(afn,premado)) < 0 ){
	fprintf( stderr, ediag(
	    " Can't open archive file %s\n",
	    " Ne povas malfermi arkivdosieron %s\n"),
					    afn );
	exit( 2 );
    }
    if ( dlon <= 0 ){
	fprintf( stderr, ediag(
	    " wrong media size: %d\n",
	    " maltauxga grandeco: %d\n"), dlon );
	exit( 3 );
    }
    if ( bkf <= 0  || bkf > 32 ){
	fprintf( stderr, ediag(
	    " wrong blocking: %d\n",
	    " maltauxga blokumopo: %d\n"), bkf );
	exit( 4 );
    }
    dlon *= 2; /* Translate KB to blocks */
    if ( dlon%bkf ){
	fprintf( stderr, ediag(
	    "\
 The size must be a multiple of blocking factor,\n\
 new blocking is set: %d\n",
	    "\
 Grandeco devas esti opa al la blokumopo,\n\
 blokumopo estos: %d\n"), bkf=1 );
/*      exit( 4 ); */
    }
    if ( (b=(unsigned char *)malloc(bimaks=bkf*B_LON)) == 0 ){
	fprintf( stderr, ediag(
	    "\
 insufficient memory for %d blocks buffer\n",
	    "\
 ne suficxas memoro por blokumado je %d blokoj\n"), bkf );
	exit( 5 );
    }
    if ( verb ){
	fprintf( stderr, premado ?
	    ediag(
 " Writing onto %s, size = %d KB, blocking = %d\n",
 " Skribo al %s, grandece %d KB, blokumante je %d blokoj\n") :
	    ediag(
 " Reading of %s, size = %d KB, blocking = %d\n",
 " Lego el %s, grandece %d KB, blokumante je %d blokoj\n"),
	 afn,      dlon/2,               bkf );
    }
    if ( premado )
	rikor();
    else
	leg();
    exit( exit_stat );
}


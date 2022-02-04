/*
 * vi configuration file
 * We try to automatically configure to various compilers and operating
 * systems. Extend the autoconf section as needed.
 */

/*************************** autoconf section ************************/

/* standard unix V (?) */
#ifdef	M_SYSV
# define UNIXV		1
#endif

/* xelos system, University of Ulm */
#ifdef	xelos
# define UNIXV		1
#endif

/* BSD UNIX? */
#ifdef bsd
# define BSD		1
#endif

/* Microsoft C: sorry, Watcom does the same thing */
#ifdef	M_I86
# ifndef M_SYSV
#  define MSDOS		1
#  define MICROSOFT	1
#  define COMPILED_BY	"Microsoft C 5.10"
# endif
#endif

/* Borlands Turbo C */
#ifdef	__TURBOC__
# define MSDOS		1
# define TURBOC		1
# define COMPILED_BY	"Turbo C 2.00"
#endif

/* Tos Mark-Williams */
#ifdef	M68000
# define TOS 1
# define COMPILED_BY	"Mark Williams C"
#endif

/* OS9/68000 */
#ifdef	OSK
# define COMPILED_BY	"Microware C V2.3 Edition 40"
#endif

/*************************** end of autoconf section ************************/

/* All undefined symbols are defined to zero here, to allow for older    */
/* compilers which dont understand #if defined() or #if UNDEFINED_SYMBOL */

/*************************** operating systems *****************************/
 
#ifndef	BSD
# define BSD	0		/* UNIX - Berkeley 4.x */
#endif

#ifndef	UNIXV
# define UNIXV	0		/* UNIX - AT&T SYSV */
#endif

#ifndef	UNIX7
# define UNIX7	0		/* UNIX - version 7 */
#endif

#ifndef	MSDOS
# define MSDOS	0		/* PC		*/
#endif

#ifndef	TOS
# define TOS	0		/* Atari ST	*/
#endif

#ifndef	AMIGA
# define AMIGA	0		/* Commodore Amiga */
#endif

#ifndef OSK
# define OSK	0		/* OS-9 / 68k */
#endif

#ifndef COHERENT
# define COHERENT 0		/* Coherent */
#endif

				/* Minix has no predefines */
#if !BSD && !UNIXV && !UNIX7 && !MSDOS && !TOS && !AMIGA && !OSK && !COHERENT
# define MINIX	1
#else
# define MINIX	0
#endif

				/* generic combination of Unices */
#if UNIXV || UNIX7 || BSD || MINIX || COHERENT
# define ANY_UNIX 1
#else
# define ANY_UNIX 0
#endif

/*************************** compilers **************************************/
 
#ifndef	MICROSOFT
# define MICROSOFT	0
#endif

#ifndef	TURBOC
# define TURBOC		0
#endif

/******************************* Credit ************************************/

#if MSDOS
# define CREDIT "Ported to MS-DOS by Guntram Blohm & Martin Patzel"
#endif

#if TOS
# define CREDIT "Ported to Atari/TOS by Guntram Blohm & Martin Patzel"
#endif

#if OSK
# define CREDIT	"Ported to Microware OS9/68k by Peter Reinig"
#endif

#if COHERENT
# define CREDIT	"Ported to Coherent by Esa Ahola"
#endif

/*************************** functions depending on OS *********************/

/* Only MSDOS, TOS, and OS9 need a special function for reading from the
 * keyboard.  All others just read from file descriptor 0.
 */
#if !MSDOS && !TOS && !OSK
# define ttyread(buf, len)	read(0, buf, (unsigned)len)	/* raw read */
#endif
#if !TOS
# define ttywrite(buf, len)	write(1, buf, (unsigned)(len))	/* raw write */
#endif

/* The strchr() function is an official standard now, so everybody has it
 * except Unix version 7 (which is old) and BSD Unix (which is academic).
 * Those guys use something called index() to do the same thing.
 */
#if BSD || UNIX7 || OSK
# define strchr	index
#endif
extern char *strchr();

/* BSD uses bcopy() instead of memcpy() */
#if BSD
#define memcpy(dest, src, siz)	bcopy(src, dest, siz)
#endif

/* text versa binary mode for read/write */
#if !TOS
#define	tread(fd,buf,n)		read(fd,buf,(unsigned)(n))
#define twrite(fd,buf,n)	write(fd,buf,(unsigned)(n))
#endif

/**************************** Compiler quirks *********************************/

/* the UNIX version 7 and (some) TOS compilers, don't allow "void" */
#if UNIX7 || TOS
# define void int
#endif

/* as far as I know, all compilers except version 7 support unsigned char */
/* NEWFLASH: the Minix-ST compiler has subtle problems with unsigned char */
#if UNIX7 || MINIX
# define UCHAR(c)	((c) & 0xff)
# define uchar		char
#else
# define UCHAR(c)	((unsigned char)(c))
# define uchar		unsigned char
#endif

/* Most compilers could benefit from using the "register" storage class */
#if 1
#define REG	register
#endif

/******************* Names of files and environment vars **********************/

#if ANY_UNIX
# ifndef TMPDIR
#  if MINIX
#   define TMPDIR	"/usr/tmp"	/* Keep elvis' temp files off RAM disk! */
#  else
#   define TMPDIR	"/tmp"		/* directory where temp files live */
#  endif
# endif
# define TMPNAME	"%s/elv%x%04x.%03x" /* temp file */
# define CUTNAME	"%s/elv_%04x%.03x" /* cut buffer's temp file */
# ifndef EXRC
#  define EXRC		".virc"		/* init file in current directory */
# endif
# define SCRATCHOUT	"%s/soXXXXXX"	/* temp file used as input to filter */
# ifndef EXINIT
#  define EXINIT	"VIINIT"
# endif
# ifndef SHELL
#  define SHELL		"/bin/sh"	/* default shell */
# endif
# if COHERENT
#  ifndef REDIRECT
#   define REDIRECT	">"		/* Coherent CC writes errors to stdout */
#  endif
# endif
#endif

#if MSDOS || TOS
/* do not change TMPNAME, CUTNAME and SCRATCH*: they MUST begin with '%s\\'! */
# ifndef TMPDIR
#  define TMPDIR	"C:\\tmp"	/* directory where temp files live */
# endif
# define TMPNAME	"%s\\elv%x%04x.%03x" /* temp file */
# define CUTNAME	"%s\\elv_%04x.%03x" /* cut buffer's temp file */
# if MSDOS
#  if MICROSOFT
#   define CC_COMMAND	"cl -c"		/* C compiler */
#  else /* TURBO_C */
#   define CC_COMMAND	"tc"		/* C compiler */
#  endif
# endif
# define SCRATCHIN	"%s\\siXXXXXX"	/* DOS ONLY - output of filter program */
# define SCRATCHOUT	"%s\\soXXXXXX"	/* temp file used as input to filter */
# define SLASH		'\\'
# ifndef SHELL
#  if TOS
#   define SHELL	"shell.ttp"	/* default shell */
#  else
#   define SHELL	"command.com"	/* default shell */
#  endif
# endif
# define NEEDSYNC	TRUE		/* assume ":se sync" by default */
# define REDIRECT	">"		/* shell's redirection of stderr */
# ifndef MAXMAPS
#  define MAXMAPS	40
# endif
# ifndef EXINIT
#  define EXINIT	"EXINIT"
# endif
#endif

#if OSK
# ifndef TMPDIR
#  define TMPDIR	"/dd/tmp"	   /* directory where temp files live */
# endif
# define TMPNAME	"%s/elv%x%04x%03x"  /* temp file */
# define CUTNAME	"%s/elv_%04x%03x"  /* cut buffer's temp file */
# ifndef CC_COMMAND
#  define CC_COMMAND	"cc -r"		   /* name of the compiler */
# endif
# ifndef EXRC
#  define EXRC		".exrc"		   /* init file in current directory */
# endif
# define SCRATCHOUT	"%s/soXXXXXX"	   /* temp file used as input to filter */
# ifndef SHELL
#  define SHELL		"shell"		   /* default shell */
# endif
# define FILEPERMS	(S_IREAD|S_IWRITE) /* file permissions used for creat() */
# define REDIRECT	">>-"		   /* shell's redirection of stderr */
#endif

#ifndef	TAGS
# define TAGS		"tags"		/* tags file */
#endif

#ifndef TMPNAME
# define TMPNAME	"%s/elv%x%04x.%03x"	/* temp file */
#endif

#ifndef CUTNAME
# define CUTNAME	"%s/elv_%04x.%03x"	/* cut buffer's temp file */
#endif

#ifndef	EXRC
# define EXRC		"elvis.rc"
#endif

#ifndef HMEXRC
# if !MSDOS && !TOS
#  define HMEXRC	EXRC
# endif
#endif

#ifndef	KEYWORDPRG
# define KEYWORDPRG	"ref"
#endif

#ifndef	SCRATCHOUT
# define SCRATCHIN	"%s/SIXXXXXX"
# define SCRATCHOUT	"%s/SOXXXXXX"
#endif

#ifndef ERRLIST
# define ERRLIST	"errlist"
#endif

#ifndef	SLASH
# define SLASH		'/'
#endif

#ifndef SHELL
# define SHELL		"shell"
#endif

#ifndef REG
# define REG
#endif

#ifndef NEEDSYNC
# define NEEDSYNC	FALSE
#endif

#ifndef FILEPERMS
# define FILEPERMS	0666
#endif

#ifndef CC_COMMAND
# define CC_COMMAND	"cc -c"
#endif

#ifndef MAKE_COMMAND
# define MAKE_COMMAND	"make"
#endif

#ifndef REDIRECT
# define REDIRECT	"2>"
#endif

#ifndef MAXMAPS
# define MAXMAPS	20		/* number of :map keys */
#endif
#ifndef MAXDIGS
# define MAXDIGS	30		/* number of :digraph combos */
#endif
#ifndef MAXABBR
# define MAXABBR	20		/* number of :abbr entries */
#endif

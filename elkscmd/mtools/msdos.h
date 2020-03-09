/*
 * msdos common header file
 */

#define MSECSIZ	512			/* sector size */
#define MDIRSIZ	32			/* directory size */
#define CLSTRBUF 1024			/* largest cluster size */
#define MAX_PATH 128

struct directory {
	unsigned char	name[8];	/* file name */
	unsigned char	ext[3];		/* file extent */
	unsigned char	attr;		/* attribute byte */
	unsigned char	reserved[10];	/* ?? */
	unsigned char	time[2];		
	unsigned char	date[2];
	unsigned char	start[2];	/* starting cluster number */
	unsigned char	size[4];	/* size of the file */
};

struct dos_word {
  unsigned char dw_low ;		/* LSB */
  unsigned char dw_high ;		/* MSB */
} ;

struct superblock {
  unsigned char jump[3] ;       /* Jump to boot code */
  unsigned char banner[8];      /* OEM name & version */
  struct dos_word secsiz;       /* Bytes per sector hopefully 512 */
  unsigned char clsiz ;         /* Cluster size in sectors */
  struct dos_word nrsvsect;     /* Number of reserved (boot) sectors */
  unsigned char nfat;           /* Number of FAT tables hopefully 2 */
  struct dos_word dirents;      /* Number of directory slots */
  struct dos_word psect;        /* Total sectors on disk */
  unsigned char descr;          /* Media descriptor=first byte of FAT */
  struct dos_word fatlen;       /* Sectors in FAT */
  struct dos_word nsect;        /* Sectors/track */
  struct dos_word ntrack;       /* tracks/cyl */
  struct dos_word nhs;          /* number of hidden sectors ? */
} ;

union bootblock {
  char dummy[MSECSIZ] ;
  struct superblock sb ;
} ;

#define WORD_VAL(x) (x.dw_low + (x.dw_high <<8))

#define SECSIZ(x)	WORD_VAL(x.secsiz)
#define CLSIZ(x)	(x.clsiz)
#define FSSIZ(x)	WORD_VAL(x.psect)
#define DIRENTS(x)	WORD_VAL(x.dirents)
#define FATLEN(x)	WORD_VAL(x.fatlen)
#define DIRLEN(x)	((DIRENTS(x)*MDIRSIZ-1)/MSECSIZ+1)
#define FATOFF(x)	WORD_VAL(x.nrsvsect)
#define DIROFF(x)	(FATOFF(x)+FATLEN(x)*x.nfat)
#define NCLUST(x)	(FSSIZ(x)-DIROFF(x)-DIRLEN(x)-WORD_VAL(x.nhs))/CLSIZ(x)
#define NSECT(x)	WORD_VAL(x.nsect)
#define NTRACK(x)	WORD_VAL(x.ntrack)
#define NCYL(x)		(FSSIZ(x)/(NTRACK(x)*NSECT(x)))
#define FATCODE(x)	(x.descr)

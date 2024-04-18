/*
 * Initialize a MSDOS diskette.  Read the boot block for disk layout
 * and switch to the proper floppy disk device to match the format
 * of the disk.  Sets a bunch of global variables.  Returns 0 on success,
 * or 1 on failure.
 */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include "devices.h"
#include "msdos.h"

/* #undef DUP_FAT */

extern int fd, dir_len, dir_start, clus_size, fat_len, num_clus;
extern unsigned char *fatbuf;
extern char *mcwd;

/* The available drivers */
extern struct device fd_devices[];

/* The bootblock */
union bootblock bb ;

int
init(mode)
int mode;
{
  int code, buflen, intr();
  void perror(), move(), reset_dir();
  char *getenv(), *fixmcwd(), *malloc(), *dummy;
  struct device *try;
  int ncyl = 0, nsect = 0, ntrack =0 ;
  char *floppy = getenv("FLOPPY");

  /*
   * Let the user set the geometry in the environment
   * It is possible for this to be the only way out of the catch-22
   * wherein the geometry needs to be read from the first block
   * which cannot be read until the geometry is set!
   */

  if ( dummy = getenv("NCYL") )
    ncyl = strtol(dummy,0,0) ;
  if ( dummy = getenv("NTRACK") )
    ntrack = strtol(dummy,0,0) ;
  if ( dummy = getenv("NSECT") )
    nsect = strtol(dummy,0,0) ;
  
  fd = -1 ;

  for ( try = fd_devices ; try->drv_dev ; ++try )
    if ( !floppy ||
	 strncmp(try->drv_dev, floppy, strlen(floppy)) == 0 ) {
      (void) fprintf(stderr,"\nTrying %s ... ",try->drv_dev) ;
      fflush(stderr);
      if ( (fd = open(try->drv_dev, mode|try->drv_mode)) > 0 ) {
	if ( try->drv_ifunc && (*(try->drv_ifunc))(fd,ncyl,ntrack,nsect) ) {
	  close(fd) ;
	  fd = -1;
	  continue ;
	}
	if ( read_boot() ) {
	  close(fd) ;
	  fd = -1;
	  continue ;
	}
	(void) fprintf(stderr,"ok\n") ;
	fflush(stderr) ;
	break ;
      }
      perror("open") ;
    }

  if ( fd < 0 ) {
    (void) fprintf(stderr,"All known devices failed.\nSorry.\n") ;
    exit(1) ;
  }
	
  dir_start = DIROFF(bb.sb) ;
  dir_len = DIRLEN(bb.sb);
  clus_size = CLSIZ(bb.sb);
  fat_len = FATLEN(bb.sb);
  num_clus = NCLUST(bb.sb);

  /* Set the parameters if needed */
  if ( try->drv_ifunc )
    if ( (*(try->drv_ifunc))(fd,NCYL(bb.sb),NTRACK(bb.sb),NSECT(bb.sb)) )
      exit(1) ;
  
  buflen = fat_len * MSECSIZ;
  fatbuf = (unsigned char *) malloc((unsigned int) buflen);

  /* read the FAT sectors */
  move(FATOFF(bb.sb));
  if (read(fd, fatbuf, buflen) != buflen) {
    (void) fprintf(stderr,"Could not read the FAT table\n");
    perror("init: read");
    exit(1) ;
  }

  /* set dir_chain to root directory */
  reset_dir();
  /* get Current Working Directory */
  mcwd = fixmcwd(getenv("MCWD"));
  /* test it out.. */
  if (subdir("")) {
    (void) fprintf(stderr, "Environment variable MCWD needs updating\n");
    exit(1);
  }
  return(0);
}

/*
 * Set the logical sector.  Non brain-dead drivers don't move the
 * head until we ask for data,  so computing relative seeks is overkill.
 */

void
move(sector)
int sector;
{
  if (lseek(fd, (long)sector*MSECSIZ, 0) < 0) {
    perror("move: lseek");
    exit(1);
  }
}

/*
 * Fix MCWD to be a proper directory name.  Always has a leading separator.
 * Never has a trailing separator (unless it is the path itself).
 */

char *
fixmcwd(dirname)
char *dirname;
{
  char *s, *ans, *malloc(), *strcpy(), *strcat();

  if (dirname == NULL)
    return("/");

  ans = malloc((unsigned int) strlen(dirname)+2);
  /* add a leading separator */
  if (*dirname != '/' && *dirname != '\\') {
    strcpy(ans, "/");
    strcat(ans, dirname);
  }
  else
    strcpy(ans, dirname);
  /* translate to upper case */
  for (s = ans; *s; ++s) {
    if (islower(*s))
      *s = toupper(*s);
  }
  /* if separator alone */
  if (strlen(ans) == 1)
    return(ans);
  /* zap the trailing separator */
  s--;
  if (*s == '/' || *s == '\\')
    *s = '\0';
  return(ans);
}

/*
 * Do a graceful exit if the program is interupted.  This will reduce
 * (but not eliminate) the risk of generating a corrupted disk on
 * a user abort.
 */

int
intr()
{
  void writefat();

  writefat();
  close(fd);
  exit(1);
}

/*
 * Write the FAT table to the disk.  Up to now the FAT manipulation has
 * been done in memory.  All errors are fatal.  (Might not be too smart
 * to wait till the end of the program to write the table.  Oh well...)
 */

void
writefat()
{
  int buflen;
  void move();

  move(FATOFF(bb.sb)) ;
  buflen = fat_len * MSECSIZ;
  if (write(fd, (char *) fatbuf, buflen) != buflen) {
    perror("writefat: write");
    exit(1);
  }
#ifdef DUP_FAT
  /* the duplicate FAT table */
  if (write(fd, (char *) fatbuf, buflen) != buflen) {
    perror("writefat: write");
    exit(1);
  }
#endif				/* DUP_FAT */
  return;
}

read_boot()
{
  unsigned char buf[MSECSIZ];
  static unsigned char ans;

  move(0) ;
	
  if (read(fd, &bb, MSECSIZ) != MSECSIZ ) {
    return(1) ;
  }
  
  /* Do not know how to deal with non 512 byte blocks! */
  if ( SECSIZ(bb.sb) != MSECSIZ ) {
    fprintf(stderr,"File system block size of %d bytes is not valid\n",
            SECSIZ(bb.sb));
    exit(1) ;
  }

  return(0) ;
}

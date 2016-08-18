#ifndef sun
	/*
         * FIX ME!
	 * All we need is a convenient way for the user to choose the
         * DOS layout parameters for the FS,  and yet I did not want
         * to hard code tables for the various drives.
         * Something like a /etc/dosformat.dat file would be nice
	 * the user can then use mkdfs <drivename>,  this involes
	 * table parsing routines etc.,  not too hard,  just unpleasant.
	 */
	main() {
		printf("Do not know how to format disks on your system\n");
		exit(1) ;
	}
#else

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <assert.h>
#include "msdos.h"
#include "bootblk.h"

#define VOLLBL 0x8
#define FAT720 0xf9
#define FAT1440 0xf0
static char floppy[] = "/dev/rfd0c" ;
static char disklabel[] = "ANYDSKLABEL" ;

void move(), Write(), usage(), formatit() ;

static char *progname, buf[MSECSIZ] ;

main(argc,argv)
  char **argv;
{
  int c, fd, sec ;
  int fat_len = 3 ;
  int fat = FAT720 ;
  int dir_len = 7 ;
  int hdflag=0, fflag = 0 ;

  progname = argv[0] ;

  while ( (c=getopt(argc,argv,"hf")) != EOF ) {
    switch(c) {
    case 'f' :
      fflag++ ;
      break;
    case 'h' :
      hdflag++ ;
      break;
    case '?':
    default:
      fprintf(stderr,"Unknown option \"%c\"\n",c) ;
      usage() ;
    }
  }

  if (fflag) 
    formatit(hdflag) ;

  /*  Lets initialize the MSDOS FS */

  if ( (fd=open(floppy,O_RDWR)) == -1 ) {
    fprintf(stderr,"%s: open: ",progname) ;
    perror(floppy) ;
    exit(1) ;
  }

  if ( hdflag ) {
    dir_len=14 ;
    fat_len=9 ;
    fat=FAT1440 ;
  }
  
  bzero(buf,sizeof(buf)) ;
  bcopy(hdflag?hdboot:ldboot,buf,MSECSIZ) ;   /* Create the boot block */
  Write(fd,buf,MSECSIZ) ;   /* Dump the boot block */
  bzero(buf,sizeof(buf)) ;
  for ( c=0; c < 2 ; ++c ) {
    buf[0] = fat ;
    buf[1] = 0xff ;
    buf[2] = 0xff ;
    Write(fd,buf,MSECSIZ) ;    /* First block of FAT */
    bzero(buf,3) ;
    for ( sec=fat_len; --sec ; )
      Write(fd,buf,MSECSIZ) ;  /* Rest of FAT */
  }
  strcpy(((struct directory *)buf)->name,disklabel) ;
  ((struct directory *)buf)->attr= VOLLBL ;
  Write(fd,buf,MSECSIZ) ;  /* Root dir */
  bzero(buf,strlen(disklabel)) ;
  ((struct directory *)buf)->attr= 0 ;
  for ( ; --dir_len ; )
    Write(fd,buf,MSECSIZ) ;  /* Root dir */
}

void 
move(fd,sector)
  int fd, sector ;
{
  if ( lseek(fd,sector*MSECSIZ,L_SET) != sector*MSECSIZ) {
    fprintf(stderr,"%s: lseek: ",progname) ;
    perror(floppy) ;
    exit(1) ;
  }
}

void
Write(fd,buf,count)
  int fd,count ;
  char * buf ;
{
  if ( write(fd,buf,count) != count ) {
    fprintf(stderr,"%s: write: ",progname) ;
    perror(floppy) ;
    exit(1) ;
  }
}

void
usage() 
{
  fprintf(stderr,"Usage: %s [-h] [-f]\n",progname) ;
  fprintf(stderr,"\tBuilds an empty DOS filesystem\n") ;
  fprintf(stderr,"\t-h: high density\n\t-f: reformat first\n") ;
  exit(1) ;
}

void
formatit(hdq)
  int hdq;
{
  int pid;
  struct wait w;
  int retval;

  if ( (pid=vfork()) == -1 ) {
    fprintf(stderr,"%s: ",progname) ;
    perror("fork") ;
    exit(1) ;
  }

  if ( !pid ) {
    if ( hdq ) 
      execl("/bin/fdformat","fdformat",floppy,0) ;
    else
      execl("/bin/fdformat","fdformat","-l",floppy,0) ;

    fprintf(stderr,"%s: ",progname) ;
    perror("exec") ;
    exit(1) ;
  }

  while ( (retval=wait4(pid,&w,0,0)) == -1 && errno == EINTR ) ;

  if (retval == -1) {
    fprintf(stderr,"%s: ",progname) ;
    perror("wait4") ;
    fprintf(stderr,"The format operation may have failed\nTry again\n") ;
    exit(1) ;
  }

  if ( WIFSIGNALED(w) ) {
    fprintf(stderr,"%s: ",progname) ;
    psignal(w.w_termsig,"/bin/fdformat") ;
    if (w.w_coredump)
      fprintf(stderr,"Core dumped\n") ;
    exit(1) ;
  }

  assert( WIFEXITED(w) ) ; /* If not signalled,  must be exited
                              we are not tracing stopped processes */

  if ( w.w_retcode ) {
    fprintf(stderr,"%s: /bin/fdformat exited with non-zero exit code %d\n",
      progname,w.w_retcode) ;
    exit(1) ;
  }
}
#endif

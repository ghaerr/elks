#ifndef lint
static char RCSid[]="$Header$" ;
#endif
/*
 *  Device switch for "mtools",  each driver defines a device name,
 *  an initialization function, and extra "open" flags,  typically to allow
 *  NDELAY for opening floppies with non-default sectoring,  which otherwise
 *  yield EIO
 *
 * $Log$
 * Revision 1.3  90/03/16  02:52:20  viktor
 * *** empty log message ***
 * 
 * Revision 1.2  90/03/08  20:29:59  viktor
 * Added unixpc support.
 * 
 */
#include "devices.h"

#ifdef sun
#include <fcntl.h>
#endif

#ifdef unixpc
#include <sys/gdioctl.h>
#include <fcntl.h>
int unixpc_fd_init() ;
#endif

struct device fd_devices[] = {

#ifdef sun
  {"/dev/rfd0c", (int (*)())0, 0 },
#endif
#ifdef unixpc
  {"/dev/rfp020", unixpc_fd_init, O_NDELAY},
#endif
  {"/dev/fd0", (int (*)())0,0 },
  {"/dev/fd1", (int (*)())0,0 },
  {(char*)0, (int (*)())0, 0}
} ;

#ifdef unixpc
unixpc_fd_init(fd,ncyl,ntrack,nsect)
     int fd,ncyl,ntrack,nsect;
{
  struct gdctl gdbuf;

  if ( ! ncyl && ! nsect && ! ntrack ) {
    ncyl = 40;
    /* Default to 1 track, will reset to 2 later if needed */
    ntrack = 1;
    nsect = 8;
  }

  if ( ioctl(fd,GDGETA,&gdbuf) == -1 ) {
    perror("init: ioctl: GDGETA") ;
    dismount(fd) ;
  }

  if (ncyl) gdbuf.params.cyls = ncyl;
  if (ntrack) gdbuf.params.heads = ntrack;
  if (nsect) gdbuf.params.psectrk = nsect;

  gdbuf.params.pseccyl = gdbuf.params.psectrk * gdbuf.params.heads ;
  gdbuf.params.flags = 1;		/* disk type flag */
  gdbuf.params.step = 0;		/* step rate for controller */
  gdbuf.params.sectorsz = 512;		/* sector size */

  if (ioctl(fd, GDSETA, &gdbuf) < 0) {
    perror("init: ioctl: GDSETA");
    dismount(fd);
  }
}

/*
 * Dismount the floppy.  Useful only if one of the above ioctls fail
 * and leaves the drive light turned on.
 */
void
dismount(fd)
{
	struct gdctl gdbuf;
	void exit();

	ioctl(fd, GDDISMNT, &gdbuf);
	exit(1);
}
#endif
